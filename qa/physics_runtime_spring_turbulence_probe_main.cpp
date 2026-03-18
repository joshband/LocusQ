#include "Source/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <cmath>
#include <iostream>
#include <thread>

namespace
{
struct ProbeResult
{
    bool passed = false;
    juce::String detail;
};

struct SceneSample
{
    float spread = 0.0f;
    float fx = 0.0f;
    Vec3 position {};
    bool found = false;
};

struct RuntimeMetrics
{
    float maxSpread = 0.0f;
    float maxAbsForceX = 0.0f;
    float maxRelativeDisplacement = 0.0f;
    float baselineSpread = 0.0f;
    float baselineAbsForceX = 0.0f;
    Vec3 originPosition {};
    bool hasOrigin = false;
    Vec3 lastPosition {};
    bool capturedBaseline = false;
};

void setActualParam (LocusQAudioProcessor& processor, const char* paramId, float actualValue)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (processor.apvts.getParameter (paramId)))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (actualValue));
}

void setChoiceParam (LocusQAudioProcessor& processor, const char* paramId, int choiceIndex)
{
    setActualParam (processor, paramId, static_cast<float> (choiceIndex));
}

SceneSample findEmitterSampleFromSceneSnapshot (const juce::String& jsonText, int emitterId)
{
    const auto parsed = juce::JSON::parse (jsonText);
    if (! parsed.isObject())
        return {};

    const auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return {};

    const auto emittersVar = root->getProperty ("emitters");
    if (! emittersVar.isArray())
        return {};

    if (const auto* emitters = emittersVar.getArray())
    {
        for (const auto& emitterVar : *emitters)
        {
            const auto* emitter = emitterVar.getDynamicObject();
            if (emitter == nullptr)
                continue;

            if (static_cast<int> (emitter->getProperty ("id")) != emitterId)
                continue;

            SceneSample sample;
            sample.spread = static_cast<float> (emitter->getProperty ("spread"));
            sample.fx = static_cast<float> (emitter->getProperty ("fx"));
            sample.position.x = static_cast<float> (emitter->getProperty ("x"));
            sample.position.y = static_cast<float> (emitter->getProperty ("y"));
            sample.position.z = static_cast<float> (emitter->getProperty ("z"));
            sample.found = true;
            return sample;
        }
    }

    return {};
}

RuntimeMetrics runAndSampleMetrics (LocusQAudioProcessor& processor,
                                    juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midi,
                                    int blocks,
                                    int sleepMs)
{
    const int emitterId = processor.getEmitterSlotId();
    RuntimeMetrics metrics;

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = 0.06f * std::sin (0.01f * static_cast<float> (block * buffer.getNumSamples() + i));
        }

        processor.processBlock (buffer, midi);
        midi.clear();

        const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
        if (sample.found)
        {
            metrics.maxSpread = juce::jmax (metrics.maxSpread, sample.spread);
            metrics.maxAbsForceX = juce::jmax (metrics.maxAbsForceX, std::abs (sample.fx));
            if (! metrics.hasOrigin)
            {
                metrics.originPosition = sample.position;
                metrics.hasOrigin = true;
            }

            const Vec3 delta
            {
                sample.position.x - metrics.originPosition.x,
                sample.position.y - metrics.originPosition.y,
                sample.position.z - metrics.originPosition.z
            };
            const float displacement = std::sqrt (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            metrics.maxRelativeDisplacement = juce::jmax (metrics.maxRelativeDisplacement, displacement);
            metrics.lastPosition = sample.position;
        }

        if (sleepMs > 0)
            std::this_thread::sleep_for (std::chrono::milliseconds (sleepMs));
    }

    return metrics;
}

void configureCommonEmitter (LocusQAudioProcessor& processor, float posX)
{
    setChoiceParam (processor, "mode", 1); // Emitter
    setActualParam (processor, "emit_spread", 0.0f);
    setActualParam (processor, "phys_enable", 1.0f);
    setActualParam (processor, "phys_drag", 0.0f);
    setActualParam (processor, "phys_gravity", 0.0f);
    setActualParam (processor, "rend_phys_pause", 0.0f);
    setChoiceParam (processor, "rend_phys_rate", 1); // 60 Hz
    setActualParam (processor, "rend_phys_wall_collide", 1.0f);
    setChoiceParam (processor, "phys_boundary_mode", 0); // Hard
    setActualParam (processor, "pos_coord_mode", 1.0f); // Cartesian
    setActualParam (processor, "pos_x", posX);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 1.2f);
    setActualParam (processor, "phys_vel_x", 0.0f);
    setActualParam (processor, "phys_vel_y", 0.0f);
    setActualParam (processor, "phys_vel_z", 0.0f);
    setActualParam (processor, "attractor_0_active", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0);
    setActualParam (processor, "rend_phys_interact", 0.0f);
}

void captureBaselineMetrics (LocusQAudioProcessor& processor,
                             juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midi,
                             RuntimeMetrics& metrics)
{
    const int emitterId = processor.getEmitterSlotId();
    const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
    if (! sample.found)
        return;

    metrics.baselineSpread = sample.spread;
    metrics.baselineAbsForceX = std::abs (sample.fx);
    metrics.originPosition = sample.position;
    metrics.lastPosition = sample.position;
    metrics.hasOrigin = true;
    metrics.capturedBaseline = true;

    buffer.clear();
    processor.processBlock (buffer, midi);
    midi.clear();
}

ProbeResult runSpringPhase()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    LocusQAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (sampleRate, blockSize);
    configureCommonEmitter (processor, 1.5f);
    setActualParam (processor, "phys_spring_enable", 1.0f);
    setActualParam (processor, "phys_spring_k", 4.0f);
    setActualParam (processor, "phys_spring_damp", 0.0f);
    setChoiceParam (processor, "phys_spring_anchor_mode", 1); // Fixed point
    setActualParam (processor, "phys_spring_anchor_x", 0.0f);
    setActualParam (processor, "phys_spring_anchor_y", 1.2f);
    setActualParam (processor, "phys_spring_anchor_z", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);

    processor.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> buffer;
    buffer.setSize (channels, blockSize, false, true, true);
    juce::MidiBuffer midi;

    const int emitterId = processor.getEmitterSlotId();
    if (emitterId < 0)
    {
        processor.releaseResources();
        return { false, "spring emitter slot not registered" };
    }

    RuntimeMetrics metrics;
    captureBaselineMetrics (processor, buffer, midi, metrics);
    const auto liveMetrics = runAndSampleMetrics (processor, buffer, midi, 64, 10);
    metrics.maxSpread = liveMetrics.maxSpread;
    metrics.maxAbsForceX = liveMetrics.maxAbsForceX;
    metrics.maxRelativeDisplacement = liveMetrics.maxRelativeDisplacement;
    metrics.lastPosition = liveMetrics.lastPosition;
    processor.releaseResources();

    const bool baselineOk = metrics.capturedBaseline
                            && metrics.baselineSpread <= 0.02f
                            && metrics.baselineAbsForceX <= 0.02f;
    const bool spreadLive = metrics.maxSpread >= 0.10f;
    const bool motionLive = metrics.maxRelativeDisplacement >= 0.20f;
    const bool forceLive = metrics.maxAbsForceX >= 0.50f;

    juce::String detail;
    detail << "emitterId=" << emitterId
           << " baselineCaptured=" << (metrics.capturedBaseline ? "1" : "0")
           << " baselineSpread=" << juce::String (metrics.baselineSpread, 3)
           << " baselineAbsForceX=" << juce::String (metrics.baselineAbsForceX, 3)
           << " maxSpread=" << juce::String (metrics.maxSpread, 3)
           << " maxAbsForceX=" << juce::String (metrics.maxAbsForceX, 3)
           << " maxDisp=" << juce::String (metrics.maxRelativeDisplacement, 3)
           << " finalPos=("
           << juce::String (metrics.lastPosition.x, 3) << ","
           << juce::String (metrics.lastPosition.y, 3) << ","
           << juce::String (metrics.lastPosition.z, 3) << ")";

    return { baselineOk && spreadLive && motionLive && forceLive, detail };
}

ProbeResult runTurbulencePhase()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    LocusQAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (sampleRate, blockSize);
    configureCommonEmitter (processor, 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.75f);
    setActualParam (processor, "phys_turbulence_rate", 4.0f);

    processor.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> buffer;
    buffer.setSize (channels, blockSize, false, true, true);
    juce::MidiBuffer midi;

    const int emitterId = processor.getEmitterSlotId();
    if (emitterId < 0)
    {
        processor.releaseResources();
        return { false, "turbulence emitter slot not registered" };
    }

    RuntimeMetrics metrics;
    captureBaselineMetrics (processor, buffer, midi, metrics);
    const auto liveMetrics = runAndSampleMetrics (processor, buffer, midi, 64, 10);
    metrics.maxSpread = liveMetrics.maxSpread;
    metrics.maxAbsForceX = liveMetrics.maxAbsForceX;
    metrics.maxRelativeDisplacement = liveMetrics.maxRelativeDisplacement;
    metrics.lastPosition = liveMetrics.lastPosition;
    processor.releaseResources();

    const bool baselineOk = metrics.capturedBaseline
                            && metrics.baselineSpread <= 0.02f
                            && metrics.baselineAbsForceX <= 0.02f;
    const bool spreadLive = metrics.maxSpread >= 0.08f;
    const bool motionLive = metrics.maxRelativeDisplacement >= 0.03f;
    const bool forceLive = metrics.maxAbsForceX >= 0.10f;

    juce::String detail;
    detail << "emitterId=" << emitterId
           << " baselineCaptured=" << (metrics.capturedBaseline ? "1" : "0")
           << " baselineSpread=" << juce::String (metrics.baselineSpread, 3)
           << " baselineAbsForceX=" << juce::String (metrics.baselineAbsForceX, 3)
           << " maxSpread=" << juce::String (metrics.maxSpread, 3)
           << " maxAbsForceX=" << juce::String (metrics.maxAbsForceX, 3)
           << " maxDisp=" << juce::String (metrics.maxRelativeDisplacement, 3)
           << " finalPos=("
           << juce::String (metrics.lastPosition.x, 3) << ","
           << juce::String (metrics.lastPosition.y, 3) << ","
           << juce::String (metrics.lastPosition.z, 3) << ")";

    return { baselineOk && spreadLive && motionLive && forceLive, detail };
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto spring = runSpringPhase();
    const auto turbulence = runTurbulencePhase();
    const bool passed = spring.passed && turbulence.passed;

    std::cout << "physics_runtime_spring_turbulence_probe: "
              << (passed ? "PASS" : "FAIL")
              << " | spring{" << spring.detail << "} turbulence{" << turbulence.detail << "}"
              << std::endl;

    return passed ? 0 : 1;
}
