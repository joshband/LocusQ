#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace locusq::shared_contracts::confidence_masking
{
inline constexpr const char* kSchemaV1 = "locusq-confidence-masking-contract-v1";

namespace fields
{
inline constexpr const char* kSchema = "schema";
inline constexpr const char* kSnapshotSeq = "snapshotSeq";
inline constexpr const char* kDistanceConfidence = "distanceConfidence";
inline constexpr const char* kOcclusionProbability = "occlusionProbability";
inline constexpr const char* kHrtfMatchQuality = "hrtfMatchQuality";
inline constexpr const char* kMaskingIndex = "maskingIndex";
inline constexpr const char* kCombinedConfidence = "combinedConfidence";
inline constexpr const char* kOverlayAlpha = "overlayAlpha";
inline constexpr const char* kOverlayBucket = "overlayBucket";
inline constexpr const char* kFallbackReason = "fallbackReason";
inline constexpr const char* kValid = "valid";
} // namespace fields

namespace bucket
{
inline constexpr const char* kLow = "low";
inline constexpr const char* kMid = "mid";
inline constexpr const char* kHigh = "high";
} // namespace bucket

namespace fallback_reason
{
inline constexpr const char* kNone = "none";
inline constexpr const char* kInactiveMode = "inactive_mode";
inline constexpr const char* kProfileMismatch = "profile_mismatch";
inline constexpr const char* kCalibrationChainFallback = "calibration_chain_fallback";
inline constexpr const char* kNonFiniteInput = "non_finite_input";
} // namespace fallback_reason

enum class OverlayBucket : int
{
    Low = 0,
    Mid = 1,
    High = 2
};

enum class FallbackReason : int
{
    None = 0,
    InactiveMode = 1,
    ProfileMismatch = 2,
    CalibrationChainFallback = 3,
    NonFiniteInput = 4
};

inline float sanitizeUnitScalar (float value, float fallback = 0.0f) noexcept
{
    if (! std::isfinite (value))
        return juce::jlimit (0.0f, 1.0f, fallback);
    return juce::jlimit (0.0f, 1.0f, value);
}

inline float computeCombinedConfidence (
    float distanceConfidence,
    float occlusionProbability,
    float hrtfMatchQuality,
    float maskingIndex) noexcept
{
    const auto sanitizedDistance = sanitizeUnitScalar (distanceConfidence, 0.0f);
    const auto sanitizedOcclusion = sanitizeUnitScalar (occlusionProbability, 1.0f);
    const auto sanitizedHrtf = sanitizeUnitScalar (hrtfMatchQuality, 0.0f);
    const auto sanitizedMasking = sanitizeUnitScalar (maskingIndex, 1.0f);

    const auto combined = 0.40f * sanitizedDistance
        + 0.30f * (1.0f - sanitizedOcclusion)
        + 0.20f * sanitizedHrtf
        + 0.10f * (1.0f - sanitizedMasking);
    return sanitizeUnitScalar (combined, 0.0f);
}

inline OverlayBucket overlayBucketForCombinedConfidence (float combinedConfidence) noexcept
{
    const auto normalizedCombined = sanitizeUnitScalar (combinedConfidence, 0.0f);

    if (normalizedCombined < 0.40f)
        return OverlayBucket::Low;
    if (normalizedCombined < 0.80f)
        return OverlayBucket::Mid;
    return OverlayBucket::High;
}

inline int sanitizeOverlayBucketIndex (int rawBucketIndex) noexcept
{
    return juce::jlimit (
        static_cast<int> (OverlayBucket::Low),
        static_cast<int> (OverlayBucket::High),
        rawBucketIndex);
}

inline int sanitizeFallbackReasonIndex (int rawReasonIndex) noexcept
{
    return juce::jlimit (
        static_cast<int> (FallbackReason::None),
        static_cast<int> (FallbackReason::NonFiniteInput),
        rawReasonIndex);
}

inline const char* overlayBucketToString (int rawBucketIndex) noexcept
{
    switch (static_cast<OverlayBucket> (sanitizeOverlayBucketIndex (rawBucketIndex)))
    {
        case OverlayBucket::Low: return bucket::kLow;
        case OverlayBucket::Mid: return bucket::kMid;
        case OverlayBucket::High: return bucket::kHigh;
        default: break;
    }

    return bucket::kLow;
}

inline const char* fallbackReasonToString (int rawReasonIndex) noexcept
{
    switch (static_cast<FallbackReason> (sanitizeFallbackReasonIndex (rawReasonIndex)))
    {
        case FallbackReason::None: return fallback_reason::kNone;
        case FallbackReason::InactiveMode: return fallback_reason::kInactiveMode;
        case FallbackReason::ProfileMismatch: return fallback_reason::kProfileMismatch;
        case FallbackReason::CalibrationChainFallback: return fallback_reason::kCalibrationChainFallback;
        case FallbackReason::NonFiniteInput: return fallback_reason::kNonFiniteInput;
        default: break;
    }

    return fallback_reason::kNone;
}
} // namespace locusq::shared_contracts::confidence_masking
