#include "../SpatialRenderer.h"

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
        const bool codecMappingFallbackActiveThisBlock =
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
            codecMappingFallbackActiveThisBlock,
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
