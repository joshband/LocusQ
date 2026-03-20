#pragma once

#include "AttractorSystem.h"
#include "BoidsSystem.h"
#include "ChoreographyWorker.h"
#include "CollisionSystem.h"
#include "PhysicsEngine.h"
#include "PhysicsDSPBridge.h"
#include "SceneGraph.h"
#include "SpringSystem.h"
#include "TurbulenceSystem.h"
#include "AngularPhysicsSystem.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

//==============================================================================
/**
 * WorkerEmitterState - simulation state for one coordinated emitter slot.
 *
 * Owned by PhysicsWorker. Written on the worker thread, read by
 * PhysicsWorker::getEmitterState() from any thread via atomic index swap
 * (same double-buffer contract as PhysicsEngine).
 */
struct WorkerEmitterState
{
    Vec3 position       { 0.0f, 0.0f,  0.0f };
    Vec3 velocity       { 0.0f, 0.0f,  0.0f };
    Vec3 force          { 0.0f, 0.0f,  0.0f };
    Vec3 restPosition   { 0.0f, 0.0f,  0.0f };
    Vec3 directivityAim { 0.0f, 0.0f, -1.0f };  // matches EmitterData default
    std::uint8_t collisionMask = 0;
    float collisionEnergy = 0.0f;
    bool active         = false;
    bool initialized    = false;
};

//==============================================================================
/**
 * PhysicsWorker - single shared simulation thread for multi-emitter coordination.
 *
 * Manages up to kMaxEmitters emitter slots on one worker thread. Each slot
 * stores double-buffered WorkerEmitterState for lock-free reading.
 *
 * Phase P1: infrastructure only — tick loop, stall guard, slot registry.
 * Subsystems (attractors, boids, collisions, spring, turbulence, angular)
 * are added in phases P2–P6 and call into the worker tick via hooks.
 *
 * Threading contract:
 * - All simulation state is written on the worker thread.
 * - Readers (audio thread, DSP bridge) only call getEmitterState() and
 *   getTickCount() — both lock-free.
 * - No heap allocation occurs after prepare().
 */
class PhysicsWorker : private juce::Thread
{
public:
    static constexpr int kMaxEmitters = 64;

    PhysicsWorker()
        : juce::Thread ("LocusQPhysicsWorker")
    {}

    ~PhysicsWorker() override
    {
        shutdown();
    }

    //==========================================================================
    // Setup / lifecycle

    void prepare (int rateIndex)
    {
        updateRateIndex.store (juce::jlimit (0, 3, rateIndex), std::memory_order_release);

        if (! isThreadRunning())
            startThread();

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

    void setPaused (bool paused)
    {
        simulationPaused.store (paused, std::memory_order_release);
        notify();
    }

    void setUpdateRateIndex (int index)
    {
        updateRateIndex.store (juce::jlimit (0, 3, index), std::memory_order_release);
        notify();
    }

    void setRoomDimensions (const Vec3& dims)
    {
        roomWidth.store  (juce::jmax (0.5f, dims.x), std::memory_order_release);
        roomDepth.store  (juce::jmax (0.5f, dims.y), std::memory_order_release);
        roomHeight.store (juce::jmax (0.5f, dims.z), std::memory_order_release);
    }

    //==========================================================================
    // Engine and bridge registration (call from non-audio, non-worker thread)

    /** Register a PhysicsEngine for coordinated simulation.
     *  The engine must outlive the PhysicsWorker (or be unregistered first). */
    void registerEngine (int index, PhysicsEngine* engine)
    {
        if (index < 0 || index >= kMaxEmitters) return;
        engines[static_cast<std::size_t>(index)] = engine;
    }

    void unregisterEngine (int index)
    {
        if (index < 0 || index >= kMaxEmitters) return;
        engines[static_cast<std::size_t>(index)] = nullptr;
        attractorSystem.resetTargetRadii(index);
    }

    /** Attach a DSP bridge for per-emitter proximity/event publishing. */
    void attachDSPBridge (PhysicsDSPBridge* bridge) { dspBridge = bridge; }

    //==========================================================================
    // Subsystem accessors — direct access for APVTS wiring

    AttractorSystem&      getAttractorSystem()      { return attractorSystem; }
    SpringSystem&         getSpringSystem()         { return springSystem; }
    TurbulenceSystem&     getTurbulenceSystem()     { return turbulenceSystem; }
    AngularPhysicsSystem& getAngularSystem()        { return angularSystem; }
    BoidsSystem&          getBoidsSystem()          { return boidsSystem; }
    CollisionSystem&      getCollisionSystem()      { return collisionSystem; }   // P6
    ChoreographyWorker&   getChoreographyWorker()   { return choreographyWorker; }

    //==========================================================================
    // Per-emitter mass override — P6

    /** Set per-emitter mass override. 0 = use global engine mass (phys_mass). */
    void setSlotMassOverride (int index, float mass)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;
        slots[static_cast<std::size_t> (index)].massOverride.store (
            juce::jmax (0.0f, mass), std::memory_order_release);
    }

    //==========================================================================
    // Emitter slot management

    /** Activate a slot and set its rest position. Must be called off the audio thread. */
    void activateSlot (int index, const Vec3& restPos)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        auto& slot = slots[static_cast<std::size_t> (index)];

        // Write into the inactive buffer so the reader never sees a torn state.
        const int writeIdx = 1 - slot.readIndex.load (std::memory_order_acquire);
        slot.buffers[writeIdx].restPosition = restPos;
        slot.buffers[writeIdx].position     = restPos;
        slot.buffers[writeIdx].velocity     = {};
        slot.buffers[writeIdx].active       = true;
        slot.buffers[writeIdx].initialized  = false;
        slot.readIndex.store (writeIdx, std::memory_order_release);

        slot.restX.store (restPos.x, std::memory_order_release);
        slot.restY.store (restPos.y, std::memory_order_release);
        slot.restZ.store (restPos.z, std::memory_order_release);
        slot.active.store (true, std::memory_order_release);
    }

    void deactivateSlot (int index)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        slots[static_cast<std::size_t> (index)].active.store (false, std::memory_order_release);
    }

    bool isSlotActive (int index) const noexcept
    {
        if (index < 0 || index >= kMaxEmitters)
            return false;

        return slots[static_cast<std::size_t> (index)].active.load (std::memory_order_acquire);
    }

    void setSlotRestPosition (int index, const Vec3& restPos)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        auto& slot = slots[static_cast<std::size_t> (index)];
        slot.restX.store (restPos.x, std::memory_order_release);
        slot.restY.store (restPos.y, std::memory_order_release);
        slot.restZ.store (restPos.z, std::memory_order_release);
    }

    void setSlotInteractionForce (int index, const Vec3& force)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        auto& slot = slots[static_cast<std::size_t> (index)];
        slot.interactionX.store (force.x, std::memory_order_release);
        slot.interactionY.store (force.y, std::memory_order_release);
        slot.interactionZ.store (force.z, std::memory_order_release);
    }

    void setGravity (float magnitude, int direction)
    {
        gravityMagnitude.store (magnitude, std::memory_order_release);
        gravityDirection.store (direction, std::memory_order_release);
    }

    void setWallCollisionEnabled (bool enabled)
    {
        wallCollisionEnabled.store (enabled, std::memory_order_release);
    }

    void setBoundaryMode (PhysicsEngine::BoundaryMode mode)
    {
        boundaryMode.store (static_cast<int> (mode), std::memory_order_release);
    }

    void setSoftBoundaryDepth (float depth)
    {
        softBoundaryDepth.store (juce::jmax (0.01f, depth), std::memory_order_release);
    }

    void requestThrow (int index, const Vec3& initialVelocity)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        auto& slot = slots[static_cast<std::size_t> (index)];
        slot.throwVelocityX.store (initialVelocity.x, std::memory_order_release);
        slot.throwVelocityY.store (initialVelocity.y, std::memory_order_release);
        slot.throwVelocityZ.store (initialVelocity.z, std::memory_order_release);
        slot.throwSequence.fetch_add (1, std::memory_order_acq_rel);
    }

    void requestReset (int index)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        slots[static_cast<std::size_t> (index)].resetSequence.fetch_add (1, std::memory_order_acq_rel);
    }

    //==========================================================================
    // Lock-free readers (safe on audio thread / any thread)

    WorkerEmitterState getEmitterState (int index) const
    {
        if (index < 0 || index >= kMaxEmitters)
            return {};

        const auto& slot = slots[static_cast<std::size_t> (index)];
        const int readIdx = slot.readIndex.load (std::memory_order_acquire);
        return slot.buffers[static_cast<std::size_t> (readIdx)];
    }

    /** Monotonically increasing counter incremented each worker tick.
     *  Audio thread compares against a saved value to detect stalls. */
    std::uint64_t getTickCount() const
    {
        return tickCount.load (std::memory_order_acquire);
    }

    /** Returns the tick rate period in milliseconds for the current rate index. */
    double getPeriodMs() const
    {
        return 1000.0 / static_cast<double> (rateHz (updateRateIndex.load (std::memory_order_acquire)));
    }

private:
    //==========================================================================
    struct EmitterSlot
    {
        std::array<WorkerEmitterState, 2> buffers {};
        std::atomic<int> readIndex { 0 };

        std::atomic<float> restX { 0.0f };
        std::atomic<float> restY { 0.0f };
        std::atomic<float> restZ { 0.0f };
        std::atomic<float> interactionX { 0.0f };
        std::atomic<float> interactionY { 0.0f };
        std::atomic<float> interactionZ { 0.0f };
        std::atomic<bool>  active { false };
        std::atomic<float> throwVelocityX { 0.0f };
        std::atomic<float> throwVelocityY { 0.0f };
        std::atomic<float> throwVelocityZ { 0.0f };
        std::atomic<std::uint32_t> throwSequence { 0 };
        std::uint32_t handledThrowSequence = 0;
        std::atomic<std::uint32_t> resetSequence { 0 };
        std::uint32_t handledResetSequence = 0;

        /** Per-emitter mass override (P6). 0 = use global engine mass. */
        std::atomic<float> massOverride { 0.0f };

        Vec3 previousRestPos {};
        bool restInitialized = false;
    };

    //==========================================================================
    void run() override
    {
        auto nextTickMs  = juce::Time::getMillisecondCounterHiRes();
        int  lastRateIdx = updateRateIndex.load (std::memory_order_acquire);

        while (! threadShouldExit())
        {
            const int    rateIdx  = updateRateIndex.load (std::memory_order_acquire);
            const double nowMs    = juce::Time::getMillisecondCounterHiRes();
            const double periodMs = 1000.0 / static_cast<double> (rateHz (rateIdx));

            if (rateIdx != lastRateIdx)
            {
                nextTickMs  = nowMs;
                lastRateIdx = rateIdx;
            }

            const double untilNext = nextTickMs - nowMs;

            if (untilNext > 0.25)
            {
                wait (juce::jlimit (1, 50, static_cast<int> (std::ceil (untilNext))));
                continue;
            }

            if (! simulationPaused.load (std::memory_order_acquire))
                tick (static_cast<float> (periodMs * 0.001));

            tickCount.fetch_add (1, std::memory_order_release);

            nextTickMs += periodMs;
            if (nextTickMs < nowMs)
                nextTickMs = nowMs + periodMs;
        }
    }

    /** Single simulation step — called on worker thread only.
     *  P1: rest-position drift tracking and slot initialization.
     *  P2: attractor forces injected into registered PhysicsEngines;
     *      DSP bridge updated with proximity and crossing events.
     *  P3: spring restoring force + turbulence injected; spread LFO
     *      and turbulence magnitude published to DSP bridge.
     *  P4: angular velocity integrated → directivityAim written to slot.
     *  P5: boids snapshot built; separation/alignment/cohesion forces injected;
     *      flock density published to DSP bridge.
     *  P6: collision snapshot built; O(n²) impulse resolution; impulses pushed
     *      to engines; collision energy published to DSP bridge. */
    void tick (float dt)
    {
        // CL-P2: Count active emitter slots for the FormationSystem.
        int activeEmitterCount = 0;
        for (int i = 0; i < kMaxEmitters; ++i)
            if (slots[static_cast<std::size_t> (i)].active.load (std::memory_order_acquire))
                ++activeEmitterCount;

        // Compute choreography offsets before any per-emitter work
        // (ADR-0020 step 3; tick sequence from choreography-lab-impl-plan.md).
        choreographyWorker.compute (dt, activeEmitterCount);

        // P4: Consume one-shot angular triggers once for the whole tick so all
        //     emitters receive the same throw/reset in the same simulation step.
        const bool  angThrowThisTick   = angularSystem.consumeThrow();
        const Vec3  angImpulseThisTick = angularSystem.getImpulse();
        const bool  angResetThisTick   = angularSystem.consumeReset();

        // P5: Build boids snapshot from current slot states — must precede the
        //     main emitter loop so all emitters see a consistent scene state.
        for (int i = 0; i < kMaxEmitters; ++i)
        {
            const auto& slot = slots[static_cast<std::size_t> (i)];
            if (! slot.active.load (std::memory_order_acquire))
            {
                boidsSystem.notifyEmitterState (i, {}, {}, false);
                continue;
            }
            const int readIdx = slot.readIndex.load (std::memory_order_acquire);
            const auto& s = slot.buffers[static_cast<std::size_t> (readIdx)];
            boidsSystem.notifyEmitterState (i, s.position, s.velocity, true);
        }
        boidsSystem.finalizeSnapshot();

        // P6: Build collision snapshot and resolve inter-emitter impulses.
        //     Done before the main loop so all emitters see a consistent scene
        //     state, matching the boids two-pass contract.
        if (collisionSystem.isEnabled())
        {
            for (int i = 0; i < kMaxEmitters; ++i)
            {
                const auto& cslot = slots[static_cast<std::size_t> (i)];

                if (! cslot.active.load (std::memory_order_acquire))
                {
                    collisionSystem.setEmitterState (i, {}, {}, 1.0f, false);
                    continue;
                }

                // Read most-current position/velocity from engine if available
                Vec3 cPos {}, cVel {};
                const int cReadIdx = cslot.readIndex.load (std::memory_order_acquire);
                const auto& cBuf   = cslot.buffers[static_cast<std::size_t> (cReadIdx)];
                cPos = cBuf.position;
                cVel = cBuf.velocity;

                PhysicsEngine* cEngine = engines[static_cast<std::size_t> (i)];
                if (cEngine != nullptr)
                {
                    const auto ces = cEngine->getState();
                    if (ces.initialized && cEngine->isStandaloneMode()) { cPos = ces.position; cVel = ces.velocity; }
                }

                // Per-emitter mass: override > 0 → use override, else engine mass
                float emitterMass = 1.0f;
                if (cEngine != nullptr)
                    emitterMass = cEngine->getMass();
                const float massOvr = cslot.massOverride.load (std::memory_order_acquire);
                if (massOvr > 0.0f)
                    emitterMass = massOvr;

                collisionSystem.setEmitterState (i, cPos, cVel, emitterMass, true);
            }

            // Use elasticity from the first registered active engine (default 0.7)
            float sceneElasticity = 0.7f;
            for (int i = 0; i < kMaxEmitters; ++i)
            {
                PhysicsEngine* eng = engines[static_cast<std::size_t> (i)];
                if (eng != nullptr && slots[static_cast<std::size_t> (i)].active.load (std::memory_order_acquire))
                {
                    sceneElasticity = eng->getElasticity();
                    break;
                }
            }
            collisionSystem.setElasticity (sceneElasticity);
            collisionSystem.resolve();
        }

        int coordinatedActiveCount = 0;
        for (int i = 0; i < kMaxEmitters; ++i)
        {
            const auto& slot = slots[static_cast<std::size_t> (i)];
            PhysicsEngine* engine = engines[static_cast<std::size_t> (i)];
            if (slot.active.load (std::memory_order_acquire) && engine != nullptr && ! engine->isStandaloneMode())
                ++coordinatedActiveCount;
        }

        for (int i = 0; i < kMaxEmitters; ++i)
        {
            auto& slot = slots[static_cast<std::size_t> (i)];

            if (! slot.active.load (std::memory_order_acquire))
                continue;

            const int readIdx = slot.readIndex.load (std::memory_order_acquire);
            WorkerEmitterState state = slot.buffers[static_cast<std::size_t> (readIdx)];

            const Vec3 restPos
            {
                slot.restX.load (std::memory_order_acquire),
                slot.restY.load (std::memory_order_acquire),
                slot.restZ.load (std::memory_order_acquire)
            };
            const Vec3 interactionForce
            {
                slot.interactionX.load (std::memory_order_acquire),
                slot.interactionY.load (std::memory_order_acquire),
                slot.interactionZ.load (std::memory_order_acquire)
            };

            // CL-P1: Compose rest pose with choreography offset (ADR-0020 Layer 3).
            // When choro_enable=false all offsets are zero — no behavioural change.
            const auto& choroOffset = choreographyWorker.getOffset (i);
            const Vec3 composedRestPos
            {
                restPos.x + choroOffset.position.x,
                restPos.y + choroOffset.position.y,
                restPos.z + choroOffset.position.z
            };

            if (! slot.restInitialized)
            {
                slot.previousRestPos = restPos;
                slot.restInitialized = true;
            }

            if (! state.initialized)
            {
                state.position    = restPos;
                state.velocity    = {};
                state.force       = {};
                state.collisionMask = 0;
                state.collisionEnergy = 0.0f;
                state.initialized = true;
                slot.previousRestPos = restPos;
                prevPositions[i]  = restPos;
            }

            const auto latestResetSeq = slot.resetSequence.load (std::memory_order_acquire);
            if (latestResetSeq != slot.handledResetSequence)
            {
                slot.handledResetSequence = latestResetSeq;
                state.position = restPos;
                state.velocity = {};
                state.force = {};
                state.collisionMask = 0;
                state.collisionEnergy = 0.0f;
                slot.previousRestPos = restPos;
                prevPositions[i] = restPos;
            }

            // Track rest-position drift (keyframe animation additive offset contract).
            state.position.x += restPos.x - slot.previousRestPos.x;
            state.position.y += restPos.y - slot.previousRestPos.y;
            state.position.z += restPos.z - slot.previousRestPos.z;
            slot.previousRestPos = restPos;
            state.restPosition   = restPos;

            //------------------------------------------------------------------
            // P2: Attractor force injection via registered PhysicsEngine
            //------------------------------------------------------------------
            PhysicsEngine* engine = engines[static_cast<std::size_t> (i)];

            Vec3 currentPos = state.position;
            Vec3 currentVel = state.velocity;
            float currentMass = 1.0f;
            float currentDrag = 0.0f;
            const bool coordinatedAuthority = engine != nullptr && ! engine->isStandaloneMode();

            if (engine != nullptr)
            {
                const auto es = engine->getState();
                if (es.initialized && ! coordinatedAuthority)
                {
                    currentPos = es.position;
                    currentVel = es.velocity;
                    state.position = currentPos;
                    state.velocity = currentVel;
                }
                currentMass = juce::jmax (0.01f, engine->getMass());
                currentDrag = juce::jmax (0.0f, engine->getDrag());
            }

            const auto latestThrowSeq = slot.throwSequence.load (std::memory_order_acquire);
            if (latestThrowSeq != slot.handledThrowSequence)
            {
                slot.handledThrowSequence = latestThrowSeq;
                currentVel.x += slot.throwVelocityX.load (std::memory_order_acquire);
                currentVel.y += slot.throwVelocityY.load (std::memory_order_acquire);
                currentVel.z += slot.throwVelocityZ.load (std::memory_order_acquire);
            }

            // P2: Attractor force
            const Vec3 attractorForce = attractorSystem.computeNetForce (
                i, currentPos, currentVel, currentMass);

            // P3: Spring restoring force (targets composedRestPos: APVTS + Layer 3)
            Vec3 springForce {};
            if (springSystem.isEnabled())
                springForce = springSystem.computeForce (
                    i, currentPos, currentVel, currentMass, composedRestPos, dt);

            // P3: Turbulence stochastic force
            const Vec3 turbForce = turbulenceSystem.computeForce (i, currentMass, dt);

            // P5: Boids steering force (snapshot already built above)
            const Vec3 boidsForce = boidsSystem.isInActiveGroup (i)
                                  ? boidsSystem.computeSteeringForce (i, currentMass)
                                  : Vec3 {};
            const Vec3 gravityForce = computeGravityVector (currentPos);
            const Vec3 containmentForce = computeContainmentForce (
                currentPos,
                currentVel,
                composedRestPos,
                currentMass,
                coordinatedAuthority,
                coordinatedActiveCount);

            // Sum all coordinated forces and inject into engine
            const Vec3 coordinatedForce
            {
                gravityForce.x + interactionForce.x + attractorForce.x + springForce.x + turbForce.x + boidsForce.x
                    + containmentForce.x,
                gravityForce.y + interactionForce.y + attractorForce.y + springForce.y + turbForce.y + boidsForce.y
                    + containmentForce.y,
                gravityForce.z + interactionForce.z + attractorForce.z + springForce.z + turbForce.z + boidsForce.z
                    + containmentForce.z
            };
            state.force = coordinatedForce;

            if (coordinatedAuthority)
            {
                const float inverseMass = 1.0f / juce::jmax (0.01f, currentMass);
                const float dragFactor = juce::jlimit (0.0f, 1.0f, 1.0f - currentDrag * dt);

                currentVel.x += coordinatedForce.x * inverseMass * dt;
                currentVel.y += coordinatedForce.y * inverseMass * dt;
                currentVel.z += coordinatedForce.z * inverseMass * dt;

                currentVel.x *= dragFactor;
                currentVel.y *= dragFactor;
                currentVel.z *= dragFactor;

                currentPos.x += currentVel.x * dt;
                currentPos.y += currentVel.y * dt;
                currentPos.z += currentVel.z * dt;

                state.position = currentPos;
                state.velocity = currentVel;
                state.collisionMask = 0;
                state.collisionEnergy = collisionSystem.isEnabled()
                    ? collisionSystem.getCollisionEnergy (i)
                    : 0.0f;

                if (wallCollisionEnabled.load (std::memory_order_acquire))
                {
                    const auto mode = static_cast<PhysicsEngine::BoundaryMode> (
                        boundaryMode.load (std::memory_order_acquire));
                    if (mode == PhysicsEngine::BoundaryMode::Hard)
                        resolveHardBoundaries (state,
                                               engine != nullptr ? engine->getElasticity() : 0.7f,
                                               0.3f,
                                               dt);
                    else if (mode == PhysicsEngine::BoundaryMode::Soft)
                        resolveSoftBoundaries (state, dt);
                }

                currentPos = state.position;
                currentVel = state.velocity;
            }

            if (engine != nullptr)
                engine->setCoordinatedForce (coordinatedForce);

            // P6: Apply collision impulse on the authoritative motion path.
            if (collisionSystem.isEnabled())
            {
                const Vec3 impulse = collisionSystem.getImpulseDelta (i);
                const float impMagSq = impulse.x * impulse.x
                                     + impulse.y * impulse.y
                                     + impulse.z * impulse.z;
                if (impMagSq > 1.0e-12f)
                {
                    if (coordinatedAuthority)
                    {
                        currentVel.x += impulse.x;
                        currentVel.y += impulse.y;
                        currentVel.z += impulse.z;
                        state.velocity = currentVel;
                    }
                    else if (engine != nullptr)
                    {
                        engine->applyCollisionImpulse (impulse);
                    }
                }
            }
            if (! coordinatedAuthority)
            {
                state.collisionMask = 0;
                state.collisionEnergy = collisionSystem.isEnabled()
                    ? collisionSystem.getCollisionEnergy (i)
                    : 0.0f;
            }

            //------------------------------------------------------------------
            // P2+P3: DSP bridge — proximity, spring LFO, turbulence jitter
            //------------------------------------------------------------------
            if (dspBridge != nullptr)
            {
                // P2: Attractor proximity → spread (inverted sigmoid)
                const float proximity = attractorSystem.getMaxProximity (currentPos);
                const bool  crossed   = attractorSystem.didCross (
                    i, prevPositions[i], currentPos);

                // map proximity [0..1] → sigmoid input [-2..2] → spread [0..1]
                const float attractorSpread = proximity > 0.0f
                    ? 1.0f / (1.0f + std::exp (-(proximity * 4.0f - 2.0f)))
                    : 0.0f;

                // P3: Spring oscillation phase → spread LFO (sine mapped to [0..1])
                const float springPhase  = springSystem.getOscillationPhase (i);
                const float springSpread = springSystem.isEnabled()
                                         ? kSpringSpreadDepth * 0.5f * (std::sin (springPhase) + 1.0f)
                                         : 0.0f;

                // P3: Turbulence magnitude → spread jitter [0..1]
                const float turbSpread = turbulenceSystem.getMagnitude (i);

                // P5: Flock density → spread (neighbor density mapped to [0..1])
                const float boidsSpread = boidsSystem.getFlockDensity (i);

                // P5: Cohesion-breakup event — event consumed here; gain-dip DSP
                //     wiring deferred to P7 (gainTransient only supports [0..1]).
                boidsSystem.consumeBreakupEvent (i);

                // Sum all spread contributions and clamp to [0..1]
                // CL-P1: choroOffset.spreadDelta is 0 until CL-P2+ subsystems run.
                const float totalSpread = juce::jlimit (0.0f, 1.0f,
                                              attractorSpread + springSpread
                                              + turbSpread + boidsSpread
                                              + choroOffset.spreadDelta);

                // P6: Collision energy → gainTransient (max with attractor crossing)
                const float collisionGainTransient = collisionSystem.isEnabled()
                    ? collisionSystem.getCollisionEnergy (i)
                    : 0.0f;

                PerEmitterDSPValues dspVals;
                dspVals.spreadMod     = totalSpread;
                dspVals.gainMod       = 0.0f;
                dspVals.gainTransient = juce::jmax (crossed ? 1.0f : 0.0f,
                                                    collisionGainTransient);

                dspBridge->publish (i, dspVals);
            }

            prevPositions[i] = currentPos;

            //------------------------------------------------------------------
            // P4: Angular physics — integrate aim direction
            //------------------------------------------------------------------
            if (angularSystem.isEnabled())
            {
                // Find the direction from this emitter to the nearest active attractor
                // so the angular system can apply torque toward it
                Vec3  toAttractorDir {};
                float minDistSq = std::numeric_limits<float>::max();

                for (int s = 0; s < AttractorSystem::kMaxSources; ++s)
                {
                    const auto& src = attractorSystem.source (s);
                    if (! src.isActive()) continue;

                    const Vec3  sPos = src.getPosition();
                    const float dx   = sPos.x - currentPos.x;
                    const float dy   = sPos.y - currentPos.y;
                    const float dz   = sPos.z - currentPos.z;
                    const float dSq  = dx * dx + dy * dy + dz * dz;

                    if (dSq < minDistSq)
                    {
                        minDistSq = dSq;
                        const float d = std::sqrt (dSq);
                        if (d > 1.0e-6f)
                        {
                            const float inv = 1.0f / d;
                            toAttractorDir = { dx * inv, dy * inv, dz * inv };
                        }
                    }
                }

                state.directivityAim = angularSystem.update (
                    i, toAttractorDir, dt,
                    angThrowThisTick, angImpulseThisTick, angResetThisTick);
            }

            // P5–P6: additional subsystem contributions inserted here.

            writeSlotState (slot, state);
        }
    }

    void writeSlotState (EmitterSlot& slot, const WorkerEmitterState& state)
    {
        const int writeIdx = 1 - slot.readIndex.load (std::memory_order_acquire);
        slot.buffers[static_cast<std::size_t> (writeIdx)] = state;
        slot.readIndex.store (writeIdx, std::memory_order_release);
    }

    static float rateHz (int index)
    {
        static constexpr float rates[] = { 30.0f, 60.0f, 120.0f, 240.0f };
        return rates[static_cast<std::size_t> (juce::jlimit (0, 3, index))];
    }

    void resolveHardBoundaries (WorkerEmitterState& state, float bounce, float surfaceFriction, float dt) const
    {
        const float halfWidth = roomWidth.load (std::memory_order_acquire) * 0.5f;
        const float halfDepth = roomDepth.load (std::memory_order_acquire) * 0.5f;
        const float minY = 0.0f;
        const float maxY = roomHeight.load (std::memory_order_acquire);

        bool collideX = false;
        bool collideY = false;
        bool collideZ = false;
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

        if (collideX || collideY || collideZ)
        {
            const float dx = state.velocity.x - velocityBefore.x;
            const float dy = state.velocity.y - velocityBefore.y;
            const float dz = state.velocity.z - velocityBefore.z;
            state.collisionEnergy = juce::jmax (state.collisionEnergy, std::sqrt (dx * dx + dy * dy + dz * dz));
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

    void resolveSoftBoundaries (WorkerEmitterState& state, float dt) const
    {
        const float halfWidth = roomWidth.load (std::memory_order_acquire) * 0.5f;
        const float halfDepth = roomDepth.load (std::memory_order_acquire) * 0.5f;
        const float maxY = roomHeight.load (std::memory_order_acquire);
        const float depth = softBoundaryDepth.load (std::memory_order_acquire);
        const float epsilon = depth * 0.01f;
        static constexpr float kSoftBase = 20.0f;

        auto bumpEnergy = [&state] (const Vec3& before)
        {
            const float dx = state.velocity.x - before.x;
            const float dy = state.velocity.y - before.y;
            const float dz = state.velocity.z - before.z;
            state.collisionEnergy = juce::jmax (state.collisionEnergy, std::sqrt (dx * dx + dy * dy + dz * dz));
        };

        {
            const Vec3 before = state.velocity;
            const float dist = std::max (state.position.x - (-halfWidth), epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.x += kSoftBase * ratio * ratio * dt;
                state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x1u);
                bumpEnergy (before);
            }
            if (state.position.x < -halfWidth)
            {
                state.position.x = -halfWidth + epsilon;
                if (state.velocity.x < 0.0f) state.velocity.x = 0.0f;
            }
        }
        {
            const Vec3 before = state.velocity;
            const float dist = std::max (halfWidth - state.position.x, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.x -= kSoftBase * ratio * ratio * dt;
                state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x1u);
                bumpEnergy (before);
            }
            if (state.position.x > halfWidth)
            {
                state.position.x = halfWidth - epsilon;
                if (state.velocity.x > 0.0f) state.velocity.x = 0.0f;
            }
        }
        {
            const Vec3 before = state.velocity;
            const float dist = std::max (state.position.y, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.y += kSoftBase * ratio * ratio * dt;
                state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x2u);
                bumpEnergy (before);
            }
            if (state.position.y < 0.0f)
            {
                state.position.y = epsilon;
                if (state.velocity.y < 0.0f) state.velocity.y = 0.0f;
            }
        }
        {
            const Vec3 before = state.velocity;
            const float dist = std::max (maxY - state.position.y, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.y -= kSoftBase * ratio * ratio * dt;
                state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x2u);
                bumpEnergy (before);
            }
            if (state.position.y > maxY)
            {
                state.position.y = maxY - epsilon;
                if (state.velocity.y > 0.0f) state.velocity.y = 0.0f;
            }
        }
        {
            const Vec3 before = state.velocity;
            const float dist = std::max (state.position.z - (-halfDepth), epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.z += kSoftBase * ratio * ratio * dt;
                state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x4u);
                bumpEnergy (before);
            }
            if (state.position.z < -halfDepth)
            {
                state.position.z = -halfDepth + epsilon;
                if (state.velocity.z < 0.0f) state.velocity.z = 0.0f;
            }
        }
        {
            const Vec3 before = state.velocity;
            const float dist = std::max (halfDepth - state.position.z, epsilon);
            if (dist < depth)
            {
                const float ratio = depth / dist;
                state.velocity.z -= kSoftBase * ratio * ratio * dt;
                state.collisionMask = static_cast<std::uint8_t> (state.collisionMask | 0x4u);
                bumpEnergy (before);
            }
            if (state.position.z > halfDepth)
            {
                state.position.z = halfDepth - epsilon;
                if (state.velocity.z > 0.0f) state.velocity.z = 0.0f;
            }
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
            case 0:
                return { 0.0f, -magnitude, 0.0f };
            case 1:
                return { 0.0f, magnitude, 0.0f };
            case 2:
            case 3:
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
            case 4:
            default:
                return { 0.0f, -magnitude, 0.0f };
        }
    }

    Vec3 computeContainmentForce (const Vec3& position,
                                  const Vec3& velocity,
                                  const Vec3& restPos,
                                  float mass,
                                  bool coordinatedAuthority,
                                  int coordinatedActiveCount) const
    {
        if (! coordinatedAuthority || coordinatedActiveCount < 2)
            return {};

        if (! collisionSystem.isEnabled() || ! wallCollisionEnabled.load (std::memory_order_acquire))
            return {};

        const auto mode = static_cast<PhysicsEngine::BoundaryMode> (boundaryMode.load (std::memory_order_acquire));
        if (mode == PhysicsEngine::BoundaryMode::Passthrough)
            return {};

        constexpr float kDeadzoneMeters = 0.2f;
        constexpr float kContainmentStiffness = 4.0f;
        constexpr float kContainmentDamping = 1.2f;

        const Vec3 offset
        {
            restPos.x - position.x,
            restPos.y - position.y,
            restPos.z - position.z
        };

        const float distanceSq = offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
        if (distanceSq <= (kDeadzoneMeters * kDeadzoneMeters))
            return {};

        const float distance = std::sqrt (distanceSq);
        const float excess = distance - kDeadzoneMeters;
        const float invDistance = distance > 1.0e-6f ? (1.0f / distance) : 0.0f;
        const Vec3 direction
        {
            offset.x * invDistance,
            offset.y * invDistance,
            offset.z * invDistance
        };

        const float clampedMass = juce::jmax (0.01f, mass);
        const float roomScale = juce::jmax (
            1.0f,
            juce::jmin (roomWidth.load (std::memory_order_acquire),
                        roomDepth.load (std::memory_order_acquire)));
        const float normalizedExcess = juce::jlimit (0.0f, 1.5f, excess / roomScale);
        const float springAccel = kContainmentStiffness * normalizedExcess;
        const float dampingAccel = kContainmentDamping;

        return {
            clampedMass * (direction.x * springAccel - velocity.x * dampingAccel),
            clampedMass * (direction.y * springAccel - velocity.y * dampingAccel),
            clampedMass * (direction.z * springAccel - velocity.z * dampingAccel)
        };
    }

    // Max spread depth contributed by spring LFO (30% of full spread range)
    static constexpr float kSpringSpreadDepth = 0.3f;

    //==========================================================================
    std::array<EmitterSlot, kMaxEmitters> slots {};
    std::array<PhysicsEngine*, kMaxEmitters> engines {};  // nullable, non-owning

    AttractorSystem      attractorSystem {};
    SpringSystem         springSystem {};
    TurbulenceSystem     turbulenceSystem {};
    AngularPhysicsSystem angularSystem {};
    BoidsSystem          boidsSystem {};
    CollisionSystem      collisionSystem {};      // P6
    ChoreographyWorker   choreographyWorker {};   // CL-P1
    PhysicsDSPBridge*    dspBridge = nullptr;

    // Per-emitter previous position for crossing detection
    Vec3 prevPositions[kMaxEmitters] {};

    std::atomic<std::uint64_t> tickCount { 0 };
    std::atomic<int>  updateRateIndex  { 1 };
    std::atomic<bool> simulationPaused { false };
    std::atomic<float> gravityMagnitude { 0.0f };
    std::atomic<int> gravityDirection { 0 };
    std::atomic<bool> wallCollisionEnabled { true };
    std::atomic<int> boundaryMode { static_cast<int> (PhysicsEngine::BoundaryMode::Hard) };
    std::atomic<float> softBoundaryDepth { 0.25f };

    std::atomic<float> roomWidth  { 6.0f };
    std::atomic<float> roomDepth  { 4.0f };
    std::atomic<float> roomHeight { 3.0f };
};
