#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "SceneGraph.h"
#include "VBAPPanner.h"
#include "DistanceAttenuator.h"
#include "AirAbsorption.h"
#include "DopplerProcessor.h"
#include "DirectivityFilter.h"
#include "SpreadProcessor.h"
#include "EarlyReflections.h"
#include "FDNReverb.h"
#include <array>
#include <cmath>
#include <limits>

//==============================================================================
/**
 * SpatialRenderer - Quad Spatialization Engine
 *
 * Phase 2.2: Core spatialization processing.
 * Reads all active emitters from the SceneGraph, applies:
 *   1. VBAP panning (azimuth → 4 speaker gains)
 *   2. Distance attenuation (selected model)
 *   3. Air absorption (distance-driven LPF)
 *   4. Per-speaker delay compensation
 *   5. Per-speaker gain trims
 *   6. Master gain
 *
 * Accumulates all emitters into a stereo output buffer.
 * (Quad output will be enabled when bus layout supports it.)
 */
class SpatialRenderer
{
public:
    static constexpr int NUM_SPEAKERS = 4;
    static constexpr int MAX_DELAY_SAMPLES = 4410; // 50ms @ 88.2kHz

    SpatialRenderer() = default;

    //==========================================================================
    void prepare (double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = maxBlockSize;

        // Prepare per-emitter air absorption filters
        for (auto& filter : emitterAbsorption)
            filter.prepare (sampleRate);

        // Prepare per-emitter smoothed gains (4 speakers per emitter)
        for (auto& emitterGains : smoothedSpeakerGains)
            for (auto& g : emitterGains)
                g.reset (sampleRate, 0.020); // 20ms gain ramp

        // Prepare per-speaker delay lines
        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            speakerDelayLines[spk].resize (MAX_DELAY_SAMPLES, 0.0f);
            delayWritePos[spk] = 0;
        }

        // Prepare accumulation buffer (4 channels)
        accumBuffer.setSize (NUM_SPEAKERS, maxBlockSize);

        // Smoothed master gain
        smoothedMasterGain.reset (sampleRate, 0.020);

        // Smoothed speaker trims
        for (auto& trim : smoothedSpeakerTrim)
            trim.reset (sampleRate, 0.020);

        // Temp mono buffer for per-emitter processing
        tempMonoBuffer.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Prepare per-emitter doppler processors
        for (auto& doppler : emitterDoppler)
            doppler.prepare (sampleRate, maxBlockSize);

        // Prepare room processors
        earlyReflections.prepare (sampleRate, maxBlockSize);
        fdnReverb.prepare (sampleRate, maxBlockSize);

        setQualityTier (qualityHigh ? 1 : 0);
        setDopplerEnabled (dopplerEnabled);
        setDopplerScale (dopplerScale);
        setRoomEnabled (roomEnabled);
        setRoomMix (roomMix);
        setRoomSize (roomSize);
        setRoomDamping (roomDamping);
        setEarlyReflectionsOnly (earlyReflectionsOnly);
    }

    void reset()
    {
        for (auto& filter : emitterAbsorption)
            filter.reset();

        for (auto& dl : speakerDelayLines)
            std::fill (dl.begin(), dl.end(), 0.0f);

        accumBuffer.clear();

        for (auto& doppler : emitterDoppler)
            doppler.reset();

        earlyReflections.reset();
        fdnReverb.reset();
    }

    //==========================================================================
    // Set renderer parameters (called from processBlock before process())
    //==========================================================================

    void setDistanceModel (int modelIndex)
    {
        const auto clamped = juce::jlimit (0, 3, modelIndex);
        if (distanceModelIndex == clamped)
            return;

        distanceModelIndex = clamped;
        distanceAttenuator.setModel (distanceModelIndex);
    }

    void setReferenceDistance (float refDist)
    {
        const auto clamped = juce::jlimit (0.1f, 20.0f, refDist);
        if (std::abs (referenceDistance - clamped) < 1.0e-6f)
            return;

        referenceDistance = clamped;
        distanceAttenuator.setReferenceDistance (referenceDistance);
    }

    void setMaxDistance (float maxDist)
    {
        const auto clamped = juce::jmax (0.1f, maxDist);
        if (std::abs (maxDistance - clamped) < 1.0e-6f)
            return;

        maxDistance = clamped;
        distanceAttenuator.setMaxDistance (maxDistance);
    }

    void setAirAbsorptionEnabled (bool enabled)
    {
        if (airAbsorptionEnabled == enabled)
            return;

        airAbsorptionEnabled = enabled;
    }

    void setDopplerEnabled (bool enabled)
    {
        if (dopplerEnabled == enabled)
            return;

        dopplerEnabled = enabled;
    }

    void setDopplerScale (float scale)
    {
        const auto clamped = juce::jlimit (0.0f, 5.0f, scale);
        if (std::abs (dopplerScale - clamped) < 1.0e-6f)
            return;

        dopplerScale = clamped;
    }

    void setRoomEnabled (bool enabled)
    {
        if (roomEnabled == enabled)
            return;

        roomEnabled = enabled;
        earlyReflections.setEnabled (enabled);
        fdnReverb.setEnabled (enabled);
    }

    void setRoomMix (float newMix)
    {
        const auto clamped = juce::jlimit (0.0f, 1.0f, newMix);
        if (std::abs (roomMix - clamped) < 1.0e-6f)
            return;

        roomMix = clamped;
        earlyReflections.setMix (roomMix);
        fdnReverb.setMix (roomMix);
    }

    void setRoomSize (float newSize)
    {
        const auto clamped = juce::jlimit (0.5f, 5.0f, newSize);
        if (std::abs (roomSize - clamped) < 1.0e-6f)
            return;

        roomSize = clamped;
        earlyReflections.setRoomSize (roomSize);
        fdnReverb.setRoomSize (roomSize);
    }

    void setRoomDamping (float newDamping)
    {
        const auto clamped = juce::jlimit (0.0f, 1.0f, newDamping);
        if (std::abs (roomDamping - clamped) < 1.0e-6f)
            return;

        roomDamping = clamped;
        earlyReflections.setDamping (roomDamping);
        fdnReverb.setDamping (roomDamping);
    }

    void setEarlyReflectionsOnly (bool enabled)
    {
        if (earlyReflectionsOnly == enabled)
            return;

        earlyReflectionsOnly = enabled;
        fdnReverb.setEarlyReflectionsOnly (enabled);
    }

    void setQualityTier (int qualityIndex)
    {
        const auto high = (qualityIndex > 0);
        if (qualityHigh == high)
            return;

        qualityHigh = high;
        earlyReflections.setHighQuality (qualityHigh);
        fdnReverb.setHighQuality (qualityHigh);
    }

    void setMasterGain (float gainDb)
    {
        const auto clamped = juce::jlimit (-60.0f, 12.0f, gainDb);
        if (std::isfinite (masterGainDb) && std::abs (masterGainDb - clamped) < 1.0e-6f)
            return;

        masterGainDb = clamped;
        smoothedMasterGain.setTargetValue (juce::Decibels::decibelsToGain (masterGainDb, -60.0f));
    }

    void setSpeakerTrim (int speakerIdx, float trimDb)
    {
        if (speakerIdx >= 0 && speakerIdx < NUM_SPEAKERS)
        {
            const auto clamped = juce::jlimit (-24.0f, 12.0f, trimDb);
            const auto cached = speakerTrimDb[static_cast<size_t> (speakerIdx)];
            if (std::isfinite (cached) && std::abs (cached - clamped) < 1.0e-6f)
                return;

            speakerTrimDb[static_cast<size_t> (speakerIdx)] = clamped;
            smoothedSpeakerTrim[speakerIdx].setTargetValue (
                juce::Decibels::decibelsToGain (clamped, -24.0f));
        }
    }

    void setSpeakerDelay (int speakerIdx, float delayMs)
    {
        if (speakerIdx >= 0 && speakerIdx < NUM_SPEAKERS)
        {
            const auto clampedMs = juce::jmax (0.0f, delayMs);
            const int delaySamples = static_cast<int> (clampedMs * 0.001f * static_cast<float> (currentSampleRate));
            const int boundedSamples = std::min (delaySamples, MAX_DELAY_SAMPLES - 1);
            if (speakerDelaySamples[speakerIdx] == boundedSamples)
                return;

            speakerDelaySamples[speakerIdx] = boundedSamples;
        }
    }

    //==========================================================================
    // Main processing: read scene graph, spatialize all emitters, output
    //==========================================================================
    void process (juce::AudioBuffer<float>& outputBuffer, const SceneGraph& scene)
    {
        const int numSamples = outputBuffer.getNumSamples();
        const int numOutputChannels = outputBuffer.getNumChannels();

        // Clear accumulation buffer
        accumBuffer.clear();

        // Process each active emitter
        for (int slotIdx = 0; slotIdx < SceneGraph::MAX_EMITTERS; ++slotIdx)
        {
            if (! scene.isSlotActive (slotIdx))
                continue;

            auto emitterData = scene.getSlot (slotIdx).read();
            if (! emitterData.active || emitterData.muted)
                continue;

            // Get emitter's audio data
            const float* const* emitterAudio = scene.getSlot (slotIdx).getAudioChannels();
            int emitterChannels = scene.getSlot (slotIdx).getAudioNumChannels();
            int emitterSamples = scene.getSlot (slotIdx).getAudioNumSamples();

            if (emitterAudio == nullptr || emitterChannels <= 0 || emitterSamples <= 0)
                continue;

            // Use the minimum of emitter and output sample counts
            int samplesToProcess = std::min (emitterSamples, numSamples);

            // Downmix emitter audio to mono for spatialization
            for (int i = 0; i < samplesToProcess; ++i)
            {
                float sum = 0.0f;
                for (int ch = 0; ch < emitterChannels; ++ch)
                    sum += emitterAudio[ch][i];
                tempMonoBuffer[static_cast<size_t> (i)] = sum / static_cast<float> (emitterChannels);
            }

            // Apply emitter gain
            float emitterGainLinear = juce::Decibels::decibelsToGain (emitterData.gain, -60.0f);
            for (int i = 0; i < samplesToProcess; ++i)
                tempMonoBuffer[static_cast<size_t> (i)] *= emitterGainLinear;

            // Doppler pitch motion (draft variable-delay implementation)
            if (slotIdx < MAX_TRACKED_EMITTERS)
            {
                emitterDoppler[static_cast<size_t> (slotIdx)].setScale (dopplerScale);
                emitterDoppler[static_cast<size_t> (slotIdx)].processBlock (
                    tempMonoBuffer.data(), samplesToProcess, emitterData.position, emitterData.velocity, dopplerEnabled);
            }

            // Apply air absorption (distance-driven LPF)
            if (airAbsorptionEnabled && slotIdx < MAX_TRACKED_EMITTERS)
            {
                float distance = calculateDistance (emitterData.position);
                emitterAbsorption[slotIdx].updateForDistance (distance);
                emitterAbsorption[slotIdx].processBlock (tempMonoBuffer.data(), samplesToProcess);
            }

            // Calculate VBAP gains for this emitter's position
            float azimuth = calculateAzimuth (emitterData.position);
            float elevation = calculateElevation (emitterData.position);
            auto panGains = vbapPanner.calculateGains (azimuth, elevation);
            auto speakerGains = panGains.gains;

            // Spread (focused -> diffuse blend)
            spreadProcessor.apply (speakerGains, emitterData.spread);

            // Directivity shaping (speaker-dependent pattern from emitter aim)
            directivityFilter.apply (speakerGains,
                                     emitterData.directivity,
                                     emitterData.directivityAim,
                                     emitterData.position);

            // Apply distance attenuation
            float distance = calculateDistance (emitterData.position);
            float distGain = distanceAttenuator.calculateGain (distance);

            // Update smoothed speaker gains for this emitter
            if (slotIdx < MAX_TRACKED_EMITTERS)
            {
                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                    smoothedSpeakerGains[slotIdx][spk].setTargetValue (speakerGains[spk] * distGain);
            }

            // Accumulate into speaker channels with per-sample gain smoothing
            for (int i = 0; i < samplesToProcess; ++i)
            {
                float sample = tempMonoBuffer[static_cast<size_t> (i)];

                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                {
                    float gain;
                    if (slotIdx < MAX_TRACKED_EMITTERS)
                        gain = smoothedSpeakerGains[slotIdx][spk].getNextValue();
                    else
                        gain = speakerGains[spk] * distGain;

                    accumBuffer.addSample (spk, i, sample * gain);
                }
            }
        }

        // Room acoustics chain (Phase 2.5)
        if (roomEnabled)
        {
            earlyReflections.process (accumBuffer);
            if (! earlyReflectionsOnly)
                fdnReverb.process (accumBuffer);
        }

        // Apply per-speaker delay compensation and gain trims
        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            auto* channelData = accumBuffer.getWritePointer (spk);
            int delay = speakerDelaySamples[spk];

            if (delay > 0)
            {
                // Write to delay line and read back delayed samples
                for (int i = 0; i < numSamples; ++i)
                {
                    speakerDelayLines[spk][static_cast<size_t> (delayWritePos[spk])] = channelData[i];

                    int readPos = delayWritePos[spk] - delay;
                    if (readPos < 0) readPos += MAX_DELAY_SAMPLES;

                    channelData[i] = speakerDelayLines[spk][static_cast<size_t> (readPos)];

                    delayWritePos[spk] = (delayWritePos[spk] + 1) % MAX_DELAY_SAMPLES;
                }
            }

            // Apply speaker trim
            for (int i = 0; i < numSamples; ++i)
                channelData[i] *= smoothedSpeakerTrim[spk].getNextValue();
        }

        // Apply master gain and write to output
        for (int i = 0; i < numSamples; ++i)
        {
            float masterGain = smoothedMasterGain.getNextValue();

            if (numOutputChannels >= NUM_SPEAKERS)
            {
                // Quad output: direct mapping
                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                    outputBuffer.setSample (spk, i, accumBuffer.getSample (spk, i) * masterGain);
            }
            else if (numOutputChannels >= 2)
            {
                // Stereo downmix: FL+RL → Left, FR+RR → Right
                float left  = (accumBuffer.getSample (0, i) + accumBuffer.getSample (3, i)) * 0.707f;
                float right = (accumBuffer.getSample (1, i) + accumBuffer.getSample (2, i)) * 0.707f;
                outputBuffer.setSample (0, i, left  * masterGain);
                outputBuffer.setSample (1, i, right * masterGain);
            }
            else if (numOutputChannels == 1)
            {
                // Mono: sum all speakers
                float mono = 0.0f;
                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                    mono += accumBuffer.getSample (spk, i);
                outputBuffer.setSample (0, i, mono * 0.5f * masterGain);
            }
        }
    }

private:
    static constexpr int MAX_TRACKED_EMITTERS = 64; // Per-emitter smoothing/filtering

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // DSP components
    VBAPPanner vbapPanner;
    DistanceAttenuator distanceAttenuator;
    SpreadProcessor spreadProcessor;
    DirectivityFilter directivityFilter;

    // Per-emitter air absorption filters
    std::array<AirAbsorption, MAX_TRACKED_EMITTERS> emitterAbsorption;
    std::array<DopplerProcessor, MAX_TRACKED_EMITTERS> emitterDoppler;

    // Per-emitter smoothed speaker gains (for click-free panning)
    std::array<std::array<juce::SmoothedValue<float>, NUM_SPEAKERS>, MAX_TRACKED_EMITTERS> smoothedSpeakerGains;

    // Speaker delay lines
    std::array<std::vector<float>, NUM_SPEAKERS> speakerDelayLines;
    std::array<int, NUM_SPEAKERS> delayWritePos {};
    std::array<int, NUM_SPEAKERS> speakerDelaySamples {};

    // Speaker trim gains
    std::array<juce::SmoothedValue<float>, NUM_SPEAKERS> smoothedSpeakerTrim;

    // Master gain
    juce::SmoothedValue<float> smoothedMasterGain { 1.0f };

    // Air absorption toggle
    bool airAbsorptionEnabled = true;
    bool dopplerEnabled = false;
    float dopplerScale = 1.0f;
    bool qualityHigh = false;
    int distanceModelIndex = 0;
    float referenceDistance = 1.0f;
    float maxDistance = 50.0f;
    float masterGainDb = std::numeric_limits<float>::quiet_NaN();
    std::array<float, NUM_SPEAKERS> speakerTrimDb {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()
    };

    // Room acoustics
    bool roomEnabled = true;
    bool earlyReflectionsOnly = false;
    float roomMix = 0.3f;
    float roomSize = 1.0f;
    float roomDamping = 0.5f;
    EarlyReflections earlyReflections;
    FDNReverb fdnReverb;

    // Accumulation buffer (4 channels, one per speaker)
    juce::AudioBuffer<float> accumBuffer;

    // Temp buffer for mono downmix of emitter audio
    std::vector<float> tempMonoBuffer;

    //==========================================================================
    // Coordinate helpers
    //==========================================================================

    static float calculateDistance (const Vec3& pos)
    {
        return std::sqrt (pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    }

    static float calculateAzimuth (const Vec3& pos)
    {
        // Azimuth: angle in XZ plane from front (Z+), clockwise positive
        // atan2(x, z) gives angle from Z+ axis, positive clockwise when X+
        float az = std::atan2 (pos.x, pos.z) * (180.0f / 3.14159265358979323846f);
        return az;
    }

    static float calculateElevation (const Vec3& pos)
    {
        float hDist = std::sqrt (pos.x * pos.x + pos.z * pos.z);
        if (hDist < 0.001f && std::abs (pos.y) < 0.001f)
            return 0.0f;
        return std::atan2 (pos.y, hDist) * (180.0f / 3.14159265358979323846f);
    }
};
