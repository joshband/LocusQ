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
    // CL-P2: Formation System — compute per-slot positions + spread delta.
    // =========================================================================

    // Advance morph phase.
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

    // Run the formation geometry.
    formationSystem.compute (formationParams, numEmitters, morphPhase);

    // Publish formation position offsets and spread delta per emitter.
    const float sd = formationSystem.getSpreadDelta();

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
        offsets[static_cast<std::size_t> (i)].spreadDelta = sd;
        offsets[static_cast<std::size_t> (i)].velocity    = pathVel;
    }
}
