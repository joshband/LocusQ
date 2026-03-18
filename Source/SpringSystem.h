#pragma once

#include "SceneGraph.h"

#include <atomic>
#include <cmath>
#include <cstddef>

//==============================================================================
/**
 * SpringAnchorMode — defines where the spring tether anchors.
 */
enum class SpringAnchorMode : int
{
    RestPose   = 0,   // anchor tracks the emitter's rest position each tick
    FixedPoint = 1    // anchor is a fixed world-space point
};

//==============================================================================
/**
 * SpringSystem — per-emitter spring/pendulum oscillator.
 *
 * Global parameters apply uniformly to all active emitters.
 * Per-emitter phase state is worker-thread-local (no atomics needed).
 *
 * Force model:
 *   d     = pos - anchor
 *   F     = -k * d  -  damp_c * vel
 *   damp_c = phys_spring_damp * 2 * sqrt(k * mass)   (fraction of critical)
 *
 * Phase accumulator:
 *   ω = sqrt(k / mass)  rad/s
 *   phase[i] += ω * dt  (mod 2π)
 *   Used by DSP bridge: spread/gain LFO at spring natural frequency.
 *
 * Acceptance gate (P3):
 *   Natural frequency measured vs ω = sqrt(k/m) within 2%.
 *
 * Threading contract:
 *   - Setters: APVTS / UI thread (atomic write).
 *   - computeForce(), getOscillationPhase(): worker thread only.
 */
class SpringSystem
{
public:
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Setters — APVTS / UI thread

    void setEnabled    (bool b)             { enabled.store    (b, std::memory_order_release); }
    void setStiffness  (float k)            { stiffness.store  (juce::jmax (0.001f, k), std::memory_order_release); }
    void setDamping    (float d)            { damping.store    (juce::jlimit (0.0f, 1.0f, d), std::memory_order_release); }
    void setAnchorMode (SpringAnchorMode m) { anchorMode.store (static_cast<int> (m), std::memory_order_release); }

    void setAnchorPos (const Vec3& p)
    {
        anchorX.store (p.x, std::memory_order_release);
        anchorY.store (p.y, std::memory_order_release);
        anchorZ.store (p.z, std::memory_order_release);
    }

    bool isEnabled() const { return enabled.load (std::memory_order_acquire); }

    //==========================================================================
    // Worker-thread API

    /**
     * Compute spring force for one emitter and advance its phase accumulator.
     *
     * @param emitterIdx  Slot index [0, kMaxEmitters)
     * @param pos         Current emitter world position
     * @param vel         Current emitter velocity
     * @param mass        Emitter mass (affects damping coefficient and ω)
     * @param restPos     Rest position this tick (used when anchor mode = RestPose)
     * @param dt          Tick period in seconds
     * @return            Spring force vector to add to coordinated force
     */
    Vec3 computeForce (int emitterIdx, const Vec3& pos, const Vec3& vel,
                       float mass, const Vec3& restPos, float dt) noexcept
    {
        if (emitterIdx < 0 || emitterIdx >= kMaxEmitters)
            return {};

        const float k    = stiffness.load (std::memory_order_acquire);
        const float damp = damping.load   (std::memory_order_acquire);
        const auto  mode = static_cast<SpringAnchorMode> (
                               anchorMode.load (std::memory_order_acquire));

        // Resolve anchor
        Vec3 anchor;
        if (mode == SpringAnchorMode::FixedPoint)
        {
            anchor.x = anchorX.load (std::memory_order_acquire);
            anchor.y = anchorY.load (std::memory_order_acquire);
            anchor.z = anchorZ.load (std::memory_order_acquire);
        }
        else
        {
            anchor = restPos;
        }

        // Spring restoring force: F = -k * (pos - anchor)
        const float dx = pos.x - anchor.x;
        const float dy = pos.y - anchor.y;
        const float dz = pos.z - anchor.z;

        Vec3 force { -k * dx, -k * dy, -k * dz };

        // Viscous damping: F_damp = -damp_c * vel
        // damp_c = damp * critical_damping = damp * 2 * sqrt(k * mass)
        const float safeMass = std::max (mass, 0.001f);
        const float dampC    = damp * 2.0f * std::sqrt (k * safeMass);
        force.x -= dampC * vel.x;
        force.y -= dampC * vel.y;
        force.z -= dampC * vel.z;

        // Clamp force magnitude to prevent instability
        const float mag = std::sqrt (force.x * force.x
                                   + force.y * force.y
                                   + force.z * force.z);
        if (mag > kMaxForceMagnitude)
        {
            const float scale = kMaxForceMagnitude / mag;
            force.x *= scale;
            force.y *= scale;
            force.z *= scale;
        }

        // Advance phase: ω = sqrt(k / mass), phase += ω * dt (mod 2π)
        const float omega = std::sqrt (k / safeMass);
        auto& ph = phase[static_cast<std::size_t> (emitterIdx)];
        ph += omega * dt;
        if (ph >= kTwoPi) ph -= kTwoPi;

        return force;
    }

    /** Returns current oscillation phase [0, 2π) for DSP spread/gain mapping. */
    float getOscillationPhase (int emitterIdx) const noexcept
    {
        if (emitterIdx < 0 || emitterIdx >= kMaxEmitters)
            return 0.0f;
        return phase[static_cast<std::size_t> (emitterIdx)];
    }

    void resetPhase (int emitterIdx) noexcept
    {
        if (emitterIdx < 0 || emitterIdx >= kMaxEmitters)
            return;
        phase[static_cast<std::size_t> (emitterIdx)] = 0.0f;
    }

private:
    static constexpr float kMaxForceMagnitude = 1000.0f;
    static constexpr float kTwoPi             = 6.283185307f;

    std::atomic<bool>  enabled    { false };
    std::atomic<float> stiffness  { 10.0f };   // N/m
    std::atomic<float> damping    { 0.3f  };   // fraction of critical damping [0..1]
    std::atomic<int>   anchorMode { static_cast<int> (SpringAnchorMode::RestPose) };
    std::atomic<float> anchorX    { 0.0f  };
    std::atomic<float> anchorY    { 0.0f  };
    std::atomic<float> anchorZ    { 0.0f  };

    // Per-emitter oscillation phase (worker-thread-local, no atomics needed)
    float phase[kMaxEmitters] {};
};
