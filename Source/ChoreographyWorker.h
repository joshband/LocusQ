#pragma once

#include "AudioRingBuffer.h"
#include "FormationSystem.h"
#include "SceneGraph.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

//==============================================================================
/**
 * ChoreographyOffset - per-emitter generative contribution for one worker tick.
 *
 * All fields are additive on the composed rest pose (ADR-0020 Layer 3):
 *   position    — added to the APVTS + Timeline rest pose before physics integration.
 *   spreadDelta — additive spread contribution clamped to [0..1] before EmitterSlot write.
 *   gainDelta   — additive gain contribution (e.g. teleport dip envelope) [0..1].
 *   velocity    — path velocity forwarded as Doppler source.
 */
struct ChoreographyOffset
{
    Vec3  position    {};           // additive position offset (metres)
    float spreadDelta = 0.0f;      // additive spread contribution
    float gainDelta   = 0.0f;      // additive gain contribution
    Vec3  velocity    {};           // path velocity (Doppler source)
};

//==============================================================================
/**
 * ChoreographyWorker - ADR-0020 Layer 3 generative motion module.
 *
 * Runs colocated within the PhysicsWorker tick — no separate OS thread.
 * Called once per tick via compute() before physics integration (step 3 of
 * the ADR-0020 tick sequence).
 *
 * Threading contract:
 *   - compute()                  — PhysicsWorker thread only.
 *   - getOffset()                — PhysicsWorker thread only (valid after compute() returns).
 *   - setEnabled()               — any thread (atomic store).
 *   - setFormation*() setters    — audio thread only (called from publishEmitterState()).
 *   - pushAudioBlock()           — audio thread only (single producer into AudioRingBuffer).
 *
 * Phase history:
 *   CL-P1: infrastructure (AudioRingBuffer, choro_enable gate, zero-offset bypass).
 *   CL-P2: FormationSystem integrated; morph animation; spread delta.
 */
class ChoreographyWorker
{
public:
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Control interface (any thread)

    /** Enable or disable choreography computation.
     *  When false, compute() produces zero offsets with no side effects. */
    void setEnabled (bool enabled) noexcept
    {
        enabledFlag.store (enabled, std::memory_order_release);
    }

    bool isEnabled() const noexcept
    {
        return enabledFlag.load (std::memory_order_acquire);
    }

    //==========================================================================
    // Formation param setters (audio thread — called from publishEmitterState())
    // Values are stored and consumed on the worker thread in compute().

    void setFormationType      (int   v) noexcept { formationParams.type        = static_cast<FormationType>  (std::clamp (v, 0, 6)); }
    void setFormationAxis      (int   v) noexcept { formationParams.axis        = static_cast<FormationAxis>  (std::clamp (v, 0, 2)); }
    void setFormationPlane     (int   v) noexcept { formationParams.plane       = static_cast<FormationPlane> (std::clamp (v, 0, 2)); }
    void setFormationRadius    (float v) noexcept { formationParams.radius      = std::max (0.1f, v); }
    void setFormationSpacing   (float v) noexcept { formationParams.spacing     = std::max (0.1f, v); }
    void setFormationArcAngle  (float v) noexcept { formationParams.arcAngle    = std::clamp (v, 0.0f, 360.0f); }
    void setFormationPhaseOffset(float v) noexcept { formationParams.phaseOffset = std::clamp (v, 0.0f, 360.0f); }
    void setFormationRows      (float v) noexcept { formationParams.rows        = std::clamp (static_cast<int> (std::lround (v)), 1, 16); }
    void setFormationCols      (float v) noexcept { formationParams.cols        = std::clamp (static_cast<int> (std::lround (v)), 1, 16); }
    void setFormationSpacingX  (float v) noexcept { formationParams.spacingX    = std::max (0.1f, v); }
    void setFormationSpacingZ  (float v) noexcept { formationParams.spacingZ    = std::max (0.1f, v); }
    void setFormationTurns     (float v) noexcept { formationParams.turns       = std::clamp (v, 0.5f, 8.0f); }
    void setFormationHeightRise(float v) noexcept { formationParams.heightRise  = v; }
    void setFormationMorphRate (float v) noexcept { morphRate                   = std::clamp (v, 0.01f, 10.0f); }
    void setFormationMorphLoop (bool  v) noexcept { morphLoop                   = v; }
    void setFormationMorphPingpong(bool v) noexcept { morphPingpong             = v; }

    //==========================================================================
    // Worker-thread interface

    /** Compute per-emitter choreography offsets for this tick.
     *  Called on the PhysicsWorker thread at the start of each tick, before
     *  physics integration (ADR-0020 step 3).
     *
     *  numEmitters: count of currently active emitter slots (from PhysicsWorker).
     *  CL-P1: drains audio ring, zeroes offsets.
     *  CL-P2: runs FormationSystem, writes position offsets and spread delta. */
    void compute (float dt, int numEmitters) noexcept;

    /** Return the choreography offset for emitter index.
     *  Only valid on the PhysicsWorker thread after the current tick's compute(). */
    const ChoreographyOffset& getOffset (int index) const noexcept
    {
        if (index < 0 || index >= kMaxEmitters)
            return kZeroOffset;
        return offsets[static_cast<std::size_t> (index)];
    }

    //==========================================================================
    // Audio-thread interface

    /** Write audio block data into the ring buffer.
     *  Called from the audio thread per processBlock. Non-blocking (drops frames
     *  on overflow). Used by CL-P5 AudioFeatureExtractor. */
    void pushAudioBlock (const float* const* channelData,
                         int numChannels,
                         int numFrames) noexcept
    {
        audioRing.write (channelData, numChannels, numFrames);
    }

private:
    std::atomic<bool>                            enabledFlag { false };
    std::array<ChoreographyOffset, kMaxEmitters> offsets     {};

    AudioRingBuffer<4096, 2> audioRing {};

    // CL-P2: Formation system + params + morph state.
    // Written by audio thread (setFormation*), read by worker thread (compute()).
    FormationSystem formationSystem {};
    FormationParams formationParams {};

    float morphPhase    = 0.0f;    // [0..1] current morph position
    float morphRate     = 1.0f;    // Hz (cycles per second)
    float morphDir      = 1.0f;    // +1.0 or -1.0 (ping-pong direction)
    bool  morphLoop     = false;
    bool  morphPingpong = false;

    static const ChoreographyOffset kZeroOffset;
};
