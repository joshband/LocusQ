#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include "SceneGraph.h"
#include "VBAPPanner.h"
#include "DistanceAttenuator.h"
#include "AirAbsorption.h"
#include "DopplerProcessor.h"
#include "DirectivityFilter.h"
#include "SpreadProcessor.h"
#include "EarlyReflections.h"
#include "FDNReverb.h"
#include "headphone_dsp/HeadphoneCalibrationChain.h"
#include "headphone_dsp/HeadphonePresetLoader.h"
#include "spatial_renderer/SpatialAuditionEngine.h"
#include "spatial_renderer/SpatialAuditionPrimitives.h"
#include "spatial_renderer/SpatialEmitterRenderPass.h"
#include "spatial_renderer/SpatialHeadphonePoseAndCompensation.h"
#include "spatial_renderer/SpatialPostFxChain.h"
#include "spatial_renderer/SpatialProfileRouter.h"
#include "spatial_renderer/SpatialRendererTypes.h"
#include "spatial_renderer/SpatialSteamAudioBackend.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
 #include <phonon.h>
#else
struct IPLVector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct IPLCoordinateSpace3
{
    IPLVector3 right { 1.0f, 0.0f, 0.0f };
    IPLVector3 up { 0.0f, 1.0f, 0.0f };
    IPLVector3 ahead { 0.0f, 0.0f, -1.0f };
    IPLVector3 origin { 0.0f, 0.0f, 0.0f };
};
#endif

//==============================================================================
/**
 * SpatialRenderer - Quad Spatialization Engine
 *
 * Phase 2.2: Core spatialization processing.
 * Reads all active emitters from the SceneGraph, applies:
 *   1. VBAP panning (azimuth → 4 speaker gains)
 *   2. Distance attenuation (selected model)
 *   3. Air absorption (distance-driven LPF)
 *   4. Per-speaker delay compensation
 *   5. Per-speaker gain trims
 *   6. Master gain
 *
 * Accumulates all emitters into a quad bus, then maps to mono/stereo/quad
 * based on the negotiated host output layout.
 */
class SpatialRenderer
{
public:
    static constexpr int NUM_SPEAKERS = 4;
    static constexpr int NUM_HEADPHONE_DEVICE_PROFILES = 5;
    static constexpr int MAX_SPEAKER_DELAY_MS = 50;
    static constexpr int MAX_DELAY_SAMPLE_RATE_HZ = 192000;
    // +1 sample keeps full-delay coverage because the ring buffer clamps to (N - 1).
    static constexpr int MAX_DELAY_SAMPLES = ((MAX_SPEAKER_DELAY_MS * MAX_DELAY_SAMPLE_RATE_HZ) / 1000) + 1;
    static_assert ((MAX_DELAY_SAMPLES - 1) >= ((MAX_SPEAKER_DELAY_MS * MAX_DELAY_SAMPLE_RATE_HZ) / 1000),
                   "speaker delay buffer must preserve full max delay at 192kHz");
    static constexpr int MAX_AUDITION_REACTIVE_SOURCES = locusq::spatial_renderer_types::kMaxAuditionReactiveSources;
    using HeadphoneRenderMode = locusq::spatial_renderer_types::HeadphoneRenderMode;
    enum class HeadphoneDeviceProfile : int
    {
        Generic       = 0,
        AirPodsPro2   = 1,
        AirPodsPro3   = 2,
        SonyWH1000XM5 = 3,
        CustomSOFA    = 4
    };
    using SpatialOutputProfile = locusq::spatial_renderer_types::SpatialOutputProfile;
    using SpatialProfileStage = locusq::spatial_renderer_types::SpatialProfileStage;
    using AmbisonicNormalization = locusq::spatial_renderer_types::AmbisonicNormalization;
    using CodecMappingMode = locusq::spatial_renderer_types::CodecMappingMode;
    using SteamInitStage = locusq::spatial_renderer_types::SteamInitStage;

    // Internal speaker order (VBAP / accumulation): FL, FR, RR, RL.
    static constexpr std::array<int, NUM_SPEAKERS> kQuadOutputSpeakerOrder
    {
        0, 1, 3, 2 // Host quad output order: FL, FR, RL, RR
    };
    using PoseSnapshot = locusq::spatial_renderer_types::PoseSnapshot;
    using ListenerOrientation = locusq::spatial_renderer_types::ListenerOrientation;
    using AuditionReactiveSnapshot = locusq::spatial_renderer_types::AuditionReactiveSnapshot;
    using AmbisonicIrContractSnapshot = locusq::spatial_renderer_types::AmbisonicIrContractSnapshot;
    using CodecMappingExecutionSnapshot = locusq::spatial_renderer_types::CodecMappingExecutionSnapshot;
    using CodecAdmRuntimePayloadSnapshot = locusq::spatial_renderer_types::CodecAdmRuntimePayloadSnapshot;
    using CodecIamfRuntimePayloadSnapshot = locusq::spatial_renderer_types::CodecIamfRuntimePayloadSnapshot;
    using AuditionReactiveHeadphoneFallbackReason = locusq::spatial_renderer_types::AuditionReactiveHeadphoneFallbackReason;

    SpatialRenderer();
    ~SpatialRenderer();

    //==========================================================================
    void prepare (double sampleRate, int maxBlockSize);

    void reset();

    void shutdown() noexcept;

    //==========================================================================
    // Set renderer parameters (called from processBlock before process())
    //==========================================================================

    void setDistanceModel (int modelIndex);

    void setReferenceDistance (float refDist);

    void setMaxDistance (float maxDist);

    void setAirAbsorptionEnabled (bool enabled);

    void setDopplerEnabled (bool enabled);

    void setDopplerScale (float scale);

    void setRoomEnabled (bool enabled);

    void setRoomMix (float newMix);

    void setRoomSize (float newSize);

    void setRoomDamping (float newDamping);

    void setEarlyReflectionsOnly (bool enabled);

    void setQualityTier (int qualityIndex);

    void setMasterGain (float gainDb);

    void setSpeakerTrim (int speakerIdx, float trimDb);

    void setSpeakerDelay (int speakerIdx, float delayMs);

    void setHeadphoneRenderMode (int modeIndex);

    void setHeadphoneDeviceProfile (int profileIndex);

    // Message-thread preset build + atomic publish into the PEQ monitoring chain.
    void loadPeqPresetForProfile (int profileIndex, double sampleRate);

    // Apply PEQ bands from a JSON-parsed var array (companion IPC path).
    // preampDb = 0.0 if the JSON schema has no preamp field.
    // Called on the message thread; coefficients are atomically published to the audio thread.
    void applyJsonPeqBands (const juce::var& bandsArray, float preampDb, double sampleRate);

    void clearFirImpulseResponse() noexcept;

    bool loadFirImpulseResponse (const float* taps, int tapCount) noexcept;

    void setHeadphoneCalibrationEnabled (bool enabled) noexcept;

    void setHeadphoneCalibrationEngine (int engineIndex) noexcept;

    int getCalibrationLatencySamples() const noexcept;

    void setSpatialOutputProfile (int profileIndex);

    void applyHeadPose (const PoseSnapshot& pose) noexcept;

    void clearHeadPose() noexcept;

    void setRequestedSofaHrtf (juce::String sofaRefRelativePath, bool enabled);

    bool reloadSteamAudioRuntime() noexcept;

    bool isUsingSofaHrtf() const noexcept;

    bool loadFirTapsFromJson (const juce::var& tapsArray) noexcept;

    void setAuditionEnabled (bool enabled) noexcept;

    void setAuditionSignalType (int signalTypeIndex) noexcept;

    void setAuditionMotionType (int motionTypeIndex) noexcept;

    void setAuditionLevelPreset (int levelPresetIndex) noexcept;

    void setAuditionPhysicsReactiveInput (
        bool active,
        float velocityNorm,
        float collisionNorm,
        float densityNorm) noexcept;

    int getHeadphoneRenderModeRequestedIndex() const noexcept;

    int getHeadphoneRenderModeActiveIndex() const noexcept;

    int getHeadphoneDeviceProfileRequestedIndex() const noexcept;

    int getHeadphoneDeviceProfileActiveIndex() const noexcept;

    bool isHeadphoneCalibrationEnabledRequested() const noexcept;

    int getHeadphoneCalibrationEngineRequestedIndex() const noexcept;

    int getHeadphoneCalibrationEngineActiveIndex() const noexcept;

    int getHeadphoneCalibrationFallbackReasonIndex() const noexcept;

    int getHeadphoneCalibrationLatencySamples() const noexcept;

    int getSpatialOutputProfileRequestedIndex() const noexcept;

    int getSpatialOutputProfileActiveIndex() const noexcept;

    int getSpatialProfileStageIndex() const noexcept;

    AmbisonicIrContractSnapshot getAmbisonicIrContractSnapshot() const noexcept;

    CodecMappingExecutionSnapshot getCodecMappingExecutionSnapshot() const noexcept;

    CodecAdmRuntimePayloadSnapshot getCodecAdmRuntimePayloadSnapshot() const noexcept;

    CodecIamfRuntimePayloadSnapshot getCodecIamfRuntimePayloadSnapshot() const noexcept;

    bool isSteamAudioAvailable() const noexcept;

    bool isSteamAudioCompiled() const noexcept;

    int getSteamAudioInitStageIndex() const noexcept;

    int getSteamAudioInitErrorCode() const noexcept;

    juce::String getSteamAudioRuntimeLibraryPath() const;

    juce::String getSteamAudioMissingSymbolName() const;

    static const char* headphoneRenderModeToString (int modeIndex) noexcept;

    static const char* steamAudioInitStageToString (int stageIndex) noexcept;

    static const char* auditionReactiveHeadphoneFallbackReasonToString (int reasonIndex) noexcept;

    static const char* headphoneDeviceProfileToString (int profileIndex) noexcept;

    static const char* headphoneCalibrationEngineToString (int engineIndex) noexcept;

    static const char* headphoneCalibrationFallbackReasonToString (int reasonIndex) noexcept;

    static const char* spatialOutputProfileToString (int profileIndex) noexcept;

    static const char* spatialProfileStageToString (int stageIndex) noexcept;

    static const char* ambisonicNormalizationToString (int normalizationIndex) noexcept;

    static const char* codecMappingModeToString (int modeIndex) noexcept;

    //==========================================================================
    // BL-052: RT-safe quad-to-binaural for calibration monitoring
    //==========================================================================
    // Apply virtual surround to external quad input (Steam canonical channel order
    // FL, FR, RL, RR). outL and outR must point to caller-allocated arrays of at
    // least numSamples floats. Returns true if rendering succeeded; false when
    // Steam Audio is unavailable (caller should apply a fallback path).
    // listenerOrientation is optional; when supplied, the quad bed is first
    // rotated into the listener frame represented by IPLCoordinateSpace3.
    //
    // RT-safe: no allocation. quadChannels must remain valid for the duration of
    // the call. Concurrent calls from multiple threads are not safe.
    bool renderVirtualSurroundForMonitoring (const float* const* quadChannels,
                                             float* outL,
                                             float* outR,
                                             int numSamples,
                                             const IPLCoordinateSpace3* listenerOrientation = nullptr) noexcept;

    //==========================================================================
    // Main processing: read scene graph, spatialize active emitters, output
    //==========================================================================
    void process (juce::AudioBuffer<float>& outputBuffer, const SceneGraph& scene);

    int getLastEligibleEmitterCount() const noexcept;

    int getLastProcessedEmitterCount() const noexcept;

    int getLastBudgetCulledEmitterCount() const noexcept;

    int getLastActivityCulledEmitterCount() const noexcept;

    bool wasGuardrailActiveLastBlock() const noexcept;

    bool isAuditionVisualActive() const noexcept;

    float getAuditionVisualX() const noexcept;

    float getAuditionVisualY() const noexcept;

    float getAuditionVisualZ() const noexcept;

    AuditionReactiveSnapshot getAuditionReactiveSnapshot() const noexcept;

private:
    static float sanitizeUnitScalar (float value, float fallback = 0.0f) noexcept;

    static int sanitizeSourceCount (int value) noexcept;

    static int sanitizeHeadphoneFallbackReasonIndex (int value) noexcept;

    static constexpr int MAX_TRACKED_EMITTERS = 64; // Per-emitter smoothing/filtering
    static constexpr int MAX_RENDER_EMITTERS_PER_BLOCK = 8; // v1-tested CPU envelope
    static constexpr float COARSE_PRIORITY_GATE_LINEAR = 1.0e-5f; // ~ -100 dB
    static constexpr float ACTIVITY_PEAK_GATE_LINEAR = 1.0e-6f;   // ~ -120 dB
    static constexpr int AUDITION_MAX_VOICES = MAX_AUDITION_REACTIVE_SOURCES;
    static constexpr int AUDITION_HISTORY_BUFFER_SAMPLES = 8192;

    struct EmitterStageResult
    {
        int eligibleEmitterCount = 0;
        int budgetCulledEmitterCount = 0;
        int activityCulledEmitterCount = 0;
        int processedEmitterCount = 0;
        bool renderedAuditionEmitter = false;
    };

    struct EmitterCandidate
    {
        int slotIdx = -1;
        EmitterData data {};
        float distance = 0.0f;
        float distanceGain = 0.0f;
        float emitterGainLinear = 0.0f;
        float priority = 0.0f;
    };

    void processSelectedEmitterCandidate (
        const SceneGraph& scene,
        const EmitterCandidate& candidate,
        int numSamples,
        EmitterStageResult& result);

    void collectEmitterCandidatesForBlock (
        const SceneGraph& scene,
        std::array<EmitterCandidate, MAX_RENDER_EMITTERS_PER_BLOCK>& selectedEmitters,
        int& selectedEmitterCount,
        int& selectedMinPriorityIndex,
        float& selectedMinPriority,
        EmitterStageResult& result);

    void finalizeEmitterStageWithAuditionFallback (EmitterStageResult& result, int numSamples);

    void processSelectedEmittersForBlock (
        const SceneGraph& scene,
        const std::array<EmitterCandidate, MAX_RENDER_EMITTERS_PER_BLOCK>& selectedEmitters,
        int selectedEmitterCount,
        int numSamples,
        EmitterStageResult& result);

    EmitterStageResult runEmitterAccumulationStage (const SceneGraph& scene, int numSamples);

    void applyRoomAndSpeakerPostFx (int numSamples);

    void publishCodecAdmPayloadContract (
        bool codecAdmPayloadActiveThisBlock,
        std::uint64_t codecFrameId,
        std::uint64_t contractTimestampSamples,
        int codecMappedChannelCount,
        int codecObjectCount);

    void publishCodecIamfPayloadContract (
        bool codecIamfPayloadActiveThisBlock,
        std::uint64_t codecFrameId,
        std::uint64_t contractTimestampSamples,
        int codecMappedChannelCount,
        int codecElementCount);

    int determineCodecMappedChannelCount (CodecMappingMode codecMode, int numOutputChannels) const noexcept;

    CodecMappingMode determineCodecModeForProfile (SpatialOutputProfile requestedSpatialProfile) const noexcept;

    int determineCodecObjectCount (CodecMappingMode codecMode, int codecMappedChannelCount) const noexcept;

    int determineCodecElementCount (CodecMappingMode codecMode, int codecMappedChannelCount) const noexcept;

    bool isCodecMappingFiniteForBlock (bool codecMappingAppliedThisBlock, int numSamples) const noexcept;

    std::uint64_t buildCodecMappingSignature (
        std::uint64_t codecFrameId,
        std::uint64_t contractTimestampSamples,
        int codecMappedChannelCount,
        int codecObjectCount,
        int codecElementCount) const noexcept;

    void publishCodecMappingContractState (
        std::uint64_t contractTimestampSamples,
        CodecMappingMode codecMode,
        int codecMappedChannelCount,
        int codecObjectCount,
        int codecElementCount,
        bool codecMappingAppliedThisBlock,
        bool codecMappingFallbackActiveThisBlock,
        bool codecMappingFiniteThisBlock,
        std::uint64_t codecSignature);

    std::uint64_t publishAmbisonicIrContractState (
        SpatialOutputProfile requestedSpatialProfile,
        SpatialOutputProfile activeSpatialProfile,
        bool profileAllowsHeadphoneRender,
        int numSamples);

    void publishAmbisonicAndCodecTelemetryContracts (
        int numSamples,
        int numOutputChannels,
        SpatialOutputProfile activeSpatialProfile,
        SpatialProfileStage activeSpatialStage,
        bool profileAllowsHeadphoneRender);

    struct AuditionHeadphoneParityAccumulator
    {
        double outputEnergy = 0.0;
        double referenceEnergy = 0.0;
        float peak = 0.0f;
        bool samplesCaptured = false;
        int fallbackReasonIndex = static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None);
    };

    int determineAuditionHeadphoneFallbackReason (
        bool renderedAuditionEmitter,
        HeadphoneRenderMode requestedHeadphoneMode,
        int numOutputChannels,
        bool profileAllowsHeadphoneRender,
        bool steamBackendAvailable,
        bool steamRenderedThisBlock,
        HeadphoneRenderMode activeHeadphoneMode) const noexcept;

    void accumulateAuditionHeadphoneParitySample (
        AuditionHeadphoneParityAccumulator& parity,
        float left,
        float right,
        bool referenceCaptured,
        float referenceLeft,
        float referenceRight) noexcept;

    void finalizeAuditionHeadphoneParity (
        bool renderedAuditionEmitter,
        int numSamples,
        const AuditionHeadphoneParityAccumulator& parity) noexcept;

    bool writeDiscreteOrAmbisonicOutputSample (
        juce::AudioBuffer<float>& outputBuffer,
        int sampleIndex,
        int numOutputChannels,
        SpatialOutputProfile activeSpatialProfile,
        float masterGain) const noexcept;

    struct StereoOutputSample
    {
        float left = 0.0f;
        float right = 0.0f;
        float referenceLeft = 0.0f;
        float referenceRight = 0.0f;
        bool referenceCaptured = false;
    };

    StereoOutputSample renderStereoOutputSample (
        int sampleIndex,
        SpatialOutputProfile activeSpatialProfile,
        bool steamRenderedThisBlock,
        HeadphoneRenderMode activeHeadphoneMode) const noexcept;

    void writeMonoOutputSample (
        juce::AudioBuffer<float>& outputBuffer,
        int sampleIndex,
        float masterGain) const noexcept;

    struct HeadphoneRuntimeState
    {
        HeadphoneRenderMode requestedMode = HeadphoneRenderMode::StereoDownmix;
        HeadphoneRenderMode activeMode = HeadphoneRenderMode::StereoDownmix;
        bool steamBackendAvailable = false;
        bool profileAllowsHeadphoneRender = false;
        bool steamRenderedThisBlock = false;
    };

    struct OutputRoutingStageContext
    {
        locusq::spatial_profile_router::SpatialProfileResolution profileResolution;
        SpatialOutputProfile activeSpatialProfile;
        HeadphoneRuntimeState headphoneState;
    };

    void applyRequestedHeadphoneCalibrationSettings();

    void publishHeadphoneCalibrationRuntimeState (bool includeRequestedEngineIndex);

    HeadphoneRuntimeState configureHeadphoneRuntime (
        int numSamples,
        int numOutputChannels,
        SpatialOutputProfile activeSpatialProfile);

    OutputRoutingStageContext prepareOutputRoutingStageContext (int numSamples, int numOutputChannels);

    void writeStereoOutputSample (
        juce::AudioBuffer<float>& outputBuffer,
        int sampleIndex,
        float masterGain,
        SpatialOutputProfile activeSpatialProfile,
        const HeadphoneRuntimeState& headphoneState,
        bool renderedAuditionEmitter,
        AuditionHeadphoneParityAccumulator& headphoneParity);

    void writeOutputSampleForChannelLayout (
        juce::AudioBuffer<float>& outputBuffer,
        int sampleIndex,
        int numOutputChannels,
        float masterGain,
        const OutputRoutingStageContext& outputContext,
        bool renderedAuditionEmitter,
        AuditionHeadphoneParityAccumulator& headphoneParity);

    AuditionHeadphoneParityAccumulator prepareAuditionHeadphoneParityAccumulator (
        bool renderedAuditionEmitter,
        int numOutputChannels,
        const HeadphoneRuntimeState& headphoneState) const noexcept;

    void publishAuditionHeadphoneParityForBlock (
        bool renderedAuditionEmitter,
        int numSamples,
        const AuditionHeadphoneParityAccumulator& parity) noexcept;

    void runOutputRoutingAndHeadphoneStage (
        juce::AudioBuffer<float>& outputBuffer,
        int numSamples,
        int numOutputChannels,
        bool renderedAuditionEmitter);

    struct PreparedScratchBuffer
    {
        void prepare (int requestedSamples)
        {
            const auto requestedSize = static_cast<std::size_t> (juce::jmax (0, requestedSamples));

            if (requestedSize != size_)
            {
                data_.malloc (requestedSize, true);
                size_ = requestedSize;
            }
            else
            {
                clear();
            }
        }

        void clear() noexcept
        {
            if (size_ > 0)
                std::fill_n (data_.get(), size_, 0.0f);
        }

        int size() const noexcept
        {
            return static_cast<int> (size_);
        }

        float* data() noexcept { return data_.get(); }
        const float* data() const noexcept { return data_.get(); }

        float* begin() noexcept { return data_.get(); }
        const float* begin() const noexcept { return data_.get(); }

        float* end() noexcept { return data_.get() + size_; }
        const float* end() const noexcept { return data_.get() + size_; }

        float& operator[] (std::size_t index) noexcept { return data_[index]; }
        const float& operator[] (std::size_t index) const noexcept { return data_[index]; }

    private:
        juce::HeapBlock<float> data_;
        std::size_t size_ = 0;
    };

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // DSP components
    VBAPPanner vbapPanner;
    DistanceAttenuator distanceAttenuator;
    SpreadProcessor spreadProcessor;
    DirectivityFilter directivityFilter;

    // Per-emitter air absorption filters
    std::array<AirAbsorption, MAX_TRACKED_EMITTERS> emitterAbsorption;
    std::array<DopplerProcessor, MAX_TRACKED_EMITTERS> emitterDoppler;

    // Per-emitter smoothed speaker gains (for click-safe panning)
    std::array<std::array<juce::SmoothedValue<float>, NUM_SPEAKERS>, MAX_TRACKED_EMITTERS> smoothedSpeakerGains;
    std::array<std::array<juce::SmoothedValue<float>, NUM_SPEAKERS>, AUDITION_MAX_VOICES> auditionSmoothedSpeakerGains;

    // Speaker delay lines
    std::array<std::array<float, MAX_DELAY_SAMPLES>, NUM_SPEAKERS> speakerDelayLines {};
    std::array<int, NUM_SPEAKERS> delayWritePos {};
    std::array<int, NUM_SPEAKERS> speakerDelaySamples {};

    // Speaker trim gains
    std::array<juce::SmoothedValue<float>, NUM_SPEAKERS> smoothedSpeakerTrim;

    // Master gain
    juce::SmoothedValue<float> smoothedMasterGain { 1.0f };

    // Air absorption toggle
    bool airAbsorptionEnabled = true;
    bool dopplerEnabled = false;
    float dopplerScale = 1.0f;
    bool qualityHigh = false;
    int distanceModelIndex = 0;
    float referenceDistance = 1.0f;
    float maxDistance = 50.0f;
    bool auditionEnabled = false;
    int auditionSignalTypeIndex = 0;
    int auditionMotionTypeIndex = 0;
    int auditionLevelPresetIndex = 2;
    double auditionPhasePrimary = 0.0;
    double auditionPhaseSecondary = 0.0;
    double auditionOrbitPhase = 0.0;
    uint32_t auditionNoiseState = 0x13579BDFu;
    float auditionNoiseOnePole = 0.0f;
    float auditionRainBed = 0.0f;
    float auditionRainDropEnv = 0.0f;
    float auditionRainDropFreqHz = 1300.0f;
    double auditionRainDropPhase = 0.0;
    float auditionSnowBed = 0.0f;
    float auditionSnowShimmer = 0.0f;
    double auditionSnowFlutterPhase = 0.0;
    float auditionBounceEnv = 0.0f;
    float auditionBounceFreqHz = 320.0f;
    double auditionBouncePhase = 0.0;
    int auditionBounceClusterRemaining = 0;
    int auditionBounceCountdownSamples = 0;
    int auditionBounceCooldownSamples = 0;
    float auditionBounceSpacingSamples = 0.0f;
    float auditionChimeEnv = 0.0f;
    float auditionChimeFreqA = 659.25f;
    float auditionChimeFreqB = 987.77f;
    double auditionChimePhaseA = 0.0;
    double auditionChimePhaseB = 0.0;
    float auditionChimeShimmer = 0.0f;
    int auditionChimeCooldownSamples = 0;
    float auditionCricketEnv = 0.0f;
    float auditionCricketFreqHz = 4200.0f;
    double auditionCricketPhase = 0.0;
    int auditionCricketBurstSamples = 0;
    int auditionCricketCooldownSamples = 0;
    float auditionBirdEnv = 0.0f;
    float auditionBirdFreqA = 1400.0f;
    float auditionBirdFreqB = 2200.0f;
    double auditionBirdPhaseA = 0.0;
    double auditionBirdPhaseB = 0.0;
    double auditionBirdWarblePhase = 0.0;
    int auditionBirdPhraseSamples = 0;
    int auditionBirdCooldownSamples = 0;
    static constexpr int kAuditionKarplusMaxDelaySamples = 4096;
    std::array<float, kAuditionKarplusMaxDelaySamples> auditionKarplusDelayLine {};
    int auditionKarplusDelaySamples = 620;
    int auditionKarplusWriteIndex = 0;
    float auditionKarplusDamping = 0.985f;
    float auditionKarplusEnv = 0.0f;
    int auditionKarplusCooldownSamples = 0;
    float auditionMembraneEnv = 0.0f;
    float auditionMembraneFreqA = 180.0f;
    float auditionMembraneFreqB = 280.0f;
    double auditionMembranePhaseA = 0.0;
    double auditionMembranePhaseB = 0.0;
    int auditionMembraneCooldownSamples = 0;
    float auditionKrellEnv = 0.0f;
    float auditionKrellFreqCurrent = 220.0f;
    float auditionKrellFreqTarget = 220.0f;
    double auditionKrellPhase = 0.0;
    int auditionKrellStepSamples = 0;
    float auditionArpEnv = 0.0f;
    float auditionArpFreqA = 330.0f;
    float auditionArpFreqB = 495.0f;
    double auditionArpPhaseA = 0.0;
    double auditionArpPhaseB = 0.0;
    int auditionArpGateSamples = 0;
    int auditionArpStepIndex = 0;
    float auditionWallPosX = 0.0f;
    float auditionWallPosZ = -1.0f;
    float auditionWallVelX = 0.92f;
    float auditionWallVelZ = 0.71f;
    std::array<float, AUDITION_HISTORY_BUFFER_SAMPLES> auditionHistoryBuffer {};
    int auditionHistoryWritePos = 0;
    std::array<double, AUDITION_MAX_VOICES> auditionVoiceModPhase {};
    std::array<double, AUDITION_MAX_VOICES> auditionVoiceExciterPhaseA {};
    std::array<double, AUDITION_MAX_VOICES> auditionVoiceExciterPhaseB {};
    std::array<float, AUDITION_MAX_VOICES> auditionVoiceExciterEnv {};
    std::array<int, AUDITION_MAX_VOICES> auditionVoiceExciterCooldownSamples {};
    std::array<std::uint32_t, AUDITION_MAX_VOICES> auditionVoiceNoiseState {};
    std::atomic<bool> auditionVisualActive { false };
    std::atomic<float> auditionVisualX { 0.0f };
    std::atomic<float> auditionVisualY { 1.2f };
    std::atomic<float> auditionVisualZ { -1.0f };
    float auditionReactiveEnvFastState = 0.0f;
    float auditionReactiveEnvSlowState = 0.0f;
    float auditionReactiveBrightnessLowpassState = 0.0f;
    bool auditionPhysicsReactiveInputActive = false;
    float auditionPhysicsReactiveVelocityTarget = 0.0f;
    float auditionPhysicsReactiveCollisionTarget = 0.0f;
    float auditionPhysicsReactiveDensityTarget = 0.0f;
    float auditionPhysicsReactiveVelocityState = 0.0f;
    float auditionPhysicsReactiveCollisionState = 0.0f;
    float auditionPhysicsReactiveDensityState = 0.0f;
    float auditionPhysicsReactiveTimbreLowpassState = 0.0f;
    std::atomic<float> auditionReactiveRms { 0.0f };
    std::atomic<float> auditionReactivePeak { 0.0f };
    std::atomic<float> auditionReactiveEnvFast { 0.0f };
    std::atomic<float> auditionReactiveEnvSlow { 0.0f };
    std::atomic<float> auditionReactiveOnset { 0.0f };
    std::atomic<float> auditionReactiveBrightness { 0.0f };
    std::atomic<float> auditionReactiveRainFadeRate { 0.0f };
    std::atomic<float> auditionReactiveSnowFadeRate { 0.0f };
    std::atomic<float> auditionReactivePhysicsVelocity { 0.0f };
    std::atomic<float> auditionReactivePhysicsCollision { 0.0f };
    std::atomic<float> auditionReactivePhysicsDensity { 0.0f };
    std::atomic<float> auditionReactivePhysicsCoupling { 0.0f };
    std::atomic<float> auditionReactiveHeadphoneOutputRms { 0.0f };
    std::atomic<float> auditionReactiveHeadphoneOutputPeak { 0.0f };
    std::atomic<float> auditionReactiveHeadphoneParity { 1.0f };
    std::atomic<int> auditionReactiveHeadphoneFallbackReasonIndex {
        static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None)
    };
    std::atomic<int> auditionReactiveSourceCount { 0 };
    std::array<std::atomic<float>, AUDITION_MAX_VOICES> auditionReactiveSourceEnergy {};
    float masterGainDb = std::numeric_limits<float>::quiet_NaN();
    std::array<float, NUM_SPEAKERS> speakerTrimDb {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()
    };

    // Room acoustics
    bool roomEnabled = true;
    bool earlyReflectionsOnly = false;
    float roomMix = 0.3f;
    float roomSize = 1.0f;
    float roomDamping = 0.5f;
    EarlyReflections earlyReflections;
    FDNReverb fdnReverb;

    // Accumulation buffer (4 channels, one per speaker)
    juce::AudioBuffer<float> accumBuffer;

    // Temp buffer for mono downmix of emitter audio
    PreparedScratchBuffer tempMonoBuffer;

    // Per-block guardrail stats (read on non-audio threads for diagnostics/UI).
    std::atomic<int> lastEligibleEmitterCount { 0 };
    std::atomic<int> lastProcessedEmitterCount { 0 };
    std::atomic<int> lastBudgetCulledEmitterCount { 0 };
    std::atomic<int> lastActivityCulledEmitterCount { 0 };
    std::atomic<bool> lastGuardrailActive { false };
    std::atomic<int> requestedHeadphoneModeIndex { static_cast<int> (HeadphoneRenderMode::StereoDownmix) };
    std::atomic<int> activeHeadphoneModeIndex { static_cast<int> (HeadphoneRenderMode::StereoDownmix) };
    std::atomic<int> requestedHeadphoneProfileIndex { static_cast<int> (HeadphoneDeviceProfile::Generic) };
    std::atomic<int> activeHeadphoneProfileIndex { static_cast<int> (HeadphoneDeviceProfile::Generic) };
    std::atomic<bool> requestedHeadphoneCalibrationEnabled { false };
    std::atomic<int> requestedHeadphoneCalibrationEngineIndex {
        static_cast<int> (locusq::headphone_core::CalibrationChainEngine::Disabled)
    };
    std::atomic<int> activeHeadphoneCalibrationEngineIndex {
        static_cast<int> (locusq::headphone_core::CalibrationChainEngine::Disabled)
    };
    std::atomic<int> activeHeadphoneCalibrationFallbackReasonIndex {
        static_cast<int> (locusq::headphone_core::CalibrationChainFallbackReason::DisabledByRequest)
    };
    std::atomic<int> activeHeadphoneCalibrationLatencySamples { 0 };
    std::atomic<int> requestedSpatialProfileIndex { static_cast<int> (SpatialOutputProfile::Auto) };
    std::atomic<int> activeSpatialProfileIndex { static_cast<int> (SpatialOutputProfile::Auto) };
    std::atomic<int> activeSpatialStageIndex { static_cast<int> (SpatialProfileStage::Direct) };
    std::atomic<std::uint64_t> ambisonicIrFrameId { 0 };
    std::atomic<std::uint64_t> ambisonicIrTimestampSamples { 0 };
    std::atomic<std::uint64_t> ambisonicIrSampleCursor { 0 };
    std::atomic<int> ambisonicIrOrder { 0 };
    std::atomic<int> ambisonicIrNormalizationIndex { static_cast<int> (AmbisonicNormalization::SN3D) };
    std::atomic<int> ambisonicIrChannelCount { 0 };
    std::atomic<bool> ambisonicIrHeadphoneRenderAllowed { false };
    std::atomic<bool> ambisonicIrFallbackActive { false };
    std::atomic<std::uint64_t> codecMappingFrameId { 0 };
    std::atomic<std::uint64_t> codecMappingTimestampSamples { 0 };
    std::atomic<int> codecMappingModeIndex { static_cast<int> (CodecMappingMode::None) };
    std::atomic<int> codecMappingMappedChannelCount { 0 };
    std::atomic<int> codecMappingObjectCount { 0 };
    std::atomic<int> codecMappingElementCount { 0 };
    std::atomic<bool> codecMappingApplied { false };
    std::atomic<bool> codecMappingFallbackActive { false };
    std::atomic<bool> codecMappingFinite { true };
    std::atomic<std::uint64_t> codecMappingSignature { 0 };
    std::atomic<bool> codecAdmPayloadActive { false };
    std::atomic<std::uint64_t> codecAdmPayloadFrameId { 0 };
    std::atomic<std::uint64_t> codecAdmPayloadTimestampSamples { 0 };
    std::atomic<int> codecAdmPayloadChannelCount { 0 };
    std::atomic<int> codecAdmPayloadObjectCount { 0 };
    std::array<std::atomic<float>, NUM_SPEAKERS> codecAdmPayloadObjectGain {};
    std::array<std::atomic<float>, NUM_SPEAKERS> codecAdmPayloadObjectAzimuthDeg {};
    std::atomic<bool> codecIamfPayloadActive { false };
    std::atomic<std::uint64_t> codecIamfPayloadFrameId { 0 };
    std::atomic<std::uint64_t> codecIamfPayloadTimestampSamples { 0 };
    std::atomic<int> codecIamfPayloadChannelCount { 0 };
    std::atomic<int> codecIamfPayloadElementCount { 0 };
    std::atomic<float> codecIamfPayloadSceneGain { 0.0f };
    std::array<std::atomic<float>, 2> codecIamfPayloadElementGain {};
    std::atomic<bool> steamAudioAvailable { false };
    std::atomic<int> steamInitStageIndex { static_cast<int> (SteamInitStage::NotCompiled) };
    std::atomic<int> steamInitErrorCode { 0 };
    mutable juce::SpinLock steamDiagnosticsLock;
    juce::String steamRuntimeLibraryPath;
    juce::String steamMissingSymbolName;

    // Steam Audio scratch/output buffers reused each block.
    PreparedScratchBuffer steamBinauralLeft;
    PreparedScratchBuffer steamBinauralRight;
    std::array<PreparedScratchBuffer, NUM_SPEAKERS> headPoseRotatedQuadScratch;
    std::array<PreparedScratchBuffer, NUM_SPEAKERS> monitoringHeadPoseRotatedQuadScratch_;
    std::array<std::array<float, NUM_SPEAKERS>, NUM_SPEAKERS> headPoseSpeakerMix {};
    std::array<std::array<float, NUM_SPEAKERS>, NUM_SPEAKERS> monitoringSpeakerMix_ {};
    PoseSnapshot headPoseSnapshot {};
    ListenerOrientation headPoseOrientation {};
    bool headPoseValid = false;
    bool headPoseInternalBinauralActive = false;
    float headphoneCompLowAlpha = 0.0f;
    float headphoneCompLowGain = 1.0f;
    float headphoneCompHighGain = 1.0f;
    float headphoneCompCrossfeed = 0.0f;
    float headphoneCompLowStateLeft = 0.0f;
    float headphoneCompLowStateRight = 0.0f;
    struct BundledPeqPresetCacheEntry
    {
        locusq::headphone_dsp::HeadphonePreset preset;
    };
    std::array<BundledPeqPresetCacheEntry, NUM_HEADPHONE_DEVICE_PROFILES> bundledPeqPresets {};
    int    lastAppliedHeadphoneProfileIndex = -1;
    int    lastLoadedPeqPresetIndex         = -1;
    double lastLoadedPeqSampleRate          = 0.0;
    juce::String requestedSofaRefRelativePath;
    bool requestedSofaHrtfEnabled = false;
    locusq::headphone_dsp::HeadphoneCalibrationChain headphoneCalibrationChain;

#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
    using IplContextCreateFn = IPLerror (IPLCALL*) (IPLContextSettings*, IPLContext*);
    using IplContextReleaseFn = void (IPLCALL*) (IPLContext*);
    using IplHRTFCreateFn = IPLerror (IPLCALL*) (IPLContext, IPLAudioSettings*, IPLHRTFSettings*, IPLHRTF*);
    using IplHRTFReleaseFn = void (IPLCALL*) (IPLHRTF*);
    using IplVirtualSurroundEffectCreateFn = IPLerror (IPLCALL*) (IPLContext, IPLAudioSettings*, IPLVirtualSurroundEffectSettings*, IPLVirtualSurroundEffect*);
    using IplVirtualSurroundEffectReleaseFn = void (IPLCALL*) (IPLVirtualSurroundEffect*);
    using IplVirtualSurroundEffectResetFn = void (IPLCALL*) (IPLVirtualSurroundEffect);
    using IplVirtualSurroundEffectApplyFn = IPLAudioEffectState (IPLCALL*) (IPLVirtualSurroundEffect, IPLVirtualSurroundEffectParams*, IPLAudioBuffer*, IPLAudioBuffer*);

    juce::DynamicLibrary steamAudioLibrary;
    IplContextCreateFn iplContextCreateFn = nullptr;
    IplContextReleaseFn iplContextReleaseFn = nullptr;
    IplHRTFCreateFn iplHRTFCreateFn = nullptr;
    IplHRTFReleaseFn iplHRTFReleaseFn = nullptr;
    IplVirtualSurroundEffectCreateFn iplVirtualSurroundEffectCreateFn = nullptr;
    IplVirtualSurroundEffectReleaseFn iplVirtualSurroundEffectReleaseFn = nullptr;
    IplVirtualSurroundEffectResetFn iplVirtualSurroundEffectResetFn = nullptr;
    IplVirtualSurroundEffectApplyFn iplVirtualSurroundEffectApplyFn = nullptr;

    IPLContext steamContext = nullptr;
    IPLHRTF steamHrtf = nullptr;
    IPLVirtualSurroundEffect steamVirtualSurroundEffect = nullptr;
    std::array<float*, NUM_SPEAKERS> steamInputChannelPtrs {};
    std::array<float*, 2> steamOutputChannelPtrs {};
    // BL-052: dedicated pointer arrays for renderVirtualSurroundForMonitoring.
    // Kept separate from steamInputChannelPtrs/steamOutputChannelPtrs so
    // renderSteamBinauralBlock and monitoring renders remain independent.
    std::array<float*, NUM_SPEAKERS> monitoringInputPtrs_ {};
    std::array<float*, 2>            monitoringOutputPtrs_ {};
#endif

    bool steamAudioRuntimeReady = false;
    bool steamAudioUsingSofaHrtf = false;

    static constexpr std::array<float, NUM_SPEAKERS> kCodecAdmObjectDefaultGains
    {
        1.0f, 1.0f, 1.0f, 1.0f
    };
    static constexpr std::array<float, NUM_SPEAKERS> kCodecAdmObjectAzimuthDeg
    {
        -45.0f, 45.0f, 135.0f, -135.0f
    };
    static constexpr std::array<float, 2> kCodecIamfDefaultElementGains
    {
        0.70710678f, 0.70710678f
    };

    static juce::String getBundledPeqPresetFilenameForProfile (HeadphoneDeviceProfile profile);

    juce::File resolveBundledPeqPresetFile (const juce::String& presetFilename) const;

    void preloadBundledPeqPresets();

    static bool tryBuildListenerOrientationFromCoordinateSpace (const IPLCoordinateSpace3& coordinateSpace,
                                                                ListenerOrientation& orientation) noexcept;

    void setHeadPoseIdentityMix() noexcept;

    void resetHeadPoseState() noexcept;

    // Quaternion follows Steam canonical axes (right +X, up +Y, ahead -Z).
    void updateHeadPoseOrientationFromSnapshot() noexcept;

    void rebuildHeadPoseSpeakerMix() noexcept;

    void getHeadPoseAdjustedQuadSample (int sampleIndex, float& fl, float& fr, float& rr, float& rl) const noexcept;

    //==========================================================================
    // Coordinate helpers
    //==========================================================================

    static float auditionLevelDbForPreset (int presetIndex) noexcept;

    float advanceAuditionOscillator (double frequencyHz, double& phase) const noexcept;

    float nextAuditionWhiteNoise() noexcept;

    float nextAuditionRand01() noexcept;

    static float wrapAuditionAzimuthDegrees (float azimuthDeg) noexcept;

    static float auditionVoiceHashUnit (int voiceIndex, std::uint32_t salt) noexcept;

    void resetAuditionVoiceFieldStates() noexcept;

    bool isAuditionCloudBoundModeAvailable() const noexcept;

    float renderAuditionVoiceExcitation (int voiceIndex, int activeVoices, float delayedSample) noexcept;

    bool isAuditionMultiSourceSignal (int signalIndex) const noexcept;

    int getAuditionVoiceCountForSignal() const noexcept;

    float getAuditionVoiceSpreadDegrees() const noexcept;

    int getAuditionVoiceDelaySamples (int voiceIndex, int voiceCount) const noexcept;

    float readAuditionHistoryDelayed (int delaySamples) const noexcept;

    void publishAuditionReactiveTelemetry (
        float rms,
        float peak,
        float envFast,
        float envSlow,
        float onset,
        float brightness,
        float rainFadeRate,
        float snowFadeRate,
        float physicsVelocity,
        float physicsCollision,
        float physicsDensity,
        float physicsCoupling,
        float headphoneOutputRms,
        float headphoneOutputPeak,
        float headphoneParity,
        int headphoneFallbackReasonIndex,
        const std::array<float, AUDITION_MAX_VOICES>& sourceEnergy,
        int sourceCount) noexcept;

    void resetAuditionReactiveTelemetry() noexcept;

    void applyAuditionReactiveHeadphoneParity (
        float headphoneOutputRms,
        float headphoneOutputPeak,
        float headphoneParity,
        int headphoneFallbackReasonIndex) noexcept;

    float applyAuditionPhysicsReactiveTimbre (
        float sample,
        float physicsVelocity,
        float physicsCollision,
        float physicsDensity,
        float motionEnergy) noexcept;

    float generateAuditionSignalSample() noexcept;

    void renderInternalAuditionEmitter (int numSamples) noexcept;

    bool isSteamAudioBackendAvailable() const noexcept;

    void setSteamInitStage (SteamInitStage stage, int errorCode) noexcept;

    void clearSteamInitDiagnosticsStrings();

    void setSteamRuntimeLibraryPathForDiagnostics (const juce::String& libraryPath);

    void setSteamMissingSymbolForDiagnostics (const juce::String& symbolName);

    void initialiseSteamAudioRuntimeIfEnabled();

    void teardownSteamAudioRuntime() noexcept;

    using SpatialProfileResolution = locusq::spatial_profile_router::SpatialProfileResolution;

    static bool isStereoOrBinauralProfile (SpatialOutputProfile profile) noexcept;

    SpatialProfileResolution resolveSpatialProfileForHost (int numOutputChannels) const noexcept;

    static int ambisonicOrderForProfile (SpatialOutputProfile profile) noexcept;

    static void encodeAmbisonicFoaProxyFromQuad (float fl, float fr, float rr, float rl,
                                                 float& w, float& x, float& y, float& z) noexcept;

    static void decodeAmbisonicFoaProxyToStereo (float w, float x, float y, float z,
                                                 float& left, float& right) noexcept;

    void renderVirtual3dStereoSample (int sampleIndex, float& left, float& right) const noexcept;

    void writeSurround521Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept;

    void writeSurround721Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept;

    void writeSurround742Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept;

    void renderStereoDownmixSample (int sampleIndex, float& left, float& right) const noexcept;

    void resetHeadphoneCompensationState() noexcept;

    void updateHeadphoneCompensationForProfile (HeadphoneDeviceProfile profile) noexcept;

    void applyHeadphoneProfileCompensation (float& left, float& right) noexcept;

    bool renderSteamBinauralBlock (int numSamples) noexcept;

    static float calculateDistance (const Vec3& pos);

    static float calculateAzimuth (const Vec3& pos);

    static float calculateElevation (const Vec3& pos);
};
