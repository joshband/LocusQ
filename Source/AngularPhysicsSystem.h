#pragma once

#include "SceneGraph.h"

#include <atomic>
#include <cmath>
#include <limits>

//==============================================================================
/**
 * AngularPhysicsSystem — per-emitter angular velocity integration driving
 * directivityAim.
 *
 * Internal state only (not promoted to EmitterSlot):
 *   angVelocity[i] — current angular velocity as axis-angle vector (rad/s)
 *   aim[i]         — current aim direction (unit Vec3)
 *
 * PhysicsWorker writes the resulting directivityAim Vec3 to WorkerEmitterState
 * each tick via the existing atomic swap — no new EmitterSlot fields required.
 *
 * Rotation model (first-order Rodrigues, valid for |ω|·dt < 0.3 rad):
 *   newAim = normalize(aim + cross(ω · dt, aim))
 *
 * NaN contract: aim is always a finite unit vector. Any NaN from adversarial
 * impulse sequences is caught by the post-normalize isfinite guard, which
 * falls back to the default aim {0, 0, -1}.
 *
 * Acceptance gate (P4):
 *   - Full yaw/pitch/roll sweep produces expected cardioid modulation ±5%.
 *   - directivityAim never produces NaN under adversarial impulse sequence.
 *
 * Threading contract:
 *   - Setters: APVTS / UI thread (atomic write).
 *   - consumeThrow(), consumeReset(), getImpulse(), update(): worker thread only.
 */
class AngularPhysicsSystem
{
public:
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Setters — APVTS / UI thread

    void setEnabled              (bool b)        { enabled.store           (b, std::memory_order_release); }
    void setAngularDrag          (float d)        { angDrag.store           (juce::jlimit (0.0f, 1.0f, d), std::memory_order_release); }
    void setImpulseX             (float v)        { impX.store              (v, std::memory_order_release); }
    void setImpulseY             (float v)        { impY.store              (v, std::memory_order_release); }
    void setImpulseZ             (float v)        { impZ.store              (v, std::memory_order_release); }
    void setAttractorTorque      (float s)        { torqueStrength.store    (juce::jmax (0.0f, s), std::memory_order_release); }

    /** One-shot trigger: apply current impulse to all active emitters next tick. */
    void requestThrow()  { throwPending.store (true, std::memory_order_release); }

    /** One-shot trigger: zero angular velocity and restore default aim next tick. */
    void requestReset()  { resetPending.store (true, std::memory_order_release); }

    bool isEnabled() const { return enabled.load (std::memory_order_acquire); }

    //==========================================================================
    // Worker-thread API — consume flags once per tick before the emitter loop

    /** Atomically consume the throw-pending flag. Returns true if a throw should fire this tick. */
    bool consumeThrow() noexcept
    {
        return throwPending.exchange (false, std::memory_order_acq_rel);
    }

    /** Atomically consume the reset-pending flag. Returns true if a reset should fire this tick. */
    bool consumeReset() noexcept
    {
        return resetPending.exchange (false, std::memory_order_acq_rel);
    }

    /** Read the current impulse vector (used once consumeThrow() returns true). */
    Vec3 getImpulse() const noexcept
    {
        return { impX.load (std::memory_order_acquire),
                 impY.load (std::memory_order_acquire),
                 impZ.load (std::memory_order_acquire) };
    }

    //==========================================================================
    /**
     * Per-emitter angular update — worker thread only.
     *
     * @param emitterIdx     Slot index [0, kMaxEmitters)
     * @param toAttractorDir Normalised direction from emitter to nearest active
     *                       attractor (zero vector if none active)
     * @param dt             Tick period in seconds
     * @param doThrow        Consumed throw flag for this tick
     * @param throwImpulse   Impulse vector to add to angular velocity (if doThrow)
     * @param doReset        Consumed reset flag for this tick
     * @return               New aim direction (finite unit vector, never NaN)
     */
    Vec3 update (int emitterIdx,
                 const Vec3& toAttractorDir,
                 float dt,
                 bool doThrow,
                 const Vec3& throwImpulse,
                 bool doReset) noexcept
    {
        if (emitterIdx < 0 || emitterIdx >= kMaxEmitters)
            return kDefaultAim;

        auto& vel = angVelocity[static_cast<std::size_t> (emitterIdx)];
        auto& a   = aim[static_cast<std::size_t> (emitterIdx)];

        // One-shot reset: zero velocity, restore default aim
        if (doReset)
        {
            vel = {};
            a   = kDefaultAim;
            return a;
        }

        // One-shot throw: add impulse to angular velocity
        if (doThrow)
        {
            vel.x += throwImpulse.x;
            vel.y += throwImpulse.y;
            vel.z += throwImpulse.z;
        }

        // Attractor torque: cross(aim, toAttractorDir) × torqueStrength
        // Pulls aim direction toward the nearest attractor
        const float ts = torqueStrength.load (std::memory_order_acquire);
        if (ts > 1.0e-6f)
        {
            const float tLen = std::sqrt (toAttractorDir.x * toAttractorDir.x
                                        + toAttractorDir.y * toAttractorDir.y
                                        + toAttractorDir.z * toAttractorDir.z);
            if (tLen > 1.0e-6f)
            {
                const float inv = 1.0f / tLen;
                const Vec3 tDir { toAttractorDir.x * inv,
                                  toAttractorDir.y * inv,
                                  toAttractorDir.z * inv };

                // Torque axis = cross(aim, tDir), magnitude proportional to sin of angle
                vel.x += (a.y * tDir.z - a.z * tDir.y) * ts * dt;
                vel.y += (a.z * tDir.x - a.x * tDir.z) * ts * dt;
                vel.z += (a.x * tDir.y - a.y * tDir.x) * ts * dt;
            }
        }

        // Clamp angular speed to prevent runaway
        const float speed = std::sqrt (vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
        if (speed > kMaxAngSpeed)
        {
            const float s = kMaxAngSpeed / speed;
            vel.x *= s;
            vel.y *= s;
            vel.z *= s;
        }

        // Apply angular drag: rate-independent exponential decay
        // kAngDragScale=5 maps [0..1] drag → [none..fast] at any tick rate
        const float drag        = angDrag.load (std::memory_order_acquire);
        const float decayFactor = std::exp (-drag * dt * kAngDragScale);
        vel.x *= decayFactor;
        vel.y *= decayFactor;
        vel.z *= decayFactor;

        // Integrate aim via first-order Rodrigues approximation:
        //   newAim ≈ aim + cross(ω·dt, aim)    (valid for |ω|·dt ≪ 1 rad)
        const float wx = vel.x * dt;
        const float wy = vel.y * dt;
        const float wz = vel.z * dt;

        // cross(w, a) = w × a
        Vec3 newAim
        {
            a.x + (wy * a.z - wz * a.y),
            a.y + (wz * a.x - wx * a.z),
            a.z + (wx * a.y - wy * a.x)
        };

        // Normalise with NaN guard
        const float len = std::sqrt (newAim.x * newAim.x
                                   + newAim.y * newAim.y
                                   + newAim.z * newAim.z);

        if (len > 1.0e-6f
            && std::isfinite (newAim.x)
            && std::isfinite (newAim.y)
            && std::isfinite (newAim.z))
        {
            const float invLen = 1.0f / len;
            newAim.x *= invLen;
            newAim.y *= invLen;
            newAim.z *= invLen;
        }
        else
        {
            // Belt-and-suspenders: adversarial input or near-zero length — fall back
            newAim = kDefaultAim;
            vel    = {};
        }

        a = newAim;
        return a;
    }

private:
    static constexpr float kMaxAngSpeed   = 50.0f;   // rad/s — prevents velocity runaway
    static constexpr float kAngDragScale  = 5.0f;    // maps [0..1] drag to sensible per-second decay
    static const     Vec3  kDefaultAim;               // {0, 0, -1} — forward direction

    //==========================================================================
    // Global params — APVTS / UI thread writes, worker thread reads
    std::atomic<bool>  enabled       { false };
    std::atomic<float> angDrag       { 0.3f  };
    std::atomic<float> impX          { 0.0f  };
    std::atomic<float> impY          { 0.0f  };
    std::atomic<float> impZ          { 0.0f  };
    std::atomic<bool>  throwPending  { false };
    std::atomic<bool>  resetPending  { false };
    std::atomic<float> torqueStrength{ 5.0f  };

    //==========================================================================
    // Per-emitter state — worker-thread-local, no atomics needed
    Vec3 angVelocity[kMaxEmitters] {};     // angular velocity (rad/s, axis-angle)
    Vec3 aim[kMaxEmitters];                // current aim direction (unit vector)

    // Initialise aim[] to {0,0,-1} — matching EmitterData default
    struct AimInit
    {
        AimInit (Vec3 (&arr)[kMaxEmitters])
        {
            for (auto& v : arr)
                v = { 0.0f, 0.0f, -1.0f };
        }
    };
    AimInit aimInitGuard { aim };
};

inline const Vec3 AngularPhysicsSystem::kDefaultAim { 0.0f, 0.0f, -1.0f };
