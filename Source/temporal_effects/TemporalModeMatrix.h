#pragma once

#include "TemporalEffectContracts.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace locusq::temporal
{
enum class TemporalBufferTopology : std::uint8_t
{
    DelayLine = 0,
    PingPongDelay,
    LoopBuffer,
    LongFeedbackLoop
};

struct TemporalModeContractRow
{
    TemporalMode mode = TemporalMode::Delay;
    std::string_view modeId = "delay";
    TemporalBufferTopology topology = TemporalBufferTopology::DelayLine;
    std::int32_t defaultDelayNoteDivisor = 4;
    std::int32_t maxDelayMilliseconds = kTemporalMaxDelayMilliseconds;
    std::int32_t maxLoopMilliseconds = 0;
    float defaultFeedbackCoefficient = 0.35f;
    bool requiresRunawayGuard = true;
    bool deterministicTransportRecall = true;
    bool supportsPingPong = false;
    bool supportsOverdub = false;
    bool quantizeToBarStartByDefault = true;
};

inline constexpr std::array<TemporalModeContractRow, 4> kTemporalModeContractMatrix {{
    { TemporalMode::Delay, "delay", TemporalBufferTopology::DelayLine, 4, 8000, 0, 0.35f, true, true, false, false, true },
    { TemporalMode::EchoPingPong, "echo_ping_pong", TemporalBufferTopology::PingPongDelay, 3, 8000, 0, 0.55f, true, true, true, false, true },
    { TemporalMode::Looper, "looper", TemporalBufferTopology::LoopBuffer, 1, 4000, 120000, 0.70f, true, true, false, true, true },
    { TemporalMode::Frippertronics, "frippertronics", TemporalBufferTopology::LongFeedbackLoop, 1, 8000, 120000, 0.82f, true, true, false, true, true }
}};

inline constexpr const TemporalModeContractRow* findTemporalModeContract (TemporalMode mode) noexcept
{
    for (const auto& row : kTemporalModeContractMatrix)
        if (row.mode == mode)
            return &row;

    return nullptr;
}

inline constexpr bool modeUsesLoopBuffer (const TemporalModeContractRow& row) noexcept
{
    return row.maxLoopMilliseconds > 0;
}

inline bool isTemporalModeContractSane (const TemporalModeContractRow& row) noexcept
{
    if (row.modeId != modeToId (row.mode))
        return false;

    if (row.defaultDelayNoteDivisor <= 0)
        return false;

    if (row.maxDelayMilliseconds <= 0
        || row.maxDelayMilliseconds > kTemporalMaxDelayMilliseconds)
        return false;

    if (row.maxLoopMilliseconds < 0
        || row.maxLoopMilliseconds > kTemporalMaxLoopMilliseconds)
        return false;

    if (row.defaultFeedbackCoefficient < 0.0f
        || row.defaultFeedbackCoefficient > kTemporalFeedbackSoftCeiling)
        return false;

    if (row.supportsPingPong && row.topology != TemporalBufferTopology::PingPongDelay)
        return false;

    if (row.supportsOverdub && ! modeUsesLoopBuffer (row))
        return false;

    return true;
}

inline std::uint32_t maxDelaySamplesForMode (const TemporalModeContractRow& row,
                                             double sampleRate) noexcept
{
    return millisecondsToSamples (row.maxDelayMilliseconds, sampleRate);
}

inline std::uint32_t maxBufferSamplesForMode (const TemporalModeContractRow& row,
                                              double sampleRate) noexcept
{
    return modeUsesLoopBuffer (row)
        ? millisecondsToSamples (row.maxLoopMilliseconds, sampleRate)
        : maxDelaySamplesForMode (row, sampleRate);
}

} // namespace locusq::temporal
