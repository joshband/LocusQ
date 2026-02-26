#include "PluginProcessor.h"
#include "processor_core/ProcessorParameterReaders.h"
#include "processor_bridge/ProcessorBridgeUtilities.h"
#include "shared_contracts/BridgeStatusContract.h"
#include "shared_contracts/HeadphoneCalibrationContract.h"

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

constexpr std::array<const char*, 4> kCalibrationDeviceProfileIds
{
    "generic",
    "airpods_pro_2",
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
        // intentional and acceptable in this lock-free multi-reader design.
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
    if (mode != LocusQMode::Emitter && emitterSlotId >= 0)
    {
        sceneGraph.unregisterEmitter (emitterSlotId);
        emitterSlotId = -1;
        lastPhysThrowGate = false;
        lastPhysResetGate = false;
    }

    if (mode != LocusQMode::Renderer && rendererRegistered)
    {
        sceneGraph.unregisterRenderer();
        sceneGraph.setPhysicsInteractionEnabled (false);
        rendererRegistered = false;
    }

    if (mode == LocusQMode::Emitter && emitterSlotId < 0)
    {
        emitterSlotId = sceneGraph.registerEmitter();
        DBG ("LocusQ: Registered emitter, slot " + juce::String (emitterSlotId));

        if (emitterSlotId >= 0)
        {
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
        }

        juce::String restoredLabel;
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            restoredLabel = emitterLabelState;
        }
        applyEmitterLabelToSceneSlotIfAvailable (restoredLabel);
    }
    else if (mode == LocusQMode::Renderer && ! rendererRegistered)
    {
        rendererRegistered = sceneGraph.registerRenderer();
        DBG ("LocusQ: Registered renderer: " + juce::String (rendererRegistered ? "OK" : "FAILED (already exists)"));
    }
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
            if (emitterSlotId >= 0)
            {
                // Publish audio buffer pointer for renderer to consume
                sceneGraph.getSlot (emitterSlotId).setAudioBuffer (
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
                SpatialRenderer::PoseSnapshot rendererPose {};
                rendererPose.qx = headTrackingPose->qx;
                rendererPose.qy = headTrackingPose->qy;
                rendererPose.qz = headTrackingPose->qz;
                rendererPose.qw = headTrackingPose->qw;
                rendererPose.timestampMs = headTrackingPose->timestampMs;
                rendererPose.seq = headTrackingPose->seq;
                rendererPose.pad = 0;
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
            break;
        }
    }

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
    if (emitterSlotId < 0)
        return;

    const auto existingData = sceneGraph.getSlot (emitterSlotId).read();

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
        interactionForce = computeEmitterInteractionForce (sceneGraph, emitterSlotId, interactionPosition);
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

    sceneGraph.getSlot (emitterSlotId).write (data);
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

//==============================================================================
juce::String LocusQAudioProcessor::getSceneStateJSON()
{
    const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting());
    applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, false);

    const auto snapshotSeq = ++sceneSnapshotSequence;
    const auto snapshotPublishedAtUtcMs = juce::Time::getCurrentTime().toMilliseconds();

    // Build JSON scene snapshot for WebView
    juce::String json = "{\"snapshotSchema\":\"" + juce::String (kSceneSnapshotSchemaProperty) + "\""
                      + ",\"snapshotSeq\":" + juce::String (static_cast<juce::int64> (snapshotSeq))
                      + ",\"profileSyncSeq\":" + juce::String (static_cast<juce::int64> (snapshotSeq))
                      + ",\"snapshotPublishedAtUtcMs\":" + juce::String (snapshotPublishedAtUtcMs)
                      + ",\"snapshotCadenceHz\":" + juce::String (kSceneSnapshotCadenceHz)
                      + ",\"snapshotStaleAfterMs\":" + juce::String (kSceneSnapshotStaleAfterMs)
                      + ",\"emitters\":[";
    bool first = true;
    double timelineTime = 0.0;
    double timelineDuration = 0.0;
    bool timelineLooping = false;
    const auto outputSet = getBusesLayout().getMainOutputChannelSet();
    const auto outputChannels = getMainBusNumOutputChannels();
    const auto outputLayout = outputLayoutToString (outputSet);
    const juce::String internalSpeakerLabelsJson { "[\"FL\",\"FR\",\"RR\",\"RL\"]" };
    const juce::String quadOutputMapJson { "[0,1,3,2]" };
    juce::String outputChannelLabelsJson { "[\"M\"]" };
    juce::String rendererOutputMode { "mono_sum" };
    const auto rendererHeadphoneModeRequestedIndex = juce::jlimit (
        0,
        1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_headphone_mode")->load())));
    const auto rendererHeadphoneProfileRequestedIndex = juce::jlimit (
        0,
        3,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_headphone_profile")->load())));
    const bool rendererAuditionEnabled = apvts.getRawParameterValue ("rend_audition_enable")->load() > 0.5f;
    const int rendererAuditionSignalIndex = juce::jlimit (
        0,
        static_cast<int> (kRendererAuditionSignalIds.size()) - 1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_audition_signal")->load())));
    const int rendererAuditionMotionIndex = juce::jlimit (
        0,
        static_cast<int> (kRendererAuditionMotionIds.size()) - 1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_audition_motion")->load())));
    const int rendererAuditionLevelIndex = juce::jlimit (
        0,
        static_cast<int> (kRendererAuditionLevelDbValues.size()) - 1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_audition_level")->load())));
    const juce::String rendererAuditionSignal { rendererAuditionSignalIdForIndex (rendererAuditionSignalIndex) };
    const juce::String rendererAuditionMotion { rendererAuditionMotionIdForIndex (rendererAuditionMotionIndex) };
    const float rendererAuditionLevelDb = rendererAuditionLevelDbForIndex (rendererAuditionLevelIndex);
    const bool rendererAuditionVisualReportedActive = spatialRenderer.isAuditionVisualActive();
    const float rendererAuditionVisualXRaw = spatialRenderer.getAuditionVisualX();
    const float rendererAuditionVisualYRaw = spatialRenderer.getAuditionVisualY();
    const float rendererAuditionVisualZRaw = spatialRenderer.getAuditionVisualZ();
    const bool rendererAuditionVisualFinite = isFiniteVector3 (
        rendererAuditionVisualXRaw,
        rendererAuditionVisualYRaw,
        rendererAuditionVisualZRaw);
    const bool rendererAuditionVisualInvalid = rendererAuditionVisualReportedActive && ! rendererAuditionVisualFinite;
    const bool rendererAuditionVisualActive = rendererAuditionVisualReportedActive && rendererAuditionVisualFinite;
    const float rendererAuditionVisualX = rendererAuditionVisualFinite ? rendererAuditionVisualXRaw : 0.0f;
    const float rendererAuditionVisualY = rendererAuditionVisualFinite ? rendererAuditionVisualYRaw : 1.2f;
    const float rendererAuditionVisualZ = rendererAuditionVisualFinite ? rendererAuditionVisualZRaw : -1.0f;
    const float rendererAuditionLevelNorm = static_cast<float> (rendererAuditionLevelIndex)
        / static_cast<float> (juce::jmax (1, static_cast<int> (kRendererAuditionLevelDbValues.size()) - 1));
    bool rendererAuditionCloudEnabled = rendererAuditionEnabled && rendererAuditionVisualActive;
    juce::String rendererAuditionCloudPattern { "tone_core" };
    int rendererAuditionCloudPointCountBase = 24;
    float rendererAuditionCloudSpreadBaseMeters = 0.45f;
    float rendererAuditionCloudPulseBaseHz = 1.9f;
    float rendererAuditionCloudCoherenceBase = 0.92f;
    juce::String rendererAuditionCloudMode { "single_core" };
    int rendererAuditionCloudEmitterCountBase = 1;
    float rendererAuditionCloudVerticalSpreadScale = 0.35f;
    switch (rendererAuditionSignalIndex)
    {
        case 0: // sine_440
            rendererAuditionCloudPattern = "tone_core";
            rendererAuditionCloudPointCountBase = 24;
            rendererAuditionCloudSpreadBaseMeters = 0.45f;
            rendererAuditionCloudPulseBaseHz = 1.9f;
            rendererAuditionCloudCoherenceBase = 0.92f;
            rendererAuditionCloudMode = "single_core";
            rendererAuditionCloudEmitterCountBase = 1;
            rendererAuditionCloudVerticalSpreadScale = 0.30f;
            break;
        case 1: // dual_tone
            rendererAuditionCloudPattern = "dual_orbit";
            rendererAuditionCloudPointCountBase = 28;
            rendererAuditionCloudSpreadBaseMeters = 0.55f;
            rendererAuditionCloudPulseBaseHz = 2.2f;
            rendererAuditionCloudCoherenceBase = 0.85f;
            rendererAuditionCloudMode = "dual_pair";
            rendererAuditionCloudEmitterCountBase = 2;
            rendererAuditionCloudVerticalSpreadScale = 0.34f;
            break;
        case 2: // pink_noise
            rendererAuditionCloudPattern = "noise_halo";
            rendererAuditionCloudPointCountBase = 42;
            rendererAuditionCloudSpreadBaseMeters = 0.70f;
            rendererAuditionCloudPulseBaseHz = 1.4f;
            rendererAuditionCloudCoherenceBase = 0.42f;
            rendererAuditionCloudMode = "noise_cluster";
            rendererAuditionCloudEmitterCountBase = 3;
            rendererAuditionCloudVerticalSpreadScale = 0.42f;
            break;
        case 3: // rain_field
            rendererAuditionCloudPattern = "rain_sheet";
            rendererAuditionCloudPointCountBase = 112;
            rendererAuditionCloudSpreadBaseMeters = 2.85f;
            rendererAuditionCloudPulseBaseHz = 3.2f;
            rendererAuditionCloudCoherenceBase = 0.24f;
            rendererAuditionCloudMode = "precipitation_rain";
            rendererAuditionCloudEmitterCountBase = 6;
            rendererAuditionCloudVerticalSpreadScale = 1.35f;
            break;
        case 4: // snow_drift
            rendererAuditionCloudPattern = "snow_cloud";
            rendererAuditionCloudPointCountBase = 104;
            rendererAuditionCloudSpreadBaseMeters = 3.10f;
            rendererAuditionCloudPulseBaseHz = 0.9f;
            rendererAuditionCloudCoherenceBase = 0.18f;
            rendererAuditionCloudMode = "precipitation_snow";
            rendererAuditionCloudEmitterCountBase = 7;
            rendererAuditionCloudVerticalSpreadScale = 1.05f;
            break;
        case 5: // bouncing_balls
            rendererAuditionCloudPattern = "bounce_cluster";
            rendererAuditionCloudPointCountBase = 74;
            rendererAuditionCloudSpreadBaseMeters = 2.45f;
            rendererAuditionCloudPulseBaseHz = 2.7f;
            rendererAuditionCloudCoherenceBase = 0.58f;
            rendererAuditionCloudMode = "impact_swarm";
            rendererAuditionCloudEmitterCountBase = 4;
            rendererAuditionCloudVerticalSpreadScale = 0.82f;
            break;
        case 6: // wind_chimes
            rendererAuditionCloudPattern = "chime_constellation";
            rendererAuditionCloudPointCountBase = 40;
            rendererAuditionCloudSpreadBaseMeters = 1.45f;
            rendererAuditionCloudPulseBaseHz = 1.2f;
            rendererAuditionCloudCoherenceBase = 0.58f;
            rendererAuditionCloudMode = "chime_cluster";
            rendererAuditionCloudEmitterCountBase = 5;
            rendererAuditionCloudVerticalSpreadScale = 0.72f;
            break;
        case 7: // crickets
            rendererAuditionCloudPattern = "cricket_field";
            rendererAuditionCloudPointCountBase = 76;
            rendererAuditionCloudSpreadBaseMeters = 2.60f;
            rendererAuditionCloudPulseBaseHz = 4.1f;
            rendererAuditionCloudCoherenceBase = 0.28f;
            rendererAuditionCloudMode = "bio_swarm";
            rendererAuditionCloudEmitterCountBase = 6;
            rendererAuditionCloudVerticalSpreadScale = 0.54f;
            break;
        case 8: // song_birds
            rendererAuditionCloudPattern = "songbird_canopy";
            rendererAuditionCloudPointCountBase = 68;
            rendererAuditionCloudSpreadBaseMeters = 2.95f;
            rendererAuditionCloudPulseBaseHz = 1.5f;
            rendererAuditionCloudCoherenceBase = 0.40f;
            rendererAuditionCloudMode = "bio_flock";
            rendererAuditionCloudEmitterCountBase = 6;
            rendererAuditionCloudVerticalSpreadScale = 1.10f;
            break;
        case 9: // karplus_plucks
            rendererAuditionCloudPattern = "pluck_strings";
            rendererAuditionCloudPointCountBase = 52;
            rendererAuditionCloudSpreadBaseMeters = 2.05f;
            rendererAuditionCloudPulseBaseHz = 1.9f;
            rendererAuditionCloudCoherenceBase = 0.52f;
            rendererAuditionCloudMode = "physical_modal";
            rendererAuditionCloudEmitterCountBase = 4;
            rendererAuditionCloudVerticalSpreadScale = 0.70f;
            break;
        case 10: // membrane_drops
            rendererAuditionCloudPattern = "membrane_impacts";
            rendererAuditionCloudPointCountBase = 60;
            rendererAuditionCloudSpreadBaseMeters = 2.30f;
            rendererAuditionCloudPulseBaseHz = 2.2f;
            rendererAuditionCloudCoherenceBase = 0.48f;
            rendererAuditionCloudMode = "physical_impacts";
            rendererAuditionCloudEmitterCountBase = 5;
            rendererAuditionCloudVerticalSpreadScale = 0.78f;
            break;
        case 11: // krell_patch
            rendererAuditionCloudPattern = "krell_glide";
            rendererAuditionCloudPointCountBase = 58;
            rendererAuditionCloudSpreadBaseMeters = 2.40f;
            rendererAuditionCloudPulseBaseHz = 1.3f;
            rendererAuditionCloudCoherenceBase = 0.44f;
            rendererAuditionCloudMode = "synth_generative";
            rendererAuditionCloudEmitterCountBase = 4;
            rendererAuditionCloudVerticalSpreadScale = 0.96f;
            break;
        case 12: // generative_arp
            rendererAuditionCloudPattern = "arp_lattice";
            rendererAuditionCloudPointCountBase = 62;
            rendererAuditionCloudSpreadBaseMeters = 2.55f;
            rendererAuditionCloudPulseBaseHz = 2.4f;
            rendererAuditionCloudCoherenceBase = 0.50f;
            rendererAuditionCloudMode = "synth_grid";
            rendererAuditionCloudEmitterCountBase = 5;
            rendererAuditionCloudVerticalSpreadScale = 0.88f;
            break;
        default:
            break;
    }
    float rendererAuditionCloudMotionSpreadScale = 0.85f;
    float rendererAuditionCloudMotionPulseScale = 1.0f;
    float rendererAuditionCloudMotionCoherenceScale = 1.0f;
    switch (rendererAuditionMotionIndex)
    {
        case 0: // center
            rendererAuditionCloudMotionSpreadScale = 0.85f;
            rendererAuditionCloudMotionPulseScale = 1.0f;
            rendererAuditionCloudMotionCoherenceScale = 1.0f;
            break;
        case 1: // orbit_slow
            rendererAuditionCloudMotionSpreadScale = 1.10f;
            rendererAuditionCloudMotionPulseScale = 1.15f;
            rendererAuditionCloudMotionCoherenceScale = 0.88f;
            break;
        case 2: // orbit_fast
            rendererAuditionCloudMotionSpreadScale = 1.35f;
            rendererAuditionCloudMotionPulseScale = 1.35f;
            rendererAuditionCloudMotionCoherenceScale = 0.72f;
            break;
        case 3: // figure8_flow
            rendererAuditionCloudMotionSpreadScale = 1.52f;
            rendererAuditionCloudMotionPulseScale = 1.22f;
            rendererAuditionCloudMotionCoherenceScale = 0.76f;
            break;
        case 4: // helix_rise
            rendererAuditionCloudMotionSpreadScale = 1.70f;
            rendererAuditionCloudMotionPulseScale = 1.42f;
            rendererAuditionCloudMotionCoherenceScale = 0.68f;
            break;
        case 5: // wall_ricochet
            rendererAuditionCloudMotionSpreadScale = 1.88f;
            rendererAuditionCloudMotionPulseScale = 1.56f;
            rendererAuditionCloudMotionCoherenceScale = 0.62f;
            break;
        default:
            break;
    }
    bool rendererAuditionCloudBoundsAdjusted = false;
    const int rendererAuditionCloudPointCountRaw = static_cast<int> (std::lround (
        rendererAuditionCloudPointCountBase * (1.0f + 0.25f * rendererAuditionLevelNorm)));
    int rendererAuditionCloudPointCount = rendererAuditionCloudEnabled
        ? sanitizeBoundedInt (
            rendererAuditionCloudPointCountRaw,
            8,
            kRendererAuditionCloudMaxPoints,
            &rendererAuditionCloudBoundsAdjusted)
        : 0;
    float rendererAuditionCloudSpreadMeters = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.20f,
            6.0f,
            rendererAuditionCloudSpreadBaseMeters * rendererAuditionCloudMotionSpreadScale
                * (0.90f + 0.20f * rendererAuditionLevelNorm))
        : 0.0f;
    float rendererAuditionCloudPulseHz = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.2f,
            6.0f,
            rendererAuditionCloudPulseBaseHz * rendererAuditionCloudMotionPulseScale
                * (0.95f + 0.15f * rendererAuditionLevelNorm))
        : 0.0f;
    float rendererAuditionCloudCoherence = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.05f,
            0.99f,
            rendererAuditionCloudCoherenceBase * rendererAuditionCloudMotionCoherenceScale
                + (rendererAuditionVisualActive ? 0.04f : -0.06f))
        : 0.0f;
    const bool rendererAuditionCloudGeometryInvalid = rendererAuditionCloudEnabled
        && (! std::isfinite (rendererAuditionCloudSpreadMeters)
            || ! std::isfinite (rendererAuditionCloudPulseHz)
            || ! std::isfinite (rendererAuditionCloudCoherence));
    if (rendererAuditionCloudGeometryInvalid)
    {
        rendererAuditionCloudSpreadMeters = 0.0f;
        rendererAuditionCloudPulseHz = 0.0f;
        rendererAuditionCloudCoherence = 0.0f;
        rendererAuditionCloudPointCount = 0;
    }
    juce::uint32 rendererAuditionCloudSeed = 0xA39F1C2Du;
    const auto rendererAuditionSignalOrdinal = static_cast<juce::uint32> (juce::jmax (0, rendererAuditionSignalIndex) + 1);
    const auto rendererAuditionMotionOrdinal = static_cast<juce::uint32> (juce::jmax (0, rendererAuditionMotionIndex) + 1);
    const auto rendererAuditionLevelOrdinal = static_cast<juce::uint32> (juce::jmax (0, rendererAuditionLevelIndex) + 1);
    rendererAuditionCloudSeed ^= rendererAuditionSignalOrdinal * 0x9E3779B9u;
    rendererAuditionCloudSeed = (rendererAuditionCloudSeed << 13u) | (rendererAuditionCloudSeed >> 19u);
    rendererAuditionCloudSeed ^= rendererAuditionMotionOrdinal * 0x85EBCA6Bu;
    rendererAuditionCloudSeed ^= rendererAuditionLevelOrdinal * 0xC2B2AE35u;
    if (rendererAuditionVisualActive)
        rendererAuditionCloudSeed ^= 0x1B873593u;
    int rendererAuditionCloudEmitterCount = 0;
    if (rendererAuditionCloudEnabled)
    {
        const int motionEmitterBoost = rendererAuditionMotionIndex == 2 ? 2 : (rendererAuditionMotionIndex == 1 ? 1 : 0);
        const int levelEmitterBoost = rendererAuditionLevelIndex >= 3 ? 1 : 0;
        const int visualEmitterBoost = rendererAuditionVisualActive ? 1 : 0;
        const auto emitterCountRaw =
            rendererAuditionCloudEmitterCountBase + motionEmitterBoost + levelEmitterBoost + visualEmitterBoost;
        rendererAuditionCloudEmitterCount = sanitizeBoundedInt (
            emitterCountRaw,
            1,
            kRendererAuditionCloudMaxEmitters,
            &rendererAuditionCloudBoundsAdjusted);
    }

    // BL-029 Slice B1: renderer-authoritative audition binding resolver.
    const auto currentMode = getCurrentMode();
    const juce::String rendererAuditionSourceMode {
        rendererAuditionCloudEnabled ? "cloud" : "single"
    };
    juce::String rendererAuditionRequestedMode { rendererAuditionSourceMode };
    juce::String rendererAuditionResolvedMode { rendererAuditionSourceMode };
    juce::String rendererAuditionBindingTarget { "none" };
    bool rendererAuditionBindingAvailable = false;
    int preferredEmitterBindingId = -1;
    int preferredPhysicsBindingId = -1;

    if (emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId))
    {
        const auto emitterData = sceneGraph.getSlot (emitterSlotId).read();
        if (emitterData.active)
        {
            preferredEmitterBindingId = emitterSlotId;
            if (emitterData.physicsEnabled)
                preferredPhysicsBindingId = emitterSlotId;
        }
    }

    for (int slot = 0; slot < SceneGraph::MAX_EMITTERS; ++slot)
    {
        if (! sceneGraph.isSlotActive (slot))
            continue;

        const auto slotData = sceneGraph.getSlot (slot).read();
        if (! slotData.active)
            continue;

        if (preferredEmitterBindingId < 0)
            preferredEmitterBindingId = slot;

        if (slotData.physicsEnabled && preferredPhysicsBindingId < 0)
            preferredPhysicsBindingId = slot;
    }

    const bool choreographyBindingRequested = apvts.getRawParameterValue ("anim_enable")->load() > 0.5f
        && static_cast<int> (std::lround (apvts.getRawParameterValue ("anim_mode")->load())) == 1;
    const bool physicsBindingRequested = apvts.getRawParameterValue ("rend_phys_interact")->load() > 0.5f;
    const bool emitterBindingRequested = sceneGraph.getActiveEmitterCount() > 0;

    if (rendererAuditionEnabled)
    {
        if (physicsBindingRequested)
            rendererAuditionRequestedMode = "bound_physics";
        else if (choreographyBindingRequested)
            rendererAuditionRequestedMode = "bound_choreography";
        else if (emitterBindingRequested)
            rendererAuditionRequestedMode = "bound_emitter";
    }

    float rendererAuditionDensity = rendererAuditionCloudEnabled
        ? juce::jlimit (0.0f, 1.0f, static_cast<float> (rendererAuditionCloudPointCount) / 160.0f)
        : 0.0f;
    float rendererAuditionReactivity = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.0f,
            1.0f,
            (0.58f * rendererAuditionLevelNorm) + (0.42f * (1.0f - rendererAuditionCloudCoherence)))
        : 0.0f;
    const bool rendererAuditionTransportSync = false;
    juce::String rendererAuditionFallbackReason { "none" };
    if (! rendererAuditionEnabled)
    {
        rendererAuditionFallbackReason = "audition_disabled";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (currentMode != LocusQMode::Renderer)
    {
        rendererAuditionFallbackReason = "renderer_mode_inactive";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (rendererAuditionVisualInvalid)
    {
        rendererAuditionFallbackReason = "visual_centroid_invalid";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (rendererAuditionRequestedMode == "bound_emitter")
    {
        if (preferredEmitterBindingId >= 0)
        {
            rendererAuditionResolvedMode = "bound_emitter";
            rendererAuditionBindingTarget = "emitter:" + juce::String (preferredEmitterBindingId);
            rendererAuditionBindingAvailable = true;
        }
        else
        {
            rendererAuditionFallbackReason = "bound_emitter_unavailable";
            rendererAuditionResolvedMode = rendererAuditionSourceMode;
        }
    }
    else if (rendererAuditionRequestedMode == "bound_choreography")
    {
        if (choreographyBindingRequested)
        {
            rendererAuditionResolvedMode = "bound_choreography";
            rendererAuditionBindingTarget = "timeline:global";
            rendererAuditionBindingAvailable = true;
        }
        else
        {
            rendererAuditionFallbackReason = "bound_choreography_unavailable";
            rendererAuditionResolvedMode = rendererAuditionSourceMode;
        }
    }
    else if (rendererAuditionRequestedMode == "bound_physics")
    {
        if (preferredPhysicsBindingId >= 0)
        {
            rendererAuditionResolvedMode = "bound_physics";
            rendererAuditionBindingTarget = "emitter:" + juce::String (preferredPhysicsBindingId);
            rendererAuditionBindingAvailable = true;
        }
        else
        {
            rendererAuditionFallbackReason = "bound_physics_unavailable";
            rendererAuditionResolvedMode = rendererAuditionSourceMode;
        }
    }
    else if (! rendererAuditionVisualActive)
    {
        rendererAuditionFallbackReason = "visual_centroid_unavailable";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (rendererAuditionSourceMode == "cloud" && rendererAuditionCloudGeometryInvalid)
    {
        rendererAuditionFallbackReason = "cloud_geometry_invalid";
        rendererAuditionResolvedMode = "single";
        rendererAuditionCloudEnabled = false;
        rendererAuditionCloudEmitterCount = 0;
        rendererAuditionCloudPointCount = 0;
    }
    else if (rendererAuditionSourceMode == "cloud" && rendererAuditionCloudEmitterCount <= 0)
    {
        rendererAuditionFallbackReason = "cloud_emitters_unavailable";
        rendererAuditionResolvedMode = "single";
        rendererAuditionCloudEnabled = false;
        rendererAuditionCloudPointCount = 0;
    }
    else
    {
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }

    rendererAuditionDensity = rendererAuditionCloudEnabled
        ? juce::jlimit (0.0f, 1.0f, static_cast<float> (rendererAuditionCloudPointCount) / 160.0f)
        : 0.0f;
    rendererAuditionReactivity = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.0f,
            1.0f,
            (0.58f * rendererAuditionLevelNorm) + (0.42f * (1.0f - rendererAuditionCloudCoherence)))
        : 0.0f;
    if (rendererAuditionFallbackReason == "none"
        && rendererAuditionSourceMode == "cloud"
        && rendererAuditionCloudBoundsAdjusted)
    {
        rendererAuditionFallbackReason = "cloud_bounds_clamped";
    }

    auto auditionCloudHashUnit = [rendererAuditionCloudSeed] (int emitterIndex, juce::uint32 salt) -> float
    {
        juce::uint32 hash = rendererAuditionCloudSeed;
        const auto emitterOrdinal = static_cast<juce::uint32> (juce::jmax (0, emitterIndex) + 1);
        hash ^= emitterOrdinal * 0x9E3779B9u;
        hash ^= salt;
        hash ^= (hash >> 16u);
        hash *= 0x7FEB352Du;
        hash ^= (hash >> 15u);
        hash *= 0x846CA68Bu;
        hash ^= (hash >> 16u);
        return static_cast<float> (hash & 0x00FFFFFFu) / static_cast<float> (0x00FFFFFFu);
    };

    juce::String rendererAuditionCloudEmittersJson { "[" };
    if (rendererAuditionCloudEnabled && rendererAuditionCloudEmitterCount > 0)
    {
        constexpr float kTwoPi = 6.28318530717958647692f;
        const float baseAngleStep = kTwoPi / static_cast<float> (rendererAuditionCloudEmitterCount);
        const float motionPhaseBias = static_cast<float> (rendererAuditionMotionIndex) * 0.31f
                                      + rendererAuditionLevelNorm * 0.27f;
        const float spreadScaleFromMotion = juce::jlimit (
            0.5f,
            1.6f,
            0.65f + 0.35f * rendererAuditionCloudMotionSpreadScale);

        for (int emitterIndex = 0; emitterIndex < rendererAuditionCloudEmitterCount; ++emitterIndex)
        {
            const float unitRadius = auditionCloudHashUnit (emitterIndex, 0xA53C9E11u);
            const float unitAngleJitter = auditionCloudHashUnit (emitterIndex, 0x3C6EF372u);
            const float unitHeight = auditionCloudHashUnit (emitterIndex, 0xBB67AE85u);
            const float unitWeight = auditionCloudHashUnit (emitterIndex, 0xC2B2AE35u);
            const float unitActivity = auditionCloudHashUnit (emitterIndex, 0x27D4EB2Fu);

            const float angle = baseAngleStep * static_cast<float> (emitterIndex)
                                + motionPhaseBias
                                + (unitAngleJitter - 0.5f) * 0.75f;
            const float radialSpread = rendererAuditionCloudSpreadMeters * spreadScaleFromMotion
                                       * (0.25f + 0.75f * unitRadius);
            const float localOffsetX = std::cos (angle) * radialSpread;
            const float localOffsetZ = std::sin (angle) * radialSpread;
            const float localOffsetY = (unitHeight - 0.5f) * 2.0f
                                       * rendererAuditionCloudSpreadMeters
                                       * rendererAuditionCloudVerticalSpreadScale;
            const float weight = juce::jlimit (
                0.05f,
                1.0f,
                (1.0f / static_cast<float> (rendererAuditionCloudEmitterCount)) * (0.82f + 0.36f * unitWeight));
            const float phase = std::fmod (
                static_cast<float> (emitterIndex) / static_cast<float> (juce::jmax (1, rendererAuditionCloudEmitterCount))
                    + motionPhaseBias
                    + unitAngleJitter * 0.33f,
                1.0f);
            const float activity = juce::jlimit (
                0.0f,
                1.0f,
                0.32f
                    + 0.58f * rendererAuditionLevelNorm
                    + 0.22f * (1.0f - rendererAuditionCloudCoherence)
                    + 0.12f * unitActivity);

            if (emitterIndex > 0)
                rendererAuditionCloudEmittersJson << ",";

            rendererAuditionCloudEmittersJson << "{\"id\":" << juce::String (emitterIndex)
                                              << ",\"weight\":" << juce::String (weight, 4)
                                              << ",\"localOffsetX\":" << juce::String (localOffsetX, 3)
                                              << ",\"localOffsetY\":" << juce::String (localOffsetY, 3)
                                              << ",\"localOffsetZ\":" << juce::String (localOffsetZ, 3)
                                              << ",\"phase\":" << juce::String (phase, 4)
                                              << ",\"activity\":" << juce::String (activity, 4) << "}";
        }
    }
    rendererAuditionCloudEmittersJson << "]";
    const bool rendererAuditionReactiveActive =
        rendererAuditionEnabled
        && currentMode == LocusQMode::Renderer
        && rendererAuditionVisualActive;
    auto rendererAuditionReactive = makeNeutralAuditionReactiveSnapshot();
    bool rendererAuditionReactiveInvalid = false;
    bool rendererAuditionReactiveMissing = false;
    if (rendererAuditionReactiveActive)
    {
        const auto sanitizedReactivePayload = sanitizeAuditionReactivePayload (
            spatialRenderer.getAuditionReactiveSnapshot());
        rendererAuditionReactive = sanitizedReactivePayload.snapshot;
        rendererAuditionReactiveInvalid = sanitizedReactivePayload.invalidScalars
            || sanitizedReactivePayload.invalidBounds;
        rendererAuditionReactiveMissing = rendererAuditionCloudEnabled
            && rendererAuditionResolvedMode == "cloud"
            && rendererAuditionReactive.sourceEnergyCount <= 0;

        if (rendererAuditionReactiveInvalid || rendererAuditionReactiveMissing)
            rendererAuditionReactive = makeNeutralAuditionReactiveSnapshot();
    }

    if (rendererAuditionFallbackReason == "none" && rendererAuditionReactiveInvalid)
        rendererAuditionFallbackReason = "reactive_payload_invalid";
    else if (rendererAuditionFallbackReason == "none" && rendererAuditionReactiveMissing)
        rendererAuditionFallbackReason = "reactive_payload_missing";
    const bool rendererAuditionReactivePublishedActive = rendererAuditionReactiveActive
        && ! rendererAuditionReactiveInvalid
        && ! rendererAuditionReactiveMissing;

    const juce::String rendererAuditionReactiveHeadphoneFallbackReason {
        SpatialRenderer::auditionReactiveHeadphoneFallbackReasonToString (
            rendererAuditionReactive.headphoneFallbackReasonIndex)
    };
    const bool rendererAuditionReactiveHeadphoneFallback =
        rendererAuditionReactive.headphoneFallbackReasonIndex
            != static_cast<int> (SpatialRenderer::AuditionReactiveHeadphoneFallbackReason::None);
    bool rendererAuditionReactiveSourceBoundsAdjusted = false;
    const auto rendererAuditionSourceEnergyCount = sanitizeBoundedInt (
        rendererAuditionReactive.sourceEnergyCount,
        0,
        SpatialRenderer::MAX_AUDITION_REACTIVE_SOURCES,
        &rendererAuditionReactiveSourceBoundsAdjusted);
    if (rendererAuditionFallbackReason == "none" && rendererAuditionReactiveSourceBoundsAdjusted)
        rendererAuditionFallbackReason = "reactive_source_count_invalid";

    juce::String rendererAuditionSourceEnergyJson { "[" };
    juce::String rendererAuditionSourceEnergyNormJson { "[" };
    for (int sourceIndex = 0; sourceIndex < rendererAuditionSourceEnergyCount; ++sourceIndex)
    {
        if (sourceIndex > 0)
        {
            rendererAuditionSourceEnergyJson << ",";
            rendererAuditionSourceEnergyNormJson << ",";
        }
        const auto sourceEnergy = sanitizeUnitScalar (
            rendererAuditionReactive.sourceEnergy[static_cast<size_t> (sourceIndex)],
            0.0f);
        rendererAuditionSourceEnergyJson << juce::String (
            sourceEnergy,
            5);
        rendererAuditionSourceEnergyNormJson << juce::String (sourceEnergy, 5);
    }
    rendererAuditionSourceEnergyJson << "]";
    rendererAuditionSourceEnergyNormJson << "]";

    const auto escapeJsonString = [] (juce::String text)
    {
        return text.replace ("\\", "\\\\").replace ("\"", "\\\"");
    };
    const bool rendererSteamAudioCompiled = spatialRenderer.isSteamAudioCompiled();
    const bool rendererSteamAudioAvailable = spatialRenderer.isSteamAudioAvailable();
    const int rendererSteamAudioInitStageIndex = spatialRenderer.getSteamAudioInitStageIndex();
    const juce::String rendererSteamAudioInitStage {
        SpatialRenderer::steamAudioInitStageToString (rendererSteamAudioInitStageIndex)
    };
    const int rendererSteamAudioInitErrorCode = spatialRenderer.getSteamAudioInitErrorCode();
    const juce::String rendererSteamAudioRuntimeLib {
        escapeJsonString (spatialRenderer.getSteamAudioRuntimeLibraryPath())
    };
    const juce::String rendererSteamAudioMissingSymbol {
        escapeJsonString (spatialRenderer.getSteamAudioMissingSymbolName())
    };
    const int rendererSpatialProfileRequestedIndex = juce::jlimit (
        0,
        11,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_spatial_profile")->load())));
    const int rendererSpatialProfileActiveIndex = spatialRenderer.getSpatialOutputProfileActiveIndex();
    const int rendererSpatialProfileStageIndex = spatialRenderer.getSpatialProfileStageIndex();
    const juce::String rendererSpatialProfileRequested {
        SpatialRenderer::spatialOutputProfileToString (rendererSpatialProfileRequestedIndex)
    };
    const juce::String rendererSpatialProfileActive {
        SpatialRenderer::spatialOutputProfileToString (rendererSpatialProfileActiveIndex)
    };
    const juce::String rendererSpatialProfileStage {
        SpatialRenderer::spatialProfileStageToString (rendererSpatialProfileStageIndex)
    };
    const bool rendererAmbiCompiled = (rendererSpatialProfileRequestedIndex
                                       == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicFOA))
                                      || (rendererSpatialProfileRequestedIndex
                                          == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicHOA));
    const bool rendererAmbiActive = (rendererSpatialProfileActiveIndex
                                     == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicFOA))
                                    || (rendererSpatialProfileActiveIndex
                                        == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicHOA));
    const int rendererAmbiMaxOrder = rendererSpatialProfileActiveIndex
                                     == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicHOA)
                                         ? 3
                                         : 1;
    const juce::String rendererAmbiNormalization { "sn3d" };
    const juce::String rendererAmbiChannelOrder { "acn" };
    const juce::String rendererAmbiDecodeLayout { rendererSpatialProfileActive };
    const juce::String rendererAmbiStage { rendererSpatialProfileStage };
    const auto clapDiagnostics = getClapRuntimeDiagnostics();
    const juce::String clapWrapperType {
        escapeJsonString (clapDiagnostics.wrapperType)
    };
    const juce::String clapLifecycleStage {
        escapeJsonString (clapDiagnostics.lifecycleStage)
    };
    const juce::String clapRuntimeMode {
        escapeJsonString (clapDiagnostics.runtimeMode)
    };
    auto rendererHeadphoneModeActiveIndex = spatialRenderer.getHeadphoneRenderModeActiveIndex();
    if (outputChannels >= 2)
    {
        rendererHeadphoneModeActiveIndex =
            (rendererHeadphoneModeRequestedIndex == static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
             && rendererSteamAudioAvailable)
                ? static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
                : static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    else
    {
        rendererHeadphoneModeActiveIndex = static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    const juce::String rendererHeadphoneModeRequested {
        SpatialRenderer::headphoneRenderModeToString (rendererHeadphoneModeRequestedIndex)
    };
    const juce::String rendererHeadphoneModeActive {
        SpatialRenderer::headphoneRenderModeToString (rendererHeadphoneModeActiveIndex)
    };
    const auto rendererHeadphoneProfileActiveIndex = spatialRenderer.getHeadphoneDeviceProfileActiveIndex();
    const juce::String rendererHeadphoneProfileRequested {
        SpatialRenderer::headphoneDeviceProfileToString (rendererHeadphoneProfileRequestedIndex)
    };
    const juce::String rendererHeadphoneProfileActive {
        SpatialRenderer::headphoneDeviceProfileToString (rendererHeadphoneProfileActiveIndex)
    };
    const bool rendererHeadPoseAvailable = headTrackingBridge.currentPose() != nullptr;
    const auto rendererMatrix = buildRendererMatrixSnapshot (
        rendererSpatialProfileRequestedIndex,
        rendererSpatialProfileActiveIndex,
        rendererSpatialProfileStageIndex,
        rendererHeadphoneModeRequestedIndex,
        rendererHeadphoneModeActiveIndex,
        outputChannels,
        rendererHeadPoseAvailable);
    const auto rendererMatrixEventSeq = static_cast<juce::uint64> (snapshotSeq);
    const bool rendererPhysicsLensEnabled = apvts.getRawParameterValue ("rend_viz_physics_lens")->load() > 0.5f;
    const float rendererPhysicsLensMix = juce::jlimit (
        0.0f,
        1.0f,
        apvts.getRawParameterValue ("rend_viz_diag_mix")->load());

    if (outputChannels >= 13
        && (rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::Surround742)
            || rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AtmosBed)))
    {
        outputChannelLabelsJson = "[\"L\",\"R\",\"C\",\"LFE1\",\"LFE2\",\"Ls\",\"Rs\",\"Lrs\",\"Rrs\",\"TopFL\",\"TopFR\",\"TopRL\",\"TopRR\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= 10
             && rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::Surround721))
    {
        outputChannelLabelsJson = "[\"L\",\"R\",\"C\",\"LFE1\",\"LFE2\",\"Ls\",\"Rs\",\"Lrs\",\"Rrs\",\"TopC\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= 8
             && rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::Surround521))
    {
        outputChannelLabelsJson = "[\"L\",\"R\",\"C\",\"LFE1\",\"LFE2\",\"Ls\",\"Rs\",\"TopC\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= SpatialRenderer::NUM_SPEAKERS
             && rendererAmbiActive)
    {
        outputChannelLabelsJson = "[\"W\",\"X\",\"Y\",\"Z\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= SpatialRenderer::NUM_SPEAKERS)
    {
        outputChannelLabelsJson = "[\"FL\",\"FR\",\"RL\",\"RR\"]";
        rendererOutputMode = "quad_map_first4";
    }
    else if (outputChannels >= 2)
    {
        outputChannelLabelsJson = "[\"L\",\"R\"]";
        rendererOutputMode = rendererSpatialProfileActive;
        if (rendererOutputMode.isEmpty() || rendererOutputMode == "auto")
            rendererOutputMode = rendererHeadphoneModeActive;
    }

    const auto currentCalSpeakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto currentCalSpeakerRouting = getCurrentCalibrationSpeakerRouting();
    const auto currentCalTopologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto currentCalMonitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto currentCalDeviceProfile = getCurrentCalibrationDeviceProfileIndex();
    const auto currentCalTopologyId = calibrationTopologyIdForIndex (currentCalTopologyProfile);
    const auto currentCalMonitoringPathId = calibrationMonitoringPathIdForIndex (currentCalMonitoringPath);
    const auto currentCalDeviceProfileId = calibrationDeviceProfileIdForIndex (currentCalDeviceProfile);
    const auto currentCalRequiredChannels = getRequiredCalibrationChannelsForTopologyIndex (currentCalTopologyProfile);
    const auto currentCalWritableChannels = resolveCalibrationWritableChannels (
        outputChannels > 0 ? outputChannels : getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        currentCalSpeakerRouting);
    const bool currentCalMappingLimitedToFirst4 = currentCalRequiredChannels > currentCalWritableChannels;
    const auto toRoutingJson = [] (const std::array<int, SpatialRenderer::NUM_SPEAKERS>& routing)
    {
        juce::String jsonArray { "[" };
        for (size_t i = 0; i < routing.size(); ++i)
        {
            if (i > 0)
                jsonArray << ",";
            jsonArray << juce::String (juce::jlimit (1, 8, routing[i]));
        }
        jsonArray << "]";
        return jsonArray;
    };

    const auto currentCalSpeakerRoutingJson = toRoutingJson (currentCalSpeakerRouting);
    const auto autoDetectedRoutingJson = toRoutingJson (lastAutoDetectedSpeakerRouting);
    const auto autoDetectedTopologyId = calibrationTopologyIdForIndex (lastAutoDetectedTopologyProfile);
    const auto rendererHeadphoneCalibration = buildHeadphoneCalibrationDiagnosticsSnapshot (
        currentCalMonitoringPath,
        rendererHeadphoneModeRequestedIndex,
        rendererHeadphoneModeActiveIndex,
        outputChannels,
        rendererSteamAudioAvailable,
        rendererSteamAudioInitStage);

    {
        const juce::SpinLock::ScopedLockType publishedCalibrationLock (publishedHeadphoneCalibrationLock);
        publishedHeadphoneCalibrationDiagnostics.profileSyncSeq = snapshotSeq;
        publishedHeadphoneCalibrationDiagnostics.requested = rendererHeadphoneCalibration.requested;
        publishedHeadphoneCalibrationDiagnostics.active = rendererHeadphoneCalibration.active;
        publishedHeadphoneCalibrationDiagnostics.stage = rendererHeadphoneCalibration.stage;
        publishedHeadphoneCalibrationDiagnostics.fallbackReady = rendererHeadphoneCalibration.fallbackReady;
        publishedHeadphoneCalibrationDiagnostics.fallbackReason = rendererHeadphoneCalibration.fallbackReason;
        publishedHeadphoneCalibrationDiagnostics.valid = true;
    }

    Vec3 listenerPosition { 0.0f, 1.2f, 0.0f };
    Vec3 roomDimensions { 6.0f, 4.0f, 3.0f };
    std::array<Vec3, SpatialRenderer::NUM_SPEAKERS> speakerPositions = kViewportFallbackSpeakerPositions;
    std::array<float, SpatialRenderer::NUM_SPEAKERS> speakerGainTrims {};
    std::array<float, SpatialRenderer::NUM_SPEAKERS> speakerDelayCompMs {};
    bool roomProfileValid = false;

    if (auto roomProfile = sceneGraph.getRoomProfile(); roomProfile != nullptr && roomProfile->valid)
    {
        roomProfileValid = true;
        listenerPosition = roomProfile->listenerPos;
        roomDimensions = roomProfile->dimensions;

        for (size_t i = 0; i < speakerPositions.size(); ++i)
        {
            speakerPositions[i] = roomProfile->speakers[i].position;
            speakerGainTrims[i] = roomProfile->speakers[i].gainTrim;
            speakerDelayCompMs[i] = roomProfile->speakers[i].delayComp;
        }
    }

    {
        const juce::SpinLock::ScopedTryLockType timelineLock (keyframeTimelineLock);
        if (timelineLock.isLocked())
        {
            timelineTime = keyframeTimeline.getCurrentTimeSeconds();
            timelineDuration = keyframeTimeline.getDurationSeconds();
            timelineLooping = keyframeTimeline.isLooping();
        }
    }

    for (int i = 0; i < SceneGraph::MAX_EMITTERS; ++i)
    {
        if (! sceneGraph.isSlotActive (i)) continue;
        auto data = sceneGraph.getSlot (i).read();
        if (! data.active) continue;

        if (! first) json += ",";
        first = false;

        const auto* emitterAudio = sceneGraph.getSlot (i).getAudioMono();
        const auto emitterAudioSamples = sceneGraph.getSlot (i).getAudioNumSamples();
        const auto emitterRmsLinear = computeMonoRmsLinear (emitterAudio, emitterAudioSamples);
        const auto emitterRmsDb = juce::Decibels::gainToDecibels (juce::jmax (1.0e-6f, emitterRmsLinear), -120.0f);

        json += "{\"id\":" + juce::String (i)
              + ",\"x\":" + juce::String (data.position.x, 3)
              + ",\"y\":" + juce::String (data.position.y, 3)
              + ",\"z\":" + juce::String (data.position.z, 3)
              + ",\"sx\":" + juce::String (data.size.x, 2)
              + ",\"sy\":" + juce::String (data.size.y, 2)
              + ",\"sz\":" + juce::String (data.size.z, 2)
              + ",\"gain\":" + juce::String (data.gain, 1)
              + ",\"spread\":" + juce::String (data.spread, 2)
              + ",\"directivity\":" + juce::String (data.directivity, 2)
              + ",\"aimX\":" + juce::String (data.directivityAim.x, 3)
              + ",\"aimY\":" + juce::String (data.directivityAim.y, 3)
              + ",\"aimZ\":" + juce::String (data.directivityAim.z, 3)
              + ",\"color\":" + juce::String (data.colorIndex)
              + ",\"muted\":" + juce::String (data.muted ? "true" : "false")
              + ",\"soloed\":" + juce::String (data.soloed ? "true" : "false")
              + ",\"physics\":" + juce::String (data.physicsEnabled ? "true" : "false")
              + ",\"vx\":" + juce::String (data.velocity.x, 3)
              + ",\"vy\":" + juce::String (data.velocity.y, 3)
              + ",\"vz\":" + juce::String (data.velocity.z, 3)
              + ",\"fx\":" + juce::String (data.force.x, 3)
              + ",\"fy\":" + juce::String (data.force.y, 3)
              + ",\"fz\":" + juce::String (data.force.z, 3)
              + ",\"collisionMask\":" + juce::String (static_cast<int> (data.collisionMask))
              + ",\"collisionEnergy\":" + juce::String (data.collisionEnergy, 4)
              + ",\"rms\":" + juce::String (emitterRmsLinear, 5)
              + ",\"rmsDb\":" + juce::String (emitterRmsDb, 2)
              + ",\"label\":\"" + juce::String (data.label) + "\""
              + "}";
    }

    juce::String speakerRmsJson { "[" };
    juce::String speakersJson { "[" };
    for (size_t i = 0; i < sceneSpeakerRms.size(); ++i)
    {
        if (i > 0)
        {
            speakerRmsJson << ",";
            speakersJson << ",";
        }

        const auto speakerRms = juce::jlimit (0.0f, 4.0f, sceneSpeakerRms[i]);
        speakerRmsJson << juce::String (speakerRms, 5);

        speakersJson << "{\"id\":" << juce::String (static_cast<int> (i))
                     << ",\"label\":\"" << juce::String (kInternalSpeakerLabels[i]) << "\""
                     << ",\"x\":" << juce::String (speakerPositions[i].x, 3)
                     << ",\"y\":" << juce::String (speakerPositions[i].y, 3)
                     << ",\"z\":" << juce::String (speakerPositions[i].z, 3)
                     << ",\"gainTrimDb\":" << juce::String (speakerGainTrims[i], 3)
                     << ",\"delayCompMs\":" << juce::String (speakerDelayCompMs[i], 3)
                     << ",\"rms\":" << juce::String (speakerRms, 5)
                     << "}";
    }
    speakerRmsJson << "]";
    speakersJson << "]";

    json += "],\"emitterCount\":" + juce::String (sceneGraph.getActiveEmitterCount())
          + ",\"localEmitterId\":" + juce::String (emitterSlotId)
          + ",\"rendererActive\":" + juce::String (sceneGraph.isRendererRegistered() ? "true" : "false")
          + ",\"rendererEligibleEmitters\":" + juce::String (spatialRenderer.getLastEligibleEmitterCount())
          + ",\"rendererProcessedEmitters\":" + juce::String (spatialRenderer.getLastProcessedEmitterCount())
          + ",\"rendererCulledBudget\":" + juce::String (spatialRenderer.getLastBudgetCulledEmitterCount())
          + ",\"rendererCulledActivity\":" + juce::String (spatialRenderer.getLastActivityCulledEmitterCount())
          + ",\"rendererGuardrailActive\":" + juce::String (spatialRenderer.wasGuardrailActiveLastBlock() ? "true" : "false")
          + ",\"outputChannels\":" + juce::String (outputChannels)
          + ",\"outputLayout\":\"" + outputLayout + "\""
          + ",\"rendererOutputMode\":\"" + rendererOutputMode + "\""
          + ",\"rendererSpatialProfileRequested\":\"" + rendererSpatialProfileRequested + "\""
          + ",\"rendererSpatialProfileActive\":\"" + rendererSpatialProfileActive + "\""
          + ",\"rendererSpatialProfileStage\":\"" + rendererSpatialProfileStage + "\""
          + ",\"rendererMatrixRequestedDomain\":\"" + escapeJsonString (rendererMatrix.requestedDomain) + "\""
          + ",\"rendererMatrixActiveDomain\":\"" + escapeJsonString (rendererMatrix.activeDomain) + "\""
          + ",\"rendererMatrixRequestedLayout\":\"" + escapeJsonString (rendererMatrix.requestedLayout) + "\""
          + ",\"rendererMatrixActiveLayout\":\"" + escapeJsonString (rendererMatrix.activeLayout) + "\""
          + ",\"rendererMatrixRuleId\":\"" + escapeJsonString (rendererMatrix.ruleId) + "\""
          + ",\"rendererMatrixRuleState\":\"" + escapeJsonString (rendererMatrix.ruleState) + "\""
          + ",\"rendererMatrixReasonCode\":\"" + escapeJsonString (rendererMatrix.reasonCode) + "\""
          + ",\"rendererMatrixFallbackMode\":\"" + escapeJsonString (rendererMatrix.fallbackMode) + "\""
          + ",\"rendererMatrixFailSafeRoute\":\"" + escapeJsonString (rendererMatrix.failSafeRoute) + "\""
          + ",\"rendererMatrixStatusText\":\"" + escapeJsonString (rendererMatrix.statusText) + "\""
          + ",\"rendererMatrixEventSeq\":" + juce::String (static_cast<juce::int64> (rendererMatrixEventSeq))
          + ",\"rendererMatrix\":{\"requestedDomain\":\"" + escapeJsonString (rendererMatrix.requestedDomain) + "\""
              + ",\"activeDomain\":\"" + escapeJsonString (rendererMatrix.activeDomain) + "\""
              + ",\"requestedLayout\":\"" + escapeJsonString (rendererMatrix.requestedLayout) + "\""
              + ",\"activeLayout\":\"" + escapeJsonString (rendererMatrix.activeLayout) + "\""
              + ",\"ruleId\":\"" + escapeJsonString (rendererMatrix.ruleId) + "\""
              + ",\"ruleState\":\"" + escapeJsonString (rendererMatrix.ruleState) + "\""
              + ",\"fallbackMode\":\"" + escapeJsonString (rendererMatrix.fallbackMode) + "\""
              + ",\"reasonCode\":\"" + escapeJsonString (rendererMatrix.reasonCode) + "\""
              + ",\"statusText\":\"" + escapeJsonString (rendererMatrix.statusText) + "\"}"
          + ",\"rendererHeadphoneModeRequested\":\"" + rendererHeadphoneModeRequested + "\""
          + ",\"rendererHeadphoneModeActive\":\"" + rendererHeadphoneModeActive + "\""
          + ",\"rendererHeadphoneProfileRequested\":\"" + rendererHeadphoneProfileRequested + "\""
          + ",\"rendererHeadphoneProfileActive\":\"" + rendererHeadphoneProfileActive + "\""
          + ",\"rendererHeadphoneCalibrationSchema\":\""
              + escapeJsonString (locusq::shared_contracts::headphone_calibration::kSchemaV1) + "\""
          + ",\"rendererHeadphoneCalibrationRequested\":\""
              + escapeJsonString (rendererHeadphoneCalibration.requested) + "\""
          + ",\"rendererHeadphoneCalibrationActive\":\""
              + escapeJsonString (rendererHeadphoneCalibration.active) + "\""
          + ",\"rendererHeadphoneCalibrationStage\":\""
              + escapeJsonString (rendererHeadphoneCalibration.stage) + "\""
          + ",\"rendererHeadphoneCalibrationFallbackReady\":"
              + juce::String (rendererHeadphoneCalibration.fallbackReady ? "true" : "false")
          + ",\"rendererHeadphoneCalibrationFallbackReason\":\""
              + escapeJsonString (rendererHeadphoneCalibration.fallbackReason) + "\""
          + ",\"rendererHeadphoneCalibration\":{\"schema\":\""
              + escapeJsonString (locusq::shared_contracts::headphone_calibration::kSchemaV1) + "\""
              + ",\"requested\":\"" + escapeJsonString (rendererHeadphoneCalibration.requested) + "\""
              + ",\"active\":\"" + escapeJsonString (rendererHeadphoneCalibration.active) + "\""
              + ",\"stage\":\"" + escapeJsonString (rendererHeadphoneCalibration.stage) + "\""
              + ",\"fallbackReady\":"
              + juce::String (rendererHeadphoneCalibration.fallbackReady ? "true" : "false")
              + ",\"fallbackReason\":\"" + escapeJsonString (rendererHeadphoneCalibration.fallbackReason) + "\"}"
          + ",\"rendererAuditionEnabled\":" + juce::String (rendererAuditionEnabled ? "true" : "false")
          + ",\"rendererAuditionSignal\":\"" + escapeJsonString (rendererAuditionSignal) + "\""
          + ",\"rendererAuditionMotion\":\"" + escapeJsonString (rendererAuditionMotion) + "\""
          + ",\"rendererAuditionLevelDb\":" + juce::String (rendererAuditionLevelDb, 1)
          + ",\"rendererAuditionSourceMode\":\"" + escapeJsonString (rendererAuditionSourceMode) + "\""
          + ",\"rendererAuditionRequestedMode\":\"" + escapeJsonString (rendererAuditionRequestedMode) + "\""
          + ",\"rendererAuditionResolvedMode\":\"" + escapeJsonString (rendererAuditionResolvedMode) + "\""
          + ",\"rendererAuditionBindingTarget\":\"" + escapeJsonString (rendererAuditionBindingTarget) + "\""
          + ",\"rendererAuditionBindingAvailable\":" + juce::String (rendererAuditionBindingAvailable ? "true" : "false")
          + ",\"rendererAuditionSeed\":" + juce::String (static_cast<juce::uint64> (rendererAuditionCloudSeed))
          + ",\"rendererAuditionTransportSync\":" + juce::String (rendererAuditionTransportSync ? "true" : "false")
          + ",\"rendererAuditionDensity\":" + juce::String (rendererAuditionDensity, 4)
          + ",\"rendererAuditionReactivity\":" + juce::String (rendererAuditionReactivity, 4)
          + ",\"rendererAuditionFallbackReason\":\"" + escapeJsonString (rendererAuditionFallbackReason) + "\""
          + ",\"rendererAuditionVisualActive\":" + juce::String (rendererAuditionVisualActive ? "true" : "false")
          + ",\"rendererAuditionVisual\":{\"x\":" + juce::String (rendererAuditionVisualX, 3)
              + ",\"y\":" + juce::String (rendererAuditionVisualY, 3)
              + ",\"z\":" + juce::String (rendererAuditionVisualZ, 3) + "}"
          + ",\"rendererAuditionCloud\":{\"enabled\":" + juce::String (rendererAuditionCloudEnabled ? "true" : "false")
              + ",\"pattern\":\"" + escapeJsonString (rendererAuditionCloudPattern) + "\""
              + ",\"mode\":\"" + escapeJsonString (rendererAuditionCloudMode) + "\""
              + ",\"emitterCount\":" + juce::String (rendererAuditionCloudEmitterCount)
              + ",\"pointCount\":" + juce::String (rendererAuditionCloudPointCount)
              + ",\"spreadMeters\":" + juce::String (rendererAuditionCloudSpreadMeters, 3)
              + ",\"seed\":" + juce::String (static_cast<juce::uint64> (rendererAuditionCloudSeed))
              + ",\"pulseHz\":" + juce::String (rendererAuditionCloudPulseHz, 3)
              + ",\"coherence\":" + juce::String (rendererAuditionCloudCoherence, 3)
              + ",\"emitters\":" + rendererAuditionCloudEmittersJson + "}"
          + ",\"rendererAuditionReactive\":{\"rms\":" + juce::String (rendererAuditionReactive.rms, 5)
              + ",\"peak\":" + juce::String (rendererAuditionReactive.peak, 5)
              + ",\"envFast\":" + juce::String (rendererAuditionReactive.envFast, 5)
              + ",\"envSlow\":" + juce::String (rendererAuditionReactive.envSlow, 5)
              + ",\"onset\":" + juce::String (rendererAuditionReactive.onset, 5)
              + ",\"brightness\":" + juce::String (rendererAuditionReactive.brightness, 5)
              + ",\"rainFadeRate\":" + juce::String (rendererAuditionReactive.rainFadeRate, 5)
              + ",\"snowFadeRate\":" + juce::String (rendererAuditionReactive.snowFadeRate, 5)
              + ",\"physicsVelocity\":" + juce::String (rendererAuditionReactive.physicsVelocity, 5)
              + ",\"physicsCollision\":" + juce::String (rendererAuditionReactive.physicsCollision, 5)
              + ",\"physicsDensity\":" + juce::String (rendererAuditionReactive.physicsDensity, 5)
              + ",\"physicsCoupling\":" + juce::String (rendererAuditionReactive.physicsCoupling, 5)
              + ",\"geometryScale\":" + juce::String (rendererAuditionReactive.geometryScale, 5)
              + ",\"geometryWidth\":" + juce::String (rendererAuditionReactive.geometryWidth, 5)
              + ",\"geometryDepth\":" + juce::String (rendererAuditionReactive.geometryDepth, 5)
              + ",\"geometryHeight\":" + juce::String (rendererAuditionReactive.geometryHeight, 5)
              + ",\"precipitationFade\":" + juce::String (rendererAuditionReactive.precipitationFade, 5)
              + ",\"collisionBurst\":" + juce::String (rendererAuditionReactive.collisionBurst, 5)
              + ",\"densitySpread\":" + juce::String (rendererAuditionReactive.densitySpread, 5)
              + ",\"headphoneOutputRms\":" + juce::String (rendererAuditionReactive.headphoneOutputRms, 5)
              + ",\"headphoneOutputPeak\":" + juce::String (rendererAuditionReactive.headphoneOutputPeak, 5)
              + ",\"headphoneParity\":" + juce::String (rendererAuditionReactive.headphoneParity, 5)
              + ",\"headphoneFallback\":" + juce::String (rendererAuditionReactiveHeadphoneFallback ? "true" : "false")
              + ",\"headphoneFallbackReason\":\"" + escapeJsonString (rendererAuditionReactiveHeadphoneFallbackReason) + "\""
              + ",\"sourceEnergy\":" + rendererAuditionSourceEnergyJson
              + ",\"reactiveActive\":" + juce::String (rendererAuditionReactivePublishedActive ? "true" : "false")
              + ",\"rmsNorm\":" + juce::String (rendererAuditionReactive.rmsNorm, 5)
              + ",\"peakNorm\":" + juce::String (rendererAuditionReactive.peakNorm, 5)
              + ",\"envFastNorm\":" + juce::String (rendererAuditionReactive.envFastNorm, 5)
              + ",\"envSlowNorm\":" + juce::String (rendererAuditionReactive.envSlowNorm, 5)
              + ",\"onsetNorm\":" + juce::String (rendererAuditionReactive.onset, 5)
              + ",\"brightnessNorm\":" + juce::String (rendererAuditionReactive.brightness, 5)
              + ",\"rainFadeRateNorm\":" + juce::String (rendererAuditionReactive.rainFadeRate, 5)
              + ",\"snowFadeRateNorm\":" + juce::String (rendererAuditionReactive.snowFadeRate, 5)
              + ",\"physicsVelocityNorm\":" + juce::String (rendererAuditionReactive.physicsVelocity, 5)
              + ",\"physicsCollisionNorm\":" + juce::String (rendererAuditionReactive.physicsCollision, 5)
              + ",\"physicsDensityNorm\":" + juce::String (rendererAuditionReactive.physicsDensity, 5)
              + ",\"physicsCouplingNorm\":" + juce::String (rendererAuditionReactive.physicsCoupling, 5)
              + ",\"headphoneOutputRmsNorm\":" + juce::String (rendererAuditionReactive.headphoneOutputRmsNorm, 5)
              + ",\"headphoneOutputPeakNorm\":" + juce::String (rendererAuditionReactive.headphoneOutputPeakNorm, 5)
              + ",\"headphoneParityNorm\":" + juce::String (rendererAuditionReactive.headphoneParityNorm, 5)
              + ",\"sourceEnergyNorm\":" + rendererAuditionSourceEnergyNormJson + "}"
          + ",\"rendererPhysicsLensEnabled\":" + juce::String (rendererPhysicsLensEnabled ? "true" : "false")
          + ",\"rendererPhysicsLensMix\":" + juce::String (rendererPhysicsLensMix, 3)
          + ",\"rendererSteamAudioCompiled\":" + juce::String (rendererSteamAudioCompiled ? "true" : "false")
          + ",\"rendererSteamAudioAvailable\":" + juce::String (rendererSteamAudioAvailable ? "true" : "false")
          + ",\"rendererSteamAudioInitStage\":\"" + rendererSteamAudioInitStage + "\""
          + ",\"rendererSteamAudioInitErrorCode\":" + juce::String (rendererSteamAudioInitErrorCode)
          + ",\"rendererSteamAudioRuntimeLib\":\"" + rendererSteamAudioRuntimeLib + "\""
          + ",\"rendererSteamAudioMissingSymbol\":\"" + rendererSteamAudioMissingSymbol + "\""
          + ",\"rendererAmbiCompiled\":" + juce::String (rendererAmbiCompiled ? "true" : "false")
          + ",\"rendererAmbiActive\":" + juce::String (rendererAmbiActive ? "true" : "false")
          + ",\"rendererAmbiMaxOrder\":" + juce::String (rendererAmbiMaxOrder)
          + ",\"rendererAmbiNormalization\":\"" + rendererAmbiNormalization + "\""
          + ",\"rendererAmbiChannelOrder\":\"" + rendererAmbiChannelOrder + "\""
          + ",\"rendererAmbiDecodeLayout\":\"" + rendererAmbiDecodeLayout + "\""
          + ",\"rendererAmbiStage\":\"" + rendererAmbiStage + "\""
          + ",\"clapBuildEnabled\":" + juce::String (clapDiagnostics.buildEnabled ? "true" : "false")
          + ",\"clapPropertiesAvailable\":" + juce::String (clapDiagnostics.propertiesAvailable ? "true" : "false")
          + ",\"clapIsPluginFormat\":" + juce::String (clapDiagnostics.isClapInstance ? "true" : "false")
          + ",\"clapIsActive\":" + juce::String (clapDiagnostics.isActive ? "true" : "false")
          + ",\"clapIsProcessing\":" + juce::String (clapDiagnostics.isProcessing ? "true" : "false")
          + ",\"clapHasTransport\":" + juce::String (clapDiagnostics.hasTransport ? "true" : "false")
          + ",\"clapWrapperType\":\"" + clapWrapperType + "\""
          + ",\"clapLifecycleStage\":\"" + clapLifecycleStage + "\""
          + ",\"clapRuntimeMode\":\"" + clapRuntimeMode + "\""
          + ",\"clapVersion\":{\"major\":" + juce::String (static_cast<int> (clapDiagnostics.versionMajor))
              + ",\"minor\":" + juce::String (static_cast<int> (clapDiagnostics.versionMinor))
              + ",\"revision\":" + juce::String (static_cast<int> (clapDiagnostics.versionRevision))
              + "}"
          + ",\"rendererOutputChannels\":" + outputChannelLabelsJson
          + ",\"rendererInternalSpeakers\":" + internalSpeakerLabelsJson
          + ",\"rendererQuadMap\":" + quadOutputMapJson
          + ",\"calCurrentTopologyProfile\":" + juce::String (currentCalTopologyProfile)
          + ",\"calCurrentTopologyId\":\"" + escapeJsonString (currentCalTopologyId) + "\""
          + ",\"calCurrentMonitoringPath\":" + juce::String (currentCalMonitoringPath)
          + ",\"calCurrentMonitoringPathId\":\"" + escapeJsonString (currentCalMonitoringPathId) + "\""
          + ",\"calCurrentDeviceProfile\":" + juce::String (currentCalDeviceProfile)
          + ",\"calCurrentDeviceProfileId\":\"" + escapeJsonString (currentCalDeviceProfileId) + "\""
          + ",\"calRequiredChannels\":" + juce::String (currentCalRequiredChannels)
          + ",\"calWritableChannels\":" + juce::String (currentCalWritableChannels)
          + ",\"calMappingLimitedToFirst4\":" + juce::String (currentCalMappingLimitedToFirst4 ? "true" : "false")
          + ",\"calTopologyAliasLegacySpeakerConfig\":" + juce::String (legacySpeakerConfigForTopologyIndex (currentCalTopologyProfile))
          + ",\"calCurrentSpeakerConfig\":" + juce::String (currentCalSpeakerConfig)
          + ",\"calCurrentSpeakerMap\":" + currentCalSpeakerRoutingJson
          + ",\"calAutoRoutingApplied\":" + juce::String (hasAppliedAutoDetectedCalibrationRouting ? "true" : "false")
          + ",\"calAutoRoutingOutputChannels\":" + juce::String (lastAutoDetectedOutputChannels)
          + ",\"calAutoRoutingTopologyProfile\":" + juce::String (lastAutoDetectedTopologyProfile)
          + ",\"calAutoRoutingTopologyId\":\"" + escapeJsonString (autoDetectedTopologyId) + "\""
          + ",\"calAutoRoutingSpeakerConfig\":" + juce::String (lastAutoDetectedSpeakerConfig)
          + ",\"calAutoRoutingMap\":" + autoDetectedRoutingJson
          + ",\"roomProfileValid\":" + juce::String (roomProfileValid ? "true" : "false")
          + ",\"roomDimensions\":{\"width\":" + juce::String (roomDimensions.x, 3)
              + ",\"depth\":" + juce::String (roomDimensions.y, 3)
              + ",\"height\":" + juce::String (roomDimensions.z, 3) + "}"
          + ",\"listener\":{\"x\":" + juce::String (listenerPosition.x, 3)
              + ",\"y\":" + juce::String (listenerPosition.y, 3)
              + ",\"z\":" + juce::String (listenerPosition.z, 3) + "}"
          + ",\"speakerRms\":" + speakerRmsJson
          + ",\"speakers\":" + speakersJson
          + ",\"physicsInteraction\":" + juce::String (sceneGraph.isPhysicsInteractionEnabled() ? "true" : "false")
          + ",\"animEnabled\":" + juce::String (apvts.getRawParameterValue ("anim_enable")->load() > 0.5f ? "true" : "false")
          + ",\"animMode\":" + juce::String (static_cast<int> (apvts.getRawParameterValue ("anim_mode")->load()))
          + ",\"animTime\":" + juce::String (timelineTime, 3)
          + ",\"animDuration\":" + juce::String (timelineDuration, 3)
          + ",\"animLooping\":" + juce::String (timelineLooping ? "true" : "false")
          + ",\"perfBlockMs\":" + juce::String (perfProcessBlockMs, 4)
          + ",\"perfEmitterMs\":" + juce::String (perfEmitterPublishMs, 4)
          + ",\"perfRendererMs\":" + juce::String (perfRendererProcessMs, 4)
          + "}";

    return json;
}

//==============================================================================
bool LocusQAudioProcessor::startCalibrationFromUI (const juce::var& options)
{
    const auto snapshotOutputChannels = getSnapshotOutputChannels();
    const auto layoutOutputChannels = static_cast<int> (getBusesLayout().getMainOutputChannelSet().size());
    const auto initialRouting = getCurrentCalibrationSpeakerRouting();
    const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
        snapshotOutputChannels,
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        initialRouting);

    applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, false);

    if (getCurrentMode() != LocusQMode::Calibrate)
    {
        const juce::String message { "Calibration start rejected: mode is not CALIBRATE." };
        calibrationEngine.recordExternalStartFailure ("mode_mismatch", message);
        DBG ("LocusQ: " << message);
        return false;
    }

    const auto state = calibrationEngine.getState();
    if (state == CalibrationEngine::State::Playing
        || state == CalibrationEngine::State::Recording
        || state == CalibrationEngine::State::Analyzing)
    {
        const juce::String message { "Calibration start rejected: calibration engine is already running." };
        calibrationEngine.recordExternalStartFailure ("engine_busy", message);
        DBG ("LocusQ: " << message);
        return false;
    }

    if (state == CalibrationEngine::State::Complete
        || state == CalibrationEngine::State::Error)
    {
        calibrationEngine.abortCalibration();
    }

    int testTypeIndex = static_cast<int> (apvts.getRawParameterValue ("cal_test_type")->load());
    float levelDb     = apvts.getRawParameterValue ("cal_test_level")->load();
    float sweepSecs   = 3.0f;
    float tailSecs    = 1.5f;
    int micChannel    = static_cast<int> (apvts.getRawParameterValue ("cal_mic_channel")->load()) - 1;
    int topologyProfile = getCurrentCalibrationTopologyProfileIndex();
    int monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    int deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    bool allowLimitedMapping = false;
    int speakerCh[4] =
    {
        static_cast<int> (apvts.getRawParameterValue ("cal_spk1_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk2_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk3_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk4_out")->load()) - 1
    };

    if (auto* obj = options.getDynamicObject())
    {
        if (obj->hasProperty ("testType"))
        {
            const auto& value = obj->getProperty ("testType");
            if (value.isString())
                testTypeIndex = toSignalTypeIndex (value.toString());
            else
                testTypeIndex = static_cast<int> (value);
        }

        if (obj->hasProperty ("testLevelDb"))
            levelDb = static_cast<float> (double (obj->getProperty ("testLevelDb")));

        if (obj->hasProperty ("sweepSeconds"))
            sweepSecs = static_cast<float> (double (obj->getProperty ("sweepSeconds")));

        if (obj->hasProperty ("tailSeconds"))
            tailSecs = static_cast<float> (double (obj->getProperty ("tailSeconds")));

        if (obj->hasProperty ("micChannel"))
            micChannel = static_cast<int> (obj->getProperty ("micChannel"));

        if (obj->hasProperty ("topologyProfile"))
        {
            const auto topologyText = normaliseCalibrationTopologyId (obj->getProperty ("topologyProfile").toString());
            const auto topologyIndex = indexOfCaseInsensitive (kCalibrationTopologyIds, topologyText);
            if (topologyIndex >= 0)
                topologyProfile = topologyIndex;
        }

        if (obj->hasProperty ("topologyProfileIndex"))
            topologyProfile = static_cast<int> (obj->getProperty ("topologyProfileIndex"));

        if (obj->hasProperty ("monitoringPath"))
        {
            const auto monitoringText = normaliseCalibrationMonitoringPathId (obj->getProperty ("monitoringPath").toString());
            const auto monitoringIndex = indexOfCaseInsensitive (kCalibrationMonitoringPathIds, monitoringText);
            if (monitoringIndex >= 0)
                monitoringPath = monitoringIndex;
        }

        if (obj->hasProperty ("monitoringPathIndex"))
            monitoringPath = static_cast<int> (obj->getProperty ("monitoringPathIndex"));

        if (obj->hasProperty ("deviceProfile"))
        {
            const auto deviceText = normaliseCalibrationDeviceProfileId (obj->getProperty ("deviceProfile").toString());
            const auto deviceIndex = indexOfCaseInsensitive (kCalibrationDeviceProfileIds, deviceText);
            if (deviceIndex >= 0)
                deviceProfile = deviceIndex;
        }

        if (obj->hasProperty ("deviceProfileIndex"))
            deviceProfile = static_cast<int> (obj->getProperty ("deviceProfileIndex"));

        if (obj->hasProperty ("allowLimitedMapping"))
            allowLimitedMapping = static_cast<bool> (obj->getProperty ("allowLimitedMapping"));

        if (obj->hasProperty ("speakerChannels"))
        {
            const auto channels = obj->getProperty ("speakerChannels");
            if (auto* arr = channels.getArray())
            {
                const auto count = juce::jmin (4, arr->size());
                for (int i = 0; i < count; ++i)
                    speakerCh[i] = static_cast<int> (arr->getReference (i));
            }
        }
    }

    micChannel = juce::jlimit (0, 7, micChannel);
    sweepSecs  = juce::jlimit (0.1f, 30.0f, sweepSecs);
    tailSecs   = juce::jlimit (0.0f, 10.0f, tailSecs);
    topologyProfile = juce::jlimit (0, static_cast<int> (kCalibrationTopologyIds.size()) - 1, topologyProfile);
    monitoringPath = juce::jlimit (0, static_cast<int> (kCalibrationMonitoringPathIds.size()) - 1, monitoringPath);
    deviceProfile = juce::jlimit (0, static_cast<int> (kCalibrationDeviceProfileIds.size()) - 1, deviceProfile);

    for (int& ch : speakerCh)
        ch = juce::jlimit (0, 7, ch);

    const auto requiredChannels = getRequiredCalibrationChannelsForTopologyIndex (topologyProfile);
    const std::array<int, SpatialRenderer::NUM_SPEAKERS> requestedRouting
    {
        speakerCh[0] + 1,
        speakerCh[1] + 1,
        speakerCh[2] + 1,
        speakerCh[3] + 1
    };
    const auto writableChannels = resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        requestedRouting);
    if (requiredChannels > writableChannels && ! allowLimitedMapping)
    {
        const juce::String message = "Calibration start rejected: topology requires "
            + juce::String (requiredChannels)
            + " writable channels but runtime reports "
            + juce::String (writableChannels)
            + ". Enable limited mapping acknowledgement to proceed.";
        calibrationEngine.recordExternalStartFailure ("writable_channel_gate", message);
        DBG ("LocusQ: " << message);
        return false;
    }

    const auto legacySpeakerConfig = legacySpeakerConfigForTopologyIndex (topologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_topology_profile", topologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_monitoring_path", monitoringPath);
    setIntegerParameterValueNotifyingHost ("cal_device_profile", deviceProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", legacySpeakerConfig);

    // Keep renderer diagnostics in sync so CALIBRATE can validate requested vs active
    // headphone/spatial states deterministically.
    const auto headphoneModeIndex = (monitoringPath == 2 || monitoringPath == 3) ? 1 : 0;
    setIntegerParameterValueNotifyingHost ("rend_headphone_mode", headphoneModeIndex);
    setIntegerParameterValueNotifyingHost ("rend_headphone_profile", deviceProfile);

    int rendererSpatialProfileIndex = 0;
    switch (topologyProfile)
    {
        case 0: rendererSpatialProfileIndex = 1; break; // stereo safe
        case 1: rendererSpatialProfileIndex = 1; break; // stereo 2.0
        case 2: rendererSpatialProfileIndex = 2; break; // quad 4.0
        case 3: rendererSpatialProfileIndex = 3; break; // surround 5.2.1
        case 4: rendererSpatialProfileIndex = 4; break; // surround 7.2.1 (7.1)
        case 5: rendererSpatialProfileIndex = 4; break; // surround 7.2.1 (7.1.2 alias target)
        case 6: rendererSpatialProfileIndex = 5; break; // surround 7.4.2
        case 7: rendererSpatialProfileIndex = 9; break; // binaural virtual 3D stereo
        case 8: rendererSpatialProfileIndex = 6; break; // ambisonic FOA
        case 9: rendererSpatialProfileIndex = 7; break; // ambisonic HOA
        case 10: rendererSpatialProfileIndex = 9; break; // downmix target
        default: break;
    }
    setIntegerParameterValueNotifyingHost ("rend_spatial_profile", rendererSpatialProfileIndex);

    if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("cal_mic_channel")))
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (micChannel + 1)));

    const auto started = calibrationEngine.startCalibration (toSignalType (testTypeIndex),
                                                             levelDb,
                                                             sweepSecs,
                                                             tailSecs,
                                                             speakerCh,
                                                             micChannel);

    if (! started)
    {
        const auto startDiagnostics = calibrationEngine.getLastStartDiagnostics();
        DBG ("LocusQ: Calibration start rejected ["
             << startDiagnostics.code
             << "] "
             << startDiagnostics.message);
        return false;
    }

    const auto startDiagnostics = calibrationEngine.getLastStartDiagnostics();
    DBG ("LocusQ: Calibration start accepted (seq="
         << static_cast<int> (startDiagnostics.seq)
         << ", writableChannels="
         << writableChannels
         << ")");
    return true;
}

void LocusQAudioProcessor::abortCalibrationFromUI()
{
    calibrationEngine.abortCalibration();
}

juce::var LocusQAudioProcessor::redetectCalibrationRoutingFromUI()
{
    const auto snapshotOutputChannels = getSnapshotOutputChannels();
    const auto layoutOutputChannels = static_cast<int> (getBusesLayout().getMainOutputChannelSet().size());
    const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
        snapshotOutputChannels,
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting());

    applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, true);

    juce::var resultVar (new juce::DynamicObject());
    auto* result = resultVar.getDynamicObject();
    if (result == nullptr)
        return resultVar;

    result->setProperty ("ok", true);
    result->setProperty ("outputChannels", effectiveWritableChannels);
    const auto topologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    const auto requiredChannels = getRequiredCalibrationChannelsForTopologyIndex (topologyProfile);
    const auto writableChannels = resolveCalibrationWritableChannels (
        snapshotOutputChannels,
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting());
    result->setProperty ("speakerConfigIndex", getCurrentCalibrationSpeakerConfigIndex());
    result->setProperty ("topologyProfileIndex", topologyProfile);
    result->setProperty ("topologyProfile", calibrationTopologyIdForIndex (topologyProfile));
    result->setProperty ("monitoringPathIndex", monitoringPath);
    result->setProperty ("monitoringPath", calibrationMonitoringPathIdForIndex (monitoringPath));
    result->setProperty ("deviceProfileIndex", deviceProfile);
    result->setProperty ("deviceProfile", calibrationDeviceProfileIdForIndex (deviceProfile));
    result->setProperty ("requiredChannels", requiredChannels);
    result->setProperty ("writableChannels", writableChannels);
    result->setProperty ("mappingLimitedToFirst4", requiredChannels > writableChannels);

    juce::Array<juce::var> routing;
    const auto map = getCurrentCalibrationSpeakerRouting();
    for (const auto channel : map)
        routing.add (juce::jlimit (1, 8, channel));
    result->setProperty ("routing", juce::var (routing));

    return resultVar;
}

juce::var LocusQAudioProcessor::getCalibrationStatus() const
{
    auto progress = calibrationEngine.getProgress();
    const auto state = progress.state;
    const auto speakerIndex = juce::jlimit (0, 3, progress.currentSpeaker);
    const auto topologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    const auto outputChannels = getMainBusNumOutputChannels();
    const bool rendererSteamAudioAvailable = spatialRenderer.isSteamAudioAvailable();
    const juce::String rendererSteamAudioInitStage {
        SpatialRenderer::steamAudioInitStageToString (spatialRenderer.getSteamAudioInitStageIndex())
    };
    const int rendererHeadphoneModeRequestedIndex = juce::jlimit (
        0,
        1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_headphone_mode")->load())));
    auto rendererHeadphoneModeActiveIndex = spatialRenderer.getHeadphoneRenderModeActiveIndex();
    if (outputChannels >= 2)
    {
        rendererHeadphoneModeActiveIndex =
            (rendererHeadphoneModeRequestedIndex == static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
             && rendererSteamAudioAvailable)
                ? static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
                : static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    else
    {
        rendererHeadphoneModeActiveIndex = static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    auto headphoneCalibration = buildHeadphoneCalibrationDiagnosticsSnapshot (
        monitoringPath,
        rendererHeadphoneModeRequestedIndex,
        rendererHeadphoneModeActiveIndex,
        outputChannels,
        rendererSteamAudioAvailable,
        rendererSteamAudioInitStage);
    auto profileSyncSeq = static_cast<juce::int64> (sceneSnapshotSequence);
    {
        const juce::SpinLock::ScopedLockType publishedCalibrationLock (publishedHeadphoneCalibrationLock);
        if (publishedHeadphoneCalibrationDiagnostics.valid)
        {
            profileSyncSeq =
                static_cast<juce::int64> (publishedHeadphoneCalibrationDiagnostics.profileSyncSeq);
            headphoneCalibration.requested = publishedHeadphoneCalibrationDiagnostics.requested;
            headphoneCalibration.active = publishedHeadphoneCalibrationDiagnostics.active;
            headphoneCalibration.stage = publishedHeadphoneCalibrationDiagnostics.stage;
            headphoneCalibration.fallbackReady = publishedHeadphoneCalibrationDiagnostics.fallbackReady;
            headphoneCalibration.fallbackReason = publishedHeadphoneCalibrationDiagnostics.fallbackReason;
        }
    }
    const auto requiredChannels = getRequiredCalibrationChannelsForTopologyIndex (topologyProfile);
    const auto routing = getCurrentCalibrationSpeakerRouting();
    const auto writableChannels = resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        routing);
    const auto mappingLimitedToFirst4 = requiredChannels > writableChannels;
    const auto startDiagnostics = calibrationEngine.getLastStartDiagnostics();
    const auto checkedRows = juce::jlimit (1, SpatialRenderer::NUM_SPEAKERS, juce::jmin (requiredChannels, writableChannels));
    std::array<bool, 9> seenChannels {};
    bool mappingDuplicateChannels = false;
    bool mappingChannelsInRange = true;

    for (int i = 0; i < checkedRows; ++i)
    {
        const auto routedChannel = juce::jlimit (1, 8, routing[static_cast<size_t> (i)]);
        if (routedChannel < 1 || routedChannel > 8)
        {
            mappingChannelsInRange = false;
            continue;
        }

        if (seenChannels[static_cast<size_t> (routedChannel)])
            mappingDuplicateChannels = true;
        seenChannels[static_cast<size_t> (routedChannel)] = true;
    }
    const bool mappingValid = mappingChannelsInRange && ! mappingDuplicateChannels && ! mappingLimitedToFirst4;

    int completedSpeakers = 0;
    float speakerPhasePercent = 0.0f;
    bool running = false;

    switch (state)
    {
        case CalibrationEngine::State::Idle:
            break;

        case CalibrationEngine::State::Playing:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = juce::jlimit (0.0f, 1.0f, progress.playPercent) * 0.5f;
            break;

        case CalibrationEngine::State::Recording:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = 0.5f + juce::jlimit (0.0f, 1.0f, progress.recordPercent) * 0.45f;
            break;

        case CalibrationEngine::State::Analyzing:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = 0.95f;
            break;

        case CalibrationEngine::State::Complete:
            completedSpeakers = 4;
            speakerPhasePercent = 1.0f;
            break;

        case CalibrationEngine::State::Error:
            completedSpeakers = speakerIndex;
            break;
    }

    auto overallPercent = (state == CalibrationEngine::State::Complete)
                            ? 1.0f
                            : (static_cast<float> (completedSpeakers) + speakerPhasePercent) / 4.0f;
    overallPercent = juce::jlimit (0.0f, 1.0f, overallPercent);

    juce::var statusVar (new juce::DynamicObject());
    auto* status = statusVar.getDynamicObject();

    status->setProperty ("state", toCalibrationStateString (state));
    status->setProperty ("stateCode", static_cast<int> (state));
    status->setProperty ("running", running);
    status->setProperty ("complete", state == CalibrationEngine::State::Complete);
    status->setProperty ("currentSpeaker", speakerIndex + 1);
    status->setProperty ("completedSpeakers", completedSpeakers);
    status->setProperty ("playPercent", juce::jlimit (0.0f, 1.0f, progress.playPercent));
    status->setProperty ("recordPercent", juce::jlimit (0.0f, 1.0f, progress.recordPercent));
    status->setProperty ("overallPercent", overallPercent);
    status->setProperty ("message", progress.message);
    status->setProperty ("startAck", startDiagnostics.accepted);
    status->setProperty ("startSeq", static_cast<int> (startDiagnostics.seq));
    status->setProperty ("startCode", startDiagnostics.code);
    status->setProperty ("startMessage", startDiagnostics.message);
    status->setProperty ("startStateAtRequest", startDiagnostics.stateAtRequest);
    status->setProperty ("startTimestampMs", startDiagnostics.timestampMs);
    status->setProperty ("profileSyncSeq", profileSyncSeq);
    status->setProperty ("topologyProfileIndex", topologyProfile);
    status->setProperty ("topologyProfile", calibrationTopologyIdForIndex (topologyProfile));
    status->setProperty ("monitoringPathIndex", monitoringPath);
    status->setProperty ("monitoringPath", calibrationMonitoringPathIdForIndex (monitoringPath));
    status->setProperty ("deviceProfileIndex", deviceProfile);
    status->setProperty ("deviceProfile", calibrationDeviceProfileIdForIndex (deviceProfile));
    status->setProperty ("headphoneCalibrationSchema", locusq::shared_contracts::headphone_calibration::kSchemaV1);
    status->setProperty ("headphoneCalibrationRequested", headphoneCalibration.requested);
    status->setProperty ("headphoneCalibrationActive", headphoneCalibration.active);
    status->setProperty ("headphoneCalibrationStage", headphoneCalibration.stage);
    status->setProperty ("headphoneCalibrationFallbackReady", headphoneCalibration.fallbackReady);
    status->setProperty ("headphoneCalibrationFallbackReason", headphoneCalibration.fallbackReason);
    status->setProperty ("requiredChannels", requiredChannels);
    status->setProperty ("writableChannels", writableChannels);
    status->setProperty ("mappingLimitedToFirst4", mappingLimitedToFirst4);
    status->setProperty ("mappingDuplicateChannels", mappingDuplicateChannels);
    status->setProperty ("mappingValid", mappingValid);

    juce::var headphoneCalibrationVar (new juce::DynamicObject());
    if (auto* headphoneContract = headphoneCalibrationVar.getDynamicObject())
    {
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kSchema,
            locusq::shared_contracts::headphone_calibration::kSchemaV1);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kRequested,
            headphoneCalibration.requested);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kActive,
            headphoneCalibration.active);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kStage,
            headphoneCalibration.stage);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kFallbackReady,
            headphoneCalibration.fallbackReady);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kFallbackReason,
            headphoneCalibration.fallbackReason);
    }
    status->setProperty ("headphoneCalibration", headphoneCalibrationVar);

    if (! running
        && state != CalibrationEngine::State::Complete
        && ! startDiagnostics.accepted
        && startDiagnostics.seq > 0
        && startDiagnostics.message.isNotEmpty())
    {
        status->setProperty ("message", startDiagnostics.message);
    }

    juce::Array<juce::var> speakerLevels;
    speakerLevels.ensureStorageAllocated (4);
    for (int i = 0; i < 4; ++i)
    {
        float level = 0.0f;

        if (state == CalibrationEngine::State::Complete || i < completedSpeakers)
        {
            level = 1.0f;
        }
        else if (running && i == speakerIndex)
        {
            if (state == CalibrationEngine::State::Playing)
                level = juce::jlimit (0.0f, 1.0f, progress.playPercent);
            else if (state == CalibrationEngine::State::Recording)
                level = juce::jlimit (0.0f, 1.0f, progress.recordPercent);
            else if (state == CalibrationEngine::State::Analyzing)
                level = 1.0f;
        }

        speakerLevels.add (juce::jlimit (0.0f, 1.0f, level));
    }
    status->setProperty ("speakerLevels", juce::var (speakerLevels));

    juce::Array<juce::var> speakerRouting;
    speakerRouting.ensureStorageAllocated (4);
    for (const auto channel : routing)
        speakerRouting.add (juce::jlimit (1, 8, channel));
    status->setProperty ("speakerRouting", juce::var (speakerRouting));

    const auto roomProfile = sceneGraph.getRoomProfile();
    status->setProperty ("profileValid", roomProfile != nullptr && roomProfile->valid);
    status->setProperty ("phasePass", state == CalibrationEngine::State::Complete);
    const auto estimatedRt60 = calibrationEngine.getResult().estimatedRT60;
    const bool delayPass = state == CalibrationEngine::State::Complete
                           && std::isfinite (estimatedRt60)
                           && estimatedRt60 > 0.0f;
    status->setProperty ("delayPass", delayPass);

    if (state == CalibrationEngine::State::Complete)
        status->setProperty ("estimatedRT60", estimatedRt60);

    return statusVar;
}

juce::var LocusQAudioProcessor::serialiseKeyframeTimelineLocked() const
{
    juce::var timelineVar (new juce::DynamicObject());
    auto* timeline = timelineVar.getDynamicObject();

    timeline->setProperty ("durationSeconds", keyframeTimeline.getDurationSeconds());
    timeline->setProperty ("looping", keyframeTimeline.isLooping());
    timeline->setProperty ("playbackRate", keyframeTimeline.getPlaybackRate());
    timeline->setProperty ("currentTimeSeconds", keyframeTimeline.getCurrentTimeSeconds());

    juce::Array<juce::var> tracks;

    for (const auto& track : keyframeTimeline.getTracks())
    {
        juce::var trackVar (new juce::DynamicObject());
        auto* trackObject = trackVar.getDynamicObject();
        trackObject->setProperty ("parameterId", track.getParameterId());

        juce::Array<juce::var> keyframes;
        for (const auto& keyframe : track.getKeyframes())
        {
            juce::var keyframeVar (new juce::DynamicObject());
            auto* keyframeObject = keyframeVar.getDynamicObject();
            keyframeObject->setProperty ("timeSeconds", keyframe.timeSeconds);
            keyframeObject->setProperty ("value", keyframe.value);
            keyframeObject->setProperty ("curve", keyframeCurveToString (keyframe.curve));
            keyframes.add (keyframeVar);
        }

        trackObject->setProperty ("keyframes", juce::var (keyframes));
        tracks.add (trackVar);
    }

    timeline->setProperty ("tracks", juce::var (tracks));
    return timelineVar;
}

bool LocusQAudioProcessor::applyKeyframeTimelineLocked (const juce::var& timelineState)
{
    auto* timeline = timelineState.getDynamicObject();
    if (timeline == nullptr)
        return false;

    auto* trackArray = timeline->getProperty ("tracks").getArray();
    if (trackArray == nullptr)
        return false;

    keyframeTimeline.clearTracks();

    for (const auto& trackValue : *trackArray)
    {
        auto* trackObject = trackValue.getDynamicObject();
        if (trackObject == nullptr)
            continue;

        const auto parameterId = trackObject->getProperty ("parameterId").toString().trim();
        if (parameterId.isEmpty())
            continue;

        std::vector<Keyframe> keyframes;
        if (auto* keyframeArray = trackObject->getProperty ("keyframes").getArray())
        {
            keyframes.reserve (static_cast<size_t> (keyframeArray->size()));

            for (const auto& keyframeValue : *keyframeArray)
            {
                auto* keyframeObject = keyframeValue.getDynamicObject();
                if (keyframeObject == nullptr)
                    continue;

                Keyframe keyframe;
                keyframe.timeSeconds = static_cast<double> (keyframeObject->getProperty ("timeSeconds"));
                keyframe.value = static_cast<float> (double (keyframeObject->getProperty ("value")));
                keyframe.curve = keyframeCurveFromVar (keyframeObject->getProperty ("curve"));
                keyframes.push_back (keyframe);
            }
        }

        if (! keyframes.empty())
        {
            KeyframeTrack track { parameterId };
            track.setKeyframes (std::move (keyframes));
            keyframeTimeline.addOrReplaceTrack (std::move (track));
        }
    }

    if (timeline->hasProperty ("durationSeconds"))
        keyframeTimeline.setDurationSeconds (static_cast<double> (timeline->getProperty ("durationSeconds")));

    if (timeline->hasProperty ("looping"))
        keyframeTimeline.setLooping (static_cast<bool> (timeline->getProperty ("looping")));

    if (timeline->hasProperty ("playbackRate"))
        keyframeTimeline.setPlaybackRate (static_cast<float> (double (timeline->getProperty ("playbackRate"))));

    if (timeline->hasProperty ("currentTimeSeconds"))
        keyframeTimeline.setCurrentTimeSeconds (static_cast<double> (timeline->getProperty ("currentTimeSeconds")));

    if (! keyframeTimeline.hasAnyTrack())
        initialiseDefaultKeyframeTimeline();

    return true;
}

juce::var LocusQAudioProcessor::getKeyframeTimelineForUI() const
{
    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    return serialiseKeyframeTimelineLocked();
}

bool LocusQAudioProcessor::setKeyframeTimelineFromUI (const juce::var& timelineState)
{
    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    return applyKeyframeTimelineLocked (timelineState);
}

bool LocusQAudioProcessor::setTimelineCurrentTimeFromUI (double timeSeconds)
{
    if (! std::isfinite (timeSeconds))
        return false;

    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    const auto clamped = juce::jlimit (0.0,
                                       juce::jmax (0.0, keyframeTimeline.getDurationSeconds()),
                                       timeSeconds);
    keyframeTimeline.setCurrentTimeSeconds (clamped);
    return true;
}

juce::String LocusQAudioProcessor::sanitisePresetName (const juce::String& presetName)
{
    return locusq::processor_bridge::sanitisePresetName (presetName);
}

juce::String LocusQAudioProcessor::normalisePresetType (const juce::String& presetType)
{
    return locusq::processor_bridge::normalisePresetType (presetType,
                                                          kEmitterPresetTypeEmitter,
                                                          kEmitterPresetTypeMotion);
}

juce::String LocusQAudioProcessor::normaliseChoreographyPackId (const juce::String& packId)
{
    return locusq::processor_bridge::normaliseChoreographyPackId (packId, kChoreographyPackIds);
}

juce::String LocusQAudioProcessor::normaliseCalibrationTopologyId (const juce::String& topologyId)
{
    return locusq::processor_bridge::normaliseCalibrationTopologyId (
        topologyId,
        kCalibrationTopologyIds,
        [] (int index) { return calibrationTopologyIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::normaliseCalibrationMonitoringPathId (const juce::String& monitoringPathId)
{
    return locusq::processor_bridge::normaliseCalibrationMonitoringPathId (
        monitoringPathId,
        kCalibrationMonitoringPathIds,
        [] (int index) { return calibrationMonitoringPathIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::normaliseCalibrationDeviceProfileId (const juce::String& deviceProfileId)
{
    return locusq::processor_bridge::normaliseCalibrationDeviceProfileId (
        deviceProfileId,
        kCalibrationDeviceProfileIds,
        [] (int index) { return calibrationDeviceProfileIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::inferPresetTypeFromPayload (const juce::var& payload)
{
    return locusq::processor_bridge::inferPresetTypeFromPayload (payload,
                                                                 kEmitterPresetTypeProperty,
                                                                 kEmitterPresetTypeEmitter,
                                                                 kEmitterPresetTypeMotion);
}

juce::String LocusQAudioProcessor::sanitiseEmitterLabel (const juce::String& label)
{
    return locusq::processor_bridge::sanitiseEmitterLabel (label);
}

juce::File LocusQAudioProcessor::getPresetDirectory() const
{
    return locusq::processor_bridge::getUserDataSubdirectory ("Presets");
}

juce::File LocusQAudioProcessor::resolvePresetFileFromOptions (const juce::var& options) const
{
    return locusq::processor_bridge::resolveNamedJsonFileFromOptions (
        options,
        getPresetDirectory(),
        [] (const juce::String& name) { return locusq::processor_bridge::sanitisePresetName (name); });
}

juce::File LocusQAudioProcessor::getCalibrationProfileDirectory() const
{
    return locusq::processor_bridge::getUserDataSubdirectory ("CalibrationProfiles");
}

juce::File LocusQAudioProcessor::resolveCalibrationProfileFileFromOptions (const juce::var& options) const
{
    return locusq::processor_bridge::resolveNamedJsonFileFromOptions (
        options,
        getCalibrationProfileDirectory(),
        [] (const juce::String& name) { return locusq::processor_bridge::sanitisePresetName (name); });
}

juce::String LocusQAudioProcessor::getSnapshotOutputLayout() const
{
    return outputLayoutToString (getBusesLayout().getMainOutputChannelSet());
}

int LocusQAudioProcessor::getSnapshotOutputChannels() const
{
    return locusq::processor_core::readSnapshotOutputChannels (getMainBusNumOutputChannels(),
                                                               getTotalNumOutputChannels());
}

std::array<int, SpatialRenderer::NUM_SPEAKERS> LocusQAudioProcessor::getCurrentCalibrationSpeakerRouting() const
{
    return locusq::processor_core::readCalibrationSpeakerRouting (apvts);
}

int LocusQAudioProcessor::getCurrentCalibrationSpeakerConfigIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (apvts,
                                                               "cal_spk_config",
                                                               0,
                                                               1,
                                                               0);
}

int LocusQAudioProcessor::getCurrentCalibrationTopologyProfileIndex() const
{
    if (apvts.getRawParameterValue ("cal_topology_profile") != nullptr)
    {
        return locusq::processor_core::readDiscreteParameterIndex (
            apvts,
            "cal_topology_profile",
            0,
            static_cast<int> (kCalibrationTopologyIds.size()) - 1,
            1);
    }

    const auto legacyConfig = getCurrentCalibrationSpeakerConfigIndex();
    return legacyConfig == 1 ? 1 : 2;
}

int LocusQAudioProcessor::getCurrentCalibrationMonitoringPathIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (
        apvts,
        "cal_monitoring_path",
        0,
        static_cast<int> (kCalibrationMonitoringPathIds.size()) - 1,
        0);
}

int LocusQAudioProcessor::getCurrentCalibrationDeviceProfileIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (
        apvts,
        "cal_device_profile",
        0,
        static_cast<int> (kCalibrationDeviceProfileIds.size()) - 1,
        0);
}

int LocusQAudioProcessor::getRequiredCalibrationChannelsForTopologyIndex (int topologyIndex) const
{
    return calibrationRequiredChannelsForTopologyIndex (topologyIndex);
}

void LocusQAudioProcessor::applyAutoDetectedCalibrationRoutingIfAppropriate (int outputChannels, bool force)
{
    const auto clampedOutputChannels = juce::jlimit (1, 16, outputChannels);

    std::array<int, SpatialRenderer::NUM_SPEAKERS> autoRouting { 1, 2, 3, 4 };
    int autoSpeakerConfig = 0; // 0 = 4x Mono, 1 = 2x Stereo
    int autoTopologyProfile = topologyProfileForOutputChannels (clampedOutputChannels);

    if (clampedOutputChannels == 1)
    {
        autoSpeakerConfig = 1;
        autoRouting = { 1, 1, 1, 1 };
    }
    else if (clampedOutputChannels == 2)
    {
        autoSpeakerConfig = 1;
        autoRouting = { 1, 2, 1, 2 };
    }
    else if (clampedOutputChannels == 3)
    {
        autoSpeakerConfig = 0;
        autoRouting = { 1, 2, 3, 3 };
    }

    const auto currentRouting = getCurrentCalibrationSpeakerRouting();
    const auto currentSpeakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto currentTopologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto isFactoryMonoRouting = currentSpeakerConfig == 0
                                      && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 3, 4 };
    const auto isFactoryStereoRouting = currentSpeakerConfig == 1
                                        && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 1, 2 };
    const auto isFactoryMonoByChoice = currentSpeakerConfig == 0
                                       && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 1, 2 };
    const auto isFactoryTopologyProfile = currentTopologyProfile == 2 || currentTopologyProfile == 1;
    const auto followsPreviousAuto = hasAppliedAutoDetectedCalibrationRouting
                                     && currentTopologyProfile == lastAutoDetectedTopologyProfile
                                     && currentSpeakerConfig == lastAutoDetectedSpeakerConfig
                                     && currentRouting == lastAutoDetectedSpeakerRouting;

    if (! force
        && ! followsPreviousAuto
        && ! isFactoryMonoRouting
        && ! isFactoryStereoRouting
        && ! isFactoryMonoByChoice
        && ! isFactoryTopologyProfile)
    {
        return;
    }

    if (hasAppliedAutoDetectedCalibrationRouting
        && clampedOutputChannels == lastAutoDetectedOutputChannels
        && autoTopologyProfile == lastAutoDetectedTopologyProfile
        && autoSpeakerConfig == lastAutoDetectedSpeakerConfig
        && autoRouting == lastAutoDetectedSpeakerRouting)
    {
        return;
    }

    setIntegerParameterValueNotifyingHost ("cal_topology_profile", autoTopologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", autoSpeakerConfig);
    setIntegerParameterValueNotifyingHost ("cal_spk1_out", autoRouting[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", autoRouting[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", autoRouting[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", autoRouting[3]);

    hasAppliedAutoDetectedCalibrationRouting = true;
    lastAutoDetectedOutputChannels = clampedOutputChannels;
    lastAutoDetectedTopologyProfile = autoTopologyProfile;
    lastAutoDetectedSpeakerConfig = autoSpeakerConfig;
    lastAutoDetectedSpeakerRouting = autoRouting;
}

void LocusQAudioProcessor::setIntegerParameterValueNotifyingHost (const char* parameterId, int value)
{
    locusq::processor_core::setIntegerParameterValueNotifyingHost (apvts, parameterId, value);
}

void LocusQAudioProcessor::migrateSnapshotLayoutIfNeeded (const juce::ValueTree& restoredState)
{
    int storedOutputChannels = 0;
    if (restoredState.hasProperty (kSnapshotOutputChannelsProperty))
    {
        storedOutputChannels = juce::jlimit (1,
                                             kMaxSnapshotOutputChannels,
                                             static_cast<int> (restoredState.getProperty (kSnapshotOutputChannelsProperty)));
    }
    else if (restoredState.hasProperty (kSnapshotOutputLayoutProperty))
    {
        const auto storedLayout = restoredState.getProperty (kSnapshotOutputLayoutProperty).toString().trim().toLowerCase();
        if (storedLayout == "mono")
            storedOutputChannels = 1;
        else if (storedLayout == "stereo")
            storedOutputChannels = 2;
        else if (storedLayout == "quad")
            storedOutputChannels = SpatialRenderer::NUM_SPEAKERS;
        else if (storedLayout == "surround_5_1")
            storedOutputChannels = 6;
        else if (storedLayout == "surround_5_2_1")
            storedOutputChannels = 8;
        else if (storedLayout == "surround_7_1")
            storedOutputChannels = 8;
        else if (storedLayout == "surround_7_2_1")
            storedOutputChannels = 10;
        else if (storedLayout == "surround_7_1_4")
            storedOutputChannels = 12;
        else if (storedLayout == "surround_7_4_2")
            storedOutputChannels = 13;
        else if (storedLayout == "multichannel")
            storedOutputChannels = juce::jmax (SpatialRenderer::NUM_SPEAKERS, storedOutputChannels);
    }

    const auto currentOutputChannels = juce::jlimit (1,
                                                     kMaxSnapshotOutputChannels,
                                                     getSnapshotOutputChannels());
    const auto isLegacySnapshot = ! restoredState.hasProperty (kSnapshotSchemaProperty);
    const auto hasLayoutMismatch = (storedOutputChannels > 0 && storedOutputChannels != currentOutputChannels);

    if (! isLegacySnapshot && ! hasLayoutMismatch)
        return;

    std::array<int, SpatialRenderer::NUM_SPEAKERS> migratedSpeakerMap { 1, 2, 3, 4 };
    int migratedSpeakerConfig = 0;
    const int migratedTopologyProfile = topologyProfileForOutputChannels (currentOutputChannels);

    if (currentOutputChannels == 1)
    {
        migratedSpeakerMap.fill (1);
        migratedSpeakerConfig = 1;
    }
    else if (currentOutputChannels == 2)
    {
        migratedSpeakerMap = { 1, 2, 1, 2 };
        migratedSpeakerConfig = 1;
    }

    setIntegerParameterValueNotifyingHost ("cal_topology_profile", migratedTopologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", migratedSpeakerConfig);
    setIntegerParameterValueNotifyingHost ("cal_spk1_out", migratedSpeakerMap[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", migratedSpeakerMap[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", migratedSpeakerMap[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", migratedSpeakerMap[3]);
}

juce::String LocusQAudioProcessor::keyframeCurveToString (KeyframeCurve curve)
{
    const auto index = static_cast<size_t> (juce::jlimit (0, static_cast<int> (kCurveNames.size()) - 1, static_cast<int> (curve)));
    return juce::String (kCurveNames[index]);
}

KeyframeCurve LocusQAudioProcessor::keyframeCurveFromVar (const juce::var& value)
{
    if (value.isInt() || value.isInt64() || value.isDouble())
        return static_cast<KeyframeCurve> (juce::jlimit (0, static_cast<int> (kCurveNames.size()) - 1, static_cast<int> (value)));

    const auto text = value.toString().trim();
    for (size_t i = 0; i < kCurveNames.size(); ++i)
    {
        if (text.equalsIgnoreCase (kCurveNames[i]))
            return static_cast<KeyframeCurve> (i);
    }

    return KeyframeCurve::linear;
}

std::optional<juce::var> LocusQAudioProcessor::readJsonFromFile (const juce::File& file)
{
    return locusq::processor_bridge::readJsonFromFile (file);
}

bool LocusQAudioProcessor::writeJsonToFile (const juce::File& file, const juce::var& payload)
{
    return locusq::processor_bridge::writeJsonToFile (file, payload);
}

void LocusQAudioProcessor::applyEmitterLabelToSceneSlotIfAvailable (const juce::String& label)
{
    if (emitterSlotId < 0 || ! sceneGraph.isSlotActive (emitterSlotId))
        return;

    auto data = sceneGraph.getSlot (emitterSlotId).read();
    const auto sanitised = sanitiseEmitterLabel (label);
    std::snprintf (data.label, sizeof (data.label), "%s", sanitised.toRawUTF8());
    sceneGraph.getSlot (emitterSlotId).write (data);
}

juce::var LocusQAudioProcessor::buildEmitterPresetLocked (const juce::String& presetName,
                                                          const juce::String& presetType,
                                                          const juce::String& choreographyPackId,
                                                          bool includeParameters,
                                                          bool includeTimeline) const
{
    juce::var presetVar (new juce::DynamicObject());
    auto* preset = presetVar.getDynamicObject();

    preset->setProperty ("schema", kEmitterPresetSchemaV2);
    preset->setProperty ("name", presetName);
    preset->setProperty (kEmitterPresetTypeProperty, normalisePresetType (presetType));
    preset->setProperty ("savedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    preset->setProperty ("choreographyPackId", normaliseChoreographyPackId (choreographyPackId));

    juce::var layoutVar (new juce::DynamicObject());
    auto* layout = layoutVar.getDynamicObject();
    layout->setProperty ("outputLayout", getSnapshotOutputLayout());
    layout->setProperty ("outputChannels", getSnapshotOutputChannels());
    preset->setProperty (kEmitterPresetLayoutProperty, layoutVar);

    if (includeParameters)
    {
        juce::var parametersVar (new juce::DynamicObject());
        auto* parameters = parametersVar.getDynamicObject();
        for (const auto* parameterId : kEmitterPresetParameterIds)
        {
            if (auto* parameter = apvts.getParameter (parameterId))
                parameters->setProperty (parameterId, parameter->getValue());
        }

        preset->setProperty ("parameters", parametersVar);
    }

    if (includeTimeline)
        preset->setProperty ("timeline", serialiseKeyframeTimelineLocked());

    return presetVar;
}

bool LocusQAudioProcessor::applyEmitterPresetLocked (const juce::var& presetState)
{
    auto* preset = presetState.getDynamicObject();
    if (preset == nullptr)
        return false;

    if (preset->hasProperty ("schema"))
    {
        const auto schema = preset->getProperty ("schema").toString();
        if (schema.isNotEmpty()
            && schema != kEmitterPresetSchemaV1
            && schema != kEmitterPresetSchemaV2)
        {
            return false;
        }
    }

    if (auto* layout = preset->getProperty (kEmitterPresetLayoutProperty).getDynamicObject())
    {
        if (layout->hasProperty ("outputChannels"))
        {
            const auto parsedChannels = static_cast<int> (layout->getProperty ("outputChannels"));
            if (parsedChannels <= 0)
                return false;
        }

        if (layout->hasProperty ("outputLayout")
            && layout->getProperty ("outputLayout").toString().trim().isEmpty())
        {
            return false;
        }
    }

    if (auto* parameters = preset->getProperty ("parameters").getDynamicObject())
    {
        for (const auto* parameterId : kEmitterPresetParameterIds)
        {
            if (parameters->hasProperty (parameterId))
            {
                if (auto* parameter = apvts.getParameter (parameterId))
                {
                    const auto normalized = juce::jlimit (0.0f, 1.0f, static_cast<float> (double (parameters->getProperty (parameterId))));
                    parameter->setValueNotifyingHost (normalized);
                }
            }
        }
    }

    if (preset->hasProperty ("timeline"))
        applyKeyframeTimelineLocked (preset->getProperty ("timeline"));

    {
        const auto choreographyPack = preset->hasProperty ("choreographyPackId")
            ? normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString())
            : juce::String ("custom");
        const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
        choreographyPackState = choreographyPack;
    }

    return true;
}

juce::var LocusQAudioProcessor::buildCalibrationProfileState (const juce::String& profileName,
                                                              const juce::var& validationSummary) const
{
    juce::var profileVar (new juce::DynamicObject());
    auto* profile = profileVar.getDynamicObject();

    profile->setProperty ("schema", kCalibrationProfileSchemaV1);
    profile->setProperty ("name", profileName);
    profile->setProperty ("savedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));

    juce::var contextVar (new juce::DynamicObject());
    auto* context = contextVar.getDynamicObject();
    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPathIndex = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfileIndex = getCurrentCalibrationDeviceProfileIndex();
    context->setProperty ("topologyProfileIndex", topologyIndex);
    context->setProperty ("topologyProfile", calibrationTopologyIdForIndex (topologyIndex));
    context->setProperty ("monitoringPathIndex", monitoringPathIndex);
    context->setProperty ("monitoringPath", calibrationMonitoringPathIdForIndex (monitoringPathIndex));
    context->setProperty ("deviceProfileIndex", deviceProfileIndex);
    context->setProperty ("deviceProfile", calibrationDeviceProfileIdForIndex (deviceProfileIndex));
    context->setProperty ("requiredChannels", getRequiredCalibrationChannelsForTopologyIndex (topologyIndex));
    context->setProperty ("writableChannels", resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting()));
    profile->setProperty ("context", contextVar);

    juce::var controlsVar (new juce::DynamicObject());
    auto* controls = controlsVar.getDynamicObject();
    for (const auto* parameterId : kCalibrationProfileParameterIds)
    {
        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        {
            const auto scaledValue = parameter->convertFrom0to1 (parameter->getValue());
            controls->setProperty (parameterId, scaledValue);
        }
    }
    profile->setProperty ("controls", controlsVar);

    juce::var layoutVar (new juce::DynamicObject());
    auto* layout = layoutVar.getDynamicObject();
    layout->setProperty ("outputLayout", getSnapshotOutputLayout());
    layout->setProperty ("outputChannels", getSnapshotOutputChannels());
    profile->setProperty ("layout", layoutVar);

    if (! validationSummary.isVoid())
        profile->setProperty ("validationSummary", validationSummary);

    return profileVar;
}

bool LocusQAudioProcessor::applyCalibrationProfileState (const juce::var& profileState)
{
    auto* profile = profileState.getDynamicObject();
    if (profile == nullptr)
        return false;

    if (profile->hasProperty ("schema"))
    {
        const auto schema = profile->getProperty ("schema").toString().trim();
        if (schema.isNotEmpty() && schema != kCalibrationProfileSchemaV1)
            return false;
    }

    auto* controls = profile->getProperty ("controls").getDynamicObject();
    if (controls == nullptr)
        return false;

    for (const auto& property : controls->getProperties())
    {
        const auto parameterId = property.name.toString();
        if (parameterId.isEmpty())
            continue;

        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        {
            const auto scaledValue = static_cast<float> (double (property.value));
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (scaledValue));
        }
    }

    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    setIntegerParameterValueNotifyingHost ("cal_spk_config", legacySpeakerConfigForTopologyIndex (topologyIndex));
    setIntegerParameterValueNotifyingHost ("rend_headphone_mode", (monitoringPath == 2 || monitoringPath == 3) ? 1 : 0);
    setIntegerParameterValueNotifyingHost ("rend_headphone_profile", deviceProfile);

    int rendererSpatialProfileIndex = 0;
    switch (topologyIndex)
    {
        case 0: rendererSpatialProfileIndex = 1; break;
        case 1: rendererSpatialProfileIndex = 1; break;
        case 2: rendererSpatialProfileIndex = 2; break;
        case 3: rendererSpatialProfileIndex = 3; break;
        case 4: rendererSpatialProfileIndex = 4; break;
        case 5: rendererSpatialProfileIndex = 4; break;
        case 6: rendererSpatialProfileIndex = 5; break;
        case 7: rendererSpatialProfileIndex = 9; break;
        case 8: rendererSpatialProfileIndex = 6; break;
        case 9: rendererSpatialProfileIndex = 7; break;
        case 10: rendererSpatialProfileIndex = 9; break;
        default: break;
    }
    setIntegerParameterValueNotifyingHost ("rend_spatial_profile", rendererSpatialProfileIndex);

    return true;
}

juce::var LocusQAudioProcessor::listEmitterPresetsFromUI() const
{
    juce::Array<juce::var> presets;
    const auto presetDir = getPresetDirectory();
    if (! presetDir.exists())
        return juce::var (presets);

    juce::Array<juce::File> files;
    presetDir.findChildFiles (files, juce::File::findFiles, false, "*.json");

    for (const auto& file : files)
    {
        juce::var entryVar (new juce::DynamicObject());
        auto* entry = entryVar.getDynamicObject();

        juce::String displayName = file.getFileNameWithoutExtension();
        juce::String choreographyPackId = "custom";
        juce::String presetType = kEmitterPresetTypeEmitter;
        if (const auto payload = readJsonFromFile (file))
        {
            if (auto* preset = payload->getDynamicObject())
            {
                if (preset->hasProperty ("name"))
                    displayName = preset->getProperty ("name").toString();

                if (preset->hasProperty ("choreographyPackId"))
                    choreographyPackId = normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString());

                presetType = inferPresetTypeFromPayload (*payload);
            }
        }

        entry->setProperty ("name", displayName);
        entry->setProperty ("file", file.getFileName());
        entry->setProperty ("path", file.getFullPathName());
        entry->setProperty ("modifiedUtc", file.getLastModificationTime().toISO8601 (true));
        entry->setProperty ("choreographyPackId", choreographyPackId);
        entry->setProperty ("presetType", presetType);
        presets.add (entryVar);
    }

    return juce::var (presets);
}

juce::var LocusQAudioProcessor::saveEmitterPresetFromUI (const juce::var& options)
{
    juce::String requestedName = "Preset";
    juce::String presetType = kEmitterPresetTypeEmitter;
    juce::String choreographyPackId = "custom";
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
        if (optionsObject->hasProperty ("presetType"))
            presetType = optionsObject->getProperty ("presetType").toString();
        if (optionsObject->hasProperty ("choreographyPackId"))
            choreographyPackId = optionsObject->getProperty ("choreographyPackId").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
        requestedName = "Preset_" + juce::String (juce::Time::getCurrentTime().toMilliseconds());

    presetType = normalisePresetType (presetType);
    choreographyPackId = normaliseChoreographyPackId (choreographyPackId);
    const auto includeParameters = presetType == kEmitterPresetTypeEmitter;
    const auto includeTimeline = presetType == kEmitterPresetTypeMotion;

    const auto safeName = sanitisePresetName (requestedName);
    auto presetDir = getPresetDirectory();
    presetDir.createDirectory();
    const auto presetFile = presetDir.getChildFile (safeName + ".json");

    juce::var presetPayload;
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        presetPayload = buildEmitterPresetLocked (requestedName, presetType, choreographyPackId, includeParameters, includeTimeline);
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! writeJsonToFile (presetFile, presetPayload))
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Failed to write preset file.");
        return response;
    }

    result->setProperty (locusq::shared_contracts::bridge_status::kOk, true);
    result->setProperty (locusq::shared_contracts::bridge_status::kName, requestedName);
    result->setProperty (locusq::shared_contracts::bridge_status::kFile, presetFile.getFileName());
    result->setProperty (locusq::shared_contracts::bridge_status::kPath, presetFile.getFullPathName());
    result->setProperty ("choreographyPackId", choreographyPackId);
    result->setProperty ("presetType", presetType);
    return response;
}

juce::var LocusQAudioProcessor::loadEmitterPresetFromUI (const juce::var& options)
{
    const auto presetFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! presetFile.existsAsFile())
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (presetFile);
    if (! payload.has_value())
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset file is invalid JSON.");
        return response;
    }

    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        if (! applyEmitterPresetLocked (*payload))
        {
            result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
            result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset payload is not compatible.");
            return response;
        }
    }

    result->setProperty (locusq::shared_contracts::bridge_status::kOk, true);
    result->setProperty (locusq::shared_contracts::bridge_status::kName, presetFile.getFileNameWithoutExtension());
    result->setProperty (locusq::shared_contracts::bridge_status::kFile, presetFile.getFileName());
    result->setProperty (locusq::shared_contracts::bridge_status::kPath, presetFile.getFullPathName());
    result->setProperty ("presetType", inferPresetTypeFromPayload (*payload));
    if (auto* preset = payload->getDynamicObject(); preset != nullptr
        && preset->hasProperty ("choreographyPackId"))
    {
        result->setProperty ("choreographyPackId",
                             normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString()));
    }
    else
    {
        result->setProperty ("choreographyPackId", "custom");
    }
    return response;
}

juce::var LocusQAudioProcessor::renameEmitterPresetFromUI (const juce::var& options)
{
    const auto sourceFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file not found.");
        return response;
    }

    juce::String requestedName;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("newName"))
            requestedName = optionsObject->getProperty ("newName").toString();
        else if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset name is required.");
        return response;
    }

    const auto safeName = sanitisePresetName (requestedName);
    const auto destinationFile = getPresetDirectory().getChildFile (safeName + ".json");
    const auto samePath = destinationFile.getFullPathName() == sourceFile.getFullPathName();

    if (! samePath && destinationFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset name already exists.");
        return response;
    }

    const auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file is invalid JSON.");
        return response;
    }

    auto updatedPayload = *payload;
    if (auto* preset = updatedPayload.getDynamicObject(); preset != nullptr)
    {
        preset->setProperty ("name", requestedName);
        preset->setProperty ("updatedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    }

    if (! writeJsonToFile (destinationFile, updatedPayload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write preset file.");
        return response;
    }

    if (! samePath)
        sourceFile.deleteFile();

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", destinationFile.getFileName());
    result->setProperty ("path", destinationFile.getFullPathName());
    result->setProperty ("presetType", inferPresetTypeFromPayload (updatedPayload));
    if (auto* preset = updatedPayload.getDynamicObject(); preset != nullptr
        && preset->hasProperty ("choreographyPackId"))
    {
        result->setProperty ("choreographyPackId",
                             normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString()));
    }
    else
    {
        result->setProperty ("choreographyPackId", "custom");
    }
    return response;
}

juce::var LocusQAudioProcessor::deleteEmitterPresetFromUI (const juce::var& options)
{
    const auto presetFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! presetFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file not found.");
        return response;
    }

    if (! presetFile.deleteFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to delete preset file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("file", presetFile.getFileName());
    result->setProperty ("path", presetFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::listCalibrationProfilesFromUI() const
{
    juce::Array<juce::var> profiles;
    const auto profileDir = getCalibrationProfileDirectory();
    if (! profileDir.exists())
        return juce::var (profiles);

    juce::Array<juce::File> files;
    profileDir.findChildFiles (files, juce::File::findFiles, false, "*.json");
    std::sort (files.begin(), files.end(), [] (const juce::File& lhs, const juce::File& rhs)
    {
        return lhs.getLastModificationTime() > rhs.getLastModificationTime();
    });

    for (const auto& file : files)
    {
        juce::var entryVar (new juce::DynamicObject());
        auto* entry = entryVar.getDynamicObject();

        juce::String displayName = file.getFileNameWithoutExtension();
        juce::String topologyId = calibrationTopologyIdForIndex (1);
        juce::String monitoringPathId = calibrationMonitoringPathIdForIndex (0);
        juce::String deviceProfileId = calibrationDeviceProfileIdForIndex (0);
        juce::var validationSummary;

        if (const auto payload = readJsonFromFile (file))
        {
            if (auto* profile = payload->getDynamicObject())
            {
                if (profile->hasProperty ("name"))
                    displayName = profile->getProperty ("name").toString();

                if (auto* context = profile->getProperty ("context").getDynamicObject())
                {
                    if (context->hasProperty ("topologyProfile"))
                        topologyId = normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString());
                    if (context->hasProperty ("monitoringPath"))
                        monitoringPathId = normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString());
                    if (context->hasProperty ("deviceProfile"))
                        deviceProfileId = normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString());
                }

                if (profile->hasProperty ("validationSummary"))
                    validationSummary = profile->getProperty ("validationSummary");
            }
        }

        entry->setProperty ("name", displayName);
        entry->setProperty ("file", file.getFileName());
        entry->setProperty ("path", file.getFullPathName());
        entry->setProperty ("modifiedUtc", file.getLastModificationTime().toISO8601 (true));
        entry->setProperty ("topologyProfile", topologyId);
        entry->setProperty ("monitoringPath", monitoringPathId);
        entry->setProperty ("deviceProfile", deviceProfileId);
        entry->setProperty ("profileTupleKey", topologyId + "::" + monitoringPathId);
        if (! validationSummary.isVoid())
            entry->setProperty ("validationSummary", validationSummary);
        profiles.add (entryVar);
    }

    return juce::var (profiles);
}

juce::var LocusQAudioProcessor::saveCalibrationProfileFromUI (const juce::var& options)
{
    juce::String requestedName;
    juce::var validationSummary;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
        if (optionsObject->hasProperty ("validationSummary"))
            validationSummary = optionsObject->getProperty ("validationSummary");
    }

    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPathIndex = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfileIndex = getCurrentCalibrationDeviceProfileIndex();
    const auto topologyId = calibrationTopologyIdForIndex (topologyIndex);
    const auto monitoringPathId = calibrationMonitoringPathIdForIndex (monitoringPathIndex);
    const auto deviceProfileId = calibrationDeviceProfileIdForIndex (deviceProfileIndex);

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
        requestedName = topologyId + "_" + monitoringPathId + "_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");

    const auto safeName = sanitisePresetName (requestedName);
    auto profileDir = getCalibrationProfileDirectory();
    profileDir.createDirectory();
    const auto profileFile = profileDir.getChildFile (safeName + ".json");
    const auto payload = buildCalibrationProfileState (requestedName, validationSummary);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! writeJsonToFile (profileFile, payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write calibration profile file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    result->setProperty ("topologyProfile", topologyId);
    result->setProperty ("monitoringPath", monitoringPathId);
    result->setProperty ("deviceProfile", deviceProfileId);
    result->setProperty ("profileTupleKey", topologyId + "::" + monitoringPathId);
    if (! validationSummary.isVoid())
        result->setProperty ("validationSummary", validationSummary);
    return response;
}

juce::var LocusQAudioProcessor::loadCalibrationProfileFromUI (const juce::var& options)
{
    const auto profileFile = resolveCalibrationProfileFileFromOptions (options);
    bool enforceTupleMatch = false;
    juce::String expectedTopologyId;
    juce::String expectedMonitoringPathId;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("enforceTupleMatch"))
            enforceTupleMatch = static_cast<bool> (optionsObject->getProperty ("enforceTupleMatch"));
        if (optionsObject->hasProperty ("topologyProfile"))
            expectedTopologyId = optionsObject->getProperty ("topologyProfile").toString();
        else if (optionsObject->hasProperty ("topologyProfileIndex"))
            expectedTopologyId = calibrationTopologyIdForIndex (static_cast<int> (optionsObject->getProperty ("topologyProfileIndex")));

        if (optionsObject->hasProperty ("monitoringPath"))
            expectedMonitoringPathId = optionsObject->getProperty ("monitoringPath").toString();
        else if (optionsObject->hasProperty ("monitoringPathIndex"))
            expectedMonitoringPathId = calibrationMonitoringPathIdForIndex (static_cast<int> (optionsObject->getProperty ("monitoringPathIndex")));
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! profileFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (profileFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file is invalid JSON.");
        return response;
    }

    auto loadedTopologyId = calibrationTopologyIdForIndex (getCurrentCalibrationTopologyProfileIndex());
    auto loadedMonitoringPathId = calibrationMonitoringPathIdForIndex (getCurrentCalibrationMonitoringPathIndex());
    auto loadedDeviceProfileId = calibrationDeviceProfileIdForIndex (getCurrentCalibrationDeviceProfileIndex());
    if (auto* profile = payload->getDynamicObject())
    {
        if (auto* context = profile->getProperty ("context").getDynamicObject())
        {
            if (context->hasProperty ("topologyProfile"))
                loadedTopologyId = normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString());
            if (context->hasProperty ("monitoringPath"))
                loadedMonitoringPathId = normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString());
            if (context->hasProperty ("deviceProfile"))
                loadedDeviceProfileId = normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString());
        }
    }

    if (expectedTopologyId.isEmpty())
        expectedTopologyId = calibrationTopologyIdForIndex (getCurrentCalibrationTopologyProfileIndex());
    if (expectedMonitoringPathId.isEmpty())
        expectedMonitoringPathId = calibrationMonitoringPathIdForIndex (getCurrentCalibrationMonitoringPathIndex());
    expectedTopologyId = normaliseCalibrationTopologyId (expectedTopologyId);
    expectedMonitoringPathId = normaliseCalibrationMonitoringPathId (expectedMonitoringPathId);

    if (enforceTupleMatch
        && (loadedTopologyId != expectedTopologyId
            || loadedMonitoringPathId != expectedMonitoringPathId))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message",
                             "Calibration profile tuple mismatch (profile="
                                 + loadedTopologyId + "/"
                                 + loadedMonitoringPathId + ", current="
                                 + expectedTopologyId + "/"
                                 + expectedMonitoringPathId + ").");
        return response;
    }

    if (! applyCalibrationProfileState (*payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile payload is not compatible.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("name", profileFile.getFileNameWithoutExtension());
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    result->setProperty ("topologyProfile", loadedTopologyId);
    result->setProperty ("monitoringPath", loadedMonitoringPathId);
    result->setProperty ("deviceProfile", loadedDeviceProfileId);
    result->setProperty ("profileTupleKey", loadedTopologyId + "::" + loadedMonitoringPathId);
    if (auto* profile = payload->getDynamicObject())
    {
        if (profile->hasProperty ("name"))
            result->setProperty ("name", profile->getProperty ("name").toString());

        if (auto* context = profile->getProperty ("context").getDynamicObject())
        {
            if (context->hasProperty ("topologyProfile"))
                result->setProperty ("topologyProfile", normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString()));
            if (context->hasProperty ("monitoringPath"))
                result->setProperty ("monitoringPath", normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString()));
            if (context->hasProperty ("deviceProfile"))
                result->setProperty ("deviceProfile", normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString()));
        }

        if (profile->hasProperty ("validationSummary"))
            result->setProperty ("validationSummary", profile->getProperty ("validationSummary"));
    }

    return response;
}

juce::var LocusQAudioProcessor::renameCalibrationProfileFromUI (const juce::var& options)
{
    const auto sourceFile = resolveCalibrationProfileFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    juce::String requestedName;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("newName"))
            requestedName = optionsObject->getProperty ("newName").toString();
        else if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile name is required.");
        return response;
    }

    const auto safeName = sanitisePresetName (requestedName);
    const auto destinationFile = getCalibrationProfileDirectory().getChildFile (safeName + ".json");
    const auto samePath = destinationFile.getFullPathName() == sourceFile.getFullPathName();

    if (! samePath && destinationFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile name already exists.");
        return response;
    }

    const auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file is invalid JSON.");
        return response;
    }

    auto updatedPayload = *payload;
    if (auto* profile = updatedPayload.getDynamicObject(); profile != nullptr)
    {
        profile->setProperty ("name", requestedName);
        profile->setProperty ("updatedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    }

    if (! writeJsonToFile (destinationFile, updatedPayload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write calibration profile file.");
        return response;
    }

    if (! samePath)
        sourceFile.deleteFile();

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", destinationFile.getFileName());
    result->setProperty ("path", destinationFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::deleteCalibrationProfileFromUI (const juce::var& options)
{
    const auto profileFile = resolveCalibrationProfileFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! profileFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    if (! profileFile.deleteFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to delete calibration profile file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::getUIStateFromUI() const
{
    juce::var stateVar (new juce::DynamicObject());
    auto* state = stateVar.getDynamicObject();

    juce::String emitterLabelSnapshot;
    juce::String physicsPresetSnapshot;
    juce::String choreographyPackSnapshot;
    {
        const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
        emitterLabelSnapshot = emitterLabelState;
        physicsPresetSnapshot = physicsPresetState;
        choreographyPackSnapshot = choreographyPackState;
    }

    if (emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId))
    {
        const auto slotData = sceneGraph.getSlot (emitterSlotId).read();
        const auto slotLabel = juce::String::fromUTF8 (slotData.label).trim();
        if (slotLabel.isNotEmpty())
            emitterLabelSnapshot = slotLabel;
    }

    if (physicsPresetSnapshot.isEmpty())
        physicsPresetSnapshot = "off";
    if (choreographyPackSnapshot.isEmpty())
        choreographyPackSnapshot = "custom";

    state->setProperty ("emitterLabel", sanitiseEmitterLabel (emitterLabelSnapshot));
    state->setProperty ("physicsPreset", physicsPresetSnapshot);
    state->setProperty ("choreographyPack", normaliseChoreographyPackId (choreographyPackSnapshot));
    return stateVar;
}

bool LocusQAudioProcessor::setUIStateFromUI (const juce::var& stateVar)
{
    auto* state = stateVar.getDynamicObject();
    if (state == nullptr)
        return false;

    bool changed = false;

    if (state->hasProperty ("emitterLabel"))
    {
        const auto nextLabel = sanitiseEmitterLabel (state->getProperty ("emitterLabel").toString());
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            emitterLabelState = nextLabel;
        }
        applyEmitterLabelToSceneSlotIfAvailable (nextLabel);
        changed = true;
    }

    if (state->hasProperty ("physicsPreset"))
    {
        auto preset = state->getProperty ("physicsPreset").toString().trim().toLowerCase();
        if (preset != "off" && preset != "bounce" && preset != "float" && preset != "orbit")
            preset = "custom";

        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            physicsPresetState = preset;
        }

        changed = true;
    }

    if (state->hasProperty ("choreographyPack"))
    {
        const auto choreographyPack = normaliseChoreographyPackId (state->getProperty ("choreographyPack").toString());
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            choreographyPackState = choreographyPack;
        }
        changed = true;
    }

    return changed;
}

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
    return new LocusQAudioProcessorEditor (*this);
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
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Calibrate", "Emitter", "Renderer" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    // ==================== CALIBRATE ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_spk_config", 1 }, "Speaker Config",
        juce::StringArray { "4x Mono", "2x Stereo" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
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

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_monitoring_path", 1 }, "Monitoring Path",
        juce::StringArray {
            "Speakers",
            "Stereo Downmix",
            "Steam Binaural",
            "Virtual Binaural"
        }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_device_profile", 1 }, "Device Profile",
        juce::StringArray {
            "Generic",
            "AirPods Pro 2",
            "Sony WH-1000XM5",
            "Custom SOFA"
        }, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_mic_channel", 1 }, "Mic Channel", 1, 8, 1));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk1_out", 1 }, "SPK1 Output", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk2_out", 1 }, "SPK2 Output", 1, 8, 2));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk3_out", 1 }, "SPK3 Output", 1, 8, 3));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk4_out", 1 }, "SPK4 Output", 1, 8, 4));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cal_test_level", 1 }, "Test Level",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_test_type", 1 }, "Test Type",
        juce::StringArray { "Sweep", "Pink", "White", "Impulse" }, 0));

    // ==================== EMITTER: POSITION ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_azimuth", 1 }, "Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_elevation", 1 }, "Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_distance", 1 }, "Distance",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f, 0.5f), 2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_x", 1 }, "Position X",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_y", 1 }, "Position Y",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_z", 1 }, "Position Z",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pos_coord_mode", 1 }, "Coord Mode",
        juce::StringArray { "Spherical", "Cartesian" }, 0));

    // ==================== EMITTER: SIZE ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_width", 1 }, "Width",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_height", 1 }, "Height",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "size_link", 1 }, "Link Size", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_uniform", 1 }, "Uniform Scale",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    // ==================== EMITTER: AUDIO ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_gain", 1 }, "Emitter Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_mute", 1 }, "Mute", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_solo", 1 }, "Solo", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_spread", 1 }, "Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_directivity", 1 }, "Directivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_azimuth", 1 }, "Dir Aim Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_elevation", 1 }, "Dir Aim Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    // ==================== EMITTER: PHYSICS ====================
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_enable", 1 }, "Physics Enable", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_mass", 1 }, "Mass",
        juce::NormalisableRange<float> (0.01f, 100.0f, 0.01f, 0.4f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_drag", 1 }, "Drag",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_elasticity", 1 }, "Elasticity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_gravity", 1 }, "Gravity",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_gravity_dir", 1 }, "Gravity Direction",
        juce::StringArray { "Down", "Up", "To Center", "From Center", "Custom" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_friction", 1 }, "Friction",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_x", 1 }, "Init Vel X",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_y", 1 }, "Init Vel Y",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_z", 1 }, "Init Vel Z",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_throw", 1 }, "Throw", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_reset", 1 }, "Reset Position", false));

    // ==================== EMITTER: ANIMATION ====================
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_enable", 1 }, "Animation Enable", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "anim_mode", 1 }, "Animation Source",
        juce::StringArray { "DAW", "Internal" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_loop", 1 }, "Loop", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "anim_speed", 1 }, "Animation Speed",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.1f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_sync", 1 }, "Transport Sync", true));

    // ==================== EMITTER: IDENTITY ====================
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "emit_color", 1 }, "Color", 0, 15, 0));

    // ==================== RENDERER: MASTER ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_master_gain", 1 }, "Master Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_gain", 1 }, "SPK1 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_gain", 1 }, "SPK2 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_gain", 1 }, "SPK3 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_gain", 1 }, "SPK4 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_delay", 1 }, "SPK1 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_delay", 1 }, "SPK2 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_delay", 1 }, "SPK3 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_delay", 1 }, "SPK4 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));

    // ==================== RENDERER: SPATIALIZATION ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_quality", 1 }, "Quality",
        juce::StringArray { "Draft", "Final" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_distance_model", 1 }, "Distance Model",
        juce::StringArray { "Inverse Square", "Linear", "Logarithmic", "Custom" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_mode", 1 }, "Headphone Mode",
        juce::StringArray { "Stereo Downmix", "Steam Binaural" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_profile", 1 }, "Headphone Profile",
        juce::StringArray { "Generic", "AirPods Pro 2", "Sony WH-1000XM5", "Custom SOFA" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_audition_enable", 1 }, "Audition Enable", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
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

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_motion", 1 }, "Audition Motion",
        juce::StringArray {
            "Center",
            "Orbit Slow",
            "Orbit Fast",
            "Figure8 Flow",
            "Helix Rise",
            "Wall Ricochet"
        }, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_level", 1 }, "Audition Level",
        juce::StringArray { "-36 dBFS", "-30 dBFS", "-24 dBFS", "-18 dBFS", "-12 dBFS" }, 2));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_ref", 1 }, "Ref Distance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_max", 1 }, "Max Distance",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_doppler", 1 }, "Doppler", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_doppler_scale", 1 }, "Doppler Scale",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_air_absorb", 1 }, "Air Absorption", true));

    // ==================== RENDERER: ROOM ====================
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_enable", 1 }, "Room Enable", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_mix", 1 }, "Room Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_size", 1 }, "Room Size",
        juce::NormalisableRange<float> (0.5f, 5.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_damping", 1 }, "Room Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_er_only", 1 }, "ER Only", false));

    // ==================== RENDERER: PHYSICS GLOBAL ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_phys_rate", 1 }, "Physics Rate",
        juce::StringArray { "30 Hz", "60 Hz", "120 Hz", "240 Hz" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_walls", 1 }, "Wall Collision", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_interact", 1 }, "Object Interaction", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_pause", 1 }, "Pause Physics", false));

    // ==================== RENDERER: VISUALIZATION ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_viz_mode", 1 }, "View Mode",
        juce::StringArray { "Perspective", "Top Down", "Front", "Side" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_trails", 1 }, "Show Trails", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_trail_len", 1 }, "Trail Length",
        juce::NormalisableRange<float> (0.5f, 30.0f, 0.1f), 5.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_vectors", 1 }, "Show Vectors", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_physics_lens", 1 }, "Physics Lens", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_diag_mix", 1 }, "Diagnostic Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.55f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_grid", 1 }, "Show Grid", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_labels", 1 }, "Show Labels", true));

    return { params.begin(), params.end() };
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LocusQAudioProcessor();
}
