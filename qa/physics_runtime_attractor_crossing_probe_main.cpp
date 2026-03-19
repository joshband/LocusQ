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
    Vec3 position {};
    float collisionEnergy = 0.0f;
    bool found = false;
};

struct CrossingMetrics
{
    float baselineMaxCollisionEnergy = 0.0f;
    float maxCollisionEnergy = 0.0f;
    float settleSumCollisionEnergy = 0.0f;
    float baselineMaxBridgeTransient = 0.0f;
    float maxBridgeTransient = 0.0f;
    float settleSumBridgeTransient = 0.0f;
    float minDistanceToAttractor = std::numeric_limits<float>::max();
    int settleSamples = 0;
    bool startedOutside = false;
    bool enteredRadius = false;
    SceneSample lastSample;
    float lastBridgeTransient = 0.0f;
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
            sample.position.x = static_cast<float> (emitter->getProperty ("x"));
            sample.position.y = static_cast<float> (emitter->getProperty ("y"));
            sample.position.z = static_cast<float> (emitter->getProperty ("z"));
            sample.collisionEnergy = static_cast<float> (emitter->getProperty ("collisionEnergy"));
            sample.found = true;
            return sample;
        }
    }

    return {};
}

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;
    const Vec3 attractorPos { 0.0f, 1.2f, 0.0f };
    constexpr float attractorRadius = 1.0f;

    LocusQAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (sampleRate, blockSize);

    setChoiceParam (processor, "mode", 1); // Emitter
    setActualParam (processor, "emit_spread", 0.0f);
    setActualParam (processor, "phys_enable", 1.0f);
    setActualParam (processor, "phys_drag", 0.0f);
    setActualParam (processor, "phys_gravity", 0.0f);
    setActualParam (processor, "rend_phys_pause", 0.0f);
    setChoiceParam (processor, "rend_phys_rate", 1); // 60 Hz
    setActualParam (processor, "rend_phys_wall_collide", 0.0f);
    setChoiceParam (processor, "phys_boundary_mode", 0); // Hard
    setActualParam (processor, "pos_coord_mode", 1.0f); // Cartesian
    setActualParam (processor, "pos_x", -1.8f);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 1.2f);
    setActualParam (processor, "phys_vel_x", 2.0f);
    setActualParam (processor, "phys_vel_y", 0.0f);
    setActualParam (processor, "phys_vel_z", 0.0f);
    setActualParam (processor, "phys_throw", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0);
    setActualParam (processor, "rend_phys_interact", 0.0f);
    setActualParam (processor, "attractor_0_pos_x", attractorPos.x);
    setActualParam (processor, "attractor_0_pos_y", attractorPos.y);
    setActualParam (processor, "attractor_0_pos_z", attractorPos.z);
    setActualParam (processor, "attractor_0_strength", 0.0f);
    setActualParam (processor, "attractor_0_radius", attractorRadius);
    setChoiceParam (processor, "attractor_0_falloff", 1);
    setActualParam (processor, "attractor_0_orbit_stabilize", 0.0f);
    setActualParam (processor, "attractor_0_active", 1.0f);

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

    CrossingMetrics metrics;

    for (int warmup = 0; warmup < 6; ++warmup)
    {
        buffer.clear();
        processor.processBlock (buffer, midi);
        midi.clear();

        const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
        if (sample.found)
            metrics.baselineMaxCollisionEnergy = juce::jmax (metrics.baselineMaxCollisionEnergy, sample.collisionEnergy);

        const auto bridgeValues = processor.getPhysicsDspValuesForTesting (emitterId);
        metrics.baselineMaxBridgeTransient = juce::jmax (metrics.baselineMaxBridgeTransient,
                                                         bridgeValues.gainTransient);

        std::this_thread::sleep_for (std::chrono::milliseconds (12));
    }

    setActualParam (processor, "phys_throw", 1.0f);
    processor.processBlock (buffer, midi);
    midi.clear();
    setActualParam (processor, "phys_throw", 0.0f);

    for (int block = 0; block < 56; ++block)
    {
        buffer.clear();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = 0.03f * std::sin (0.012f * static_cast<float> (block * buffer.getNumSamples() + i));
        }

        processor.processBlock (buffer, midi);
        midi.clear();

        const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
        const auto bridgeValues = processor.getPhysicsDspValuesForTesting (emitterId);
        metrics.maxBridgeTransient = juce::jmax (metrics.maxBridgeTransient, bridgeValues.gainTransient);
        metrics.lastBridgeTransient = bridgeValues.gainTransient;

        if (sample.found)
        {
            const Vec3 delta
            {
                sample.position.x - attractorPos.x,
                sample.position.y - attractorPos.y,
                sample.position.z - attractorPos.z
            };
            const float distanceToAttractor = std::sqrt (delta.x * delta.x
                                                         + delta.y * delta.y
                                                         + delta.z * delta.z);
            metrics.minDistanceToAttractor = juce::jmin (metrics.minDistanceToAttractor, distanceToAttractor);
            metrics.maxCollisionEnergy = juce::jmax (metrics.maxCollisionEnergy, sample.collisionEnergy);
            metrics.startedOutside = metrics.startedOutside || distanceToAttractor > attractorRadius + 0.25f;
            metrics.enteredRadius = metrics.enteredRadius || distanceToAttractor < attractorRadius - 0.05f;
            metrics.lastSample = sample;

            if (block >= 44)
            {
                metrics.settleSumCollisionEnergy += sample.collisionEnergy;
                metrics.settleSumBridgeTransient += bridgeValues.gainTransient;
                ++metrics.settleSamples;
            }
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }

    processor.releaseResources();

    const float settleMeanCollisionEnergy = metrics.settleSamples > 0
        ? metrics.settleSumCollisionEnergy / static_cast<float> (metrics.settleSamples)
        : std::numeric_limits<float>::max();
    const float settleMeanBridgeTransient = metrics.settleSamples > 0
        ? metrics.settleSumBridgeTransient / static_cast<float> (metrics.settleSamples)
        : std::numeric_limits<float>::max();

    const bool baselineQuiet = metrics.baselineMaxCollisionEnergy <= 0.05f
                            && metrics.baselineMaxBridgeTransient <= 0.05f;
    const bool crossingObserved = metrics.startedOutside && metrics.enteredRadius && metrics.minDistanceToAttractor <= 0.95f;
    const bool bridgeTransientVisible = metrics.maxBridgeTransient >= 0.40f;
    const bool sceneTransientVisible = metrics.maxCollisionEnergy >= 0.25f;
    const bool transientDecayed = metrics.settleSamples >= 4
                               && settleMeanCollisionEnergy <= 0.25f
                               && settleMeanBridgeTransient <= 0.25f;

    juce::String detail;
    detail << "emitterId=" << emitterId
           << " baselineMaxCollisionEnergy=" << juce::String (metrics.baselineMaxCollisionEnergy, 3)
           << " maxCollisionEnergy=" << juce::String (metrics.maxCollisionEnergy, 3)
           << " baselineMaxBridgeTransient=" << juce::String (metrics.baselineMaxBridgeTransient, 3)
           << " maxBridgeTransient=" << juce::String (metrics.maxBridgeTransient, 3)
           << " minDistanceToAttractor=" << juce::String (metrics.minDistanceToAttractor, 3)
           << " settleSamples=" << metrics.settleSamples
           << " settleMeanCollisionEnergy=" << juce::String (settleMeanCollisionEnergy, 3)
           << " settleMeanBridgeTransient=" << juce::String (settleMeanBridgeTransient, 3)
           << " finalPos=(" << juce::String (metrics.lastSample.position.x, 3)
           << "," << juce::String (metrics.lastSample.position.y, 3)
           << "," << juce::String (metrics.lastSample.position.z, 3) << ")"
           << " finalCollisionEnergy=" << juce::String (metrics.lastSample.collisionEnergy, 3)
           << " finalBridgeTransient=" << juce::String (metrics.lastBridgeTransient, 3);

    return { baselineQuiet && crossingObserved && bridgeTransientVisible && sceneTransientVisible && transientDecayed, detail };
}
} // namespace

int main()
{
    const auto result = runProbe();
    std::cout << "physics_runtime_attractor_crossing_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;
    return result.passed ? 0 : 1;
}
