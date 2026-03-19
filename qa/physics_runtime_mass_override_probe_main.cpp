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
    EmitterSample heavy;
    EmitterSample light;
};

struct MassMetrics
{
    float minDistance = std::numeric_limits<float>::max();
    float maxCollisionEnergy = 0.0f;
    float heavyPostCollisionSumVx = 0.0f;
    float lightPostCollisionSumVx = 0.0f;
    int postCollisionSamples = 0;
    bool collisionSeen = false;
    EmitterSample lastHeavy;
    EmitterSample lastLight;
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

void configureMassEmitter (LocusQAudioProcessor& processor, float posX, float velX, float massOverride)
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
    setActualParam (processor, "phys_collision_radius", 0.70f);
    setActualParam (processor, "phys_mass_override", massOverride);
    setActualParam (processor, "phys_collide_emitters", 1.0f);
    setActualParam (processor, "phys_collision_gain_scale", 1.0f);
    setActualParam (processor, "phys_collision_decay_ms", 50.0f);
    setActualParam (processor, "attractor_0_active", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_throw", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0);
    setActualParam (processor, "rend_phys_interact", 0.0f);
}

SceneSnapshot parseSceneSnapshot (const juce::String& jsonText, int heavyEmitterId, int lightEmitterId)
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

            if (emitterId == heavyEmitterId)
                snapshot.heavy = sample;
            else if (emitterId == lightEmitterId)
                snapshot.light = sample;
        }
    }

    return snapshot;
}

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    LocusQAudioProcessor heavy;
    LocusQAudioProcessor light;
    heavy.setRateAndBufferSizeDetails (sampleRate, blockSize);
    light.setRateAndBufferSizeDetails (sampleRate, blockSize);

    configureMassEmitter (heavy, -0.45f, 4.0f, 4.0f);
    configureMassEmitter (light, 0.45f, -4.0f, 0.5f);

    heavy.prepareToPlay (sampleRate, blockSize);
    light.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> heavyBuffer;
    juce::AudioBuffer<float> lightBuffer;
    heavyBuffer.setSize (channels, blockSize, false, true, true);
    lightBuffer.setSize (channels, blockSize, false, true, true);
    juce::MidiBuffer heavyMidi;
    juce::MidiBuffer lightMidi;

    const int heavyEmitterId = heavy.getEmitterSlotId();
    const int lightEmitterId = light.getEmitterSlotId();
    if (heavyEmitterId < 0 || lightEmitterId < 0 || heavyEmitterId == lightEmitterId)
    {
        heavy.releaseResources();
        light.releaseResources();
        return { false, "emitters did not register as distinct shared slots" };
    }

    for (int warmup = 0; warmup < 4; ++warmup)
    {
        heavyBuffer.clear();
        lightBuffer.clear();
        heavy.processBlock (heavyBuffer, heavyMidi);
        light.processBlock (lightBuffer, lightMidi);
        heavyMidi.clear();
        lightMidi.clear();
        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }

    setActualParam (heavy, "phys_throw", 1.0f);
    setActualParam (light, "phys_throw", 1.0f);
    heavy.processBlock (heavyBuffer, heavyMidi);
    light.processBlock (lightBuffer, lightMidi);
    heavyMidi.clear();
    lightMidi.clear();
    setActualParam (heavy, "phys_throw", 0.0f);
    setActualParam (light, "phys_throw", 0.0f);

    MassMetrics metrics;
    int captureStartBlock = -1;

    for (int block = 0; block < 72; ++block)
    {
        heavyBuffer.clear();
        lightBuffer.clear();

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* heavySamples = heavyBuffer.getWritePointer (channel);
            auto* lightSamples = lightBuffer.getWritePointer (channel);
            for (int i = 0; i < blockSize; ++i)
            {
                const float phase = 0.01f * static_cast<float> (block * blockSize + i);
                heavySamples[i] = 0.04f * std::sin (phase);
                lightSamples[i] = 0.04f * std::cos (phase);
            }
        }

        heavy.processBlock (heavyBuffer, heavyMidi);
        light.processBlock (lightBuffer, lightMidi);
        heavyMidi.clear();
        lightMidi.clear();

        const auto snapshot = parseSceneSnapshot (heavy.getSceneStateJSON(), heavyEmitterId, lightEmitterId);
        if (snapshot.emitterCount >= 2 && snapshot.heavy.found && snapshot.light.found)
        {
            const float distance = std::abs (snapshot.light.x - snapshot.heavy.x);
            metrics.minDistance = juce::jmin (metrics.minDistance, distance);
            metrics.maxCollisionEnergy = juce::jmax (
                metrics.maxCollisionEnergy,
                juce::jmax (snapshot.heavy.collisionEnergy, snapshot.light.collisionEnergy));
            metrics.lastHeavy = snapshot.heavy;
            metrics.lastLight = snapshot.light;

            if (! metrics.collisionSeen
                && (snapshot.heavy.collisionEnergy >= 0.005f || snapshot.light.collisionEnergy >= 0.005f))
            {
                metrics.collisionSeen = true;
                captureStartBlock = block + 1;
            }

            if (captureStartBlock >= 0 && block >= captureStartBlock && metrics.postCollisionSamples < 8)
            {
                metrics.heavyPostCollisionSumVx += snapshot.heavy.vx;
                metrics.lightPostCollisionSumVx += snapshot.light.vx;
                ++metrics.postCollisionSamples;
            }
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }

    heavy.releaseResources();
    light.releaseResources();

    const float heavyMeanPostCollisionVx = metrics.postCollisionSamples > 0
        ? metrics.heavyPostCollisionSumVx / static_cast<float> (metrics.postCollisionSamples)
        : 0.0f;
    const float lightMeanPostCollisionVx = metrics.postCollisionSamples > 0
        ? metrics.lightPostCollisionSumVx / static_cast<float> (metrics.postCollisionSamples)
        : 0.0f;

    const bool collisionSeen = metrics.collisionSeen && metrics.maxCollisionEnergy >= 0.005f;
    const bool overlapSeen = metrics.minDistance <= 0.65f;
    const bool postWindowCaptured = metrics.postCollisionSamples >= 4;
    const bool heavyStayedSlower = postWindowCaptured && heavyMeanPostCollisionVx >= 0.5f && heavyMeanPostCollisionVx <= 3.5f;
    const bool lightExitedFaster = postWindowCaptured
        && lightMeanPostCollisionVx >= heavyMeanPostCollisionVx + 2.0f
        && lightMeanPostCollisionVx >= 4.0f;

    juce::String detail;
    detail << "heavyEmitterId=" << heavyEmitterId
           << " lightEmitterId=" << lightEmitterId
           << " minDistance=" << juce::String (metrics.minDistance, 3)
           << " maxCollisionEnergy=" << juce::String (metrics.maxCollisionEnergy, 4)
           << " postCollisionSamples=" << metrics.postCollisionSamples
           << " heavyMeanPostCollisionVx=" << juce::String (heavyMeanPostCollisionVx, 3)
           << " lightMeanPostCollisionVx=" << juce::String (lightMeanPostCollisionVx, 3)
           << " finalVx=("
           << juce::String (metrics.lastHeavy.vx, 3) << ","
           << juce::String (metrics.lastLight.vx, 3) << ")"
           << " finalX=("
           << juce::String (metrics.lastHeavy.x, 3) << ","
           << juce::String (metrics.lastLight.x, 3) << ")";

    return { collisionSeen && overlapSeen && heavyStayedSlower && lightExitedFaster, detail };
}
} // namespace

int main()
{
    const auto result = runProbe();
    std::cout << "physics_runtime_mass_override_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;
    return result.passed ? 0 : 1;
}
