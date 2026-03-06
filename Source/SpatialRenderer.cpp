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


void SpatialRenderer::setHeadphoneRenderMode (int modeIndex)
{
        const auto clamped = juce::jlimit (0, 1, modeIndex);
        if (requestedHeadphoneModeIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedHeadphoneModeIndex.store (clamped, std::memory_order_relaxed);
    }


void SpatialRenderer::setHeadphoneDeviceProfile (int profileIndex)
{
        const auto clamped = juce::jlimit (0, NUM_HEADPHONE_DEVICE_PROFILES - 1, profileIndex);
        if (requestedHeadphoneProfileIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedHeadphoneProfileIndex.store (clamped, std::memory_order_relaxed);
    }


void SpatialRenderer::loadPeqPresetForProfile (int profileIndex, double sampleRate)
{
        const auto clampedProfileIndex = juce::jlimit (0, NUM_HEADPHONE_DEVICE_PROFILES - 1, profileIndex);
        if (lastLoadedPeqPresetIndex == clampedProfileIndex && lastLoadedPeqSampleRate == sampleRate)
            return;

        const auto& preset = bundledPeqPresets[static_cast<size_t> (clampedProfileIndex)].preset;

        if (sampleRate <= 0.0 || ! preset.valid || preset.bands.empty())
        {
            headphoneCalibrationChain.clearPeqPreset();
            lastLoadedPeqPresetIndex = clampedProfileIndex;
            lastLoadedPeqSampleRate  = sampleRate;
            return;
        }

        headphoneCalibrationChain.clearPeqPreset();

        // NOTE: clearPeqPreset -> setPeqPreampDb -> setPeqStage writes are not atomic with respect
        // to the audio thread. A brief glitch may occur during a profile switch while audio is
        // processing. This is acceptable: profile changes are non-RT events on the message thread.
        headphoneCalibrationChain.setPeqPreampDb (preset.preampDb);

        const auto sr = static_cast<float> (sampleRate);
        const int maxStages = juce::jmin (
            static_cast<int> (preset.bands.size()),
            locusq::headphone_dsp::HeadphonePeqHook::kMaxStages);

        for (int i = 0; i < maxStages; ++i)
        {
            const auto& band = preset.bands[static_cast<size_t> (i)];
            locusq::headphone_dsp::HeadphonePeqHook::Coefficients c;
            switch (band.type)
            {
                case locusq::headphone_dsp::PeqBandSpec::Type::LSC:
                    c = locusq::headphone_dsp::HeadphonePeqHook::makeLowShelf  (band.fcHz, band.gainDb, band.q, sr); break;
                case locusq::headphone_dsp::PeqBandSpec::Type::HSC:
                    c = locusq::headphone_dsp::HeadphonePeqHook::makeHighShelf (band.fcHz, band.gainDb, band.q, sr); break;
                default:
                    c = locusq::headphone_dsp::HeadphonePeqHook::makePeakEQ    (band.fcHz, band.gainDb, band.q, sr); break;
            }
            headphoneCalibrationChain.setPeqStage (i, c);
        }

        lastLoadedPeqPresetIndex = clampedProfileIndex;
        lastLoadedPeqSampleRate  = sampleRate;
    }


void SpatialRenderer::applyJsonPeqBands (const juce::var& bandsArray, float preampDb, double sampleRate)
{
        headphoneCalibrationChain.clearPeqPreset();
        headphoneCalibrationChain.setPeqPreampDb (preampDb);

        if (! bandsArray.isArray())
            return;

        const auto sr = static_cast<float> (sampleRate);
        const int maxStages = juce::jmin (
            bandsArray.getArray()->size(),
            locusq::headphone_dsp::HeadphonePeqHook::kMaxStages);

        for (int i = 0; i < maxStages; ++i)
        {
            auto* band = (*bandsArray.getArray())[i].getDynamicObject();
            if (band == nullptr)
                continue;

            const auto typeStr = band->getProperty ("type").toString().trim().toUpperCase();
            const auto fcHz    = static_cast<float> (static_cast<double> (band->getProperty ("fc_hz")));
            const auto gainDb  = static_cast<float> (static_cast<double> (band->getProperty ("gain_db")));
            const auto q       = static_cast<float> (static_cast<double> (band->getProperty ("q")));

            locusq::headphone_dsp::HeadphonePeqHook::Coefficients c;
            if (typeStr == "LSC")
                c = locusq::headphone_dsp::HeadphonePeqHook::makeLowShelf  (fcHz, gainDb, q, sr);
            else if (typeStr == "HSC")
                c = locusq::headphone_dsp::HeadphonePeqHook::makeHighShelf (fcHz, gainDb, q, sr);
            else
                c = locusq::headphone_dsp::HeadphonePeqHook::makePeakEQ    (fcHz, gainDb, q, sr);

            headphoneCalibrationChain.setPeqStage (i, c);
        }
    }


void SpatialRenderer::setHeadphoneCalibrationEnabled (bool enabled) noexcept
{
        if (requestedHeadphoneCalibrationEnabled.load (std::memory_order_relaxed) == enabled)
            return;

        requestedHeadphoneCalibrationEnabled.store (enabled, std::memory_order_relaxed);
    }


void SpatialRenderer::setHeadphoneCalibrationEngine (int engineIndex) noexcept
{
        if (requestedHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed) == engineIndex)
            return;

        requestedHeadphoneCalibrationEngineIndex.store (engineIndex, std::memory_order_relaxed);
    }


int SpatialRenderer::getCalibrationLatencySamples() const noexcept
{
        return headphoneCalibrationChain.getActiveLatencySamples();
    }


void SpatialRenderer::setSpatialOutputProfile (int profileIndex)
{
        const auto clamped = juce::jlimit (0, 11, profileIndex);
        if (requestedSpatialProfileIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedSpatialProfileIndex.store (clamped, std::memory_order_relaxed);
    }


void SpatialRenderer::applyHeadPose (const SpatialRenderer::PoseSnapshot& pose) noexcept
{
        if (! std::isfinite (pose.qx)
            || ! std::isfinite (pose.qy)
            || ! std::isfinite (pose.qz)
            || ! std::isfinite (pose.qw))
        {
            return;
        }

        const float normSq = (pose.qx * pose.qx)
                           + (pose.qy * pose.qy)
                           + (pose.qz * pose.qz)
                           + (pose.qw * pose.qw);
        if (! std::isfinite (normSq) || normSq < 1.0e-12f)
            return;

        const float invNorm = 1.0f / std::sqrt (normSq);
        headPoseSnapshot.qx = pose.qx * invNorm;
        headPoseSnapshot.qy = pose.qy * invNorm;
        headPoseSnapshot.qz = pose.qz * invNorm;
        headPoseSnapshot.qw = pose.qw * invNorm;
        headPoseSnapshot.timestampMs = pose.timestampMs;
        headPoseSnapshot.seq = pose.seq;
        headPoseSnapshot.pad = 0;
        headPoseValid = true;

        updateHeadPoseOrientationFromSnapshot();
        rebuildHeadPoseSpeakerMix();
    }


int SpatialRenderer::getHeadphoneRenderModeRequestedIndex() const noexcept
{
        return requestedHeadphoneModeIndex.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getHeadphoneRenderModeActiveIndex() const noexcept
{
        return activeHeadphoneModeIndex.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getHeadphoneDeviceProfileRequestedIndex() const noexcept
{
        return requestedHeadphoneProfileIndex.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getHeadphoneDeviceProfileActiveIndex() const noexcept
{
        return activeHeadphoneProfileIndex.load (std::memory_order_relaxed);
    }


bool SpatialRenderer::isHeadphoneCalibrationEnabledRequested() const noexcept
{
        return requestedHeadphoneCalibrationEnabled.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getHeadphoneCalibrationEngineRequestedIndex() const noexcept
{
        return locusq::headphone_core::sanitizeCalibrationEngineIndex (
            requestedHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed));
    }


int SpatialRenderer::getHeadphoneCalibrationEngineActiveIndex() const noexcept
{
        return locusq::headphone_core::sanitizeCalibrationEngineIndex (
            activeHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed));
    }


int SpatialRenderer::getHeadphoneCalibrationFallbackReasonIndex() const noexcept
{
        return locusq::headphone_core::sanitizeCalibrationFallbackReasonIndex (
            activeHeadphoneCalibrationFallbackReasonIndex.load (std::memory_order_relaxed));
    }


int SpatialRenderer::getHeadphoneCalibrationLatencySamples() const noexcept
{
        return locusq::headphone_core::sanitizeCalibrationLatencySamples (
            activeHeadphoneCalibrationLatencySamples.load (std::memory_order_relaxed));
    }


int SpatialRenderer::getSpatialOutputProfileRequestedIndex() const noexcept
{
        return requestedSpatialProfileIndex.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getSpatialOutputProfileActiveIndex() const noexcept
{
        return activeSpatialProfileIndex.load (std::memory_order_relaxed);
    }


int SpatialRenderer::getSpatialProfileStageIndex() const noexcept
{
        return activeSpatialStageIndex.load (std::memory_order_relaxed);
    }


SpatialRenderer::AmbisonicIrContractSnapshot SpatialRenderer::getAmbisonicIrContractSnapshot() const noexcept
{
        AmbisonicIrContractSnapshot snapshot;
        snapshot.frameId = ambisonicIrFrameId.load (std::memory_order_relaxed);
        snapshot.timestampSamples = ambisonicIrTimestampSamples.load (std::memory_order_relaxed);
        snapshot.order = ambisonicIrOrder.load (std::memory_order_relaxed);
        snapshot.normalizationIndex = ambisonicIrNormalizationIndex.load (std::memory_order_relaxed);
        snapshot.channelCount = ambisonicIrChannelCount.load (std::memory_order_relaxed);
        snapshot.requestedSpatialProfileIndex = requestedSpatialProfileIndex.load (std::memory_order_relaxed);
        snapshot.activeSpatialProfileIndex = activeSpatialProfileIndex.load (std::memory_order_relaxed);
        snapshot.activeSpatialStageIndex = activeSpatialStageIndex.load (std::memory_order_relaxed);
        snapshot.requestedHeadphoneModeIndex = requestedHeadphoneModeIndex.load (std::memory_order_relaxed);
        snapshot.activeHeadphoneModeIndex = activeHeadphoneModeIndex.load (std::memory_order_relaxed);
        snapshot.steamAudioAvailable = steamAudioAvailable.load (std::memory_order_relaxed);
        snapshot.headphoneRenderAllowed = ambisonicIrHeadphoneRenderAllowed.load (std::memory_order_relaxed);
        snapshot.fallbackActive = ambisonicIrFallbackActive.load (std::memory_order_relaxed);
        return snapshot;
    }


SpatialRenderer::CodecMappingExecutionSnapshot SpatialRenderer::getCodecMappingExecutionSnapshot() const noexcept
{
        CodecMappingExecutionSnapshot snapshot;
        snapshot.frameId = codecMappingFrameId.load (std::memory_order_relaxed);
        snapshot.timestampSamples = codecMappingTimestampSamples.load (std::memory_order_relaxed);
        snapshot.modeIndex = codecMappingModeIndex.load (std::memory_order_relaxed);
        snapshot.mappedChannelCount = codecMappingMappedChannelCount.load (std::memory_order_relaxed);
        snapshot.objectCount = codecMappingObjectCount.load (std::memory_order_relaxed);
        snapshot.elementCount = codecMappingElementCount.load (std::memory_order_relaxed);
        snapshot.mappingApplied = codecMappingApplied.load (std::memory_order_relaxed);
        snapshot.fallbackActive = codecMappingFallbackActive.load (std::memory_order_relaxed);
        snapshot.finite = codecMappingFinite.load (std::memory_order_relaxed);
        snapshot.signature = codecMappingSignature.load (std::memory_order_relaxed);
        return snapshot;
    }


SpatialRenderer::CodecAdmRuntimePayloadSnapshot SpatialRenderer::getCodecAdmRuntimePayloadSnapshot() const noexcept
{
        CodecAdmRuntimePayloadSnapshot snapshot;
        snapshot.active = codecAdmPayloadActive.load (std::memory_order_relaxed);
        snapshot.frameId = codecAdmPayloadFrameId.load (std::memory_order_relaxed);
        snapshot.timestampSamples = codecAdmPayloadTimestampSamples.load (std::memory_order_relaxed);
        snapshot.channelCount = codecAdmPayloadChannelCount.load (std::memory_order_relaxed);
        snapshot.objectCount = codecAdmPayloadObjectCount.load (std::memory_order_relaxed);
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            snapshot.objectGain[static_cast<size_t> (i)] =
                codecAdmPayloadObjectGain[static_cast<size_t> (i)].load (std::memory_order_relaxed);
            snapshot.objectAzimuthDeg[static_cast<size_t> (i)] =
                codecAdmPayloadObjectAzimuthDeg[static_cast<size_t> (i)].load (std::memory_order_relaxed);
        }
        return snapshot;
    }


SpatialRenderer::CodecIamfRuntimePayloadSnapshot SpatialRenderer::getCodecIamfRuntimePayloadSnapshot() const noexcept
{
        CodecIamfRuntimePayloadSnapshot snapshot;
        snapshot.active = codecIamfPayloadActive.load (std::memory_order_relaxed);
        snapshot.frameId = codecIamfPayloadFrameId.load (std::memory_order_relaxed);
        snapshot.timestampSamples = codecIamfPayloadTimestampSamples.load (std::memory_order_relaxed);
        snapshot.channelCount = codecIamfPayloadChannelCount.load (std::memory_order_relaxed);
        snapshot.elementCount = codecIamfPayloadElementCount.load (std::memory_order_relaxed);
        snapshot.sceneGain = codecIamfPayloadSceneGain.load (std::memory_order_relaxed);
        snapshot.elementGain[0] = codecIamfPayloadElementGain[0].load (std::memory_order_relaxed);
        snapshot.elementGain[1] = codecIamfPayloadElementGain[1].load (std::memory_order_relaxed);
        return snapshot;
    }


const char* SpatialRenderer::headphoneRenderModeToString (int modeIndex) noexcept
{
        switch (juce::jlimit (0, 1, modeIndex))
        {
            case static_cast<int> (HeadphoneRenderMode::SteamBinaural): return "steam_binaural";
            case static_cast<int> (HeadphoneRenderMode::StereoDownmix):
            default: break;
        }

        return "stereo_downmix";
    }


const char* SpatialRenderer::headphoneDeviceProfileToString (int profileIndex) noexcept
{
        switch (juce::jlimit (0, NUM_HEADPHONE_DEVICE_PROFILES - 1, profileIndex))
        {
            case static_cast<int> (HeadphoneDeviceProfile::AirPodsPro2): return "airpods_pro_2";
            case static_cast<int> (HeadphoneDeviceProfile::AirPodsPro3): return "airpods_pro_3";
            case static_cast<int> (HeadphoneDeviceProfile::SonyWH1000XM5): return "sony_wh1000xm5";
            case static_cast<int> (HeadphoneDeviceProfile::CustomSOFA): return "custom_sofa";
            case static_cast<int> (HeadphoneDeviceProfile::Generic):
            default: break;
        }

        return "generic";
    }


const char* SpatialRenderer::headphoneCalibrationEngineToString (int engineIndex) noexcept
{
        return locusq::headphone_core::calibrationChainEngineToString (engineIndex);
    }


const char* SpatialRenderer::headphoneCalibrationFallbackReasonToString (int reasonIndex) noexcept
{
        return locusq::headphone_core::calibrationChainFallbackReasonToString (reasonIndex);
    }


const char* SpatialRenderer::spatialOutputProfileToString (int profileIndex) noexcept
{
        switch (static_cast<SpatialOutputProfile> (profileIndex))
        {
            case SpatialOutputProfile::Auto: return "auto";
            case SpatialOutputProfile::Stereo20: return "stereo_2_0";
            case SpatialOutputProfile::Quad40: return "quad_4_0";
            case SpatialOutputProfile::Surround521: return "surround_5_2_1";
            case SpatialOutputProfile::Surround721: return "surround_7_2_1";
            case SpatialOutputProfile::Surround742: return "surround_7_4_2";
            case SpatialOutputProfile::AmbisonicFOA: return "ambisonic_foa";
            case SpatialOutputProfile::AmbisonicHOA: return "ambisonic_hoa";
            case SpatialOutputProfile::AtmosBed: return "atmos_bed";
            case SpatialOutputProfile::Virtual3dStereo: return "virtual_3d_stereo";
            case SpatialOutputProfile::CodecIAMF: return "codec_iamf";
            case SpatialOutputProfile::CodecADM: return "codec_adm";
            default: break;
        }

        return "auto";
    }


const char* SpatialRenderer::spatialProfileStageToString (int stageIndex) noexcept
{
        switch (static_cast<SpatialProfileStage> (stageIndex))
        {
            case SpatialProfileStage::Direct: return "direct";
            case SpatialProfileStage::FallbackStereo: return "fallback_stereo";
            case SpatialProfileStage::FallbackQuad: return "fallback_quad";
            case SpatialProfileStage::AmbiDecodeStereo: return "ambi_decode_stereo";
            case SpatialProfileStage::CodecLayoutPlaceholder: return "codec_layout_placeholder";
            default: break;
        }

        return "direct";
    }


const char* SpatialRenderer::ambisonicNormalizationToString (int normalizationIndex) noexcept
{
        switch (juce::jlimit (0, 1, normalizationIndex))
        {
            case static_cast<int> (AmbisonicNormalization::N3D): return "n3d";
            case static_cast<int> (AmbisonicNormalization::SN3D):
            default:
                break;
        }

        return "sn3d";
    }


const char* SpatialRenderer::codecMappingModeToString (int modeIndex) noexcept
{
        switch (juce::jlimit (0, 2, modeIndex))
        {
            case static_cast<int> (CodecMappingMode::ADM): return "adm";
            case static_cast<int> (CodecMappingMode::IAMF): return "iamf";
            case static_cast<int> (CodecMappingMode::None):
            default:
                break;
        }

        return "none";
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


void SpatialRenderer::publishCodecAdmPayloadContract (bool codecAdmPayloadActiveThisBlock,
    std::uint64_t codecFrameId,
    std::uint64_t contractTimestampSamples,
    int codecMappedChannelCount,
    int codecObjectCount)
{
        codecAdmPayloadActive.store (codecAdmPayloadActiveThisBlock, std::memory_order_relaxed);
        codecAdmPayloadFrameId.store (codecFrameId, std::memory_order_relaxed);
        codecAdmPayloadTimestampSamples.store (contractTimestampSamples, std::memory_order_relaxed);
        codecAdmPayloadChannelCount.store (codecMappedChannelCount, std::memory_order_relaxed);
        codecAdmPayloadObjectCount.store (
            codecAdmPayloadActiveThisBlock ? codecObjectCount : 0,
            std::memory_order_relaxed);
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            const bool objectActive = codecAdmPayloadActiveThisBlock && i < codecObjectCount;
            codecAdmPayloadObjectGain[static_cast<size_t> (i)].store (
                objectActive ? kCodecAdmObjectDefaultGains[static_cast<size_t> (i)] : 0.0f,
                std::memory_order_relaxed);
            codecAdmPayloadObjectAzimuthDeg[static_cast<size_t> (i)].store (
                objectActive ? kCodecAdmObjectAzimuthDeg[static_cast<size_t> (i)] : 0.0f,
                std::memory_order_relaxed);
        }
    }


void SpatialRenderer::publishCodecIamfPayloadContract (bool codecIamfPayloadActiveThisBlock,
    std::uint64_t codecFrameId,
    std::uint64_t contractTimestampSamples,
    int codecMappedChannelCount,
    int codecElementCount)
{
        codecIamfPayloadActive.store (codecIamfPayloadActiveThisBlock, std::memory_order_relaxed);
        codecIamfPayloadFrameId.store (codecFrameId, std::memory_order_relaxed);
        codecIamfPayloadTimestampSamples.store (contractTimestampSamples, std::memory_order_relaxed);
        codecIamfPayloadChannelCount.store (codecMappedChannelCount, std::memory_order_relaxed);
        codecIamfPayloadElementCount.store (
            codecIamfPayloadActiveThisBlock ? codecElementCount : 0,
            std::memory_order_relaxed);
        codecIamfPayloadSceneGain.store (
            codecIamfPayloadActiveThisBlock ? 1.0f : 0.0f,
            std::memory_order_relaxed);
        for (int i = 0; i < 2; ++i)
        {
            const bool elementActive = codecIamfPayloadActiveThisBlock && i < codecElementCount;
            codecIamfPayloadElementGain[static_cast<size_t> (i)].store (
                elementActive ? kCodecIamfDefaultElementGains[static_cast<size_t> (i)] : 0.0f,
                std::memory_order_relaxed);
        }
    }


int SpatialRenderer::determineCodecMappedChannelCount (SpatialRenderer::CodecMappingMode codecMode, int numOutputChannels) const noexcept
{
        if (codecMode == CodecMappingMode::None)
            return 0;

        if (numOutputChannels >= 13)
            return 13;
        if (numOutputChannels >= 10)
            return 10;
        if (numOutputChannels >= 8)
            return 8;
        if (numOutputChannels >= NUM_SPEAKERS)
            return NUM_SPEAKERS;

        return juce::jmax (0, numOutputChannels);
    }


SpatialRenderer::CodecMappingMode SpatialRenderer::determineCodecModeForProfile (SpatialRenderer::SpatialOutputProfile requestedSpatialProfile) const noexcept
{
        if (requestedSpatialProfile == SpatialOutputProfile::CodecADM)
            return CodecMappingMode::ADM;
        if (requestedSpatialProfile == SpatialOutputProfile::CodecIAMF)
            return CodecMappingMode::IAMF;
        return CodecMappingMode::None;
    }


int SpatialRenderer::determineCodecObjectCount (SpatialRenderer::CodecMappingMode codecMode, int codecMappedChannelCount) const noexcept
{
        if (codecMode != CodecMappingMode::ADM)
            return 0;

        return juce::jmax (0, juce::jmin (NUM_SPEAKERS, codecMappedChannelCount));
    }


int SpatialRenderer::determineCodecElementCount (SpatialRenderer::CodecMappingMode codecMode, int codecMappedChannelCount) const noexcept
{
        if (codecMode != CodecMappingMode::IAMF)
            return 0;

        return juce::jmax (0, juce::jmin (2, codecMappedChannelCount));
    }


bool SpatialRenderer::isCodecMappingFiniteForBlock (bool codecMappingAppliedThisBlock, int numSamples) const noexcept
{
        if (! codecMappingAppliedThisBlock || numSamples <= 0)
            return true;

        const int channelsToInspect = juce::jmin (NUM_SPEAKERS, accumBuffer.getNumChannels());
        for (int channel = 0; channel < channelsToInspect; ++channel)
        {
            const auto sample = accumBuffer.getSample (channel, 0);
            if (! std::isfinite (sample))
                return false;
        }

        return true;
    }


std::uint64_t SpatialRenderer::buildCodecMappingSignature (std::uint64_t codecFrameId,
    std::uint64_t contractTimestampSamples,
    int codecMappedChannelCount,
    int codecObjectCount,
    int codecElementCount) const noexcept
{
        return (codecFrameId * 1315423911ull)
               ^ (contractTimestampSamples * 2654435761ull)
               ^ (static_cast<std::uint64_t> (juce::jmax (0, codecMappedChannelCount)) << 32u)
               ^ (static_cast<std::uint64_t> (juce::jmax (0, codecObjectCount)) << 16u)
               ^ static_cast<std::uint64_t> (juce::jmax (0, codecElementCount));
    }


void SpatialRenderer::publishCodecMappingContractState (std::uint64_t contractTimestampSamples,
    SpatialRenderer::CodecMappingMode codecMode,
    int codecMappedChannelCount,
    int codecObjectCount,
    int codecElementCount,
    bool codecMappingAppliedThisBlock,
    bool codecMappingFallbackActiveThisBlock,
    bool codecMappingFiniteThisBlock,
    std::uint64_t codecSignature)
{
        codecMappingTimestampSamples.store (contractTimestampSamples, std::memory_order_relaxed);
        codecMappingModeIndex.store (static_cast<int> (codecMode), std::memory_order_relaxed);
        codecMappingMappedChannelCount.store (codecMappedChannelCount, std::memory_order_relaxed);
        codecMappingObjectCount.store (codecObjectCount, std::memory_order_relaxed);
        codecMappingElementCount.store (codecElementCount, std::memory_order_relaxed);
        codecMappingApplied.store (codecMappingAppliedThisBlock, std::memory_order_relaxed);
        this->codecMappingFallbackActive.store (codecMappingFallbackActiveThisBlock, std::memory_order_relaxed);
        codecMappingFinite.store (codecMappingFiniteThisBlock, std::memory_order_relaxed);
        codecMappingSignature.store (codecSignature, std::memory_order_relaxed);
    }


std::uint64_t SpatialRenderer::publishAmbisonicIrContractState (SpatialRenderer::SpatialOutputProfile requestedSpatialProfile,
    SpatialRenderer::SpatialOutputProfile activeSpatialProfile,
    bool profileAllowsHeadphoneRender,
    int numSamples)
{
        const int requestedAmbisonicOrder = ambisonicOrderForProfile (requestedSpatialProfile);
        const int activeAmbisonicOrder = ambisonicOrderForProfile (activeSpatialProfile);
        const int contractAmbisonicOrder = requestedAmbisonicOrder > 0 ? requestedAmbisonicOrder
                                                                        : activeAmbisonicOrder;
        const int contractChannelCount = contractAmbisonicOrder > 0
                                             ? (contractAmbisonicOrder + 1) * (contractAmbisonicOrder + 1)
                                             : 0;
        const bool contractFallbackActive = requestedAmbisonicOrder > 0 && activeAmbisonicOrder == 0;
        const auto contractTimestampSamples = ambisonicIrSampleCursor.fetch_add (
            static_cast<std::uint64_t> (juce::jmax (0, numSamples)),
            std::memory_order_relaxed);

        ambisonicIrFrameId.fetch_add (1, std::memory_order_relaxed);
        ambisonicIrTimestampSamples.store (contractTimestampSamples, std::memory_order_relaxed);
        ambisonicIrOrder.store (contractAmbisonicOrder, std::memory_order_relaxed);
        ambisonicIrNormalizationIndex.store (
            static_cast<int> (AmbisonicNormalization::SN3D),
            std::memory_order_relaxed);
        ambisonicIrChannelCount.store (contractChannelCount, std::memory_order_relaxed);
        ambisonicIrHeadphoneRenderAllowed.store (profileAllowsHeadphoneRender, std::memory_order_relaxed);
        ambisonicIrFallbackActive.store (contractFallbackActive, std::memory_order_relaxed);

        return contractTimestampSamples;
    }


void SpatialRenderer::publishAmbisonicAndCodecTelemetryContracts (int numSamples,
    int numOutputChannels,
    SpatialRenderer::SpatialOutputProfile activeSpatialProfile,
    SpatialRenderer::SpatialProfileStage activeSpatialStage,
    bool profileAllowsHeadphoneRender)
{
        const auto requestedSpatialProfileIndexValue = juce::jlimit (
            0,
            11,
            requestedSpatialProfileIndex.load (std::memory_order_relaxed));
        const auto requestedSpatialProfile = static_cast<SpatialOutputProfile> (requestedSpatialProfileIndexValue);
        const auto contractTimestampSamples = publishAmbisonicIrContractState (
            requestedSpatialProfile,
            activeSpatialProfile,
            profileAllowsHeadphoneRender,
            numSamples);

        const auto codecMode = determineCodecModeForProfile (requestedSpatialProfile);
        const int codecMappedChannelCount = determineCodecMappedChannelCount (codecMode, numOutputChannels);

        const int codecObjectCount = determineCodecObjectCount (codecMode, codecMappedChannelCount);
        const int codecElementCount = determineCodecElementCount (codecMode, codecMappedChannelCount);
        const bool codecMappingAppliedThisBlock =
            codecMode != CodecMappingMode::None && codecMappedChannelCount > 0;
        const bool codecMappingFallbackActive =
            codecMode != CodecMappingMode::None
            && activeSpatialStage != SpatialProfileStage::CodecLayoutPlaceholder;
        const bool codecMappingFiniteThisBlock =
            isCodecMappingFiniteForBlock (codecMappingAppliedThisBlock, numSamples);

        const auto codecFrameId = codecMappingFrameId.fetch_add (1, std::memory_order_relaxed) + 1u;
        const auto codecSignature = buildCodecMappingSignature (
            codecFrameId,
            contractTimestampSamples,
            codecMappedChannelCount,
            codecObjectCount,
            codecElementCount);
        publishCodecMappingContractState (
            contractTimestampSamples,
            codecMode,
            codecMappedChannelCount,
            codecObjectCount,
            codecElementCount,
            codecMappingAppliedThisBlock,
            codecMappingFallbackActive,
            codecMappingFiniteThisBlock,
            codecSignature);

        const bool codecAdmPayloadActiveThisBlock =
            codecMode == CodecMappingMode::ADM
            && codecMappingAppliedThisBlock
            && codecMappingFiniteThisBlock;
        publishCodecAdmPayloadContract (
            codecAdmPayloadActiveThisBlock,
            codecFrameId,
            contractTimestampSamples,
            codecMappedChannelCount,
            codecObjectCount);

        const bool codecIamfPayloadActiveThisBlock =
            codecMode == CodecMappingMode::IAMF
            && codecMappingAppliedThisBlock
            && codecMappingFiniteThisBlock;
        publishCodecIamfPayloadContract (
            codecIamfPayloadActiveThisBlock,
            codecFrameId,
            contractTimestampSamples,
            codecMappedChannelCount,
            codecElementCount);
    }


bool SpatialRenderer::writeDiscreteOrAmbisonicOutputSample (juce::AudioBuffer<float>& outputBuffer,
    int sampleIndex,
    int numOutputChannels,
    SpatialRenderer::SpatialOutputProfile activeSpatialProfile,
    float masterGain) const noexcept
{
        if (numOutputChannels >= 13
            && (activeSpatialProfile == SpatialOutputProfile::Surround742
                || activeSpatialProfile == SpatialOutputProfile::AtmosBed))
        {
            writeSurround742Sample (outputBuffer, sampleIndex, masterGain);
            return true;
        }

        if (numOutputChannels >= 10 && activeSpatialProfile == SpatialOutputProfile::Surround721)
        {
            writeSurround721Sample (outputBuffer, sampleIndex, masterGain);
            return true;
        }

        if (numOutputChannels >= 8 && activeSpatialProfile == SpatialOutputProfile::Surround521)
        {
            writeSurround521Sample (outputBuffer, sampleIndex, masterGain);
            return true;
        }

        if (numOutputChannels >= 4
            && (activeSpatialProfile == SpatialOutputProfile::AmbisonicFOA
                || activeSpatialProfile == SpatialOutputProfile::AmbisonicHOA))
        {
            const float fl = accumBuffer.getSample (0, sampleIndex);
            const float fr = accumBuffer.getSample (1, sampleIndex);
            const float rr = accumBuffer.getSample (2, sampleIndex);
            const float rl = accumBuffer.getSample (3, sampleIndex);
            float w = 0.0f;
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            encodeAmbisonicFoaProxyFromQuad (fl, fr, rr, rl, w, x, y, z);
            outputBuffer.setSample (0, sampleIndex, w * masterGain);
            outputBuffer.setSample (1, sampleIndex, x * masterGain);
            outputBuffer.setSample (2, sampleIndex, y * masterGain);
            outputBuffer.setSample (3, sampleIndex, z * masterGain);
            for (int ch = 4; ch < numOutputChannels; ++ch)
                outputBuffer.setSample (ch, sampleIndex, 0.0f);
            return true;
        }

        if (numOutputChannels >= NUM_SPEAKERS)
        {
            for (int outCh = 0; outCh < NUM_SPEAKERS; ++outCh)
            {
                const int speakerIdx = kQuadOutputSpeakerOrder[static_cast<size_t> (outCh)];
                outputBuffer.setSample (
                    outCh,
                    sampleIndex,
                    accumBuffer.getSample (speakerIdx, sampleIndex) * masterGain);
            }

            for (int outCh = NUM_SPEAKERS; outCh < numOutputChannels; ++outCh)
                outputBuffer.setSample (outCh, sampleIndex, 0.0f);
            return true;
        }

        return false;
    }


SpatialRenderer::StereoOutputSample SpatialRenderer::renderStereoOutputSample (int sampleIndex,
    SpatialRenderer::SpatialOutputProfile activeSpatialProfile,
    bool steamRenderedThisBlock,
    SpatialRenderer::HeadphoneRenderMode activeHeadphoneMode) const noexcept
{
        StereoOutputSample sample {};

        if (steamRenderedThisBlock && activeHeadphoneMode == HeadphoneRenderMode::SteamBinaural)
        {
            sample.left = steamBinauralLeft[static_cast<size_t> (sampleIndex)];
            sample.right = steamBinauralRight[static_cast<size_t> (sampleIndex)];
            renderStereoDownmixSample (sampleIndex, sample.referenceLeft, sample.referenceRight);
            sample.referenceCaptured = true;
            return sample;
        }

        if (activeSpatialProfile == SpatialOutputProfile::Virtual3dStereo)
        {
            renderVirtual3dStereoSample (sampleIndex, sample.left, sample.right);
            return sample;
        }

        if (activeSpatialProfile == SpatialOutputProfile::AmbisonicFOA
            || activeSpatialProfile == SpatialOutputProfile::AmbisonicHOA)
        {
            float fl = 0.0f;
            float fr = 0.0f;
            float rr = 0.0f;
            float rl = 0.0f;
            getHeadPoseAdjustedQuadSample (sampleIndex, fl, fr, rr, rl);
            float w = 0.0f;
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            encodeAmbisonicFoaProxyFromQuad (fl, fr, rr, rl, w, x, y, z);
            decodeAmbisonicFoaProxyToStereo (w, x, y, z, sample.left, sample.right);
            return sample;
        }

        renderStereoDownmixSample (sampleIndex, sample.left, sample.right);
        return sample;
    }


void SpatialRenderer::writeMonoOutputSample (juce::AudioBuffer<float>& outputBuffer,
    int sampleIndex,
    float masterGain) const noexcept
{
        float mono = 0.0f;
        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
            mono += accumBuffer.getSample (spk, sampleIndex);
        outputBuffer.setSample (0, sampleIndex, mono * 0.5f * masterGain);
    }


void SpatialRenderer::applyRequestedHeadphoneCalibrationSettings()
{
        headphoneCalibrationChain.setEnabled (
            requestedHeadphoneCalibrationEnabled.load (std::memory_order_relaxed));
        headphoneCalibrationChain.setRequestedEngineIndex (
            requestedHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed));
    }


void SpatialRenderer::publishHeadphoneCalibrationRuntimeState (bool includeRequestedEngineIndex)
{
        if (includeRequestedEngineIndex)
        {
            requestedHeadphoneCalibrationEngineIndex.store (
                headphoneCalibrationChain.getRequestedEngineIndex(),
                std::memory_order_relaxed);
        }

        activeHeadphoneCalibrationEngineIndex.store (
            headphoneCalibrationChain.getActiveEngineIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationFallbackReasonIndex.store (
            headphoneCalibrationChain.getFallbackReasonIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationLatencySamples.store (
            headphoneCalibrationChain.getActiveLatencySamples(),
            std::memory_order_relaxed);
    }


SpatialRenderer::HeadphoneRuntimeState SpatialRenderer::configureHeadphoneRuntime (int numSamples,
    int numOutputChannels,
    SpatialRenderer::SpatialOutputProfile activeSpatialProfile)
{
        HeadphoneRuntimeState state {};
        state.requestedMode = static_cast<HeadphoneRenderMode> (
            requestedHeadphoneModeIndex.load (std::memory_order_relaxed));
        const auto requestedHeadphoneProfile = static_cast<HeadphoneDeviceProfile> (
            juce::jlimit (
                0,
                NUM_HEADPHONE_DEVICE_PROFILES - 1,
                requestedHeadphoneProfileIndex.load (std::memory_order_relaxed)));
        state.steamBackendAvailable = isSteamAudioBackendAvailable();
        state.profileAllowsHeadphoneRender = isStereoOrBinauralProfile (activeSpatialProfile)
                                             || numOutputChannels <= 2;
        headPoseInternalBinauralActive = state.profileAllowsHeadphoneRender
                                         && numOutputChannels >= 2
                                         && numOutputChannels < NUM_SPEAKERS;
        state.activeMode = (state.requestedMode == HeadphoneRenderMode::SteamBinaural
                            && state.profileAllowsHeadphoneRender
                            && numOutputChannels >= 2
                            && state.steamBackendAvailable)
                               ? HeadphoneRenderMode::SteamBinaural
                               : HeadphoneRenderMode::StereoDownmix;

        const auto activeHeadphoneProfile = (numOutputChannels >= 2)
                                                ? requestedHeadphoneProfile
                                                : HeadphoneDeviceProfile::Generic;
        const auto activeHeadphoneProfileIndexValue = static_cast<int> (activeHeadphoneProfile);
        if (lastAppliedHeadphoneProfileIndex != activeHeadphoneProfileIndexValue)
        {
            updateHeadphoneCompensationForProfile (activeHeadphoneProfile);
            lastAppliedHeadphoneProfileIndex = activeHeadphoneProfileIndexValue;
        }

        applyRequestedHeadphoneCalibrationSettings();
        publishHeadphoneCalibrationRuntimeState (true);

        state.steamRenderedThisBlock = (state.profileAllowsHeadphoneRender
                                        && numOutputChannels >= 2
                                        && state.activeMode == HeadphoneRenderMode::SteamBinaural
                                        && renderSteamBinauralBlock (numSamples));
        if (state.activeMode == HeadphoneRenderMode::SteamBinaural && ! state.steamRenderedThisBlock)
            state.activeMode = HeadphoneRenderMode::StereoDownmix;

        activeHeadphoneModeIndex.store (static_cast<int> (state.activeMode), std::memory_order_relaxed);
        activeHeadphoneProfileIndex.store (activeHeadphoneProfileIndexValue, std::memory_order_relaxed);
        steamAudioAvailable.store (state.steamBackendAvailable, std::memory_order_relaxed);
        return state;
    }


SpatialRenderer::OutputRoutingStageContext SpatialRenderer::prepareOutputRoutingStageContext (int numSamples, int numOutputChannels)
{
        const auto profileResolution = resolveSpatialProfileForHost (numOutputChannels);
        const auto activeSpatialProfile = profileResolution.profile;
        activeSpatialProfileIndex.store (static_cast<int> (activeSpatialProfile), std::memory_order_relaxed);
        activeSpatialStageIndex.store (static_cast<int> (profileResolution.stage), std::memory_order_relaxed);

        return {
            profileResolution,
            activeSpatialProfile,
            configureHeadphoneRuntime (numSamples, numOutputChannels, activeSpatialProfile)
        };
    }


void SpatialRenderer::writeStereoOutputSample (juce::AudioBuffer<float>& outputBuffer,
    int sampleIndex,
    float masterGain,
    SpatialRenderer::SpatialOutputProfile activeSpatialProfile,
    const SpatialRenderer::HeadphoneRuntimeState& headphoneState,
    bool renderedAuditionEmitter,
    SpatialRenderer::AuditionHeadphoneParityAccumulator& headphoneParity)
{
        auto stereo = renderStereoOutputSample (
            sampleIndex,
            activeSpatialProfile,
            headphoneState.steamRenderedThisBlock,
            headphoneState.activeMode);

        if (renderedAuditionEmitter)
        {
            accumulateAuditionHeadphoneParitySample (
                headphoneParity,
                stereo.left,
                stereo.right,
                stereo.referenceCaptured,
                stereo.referenceLeft,
                stereo.referenceRight);
        }

        applyHeadphoneProfileCompensation (stereo.left, stereo.right);
        headphoneCalibrationChain.processStereoSample (stereo.left, stereo.right);
        outputBuffer.setSample (0, sampleIndex, stereo.left * masterGain);
        outputBuffer.setSample (1, sampleIndex, stereo.right * masterGain);
    }


void SpatialRenderer::writeOutputSampleForChannelLayout (juce::AudioBuffer<float>& outputBuffer,
    int sampleIndex,
    int numOutputChannels,
    float masterGain,
    const SpatialRenderer::OutputRoutingStageContext& outputContext,
    bool renderedAuditionEmitter,
    SpatialRenderer::AuditionHeadphoneParityAccumulator& headphoneParity)
{
        if (writeDiscreteOrAmbisonicOutputSample (
                outputBuffer,
                sampleIndex,
                numOutputChannels,
                outputContext.activeSpatialProfile,
                masterGain))
        {
            return;
        }

        if (numOutputChannels >= 2)
        {
            writeStereoOutputSample (
                outputBuffer,
                sampleIndex,
                masterGain,
                outputContext.activeSpatialProfile,
                outputContext.headphoneState,
                renderedAuditionEmitter,
                headphoneParity);
            return;
        }

        if (numOutputChannels == 1)
            writeMonoOutputSample (outputBuffer, sampleIndex, masterGain);
    }


void SpatialRenderer::runOutputRoutingAndHeadphoneStage (juce::AudioBuffer<float>& outputBuffer,
    int numSamples,
    int numOutputChannels,
    bool renderedAuditionEmitter)
{
        const auto outputContext = prepareOutputRoutingStageContext (numSamples, numOutputChannels);
        publishAmbisonicAndCodecTelemetryContracts (
            numSamples,
            numOutputChannels,
            outputContext.activeSpatialProfile,
            outputContext.profileResolution.stage,
            outputContext.headphoneState.profileAllowsHeadphoneRender);

        auto headphoneParity = prepareAuditionHeadphoneParityAccumulator (
            renderedAuditionEmitter,
            numOutputChannels,
            outputContext.headphoneState);

        for (int i = 0; i < numSamples; ++i)
        {
            const float masterGain = smoothedMasterGain.getNextValue();
            writeOutputSampleForChannelLayout (
                outputBuffer,
                i,
                numOutputChannels,
                masterGain,
                outputContext,
                renderedAuditionEmitter,
                headphoneParity);
        }

        publishAuditionHeadphoneParityForBlock (renderedAuditionEmitter, numSamples, headphoneParity);
    }


juce::String SpatialRenderer::getBundledPeqPresetFilenameForProfile (SpatialRenderer::HeadphoneDeviceProfile profile)
{
        switch (profile)
        {
            case HeadphoneDeviceProfile::AirPodsPro2:   return "airpods_pro_2_anc_on.yaml";
            case HeadphoneDeviceProfile::AirPodsPro3:   return "airpods_pro_3_anc_on.yaml";
            case HeadphoneDeviceProfile::SonyWH1000XM5: return "sony_wh1000xm5_anc_on.yaml";
            case HeadphoneDeviceProfile::Generic:
            case HeadphoneDeviceProfile::CustomSOFA:
            default: break;
        }

        return {};
    }


juce::File SpatialRenderer::resolveBundledPeqPresetFile (const juce::String& presetFilename) const
{
        if (presetFilename.isEmpty())
            return {};

        // NOTE: path traversal assumes macOS AU/VST3 bundle layout (Contents/MacOS/ + Contents/Resources/).
        // On Windows/Linux this resolves empty and cache entries remain invalid.
#if JUCE_MAC
        return juce::File::getSpecialLocation (juce::File::currentExecutableFile)
            .getParentDirectory()
            .getSiblingFile ("Resources")
            .getChildFile ("eq_presets")
            .getChildFile (presetFilename);
#else
        juce::ignoreUnused (presetFilename);
        return {};
#endif
    }


void SpatialRenderer::preloadBundledPeqPresets()
{
        for (int profileIndex = 0; profileIndex < NUM_HEADPHONE_DEVICE_PROFILES; ++profileIndex)
        {
            auto& cacheEntry = bundledPeqPresets[static_cast<size_t> (profileIndex)];
            cacheEntry = BundledPeqPresetCacheEntry {};

            const auto profile = static_cast<HeadphoneDeviceProfile> (profileIndex);
            const auto presetFilename = getBundledPeqPresetFilenameForProfile (profile);
            if (presetFilename.isEmpty())
                continue;

            const auto presetFile = resolveBundledPeqPresetFile (presetFilename);
            if (! presetFile.existsAsFile())
                continue;

            cacheEntry.preset = locusq::headphone_dsp::loadHeadphonePreset (presetFile);
        }
    }


bool SpatialRenderer::tryBuildListenerOrientationFromCoordinateSpace (const IPLCoordinateSpace3& coordinateSpace,
                                                            SpatialRenderer::ListenerOrientation& orientation) noexcept
{
        orientation.right = { coordinateSpace.right.x, coordinateSpace.right.y, coordinateSpace.right.z };
        orientation.up = { coordinateSpace.up.x, coordinateSpace.up.y, coordinateSpace.up.z };
        orientation.ahead = { coordinateSpace.ahead.x, coordinateSpace.ahead.y, coordinateSpace.ahead.z };

        if (! locusq::spatial_headphone_pose::normalizeVector3 (orientation.right)
            || ! locusq::spatial_headphone_pose::normalizeVector3 (orientation.up)
            || ! locusq::spatial_headphone_pose::normalizeVector3 (orientation.ahead))
        {
            orientation = ListenerOrientation {};
            return false;
        }

        return true;
    }


void SpatialRenderer::setHeadPoseIdentityMix() noexcept
{
        locusq::spatial_headphone_pose::setHeadPoseIdentityMix (headPoseSpeakerMix);
    }


void SpatialRenderer::resetHeadPoseState() noexcept
{
        headPoseSnapshot = PoseSnapshot {};
        headPoseOrientation = ListenerOrientation {};
        headPoseValid = false;
        headPoseInternalBinauralActive = false;
        setHeadPoseIdentityMix();
    }


void SpatialRenderer::updateHeadPoseOrientationFromSnapshot() noexcept
{
        locusq::spatial_headphone_pose::updateOrientationFromQuaternion (
            headPoseSnapshot.qx,
            headPoseSnapshot.qy,
            headPoseSnapshot.qz,
            headPoseSnapshot.qw,
            headPoseOrientation);
    }


void SpatialRenderer::rebuildHeadPoseSpeakerMix() noexcept
{
        if (! headPoseValid)
        {
            setHeadPoseIdentityMix();
            return;
        }
        locusq::spatial_headphone_pose::buildSpeakerMixFromOrientation (
            headPoseOrientation,
            headPoseSpeakerMix);
    }


void SpatialRenderer::getHeadPoseAdjustedQuadSample (int sampleIndex, float& fl, float& fr, float& rr, float& rl) const noexcept
{
        if (! headPoseInternalBinauralActive || ! headPoseValid)
        {
            fl = accumBuffer.getSample (0, sampleIndex);
            fr = accumBuffer.getSample (1, sampleIndex);
            rr = accumBuffer.getSample (2, sampleIndex);
            rl = accumBuffer.getSample (3, sampleIndex);
            return;
        }

        const float sourceFl = accumBuffer.getSample (0, sampleIndex);
        const float sourceFr = accumBuffer.getSample (1, sampleIndex);
        const float sourceRr = accumBuffer.getSample (2, sampleIndex);
        const float sourceRl = accumBuffer.getSample (3, sampleIndex);
        locusq::spatial_headphone_pose::mixHeadPoseAdjustedQuadSample (
            headPoseSpeakerMix,
            sourceFl,
            sourceFr,
            sourceRr,
            sourceRl,
            fl,
            fr,
            rr,
            rl);
    }


bool SpatialRenderer::isStereoOrBinauralProfile (SpatialRenderer::SpatialOutputProfile profile) noexcept
{
        return locusq::spatial_profile_router::isStereoOrBinauralProfile (profile);
    }


SpatialRenderer::SpatialProfileResolution SpatialRenderer::resolveSpatialProfileForHost (int numOutputChannels) const noexcept
{
        const auto requested = static_cast<SpatialOutputProfile> (
            juce::jlimit (0, 11, requestedSpatialProfileIndex.load (std::memory_order_relaxed)));

        return locusq::spatial_profile_router::resolveSpatialProfileForHost (
            requested,
            numOutputChannels,
            NUM_SPEAKERS);
    }


int SpatialRenderer::ambisonicOrderForProfile (SpatialRenderer::SpatialOutputProfile profile) noexcept
{
        return locusq::spatial_profile_router::ambisonicOrderForProfile (profile);
    }


void SpatialRenderer::encodeAmbisonicFoaProxyFromQuad (float fl, float fr, float rr, float rl,
                                             float& w, float& x, float& y, float& z) noexcept
{
        locusq::spatial_profile_router::encodeAmbisonicFoaProxyFromQuad (fl, fr, rr, rl, w, x, y, z);
    }


void SpatialRenderer::decodeAmbisonicFoaProxyToStereo (float w, float x, float y, float z,
                                             float& left, float& right) noexcept
{
        locusq::spatial_profile_router::decodeAmbisonicFoaProxyToStereo (w, x, y, z, left, right);
    }


void SpatialRenderer::renderVirtual3dStereoSample (int sampleIndex, float& left, float& right) const noexcept
{
        float fl = 0.0f;
        float fr = 0.0f;
        float rr = 0.0f;
        float rl = 0.0f;
        getHeadPoseAdjustedQuadSample (sampleIndex, fl, fr, rr, rl);
        locusq::spatial_headphone_pose::renderVirtual3dStereoFromQuad (
            fl,
            fr,
            rr,
            rl,
            left,
            right);
    }


void SpatialRenderer::writeSurround521Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
{
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);

        locusq::spatial_profile_router::writeSurround521Sample (
            outputBuffer, sampleIndex, masterGain, fl, fr, rr, rl);
    }


void SpatialRenderer::writeSurround721Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
{
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);

        locusq::spatial_profile_router::writeSurround721Sample (
            outputBuffer, sampleIndex, masterGain, fl, fr, rr, rl);
    }


void SpatialRenderer::writeSurround742Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
{
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);

        locusq::spatial_profile_router::writeSurround742Sample (
            outputBuffer, sampleIndex, masterGain, fl, fr, rr, rl);
    }


void SpatialRenderer::renderStereoDownmixSample (int sampleIndex, float& left, float& right) const noexcept
{
        float fl = 0.0f;
        float fr = 0.0f;
        float rr = 0.0f;
        float rl = 0.0f;
        getHeadPoseAdjustedQuadSample (sampleIndex, fl, fr, rr, rl);
        locusq::spatial_headphone_pose::renderStereoDownmixFromQuad (
            fl,
            fr,
            rr,
            rl,
            left,
            right);
    }


void SpatialRenderer::resetHeadphoneCompensationState() noexcept
{
        locusq::spatial_headphone_pose::resetHeadphoneCompensationState (
            headphoneCompLowStateLeft,
            headphoneCompLowStateRight);
    }


void SpatialRenderer::updateHeadphoneCompensationForProfile (SpatialRenderer::HeadphoneDeviceProfile profile) noexcept
{
        const auto config = locusq::spatial_headphone_pose::makeHeadphoneCompensationConfig (
            static_cast<int> (profile),
            currentSampleRate);
        headphoneCompLowAlpha = config.lowAlpha;
        headphoneCompLowGain = config.lowGain;
        headphoneCompHighGain = config.highGain;
        headphoneCompCrossfeed = config.crossfeed;
    }


void SpatialRenderer::applyHeadphoneProfileCompensation (float& left, float& right) noexcept
{
        const locusq::spatial_headphone_pose::HeadphoneCompensationConfig config
        {
            headphoneCompLowAlpha,
            headphoneCompLowGain,
            headphoneCompHighGain,
            headphoneCompCrossfeed
        };

        locusq::spatial_headphone_pose::applyHeadphoneCompensation (
            left,
            right,
            config,
            headphoneCompLowStateLeft,
            headphoneCompLowStateRight);
    }


float SpatialRenderer::calculateDistance (const Vec3& pos)
{
        return std::sqrt (pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    }


float SpatialRenderer::calculateAzimuth (const Vec3& pos)
{
        // Azimuth: angle in XZ plane from front (Z+), clockwise positive
        // atan2(x, z) gives angle from Z+ axis, positive clockwise when X+
        float az = std::atan2 (pos.x, pos.z) * (180.0f / 3.14159265358979323846f);
        return az;
    }


float SpatialRenderer::calculateElevation (const Vec3& pos)
{
        float hDist = std::sqrt (pos.x * pos.x + pos.z * pos.z);
        if (hDist < 0.001f && std::abs (pos.y) < 0.001f)
            return 0.0f;
        return std::atan2 (pos.y, hDist) * (180.0f / 3.14159265358979323846f);
    }
