#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <optional>
#include "SceneGraph.h"
#include "SpatialRenderer.h"
#include "CalibrationEngine.h"
#include "PhysicsEngine.h"
#include "KeyframeTimeline.h"

//==============================================================================
// LocusQ Operating Mode
//==============================================================================
enum class LocusQMode
{
    Calibrate = 0,
    Emitter   = 1,
    Renderer  = 2
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

    // Calibration control/status API for WebView bridge
    bool startCalibrationFromUI (const juce::var& options);
    void abortCalibrationFromUI();
    juce::var redetectCalibrationRoutingFromUI();
    juce::var getCalibrationStatus() const;

    // Timeline and preset API for WebView bridge (Phase 2.6)
    juce::var getKeyframeTimelineForUI() const;
    bool setKeyframeTimelineFromUI (const juce::var& timelineState);
    bool setTimelineCurrentTimeFromUI (double timeSeconds);
    juce::var listEmitterPresetsFromUI() const;
    juce::var saveEmitterPresetFromUI (const juce::var& options);
    juce::var loadEmitterPresetFromUI (const juce::var& options);
    juce::var getUIStateFromUI() const;
    bool setUIStateFromUI (const juce::var& state);

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Scene Graph integration
    SceneGraph& sceneGraph;
    int emitterSlotId = -1;
    bool rendererRegistered = false;
    void syncSceneGraphRegistrationForMode (LocusQMode mode);

    // Publish emitter state to scene graph (called in processBlock for Emitter mode)
    void publishEmitterState (int numSamplesInBlock);

    //==============================================================================
    // Spatialization engine (Phase 2.2)
    SpatialRenderer spatialRenderer;

    // Update renderer parameters from APVTS (called before processing)
    void updateRendererParameters();

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
    juce::File getPresetDirectory() const;
    juce::String getSnapshotOutputLayout() const;
    int getSnapshotOutputChannels() const;
    void migrateSnapshotLayoutIfNeeded (const juce::ValueTree& restoredState);
    std::array<int, SpatialRenderer::NUM_SPEAKERS> getCurrentCalibrationSpeakerRouting() const;
    int getCurrentCalibrationSpeakerConfigIndex() const;
    void applyAutoDetectedCalibrationRoutingIfAppropriate (int outputChannels, bool force);
    void setIntegerParameterValueNotifyingHost (const char* parameterId, int value);
    juce::var buildEmitterPresetLocked (const juce::String& presetName) const;
    bool applyEmitterPresetLocked (const juce::var& presetState);
    static juce::String keyframeCurveToString (KeyframeCurve curve);
    static KeyframeCurve keyframeCurveFromVar (const juce::var& value);
    static juce::String sanitiseEmitterLabel (const juce::String& label);
    static std::optional<juce::var> readJsonFromFile (const juce::File& file);
    static bool writeJsonToFile (const juce::File& file, const juce::var& payload);
    void applyEmitterLabelToSceneSlotIfAvailable (const juce::String& label);

    // Runtime perf telemetry (EMA values in milliseconds)
    static void updatePerfEma (double& accumulator, double sampleMs) noexcept;
    double perfProcessBlockMs = 0.0;
    double perfEmitterPublishMs = 0.0;
    double perfRendererProcessMs = 0.0;

    //==============================================================================
    // Sample rate tracking
    double currentSampleRate = 44100.0;

    //==============================================================================
    // UI-only state persisted in plugin snapshot (non-APVTS)
    mutable juce::SpinLock uiStateLock;
    juce::String emitterLabelState { "Emitter" };
    juce::String physicsPresetState { "off" };
    bool hasAppliedAutoDetectedCalibrationRouting = false;
    int lastAutoDetectedOutputChannels = 0;
    int lastAutoDetectedSpeakerConfig = 0;
    std::array<int, SpatialRenderer::NUM_SPEAKERS> lastAutoDetectedSpeakerRouting { 1, 2, 3, 4 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocusQAudioProcessor)
};
