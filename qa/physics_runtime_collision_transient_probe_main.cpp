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

struct EmitterSample
{
    float collisionEnergy = 0.0f;
    bool found = false;
};

struct SceneSnapshot
{
    int emitterCount = 0;
    EmitterSample first;
    EmitterSample second;
};

struct ScenarioMetrics
{
    float peakSceneTransient = 0.0f;
    float peakBridgeTransient = 0.0f;
    float lateSceneTransientSum = 0.0f;
    float lateBridgeTransientSum = 0.0f;
    int lateSampleCount = 0;
    int collisionBlock = -1;
    bool sceneTransientVisible = false;
    bool bridgeTransientVisible = false;
};

struct ScenarioResult
{
    ScenarioMetrics metrics;
    juce::String detail;
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

void configureCollisionTransientEmitter (LocusQAudioProcessor& processor,
                                         float posX,
                                         float velX,
                                         float gainScale,
                                         float decayMs)
{
    setChoiceParam (processor, "mode", 1); // Emitter
    setActualParam (processor, "phys_enable", 1.0f);
    setActualParam (processor, "phys_drag", 0.0f);
    setActualParam (processor, "phys_gravity", 0.0f);
    setActualParam (processor, "rend_phys_pause", 0.0f);
    setChoiceParam (processor, "rend_phys_rate", 1); // 60 Hz
    setActualParam (processor, "rend_phys_wall_collide", 0.0f);
    setChoiceParam (processor, "phys_boundary_mode", 0); // Hard
    setActualParam (processor, "pos_coord_mode", 1.0f); // Cartesian
    setActualParam (processor, "pos_x", posX);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 1.2f);
    setActualParam (processor, "phys_vel_x", velX);
    setActualParam (processor, "phys_vel_y", 0.0f);
    setActualParam (processor, "phys_vel_z", 0.0f);
    setActualParam (processor, "phys_collision_radius", 1.00f);
    setActualParam (processor, "phys_mass_override", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 1.0f);
    setActualParam (processor, "phys_collision_gain_scale", gainScale);
    setActualParam (processor, "phys_collision_decay_ms", decayMs);
    setActualParam (processor, "attractor_0_active", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_throw", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0);
    setActualParam (processor, "rend_phys_interact", 0.0f);
}

SceneSnapshot parseSceneSnapshot (const juce::String& jsonText, int firstEmitterId, int secondEmitterId)
{
    const auto parsed = juce::JSON::parse (jsonText);
    if (! parsed.isObject())
        return {};

    const auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return {};

    SceneSnapshot snapshot;
    snapshot.emitterCount = static_cast<int> (root->getProperty ("emitterCount"));

    const auto emittersVar = root->getProperty ("emitters");
    if (! emittersVar.isArray())
        return snapshot;

    if (const auto* emitters = emittersVar.getArray())
    {
        for (const auto& emitterVar : *emitters)
        {
            const auto* emitter = emitterVar.getDynamicObject();
            if (emitter == nullptr)
                continue;

            const int emitterId = static_cast<int> (emitter->getProperty ("id"));
            EmitterSample sample;
            sample.collisionEnergy = static_cast<float> (emitter->getProperty ("collisionEnergy"));
            sample.found = true;

            if (emitterId == firstEmitterId)
                snapshot.first = sample;
            else if (emitterId == secondEmitterId)
                snapshot.second = sample;
        }
    }

    return snapshot;
}

ScenarioResult runScenario (float gainScale, float decayMs)
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    LocusQAudioProcessor first;
    LocusQAudioProcessor second;
    first.setRateAndBufferSizeDetails (sampleRate, blockSize);
    second.setRateAndBufferSizeDetails (sampleRate, blockSize);

    configureCollisionTransientEmitter (first, -0.25f, 8.0f, gainScale, decayMs);
    configureCollisionTransientEmitter (second, 0.25f, -8.0f, gainScale, decayMs);

    first.prepareToPlay (sampleRate, blockSize);
    second.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> firstBuffer;
    juce::AudioBuffer<float> secondBuffer;
    firstBuffer.setSize (channels, blockSize, false, true, true);
    secondBuffer.setSize (channels, blockSize, false, true, true);
    juce::MidiBuffer firstMidi;
    juce::MidiBuffer secondMidi;

    const int firstEmitterId = first.getEmitterSlotId();
    const int secondEmitterId = second.getEmitterSlotId();
    if (firstEmitterId < 0 || secondEmitterId < 0 || firstEmitterId == secondEmitterId)
    {
        first.releaseResources();
        second.releaseResources();
        return { {}, "emitters did not register as distinct shared slots" };
    }

    for (int warmup = 0; warmup < 4; ++warmup)
    {
        firstBuffer.clear();
        secondBuffer.clear();
        first.processBlock (firstBuffer, firstMidi);
        second.processBlock (secondBuffer, secondMidi);
        firstMidi.clear();
        secondMidi.clear();
        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }

    setActualParam (first, "phys_throw", 1.0f);
    setActualParam (second, "phys_throw", 1.0f);
    first.processBlock (firstBuffer, firstMidi);
    second.processBlock (secondBuffer, secondMidi);
    firstMidi.clear();
    secondMidi.clear();
    setActualParam (first, "phys_throw", 0.0f);
    setActualParam (second, "phys_throw", 0.0f);

    ScenarioMetrics metrics;

    for (int block = 0; block < 72; ++block)
    {
        firstBuffer.clear();
        secondBuffer.clear();

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* firstSamples = firstBuffer.getWritePointer (channel);
            auto* secondSamples = secondBuffer.getWritePointer (channel);
            for (int i = 0; i < blockSize; ++i)
            {
                const float phase = 0.01f * static_cast<float> (block * blockSize + i);
                firstSamples[i] = 0.03f * std::sin (phase);
                secondSamples[i] = 0.03f * std::cos (phase);
            }
        }

        first.processBlock (firstBuffer, firstMidi);
        second.processBlock (secondBuffer, secondMidi);
        firstMidi.clear();
        secondMidi.clear();

        const auto snapshot = parseSceneSnapshot (first.getSceneStateJSON(), firstEmitterId, secondEmitterId);
        const auto firstBridge = first.getPhysicsDspValuesForTesting (firstEmitterId);
        const auto secondBridge = first.getPhysicsDspValuesForTesting (secondEmitterId);

        const float sceneTransient = juce::jmax (snapshot.first.collisionEnergy, snapshot.second.collisionEnergy);
        const float bridgeTransient = juce::jmax (firstBridge.gainTransient, secondBridge.gainTransient);

        metrics.peakSceneTransient = juce::jmax (metrics.peakSceneTransient, sceneTransient);
        metrics.peakBridgeTransient = juce::jmax (metrics.peakBridgeTransient, bridgeTransient);
        metrics.sceneTransientVisible = metrics.sceneTransientVisible || sceneTransient >= 0.005f;
        metrics.bridgeTransientVisible = metrics.bridgeTransientVisible || bridgeTransient >= 0.005f;

        if (metrics.collisionBlock < 0 && (sceneTransient >= 0.005f || bridgeTransient >= 0.005f))
            metrics.collisionBlock = block;

        if (metrics.collisionBlock >= 0
            && block >= metrics.collisionBlock + 3
            && block <= metrics.collisionBlock + 8)
        {
            metrics.lateSceneTransientSum += sceneTransient;
            metrics.lateBridgeTransientSum += bridgeTransient;
            ++metrics.lateSampleCount;
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }

    first.releaseResources();
    second.releaseResources();

    const float lateSceneMean = metrics.lateSampleCount > 0
        ? metrics.lateSceneTransientSum / static_cast<float> (metrics.lateSampleCount)
        : 0.0f;
    const float lateBridgeMean = metrics.lateSampleCount > 0
        ? metrics.lateBridgeTransientSum / static_cast<float> (metrics.lateSampleCount)
        : 0.0f;

    juce::String detail;
    detail << "gainScale=" << juce::String (gainScale, 2)
           << " decayMs=" << juce::String (decayMs, 1)
           << " collisionBlock=" << metrics.collisionBlock
           << " peakSceneTransient=" << juce::String (metrics.peakSceneTransient, 3)
           << " peakBridgeTransient=" << juce::String (metrics.peakBridgeTransient, 3)
           << " lateSceneMean=" << juce::String (lateSceneMean, 3)
           << " lateBridgeMean=" << juce::String (lateBridgeMean, 3);

    return { metrics, detail };
}

ProbeResult runProbe()
{
    const auto lowGain = runScenario (1.00f, 50.0f);
    const auto highGain = runScenario (10.00f, 50.0f);
    const auto shortDecay = runScenario (10.00f, 25.0f);
    const auto longDecay = runScenario (10.00f, 200.0f);

    if (lowGain.metrics.collisionBlock < 0 || highGain.metrics.collisionBlock < 0
        || shortDecay.metrics.collisionBlock < 0 || longDecay.metrics.collisionBlock < 0)
    {
        juce::String detail;
        detail << "collision not observed consistently | "
               << "lowGain={" << lowGain.detail << "} "
               << "highGain={" << highGain.detail << "} "
               << "shortDecay={" << shortDecay.detail << "} "
               << "longDecay={" << longDecay.detail << "}";
        return { false, detail };
    }

    const float highLateBridgeMean = highGain.metrics.lateSampleCount > 0
        ? highGain.metrics.lateBridgeTransientSum / static_cast<float> (highGain.metrics.lateSampleCount)
        : 0.0f;
    const float shortLateBridgeMean = shortDecay.metrics.lateSampleCount > 0
        ? shortDecay.metrics.lateBridgeTransientSum / static_cast<float> (shortDecay.metrics.lateSampleCount)
        : 0.0f;
    const float longLateBridgeMean = longDecay.metrics.lateSampleCount > 0
        ? longDecay.metrics.lateBridgeTransientSum / static_cast<float> (longDecay.metrics.lateSampleCount)
        : 0.0f;

    const bool gainScaleVisible =
        lowGain.metrics.bridgeTransientVisible
        && highGain.metrics.bridgeTransientVisible
        && lowGain.metrics.sceneTransientVisible
        && highGain.metrics.sceneTransientVisible;
    const bool gainScaleResponsive =
        highGain.metrics.peakBridgeTransient >= lowGain.metrics.peakBridgeTransient + 0.05f
        && highGain.metrics.peakSceneTransient >= lowGain.metrics.peakSceneTransient + 0.05f;
    const bool decayVisible =
        shortDecay.metrics.bridgeTransientVisible
        && longDecay.metrics.bridgeTransientVisible
        && shortDecay.metrics.lateSampleCount >= 4
        && longDecay.metrics.lateSampleCount >= 4;
    const bool decayResponsive =
        longLateBridgeMean >= shortLateBridgeMean + 0.05f;

    juce::String detail;
    detail << "lowGain={" << lowGain.detail << "} "
           << "highGain={" << highGain.detail << "} "
           << "shortDecay={" << shortDecay.detail << "} "
           << "longDecay={" << longDecay.detail << "} "
           << "gainDeltaBridge=" << juce::String (highGain.metrics.peakBridgeTransient - lowGain.metrics.peakBridgeTransient, 3)
           << " gainDeltaScene=" << juce::String (highGain.metrics.peakSceneTransient - lowGain.metrics.peakSceneTransient, 3)
           << " decayLateDeltaBridge=" << juce::String (longLateBridgeMean - shortLateBridgeMean, 3)
           << " highLateBridgeMean=" << juce::String (highLateBridgeMean, 3);

    return { gainScaleVisible && gainScaleResponsive && decayVisible && decayResponsive, detail };
}
} // namespace

int main()
{
    const auto result = runProbe();
    std::cout << "physics_runtime_collision_transient_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;
    return result.passed ? 0 : 1;
}
