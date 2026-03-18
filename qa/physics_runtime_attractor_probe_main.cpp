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
    float spread = -1.0f;
    Vec3 position {};
    bool found = false;
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
            sample.position.x = static_cast<float> (emitter->getProperty ("x"));
            sample.position.y = static_cast<float> (emitter->getProperty ("y"));
            sample.position.z = static_cast<float> (emitter->getProperty ("z"));
            sample.found = true;
            return sample;
        }
    }

    return {};
}

struct RuntimeProbeMetrics
{
    float maxSpread = 0.0f;
    float maxRelativeDisplacement = 0.0f;
    Vec3 originPosition {};
    bool hasOrigin = false;
    Vec3 lastPosition {};
};

RuntimeProbeMetrics runAndSampleMetrics (LocusQAudioProcessor& processor,
                                         juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midi,
                                         int blocks,
                                         int sleepMs)
{
    const int emitterId = processor.getEmitterSlotId();
    RuntimeProbeMetrics metrics;

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = 0.1f * std::sin (0.01f * static_cast<float> (block * buffer.getNumSamples() + i));
        }

        processor.processBlock (buffer, midi);
        midi.clear();

        const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
        if (sample.found)
        {
            metrics.maxSpread = juce::jmax (metrics.maxSpread, sample.spread);
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

void warmupRuntime (LocusQAudioProcessor& processor,
                    juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midi,
                    int blocks,
                    int sleepMs)
{
    (void) runAndSampleMetrics (processor, buffer, midi, blocks, sleepMs);
}

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    LocusQAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (sampleRate, blockSize);

    setChoiceParam (processor, "mode", 1); // Emitter mode
    setActualParam (processor, "emit_spread", 0.0f);
    setActualParam (processor, "emit_gain", 0.0f);
    setActualParam (processor, "emit_directivity", 0.0f);
    setActualParam (processor, "phys_enable", 1.0f);
    setActualParam (processor, "phys_drag", 0.3f);
    setActualParam (processor, "phys_gravity", 0.0f);
    setActualParam (processor, "rend_phys_pause", 0.0f);
    setChoiceParam (processor, "rend_phys_rate", 1); // 60 Hz
    setActualParam (processor, "pos_coord_mode", 1.0f); // Cartesian
    setActualParam (processor, "pos_x", 0.0f);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 1.2f);
    setActualParam (processor, "attractor_0_pos_x", 2.0f);
    setActualParam (processor, "attractor_0_pos_y", 1.2f);
    setActualParam (processor, "attractor_0_pos_z", 0.0f);
    setActualParam (processor, "attractor_0_strength", 80.0f);
    setActualParam (processor, "attractor_0_radius", 10.0f);
    setChoiceParam (processor, "attractor_0_falloff", 1);
    setActualParam (processor, "attractor_0_orbit_stabilize", 0.0f);
    setActualParam (processor, "attractor_0_active", 0.0f);

    processor.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> buffer;
    buffer.setSize (channels, blockSize, false, true, true);
    juce::MidiBuffer midi;

    const int emitterId = processor.getEmitterSlotId();
    if (emitterId < 0)
    {
        processor.releaseResources();
        return { false, "emitter slot not registered in Emitter mode" };
    }

    warmupRuntime (processor, buffer, midi, 8, 8);
    const auto baselineMetrics = runAndSampleMetrics (processor, buffer, midi, 18, 8);

    setActualParam (processor, "attractor_0_active", 1.0f);
    warmupRuntime (processor, buffer, midi, 4, 10);
    const auto attractorMetrics = runAndSampleMetrics (processor, buffer, midi, 32, 10);

    processor.releaseResources();

    const bool baselineOk = baselineMetrics.maxSpread <= 0.05f && baselineMetrics.maxRelativeDisplacement <= 0.02f;
    const bool attractorLive = attractorMetrics.maxSpread >= 0.20f;
    const bool deltaOk = (attractorMetrics.maxSpread - baselineMetrics.maxSpread) >= 0.15f;
    const bool motionLive = attractorMetrics.maxRelativeDisplacement >= 0.05f;

    juce::String detail;
    detail << "emitterId=" << emitterId
           << " baselineMaxSpread=" << juce::String (baselineMetrics.maxSpread, 3)
           << " attractorMaxSpread=" << juce::String (attractorMetrics.maxSpread, 3)
           << " spreadDelta=" << juce::String (attractorMetrics.maxSpread - baselineMetrics.maxSpread, 3)
           << " baselineMaxDisp=" << juce::String (baselineMetrics.maxRelativeDisplacement, 3)
           << " attractorMaxDisp=" << juce::String (attractorMetrics.maxRelativeDisplacement, 3)
           << " finalPos=("
           << juce::String (attractorMetrics.lastPosition.x, 3) << ","
           << juce::String (attractorMetrics.lastPosition.y, 3) << ","
           << juce::String (attractorMetrics.lastPosition.z, 3) << ")";

    return { baselineOk && attractorLive && deltaOk && motionLive, detail };
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto result = runProbe();
    std::cout << "physics_runtime_attractor_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;

    return result.passed ? 0 : 1;
}
