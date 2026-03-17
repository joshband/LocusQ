#pragma once

#include "temporal_effects/TemporalModeMatrix.h"

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

struct TemporalFrameContractEvaluation
{
    bool frameStateFinite = false;
    std::uint32_t maxAddressableSamples = 0;
    float clampedFeedback = 0.0f;
    float sanitizedWetSample = 0.0f;
    std::uint32_t automationRampSamples = 0;
    std::uint32_t clickSafeRampSamples = 0;
    std::uint64_t recallToken = 0;
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

inline temporal::TransportRecallSnapshot makeTemporalRecallSnapshot (const TemporalRealtimeFrameState& state,
                                                                     std::uint32_t overdubGeneration,
                                                                     bool transportPlaying,
                                                                     bool quantizeToBarStart,
                                                                     std::int64_t hostBarStartSample = 0) noexcept
{
    temporal::TransportRecallSnapshot snapshot;
    snapshot.hostSamplePosition = state.samplePosition;
    snapshot.hostBarStartSample = hostBarStartSample;
    snapshot.loopWriteOffset = state.loopWriteOffset;
    snapshot.overdubGeneration = overdubGeneration;
    snapshot.transportPlaying = transportPlaying;
    snapshot.quantizeToBarStart = quantizeToBarStart;
    return snapshot;
}

inline TemporalFrameContractEvaluation evaluateTemporalFrameContract (const TemporalRealtimeFrameState& state,
                                                                      temporal::TemporalMode mode,
                                                                      double sampleRate,
                                                                      std::uint32_t overdubGeneration = 0,
                                                                      bool transportPlaying = true,
                                                                      bool quantizeToBarStart = true,
                                                                      std::int64_t hostBarStartSample = 0) noexcept
{
    const auto* row = temporal::findTemporalModeContract (mode);
    const auto maxAddressableSamples = row != nullptr
        ? temporal::maxBufferSamplesForMode (*row, sampleRate)
        : 0u;
    const auto safetyEnvelope = temporal::evaluateTemporalSafetyEnvelope (state.feedbackRequest,
                                                                          state.wetSample);
    const auto recallSnapshot = makeTemporalRecallSnapshot (state,
                                                            overdubGeneration,
                                                            transportPlaying,
                                                            quantizeToBarStart,
                                                            hostBarStartSample);

    TemporalFrameContractEvaluation evaluation;
    evaluation.frameStateFinite = isTemporalFrameStateFinite (state, maxAddressableSamples);
    evaluation.maxAddressableSamples = maxAddressableSamples;
    evaluation.clampedFeedback = safetyEnvelope.clampedFeedback;
    evaluation.sanitizedWetSample = safetyEnvelope.sanitizedWetSample;
    evaluation.automationRampSamples = temporal::automationSlewSamples (sampleRate);
    evaluation.clickSafeRampSamples = temporal::clickSafeRampSamples (sampleRate);
    evaluation.recallToken = temporal::deterministicRecallToken (recallSnapshot);
    return evaluation;
}

} // namespace locusq::dsp
