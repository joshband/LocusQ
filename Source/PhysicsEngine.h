#pragma once

#include "SceneGraph.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cmath>

//==============================================================================
/**
 * PhysicsBody - runtime physical properties for one emitter.
 */
struct PhysicsBody
{
    float mass = 1.0f;
    float drag = 0.5f;
    float elasticity = 0.7f;
    float friction = 0.3f;
    bool enabled = false;
};

//==============================================================================
/**
 * PhysicsEngine - dedicated simulation thread for one emitter instance.
 *
 * The worker thread advances position/velocity at a configurable tick rate.
 * Audio thread interaction is lock-free via atomics and double-buffered state.
 */
class PhysicsEngine : private juce::Thread
{
public:
    struct PhysicsState
    {
        Vec3 position { 0.0f, 0.0f, 0.0f };
        Vec3 velocity { 0.0f, 0.0f, 0.0f };
        Vec3 force { 0.0f, 0.0f, 0.0f };
        std::uint8_t collisionMask = 0; // bit0=X wall, bit1=Y floor/ceiling, bit2=Z wall
        float collisionEnergy = 0.0f;
        bool initialized = false;
    };

    PhysicsEngine()
        : juce::Thread ("LocusQPhysicsEngine")
    {
    }

    ~PhysicsEngine() override
    {
        shutdown();
    }

    //==========================================================================
    void prepare (double sampleRate)
    {
        currentSampleRate.store (sampleRate, std::memory_order_relaxed);
        startThreadIfNeeded();
        notify();
    }

    void shutdown()
    {
        if (! isThreadRunning())
            return;

        signalThreadShouldExit();
        notify();
        stopThread (1000);
    }

    //==========================================================================
    /** Wall boundary behaviour. */
    enum class BoundaryMode : int { Hard = 0, Soft = 1, Passthrough = 2 };

    void setBoundaryMode    (BoundaryMode m) { boundaryMode.store(static_cast<int>(m), std::memory_order_release); }
    void setSoftBoundaryDepth(float d)       { softBoundaryDepth.store(juce::jmax(0.01f, d), std::memory_order_release); }

    /**
     * Additional force injected by PhysicsWorker for coordinated features
     * (attractors, boids, spring, turbulence). Accumulated with interactionForce
     * inside step() — does not replace it.
     */
    void setCoordinatedForce(const Vec3& force)
    {
        coordinatedForceX.store(force.x, std::memory_order_release);
        coordinatedForceY.store(force.y, std::memory_order_release);
        coordinatedForceZ.store(force.z, std::memory_order_release);
    }

    /** When true, this engine runs in standalone per-emitter mode.
     *  When false, position/velocity authority belongs to PhysicsWorker
     *  (coordinated features: boids, attractors, inter-emitter collisions).
     *  Defaults to true; set to false before the PhysicsWorker activates
     *  the corresponding slot. */
    void setStandaloneMode (bool standalone)           { standaloneMode.store (standalone, std::memory_order_release); }
    bool isStandaloneMode() const                      { return standaloneMode.load (std::memory_order_acquire); }

    void setPhysicsEnabled (bool enabled)              { bodyEnabled.store (enabled, std::memory_order_release); notify(); }
    void setPaused (bool paused)                       { simulationPaused.store (paused, std::memory_order_release); notify(); }
    void setWallCollisionEnabled (bool enabled)        { wallCollisionEnabled.store (enabled, std::memory_order_release); }
    void setUpdateRateIndex (int index)                { updateRateIndex.store (juce::jlimit (0, 3, index), std::memory_order_release); notify(); }

    void setMass (float value)                         { mass.store (juce::jmax (0.01f, value), std::memory_order_release); }
    void setDrag (float value)                         { drag.store (juce::jlimit (0.0f, 10.0f, value), std::memory_order_release); }
    void setElasticity (float value)                   { elasticity.store (juce::jlimit (0.0f, 1.0f, value), std::memory_order_release); }
    void setFriction (float value)                     { friction.store (juce::jlimit (0.0f, 1.0f, value), std::memory_order_release); }
    void setGravity (float magnitude, int direction)   { gravityMagnitude.store (magnitude, std::memory_order_release); gravityDirection.store (direction, std::memory_order_release); }
    void setInteractionForce (const Vec3& force)
    {
        interactionForceX.store (force.x, std::memory_order_release);
        interactionForceY.store (force.y, std::memory_order_release);
        interactionForceZ.store (force.z, std::memory_order_release);
    }

    void setRestPosition (const Vec3& position)
    {
        restX.store (position.x, std::memory_order_release);
        restY.store (position.y, std::memory_order_release);
        restZ.store (position.z, std::memory_order_release);
    }

    void setRoomDimensions (const Vec3& dimensions)
    {
        roomWidth.store (juce::jmax (0.5f, dimensions.x), std::memory_order_release);
        roomDepth.store (juce::jmax (0.5f, dimensions.y), std::memory_order_release);
        roomHeight.store (juce::jmax (0.5f, dimensions.z), std::memory_order_release);
    }

    void requestThrow (const Vec3& initialVelocity)
    {
        throwVelocityX.store (initialVelocity.x, std::memory_order_release);
        throwVelocityY.store (initialVelocity.y, std::memory_order_release);
        throwVelocityZ.store (initialVelocity.z, std::memory_order_release);
        throwSequence.fetch_add (1, std::memory_order_acq_rel);
        notify();
    }

    void requestReset()
    {
        resetSequence.fetch_add (1, std::memory_order_acq_rel);
        notify();
    }

    /**
     * Apply a one-shot velocity impulse from inter-emitter collision resolution.
     * Called by PhysicsWorker after CollisionSystem::resolve() (P6).
     * Consumed once in the next step() tick, analogous to requestThrow().
     */
    void applyCollisionImpulse (const Vec3& impulse)
    {
        collisionImpulseX.store (impulse.x, std::memory_order_release);
        collisionImpulseY.store (impulse.y, std::memory_order_release);
        collisionImpulseZ.store (impulse.z, std::memory_order_release);
        collisionImpulseSeq.fetch_add (1, std::memory_order_acq_rel);
    }

    float getMass()        const { return mass.load (std::memory_order_acquire); }
    float getDrag()        const { return drag.load (std::memory_order_acquire); }
    float getElasticity()  const { return elasticity.load (std::memory_order_acquire); }
    float getGravityMagnitude() const { return gravityMagnitude.load (std::memory_order_acquire); }
    int getGravityDirection() const { return gravityDirection.load (std::memory_order_acquire); }

    PhysicsState getState() const
    {
        const int idx = readIndex.load (std::memory_order_acquire);
        return stateBuffers[static_cast<size_t> (idx)];
    }

private:
    //==========================================================================
    void startThreadIfNeeded()
    {
        if (isThreadRunning())
            return;

        startThread();
    }

    void run() override
    {
        auto nextTickMs = juce::Time::getMillisecondCounterHiRes();
        auto lastRateIndex = updateRateIndex.load (std::memory_order_acquire);

        while (! threadShouldExit())
        {
            const auto rateIndex = updateRateIndex.load (std::memory_order_acquire);
            const auto nowMs = juce::Time::getMillisecondCounterHiRes();

            if (rateIndex != lastRateIndex)
            {
                nextTickMs = nowMs;
                lastRateIndex = rateIndex;
            }

            const float rateHz = getUpdateRateHz (rateIndex);
            const double periodMs = 1000.0 / static_cast<double> (rateHz);
            const auto timeUntilNextTickMs = nextTickMs - nowMs;

            if (timeUntilNextTickMs > 0.25)
            {
                wait (juce::jlimit (1, 50, static_cast<int> (std::ceil (timeUntilNextTickMs))));
                continue;
            }

            step (static_cast<float> (periodMs * 0.001));
            nextTickMs += periodMs;

            if (nextTickMs < nowMs)
                nextTickMs = nowMs + periodMs;
        }
    }

    void step (float dt)
    {
        auto state = stateBuffers[static_cast<size_t> (readIndex.load (std::memory_order_acquire))];

        const Vec3 restPosition
        {
            restX.load (std::memory_order_acquire),
            restY.load (std::memory_order_acquire),
            restZ.load (std::memory_order_acquire)
        };

        if (! restPositionInitialized)
        {
            previousRestPosition = restPosition;
            restPositionInitialized = true;
        }

        if (! state.initialized)
        {
            state.position = restPosition;
            state.velocity = {};
            state.force = {};
            state.collisionMask = 0;
            state.collisionEnergy = 0.0f;
            state.initialized = true;
            previousRestPosition = restPosition;
        }

        const auto latestResetSeq = resetSequence.load (std::memory_order_acquire);
        if (latestResetSeq != handledResetSequence)
        {
            handledResetSequence = latestResetSeq;
            state.position = restPosition;
            state.velocity = {};
            state.force = {};
            state.collisionMask = 0;
            state.collisionEnergy = 0.0f;
            previousRestPosition = restPosition;
        }

        if (! bodyEnabled.load (std::memory_order_acquire))
        {
            state.position = restPosition;
            state.velocity = {};
            state.force = {};
            state.collisionMask = 0;
            state.collisionEnergy = 0.0f;
            previousRestPosition = restPosition;
            writeState (state);
            return;
        }

        // Treat physics state as an offset from a moving rest pose:
        // if animation/keyframes move the rest point, shift the body with it.
        const Vec3 restDelta
        {
            restPosition.x - previousRestPosition.x,
            restPosition.y - previousRestPosition.y,
            restPosition.z - previousRestPosition.z
        };

        state.position.x += restDelta.x;
        state.position.y += restDelta.y;
        state.position.z += restDelta.z;
        previousRestPosition = restPosition;

        const auto latestThrowSeq = throwSequence.load (std::memory_order_acquire);
        if (latestThrowSeq != handledThrowSequence)
        {
            handledThrowSequence = latestThrowSeq;
            state.velocity.x += throwVelocityX.load (std::memory_order_acquire);
            state.velocity.y += throwVelocityY.load (std::memory_order_acquire);
            state.velocity.z += throwVelocityZ.load (std::memory_order_acquire);
        }

        // P6: Consume one-shot collision impulse from CollisionSystem::resolve()
        const auto latestCollisionSeq = collisionImpulseSeq.load (std::memory_order_acquire);
        if (latestCollisionSeq != handledCollisionSeq)
        {
            handledCollisionSeq = latestCollisionSeq;
            state.velocity.x += collisionImpulseX.load (std::memory_order_acquire);
            state.velocity.y += collisionImpulseY.load (std::memory_order_acquire);
            state.velocity.z += collisionImpulseZ.load (std::memory_order_acquire);
        }

        if (simulationPaused.load (std::memory_order_acquire))
        {
            state.force = {};
            state.collisionMask = 0;
            state.collisionEnergy = 0.0f;
            writeState (state);
            return;
        }

        const float currentMass       = mass.load (std::memory_order_acquire);
        const float currentDrag       = drag.load (std::memory_order_acquire);
        const float currentElasticity = elasticity.load (std::memory_order_acquire);
        const float currentFriction   = friction.load (std::memory_order_acquire);

        const Vec3 gravity = computeGravityVector (state.position);
        const Vec3 interactionForce
        {
            interactionForceX.load (std::memory_order_acquire),
            interactionForceY.load (std::memory_order_acquire),
            interactionForceZ.load (std::memory_order_acquire)
        };
        const Vec3 coordinatedForce = standaloneMode.load (std::memory_order_acquire)
            ? Vec3 {}
            : Vec3 {
                coordinatedForceX.load (std::memory_order_acquire),
                coordinatedForceY.load (std::memory_order_acquire),
                coordinatedForceZ.load (std::memory_order_acquire)
            };
        const float inverseMass = 1.0f / juce::jmax (0.01f, currentMass);

        // Combined force: gravity + host interaction + PhysicsWorker coordinated force
        state.force.x = gravity.x + interactionForce.x + coordinatedForce.x;
        state.force.y = gravity.y + interactionForce.y + coordinatedForce.y;
        state.force.z = gravity.z + interactionForce.z + coordinatedForce.z;
        state.collisionMask = 0;
        state.collisionEnergy = 0.0f;

        state.velocity.x += state.force.x * inverseMass * dt;
        state.velocity.y += state.force.y * inverseMass * dt;
        state.velocity.z += state.force.z * inverseMass * dt;

        const float dragFactor = juce::jlimit (0.0f, 1.0f, 1.0f - currentDrag * dt);
        state.velocity.x *= dragFactor;
        state.velocity.y *= dragFactor;
        state.velocity.z *= dragFactor;

        state.position.x += state.velocity.x * dt;
        state.position.y += state.velocity.y * dt;
        state.position.z += state.velocity.z * dt;

        if (wallCollisionEnabled.load (std::memory_order_acquire))
        {
            const auto mode = static_cast<BoundaryMode> (
                boundaryMode.load (std::memory_order_acquire));

            if (mode == BoundaryMode::Hard)
                resolveCollisions (state, currentElasticity, currentFriction, dt);
            else if (mode == BoundaryMode::Soft)
                resolveSoftBoundary (state, currentMass, dt);
            // BoundaryMode::Passthrough — no collision response
        }

        writeState (state);
    }

    void resolveCollisions (PhysicsState& state, float bounce, float surfaceFriction, float dt)
    {
        const float halfWidth = roomWidth.load (std::memory_order_acquire) * 0.5f;
        const float halfDepth = roomDepth.load (std::memory_order_acquire) * 0.5f;
        const float minY = 0.0f;
        const float maxY = roomHeight.load (std::memory_order_acquire);

        bool collideX = false;
        bool collideY = false;
        bool collideZ = false;

        // Capture pre-collision velocity for normalized energy computation.
        const Vec3 velocityBefore = state.velocity;

        if (state.position.x < -halfWidth)
        {
            state.position.x = -halfWidth;
            state.velocity.x = std::abs (state.velocity.x) * bounce;
            collideX = true;
        }
        else if (state.position.x > halfWidth)
        {
            state.position.x = halfWidth;
            state.velocity.x = -std::abs (state.velocity.x) * bounce;
            collideX = true;
        }

        if (state.position.y < minY)
        {
            state.position.y = minY;
            state.velocity.y = std::abs (state.velocity.y) * bounce;
            collideY = true;
        }
        else if (state.position.y > maxY)
        {
            state.position.y = maxY;
            state.velocity.y = -std::abs (state.velocity.y) * bounce;
            collideY = true;
        }

        if (state.position.z < -halfDepth)
        {
            state.position.z = -halfDepth;
            state.velocity.z = std::abs (state.velocity.z) * bounce;
            collideZ = true;
        }
        else if (state.position.z > halfDepth)
        {
            state.position.z = halfDepth;
            state.velocity.z = -std::abs (state.velocity.z) * bounce;
            collideZ = true;
        }

        // Compute collision energy as Euclidean magnitude of velocity delta,
        // so corner collisions produce consistent energy regardless of axis count.
        if (collideX || collideY || collideZ)
        {
            const float dx = state.velocity.x - velocityBefore.x;
            const float dy = state.velocity.y - velocityBefore.y;
            const float dz = state.velocity.z - velocityBefore.z;
            state.collisionEnergy = std::sqrt (dx * dx + dy * dy + dz * dz);
        }

        if (collideX)
        {
            applySurfaceFriction (state.velocity.y, state.velocity.z, surfaceFriction, dt);
            state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x1u);
        }
        if (collideY)
        {
            applySurfaceFriction (state.velocity.x, state.velocity.z, surfaceFriction, dt);
            state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x2u);
        }
        if (collideZ)
        {
            applySurfaceFriction (state.velocity.x, state.velocity.y, surfaceFriction, dt);
            state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x4u);
        }
    }

    /**
     * Soft boundary: apply 1/(d²) repulsive acceleration within softBoundaryDepth.
     * Emitter decelerates and curves away; hard position clamp at surface as safety net.
     *
     * Acceleration at boundary edge (d = depth):  kSoftBase = 20.0 m/s²  (~2g)
     * Acceleration formula: a = kSoftBase * (depth / d)²
     * As d → 0 the acceleration diverges, ensuring emitter never penetrates.
     */
    void resolveSoftBoundary (PhysicsState& state, float mass, float dt)
    {
        const float halfWidth = roomWidth.load (std::memory_order_acquire) * 0.5f;
        const float halfDepth = roomDepth.load (std::memory_order_acquire) * 0.5f;
        const float maxY      = roomHeight.load (std::memory_order_acquire);
        const float depth     = softBoundaryDepth.load (std::memory_order_acquire);
        const float epsilon   = depth * 0.01f;  // minimum distance, prevents div/0
        static constexpr float kSoftBase = 20.0f; // m/s² at boundary edge

        (void) mass; // acceleration is mass-independent (F = m*a → a = F/m, k already in m/s²)

        auto applyWallRepulsion = [&](float pos, float wallSurface, float sign, float& vel)
        {
            // sign = +1 means "push toward +" (positive-side wall pushes emitter in -x)
            // actual: dist = |wallSurface - pos|, push direction = sign
            const float dist = std::max (std::abs (wallSurface - pos), epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                const float acc = kSoftBase * ratio * ratio;
                vel += sign * acc * dt;
            }
            // Safety clamp: ensure emitter never crosses wall surface
            if (sign > 0.0f && pos < wallSurface)
            {
                state.position.x = (vel == state.velocity.x) ? wallSurface + epsilon : state.position.x;
                vel = std::max (vel, 0.0f);
            }
            else if (sign < 0.0f && pos > wallSurface)
            {
                vel = std::min (vel, 0.0f);
            }
        };

        // -X wall (push in +X direction): surface at -halfWidth
        {
            const float dist = std::max (state.position.x - (-halfWidth), epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.x += kSoftBase * ratio * ratio * dt;
            }
            if (state.position.x < -halfWidth)
            {
                state.position.x = -halfWidth + epsilon;
                if (state.velocity.x < 0.0f) state.velocity.x = 0.0f;
            }
        }
        // +X wall (push in -X direction): surface at +halfWidth
        {
            const float dist = std::max (halfWidth - state.position.x, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.x -= kSoftBase * ratio * ratio * dt;
            }
            if (state.position.x > halfWidth)
            {
                state.position.x = halfWidth - epsilon;
                if (state.velocity.x > 0.0f) state.velocity.x = 0.0f;
            }
        }
        // Floor (Y=0, push in +Y direction)
        {
            const float dist = std::max (state.position.y - 0.0f, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.y += kSoftBase * ratio * ratio * dt;
            }
            if (state.position.y < 0.0f)
            {
                state.position.y = epsilon;
                if (state.velocity.y < 0.0f) state.velocity.y = 0.0f;
            }
        }
        // Ceiling (Y=maxY, push in -Y direction)
        {
            const float dist = std::max (maxY - state.position.y, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.y -= kSoftBase * ratio * ratio * dt;
            }
            if (state.position.y > maxY)
            {
                state.position.y = maxY - epsilon;
                if (state.velocity.y > 0.0f) state.velocity.y = 0.0f;
            }
        }
        // -Z wall (push in +Z direction)
        {
            const float dist = std::max (state.position.z - (-halfDepth), epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.z += kSoftBase * ratio * ratio * dt;
            }
            if (state.position.z < -halfDepth)
            {
                state.position.z = -halfDepth + epsilon;
                if (state.velocity.z < 0.0f) state.velocity.z = 0.0f;
            }
        }
        // +Z wall (push in -Z direction)
        {
            const float dist = std::max (halfDepth - state.position.z, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.z -= kSoftBase * ratio * ratio * dt;
            }
            if (state.position.z > halfDepth)
            {
                state.position.z = halfDepth - epsilon;
                if (state.velocity.z > 0.0f) state.velocity.z = 0.0f;
            }
        }

        (void) applyWallRepulsion; // helper defined but dispatch done inline above
    }

    static void applySurfaceFriction (float& tangentA, float& tangentB, float frictionAmount, float dt)
    {
        const float friction = juce::jlimit (0.0f, 1.0f, frictionAmount);
        const float damp = juce::jlimit (0.0f, 1.0f, 1.0f - friction * dt * 60.0f);
        tangentA *= damp;
        tangentB *= damp;
    }

    Vec3 computeGravityVector (const Vec3& position) const
    {
        const float magnitude = gravityMagnitude.load (std::memory_order_acquire);
        const int direction = gravityDirection.load (std::memory_order_acquire);

        switch (direction)
        {
            case 0: // Down
                return { 0.0f, -magnitude, 0.0f };

            case 1: // Up
                return { 0.0f, magnitude, 0.0f };

            case 2: // To center
            case 3: // From center
            {
                Vec3 toCenter { -position.x, 1.2f - position.y, -position.z };
                const float length = std::sqrt (toCenter.x * toCenter.x
                                              + toCenter.y * toCenter.y
                                              + toCenter.z * toCenter.z);

                if (length < 1.0e-5f)
                    return {};

                const float scale = magnitude / length;
                const float sign = (direction == 2) ? 1.0f : -1.0f;
                return { toCenter.x * scale * sign,
                         toCenter.y * scale * sign,
                         toCenter.z * scale * sign };
            }

            case 4: // Custom (placeholder: use Down until vector params exist)
            default:
                return { 0.0f, -magnitude, 0.0f };
        }
    }

    void writeState (const PhysicsState& state)
    {
        const int nextWrite = 1 - readIndex.load (std::memory_order_acquire);
        stateBuffers[static_cast<size_t> (nextWrite)] = state;
        readIndex.store (nextWrite, std::memory_order_release);
    }

    static float getUpdateRateHz (int index)
    {
        static constexpr float rates[] = { 30.0f, 60.0f, 120.0f, 240.0f };
        return rates[static_cast<size_t> (juce::jlimit (0, 3, index))];
    }

    //==========================================================================
    std::array<PhysicsState, 2> stateBuffers {};
    std::atomic<int> readIndex { 0 };

    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<int> updateRateIndex { 1 };
    std::atomic<bool> simulationPaused { false };
    std::atomic<bool> wallCollisionEnabled { true };

    std::atomic<bool>  standaloneMode    { true };
    std::atomic<bool>  bodyEnabled       { false };
    std::atomic<int>   boundaryMode      { static_cast<int>(BoundaryMode::Hard) };
    std::atomic<float> softBoundaryDepth { 0.5f };
    std::atomic<float> coordinatedForceX { 0.0f };
    std::atomic<float> coordinatedForceY { 0.0f };
    std::atomic<float> coordinatedForceZ { 0.0f };
    std::atomic<float> mass { 1.0f };
    std::atomic<float> drag { 0.5f };
    std::atomic<float> elasticity { 0.7f };
    std::atomic<float> friction { 0.3f };

    std::atomic<float> gravityMagnitude { 0.0f };
    std::atomic<int> gravityDirection { 0 };
    std::atomic<float> interactionForceX { 0.0f };
    std::atomic<float> interactionForceY { 0.0f };
    std::atomic<float> interactionForceZ { 0.0f };

    std::atomic<float> restX { 0.0f };
    std::atomic<float> restY { 0.0f };
    std::atomic<float> restZ { 0.0f };

    std::atomic<float> roomWidth { 6.0f };
    std::atomic<float> roomDepth { 4.0f };
    std::atomic<float> roomHeight { 3.0f };

    std::atomic<float> throwVelocityX { 0.0f };
    std::atomic<float> throwVelocityY { 0.0f };
    std::atomic<float> throwVelocityZ { 0.0f };
    std::atomic<uint32_t> throwSequence { 0 };
    std::atomic<uint32_t> resetSequence { 0 };

    // P6: one-shot collision impulse from CollisionSystem::resolve()
    std::atomic<float>    collisionImpulseX   { 0.0f };
    std::atomic<float>    collisionImpulseY   { 0.0f };
    std::atomic<float>    collisionImpulseZ   { 0.0f };
    std::atomic<uint32_t> collisionImpulseSeq { 0 };

    uint32_t handledThrowSequence     = 0;
    uint32_t handledResetSequence     = 0;
    uint32_t handledCollisionSeq      = 0;
    Vec3 previousRestPosition {};
    bool restPositionInitialized = false;
};
