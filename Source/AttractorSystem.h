#pragma once

#include "SceneGraph.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

//==============================================================================
/**
 * AttractorFalloff — how attractor force scales with distance.
 */
enum class AttractorFalloff : int
{
    InverseR  = 0,   // F ∝ 1/r
    InverseR2 = 1,   // F ∝ 1/r²  (default — realistic gravitational/electrostatic)
    Constant  = 2    // F = strength (uniform field regardless of distance)
};

//==============================================================================
/**
 * AttractorSource — thread-safe accessors for one attractor/repulsor source.
 *
 * All setters are called from the UI/APVTS thread (non-audio).
 * computeForce() and helpers are called from the PhysicsWorker thread.
 */
struct AttractorSource
{
    // Setters — APVTS/UI thread
    void setPosition   (const Vec3& p) { px.store(p.x, std::memory_order_release); py.store(p.y, std::memory_order_release); pz.store(p.z, std::memory_order_release); }
    void setStrength   (float s)       { strength.store(s, std::memory_order_release); }
    void setFalloff    (AttractorFalloff f) { falloff.store(static_cast<int>(f), std::memory_order_release); }
    void setRadius     (float r)       { radius.store(juce::jmax(0.01f, r), std::memory_order_release); }
    void setOrbitStabilize(bool b)     { orbitStabilize.store(b, std::memory_order_release); }
    void setActive     (bool b)        { active.store(b, std::memory_order_release); }

    // Accessors
    bool isActive()    const { return active.load(std::memory_order_acquire); }

    Vec3 getPosition() const
    {
        return { px.load(std::memory_order_acquire),
                 py.load(std::memory_order_acquire),
                 pz.load(std::memory_order_acquire) };
    }

    float getRadius()   const { return radius.load(std::memory_order_acquire); }
    float getStrength() const { return strength.load(std::memory_order_acquire); }

    //==========================================================================
    /**
     * Compute the force this source exerts on an emitter.
     *
     * @param emitterPos  Current emitter world position
     * @param emitterVel  Current emitter velocity (needed for orbit stabilization)
     * @param mass        Emitter mass (used to scale orbital spring stiffness)
     * @param targetRadius  Per-source target orbital radius (tracked by caller)
     * @return            Force vector to add to emitter acceleration input
     */
    Vec3 computeForce(const Vec3& emitterPos, const Vec3& emitterVel,
                      float mass, float targetRadius) const
    {
        if (! active.load(std::memory_order_acquire))
            return {};

        const Vec3 attPos = getPosition();
        const float s     = strength.load(std::memory_order_acquire);
        const float r     = radius.load(std::memory_order_acquire);
        const auto  fall  = static_cast<AttractorFalloff>(falloff.load(std::memory_order_acquire));
        const bool  orbit = orbitStabilize.load(std::memory_order_acquire);

        const float dx = attPos.x - emitterPos.x;
        const float dy = attPos.y - emitterPos.y;
        const float dz = attPos.z - emitterPos.z;
        const float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

        // Outside influence radius — no force
        if (dist > r) return {};

        // Prevent singularity at origin (emitter on top of attractor)
        const float safeDist = std::max(dist, 0.001f);

        // Unit vector from emitter toward attractor
        const float invDist = 1.0f / safeDist;
        const Vec3 toAtt { dx * invDist, dy * invDist, dz * invDist };

        // Falloff model
        float falloffFactor = 1.0f;
        switch (fall)
        {
            case AttractorFalloff::InverseR:  falloffFactor = 1.0f / safeDist;              break;
            case AttractorFalloff::InverseR2: falloffFactor = 1.0f / (safeDist * safeDist); break;
            case AttractorFalloff::Constant:  falloffFactor = 1.0f;                         break;
        }

        // Primary radial force (positive s = attract = toward attractor)
        const float fMag = s * falloffFactor;
        const float clampedMag = juce::jlimit(-1000.0f, 1000.0f, fMag);
        Vec3 force { toAtt.x * clampedMag, toAtt.y * clampedMag, toAtt.z * clampedMag };

        // Orbital stabilization — spring force toward target orbital radius
        if (orbit && targetRadius > 0.0f)
        {
            const float radialError = dist - targetRadius;

            // Stiffness scales with mass so the correction is independent of mass
            const float kOrbit = mass * 5.0f;
            const float corrMag = juce::jlimit(-500.0f, 500.0f, -kOrbit * radialError);

            // Direction: toward attractor if inside target orbit, away if outside
            force.x += toAtt.x * corrMag;
            force.y += toAtt.y * corrMag;
            force.z += toAtt.z * corrMag;

            // Tangential correction: inject force perpendicular to radial direction
            // in the plane of motion to compensate for tangential drag decay.
            // F_tangential = k_damp * tangential_velocity (cancels drag on orbit).
            const float radialVelDot = emitterVel.x * toAtt.x
                                     + emitterVel.y * toAtt.y
                                     + emitterVel.z * toAtt.z;
            const Vec3 tangentialVel
            {
                emitterVel.x - radialVelDot * toAtt.x,
                emitterVel.y - radialVelDot * toAtt.y,
                emitterVel.z - radialVelDot * toAtt.z
            };

            // Compensate a fraction of tangential drag (kTangential tuned to ~drag/2)
            const float kTangential = 0.4f;
            force.x += kTangential * tangentialVel.x;
            force.y += kTangential * tangentialVel.y;
            force.z += kTangential * tangentialVel.z;
        }

        return force;
    }

    /**
     * Normalized proximity value for DSP mapping.
     * Returns 1.0 when emitter is at the attractor center, 0.0 at or beyond radius.
     * Uses normalization: proximity = 1 - dist/radius  (spec: "0=at attractor" is distance-based).
     */
    float getProximity(const Vec3& emitterPos) const
    {
        if (! active.load(std::memory_order_acquire))
            return 0.0f;

        const Vec3 attPos = getPosition();
        const float r     = radius.load(std::memory_order_acquire);
        const float dx    = attPos.x - emitterPos.x;
        const float dy    = attPos.y - emitterPos.y;
        const float dz    = attPos.z - emitterPos.z;
        const float dist  = std::sqrt(dx*dx + dy*dy + dz*dz);

        if (dist >= r) return 0.0f;
        return juce::jlimit(0.0f, 1.0f, 1.0f - dist / r);
    }

    /**
     * Returns true if the emitter has transitioned across the attractor radius boundary
     * between prevPos and newPos (inside↔outside crossing).
     */
    bool detectCrossing(const Vec3& prevPos, const Vec3& newPos) const
    {
        if (! active.load(std::memory_order_acquire))
            return false;

        const Vec3  attPos = getPosition();
        const float r      = radius.load(std::memory_order_acquire);

        auto distSq = [&](const Vec3& p) {
            const float dx = attPos.x - p.x;
            const float dy = attPos.y - p.y;
            const float dz = attPos.z - p.z;
            return dx*dx + dy*dy + dz*dz;
        };

        const float rSq = r * r;
        const bool prevInside = distSq(prevPos) < rSq;
        const bool nowInside  = distSq(newPos)  < rSq;
        return prevInside != nowInside;
    }

private:
    std::atomic<float> px        { 0.0f };
    std::atomic<float> py        { 0.0f };
    std::atomic<float> pz        { 0.0f };
    std::atomic<float> strength  { 0.0f };
    std::atomic<int>   falloff   { static_cast<int>(AttractorFalloff::InverseR2) };
    std::atomic<float> radius    { 5.0f };
    std::atomic<bool>  orbitStabilize { false };
    std::atomic<bool>  active    { false };
};

//==============================================================================
/**
 * AttractorSystem — manages up to kMaxSources attractor/repulsor sources for
 * the shared PhysicsWorker scene tick.
 *
 * Orbital target radii are tracked per (source × emitter) pair so that when
 * orbit stabilization is enabled the emitter holds its radius at activation.
 *
 * Usage:
 *   1. Set source parameters via setters (APVTS thread).
 *   2. PhysicsWorker calls tick() each simulation step — updates internal state.
 *   3. getCoordinatedForce() returns the net force for one emitter (to inject
 *      into PhysicsEngine via setCoordinatedForce()).
 *   4. getMaxProximity() returns closest-source proximity for DSP bridge.
 *   5. consumeCrossingEvent() returns and clears the per-emitter crossing flag.
 */
class AttractorSystem
{
public:
    static constexpr int kMaxSources  = 4;
    static constexpr int kMaxEmitters = 64;

    AttractorSource& source(int idx)
    {
        return sources[static_cast<std::size_t>(juce::jlimit(0, kMaxSources - 1, idx))];
    }

    const AttractorSource& source(int idx) const
    {
        return sources[static_cast<std::size_t>(juce::jlimit(0, kMaxSources - 1, idx))];
    }

    //==========================================================================
    /**
     * Compute net attractor force for one emitter. Called from worker thread.
     *
     * @param emitterIdx   Slot index [0, kMaxEmitters)
     * @param pos          Emitter world position
     * @param vel          Emitter velocity
     * @param mass         Emitter mass
     */
    Vec3 computeNetForce(int emitterIdx, const Vec3& pos, const Vec3& vel, float mass)
    {
        Vec3 net {};
        for (int s = 0; s < kMaxSources; ++s)
        {
            if (! sources[s].isActive()) continue;

            // Update orbital target radius on first encounter or when orbit
            // stabilize is newly enabled.
            auto& tr = targetRadius[static_cast<std::size_t>(s)]
                                    [static_cast<std::size_t>(emitterIdx)];
            if (tr < 0.0f)
            {
                // Initialize target radius to current distance from attractor
                const Vec3 attPos = sources[s].getPosition();
                const float dx = attPos.x - pos.x;
                const float dy = attPos.y - pos.y;
                const float dz = attPos.z - pos.z;
                tr = std::sqrt(dx*dx + dy*dy + dz*dz);
            }

            const Vec3 f = sources[s].computeForce(pos, vel, mass, tr);
            net.x += f.x;
            net.y += f.y;
            net.z += f.z;
        }

        // Clamp net force magnitude to prevent instability
        const float mag = std::sqrt(net.x*net.x + net.y*net.y + net.z*net.z);
        if (mag > kMaxForceMagnitude)
        {
            const float scale = kMaxForceMagnitude / mag;
            net.x *= scale; net.y *= scale; net.z *= scale;
        }

        return net;
    }

    /**
     * Returns the maximum normalized proximity of all active sources for this emitter.
     * Value in [0..1]: 1 = emitter is at an attractor center.
     */
    float getMaxProximity(const Vec3& pos) const
    {
        float maxProx = 0.0f;
        for (int s = 0; s < kMaxSources; ++s)
            maxProx = std::max(maxProx, sources[s].getProximity(pos));
        return maxProx;
    }

    /**
     * Check and consume a crossing event for (emitterIdx, prevPos → newPos).
     * Returns true if any source boundary was crossed. Callable from worker thread.
     */
    bool didCross(int /*emitterIdx*/, const Vec3& prevPos, const Vec3& newPos) const
    {
        for (int s = 0; s < kMaxSources; ++s)
            if (sources[s].detectCrossing(prevPos, newPos))
                return true;
        return false;
    }

    /** Reset orbital target radii for one emitter (call when emitter is reset). */
    void resetTargetRadii(int emitterIdx)
    {
        if (emitterIdx < 0 || emitterIdx >= kMaxEmitters) return;
        for (int s = 0; s < kMaxSources; ++s)
            targetRadius[s][static_cast<std::size_t>(emitterIdx)] = -1.0f;
    }

private:
    static constexpr float kMaxForceMagnitude = 2000.0f;

    std::array<AttractorSource, kMaxSources> sources {};

    // targetRadius[sourceIdx][emitterIdx] — initialized to -1 (unset)
    float targetRadius[kMaxSources][kMaxEmitters];

    struct Init
    {
        Init(float (&arr)[kMaxSources][kMaxEmitters])
        {
            for (auto& row : arr)
                for (auto& v : row)
                    v = -1.0f;
        }
    };

    Init initGuard { targetRadius };
};
