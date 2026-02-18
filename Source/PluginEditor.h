#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

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

    //==============================================================================
    // CRITICAL: MEMBER DECLARATION ORDER
    // DO NOT REORDER - This prevents DAW crash on unload
    //==============================================================================

    // 1. PARAMETER RELAYS (Destroyed last)
    // Global
    juce::WebComboBoxRelay modeRelay { "mode" };
    juce::WebToggleButtonRelay bypassRelay { "bypass" };

    // Emitter Position
    juce::WebSliderRelay azimuthRelay { "pos_azimuth" };
    juce::WebSliderRelay elevationRelay { "pos_elevation" };
    juce::WebSliderRelay distanceRelay { "pos_distance" };

    // Emitter Audio
    juce::WebSliderRelay emitGainRelay { "emit_gain" };
    juce::WebSliderRelay spreadRelay { "emit_spread" };
    juce::WebSliderRelay directivityRelay { "emit_directivity" };
    juce::WebToggleButtonRelay muteRelay { "emit_mute" };
    juce::WebToggleButtonRelay soloRelay { "emit_solo" };

    // Emitter Physics
    juce::WebToggleButtonRelay physEnableRelay { "phys_enable" };

    // Renderer
    juce::WebSliderRelay masterGainRelay { "rend_master_gain" };
    juce::WebComboBoxRelay qualityRelay { "rend_quality" };

    // 2. WEBBROWSERCOMPONENT (Destroyed middle)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. PARAMETER ATTACHMENTS (Destroyed first)
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> azimuthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> elevationAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> distanceAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> emitGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> directivityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> muteAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> soloAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> physEnableAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> masterGainAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> qualityAttachment;

    //==============================================================================
    // Resource provider for embedded web files
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    // Helper functions
    static const char* getMimeForExtension (const juce::String& extension);

    // Reference to processor
    LocusQAudioProcessor& audioProcessor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocusQAudioProcessorEditor)
};
