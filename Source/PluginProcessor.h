#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "SceneGraph.h"

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

    // Scene graph JSON for WebView (called from editor timer)
    juce::String getSceneStateJSON() const;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Scene Graph integration
    SceneGraph& sceneGraph;
    int emitterSlotId = -1;
    bool rendererRegistered = false;

    // Publish emitter state to scene graph (called in processBlock for Emitter mode)
    void publishEmitterState();

    //==============================================================================
    // Sample rate tracking
    double currentSampleRate = 44100.0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocusQAudioProcessor)
};
