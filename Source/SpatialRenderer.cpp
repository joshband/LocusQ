#include "SpatialRenderer.h"

SpatialRenderer::SpatialRenderer()
{
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        steamInitStageIndex.store (static_cast<int> (SteamInitStage::Uninitialized), std::memory_order_relaxed);
#else
        steamInitStageIndex.store (static_cast<int> (SteamInitStage::NotCompiled), std::memory_order_relaxed);
#endif
}

SpatialRenderer::~SpatialRenderer()
{
        shutdown();
    }


void SpatialRenderer::prepare (double sampleRate, int maxBlockSize)
{
        shutdown();
        currentSampleRate = sampleRate;
        currentBlockSize = maxBlockSize;
        ambisonicIrFrameId.store (0, std::memory_order_relaxed);
        ambisonicIrTimestampSamples.store (0, std::memory_order_relaxed);
        ambisonicIrSampleCursor.store (0, std::memory_order_relaxed);
        ambisonicIrOrder.store (0, std::memory_order_relaxed);
        ambisonicIrChannelCount.store (0, std::memory_order_relaxed);
        ambisonicIrNormalizationIndex.store (
            static_cast<int> (AmbisonicNormalization::SN3D),
            std::memory_order_relaxed);
        ambisonicIrHeadphoneRenderAllowed.store (false, std::memory_order_relaxed);
        ambisonicIrFallbackActive.store (false, std::memory_order_relaxed);
        codecMappingFrameId.store (0, std::memory_order_relaxed);
        codecMappingTimestampSamples.store (0, std::memory_order_relaxed);
        codecMappingModeIndex.store (static_cast<int> (CodecMappingMode::None), std::memory_order_relaxed);
        codecMappingMappedChannelCount.store (0, std::memory_order_relaxed);
        codecMappingObjectCount.store (0, std::memory_order_relaxed);
        codecMappingElementCount.store (0, std::memory_order_relaxed);
        codecMappingApplied.store (false, std::memory_order_relaxed);
        codecMappingFallbackActive.store (false, std::memory_order_relaxed);
        codecMappingFinite.store (true, std::memory_order_relaxed);
        codecMappingSignature.store (0, std::memory_order_relaxed);
        codecAdmPayloadActive.store (false, std::memory_order_relaxed);
        codecAdmPayloadFrameId.store (0, std::memory_order_relaxed);
        codecAdmPayloadTimestampSamples.store (0, std::memory_order_relaxed);
        codecAdmPayloadChannelCount.store (0, std::memory_order_relaxed);
        codecAdmPayloadObjectCount.store (0, std::memory_order_relaxed);
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            codecAdmPayloadObjectGain[static_cast<size_t> (i)].store (0.0f, std::memory_order_relaxed);
            codecAdmPayloadObjectAzimuthDeg[static_cast<size_t> (i)].store (0.0f, std::memory_order_relaxed);
        }
        codecIamfPayloadActive.store (false, std::memory_order_relaxed);
        codecIamfPayloadFrameId.store (0, std::memory_order_relaxed);
        codecIamfPayloadTimestampSamples.store (0, std::memory_order_relaxed);
        codecIamfPayloadChannelCount.store (0, std::memory_order_relaxed);
        codecIamfPayloadElementCount.store (0, std::memory_order_relaxed);
        codecIamfPayloadSceneGain.store (0.0f, std::memory_order_relaxed);
        codecIamfPayloadElementGain[0].store (0.0f, std::memory_order_relaxed);
        codecIamfPayloadElementGain[1].store (0.0f, std::memory_order_relaxed);

        // Prepare per-emitter air absorption filters
        for (auto& filter : emitterAbsorption)
            filter.prepare (sampleRate);

        // Prepare per-emitter smoothed gains (4 speakers per emitter)
        for (auto& emitterGains : smoothedSpeakerGains)
            for (auto& g : emitterGains)
                g.reset (sampleRate, 0.020); // 20ms gain ramp

        auto ensureZeroedBuffer = [] (std::vector<float>& buffer, size_t size)
        {
            if (buffer.size() != size)
                buffer = std::vector<float> (size, 0.0f);
            else
                std::fill (buffer.begin(), buffer.end(), 0.0f);
        };

        // Prepare per-speaker delay lines
        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            ensureZeroedBuffer (speakerDelayLines[spk], MAX_DELAY_SAMPLES);
            delayWritePos[spk] = 0;
        }

        for (auto& voiceGains : auditionSmoothedSpeakerGains)
        {
            for (auto& gain : voiceGains)
            {
                gain.reset (sampleRate, 0.015); // 15ms smoothing to avoid block-step buzzing.
                gain.setCurrentAndTargetValue (0.0f);
            }
        }

        // Prepare accumulation buffer (4 channels)
        accumBuffer.setSize (NUM_SPEAKERS, maxBlockSize);

        // Smoothed master gain
        smoothedMasterGain.reset (sampleRate, 0.020);

        // Smoothed speaker trims
        for (auto& trim : smoothedSpeakerTrim)
            trim.reset (sampleRate, 0.020);

        // Temp mono buffer for per-emitter processing
        ensureZeroedBuffer (tempMonoBuffer, static_cast<size_t> (maxBlockSize));

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

        ensureZeroedBuffer (steamBinauralLeft, static_cast<size_t> (maxBlockSize));
        ensureZeroedBuffer (steamBinauralRight, static_cast<size_t> (maxBlockSize));
        for (auto& rotated : headPoseRotatedQuadScratch)
            ensureZeroedBuffer (rotated, static_cast<size_t> (maxBlockSize));
        for (auto& rotated : monitoringHeadPoseRotatedQuadScratch_)
            ensureZeroedBuffer (rotated, static_cast<size_t> (maxBlockSize));
        resetHeadPoseState();
        resetHeadphoneCompensationState();
        for (auto& voiceGains : auditionSmoothedSpeakerGains)
            for (auto& gain : voiceGains)
                gain.setCurrentAndTargetValue (0.0f);
        std::fill (auditionHistoryBuffer.begin(), auditionHistoryBuffer.end(), 0.0f);
        auditionHistoryWritePos = 0;
        resetAuditionVoiceFieldStates();
        resetAuditionReactiveTelemetry();
        preloadBundledPeqPresets();
        lastLoadedPeqPresetIndex = -1;
        lastLoadedPeqSampleRate = 0.0;
        updateHeadphoneCompensationForProfile (HeadphoneDeviceProfile::Generic);
        headphoneCalibrationChain.prepare (sampleRate, maxBlockSize);
        applyRequestedHeadphoneCalibrationSettings();
        publishHeadphoneCalibrationRuntimeState (true);
        initialiseSteamAudioRuntimeIfEnabled();
    }


void SpatialRenderer::reset()
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
        resetHeadPoseState();
        resetHeadphoneCompensationState();
        headphoneCalibrationChain.reset();
        publishHeadphoneCalibrationRuntimeState (false);
        for (auto& voiceGains : auditionSmoothedSpeakerGains)
            for (auto& gain : voiceGains)
                gain.setCurrentAndTargetValue (0.0f);
        std::fill (auditionHistoryBuffer.begin(), auditionHistoryBuffer.end(), 0.0f);
        auditionHistoryWritePos = 0;
        resetAuditionVoiceFieldStates();
        resetAuditionReactiveTelemetry();

#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        if (steamVirtualSurroundEffect != nullptr && iplVirtualSurroundEffectResetFn != nullptr)
            iplVirtualSurroundEffectResetFn (steamVirtualSurroundEffect);
#endif
    }


void SpatialRenderer::shutdown() noexcept
{
        teardownSteamAudioRuntime();
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        setSteamInitStage (SteamInitStage::Uninitialized, 0);
#else
        setSteamInitStage (SteamInitStage::NotCompiled, 0);
#endif
    }


void SpatialRenderer::setDistanceModel (int modelIndex)
{
        const auto clamped = juce::jlimit (0, 3, modelIndex);
        if (distanceModelIndex == clamped)
            return;

        distanceModelIndex = clamped;
        distanceAttenuator.setModel (distanceModelIndex);
    }


void SpatialRenderer::setReferenceDistance (float refDist)
{
        const auto clamped = juce::jlimit (0.1f, 20.0f, refDist);
        if (std::abs (referenceDistance - clamped) < 1.0e-6f)
            return;

        referenceDistance = clamped;
        distanceAttenuator.setReferenceDistance (referenceDistance);
    }


void SpatialRenderer::setMaxDistance (float maxDist)
{
        const auto clamped = juce::jmax (0.1f, maxDist);
        if (std::abs (maxDistance - clamped) < 1.0e-6f)
            return;

        maxDistance = clamped;
        distanceAttenuator.setMaxDistance (maxDistance);
    }


void SpatialRenderer::setAirAbsorptionEnabled (bool enabled)
{
        if (airAbsorptionEnabled == enabled)
            return;

        airAbsorptionEnabled = enabled;
    }


void SpatialRenderer::setDopplerEnabled (bool enabled)
{
        if (dopplerEnabled == enabled)
            return;

        dopplerEnabled = enabled;
    }


void SpatialRenderer::setDopplerScale (float scale)
{
        const auto clamped = juce::jlimit (0.0f, 5.0f, scale);
        if (std::abs (dopplerScale - clamped) < 1.0e-6f)
            return;

        dopplerScale = clamped;
    }


void SpatialRenderer::setRoomEnabled (bool enabled)
{
        if (roomEnabled == enabled)
            return;

        roomEnabled = enabled;
        earlyReflections.setEnabled (enabled);
        fdnReverb.setEnabled (enabled);
    }


void SpatialRenderer::setRoomMix (float newMix)
{
        const auto clamped = juce::jlimit (0.0f, 1.0f, newMix);
        if (std::abs (roomMix - clamped) < 1.0e-6f)
            return;

        roomMix = clamped;
        earlyReflections.setMix (roomMix);
        fdnReverb.setMix (roomMix);
    }


void SpatialRenderer::setRoomSize (float newSize)
{
        const auto clamped = juce::jlimit (0.5f, 5.0f, newSize);
        if (std::abs (roomSize - clamped) < 1.0e-6f)
            return;

        roomSize = clamped;
        earlyReflections.setRoomSize (roomSize);
        fdnReverb.setRoomSize (roomSize);
    }


void SpatialRenderer::setRoomDamping (float newDamping)
{
        const auto clamped = juce::jlimit (0.0f, 1.0f, newDamping);
        if (std::abs (roomDamping - clamped) < 1.0e-6f)
            return;

        roomDamping = clamped;
        earlyReflections.setDamping (roomDamping);
        fdnReverb.setDamping (roomDamping);
    }


void SpatialRenderer::setEarlyReflectionsOnly (bool enabled)
{
        if (earlyReflectionsOnly == enabled)
            return;

        earlyReflectionsOnly = enabled;
        fdnReverb.setEarlyReflectionsOnly (enabled);
    }


void SpatialRenderer::setQualityTier (int qualityIndex)
{
        const auto high = (qualityIndex > 0);
        if (qualityHigh == high)
            return;

        qualityHigh = high;
        earlyReflections.setHighQuality (qualityHigh);
        fdnReverb.setHighQuality (qualityHigh);
    }


void SpatialRenderer::setMasterGain (float gainDb)
{
        const auto clamped = juce::jlimit (-60.0f, 12.0f, gainDb);
        if (std::isfinite (masterGainDb) && std::abs (masterGainDb - clamped) < 1.0e-6f)
            return;

        masterGainDb = clamped;
        smoothedMasterGain.setTargetValue (juce::Decibels::decibelsToGain (masterGainDb, -60.0f));
    }


void SpatialRenderer::setSpeakerTrim (int speakerIdx, float trimDb)
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


void SpatialRenderer::setSpeakerDelay (int speakerIdx, float delayMs)
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


void SpatialRenderer::process (juce::AudioBuffer<float>& outputBuffer, const SceneGraph& scene)
{
        const int numSamples = outputBuffer.getNumSamples();
        const int numOutputChannels = outputBuffer.getNumChannels();

        // Clear accumulation buffer
        accumBuffer.clear();

        const auto emitterStage = runEmitterAccumulationStage (scene, numSamples);
        const bool renderedAuditionEmitter = emitterStage.renderedAuditionEmitter;
        auditionVisualActive.store (renderedAuditionEmitter, std::memory_order_relaxed);

        lastEligibleEmitterCount.store (emitterStage.eligibleEmitterCount, std::memory_order_relaxed);
        lastProcessedEmitterCount.store (emitterStage.processedEmitterCount, std::memory_order_relaxed);
        lastBudgetCulledEmitterCount.store (emitterStage.budgetCulledEmitterCount, std::memory_order_relaxed);
        lastActivityCulledEmitterCount.store (emitterStage.activityCulledEmitterCount, std::memory_order_relaxed);
        lastGuardrailActive.store (emitterStage.eligibleEmitterCount > MAX_RENDER_EMITTERS_PER_BLOCK, std::memory_order_relaxed);

        applyRoomAndSpeakerPostFx (numSamples);
        runOutputRoutingAndHeadphoneStage (
            outputBuffer,
            numSamples,
            numOutputChannels,
            renderedAuditionEmitter);
    }


int SpatialRenderer::getLastEligibleEmitterCount() const noexcept
{
        return lastEligibleEmitterCount.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getLastProcessedEmitterCount() const noexcept
{
        return lastProcessedEmitterCount.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getLastBudgetCulledEmitterCount() const noexcept
{
        return lastBudgetCulledEmitterCount.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getLastActivityCulledEmitterCount() const noexcept
{
        return lastActivityCulledEmitterCount.load (std::memory_order_relaxed);
    }


bool SpatialRenderer::wasGuardrailActiveLastBlock() const noexcept
{
        return lastGuardrailActive.load (std::memory_order_relaxed);
    }


float SpatialRenderer::sanitizeUnitScalar (float value, float fallback) noexcept
{
        if (! std::isfinite (value))
            return juce::jlimit (0.0f, 1.0f, fallback);
        return juce::jlimit (0.0f, 1.0f, value);
    }


int SpatialRenderer::sanitizeSourceCount (int value) noexcept
{
        return juce::jlimit (0, MAX_AUDITION_REACTIVE_SOURCES, value);
    }


int SpatialRenderer::sanitizeHeadphoneFallbackReasonIndex (int value) noexcept
{
        return juce::jlimit (
            0,
            static_cast<int> (AuditionReactiveHeadphoneFallbackReason::OutputIncompatible),
            value);
    }


void SpatialRenderer::processSelectedEmitterCandidate (const SceneGraph& scene,
    const SpatialRenderer::EmitterCandidate& candidate,
    int numSamples,
    SpatialRenderer::EmitterStageResult& result)
{
        const int slotIdx = candidate.slotIdx;
        const auto audioSnapshot = scene.getSlot (slotIdx).readAudioSnapshot();
        const float* emitterAudio = audioSnapshot.mono;
        const int emitterSamples = audioSnapshot.numSamples;

        if (emitterAudio == nullptr || emitterSamples <= 0)
            return;

        const int samplesToProcess = std::min (emitterSamples, numSamples);

        float blockPeak = 0.0f;
        for (int i = 0; i < samplesToProcess; ++i)
        {
            const float sample = emitterAudio[i] * candidate.emitterGainLinear;
            tempMonoBuffer[static_cast<size_t> (i)] = sample;
            blockPeak = juce::jmax (blockPeak, std::abs (sample));
        }

        if (blockPeak < ACTIVITY_PEAK_GATE_LINEAR)
        {
            ++result.activityCulledEmitterCount;
            return;
        }

        ++result.processedEmitterCount;

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

        if (airAbsorptionEnabled && slotIdx < MAX_TRACKED_EMITTERS)
        {
            emitterAbsorption[static_cast<size_t> (slotIdx)].updateForDistance (candidate.distance);
            emitterAbsorption[static_cast<size_t> (slotIdx)].processBlock (tempMonoBuffer.data(), samplesToProcess);
        }

        const float azimuth = calculateAzimuth (candidate.data.position);
        const float elevation = calculateElevation (candidate.data.position);
        auto panGains = vbapPanner.calculateGains (azimuth, elevation);
        auto speakerGains = panGains.gains;
        spreadProcessor.apply (speakerGains, candidate.data.spread);
        directivityFilter.apply (speakerGains,
                                 candidate.data.directivity,
                                 candidate.data.directivityAim,
                                 candidate.data.position);

        if (slotIdx < MAX_TRACKED_EMITTERS)
        {
            for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
            {
                smoothedSpeakerGains[static_cast<size_t> (slotIdx)][static_cast<size_t> (spk)].setTargetValue (
                    speakerGains[static_cast<size_t> (spk)] * candidate.distanceGain);
            }
        }

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


void SpatialRenderer::collectEmitterCandidatesForBlock (const SceneGraph& scene,
    std::array<SpatialRenderer::EmitterCandidate, MAX_RENDER_EMITTERS_PER_BLOCK>& selectedEmitters,
    int& selectedEmitterCount,
    int& selectedMinPriorityIndex,
    float& selectedMinPriority,
    SpatialRenderer::EmitterStageResult& result)
{
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

            ++result.eligibleEmitterCount;

            EmitterCandidate candidate;
            candidate.slotIdx = slotIdx;
            candidate.data = emitterData;
            candidate.distance = distance;
            candidate.distanceGain = distanceGain;
            candidate.emitterGainLinear = emitterGainLinear;
            candidate.priority = priority;

            locusq::spatial_emitter_render_pass::insertCandidateWithBudget (
                selectedEmitters,
                selectedEmitterCount,
                selectedMinPriorityIndex,
                selectedMinPriority,
                candidate,
                result.budgetCulledEmitterCount);
        }
    }


void SpatialRenderer::processSelectedEmittersForBlock (const SceneGraph& scene,
    const std::array<SpatialRenderer::EmitterCandidate, MAX_RENDER_EMITTERS_PER_BLOCK>& selectedEmitters,
    int selectedEmitterCount,
    int numSamples,
    SpatialRenderer::EmitterStageResult& result)
{
        for (int selectedIdx = 0; selectedIdx < selectedEmitterCount; ++selectedIdx)
        {
            const auto& candidate = selectedEmitters[static_cast<size_t> (selectedIdx)];
            processSelectedEmitterCandidate (scene, candidate, numSamples, result);
        }
    }


SpatialRenderer::EmitterStageResult SpatialRenderer::runEmitterAccumulationStage (const SceneGraph& scene, int numSamples)
{
        std::array<EmitterCandidate, MAX_RENDER_EMITTERS_PER_BLOCK> selectedEmitters {};
        int selectedEmitterCount = 0;
        int selectedMinPriorityIndex = -1;
        float selectedMinPriority = std::numeric_limits<float>::max();

        EmitterStageResult result {};

        // First pass: collect eligible emitters and enforce a hard per-block budget.
        collectEmitterCandidatesForBlock (
            scene,
            selectedEmitters,
            selectedEmitterCount,
            selectedMinPriorityIndex,
            selectedMinPriority,
            result);

        // Preserve deterministic ordering when the guardrail is active.
        locusq::spatial_emitter_render_pass::sortSelectedBySlotIndex (selectedEmitters, selectedEmitterCount);

        // Second pass: process only selected emitters.
        processSelectedEmittersForBlock (scene, selectedEmitters, selectedEmitterCount, numSamples, result);

        finalizeEmitterStageWithAuditionFallback (result, numSamples);

        return result;
    }


void SpatialRenderer::applyRoomAndSpeakerPostFx (int numSamples)
{
        locusq::spatial_post_fx_chain::applyRoomFxIfEnabled (
            roomEnabled,
            earlyReflectionsOnly,
            earlyReflections,
            fdnReverb,
            accumBuffer);

        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            auto* channelData = accumBuffer.getWritePointer (spk);
            const int delay = speakerDelaySamples[static_cast<size_t> (spk)];

            locusq::spatial_post_fx_chain::processSpeakerDelayLine (
                channelData,
                numSamples,
                delay,
                speakerDelayLines[static_cast<size_t> (spk)],
                delayWritePos[static_cast<size_t> (spk)],
                MAX_DELAY_SAMPLES);
            locusq::spatial_post_fx_chain::applySpeakerTrim (
                channelData,
                numSamples,
                smoothedSpeakerTrim[static_cast<size_t> (spk)]);
        }
    }
