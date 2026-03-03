#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace locusq::temporal
{
inline constexpr int kTemporalContractVersion = 1;
inline constexpr double kTemporalReferenceSampleRate = 48000.0;
inline constexpr int kTemporalMaxDelayMilliseconds = 8000;
inline constexpr int kTemporalMaxLoopMilliseconds = 120000;

inline constexpr float kTemporalFeedbackSoftCeiling = 0.925f;
inline constexpr float kTemporalFeedbackSafetyCeiling = 0.975f;
inline constexpr float kTemporalFeedbackRunawayDamp = 0.85f;
inline constexpr float kTemporalFiniteOutputGuardAbs = 8.0f;
inline constexpr float kTemporalAutomationSlewMs = 20.0f;
inline constexpr float kTemporalClickSafeRampMs = 15.0f;

inline constexpr double kTemporalCpuBudgetPct44k1 = 8.0;
inline constexpr int kTemporalLatencyBudgetSamples44k1 = 64;
inline constexpr double kTemporalCpuBudgetPct192k = 18.0;
inline constexpr int kTemporalLatencyBudgetSamples192k = 256;

enum class TemporalMode : std::uint8_t
{
    Delay = 0,
    EchoPingPong,
    Looper,
    Frippertronics
};

inline constexpr std::array<TemporalMode, 4> kTemporalModes {
    TemporalMode::Delay,
    TemporalMode::EchoPingPong,
    TemporalMode::Looper,
    TemporalMode::Frippertronics
};

inline constexpr std::string_view modeToId (TemporalMode mode) noexcept
{
    switch (mode)
    {
        case TemporalMode::Delay: return "delay";
        case TemporalMode::EchoPingPong: return "echo_ping_pong";
        case TemporalMode::Looper: return "looper";
        case TemporalMode::Frippertronics: return "frippertronics";
    }

    return "unknown";
}

inline float clampFeedbackCoefficient (float requestedCoefficient) noexcept
{
    if (! std::isfinite (requestedCoefficient) || requestedCoefficient <= 0.0f)
        return 0.0f;

    float bounded = std::min (requestedCoefficient, kTemporalFeedbackSafetyCeiling);
    if (bounded > kTemporalFeedbackSoftCeiling)
        bounded = kTemporalFeedbackSoftCeiling
            + ((bounded - kTemporalFeedbackSoftCeiling) * kTemporalFeedbackRunawayDamp);

    return std::clamp (bounded, 0.0f, kTemporalFeedbackSafetyCeiling);
}

inline float sanitizeAudioSample (float sample) noexcept
{
    if (! std::isfinite (sample))
        return 0.0f;

    return std::clamp (sample,
                       -kTemporalFiniteOutputGuardAbs,
                       kTemporalFiniteOutputGuardAbs);
}

struct TransportRecallSnapshot
{
    std::int64_t hostSamplePosition = 0;
    std::int64_t hostBarStartSample = 0;
    std::uint32_t loopWriteOffset = 0;
    std::uint32_t overdubGeneration = 0;
    bool transportPlaying = false;
    bool quantizeToBarStart = true;
};

inline constexpr std::uint64_t mixRecallToken (std::uint64_t token,
                                                std::uint64_t value) noexcept
{
    constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ull;
    constexpr std::uint64_t kFnvPrime = 1099511628211ull;
    if (token == 0)
        token = kFnvOffsetBasis;

    token ^= value;
    token *= kFnvPrime;
    return token;
}

inline std::uint64_t deterministicRecallToken (const TransportRecallSnapshot& snapshot) noexcept
{
    std::uint64_t token = 0;
    token = mixRecallToken (token, static_cast<std::uint64_t> (snapshot.hostSamplePosition));
    token = mixRecallToken (token, static_cast<std::uint64_t> (snapshot.hostBarStartSample));
    token = mixRecallToken (token, snapshot.loopWriteOffset);
    token = mixRecallToken (token, snapshot.overdubGeneration);
    token = mixRecallToken (token, snapshot.transportPlaying ? 1u : 0u);
    token = mixRecallToken (token, snapshot.quantizeToBarStart ? 1u : 0u);
    return token;
}

} // namespace locusq::temporal
