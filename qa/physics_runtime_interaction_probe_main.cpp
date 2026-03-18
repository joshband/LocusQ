#include "Source/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <cmath>
#include <iostream>
#include <limits>
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
    float x = 0.0f;
    float vx = 0.0f;
    float fx = 0.0f;
    bool found = false;
};

struct SceneSnapshot
{
    int emitterCount = 0;
    EmitterSample first;
    EmitterSample second;
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

void configureInteractionEmitter (LocusQAudioProcessor& processor, float posX)
{
    setChoiceParam (processor, "mode", 1); // Emitter
    setActualParam (processor, "emit_spread", 0.0f);
    setActualParam (processor, "phys_enable", 1.0f);
    setActualParam (processor, "phys_drag", 0.0f);
    setActualParam (processor, "phys_gravity", 0.0f);
    setActualParam (processor, "rend_phys_pause", 0.0f);
    setChoiceParam (processor, "rend_phys_rate", 1); // 60 Hz
    setActualParam (processor, "rend_phys_wall_collide", 1.0f);
    setActualParam (processor, "rend_phys_interact", 1.0f);
    setChoiceParam (processor, "phys_boundary_mode", 0); // Hard
    setActualParam (processor, "pos_coord_mode", 1.0f); // Cartesian
    setActualParam (processor, "pos_x", posX);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 1.2f);
    setActualParam (processor, "phys_vel_x", 0.0f);
    setActualParam (processor, "phys_vel_y", 0.0f);
    setActualParam (processor, "phys_vel_z", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 0.0f);
    setActualParam (processor, "attractor_0_active", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0); // Off
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
            sample.x = static_cast<float> (emitter->getProperty ("x"));
            sample.vx = static_cast<float> (emitter->getProperty ("vx"));
            sample.fx = static_cast<float> (emitter->getProperty ("fx"));
            sample.found = true;

            if (emitterId == firstEmitterId)
                snapshot.first = sample;
            else if (emitterId == secondEmitterId)
                snapshot.second = sample;
        }
    }

    return snapshot;
}

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    LocusQAudioProcessor first;
    LocusQAudioProcessor second;
    first.setRateAndBufferSizeDetails (sampleRate, blockSize);
    second.setRateAndBufferSizeDetails (sampleRate, blockSize);

    configureInteractionEmitter (first, -0.35f);
    configureInteractionEmitter (second, 0.35f);

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
        return { false, "emitters did not register as distinct shared slots" };
    }

    for (int warmup = 0; warmup < 8; ++warmup)
    {
        firstBuffer.clear();
        secondBuffer.clear();
        first.processBlock (firstBuffer, firstMidi);
        second.processBlock (secondBuffer, secondMidi);
        firstMidi.clear();
        secondMidi.clear();
        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    bool sawBothEmitters = false;
    bool initialDistanceCaptured = false;
    bool oppositeForcesSeen = false;
    float initialDistance = 0.0f;
    float maxDistance = 0.0f;
    float maxAbsForce = 0.0f;
    EmitterSample lastFirst;
    EmitterSample lastSecond;

    for (int block = 0; block < 64; ++block)
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
        if (snapshot.emitterCount >= 2 && snapshot.first.found && snapshot.second.found)
        {
            sawBothEmitters = true;
            const float distance = std::abs (snapshot.second.x - snapshot.first.x);
            if (! initialDistanceCaptured && distance >= 0.50f)
            {
                initialDistance = distance;
                initialDistanceCaptured = true;
            }

            if (initialDistanceCaptured)
                maxDistance = juce::jmax (maxDistance, distance);
            maxAbsForce = juce::jmax (
                maxAbsForce,
                juce::jmax (std::abs (snapshot.first.fx), std::abs (snapshot.second.fx)));
            oppositeForcesSeen = oppositeForcesSeen
                                 || (snapshot.first.fx < -0.05f && snapshot.second.fx > 0.05f);
            lastFirst = snapshot.first;
            lastSecond = snapshot.second;
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    first.releaseResources();
    second.releaseResources();

    const bool separated = initialDistanceCaptured && maxDistance > (initialDistance + 0.20f);
    const bool forceLive = maxAbsForce >= 0.50f;
    const bool oppositeVelocitySeen =
        (lastFirst.vx < -0.05f) && (lastSecond.vx > 0.05f);

    juce::String detail;
    detail << "emitterIds=(" << firstEmitterId << "," << secondEmitterId << ")"
           << " initialDistance=" << juce::String (initialDistance, 3)
           << " maxDistance=" << juce::String (maxDistance, 3)
           << " maxAbsForce=" << juce::String (maxAbsForce, 3)
           << " finalFx=(" << juce::String (lastFirst.fx, 3) << "," << juce::String (lastSecond.fx, 3) << ")"
           << " finalVx=(" << juce::String (lastFirst.vx, 3) << "," << juce::String (lastSecond.vx, 3) << ")";

    return { sawBothEmitters && initialDistanceCaptured && separated && forceLive && oppositeForcesSeen && oppositeVelocitySeen,
             detail };
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto result = runProbe();
    std::cout << "physics_runtime_interaction_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;

    return result.passed ? 0 : 1;
}
