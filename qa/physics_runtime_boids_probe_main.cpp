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
    float spread = 0.0f;
    bool found = false;
};

struct SceneSnapshot
{
    int emitterCount = 0;
    EmitterSample first;
    EmitterSample second;
};

struct BoidsSettleMetrics
{
    float settleMinDistance = std::numeric_limits<float>::max();
    float settleMaxDistance = 0.0f;
    float settleSumDistance = 0.0f;
    int settleSampleCount = 0;
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

void configureBoidsEmitter (LocusQAudioProcessor& processor, float posX)
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

    setChoiceParam (processor, "phys_flock_group", 1); // Group 1
    setActualParam (processor, "phys_flock_0_enable", 1.0f);
    setActualParam (processor, "phys_flock_0_sep_weight", 0.10f);
    setActualParam (processor, "phys_flock_0_align_weight", 0.35f);
    setActualParam (processor, "phys_flock_0_coh_weight", 0.90f);
    setActualParam (processor, "phys_flock_0_sep_radius", 0.75f);
    setActualParam (processor, "phys_flock_0_align_radius", 4.0f);
    setActualParam (processor, "phys_flock_0_coh_radius", 8.0f);
    setActualParam (processor, "phys_flock_0_max_speed", 1.5f);

    setActualParam (processor, "attractor_0_active", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 0.0f);
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
            sample.x = static_cast<float> (emitter->getProperty ("x"));
            sample.spread = static_cast<float> (emitter->getProperty ("spread"));
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

    configureBoidsEmitter (first, -1.5f);
    configureBoidsEmitter (second, 1.5f);

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
    float initialDistance = 0.0f;
    float minDistance = std::numeric_limits<float>::max();
    float maxSpread = 0.0f;
    BoidsSettleMetrics settleMetrics;

    const auto baselineSnapshot = parseSceneSnapshot (first.getSceneStateJSON(), firstEmitterId, secondEmitterId);
    if (baselineSnapshot.emitterCount >= 2 && baselineSnapshot.first.found && baselineSnapshot.second.found)
    {
        initialDistance = std::abs (baselineSnapshot.second.x - baselineSnapshot.first.x);
        initialDistanceCaptured = initialDistance >= 1.0f;
    }

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
                firstSamples[i] = 0.04f * std::sin (phase);
                secondSamples[i] = 0.04f * std::cos (phase);
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
            if (! initialDistanceCaptured && distance >= 1.0f)
            {
                initialDistance = distance;
                initialDistanceCaptured = true;
            }
            if (initialDistanceCaptured)
                minDistance = juce::jmin (minDistance, distance);
            maxSpread = juce::jmax (maxSpread, juce::jmax (snapshot.first.spread, snapshot.second.spread));

            if (block >= 56 && initialDistanceCaptured)
            {
                settleMetrics.settleMinDistance = juce::jmin (settleMetrics.settleMinDistance, distance);
                settleMetrics.settleMaxDistance = juce::jmax (settleMetrics.settleMaxDistance, distance);
                settleMetrics.settleSumDistance += distance;
                ++settleMetrics.settleSampleCount;
            }
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    first.releaseResources();
    second.releaseResources();

    const bool capturedValidBaseline = initialDistanceCaptured && initialDistance >= 1.0f;
    const bool approached = capturedValidBaseline && minDistance < (initialDistance - 0.25f);
    const bool spreadLive = maxSpread >= 0.20f;
    const float settleMeanDistance = settleMetrics.settleSampleCount > 0
        ? settleMetrics.settleSumDistance / static_cast<float> (settleMetrics.settleSampleCount)
        : 0.0f;
    const float settleRangeDistance = settleMetrics.settleSampleCount > 0
        ? settleMetrics.settleMaxDistance - settleMetrics.settleMinDistance
        : std::numeric_limits<float>::max();
    const bool settleWindowCaptured = settleMetrics.settleSampleCount >= 4;
    const bool settleBandOk = settleWindowCaptured
        && settleMeanDistance >= 2.20f
        && settleMeanDistance <= 2.70f
        && settleRangeDistance <= 0.30f;

    juce::String detail;
    detail << "emitterIds=(" << firstEmitterId << "," << secondEmitterId << ")"
           << " initialDistance=" << juce::String (initialDistance, 3)
           << " minDistance=" << juce::String (minDistance, 3)
           << " settleSamples=" << settleMetrics.settleSampleCount
           << " settleMeanDistance=" << juce::String (settleMeanDistance, 3)
           << " settleRangeDistance=" << juce::String (settleRangeDistance, 3)
           << " maxSpread=" << juce::String (maxSpread, 3);

    return { sawBothEmitters && capturedValidBaseline && approached && spreadLive && settleBandOk, detail };
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto result = runProbe();
    std::cout << "physics_runtime_boids_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;

    return result.passed ? 0 : 1;
}
