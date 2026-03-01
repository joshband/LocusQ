#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <optional>
#include "VisualTokenScheduler.h"
#include "SceneGraph.h"
#include "SpatialRenderer.h"
#include "HeadTrackingBridge.h"
#include "HeadPoseInterpolator.h"
#include "CalibrationEngine.h"
#include "PhysicsEngine.h"
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
class LocusQAudioProcessor : public juce::AudioProcessor
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
    void primeRendererStateFromCurrentParameters();

    // Scene graph JSON for WebView (called from editor timer)
    juce::String getSceneStateJSON();
    const VisualTokenSnapshot& getVisualTokenSnapshot() const noexcept { return visualTokenScheduler.getSnapshot(); }
    juce::var getConfidenceMaskingStatus() const;

    // Calibration control/status API for WebView bridge
    bool startCalibrationFromUI (const juce::var& options);
    void abortCalibrationFromUI();
    juce::var redetectCalibrationRoutingFromUI();
    juce::var getCalibrationStatus() const;
    juce::var listCalibrationProfilesFromUI() const;
    juce::var saveCalibrationProfileFromUI (const juce::var& options);
    juce::var loadCalibrationProfileFromUI (const juce::var& options);
    juce::var renameCalibrationProfileFromUI (const juce::var& options);
    juce::var deleteCalibrationProfileFromUI (const juce::var& options);
    void pollCompanionCalibrationProfileFromDisk();

    // Timeline and preset API for WebView bridge (Phase 2.6)
    juce::var getKeyframeTimelineForUI() const;
    bool setKeyframeTimelineFromUI (const juce::var& timelineState);
    bool setTimelineCurrentTimeFromUI (double timeSeconds);
    juce::var listEmitterPresetsFromUI() const;
    juce::var saveEmitterPresetFromUI (const juce::var& options);
    juce::var loadEmitterPresetFromUI (const juce::var& options);
    juce::var renameEmitterPresetFromUI (const juce::var& options);
    juce::var deleteEmitterPresetFromUI (const juce::var& options);
    juce::var getUIStateFromUI() const;
    bool setUIStateFromUI (const juce::var& state);

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
    struct PublishedHeadphoneCalibrationDiagnostics
    {
        std::uint64_t profileSyncSeq = 0;
        juce::String requested;
        juce::String active;
        juce::String stage;
        bool fallbackReady = true;
        juce::String fallbackReason;
        bool valid = false;
    };

    struct PublishedHeadphoneVerificationDiagnostics
    {
        std::uint64_t profileSyncSeq = 0;
        juce::String profileId;
        juce::String requestedProfileId;
        juce::String activeProfileId;
        juce::String requestedEngineId;
        juce::String activeEngineId;
        juce::String fallbackReasonCode;
        juce::String fallbackTarget;
        juce::String fallbackReasonText;
        float frontBackScore = 0.0f;
        float elevationScore = 0.0f;
        float externalizationScore = 0.0f;
        float confidence = 0.0f;
        juce::String verificationStage;
        juce::String verificationScoreStatus;
        int chainLatencySamples = 0;
        bool valid = false;
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

    ClapRuntimeDiagnostics getClapRuntimeDiagnostics() const;

    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Scene Graph integration
    SceneGraph& sceneGraph;
    int emitterSlotId = -1;
    bool rendererRegistered = false;
    RegistrationTransitionDiagnostics registrationTransitionDiagnostics;
    RegistrationClaimReleaseDiagnostics registrationClaimReleaseDiagnostics;
    void syncSceneGraphRegistrationForMode (LocusQMode mode);

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
    PhysicsEngine physicsEngine;
    bool lastPhysThrowGate = false;
    bool lastPhysResetGate = false;

    //==============================================================================
    // Keyframe animation timeline (Phase 2.6)
    KeyframeTimeline keyframeTimeline;
    mutable juce::SpinLock keyframeTimelineLock;
    void initialiseDefaultKeyframeTimeline();
    std::optional<double> getTransportTimeSeconds() const;

    // Timeline serialization helpers (call while holding keyframeTimelineLock)
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
    void applyAutoDetectedCalibrationRoutingIfAppropriate (int outputChannels, bool force);
    void setIntegerParameterValueNotifyingHost (const char* parameterId, int value);
    juce::var buildEmitterPresetLocked (const juce::String& presetName,
                                        const juce::String& presetType,
                                        const juce::String& choreographyPackId,
                                        bool includeParameters,
                                        bool includeTimeline) const;
    juce::var buildCalibrationProfileState (const juce::String& profileName,
                                            const juce::var& validationSummary) const;
    bool applyEmitterPresetLocked (const juce::var& presetState);
    bool applyCalibrationProfileState (const juce::var& profileState);
    static juce::String keyframeCurveToString (KeyframeCurve curve);
    static KeyframeCurve keyframeCurveFromVar (const juce::var& value);
    static juce::String sanitiseEmitterLabel (const juce::String& label);
    static std::optional<juce::var> readJsonFromFile (const juce::File& file);
    static bool writeJsonToFile (const juce::File& file, const juce::var& payload);
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
    std::uint64_t sceneSnapshotSequence = 0;
    mutable juce::SpinLock publishedHeadphoneCalibrationLock;
    mutable PublishedHeadphoneCalibrationDiagnostics publishedHeadphoneCalibrationDiagnostics;
    mutable PublishedHeadphoneVerificationDiagnostics publishedHeadphoneVerificationDiagnostics;
    mutable PublishedConfidenceMaskingDiagnostics publishedConfidenceMaskingDiagnostics;

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
    bool         cachedCalibrationTrackingEnabled = false;
    int          cachedCalibrationFirLatency      = 0;
    float        cachedExternalizationScore       = -1.0f;  // -1 = not yet available
    float        cachedFrontBackConfusionRate     = -1.0f;  // -1 = not yet available

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocusQAudioProcessor)
};
