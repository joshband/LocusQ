#pragma once

// HeadPoseInterpolator — BL-045 Slice B
//
// Header-only, allocation-free, audio-thread safe.
// Provides:
//   - Quaternion slerp interpolation between the two most recent snapshots.
//   - Bounded angular-velocity prediction (max extrapolation min(50ms, π/4/‖angV‖)).
//   - Sensor-switch crossfade (50ms blend on earbud location change).
//
// Usage (processBlock):
//   if (const auto* pose = bridge.currentPose())
//   {
//       const float nowMs = static_cast<float>(juce::Time::getMillisecondCounterHiRes());
//       headPoseInterpolator.ingest(*pose, nowMs);
//       const auto interpolated = headPoseInterpolator.interpolatedAt(nowMs);
//       ... apply interpolated to renderer ...
//   }

#include "HeadTrackingBridge.h"

#include <cmath>
#include <cstdint>

class HeadPoseInterpolator
{
public:
    HeadPoseInterpolator() = default;

    // Ingest a new snapshot. Safe to call on the audio thread.
    // Duplicate snapshots (same seq) are silently ignored.
    // On earbud sensor-location change: captures the current interpolation as the
    // blend-out pose and starts a 50ms crossfade to the new sensor's orientation.
    void ingest (const HeadTrackingPoseSnapshot& snap, float nowMs) noexcept
    {
        // Skip if same snapshot (bridge has not published a new one since last call)
        if (hasPrev && snap.seq == currSnapshot.seq)
            return;

        const auto newLoc = static_cast<std::uint8_t> (snap.sensorLocationFlags & 0x3u);

        if (hasPrev && newLoc != prevSensorLocation)
        {
            // Capture interpolated pose just before the sensor switch
            blendOutSnapshot = interpolatedAt (nowMs);
            sensorSwitchBlendRemaining = kSensorSwitchBlendMs;
        }

        prevSnapshot = currSnapshot;
        currSnapshot = snap;
        hasPrev     = true;
        prevSensorLocation = newLoc;
    }

    // Return the best-estimate pose at nowMs.
    // Applies slerp, optional angular-velocity prediction, and crossfade if active.
    // Const + allocation-free; safe on the audio thread.
    HeadTrackingPoseSnapshot interpolatedAt (float nowMs) const noexcept
    {
        // Track block dt for blend countdown
        const float blockDt = (lastInterpolatedMs > 0.0f) ? (nowMs - lastInterpolatedMs) : 0.0f;
        lastInterpolatedMs = nowMs;

        if (!hasPrev)
            return currSnapshot; // identity quaternion until first snapshot

        const float prevTs = static_cast<float> (prevSnapshot.timestampMs);
        const float currTs = static_cast<float> (currSnapshot.timestampMs);

        // ── Slerp interpolation ────────────────────────────────────────────
        HeadTrackingPoseSnapshot result = currSnapshot;

        if (currTs > prevTs)
        {
            const float t = clamp01 ((nowMs - prevTs) / (currTs - prevTs));
            result = slerpSnapshots (prevSnapshot, currSnapshot, t);
        }
        // else (currTs <= prevTs): timestamps equal or regressed → use currSnapshot as-is

        // ── Bounded angular-velocity prediction ───────────────────────────
        const bool hasRotRate = (currSnapshot.sensorLocationFlags & 0x4u) != 0;

        if (hasRotRate && nowMs > currTs + 1.0f)
        {
            const float wx = currSnapshot.angVx;
            const float wy = currSnapshot.angVy;
            const float wz = currSnapshot.angVz;

            const float angMag = std::sqrt (wx * wx + wy * wy + wz * wz);

            // Cap prediction so rotation angle never exceeds π/4 (≈45°)
            const float maxHorizonSec = std::min (
                kMaxPredictionMs / 1000.0f,
                kPiOver4 / std::max (angMag, 1.0e-6f));

            const float dt = std::min ((nowMs - currTs) / 1000.0f, maxHorizonSec);

            // Small-angle quaternion extrapolation: q_pred = q_curr ⊗ q_delta
            // q_delta ≈ normalize(1, wx·dt/2, wy·dt/2, wz·dt/2)
            const float hx = wx * dt * 0.5f;
            const float hy = wy * dt * 0.5f;
            const float hz = wz * dt * 0.5f;

            const float qw = result.qw, qx = result.qx, qy = result.qy, qz = result.qz;

            float pw = qw         - qx * hx - qy * hy - qz * hz;
            float px = qx + qw * hx          + qy * hz - qz * hy;
            float py = qy + qw * hy - qx * hz          + qz * hx;
            float pz = qz + qw * hz + qx * hy - qy * hx;

            const float normSq = pw * pw + px * px + py * py + pz * pz;
            if (normSq > 1.0e-12f)
            {
                const float inv = 1.0f / std::sqrt (normSq);
                result.qw = pw * inv;
                result.qx = px * inv;
                result.qy = py * inv;
                result.qz = pz * inv;
            }
        }

        // ── Sensor-switch crossfade ────────────────────────────────────────
        if (sensorSwitchBlendRemaining > 0.0f)
        {
            // alpha: 0 = fully blendOut (old sensor), 1 = fully result (new sensor)
            const float alpha = 1.0f - (sensorSwitchBlendRemaining / kSensorSwitchBlendMs);
            result = slerpSnapshots (blendOutSnapshot, result, clamp01 (alpha));
            sensorSwitchBlendRemaining = std::max (0.0f, sensorSwitchBlendRemaining - blockDt);
        }

        return result;
    }

private:
    static constexpr float kMaxPredictionMs    = 50.0f;
    static constexpr float kSensorSwitchBlendMs = 50.0f;
    static constexpr float kPiOver4            = 0.78539816339744830f; // π/4

    static float clamp01 (float v) noexcept
    {
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    // Shortest-path slerp between two snapshots.
    // Returns `to` with the interpolated quaternion; all other fields from `to`.
    static HeadTrackingPoseSnapshot slerpSnapshots (
        const HeadTrackingPoseSnapshot& from,
        const HeadTrackingPoseSnapshot& to,
        float t) noexcept
    {
        HeadTrackingPoseSnapshot result = to;

        float ax = from.qx, ay = from.qy, az = from.qz, aw = from.qw;
        float bx = to.qx,   by = to.qy,   bz = to.qz,   bw = to.qw;

        float dot = ax * bx + ay * by + az * bz + aw * bw;

        // Ensure shortest arc
        if (dot < 0.0f)
        {
            bx = -bx; by = -by; bz = -bz; bw = -bw;
            dot = -dot;
        }

        // Clamp for numerical safety before acos
        dot = dot > 1.0f ? 1.0f : dot;

        if (dot > 0.9995f)
        {
            // Nearly identical quaternions: use nlerp to avoid division by ~zero
            const float s  = 1.0f - t;
            float rx = s * ax + t * bx;
            float ry = s * ay + t * by;
            float rz = s * az + t * bz;
            float rw = s * aw + t * bw;
            const float normSq = rx * rx + ry * ry + rz * rz + rw * rw;
            if (normSq > 1.0e-12f)
            {
                const float inv = 1.0f / std::sqrt (normSq);
                result.qx = rx * inv;
                result.qy = ry * inv;
                result.qz = rz * inv;
                result.qw = rw * inv;
            }
        }
        else
        {
            const float theta0    = std::acos (dot);
            const float sinTheta0 = std::sin (theta0);
            const float s0        = std::sin ((1.0f - t) * theta0) / sinTheta0;
            const float s1        = std::sin (t * theta0)          / sinTheta0;
            result.qx = s0 * ax + s1 * bx;
            result.qy = s0 * ay + s1 * by;
            result.qz = s0 * az + s1 * bz;
            result.qw = s0 * aw + s1 * bw;
        }

        return result;
    }

    HeadTrackingPoseSnapshot prevSnapshot  {};
    HeadTrackingPoseSnapshot currSnapshot  {};
    HeadTrackingPoseSnapshot blendOutSnapshot {};

    bool         hasPrev           = false;
    std::uint8_t prevSensorLocation = 0;

    mutable float sensorSwitchBlendRemaining = 0.0f; // ms remaining in crossfade
    mutable float lastInterpolatedMs         = 0.0f; // for blockDt computation
};
