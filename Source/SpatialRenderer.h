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
#include <atomic>
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
 * Accumulates all emitters into a quad bus, then maps to mono/stereo/quad
 * based on the negotiated host output layout.
 */
class SpatialRenderer
{
public:
    static constexpr int NUM_SPEAKERS = 4;
    static constexpr int MAX_DELAY_SAMPLES = 4410; // 50ms @ 88.2kHz
    // Internal speaker order (VBAP / accumulation): FL, FR, RR, RL.
    static constexpr std::array<int, NUM_SPEAKERS> kQuadOutputSpeakerOrder
    {
        0, 1, 3, 2 // Host quad output order: FL, FR, RL, RR
    };

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
    // Main processing: read scene graph, spatialize active emitters, output
    //==========================================================================
    void process (juce::AudioBuffer<float>& outputBuffer, const SceneGraph& scene)
    {
        const int numSamples = outputBuffer.getNumSamples();
        const int numOutputChannels = outputBuffer.getNumChannels();

        // Clear accumulation buffer
        accumBuffer.clear();

        struct EmitterCandidate
        {
            int slotIdx = -1;
            EmitterData data {};
            float distance = 0.0f;
            float distanceGain = 0.0f;
            float emitterGainLinear = 0.0f;
            float priority = 0.0f;
        };

        std::array<EmitterCandidate, MAX_RENDER_EMITTERS_PER_BLOCK> selectedEmitters {};
        int selectedEmitterCount = 0;
        int selectedMinPriorityIndex = -1;
        float selectedMinPriority = std::numeric_limits<float>::max();

        int eligibleEmitterCount = 0;
        int budgetCulledEmitterCount = 0;
        int activityCulledEmitterCount = 0;
        int processedEmitterCount = 0;

        const auto refreshMinPriority = [&]()
        {
            selectedMinPriorityIndex = -1;
            selectedMinPriority = std::numeric_limits<float>::max();

            for (int i = 0; i < selectedEmitterCount; ++i)
            {
                if (selectedEmitters[static_cast<size_t> (i)].priority < selectedMinPriority)
                {
                    selectedMinPriority = selectedEmitters[static_cast<size_t> (i)].priority;
                    selectedMinPriorityIndex = i;
                }
            }
        };

        // First pass: collect eligible emitters and enforce a hard per-block budget.
        for (int slotIdx = 0; slotIdx < SceneGraph::MAX_EMITTERS; ++slotIdx)
        {
            if (! scene.isSlotActive (slotIdx))
                continue;

            const auto emitterData = scene.getSlot (slotIdx).read();
            if (! emitterData.active || emitterData.muted)
                continue;

            const float emitterGainLinear = juce::Decibels::decibelsToGain (emitterData.gain, -60.0f);
            if (! std::isfinite (emitterGainLinear) || emitterGainLinear <= 0.0f)
                continue;

            const float distance = calculateDistance (emitterData.position);
            if (! std::isfinite (distance))
                continue;

            const float distanceGain = distanceAttenuator.calculateGain (distance);
            if (! std::isfinite (distanceGain) || distanceGain <= 0.0f)
                continue;

            const float priority = emitterGainLinear * distanceGain;
            if (! std::isfinite (priority) || priority < COARSE_PRIORITY_GATE_LINEAR)
                continue;

            ++eligibleEmitterCount;

            EmitterCandidate candidate;
            candidate.slotIdx = slotIdx;
            candidate.data = emitterData;
            candidate.distance = distance;
            candidate.distanceGain = distanceGain;
            candidate.emitterGainLinear = emitterGainLinear;
            candidate.priority = priority;

            if (selectedEmitterCount < MAX_RENDER_EMITTERS_PER_BLOCK)
            {
                selectedEmitters[static_cast<size_t> (selectedEmitterCount)] = candidate;
                ++selectedEmitterCount;
                refreshMinPriority();
                continue;
            }

            if (priority <= selectedMinPriority)
            {
                ++budgetCulledEmitterCount;
                continue;
            }

            if (selectedMinPriorityIndex >= 0)
            {
                selectedEmitters[static_cast<size_t> (selectedMinPriorityIndex)] = candidate;
                ++budgetCulledEmitterCount;
                refreshMinPriority();
            }
        }

        // Preserve deterministic ordering when the guardrail is active.
        for (int i = 1; i < selectedEmitterCount; ++i)
        {
            auto current = selectedEmitters[static_cast<size_t> (i)];
            int j = i - 1;

            while (j >= 0 && selectedEmitters[static_cast<size_t> (j)].slotIdx > current.slotIdx)
            {
                selectedEmitters[static_cast<size_t> (j + 1)] = selectedEmitters[static_cast<size_t> (j)];
                --j;
            }

            selectedEmitters[static_cast<size_t> (j + 1)] = current;
        }

        // Second pass: process only selected emitters.
        for (int selectedIdx = 0; selectedIdx < selectedEmitterCount; ++selectedIdx)
        {
            const auto& candidate = selectedEmitters[static_cast<size_t> (selectedIdx)];
            const int slotIdx = candidate.slotIdx;

            // Get emitter's audio data
            const float* const* emitterAudio = scene.getSlot (slotIdx).getAudioChannels();
            const int emitterChannels = scene.getSlot (slotIdx).getAudioNumChannels();
            const int emitterSamples = scene.getSlot (slotIdx).getAudioNumSamples();

            if (emitterAudio == nullptr || emitterChannels <= 0 || emitterSamples <= 0)
                continue;

            const int samplesToProcess = std::min (emitterSamples, numSamples);

            // Downmix emitter audio to mono and apply emitter gain in one pass.
            float blockPeak = 0.0f;
            for (int i = 0; i < samplesToProcess; ++i)
            {
                float sum = 0.0f;
                for (int ch = 0; ch < emitterChannels; ++ch)
                    sum += emitterAudio[ch][i];

                const float sample = (sum / static_cast<float> (emitterChannels)) * candidate.emitterGainLinear;
                tempMonoBuffer[static_cast<size_t> (i)] = sample;
                blockPeak = juce::jmax (blockPeak, std::abs (sample));
            }

            if (blockPeak < ACTIVITY_PEAK_GATE_LINEAR)
            {
                ++activityCulledEmitterCount;
                continue;
            }

            ++processedEmitterCount;

            // Doppler pitch motion (draft variable-delay implementation)
            if (slotIdx < MAX_TRACKED_EMITTERS)
            {
                emitterDoppler[static_cast<size_t> (slotIdx)].setScale (dopplerScale);
                emitterDoppler[static_cast<size_t> (slotIdx)].processBlock (
                    tempMonoBuffer.data(),
                    samplesToProcess,
                    candidate.data.position,
                    candidate.data.velocity,
                    dopplerEnabled);
            }

            // Apply air absorption (distance-driven LPF)
            if (airAbsorptionEnabled && slotIdx < MAX_TRACKED_EMITTERS)
            {
                emitterAbsorption[static_cast<size_t> (slotIdx)].updateForDistance (candidate.distance);
                emitterAbsorption[static_cast<size_t> (slotIdx)].processBlock (tempMonoBuffer.data(), samplesToProcess);
            }

            // Calculate VBAP gains for this emitter's position
            const float azimuth = calculateAzimuth (candidate.data.position);
            const float elevation = calculateElevation (candidate.data.position);
            auto panGains = vbapPanner.calculateGains (azimuth, elevation);
            auto speakerGains = panGains.gains;

            // Spread (focused -> diffuse blend)
            spreadProcessor.apply (speakerGains, candidate.data.spread);

            // Directivity shaping (speaker-dependent pattern from emitter aim)
            directivityFilter.apply (speakerGains,
                                     candidate.data.directivity,
                                     candidate.data.directivityAim,
                                     candidate.data.position);

            // Update smoothed speaker gains for this emitter
            if (slotIdx < MAX_TRACKED_EMITTERS)
            {
                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                {
                    smoothedSpeakerGains[static_cast<size_t> (slotIdx)][static_cast<size_t> (spk)].setTargetValue (
                        speakerGains[static_cast<size_t> (spk)] * candidate.distanceGain);
                }
            }

            // Accumulate into speaker channels with per-sample gain smoothing
            for (int i = 0; i < samplesToProcess; ++i)
            {
                const float sample = tempMonoBuffer[static_cast<size_t> (i)];

                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                {
                    float gain;
                    if (slotIdx < MAX_TRACKED_EMITTERS)
                    {
                        gain = smoothedSpeakerGains[static_cast<size_t> (slotIdx)][static_cast<size_t> (spk)].getNextValue();
                    }
                    else
                    {
                        gain = speakerGains[static_cast<size_t> (spk)] * candidate.distanceGain;
                    }

                    accumBuffer.addSample (spk, i, sample * gain);
                }
            }
        }

        lastEligibleEmitterCount.store (eligibleEmitterCount, std::memory_order_relaxed);
        lastProcessedEmitterCount.store (processedEmitterCount, std::memory_order_relaxed);
        lastBudgetCulledEmitterCount.store (budgetCulledEmitterCount, std::memory_order_relaxed);
        lastActivityCulledEmitterCount.store (activityCulledEmitterCount, std::memory_order_relaxed);
        lastGuardrailActive.store (eligibleEmitterCount > MAX_RENDER_EMITTERS_PER_BLOCK, std::memory_order_relaxed);

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
            const int delay = speakerDelaySamples[static_cast<size_t> (spk)];

            if (delay > 0)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    speakerDelayLines[static_cast<size_t> (spk)][static_cast<size_t> (delayWritePos[static_cast<size_t> (spk)])] = channelData[i];

                    int readPos = delayWritePos[static_cast<size_t> (spk)] - delay;
                    if (readPos < 0)
                        readPos += MAX_DELAY_SAMPLES;

                    channelData[i] = speakerDelayLines[static_cast<size_t> (spk)][static_cast<size_t> (readPos)];
                    delayWritePos[static_cast<size_t> (spk)] = (delayWritePos[static_cast<size_t> (spk)] + 1) % MAX_DELAY_SAMPLES;
                }
            }

            for (int i = 0; i < numSamples; ++i)
                channelData[i] *= smoothedSpeakerTrim[static_cast<size_t> (spk)].getNextValue();
        }

        // Apply master gain and write to output
        for (int i = 0; i < numSamples; ++i)
        {
            const float masterGain = smoothedMasterGain.getNextValue();

            if (numOutputChannels >= NUM_SPEAKERS)
            {
                // Quad output: explicit host order FL, FR, RL, RR.
                for (int outCh = 0; outCh < NUM_SPEAKERS; ++outCh)
                {
                    const int speakerIdx = kQuadOutputSpeakerOrder[static_cast<size_t> (outCh)];
                    outputBuffer.setSample (outCh, i, accumBuffer.getSample (speakerIdx, i) * masterGain);
                }
            }
            else if (numOutputChannels >= 2)
            {
                // Stereo downmix: FL+RL -> Left, FR+RR -> Right
                const float left = (accumBuffer.getSample (0, i) + accumBuffer.getSample (3, i)) * 0.707f;
                const float right = (accumBuffer.getSample (1, i) + accumBuffer.getSample (2, i)) * 0.707f;
                outputBuffer.setSample (0, i, left * masterGain);
                outputBuffer.setSample (1, i, right * masterGain);
            }
            else if (numOutputChannels == 1)
            {
                float mono = 0.0f;
                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                    mono += accumBuffer.getSample (spk, i);
                outputBuffer.setSample (0, i, mono * 0.5f * masterGain);
            }
        }
    }

    int getLastEligibleEmitterCount() const noexcept
    {
        return lastEligibleEmitterCount.load (std::memory_order_relaxed);
    }

    int getLastProcessedEmitterCount() const noexcept
    {
        return lastProcessedEmitterCount.load (std::memory_order_relaxed);
    }

    int getLastBudgetCulledEmitterCount() const noexcept
    {
        return lastBudgetCulledEmitterCount.load (std::memory_order_relaxed);
    }

    int getLastActivityCulledEmitterCount() const noexcept
    {
        return lastActivityCulledEmitterCount.load (std::memory_order_relaxed);
    }

    bool wasGuardrailActiveLastBlock() const noexcept
    {
        return lastGuardrailActive.load (std::memory_order_relaxed);
    }

private:
    static constexpr int MAX_TRACKED_EMITTERS = 64; // Per-emitter smoothing/filtering
    static constexpr int MAX_RENDER_EMITTERS_PER_BLOCK = 8; // v1-tested CPU envelope
    static constexpr float COARSE_PRIORITY_GATE_LINEAR = 1.0e-5f; // ~ -100 dB
    static constexpr float ACTIVITY_PEAK_GATE_LINEAR = 1.0e-6f;   // ~ -120 dB

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

    // Per-block guardrail stats (read on non-audio threads for diagnostics/UI).
    std::atomic<int> lastEligibleEmitterCount { 0 };
    std::atomic<int> lastProcessedEmitterCount { 0 };
    std::atomic<int> lastBudgetCulledEmitterCount { 0 };
    std::atomic<int> lastActivityCulledEmitterCount { 0 };
    std::atomic<bool> lastGuardrailActive { false };

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
