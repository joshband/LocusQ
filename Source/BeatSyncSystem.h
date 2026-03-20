#pragma once

#include <algorithm>
#include <array>
#include <cmath>

enum class BeatDivision : int
{
    Whole      = 0,   // 4 PPQ (one bar in 4/4)
    Half       = 1,   // 2 PPQ
    Quarter    = 2,   // 1 PPQ
    Eighth     = 3,   // 0.5 PPQ
    Sixteenth  = 4    // 0.25 PPQ
};

enum class BeatMode : int
{
    Snap     = 0,   // immediate formation step; no gain change
    Glide    = 1,   // reserved: interpolate toward target (future)
    Teleport = 2    // instant formation step + gain-dip envelope
};

enum class BeatStepAction : int
{
    Hold    = 0,   // maintain current morph phase
    Advance = 1,   // increment morph phase by 1/kNumSteps
    Jump    = 2    // jump morph phase to targetSlot / kNumSteps
};

struct BeatStep
{
    BeatStepAction action     = BeatStepAction::Advance;
    int            targetSlot = 0;   // used for Jump; clamped to [0, kNumSteps-1]
};

struct BeatSyncParams
{
    BeatDivision division = BeatDivision::Quarter;
    BeatMode     mode     = BeatMode::Snap;
    float        dipDb    = -18.0f;   // ≤ 0 dB; depth of gain dip on Teleport
    float        decayMs  = 100.0f;   // > 0 ms; one-pole recovery time
};

//==============================================================================
/**
 * BeatSyncSystem — ADR-0020 Layer 3 beat-quantization module.
 *
 * Detects beat-boundary crossings from DAW PPQ position and fires
 * per-beat events that drive step-sequencer advancement and mode behavior.
 *
 * Threading contract:
 *   - compute()     — PhysicsWorker thread only.
 *   - setPattern()  — audio thread only (called from publishEmitterState()).
 *   - reset()       — audio thread only (called from prepareToPlay()).
 *
 * Gain convention:
 *   TickResult::gainDelta is a multiplicative factor in [0..1].
 *   1.0 = no change; 0.0 = fully muted.
 *   Caller converts to ChoreographyOffset.gainDelta via (1.0 - gainDelta).
 */
class BeatSyncSystem
{
public:
    static constexpr int kNumSteps = 16;

    /** Per-tick result returned by compute(). */
    struct TickResult
    {
        bool  beatFired   = false;   // true when a beat boundary was crossed
        int   step        = 0;       // current step index (0..kNumSteps-1)
        float gainDelta   = 1.0f;    // multiplicative gain factor [0..1]; 1.0 = no change
        float morphStep   = 0.0f;    // additive morph phase delta from Advance step
        bool  morphJump   = false;   // true when step action = Jump
        float morphTarget = 0.0f;    // jump target morph phase [0..1]
    };

    /** Compute one worker tick.
     *  ppqPosition: DAW playhead in PPQ (quarter-note units).
     *  bpm:         current tempo in BPM (≥ 1.0).
     *  isPlaying:   true when DAW transport is running.
     *  dt:          tick duration in seconds (> 0). */
    TickResult compute(const BeatSyncParams& params,
                       double ppqPosition, double bpm,
                       bool isPlaying, float dt) noexcept;

    /** Reset internal state (call from prepareToPlay). */
    void reset() noexcept;

    /** Set the 16-step pattern. Audio-thread safe (plain write; pattern is
     *  written only from the audio thread and read only from worker thread). */
    void setPattern(const std::array<BeatStep, kNumSteps>& p) noexcept
    {
        pattern = p;
    }

private:
    double  lastPpq     = -1.0;   // -1 sentinel = not yet initialized
    int     currentStep = 0;
    float   dipGain     = 1.0f;   // current gain factor; 1.0 = no dip

    std::array<BeatStep, kNumSteps> pattern {};   // default: all Advance

    // Returns the beat division interval in PPQ units.
    static double divisionInPPQ(BeatDivision d) noexcept;

    // Returns the gain dip target as a linear factor in [0..1].
    // dipDb must be ≤ 0; clamped to [-60, 0] before conversion.
    static float dipTarget(float dipDb) noexcept;
};
