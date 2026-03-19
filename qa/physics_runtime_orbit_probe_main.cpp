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
    bool found = false;
};

struct OrbitMetrics
{
    float initialRadius = 0.0f;
    float minRadius = std::numeric_limits<float>::max();
    float maxRadius = 0.0f;
    float settleMinRadius = std::numeric_limits<float>::max();
    float settleMaxRadius = 0.0f;
    float settleSumRadius = 0.0f;
    float maxAngularDeviationDegrees = 0.0f;
    int settleSampleCount = 0;
    Vec3 initialRelativePosition {};
    Vec3 lastPosition {};
    bool capturedInitialRadius = false;
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
            sample.found = true;
            return sample;
        }
    }

    return {};
}

void configureOrbitEmitter (LocusQAudioProcessor& processor)
{
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
    setActualParam (processor, "pos_x", 2.0f);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 0.0f);
    setActualParam (processor, "phys_vel_x", 0.0f);
    setActualParam (processor, "phys_vel_y", 0.0f);
    setActualParam (processor, "phys_vel_z", 2.0f);
    setActualParam (processor, "phys_throw", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0);
    setActualParam (processor, "rend_phys_interact", 0.0f);
    setActualParam (processor, "phys_ang_enable", 0.0f);
    setActualParam (processor, "attractor_0_pos_x", 0.0f);
    setActualParam (processor, "attractor_0_pos_y", 0.0f);
    setActualParam (processor, "attractor_0_pos_z", 0.0f);
    setActualParam (processor, "attractor_0_strength", 8.0f);
    setActualParam (processor, "attractor_0_radius", 10.0f);
    setChoiceParam (processor, "attractor_0_falloff", 1); // InverseR2
    setActualParam (processor, "attractor_0_orbit_stabilize", 1.0f);
    setActualParam (processor, "attractor_0_active", 1.0f);
}

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;
    constexpr Vec3 attractorPosition { 0.0f, 0.0f, 0.0f };

    LocusQAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (sampleRate, blockSize);
    configureOrbitEmitter (processor);
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

    buffer.clear();
    setActualParam (processor, "phys_throw", 1.0f);
    processor.processBlock (buffer, midi);
    midi.clear();
    setActualParam (processor, "phys_throw", 0.0f);

    OrbitMetrics metrics;

    for (int block = 0; block < 72; ++block)
    {
        buffer.clear();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = 0.04f * std::sin (0.01f * static_cast<float> (block * buffer.getNumSamples() + i));
        }

        processor.processBlock (buffer, midi);
        midi.clear();

        const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
        if (sample.found)
        {
            const float relX = sample.position.x - attractorPosition.x;
            const float relY = sample.position.y - attractorPosition.y;
            const float relZ = sample.position.z - attractorPosition.z;
            const float radius = std::sqrt (relX * relX + relY * relY + relZ * relZ);
            if (! metrics.capturedInitialRadius && radius >= 0.25f)
            {
                metrics.initialRadius = radius;
                metrics.initialRelativePosition = { relX, relY, relZ };
                metrics.capturedInitialRadius = true;
            }

            if (metrics.capturedInitialRadius)
            {
                metrics.minRadius = juce::jmin (metrics.minRadius, radius);
                metrics.maxRadius = juce::jmax (metrics.maxRadius, radius);

                const float initialNorm = std::sqrt (metrics.initialRelativePosition.x * metrics.initialRelativePosition.x
                                                   + metrics.initialRelativePosition.y * metrics.initialRelativePosition.y
                                                   + metrics.initialRelativePosition.z * metrics.initialRelativePosition.z);
                const float currentNorm = radius;
                if (initialNorm > 0.001f && currentNorm > 0.001f)
                {
                    const float dot = metrics.initialRelativePosition.x * relX
                                    + metrics.initialRelativePosition.y * relY
                                    + metrics.initialRelativePosition.z * relZ;
                    const float cosTheta = juce::jlimit (-1.0f, 1.0f, dot / (initialNorm * currentNorm));
                    const float angularDeviation = std::acos (cosTheta) * 180.0f / juce::MathConstants<float>::pi;
                    metrics.maxAngularDeviationDegrees = juce::jmax (metrics.maxAngularDeviationDegrees, angularDeviation);
                }
            }

            metrics.lastPosition = sample.position;

            if (block >= 56)
            {
                metrics.settleMinRadius = juce::jmin (metrics.settleMinRadius, radius);
                metrics.settleMaxRadius = juce::jmax (metrics.settleMaxRadius, radius);
                metrics.settleSumRadius += radius;
                ++metrics.settleSampleCount;
            }
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    processor.releaseResources();

    const float settleMeanRadius = metrics.settleSampleCount > 0
        ? metrics.settleSumRadius / static_cast<float> (metrics.settleSampleCount)
        : 0.0f;
    const float settleRangeRadius = metrics.settleSampleCount > 0
        ? metrics.settleMaxRadius - metrics.settleMinRadius
        : std::numeric_limits<float>::max();
    const float settleRangePct = settleMeanRadius > 0.001f
        ? (settleRangeRadius / settleMeanRadius) * 100.0f
        : 1000.0f;
    const bool initialCaptured = metrics.capturedInitialRadius && metrics.initialRadius >= 1.70f && metrics.initialRadius <= 2.30f;
    const bool orbitMoved = metrics.maxAngularDeviationDegrees >= 20.0f;
    const bool settleWindowCaptured = metrics.settleSampleCount >= 6;
    const bool settleBandOk = settleWindowCaptured
        && settleMeanRadius >= 1.60f
        && settleMeanRadius <= 2.40f
        && settleRangePct <= 18.0f;

    juce::String detail;
    detail << "emitterId=" << emitterId
           << " initialRadius=" << juce::String (metrics.initialRadius, 3)
           << " minRadius=" << juce::String (metrics.minRadius, 3)
           << " maxRadius=" << juce::String (metrics.maxRadius, 3)
           << " settleSamples=" << metrics.settleSampleCount
           << " settleMeanRadius=" << juce::String (settleMeanRadius, 3)
           << " settleRangeRadius=" << juce::String (settleRangeRadius, 3)
           << " settleRangePct=" << juce::String (settleRangePct, 2)
           << " maxAngularDeviationDeg=" << juce::String (metrics.maxAngularDeviationDegrees, 2)
           << " finalPos=("
           << juce::String (metrics.lastPosition.x, 3) << ","
           << juce::String (metrics.lastPosition.y, 3) << ","
           << juce::String (metrics.lastPosition.z, 3) << ")";

    return { initialCaptured && orbitMoved && settleBandOk, detail };
}
} // namespace

int main()
{
    const auto result = runProbe();
    std::cout << "physics_runtime_orbit_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;
    return result.passed ? 0 : 1;
}
