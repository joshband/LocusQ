#include "../SpatialRenderer.h"

namespace
{
using PeqPreset = locusq::headphone_dsp::HeadphonePeqHook::Preset;
using PeqCoefficients = locusq::headphone_dsp::HeadphonePeqHook::Coefficients;

PeqPreset buildBundledPeqPreset (const locusq::headphone_dsp::HeadphonePreset& preset,
                                 double sampleRate)
{
        auto peqPreset = locusq::headphone_dsp::HeadphonePeqHook::makeIdentityPreset();
        if (sampleRate <= 0.0 || ! preset.valid || preset.bands.empty())
            return peqPreset;

        locusq::headphone_dsp::HeadphonePeqHook::setPresetPreampDb (peqPreset, preset.preampDb);

        const auto sr = static_cast<float> (sampleRate);
        const int maxStages = juce::jmin (
            static_cast<int> (preset.bands.size()),
            locusq::headphone_dsp::HeadphonePeqHook::kMaxStages);

        for (int i = 0; i < maxStages; ++i)
        {
            const auto& band = preset.bands[static_cast<size_t> (i)];
            PeqCoefficients coefficients;
            switch (band.type)
            {
                case locusq::headphone_dsp::PeqBandSpec::Type::LSC:
                    coefficients = locusq::headphone_dsp::HeadphonePeqHook::makeLowShelf (band.fcHz, band.gainDb, band.q, sr);
                    break;
                case locusq::headphone_dsp::PeqBandSpec::Type::HSC:
                    coefficients = locusq::headphone_dsp::HeadphonePeqHook::makeHighShelf (band.fcHz, band.gainDb, band.q, sr);
                    break;
                default:
                    coefficients = locusq::headphone_dsp::HeadphonePeqHook::makePeakEQ (band.fcHz, band.gainDb, band.q, sr);
                    break;
            }

            locusq::headphone_dsp::HeadphonePeqHook::setPresetStage (peqPreset, i, coefficients);
        }

        return peqPreset;
}

PeqPreset buildJsonPeqPreset (const juce::var& bandsArray, float preampDb, double sampleRate)
{
        auto peqPreset = locusq::headphone_dsp::HeadphonePeqHook::makeIdentityPreset();
        locusq::headphone_dsp::HeadphonePeqHook::setPresetPreampDb (peqPreset, preampDb);

        if (! bandsArray.isArray() || sampleRate <= 0.0)
            return peqPreset;

        const auto* bandArray = bandsArray.getArray();
        if (bandArray == nullptr)
            return peqPreset;

        const auto sr = static_cast<float> (sampleRate);
        const int maxStages = juce::jmin (
            bandArray->size(),
            locusq::headphone_dsp::HeadphonePeqHook::kMaxStages);

        for (int i = 0; i < maxStages; ++i)
        {
            auto* band = (*bandArray)[i].getDynamicObject();
            if (band == nullptr)
                continue;

            const auto typeStr = band->getProperty ("type").toString().trim().toUpperCase();
            const auto fcHz    = static_cast<float> (static_cast<double> (band->getProperty ("fc_hz")));
            const auto gainDb  = static_cast<float> (static_cast<double> (band->getProperty ("gain_db")));
            const auto q       = static_cast<float> (static_cast<double> (band->getProperty ("q")));

            PeqCoefficients coefficients;
            if (typeStr == "LSC")
                coefficients = locusq::headphone_dsp::HeadphonePeqHook::makeLowShelf (fcHz, gainDb, q, sr);
            else if (typeStr == "HSC")
                coefficients = locusq::headphone_dsp::HeadphonePeqHook::makeHighShelf (fcHz, gainDb, q, sr);
            else
                coefficients = locusq::headphone_dsp::HeadphonePeqHook::makePeakEQ (fcHz, gainDb, q, sr);

            locusq::headphone_dsp::HeadphonePeqHook::setPresetStage (peqPreset, i, coefficients);
        }

        return peqPreset;
}
} // namespace

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

        headphoneCalibrationChain.applyPeqPreset (buildBundledPeqPreset (preset, sampleRate));

        lastLoadedPeqPresetIndex = clampedProfileIndex;
        lastLoadedPeqSampleRate  = sampleRate;
    }


void SpatialRenderer::applyJsonPeqBands (const juce::var& bandsArray, float preampDb, double sampleRate)
{
        headphoneCalibrationChain.applyPeqPreset (buildJsonPeqPreset (bandsArray, preampDb, sampleRate));
    }


void SpatialRenderer::clearFirImpulseResponse() noexcept
{
        headphoneCalibrationChain.clearFirImpulseResponse();
    }


bool SpatialRenderer::loadFirImpulseResponse (const float* taps, int tapCount) noexcept
{
        return headphoneCalibrationChain.loadFirImpulseResponse (taps, tapCount);
    }


bool SpatialRenderer::loadFirTapsFromJson (const juce::var& tapsArray) noexcept
{
        headphoneCalibrationChain.clearFirImpulseResponse();

        if (! tapsArray.isArray())
            return false;

        const auto* taps = tapsArray.getArray();
        if (taps == nullptr || taps->isEmpty())
            return false;

        juce::Array<float> coefficients;
        coefficients.ensureStorageAllocated (taps->size());

        for (const auto& tapVar : *taps)
        {
            float tap = 0.0f;
            if (tapVar.isDouble() || tapVar.isInt() || tapVar.isInt64() || tapVar.isBool())
                tap = static_cast<float> (static_cast<double> (tapVar));
            else
                tap = tapVar.toString().getFloatValue();

            if (! std::isfinite (tap))
                return false;

            coefficients.add (tap);
        }

        if (coefficients.isEmpty())
            return false;

        return headphoneCalibrationChain.loadFirImpulseResponse (
            coefficients.getRawDataPointer(),
            coefficients.size());
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


void SpatialRenderer::clearHeadPose() noexcept
{
        resetHeadPoseState();
    }


void SpatialRenderer::setRequestedSofaHrtf (juce::String sofaRefRelativePath, bool enabled)
{
        requestedSofaRefRelativePath = std::move (sofaRefRelativePath);
        requestedSofaHrtfEnabled = enabled && requestedSofaRefRelativePath.isNotEmpty();
    }


bool SpatialRenderer::reloadSteamAudioRuntime() noexcept
{
        teardownSteamAudioRuntime();
        initialiseSteamAudioRuntimeIfEnabled();
        return steamAudioUsingSofaHrtf;
    }


bool SpatialRenderer::isUsingSofaHrtf() const noexcept
{
        return steamAudioUsingSofaHrtf;
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
