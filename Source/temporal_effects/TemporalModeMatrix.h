#pragma once

#include "TemporalEffectContracts.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace locusq::temporal
{
struct TemporalModeContractRow
{
    TemporalMode mode = TemporalMode::Delay;
    std::string_view modeId = "delay";
    std::int32_t defaultDelayNoteDivisor = 4;
    bool requiresRunawayGuard = true;
    bool deterministicTransportRecall = true;
};

inline constexpr std::array<TemporalModeContractRow, 4> kTemporalModeContractMatrix {{
    { TemporalMode::Delay, "delay", 4, true, true },
    { TemporalMode::EchoPingPong, "echo_ping_pong", 3, true, true },
    { TemporalMode::Looper, "looper", 1, true, true },
    { TemporalMode::Frippertronics, "frippertronics", 1, true, true }
}};

} // namespace locusq::temporal
