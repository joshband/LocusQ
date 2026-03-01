#pragma once

// Canonical wire contract keys for headphone calibration path/stage payloads.
namespace locusq::shared_contracts::headphone_calibration
{
inline constexpr const char* kSchemaV1 = "locusq-headphone-calibration-contract-v1";

namespace fields
{
inline constexpr const char* kSchema = "schema";
inline constexpr const char* kRequested = "requested";
inline constexpr const char* kActive = "active";
inline constexpr const char* kStage = "stage";
inline constexpr const char* kFallbackReady = "fallbackReady";
inline constexpr const char* kFallbackReason = "fallbackReason";
} // namespace fields

namespace path
{
inline constexpr const char* kSpeakers = "speakers";
inline constexpr const char* kStereoDownmix = "stereo_downmix";
inline constexpr const char* kSteamBinaural = "steam_binaural";
inline constexpr const char* kVirtualBinaural = "virtual_binaural";
} // namespace path

namespace stage
{
inline constexpr const char* kDirect = "direct";
inline constexpr const char* kReady = "ready";
inline constexpr const char* kInitializing = "initializing";
inline constexpr const char* kFallback = "fallback";
inline constexpr const char* kUnavailable = "unavailable";
} // namespace stage

namespace fallback_reason
{
inline constexpr const char* kNone = "none";
inline constexpr const char* kSteamUnavailable = "steam_unavailable";
inline constexpr const char* kOutputIncompatible = "output_incompatible";
inline constexpr const char* kMonitoringPathBypassed = "monitoring_path_bypassed";
} // namespace fallback_reason
} // namespace locusq::shared_contracts::headphone_calibration
