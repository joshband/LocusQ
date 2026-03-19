#include "Source/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <cmath>
#include <functional>
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
    Vec3 aim { 0.0f, 0.0f, 0.0f };
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
            sample.aim.x = static_cast<float> (emitter->getProperty ("aimX"));
            sample.aim.y = static_cast<float> (emitter->getProperty ("aimY"));
            sample.aim.z = static_cast<float> (emitter->getProperty ("aimZ"));
            sample.found = true;
            return sample;
        }
    }

    return {};
}

struct AngularMetrics
{
    float baselineAimX = 0.0f;
    float baselineAimY = 0.0f;
    float baselineAimZ = 0.0f;
    float maxAbsAimYAfterThrow = 0.0f;
    float maxAimNormError = 0.0f;
    float resetMeanAimY = 0.0f;
    float resetMeanAimZ = 0.0f;
    int resetSamples = 0;
    Vec3 lastAim {};
    bool baselineCaptured = false;
};

void runBlocks (LocusQAudioProcessor& processor,
                juce::AudioBuffer<float>& buffer,
                juce::MidiBuffer& midi,
                int blocks,
                int sleepMs,
                const std::function<void(const SceneSample&, int)>& onSample)
{
    const int emitterId = processor.getEmitterSlotId();

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = 0.05f * std::sin (0.011f * static_cast<float> (block * buffer.getNumSamples() + i));
        }

        processor.processBlock (buffer, midi);
        midi.clear();

        const auto sample = findEmitterSampleFromSceneSnapshot (processor.getSceneStateJSON(), emitterId);
        if (sample.found)
            onSample (sample, block);

        if (sleepMs > 0)
            std::this_thread::sleep_for (std::chrono::milliseconds (sleepMs));
    }
}

ProbeResult runProbe()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

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
    setActualParam (processor, "pos_x", 0.0f);
    setActualParam (processor, "pos_y", 0.0f);
    setActualParam (processor, "pos_z", 1.2f);
    setActualParam (processor, "emit_dir_azimuth", 180.0f);
    setActualParam (processor, "emit_dir_elevation", 0.0f);
    setActualParam (processor, "attractor_0_active", 0.0f);
    setActualParam (processor, "phys_spring_enable", 0.0f);
    setActualParam (processor, "phys_turbulence", 0.0f);
    setActualParam (processor, "phys_collide_emitters", 0.0f);
    setChoiceParam (processor, "phys_flock_group", 0);
    setActualParam (processor, "rend_phys_interact", 0.0f);
    setActualParam (processor, "phys_ang_enable", 1.0f);
    setActualParam (processor, "phys_ang_drag", 0.15f);
    setActualParam (processor, "phys_ang_impulse_x", 18.0f);
    setActualParam (processor, "phys_ang_impulse_y", 0.0f);
    setActualParam (processor, "phys_ang_impulse_z", 0.0f);
    setActualParam (processor, "phys_ang_attractor_torque", 0.0f);
    setActualParam (processor, "phys_ang_throw", 0.0f);
    setActualParam (processor, "phys_ang_reset", 0.0f);

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

    AngularMetrics metrics;

    runBlocks (processor, buffer, midi, 8, 8,
               [&] (const SceneSample& sample, int)
               {
                   metrics.baselineCaptured = true;
                   metrics.baselineAimX = sample.aim.x;
                   metrics.baselineAimY = sample.aim.y;
                   metrics.baselineAimZ = sample.aim.z;
                   metrics.lastAim = sample.aim;
               });

    setActualParam (processor, "phys_ang_throw", 1.0f);
    runBlocks (processor, buffer, midi, 1, 8, [] (const SceneSample&, int) {});
    setActualParam (processor, "phys_ang_throw", 0.0f);

    runBlocks (processor, buffer, midi, 16, 8,
               [&] (const SceneSample& sample, int)
               {
                   metrics.maxAbsAimYAfterThrow = juce::jmax (metrics.maxAbsAimYAfterThrow, std::abs (sample.aim.y));
                   const float aimNorm = std::sqrt (sample.aim.x * sample.aim.x
                                                  + sample.aim.y * sample.aim.y
                                                  + sample.aim.z * sample.aim.z);
                   metrics.maxAimNormError = juce::jmax (metrics.maxAimNormError, std::abs (1.0f - aimNorm));
                   metrics.lastAim = sample.aim;
               });

    setActualParam (processor, "phys_ang_reset", 1.0f);
    runBlocks (processor, buffer, midi, 1, 8, [] (const SceneSample&, int) {});
    setActualParam (processor, "phys_ang_reset", 0.0f);

    runBlocks (processor, buffer, midi, 8, 8,
               [&] (const SceneSample& sample, int)
               {
                   metrics.resetMeanAimY += sample.aim.y;
                   metrics.resetMeanAimZ += sample.aim.z;
                   ++metrics.resetSamples;
                   metrics.lastAim = sample.aim;
               });

    processor.releaseResources();

    const float resetMeanAimY = metrics.resetSamples > 0
        ? metrics.resetMeanAimY / static_cast<float> (metrics.resetSamples)
        : 0.0f;
    const float resetMeanAimZ = metrics.resetSamples > 0
        ? metrics.resetMeanAimZ / static_cast<float> (metrics.resetSamples)
        : 0.0f;

    const bool baselineOk = metrics.baselineCaptured
        && std::abs (metrics.baselineAimY) <= 0.05f
        && std::abs (metrics.baselineAimX) <= 0.05f
        && metrics.baselineAimZ <= -0.95f;
    const bool throwVisible = metrics.maxAbsAimYAfterThrow >= 0.15f;
    const bool normStable = metrics.maxAimNormError <= 0.05f;
    const bool resetCaptured = metrics.resetSamples >= 4;
    const bool resetOk = resetCaptured
        && std::abs (resetMeanAimY) <= 0.08f
        && resetMeanAimZ <= -0.92f;

    juce::String detail;
    detail << "emitterId=" << emitterId
           << " baselineCaptured=" << (metrics.baselineCaptured ? "1" : "0")
           << " baselineAim=("
           << juce::String (metrics.baselineAimX, 3) << ","
           << juce::String (metrics.baselineAimY, 3) << ","
           << juce::String (metrics.baselineAimZ, 3) << ")"
           << " maxAbsAimYAfterThrow=" << juce::String (metrics.maxAbsAimYAfterThrow, 3)
           << " maxAimNormError=" << juce::String (metrics.maxAimNormError, 4)
           << " resetSamples=" << metrics.resetSamples
           << " resetMeanAim=("
           << juce::String (0.0f, 3) << ","
           << juce::String (resetMeanAimY, 3) << ","
           << juce::String (resetMeanAimZ, 3) << ")"
           << " finalAim=("
           << juce::String (metrics.lastAim.x, 3) << ","
           << juce::String (metrics.lastAim.y, 3) << ","
           << juce::String (metrics.lastAim.z, 3) << ")";

    return { baselineOk && throwVisible && normStable && resetOk, detail };
}
} // namespace

int main()
{
    const auto result = runProbe();
    std::cout << "physics_runtime_angular_probe: "
              << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << std::endl;
    return result.passed ? 0 : 1;
}
