#include "PluginProcessor.h"
#include "processor_core/ProcessorConstants.h"
#include "processor_core/ProcessorParameterReaders.h"
#include "processor_bridge/ProcessorBridgeUtilities.h"
#include "shared_contracts/BridgeStatusContract.h"
#include "shared_contracts/CalibrationRegistry.h"
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

constexpr int kPhysicsAttractorSourceCount = 4;

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

bool isHeadTrackingPoseFresh (const HeadTrackingBridge::PoseSnapshot& pose,
                              std::uint64_t nowEpochMs) noexcept
{
    return pose.timestampMs > 0
           && nowEpochMs >= pose.timestampMs
           && static_cast<double> (nowEpochMs - pose.timestampMs) <= kRendererHeadTrackingStaleMs;
}

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

void applyYawOffsetToPose (SpatialRenderer::PoseSnapshot& pose, float yawOffsetDeg) noexcept
{
    if (! std::isfinite (yawOffsetDeg))
        return;

    const float refRad = yawOffsetDeg * (juce::MathConstants<float>::pi / 180.0f);
    const float halfRef = refRad * 0.5f;
    // q_ref = rotation about Z by -refDeg = (0, 0, -sin(halfRef), cos(halfRef))
    const float qrz = -std::sin (halfRef);
    const float qrw =  std::cos (halfRef);
    // q_eff = q_ref * pose  (quaternion product; q_ref.x = q_ref.y = 0)
    const float bx = pose.qx;
    const float by = pose.qy;
    const float bz = pose.qz;
    const float bw = pose.qw;
    pose.qx = qrw * bx - qrz * by;
    pose.qy = qrz * bx + qrw * by;
    pose.qz = qrw * bz + qrz * bw;
    pose.qw = qrw * bw - qrz * bz;
}

bool tryBuildFreshInterpolatedHeadPose (const HeadTrackingBridge::PoseSnapshot* pose,
                                        HeadPoseInterpolator& interpolator,
                                        SpatialRenderer::PoseSnapshot& rendererPose,
                                        float* rawYawDeg = nullptr) noexcept
{
    if (pose == nullptr)
        return false;

    const auto nowEpochMs = static_cast<std::uint64_t> (juce::Time::currentTimeMillis());
    if (! isHeadTrackingPoseFresh (*pose, nowEpochMs))
        return false;

    const double nowMs = static_cast<double> (nowEpochMs);
    interpolator.ingest (*pose, nowMs);
    const auto interpolated = interpolator.interpolatedAt (nowMs);

    if (rawYawDeg != nullptr)
    {
        float pitchDeg = 0.0f;
        float rollDeg = 0.0f;
        computeHeadTrackingEulerDegrees (interpolated, *rawYawDeg, pitchDeg, rollDeg);
    }

    rendererPose.qx = interpolated.qx;
    rendererPose.qy = interpolated.qy;
    rendererPose.qz = interpolated.qz;
    rendererPose.qw = interpolated.qw;
    rendererPose.timestampMs = interpolated.timestampMs;
    rendererPose.seq = interpolated.seq;
    rendererPose.angVx = interpolated.angVx;
    rendererPose.angVy = interpolated.angVy;
    rendererPose.angVz = interpolated.angVz;
    rendererPose.sensorLocationFlags = interpolated.sensorLocationFlags;
    rendererPose.pad = interpolated.pad;
    return true;
}

bool poseSnapshotToCoordinateSpace (const SpatialRenderer::PoseSnapshot& pose,
                                    IPLCoordinateSpace3& coordinateSpace) noexcept
{
    const float normSq = (pose.qx * pose.qx)
                       + (pose.qy * pose.qy)
                       + (pose.qz * pose.qz)
                       + (pose.qw * pose.qw);
    if (! std::isfinite (normSq) || normSq <= 1.0e-12f)
        return false;

    const float invNorm = 1.0f / std::sqrt (normSq);
    const float x = pose.qx * invNorm;
    const float y = pose.qy * invNorm;
    const float z = pose.qz * invNorm;
    const float w = pose.qw * invNorm;

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;
    const float xw = x * w;
    const float yw = y * w;
    const float zw = z * w;

    coordinateSpace.right = { 1.0f - 2.0f * (yy + zz),
                              2.0f * (xy + zw),
                              2.0f * (xz - yw) };
    coordinateSpace.up = { 2.0f * (xy - zw),
                           1.0f - 2.0f * (xx + zz),
                           2.0f * (yz + xw) };
    coordinateSpace.ahead = { -(2.0f * (xz + yw)),
                              -(2.0f * (yz - xw)),
                              -(1.0f - 2.0f * (xx + yy)) };
    coordinateSpace.origin = { 0.0f, 0.0f, 0.0f };

    return std::isfinite (coordinateSpace.right.x)
           && std::isfinite (coordinateSpace.right.y)
           && std::isfinite (coordinateSpace.right.z)
           && std::isfinite (coordinateSpace.up.x)
           && std::isfinite (coordinateSpace.up.y)
           && std::isfinite (coordinateSpace.up.z)
           && std::isfinite (coordinateSpace.ahead.x)
           && std::isfinite (coordinateSpace.ahead.y)
           && std::isfinite (coordinateSpace.ahead.z);
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

// Snapshot constants moved to processor_core/ProcessorConstants.h (W0-A).
using namespace locusq::constants;
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

constexpr std::array<const char*, 7> kCurveNames
{
    "linear",
    "easeIn",
    "easeOut",
    "easeInOut",
    "step",
    "glide",
    "teleport"
};

constexpr std::array<const char*, 4> kChoreographyPackIds
{
    "orbit",
    "pendulum",
    "swarm_arc",
    "rise_fall"
};

constexpr auto& kCalibrationTopologyIds = locusq::shared_contracts::calibration_registry::kTopologyIds;
constexpr auto& kCalibrationTopologyRequiredChannels = locusq::shared_contracts::calibration_registry::kTopologyRequiredChannels;
constexpr auto& kCalibrationMonitoringPathIds = locusq::shared_contracts::calibration_registry::kMonitoringPathIds;
constexpr auto& kCalibrationDeviceProfileIds = locusq::shared_contracts::calibration_registry::kDeviceProfileIds;

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
    juce::String scoreProvenance {
        locusq::shared_contracts::headphone_verification::provenance::kUnavailable
    };
    juce::String compensationLabel { "Generic baseline compensation" };
    juce::String compensationProvenance {
        locusq::shared_contracts::headphone_verification::provenance::kGeneric
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

    // BL-099: the current renderer-side scores are policy placeholders, not measured
    // perceptual evidence. Keep stage/fallback telemetry, but do not publish the
    // synthetic values as operator-facing verification evidence.
    snapshot.frontBackScore = 0.0f;
    snapshot.elevationScore = 0.0f;
    snapshot.externalizationScore = 0.0f;
    snapshot.confidence = 0.0f;
    snapshot.chainLatencySamples = sanitizeLatencySamples (snapshot.chainLatencySamples);
    snapshot.verificationStage = sanitizeVerificationStage (snapshot.verificationStage);
    snapshot.fallbackReasonCode = sanitizeFallbackReasonCode (snapshot.fallbackReasonCode);
    snapshot.fallbackTarget = sanitizeFallbackTargetForReason (
        snapshot.fallbackReasonCode,
        snapshot.fallbackTarget,
        snapshot.activeEngineId);
    snapshot.fallbackReasonText = fallbackReasonTextForCode (snapshot.fallbackReasonCode);

    snapshot.scoreProvenance = provenance::kUnavailable;
    snapshot.compensationLabel = "Generic baseline compensation";
    snapshot.compensationProvenance = provenance::kGeneric;
    snapshot.verificationScoreStatus = scoreStatusFromProvenance (snapshot.scoreProvenance);

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

float computeMonoBrightness (const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 1)
        return 0.0f;

    double sumAbs = 0.0;
    double sumDeltaAbs = 0.0;
    auto previous = static_cast<double> (samples[0]);
    sumAbs += std::abs (previous);

    for (int i = 1; i < numSamples; ++i)
    {
        const auto current = static_cast<double> (samples[i]);
        sumAbs += std::abs (current);
        sumDeltaAbs += std::abs (current - previous);
        previous = current;
    }

    const auto denom = juce::jmax (1.0e-9, sumAbs);
    const auto normalized = juce::jlimit (0.0, 1.0, (sumDeltaAbs / denom) * 0.5);
    return static_cast<float> (normalized);
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
      sceneGraph (SceneGraph::getInstance()),
      physicsSharedRuntime (PhysicsSharedRuntime::getInstance()),
      physicsDspBridge (physicsSharedRuntime.getDspBridge()),
      physicsWorker (physicsSharedRuntime.getWorker())
{
    sceneGraphAudioReservationId = sceneGraph.claimAudioReservation();
    initialiseDefaultKeyframeTimeline (keyframeTimelineState);
    publishKeyframeTimelinePlaybackState (keyframeTimelineState);

    for (int i = 0; i < kPhysicsDAWSlotCount; ++i)
    {
        physGainModParams[i]   = apvts.getRawParameterValue ("phys_out_gain_mod_"   + juce::String (i));
        physSpreadModParams[i] = apvts.getRawParameterValue ("phys_out_spread_mod_" + juce::String (i));
        physTransientParams[i] = apvts.getRawParameterValue ("phys_out_transient_"  + juce::String (i));
        physFrozenParams[i]    = apvts.getRawParameterValue ("phys_frozen_"         + juce::String (i));
        physSpreadNotifyTargets[i] = dynamic_cast<juce::RangedAudioParameter*> (
            apvts.getParameter ("phys_out_spread_mod_" + juce::String (i)));
        physTransientNotifyTargets[i] = dynamic_cast<juce::RangedAudioParameter*> (
            apvts.getParameter ("phys_out_transient_" + juce::String (i)));
        physSpreadHostPending[i].store (0.0f, std::memory_order_relaxed);
        physSpreadHostPublished[i].store (0.0f, std::memory_order_relaxed);
        physSpreadHostDirty[i].store (false, std::memory_order_relaxed);
        physTransientHostPending[i].store (0.0f, std::memory_order_relaxed);
        physTransientHostPublished[i].store (0.0f, std::memory_order_relaxed);
        physTransientHostDirty[i].store (false, std::memory_order_relaxed);
        physTransientHostMirrorState[i] = 0.0f;
    }

    physDebugActiveSlotParam = apvts.getRawParameterValue ("phys_dbg_active_slot");
    physDebugActiveEmittersParam = apvts.getRawParameterValue ("phys_dbg_active_emitters");
    physDebugCoordinatedWorkerParam = apvts.getRawParameterValue ("phys_dbg_coordinated_worker");
    physDebugBoidsDensityParam = apvts.getRawParameterValue ("phys_dbg_boids_density");
    physDebugWorkerSlotActiveParam = apvts.getRawParameterValue ("phys_dbg_worker_slot_active");
    physDebugWorkerBoidsActiveParam = apvts.getRawParameterValue ("phys_dbg_worker_boids_active");
    physDebugBoidsGroupSizeParam = apvts.getRawParameterValue ("phys_dbg_boids_group_size");
    physDebugWorkerPosXParam = apvts.getRawParameterValue ("phys_dbg_worker_pos_x");
    physDebugWorkerPosYParam = apvts.getRawParameterValue ("phys_dbg_worker_pos_y");
    physDebugWorkerPosZParam = apvts.getRawParameterValue ("phys_dbg_worker_pos_z");
    physDebugAlignNeighborsParam = apvts.getRawParameterValue ("phys_dbg_align_neighbors");
    physDebugCohNeighborsParam = apvts.getRawParameterValue ("phys_dbg_coh_neighbors");
    physDebugActiveSlotNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_active_slot"));
    physDebugActiveEmittersNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_active_emitters"));
    physDebugCoordinatedWorkerNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_coordinated_worker"));
    physDebugBoidsDensityNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_boids_density"));
    physDebugWorkerSlotActiveNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_worker_slot_active"));
    physDebugWorkerBoidsActiveNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_worker_boids_active"));
    physDebugBoidsGroupSizeNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_boids_group_size"));
    physDebugWorkerPosXNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_worker_pos_x"));
    physDebugWorkerPosYNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_worker_pos_y"));
    physDebugWorkerPosZNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_worker_pos_z"));
    physDebugAlignNeighborsNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_align_neighbors"));
    physDebugCohNeighborsNotifyTarget = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("phys_dbg_coh_neighbors"));

    auto rawParam = [this] (const char* paramId) -> std::atomic<float>*
    {
        return apvts.getRawParameterValue (paramId);
    };

    rendPhysRateParam = rawParam ("rend_phys_rate");
    rendPhysPauseParam = rawParam ("rend_phys_pause");
    rendPhysWallsParam = rawParam ("rend_phys_walls");
    rendPhysInteractParam = rawParam ("rend_phys_interact");
    posCoordModeParam = rawParam ("pos_coord_mode");
    posAzimuthParam = rawParam ("pos_azimuth");
    posElevationParam = rawParam ("pos_elevation");
    posDistanceParam = rawParam ("pos_distance");
    posXParam = rawParam ("pos_x");
    posYParam = rawParam ("pos_y");
    posZParam = rawParam ("pos_z");
    sizeUniformParam = rawParam ("size_uniform");
    sizeLinkParam = rawParam ("size_link");
    sizeWidthParam = rawParam ("size_width");
    sizeHeightParam = rawParam ("size_height");
    sizeDepthParam = rawParam ("size_depth");
    animEnableParam = rawParam ("anim_enable");
    animModeParam = rawParam ("anim_mode");
    animLoopParam = rawParam ("anim_loop");
    animSpeedParam = rawParam ("anim_speed");
    animSyncParam = rawParam ("anim_sync");
    choroEnableParam            = rawParam ("choro_enable");                 // CL-P1
    // CL-P2: formation params
    choroFormationTypeParam     = rawParam ("choro_formation_type");
    choroFormAxisParam          = rawParam ("choro_form_axis");
    choroFormPlaneParam         = rawParam ("choro_form_plane");
    choroFormRadiusParam        = rawParam ("choro_form_radius");
    choroFormSpacingParam       = rawParam ("choro_form_spacing");
    choroFormArcAngleParam      = rawParam ("choro_form_arc_angle");
    choroFormPhaseOffsetParam   = rawParam ("choro_form_phase_offset");
    choroFormRowsParam          = rawParam ("choro_form_rows");
    choroFormColsParam          = rawParam ("choro_form_cols");
    choroFormSpacingXParam      = rawParam ("choro_form_spacing_x");
    choroFormSpacingZParam      = rawParam ("choro_form_spacing_z");
    choroFormTurnsParam         = rawParam ("choro_form_turns");
    choroFormHeightRiseParam    = rawParam ("choro_form_height_rise");
    choroFormMorphRateParam     = rawParam ("choro_formation_morph_rate");
    choroFormMorphLoopParam     = rawParam ("choro_formation_morph_loop");
    choroFormMorphPingpongParam = rawParam ("choro_formation_morph_pingpong");

    // CL-P3: Path system raw param inits
    choroPathTypeParam        = rawParam ("choro_path_type");
    choroPathPeriodParam      = rawParam ("choro_path_period");
    choroPathSpeedParam       = rawParam ("choro_path_speed");
    choroPathLissFreqAParam   = rawParam ("choro_path_liss_freq_a");
    choroPathLissFreqBParam   = rawParam ("choro_path_liss_freq_b");
    choroPathLissFreqCParam   = rawParam ("choro_path_liss_freq_c");
    choroPathLissAmpXParam    = rawParam ("choro_path_liss_amp_x");
    choroPathLissAmpYParam    = rawParam ("choro_path_liss_amp_y");
    choroPathLissAmpZParam    = rawParam ("choro_path_liss_amp_z");
    choroPathLissPhaseParam   = rawParam ("choro_path_liss_phase");
    choroPathOrbitRxParam     = rawParam ("choro_path_orbit_rx");
    choroPathOrbitRzParam     = rawParam ("choro_path_orbit_rz");
    choroPathOrbitHeightParam = rawParam ("choro_path_orbit_height");
    choroPathPendLengthParam  = rawParam ("choro_path_pend_length");
    choroPathPendAmpParam     = rawParam ("choro_path_pend_amp");
    choroPathPendPlaneParam   = rawParam ("choro_path_pend_plane");
    choroPathFig8ScaleParam   = rawParam ("choro_path_fig8_scale");
    choroPathFig8PlaneParam   = rawParam ("choro_path_fig8_plane");
    choroPathHelixRadiusParam = rawParam ("choro_path_helix_radius");
    choroPathHelixPitchParam  = rawParam ("choro_path_helix_pitch");
    choroPathHelixDirParam    = rawParam ("choro_path_helix_dir");
    choroPathWalkStepParam    = rawParam ("choro_path_walk_step");
    choroPathWalkBoundsParam  = rawParam ("choro_path_walk_bounds");
    choroPathWalkSeedParam    = rawParam ("choro_path_walk_seed");

    // CL-P4: Beat-sync raw param inits
    choroBeatEnableParam      = rawParam ("choro_beat_enable");
    choroBeatDivisionParam    = rawParam ("choro_beat_division");
    choroBeatModeParam        = rawParam ("choro_beat_mode");
    choroTeleportDipDbParam   = rawParam ("choro_teleport_dip_db");
    choroTeleportDecayMsParam = rawParam ("choro_teleport_decay_ms");

    emitGainParam = rawParam ("emit_gain");
    emitSpreadParam = rawParam ("emit_spread");
    emitDirectivityParam = rawParam ("emit_directivity");
    emitMuteParam = rawParam ("emit_mute");
    emitSoloParam = rawParam ("emit_solo");
    emitDirAzimuthParam = rawParam ("emit_dir_azimuth");
    emitDirElevationParam = rawParam ("emit_dir_elevation");
    physEnableParam = rawParam ("phys_enable");
    physBoundaryModeParam = rawParam ("phys_boundary_mode");
    physSoftBoundaryDepthParam = rawParam ("phys_soft_boundary_depth");
    physFlockGroupParam = rawParam ("phys_flock_group");
    physSpringEnableParam = rawParam ("phys_spring_enable");
    physSpringKParam = rawParam ("phys_spring_k");
    physSpringDampParam = rawParam ("phys_spring_damp");
    physSpringAnchorModeParam = rawParam ("phys_spring_anchor_mode");
    physSpringAnchorXParam = rawParam ("phys_spring_anchor_x");
    physSpringAnchorYParam = rawParam ("phys_spring_anchor_y");
    physSpringAnchorZParam = rawParam ("phys_spring_anchor_z");
    physTurbulenceParam = rawParam ("phys_turbulence");
    physTurbulenceRateParam = rawParam ("phys_turbulence_rate");
    physAngEnableParam = rawParam ("phys_ang_enable");
    physAngDragParam = rawParam ("phys_ang_drag");
    physAngImpulseXParam = rawParam ("phys_ang_impulse_x");
    physAngImpulseYParam = rawParam ("phys_ang_impulse_y");
    physAngImpulseZParam = rawParam ("phys_ang_impulse_z");
    physAngAttractorTorqueParam = rawParam ("phys_ang_attractor_torque");
    physAngThrowParam = rawParam ("phys_ang_throw");
    physAngResetParam = rawParam ("phys_ang_reset");
    physMassOverrideParam = rawParam ("phys_mass_override");
    physCollideEmittersParam = rawParam ("phys_collide_emitters");
    physCollisionRadiusParam = rawParam ("phys_collision_radius");
    physCollisionGainScaleParam = rawParam ("phys_collision_gain_scale");
    physCollisionDecayMsParam = rawParam ("phys_collision_decay_ms");
    physMassParam = rawParam ("phys_mass");
    physDragParam = rawParam ("phys_drag");
    physElasticityParam = rawParam ("phys_elasticity");
    physFrictionParam = rawParam ("phys_friction");
    physGravityParam = rawParam ("phys_gravity");
    physGravityDirParam = rawParam ("phys_gravity_dir");
    physThrowParam = rawParam ("phys_throw");
    physVelXParam = rawParam ("phys_vel_x");
    physVelYParam = rawParam ("phys_vel_y");
    physVelZParam = rawParam ("phys_vel_z");
    physResetParam = rawParam ("phys_reset");

    for (int i = 0; i < kAttractorSlotCount; ++i)
    {
        const auto indexText = juce::String (i);
        attractorActiveParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_active");
        attractorPosXParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_pos_x");
        attractorPosYParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_pos_y");
        attractorPosZParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_pos_z");
        attractorStrengthParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_strength");
        attractorRadiusParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_radius");
        attractorFalloffParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_falloff");
        attractorOrbitStabilizeParams[i] = apvts.getRawParameterValue ("attractor_" + indexText + "_orbit_stabilize");
    }

    for (int i = 0; i < kFlockGroupCount; ++i)
    {
        const auto indexText = juce::String (i);
        flockEnableParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_enable");
        flockSepWeightParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_sep_weight");
        flockAlignWeightParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_align_weight");
        flockCohWeightParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_coh_weight");
        flockSepRadiusParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_sep_radius");
        flockAlignRadiusParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_align_radius");
        flockCohRadiusParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_coh_radius");
        flockMaxSpeedParams[i] = apvts.getRawParameterValue ("phys_flock_" + indexText + "_max_speed");
    }

    // Register with scene graph based on initial mode
    // Mode registration happens in prepareToPlay once we know the context
}

LocusQAudioProcessor::~LocusQAudioProcessor()
{
    headTrackingBridge.stop();

    // Unregister from scene graph
    if (emitterSlotId >= 0)
    {
        physicsWorker.unregisterEngine (emitterSlotId);
        physicsWorker.deactivateSlot (emitterSlotId);
        physicsDspBridge.publishZero (emitterSlotId);
        sceneGraph.unregisterEmitter (emitterSlotId);
    }

    if (rendererRegistered)
        sceneGraph.unregisterRenderer();

    sceneGraph.releaseAudioReservation (sceneGraphAudioReservationId);
}

//==============================================================================
//==============================================================================
void LocusQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    sceneGraph.ensureAudioReservationCapacity (sceneGraphAudioReservationId, samplesPerBlock);
    sceneGraph.clearAudioReservation (sceneGraphAudioReservationId);
    visualTokenScheduler.reset();
    modeTransitionInputSnapshotBuffer.setSize (kModeTransitionScratchChannels, samplesPerBlock, false, false, true);
    modeTransitionRendererScratchBuffer.setSize (kModeTransitionScratchChannels, samplesPerBlock, false, false, true);
    modeTransitionInputSnapshotBuffer.clear();
    modeTransitionRendererScratchBuffer.clear();
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        keyframeTimelineState.prepare (sampleRate);
        initialiseDefaultKeyframeTimeline (keyframeTimelineState);
        publishKeyframeTimelineStateToRtLocked();
    }

    // Prepare physics engine (Phase 2.4)
    physicsEngine.prepare (sampleRate);
    if (! physicsSharedRuntimeAcquired)
    {
        physicsSharedRuntime.acquire (sampleRate, sceneGraph.getPhysicsRateIndex());
        physicsSharedRuntimeAcquired = true;
    }
    else
    {
        physicsDspBridge.prepare (sampleRate, physicsWorker.getPeriodMs() * 0.001);
        physicsWorker.setUpdateRateIndex (sceneGraph.getPhysicsRateIndex());
    }

    // Prepare spatial renderer (Phase 2.2)
    spatialRenderer.prepare (sampleRate, samplesPerBlock);

    // Prepare calibration engine (Phase 2.3)
    calibrationEngine.prepare (sampleRate, samplesPerBlock);

    // BL-052: prepare calibration monitoring virtual-surround adapter.
    calMonitorVirtualSurround.prepare (samplesPerBlock);

    headTrackingBridge.start();

    lastProcessedMode = getCurrentMode();
    hasLastProcessedMode = true;
    syncSceneGraphRegistrationForMode (lastProcessedMode);
}

void LocusQAudioProcessor::releaseResources()
{
    sceneGraph.clearAudioReservation (sceneGraphAudioReservationId);
    headTrackingBridge.stop();
    if (emitterSlotId >= 0)
    {
        physicsWorker.unregisterEngine (emitterSlotId);
        physicsWorker.deactivateSlot (emitterSlotId);
        physicsDspBridge.publishZero (emitterSlotId);
    }
    if (physicsSharedRuntimeAcquired)
    {
        physicsSharedRuntime.release();
        physicsSharedRuntimeAcquired = false;
    }
    physicsEngine.shutdown();
    spatialRenderer.shutdown();
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        keyframeTimelineState.reset();
        publishKeyframeTimelinePlaybackState (keyframeTimelineState);
        publishKeyframeTimelineStateToRtLocked();
    }
    visualTokenScheduler.reset();
    modeTransitionInputSnapshotBuffer.clear();
    modeTransitionRendererScratchBuffer.clear();
    hasLastProcessedMode = false;
}

void LocusQAudioProcessor::handleAsyncUpdate()
{
    for (int slot = 0; slot < kPhysicsDAWSlotCount; ++slot)
    {
        if (physSpreadHostDirty[slot].exchange (false, std::memory_order_acq_rel))
        {
            auto* parameter = physSpreadNotifyTargets[slot];
            if (parameter != nullptr)
            {
                const auto value = juce::jlimit (
                    0.0f,
                    1.0f,
                    physSpreadHostPending[slot].load (std::memory_order_acquire));
                parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
                physSpreadHostPublished[slot].store (value, std::memory_order_release);
            }
        }

        if (! physTransientHostDirty[slot].exchange (false, std::memory_order_acq_rel))
            continue;

        auto* parameter = physTransientNotifyTargets[slot];
        if (parameter == nullptr)
            continue;

        const auto value = juce::jlimit (
            0.0f,
            1.0f,
            physTransientHostPending[slot].load (std::memory_order_acquire));
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        physTransientHostPublished[slot].store (value, std::memory_order_release);
    }

    auto publishDebugValue = [] (std::atomic<bool>& dirty,
                                 std::atomic<float>& pending,
                                 std::atomic<float>& published,
                                 juce::RangedAudioParameter* parameter)
    {
        if (! dirty.exchange (false, std::memory_order_acq_rel))
            return;

        if (parameter == nullptr)
            return;

        const auto value = pending.load (std::memory_order_acquire);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        published.store (value, std::memory_order_release);
    };

    publishDebugValue (physDebugActiveSlotDirty,
                       physDebugActiveSlotPending,
                       physDebugActiveSlotPublished,
                       physDebugActiveSlotNotifyTarget);
    publishDebugValue (physDebugActiveEmittersDirty,
                       physDebugActiveEmittersPending,
                       physDebugActiveEmittersPublished,
                       physDebugActiveEmittersNotifyTarget);
    publishDebugValue (physDebugCoordinatedWorkerDirty,
                       physDebugCoordinatedWorkerPending,
                       physDebugCoordinatedWorkerPublished,
                       physDebugCoordinatedWorkerNotifyTarget);
    publishDebugValue (physDebugBoidsDensityDirty,
                       physDebugBoidsDensityPending,
                       physDebugBoidsDensityPublished,
                       physDebugBoidsDensityNotifyTarget);
    publishDebugValue (physDebugWorkerSlotActiveDirty,
                       physDebugWorkerSlotActivePending,
                       physDebugWorkerSlotActivePublished,
                       physDebugWorkerSlotActiveNotifyTarget);
    publishDebugValue (physDebugWorkerBoidsActiveDirty,
                       physDebugWorkerBoidsActivePending,
                       physDebugWorkerBoidsActivePublished,
                       physDebugWorkerBoidsActiveNotifyTarget);
    publishDebugValue (physDebugBoidsGroupSizeDirty,
                       physDebugBoidsGroupSizePending,
                       physDebugBoidsGroupSizePublished,
                       physDebugBoidsGroupSizeNotifyTarget);
    publishDebugValue (physDebugWorkerPosXDirty,
                       physDebugWorkerPosXPending,
                       physDebugWorkerPosXPublished,
                       physDebugWorkerPosXNotifyTarget);
    publishDebugValue (physDebugWorkerPosYDirty,
                       physDebugWorkerPosYPending,
                       physDebugWorkerPosYPublished,
                       physDebugWorkerPosYNotifyTarget);
    publishDebugValue (physDebugWorkerPosZDirty,
                       physDebugWorkerPosZPending,
                       physDebugWorkerPosZPublished,
                       physDebugWorkerPosZNotifyTarget);
    publishDebugValue (physDebugAlignNeighborsDirty,
                       physDebugAlignNeighborsPending,
                       physDebugAlignNeighborsPublished,
                       physDebugAlignNeighborsNotifyTarget);
    publishDebugValue (physDebugCohNeighborsDirty,
                       physDebugCohNeighborsPending,
                       physDebugCohNeighborsPublished,
                       physDebugCohNeighborsNotifyTarget);

    updateHostDisplay();
}

void LocusQAudioProcessor::captureModeTransitionInputSnapshot (const juce::AudioBuffer<float>& sourceBuffer,
                                                               int totalNumInputChannels,
                                                               int totalNumOutputChannels) noexcept
{
    const auto numSamples = juce::jmin (sourceBuffer.getNumSamples(), modeTransitionInputSnapshotBuffer.getNumSamples());
    const auto channelsToPrepare = juce::jlimit (0,
                                                 kModeTransitionScratchChannels,
                                                 juce::jmax (totalNumInputChannels, totalNumOutputChannels));

    for (int channel = 0; channel < channelsToPrepare; ++channel)
        modeTransitionInputSnapshotBuffer.clear (channel, 0, numSamples);

    const auto channelsToCopy = juce::jlimit (0, channelsToPrepare, totalNumInputChannels);
    for (int channel = 0; channel < channelsToCopy; ++channel)
    {
        modeTransitionInputSnapshotBuffer.copyFrom (channel,
                                                    0,
                                                    sourceBuffer,
                                                    channel,
                                                    0,
                                                    numSamples);
    }
}

void LocusQAudioProcessor::copyModeTransitionInputSnapshotToBuffer (juce::AudioBuffer<float>& targetBuffer,
                                                                    int totalNumOutputChannels) noexcept
{
    const auto numSamples = juce::jmin (targetBuffer.getNumSamples(), modeTransitionInputSnapshotBuffer.getNumSamples());
    const auto channelsToCopy = juce::jlimit (0,
                                              juce::jmin (targetBuffer.getNumChannels(), kModeTransitionScratchChannels),
                                              totalNumOutputChannels);

    for (int channel = 0; channel < channelsToCopy; ++channel)
    {
        targetBuffer.copyFrom (channel,
                               0,
                               modeTransitionInputSnapshotBuffer,
                               channel,
                               0,
                               numSamples);
    }
}

void LocusQAudioProcessor::prepareRendererRealtimeStateForBlock()
{
    sceneGraph.setPhysicsRateIndex (
        static_cast<int> (apvts.getRawParameterValue ("rend_phys_rate")->load()));
    sceneGraph.setPhysicsPaused (
        apvts.getRawParameterValue ("rend_phys_pause")->load() > 0.5f);
    sceneGraph.setPhysicsWallCollisionEnabled (
        apvts.getRawParameterValue ("rend_phys_walls")->load() > 0.5f);
    const bool physicsInteractionEnabled = apvts.getRawParameterValue ("rend_phys_interact")->load() > 0.5f;
    sceneGraph.setPhysicsInteractionEnabled (physicsInteractionEnabled);

    updateRendererParameters();
    const auto auditionPhysicsReactiveInput = computeAuditionPhysicsReactiveInput (
        sceneGraph,
        physicsInteractionEnabled);
    spatialRenderer.setAuditionPhysicsReactiveInput (
        auditionPhysicsReactiveInput.active,
        auditionPhysicsReactiveInput.velocityNorm,
        auditionPhysicsReactiveInput.collisionNorm,
        auditionPhysicsReactiveInput.densityNorm);

    SpatialRenderer::PoseSnapshot rendererPose {};
    float rawYaw = 0.0f;
    if (tryBuildFreshInterpolatedHeadPose (headTrackingBridge.currentPose(),
                                           headPoseInterpolator,
                                           rendererPose,
                                           &rawYaw))
    {
        lastHeadTrackYawDeg.store (rawYaw, std::memory_order_relaxed);

        if (yawReferenceSet.load (std::memory_order_relaxed))
            applyYawOffsetToPose (rendererPose, yawReferenceDeg.load (std::memory_order_relaxed));

        spatialRenderer.applyHeadPose (rendererPose);
        return;
    }

    headPoseInterpolator.reset();
    spatialRenderer.clearHeadPose();
    lastHeadTrackYawDeg.store (0.0f, std::memory_order_relaxed);
}

void LocusQAudioProcessor::renderRendererScratchForModeTransition (int totalNumOutputChannels, int numSamples)
{
    const auto scratchChannels = juce::jlimit (1, kModeTransitionScratchChannels, totalNumOutputChannels);
    const auto scratchSamples = juce::jmin (numSamples, modeTransitionRendererScratchBuffer.getNumSamples());

    std::array<float*, kModeTransitionScratchChannels> scratchPointers {};
    for (int channel = 0; channel < scratchChannels; ++channel)
    {
        modeTransitionRendererScratchBuffer.clear (channel, 0, scratchSamples);
        scratchPointers[static_cast<size_t> (channel)] = modeTransitionRendererScratchBuffer.getWritePointer (channel);
    }

    juce::AudioBuffer<float> scratchView (scratchPointers.data(), scratchChannels, scratchSamples);
    prepareRendererRealtimeStateForBlock();
    spatialRenderer.process (scratchView, sceneGraph);
}

void LocusQAudioProcessor::applyModeTransitionCrossfade (juce::AudioBuffer<float>& targetBuffer,
                                                         const juce::AudioBuffer<float>& fromBuffer,
                                                         int totalNumOutputChannels,
                                                         int numSamples) noexcept
{
    const auto channelsToBlend = juce::jlimit (0,
                                               juce::jmin (targetBuffer.getNumChannels(), fromBuffer.getNumChannels()),
                                               totalNumOutputChannels);
    if (channelsToBlend <= 0 || numSamples <= 0)
        return;

    if (numSamples == 1)
    {
        for (int channel = 0; channel < channelsToBlend; ++channel)
        {
            auto* target = targetBuffer.getWritePointer (channel);
            const auto* from = fromBuffer.getReadPointer (channel);
            target[0] = 0.5f * (target[0] + from[0]);
        }
        return;
    }

    const auto denom = static_cast<float> (numSamples - 1);
    for (int channel = 0; channel < channelsToBlend; ++channel)
    {
        auto* target = targetBuffer.getWritePointer (channel);
        const auto* from = fromBuffer.getReadPointer (channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto blend = static_cast<float> (sample) / denom;
            target[sample] = from[sample] + ((target[sample] - from[sample]) * blend);
        }
    }
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
    {
        if (lastReportedCalibrationLatency != 0)
        {
            lastReportedCalibrationLatency = 0;
            setLatencySamples (0);
        }
        // Clear stale pose state so the renderer does not keep animating with
        // the last ingested quaternion while the plugin is inactive.
        headPoseInterpolator.reset();
        spatialRenderer.clearHeadPose();
        lastHeadTrackYawDeg.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const auto requestedMode = getCurrentMode();
    const auto previousMode = hasLastProcessedMode ? lastProcessedMode : requestedMode;
    const bool transitioningFromRenderer =
        previousMode == LocusQMode::Renderer && requestedMode != LocusQMode::Renderer;
    const bool transitioningToRenderer =
        previousMode == LocusQMode::Emitter && requestedMode == LocusQMode::Renderer;
    const auto transitionNumSamples = juce::jmin (buffer.getNumSamples(), modeTransitionInputSnapshotBuffer.getNumSamples());

    if (transitioningToRenderer)
        captureModeTransitionInputSnapshot (buffer, totalNumInputChannels, totalNumOutputChannels);

    if (transitioningFromRenderer)
        renderRendererScratchForModeTransition (totalNumOutputChannels, buffer.getNumSamples());

    syncSceneGraphRegistrationForMode (requestedMode);

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

    switch (requestedMode)
    {
        case LocusQMode::Calibrate:
        {
            // Read mic input channel from parameter (1-indexed → 0-indexed)
            int micCh = static_cast<int> (apvts.getRawParameterValue ("cal_mic_channel")->load()) - 1;
            micCh = juce::jlimit (0, buffer.getNumChannels() - 1, micCh);

            // Preserve host-program passthrough while calibration is not actively
            // running. The engine is allowed to own the signal path only during
            // active run states.
            const auto calibrationState = calibrationEngine.getState();
            if (calibrationState == CalibrationEngine::State::Playing
                || calibrationState == CalibrationEngine::State::Recording
                || calibrationState == CalibrationEngine::State::Analyzing)
            {
                // CalibrationEngine manages signal generation, recording, and analysis.
                // processBlock() is RT safe: no allocation, atomic state reads only.
                calibrationEngine.processBlock (buffer, micCh);
            }

            // BL-052: apply cal_monitoring_path (speakers / steam_binaural / virtual_binaural).
            const int monPathIndex = static_cast<int> (
                apvts.getRawParameterValue ("cal_monitoring_path")->load());
            applyCalibrationMonitoringPath (buffer, monPathIndex);

            for (auto& rms : sceneSpeakerRms)
                rms.store (rms.load (std::memory_order_relaxed) * 0.92f, std::memory_order_relaxed);
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

                // CL-P1: feed audio ring buffer for ChoreographyWorker feature extraction.
                physicsWorker.getChoreographyWorker().pushAudioBlock (
                    buffer.getArrayOfReadPointers(),
                    buffer.getNumChannels(),
                    buffer.getNumSamples());

                // CL-P4: forward DAW transport info to ChoreographyWorker (atomic stores;
                // no DAW callback on audio thread — playhead reads are always synchronous).
                {
                    double ppq = 0.0, bpm = 120.0;
                    bool   playing = false;
                    if (auto* ph = getPlayHead())
                    {
                        juce::AudioPlayHead::CurrentPositionInfo pos;
                        if (ph->getCurrentPosition (pos))
                        {
                            ppq     = pos.ppqPosition;
                            bpm     = pos.bpm;
                            playing = pos.isPlaying;
                        }
                    }
                    physicsWorker.getChoreographyWorker().setTransportInfo (ppq, bpm, playing);
                }

                // Publish spatial state
                const auto emitterStartTicks = juce::Time::getHighResolutionTicks();
                publishEmitterState (buffer.getNumSamples());
                const auto emitterElapsedTicks = juce::Time::getHighResolutionTicks() - emitterStartTicks;
                const auto emitterMs = (static_cast<double> (emitterElapsedTicks) * 1000.0) / ticksPerSecond;
                updatePerfEma (perfEmitterPublishMs, emitterMs);
            }

            // Audio passes through unchanged in Emitter mode
            for (auto& rms : sceneSpeakerRms)
                rms.store (rms.load (std::memory_order_relaxed) * 0.94f, std::memory_order_relaxed);
            break;
        }

        case LocusQMode::Renderer:
        {
            prepareRendererRealtimeStateForBlock();

            // Clear output buffer (renderer generates its own audio from emitters)
            buffer.clear();

            // Spatialize all emitters into output
            const auto rendererStartTicks = juce::Time::getHighResolutionTicks();
            spatialRenderer.process (buffer, sceneGraph);
            const auto rendererElapsedTicks = juce::Time::getHighResolutionTicks() - rendererStartTicks;
            const auto rendererMs = (static_cast<double> (rendererElapsedTicks) * 1000.0) / ticksPerSecond;
            updatePerfEma (perfRendererProcessMs, rendererMs);

            // Finite-output guardrail pass: applied after spatial renderer writes to buffer,
            // before the host buffer is returned. Non-finite samples are silenced, denormals
            // are flushed, and output is clamped within the two-stage protection envelope
            // (preferred abs ≤ 1.0, hard ceiling abs ≤ 4.0).
            {
                std::uint32_t nonFiniteCount     = 0;
                std::uint32_t denormalCount      = 0;
                std::uint32_t limiterClampCount  = 0;
                std::uint32_t hardClampCount     = 0;
                int           fallbackReason     = 0; // 0=none, 3=denormal, 5=non-finite/limiter, 6=hard-clamp

                const int numGuardChannels = buffer.getNumChannels();
                const int numGuardSamples  = buffer.getNumSamples();

                for (int ch = 0; ch < numGuardChannels; ++ch)
                {
                    auto* data = buffer.getWritePointer (ch);
                    for (int i = 0; i < numGuardSamples; ++i)
                    {
                        float s = data[i];

                        if (! std::isfinite (s))
                        {
                            // Non-finite output sample → replace with silence
                            data[i] = 0.0f;
                            ++nonFiniteCount;
                            fallbackReason = 5;
                            continue;
                        }

                        const float absS = std::abs (s);

                        if (absS > 0.0f && absS < 1.0e-30f)
                        {
                            // Denormal-range value → flush to zero
                            data[i] = 0.0f;
                            ++denormalCount;
                            if (fallbackReason == 0)
                                fallbackReason = 3;
                            continue;
                        }

                        if (absS > 4.0f)
                        {
                            // Hard safety clamp: abs > 4.0
                            data[i] = s > 0.0f ? 4.0f : -4.0f;
                            ++hardClampCount;
                            if (fallbackReason == 0)
                                fallbackReason = 6;
                        }
                        else if (absS > 1.0f)
                        {
                            // Preferred envelope clamp: abs > 1.0
                            data[i] = s > 0.0f ? 1.0f : -1.0f;
                            ++limiterClampCount;
                            if (fallbackReason == 0)
                                fallbackReason = 5;
                        }
                    }
                }

                // Publish finite-output diagnostics for observability
                const bool guardrailsActive = (nonFiniteCount > 0 || denormalCount > 0
                                               || limiterClampCount > 0 || hardClampCount > 0);
                publishedFiniteGuardrailDiagnostics.finiteGuardrailsActive.store (guardrailsActive, std::memory_order_relaxed);
                publishedFiniteGuardrailDiagnostics.finiteGuardrailsFallbackReason.store (fallbackReason, std::memory_order_relaxed);
                publishedFiniteGuardrailDiagnostics.finiteGuardrailsNonFiniteCount.store (nonFiniteCount, std::memory_order_relaxed);
                publishedFiniteGuardrailDiagnostics.finiteGuardrailsDenormalCount.store (denormalCount, std::memory_order_relaxed);
                publishedFiniteGuardrailDiagnostics.finiteGuardrailsLimiterClampCount.store (limiterClampCount, std::memory_order_relaxed);
                publishedFiniteGuardrailDiagnostics.finiteGuardrailsHardClampCount.store (hardClampCount, std::memory_order_relaxed);
                publishedFiniteGuardrailDiagnostics.snapshotSeq.fetch_add (1, std::memory_order_release);
            }

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
                const auto current = sceneSpeakerRms[i].load (std::memory_order_relaxed);
                const auto smoothed = current + (clamped - current) * kRmsSmoothing;
                sceneSpeakerRms[i].store (smoothed, std::memory_order_relaxed);
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

    if (transitioningFromRenderer)
    {
        applyModeTransitionCrossfade (buffer,
                                      modeTransitionRendererScratchBuffer,
                                      totalNumOutputChannels,
                                      transitionNumSamples);
    }
    else if (transitioningToRenderer)
    {
        applyModeTransitionCrossfade (buffer,
                                      modeTransitionInputSnapshotBuffer,
                                      totalNumOutputChannels,
                                      transitionNumSamples);
    }

    lastProcessedMode = requestedMode;
    hasLastProcessedMode = true;

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

    // Report calibration chain latency to the DAW host — guarded to avoid spamming
    // hosts with redundant PDC-recalculation notifications on every block.
    const int calLatency = spatialRenderer.getCalibrationLatencySamples();
    if (calLatency != lastReportedCalibrationLatency)
    {
        lastReportedCalibrationLatency = calLatency;
        setLatencySamples (calLatency);
    }
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
    const auto headphoneProfileIndex = static_cast<int> (
        apvts.getRawParameterValue ("rend_headphone_profile")->load());
    spatialRenderer.setHeadphoneDeviceProfile (headphoneProfileIndex);
    spatialRenderer.loadPeqPresetForProfile (headphoneProfileIndex, currentSampleRate);
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
// BL-052: Calibration monitoring path routing.
// Called after calibrationEngine.processBlock() in the Calibrate mode audio path.
// RT-safe: no allocation. Monitoring mode is read atomically from APVTS.
//
// Path semantics:
//   speakers (0)        — no-op; audio reaches speakers unchanged (current default).
//   stereo_downmix (1)  — no-op here; CalibrationEngine already outputs mono-ish signal.
//   steam_binaural (2)  — fold quad output to stereo binaural via Steam Audio virtual surround.
//   virtual_binaural (3)— steam_binaural plus optional head-tracking orientation injection.
//                         Companion disconnect/stale pose falls back to identity.
void LocusQAudioProcessor::applyCalibrationMonitoringPath (juce::AudioBuffer<float>& buffer,
                                                            int monPathIndex)
{
    using namespace locusq::shared_contracts::headphone_calibration;

    const auto monPathId = calibrationMonitoringPathIdForIndex (monPathIndex);

    // speakers and stereo_downmix: leave the buffer untouched.
    if (monPathId == path::kSpeakers || monPathId == path::kStereoDownmix)
        return;

    // steam_binaural or virtual_binaural: attempt quad → stereo binaural via Steam Audio.
    const int numCh      = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numCh < 2 || numSamples <= 0)
        return;

    // Assemble quad input channel pointers (host output order: FL=0, FR=1, RL=2, RR=3).
    // SteamAudioVirtualSurround zero-pads any channel index beyond numCh.
    const float* inputPtrs[kQuadSpeakerCount] {};
    for (int ch = 0; ch < kQuadSpeakerCount; ++ch)
        inputPtrs[ch] = ch < numCh ? buffer.getReadPointer (ch) : nullptr;

    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getWritePointer (1);

    IPLCoordinateSpace3 monitoringOrientation {};
    const IPLCoordinateSpace3* monitoringOrientationPtr = nullptr;

    if (monPathId == path::kVirtualBinaural
        && calibrationProfileTrackingEnabled.load (std::memory_order_relaxed))
    {
        SpatialRenderer::PoseSnapshot listenerPose {};
        if (tryBuildFreshInterpolatedHeadPose (headTrackingBridge.currentPose(),
                                               headPoseInterpolator,
                                               listenerPose))
        {
            const float profileYawOffsetDeg = calibrationProfileYawOffsetDeg.load (std::memory_order_relaxed);
            const float runtimeYawOffsetDeg = yawReferenceSet.load (std::memory_order_relaxed)
                ? yawReferenceDeg.load (std::memory_order_relaxed)
                : 0.0f;
            applyYawOffsetToPose (listenerPose, profileYawOffsetDeg + runtimeYawOffsetDeg);

            if (poseSnapshotToCoordinateSpace (listenerPose, monitoringOrientation))
                monitoringOrientationPtr = &monitoringOrientation;
        }
        else
        {
            headPoseInterpolator.reset();
        }
    }

    if (calMonitorVirtualSurround.applyBlock (inputPtrs,
                                              numCh,
                                              outL,
                                              outR,
                                              numSamples,
                                              QuadSpeakerLayout::Quadraphonic,
                                              monitoringOrientationPtr))
    {
        // Binaural applied — clear remaining channels so no quad signal leaks out.
        for (int ch = 2; ch < numCh; ++ch)
            buffer.clear (ch, 0, numSamples);
    }
    // If applyBlock returns false (Steam unavailable), the buffer passes through
    // unchanged (speakers path fallback is implicit).
}

//==============================================================================
void LocusQAudioProcessor::initialiseDefaultKeyframeTimeline (KeyframeTimeline& timeline) const
{
    if (timeline.hasAnyTrack())
        return;

    KeyframeTrack azimuthTrack { kTrackPosAzimuth };
    azimuthTrack.setKeyframes ({
        { 0.0, -60.0f, KeyframeCurve::easeInOut },
        { 2.0, 20.0f,  KeyframeCurve::easeInOut },
        { 4.0, 95.0f,  KeyframeCurve::easeInOut },
        { 6.0, 10.0f,  KeyframeCurve::easeInOut },
        { 8.0, -60.0f, KeyframeCurve::easeInOut }
    });
    timeline.addOrReplaceTrack (std::move (azimuthTrack));

    KeyframeTrack elevationTrack { kTrackPosElevation };
    elevationTrack.setKeyframes ({
        { 0.0,  0.0f,  KeyframeCurve::easeInOut },
        { 2.0,  18.0f, KeyframeCurve::easeInOut },
        { 4.0,  2.0f,  KeyframeCurve::easeInOut },
        { 6.0, -14.0f, KeyframeCurve::easeInOut },
        { 8.0,  0.0f,  KeyframeCurve::easeInOut }
    });
    timeline.addOrReplaceTrack (std::move (elevationTrack));

    KeyframeTrack distanceTrack { kTrackPosDistance };
    distanceTrack.setKeyframes ({
        { 0.0, 2.1f, KeyframeCurve::easeInOut },
        { 2.0, 3.6f, KeyframeCurve::easeInOut },
        { 4.0, 2.4f, KeyframeCurve::easeInOut },
        { 6.0, 1.3f, KeyframeCurve::easeInOut },
        { 8.0, 2.1f, KeyframeCurve::easeInOut }
    });
    timeline.addOrReplaceTrack (std::move (distanceTrack));

    KeyframeTrack sizeTrack { kTrackSizeUniform };
    sizeTrack.setKeyframes ({
        { 0.0, 0.45f, KeyframeCurve::easeInOut },
        { 2.0, 0.62f, KeyframeCurve::easeInOut },
        { 4.0, 0.35f, KeyframeCurve::easeInOut },
        { 6.0, 0.74f, KeyframeCurve::easeInOut },
        { 8.0, 0.45f, KeyframeCurve::easeInOut }
    });
    timeline.addOrReplaceTrack (std::move (sizeTrack));

    timeline.setDurationSeconds (8.0);
    timeline.setLooping (true);
    timeline.setPlaybackRate (1.0f);
}

void LocusQAudioProcessor::publishKeyframeTimelineStateToRtLocked()
{
    const auto readIndex = keyframeTimelineRtReadIndex.load (std::memory_order_acquire);
    const auto pendingIndex = keyframeTimelineRtPendingIndex.load (std::memory_order_acquire);

    int writeIndex = 0;
    for (int candidate = 0; candidate < static_cast<int> (keyframeTimelineRtBuffers.size()); ++candidate)
    {
        if (candidate != readIndex && candidate != pendingIndex)
        {
            writeIndex = candidate;
            break;
        }
    }

    keyframeTimelineRtBuffers[static_cast<size_t> (writeIndex)] = keyframeTimelineState;
    publishKeyframeTimelinePlaybackState (keyframeTimelineState);
    keyframeTimelineRtPendingIndex.store (writeIndex, std::memory_order_release);
}

void LocusQAudioProcessor::syncPendingKeyframeTimelineForAudioThread() noexcept
{
    const auto pendingIndex = keyframeTimelineRtPendingIndex.exchange (-1, std::memory_order_acq_rel);
    if (pendingIndex >= 0)
        keyframeTimelineRtReadIndex.store (pendingIndex, std::memory_order_release);
}

void LocusQAudioProcessor::publishKeyframeTimelinePlaybackState (const KeyframeTimeline& timeline) noexcept
{
    keyframeTimelinePublishedCurrentTimeSeconds.store (timeline.getCurrentTimeSeconds(), std::memory_order_release);
    keyframeTimelinePublishedDurationSeconds.store (timeline.getDurationSeconds(), std::memory_order_release);
    keyframeTimelinePublishedLooping.store (timeline.isLooping(), std::memory_order_release);
    keyframeTimelinePublishedPlaybackRate.store (timeline.getPlaybackRate(), std::memory_order_release);
}

void LocusQAudioProcessor::publishHeadphoneDiagnosticsSnapshot (
    const PublishedHeadphoneCalibrationDiagnostics& calibration,
    const PublishedHeadphoneVerificationDiagnostics& verification) noexcept
{
    publishedHeadphoneDiagnostics.seq.fetch_add (1, std::memory_order_acq_rel);
    publishedHeadphoneDiagnostics.calibration = calibration;
    publishedHeadphoneDiagnostics.verification = verification;
    publishedHeadphoneDiagnostics.seq.fetch_add (1, std::memory_order_release);
}

bool LocusQAudioProcessor::copyPublishedHeadphoneDiagnosticsSnapshot (
    PublishedHeadphoneCalibrationDiagnostics& calibration,
    PublishedHeadphoneVerificationDiagnostics& verification) const noexcept
{
    constexpr int kMaxReadAttempts = 3;
    for (int attempt = 0; attempt < kMaxReadAttempts; ++attempt)
    {
        const auto seqBefore = publishedHeadphoneDiagnostics.seq.load (std::memory_order_acquire);
        if ((seqBefore & 1u) != 0u)
            continue;

        calibration = publishedHeadphoneDiagnostics.calibration;
        verification = publishedHeadphoneDiagnostics.verification;

        const auto seqAfter = publishedHeadphoneDiagnostics.seq.load (std::memory_order_acquire);
        if (seqBefore == seqAfter && (seqAfter & 1u) == 0u)
            return true;
    }

    return false;
}

#if defined (LOCUSQ_TESTING) && LOCUSQ_TESTING
void LocusQAudioProcessor::publishBoidsHostDebugSnapshot (const BoidsHostDebugSnapshot& snapshot) noexcept
{
    publishedBoidsHostDebugState.seq.fetch_add (1, std::memory_order_acq_rel);
    publishedBoidsHostDebugState.snapshot = snapshot;
    publishedBoidsHostDebugState.seq.fetch_add (1, std::memory_order_release);
}

bool LocusQAudioProcessor::copyPublishedBoidsHostDebugSnapshot (BoidsHostDebugSnapshot& snapshot) const noexcept
{
    constexpr int kMaxReadAttempts = 3;
    for (int attempt = 0; attempt < kMaxReadAttempts; ++attempt)
    {
        const auto seqBefore = publishedBoidsHostDebugState.seq.load (std::memory_order_acquire);
        if ((seqBefore & 1u) != 0u)
            continue;

        snapshot = publishedBoidsHostDebugState.snapshot;

        const auto seqAfter = publishedBoidsHostDebugState.seq.load (std::memory_order_acquire);
        if (seqBefore == seqAfter && (seqAfter & 1u) == 0u)
            return true;
    }

    return false;
}
#endif

std::optional<TransportTimelineSyncSnapshot> LocusQAudioProcessor::getTransportTimelineSyncSnapshot() const
{
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            TransportTimelineSyncSnapshot snapshot;
            if (const auto ppqPosition = position->getPpqPosition())
                snapshot.ppqPosition = *ppqPosition;

            if (const auto bpm = position->getBpm())
                snapshot.bpm = *bpm;

            if (const auto timeSeconds = position->getTimeInSeconds())
            {
                snapshot.timeSeconds = *timeSeconds;
                return snapshot;
            }

            if (const auto samplePosition = position->getTimeInSamples())
            {
                snapshot.timeSeconds = static_cast<double> (*samplePosition) / juce::jmax (1.0, currentSampleRate);
                return snapshot;
            }

            if (snapshot.ppqPosition.has_value())
            {
                if (snapshot.bpm.has_value() && *snapshot.bpm > 1.0e-6)
                {
                    snapshot.timeSeconds = (*snapshot.ppqPosition * 60.0) / *snapshot.bpm;
                    return snapshot;
                }
            }
        }
    }

    return std::nullopt;
}

//==============================================================================
void LocusQAudioProcessor::publishEmitterState (int numSamplesInBlock)
{
    const auto loadParam = [] (const std::atomic<float>* param) noexcept -> float
    {
        jassert (param != nullptr);
        return param != nullptr ? param->load() : 0.0f;
    };

    const int activeEmitterSlot = emitterSlotId;
    if (activeEmitterSlot < 0)
        return;

    sceneGraph.setPhysicsRateIndex (
        static_cast<int> (loadParam (rendPhysRateParam)));
    sceneGraph.setPhysicsPaused (
        loadParam (rendPhysPauseParam) > 0.5f);
    sceneGraph.setPhysicsWallCollisionEnabled (
        loadParam (rendPhysWallsParam) > 0.5f);
    sceneGraph.setPhysicsInteractionEnabled (
        loadParam (rendPhysInteractParam) > 0.5f);

    const auto existingData = sceneGraph.getSlot (activeEmitterSlot).read();

    EmitterData data;
    data.active = true;
    std::memcpy (data.label, existingData.label, sizeof (data.label));
    data.label[sizeof (data.label) - 1] = '\0';

    const auto coordMode = loadParam (posCoordModeParam);
    float azimuthDeg = loadParam (posAzimuthParam);
    float elevationDeg = loadParam (posElevationParam);
    float distance = loadParam (posDistanceParam);
    float posX = loadParam (posXParam);
    float posY = loadParam (posYParam);
    float posZ = loadParam (posZParam);
    float sizeUniform = loadParam (sizeUniformParam);
    std::optional<TimelinePoint3D> formationPosition;
    bool teleportTriggeredThisBlock = false;
    auto teleportBeatDivision = std::optional<TimelineBeatDivision> {};
    auto transportBpm = std::optional<double> {};
    const auto blockDurationSeconds = (currentSampleRate > 0.0)
        ? static_cast<double> (numSamplesInBlock) / currentSampleRate
        : 0.0;

    const bool animationEnabled = loadParam (animEnableParam) > 0.5f;
    const bool internalAnimation = animationEnabled
                               && static_cast<int> (loadParam (animModeParam)) == 1;

    if (internalAnimation)
    {
        syncPendingKeyframeTimelineForAudioThread();
        auto& keyframeTimeline =
            keyframeTimelineRtBuffers[static_cast<size_t> (keyframeTimelineRtReadIndex.load (std::memory_order_acquire))];

        keyframeTimeline.setLooping (loadParam (animLoopParam) > 0.5f);
        keyframeTimeline.setPlaybackRate (loadParam (animSpeedParam));

        bool advancedFromTransport = false;
        if (loadParam (animSyncParam) > 0.5f)
        {
            if (const auto transportSnapshot = getTransportTimelineSyncSnapshot())
            {
                transportBpm = transportSnapshot->bpm;
                auto playbackSeconds = transportSnapshot->timeSeconds;

                if (keyframeTimeline.hasBeatSyncCurves()
                    && transportSnapshot->ppqPosition.has_value()
                    && transportSnapshot->bpm.has_value()
                    && *transportSnapshot->bpm > 1.0e-6)
                {
                    const auto beatDivision = keyframeTimeline.getPreferredBeatDivision().value_or (beatDivisionFromString ("beat"));
                    const auto quantizedPpq = quantizePpqToBeatDivision (*transportSnapshot->ppqPosition,
                                                                         beatDivision);
                    playbackSeconds = (quantizedPpq * 60.0) / *transportSnapshot->bpm;
                }

                playbackSeconds *= static_cast<double> (keyframeTimeline.getPlaybackRate());
                keyframeTimeline.setCurrentTimeSeconds (playbackSeconds);

                if (keyframeTimeline.hasCurveAtCurrentTime (KeyframeCurve::teleport))
                {
                    teleportBeatDivision = keyframeTimeline.getCurveBeatDivisionAtCurrentTime (KeyframeCurve::teleport);
                    const bool quantizedJumped = lastTimelineBeatSyncQuantizedSeconds < 0.0
                        || std::abs (playbackSeconds - lastTimelineBeatSyncQuantizedSeconds) > 1.0e-6;
                    teleportTriggeredThisBlock = quantizedJumped;
                }

                lastTimelineBeatSyncQuantizedSeconds = playbackSeconds;
                advancedFromTransport = true;
            }
        }

        if (! advancedFromTransport)
        {
            lastTimelineBeatSyncQuantizedSeconds = -1.0;
            keyframeTimeline.advance (blockDurationSeconds);
        }

        formationPosition = keyframeTimeline.resolveFormationPositionAtCurrentTime (activeEmitterSlot);
        const auto proceduralPathPosition = keyframeTimeline.resolveProceduralPathPositionAtCurrentTime();

        if (! formationPosition.has_value() && coordMode < 0.5f)
        {
            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosAzimuth))
                azimuthDeg = *value;
            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosElevation))
                elevationDeg = *value;
            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosDistance))
                distance = *value;
        }
        else if (! formationPosition.has_value())
        {
            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosX))
                posX = *value;
            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosY))
                posY = *value;
            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosZ))
                posZ = *value;
        }

        if (proceduralPathPosition.has_value())
        {
            posX = proceduralPathPosition->x;
            posY = proceduralPathPosition->z;
            posZ = proceduralPathPosition->y;
        }

        if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackSizeUniform))
            sizeUniform = *value;

        publishKeyframeTimelinePlaybackState (keyframeTimeline);
    }

    Vec3 basePosition;

    if (formationPosition.has_value())
    {
        basePosition.x = formationPosition->x;
        basePosition.y = formationPosition->y;
        basePosition.z = formationPosition->z;
    }
    else if (coordMode < 0.5f) // Spherical
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

    const bool linkedSize = loadParam (sizeLinkParam) > 0.5f;
    if (linkedSize)
    {
        const float clampedSize = juce::jlimit (0.01f, 20.0f, sizeUniform);
        data.size = { clampedSize, clampedSize, clampedSize };
    }
    else
    {
        data.size.x = loadParam (sizeWidthParam);
        data.size.y = loadParam (sizeHeightParam);
        data.size.z = loadParam (sizeDepthParam);
    }

    data.gain        = loadParam (emitGainParam);
    data.spread      = loadParam (emitSpreadParam);
    data.directivity = loadParam (emitDirectivityParam);
    data.muted       = loadParam (emitMuteParam) > 0.5f;
    data.soloed      = loadParam (emitSoloParam) > 0.5f;

    if (! internalAnimation)
    {
        lastTimelineBeatSyncQuantizedSeconds = -1.0;
        timelineTeleportGainDip = 1.0f;
    }
    else
    {
        if (teleportTriggeredThisBlock)
            timelineTeleportGainDip = 0.35f;

        data.gain *= timelineTeleportGainDip;

        auto teleportReleaseSeconds = 0.035;
        if (teleportBeatDivision.has_value()
            && loadParam (animSyncParam) > 0.5f
            && transportBpm.has_value()
            && *transportBpm > 1.0e-6)
        {
            teleportReleaseSeconds = juce::jlimit (0.0125,
                                                   0.125,
                                                   beatDivisionSeconds (*teleportBeatDivision, *transportBpm) * 0.5);
        }

        const auto releaseT = static_cast<float> (1.0 - std::exp (-blockDurationSeconds / teleportReleaseSeconds));
        timelineTeleportGainDip += (1.0f - timelineTeleportGainDip) * juce::jlimit (0.0f, 1.0f, releaseT);
    }

    const float aimAzimuth = loadParam (emitDirAzimuthParam);
    const float aimElevation = loadParam (emitDirElevationParam);
    const float aimAzimuthRad = aimAzimuth * juce::MathConstants<float>::pi / 180.0f;
    const float aimElevationRad = aimElevation * juce::MathConstants<float>::pi / 180.0f;
    data.directivityAim.x = std::cos (aimElevationRad) * std::sin (aimAzimuthRad);
    data.directivityAim.z = std::cos (aimElevationRad) * std::cos (aimAzimuthRad);
    data.directivityAim.y = std::sin (aimElevationRad);

    const bool physicsEnabled = loadParam (physEnableParam) > 0.5f;
    data.physicsEnabled = physicsEnabled;

    // CL-P1: propagate choro_enable to the ChoreographyWorker each block.
    const bool choroEnabled = loadParam (choroEnableParam) > 0.5f;
    physicsWorker.getChoreographyWorker().setEnabled (choroEnabled);

    // CL-P2: propagate formation params to the ChoreographyWorker each block.
    {
        auto& cw = physicsWorker.getChoreographyWorker();
        cw.setFormationType       (static_cast<int> (std::lround (loadParam (choroFormationTypeParam))));
        cw.setFormationAxis       (static_cast<int> (std::lround (loadParam (choroFormAxisParam))));
        cw.setFormationPlane      (static_cast<int> (std::lround (loadParam (choroFormPlaneParam))));
        cw.setFormationRadius     (loadParam (choroFormRadiusParam));
        cw.setFormationSpacing    (loadParam (choroFormSpacingParam));
        cw.setFormationArcAngle   (loadParam (choroFormArcAngleParam));
        cw.setFormationPhaseOffset(loadParam (choroFormPhaseOffsetParam));
        cw.setFormationRows       (loadParam (choroFormRowsParam));
        cw.setFormationCols       (loadParam (choroFormColsParam));
        cw.setFormationSpacingX   (loadParam (choroFormSpacingXParam));
        cw.setFormationSpacingZ   (loadParam (choroFormSpacingZParam));
        cw.setFormationTurns      (loadParam (choroFormTurnsParam));
        cw.setFormationHeightRise (loadParam (choroFormHeightRiseParam));
        cw.setFormationMorphRate  (loadParam (choroFormMorphRateParam));
        cw.setFormationMorphLoop      (loadParam (choroFormMorphLoopParam)      > 0.5f);
        cw.setFormationMorphPingpong  (loadParam (choroFormMorphPingpongParam)  > 0.5f);

        // CL-P3: propagate path params to the ChoreographyWorker each block.
        cw.setPathType        (static_cast<int> (std::lround (loadParam (choroPathTypeParam))));
        cw.setPathPeriod      (loadParam (choroPathPeriodParam));
        cw.setPathSpeed       (loadParam (choroPathSpeedParam));
        cw.setPathLissFreqA   (loadParam (choroPathLissFreqAParam));
        cw.setPathLissFreqB   (loadParam (choroPathLissFreqBParam));
        cw.setPathLissFreqC   (loadParam (choroPathLissFreqCParam));
        cw.setPathLissAmpX    (loadParam (choroPathLissAmpXParam));
        cw.setPathLissAmpY    (loadParam (choroPathLissAmpYParam));
        cw.setPathLissAmpZ    (loadParam (choroPathLissAmpZParam));
        cw.setPathLissPhase   (loadParam (choroPathLissPhaseParam));
        cw.setPathOrbitRx     (loadParam (choroPathOrbitRxParam));
        cw.setPathOrbitRz     (loadParam (choroPathOrbitRzParam));
        cw.setPathOrbitHeight (loadParam (choroPathOrbitHeightParam));
        cw.setPathPendLength  (loadParam (choroPathPendLengthParam));
        cw.setPathPendAmp     (loadParam (choroPathPendAmpParam));
        cw.setPathPendPlane   (static_cast<int> (std::lround (loadParam (choroPathPendPlaneParam))));
        cw.setPathFig8Scale   (loadParam (choroPathFig8ScaleParam));
        cw.setPathFig8Plane   (static_cast<int> (std::lround (loadParam (choroPathFig8PlaneParam))));
        cw.setPathHelixRadius (loadParam (choroPathHelixRadiusParam));
        cw.setPathHelixPitch  (loadParam (choroPathHelixPitchParam));
        cw.setPathHelixDir    (static_cast<int> (std::lround (loadParam (choroPathHelixDirParam))));
        cw.setPathWalkStep    (loadParam (choroPathWalkStepParam));
        cw.setPathWalkBounds  (loadParam (choroPathWalkBoundsParam));
        cw.setPathWalkSeed    (static_cast<int> (std::lround (loadParam (choroPathWalkSeedParam))));

        // CL-P4: propagate beat-sync params to the ChoreographyWorker each block.
        cw.setBeatEnabled     (loadParam (choroBeatEnableParam)      > 0.5f);
        cw.setBeatDivision    (static_cast<int> (std::lround (loadParam (choroBeatDivisionParam))));
        cw.setBeatMode        (static_cast<int> (std::lround (loadParam (choroBeatModeParam))));
        cw.setTeleportDipDb   (loadParam (choroTeleportDipDbParam));
        cw.setTeleportDecayMs (loadParam (choroTeleportDecayMsParam));
    }

    const int physicsRateIndex = sceneGraph.getPhysicsRateIndex();
    physicsEngine.setUpdateRateIndex (physicsRateIndex);
    physicsEngine.setPaused (sceneGraph.isPhysicsPaused());
    physicsEngine.setWallCollisionEnabled (sceneGraph.isPhysicsWallCollisionEnabled());
    const auto boundaryMode = static_cast<PhysicsEngine::BoundaryMode> (juce::jlimit (
        0,
        2,
        static_cast<int> (loadParam (physBoundaryModeParam))));
    const float softBoundaryDepth = loadParam (physSoftBoundaryDepthParam);
    physicsEngine.setBoundaryMode (boundaryMode);
    physicsEngine.setSoftBoundaryDepth (softBoundaryDepth);
    physicsWorker.setWallCollisionEnabled (sceneGraph.isPhysicsWallCollisionEnabled());
    physicsWorker.setBoundaryMode (boundaryMode);
    physicsWorker.setSoftBoundaryDepth (softBoundaryDepth);

    if (auto profile = sceneGraph.getRoomProfile(); profile != nullptr && profile->valid)
    {
        physicsEngine.setRoomDimensions (profile->dimensions);
        physicsWorker.setRoomDimensions (profile->dimensions);
    }

    auto& attractorSystem = physicsWorker.getAttractorSystem();
    auto& springSystem = physicsWorker.getSpringSystem();
    auto& turbulenceSystem = physicsWorker.getTurbulenceSystem();
    auto& angularSystem = physicsWorker.getAngularSystem();
    auto& boidsSystem = physicsWorker.getBoidsSystem();
    auto& collisionSystem = physicsWorker.getCollisionSystem();
    bool anyActiveAttractor = false;
    for (int sourceIndex = 0; sourceIndex < kPhysicsAttractorSourceCount; ++sourceIndex)
    {
        auto& source = attractorSystem.source (sourceIndex);
        const bool sourceActive = loadParam (attractorActiveParams[static_cast<size_t> (sourceIndex)]) > 0.5f;
        source.setActive (sourceActive);
        source.setPosition ({
            loadParam (attractorPosXParams[static_cast<size_t> (sourceIndex)]),
            loadParam (attractorPosYParams[static_cast<size_t> (sourceIndex)]),
            loadParam (attractorPosZParams[static_cast<size_t> (sourceIndex)])
        });
        source.setStrength (loadParam (attractorStrengthParams[static_cast<size_t> (sourceIndex)]));
        source.setRadius (loadParam (attractorRadiusParams[static_cast<size_t> (sourceIndex)]));
        source.setFalloff (static_cast<AttractorFalloff> (juce::jlimit (
            0,
            2,
            static_cast<int> (loadParam (attractorFalloffParams[static_cast<size_t> (sourceIndex)])))));
        source.setOrbitStabilize (
            loadParam (attractorOrbitStabilizeParams[static_cast<size_t> (sourceIndex)]) > 0.5f);
        anyActiveAttractor = anyActiveAttractor || sourceActive;
    }

    for (int groupIndex = 0; groupIndex < BoidsSystem::kMaxGroups; ++groupIndex)
    {
        const auto groupSlot = static_cast<size_t> (groupIndex);
        const bool groupEnabled = loadParam (flockEnableParams[groupSlot]) > 0.5f;
        boidsSystem.setGroupEnabled  (groupIndex, groupEnabled);
        boidsSystem.setSepWeight     (groupIndex, loadParam (flockSepWeightParams[groupSlot]));
        boidsSystem.setAlignWeight   (groupIndex, loadParam (flockAlignWeightParams[groupSlot]));
        boidsSystem.setCohWeight     (groupIndex, loadParam (flockCohWeightParams[groupSlot]));
        boidsSystem.setSepRadius     (groupIndex, loadParam (flockSepRadiusParams[groupSlot]));
        boidsSystem.setAlignRadius   (groupIndex, loadParam (flockAlignRadiusParams[groupSlot]));
        boidsSystem.setCohRadius     (groupIndex, loadParam (flockCohRadiusParams[groupSlot]));
        boidsSystem.setMaxSpeed      (groupIndex, loadParam (flockMaxSpeedParams[groupSlot]));
    }

    const int flockGroupChoice = static_cast<int> (std::lround (loadParam (physFlockGroupParam)));
    const int flockGroupIndex = flockGroupChoice - 1;
    boidsSystem.setEmitterGroup (activeEmitterSlot, flockGroupIndex);
    const bool boidsEnabledForEmitter = flockGroupIndex >= 0 && flockGroupIndex < BoidsSystem::kMaxGroups
        && boidsSystem.isGroupEnabled (flockGroupIndex);
    const bool springEnabledForEmitter =
        physicsEnabled && loadParam (physSpringEnableParam) > 0.5f;
    springSystem.setEnabled (springEnabledForEmitter);
    springSystem.setStiffness (loadParam (physSpringKParam));
    springSystem.setDamping (loadParam (physSpringDampParam));
    springSystem.setAnchorMode (static_cast<SpringAnchorMode> (juce::jlimit (
        0,
        1,
        static_cast<int> (std::lround (loadParam (physSpringAnchorModeParam))))));
    springSystem.setAnchorPos ({
        loadParam (physSpringAnchorXParam),
        loadParam (physSpringAnchorYParam),
        loadParam (physSpringAnchorZParam)
    });
    const float turbulenceAmount = physicsEnabled
        ? loadParam (physTurbulenceParam)
        : 0.0f;
    turbulenceSystem.setAmplitude (turbulenceAmount);
    turbulenceSystem.setRate (loadParam (physTurbulenceRateParam));
    const bool turbulenceEnabledForEmitter = turbulenceAmount > 1.0e-4f;
    const bool angularEnabledForEmitter =
        physicsEnabled && loadParam (physAngEnableParam) > 0.5f;
    angularSystem.setEnabled (angularEnabledForEmitter);
    angularSystem.setAngularDrag (loadParam (physAngDragParam));
    angularSystem.setImpulseX (loadParam (physAngImpulseXParam));
    angularSystem.setImpulseY (loadParam (physAngImpulseYParam));
    angularSystem.setImpulseZ (loadParam (physAngImpulseZParam));
    angularSystem.setAttractorTorque (loadParam (physAngAttractorTorqueParam));
    const bool angThrowGate = loadParam (physAngThrowParam) > 0.5f;
    if (angThrowGate && ! lastAngThrowGate)
        angularSystem.requestThrow();
    lastAngThrowGate = angThrowGate;

    const bool angResetGate = loadParam (physAngResetParam) > 0.5f;
    if (angResetGate && ! lastAngResetGate)
        angularSystem.requestReset();
    lastAngResetGate = angResetGate;
    const bool interactionEnabledForScene =
        physicsEnabled
        && sceneGraph.isPhysicsInteractionEnabled()
        && sceneGraph.getActiveEmitterCount() > 1;

    physicsWorker.setSlotMassOverride (activeEmitterSlot, loadParam (physMassOverrideParam));
    const bool collisionEnabled =
        physicsEnabled && loadParam (physCollideEmittersParam) > 0.5f;
    collisionSystem.setEnabled (collisionEnabled);
    collisionSystem.setCollisionRadius (activeEmitterSlot,
                                        loadParam (physCollisionRadiusParam));
    collisionSystem.setGainScale (loadParam (physCollisionGainScaleParam));
    collisionSystem.setDecayRateHz (
        1000.0f / juce::jmax (1.0f, loadParam (physCollisionDecayMsParam)));
    {
        PhysicsDSPBridge::SmoothConfig bridgeSmoothConfig;
        bridgeSmoothConfig.transientDecayHz = collisionSystem.getDecayRateHz();
        physicsDspBridge.setSmoothConfig (bridgeSmoothConfig);
    }

    const bool coordinatedWorkerActive =
        physicsEnabled && (anyActiveAttractor || springEnabledForEmitter || turbulenceEnabledForEmitter
                           || angularEnabledForEmitter || collisionEnabled
                           || boidsEnabledForEmitter || interactionEnabledForScene);
    physicsEngine.setStandaloneMode (! coordinatedWorkerActive);
    physicsEngine.setWallCollisionEnabled (sceneGraph.isPhysicsWallCollisionEnabled() && ! coordinatedWorkerActive);
    physicsWorker.setUpdateRateIndex (physicsRateIndex);
    physicsWorker.setPaused (sceneGraph.isPhysicsPaused());
    if (physicsRateIndex != lastPhysicsRateIndex)
    {
        lastPhysicsRateIndex = physicsRateIndex;
        physicsDspBridge.prepare (currentSampleRate, physicsWorker.getPeriodMs() * 0.001);
    }

    if (coordinatedWorkerActive)
    {
        physicsWorker.registerEngine (activeEmitterSlot, &physicsEngine);
        if (! physicsWorker.isSlotActive (activeEmitterSlot))
            physicsWorker.activateSlot (activeEmitterSlot, basePosition);
        physicsWorker.setSlotRestPosition (activeEmitterSlot, basePosition);
    }
    else
    {
        physicsEngine.setCoordinatedForce ({});
        physicsWorker.unregisterEngine (activeEmitterSlot);
        physicsWorker.deactivateSlot (activeEmitterSlot);
        boidsSystem.setEmitterGroup (activeEmitterSlot, -1);
        physicsDspBridge.publishZero (activeEmitterSlot);
    }

    physicsEngine.setRestPosition (basePosition);
    physicsEngine.setPhysicsEnabled (physicsEnabled);
    physicsEngine.setMass (loadParam (physMassParam));
    physicsEngine.setDrag (loadParam (physDragParam));
    physicsEngine.setElasticity (loadParam (physElasticityParam));
    physicsEngine.setFriction (loadParam (physFrictionParam));
    const float gravityMagnitude = loadParam (physGravityParam);
    const int gravityDirection = static_cast<int> (loadParam (physGravityDirParam));
    physicsWorker.setGravity (gravityMagnitude, gravityDirection);
    physicsEngine.setGravity (coordinatedWorkerActive ? 0.0f : gravityMagnitude,
                              coordinatedWorkerActive ? 0 : gravityDirection);

    Vec3 interactionForce {};
    if (interactionEnabledForScene)
    {
        const auto workerState = physicsWorker.getEmitterState (activeEmitterSlot);
        const auto physicsState = physicsEngine.getState();
        const Vec3 interactionPosition =
            (coordinatedWorkerActive && workerState.initialized)
                ? workerState.position
                : (physicsState.initialized ? physicsState.position : basePosition);
        interactionForce = computeEmitterInteractionForce (sceneGraph, activeEmitterSlot, interactionPosition);
    }
    physicsWorker.setSlotInteractionForce (activeEmitterSlot, interactionForce);
    physicsEngine.setInteractionForce (coordinatedWorkerActive ? Vec3 {} : interactionForce);

    const bool throwGate = loadParam (physThrowParam) > 0.5f;
    if (throwGate && ! lastPhysThrowGate)
    {
        const Vec3 throwVelocity
        {
            loadParam (physVelXParam),
            loadParam (physVelZParam), // Z in param = Y in 3D (height)
            loadParam (physVelYParam)
        };
        if (coordinatedWorkerActive)
            physicsWorker.requestThrow (activeEmitterSlot, throwVelocity);
        else
            physicsEngine.requestThrow (throwVelocity);
    }
    lastPhysThrowGate = throwGate;

    const bool resetGate = loadParam (physResetParam) > 0.5f;
    if (resetGate && ! lastPhysResetGate)
    {
        if (coordinatedWorkerActive)
            physicsWorker.requestReset (activeEmitterSlot);
        else
            physicsEngine.requestReset();
    }
    lastPhysResetGate = resetGate;

    if (physicsEnabled)
    {
        const auto workerState = physicsWorker.getEmitterState (activeEmitterSlot);
        const bool workerOwnsPublishedMotion =
            coordinatedWorkerActive
            && workerState.initialized
            && (std::abs (workerState.position.x - basePosition.x)
                + std::abs (workerState.position.y - basePosition.y)
                + std::abs (workerState.position.z - basePosition.z)
                + std::abs (workerState.velocity.x)
                + std::abs (workerState.velocity.y)
                + std::abs (workerState.velocity.z)) > 1.0e-5f;
        const bool workerOwnsPublishedAim =
            coordinatedWorkerActive
            && angularEnabledForEmitter
            && workerState.initialized;

        const auto physicsState = physicsEngine.getState();

        // Dual-integration guard: when the worker owns authority, the legacy
        // engine must not have produced independent motion. The engine's
        // standaloneMode guard should ensure this; the assertion catches any
        // future regression where that guard is bypassed.
#if JUCE_DEBUG
        if (coordinatedWorkerActive && physicsState.initialized)
        {
            const float legacyMotion = std::abs (physicsState.position.x - basePosition.x)
                                     + std::abs (physicsState.position.y - basePosition.y)
                                     + std::abs (physicsState.position.z - basePosition.z)
                                     + std::abs (physicsState.velocity.x)
                                     + std::abs (physicsState.velocity.y)
                                     + std::abs (physicsState.velocity.z);
            jassert (legacyMotion < 1.0e-4f); // legacy engine must not integrate when worker owns authority
        }
#endif

        if (workerOwnsPublishedMotion)
        {
            data.position = workerState.position;
            data.velocity = workerState.velocity;
            data.force = workerState.force;
            data.collisionMask = workerState.collisionMask;
            data.collisionEnergy = workerState.collisionEnergy;
        }
        else if (physicsState.initialized)
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

        if (workerOwnsPublishedAim)
            data.directivityAim = workerState.directivityAim;
    }
    else
    {
        physicsDspBridge.publishZero (activeEmitterSlot);
        data.velocity = {};
        data.force = {};
        data.collisionMask = 0;
        data.collisionEnergy = 0.0f;
    }

    if (physicsEnabled)
    {
        const int   slot = activeEmitterSlot;

        // Bounds guard — DAW automation params cover slots 0..kPhysicsDAWSlotCount-1 only
        if (slot < 0 || slot >= kPhysicsDAWSlotCount)
        {
            jassert (false);  // unexpected slot index from SceneGraph::registerEmitter()
            const auto fallback = physicsDspBridge.read (slot);
            data.collisionEnergy = juce::jmax (data.collisionEnergy, fallback.gainTransient);
            // gainTransient still flows; freeze/mirror logic skipped to avoid UB on lastFrozenState
        }
        else
        {
            const bool nowFrozen  = physFrozenParams[slot]->load() > 0.5f;
            const bool wasFrozen  = lastFrozenState[slot];
            const bool justFroze  = !wasFrozen && nowFrozen;

            const auto dspValues = physicsDspBridge.read (slot);  // single read per slot per block

            lastFrozenState[slot] = nowFrozen;

            // Host-facing transient mirrors need a slower observation envelope than the
            // raw bridge burst, otherwise REAPER only catches the initial peak and loses
            // the short-vs-long decay distinction. This mirror remains observation-only:
            // DSP and scene state still consume the raw `gainTransient`.
            const auto blockPeriodSec = static_cast<float> (numSamplesInBlock / juce::jmax (1.0, getSampleRate()));
            const auto hostDecayMs = juce::jlimit (40.0f,
                                                   600.0f,
                                                   loadParam (physCollisionDecayMsParam) * 1.5f);
            const auto hostReleaseSec = hostDecayMs * 0.001f;
            const auto hostReleaseCoef = 1.0f - std::exp (-blockPeriodSec / juce::jmax (1.0e-4f, hostReleaseSec));
            auto& hostTransientState = physTransientHostMirrorState[slot];
            if (dspValues.gainTransient >= hostTransientState)
                hostTransientState = dspValues.gainTransient;
            else
                hostTransientState += hostReleaseCoef * (dspValues.gainTransient - hostTransientState);

            const auto hostObservedTransient = juce::jlimit (0.0f, 1.0f, hostTransientState);
            physTransientParams[slot]->store (hostObservedTransient);
            const auto publishedTransient = physTransientHostPublished[slot].load (std::memory_order_acquire);
            const bool transientStateChanged = std::abs (hostObservedTransient - publishedTransient) >= kPhysicsHostMirrorNotifyEpsilon
                                            || ((hostObservedTransient <= kPhysicsHostMirrorNotifyEpsilon)
                                                != (publishedTransient <= kPhysicsHostMirrorNotifyEpsilon));
            if (transientStateChanged)
            {
                physTransientHostPending[slot].store (hostObservedTransient, std::memory_order_release);
                physTransientHostDirty[slot].store (true, std::memory_order_release);
                triggerAsyncUpdate();
            }

            float spreadMod, gainMod;
            if (!nowFrozen)
            {
                // Live path: mirror atomics to APVTS output params - DAW polls these for recording
                physSpreadModParams[slot]->store (dspValues.spreadMod);
                physGainModParams[slot]->store   (dspValues.gainMod);
                const auto publishedSpread = physSpreadHostPublished[slot].load (std::memory_order_acquire);
                const bool spreadStateChanged = std::abs (dspValues.spreadMod - publishedSpread) >= kPhysicsHostMirrorNotifyEpsilon
                                            || ((dspValues.spreadMod <= kPhysicsHostMirrorNotifyEpsilon)
                                                != (publishedSpread <= kPhysicsHostMirrorNotifyEpsilon));
                if (spreadStateChanged)
                {
                    physSpreadHostPending[slot].store (dspValues.spreadMod, std::memory_order_release);
                    physSpreadHostDirty[slot].store (true, std::memory_order_release);
                    triggerAsyncUpdate();
                }
                spreadMod = dspValues.spreadMod;
                gainMod   = dspValues.gainMod;
            }
            else
            {
                if (justFroze)
                {
                    // LIVE -> FROZEN: snapshot current values at the freeze-effective block boundary
                    physSpreadModParams[slot]->store (dspValues.spreadMod);
                    physGainModParams[slot]->store   (dspValues.gainMod);
                    physSpreadHostPending[slot].store (dspValues.spreadMod, std::memory_order_release);
                    physSpreadHostDirty[slot].store (true, std::memory_order_release);
                    triggerAsyncUpdate();
                }
                // Frozen path: DAW playback owns the APVTS value; load() it for DSP
                spreadMod = physSpreadModParams[slot]->load();
                gainMod   = physGainModParams[slot]->load();
            }

            data.spread       = juce::jlimit (0.0f, 1.0f, data.spread + spreadMod);
            data.gain         = juce::jlimit (0.0f, 1.0f, data.gain   + gainMod);
            data.collisionEnergy = juce::jmax (data.collisionEnergy, dspValues.gainTransient);
            // gainTransient bypasses freeze state - one-shot bursts always flow through
        }
    }

    if (physDebugActiveSlotParam != nullptr)
        physDebugActiveSlotParam->store (static_cast<float> (activeEmitterSlot));
    if (physDebugActiveEmittersParam != nullptr)
        physDebugActiveEmittersParam->store (static_cast<float> (sceneGraph.getActiveEmitterCount()));
    if (physDebugCoordinatedWorkerParam != nullptr)
        physDebugCoordinatedWorkerParam->store (coordinatedWorkerActive ? 1.0f : 0.0f);

    const bool workerSlotActive = physicsWorker.isSlotActive (activeEmitterSlot);
    const bool workerBoidsActive = boidsSystem.isInActiveGroup (activeEmitterSlot);
    const float boidsGroupSize = static_cast<float> (boidsSystem.getGroupSizeForEmitter (activeEmitterSlot));
    const auto workerStateForDebug = physicsWorker.getEmitterState (activeEmitterSlot);
    const float alignNeighbors = static_cast<float> (boidsSystem.getAlignNeighborCount (activeEmitterSlot));
    const float cohNeighbors = static_cast<float> (boidsSystem.getCohNeighborCount (activeEmitterSlot));
    const auto boidsDensity = juce::jlimit (0.0f, 1.0f, boidsSystem.getFlockDensity (activeEmitterSlot));
    if (physDebugBoidsDensityParam != nullptr)
        physDebugBoidsDensityParam->store (boidsDensity);
    if (physDebugWorkerSlotActiveParam != nullptr)
        physDebugWorkerSlotActiveParam->store (workerSlotActive ? 1.0f : 0.0f);
    if (physDebugWorkerBoidsActiveParam != nullptr)
        physDebugWorkerBoidsActiveParam->store (workerBoidsActive ? 1.0f : 0.0f);
    if (physDebugBoidsGroupSizeParam != nullptr)
        physDebugBoidsGroupSizeParam->store (boidsGroupSize);
    if (physDebugWorkerPosXParam != nullptr)
        physDebugWorkerPosXParam->store (workerStateForDebug.position.x);
    if (physDebugWorkerPosYParam != nullptr)
        physDebugWorkerPosYParam->store (workerStateForDebug.position.y);
    if (physDebugWorkerPosZParam != nullptr)
        physDebugWorkerPosZParam->store (workerStateForDebug.position.z);
    if (physDebugAlignNeighborsParam != nullptr)
        physDebugAlignNeighborsParam->store (alignNeighbors);
    if (physDebugCohNeighborsParam != nullptr)
        physDebugCohNeighborsParam->store (cohNeighbors);

    auto queueDebugHostMirror = [this] (float value,
                                        std::atomic<float>& pending,
                                        std::atomic<float>& published,
                                        std::atomic<bool>& dirty)
    {
        const auto lastPublished = published.load (std::memory_order_acquire);
        const bool changed = std::abs (value - lastPublished) >= kPhysicsHostMirrorNotifyEpsilon
                          || ((value <= kPhysicsHostMirrorNotifyEpsilon)
                              != (lastPublished <= kPhysicsHostMirrorNotifyEpsilon));
        if (! changed)
            return;

        pending.store (value, std::memory_order_release);
        dirty.store (true, std::memory_order_release);
        triggerAsyncUpdate();
    };

    queueDebugHostMirror (static_cast<float> (activeEmitterSlot),
                          physDebugActiveSlotPending,
                          physDebugActiveSlotPublished,
                          physDebugActiveSlotDirty);
    queueDebugHostMirror (static_cast<float> (sceneGraph.getActiveEmitterCount()),
                          physDebugActiveEmittersPending,
                          physDebugActiveEmittersPublished,
                          physDebugActiveEmittersDirty);
    queueDebugHostMirror (coordinatedWorkerActive ? 1.0f : 0.0f,
                          physDebugCoordinatedWorkerPending,
                          physDebugCoordinatedWorkerPublished,
                          physDebugCoordinatedWorkerDirty);
    queueDebugHostMirror (boidsDensity,
                          physDebugBoidsDensityPending,
                          physDebugBoidsDensityPublished,
                          physDebugBoidsDensityDirty);
    queueDebugHostMirror (workerSlotActive ? 1.0f : 0.0f,
                          physDebugWorkerSlotActivePending,
                          physDebugWorkerSlotActivePublished,
                          physDebugWorkerSlotActiveDirty);
    queueDebugHostMirror (workerBoidsActive ? 1.0f : 0.0f,
                          physDebugWorkerBoidsActivePending,
                          physDebugWorkerBoidsActivePublished,
                          physDebugWorkerBoidsActiveDirty);
    queueDebugHostMirror (boidsGroupSize,
                          physDebugBoidsGroupSizePending,
                          physDebugBoidsGroupSizePublished,
                          physDebugBoidsGroupSizeDirty);
    queueDebugHostMirror (workerStateForDebug.position.x,
                          physDebugWorkerPosXPending,
                          physDebugWorkerPosXPublished,
                          physDebugWorkerPosXDirty);
    queueDebugHostMirror (workerStateForDebug.position.y,
                          physDebugWorkerPosYPending,
                          physDebugWorkerPosYPublished,
                          physDebugWorkerPosYDirty);
    queueDebugHostMirror (workerStateForDebug.position.z,
                          physDebugWorkerPosZPending,
                          physDebugWorkerPosZPublished,
                          physDebugWorkerPosZDirty);
    queueDebugHostMirror (alignNeighbors,
                          physDebugAlignNeighborsPending,
                          physDebugAlignNeighborsPublished,
                          physDebugAlignNeighborsDirty);
    queueDebugHostMirror (cohNeighbors,
                          physDebugCohNeighborsPending,
                          physDebugCohNeighborsPublished,
                          physDebugCohNeighborsDirty);

#if defined (LOCUSQ_TESTING) && LOCUSQ_TESTING
    {
        BoidsHostDebugSnapshot boidsDebugSnapshot;
        boidsDebugSnapshot.sampleSeq = physicsWorker.getTickCount();
        boidsDebugSnapshot.emitterSlot = activeEmitterSlot;
        boidsDebugSnapshot.flockGroupIndex = flockGroupIndex;
        boidsDebugSnapshot.activeEmitterCount = sceneGraph.getActiveEmitterCount();
        boidsDebugSnapshot.physicsEnabled = physicsEnabled;
        boidsDebugSnapshot.coordinatedWorkerActive = coordinatedWorkerActive;
        boidsDebugSnapshot.boidsEnabledForEmitter = boidsEnabledForEmitter;
        boidsDebugSnapshot.workerSlotActive = physicsWorker.isSlotActive (activeEmitterSlot);
        boidsDebugSnapshot.workerBoidsActive = boidsSystem.isInActiveGroup (activeEmitterSlot);
        boidsDebugSnapshot.workerBoidsDensity = boidsSystem.getFlockDensity (activeEmitterSlot);

        if (activeEmitterSlot >= 0 && activeEmitterSlot < kPhysicsDAWSlotCount)
        {
            const auto bridgeValues = physicsDspBridge.read (activeEmitterSlot);
            boidsDebugSnapshot.bridgeSpreadMod = bridgeValues.spreadMod;
            boidsDebugSnapshot.apvtsSpreadMirror = physSpreadModParams[activeEmitterSlot] != nullptr
                ? physSpreadModParams[activeEmitterSlot]->load()
                : 0.0f;
            boidsDebugSnapshot.pendingHostSpread = physSpreadHostPending[activeEmitterSlot].load (std::memory_order_acquire);
            boidsDebugSnapshot.publishedHostSpread = physSpreadHostPublished[activeEmitterSlot].load (std::memory_order_acquire);
        }

        boidsDebugSnapshot.sceneSpread = data.spread;
        publishBoidsHostDebugSnapshot (boidsDebugSnapshot);
    }
#endif

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

void LocusQAudioProcessor::updatePerfEma (std::atomic<float>& accumulator, double sampleMs) noexcept
{
    if (sampleMs <= 0.0)
        return;

    constexpr float alpha = 0.08f;
    const auto sample = static_cast<float> (sampleMs);
    const auto current = accumulator.load (std::memory_order_relaxed);
    const auto next = current <= 0.0f
        ? sample
        : current + (sample - current) * alpha;
    accumulator.store (next, std::memory_order_relaxed);
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

// State serialization extracted to processor_core/ProcessorStateSerializer.cpp (W0-A)
// Parameter layout extracted to Source/processor_core/ProcessorParameterLayout.cpp (W0-A)

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return std::make_unique<LocusQAudioProcessor>().release();
}
