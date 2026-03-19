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
    std::uint8_t collisionMask = 0;
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
            sample.position.x = static_cast<float> (emitter->getProperty ("x"));
            sample.position.y = static_cast<float> (emitter->getProperty ("y"));
            sample.position.z = static_cast<float> (emitter->getProperty ("z"));
            sample.collisionMask = static_cast<std::uint8_t> (static_cast<int> (emitter->getProperty ("collisionMask")));
            sample.found = true;
            return sample;
        }
    }

    return {};
}

struct BoundaryMetrics
{
    float minX = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float settleMinX = std::numeric_limits<float>::max();
    float settleMaxX = -std::numeric_limits<float>::max();
    float settleSumX = 0.0f;
    int settleSampleCount = 0;
    std::uint8_t maxCollisionMask = 0;
    Vec3 lastPosition {};
};

BoundaryMetrics runAndSampleMetrics (LocusQAudioProcessor& processor,
                                     juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midi,
                                     int blocks,
                                     int sleepMs,
                                     int settleWindowBlocks = 0)
{
    const int emitterId = processor.getEmitterSlotId();
    BoundaryMetrics metrics;

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = 0.05f * std::sin (0.015f * static_cast<float> (block * buffer.getNumSamples() + i));
        }

        processor.processBlock (buffer, midi);
        midi.clear();

        const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
        if (sample.found)
        {
            metrics.minX = juce::jmin (metrics.minX, sample.position.x);
            metrics.maxX = juce::jmax (metrics.maxX, sample.position.x);
            metrics.maxCollisionMask = static_cast<std::uint8_t> (metrics.maxCollisionMask | sample.collisionMask);
            metrics.lastPosition = sample.position;

            if (settleWindowBlocks > 0 && block >= (blocks - settleWindowBlocks))
            {
                metrics.settleMinX = juce::jmin (metrics.settleMinX, sample.position.x);
                metrics.settleMaxX = juce::jmax (metrics.settleMaxX, sample.position.x);
                metrics.settleSumX += sample.position.x;
                ++metrics.settleSampleCount;
            }
        }

        if (sleepMs > 0)
            std::this_thread::sleep_for (std::chrono::milliseconds (sleepMs));
    }

    return metrics;
}

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;
    constexpr float expectedWallX = 3.0f;

    LocusQAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (sampleRate, blockSize);

    setChoiceParam (processor, "mode", 1); // Emitter
    setActualParam (processor, "phys_enable", 1.0f);
    setActualParam (processor, "phys_drag", 0.0f);
    setActualParam (processor, "phys_gravity", 0.0f);
    setActualParam (processor, "rend_phys_pause", 0.0f);
    setChoiceParam (processor, "rend_phys_rate", 1); // 60 Hz
    setActualParam (processor, "rend_phys_wall_collide", 1.0f);
    setChoiceParam (processor, "phys_boundary_mode", 0); // Hard
    setActualParam (processor, "pos_coord_mode", 1.0f); // Cartesian
    setActualParam (processor, "pos_x", 2.85f);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 1.2f);
    setActualParam (processor, "phys_vel_x", 6.0f);
    setActualParam (processor, "phys_vel_y", 0.0f);
    setActualParam (processor, "phys_vel_z", 0.0f);
    setActualParam (processor, "attractor_0_active", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0);
    setActualParam (processor, "rend_phys_interact", 0.0f);
    setActualParam (processor, "phys_throw", 0.0f);

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

    setActualParam (processor, "phys_throw", 1.0f);
    processor.processBlock (buffer, midi);
    midi.clear();
    setActualParam (processor, "phys_throw", 0.0f);

    const auto metrics = runAndSampleMetrics (processor, buffer, midi, 40, 10, 8);
    processor.releaseResources();

    const bool clampedToWall = metrics.maxX <= (expectedWallX + 0.01f);
    const bool reachedWall = metrics.maxX >= 2.98f;
    const bool collisionSeen = (metrics.maxCollisionMask & 0x1u) != 0;
    const bool bouncedBack = metrics.lastPosition.x < metrics.maxX - 0.02f;
    const float settleMeanX = metrics.settleSampleCount > 0
        ? metrics.settleSumX / static_cast<float> (metrics.settleSampleCount)
        : 0.0f;
    const float settleRangeX = metrics.settleSampleCount > 0
        ? metrics.settleMaxX - metrics.settleMinX
        : std::numeric_limits<float>::max();
    const bool settleWindowCaptured = metrics.settleSampleCount >= 4;
    const bool settleBandOk = settleWindowCaptured
        && settleMeanX >= 0.80f
        && settleMeanX <= 1.80f
        && settleRangeX <= 0.90f;

    juce::String detail;
    detail << "emitterId=" << emitterId
           << " minX=" << juce::String (metrics.minX, 3)
           << " maxX=" << juce::String (metrics.maxX, 3)
           << " settleSamples=" << metrics.settleSampleCount
           << " settleMeanX=" << juce::String (settleMeanX, 3)
           << " settleRangeX=" << juce::String (settleRangeX, 3)
           << " collisionMask=" << juce::String (static_cast<int> (metrics.maxCollisionMask))
           << " finalPos=("
           << juce::String (metrics.lastPosition.x, 3) << ","
           << juce::String (metrics.lastPosition.y, 3) << ","
           << juce::String (metrics.lastPosition.z, 3) << ")";

    return { clampedToWall && reachedWall && collisionSeen && bouncedBack && settleBandOk, detail };
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto result = runProbe();
    std::cout << "physics_runtime_boundary_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;

    return result.passed ? 0 : 1;
}
