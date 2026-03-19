#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <atomic>
#include <array>
#include <cstdio>
#include <cstdint>
#include <optional>
#include <vector>
#include "VisualTokenScheduler.h"
#include "SceneGraph.h"
#include "SpatialRenderer.h"
#include "HeadTrackingBridge.h"
#include "HeadPoseInterpolator.h"
#include "CalibrationEngine.h"
#include "PhysicsEngine.h"
#include "PhysicsSharedRuntime.h"
#include "KeyframeTimeline.h"
#include "shared_contracts/ConfidenceMaskingContract.h"
#include "shared_contracts/RegistrationLockFreeContract.h"
#include "SteamAudioVirtualSurround.h"

#if LOCUSQ_ENABLE_CLAP
 #if __has_include(<clap-juce-extensions/clap-juce-extensions.h>)
  JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wnon-virtual-dtor", "-Wunused-parameter", "-Wextra-semi")
  #include <clap-juce-extensions/clap-juce-extensions.h>
  JUCE_END_IGNORE_WARNINGS_GCC_LIKE
  #define LOCUSQ_CLAP_PROPERTIES_AVAILABLE 1
 #else
  #define LOCUSQ_CLAP_PROPERTIES_AVAILABLE 0
 #endif
#else
 #define LOCUSQ_CLAP_PROPERTIES_AVAILABLE 0
#endif

//==============================================================================
// LocusQ Operating Mode
//==============================================================================
enum class LocusQMode
{
    Calibrate = 0,
    Emitter   = 1,
    Renderer  = 2
};

enum class RegistrationTransitionStage : int
{
    Stable = 0,
    ClaimConflict = 1,
    Recovered = 2,
    Ambiguous = 3
};

enum class RegistrationTransitionFallbackReason : int
{
    None = 0,
    EmitterSlotUnavailable = 1,
    RendererAlreadyClaimed = 2,
    StaleEmitterOwner = 3,
    DualOwnershipResolved = 4,
    RendererStateDrift = 5,
    ReleaseIncomplete = 6
};

//==============================================================================
/**
 * LocusQ - Quadraphonic 3D Spatial Audio Tool
 *
 * Single binary, three modes: Calibrate / Emitter / Renderer.
 * Emitters publish spatial state to a shared SceneGraph.
 * Renderer reads the scene and produces quad output.
 *
 * Phase 2.1: Foundation & Scene Graph
 */
class LocusQAudioProcessor : public juce::AudioProcessor,
                             public juce::AsyncUpdater
#if LOCUSQ_CLAP_PROPERTIES_AVAILABLE
                          , public clap_juce_extensions::clap_properties
                          , public clap_juce_extensions::clap_juce_audio_processor_capabilities
#endif
{
public:
    //==============================================================================
    LocusQAudioProcessor();
    ~LocusQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void handleAsyncUpdate() override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    // Current operating mode
    LocusQMode getCurrentMode() const;
    int getEmitterSlotId() const { return emitterSlotId; }
#if defined (LOCUSQ_TESTING) && LOCUSQ_TESTING
    PerEmitterDSPValues getPhysicsDspValuesForTesting (int slot) const { return physicsDspBridge.read (slot); }
#endif
    void primeRendererStateFromCurrentParameters();

    // Scene graph JSON for WebView (called from editor timer)
    juce::String getSceneStateJSON();
    const VisualTokenSnapshot& getVisualTokenSnapshot() const noexcept { return visualTokenScheduler.getSnapshot(); }
    juce::var getConfidenceMaskingStatus() const;

    // Calibration control/status API for WebView bridge
    bool startCalibrationFromUI (const juce::var& options);
    void abortCalibrationFromUI();
    juce::var redetectCalibrationRoutingFromUI();
    juce::var applyBestCalibrationOutputMapFromUI();
    juce::var applyBestCalibrationTopologyFromUI();
    juce::var getCalibrationStatus() const;
    juce::var listCalibrationProfilesFromUI() const;
    juce::var saveCalibrationProfileFromUI (const juce::var& options);
    juce::var loadCalibrationProfileFromUI (const juce::var& options);
    juce::var renameCalibrationProfileFromUI (const juce::var& options);
    juce::var deleteCalibrationProfileFromUI (const juce::var& options);
    juce::var exportCalibrationProfileFromUI (const juce::var& options) const;
    juce::var importCalibrationProfileFromUI (const juce::var& options);
    void pollCompanionCalibrationProfileFromDisk();

    // Timeline and preset API for WebView bridge (Phase 2.6)
    juce::var getKeyframeTimelineForUI() const;
    bool setKeyframeTimelineFromUI (const juce::var& timelineState);
    juce::var commitKeyframeTimelineFromUI (const juce::var& payload);
    bool setTimelineCurrentTimeFromUI (double timeSeconds);
    juce::var listEmitterPresetsFromUI() const;
    juce::var saveEmitterPresetFromUI (const juce::var& options);
    juce::var loadEmitterPresetFromUI (const juce::var& options);
    juce::var renameEmitterPresetFromUI (const juce::var& options);
    juce::var deleteEmitterPresetFromUI (const juce::var& options);
    juce::var getUIStateFromUI() const;
    bool setUIStateFromUI (const juce::var& state);
    juce::var getAuthoringHistoryStatusFromUI() const;
    juce::var undoAuthoringActionFromUI();
    juce::var redoAuthoringActionFromUI();

    // BL-045 Slice C: re-center UX + drift telemetry (public — accessed from EditorWebViewRuntime)
    // yawReferenceDeg and yawReferenceSet are transient (not persisted to state XML).
    std::atomic<float> yawReferenceDeg    { 0.0f };
    std::atomic<bool>  yawReferenceSet    { false };
    std::atomic<float> lastHeadTrackYawDeg { 0.0f }; // raw yaw; updated each processBlock call
    void setYawReference (float yawDeg) noexcept;

#if LOCUSQ_CLAP_PROPERTIES_AVAILABLE
    bool supportsDirectEvent (uint16_t space_id, uint16_t type) override;
    void handleDirectEvent (const clap_event_header_t* event, int sampleOffset) override;
#endif

private:
    template <size_t N>
    struct FixedUtf8Text
    {
        void set (const juce::String& value) noexcept
        {
            std::snprintf (bytes.data(), bytes.size(), "%s", value.toRawUTF8());
            bytes.back() = '\0';
        }

        juce::String toString() const
        {
            return juce::String (bytes.data());
        }

        std::array<char, N> bytes {};
    };

    struct PublishedHeadphoneCalibrationDiagnostics
    {
        std::uint64_t profileSyncSeq = 0;
        FixedUtf8Text<64> requested;
        FixedUtf8Text<64> active;
        FixedUtf8Text<64> stage;
        bool fallbackReady = true;
        FixedUtf8Text<128> fallbackReason;
        bool valid = false;
    };

    struct PublishedHeadphoneVerificationDiagnostics
    {
        std::uint64_t profileSyncSeq = 0;
        FixedUtf8Text<64> profileId;
        FixedUtf8Text<64> requestedProfileId;
        FixedUtf8Text<64> activeProfileId;
        FixedUtf8Text<64> requestedEngineId;
        FixedUtf8Text<64> activeEngineId;
        FixedUtf8Text<64> fallbackReasonCode;
        FixedUtf8Text<64> fallbackTarget;
        FixedUtf8Text<192> fallbackReasonText;
        float frontBackScore = 0.0f;
        float elevationScore = 0.0f;
        float externalizationScore = 0.0f;
        float confidence = 0.0f;
        FixedUtf8Text<64> verificationStage;
        FixedUtf8Text<64> verificationScoreStatus;
        FixedUtf8Text<64> scoreProvenance;
        FixedUtf8Text<96> compensationLabel;
        FixedUtf8Text<64> compensationProvenance;
        int chainLatencySamples = 0;
        bool valid = false;
    };

    struct PublishedHeadphoneDiagnosticsSnapshot
    {
        std::atomic<std::uint32_t> seq { 0 };
        PublishedHeadphoneCalibrationDiagnostics calibration;
        PublishedHeadphoneVerificationDiagnostics verification;
    };

    struct PublishedConfidenceMaskingDiagnostics
    {
        std::atomic<std::uint64_t> snapshotSeq { 0 };
        std::atomic<float> distanceConfidence { 0.0f };
        std::atomic<float> occlusionProbability { 0.0f };
        std::atomic<float> hrtfMatchQuality { 0.0f };
        std::atomic<float> maskingIndex { 1.0f };
        std::atomic<float> combinedConfidence { 0.0f };
        std::atomic<float> overlayAlpha { 0.0f };
        std::atomic<int> overlayBucketIndex { 0 };
        std::atomic<int> fallbackReasonIndex {
            static_cast<int> (locusq::shared_contracts::confidence_masking::FallbackReason::InactiveMode)
        };
        std::atomic<bool> valid { false };
    };

    struct PublishedFiniteGuardrailDiagnostics
    {
        std::atomic<std::uint64_t> snapshotSeq { 0 };
        std::atomic<bool> finiteGuardrailsActive { false };
        std::atomic<int> finiteGuardrailsFallbackReason { 0 }; // 0=none, 3=denormal-flushed, 5=non-finite/limiter-clamped, 6=hard-clamped
        std::atomic<std::uint32_t> finiteGuardrailsNonFiniteCount { 0 };
        std::atomic<std::uint32_t> finiteGuardrailsDenormalCount { 0 };
        std::atomic<std::uint32_t> finiteGuardrailsLimiterClampCount { 0 };
        std::atomic<std::uint32_t> finiteGuardrailsHardClampCount { 0 };
    };

    struct ClapRuntimeDiagnostics
    {
        bool buildEnabled = false;
        bool propertiesAvailable = false;
        bool isClapInstance = false;
        bool isActive = false;
        bool isProcessing = false;
        bool hasTransport = false;
        juce::String wrapperType { "Unknown" };
        juce::String lifecycleStage { "not_compiled" };
        juce::String runtimeMode { "disabled" };
        std::uint32_t versionMajor = 0;
        std::uint32_t versionMinor = 0;
        std::uint32_t versionRevision = 0;
    };

    struct RegistrationTransitionDiagnostics
    {
        std::atomic<std::uint64_t> seq { 0 };
        std::atomic<int> requestedMode { static_cast<int> (LocusQMode::Calibrate) };
        std::atomic<int> stageCode { static_cast<int> (RegistrationTransitionStage::Stable) };
        std::atomic<int> fallbackCode { static_cast<int> (RegistrationTransitionFallbackReason::None) };
        std::atomic<int> emitterSlot { -1 };
        std::atomic<bool> emitterActive { false };
        std::atomic<bool> rendererOwned { false };
        std::atomic<std::uint32_t> ambiguityCount { 0 };
        std::atomic<std::uint32_t> staleOwnerCount { 0 };
    };

    struct RegistrationClaimReleaseDiagnostics
    {
        std::atomic<std::uint64_t> seq { 0 };
        std::atomic<int> lastOperationCode {
            static_cast<int> (locusq::shared_contracts::registration_lock_free::Operation::None)
        };
        std::atomic<int> lastOutcomeCode {
            static_cast<int> (locusq::shared_contracts::registration_lock_free::Outcome::Noop)
        };
        std::atomic<std::uint32_t> contentionCount { 0 };
        std::atomic<std::uint32_t> releaseIncompleteCount { 0 };
    };

    struct AuthoringFileState
    {
        juce::String path;
        bool exists = false;
        juce::String contents;
    };

    struct AuthoringHistorySelectionHint
    {
        juce::String preferredPresetPath;
        juce::String preferredPresetType;
    };

    struct AuthoringHistoryEntry
    {
        juce::String actionId;
        juce::String label;
        juce::var beforeState;
        juce::var afterState;
        std::vector<AuthoringFileState> beforeFiles;
        std::vector<AuthoringFileState> afterFiles;
        AuthoringHistorySelectionHint beforeSelection;
        AuthoringHistorySelectionHint afterSelection;
    };

    ClapRuntimeDiagnostics getClapRuntimeDiagnostics() const;

    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Scene Graph integration
    SceneGraph& sceneGraph;
    // Claimed before audio starts so emitter registration can stay allocation-free in processBlock.
    int sceneGraphAudioReservationId = -1;
    int emitterSlotId = -1;
    bool rendererRegistered = false;
    RegistrationTransitionDiagnostics registrationTransitionDiagnostics;
    RegistrationClaimReleaseDiagnostics registrationClaimReleaseDiagnostics;
    void syncSceneGraphRegistrationForMode (LocusQMode mode);
    void captureModeTransitionInputSnapshot (const juce::AudioBuffer<float>& sourceBuffer,
                                             int totalNumInputChannels,
                                             int totalNumOutputChannels) noexcept;
    void copyModeTransitionInputSnapshotToBuffer (juce::AudioBuffer<float>& targetBuffer,
                                                  int totalNumOutputChannels) noexcept;
    void prepareRendererRealtimeStateForBlock();
    void renderRendererScratchForModeTransition (int totalNumOutputChannels, int numSamples);
    static void applyModeTransitionCrossfade (juce::AudioBuffer<float>& targetBuffer,
                                              const juce::AudioBuffer<float>& fromBuffer,
                                              int totalNumOutputChannels,
                                              int numSamples) noexcept;

    // Publish emitter state to scene graph (called in processBlock for Emitter mode)
    void publishEmitterState (int numSamplesInBlock);

    //==============================================================================
    // Spatialization engine (Phase 2.2)
    SpatialRenderer spatialRenderer;
    // BL-052: calibration monitoring virtual-surround adapter (constructed after
    // spatialRenderer to ensure valid reference lifetime).
    SteamAudioVirtualSurround calMonitorVirtualSurround { spatialRenderer };
    HeadTrackingBridge   headTrackingBridge;
    HeadPoseInterpolator headPoseInterpolator;
    std::atomic<bool>    calibrationProfileTrackingEnabled { false };
    std::atomic<float>   calibrationProfileYawOffsetDeg { 0.0f };

    // Update renderer parameters from APVTS (called before processing)
    void updateRendererParameters();

    // BL-052: apply cal_monitoring_path routing after calibrationEngine.processBlock.
    // monPathIndex is the raw integer value from the "cal_monitoring_path" APVTS param.
    void applyCalibrationMonitoringPath (juce::AudioBuffer<float>& buffer, int monPathIndex);

    //==============================================================================
    // Room calibration engine (Phase 2.3)
    CalibrationEngine calibrationEngine;

    //==============================================================================
    // Physics engine (Phase 2.4)
    PhysicsSharedRuntime& physicsSharedRuntime;
    PhysicsDSPBridge& physicsDspBridge;
    PhysicsWorker& physicsWorker;
    PhysicsEngine physicsEngine;
    bool physicsSharedRuntimeAcquired = false;
    bool lastPhysThrowGate = false;
    bool lastPhysResetGate = false;
    bool lastAngThrowGate = false;
    bool lastAngResetGate = false;
    static constexpr int kPhysicsDAWSlotCount = 8;
    static constexpr int kAttractorSlotCount = 4;
    static constexpr int kFlockGroupCount = 4;
    static constexpr float kPhysicsHostMirrorNotifyEpsilon = 0.0005f;
    std::array<bool, kPhysicsDAWSlotCount> lastFrozenState {};
    int lastPhysicsRateIndex = -1; // guards per-block physicsDspBridge.prepare() calls
    std::array<std::atomic<float>*, kPhysicsDAWSlotCount> physGainModParams  {};
    std::array<std::atomic<float>*, kPhysicsDAWSlotCount> physSpreadModParams {};
    std::array<std::atomic<float>*, kPhysicsDAWSlotCount> physTransientParams {};
    std::array<std::atomic<float>*, kPhysicsDAWSlotCount> physFrozenParams    {};
    std::array<juce::RangedAudioParameter*, kPhysicsDAWSlotCount> physTransientNotifyTargets {};
    std::array<std::atomic<float>, kPhysicsDAWSlotCount> physTransientHostPending {};
    std::array<std::atomic<float>, kPhysicsDAWSlotCount> physTransientHostPublished {};
    std::array<std::atomic<bool>, kPhysicsDAWSlotCount> physTransientHostDirty {};
    std::atomic<float>* rendPhysRateParam = nullptr;
    std::atomic<float>* rendPhysPauseParam = nullptr;
    std::atomic<float>* rendPhysWallsParam = nullptr;
    std::atomic<float>* rendPhysInteractParam = nullptr;
    std::atomic<float>* posCoordModeParam = nullptr;
    std::atomic<float>* posAzimuthParam = nullptr;
    std::atomic<float>* posElevationParam = nullptr;
    std::atomic<float>* posDistanceParam = nullptr;
    std::atomic<float>* posXParam = nullptr;
    std::atomic<float>* posYParam = nullptr;
    std::atomic<float>* posZParam = nullptr;
    std::atomic<float>* sizeUniformParam = nullptr;
    std::atomic<float>* sizeLinkParam = nullptr;
    std::atomic<float>* sizeWidthParam = nullptr;
    std::atomic<float>* sizeHeightParam = nullptr;
    std::atomic<float>* sizeDepthParam = nullptr;
    std::atomic<float>* animEnableParam = nullptr;
    std::atomic<float>* animModeParam = nullptr;
    std::atomic<float>* animLoopParam = nullptr;
    std::atomic<float>* animSpeedParam = nullptr;
    std::atomic<float>* animSyncParam = nullptr;
    std::atomic<float>* emitGainParam = nullptr;
    std::atomic<float>* emitSpreadParam = nullptr;
    std::atomic<float>* emitDirectivityParam = nullptr;
    std::atomic<float>* emitMuteParam = nullptr;
    std::atomic<float>* emitSoloParam = nullptr;
    std::atomic<float>* emitDirAzimuthParam = nullptr;
    std::atomic<float>* emitDirElevationParam = nullptr;
    std::atomic<float>* physEnableParam = nullptr;
    std::atomic<float>* physBoundaryModeParam = nullptr;
    std::atomic<float>* physSoftBoundaryDepthParam = nullptr;
    std::atomic<float>* physFlockGroupParam = nullptr;
    std::atomic<float>* physSpringEnableParam = nullptr;
    std::atomic<float>* physSpringKParam = nullptr;
    std::atomic<float>* physSpringDampParam = nullptr;
    std::atomic<float>* physSpringAnchorModeParam = nullptr;
    std::atomic<float>* physSpringAnchorXParam = nullptr;
    std::atomic<float>* physSpringAnchorYParam = nullptr;
    std::atomic<float>* physSpringAnchorZParam = nullptr;
    std::atomic<float>* physTurbulenceParam = nullptr;
    std::atomic<float>* physTurbulenceRateParam = nullptr;
    std::atomic<float>* physAngEnableParam = nullptr;
    std::atomic<float>* physAngDragParam = nullptr;
    std::atomic<float>* physAngImpulseXParam = nullptr;
    std::atomic<float>* physAngImpulseYParam = nullptr;
    std::atomic<float>* physAngImpulseZParam = nullptr;
    std::atomic<float>* physAngAttractorTorqueParam = nullptr;
    std::atomic<float>* physAngThrowParam = nullptr;
    std::atomic<float>* physAngResetParam = nullptr;
    std::atomic<float>* physMassOverrideParam = nullptr;
    std::atomic<float>* physCollideEmittersParam = nullptr;
    std::atomic<float>* physCollisionRadiusParam = nullptr;
    std::atomic<float>* physCollisionGainScaleParam = nullptr;
    std::atomic<float>* physCollisionDecayMsParam = nullptr;
    std::atomic<float>* physMassParam = nullptr;
    std::atomic<float>* physDragParam = nullptr;
    std::atomic<float>* physElasticityParam = nullptr;
    std::atomic<float>* physFrictionParam = nullptr;
    std::atomic<float>* physGravityParam = nullptr;
    std::atomic<float>* physGravityDirParam = nullptr;
    std::atomic<float>* physThrowParam = nullptr;
    std::atomic<float>* physVelXParam = nullptr;
    std::atomic<float>* physVelYParam = nullptr;
    std::atomic<float>* physVelZParam = nullptr;
    std::atomic<float>* physResetParam = nullptr;
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorActiveParams {};
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorPosXParams {};
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorPosYParams {};
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorPosZParams {};
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorStrengthParams {};
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorRadiusParams {};
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorFalloffParams {};
    std::array<std::atomic<float>*, kAttractorSlotCount> attractorOrbitStabilizeParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockEnableParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockSepWeightParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockAlignWeightParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockCohWeightParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockSepRadiusParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockAlignRadiusParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockCohRadiusParams {};
    std::array<std::atomic<float>*, kFlockGroupCount> flockMaxSpeedParams {};

    //==============================================================================
    // Keyframe animation timeline (Phase 2.6)
    KeyframeTimeline keyframeTimelineState;
    mutable juce::CriticalSection keyframeTimelineStateLock;
    std::array<KeyframeTimeline, 3> keyframeTimelineRtBuffers {};
    std::atomic<int> keyframeTimelineRtReadIndex { 0 };
    std::atomic<int> keyframeTimelineRtPendingIndex { -1 };
    std::atomic<double> keyframeTimelinePublishedCurrentTimeSeconds { 0.0 };
    std::atomic<double> keyframeTimelinePublishedDurationSeconds { 0.0 };
    std::atomic<bool> keyframeTimelinePublishedLooping { false };
    std::atomic<float> keyframeTimelinePublishedPlaybackRate { 1.0f };
    void initialiseDefaultKeyframeTimeline (KeyframeTimeline& timeline) const;
    std::optional<double> getTransportTimeSeconds() const;
    void publishKeyframeTimelineStateToRtLocked();
    void syncPendingKeyframeTimelineForAudioThread() noexcept;
    void publishKeyframeTimelinePlaybackState (const KeyframeTimeline& timeline) noexcept;

    // Timeline serialization helpers (call while holding keyframeTimelineStateLock)
    juce::var serialiseKeyframeTimelineLocked() const;
    bool applyKeyframeTimelineLocked (const juce::var& timelineState);

    // Emitter preset helpers
    static juce::String sanitisePresetName (const juce::String& presetName);
    static juce::String normalisePresetType (const juce::String& presetType);
    static juce::String normaliseChoreographyPackId (const juce::String& packId);
    static juce::String normaliseCalibrationTopologyId (const juce::String& topologyId);
    static juce::String normaliseCalibrationMonitoringPathId (const juce::String& monitoringPathId);
    static juce::String normaliseCalibrationDeviceProfileId (const juce::String& deviceProfileId);
    static juce::String inferPresetTypeFromPayload (const juce::var& payload);
    juce::File getPresetDirectory() const;
    juce::File resolvePresetFileFromOptions (const juce::var& options) const;
    juce::File getCalibrationProfileDirectory() const;
    juce::File resolveCalibrationProfileFileFromOptions (const juce::var& options) const;
    juce::String getSnapshotOutputLayout() const;
    int getSnapshotOutputChannels() const;
    void migrateSnapshotLayoutIfNeeded (const juce::ValueTree& restoredState);
    std::array<int, SpatialRenderer::NUM_SPEAKERS> getCurrentCalibrationSpeakerRouting() const;
    int getCurrentCalibrationSpeakerConfigIndex() const;
    int getCurrentCalibrationTopologyProfileIndex() const;
    int getCurrentCalibrationMonitoringPathIndex() const;
    int getCurrentCalibrationDeviceProfileIndex() const;
    int getRequiredCalibrationChannelsForTopologyIndex (int topologyIndex) const;
    static int resolveCalibrationWritableChannels (
        int snapshotOutputChannels,
        int layoutOutputChannels,
        int cachedAutoOutputChannels,
        const std::array<int, SpatialRenderer::NUM_SPEAKERS>& routing) noexcept;
    void applyAutoDetectedCalibrationRoutingIfAppropriate (int outputChannels, bool force);
    void setIntegerParameterValueNotifyingHost (const char* parameterId, int value);
    juce::var buildEmitterPresetLocked (const juce::String& presetName,
                                        const juce::String& presetType,
                                        const juce::String& choreographyPackId,
                                        bool includeParameters,
                                        bool includeTimeline) const;
    juce::var captureEmitterParameterState() const;
    bool applyEmitterParameterState (const juce::var& parametersState);
    juce::var buildCalibrationProfileState (const juce::String& profileName,
                                            const juce::var& validationSummary,
                                            const juce::var& discoveryReconciliation = {}) const;
    bool applyEmitterPresetLocked (const juce::var& presetState);
    bool applyCalibrationProfileState (const juce::var& profileState);
    static juce::String keyframeCurveToString (KeyframeCurve curve);
    static KeyframeCurve keyframeCurveFromVar (const juce::var& value);
    static juce::String sanitiseEmitterLabel (const juce::String& label);
    static std::optional<juce::var> readJsonFromFile (const juce::File& file);
    static bool writeJsonToFile (const juce::File& file, const juce::var& payload);

    juce::var captureAuthoringStateSnapshotLocked() const;
    bool applyAuthoringStateSnapshotLocked (const juce::var& snapshot);
    static juce::var cloneJsonLikeVar (const juce::var& value);
    static bool jsonLikeVarsEqual (const juce::var& lhs, const juce::var& rhs);
    static AuthoringFileState captureAuthoringFileState (const juce::File& file);
    static bool restoreAuthoringFileStates (const std::vector<AuthoringFileState>& fileStates);
    void pushAuthoringHistoryEntry (AuthoringHistoryEntry entry);
    static AuthoringHistorySelectionHint makeSelectionHint (const juce::String& preferredPresetPath,
                                                            const juce::String& preferredPresetType);
    juce::var buildAuthoringHistoryStatusResponse (bool ok,
                                                   const juce::String& label,
                                                   const juce::String& message,
                                                   const AuthoringHistorySelectionHint& selectionHint) const;
    juce::var commitAuthoringHistoryEntry (const juce::String& actionId,
                                           const juce::String& label,
                                           const juce::var& beforeState,
                                           const juce::var& afterState,
                                           std::vector<AuthoringFileState> beforeFiles,
                                           std::vector<AuthoringFileState> afterFiles,
                                           const AuthoringHistorySelectionHint& beforeSelection,
                                           const AuthoringHistorySelectionHint& afterSelection);
    juce::var applyAuthoringHistoryEntryFromUI (bool redo);

    static constexpr size_t kMaxAuthoringHistoryEntries = 64;
    mutable juce::CriticalSection authoringHistoryLock;
    std::vector<AuthoringHistoryEntry> authoringUndoHistory;
    std::vector<AuthoringHistoryEntry> authoringRedoHistory;
    void applyEmitterLabelToSceneSlotIfAvailable (const juce::String& label);

    // Runtime perf telemetry (EMA values in milliseconds)
    static void updatePerfEma (std::atomic<float>& accumulator, double sampleMs) noexcept;
    std::atomic<float> perfProcessBlockMs { 0.0f };
    std::atomic<float> perfEmitterPublishMs { 0.0f };
    std::atomic<float> perfRendererProcessMs { 0.0f };
    std::array<std::atomic<float>, SpatialRenderer::NUM_SPEAKERS> sceneSpeakerRms {
        std::atomic<float> { 0.0f },
        std::atomic<float> { 0.0f },
        std::atomic<float> { 0.0f },
        std::atomic<float> { 0.0f }
    };
    static constexpr int kModeTransitionScratchChannels = 16;
    juce::AudioBuffer<float> modeTransitionInputSnapshotBuffer;
    juce::AudioBuffer<float> modeTransitionRendererScratchBuffer;
    LocusQMode lastProcessedMode = LocusQMode::Calibrate;
    bool hasLastProcessedMode = false;
    std::uint64_t sceneSnapshotSequence = 0;
    mutable PublishedHeadphoneDiagnosticsSnapshot publishedHeadphoneDiagnostics;
    void publishHeadphoneDiagnosticsSnapshot (const PublishedHeadphoneCalibrationDiagnostics& calibration,
                                             const PublishedHeadphoneVerificationDiagnostics& verification) noexcept;
    bool copyPublishedHeadphoneDiagnosticsSnapshot (PublishedHeadphoneCalibrationDiagnostics& calibration,
                                                    PublishedHeadphoneVerificationDiagnostics& verification) const noexcept;
    mutable PublishedConfidenceMaskingDiagnostics publishedConfidenceMaskingDiagnostics;
    mutable PublishedFiniteGuardrailDiagnostics publishedFiniteGuardrailDiagnostics;

    //==============================================================================
    // Sample rate tracking
    double currentSampleRate = 44100.0;
    VisualTokenScheduler visualTokenScheduler;

    //==============================================================================
    // UI-only state persisted in plugin snapshot (non-APVTS)
    mutable juce::SpinLock uiStateLock;
    juce::String emitterLabelState { "Emitter" };
    SharedPtrAtomicContract<juce::String> emitterLabelRtState { std::make_shared<juce::String> ("Emitter") };
    juce::String physicsPresetState { "off" };
    juce::String choreographyPackState { "custom" };
    bool hasAppliedAutoDetectedCalibrationRouting = false;
    int lastAutoDetectedOutputChannels = 0;
    int lastAutoDetectedSpeakerConfig = 0;
    int lastAutoDetectedTopologyProfile = 2;
    std::array<int, SpatialRenderer::NUM_SPEAKERS> lastAutoDetectedSpeakerRouting { 1, 2, 3, 4 };
    bool hasRestoredSnapshotState = false;
    bool hasSeededInitialEmitterColor = false;
    int lastReportedCalibrationLatency = -1;  // -1 forces first-block update
    juce::int64 companionCalibrationProfileLastModifiedMs = -1;

    // Cached companion CalibrationProfile.json fields — populated on the message thread
    // by pollCompanionCalibrationProfileFromDisk(). Read by getCalibrationStatus() bridge handler.
    // Do NOT access from processBlock().
    juce::String cachedCalibrationDevice         = "unknown";
    juce::String cachedCalibrationEqMode         = "off";
    juce::String cachedCalibrationHrtfMode       = "default";
    juce::String cachedCalibrationSofaRef;
    juce::String cachedCalibrationProfileSource  = "unknown";
    juce::String cachedCalibrationHeadphoneProvenance = "unavailable";
    juce::String cachedCalibrationVerificationProvenance = "unavailable";
    bool         cachedCalibrationRequestedSofa  = false;
    bool         cachedCalibrationTrackingEnabled = false;
    int          cachedCalibrationFirLatency      = 0;
    std::int64_t cachedCalibrationProfileUpdatedAtUtcMs = 0;
    float        cachedExternalizationScore       = -1.0f;  // -1 = not yet available
    float        cachedFrontBackConfusionRate     = -1.0f;  // -1 = not yet available

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocusQAudioProcessor)
};
