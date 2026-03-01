#pragma once

#include <cstdint>

// Canonical operation/outcome enums for lock-free registration diagnostics and payloads.
namespace locusq::shared_contracts::registration_lock_free
{
inline constexpr const char* kSchemaV1 = "locusq-registration-lock-free-contract-v1";

enum class Operation : std::uint8_t
{
    None = 0,
    ClaimEmitter = 1,
    ReleaseEmitter = 2,
    ClaimRenderer = 3,
    ReleaseRenderer = 4
};

enum class Outcome : std::uint8_t
{
    Success = 0,
    Noop = 1,
    Contention = 2,
    ReleaseIncomplete = 3,
    StateDrift = 4
};

inline const char* operationToString (Operation operation) noexcept
{
    switch (operation)
    {
        case Operation::None: return "none";
        case Operation::ClaimEmitter: return "claim_emitter";
        case Operation::ReleaseEmitter: return "release_emitter";
        case Operation::ClaimRenderer: return "claim_renderer";
        case Operation::ReleaseRenderer: return "release_renderer";
        default: break;
    }

    return "none";
}

inline const char* outcomeToString (Outcome outcome) noexcept
{
    switch (outcome)
    {
        case Outcome::Success: return "success";
        case Outcome::Noop: return "noop";
        case Outcome::Contention: return "contention";
        case Outcome::ReleaseIncomplete: return "release_incomplete";
        case Outcome::StateDrift: return "state_drift";
        default: break;
    }

    return "success";
}

inline bool isFailure (Outcome outcome) noexcept
{
    return outcome == Outcome::Contention || outcome == Outcome::ReleaseIncomplete;
}

inline bool isContention (Outcome outcome) noexcept
{
    return outcome == Outcome::Contention;
}
} // namespace locusq::shared_contracts::registration_lock_free
