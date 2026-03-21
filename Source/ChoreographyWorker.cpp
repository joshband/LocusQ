#include "ChoreographyWorker.h"

#include <algorithm>

const ChoreographyOffset ChoreographyWorker::kZeroOffset {};

void ChoreographyWorker::compute (float dt, int numEmitters) noexcept
{
    // Drain the audio ring buffer every tick to prevent overflow.
    // CL-P5 will replace this with AudioFeatureExtractor reads.
    audioRing.discardAll();

    // Zero all offsets first; fills in non-zero values below when enabled.
    for (auto& o : offsets)
        o = {};

    if (! enabledFlag.load (std::memory_order_acquire))
        return;

    // =========================================================================
    // CL-P2: Formation System — advance morph phase.
    // =========================================================================

    if (morphLoop)
    {
        morphPhase += morphRate * dt * morphDir;

        if (morphPingpong)
        {
            if (morphPhase >= 1.0f) { morphPhase = 1.0f; morphDir = -1.0f; }
            if (morphPhase <= 0.0f) { morphPhase = 0.0f; morphDir =  1.0f; }
        }
        else
        {
            // Non-pingpong loop: wrap at 1.0 back to 0.0.
            if (morphPhase >= 1.0f) morphPhase -= 1.0f;
            if (morphPhase < 0.0f)  morphPhase += 1.0f;
        }
    }
    else
    {
        // Non-looping: clamp at 1.0.
        morphPhase = std::min (morphPhase + morphRate * dt, 1.0f);
    }

    // =========================================================================
    // CL-P4: Beat-Sync System — detect beat boundaries; update morph; gain-dip.
    // Beat runs after CL-P2 morph advance so it can further modify morphPhase.
    // =========================================================================

    float beatGainFactor = 1.0f;   // multiplicative [0..1]; 1.0 = no change

    if (beatEnabled)
    {
        const double ppq     = transportPpq.load     (std::memory_order_relaxed);
        const double bpm     = transportBpm.load     (std::memory_order_relaxed);
        const bool   playing = transportPlaying.load (std::memory_order_relaxed);

        const BeatSyncSystem::TickResult beat =
            beatSyncSystem.compute (beatParams, ppq, bpm, playing, dt);

        beatGainFactor = beat.gainDelta;

        if (beat.beatFired)
        {
            if (beat.morphJump)
                morphPhase = beat.morphTarget;
            else
                morphPhase = std::clamp (morphPhase + beat.morphStep, 0.0f, 1.0f);
        }
    }

    // Run formation geometry once with the fully-resolved morphPhase.
    formationSystem.compute (formationParams, numEmitters, morphPhase);
    const float sdUpdated = formationSystem.getSpreadDelta();

    // =========================================================================
    // CL-P3: Path System — compute shared path position + velocity.
    // =========================================================================

    const Vec3 pathPos = pathSystem.compute (pathParams, dt);
    const Vec3 pathVel = pathSystem.getLastVelocity();

    for (int i = 0; i < numEmitters && i < kMaxEmitters; ++i)
    {
        const Vec3 formPos = formationSystem.getSlotPosition (i);
        offsets[static_cast<std::size_t> (i)].position    = { formPos.x + pathPos.x,
                                                                formPos.y + pathPos.y,
                                                                formPos.z + pathPos.z };
        offsets[static_cast<std::size_t> (i)].spreadDelta = sdUpdated;
        offsets[static_cast<std::size_t> (i)].velocity    = pathVel;
        // gainDelta: convert multiplicative [0..1] to additive dip [0..1]
        // (0 = no dip, 1 = muted; consumer applies as: gain * (1 - gainDelta)).
        offsets[static_cast<std::size_t> (i)].gainDelta   = 1.0f - beatGainFactor;
    }

    // =========================================================================
    // CL-P7: BakeRecorder — capture positions after all offset fields are set.
    // =========================================================================

    {
        const double ppq     = transportPpq.load     (std::memory_order_relaxed);
        const double bpm     = transportBpm.load     (std::memory_order_relaxed);
        const bool   playing = transportPlaying.load (std::memory_order_relaxed);
        bakeRecorder.tick (offsets.data(), numEmitters, ppq, bpm, playing);
    }
}
