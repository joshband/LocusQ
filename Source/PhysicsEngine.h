#pragma once

#include "SceneGraph.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <thread>

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
class PhysicsEngine
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

    PhysicsEngine() = default;

    ~PhysicsEngine()
    {
        shutdown();
    }

    //==========================================================================
    void prepare (double sampleRate)
    {
        currentSampleRate.store (sampleRate, std::memory_order_relaxed);
        startThreadIfNeeded();
    }

    void shutdown()
    {
        if (! running.exchange (false, std::memory_order_acq_rel))
            return;

        if (worker.joinable())
            worker.join();
    }

    //==========================================================================
    void setPhysicsEnabled (bool enabled)              { bodyEnabled.store (enabled, std::memory_order_release); }
    void setPaused (bool paused)                       { simulationPaused.store (paused, std::memory_order_release); }
    void setWallCollisionEnabled (bool enabled)        { wallCollisionEnabled.store (enabled, std::memory_order_release); }
    void setUpdateRateIndex (int index)                { updateRateIndex.store (juce::jlimit (0, 3, index), std::memory_order_release); }

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
    }

    void requestReset()
    {
        resetSequence.fetch_add (1, std::memory_order_acq_rel);
    }

    PhysicsState getState() const
    {
        const int idx = readIndex.load (std::memory_order_acquire);
        return stateBuffers[static_cast<size_t> (idx)];
    }

private:
    //==========================================================================
    void startThreadIfNeeded()
    {
        if (running.load (std::memory_order_acquire))
            return;

        running.store (true, std::memory_order_release);
        worker = std::thread ([this] { runLoop(); });
    }

    void runLoop()
    {
        while (running.load (std::memory_order_acquire))
        {
            const float rateHz = getUpdateRateHz (updateRateIndex.load (std::memory_order_acquire));
            const float dt = 1.0f / rateHz;

            step (dt);
            std::this_thread::sleep_for (std::chrono::duration<float> (dt));
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
        const float inverseMass = 1.0f / juce::jmax (0.01f, currentMass);
        state.force.x = gravity.x + interactionForce.x;
        state.force.y = gravity.y + interactionForce.y;
        state.force.z = gravity.z + interactionForce.z;
        state.collisionMask = 0;
        state.collisionEnergy = 0.0f;

        state.velocity.x += gravity.x * inverseMass * dt;
        state.velocity.y += gravity.y * inverseMass * dt;
        state.velocity.z += gravity.z * inverseMass * dt;

        state.velocity.x += interactionForce.x * inverseMass * dt;
        state.velocity.y += interactionForce.y * inverseMass * dt;
        state.velocity.z += interactionForce.z * inverseMass * dt;

        const float dragFactor = juce::jlimit (0.0f, 1.0f, 1.0f - currentDrag * dt);
        state.velocity.x *= dragFactor;
        state.velocity.y *= dragFactor;
        state.velocity.z *= dragFactor;

        state.position.x += state.velocity.x * dt;
        state.position.y += state.velocity.y * dt;
        state.position.z += state.velocity.z * dt;

        if (wallCollisionEnabled.load (std::memory_order_acquire))
            resolveCollisions (state, currentElasticity, currentFriction, dt);

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

        if (state.position.x < -halfWidth)
        {
            const auto velocityBefore = state.velocity.x;
            state.position.x = -halfWidth;
            state.velocity.x = std::abs (state.velocity.x) * bounce;
            collideX = true;
            state.collisionEnergy += std::abs (state.velocity.x - velocityBefore);
        }
        else if (state.position.x > halfWidth)
        {
            const auto velocityBefore = state.velocity.x;
            state.position.x = halfWidth;
            state.velocity.x = -std::abs (state.velocity.x) * bounce;
            collideX = true;
            state.collisionEnergy += std::abs (state.velocity.x - velocityBefore);
        }

        if (state.position.y < minY)
        {
            const auto velocityBefore = state.velocity.y;
            state.position.y = minY;
            state.velocity.y = std::abs (state.velocity.y) * bounce;
            collideY = true;
            state.collisionEnergy += std::abs (state.velocity.y - velocityBefore);
        }
        else if (state.position.y > maxY)
        {
            const auto velocityBefore = state.velocity.y;
            state.position.y = maxY;
            state.velocity.y = -std::abs (state.velocity.y) * bounce;
            collideY = true;
            state.collisionEnergy += std::abs (state.velocity.y - velocityBefore);
        }

        if (state.position.z < -halfDepth)
        {
            const auto velocityBefore = state.velocity.z;
            state.position.z = -halfDepth;
            state.velocity.z = std::abs (state.velocity.z) * bounce;
            collideZ = true;
            state.collisionEnergy += std::abs (state.velocity.z - velocityBefore);
        }
        else if (state.position.z > halfDepth)
        {
            const auto velocityBefore = state.velocity.z;
            state.position.z = halfDepth;
            state.velocity.z = -std::abs (state.velocity.z) * bounce;
            collideZ = true;
            state.collisionEnergy += std::abs (state.velocity.z - velocityBefore);
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

    std::atomic<bool> running { false };
    std::thread worker;

    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<int> updateRateIndex { 1 };
    std::atomic<bool> simulationPaused { false };
    std::atomic<bool> wallCollisionEnabled { true };

    std::atomic<bool> bodyEnabled { false };
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

    uint32_t handledThrowSequence = 0;
    uint32_t handledResetSequence = 0;
    Vec3 previousRestPosition {};
    bool restPositionInitialized = false;
};
