#pragma once

#include "temporal_effects/TemporalEffectContracts.h"

#include <cmath>
#include <cstdint>

namespace locusq::dsp
{
struct TemporalRealtimeFrameState
{
    std::int64_t samplePosition = 0;
    std::uint32_t loopWriteOffset = 0;
    float feedbackRequest = 0.0f;
    float wetSample = 0.0f;
};

inline bool isTemporalFrameStateFinite (const TemporalRealtimeFrameState& state,
                                        std::uint32_t maxLoopSamples) noexcept
{
    if (maxLoopSamples == 0)
        return false;

    if (state.samplePosition < 0)
        return false;

    if (state.loopWriteOffset >= maxLoopSamples)
        return false;

    return std::isfinite (state.feedbackRequest) && std::isfinite (state.wetSample);
}

inline float clampTemporalFeedbackForFrame (float requestedFeedback) noexcept
{
    return temporal::clampFeedbackCoefficient (requestedFeedback);
}

inline float sanitizeTemporalWetSample (float wetSample) noexcept
{
    return temporal::sanitizeAudioSample (wetSample);
}

} // namespace locusq::dsp
