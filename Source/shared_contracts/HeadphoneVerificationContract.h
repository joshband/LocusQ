#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace locusq::shared_contracts::headphone_verification
{
inline constexpr const char* kSchemaV1 = "locusq-headphone-verification-contract-v1";

namespace fields
{
inline constexpr const char* kSchema = "schema";
inline constexpr const char* kProfileId = "profileId";
inline constexpr const char* kRequestedProfileId = "requestedProfileId";
inline constexpr const char* kActiveProfileId = "activeProfileId";
inline constexpr const char* kRequestedEngineId = "requestedEngineId";
inline constexpr const char* kActiveEngineId = "activeEngineId";
inline constexpr const char* kFallbackReasonCode = "fallbackReasonCode";
inline constexpr const char* kFallbackTarget = "fallbackTarget";
inline constexpr const char* kFallbackReasonText = "fallbackReasonText";
inline constexpr const char* kFrontBackScore = "frontBackScore";
inline constexpr const char* kElevationScore = "elevationScore";
inline constexpr const char* kExternalizationScore = "externalizationScore";
inline constexpr const char* kConfidence = "confidence";
inline constexpr const char* kVerificationStage = "verificationStage";
inline constexpr const char* kLatencySamples = "latencySamples";
inline constexpr const char* kVerificationScoreStatus = "verificationScoreStatus";
} // namespace fields

namespace engine
{
inline constexpr const char* kDisabled = "disabled";
inline constexpr const char* kPeq = "peq";
inline constexpr const char* kFir = "fir";
} // namespace engine

namespace stage
{
inline constexpr const char* kDisabled = "disabled";
inline constexpr const char* kInitializing = "initializing";
inline constexpr const char* kVerified = "verified";
inline constexpr const char* kFallback = "fallback";
inline constexpr const char* kUnavailable = "unavailable";
} // namespace stage

namespace fallback_reason
{
inline constexpr const char* kNone = "none";
inline constexpr const char* kDisabledByRequest = "disabled_by_request";
inline constexpr const char* kInvalidEngineSelection = "invalid_engine_selection";
inline constexpr const char* kDspNotPrepared = "dsp_not_prepared";
inline constexpr const char* kPeqUnavailable = "peq_unavailable";
inline constexpr const char* kFirUnavailable = "fir_unavailable";
} // namespace fallback_reason

namespace fallback_target
{
inline constexpr const char* kNone = "none";
inline constexpr const char* kDisabled = "disabled";
inline constexpr const char* kPeq = "peq";
inline constexpr const char* kFir = "fir";
} // namespace fallback_target

namespace score_status
{
inline constexpr const char* kUnavailable = "unavailable";
inline constexpr const char* kInitializing = "initializing";
inline constexpr const char* kFallback = "fallback";
inline constexpr const char* kVerified = "verified";
} // namespace score_status

inline float sanitizeScore (float rawScore, float fallback = 0.0f) noexcept
{
    if (! std::isfinite (rawScore))
        return juce::jlimit (0.0f, 1.0f, fallback);

    return juce::jlimit (0.0f, 1.0f, rawScore);
}

inline int sanitizeLatencySamples (int rawSamples) noexcept
{
    return juce::jlimit (0, 16384, rawSamples);
}

inline const char* sanitizeProfileId (juce::String profileId) noexcept
{
    profileId = profileId.trim().toLowerCase();

    if (profileId == "generic")
        return "generic";
    if (profileId == "airpods_pro_2")
        return "airpods_pro_2";
    if (profileId == "sony_wh1000xm5")
        return "sony_wh1000xm5";
    if (profileId == "custom_sofa")
        return "custom_sofa";

    return "generic";
}

inline const char* sanitizeEngineId (juce::String engineId) noexcept
{
    engineId = engineId.trim().toLowerCase();

    if (engineId == engine::kPeq)
        return engine::kPeq;
    if (engineId == engine::kFir)
        return engine::kFir;

    return engine::kDisabled;
}

inline const char* sanitizeFallbackReasonCode (juce::String reasonCode) noexcept
{
    reasonCode = reasonCode.trim().toLowerCase();

    if (reasonCode == fallback_reason::kNone)
        return fallback_reason::kNone;
    if (reasonCode == fallback_reason::kDisabledByRequest)
        return fallback_reason::kDisabledByRequest;
    if (reasonCode == fallback_reason::kInvalidEngineSelection)
        return fallback_reason::kInvalidEngineSelection;
    if (reasonCode == fallback_reason::kDspNotPrepared)
        return fallback_reason::kDspNotPrepared;
    if (reasonCode == fallback_reason::kPeqUnavailable)
        return fallback_reason::kPeqUnavailable;
    if (reasonCode == fallback_reason::kFirUnavailable)
        return fallback_reason::kFirUnavailable;

    return fallback_reason::kInvalidEngineSelection;
}

inline const char* sanitizeFallbackTarget (juce::String fallbackTarget) noexcept
{
    fallbackTarget = fallbackTarget.trim().toLowerCase();

    if (fallbackTarget == fallback_target::kNone)
        return fallback_target::kNone;
    if (fallbackTarget == fallback_target::kPeq)
        return fallback_target::kPeq;
    if (fallbackTarget == fallback_target::kFir)
        return fallback_target::kFir;

    return fallback_target::kDisabled;
}

inline const char* deriveFallbackTarget (juce::String reasonCode, juce::String activeEngineId) noexcept
{
    const auto sanitizedReason = sanitizeFallbackReasonCode (reasonCode);
    if (sanitizedReason == fallback_reason::kNone)
        return fallback_target::kNone;

    const auto sanitizedEngine = sanitizeEngineId (activeEngineId);
    if (sanitizedEngine == engine::kPeq)
        return fallback_target::kPeq;
    if (sanitizedEngine == engine::kFir)
        return fallback_target::kFir;

    return fallback_target::kDisabled;
}

inline const char* sanitizeFallbackTargetForReason (
    juce::String reasonCode,
    juce::String fallbackTarget,
    juce::String activeEngineId) noexcept
{
    const auto sanitizedReason = sanitizeFallbackReasonCode (reasonCode);
    if (sanitizedReason == fallback_reason::kNone)
        return fallback_target::kNone;

    const auto sanitizedTarget = sanitizeFallbackTarget (fallbackTarget);
    if (sanitizedTarget == fallback_target::kNone)
        return deriveFallbackTarget (sanitizedReason, activeEngineId);

    return sanitizedTarget;
}

inline const char* sanitizeVerificationStage (juce::String verificationStage) noexcept
{
    verificationStage = verificationStage.trim().toLowerCase();

    if (verificationStage == stage::kDisabled)
        return stage::kDisabled;
    if (verificationStage == stage::kInitializing)
        return stage::kInitializing;
    if (verificationStage == stage::kVerified)
        return stage::kVerified;
    if (verificationStage == stage::kFallback)
        return stage::kFallback;
    if (verificationStage == stage::kUnavailable)
        return stage::kUnavailable;

    return stage::kUnavailable;
}

inline const char* scoreStatusFromStage (juce::String verificationStage) noexcept
{
    const auto sanitizedStage = sanitizeVerificationStage (verificationStage);

    if (sanitizedStage == stage::kVerified)
        return score_status::kVerified;
    if (sanitizedStage == stage::kFallback)
        return score_status::kFallback;
    if (sanitizedStage == stage::kInitializing)
        return score_status::kInitializing;

    return score_status::kUnavailable;
}

inline const char* fallbackReasonTextForCode (juce::String reasonCode) noexcept
{
    reasonCode = sanitizeFallbackReasonCode (reasonCode);

    if (reasonCode == fallback_reason::kNone)
        return "Requested calibration engine active.";
    if (reasonCode == fallback_reason::kDisabledByRequest)
        return "Headphone verification disabled by request.";
    if (reasonCode == fallback_reason::kInvalidEngineSelection)
        return "Requested calibration engine was invalid.";
    if (reasonCode == fallback_reason::kDspNotPrepared)
        return "Calibration DSP chain is not prepared.";
    if (reasonCode == fallback_reason::kPeqUnavailable)
        return "Requested PEQ engine unavailable; processing disabled.";
    if (reasonCode == fallback_reason::kFirUnavailable)
        return "Requested FIR engine unavailable; fallback engine selected.";

    return "Requested calibration engine was invalid.";
}

} // namespace locusq::shared_contracts::headphone_verification
