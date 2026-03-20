#include "BeatSyncSystem.h"

// ────────────────────────────────────────────────────────────────
//  Static helpers
// ────────────────────────────────────────────────────────────────

double BeatSyncSystem::divisionInPPQ(BeatDivision d) noexcept
{
    switch (d)
    {
        case BeatDivision::Whole:     return 4.0;
        case BeatDivision::Half:      return 2.0;
        case BeatDivision::Quarter:   return 1.0;
        case BeatDivision::Eighth:    return 0.5;
        case BeatDivision::Sixteenth: return 0.25;
        default:                      return 1.0;
    }
}

float BeatSyncSystem::dipTarget(float dipDb) noexcept
{
    const float clamped = std::clamp(dipDb, -60.0f, 0.0f);
    return std::pow(10.0f, clamped / 20.0f);
}

// ────────────────────────────────────────────────────────────────
//  Reset
// ────────────────────────────────────────────────────────────────

void BeatSyncSystem::reset() noexcept
{
    lastPpq     = -1.0;
    currentStep = 0;
    dipGain     = 1.0f;
}

// ────────────────────────────────────────────────────────────────
//  compute() — called on PhysicsWorker thread each tick
// ────────────────────────────────────────────────────────────────

BeatSyncSystem::TickResult BeatSyncSystem::compute(
    const BeatSyncParams& params,
    double ppqPosition, double bpm,
    bool isPlaying, float dt) noexcept
{
    TickResult result;
    result.step = currentStep;

    // Always decay the gain envelope each tick (even when stopped).
    const float tau   = std::max(params.decayMs, 1.0f) / 1000.0f;
    const float alpha = 1.0f - std::exp(-dt / tau);
    dipGain += (1.0f - dipGain) * alpha;
    dipGain  = std::clamp(dipGain, 0.0f, 1.0f);

    result.gainDelta = dipGain;

    if (!isPlaying || bpm < 1.0 || dt <= 0.0f)
    {
        // Transport stopped: clear PPQ history; no beat fires.
        lastPpq = -1.0;
        return result;
    }

    const double divQN = divisionInPPQ(params.division);

    // ── Beat boundary detection ──────────────────────────────────
    // A boundary is crossed when the integer division-grid count increases.
    // This handles DAW loopback correctly: if ppqPosition < lastPpq we reset.
    bool beatFired = false;

    if (lastPpq >= 0.0 && ppqPosition >= lastPpq)
    {
        const double prevGrid = std::floor(lastPpq     / divQN);
        const double currGrid = std::floor(ppqPosition / divQN);
        beatFired = (currGrid > prevGrid);
    }
    // else: first tick or loop-back — skip; no beat fire.

    lastPpq = ppqPosition;

    if (!beatFired)
        return result;

    result.beatFired = true;

    // ── Step-sequencer advancement ───────────────────────────────
    const BeatStep& step = pattern[static_cast<std::size_t>(currentStep)];

    switch (step.action)
    {
        case BeatStepAction::Hold:
            // No morph change.
            break;

        case BeatStepAction::Advance:
            result.morphStep = 1.0f / static_cast<float>(kNumSteps);
            break;

        case BeatStepAction::Jump:
        {
            const int t = std::clamp(step.targetSlot, 0, kNumSteps - 1);
            result.morphJump   = true;
            result.morphTarget = static_cast<float>(t) / static_cast<float>(kNumSteps);
            break;
        }
    }

    // Advance step index (wraps at 16).
    currentStep = (currentStep + 1) % kNumSteps;
    result.step = currentStep;

    // ── Mode behavior ────────────────────────────────────────────
    if (params.mode == BeatMode::Teleport)
    {
        // Fire gain-dip: snap to dip target; decay runs every tick above.
        dipGain = dipTarget(params.dipDb);
        result.gainDelta = dipGain;
    }
    // Snap and Glide: no gain change on beat (gainDelta stays decayed value).

    return result;
}
