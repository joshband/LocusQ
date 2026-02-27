#include "PluginProcessor.h"
#include "processor_core/ProcessorParameterReaders.h"
#include "processor_bridge/ProcessorBridgeUtilities.h"
#include "shared_contracts/BridgeStatusContract.h"
#include "shared_contracts/ConfidenceMaskingContract.h"
#include "shared_contracts/HeadphoneCalibrationContract.h"
#include "shared_contracts/HeadphoneVerificationContract.h"
#include "shared_contracts/RegistrationLockFreeContract.h"

#if ! defined (LOCUSQ_TESTING) || ! LOCUSQ_TESTING
#include "PluginEditor.h"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
static_assert (std::atomic<int>::is_always_lock_free,
               "Registration claim/release diagnostics require lockless atomics.");

const char* toCalibrationStateString (CalibrationEngine::State state)
{
    switch (state)
    {
        case CalibrationEngine::State::Idle:      return "idle";
        case CalibrationEngine::State::Playing:   return "playing";
        case CalibrationEngine::State::Recording: return "recording";
        case CalibrationEngine::State::Analyzing: return "analyzing";
        case CalibrationEngine::State::Complete:  return "complete";
        case CalibrationEngine::State::Error:     return "error";
    }

    return "unknown";
}

const char* locusQModeToString (LocusQMode mode) noexcept
{
    switch (mode)
    {
        case LocusQMode::Calibrate: return "calibrate";
        case LocusQMode::Emitter: return "emitter";
        case LocusQMode::Renderer: return "renderer";
        default: break;
    }

    return "calibrate";
}

RegistrationTransitionStage registrationTransitionStageFromCode (int code) noexcept
{
    switch (code)
    {
        case static_cast<int> (RegistrationTransitionStage::Stable): return RegistrationTransitionStage::Stable;
        case static_cast<int> (RegistrationTransitionStage::ClaimConflict): return RegistrationTransitionStage::ClaimConflict;
        case static_cast<int> (RegistrationTransitionStage::Recovered): return RegistrationTransitionStage::Recovered;
        case static_cast<int> (RegistrationTransitionStage::Ambiguous): return RegistrationTransitionStage::Ambiguous;
        default: break;
    }

    return RegistrationTransitionStage::Stable;
}

const char* registrationTransitionStageToString (RegistrationTransitionStage stage) noexcept
{
    switch (stage)
    {
        case RegistrationTransitionStage::Stable: return "stable";
        case RegistrationTransitionStage::ClaimConflict: return "claim_conflict";
        case RegistrationTransitionStage::Recovered: return "recovered";
        case RegistrationTransitionStage::Ambiguous: return "ambiguous";
        default: break;
    }

    return "stable";
}

RegistrationTransitionFallbackReason registrationTransitionFallbackReasonFromCode (int code) noexcept
{
    switch (code)
    {
        case static_cast<int> (RegistrationTransitionFallbackReason::None):
            return RegistrationTransitionFallbackReason::None;
        case static_cast<int> (RegistrationTransitionFallbackReason::EmitterSlotUnavailable):
            return RegistrationTransitionFallbackReason::EmitterSlotUnavailable;
        case static_cast<int> (RegistrationTransitionFallbackReason::RendererAlreadyClaimed):
            return RegistrationTransitionFallbackReason::RendererAlreadyClaimed;
        case static_cast<int> (RegistrationTransitionFallbackReason::StaleEmitterOwner):
            return RegistrationTransitionFallbackReason::StaleEmitterOwner;
        case static_cast<int> (RegistrationTransitionFallbackReason::DualOwnershipResolved):
            return RegistrationTransitionFallbackReason::DualOwnershipResolved;
        case static_cast<int> (RegistrationTransitionFallbackReason::RendererStateDrift):
            return RegistrationTransitionFallbackReason::RendererStateDrift;
        case static_cast<int> (RegistrationTransitionFallbackReason::ReleaseIncomplete):
            return RegistrationTransitionFallbackReason::ReleaseIncomplete;
        default:
            break;
    }

    return RegistrationTransitionFallbackReason::None;
}

const char* registrationTransitionFallbackReasonToString (RegistrationTransitionFallbackReason reason) noexcept
{
    switch (reason)
    {
        case RegistrationTransitionFallbackReason::None: return "none";
        case RegistrationTransitionFallbackReason::EmitterSlotUnavailable: return "emitter_slot_unavailable";
        case RegistrationTransitionFallbackReason::RendererAlreadyClaimed: return "renderer_already_claimed";
        case RegistrationTransitionFallbackReason::StaleEmitterOwner: return "stale_emitter_owner";
        case RegistrationTransitionFallbackReason::DualOwnershipResolved: return "dual_ownership_resolved";
        case RegistrationTransitionFallbackReason::RendererStateDrift: return "renderer_state_drift";
        case RegistrationTransitionFallbackReason::ReleaseIncomplete: return "release_incomplete";
        default: break;
    }

    return "none";
}

using RegistrationContractOperation = locusq::shared_contracts::registration_lock_free::Operation;
using RegistrationContractOutcome = locusq::shared_contracts::registration_lock_free::Outcome;

RegistrationTransitionStage registrationStageFromContractOutcome (
    RegistrationContractOutcome outcome) noexcept
{
    switch (outcome)
    {
        case RegistrationContractOutcome::Success:
        case RegistrationContractOutcome::Noop:
            return RegistrationTransitionStage::Stable;
        case RegistrationContractOutcome::Contention:
            return RegistrationTransitionStage::ClaimConflict;
        case RegistrationContractOutcome::StateDrift:
            return RegistrationTransitionStage::Recovered;
        case RegistrationContractOutcome::ReleaseIncomplete:
            return RegistrationTransitionStage::Ambiguous;
        default:
            break;
    }

    return RegistrationTransitionStage::Stable;
}

RegistrationTransitionFallbackReason registrationFallbackFromContractStep (
    RegistrationContractOperation operation,
    RegistrationContractOutcome outcome) noexcept
{
    switch (outcome)
    {
        case RegistrationContractOutcome::Contention:
            if (operation == RegistrationContractOperation::ClaimRenderer)
                return RegistrationTransitionFallbackReason::RendererAlreadyClaimed;
            return RegistrationTransitionFallbackReason::EmitterSlotUnavailable;

        case RegistrationContractOutcome::StateDrift:
            if (operation == RegistrationContractOperation::ReleaseEmitter)
                return RegistrationTransitionFallbackReason::StaleEmitterOwner;
            if (operation == RegistrationContractOperation::ReleaseRenderer)
                return RegistrationTransitionFallbackReason::RendererStateDrift;
            return RegistrationTransitionFallbackReason::RendererStateDrift;

        case RegistrationContractOutcome::ReleaseIncomplete:
            return RegistrationTransitionFallbackReason::ReleaseIncomplete;

        case RegistrationContractOutcome::Success:
        case RegistrationContractOutcome::Noop:
        default:
            break;
    }

    return RegistrationTransitionFallbackReason::None;
}

TestSignalGenerator::Type toSignalType (int typeIndex)
{
    switch (juce::jlimit (0, 3, typeIndex))
    {
        case 0: return TestSignalGenerator::Type::LogSweep;
        case 1: return TestSignalGenerator::Type::PinkNoise;
        case 2: return TestSignalGenerator::Type::WhiteNoise;
        case 3: return TestSignalGenerator::Type::Impulse;
        default: break;
    }

    return TestSignalGenerator::Type::LogSweep;
}

int toSignalTypeIndex (juce::String type)
{
    type = type.trim().toLowerCase();

    if (type == "sweep" || type == "logsweep" || type == "log_sweep")
        return 0;
    if (type == "pink" || type == "pinknoise" || type == "pink_noise")
        return 1;
    if (type == "white" || type == "whitenoise" || type == "white_noise")
        return 2;
    if (type == "impulse")
        return 3;

    return 0;
}

const juce::String kTrackPosAzimuth { "pos_azimuth" };
const juce::String kTrackPosElevation { "pos_elevation" };
const juce::String kTrackPosDistance { "pos_distance" };
const juce::String kTrackPosX { "pos_x" };
const juce::String kTrackPosY { "pos_y" };
const juce::String kTrackPosZ { "pos_z" };
const juce::String kTrackSizeUniform { "size_uniform" };

juce::String outputLayoutToString (const juce::AudioChannelSet& outputSet)
{
    if (outputSet == juce::AudioChannelSet::mono())
        return "mono";
    if (outputSet == juce::AudioChannelSet::stereo())
        return "stereo";
    if (outputSet == juce::AudioChannelSet::quadraphonic()
        || outputSet == juce::AudioChannelSet::discreteChannels (4))
    {
        return "quad";
    }

    if (outputSet == juce::AudioChannelSet::create5point1())
        return "surround_5_1";

    if (outputSet == juce::AudioChannelSet::discreteChannels (8))
    {
        return "surround_5_2_1";
    }

    if (outputSet == juce::AudioChannelSet::create7point1())
        return "surround_7_1";

    if (outputSet == juce::AudioChannelSet::discreteChannels (10))
    {
        return "surround_7_2_1";
    }

    if (outputSet == juce::AudioChannelSet::create7point1point4())
        return "surround_7_1_4";

    if (outputSet == juce::AudioChannelSet::discreteChannels (13))
    {
        return "surround_7_4_2";
    }

    if (outputSet.size() >= SpatialRenderer::NUM_SPEAKERS)
        return "multichannel";

    return "other";
}

enum class RendererMatrixDomain
{
    InternalBinaural,
    Multichannel,
    ExternalSpatial
};

const char* rendererMatrixDomainToString (RendererMatrixDomain domain) noexcept
{
    switch (domain)
    {
        case RendererMatrixDomain::InternalBinaural: return "InternalBinaural";
        case RendererMatrixDomain::Multichannel: return "Multichannel";
        case RendererMatrixDomain::ExternalSpatial: return "ExternalSpatial";
        default: break;
    }

    return "InternalBinaural";
}

const char* rendererMatrixLayoutFromOutputChannels (int outputChannels) noexcept
{
    if (outputChannels >= 13)
        return "immersive_7_4_2";
    if (outputChannels >= 10)
        return "surround_7_1";
    if (outputChannels >= 8)
        return "surround_5_1";
    if (outputChannels >= 4)
        return "quad_4_0";
    return "stereo_2_0";
}

const char* rendererMatrixLayoutForProfileIndex (int profileIndex, int outputChannels) noexcept
{
    switch (static_cast<SpatialRenderer::SpatialOutputProfile> (juce::jlimit (0, 11, profileIndex)))
    {
        case SpatialRenderer::SpatialOutputProfile::Stereo20:
        case SpatialRenderer::SpatialOutputProfile::Virtual3dStereo:
            return "stereo_2_0";

        case SpatialRenderer::SpatialOutputProfile::Quad40:
        case SpatialRenderer::SpatialOutputProfile::AmbisonicFOA:
            return "quad_4_0";

        case SpatialRenderer::SpatialOutputProfile::Surround521:
            return "surround_5_1";

        case SpatialRenderer::SpatialOutputProfile::Surround721:
            return "surround_7_1";

        case SpatialRenderer::SpatialOutputProfile::Surround742:
        case SpatialRenderer::SpatialOutputProfile::AtmosBed:
        case SpatialRenderer::SpatialOutputProfile::CodecIAMF:
        case SpatialRenderer::SpatialOutputProfile::CodecADM:
        case SpatialRenderer::SpatialOutputProfile::AmbisonicHOA:
            return "immersive_7_4_2";

        case SpatialRenderer::SpatialOutputProfile::Auto:
        default:
            break;
    }

    return rendererMatrixLayoutFromOutputChannels (outputChannels);
}

RendererMatrixDomain rendererMatrixRequestedDomainForProfile (int profileIndex,
                                                              int requestedHeadphoneModeIndex,
                                                              int outputChannels) noexcept
{
    const auto requestedProfile = static_cast<SpatialRenderer::SpatialOutputProfile> (
        juce::jlimit (0, 11, profileIndex));
    const auto requestedHeadphoneMode = static_cast<SpatialRenderer::HeadphoneRenderMode> (
        juce::jlimit (0, 1, requestedHeadphoneModeIndex));

    if (requestedProfile == SpatialRenderer::SpatialOutputProfile::CodecIAMF
        || requestedProfile == SpatialRenderer::SpatialOutputProfile::CodecADM
        || requestedProfile == SpatialRenderer::SpatialOutputProfile::AtmosBed)
    {
        return RendererMatrixDomain::ExternalSpatial;
    }

    if (requestedProfile == SpatialRenderer::SpatialOutputProfile::Virtual3dStereo)
        return RendererMatrixDomain::InternalBinaural;

    if (requestedProfile == SpatialRenderer::SpatialOutputProfile::Stereo20
        || (requestedProfile == SpatialRenderer::SpatialOutputProfile::Auto && outputChannels <= 2)
        || (requestedHeadphoneMode == SpatialRenderer::HeadphoneRenderMode::SteamBinaural
            && outputChannels <= 2))
    {
        return RendererMatrixDomain::InternalBinaural;
    }

    return RendererMatrixDomain::Multichannel;
}

const char* rendererMatrixStatusTextForReason (const juce::String& reasonCode) noexcept
{
    if (reasonCode == "ok")
        return "Spatial output matrix valid.";
    if (reasonCode == "binaural_requires_stereo")
        return "Binaural requires stereo output. Previous legal routing retained.";
    if (reasonCode == "multichannel_requires_min_4ch")
        return "Multichannel requires at least 4 output channels.";
    if (reasonCode == "headtracking_not_supported_in_multichannel")
        return "Head tracking is available only in internal binaural mode.";
    if (reasonCode == "external_spatial_requires_multichannel_bed")
        return "External spatial mode requires a multichannel bed.";
    if (reasonCode == "fallback_derived_from_layout")
        return "No legal prior state; routing derived from current host layout.";
    if (reasonCode == "fallback_safe_stereo_passthrough")
        return "Fail-safe stereo passthrough active; review output configuration.";

    return "Spatial output matrix valid.";
}

struct RendererMatrixSnapshot
{
    juce::String requestedDomain { "InternalBinaural" };
    juce::String activeDomain { "InternalBinaural" };
    juce::String requestedLayout { "stereo_2_0" };
    juce::String activeLayout { "stereo_2_0" };
    juce::String ruleId { "SOM-028-01" };
    juce::String ruleState { "allowed" };
    juce::String reasonCode { "ok" };
    juce::String fallbackMode { "none" };
    juce::String failSafeRoute { "none" };
    juce::String statusText { "Spatial output matrix valid." };
    bool blocked = false;
};

struct RendererHeadTrackingSnapshot
{
    bool bridgeEnabled = false;
    juce::String source { "disabled" };
    bool poseAvailable = false;
    bool poseStale = true;
    bool orientationValid = false;
    std::uint32_t invalidPacketCount = 0;
    std::uint32_t seq = 0;
    std::uint64_t timestampMs = 0;
    double ageMs = 0.0;
    float qx = 0.0f;
    float qy = 0.0f;
    float qz = 0.0f;
    float qw = 1.0f;
    float yawDeg = 0.0f;
    float pitchDeg = 0.0f;
    float rollDeg = 0.0f;
};

constexpr double kRendererHeadTrackingStaleMs = 500.0;

void computeHeadTrackingEulerDegrees (const HeadTrackingBridge::PoseSnapshot& pose,
                                      float& yawDeg,
                                      float& pitchDeg,
                                      float& rollDeg) noexcept
{
    // Intrinsic Tait-Bryan ZYX extraction (yaw, pitch, roll) from unit quaternion.
    const float x = pose.qx;
    const float y = pose.qy;
    const float z = pose.qz;
    const float w = pose.qw;

    const float sinrCosp = 2.0f * ((w * x) + (y * z));
    const float cosrCosp = 1.0f - 2.0f * ((x * x) + (y * y));
    const float roll = std::atan2 (sinrCosp, cosrCosp);

    const float sinp = 2.0f * ((w * y) - (z * x));
    const float pitch = (std::abs (sinp) >= 1.0f)
        ? std::copysign (static_cast<float> (juce::MathConstants<double>::halfPi), sinp)
        : std::asin (sinp);

    const float sinyCosp = 2.0f * ((w * z) + (x * y));
    const float cosyCosp = 1.0f - 2.0f * ((y * y) + (z * z));
    const float yaw = std::atan2 (sinyCosp, cosyCosp);

    constexpr float kRadToDeg = 57.2957795f;
    yawDeg = yaw * kRadToDeg;
    pitchDeg = pitch * kRadToDeg;
    rollDeg = roll * kRadToDeg;
}

RendererHeadTrackingSnapshot buildRendererHeadTrackingSnapshot (
    const HeadTrackingBridge::PoseSnapshot* pose,
    std::uint32_t invalidPacketCount,
    std::uint64_t nowMs) noexcept
{
    RendererHeadTrackingSnapshot snapshot;
    snapshot.invalidPacketCount = invalidPacketCount;

#if LOCUS_HEAD_TRACKING
    snapshot.bridgeEnabled = true;
    snapshot.source = "udp_loopback:19765";
#else
    snapshot.bridgeEnabled = false;
    snapshot.source = "disabled";
#endif

    if (pose == nullptr)
        return snapshot;

    snapshot.poseAvailable = true;
    snapshot.qx = pose->qx;
    snapshot.qy = pose->qy;
    snapshot.qz = pose->qz;
    snapshot.qw = pose->qw;
    snapshot.timestampMs = pose->timestampMs;
    snapshot.seq = pose->seq;

    const auto timestampMs = pose->timestampMs;
    if (timestampMs > 0 && nowMs >= timestampMs)
        snapshot.ageMs = static_cast<double> (nowMs - timestampMs);
    else if (timestampMs > 0)
        snapshot.ageMs = 0.0;
    else
        snapshot.ageMs = kRendererHeadTrackingStaleMs + 1.0;

    snapshot.poseStale = snapshot.ageMs > kRendererHeadTrackingStaleMs;

    if (std::isfinite (pose->qx)
        && std::isfinite (pose->qy)
        && std::isfinite (pose->qz)
        && std::isfinite (pose->qw))
    {
        float yawDeg = 0.0f;
        float pitchDeg = 0.0f;
        float rollDeg = 0.0f;
        computeHeadTrackingEulerDegrees (*pose, yawDeg, pitchDeg, rollDeg);
        if (std::isfinite (yawDeg) && std::isfinite (pitchDeg) && std::isfinite (rollDeg))
        {
            snapshot.yawDeg = yawDeg;
            snapshot.pitchDeg = pitchDeg;
            snapshot.rollDeg = rollDeg;
            snapshot.orientationValid = true;
        }
    }

    return snapshot;
}

RendererMatrixSnapshot buildRendererMatrixSnapshot (int requestedProfileIndex,
                                                    int activeProfileIndex,
                                                    int activeStageIndex,
                                                    int requestedHeadphoneModeIndex,
                                                    int activeHeadphoneModeIndex,
                                                    int outputChannels,
                                                    bool headPoseAvailable) noexcept
{
    RendererMatrixSnapshot matrix;
    const auto requestedDomain = rendererMatrixRequestedDomainForProfile (
        requestedProfileIndex,
        requestedHeadphoneModeIndex,
        outputChannels);
    matrix.requestedDomain = rendererMatrixDomainToString (requestedDomain);
    matrix.requestedLayout = rendererMatrixLayoutForProfileIndex (requestedProfileIndex, outputChannels);

    matrix.activeLayout = rendererMatrixLayoutForProfileIndex (activeProfileIndex, outputChannels);
    if (activeStageIndex == static_cast<int> (SpatialRenderer::SpatialProfileStage::FallbackStereo)
        || activeStageIndex == static_cast<int> (SpatialRenderer::SpatialProfileStage::AmbiDecodeStereo))
    {
        matrix.activeLayout = "stereo_2_0";
    }
    else if (activeStageIndex == static_cast<int> (SpatialRenderer::SpatialProfileStage::FallbackQuad))
    {
        matrix.activeLayout = "quad_4_0";
    }
    else if (activeStageIndex == static_cast<int> (SpatialRenderer::SpatialProfileStage::CodecLayoutPlaceholder))
    {
        matrix.activeLayout = "immersive_7_4_2";
    }

    if (requestedDomain == RendererMatrixDomain::ExternalSpatial && matrix.activeLayout != "stereo_2_0")
        matrix.activeDomain = "ExternalSpatial";
    else if (requestedDomain == RendererMatrixDomain::Multichannel && matrix.activeLayout != "stereo_2_0")
        matrix.activeDomain = "Multichannel";
    else
        matrix.activeDomain = "InternalBinaural";

    const auto activeHeadphoneMode = static_cast<SpatialRenderer::HeadphoneRenderMode> (
        juce::jlimit (0, 1, activeHeadphoneModeIndex));

    if (requestedDomain == RendererMatrixDomain::InternalBinaural)
    {
        if (matrix.activeLayout == "stereo_2_0")
        {
            matrix.ruleId = headPoseAvailable ? "SOM-028-02" : "SOM-028-01";
            matrix.ruleState = "allowed";
            matrix.reasonCode = "ok";
        }
        else
        {
            matrix.ruleId = "SOM-028-03";
            matrix.ruleState = "blocked";
            matrix.reasonCode = "binaural_requires_stereo";
            matrix.fallbackMode = "retain_last_legal";
            matrix.failSafeRoute = "last_legal";
            matrix.blocked = true;
        }
    }
    else if (requestedDomain == RendererMatrixDomain::ExternalSpatial)
    {
        if (matrix.activeLayout == "stereo_2_0")
        {
            matrix.ruleId = "SOM-028-11";
            matrix.ruleState = "blocked";
            matrix.reasonCode = "external_spatial_requires_multichannel_bed";
            matrix.fallbackMode = "derive_from_host_layout";
            matrix.failSafeRoute = "layout_derived";
            matrix.blocked = true;
        }
        else
        {
            matrix.ruleId = "SOM-028-10";
            matrix.ruleState = "allowed";
            matrix.reasonCode = "ok";
        }
    }
    else
    {
        if (matrix.activeLayout == "quad_4_0")
        {
            matrix.ruleId = "SOM-028-04";
            matrix.ruleState = "allowed";
            matrix.reasonCode = "ok";
        }
        else if (matrix.activeLayout == "surround_5_1")
        {
            matrix.ruleId = "SOM-028-05";
            matrix.ruleState = "allowed";
            matrix.reasonCode = "ok";
        }
        else if (matrix.activeLayout == "surround_7_1")
        {
            matrix.ruleId = "SOM-028-06";
            matrix.ruleState = "allowed";
            matrix.reasonCode = "ok";
        }
        else if (matrix.activeLayout == "immersive_7_4_2")
        {
            matrix.ruleId = "SOM-028-07";
            matrix.ruleState = "allowed";
            matrix.reasonCode = "ok";
        }
        else
        {
            matrix.ruleId = "SOM-028-08";
            matrix.ruleState = "blocked";
            matrix.reasonCode = "multichannel_requires_min_4ch";
            matrix.fallbackMode = "derive_from_host_layout";
            matrix.failSafeRoute = "layout_derived";
            matrix.blocked = true;
        }
    }

    if (matrix.blocked && outputChannels <= 1)
    {
        matrix.reasonCode = "fallback_safe_stereo_passthrough";
        matrix.fallbackMode = "safe_stereo_passthrough";
        matrix.failSafeRoute = "stereo_passthrough";
    }

    if (activeHeadphoneMode == SpatialRenderer::HeadphoneRenderMode::SteamBinaural
        && matrix.activeLayout == "stereo_2_0")
    {
        matrix.activeDomain = "InternalBinaural";
    }

    matrix.statusText = rendererMatrixStatusTextForReason (matrix.reasonCode);
    return matrix;
}

constexpr const char* kSnapshotSchemaProperty = "locusq_snapshot_schema";
constexpr const char* kSnapshotSchemaValueV2 = "locusq-state-v2";
constexpr const char* kSnapshotOutputLayoutProperty = "locusq_output_layout";
constexpr const char* kSnapshotOutputChannelsProperty = "locusq_output_channels";
constexpr const char* kSceneSnapshotSchemaProperty = "locusq-scene-snapshot-v1";
constexpr int kMaxSnapshotOutputChannels = 16;
constexpr int kSceneSnapshotCadenceHz = 30;
constexpr int kSceneSnapshotStaleAfterMs = 750;
constexpr int kRendererAuditionCloudMaxEmitters = 8;
constexpr int kRendererAuditionCloudMaxPoints = 160;
constexpr const char* kEmitterPresetSchemaV1 = "locusq-emitter-preset-v1";
constexpr const char* kEmitterPresetSchemaV2 = "locusq-emitter-preset-v2";
constexpr const char* kEmitterPresetLayoutProperty = "layout";
constexpr const char* kEmitterPresetTypeProperty = "presetType";
constexpr const char* kEmitterPresetTypeEmitter = "emitter";
constexpr const char* kEmitterPresetTypeMotion = "motion";
constexpr const char* kCalibrationProfileSchemaV1 = "locusq-calibration-profile-v1";

constexpr std::array<const char*, 35> kEmitterPresetParameterIds
{
    "pos_azimuth", "pos_elevation", "pos_distance",
    "pos_x", "pos_y", "pos_z", "pos_coord_mode",
    "size_width", "size_depth", "size_height", "size_link", "size_uniform",
    "emit_gain", "emit_mute", "emit_solo", "emit_spread", "emit_directivity",
    "emit_dir_azimuth", "emit_dir_elevation", "emit_color",
    "phys_enable", "phys_mass", "phys_drag", "phys_elasticity",
    "phys_gravity", "phys_gravity_dir", "phys_friction",
    "phys_vel_x", "phys_vel_y", "phys_vel_z",
    "anim_enable", "anim_mode", "anim_loop", "anim_speed", "anim_sync"
};

constexpr std::array<const char*, 5> kCurveNames
{
    "linear",
    "easeIn",
    "easeOut",
    "easeInOut",
    "step"
};

constexpr std::array<const char*, 4> kChoreographyPackIds
{
    "orbit",
    "pendulum",
    "swarm_arc",
    "rise_fall"
};

constexpr std::array<const char*, 11> kCalibrationTopologyIds
{
    "mono",
    "stereo",
    "quad",
    "surround_51",
    "surround_71",
    "surround_712",
    "surround_742",
    "binaural",
    "ambisonic_1st",
    "ambisonic_3rd",
    "downmix_stereo"
};

constexpr std::array<int, 11> kCalibrationTopologyRequiredChannels
{
    1, 2, 4, 6, 8, 10, 13, 2, 4, 16, 2
};

constexpr std::array<const char*, 4> kCalibrationMonitoringPathIds
{
    "speakers",
    "stereo_downmix",
    "steam_binaural",
    "virtual_binaural"
};

constexpr std::array<const char*, 5> kCalibrationDeviceProfileIds
{
    "generic",
    "airpods_pro_2",
    "airpods_pro_3",
    "sony_wh1000xm5",
    "custom_sofa"
};

constexpr std::array<const char*, 13> kRendererAuditionSignalIds
{
    "sine_440",
    "dual_tone",
    "pink_noise",
    "rain_field",
    "snow_drift",
    "bouncing_balls",
    "wind_chimes",
    "crickets",
    "song_birds",
    "karplus_plucks",
    "membrane_drops",
    "krell_patch",
    "generative_arp"
};

constexpr std::array<const char*, 6> kRendererAuditionMotionIds
{
    "center",
    "orbit_slow",
    "orbit_fast",
    "figure8_flow",
    "helix_rise",
    "wall_ricochet"
};

constexpr std::array<float, 5> kRendererAuditionLevelDbValues
{
    -36.0f, -30.0f, -24.0f, -18.0f, -12.0f
};

constexpr std::array<const char*, 11> kCalibrationProfileParameterIds
{
    "cal_spk_config",
    "cal_topology_profile",
    "cal_monitoring_path",
    "cal_device_profile",
    "cal_mic_channel",
    "cal_spk1_out",
    "cal_spk2_out",
    "cal_spk3_out",
    "cal_spk4_out",
    "cal_test_level",
    "cal_test_type"
};

bool isFiniteVector3 (float x, float y, float z) noexcept
{
    return std::isfinite (x) && std::isfinite (y) && std::isfinite (z);
}

float sanitizeUnitScalar (float value, float fallback, bool* adjusted = nullptr) noexcept
{
    float sanitized = fallback;
    bool changed = false;

    if (std::isfinite (value))
    {
        sanitized = juce::jlimit (0.0f, 1.0f, value);
        changed = std::abs (sanitized - value) > 1.0e-6f;
    }
    else
    {
        sanitized = juce::jlimit (0.0f, 1.0f, fallback);
        changed = true;
    }

    if (adjusted != nullptr)
        *adjusted |= changed;

    return sanitized;
}

int sanitizeBoundedInt (int value, int minValue, int maxValue, bool* adjusted = nullptr) noexcept
{
    const auto sanitized = juce::jlimit (minValue, maxValue, value);
    if (adjusted != nullptr)
        *adjusted |= (sanitized != value);
    return sanitized;
}

SpatialRenderer::AuditionReactiveSnapshot makeNeutralAuditionReactiveSnapshot() noexcept
{
    SpatialRenderer::AuditionReactiveSnapshot snapshot {};
    snapshot.rms = 0.0f;
    snapshot.peak = 0.0f;
    snapshot.envFast = 0.0f;
    snapshot.envSlow = 0.0f;
    snapshot.onset = 0.0f;
    snapshot.brightness = 0.0f;
    snapshot.rainFadeRate = 0.0f;
    snapshot.snowFadeRate = 0.0f;
    snapshot.physicsVelocity = 0.0f;
    snapshot.physicsCollision = 0.0f;
    snapshot.physicsDensity = 0.0f;
    snapshot.physicsCoupling = 0.0f;
    snapshot.geometryScale = 0.0f;
    snapshot.geometryWidth = 0.0f;
    snapshot.geometryDepth = 0.0f;
    snapshot.geometryHeight = 0.0f;
    snapshot.precipitationFade = 0.0f;
    snapshot.collisionBurst = 0.0f;
    snapshot.densitySpread = 0.0f;
    snapshot.headphoneOutputRms = 0.0f;
    snapshot.headphoneOutputPeak = 0.0f;
    snapshot.headphoneParity = 0.0f;
    snapshot.rmsNorm = 0.0f;
    snapshot.peakNorm = 0.0f;
    snapshot.envFastNorm = 0.0f;
    snapshot.envSlowNorm = 0.0f;
    snapshot.headphoneOutputRmsNorm = 0.0f;
    snapshot.headphoneOutputPeakNorm = 0.0f;
    snapshot.headphoneParityNorm = 0.0f;
    snapshot.headphoneFallbackReasonIndex =
        static_cast<int> (SpatialRenderer::AuditionReactiveHeadphoneFallbackReason::None);
    snapshot.sourceEnergyCount = 0;
    for (auto& value : snapshot.sourceEnergy)
        value = 0.0f;
    return snapshot;
}

struct SanitizedAuditionReactivePayload
{
    SpatialRenderer::AuditionReactiveSnapshot snapshot {};
    bool invalidScalars = false;
    bool invalidBounds = false;
};

SanitizedAuditionReactivePayload sanitizeAuditionReactivePayload (
    const SpatialRenderer::AuditionReactiveSnapshot& raw) noexcept
{
    SanitizedAuditionReactivePayload payload;
    payload.snapshot = raw;

    payload.snapshot.rms = sanitizeUnitScalar (raw.rmsNorm, raw.rms * 0.5f, &payload.invalidScalars);
    payload.snapshot.peak = sanitizeUnitScalar (raw.peakNorm, raw.peak * 0.5f, &payload.invalidScalars);
    payload.snapshot.envFast = sanitizeUnitScalar (raw.envFastNorm, raw.envFast * 0.5f, &payload.invalidScalars);
    payload.snapshot.envSlow = sanitizeUnitScalar (raw.envSlowNorm, raw.envSlow * 0.5f, &payload.invalidScalars);
    payload.snapshot.onset = sanitizeUnitScalar (raw.onset, 0.0f, &payload.invalidScalars);
    payload.snapshot.brightness = sanitizeUnitScalar (raw.brightness, 0.0f, &payload.invalidScalars);
    payload.snapshot.rainFadeRate = sanitizeUnitScalar (raw.rainFadeRate, 0.0f, &payload.invalidScalars);
    payload.snapshot.snowFadeRate = sanitizeUnitScalar (raw.snowFadeRate, 0.0f, &payload.invalidScalars);
    payload.snapshot.physicsVelocity = sanitizeUnitScalar (raw.physicsVelocity, 0.0f, &payload.invalidScalars);
    payload.snapshot.physicsCollision = sanitizeUnitScalar (raw.physicsCollision, 0.0f, &payload.invalidScalars);
    payload.snapshot.physicsDensity = sanitizeUnitScalar (raw.physicsDensity, 0.0f, &payload.invalidScalars);
    payload.snapshot.physicsCoupling = sanitizeUnitScalar (raw.physicsCoupling, 0.0f, &payload.invalidScalars);
    payload.snapshot.geometryScale = sanitizeUnitScalar (raw.geometryScale, 0.0f, &payload.invalidScalars);
    payload.snapshot.geometryWidth = sanitizeUnitScalar (raw.geometryWidth, 0.0f, &payload.invalidScalars);
    payload.snapshot.geometryDepth = sanitizeUnitScalar (raw.geometryDepth, 0.0f, &payload.invalidScalars);
    payload.snapshot.geometryHeight = sanitizeUnitScalar (raw.geometryHeight, 0.0f, &payload.invalidScalars);
    payload.snapshot.precipitationFade = sanitizeUnitScalar (raw.precipitationFade, 0.0f, &payload.invalidScalars);
    payload.snapshot.collisionBurst = sanitizeUnitScalar (raw.collisionBurst, 0.0f, &payload.invalidScalars);
    payload.snapshot.densitySpread = sanitizeUnitScalar (raw.densitySpread, 0.0f, &payload.invalidScalars);
    payload.snapshot.headphoneOutputRms = sanitizeUnitScalar (
        raw.headphoneOutputRmsNorm,
        raw.headphoneOutputRms * 0.5f,
        &payload.invalidScalars);
    payload.snapshot.headphoneOutputPeak = sanitizeUnitScalar (
        raw.headphoneOutputPeakNorm,
        raw.headphoneOutputPeak * 0.5f,
        &payload.invalidScalars);
    payload.snapshot.headphoneParity = sanitizeUnitScalar (
        raw.headphoneParityNorm,
        raw.headphoneParity,
        &payload.invalidScalars);
    payload.snapshot.rmsNorm = payload.snapshot.rms;
    payload.snapshot.peakNorm = payload.snapshot.peak;
    payload.snapshot.envFastNorm = payload.snapshot.envFast;
    payload.snapshot.envSlowNorm = payload.snapshot.envSlow;
    payload.snapshot.headphoneOutputRmsNorm = payload.snapshot.headphoneOutputRms;
    payload.snapshot.headphoneOutputPeakNorm = payload.snapshot.headphoneOutputPeak;
    payload.snapshot.headphoneParityNorm = payload.snapshot.headphoneParity;
    payload.snapshot.headphoneFallbackReasonIndex = sanitizeBoundedInt (
        raw.headphoneFallbackReasonIndex,
        0,
        3,
        &payload.invalidBounds);
    payload.snapshot.sourceEnergyCount = sanitizeBoundedInt (
        raw.sourceEnergyCount,
        0,
        SpatialRenderer::MAX_AUDITION_REACTIVE_SOURCES,
        &payload.invalidBounds);

    for (int sourceIndex = 0; sourceIndex < SpatialRenderer::MAX_AUDITION_REACTIVE_SOURCES; ++sourceIndex)
    {
        const auto rawEnergy = raw.sourceEnergy[static_cast<size_t> (sourceIndex)];
        payload.snapshot.sourceEnergy[static_cast<size_t> (sourceIndex)] = sanitizeUnitScalar (
            rawEnergy,
            0.0f,
            &payload.invalidScalars);
    }

    return payload;
}

template <size_t N>
int indexOfCaseInsensitive (const std::array<const char*, N>& values, const juce::String& target)
{
    const auto normalised = target.trim().toLowerCase();
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (normalised == values[i])
            return static_cast<int> (i);
    }

    return -1;
}

juce::String calibrationTopologyIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationTopologyIds.size()) - 1, index);
    return kCalibrationTopologyIds[static_cast<size_t> (clamped)];
}

juce::String calibrationMonitoringPathIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationMonitoringPathIds.size()) - 1, index);
    return kCalibrationMonitoringPathIds[static_cast<size_t> (clamped)];
}

juce::String calibrationDeviceProfileIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationDeviceProfileIds.size()) - 1, index);
    return kCalibrationDeviceProfileIds[static_cast<size_t> (clamped)];
}

struct HeadphoneCalibrationDiagnosticsSnapshot
{
    juce::String requested { locusq::shared_contracts::headphone_calibration::path::kSpeakers };
    juce::String active { locusq::shared_contracts::headphone_calibration::path::kSpeakers };
    juce::String stage { locusq::shared_contracts::headphone_calibration::stage::kDirect };
    bool fallbackReady = true;
    juce::String fallbackReason { locusq::shared_contracts::headphone_calibration::fallback_reason::kNone };
};

juce::String sanitizeHeadphoneCalibrationPath (juce::String path)
{
    path = path.trim().toLowerCase();

    if (path == locusq::shared_contracts::headphone_calibration::path::kSpeakers
        || path == locusq::shared_contracts::headphone_calibration::path::kStereoDownmix
        || path == locusq::shared_contracts::headphone_calibration::path::kSteamBinaural
        || path == locusq::shared_contracts::headphone_calibration::path::kVirtualBinaural)
    {
        return path;
    }

    return locusq::shared_contracts::headphone_calibration::path::kSpeakers;
}

HeadphoneCalibrationDiagnosticsSnapshot buildHeadphoneCalibrationDiagnosticsSnapshot (
    int monitoringPathIndex,
    int requestedHeadphoneModeIndex,
    int activeHeadphoneModeIndex,
    int outputChannels,
    bool steamAudioAvailable,
    const juce::String& steamAudioInitStage)
{
    HeadphoneCalibrationDiagnosticsSnapshot snapshot;
    snapshot.requested = sanitizeHeadphoneCalibrationPath (calibrationMonitoringPathIdForIndex (monitoringPathIndex));
    snapshot.active = snapshot.requested;
    snapshot.stage = locusq::shared_contracts::headphone_calibration::stage::kDirect;
    snapshot.fallbackReady = true;
    snapshot.fallbackReason = locusq::shared_contracts::headphone_calibration::fallback_reason::kNone;

    const bool stereoCompatible = outputChannels >= 2;
    const bool requestedSteamMode =
        requestedHeadphoneModeIndex == static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural);
    const bool activeSteamMode =
        activeHeadphoneModeIndex == static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural);
    const auto steamStage = steamAudioInitStage.trim().toLowerCase();

    if (snapshot.requested == locusq::shared_contracts::headphone_calibration::path::kSteamBinaural)
    {
        snapshot.fallbackReady = stereoCompatible;

        if (! stereoCompatible)
        {
            snapshot.active = locusq::shared_contracts::headphone_calibration::path::kStereoDownmix;
            snapshot.stage = locusq::shared_contracts::headphone_calibration::stage::kFallback;
            snapshot.fallbackReason = locusq::shared_contracts::headphone_calibration::fallback_reason::kOutputIncompatible;
            return snapshot;
        }

        if (requestedSteamMode && activeSteamMode && steamAudioAvailable)
        {
            snapshot.active = locusq::shared_contracts::headphone_calibration::path::kSteamBinaural;
            snapshot.stage = steamStage == "ready"
                                 ? locusq::shared_contracts::headphone_calibration::stage::kReady
                                 : locusq::shared_contracts::headphone_calibration::stage::kInitializing;
            snapshot.fallbackReason = locusq::shared_contracts::headphone_calibration::fallback_reason::kNone;
            return snapshot;
        }

        snapshot.active = locusq::shared_contracts::headphone_calibration::path::kStereoDownmix;
        snapshot.stage = steamAudioAvailable
                             ? locusq::shared_contracts::headphone_calibration::stage::kFallback
                             : locusq::shared_contracts::headphone_calibration::stage::kUnavailable;
        snapshot.fallbackReason = steamAudioAvailable
                                      ? locusq::shared_contracts::headphone_calibration::fallback_reason::kMonitoringPathBypassed
                                      : locusq::shared_contracts::headphone_calibration::fallback_reason::kSteamUnavailable;
        return snapshot;
    }

    if (snapshot.requested == locusq::shared_contracts::headphone_calibration::path::kVirtualBinaural
        && ! stereoCompatible)
    {
        snapshot.active = locusq::shared_contracts::headphone_calibration::path::kStereoDownmix;
        snapshot.stage = locusq::shared_contracts::headphone_calibration::stage::kFallback;
        snapshot.fallbackReady = false;
        snapshot.fallbackReason = locusq::shared_contracts::headphone_calibration::fallback_reason::kOutputIncompatible;
    }

    return snapshot;
}

struct HeadphoneVerificationSnapshot
{
    juce::String profileId { "generic" };
    juce::String requestedProfileId { "generic" };
    juce::String activeProfileId { "generic" };
    juce::String requestedEngineId { locusq::shared_contracts::headphone_verification::engine::kDisabled };
    juce::String activeEngineId { locusq::shared_contracts::headphone_verification::engine::kDisabled };
    juce::String fallbackReasonCode {
        locusq::shared_contracts::headphone_verification::fallback_reason::kDisabledByRequest
    };
    juce::String fallbackTarget {
        locusq::shared_contracts::headphone_verification::fallback_target::kDisabled
    };
    juce::String fallbackReasonText {
        locusq::shared_contracts::headphone_verification::fallbackReasonTextForCode (
            locusq::shared_contracts::headphone_verification::fallback_reason::kDisabledByRequest)
    };
    float frontBackScore = 0.0f;
    float elevationScore = 0.0f;
    float externalizationScore = 0.0f;
    float confidence = 0.0f;
    juce::String verificationStage { locusq::shared_contracts::headphone_verification::stage::kDisabled };
    juce::String verificationScoreStatus {
        locusq::shared_contracts::headphone_verification::score_status::kUnavailable
    };
    int chainLatencySamples = 0;
};

HeadphoneVerificationSnapshot buildHeadphoneVerificationSnapshot (
    int requestedProfileIndex,
    int activeProfileIndex,
    bool calibrationEnabledRequested,
    int requestedEngineIndex,
    int activeEngineIndex,
    int fallbackReasonIndex,
    int chainLatencySamples) noexcept
{
    using namespace locusq::shared_contracts::headphone_verification;

    HeadphoneVerificationSnapshot snapshot;
    snapshot.requestedProfileId = sanitizeProfileId (
        SpatialRenderer::headphoneDeviceProfileToString (requestedProfileIndex));
    snapshot.activeProfileId = sanitizeProfileId (
        SpatialRenderer::headphoneDeviceProfileToString (activeProfileIndex));
    snapshot.profileId = snapshot.activeProfileId;
    snapshot.requestedEngineId = sanitizeEngineId (
        SpatialRenderer::headphoneCalibrationEngineToString (requestedEngineIndex));
    snapshot.activeEngineId = sanitizeEngineId (
        SpatialRenderer::headphoneCalibrationEngineToString (activeEngineIndex));
    snapshot.fallbackReasonCode = sanitizeFallbackReasonCode (
        SpatialRenderer::headphoneCalibrationFallbackReasonToString (fallbackReasonIndex));
    snapshot.fallbackTarget = deriveFallbackTarget (snapshot.fallbackReasonCode, snapshot.activeEngineId);
    snapshot.fallbackReasonText = fallbackReasonTextForCode (snapshot.fallbackReasonCode);
    snapshot.chainLatencySamples = sanitizeLatencySamples (chainLatencySamples);

    const auto requestedEngineSanitized =
        locusq::headphone_core::sanitizeCalibrationEngineIndex (requestedEngineIndex);
    const auto activeEngineSanitized =
        locusq::headphone_core::sanitizeCalibrationEngineIndex (activeEngineIndex);
    const auto fallbackReasonSanitized =
        locusq::headphone_core::sanitizeCalibrationFallbackReasonIndex (fallbackReasonIndex);
    const auto disabledEngineIndex =
        static_cast<int> (locusq::headphone_core::CalibrationChainEngine::Disabled);

    const bool verificationDisabled = ! calibrationEnabledRequested
        || requestedEngineSanitized == disabledEngineIndex;

    if (verificationDisabled)
    {
        snapshot.activeEngineId = engine::kDisabled;
        snapshot.fallbackReasonCode = fallback_reason::kDisabledByRequest;
        snapshot.fallbackTarget = fallback_target::kDisabled;
        snapshot.fallbackReasonText = fallbackReasonTextForCode (snapshot.fallbackReasonCode);
        snapshot.chainLatencySamples = 0;
    }
    else if (activeEngineSanitized == disabledEngineIndex
             && fallbackReasonSanitized
                    == static_cast<int> (locusq::headphone_core::CalibrationChainFallbackReason::None))
    {
        snapshot.fallbackReasonCode = fallback_reason::kInvalidEngineSelection;
        snapshot.fallbackTarget = fallback_target::kDisabled;
        snapshot.fallbackReasonText = fallbackReasonTextForCode (snapshot.fallbackReasonCode);
    }

    if (verificationDisabled)
    {
        snapshot.verificationStage = stage::kDisabled;
    }
    else if (fallbackReasonSanitized
             == static_cast<int> (locusq::headphone_core::CalibrationChainFallbackReason::DspNotPrepared))
    {
        snapshot.verificationStage = stage::kInitializing;
    }
    else if (activeEngineSanitized == requestedEngineSanitized
             && fallbackReasonSanitized
                    == static_cast<int> (locusq::headphone_core::CalibrationChainFallbackReason::None)
             && activeEngineSanitized != disabledEngineIndex)
    {
        snapshot.verificationStage = stage::kVerified;
    }
    else if (activeEngineSanitized != disabledEngineIndex)
    {
        snapshot.verificationStage = stage::kFallback;
    }
    else
    {
        snapshot.verificationStage = stage::kUnavailable;
    }

    float baseFrontBack = 0.0f;
    float baseElevation = 0.0f;
    float baseExternalization = 0.0f;

    switch (static_cast<locusq::headphone_core::CalibrationChainEngine> (activeEngineSanitized))
    {
        case locusq::headphone_core::CalibrationChainEngine::ParametricEq:
            baseFrontBack = 0.70f;
            baseElevation = 0.62f;
            baseExternalization = 0.66f;
            break;

        case locusq::headphone_core::CalibrationChainEngine::FirConvolution:
            baseFrontBack = 0.84f;
            baseElevation = 0.79f;
            baseExternalization = 0.82f;
            break;

        case locusq::headphone_core::CalibrationChainEngine::Disabled:
        default:
            break;
    }

    float penalty = 0.0f;
    switch (static_cast<locusq::headphone_core::CalibrationChainFallbackReason> (fallbackReasonSanitized))
    {
        case locusq::headphone_core::CalibrationChainFallbackReason::None:
            penalty = 0.0f;
            break;

        case locusq::headphone_core::CalibrationChainFallbackReason::DisabledByRequest:
            penalty = 1.0f;
            break;

        case locusq::headphone_core::CalibrationChainFallbackReason::InvalidEngineSelection:
            penalty = 0.55f;
            break;

        case locusq::headphone_core::CalibrationChainFallbackReason::DspNotPrepared:
            penalty = 0.85f;
            break;

        case locusq::headphone_core::CalibrationChainFallbackReason::PeqUnavailable:
            penalty = 0.45f;
            break;

        case locusq::headphone_core::CalibrationChainFallbackReason::FirUnavailable:
            penalty = 0.25f;
            break;
    }

    if (verificationDisabled)
        penalty = 1.0f;

    snapshot.frontBackScore = sanitizeScore (baseFrontBack - penalty, 0.0f);
    snapshot.elevationScore = sanitizeScore (baseElevation - penalty, 0.0f);
    snapshot.externalizationScore = sanitizeScore (baseExternalization - penalty, 0.0f);

    const float aggregateScore =
        (snapshot.frontBackScore + snapshot.elevationScore + snapshot.externalizationScore) / 3.0f;
    float confidenceBias = -0.25f;

    if (snapshot.verificationStage == stage::kVerified)
        confidenceBias = 0.08f;
    else if (snapshot.verificationStage == stage::kFallback)
        confidenceBias = -0.08f;
    else if (snapshot.verificationStage == stage::kInitializing)
        confidenceBias = -0.20f;
    else if (snapshot.verificationStage == stage::kUnavailable)
        confidenceBias = -0.35f;

    snapshot.confidence = sanitizeScore (aggregateScore + confidenceBias, 0.0f);
    snapshot.chainLatencySamples = sanitizeLatencySamples (snapshot.chainLatencySamples);
    snapshot.verificationStage = sanitizeVerificationStage (snapshot.verificationStage);
    snapshot.fallbackReasonCode = sanitizeFallbackReasonCode (snapshot.fallbackReasonCode);
    snapshot.fallbackTarget = sanitizeFallbackTargetForReason (
        snapshot.fallbackReasonCode,
        snapshot.fallbackTarget,
        snapshot.activeEngineId);
    snapshot.fallbackReasonText = fallbackReasonTextForCode (snapshot.fallbackReasonCode);
    snapshot.verificationScoreStatus = scoreStatusFromStage (snapshot.verificationStage);

    return snapshot;
}

int calibrationRequiredChannelsForTopologyIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationTopologyRequiredChannels.size()) - 1, index);
    return kCalibrationTopologyRequiredChannels[static_cast<size_t> (clamped)];
}

juce::String rendererAuditionSignalIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kRendererAuditionSignalIds.size()) - 1, index);
    return kRendererAuditionSignalIds[static_cast<size_t> (clamped)];
}

juce::String rendererAuditionMotionIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kRendererAuditionMotionIds.size()) - 1, index);
    return kRendererAuditionMotionIds[static_cast<size_t> (clamped)];
}

float rendererAuditionLevelDbForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kRendererAuditionLevelDbValues.size()) - 1, index);
    return kRendererAuditionLevelDbValues[static_cast<size_t> (clamped)];
}

int legacySpeakerConfigForTopologyIndex (int topologyIndex)
{
    const auto requiredChannels = calibrationRequiredChannelsForTopologyIndex (topologyIndex);
    return requiredChannels <= 2 ? 1 : 0;
}

int topologyProfileForOutputChannels (int outputChannels)
{
    const auto clampedChannels = juce::jlimit (1, 16, outputChannels);
    if (clampedChannels <= 1)
        return 0;
    if (clampedChannels == 2)
        return 1;
    if (clampedChannels == 6)
        return 3;
    if (clampedChannels == 8)
        return 4;
    if (clampedChannels == 10)
        return 5;
    if (clampedChannels >= 16)
        return 9;
    if (clampedChannels >= 13)
        return 6;

    return 2;
}

constexpr std::array<const char*, SpatialRenderer::NUM_SPEAKERS> kInternalSpeakerLabels
{
    "FL", "FR", "RR", "RL"
};

constexpr std::array<Vec3, SpatialRenderer::NUM_SPEAKERS> kViewportFallbackSpeakerPositions
{
    Vec3 { -2.7f, 1.2f, -1.7f }, // FL
    Vec3 {  2.7f, 1.2f, -1.7f }, // FR
    Vec3 {  2.7f, 1.2f,  1.7f }, // RR
    Vec3 { -2.7f, 1.2f,  1.7f }  // RL
};

float computeMonoRmsLinear (const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0)
        return 0.0f;

    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        const auto sample = static_cast<double> (samples[i]);
        sumSquares += sample * sample;
    }

    return static_cast<float> (std::sqrt (sumSquares / static_cast<double> (numSamples)));
}

struct AuditionPhysicsReactiveInput
{
    bool active = false;
    float velocityNorm = 0.0f;
    float collisionNorm = 0.0f;
    float densityNorm = 0.0f;
};

AuditionPhysicsReactiveInput computeAuditionPhysicsReactiveInput (
    const SceneGraph& sceneGraph,
    bool physicsBindingRequested) noexcept
{
    AuditionPhysicsReactiveInput result;
    if (! physicsBindingRequested)
        return result;

    int physicsEmitterCount = 0;
    float maxVelocity = 0.0f;
    float velocityAccumulator = 0.0f;
    float maxCollision = 0.0f;
    float collisionAccumulator = 0.0f;

    for (int slot = 0; slot < SceneGraph::MAX_EMITTERS; ++slot)
    {
        if (! sceneGraph.isSlotActive (slot))
            continue;

        const auto data = sceneGraph.getSlot (slot).read();
        if (! data.active || ! data.physicsEnabled)
            continue;

        ++physicsEmitterCount;

        const auto speed = std::sqrt (
            data.velocity.x * data.velocity.x
            + data.velocity.y * data.velocity.y
            + data.velocity.z * data.velocity.z);
        const auto finiteSpeed = std::isfinite (speed) ? speed : 0.0f;
        maxVelocity = juce::jmax (maxVelocity, finiteSpeed);
        velocityAccumulator += finiteSpeed;

        const auto collisionEnergy = std::isfinite (data.collisionEnergy) ? data.collisionEnergy : 0.0f;
        const auto boundedCollision = juce::jlimit (0.0f, 16.0f, collisionEnergy);
        maxCollision = juce::jmax (maxCollision, boundedCollision);
        collisionAccumulator += boundedCollision;
    }

    if (physicsEmitterCount <= 0)
        return result;

    const auto avgVelocity = velocityAccumulator / static_cast<float> (physicsEmitterCount);
    const auto avgCollision = collisionAccumulator / static_cast<float> (physicsEmitterCount);
    const auto normaliseSoft = [] (float value, float scale) noexcept
    {
        const auto x = juce::jmax (0.0f, value * scale);
        return x / (1.0f + x);
    };

    result.velocityNorm = juce::jlimit (
        0.0f,
        1.0f,
        0.58f * normaliseSoft (maxVelocity, 0.40f)
            + 0.42f * normaliseSoft (avgVelocity, 0.55f));
    result.collisionNorm = juce::jlimit (
        0.0f,
        1.0f,
        0.62f * normaliseSoft (maxCollision, 1.35f)
            + 0.38f * normaliseSoft (avgCollision, 1.85f));
    result.densityNorm = juce::jlimit (
        0.0f,
        1.0f,
        static_cast<float> (physicsEmitterCount) / 8.0f);
    result.active = true;
    return result;
}

Vec3 computeEmitterInteractionForce (const SceneGraph& sceneGraph,
                                     int selfSlotId,
                                     const Vec3& selfPosition)
{
    if (sceneGraph.getActiveEmitterCount() <= 1)
        return {};

    // Radius within which emitters repel each other (metres in normalised scene space).
    // 2.0 m covers roughly one quadrant of the ±3 m scene at typical multi-emitter densities.
    constexpr float kInteractionRadius = 2.0f;
    constexpr float kInteractionRadiusSq = kInteractionRadius * kInteractionRadius;
    constexpr float kMinimumDistance = 0.05f;
    constexpr float kMinimumDistanceSq = kMinimumDistance * kMinimumDistance;
    // Peak repulsion acceleration (m/s² equivalent). Tuned so two nearby emitters
    // separate at a perceptible but not violent rate at the default physics rate.
    constexpr float kInteractionStrength = 8.0f;
    // Hard cap prevents runaway force accumulation when many emitters overlap.
    constexpr float kMaxForce = 12.0f;

    Vec3 interactionForce {};

    // Early-exit once all active slots have been visited to avoid scanning the
    // full MAX_EMITTERS tail when only a few slots are occupied.
    int remaining = sceneGraph.getActiveEmitterCount();

    for (int slotId = 0; slotId < SceneGraph::MAX_EMITTERS && remaining > 0; ++slotId)
    {
        if (! sceneGraph.isSlotActive (slotId))
            continue;

        --remaining;

        if (slotId == selfSlotId)
            continue;

        // NOTE: other.position is written by the other emitter's processBlock and
        // read here one audio callback later — a 1-frame temporal lag that is
        // intentional and acceptable in this lockless multi-reader design.
        const auto other = sceneGraph.getSlot (slotId).read();
        if (! other.active || ! other.physicsEnabled)
            continue;

        float dx = selfPosition.x - other.position.x;
        float dy = selfPosition.y - other.position.y;
        float dz = selfPosition.z - other.position.z;
        float distanceSq = (dx * dx) + (dy * dy) + (dz * dz);

        if (distanceSq >= kInteractionRadiusSq)
            continue;

        if (distanceSq < kMinimumDistanceSq)
        {
            const float direction = (((selfSlotId + slotId) & 1) == 0) ? 1.0f : -1.0f;
            dx = direction * kMinimumDistance;
            dy = 0.0f;
            dz = -direction * kMinimumDistance;
            // Include dy² for formula consistency (dy = 0 here, so no numeric change).
            distanceSq = (dx * dx) + (dy * dy) + (dz * dz);
        }

        const float distance = std::sqrt (distanceSq);
        if (distance <= 0.0f)
            continue;

        // Smoothstep falloff: C1-continuous at the boundary (no derivative
        // discontinuity), giving a smoother force transition than linear.
        const float t = juce::jlimit (0.0f, 1.0f, 1.0f - (distance / kInteractionRadius));
        const float falloff = t * t * (3.0f - 2.0f * t);
        const float forceMagnitude = falloff * kInteractionStrength;
        const float invDistance = 1.0f / distance;

        interactionForce.x += dx * invDistance * forceMagnitude;
        interactionForce.y += dy * invDistance * forceMagnitude;
        interactionForce.z += dz * invDistance * forceMagnitude;
    }

    const float forceMagSq = (interactionForce.x * interactionForce.x)
                           + (interactionForce.y * interactionForce.y)
                           + (interactionForce.z * interactionForce.z);
    if (forceMagSq > (kMaxForce * kMaxForce))
    {
        const float scale = kMaxForce / std::sqrt (forceMagSq);
        interactionForce.x *= scale;
        interactionForce.y *= scale;
        interactionForce.z *= scale;
    }

    return interactionForce;
}

int resolveCalibrationWritableChannels (int snapshotOutputChannels,
                                        int layoutOutputChannels,
                                        int cachedAutoOutputChannels,
                                        const std::array<int, SpatialRenderer::NUM_SPEAKERS>& routing) noexcept
{
    const auto snapshot = juce::jlimit (1, SpatialRenderer::NUM_SPEAKERS, snapshotOutputChannels);
    const auto layout = juce::jlimit (0, SpatialRenderer::NUM_SPEAKERS, layoutOutputChannels);
    const auto cached = juce::jlimit (0, SpatialRenderer::NUM_SPEAKERS, cachedAutoOutputChannels);

    int effective = juce::jmax (snapshot, layout);

    // Guard against transient "1 writable channel" telemetry during startup:
    // if routing intent requires >1 output and we previously detected >1,
    // keep that previous value until host telemetry stabilises.
    if (effective <= 1)
    {
        const bool routingUsesMultipleOutputs = std::any_of (
            routing.begin(),
            routing.end(),
            [] (int channel) { return channel > 1; });

        if (routingUsesMultipleOutputs)
            effective = juce::jmax (effective, cached);
    }

    return juce::jlimit (1, SpatialRenderer::NUM_SPEAKERS, effective);
}
} // end anonymous namespace

// BL-045 Slice C: store raw yaw reference for re-center UX.
void LocusQAudioProcessor::setYawReference (float yawDeg) noexcept
{
    yawReferenceDeg.store (yawDeg, std::memory_order_relaxed);
    yawReferenceSet.store (true,   std::memory_order_relaxed);
}

//==============================================================================
LocusQAudioProcessor::LocusQAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      sceneGraph (SceneGraph::getInstance())
{
    initialiseDefaultKeyframeTimeline();

    // Register with scene graph based on initial mode
    // Mode registration happens in prepareToPlay once we know the context
}

LocusQAudioProcessor::~LocusQAudioProcessor()
{
    headTrackingBridge.stop();

    // Unregister from scene graph
    if (emitterSlotId >= 0)
        sceneGraph.unregisterEmitter (emitterSlotId);

    if (rendererRegistered)
        sceneGraph.unregisterRenderer();
}

//==============================================================================
void LocusQAudioProcessor::syncSceneGraphRegistrationForMode (LocusQMode mode)
{
    auto stage = RegistrationTransitionStage::Stable;
    auto fallback = RegistrationTransitionFallbackReason::None;
    bool transitionAmbiguityObserved = false;
    bool staleOwnerRecovered = false;
    bool releaseIncomplete = false;

    auto applyContractStep = [&] (RegistrationContractOperation operation,
                                  RegistrationContractOutcome outcome)
    {
        registrationClaimReleaseDiagnostics.lastOperationCode.store (
            static_cast<int> (operation),
            std::memory_order_relaxed);
        registrationClaimReleaseDiagnostics.lastOutcomeCode.store (
            static_cast<int> (outcome),
            std::memory_order_relaxed);
        registrationClaimReleaseDiagnostics.seq.fetch_add (1, std::memory_order_release);

        if (locusq::shared_contracts::registration_lock_free::isContention (outcome))
        {
            registrationClaimReleaseDiagnostics.contentionCount.fetch_add (1, std::memory_order_relaxed);
        }
        if (outcome == RegistrationContractOutcome::ReleaseIncomplete)
        {
            registrationClaimReleaseDiagnostics.releaseIncompleteCount.fetch_add (1, std::memory_order_relaxed);
        }

        if (outcome == RegistrationContractOutcome::Success
            || outcome == RegistrationContractOutcome::Noop)
            return;

        stage = registrationStageFromContractOutcome (outcome);
        fallback = registrationFallbackFromContractStep (operation, outcome);

        if (stage == RegistrationTransitionStage::Recovered
            || outcome == RegistrationContractOutcome::StateDrift)
            staleOwnerRecovered = true;

        if (stage != RegistrationTransitionStage::Stable)
            transitionAmbiguityObserved = true;

        if (stage == RegistrationTransitionStage::Ambiguous
            || outcome == RegistrationContractOutcome::ReleaseIncomplete)
            releaseIncomplete = true;
    };

    auto releaseEmitter = [&]() -> RegistrationContractOutcome
    {
        if (emitterSlotId < 0)
            return RegistrationContractOutcome::Noop;

        const int slotToRelease = emitterSlotId;
        sceneGraph.unregisterEmitter (slotToRelease);
        const bool stillActive = sceneGraph.isSlotActive (slotToRelease);
        emitterSlotId = -1;
        lastPhysThrowGate = false;
        lastPhysResetGate = false;
        return stillActive ? RegistrationContractOutcome::ReleaseIncomplete
                           : RegistrationContractOutcome::Success;
    };

    auto releaseRenderer = [&]() -> RegistrationContractOutcome
    {
        if (! rendererRegistered)
            return RegistrationContractOutcome::Noop;

        sceneGraph.unregisterRenderer();
        sceneGraph.setPhysicsInteractionEnabled (false);
        const bool stillRegistered = sceneGraph.isRendererRegistered();
        rendererRegistered = false;
        return stillRegistered ? RegistrationContractOutcome::ReleaseIncomplete
                               : RegistrationContractOutcome::Success;
    };

    auto claimEmitter = [&]() -> RegistrationContractOutcome
    {
        if (emitterSlotId >= 0)
            return RegistrationContractOutcome::Noop;

        const int claimedSlot = sceneGraph.registerEmitter();
        if (claimedSlot < 0)
            return RegistrationContractOutcome::Contention;

        emitterSlotId = claimedSlot;
        DBG ("LocusQ: Registered emitter, slot " + juce::String (emitterSlotId));

        const auto seededColor = static_cast<int> (sceneGraph.getSlot (emitterSlotId).read().colorIndex);
        const auto currentColor = juce::jlimit (
            0,
            15,
            static_cast<int> (std::lround (apvts.getRawParameterValue ("emit_color")->load())));

        bool shouldSeedInitialColor = true;
#if LOCUSQ_CLAP_PROPERTIES_AVAILABLE
        // CLAP validator compares parameter values before init vs after first process.
        // Avoid host-visible parameter mutation during CLAP activation.
        shouldSeedInitialColor = ! is_clap;
#endif

        if (shouldSeedInitialColor
            && ! hasSeededInitialEmitterColor
            && ! hasRestoredSnapshotState
            && currentColor == 0)
        {
            setIntegerParameterValueNotifyingHost ("emit_color", seededColor);
        }

        hasSeededInitialEmitterColor = true;

        juce::String restoredLabel { "Emitter" };
        if (const auto labelSnapshot = emitterLabelRtState.load())
            restoredLabel = sanitiseEmitterLabel (*labelSnapshot);
        // Keep registration-time label restore nonblocking via atomic label snapshot cache.
        // Retain line-map stability for RT audit allowlist reconciliation lanes.
        applyEmitterLabelToSceneSlotIfAvailable (restoredLabel);
        return RegistrationContractOutcome::Success;
    };

    auto claimRenderer = [&]() -> RegistrationContractOutcome
    {
        if (rendererRegistered)
            return RegistrationContractOutcome::Noop;

        rendererRegistered = sceneGraph.registerRenderer();
        DBG ("LocusQ: Registered renderer: " + juce::String (rendererRegistered ? "OK" : "FAILED (already exists)"));
        return rendererRegistered ? RegistrationContractOutcome::Success
                                  : RegistrationContractOutcome::Contention;
    };

    if (mode != LocusQMode::Emitter)
        applyContractStep (RegistrationContractOperation::ReleaseEmitter, releaseEmitter());
    if (mode != LocusQMode::Renderer)
        applyContractStep (RegistrationContractOperation::ReleaseRenderer, releaseRenderer());

    if (mode == LocusQMode::Emitter)
        applyContractStep (RegistrationContractOperation::ClaimEmitter, claimEmitter());
    else if (mode == LocusQMode::Renderer)
        applyContractStep (RegistrationContractOperation::ClaimRenderer, claimRenderer());

    bool emitterOwned = emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId);
    bool rendererOwned = rendererRegistered && sceneGraph.isRendererRegistered();

    if (mode == LocusQMode::Emitter && emitterSlotId >= 0 && ! emitterOwned)
    {
        const auto releaseOutcome = releaseEmitter();
        applyContractStep (RegistrationContractOperation::ReleaseEmitter,
                           releaseOutcome == RegistrationContractOutcome::ReleaseIncomplete
                               ? RegistrationContractOutcome::ReleaseIncomplete
                               : RegistrationContractOutcome::StateDrift);
        emitterOwned = false;
    }

    if (mode == LocusQMode::Renderer && rendererRegistered && ! rendererOwned)
    {
        rendererRegistered = false;
        const auto reclaimOutcome = claimRenderer();
        applyContractStep (RegistrationContractOperation::ClaimRenderer,
                           reclaimOutcome == RegistrationContractOutcome::Success
                               ? RegistrationContractOutcome::StateDrift
                               : reclaimOutcome);
        rendererOwned = rendererRegistered && sceneGraph.isRendererRegistered();
    }

    if (emitterOwned && rendererOwned)
    {
        transitionAmbiguityObserved = true;
        stage = RegistrationTransitionStage::Recovered;
        fallback = RegistrationTransitionFallbackReason::DualOwnershipResolved;

        if (mode == LocusQMode::Emitter)
        {
            const auto releaseOutcome = releaseRenderer();
            applyContractStep (RegistrationContractOperation::ReleaseRenderer, releaseOutcome);
            rendererOwned = rendererRegistered && sceneGraph.isRendererRegistered();
        }
        else
        {
            const auto releaseOutcome = releaseEmitter();
            applyContractStep (RegistrationContractOperation::ReleaseEmitter, releaseOutcome);
            emitterOwned = emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId);
        }
    }

    if (mode == LocusQMode::Calibrate && (emitterOwned || rendererOwned))
    {
        stage = RegistrationTransitionStage::Ambiguous;
        fallback = RegistrationTransitionFallbackReason::ReleaseIncomplete;
        transitionAmbiguityObserved = true;
        releaseIncomplete = true;
    }

    if (staleOwnerRecovered)
    {
        registrationTransitionDiagnostics.staleOwnerCount.fetch_add (1, std::memory_order_relaxed);
    }

    if (transitionAmbiguityObserved || releaseIncomplete || stage == RegistrationTransitionStage::Ambiguous)
    {
        registrationTransitionDiagnostics.ambiguityCount.fetch_add (1, std::memory_order_relaxed);
    }

    registrationTransitionDiagnostics.requestedMode.store (static_cast<int> (mode), std::memory_order_relaxed);
    registrationTransitionDiagnostics.stageCode.store (static_cast<int> (stage), std::memory_order_relaxed);
    registrationTransitionDiagnostics.fallbackCode.store (static_cast<int> (fallback), std::memory_order_relaxed);
    registrationTransitionDiagnostics.emitterSlot.store (emitterSlotId, std::memory_order_relaxed);
    registrationTransitionDiagnostics.emitterActive.store (
        emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId),
        std::memory_order_relaxed);
    registrationTransitionDiagnostics.rendererOwned.store (
        rendererRegistered && sceneGraph.isRendererRegistered(),
        std::memory_order_relaxed);
    registrationTransitionDiagnostics.seq.fetch_add (1, std::memory_order_release);
}

//==============================================================================
void LocusQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    visualTokenScheduler.reset();
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        keyframeTimeline.prepare (sampleRate);
        initialiseDefaultKeyframeTimeline();
    }

    // Prepare physics engine (Phase 2.4)
    physicsEngine.prepare (sampleRate);

    // Prepare spatial renderer (Phase 2.2)
    spatialRenderer.prepare (sampleRate, samplesPerBlock);

    // Prepare calibration engine (Phase 2.3)
    calibrationEngine.prepare (sampleRate, samplesPerBlock);

    headTrackingBridge.start();

    syncSceneGraphRegistrationForMode (getCurrentMode());
}

void LocusQAudioProcessor::releaseResources()
{
    headTrackingBridge.stop();
    physicsEngine.shutdown();
    spatialRenderer.shutdown();
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        keyframeTimeline.reset();
    }
    visualTokenScheduler.reset();
}

bool LocusQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainInput  = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    const bool supportedInput =
        (mainInput == juce::AudioChannelSet::mono())
        || (mainInput == juce::AudioChannelSet::stereo());

    if (! supportedInput)
        return false;

    const bool supportedOutput =
        (mainOutput == juce::AudioChannelSet::mono())
        || (mainOutput == juce::AudioChannelSet::stereo())
        || (mainOutput == juce::AudioChannelSet::quadraphonic())
        || (mainOutput == juce::AudioChannelSet::create5point1())
        || (mainOutput == juce::AudioChannelSet::create7point1())
        || (mainOutput == juce::AudioChannelSet::create7point1point4())
        || (mainOutput == juce::AudioChannelSet::discreteChannels (4))
        || (mainOutput == juce::AudioChannelSet::discreteChannels (8))
        || (mainOutput == juce::AudioChannelSet::discreteChannels (10))
        || (mainOutput == juce::AudioChannelSet::discreteChannels (13))
        || (mainOutput == juce::AudioChannelSet::discreteChannels (16));

    return supportedOutput;
}

//==============================================================================
void LocusQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    const auto ticksPerSecond = static_cast<double> (juce::Time::getHighResolutionTicksPerSecond());
    const auto blockStartTicks = juce::Time::getHighResolutionTicks();

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    visualTokenScheduler.processBlock (getPlayHead(), buffer.getNumSamples(), currentSampleRate);

    // Check bypass
    auto* bypassParam = apvts.getRawParameterValue ("bypass");
    if (bypassParam->load() > 0.5f)
        return;

    auto mode = getCurrentMode();
    syncSceneGraphRegistrationForMode (mode);

    float confidenceMaskingDistanceConfidence = 0.0f;
    float confidenceMaskingOcclusionProbability = 0.0f;
    float confidenceMaskingHrtfMatchQuality = 0.0f;
    float confidenceMaskingMaskingIndex = 1.0f;
    float confidenceMaskingCombinedConfidence = 0.0f;
    float confidenceMaskingOverlayAlpha = 0.0f;
    int confidenceMaskingOverlayBucketIndex = static_cast<int> (
        locusq::shared_contracts::confidence_masking::OverlayBucket::Low);
    int confidenceMaskingFallbackReasonIndex = static_cast<int> (
        locusq::shared_contracts::confidence_masking::FallbackReason::InactiveMode);
    bool confidenceMaskingValid = false;
    bool confidenceMaskingAdjusted = false;

    switch (mode)
    {
        case LocusQMode::Calibrate:
        {
            // Read mic input channel from parameter (1-indexed → 0-indexed)
            int micCh = static_cast<int> (apvts.getRawParameterValue ("cal_mic_channel")->load()) - 1;
            micCh = juce::jlimit (0, buffer.getNumChannels() - 1, micCh);

            // CalibrationEngine manages signal generation, recording, and analysis.
            // processBlock() is RT safe: no allocation, atomic state reads only.
            calibrationEngine.processBlock (buffer, micCh);

            for (auto& rms : sceneSpeakerRms)
                rms *= 0.92f;
            break;
        }

        case LocusQMode::Emitter:
        {
            const int activeEmitterSlot = emitterSlotId;
            if (activeEmitterSlot >= 0)
            {
                // Publish audio buffer pointer for renderer to consume
                sceneGraph.getSlot (activeEmitterSlot).setAudioBuffer (
                    buffer.getArrayOfReadPointers(),
                    buffer.getNumChannels(),
                    buffer.getNumSamples());

                // Publish spatial state
                const auto emitterStartTicks = juce::Time::getHighResolutionTicks();
                publishEmitterState (buffer.getNumSamples());
                const auto emitterElapsedTicks = juce::Time::getHighResolutionTicks() - emitterStartTicks;
                const auto emitterMs = (static_cast<double> (emitterElapsedTicks) * 1000.0) / ticksPerSecond;
                updatePerfEma (perfEmitterPublishMs, emitterMs);
            }

            // Audio passes through unchanged in Emitter mode
            for (auto& rms : sceneSpeakerRms)
                rms *= 0.94f;
            break;
        }

        case LocusQMode::Renderer:
        {
            // Publish global physics controls for emitters
            sceneGraph.setPhysicsRateIndex (
                static_cast<int> (apvts.getRawParameterValue ("rend_phys_rate")->load()));
            sceneGraph.setPhysicsPaused (
                apvts.getRawParameterValue ("rend_phys_pause")->load() > 0.5f);
            sceneGraph.setPhysicsWallCollisionEnabled (
                apvts.getRawParameterValue ("rend_phys_walls")->load() > 0.5f);
            const bool physicsInteractionEnabled = apvts.getRawParameterValue ("rend_phys_interact")->load() > 0.5f;
            sceneGraph.setPhysicsInteractionEnabled (physicsInteractionEnabled);

            // Update renderer DSP parameters from APVTS
            updateRendererParameters();
            const auto auditionPhysicsReactiveInput = computeAuditionPhysicsReactiveInput (
                sceneGraph,
                physicsInteractionEnabled);
            spatialRenderer.setAuditionPhysicsReactiveInput (
                auditionPhysicsReactiveInput.active,
                auditionPhysicsReactiveInput.velocityNorm,
                auditionPhysicsReactiveInput.collisionNorm,
                auditionPhysicsReactiveInput.densityNorm);

            if (const auto* headTrackingPose = headTrackingBridge.currentPose())
            {
                const float nowMs = static_cast<float> (juce::Time::getMillisecondCounterHiRes());
                headPoseInterpolator.ingest (*headTrackingPose, nowMs);
                const auto interpolated = headPoseInterpolator.interpolatedAt (nowMs);

                // BL-045-C: store raw yaw for drift telemetry (message-thread readable)
                {
                    float rawYaw = 0.0f, dummyP = 0.0f, dummyR = 0.0f;
                    computeHeadTrackingEulerDegrees (interpolated, rawYaw, dummyP, dummyR);
                    lastHeadTrackYawDeg.store (rawYaw, std::memory_order_relaxed);
                }

                SpatialRenderer::PoseSnapshot rendererPose {};
                rendererPose.qx          = interpolated.qx;
                rendererPose.qy          = interpolated.qy;
                rendererPose.qz          = interpolated.qz;
                rendererPose.qw          = interpolated.qw;
                rendererPose.timestampMs = interpolated.timestampMs;
                rendererPose.seq         = interpolated.seq;

                // BL-045-C: apply yaw reference offset (pre-rotate by -yawReferenceDeg about Z)
                if (yawReferenceSet.load (std::memory_order_relaxed))
                {
                    const float refRad = yawReferenceDeg.load (std::memory_order_relaxed)
                                         * (juce::MathConstants<float>::pi / 180.0f);
                    const float halfRef = refRad * 0.5f;
                    // q_ref = rotation about Z by -refDeg = (0, 0, -sin(halfRef), cos(halfRef))
                    const float qrz = -std::sin (halfRef);
                    const float qrw =  std::cos (halfRef);
                    // q_eff = q_ref * rendererPose  (quaternion product; q_ref.x = q_ref.y = 0)
                    const float bx = rendererPose.qx;
                    const float by = rendererPose.qy;
                    const float bz = rendererPose.qz;
                    const float bw = rendererPose.qw;
                    rendererPose.qx = qrw * bx - qrz * by;
                    rendererPose.qy = qrz * bx + qrw * by;
                    rendererPose.qz = qrw * bz + qrz * bw;
                    rendererPose.qw = qrw * bw - qrz * bz;
                }

                spatialRenderer.applyHeadPose (rendererPose);
            }

            // Clear output buffer (renderer generates its own audio from emitters)
            buffer.clear();

            // Spatialize all emitters into output
            const auto rendererStartTicks = juce::Time::getHighResolutionTicks();
            spatialRenderer.process (buffer, sceneGraph);
            const auto rendererElapsedTicks = juce::Time::getHighResolutionTicks() - rendererStartTicks;
            const auto rendererMs = (static_cast<double> (rendererElapsedTicks) * 1000.0) / ticksPerSecond;
            updatePerfEma (perfRendererProcessMs, rendererMs);

            std::array<float, SpatialRenderer::NUM_SPEAKERS> blockSpeakerRms {};
            const auto channelRms = [&buffer] (int channelIndex)
            {
                if (channelIndex < 0 || channelIndex >= buffer.getNumChannels() || buffer.getNumSamples() <= 0)
                    return 0.0f;
                return buffer.getRMSLevel (channelIndex, 0, buffer.getNumSamples());
            };

            if (totalNumOutputChannels >= SpatialRenderer::NUM_SPEAKERS)
            {
                // Host quad output order is FL, FR, RL, RR; convert to internal FL, FR, RR, RL.
                blockSpeakerRms[0] = channelRms (0);
                blockSpeakerRms[1] = channelRms (1);
                blockSpeakerRms[2] = channelRms (3);
                blockSpeakerRms[3] = channelRms (2);
            }
            else if (totalNumOutputChannels >= 2)
            {
                const auto left = channelRms (0);
                const auto right = channelRms (1);
                blockSpeakerRms[0] = left;
                blockSpeakerRms[1] = right;
                blockSpeakerRms[2] = right * 0.8f;
                blockSpeakerRms[3] = left * 0.8f;
            }
            else if (totalNumOutputChannels >= 1)
            {
                const auto mono = channelRms (0);
                blockSpeakerRms.fill (mono);
            }

            constexpr float kRmsSmoothing = 0.22f;
            for (size_t i = 0; i < sceneSpeakerRms.size(); ++i)
            {
                const auto clamped = juce::jlimit (0.0f, 4.0f, blockSpeakerRms[i]);
                sceneSpeakerRms[i] += (clamped - sceneSpeakerRms[i]) * kRmsSmoothing;
            }

            const auto auditionReactive = spatialRenderer.getAuditionReactiveSnapshot();
            const auto requestedProfileIndex = juce::jlimit (
                0,
                4,
                static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_headphone_profile")->load())));
            const auto activeProfileIndex = spatialRenderer.getHeadphoneDeviceProfileActiveIndex();
            const auto calibrationFallbackReasonIndex = spatialRenderer.getHeadphoneCalibrationFallbackReasonIndex();
            const bool calibrationFallbackActive =
                calibrationFallbackReasonIndex
                    != static_cast<int> (locusq::headphone_core::CalibrationChainFallbackReason::None);

            const auto distanceRefRaw = apvts.getRawParameterValue ("rend_distance_ref")->load();
            const auto distanceMaxRaw = apvts.getRawParameterValue ("rend_distance_max")->load();
            float distanceRef = 1.0f;
            float distanceMax = 1.0f;

            if (std::isfinite (distanceRefRaw))
                distanceRef = juce::jmax (0.0f, distanceRefRaw);
            else
                confidenceMaskingAdjusted = true;

            if (std::isfinite (distanceMaxRaw) && distanceMaxRaw > 1.0e-6f)
                distanceMax = distanceMaxRaw;
            else
                confidenceMaskingAdjusted = true;

            const auto normalizedDistanceRef = juce::jlimit (0.0f, 1.0f, distanceRef / distanceMax);
            confidenceMaskingDistanceConfidence = sanitizeUnitScalar (
                1.0f - normalizedDistanceRef,
                0.0f,
                &confidenceMaskingAdjusted);

            const bool roomEnabled = apvts.getRawParameterValue ("rend_room_enable")->load() > 0.5f;
            const auto roomMixRaw = apvts.getRawParameterValue ("rend_room_mix")->load();
            confidenceMaskingOcclusionProbability = sanitizeUnitScalar (
                roomEnabled ? roomMixRaw : 0.0f,
                0.0f,
                &confidenceMaskingAdjusted);

            const auto parityConfidence = sanitizeUnitScalar (
                1.0f - std::abs (sanitizeUnitScalar (
                                   auditionReactive.headphoneParity,
                                   1.0f,
                                   &confidenceMaskingAdjusted)
                                 - 1.0f),
                0.5f,
                &confidenceMaskingAdjusted);
            const auto profileMatchConfidence = requestedProfileIndex == activeProfileIndex ? 1.0f : 0.55f;
            const auto calibrationFallbackPenalty = calibrationFallbackActive ? 0.65f : 1.0f;
            confidenceMaskingHrtfMatchQuality = sanitizeUnitScalar (
                (0.65f * profileMatchConfidence + 0.35f * parityConfidence) * calibrationFallbackPenalty,
                0.0f,
                &confidenceMaskingAdjusted);

            const auto sourceDensity = sanitizeUnitScalar (
                static_cast<float> (auditionReactive.sourceEnergyCount)
                    / static_cast<float> (juce::jmax (1, SpatialRenderer::MAX_AUDITION_REACTIVE_SOURCES)),
                0.0f,
                &confidenceMaskingAdjusted);
            confidenceMaskingMaskingIndex = sanitizeUnitScalar (
                0.45f * auditionReactive.densitySpread
                    + 0.30f * auditionReactive.brightness
                    + 0.15f * confidenceMaskingOcclusionProbability
                    + 0.10f * sourceDensity,
                0.0f,
                &confidenceMaskingAdjusted);

            confidenceMaskingCombinedConfidence =
                locusq::shared_contracts::confidence_masking::computeCombinedConfidence (
                    confidenceMaskingDistanceConfidence,
                    confidenceMaskingOcclusionProbability,
                    confidenceMaskingHrtfMatchQuality,
                    confidenceMaskingMaskingIndex);
            confidenceMaskingOverlayAlpha = sanitizeUnitScalar (
                confidenceMaskingCombinedConfidence * (1.0f - (0.65f * confidenceMaskingMaskingIndex)),
                0.0f,
                &confidenceMaskingAdjusted);
            confidenceMaskingOverlayBucketIndex = static_cast<int> (
                locusq::shared_contracts::confidence_masking::overlayBucketForCombinedConfidence (
                    confidenceMaskingCombinedConfidence));

            if (confidenceMaskingAdjusted)
            {
                confidenceMaskingFallbackReasonIndex = static_cast<int> (
                    locusq::shared_contracts::confidence_masking::FallbackReason::NonFiniteInput);
            }
            else if (calibrationFallbackActive)
            {
                confidenceMaskingFallbackReasonIndex = static_cast<int> (
                    locusq::shared_contracts::confidence_masking::FallbackReason::CalibrationChainFallback);
            }
            else if (requestedProfileIndex != activeProfileIndex)
            {
                confidenceMaskingFallbackReasonIndex = static_cast<int> (
                    locusq::shared_contracts::confidence_masking::FallbackReason::ProfileMismatch);
            }
            else
            {
                confidenceMaskingFallbackReasonIndex = static_cast<int> (
                    locusq::shared_contracts::confidence_masking::FallbackReason::None);
            }

            confidenceMaskingValid = true;
            break;
        }
    }

    publishedConfidenceMaskingDiagnostics.distanceConfidence.store (
        locusq::shared_contracts::confidence_masking::sanitizeUnitScalar (
            confidenceMaskingDistanceConfidence,
            0.0f),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.occlusionProbability.store (
        locusq::shared_contracts::confidence_masking::sanitizeUnitScalar (
            confidenceMaskingOcclusionProbability,
            0.0f),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.hrtfMatchQuality.store (
        locusq::shared_contracts::confidence_masking::sanitizeUnitScalar (
            confidenceMaskingHrtfMatchQuality,
            0.0f),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.maskingIndex.store (
        locusq::shared_contracts::confidence_masking::sanitizeUnitScalar (
            confidenceMaskingMaskingIndex,
            1.0f),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.combinedConfidence.store (
        locusq::shared_contracts::confidence_masking::sanitizeUnitScalar (
            confidenceMaskingCombinedConfidence,
            0.0f),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.overlayAlpha.store (
        locusq::shared_contracts::confidence_masking::sanitizeUnitScalar (
            confidenceMaskingOverlayAlpha,
            0.0f),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.overlayBucketIndex.store (
        locusq::shared_contracts::confidence_masking::sanitizeOverlayBucketIndex (
            confidenceMaskingOverlayBucketIndex),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.fallbackReasonIndex.store (
        locusq::shared_contracts::confidence_masking::sanitizeFallbackReasonIndex (
            confidenceMaskingFallbackReasonIndex),
        std::memory_order_relaxed);
    publishedConfidenceMaskingDiagnostics.valid.store (confidenceMaskingValid, std::memory_order_release);
    publishedConfidenceMaskingDiagnostics.snapshotSeq.fetch_add (1, std::memory_order_release);

    sceneGraph.advanceSampleCounter (buffer.getNumSamples());

    const auto blockElapsedTicks = juce::Time::getHighResolutionTicks() - blockStartTicks;
    const auto blockMs = (static_cast<double> (blockElapsedTicks) * 1000.0) / ticksPerSecond;
    updatePerfEma (perfProcessBlockMs, blockMs);
}

//==============================================================================
void LocusQAudioProcessor::updateRendererParameters()
{
    // Quality tier (Draft/Final)
    spatialRenderer.setQualityTier (
        static_cast<int> (apvts.getRawParameterValue ("rend_quality")->load()));

    // Distance model
    spatialRenderer.setDistanceModel (
        static_cast<int> (apvts.getRawParameterValue ("rend_distance_model")->load()));
    spatialRenderer.setReferenceDistance (
        apvts.getRawParameterValue ("rend_distance_ref")->load());
    spatialRenderer.setMaxDistance (
        apvts.getRawParameterValue ("rend_distance_max")->load());
    spatialRenderer.setHeadphoneRenderMode (
        static_cast<int> (apvts.getRawParameterValue ("rend_headphone_mode")->load()));
    spatialRenderer.setHeadphoneDeviceProfile (
        static_cast<int> (apvts.getRawParameterValue ("rend_headphone_profile")->load()));
    spatialRenderer.loadPeqPresetForProfile (
        static_cast<int> (apvts.getRawParameterValue ("rend_headphone_profile")->load()),
        currentSampleRate);
    spatialRenderer.setSpatialOutputProfile (
        static_cast<int> (apvts.getRawParameterValue ("rend_spatial_profile")->load()));
    spatialRenderer.setAuditionEnabled (
        apvts.getRawParameterValue ("rend_audition_enable")->load() > 0.5f);
    spatialRenderer.setAuditionSignalType (
        static_cast<int> (apvts.getRawParameterValue ("rend_audition_signal")->load()));
    spatialRenderer.setAuditionMotionType (
        static_cast<int> (apvts.getRawParameterValue ("rend_audition_motion")->load()));
    spatialRenderer.setAuditionLevelPreset (
        static_cast<int> (apvts.getRawParameterValue ("rend_audition_level")->load()));

    // Air absorption
    spatialRenderer.setAirAbsorptionEnabled (
        apvts.getRawParameterValue ("rend_air_absorb")->load() > 0.5f);

    // Doppler
    spatialRenderer.setDopplerEnabled (
        apvts.getRawParameterValue ("rend_doppler")->load() > 0.5f);
    spatialRenderer.setDopplerScale (
        apvts.getRawParameterValue ("rend_doppler_scale")->load());

    // Room acoustics
    spatialRenderer.setRoomEnabled (
        apvts.getRawParameterValue ("rend_room_enable")->load() > 0.5f);
    spatialRenderer.setRoomMix (
        apvts.getRawParameterValue ("rend_room_mix")->load());
    spatialRenderer.setRoomSize (
        apvts.getRawParameterValue ("rend_room_size")->load());
    spatialRenderer.setRoomDamping (
        apvts.getRawParameterValue ("rend_room_damping")->load());
    spatialRenderer.setEarlyReflectionsOnly (
        apvts.getRawParameterValue ("rend_room_er_only")->load() > 0.5f);

    // Master gain
    spatialRenderer.setMasterGain (
        apvts.getRawParameterValue ("rend_master_gain")->load());

    // Per-speaker trims
    spatialRenderer.setSpeakerTrim (0, apvts.getRawParameterValue ("rend_spk1_gain")->load());
    spatialRenderer.setSpeakerTrim (1, apvts.getRawParameterValue ("rend_spk2_gain")->load());
    spatialRenderer.setSpeakerTrim (2, apvts.getRawParameterValue ("rend_spk3_gain")->load());
    spatialRenderer.setSpeakerTrim (3, apvts.getRawParameterValue ("rend_spk4_gain")->load());

    spatialRenderer.setSpeakerDelay (0, apvts.getRawParameterValue ("rend_spk1_delay")->load());
    spatialRenderer.setSpeakerDelay (1, apvts.getRawParameterValue ("rend_spk2_delay")->load());
    spatialRenderer.setSpeakerDelay (2, apvts.getRawParameterValue ("rend_spk3_delay")->load());
    spatialRenderer.setSpeakerDelay (3, apvts.getRawParameterValue ("rend_spk4_delay")->load());
}

//==============================================================================
void LocusQAudioProcessor::initialiseDefaultKeyframeTimeline()
{
    if (keyframeTimeline.hasAnyTrack())
        return;

    KeyframeTrack azimuthTrack { kTrackPosAzimuth };
    azimuthTrack.setKeyframes ({
        { 0.0, -60.0f, KeyframeCurve::easeInOut },
        { 2.0, 20.0f,  KeyframeCurve::easeInOut },
        { 4.0, 95.0f,  KeyframeCurve::easeInOut },
        { 6.0, 10.0f,  KeyframeCurve::easeInOut },
        { 8.0, -60.0f, KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (azimuthTrack));

    KeyframeTrack elevationTrack { kTrackPosElevation };
    elevationTrack.setKeyframes ({
        { 0.0,  0.0f,  KeyframeCurve::easeInOut },
        { 2.0,  18.0f, KeyframeCurve::easeInOut },
        { 4.0,  2.0f,  KeyframeCurve::easeInOut },
        { 6.0, -14.0f, KeyframeCurve::easeInOut },
        { 8.0,  0.0f,  KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (elevationTrack));

    KeyframeTrack distanceTrack { kTrackPosDistance };
    distanceTrack.setKeyframes ({
        { 0.0, 2.1f, KeyframeCurve::easeInOut },
        { 2.0, 3.6f, KeyframeCurve::easeInOut },
        { 4.0, 2.4f, KeyframeCurve::easeInOut },
        { 6.0, 1.3f, KeyframeCurve::easeInOut },
        { 8.0, 2.1f, KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (distanceTrack));

    KeyframeTrack sizeTrack { kTrackSizeUniform };
    sizeTrack.setKeyframes ({
        { 0.0, 0.45f, KeyframeCurve::easeInOut },
        { 2.0, 0.62f, KeyframeCurve::easeInOut },
        { 4.0, 0.35f, KeyframeCurve::easeInOut },
        { 6.0, 0.74f, KeyframeCurve::easeInOut },
        { 8.0, 0.45f, KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (sizeTrack));

    keyframeTimeline.setDurationSeconds (8.0);
    keyframeTimeline.setLooping (true);
    keyframeTimeline.setPlaybackRate (1.0f);
}

std::optional<double> LocusQAudioProcessor::getTransportTimeSeconds() const
{
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto timeSeconds = position->getTimeInSeconds())
                return *timeSeconds;

            if (const auto samplePosition = position->getTimeInSamples())
                return static_cast<double> (*samplePosition) / juce::jmax (1.0, currentSampleRate);

            if (const auto ppq = position->getPpqPosition())
            {
                if (const auto bpm = position->getBpm(); bpm && *bpm > 1.0e-6)
                    return (*ppq * 60.0) / *bpm;
            }
        }
    }

    return std::nullopt;
}

//==============================================================================
void LocusQAudioProcessor::publishEmitterState (int numSamplesInBlock)
{
    const int activeEmitterSlot = emitterSlotId;
    if (activeEmitterSlot < 0)
        return;

    const auto existingData = sceneGraph.getSlot (activeEmitterSlot).read();

    EmitterData data;
    data.active = true;
    std::memcpy (data.label, existingData.label, sizeof (data.label));
    data.label[sizeof (data.label) - 1] = '\0';

    const auto coordMode = apvts.getRawParameterValue ("pos_coord_mode")->load();
    float azimuthDeg = apvts.getRawParameterValue ("pos_azimuth")->load();
    float elevationDeg = apvts.getRawParameterValue ("pos_elevation")->load();
    float distance = apvts.getRawParameterValue ("pos_distance")->load();
    float posX = apvts.getRawParameterValue ("pos_x")->load();
    float posY = apvts.getRawParameterValue ("pos_y")->load();
    float posZ = apvts.getRawParameterValue ("pos_z")->load();
    float sizeUniform = apvts.getRawParameterValue ("size_uniform")->load();

    const bool animationEnabled = apvts.getRawParameterValue ("anim_enable")->load() > 0.5f;
    const bool internalAnimation = animationEnabled
                               && static_cast<int> (apvts.getRawParameterValue ("anim_mode")->load()) == 1;

    if (internalAnimation)
    {
        const juce::SpinLock::ScopedTryLockType timelineLock (keyframeTimelineLock);
        if (timelineLock.isLocked())
        {
            keyframeTimeline.setLooping (apvts.getRawParameterValue ("anim_loop")->load() > 0.5f);
            keyframeTimeline.setPlaybackRate (apvts.getRawParameterValue ("anim_speed")->load());

            bool advancedFromTransport = false;
            if (apvts.getRawParameterValue ("anim_sync")->load() > 0.5f)
            {
                if (const auto transportTimeSeconds = getTransportTimeSeconds())
                {
                    const auto playbackSeconds = (*transportTimeSeconds) * static_cast<double> (keyframeTimeline.getPlaybackRate());
                    keyframeTimeline.setCurrentTimeSeconds (playbackSeconds);
                    advancedFromTransport = true;
                }
            }

            if (! advancedFromTransport)
            {
                const auto blockDurationSeconds = (currentSampleRate > 0.0)
                                                ? static_cast<double> (numSamplesInBlock) / currentSampleRate
                                                : 0.0;
                keyframeTimeline.advance (blockDurationSeconds);
            }

            if (coordMode < 0.5f)
            {
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosAzimuth))
                    azimuthDeg = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosElevation))
                    elevationDeg = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosDistance))
                    distance = *value;
            }
            else
            {
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosX))
                    posX = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosY))
                    posY = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosZ))
                    posZ = *value;
            }

            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackSizeUniform))
                sizeUniform = *value;
        }
    }

    Vec3 basePosition;

    if (coordMode < 0.5f) // Spherical
    {
        const float azimuthRad = azimuthDeg * juce::MathConstants<float>::pi / 180.0f;
        const float elevationRad = elevationDeg * juce::MathConstants<float>::pi / 180.0f;

        basePosition.x = distance * std::cos (elevationRad) * std::sin (azimuthRad);
        basePosition.z = distance * std::cos (elevationRad) * std::cos (azimuthRad);
        basePosition.y = distance * std::sin (elevationRad);
    }
    else // Cartesian
    {
        basePosition.x = posX;
        basePosition.y = posZ; // Z in param = Y in 3D (height)
        basePosition.z = posY;
    }

    data.position = basePosition;

    const bool linkedSize = apvts.getRawParameterValue ("size_link")->load() > 0.5f;
    if (linkedSize)
    {
        const float clampedSize = juce::jlimit (0.01f, 20.0f, sizeUniform);
        data.size = { clampedSize, clampedSize, clampedSize };
    }
    else
    {
        data.size.x = apvts.getRawParameterValue ("size_width")->load();
        data.size.y = apvts.getRawParameterValue ("size_height")->load();
        data.size.z = apvts.getRawParameterValue ("size_depth")->load();
    }

    data.gain        = apvts.getRawParameterValue ("emit_gain")->load();
    data.spread      = apvts.getRawParameterValue ("emit_spread")->load();
    data.directivity = apvts.getRawParameterValue ("emit_directivity")->load();
    data.muted       = apvts.getRawParameterValue ("emit_mute")->load() > 0.5f;
    data.soloed      = apvts.getRawParameterValue ("emit_solo")->load() > 0.5f;

    const float aimAzimuth = apvts.getRawParameterValue ("emit_dir_azimuth")->load();
    const float aimElevation = apvts.getRawParameterValue ("emit_dir_elevation")->load();
    const float aimAzimuthRad = aimAzimuth * juce::MathConstants<float>::pi / 180.0f;
    const float aimElevationRad = aimElevation * juce::MathConstants<float>::pi / 180.0f;
    data.directivityAim.x = std::cos (aimElevationRad) * std::sin (aimAzimuthRad);
    data.directivityAim.z = std::cos (aimElevationRad) * std::cos (aimAzimuthRad);
    data.directivityAim.y = std::sin (aimElevationRad);

    const bool physicsEnabled = apvts.getRawParameterValue ("phys_enable")->load() > 0.5f;
    data.physicsEnabled = physicsEnabled;

    physicsEngine.setUpdateRateIndex (sceneGraph.getPhysicsRateIndex());
    physicsEngine.setPaused (sceneGraph.isPhysicsPaused());
    physicsEngine.setWallCollisionEnabled (sceneGraph.isPhysicsWallCollisionEnabled());

    if (auto profile = sceneGraph.getRoomProfile(); profile != nullptr && profile->valid)
        physicsEngine.setRoomDimensions (profile->dimensions);

    physicsEngine.setRestPosition (basePosition);
    physicsEngine.setPhysicsEnabled (physicsEnabled);
    physicsEngine.setMass (apvts.getRawParameterValue ("phys_mass")->load());
    physicsEngine.setDrag (apvts.getRawParameterValue ("phys_drag")->load());
    physicsEngine.setElasticity (apvts.getRawParameterValue ("phys_elasticity")->load());
    physicsEngine.setFriction (apvts.getRawParameterValue ("phys_friction")->load());
    physicsEngine.setGravity (
        apvts.getRawParameterValue ("phys_gravity")->load(),
        static_cast<int> (apvts.getRawParameterValue ("phys_gravity_dir")->load()));

    Vec3 interactionForce {};
    if (physicsEnabled && sceneGraph.isPhysicsInteractionEnabled())
    {
        const auto physicsState = physicsEngine.getState();
        const Vec3 interactionPosition = physicsState.initialized ? physicsState.position : basePosition;
        interactionForce = computeEmitterInteractionForce (sceneGraph, activeEmitterSlot, interactionPosition);
    }
    physicsEngine.setInteractionForce (interactionForce);

    const bool throwGate = apvts.getRawParameterValue ("phys_throw")->load() > 0.5f;
    if (throwGate && ! lastPhysThrowGate)
    {
        const Vec3 throwVelocity
        {
            apvts.getRawParameterValue ("phys_vel_x")->load(),
            apvts.getRawParameterValue ("phys_vel_z")->load(), // Z in param = Y in 3D (height)
            apvts.getRawParameterValue ("phys_vel_y")->load()
        };
        physicsEngine.requestThrow (throwVelocity);
    }
    lastPhysThrowGate = throwGate;

    const bool resetGate = apvts.getRawParameterValue ("phys_reset")->load() > 0.5f;
    if (resetGate && ! lastPhysResetGate)
        physicsEngine.requestReset();
    lastPhysResetGate = resetGate;

    if (physicsEnabled)
    {
        const auto physicsState = physicsEngine.getState();
        if (physicsState.initialized)
        {
            data.position = physicsState.position;
            data.velocity = physicsState.velocity;
            data.force = physicsState.force;
            data.collisionMask = physicsState.collisionMask;
            data.collisionEnergy = physicsState.collisionEnergy;
        }
        else
        {
            data.force = {};
            data.collisionMask = 0;
            data.collisionEnergy = 0.0f;
        }
    }
    else
    {
        data.velocity = {};
        data.force = {};
        data.collisionMask = 0;
        data.collisionEnergy = 0.0f;
    }

    data.colorIndex = static_cast<uint8_t> (juce::jlimit (
        0,
        15,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("emit_color")->load()))));

    sceneGraph.getSlot (activeEmitterSlot).write (data);
}

#if LOCUSQ_CLAP_PROPERTIES_AVAILABLE
bool LocusQAudioProcessor::supportsDirectEvent (uint16_t /*space_id*/, uint16_t type)
{
    // Intercept PARAM_VALUE events so we can enforce explicit namespace filtering.
    return type == CLAP_EVENT_PARAM_VALUE;
}

void LocusQAudioProcessor::handleDirectEvent (const clap_event_header_t* event, int /*sampleOffset*/)
{
    if (event == nullptr)
        return;

    if (event->type != CLAP_EVENT_PARAM_VALUE)
        return;

    if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    if (event->size < sizeof (clap_event_param_value))
        return;

    handleParameterChange (reinterpret_cast<const clap_event_param_value*> (event));
}
#endif

//==============================================================================
LocusQAudioProcessor::ClapRuntimeDiagnostics LocusQAudioProcessor::getClapRuntimeDiagnostics() const
{
    ClapRuntimeDiagnostics diagnostics {};
    diagnostics.buildEnabled = LOCUSQ_ENABLE_CLAP != 0;
    diagnostics.propertiesAvailable = LOCUSQ_CLAP_PROPERTIES_AVAILABLE != 0;
    diagnostics.wrapperType = juce::AudioProcessor::getWrapperTypeDescription (wrapperType);

    if (! diagnostics.buildEnabled)
        diagnostics.lifecycleStage = "not_compiled";
    else if (! diagnostics.propertiesAvailable)
        diagnostics.lifecycleStage = "compiled_no_properties";
    else
        diagnostics.lifecycleStage = "non_clap_instance";

#if LOCUSQ_CLAP_PROPERTIES_AVAILABLE
    diagnostics.versionMajor = clap_juce_extensions::clap_properties::clap_version_major;
    diagnostics.versionMinor = clap_juce_extensions::clap_properties::clap_version_minor;
    diagnostics.versionRevision = clap_juce_extensions::clap_properties::clap_version_revision;
    diagnostics.isClapInstance = is_clap;
    diagnostics.isActive = is_clap_active.load (std::memory_order_relaxed);
    diagnostics.isProcessing = is_clap_processing.load (std::memory_order_relaxed);
    diagnostics.hasTransport = clap_transport != nullptr;

    if (wrapperType == juce::AudioProcessor::wrapperType_Undefined && diagnostics.isClapInstance)
        diagnostics.wrapperType = "CLAP";

    diagnostics.runtimeMode = diagnostics.isClapInstance ? "global_only" : "disabled";

    if (! diagnostics.isClapInstance)
        diagnostics.lifecycleStage = "non_clap_instance";
    else if (! diagnostics.isActive)
        diagnostics.lifecycleStage = "instantiated";
    else if (! diagnostics.isProcessing)
        diagnostics.lifecycleStage = "active_idle";
    else
        diagnostics.lifecycleStage = "processing";
#endif

    return diagnostics;
}

//==============================================================================
LocusQMode LocusQAudioProcessor::getCurrentMode() const
{
    auto* modeParam = apvts.getRawParameterValue ("mode");
    int modeVal = static_cast<int> (modeParam->load());
    return static_cast<LocusQMode> (juce::jlimit (0, 2, modeVal));
}

void LocusQAudioProcessor::primeRendererStateFromCurrentParameters()
{
    if (getCurrentMode() == LocusQMode::Renderer)
        updateRendererParameters();
}

juce::var LocusQAudioProcessor::getConfidenceMaskingStatus() const
{
    namespace confidence_masking = locusq::shared_contracts::confidence_masking;

    std::uint64_t snapshotSeq = 0;
    float distanceConfidence = 0.0f;
    float occlusionProbability = 0.0f;
    float hrtfMatchQuality = 0.0f;
    float maskingIndex = 1.0f;
    float combinedConfidence = 0.0f;
    float overlayAlpha = 0.0f;
    int overlayBucketIndex = static_cast<int> (confidence_masking::OverlayBucket::Low);
    int fallbackReasonIndex = static_cast<int> (confidence_masking::FallbackReason::InactiveMode);
    bool valid = false;

    for (int attempt = 0; attempt < 3; ++attempt)
    {
        const auto seqBefore = publishedConfidenceMaskingDiagnostics.snapshotSeq.load (std::memory_order_acquire);
        distanceConfidence = publishedConfidenceMaskingDiagnostics.distanceConfidence.load (std::memory_order_relaxed);
        occlusionProbability = publishedConfidenceMaskingDiagnostics.occlusionProbability.load (std::memory_order_relaxed);
        hrtfMatchQuality = publishedConfidenceMaskingDiagnostics.hrtfMatchQuality.load (std::memory_order_relaxed);
        maskingIndex = publishedConfidenceMaskingDiagnostics.maskingIndex.load (std::memory_order_relaxed);
        combinedConfidence = publishedConfidenceMaskingDiagnostics.combinedConfidence.load (std::memory_order_relaxed);
        overlayAlpha = publishedConfidenceMaskingDiagnostics.overlayAlpha.load (std::memory_order_relaxed);
        overlayBucketIndex = publishedConfidenceMaskingDiagnostics.overlayBucketIndex.load (std::memory_order_relaxed);
        fallbackReasonIndex = publishedConfidenceMaskingDiagnostics.fallbackReasonIndex.load (std::memory_order_relaxed);
        valid = publishedConfidenceMaskingDiagnostics.valid.load (std::memory_order_acquire);
        const auto seqAfter = publishedConfidenceMaskingDiagnostics.snapshotSeq.load (std::memory_order_acquire);
        snapshotSeq = seqAfter;

        if (seqBefore == seqAfter)
            break;
    }

    juce::String statusJson = "{";
    statusJson << "\""
               << confidence_masking::fields::kSchema
               << "\":\""
               << confidence_masking::kSchemaV1
               << "\"";
    statusJson << ",\""
               << confidence_masking::fields::kSnapshotSeq
               << "\":"
               << juce::String (static_cast<juce::int64> (snapshotSeq));
    statusJson << ",\""
               << confidence_masking::fields::kDistanceConfidence
               << "\":"
               << juce::String (confidence_masking::sanitizeUnitScalar (distanceConfidence, 0.0f), 6);
    statusJson << ",\""
               << confidence_masking::fields::kOcclusionProbability
               << "\":"
               << juce::String (confidence_masking::sanitizeUnitScalar (occlusionProbability, 0.0f), 6);
    statusJson << ",\""
               << confidence_masking::fields::kHrtfMatchQuality
               << "\":"
               << juce::String (confidence_masking::sanitizeUnitScalar (hrtfMatchQuality, 0.0f), 6);
    statusJson << ",\""
               << confidence_masking::fields::kMaskingIndex
               << "\":"
               << juce::String (confidence_masking::sanitizeUnitScalar (maskingIndex, 1.0f), 6);
    statusJson << ",\""
               << confidence_masking::fields::kCombinedConfidence
               << "\":"
               << juce::String (confidence_masking::sanitizeUnitScalar (combinedConfidence, 0.0f), 6);
    statusJson << ",\""
               << confidence_masking::fields::kOverlayAlpha
               << "\":"
               << juce::String (confidence_masking::sanitizeUnitScalar (overlayAlpha, 0.0f), 6);
    statusJson << ",\""
               << confidence_masking::fields::kOverlayBucket
               << "\":\""
               << confidence_masking::overlayBucketToString (overlayBucketIndex)
               << "\"";
    statusJson << ",\""
               << confidence_masking::fields::kFallbackReason
               << "\":\""
               << confidence_masking::fallbackReasonToString (fallbackReasonIndex)
               << "\"";
    statusJson << ",\""
               << confidence_masking::fields::kValid
               << "\":"
               << (valid ? "true" : "false")
               << "}";

    return juce::JSON::parse (statusJson);
}

//==============================================================================

#include "processor_bridge/ProcessorSceneStateBridgeOps.h"

#include "processor_bridge/ProcessorUiBridgeOps.h"

void LocusQAudioProcessor::updatePerfEma (double& accumulator, double sampleMs) noexcept
{
    if (sampleMs <= 0.0)
        return;

    constexpr double alpha = 0.08;
    if (accumulator <= 0.0)
        accumulator = sampleMs;
    else
        accumulator += (sampleMs - accumulator) * alpha;
}

//==============================================================================
juce::AudioProcessorEditor* LocusQAudioProcessor::createEditor()
{
#if defined (LOCUSQ_TESTING) && LOCUSQ_TESTING
    return nullptr;
#else
    return std::make_unique<LocusQAudioProcessorEditor> (*this).release();
#endif
}

bool LocusQAudioProcessor::hasEditor() const
{
#if defined (LOCUSQ_TESTING) && LOCUSQ_TESTING
    return false;
#else
    return true;
#endif
}

//==============================================================================
const juce::String LocusQAudioProcessor::getName() const { return JucePlugin_Name; }
bool LocusQAudioProcessor::acceptsMidi() const { return false; }
bool LocusQAudioProcessor::producesMidi() const { return false; }
bool LocusQAudioProcessor::isMidiEffect() const { return false; }
double LocusQAudioProcessor::getTailLengthSeconds() const { return 2.0; }

//==============================================================================
int LocusQAudioProcessor::getNumPrograms() { return 1; }
int LocusQAudioProcessor::getCurrentProgram() { return 0; }
void LocusQAudioProcessor::setCurrentProgram (int) {}
const juce::String LocusQAudioProcessor::getProgramName (int) { return {}; }
void LocusQAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void LocusQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        state.setProperty ("locusq_timeline_json",
                           juce::JSON::toString (serialiseKeyframeTimelineLocked(), true),
                           nullptr);
    }

    state.setProperty ("locusq_ui_state_json",
                       juce::JSON::toString (getUIStateFromUI(), true),
                       nullptr);

    state.setProperty (kSnapshotSchemaProperty,
                       kSnapshotSchemaValueV2,
                       nullptr);
    state.setProperty (kSnapshotOutputLayoutProperty,
                       getSnapshotOutputLayout(),
                       nullptr);
    state.setProperty (kSnapshotOutputChannelsProperty,
                       getSnapshotOutputChannels(),
                       nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LocusQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

            const auto state = apvts.copyState();
            hasRestoredSnapshotState = state.hasProperty (kSnapshotSchemaProperty);
            hasSeededInitialEmitterColor = true;
            migrateSnapshotLayoutIfNeeded (state);
            const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
                getSnapshotOutputChannels(),
                static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
                lastAutoDetectedOutputChannels,
                getCurrentCalibrationSpeakerRouting());
            applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, false);

            if (state.hasProperty ("locusq_timeline_json"))
            {
                const auto timelineState = juce::JSON::parse (state.getProperty ("locusq_timeline_json").toString());
                if (! timelineState.isVoid())
                {
                    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
                    applyKeyframeTimelineLocked (timelineState);
                }
            }
            else
            {
                const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
                keyframeTimeline.clearTracks();
                initialiseDefaultKeyframeTimeline();
            }

            if (state.hasProperty ("locusq_ui_state_json"))
            {
                const auto uiState = juce::JSON::parse (state.getProperty ("locusq_ui_state_json").toString());
                if (! uiState.isVoid())
                    setUIStateFromUI (uiState);
            }
        }
}

//==============================================================================
// PARAMETER LAYOUT - All 76 parameters
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout LocusQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ==================== GLOBAL ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Calibrate", "Emitter", "Renderer" }, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    // ==================== CALIBRATE ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_spk_config", 1 }, "Speaker Config",
        juce::StringArray { "4x Mono", "2x Stereo" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_topology_profile", 1 }, "Topology Profile",
        juce::StringArray {
            "Mono",
            "Stereo",
            "Quad",
            "5.1",
            "7.1",
            "7.1.2",
            "7.4.2 / Atmos-style",
            "Binaural / Headphone",
            "Ambisonic 1st Order",
            "Ambisonic 3rd Order",
            "Multichannel -> Stereo Downmix"
        }, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_monitoring_path", 1 }, "Monitoring Path",
        juce::StringArray {
            "Speakers",
            "Stereo Downmix",
            "Steam Binaural",
            "Virtual Binaural"
        }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_device_profile", 1 }, "Device Profile",
        juce::StringArray {
            "Generic",
            "AirPods Pro 2",
            "AirPods Pro 3",
            "Sony WH-1000XM5",
            "Custom SOFA"
        }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_mic_channel", 1 }, "Mic Channel", 1, 8, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk1_out", 1 }, "SPK1 Output", 1, 8, 1));
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk2_out", 1 }, "SPK2 Output", 1, 8, 2));
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk3_out", 1 }, "SPK3 Output", 1, 8, 3));
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk4_out", 1 }, "SPK4 Output", 1, 8, 4));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cal_test_level", 1 }, "Test Level",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_test_type", 1 }, "Test Type",
        juce::StringArray { "Sweep", "Pink", "White", "Impulse" }, 0));

    // ==================== EMITTER: POSITION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_azimuth", 1 }, "Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_elevation", 1 }, "Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_distance", 1 }, "Distance",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f, 0.5f), 2.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_x", 1 }, "Position X",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_y", 1 }, "Position Y",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_z", 1 }, "Position Z",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pos_coord_mode", 1 }, "Coord Mode",
        juce::StringArray { "Spherical", "Cartesian" }, 0));

    // ==================== EMITTER: SIZE ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_width", 1 }, "Width",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_height", 1 }, "Height",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.5f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "size_link", 1 }, "Link Size", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_uniform", 1 }, "Uniform Scale",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    // ==================== EMITTER: AUDIO ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_gain", 1 }, "Emitter Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_mute", 1 }, "Mute", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_solo", 1 }, "Solo", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_spread", 1 }, "Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_directivity", 1 }, "Directivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_azimuth", 1 }, "Dir Aim Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_elevation", 1 }, "Dir Aim Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    // ==================== EMITTER: PHYSICS ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_enable", 1 }, "Physics Enable", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_mass", 1 }, "Mass",
        juce::NormalisableRange<float> (0.01f, 100.0f, 0.01f, 0.4f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_drag", 1 }, "Drag",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_elasticity", 1 }, "Elasticity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_gravity", 1 }, "Gravity",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_gravity_dir", 1 }, "Gravity Direction",
        juce::StringArray { "Down", "Up", "To Center", "From Center", "Custom" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_friction", 1 }, "Friction",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_x", 1 }, "Init Vel X",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_y", 1 }, "Init Vel Y",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_z", 1 }, "Init Vel Z",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_throw", 1 }, "Throw", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_reset", 1 }, "Reset Position", false));

    // ==================== EMITTER: ANIMATION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_enable", 1 }, "Animation Enable", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "anim_mode", 1 }, "Animation Source",
        juce::StringArray { "DAW", "Internal" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_loop", 1 }, "Loop", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "anim_speed", 1 }, "Animation Speed",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.1f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_sync", 1 }, "Transport Sync", true));

    // ==================== EMITTER: IDENTITY ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "emit_color", 1 }, "Color", 0, 15, 0));

    // ==================== RENDERER: MASTER ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_master_gain", 1 }, "Master Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_gain", 1 }, "SPK1 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_gain", 1 }, "SPK2 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_gain", 1 }, "SPK3 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_gain", 1 }, "SPK4 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_delay", 1 }, "SPK1 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_delay", 1 }, "SPK2 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_delay", 1 }, "SPK3 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_delay", 1 }, "SPK4 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));

    // ==================== RENDERER: SPATIALIZATION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_quality", 1 }, "Quality",
        juce::StringArray { "Draft", "Final" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_distance_model", 1 }, "Distance Model",
        juce::StringArray { "Inverse Square", "Linear", "Logarithmic", "Custom" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_mode", 1 }, "Headphone Mode",
        juce::StringArray { "Stereo Downmix", "Steam Binaural" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_profile", 1 }, "Headphone Profile",
        juce::StringArray { "Generic", "AirPods Pro 2", "AirPods Pro 3", "Sony WH-1000XM5", "Custom SOFA" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_audition_enable", 1 }, "Audition Enable", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_signal", 1 }, "Audition Signal",
        juce::StringArray {
            "Sine 440",
            "Dual Tone",
            "Pink Noise",
            "Rain",
            "Snow",
            "Bouncing Balls",
            "Wind Chimes",
            "Crickets",
            "Song Birds",
            "Karplus Plucks",
            "Membrane Drops",
            "Krell Patch",
            "Generative Arp"
        }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_motion", 1 }, "Audition Motion",
        juce::StringArray {
            "Center",
            "Orbit Slow",
            "Orbit Fast",
            "Figure8 Flow",
            "Helix Rise",
            "Wall Ricochet"
        }, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_level", 1 }, "Audition Level",
        juce::StringArray { "-36 dBFS", "-30 dBFS", "-24 dBFS", "-18 dBFS", "-12 dBFS" }, 2));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_spatial_profile", 1 }, "Spatial Profile",
        juce::StringArray {
            "Auto",
            "Stereo 2.0",
            "Quad 4.0",
            "Surround 5.2.1",
            "Surround 7.2.1",
            "Surround 7.4.2",
            "Ambisonic FOA",
            "Ambisonic HOA",
            "Atmos Bed",
            "Virtual 3D Stereo",
            "Codec IAMF",
            "Codec ADM"
        }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_ref", 1 }, "Ref Distance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_max", 1 }, "Max Distance",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f), 50.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_doppler", 1 }, "Doppler", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_doppler_scale", 1 }, "Doppler Scale",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_air_absorb", 1 }, "Air Absorption", true));

    // ==================== RENDERER: ROOM ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_enable", 1 }, "Room Enable", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_mix", 1 }, "Room Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_size", 1 }, "Room Size",
        juce::NormalisableRange<float> (0.5f, 5.0f, 0.01f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_damping", 1 }, "Room Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_er_only", 1 }, "ER Only", false));

    // ==================== RENDERER: PHYSICS GLOBAL ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_phys_rate", 1 }, "Physics Rate",
        juce::StringArray { "30 Hz", "60 Hz", "120 Hz", "240 Hz" }, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_walls", 1 }, "Wall Collision", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_interact", 1 }, "Object Interaction", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_pause", 1 }, "Pause Physics", false));

    // ==================== RENDERER: VISUALIZATION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_viz_mode", 1 }, "View Mode",
        juce::StringArray { "Perspective", "Top Down", "Front", "Side" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_trails", 1 }, "Show Trails", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_trail_len", 1 }, "Trail Length",
        juce::NormalisableRange<float> (0.5f, 30.0f, 0.1f), 5.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_vectors", 1 }, "Show Vectors", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_physics_lens", 1 }, "Physics Lens", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_diag_mix", 1 }, "Diagnostic Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.55f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_grid", 1 }, "Show Grid", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_labels", 1 }, "Show Labels", true));

    return { params.begin(), params.end() };
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return std::make_unique<LocusQAudioProcessor>().release();
}
