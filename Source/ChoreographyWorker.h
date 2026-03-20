#pragma once

#include "AudioRingBuffer.h"
#include "SceneGraph.h"

#include <array>
#include <atomic>

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
 * Phase CL-P1: infrastructure only.
 *   - Zero-offset bypass when disabled.
 *   - AudioRingBuffer plumbing for future feature extraction (CL-P5).
 *   - choro_enable gate wired from APVTS.
 * Subsystems (FormationSystem, PathSystem, BeatSyncSystem, BakeRecorder)
 * are added in phases CL-P2 through CL-P7.
 *
 * Threading contract:
 *   - compute()        — PhysicsWorker thread only.
 *   - getOffset()      — PhysicsWorker thread only (valid after compute() returns).
 *   - setEnabled()     — any thread (atomic store).
 *   - pushAudioBlock() — audio thread only (single producer into AudioRingBuffer).
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
    // Worker-thread interface

    /** Compute per-emitter choreography offsets for this tick.
     *  Called on the PhysicsWorker thread at the start of each tick, before
     *  physics integration (ADR-0020 step 3).
     *
     *  CL-P1: drains the audio ring and zeroes all offsets.
     *  CL-P2+: subsystems will write non-zero offsets when enabled. */
    void compute (float dt) noexcept;

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
    std::atomic<bool>                          enabledFlag { false };
    std::array<ChoreographyOffset, kMaxEmitters> offsets  {};

    AudioRingBuffer<4096, 2> audioRing {};

    static const ChoreographyOffset kZeroOffset;
};
