#pragma once

#include <array>
#include <cstdint>

namespace locusq::spatial_renderer_types
{

inline constexpr int kNumSpeakers = 4;
inline constexpr int kMaxAuditionReactiveSources = 8;

enum class HeadphoneRenderMode : int
{
    StereoDownmix = 0,
    SteamBinaural = 1
};

enum class SpatialOutputProfile : int
{
    Auto = 0,
    Stereo20 = 1,
    Quad40 = 2,
    Surround521 = 3,
    Surround721 = 4,
    Surround742 = 5,
    AmbisonicFOA = 6,
    AmbisonicHOA = 7,
    AtmosBed = 8,
    Virtual3dStereo = 9,
    CodecIAMF = 10,
    CodecADM = 11
};

enum class SpatialProfileStage : int
{
    Direct = 0,
    FallbackStereo = 1,
    FallbackQuad = 2,
    AmbiDecodeStereo = 3,
    CodecLayoutPlaceholder = 4
};

enum class AmbisonicNormalization : int
{
    SN3D = 0,
    N3D = 1
};

enum class CodecMappingMode : int
{
    None = 0,
    ADM = 1,
    IAMF = 2
};

enum class SteamInitStage : int
{
    NotCompiled = 0,
    Uninitialized = 1,
    LoadingLibrary = 2,
    LibraryOpenFailed = 3,
    ResolvingSymbols = 4,
    SymbolsMissing = 5,
    CreatingContext = 6,
    ContextCreateFailed = 7,
    CreatingHRTF = 8,
    HRTFCreateFailed = 9,
    CreatingVirtualSurround = 10,
    VirtualSurroundCreateFailed = 11,
    Ready = 12
};

struct alignas(16) PoseSnapshot
{
    float qx = 0.0f;               // +0
    float qy = 0.0f;               // +4
    float qz = 0.0f;               // +8
    float qw = 1.0f;               // +12
    std::uint64_t timestampMs = 0; // +16
    std::uint32_t seq = 0;         // +24
    std::uint32_t pad = 0;         // +28
    float angVx = 0.0f;            // +32
    float angVy = 0.0f;            // +36
    float angVz = 0.0f;            // +40
    std::uint32_t sensorLocationFlags = 0; // +44
};                                 // = 48 bytes

static_assert (sizeof (PoseSnapshot) == 48, "PoseSnapshot size contract");

struct ListenerOrientation
{
    std::array<float, 3> right { 1.0f, 0.0f, 0.0f };
    std::array<float, 3> up { 0.0f, 1.0f, 0.0f };
    std::array<float, 3> ahead { 0.0f, 0.0f, -1.0f };
};

struct AuditionReactiveSnapshot
{
    float rms = 0.0f;
    float peak = 0.0f;
    float envFast = 0.0f;
    float envSlow = 0.0f;
    float onset = 0.0f;
    float brightness = 0.0f;
    float rainFadeRate = 0.0f;
    float snowFadeRate = 0.0f;
    float physicsVelocity = 0.0f;
    float physicsCollision = 0.0f;
    float physicsDensity = 0.0f;
    float physicsCoupling = 0.0f;
    float geometryScale = 0.0f;
    float geometryWidth = 0.0f;
    float geometryDepth = 0.0f;
    float geometryHeight = 0.0f;
    float precipitationFade = 0.0f;
    float collisionBurst = 0.0f;
    float densitySpread = 0.0f;
    float headphoneOutputRms = 0.0f;
    float headphoneOutputPeak = 0.0f;
    float headphoneParity = 1.0f;
    float rmsNorm = 0.0f;
    float peakNorm = 0.0f;
    float envFastNorm = 0.0f;
    float envSlowNorm = 0.0f;
    float headphoneOutputRmsNorm = 0.0f;
    float headphoneOutputPeakNorm = 0.0f;
    float headphoneParityNorm = 0.0f;
    int headphoneFallbackReasonIndex = 0;
    int sourceEnergyCount = 0;
    std::array<float, kMaxAuditionReactiveSources> sourceEnergy {};
};

struct AmbisonicIrContractSnapshot
{
    std::uint64_t frameId = 0;
    std::uint64_t timestampSamples = 0;
    int order = 0;
    int normalizationIndex = static_cast<int> (AmbisonicNormalization::SN3D);
    int channelCount = 0;
    int requestedSpatialProfileIndex = static_cast<int> (SpatialOutputProfile::Auto);
    int activeSpatialProfileIndex = static_cast<int> (SpatialOutputProfile::Auto);
    int activeSpatialStageIndex = static_cast<int> (SpatialProfileStage::Direct);
    int requestedHeadphoneModeIndex = static_cast<int> (HeadphoneRenderMode::StereoDownmix);
    int activeHeadphoneModeIndex = static_cast<int> (HeadphoneRenderMode::StereoDownmix);
    bool steamAudioAvailable = false;
    bool headphoneRenderAllowed = false;
    bool fallbackActive = false;
};

struct CodecMappingExecutionSnapshot
{
    std::uint64_t frameId = 0;
    std::uint64_t timestampSamples = 0;
    int modeIndex = static_cast<int> (CodecMappingMode::None);
    int mappedChannelCount = 0;
    int objectCount = 0;
    int elementCount = 0;
    bool mappingApplied = false;
    bool fallbackActive = false;
    bool finite = true;
    std::uint64_t signature = 0;
};

struct CodecAdmRuntimePayloadSnapshot
{
    bool active = false;
    std::uint64_t frameId = 0;
    std::uint64_t timestampSamples = 0;
    int channelCount = 0;
    int objectCount = 0;
    std::array<float, kNumSpeakers> objectGain {};
    std::array<float, kNumSpeakers> objectAzimuthDeg {};
};

struct CodecIamfRuntimePayloadSnapshot
{
    bool active = false;
    std::uint64_t frameId = 0;
    std::uint64_t timestampSamples = 0;
    int channelCount = 0;
    int elementCount = 0;
    float sceneGain = 0.0f;
    std::array<float, 2> elementGain {};
};

enum class AuditionReactiveHeadphoneFallbackReason : int
{
    None = 0,
    SteamUnavailable = 1,
    SteamRenderFailed = 2,
    OutputIncompatible = 3
};

} // namespace locusq::spatial_renderer_types
