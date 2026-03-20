#include "ChoreographyWorker.h"

const ChoreographyOffset ChoreographyWorker::kZeroOffset {};

void ChoreographyWorker::compute (float /*dt*/) noexcept
{
    // Drain the audio ring buffer every tick to prevent overflow.
    // CL-P5 will replace this with AudioFeatureExtractor reads.
    audioRing.discardAll();

    // Zero all offsets. When disabled, return immediately.
    // CL-P2..CL-P4 subsystems will write formation/path/beat-sync offsets here.
    for (auto& o : offsets)
        o = {};
}
