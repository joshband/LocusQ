#pragma once

#include "SceneGraph.h"

#include <array>
#include <atomic>
#include <cmath>

//==============================================================================
/**
 * CollisionSystem — O(n²) hard-body inter-emitter impulse resolution (P6).
 *
 * Usage per worker tick:
 *   1. Call setEmitterState() for every slot (active or not) to build snapshot.
 *   2. Call resolve() — O(n²) overlap detection, modifies snapshot velocities.
 *   3. Call getImpulseDelta() per active slot to retrieve the velocity change.
 *   4. Caller pushes impulse via PhysicsEngine::applyCollisionImpulse().
 *   5. Call getCollisionEnergy() per slot for the DSP bridge gainTransient.
 *
 * RT safety contract:
 *   Snapshot / impulse / energy arrays are worker-thread-only POD — no atomics.
 *   Global enable, per-emitter radii, gainScale, and decayRateHz are atomics
 *   written by the APVTS / UI thread and read by the worker thread.
 *   No heap allocation, no locks, no blocking calls.
 *
 * Spec impulse formula (physics-simulation-spec.md §Hard-Body Inter-Emitter Collision):
 *   normal     = normalize(pos_a - pos_b)           // from b toward a
 *   rel_vel    = v_a - v_b
 *   j          = -(1 + e) * dot(rel_vel, normal) / (1/mass_a + 1/mass_b)
 *   v_a       += j / mass_a * normal
 *   v_b       -= j / mass_b * normal
 *   (only applied when dot(rel_vel, normal) < 0 — emitters approaching)
 *
 * Finite-safe contract:
 *   |j| clamped to kMaxImpulse before application.
 *   collisionEnergy per emitter clamped to [0..1] before DSP publish.
 *   Overlap tolerance: pairs with distSq < 1e-12 skipped to prevent singularity.
 *
 * DSP hook:
 *   getCollisionEnergy(i) → [0..1] → gainTransient in PhysicsDSPBridge.
 *   Scale applied via gainScale atomic before normalize.
 *
 * Acceptance gates (P6):
 *   - Determinism: same initial conditions → same impulses across 3 runs.
 *   - O(n²) CPU headroom at 64 emitters logged.
 *   - Finite-safe: no energy blow-up under rapid repeated collisions.
 */
class CollisionSystem
{
public:
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Setters — APVTS / UI thread

    void setEnabled      (bool  e) { enabled.store (e, std::memory_order_release); }

    void setCollisionRadius (int idx, float r)
    {
        if (idx >= 0 && idx < kMaxEmitters)
            radii[static_cast<std::size_t> (idx)].store (
                juce::jmax (0.01f, r), std::memory_order_release);
    }

    /** Scale factor applied to raw |impulse| / kMaxImpulse before DSP publish. */
    void setGainScale   (float s) { gainScale.store (juce::jlimit (0.0f, 10.0f, s), std::memory_order_release); }

    /** Decay rate forwarded to the DSP bridge config (informational; applied at P7 wiring). */
    void setDecayRateHz (float hz) { decayRateHz.store (juce::jmax (0.1f, hz), std::memory_order_release); }

    void setElasticity  (float e)  { elasticity.store (juce::jlimit (0.0f, 1.0f, e), std::memory_order_release); }

    bool isEnabled() const noexcept { return enabled.load (std::memory_order_acquire); }

    //==========================================================================
    // Worker-thread API

    /**
     * Step 1 — populate snapshot for one emitter.
     * Call for every slot (active or not) before resolve().
     * Resets per-emitter impulse and energy accumulators.
     */
    void setEmitterState (int         idx,
                          const Vec3& pos,
                          const Vec3& vel,
                          float       mass,
                          bool        active) noexcept
    {
        if (idx < 0 || idx >= kMaxEmitters)
            return;

        const std::size_t si = static_cast<std::size_t> (idx);
        auto& s  = snap[si];
        s.pos    = pos;
        s.vel    = vel;
        s.mass   = std::max (mass, 0.001f);
        s.radius = radii[si].load (std::memory_order_acquire);
        s.active = active;
        impulseDelta   [si] = {};
        collisionEnergy[si] = 0.0f;
    }

    /**
     * Step 2 — O(n²) impulse resolution.
     *
     * Iterates all active pairs, detects overlaps, and applies the spec formula.
     * Snapshot velocities are modified in-place so multiple-collision scenarios
     * accumulate correctly within the same tick.
     *
     * Uses elasticity set via setElasticity(); defaults to 0.7.
     */
    void resolve() noexcept
    {
        if (! enabled.load (std::memory_order_acquire))
            return;

        const float e = elasticity.load (std::memory_order_acquire);

        for (int a = 0; a < kMaxEmitters - 1; ++a)
        {
            const std::size_t sa_idx = static_cast<std::size_t> (a);
            auto& sa = snap[sa_idx];
            if (! sa.active)
                continue;

            for (int b = a + 1; b < kMaxEmitters; ++b)
            {
                const std::size_t sb_idx = static_cast<std::size_t> (b);
                auto& sb = snap[sb_idx];
                if (! sb.active)
                    continue;

                const float dx = sa.pos.x - sb.pos.x;
                const float dy = sa.pos.y - sb.pos.y;
                const float dz = sa.pos.z - sb.pos.z;
                const float distSq = dx * dx + dy * dy + dz * dz;
                const float minDist = sa.radius + sb.radius;

                // Not overlapping, or degenerate (avoid singularity)
                if (distSq >= minDist * minDist || distSq < 1.0e-12f)
                    continue;

                const float dist    = std::sqrt (distSq);
                const float invDist = 1.0f / dist;

                // Contact normal: from b toward a
                const float nx = dx * invDist;
                const float ny = dy * invDist;
                const float nz = dz * invDist;

                // Relative velocity projected onto normal
                const float rvx    = sa.vel.x - sb.vel.x;
                const float rvy    = sa.vel.y - sb.vel.y;
                const float rvz    = sa.vel.z - sb.vel.z;
                const float rvDotN = rvx * nx + rvy * ny + rvz * nz;

                // Only resolve separating or approaching pairs (rvDotN < 0 = approaching)
                if (rvDotN >= 0.0f)
                    continue;

                const float invMassA = 1.0f / sa.mass;
                const float invMassB = 1.0f / sb.mass;

                // Impulse scalar j (spec formula)
                const float j = -(1.0f + e) * rvDotN / (invMassA + invMassB);

                // Clamp impulse to prevent energy blow-up (finite-safe contract)
                const float jClamped = juce::jlimit (-kMaxImpulse, kMaxImpulse, j);

                const float jA =  jClamped * invMassA;   // vel change for a
                const float jB =  jClamped * invMassB;   // vel change for b (opposite)

                // Update snapshot velocities (accumulates across multi-body tick)
                sa.vel.x += jA * nx;  sa.vel.y += jA * ny;  sa.vel.z += jA * nz;
                sb.vel.x -= jB * nx;  sb.vel.y -= jB * ny;  sb.vel.z -= jB * nz;

                // Accumulate impulse deltas (to push back to engines)
                impulseDelta[sa_idx].x += jA * nx;
                impulseDelta[sa_idx].y += jA * ny;
                impulseDelta[sa_idx].z += jA * nz;
                impulseDelta[sb_idx].x -= jB * nx;
                impulseDelta[sb_idx].y -= jB * ny;
                impulseDelta[sb_idx].z -= jB * nz;

                // Collision energy: |j| normalized to [0..1] via kMaxImpulse
                const float scale   = gainScale.load (std::memory_order_acquire);
                const float rawEnergy = std::abs (jClamped) / kMaxImpulse;
                const float scaledEnergy = juce::jlimit (0.0f, 1.0f, rawEnergy * scale);

                collisionEnergy[sa_idx] = juce::jmin (1.0f, collisionEnergy[sa_idx] + scaledEnergy);
                collisionEnergy[sb_idx] = juce::jmin (1.0f, collisionEnergy[sb_idx] + scaledEnergy);
            }
        }
    }

    /**
     * Step 3 — accumulated velocity impulse for emitter idx after resolve().
     * Push this to PhysicsEngine::applyCollisionImpulse() on the matching engine.
     */
    Vec3 getImpulseDelta (int idx) const noexcept
    {
        if (idx < 0 || idx >= kMaxEmitters) return {};
        return impulseDelta[static_cast<std::size_t> (idx)];
    }

    /**
     * DSP value: normalized collision energy [0..1] for gainTransient publish.
     * Clamped before return; safe to pass directly to PhysicsDSPBridge::publish().
     */
    float getCollisionEnergy (int idx) const noexcept
    {
        if (idx < 0 || idx >= kMaxEmitters) return 0.0f;
        return collisionEnergy[static_cast<std::size_t> (idx)];
    }

    /** Informational: configured decay rate (Hz). Consumer wires to bridge config in P7. */
    float getDecayRateHz() const noexcept { return decayRateHz.load (std::memory_order_acquire); }

private:
    // Maximum impulse scalar magnitude — bounds energy under degenerate overlap
    static constexpr float kMaxImpulse = 500.0f;

    struct EmitterSnapshot
    {
        Vec3  pos    {};
        Vec3  vel    {};
        float mass   = 1.0f;
        float radius = 0.3f;
        bool  active = false;
    };

    // Worker-thread-only state — no atomics needed
    EmitterSnapshot snap          [kMaxEmitters] {};
    Vec3            impulseDelta  [kMaxEmitters] {};
    float           collisionEnergy[kMaxEmitters] {};

    // APVTS / UI thread writes; worker thread reads
    std::atomic<bool>  enabled     { false };
    std::atomic<float> gainScale   { 1.0f  };
    std::atomic<float> decayRateHz { 8.0f  };
    std::atomic<float> elasticity  { 0.7f  };

    std::atomic<float> radii[kMaxEmitters];

    // Initialise all radius slots to the default (0.3 m)
    struct RadiiInit
    {
        RadiiInit (std::atomic<float> (&arr)[kMaxEmitters])
        {
            for (auto& v : arr)
                v.store (0.3f, std::memory_order_relaxed);
        }
    };
    RadiiInit radiiInitGuard { radii };
};
