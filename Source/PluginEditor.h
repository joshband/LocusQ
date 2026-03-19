#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "editor_webview/EditorParameterBridge.h"
#include "editor_webview/EditorWebViewRuntime.h"

//==============================================================================
/**
 * LocusQ Plugin Editor - WebView UI with Three.js 3D Viewport
 *
 * CRITICAL: Member declaration order MUST be:
 * 1. Parameter relays (destroyed last)
 * 2. WebBrowserComponent (destroyed middle)
 * 3. Parameter attachments (destroyed first)
 *
 * This order prevents DAW crashes on plugin unload.
 */
class LocusQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    LocusQAudioProcessorEditor (LocusQAudioProcessor&);
    ~LocusQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Timer for pushing scene state to WebView
    void timerCallback() override;
    void updateStandaloneWindowTitle();
    void pushBridgePayloadsIfDue();

    //==============================================================================
    // CRITICAL: MEMBER DECLARATION ORDER
    // DO NOT REORDER - This prevents DAW crash on unload
    //==============================================================================

    // 1. PARAMETER RELAYS (Destroyed last)
    locusq::editor_webview::ParameterBridgeRelayStore parameterBridgeRelays;

    // 2. WEBBROWSERCOMPONENT (Destroyed middle)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. PARAMETER ATTACHMENTS (Destroyed first)
    locusq::editor_webview::ParameterBridgeAttachmentStore parameterBridgeAttachments;

    locusq::editor_webview::RuntimeConfig runtimeConfig;
    bool runtimeProbeDone = false;
    int runtimeProbeTicks = 0;
    bool standaloneWindowTitleUpdated = false;
    bool uiSelfTestProbeInFlight = false;
    bool uiSelfTestResultWritten = false;
    int uiSelfTestPollTicks = 0;
    int bridgeTickCount = 0;
    juce::String lastCalibrationPayload;

    // BL-097: keep the editor timer responsive, but tier the expensive work.
    static constexpr int kEditorTimerHz = 30;
    static constexpr int kStructuralScenePublishIntervalTicks = 3;   // 10 Hz
    static constexpr int kCalibrationStatusPublishIntervalTicks = 6; // 5 Hz
    static constexpr int kCalibrationProfilePollIntervalTicks = 15;  // 2 Hz

    // BL-045 Slice C: drift telemetry push — fires every ~500ms (15 ticks at 30Hz)
    static constexpr int kDriftTelemetryIntervalTicks = 15;
    int driftTelemetryTickCount = 0;

    // Reference to processor
    LocusQAudioProcessor& audioProcessor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocusQAudioProcessorEditor)
};
