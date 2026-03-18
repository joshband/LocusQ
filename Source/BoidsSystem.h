#pragma once

#include "SceneGraph.h"

#include <array>
#include <atomic>
#include <cmath>
#include <limits>

//==============================================================================
/**
 * BoidsSystem — Reynolds flocking (separation / alignment / cohesion) for up to
 * 4 independent groups, 64 emitters max.
 *
 * Design:
 *   Each emitter belongs to at most one group (-1 = no group).
 *   The three classic rules are computed from a position/velocity snapshot
 *   taken at the start of each worker tick — all emitters see a consistent
 *   scene state and forces are deterministic regardless of slot ordering.
 *
 * Worker-thread usage per tick:
 *   1. Call notifyEmitterState() for every slot (active or not).
 *   2. Call finalizeSnapshot()  — computes group centroids / averages.
 *   3. Call computeSteeringForce() per active emitter to get force to inject.
 *   4. Call getFlockDensity() / consumeBreakupEvent() for DSP bridge values.
 *
 * Speed capping:
 *   A velocity-damping braking force is included in computeSteeringForce()
 *   when the snapshot velocity exceeds the group max speed.
 *
 * RT safety contract:
 *   No heap allocation, no locks, no blocking.  All per-group parameters are
 *   atomics written by the APVTS / UI thread and read by the worker thread.
 *   All snapshot / metric arrays are plain POD — worker thread only.
 *
 * DSP hooks (P5):
 *   getFlockDensity(i)       → [0..1] neighbor count / max, maps to spread
 *   consumeBreakupEvent(i)   → true once on cohesion-breakup; gain-dip wiring
 *                              deferred to P7 (event API ready for consumer)
 *
 * Acceptance gate (P5):
 *   - CPU headroom at 64 emitters with 4 active groups within worker budget.
 *   - Determinism: same initial conditions → same centroids at tick 100
 *     across 3 independent runs.
 */
class BoidsSystem
{
public:
    static constexpr int kMaxGroups   = 4;
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Setters — APVTS / UI thread

    void setGroupEnabled  (int g, bool  b) { if (validG (g)) groups[g].enabled.store    (b,                          std::memory_order_release); }
    void setSepWeight     (int g, float w) { if (validG (g)) groups[g].sepWeight.store   (juce::jlimit (0.0f, 1.0f, w), std::memory_order_release); }
    void setAlignWeight   (int g, float w) { if (validG (g)) groups[g].alignWeight.store (juce::jlimit (0.0f, 1.0f, w), std::memory_order_release); }
    void setCohWeight     (int g, float w) { if (validG (g)) groups[g].cohWeight.store   (juce::jlimit (0.0f, 1.0f, w), std::memory_order_release); }
    void setSepRadius     (int g, float r) { if (validG (g)) groups[g].sepRadius.store   (juce::jmax   (0.01f, r),       std::memory_order_release); }
    void setAlignRadius   (int g, float r) { if (validG (g)) groups[g].alignRadius.store (juce::jmax   (0.01f, r),       std::memory_order_release); }
    void setCohRadius     (int g, float r) { if (validG (g)) groups[g].cohRadius.store   (juce::jmax   (0.01f, r),       std::memory_order_release); }
    void setMaxSpeed      (int g, float s) { if (validG (g)) groups[g].maxSpeed.store    (juce::jmax   (0.01f, s),       std::memory_order_release); }

    /** Assign an emitter to a group (-1 = no group / removed from flocking). */
    void setEmitterGroup (int emitterIdx, int groupIdx)
    {
        if (validE (emitterIdx))
            emitterGroup[static_cast<std::size_t> (emitterIdx)].store (groupIdx, std::memory_order_release);
    }

    bool isGroupEnabled (int g) const
    {
        return validG (g) && groups[g].enabled.load (std::memory_order_acquire);
    }

    //==========================================================================
    // Worker-thread API

    /**
     * Step 1 — Snapshot one emitter's current state.
     * Call for every slot (active or not) before finalizeSnapshot().
     *
     * Reads the group assignment atomic so the snapshot is self-consistent
     * even if the UI thread changes group membership mid-tick.
     */
    void notifyEmitterState (int emitterIdx,
                             const Vec3& pos,
                             const Vec3& vel,
                             bool        active) noexcept
    {
        if (! validE (emitterIdx))
            return;

        auto& s  = snap[static_cast<std::size_t> (emitterIdx)];
        s.pos    = pos;
        s.vel    = vel;
        s.group  = emitterGroup[static_cast<std::size_t> (emitterIdx)].load (std::memory_order_acquire);
        s.active = active;
    }

    /**
     * Step 2 — Finalise snapshot: compute per-group centroid, average velocity,
     * and member count. Call once after all notifyEmitterState() calls.
     */
    void finalizeSnapshot() noexcept
    {
        // Reset group accumulators
        for (int g = 0; g < kMaxGroups; ++g)
        {
            groupCentroid[g] = {};
            groupVelAvg[g]   = {};
            groupSize[g]     = 0;
        }

        // Accumulate active members
        for (int i = 0; i < kMaxEmitters; ++i)
        {
            if (! snap[i].active) continue;

            const int g = snap[i].group;
            if (g < 0 || g >= kMaxGroups) continue;
            if (! groups[g].enabled.load (std::memory_order_acquire)) continue;

            groupCentroid[g].x += snap[i].pos.x;
            groupCentroid[g].y += snap[i].pos.y;
            groupCentroid[g].z += snap[i].pos.z;
            groupVelAvg[g].x   += snap[i].vel.x;
            groupVelAvg[g].y   += snap[i].vel.y;
            groupVelAvg[g].z   += snap[i].vel.z;
            groupSize[g]++;
        }

        // Normalise to means
        for (int g = 0; g < kMaxGroups; ++g)
        {
            if (groupSize[g] > 0)
            {
                const float inv = 1.0f / static_cast<float> (groupSize[g]);
                groupCentroid[g].x *= inv;  groupCentroid[g].y *= inv;  groupCentroid[g].z *= inv;
                groupVelAvg[g].x   *= inv;  groupVelAvg[g].y   *= inv;  groupVelAvg[g].z   *= inv;
            }
        }
    }

    /**
     * Step 3 — Compute boids steering force for one emitter.
     * Must be called after finalizeSnapshot(). Returns a force vector (N, where
     * F = mass * acceleration) ready to be added to the coordinated-force sum.
     *
     * Also updates DSP metrics: flockDensity[i] and breakupEvent[i].
     *
     * @param emitterIdx  Slot index [0, kMaxEmitters)
     * @param mass        Emitter mass (scales returned force)
     * @return            Net boids steering force, or zero if not in an active group
     */
    Vec3 computeSteeringForce (int emitterIdx, float mass) noexcept
    {
        if (! validE (emitterIdx) || ! snap[emitterIdx].active)
            return {};

        const int g = snap[emitterIdx].group;
        if (g < 0 || g >= kMaxGroups)
            return {};

        if (! groups[g].enabled.load (std::memory_order_acquire))
            return {};

        const float sepW    = groups[g].sepWeight.load   (std::memory_order_acquire);
        const float alignW  = groups[g].alignWeight.load (std::memory_order_acquire);
        const float cohW    = groups[g].cohWeight.load   (std::memory_order_acquire);
        const float sepR    = groups[g].sepRadius.load   (std::memory_order_acquire);
        const float alignR  = groups[g].alignRadius.load (std::memory_order_acquire);
        const float cohR    = groups[g].cohRadius.load   (std::memory_order_acquire);
        const float maxSpd  = groups[g].maxSpeed.load    (std::memory_order_acquire);

        const Vec3& myPos = snap[emitterIdx].pos;
        const Vec3& myVel = snap[emitterIdx].vel;

        Vec3  sepAccum   {};
        Vec3  alignVelSum{};
        Vec3  cohPosSum  {};
        int   alignCount = 0;
        int   cohCount   = 0;

        for (int j = 0; j < kMaxEmitters; ++j)
        {
            if (j == emitterIdx)    continue;
            if (! snap[j].active)   continue;
            if (snap[j].group != g) continue;

            const float dx   = snap[j].pos.x - myPos.x;
            const float dy   = snap[j].pos.y - myPos.y;
            const float dz   = snap[j].pos.z - myPos.z;
            const float dSq  = dx * dx + dy * dy + dz * dz;
            const float dist = std::sqrt (dSq);

            // -- Separation --
            if (sepW > 1.0e-6f && dist < sepR && dist > 1.0e-6f)
            {
                // Repulsion strength grows as 1/dist — strongest when very close
                const float invDist = 1.0f / dist;
                const float mag     = sepW * invDist;
                // Steer away: force is opposite to direction toward neighbor
                sepAccum.x -= dx * invDist * mag;
                sepAccum.y -= dy * invDist * mag;
                sepAccum.z -= dz * invDist * mag;
            }

            // -- Alignment --
            if (alignW > 1.0e-6f && dist < alignR)
            {
                alignVelSum.x += snap[j].vel.x;
                alignVelSum.y += snap[j].vel.y;
                alignVelSum.z += snap[j].vel.z;
                ++alignCount;
            }

            // -- Cohesion --
            if (cohW > 1.0e-6f && dist < cohR)
            {
                cohPosSum.x += snap[j].pos.x;
                cohPosSum.y += snap[j].pos.y;
                cohPosSum.z += snap[j].pos.z;
                ++cohCount;
            }
        }

        Vec3 steer = sepAccum;

        // Alignment: steer toward local average velocity
        if (alignCount > 0)
        {
            const float inv = 1.0f / static_cast<float> (alignCount);
            steer.x += (alignVelSum.x * inv - myVel.x) * alignW;
            steer.y += (alignVelSum.y * inv - myVel.y) * alignW;
            steer.z += (alignVelSum.z * inv - myVel.z) * alignW;
        }

        // Cohesion: steer toward local centroid
        if (cohCount > 0)
        {
            const float inv  = 1.0f / static_cast<float> (cohCount);
            const Vec3 toCenter
            {
                cohPosSum.x * inv - myPos.x,
                cohPosSum.y * inv - myPos.y,
                cohPosSum.z * inv - myPos.z
            };
            const float len = std::sqrt (toCenter.x * toCenter.x
                                       + toCenter.y * toCenter.y
                                       + toCenter.z * toCenter.z);
            if (len > 1.0e-6f)
            {
                const float w = cohW / len;
                steer.x += toCenter.x * w;
                steer.y += toCenter.y * w;
                steer.z += toCenter.z * w;
            }
        }

        // Speed capping via velocity-damping braking force
        // When snapshot velocity exceeds maxSpeed, add counter-force to brake
        const float curSpeed = std::sqrt (myVel.x * myVel.x
                                        + myVel.y * myVel.y
                                        + myVel.z * myVel.z);
        if (maxSpd > 0.0f && curSpeed > maxSpd && curSpeed > 1.0e-6f)
        {
            const float excessFraction = (curSpeed - maxSpd) / curSpeed;
            steer.x -= myVel.x * excessFraction * kSpeedBrakeFactor;
            steer.y -= myVel.y * excessFraction * kSpeedBrakeFactor;
            steer.z -= myVel.z * excessFraction * kSpeedBrakeFactor;
        }

        // Clamp steer magnitude to prevent runaway before mass scaling
        const float mag = std::sqrt (steer.x * steer.x
                                   + steer.y * steer.y
                                   + steer.z * steer.z);
        if (mag > kMaxSteerAccel)
        {
            const float s = kMaxSteerAccel / mag;
            steer.x *= s;  steer.y *= s;  steer.z *= s;
        }

        // Convert acceleration → force
        const float safeMass = std::max (mass, 0.001f);
        steer.x *= safeMass;
        steer.y *= safeMass;
        steer.z *= safeMass;

        // ---- Update DSP metrics ----

        // Flock density: max(alignCount, cohCount) / (groupSize - 1)
        const int gs = groupSize[g];
        const float maxNeighbors = static_cast<float> (std::max (gs - 1, 1));
        flockDensity[static_cast<std::size_t> (emitterIdx)] =
            juce::jlimit (0.0f, 1.0f,
                          static_cast<float> (std::max (alignCount, cohCount)) / maxNeighbors);

        // Cohesion-breakup event: emitter was in cohesion radius last tick but not now
        const bool inCohesion = (cohCount > 0);
        if (! inCohesion && prevInCohesion[static_cast<std::size_t> (emitterIdx)])
            breakupEvent[static_cast<std::size_t> (emitterIdx)] = true;
        prevInCohesion[static_cast<std::size_t> (emitterIdx)] = inCohesion;

        return steer;
    }

    //==========================================================================
    // DSP helpers — worker thread, call after computeSteeringForce()

    /** Normalised flock density [0..1]: neighbor count / max possible neighbors. */
    float getFlockDensity (int emitterIdx) const noexcept
    {
        if (! validE (emitterIdx)) return 0.0f;
        return flockDensity[static_cast<std::size_t> (emitterIdx)];
    }

    /**
     * Cohesion-breakup event — true once per separation from cohesion radius.
     * Caller (PhysicsWorker) consumes this and wires to DSP gain-dip in P7.
     */
    bool consumeBreakupEvent (int emitterIdx) noexcept
    {
        if (! validE (emitterIdx)) return false;
        const std::size_t idx = static_cast<std::size_t> (emitterIdx);
        if (breakupEvent[idx])
        {
            breakupEvent[idx] = false;
            return true;
        }
        return false;
    }

    /** Group centroid for visualization (call after finalizeSnapshot()). */
    Vec3 getGroupCentroid (int groupIdx) const noexcept
    {
        if (groupIdx < 0 || groupIdx >= kMaxGroups) return {};
        return groupCentroid[groupIdx];
    }

    /** True if emitter is assigned to an enabled group. */
    bool isInActiveGroup (int emitterIdx) const noexcept
    {
        if (! validE (emitterIdx)) return false;
        const int g = emitterGroup[static_cast<std::size_t> (emitterIdx)].load (std::memory_order_acquire);
        if (g < 0 || g >= kMaxGroups) return false;
        return groups[g].enabled.load (std::memory_order_acquire);
    }

private:
    // Maximum per-axis steering acceleration before mass scaling (m/s²)
    static constexpr float kMaxSteerAccel    = 20.0f;
    // Speed brake stiffness multiplier applied to excess-velocity fraction
    static constexpr float kSpeedBrakeFactor =  5.0f;

    static bool validG (int g) noexcept { return g >= 0 && g < kMaxGroups;   }
    static bool validE (int e) noexcept { return e >= 0 && e < kMaxEmitters; }

    //==========================================================================
    // Per-group parameters — APVTS / UI thread writes, worker thread reads
    struct GroupParams
    {
        std::atomic<bool>  enabled     { false };
        std::atomic<float> sepWeight   { 1.0f  };
        std::atomic<float> alignWeight { 0.5f  };
        std::atomic<float> cohWeight   { 0.5f  };
        std::atomic<float> sepRadius   { 1.5f  };   // m
        std::atomic<float> alignRadius { 3.0f  };   // m
        std::atomic<float> cohRadius   { 5.0f  };   // m
        std::atomic<float> maxSpeed    { 5.0f  };   // m/s
    };

    GroupParams groups[kMaxGroups] {};

    // Per-emitter group assignment (-1 = no group)
    // APVTS / UI thread writes; worker thread reads via notifyEmitterState()
    std::atomic<int> emitterGroup[kMaxEmitters];

    // Initialise all emitter group slots to -1 (no group)
    struct GroupAssignInit
    {
        GroupAssignInit (std::atomic<int> (&arr)[kMaxEmitters])
        {
            for (auto& v : arr)
                v.store (-1, std::memory_order_relaxed);
        }
    };
    GroupAssignInit groupAssignInitGuard { emitterGroup };

    //==========================================================================
    // Snapshot state — worker thread only, no atomics needed

    struct EmitterSnapshot
    {
        Vec3 pos   {};
        Vec3 vel   {};
        int  group = -1;
        bool active = false;
    };

    EmitterSnapshot snap[kMaxEmitters] {};

    // Group-level aggregates recomputed each tick in finalizeSnapshot()
    Vec3 groupCentroid[kMaxGroups] {};
    Vec3 groupVelAvg  [kMaxGroups] {};
    int  groupSize    [kMaxGroups] {};

    // Per-emitter DSP metrics written by computeSteeringForce()
    float flockDensity  [kMaxEmitters] {};
    bool  breakupEvent  [kMaxEmitters] {};
    bool  prevInCohesion[kMaxEmitters] {};
};
