#pragma once

#include <juce_core/juce_core.h>

namespace locusq::headphone_core
{

enum class CalibrationChainEngine : int
{
    Disabled = 0,
    ParametricEq = 1,
    FirConvolution = 2
};

enum class CalibrationChainFallbackReason : int
{
    None = 0,
    DisabledByRequest = 1,
    InvalidEngineSelection = 2,
    DspNotPrepared = 3,
    PeqUnavailable = 4,
    FirUnavailable = 5
};

struct CalibrationChainRequest
{
    bool enabled = false;
    int requestedEngineIndex = static_cast<int> (CalibrationChainEngine::Disabled);
};

struct CalibrationChainResolvedState
{
    int requestedEngineIndex = static_cast<int> (CalibrationChainEngine::Disabled);
    int activeEngineIndex = static_cast<int> (CalibrationChainEngine::Disabled);
    int fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::DisabledByRequest);
    int activeLatencySamples = 0;
};

inline int sanitizeCalibrationEngineIndex (int rawIndex) noexcept
{
    return juce::jlimit (
        static_cast<int> (CalibrationChainEngine::Disabled),
        static_cast<int> (CalibrationChainEngine::FirConvolution),
        rawIndex);
}

inline int sanitizeCalibrationFallbackReasonIndex (int rawIndex) noexcept
{
    return juce::jlimit (
        static_cast<int> (CalibrationChainFallbackReason::None),
        static_cast<int> (CalibrationChainFallbackReason::FirUnavailable),
        rawIndex);
}

inline int sanitizeCalibrationLatencySamples (int rawSamples) noexcept
{
    return juce::jlimit (0, 16384, rawSamples);
}

inline const char* calibrationChainEngineToString (int engineIndex) noexcept
{
    switch (static_cast<CalibrationChainEngine> (sanitizeCalibrationEngineIndex (engineIndex)))
    {
        case CalibrationChainEngine::ParametricEq: return "peq";
        case CalibrationChainEngine::FirConvolution: return "fir";
        case CalibrationChainEngine::Disabled:
        default: break;
    }

    return "disabled";
}

inline const char* calibrationChainFallbackReasonToString (int reasonIndex) noexcept
{
    switch (static_cast<CalibrationChainFallbackReason> (sanitizeCalibrationFallbackReasonIndex (reasonIndex)))
    {
        case CalibrationChainFallbackReason::None: return "none";
        case CalibrationChainFallbackReason::DisabledByRequest: return "disabled_by_request";
        case CalibrationChainFallbackReason::InvalidEngineSelection: return "invalid_engine_selection";
        case CalibrationChainFallbackReason::DspNotPrepared: return "dsp_not_prepared";
        case CalibrationChainFallbackReason::PeqUnavailable: return "peq_unavailable";
        case CalibrationChainFallbackReason::FirUnavailable: return "fir_unavailable";
        default: break;
    }

    return "none";
}

inline CalibrationChainResolvedState resolveCalibrationChainState (
    const CalibrationChainRequest& request,
    bool chainPrepared,
    bool peqReady,
    bool firReady,
    int peqLatencySamples,
    int firLatencySamples) noexcept
{
    CalibrationChainResolvedState resolved {};
    const auto rawRequestedEngineIndex = request.requestedEngineIndex;
    const bool requestedEngineInRange =
        rawRequestedEngineIndex >= static_cast<int> (CalibrationChainEngine::Disabled)
        && rawRequestedEngineIndex <= static_cast<int> (CalibrationChainEngine::FirConvolution);

    resolved.requestedEngineIndex = sanitizeCalibrationEngineIndex (rawRequestedEngineIndex);
    resolved.activeEngineIndex = static_cast<int> (CalibrationChainEngine::Disabled);
    resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::DisabledByRequest);
    resolved.activeLatencySamples = 0;

    const auto requestedEngine = static_cast<CalibrationChainEngine> (resolved.requestedEngineIndex);
    if (! request.enabled || requestedEngine == CalibrationChainEngine::Disabled)
        return resolved;

    if (! requestedEngineInRange)
    {
        resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::InvalidEngineSelection);
        return resolved;
    }

    if (! chainPrepared)
    {
        resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::DspNotPrepared);
        return resolved;
    }

    if (requestedEngine == CalibrationChainEngine::ParametricEq)
    {
        if (peqReady)
        {
            resolved.activeEngineIndex = static_cast<int> (CalibrationChainEngine::ParametricEq);
            resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::None);
            resolved.activeLatencySamples = sanitizeCalibrationLatencySamples (peqLatencySamples);
        }
        else
        {
            resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::PeqUnavailable);
        }

        return resolved;
    }

    if (firReady)
    {
        resolved.activeEngineIndex = static_cast<int> (CalibrationChainEngine::FirConvolution);
        resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::None);
        resolved.activeLatencySamples = sanitizeCalibrationLatencySamples (firLatencySamples);
        return resolved;
    }

    if (peqReady)
    {
        resolved.activeEngineIndex = static_cast<int> (CalibrationChainEngine::ParametricEq);
        resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::FirUnavailable);
        resolved.activeLatencySamples = sanitizeCalibrationLatencySamples (peqLatencySamples);
        return resolved;
    }

    resolved.fallbackReasonIndex = static_cast<int> (CalibrationChainFallbackReason::FirUnavailable);
    return resolved;
}

} // namespace locusq::headphone_core
