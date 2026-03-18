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
    float x = 0.0f;
    float vx = 0.0f;
    float collisionEnergy = 0.0f;
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

void configureCollisionEmitter (LocusQAudioProcessor& processor, float posX, float velX)
{
    setChoiceParam (processor, "mode", 1); // Emitter
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
    setActualParam (processor, "phys_vel_x", velX);
    setActualParam (processor, "phys_vel_y", 0.0f);
    setActualParam (processor, "phys_vel_z", 0.0f);
    setActualParam (processor, "phys_collision_radius", 0.70f);
    setActualParam (processor, "phys_mass_override", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 1.0f);
    setActualParam (processor, "phys_collision_gain_scale", 1.0f);
    setActualParam (processor, "phys_collision_decay_ms", 50.0f);
    setActualParam (processor, "attractor_0_active", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_throw", 0.0f);
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

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    LocusQAudioProcessor first;
    LocusQAudioProcessor second;
    first.setRateAndBufferSizeDetails (sampleRate, blockSize);
    second.setRateAndBufferSizeDetails (sampleRate, blockSize);

    configureCollisionEmitter (first, -0.45f, 4.0f);
    configureCollisionEmitter (second, 0.45f, -4.0f);

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

    bool sawBothEmitters = false;
    bool velocityReversed = false;
    float initialDistance = 0.0f;
    float minDistance = std::numeric_limits<float>::max();
    float finalDistance = 0.0f;
    float maxCollisionEnergy = 0.0f;
    EmitterSample lastFirst;
    EmitterSample lastSecond;

    bool initialDistanceCaptured = false;
    float maxAbsX = 0.0f;

    const auto preThrowSnapshot = parseSceneSnapshot (first.getSceneStateJSON(), firstEmitterId, secondEmitterId);
    if (preThrowSnapshot.emitterCount >= 2 && preThrowSnapshot.first.found && preThrowSnapshot.second.found)
    {
        initialDistance = std::abs (preThrowSnapshot.second.x - preThrowSnapshot.first.x);
        initialDistanceCaptured = initialDistance >= 0.85f;
    }

    for (int block = 0; block < 80; ++block)
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
                firstSamples[i] = 0.05f * std::sin (phase);
                secondSamples[i] = 0.05f * std::cos (phase);
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
            if (! initialDistanceCaptured && distance >= 0.85f)
            {
                initialDistance = distance;
                initialDistanceCaptured = true;
            }

            if (initialDistanceCaptured)
            {
                minDistance = juce::jmin (minDistance, distance);
                finalDistance = distance;
                maxAbsX = juce::jmax (maxAbsX, juce::jmax (std::abs (snapshot.first.x), std::abs (snapshot.second.x)));
                maxCollisionEnergy = juce::jmax (
                    maxCollisionEnergy,
                    juce::jmax (snapshot.first.collisionEnergy, snapshot.second.collisionEnergy));
                velocityReversed = velocityReversed
                                   || (snapshot.first.vx < -0.05f && snapshot.second.vx > 0.05f);
                lastFirst = snapshot.first;
                lastSecond = snapshot.second;
            }
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }

    first.releaseResources();
    second.releaseResources();

    const bool approached = sawBothEmitters && initialDistanceCaptured && minDistance < (initialDistance - 0.15f);
    const bool collisionSeen = maxCollisionEnergy >= 0.005f;
    const bool separatedAfterCollision = sawBothEmitters && finalDistance > (minDistance + 0.20f);
    const bool boundedInRoom = maxAbsX <= 3.01f;
    const bool containedAwayFromWalls = maxAbsX <= 2.10f;
    const bool settledTowardRestWindow = finalDistance <= 4.00f;
    const bool oppositeDirectionMotion =
        std::abs (lastFirst.vx) > 0.05f
        && std::abs (lastSecond.vx) > 0.05f
        && (lastFirst.vx * lastSecond.vx) < 0.0f;

    juce::String detail;
    detail << "emitterIds=(" << firstEmitterId << "," << secondEmitterId << ")"
           << " initialDistance=" << juce::String (initialDistance, 3)
           << " minDistance=" << juce::String (minDistance, 3)
           << " finalDistance=" << juce::String (finalDistance, 3)
           << " maxAbsX=" << juce::String (maxAbsX, 3)
           << " maxCollisionEnergy=" << juce::String (maxCollisionEnergy, 4)
           << " finalVx=(" << juce::String (lastFirst.vx, 3) << "," << juce::String (lastSecond.vx, 3) << ")"
           << " finalX=(" << juce::String (lastFirst.x, 3) << "," << juce::String (lastSecond.x, 3) << ")";

    return { sawBothEmitters && approached && collisionSeen && separatedAfterCollision
             && oppositeDirectionMotion && boundedInRoom && containedAwayFromWalls
             && settledTowardRestWindow,
             detail };
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto result = runProbe();
    std::cout << "physics_runtime_collision_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;

    return result.passed ? 0 : 1;
}
