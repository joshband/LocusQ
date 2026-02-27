#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
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
#include "headphone_dsp/HeadphoneCalibrationChain.h"
#include "headphone_dsp/HeadphonePresetLoader.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>

#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
 #include <phonon.h>
#endif

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
    static constexpr int MAX_AUDITION_REACTIVE_SOURCES = 8;
    enum class HeadphoneRenderMode : int
    {
        StereoDownmix = 0,
        SteamBinaural = 1
    };
    enum class HeadphoneDeviceProfile : int
    {
        Generic       = 0,
        AirPodsPro2   = 1,
        AirPodsPro3   = 2,
        SonyWH1000XM5 = 3,
        CustomSOFA    = 4
    };
    enum class SpatialOutputProfile : int
    {
        Auto = 0,
        Stereo20 = 1,
        Quad40 = 2,
        Surround521 = 3,
        Surround721 = 4,
        Surround742 = 5,
        AmbisonicFOA = 6,
        AmbisonicHOA = 7,
        AtmosBed = 8,
        Virtual3dStereo = 9,
        CodecIAMF = 10,
        CodecADM = 11
    };
    enum class SpatialProfileStage : int
    {
        Direct = 0,
        FallbackStereo = 1,
        FallbackQuad = 2,
        AmbiDecodeStereo = 3,
        CodecLayoutPlaceholder = 4
    };
    enum class SteamInitStage : int
    {
        NotCompiled = 0,
        Uninitialized = 1,
        LoadingLibrary = 2,
        LibraryOpenFailed = 3,
        ResolvingSymbols = 4,
        SymbolsMissing = 5,
        CreatingContext = 6,
        ContextCreateFailed = 7,
        CreatingHRTF = 8,
        HRTFCreateFailed = 9,
        CreatingVirtualSurround = 10,
        VirtualSurroundCreateFailed = 11,
        Ready = 12
    };

    // Internal speaker order (VBAP / accumulation): FL, FR, RR, RL.
    static constexpr std::array<int, NUM_SPEAKERS> kQuadOutputSpeakerOrder
    {
        0, 1, 3, 2 // Host quad output order: FL, FR, RL, RR
    };

    struct alignas(16) PoseSnapshot
    {
        float qx = 0.0f;              // +0
        float qy = 0.0f;              // +4
        float qz = 0.0f;              // +8
        float qw = 1.0f;              // +12
        std::uint64_t timestampMs = 0; // +16
        std::uint32_t seq = 0;        // +24
        std::uint32_t pad = 0;        // +28
        float angVx = 0.0f;           // +32  rad/s body frame
        float angVy = 0.0f;           // +36
        float angVz = 0.0f;           // +40
        std::uint32_t sensorLocationFlags = 0; // +44
    };                                // = 48 bytes

    static_assert (sizeof (PoseSnapshot) == 48, "PoseSnapshot size contract");

    struct ListenerOrientation
    {
        std::array<float, 3> right { 1.0f, 0.0f, 0.0f };
        std::array<float, 3> up { 0.0f, 1.0f, 0.0f };
        std::array<float, 3> ahead { 0.0f, 0.0f, -1.0f };
    };

    struct AuditionReactiveSnapshot
    {
        float rms = 0.0f;
        float peak = 0.0f;
        float envFast = 0.0f;
        float envSlow = 0.0f;
        float onset = 0.0f;
        float brightness = 0.0f;
        float rainFadeRate = 0.0f;
        float snowFadeRate = 0.0f;
        float physicsVelocity = 0.0f;
        float physicsCollision = 0.0f;
        float physicsDensity = 0.0f;
        float physicsCoupling = 0.0f;
        float geometryScale = 0.0f;
        float geometryWidth = 0.0f;
        float geometryDepth = 0.0f;
        float geometryHeight = 0.0f;
        float precipitationFade = 0.0f;
        float collisionBurst = 0.0f;
        float densitySpread = 0.0f;
        float headphoneOutputRms = 0.0f;
        float headphoneOutputPeak = 0.0f;
        float headphoneParity = 1.0f;
        float rmsNorm = 0.0f;
        float peakNorm = 0.0f;
        float envFastNorm = 0.0f;
        float envSlowNorm = 0.0f;
        float headphoneOutputRmsNorm = 0.0f;
        float headphoneOutputPeakNorm = 0.0f;
        float headphoneParityNorm = 0.0f;
        int headphoneFallbackReasonIndex = 0;
        int sourceEnergyCount = 0;
        std::array<float, MAX_AUDITION_REACTIVE_SOURCES> sourceEnergy {};
    };

    enum class AuditionReactiveHeadphoneFallbackReason : int
    {
        None = 0,
        SteamUnavailable = 1,
        SteamRenderFailed = 2,
        OutputIncompatible = 3
    };

    SpatialRenderer()
    {
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        steamInitStageIndex.store (static_cast<int> (SteamInitStage::Uninitialized), std::memory_order_relaxed);
#else
        steamInitStageIndex.store (static_cast<int> (SteamInitStage::NotCompiled), std::memory_order_relaxed);
#endif
    }
    ~SpatialRenderer()
    {
        shutdown();
    }

    //==========================================================================
    void prepare (double sampleRate, int maxBlockSize)
    {
        shutdown();
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

        steamBinauralLeft.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        steamBinauralRight.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        for (auto& rotated : headPoseRotatedQuadScratch)
            rotated.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        resetHeadPoseState();
        resetHeadphoneCompensationState();
        for (auto& voiceGains : auditionSmoothedSpeakerGains)
            for (auto& gain : voiceGains)
                gain.setCurrentAndTargetValue (0.0f);
        std::fill (auditionHistoryBuffer.begin(), auditionHistoryBuffer.end(), 0.0f);
        auditionHistoryWritePos = 0;
        resetAuditionVoiceFieldStates();
        resetAuditionReactiveTelemetry();
        updateHeadphoneCompensationForProfile (HeadphoneDeviceProfile::Generic);
        headphoneCalibrationChain.prepare (sampleRate, maxBlockSize);
        headphoneCalibrationChain.setEnabled (requestedHeadphoneCalibrationEnabled.load (std::memory_order_relaxed));
        headphoneCalibrationChain.setRequestedEngineIndex (
            requestedHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed));
        requestedHeadphoneCalibrationEngineIndex.store (
            headphoneCalibrationChain.getRequestedEngineIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationEngineIndex.store (
            headphoneCalibrationChain.getActiveEngineIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationFallbackReasonIndex.store (
            headphoneCalibrationChain.getFallbackReasonIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationLatencySamples.store (
            headphoneCalibrationChain.getActiveLatencySamples(),
            std::memory_order_relaxed);
        initialiseSteamAudioRuntimeIfEnabled();
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
        resetHeadPoseState();
        resetHeadphoneCompensationState();
        headphoneCalibrationChain.reset();
        activeHeadphoneCalibrationEngineIndex.store (
            headphoneCalibrationChain.getActiveEngineIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationFallbackReasonIndex.store (
            headphoneCalibrationChain.getFallbackReasonIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationLatencySamples.store (
            headphoneCalibrationChain.getActiveLatencySamples(),
            std::memory_order_relaxed);
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

    void shutdown() noexcept
    {
        teardownSteamAudioRuntime();
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        setSteamInitStage (SteamInitStage::Uninitialized, 0);
#else
        setSteamInitStage (SteamInitStage::NotCompiled, 0);
#endif
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

    void setHeadphoneRenderMode (int modeIndex)
    {
        const auto clamped = juce::jlimit (0, 1, modeIndex);
        if (requestedHeadphoneModeIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedHeadphoneModeIndex.store (clamped, std::memory_order_relaxed);
    }

    void setHeadphoneDeviceProfile (int profileIndex)
    {
        const auto clamped = juce::jlimit (0, 4, profileIndex);
        if (requestedHeadphoneProfileIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedHeadphoneProfileIndex.store (clamped, std::memory_order_relaxed);
    }

    void loadPeqPresetForProfile (int profileIndex, double sampleRate)
    {
        if (lastLoadedPeqPresetIndex == profileIndex && lastLoadedPeqSampleRate == sampleRate)
            return;

        const auto profile = static_cast<HeadphoneDeviceProfile> (
            juce::jlimit (0, 4, profileIndex));

        juce::String presetFilename;
        switch (profile)
        {
            case HeadphoneDeviceProfile::AirPodsPro2:   presetFilename = "airpods_pro_2_anc_on.yaml"; break;
            case HeadphoneDeviceProfile::AirPodsPro3:   presetFilename = "airpods_pro_3_anc_on.yaml"; break;
            case HeadphoneDeviceProfile::SonyWH1000XM5: presetFilename = "sony_wh1000xm5_anc_on.yaml"; break;
            default: break;
        }

        if (presetFilename.isEmpty() || sampleRate <= 0.0)
        {
            headphoneCalibrationChain.clearPeqPreset();
            lastLoadedPeqPresetIndex = profileIndex;
            lastLoadedPeqSampleRate  = sampleRate;
            return;
        }

        // Resolve preset from plugin bundle.
        // NOTE: path traversal assumes macOS AU/VST3 bundle layout (Contents/MacOS/ + Contents/Resources/).
        // On Windows/Linux presetsDir will not resolve and loadHeadphonePreset will return invalid.
#if JUCE_MAC
        const auto presetsDir = juce::File::getSpecialLocation (
            juce::File::currentExecutableFile)
            .getParentDirectory()
            .getSiblingFile ("Resources")
            .getChildFile ("eq_presets");
#else
        const juce::File presetsDir {};
#endif

        const auto preset = locusq::headphone_dsp::loadHeadphonePreset (
            presetsDir.getChildFile (presetFilename));

        headphoneCalibrationChain.clearPeqPreset();

        // NOTE: clearPeqPreset -> setPeqPreampDb -> setPeqStage writes are not atomic with respect
        // to the audio thread. A brief glitch may occur during a profile switch while audio is
        // processing. This is acceptable: profile changes are non-RT events on the message thread.
        if (! preset.valid || preset.bands.empty())
            return;  // Do not cache — allow retry if file was temporarily unavailable.

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

        lastLoadedPeqPresetIndex = profileIndex;
        lastLoadedPeqSampleRate  = sampleRate;
    }

    // Apply PEQ bands from a JSON-parsed var array (companion IPC path).
    // preampDb = 0.0 if the JSON schema has no preamp field.
    // Called on message thread; not RT-safe (see loadPeqPresetForProfile note).
    void applyJsonPeqBands (const juce::var& bandsArray, float preampDb, double sampleRate)
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

    void setHeadphoneCalibrationEnabled (bool enabled) noexcept
    {
        if (requestedHeadphoneCalibrationEnabled.load (std::memory_order_relaxed) == enabled)
            return;

        requestedHeadphoneCalibrationEnabled.store (enabled, std::memory_order_relaxed);
    }

    void setHeadphoneCalibrationEngine (int engineIndex) noexcept
    {
        if (requestedHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed) == engineIndex)
            return;

        requestedHeadphoneCalibrationEngineIndex.store (engineIndex, std::memory_order_relaxed);
    }

    int getCalibrationLatencySamples() const noexcept
    {
        return headphoneCalibrationChain.getActiveLatencySamples();
    }

    void setSpatialOutputProfile (int profileIndex)
    {
        const auto clamped = juce::jlimit (0, 11, profileIndex);
        if (requestedSpatialProfileIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedSpatialProfileIndex.store (clamped, std::memory_order_relaxed);
    }

    void applyHeadPose (const PoseSnapshot& pose) noexcept
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

    void setAuditionEnabled (bool enabled) noexcept
    {
        auditionEnabled = enabled;
    }

    void setAuditionSignalType (int signalTypeIndex) noexcept
    {
        const auto clamped = juce::jlimit (0, 12, signalTypeIndex);
        if (auditionSignalTypeIndex == clamped)
            return;

        auditionSignalTypeIndex = clamped;
        resetAuditionVoiceFieldStates();
    }

    void setAuditionMotionType (int motionTypeIndex) noexcept
    {
        auditionMotionTypeIndex = juce::jlimit (0, 5, motionTypeIndex);
    }

    void setAuditionLevelPreset (int levelPresetIndex) noexcept
    {
        auditionLevelPresetIndex = juce::jlimit (0, 4, levelPresetIndex);
    }

    void setAuditionPhysicsReactiveInput (
        bool active,
        float velocityNorm,
        float collisionNorm,
        float densityNorm) noexcept
    {
        auditionPhysicsReactiveInputActive = active;
        auditionPhysicsReactiveVelocityTarget = sanitizeUnitScalar (velocityNorm);
        auditionPhysicsReactiveCollisionTarget = sanitizeUnitScalar (collisionNorm);
        auditionPhysicsReactiveDensityTarget = sanitizeUnitScalar (densityNorm);
    }

    int getHeadphoneRenderModeRequestedIndex() const noexcept
    {
        return requestedHeadphoneModeIndex.load (std::memory_order_relaxed);
    }

    int getHeadphoneRenderModeActiveIndex() const noexcept
    {
        return activeHeadphoneModeIndex.load (std::memory_order_relaxed);
    }

    int getHeadphoneDeviceProfileRequestedIndex() const noexcept
    {
        return requestedHeadphoneProfileIndex.load (std::memory_order_relaxed);
    }

    int getHeadphoneDeviceProfileActiveIndex() const noexcept
    {
        return activeHeadphoneProfileIndex.load (std::memory_order_relaxed);
    }

    bool isHeadphoneCalibrationEnabledRequested() const noexcept
    {
        return requestedHeadphoneCalibrationEnabled.load (std::memory_order_relaxed);
    }

    int getHeadphoneCalibrationEngineRequestedIndex() const noexcept
    {
        return locusq::headphone_core::sanitizeCalibrationEngineIndex (
            requestedHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed));
    }

    int getHeadphoneCalibrationEngineActiveIndex() const noexcept
    {
        return locusq::headphone_core::sanitizeCalibrationEngineIndex (
            activeHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed));
    }

    int getHeadphoneCalibrationFallbackReasonIndex() const noexcept
    {
        return locusq::headphone_core::sanitizeCalibrationFallbackReasonIndex (
            activeHeadphoneCalibrationFallbackReasonIndex.load (std::memory_order_relaxed));
    }

    int getHeadphoneCalibrationLatencySamples() const noexcept
    {
        return locusq::headphone_core::sanitizeCalibrationLatencySamples (
            activeHeadphoneCalibrationLatencySamples.load (std::memory_order_relaxed));
    }

    int getSpatialOutputProfileRequestedIndex() const noexcept
    {
        return requestedSpatialProfileIndex.load (std::memory_order_relaxed);
    }

    int getSpatialOutputProfileActiveIndex() const noexcept
    {
        return activeSpatialProfileIndex.load (std::memory_order_relaxed);
    }

    int getSpatialProfileStageIndex() const noexcept
    {
        return activeSpatialStageIndex.load (std::memory_order_relaxed);
    }

    bool isSteamAudioAvailable() const noexcept
    {
        return steamAudioAvailable.load (std::memory_order_relaxed);
    }

    bool isSteamAudioCompiled() const noexcept
    {
        return isSteamAudioBackendCompiled();
    }

    int getSteamAudioInitStageIndex() const noexcept
    {
        return steamInitStageIndex.load (std::memory_order_relaxed);
    }

    int getSteamAudioInitErrorCode() const noexcept
    {
        return steamInitErrorCode.load (std::memory_order_relaxed);
    }

    juce::String getSteamAudioRuntimeLibraryPath() const
    {
        const juce::SpinLock::ScopedLockType diagnosticsLock (steamDiagnosticsLock);
        return steamRuntimeLibraryPath;
    }

    juce::String getSteamAudioMissingSymbolName() const
    {
        const juce::SpinLock::ScopedLockType diagnosticsLock (steamDiagnosticsLock);
        return steamMissingSymbolName;
    }

    static const char* headphoneRenderModeToString (int modeIndex) noexcept
    {
        switch (juce::jlimit (0, 1, modeIndex))
        {
            case static_cast<int> (HeadphoneRenderMode::SteamBinaural): return "steam_binaural";
            case static_cast<int> (HeadphoneRenderMode::StereoDownmix):
            default: break;
        }

        return "stereo_downmix";
    }

    static const char* steamAudioInitStageToString (int stageIndex) noexcept
    {
        switch (static_cast<SteamInitStage> (stageIndex))
        {
            case SteamInitStage::NotCompiled: return "not_compiled";
            case SteamInitStage::Uninitialized: return "uninitialized";
            case SteamInitStage::LoadingLibrary: return "loading_library";
            case SteamInitStage::LibraryOpenFailed: return "library_open_failed";
            case SteamInitStage::ResolvingSymbols: return "resolving_symbols";
            case SteamInitStage::SymbolsMissing: return "symbols_missing";
            case SteamInitStage::CreatingContext: return "creating_context";
            case SteamInitStage::ContextCreateFailed: return "context_create_failed";
            case SteamInitStage::CreatingHRTF: return "creating_hrtf";
            case SteamInitStage::HRTFCreateFailed: return "hrtf_create_failed";
            case SteamInitStage::CreatingVirtualSurround: return "creating_virtual_surround";
            case SteamInitStage::VirtualSurroundCreateFailed: return "virtual_surround_create_failed";
            case SteamInitStage::Ready: return "ready";
            default: break;
        }

        return "unknown";
    }

    static const char* auditionReactiveHeadphoneFallbackReasonToString (int reasonIndex) noexcept
    {
        switch (static_cast<AuditionReactiveHeadphoneFallbackReason> (reasonIndex))
        {
            case AuditionReactiveHeadphoneFallbackReason::None: return "none";
            case AuditionReactiveHeadphoneFallbackReason::SteamUnavailable: return "steam_unavailable";
            case AuditionReactiveHeadphoneFallbackReason::SteamRenderFailed: return "steam_render_failed";
            case AuditionReactiveHeadphoneFallbackReason::OutputIncompatible: return "output_incompatible";
            default: break;
        }

        return "unknown";
    }

    static const char* headphoneDeviceProfileToString (int profileIndex) noexcept
    {
        switch (juce::jlimit (0, 4, profileIndex))
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

    static const char* headphoneCalibrationEngineToString (int engineIndex) noexcept
    {
        return locusq::headphone_core::calibrationChainEngineToString (engineIndex);
    }

    static const char* headphoneCalibrationFallbackReasonToString (int reasonIndex) noexcept
    {
        return locusq::headphone_core::calibrationChainFallbackReasonToString (reasonIndex);
    }

    static const char* spatialOutputProfileToString (int profileIndex) noexcept
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

    static const char* spatialProfileStageToString (int stageIndex) noexcept
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
            const float* emitterAudio = scene.getSlot (slotIdx).getAudioMono();
            const int emitterSamples = scene.getSlot (slotIdx).getAudioNumSamples();

            if (emitterAudio == nullptr || emitterSamples <= 0)
                continue;

            const int samplesToProcess = std::min (emitterSamples, numSamples);

            // Apply emitter gain to pre-downmixed mono audio.
            float blockPeak = 0.0f;
            for (int i = 0; i < samplesToProcess; ++i)
            {
                const float sample = emitterAudio[i] * candidate.emitterGainLinear;
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

        bool renderedAuditionEmitter = false;
        if (processedEmitterCount == 0 && auditionEnabled)
        {
            renderInternalAuditionEmitter (numSamples);
            eligibleEmitterCount = juce::jmax (eligibleEmitterCount, 1);
            processedEmitterCount = juce::jmax (processedEmitterCount, 1);
            renderedAuditionEmitter = true;
        }
        else
        {
            resetAuditionReactiveTelemetry();
        }
        auditionVisualActive.store (renderedAuditionEmitter, std::memory_order_relaxed);

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

        const auto profileResolution = resolveSpatialProfileForHost (numOutputChannels);
        const auto activeSpatialProfile = profileResolution.profile;
        activeSpatialProfileIndex.store (static_cast<int> (activeSpatialProfile), std::memory_order_relaxed);
        activeSpatialStageIndex.store (static_cast<int> (profileResolution.stage), std::memory_order_relaxed);

        const auto requestedHeadphoneMode = static_cast<HeadphoneRenderMode> (
            requestedHeadphoneModeIndex.load (std::memory_order_relaxed));
        const auto requestedHeadphoneProfile = static_cast<HeadphoneDeviceProfile> (
            juce::jlimit (0, 4, requestedHeadphoneProfileIndex.load (std::memory_order_relaxed)));
        const auto steamBackendAvailable = isSteamAudioBackendAvailable();
        const bool profileAllowsHeadphoneRender = isStereoOrBinauralProfile (activeSpatialProfile)
                                                  || numOutputChannels <= 2;
        headPoseInternalBinauralActive = profileAllowsHeadphoneRender
                                         && numOutputChannels >= 2
                                         && numOutputChannels < NUM_SPEAKERS;
        auto activeHeadphoneMode = (requestedHeadphoneMode == HeadphoneRenderMode::SteamBinaural
                                    && profileAllowsHeadphoneRender
                                    && numOutputChannels >= 2
                                    && steamBackendAvailable)
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

        headphoneCalibrationChain.setEnabled (
            requestedHeadphoneCalibrationEnabled.load (std::memory_order_relaxed));
        headphoneCalibrationChain.setRequestedEngineIndex (
            requestedHeadphoneCalibrationEngineIndex.load (std::memory_order_relaxed));
        requestedHeadphoneCalibrationEngineIndex.store (
            headphoneCalibrationChain.getRequestedEngineIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationEngineIndex.store (
            headphoneCalibrationChain.getActiveEngineIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationFallbackReasonIndex.store (
            headphoneCalibrationChain.getFallbackReasonIndex(),
            std::memory_order_relaxed);
        activeHeadphoneCalibrationLatencySamples.store (
            headphoneCalibrationChain.getActiveLatencySamples(),
            std::memory_order_relaxed);

        const bool steamRenderedThisBlock = (profileAllowsHeadphoneRender
                                             && numOutputChannels >= 2
                                             && activeHeadphoneMode == HeadphoneRenderMode::SteamBinaural
                                             && renderSteamBinauralBlock (numSamples));

        if (activeHeadphoneMode == HeadphoneRenderMode::SteamBinaural && ! steamRenderedThisBlock)
            activeHeadphoneMode = HeadphoneRenderMode::StereoDownmix;

        activeHeadphoneModeIndex.store (static_cast<int> (activeHeadphoneMode), std::memory_order_relaxed);
        activeHeadphoneProfileIndex.store (activeHeadphoneProfileIndexValue, std::memory_order_relaxed);
        steamAudioAvailable.store (steamBackendAvailable, std::memory_order_relaxed);

        double auditionReactiveHeadphoneEnergy = 0.0;
        double auditionReactiveHeadphoneReferenceEnergy = 0.0;
        float auditionReactiveHeadphonePeak = 0.0f;
        bool auditionReactiveHeadphoneSamplesCaptured = false;
        int auditionReactiveHeadphoneFallbackReasonIndex = static_cast<int> (
            AuditionReactiveHeadphoneFallbackReason::None);
        if (renderedAuditionEmitter && requestedHeadphoneMode == HeadphoneRenderMode::SteamBinaural)
        {
            if (numOutputChannels < 2 || ! profileAllowsHeadphoneRender)
            {
                auditionReactiveHeadphoneFallbackReasonIndex = static_cast<int> (
                    AuditionReactiveHeadphoneFallbackReason::OutputIncompatible);
            }
            else if (! steamBackendAvailable)
            {
                auditionReactiveHeadphoneFallbackReasonIndex = static_cast<int> (
                    AuditionReactiveHeadphoneFallbackReason::SteamUnavailable);
            }
            else if (! steamRenderedThisBlock || activeHeadphoneMode != HeadphoneRenderMode::SteamBinaural)
            {
                auditionReactiveHeadphoneFallbackReasonIndex = static_cast<int> (
                    AuditionReactiveHeadphoneFallbackReason::SteamRenderFailed);
            }
        }

        // Apply master gain and write to output
        for (int i = 0; i < numSamples; ++i)
        {
            const float masterGain = smoothedMasterGain.getNextValue();

            if (numOutputChannels >= 13
                && (activeSpatialProfile == SpatialOutputProfile::Surround742
                    || activeSpatialProfile == SpatialOutputProfile::AtmosBed))
            {
                writeSurround742Sample (outputBuffer, i, masterGain);
                continue;
            }

            if (numOutputChannels >= 10 && activeSpatialProfile == SpatialOutputProfile::Surround721)
            {
                writeSurround721Sample (outputBuffer, i, masterGain);
                continue;
            }

            if (numOutputChannels >= 8 && activeSpatialProfile == SpatialOutputProfile::Surround521)
            {
                writeSurround521Sample (outputBuffer, i, masterGain);
                continue;
            }

            if (numOutputChannels >= 4
                && (activeSpatialProfile == SpatialOutputProfile::AmbisonicFOA
                    || activeSpatialProfile == SpatialOutputProfile::AmbisonicHOA))
            {
                const float fl = accumBuffer.getSample (0, i);
                const float fr = accumBuffer.getSample (1, i);
                const float rr = accumBuffer.getSample (2, i);
                const float rl = accumBuffer.getSample (3, i);
                float w = 0.0f;
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                encodeAmbisonicFoaProxyFromQuad (fl, fr, rr, rl, w, x, y, z);
                outputBuffer.setSample (0, i, w * masterGain);
                outputBuffer.setSample (1, i, x * masterGain);
                outputBuffer.setSample (2, i, y * masterGain);
                outputBuffer.setSample (3, i, z * masterGain);
                for (int ch = 4; ch < numOutputChannels; ++ch)
                    outputBuffer.setSample (ch, i, 0.0f);
                continue;
            }

            if (numOutputChannels >= NUM_SPEAKERS)
            {
                // Quad output: explicit host order FL, FR, RL, RR.
                for (int outCh = 0; outCh < NUM_SPEAKERS; ++outCh)
                {
                    const int speakerIdx = kQuadOutputSpeakerOrder[static_cast<size_t> (outCh)];
                    outputBuffer.setSample (outCh, i, accumBuffer.getSample (speakerIdx, i) * masterGain);
                }

                for (int outCh = NUM_SPEAKERS; outCh < numOutputChannels; ++outCh)
                    outputBuffer.setSample (outCh, i, 0.0f);
                continue;
            }

            if (numOutputChannels >= 2)
            {
                float left = 0.0f;
                float right = 0.0f;
                float referenceLeft = 0.0f;
                float referenceRight = 0.0f;
                bool referenceCaptured = false;

                if (steamRenderedThisBlock && activeHeadphoneMode == HeadphoneRenderMode::SteamBinaural)
                {
                    left = steamBinauralLeft[static_cast<size_t> (i)];
                    right = steamBinauralRight[static_cast<size_t> (i)];
                    renderStereoDownmixSample (i, referenceLeft, referenceRight);
                    referenceCaptured = true;
                }
                else if (activeSpatialProfile == SpatialOutputProfile::Virtual3dStereo)
                {
                    renderVirtual3dStereoSample (i, left, right);
                }
                else if (activeSpatialProfile == SpatialOutputProfile::AmbisonicFOA
                         || activeSpatialProfile == SpatialOutputProfile::AmbisonicHOA)
                {
                    float fl = 0.0f;
                    float fr = 0.0f;
                    float rr = 0.0f;
                    float rl = 0.0f;
                    getHeadPoseAdjustedQuadSample (i, fl, fr, rr, rl);
                    float w = 0.0f;
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 0.0f;
                    encodeAmbisonicFoaProxyFromQuad (fl, fr, rr, rl, w, x, y, z);
                    decodeAmbisonicFoaProxyToStereo (w, x, y, z, left, right);
                }
                else
                {
                    renderStereoDownmixSample (i, left, right);
                }

                if (renderedAuditionEmitter)
                {
                    const auto mono = 0.5f * (left + right);
                    auditionReactiveHeadphoneEnergy += static_cast<double> (mono * mono);
                    auditionReactiveHeadphonePeak = juce::jmax (
                        auditionReactiveHeadphonePeak,
                        juce::jmax (std::abs (left), std::abs (right)));

                    if (referenceCaptured)
                    {
                        const auto referenceMono = 0.5f * (referenceLeft + referenceRight);
                        auditionReactiveHeadphoneReferenceEnergy += static_cast<double> (referenceMono * referenceMono);
                    }
                    else
                    {
                        auditionReactiveHeadphoneReferenceEnergy += static_cast<double> (mono * mono);
                    }

                    auditionReactiveHeadphoneSamplesCaptured = true;
                }

                applyHeadphoneProfileCompensation (left, right);
                headphoneCalibrationChain.processStereoSample (left, right);
                outputBuffer.setSample (0, i, left * masterGain);
                outputBuffer.setSample (1, i, right * masterGain);
                continue;
            }

            if (numOutputChannels == 1)
            {
                float mono = 0.0f;
                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                    mono += accumBuffer.getSample (spk, i);
                outputBuffer.setSample (0, i, mono * 0.5f * masterGain);
            }
        }

        if (renderedAuditionEmitter && auditionReactiveHeadphoneSamplesCaptured && numSamples > 0)
        {
            const auto invNumSamples = 1.0f / static_cast<float> (numSamples);
            const auto headphoneOutputRms = juce::jlimit (
                0.0f,
                2.0f,
                std::sqrt (static_cast<float> (auditionReactiveHeadphoneEnergy * static_cast<double> (invNumSamples))));
            const auto headphoneReferenceRms = juce::jlimit (
                0.0f,
                2.0f,
                std::sqrt (static_cast<float> (auditionReactiveHeadphoneReferenceEnergy * static_cast<double> (invNumSamples))));
            const auto headphoneParity = headphoneOutputRms > 1.0e-6f
                ? juce::jlimit (0.5f, 2.0f, headphoneReferenceRms / headphoneOutputRms)
                : 1.0f;

            applyAuditionReactiveHeadphoneParity (
                headphoneOutputRms,
                auditionReactiveHeadphonePeak,
                headphoneParity,
                auditionReactiveHeadphoneFallbackReasonIndex);
        }
        else if (renderedAuditionEmitter)
        {
            applyAuditionReactiveHeadphoneParity (
                0.0f,
                0.0f,
                1.0f,
                auditionReactiveHeadphoneFallbackReasonIndex);
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

    bool isAuditionVisualActive() const noexcept
    {
        return auditionVisualActive.load (std::memory_order_relaxed);
    }

    float getAuditionVisualX() const noexcept
    {
        return auditionVisualX.load (std::memory_order_relaxed);
    }

    float getAuditionVisualY() const noexcept
    {
        return auditionVisualY.load (std::memory_order_relaxed);
    }

    float getAuditionVisualZ() const noexcept
    {
        return auditionVisualZ.load (std::memory_order_relaxed);
    }

    AuditionReactiveSnapshot getAuditionReactiveSnapshot() const noexcept
    {
        AuditionReactiveSnapshot snapshot;
        snapshot.rms = sanitizeUnitScalar (auditionReactiveRms.load (std::memory_order_relaxed));
        snapshot.peak = sanitizeUnitScalar (auditionReactivePeak.load (std::memory_order_relaxed));
        snapshot.envFast = sanitizeUnitScalar (auditionReactiveEnvFast.load (std::memory_order_relaxed));
        snapshot.envSlow = sanitizeUnitScalar (auditionReactiveEnvSlow.load (std::memory_order_relaxed));
        snapshot.onset = sanitizeUnitScalar (auditionReactiveOnset.load (std::memory_order_relaxed));
        snapshot.brightness = sanitizeUnitScalar (auditionReactiveBrightness.load (std::memory_order_relaxed));
        snapshot.rainFadeRate = sanitizeUnitScalar (auditionReactiveRainFadeRate.load (std::memory_order_relaxed));
        snapshot.snowFadeRate = sanitizeUnitScalar (auditionReactiveSnowFadeRate.load (std::memory_order_relaxed));
        snapshot.physicsVelocity = sanitizeUnitScalar (auditionReactivePhysicsVelocity.load (std::memory_order_relaxed));
        snapshot.physicsCollision = sanitizeUnitScalar (auditionReactivePhysicsCollision.load (std::memory_order_relaxed));
        snapshot.physicsDensity = sanitizeUnitScalar (auditionReactivePhysicsDensity.load (std::memory_order_relaxed));
        snapshot.physicsCoupling = sanitizeUnitScalar (auditionReactivePhysicsCoupling.load (std::memory_order_relaxed));
        snapshot.headphoneOutputRms = sanitizeUnitScalar (auditionReactiveHeadphoneOutputRms.load (std::memory_order_relaxed));
        snapshot.headphoneOutputPeak = sanitizeUnitScalar (auditionReactiveHeadphoneOutputPeak.load (std::memory_order_relaxed));
        snapshot.headphoneParity = sanitizeUnitScalar (auditionReactiveHeadphoneParity.load (std::memory_order_relaxed));
        snapshot.rmsNorm = snapshot.rms;
        snapshot.peakNorm = snapshot.peak;
        snapshot.envFastNorm = snapshot.envFast;
        snapshot.envSlowNorm = snapshot.envSlow;
        snapshot.headphoneOutputRmsNorm = snapshot.headphoneOutputRms;
        snapshot.headphoneOutputPeakNorm = snapshot.headphoneOutputPeak;
        snapshot.headphoneParityNorm = snapshot.headphoneParity;
        snapshot.headphoneFallbackReasonIndex = sanitizeHeadphoneFallbackReasonIndex (
            auditionReactiveHeadphoneFallbackReasonIndex.load (std::memory_order_relaxed));
        snapshot.sourceEnergyCount = sanitizeSourceCount (
            auditionReactiveSourceCount.load (std::memory_order_relaxed));

        for (int i = 0; i < MAX_AUDITION_REACTIVE_SOURCES; ++i)
        {
            snapshot.sourceEnergy[static_cast<size_t> (i)] = sanitizeUnitScalar (
                auditionReactiveSourceEnergy[static_cast<size_t> (i)].load (std::memory_order_relaxed));
        }

        const auto sourceDensity = sanitizeUnitScalar (
            static_cast<float> (snapshot.sourceEnergyCount)
                / static_cast<float> (juce::jmax (1, MAX_AUDITION_REACTIVE_SOURCES)));
        snapshot.geometryScale = sanitizeUnitScalar (
            0.30f * snapshot.envFast
                + 0.20f * snapshot.envSlow
                + 0.20f * snapshot.physicsCoupling
                + 0.20f * snapshot.headphoneParity
                + 0.10f * sourceDensity);
        snapshot.geometryWidth = sanitizeUnitScalar (
            0.40f * snapshot.physicsDensity
                + 0.25f * snapshot.physicsVelocity
                + 0.20f * snapshot.brightness
                + 0.15f * sourceDensity);
        snapshot.geometryDepth = sanitizeUnitScalar (
            0.35f * snapshot.envSlow
                + 0.30f * (1.0f - snapshot.brightness)
                + 0.20f * snapshot.physicsCoupling
                + 0.15f * sourceDensity);
        snapshot.geometryHeight = sanitizeUnitScalar (
            0.45f * snapshot.onset
                + 0.30f * snapshot.physicsCollision
                + 0.15f * snapshot.envFast
                + 0.10f * snapshot.headphoneOutputPeak);
        snapshot.precipitationFade = sanitizeUnitScalar (
            0.55f * snapshot.rainFadeRate
                + 0.45f * snapshot.snowFadeRate);
        snapshot.collisionBurst = sanitizeUnitScalar (
            snapshot.physicsCollision * (0.55f + 0.45f * snapshot.onset));
        snapshot.densitySpread = sanitizeUnitScalar (
            0.60f * snapshot.physicsDensity
                + 0.25f * sourceDensity
                + 0.15f * snapshot.physicsVelocity);

        return snapshot;
    }

private:
    static float sanitizeUnitScalar (float value, float fallback = 0.0f) noexcept
    {
        if (! std::isfinite (value))
            return juce::jlimit (0.0f, 1.0f, fallback);
        return juce::jlimit (0.0f, 1.0f, value);
    }

    static int sanitizeSourceCount (int value) noexcept
    {
        return juce::jlimit (0, MAX_AUDITION_REACTIVE_SOURCES, value);
    }

    static int sanitizeHeadphoneFallbackReasonIndex (int value) noexcept
    {
        return juce::jlimit (
            0,
            static_cast<int> (AuditionReactiveHeadphoneFallbackReason::OutputIncompatible),
            value);
    }

    static constexpr int MAX_TRACKED_EMITTERS = 64; // Per-emitter smoothing/filtering
    static constexpr int MAX_RENDER_EMITTERS_PER_BLOCK = 8; // v1-tested CPU envelope
    static constexpr float COARSE_PRIORITY_GATE_LINEAR = 1.0e-5f; // ~ -100 dB
    static constexpr float ACTIVITY_PEAK_GATE_LINEAR = 1.0e-6f;   // ~ -120 dB
    static constexpr int AUDITION_MAX_VOICES = MAX_AUDITION_REACTIVE_SOURCES;
    static constexpr int AUDITION_HISTORY_BUFFER_SAMPLES = 8192;

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
    std::array<std::array<juce::SmoothedValue<float>, NUM_SPEAKERS>, AUDITION_MAX_VOICES> auditionSmoothedSpeakerGains;

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
    bool auditionEnabled = false;
    int auditionSignalTypeIndex = 0;
    int auditionMotionTypeIndex = 0;
    int auditionLevelPresetIndex = 2;
    double auditionPhasePrimary = 0.0;
    double auditionPhaseSecondary = 0.0;
    double auditionOrbitPhase = 0.0;
    uint32_t auditionNoiseState = 0x13579BDFu;
    float auditionNoiseOnePole = 0.0f;
    float auditionRainBed = 0.0f;
    float auditionRainDropEnv = 0.0f;
    float auditionRainDropFreqHz = 1300.0f;
    double auditionRainDropPhase = 0.0;
    float auditionSnowBed = 0.0f;
    float auditionSnowShimmer = 0.0f;
    double auditionSnowFlutterPhase = 0.0;
    float auditionBounceEnv = 0.0f;
    float auditionBounceFreqHz = 320.0f;
    double auditionBouncePhase = 0.0;
    int auditionBounceClusterRemaining = 0;
    int auditionBounceCountdownSamples = 0;
    int auditionBounceCooldownSamples = 0;
    float auditionBounceSpacingSamples = 0.0f;
    float auditionChimeEnv = 0.0f;
    float auditionChimeFreqA = 659.25f;
    float auditionChimeFreqB = 987.77f;
    double auditionChimePhaseA = 0.0;
    double auditionChimePhaseB = 0.0;
    float auditionChimeShimmer = 0.0f;
    int auditionChimeCooldownSamples = 0;
    float auditionCricketEnv = 0.0f;
    float auditionCricketFreqHz = 4200.0f;
    double auditionCricketPhase = 0.0;
    int auditionCricketBurstSamples = 0;
    int auditionCricketCooldownSamples = 0;
    float auditionBirdEnv = 0.0f;
    float auditionBirdFreqA = 1400.0f;
    float auditionBirdFreqB = 2200.0f;
    double auditionBirdPhaseA = 0.0;
    double auditionBirdPhaseB = 0.0;
    double auditionBirdWarblePhase = 0.0;
    int auditionBirdPhraseSamples = 0;
    int auditionBirdCooldownSamples = 0;
    static constexpr int kAuditionKarplusMaxDelaySamples = 4096;
    std::array<float, kAuditionKarplusMaxDelaySamples> auditionKarplusDelayLine {};
    int auditionKarplusDelaySamples = 620;
    int auditionKarplusWriteIndex = 0;
    float auditionKarplusDamping = 0.985f;
    float auditionKarplusEnv = 0.0f;
    int auditionKarplusCooldownSamples = 0;
    float auditionMembraneEnv = 0.0f;
    float auditionMembraneFreqA = 180.0f;
    float auditionMembraneFreqB = 280.0f;
    double auditionMembranePhaseA = 0.0;
    double auditionMembranePhaseB = 0.0;
    int auditionMembraneCooldownSamples = 0;
    float auditionKrellEnv = 0.0f;
    float auditionKrellFreqCurrent = 220.0f;
    float auditionKrellFreqTarget = 220.0f;
    double auditionKrellPhase = 0.0;
    int auditionKrellStepSamples = 0;
    float auditionArpEnv = 0.0f;
    float auditionArpFreqA = 330.0f;
    float auditionArpFreqB = 495.0f;
    double auditionArpPhaseA = 0.0;
    double auditionArpPhaseB = 0.0;
    int auditionArpGateSamples = 0;
    int auditionArpStepIndex = 0;
    float auditionWallPosX = 0.0f;
    float auditionWallPosZ = -1.0f;
    float auditionWallVelX = 0.92f;
    float auditionWallVelZ = 0.71f;
    std::array<float, AUDITION_HISTORY_BUFFER_SAMPLES> auditionHistoryBuffer {};
    int auditionHistoryWritePos = 0;
    std::array<double, AUDITION_MAX_VOICES> auditionVoiceModPhase {};
    std::array<double, AUDITION_MAX_VOICES> auditionVoiceExciterPhaseA {};
    std::array<double, AUDITION_MAX_VOICES> auditionVoiceExciterPhaseB {};
    std::array<float, AUDITION_MAX_VOICES> auditionVoiceExciterEnv {};
    std::array<int, AUDITION_MAX_VOICES> auditionVoiceExciterCooldownSamples {};
    std::array<std::uint32_t, AUDITION_MAX_VOICES> auditionVoiceNoiseState {};
    std::atomic<bool> auditionVisualActive { false };
    std::atomic<float> auditionVisualX { 0.0f };
    std::atomic<float> auditionVisualY { 1.2f };
    std::atomic<float> auditionVisualZ { -1.0f };
    float auditionReactiveEnvFastState = 0.0f;
    float auditionReactiveEnvSlowState = 0.0f;
    float auditionReactiveBrightnessLowpassState = 0.0f;
    bool auditionPhysicsReactiveInputActive = false;
    float auditionPhysicsReactiveVelocityTarget = 0.0f;
    float auditionPhysicsReactiveCollisionTarget = 0.0f;
    float auditionPhysicsReactiveDensityTarget = 0.0f;
    float auditionPhysicsReactiveVelocityState = 0.0f;
    float auditionPhysicsReactiveCollisionState = 0.0f;
    float auditionPhysicsReactiveDensityState = 0.0f;
    float auditionPhysicsReactiveTimbreLowpassState = 0.0f;
    std::atomic<float> auditionReactiveRms { 0.0f };
    std::atomic<float> auditionReactivePeak { 0.0f };
    std::atomic<float> auditionReactiveEnvFast { 0.0f };
    std::atomic<float> auditionReactiveEnvSlow { 0.0f };
    std::atomic<float> auditionReactiveOnset { 0.0f };
    std::atomic<float> auditionReactiveBrightness { 0.0f };
    std::atomic<float> auditionReactiveRainFadeRate { 0.0f };
    std::atomic<float> auditionReactiveSnowFadeRate { 0.0f };
    std::atomic<float> auditionReactivePhysicsVelocity { 0.0f };
    std::atomic<float> auditionReactivePhysicsCollision { 0.0f };
    std::atomic<float> auditionReactivePhysicsDensity { 0.0f };
    std::atomic<float> auditionReactivePhysicsCoupling { 0.0f };
    std::atomic<float> auditionReactiveHeadphoneOutputRms { 0.0f };
    std::atomic<float> auditionReactiveHeadphoneOutputPeak { 0.0f };
    std::atomic<float> auditionReactiveHeadphoneParity { 1.0f };
    std::atomic<int> auditionReactiveHeadphoneFallbackReasonIndex {
        static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None)
    };
    std::atomic<int> auditionReactiveSourceCount { 0 };
    std::array<std::atomic<float>, AUDITION_MAX_VOICES> auditionReactiveSourceEnergy {};
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
    std::atomic<int> requestedHeadphoneModeIndex { static_cast<int> (HeadphoneRenderMode::StereoDownmix) };
    std::atomic<int> activeHeadphoneModeIndex { static_cast<int> (HeadphoneRenderMode::StereoDownmix) };
    std::atomic<int> requestedHeadphoneProfileIndex { static_cast<int> (HeadphoneDeviceProfile::Generic) };
    std::atomic<int> activeHeadphoneProfileIndex { static_cast<int> (HeadphoneDeviceProfile::Generic) };
    std::atomic<bool> requestedHeadphoneCalibrationEnabled { false };
    std::atomic<int> requestedHeadphoneCalibrationEngineIndex {
        static_cast<int> (locusq::headphone_core::CalibrationChainEngine::Disabled)
    };
    std::atomic<int> activeHeadphoneCalibrationEngineIndex {
        static_cast<int> (locusq::headphone_core::CalibrationChainEngine::Disabled)
    };
    std::atomic<int> activeHeadphoneCalibrationFallbackReasonIndex {
        static_cast<int> (locusq::headphone_core::CalibrationChainFallbackReason::DisabledByRequest)
    };
    std::atomic<int> activeHeadphoneCalibrationLatencySamples { 0 };
    std::atomic<int> requestedSpatialProfileIndex { static_cast<int> (SpatialOutputProfile::Auto) };
    std::atomic<int> activeSpatialProfileIndex { static_cast<int> (SpatialOutputProfile::Auto) };
    std::atomic<int> activeSpatialStageIndex { static_cast<int> (SpatialProfileStage::Direct) };
    std::atomic<bool> steamAudioAvailable { false };
    std::atomic<int> steamInitStageIndex { static_cast<int> (SteamInitStage::NotCompiled) };
    std::atomic<int> steamInitErrorCode { 0 };
    mutable juce::SpinLock steamDiagnosticsLock;
    juce::String steamRuntimeLibraryPath;
    juce::String steamMissingSymbolName;

    // Steam Audio scratch/output buffers reused each block.
    std::vector<float> steamBinauralLeft;
    std::vector<float> steamBinauralRight;
    std::array<std::vector<float>, NUM_SPEAKERS> headPoseRotatedQuadScratch;
    std::array<std::array<float, NUM_SPEAKERS>, NUM_SPEAKERS> headPoseSpeakerMix {};
    PoseSnapshot headPoseSnapshot {};
    ListenerOrientation headPoseOrientation {};
    bool headPoseValid = false;
    bool headPoseInternalBinauralActive = false;
    float headphoneCompLowAlpha = 0.0f;
    float headphoneCompLowGain = 1.0f;
    float headphoneCompHighGain = 1.0f;
    float headphoneCompCrossfeed = 0.0f;
    float headphoneCompLowStateLeft = 0.0f;
    float headphoneCompLowStateRight = 0.0f;
    int    lastAppliedHeadphoneProfileIndex = -1;
    int    lastLoadedPeqPresetIndex         = -1;
    double lastLoadedPeqSampleRate          = 0.0;
    locusq::headphone_dsp::HeadphoneCalibrationChain headphoneCalibrationChain;

#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
    using IplContextCreateFn = IPLerror (IPLCALL*) (IPLContextSettings*, IPLContext*);
    using IplContextReleaseFn = void (IPLCALL*) (IPLContext*);
    using IplHRTFCreateFn = IPLerror (IPLCALL*) (IPLContext, IPLAudioSettings*, IPLHRTFSettings*, IPLHRTF*);
    using IplHRTFReleaseFn = void (IPLCALL*) (IPLHRTF*);
    using IplVirtualSurroundEffectCreateFn = IPLerror (IPLCALL*) (IPLContext, IPLAudioSettings*, IPLVirtualSurroundEffectSettings*, IPLVirtualSurroundEffect*);
    using IplVirtualSurroundEffectReleaseFn = void (IPLCALL*) (IPLVirtualSurroundEffect*);
    using IplVirtualSurroundEffectResetFn = void (IPLCALL*) (IPLVirtualSurroundEffect);
    using IplVirtualSurroundEffectApplyFn = IPLAudioEffectState (IPLCALL*) (IPLVirtualSurroundEffect, IPLVirtualSurroundEffectParams*, IPLAudioBuffer*, IPLAudioBuffer*);

    juce::DynamicLibrary steamAudioLibrary;
    IplContextCreateFn iplContextCreateFn = nullptr;
    IplContextReleaseFn iplContextReleaseFn = nullptr;
    IplHRTFCreateFn iplHRTFCreateFn = nullptr;
    IplHRTFReleaseFn iplHRTFReleaseFn = nullptr;
    IplVirtualSurroundEffectCreateFn iplVirtualSurroundEffectCreateFn = nullptr;
    IplVirtualSurroundEffectReleaseFn iplVirtualSurroundEffectReleaseFn = nullptr;
    IplVirtualSurroundEffectResetFn iplVirtualSurroundEffectResetFn = nullptr;
    IplVirtualSurroundEffectApplyFn iplVirtualSurroundEffectApplyFn = nullptr;

    IPLContext steamContext = nullptr;
    IPLHRTF steamHrtf = nullptr;
    IPLVirtualSurroundEffect steamVirtualSurroundEffect = nullptr;
    std::array<float*, NUM_SPEAKERS> steamInputChannelPtrs {};
    std::array<float*, 2> steamOutputChannelPtrs {};
#endif

    bool steamAudioRuntimeReady = false;

    // World speaker vectors use scene coordinates where +Z is front.
    static constexpr std::array<std::array<float, 3>, NUM_SPEAKERS> kQuadWorldSpeakerDirs
    {{
        { -0.70710678f, 0.0f,  0.70710678f }, // FL
        {  0.70710678f, 0.0f,  0.70710678f }, // FR
        {  0.70710678f, 0.0f, -0.70710678f }, // RR
        { -0.70710678f, 0.0f, -0.70710678f }  // RL
    }};

    // Listener-local speaker vectors follow Steam canonical axes where -Z is ahead.
    static constexpr std::array<std::array<float, 2>, NUM_SPEAKERS> kQuadListenerSpeakerDirsXZ
    {{
        { -0.70710678f, -0.70710678f }, // FL
        {  0.70710678f, -0.70710678f }, // FR
        {  0.70710678f,  0.70710678f }, // RR
        { -0.70710678f,  0.70710678f }  // RL
    }};

    static float dot3 (const std::array<float, 3>& lhs, const std::array<float, 3>& rhs) noexcept
    {
        return (lhs[0] * rhs[0]) + (lhs[1] * rhs[1]) + (lhs[2] * rhs[2]);
    }

    void setHeadPoseIdentityMix() noexcept
    {
        for (int dst = 0; dst < NUM_SPEAKERS; ++dst)
        {
            for (int src = 0; src < NUM_SPEAKERS; ++src)
                headPoseSpeakerMix[static_cast<size_t> (dst)][static_cast<size_t> (src)] = (dst == src) ? 1.0f : 0.0f;
        }
    }

    void resetHeadPoseState() noexcept
    {
        headPoseSnapshot = PoseSnapshot {};
        headPoseOrientation = ListenerOrientation {};
        headPoseValid = false;
        headPoseInternalBinauralActive = false;
        setHeadPoseIdentityMix();
    }

    // Quaternion follows Steam canonical axes (right +X, up +Y, ahead -Z).
    void updateHeadPoseOrientationFromSnapshot() noexcept
    {
        const float x = headPoseSnapshot.qx;
        const float y = headPoseSnapshot.qy;
        const float z = headPoseSnapshot.qz;
        const float w = headPoseSnapshot.qw;

        const float xx = x * x;
        const float yy = y * y;
        const float zz = z * z;
        const float xy = x * y;
        const float xz = x * z;
        const float yz = y * z;
        const float xw = x * w;
        const float yw = y * w;
        const float zw = z * w;

        const float m00 = 1.0f - 2.0f * (yy + zz);
        const float m10 = 2.0f * (xy + zw);
        const float m20 = 2.0f * (xz - yw);

        const float m01 = 2.0f * (xy - zw);
        const float m11 = 1.0f - 2.0f * (xx + zz);
        const float m21 = 2.0f * (yz + xw);

        const float m02 = 2.0f * (xz + yw);
        const float m12 = 2.0f * (yz - xw);
        const float m22 = 1.0f - 2.0f * (xx + yy);

        headPoseOrientation.right = { m00, m10, m20 };
        headPoseOrientation.up = { m01, m11, m21 };
        headPoseOrientation.ahead = { -m02, -m12, -m22 };
    }

    void rebuildHeadPoseSpeakerMix() noexcept
    {
        if (! headPoseValid)
        {
            setHeadPoseIdentityMix();
            return;
        }

        for (int sourceSpeaker = 0; sourceSpeaker < NUM_SPEAKERS; ++sourceSpeaker)
        {
            const auto& worldDir = kQuadWorldSpeakerDirs[static_cast<size_t> (sourceSpeaker)];
            const float relX = dot3 (worldDir, headPoseOrientation.right);
            const float relZ = dot3 (worldDir, headPoseOrientation.ahead);
            const float planarMag = std::sqrt ((relX * relX) + (relZ * relZ));

            float planarX = 0.0f;
            float planarZ = -1.0f;
            if (planarMag > 1.0e-6f && std::isfinite (planarMag))
            {
                const float invPlanar = 1.0f / planarMag;
                planarX = relX * invPlanar;
                planarZ = relZ * invPlanar;
            }

            float weightSum = 0.0f;
            float bestDot = -2.0f;
            int bestSpeaker = 0;
            for (int targetSpeaker = 0; targetSpeaker < NUM_SPEAKERS; ++targetSpeaker)
            {
                const auto& targetDir = kQuadListenerSpeakerDirsXZ[static_cast<size_t> (targetSpeaker)];
                const float projection = (planarX * targetDir[0]) + (planarZ * targetDir[1]);
                if (projection > bestDot)
                {
                    bestDot = projection;
                    bestSpeaker = targetSpeaker;
                }

                const float weight = juce::jmax (0.0f, projection);
                headPoseSpeakerMix[static_cast<size_t> (targetSpeaker)][static_cast<size_t> (sourceSpeaker)] = weight;
                weightSum += weight;
            }

            if (weightSum > 1.0e-6f && std::isfinite (weightSum))
            {
                const float invWeightSum = 1.0f / weightSum;
                for (int targetSpeaker = 0; targetSpeaker < NUM_SPEAKERS; ++targetSpeaker)
                {
                    headPoseSpeakerMix[static_cast<size_t> (targetSpeaker)][static_cast<size_t> (sourceSpeaker)] *= invWeightSum;
                }
            }
            else
            {
                for (int targetSpeaker = 0; targetSpeaker < NUM_SPEAKERS; ++targetSpeaker)
                {
                    headPoseSpeakerMix[static_cast<size_t> (targetSpeaker)][static_cast<size_t> (sourceSpeaker)] =
                        (targetSpeaker == bestSpeaker) ? 1.0f : 0.0f;
                }
            }
        }
    }

    inline void getHeadPoseAdjustedQuadSample (int sampleIndex, float& fl, float& fr, float& rr, float& rl) const noexcept
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

        const auto mixSpeaker = [this, sourceFl, sourceFr, sourceRr, sourceRl] (int targetSpeaker) noexcept
        {
            const auto& mix = headPoseSpeakerMix[static_cast<size_t> (targetSpeaker)];
            return (mix[0] * sourceFl)
                 + (mix[1] * sourceFr)
                 + (mix[2] * sourceRr)
                 + (mix[3] * sourceRl);
        };

        fl = mixSpeaker (0);
        fr = mixSpeaker (1);
        rr = mixSpeaker (2);
        rl = mixSpeaker (3);
    }

    //==========================================================================
    // Coordinate helpers
    //==========================================================================

    static float auditionLevelDbForPreset (int presetIndex) noexcept
    {
        switch (presetIndex)
        {
            case 0: return -36.0f;
            case 1: return -30.0f;
            case 2: return -24.0f;
            case 3: return -18.0f;
            case 4: return -12.0f;
            default: break;
        }

        return -24.0f;
    }

    float advanceAuditionOscillator (double frequencyHz, double& phase) const noexcept
    {
        const auto sampleRate = juce::jmax (1.0, currentSampleRate);
        const auto sample = std::sin (juce::MathConstants<double>::twoPi * phase);
        phase += frequencyHz / sampleRate;
        phase -= std::floor (phase);
        return static_cast<float> (sample);
    }

    float nextAuditionWhiteNoise() noexcept
    {
        auditionNoiseState = auditionNoiseState * 1664525u + 1013904223u;
        return static_cast<float> ((auditionNoiseState >> 8) & 0x00FFFFFFu) / 8388608.0f - 1.0f;
    }

    float nextAuditionRand01() noexcept
    {
        return 0.5f * (nextAuditionWhiteNoise() + 1.0f);
    }

    static float wrapAuditionAzimuthDegrees (float azimuthDeg) noexcept
    {
        while (azimuthDeg > 180.0f)
            azimuthDeg -= 360.0f;
        while (azimuthDeg < -180.0f)
            azimuthDeg += 360.0f;
        return azimuthDeg;
    }

    static float auditionVoiceHashUnit (int voiceIndex, std::uint32_t salt) noexcept
    {
        auto hash = static_cast<std::uint32_t> (voiceIndex + 1);
        hash ^= salt;
        hash ^= hash >> 16;
        hash *= 0x7FEB352Du;
        hash ^= hash >> 15;
        hash *= 0x846CA68Bu;
        hash ^= hash >> 16;
        return static_cast<float> (hash & 0x00FFFFFFu) / 16777215.0f;
    }

    void resetAuditionVoiceFieldStates() noexcept
    {
        std::fill (auditionVoiceModPhase.begin(), auditionVoiceModPhase.end(), 0.0);
        std::fill (auditionVoiceExciterPhaseA.begin(), auditionVoiceExciterPhaseA.end(), 0.0);
        std::fill (auditionVoiceExciterPhaseB.begin(), auditionVoiceExciterPhaseB.end(), 0.0);
        std::fill (auditionVoiceExciterEnv.begin(), auditionVoiceExciterEnv.end(), 0.0f);
        std::fill (auditionVoiceExciterCooldownSamples.begin(), auditionVoiceExciterCooldownSamples.end(), 0);

        for (int voice = 0; voice < AUDITION_MAX_VOICES; ++voice)
        {
            auto seed = 0x13579BDFu ^ (0x9E3779B9u * static_cast<std::uint32_t> (voice + 1));
            seed ^= static_cast<std::uint32_t> (auditionSignalTypeIndex + 1) * 0x85EBCA6Bu;
            auditionVoiceNoiseState[static_cast<size_t> (voice)] = seed;
        }
    }

    bool isAuditionCloudBoundModeAvailable() const noexcept
    {
        if (currentSampleRate < 8000.0 || currentBlockSize <= 0)
            return false;

        return currentBlockSize <= 2048
            && currentBlockSize <= (AUDITION_HISTORY_BUFFER_SAMPLES / 2);
    }

    float nextAuditionVoiceWhiteNoise (int voiceIndex) noexcept
    {
        const auto idx = static_cast<size_t> (juce::jlimit (0, AUDITION_MAX_VOICES - 1, voiceIndex));
        auto& state = auditionVoiceNoiseState[idx];
        state = state * 1664525u + 1013904223u;
        const auto scrambled = state ^ (state >> 11) ^ (state << 7);
        return static_cast<float> (scrambled & 0x00FFFFFFu) / 8388608.0f - 1.0f;
    }

    float renderAuditionVoiceExcitation (int voiceIndex, int activeVoices, float delayedSample) noexcept
    {
        if (activeVoices <= 1 || ! isAuditionMultiSourceSignal (auditionSignalTypeIndex))
            return delayedSample;

        const auto idx = static_cast<size_t> (juce::jlimit (0, AUDITION_MAX_VOICES - 1, voiceIndex));
        auto& phaseA = auditionVoiceExciterPhaseA[idx];
        auto& phaseB = auditionVoiceExciterPhaseB[idx];
        auto& env = auditionVoiceExciterEnv[idx];
        auto& cooldown = auditionVoiceExciterCooldownSamples[idx];
        const auto sampleRate = juce::jmax (1.0, currentSampleRate);
        const auto hashA = auditionVoiceHashUnit (voiceIndex, 0xB5297A4Du);
        const auto hashB = auditionVoiceHashUnit (voiceIndex, 0x68E31DA4u);

        const auto advanceSine = [&sampleRate] (double& phase, double frequencyHz) noexcept -> float
        {
            phase += frequencyHz / sampleRate;
            phase -= std::floor (phase);
            return static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * phase));
        };

        if (cooldown > 0)
            --cooldown;

        switch (auditionSignalTypeIndex)
        {
            case 3: // rain
            {
                const auto toneA = advanceSine (phaseA, 740.0 + 2420.0 * (0.25 + 0.75 * hashA));
                const auto toneB = advanceSine (phaseB, 520.0 + 1560.0 * (0.30 + 0.70 * hashB));
                const auto triggerGate = qualityHigh ? 0.84f : 0.89f;
                const auto voiceNoise = nextAuditionVoiceWhiteNoise (voiceIndex);
                if (cooldown <= 0 && voiceNoise > triggerGate)
                {
                    const auto dropletPulse = 0.5f + 0.5f * toneB;
                    env = juce::jmax (env, 0.24f + 0.72f * dropletPulse);
                    const auto cooldownSeconds = 0.010f + 0.024f * hashB;
                    cooldown = static_cast<int> (std::round (cooldownSeconds * static_cast<float> (sampleRate)));
                }

                env *= qualityHigh ? 0.9939f : 0.9920f;
                const auto sparkle = toneA * std::abs (toneA);
                const auto droplet = (0.66f * toneA + 0.34f * sparkle) * env;
                const auto mist = 0.09f * voiceNoise * (0.35f + 0.65f * env);
                return juce::jlimit (-2.0f, 2.0f, 0.76f * delayedSample + 0.24f * droplet + mist);
            }
            case 4: // snow
            {
                const auto drift = advanceSine (phaseA, 72.0 + 120.0 * hashA);
                const auto flutter = advanceSine (phaseB, 0.38 + 0.44 * hashB);
                const auto frostNoise = nextAuditionVoiceWhiteNoise (voiceIndex)
                    * (0.26f + 0.74f * (0.5f + 0.5f * flutter));
                const auto veil = 0.88f * delayedSample + 0.12f * frostNoise;
                const auto shimmer = 0.17f * drift * (0.45f + 0.55f * std::abs (flutter));
                return juce::jlimit (-2.0f, 2.0f, 0.84f * veil + shimmer);
            }
            case 5:  // bouncing balls
            case 10: // membrane drops
            {
                const auto triggerGate = (auditionSignalTypeIndex == 5)
                    ? (qualityHigh ? 0.80f : 0.86f)
                    : (qualityHigh ? 0.78f : 0.84f);
                const auto voiceNoise = nextAuditionVoiceWhiteNoise (voiceIndex);
                if (cooldown <= 0 && voiceNoise > triggerGate)
                {
                    env = juce::jmax (env, 0.52f + 0.42f * (0.5f + 0.5f * voiceNoise));
                    const auto cooldownSeconds = (auditionSignalTypeIndex == 5 ? 0.040f : 0.055f)
                        + (auditionSignalTypeIndex == 5 ? 0.090f : 0.120f) * hashA;
                    cooldown = static_cast<int> (std::round (cooldownSeconds * static_cast<float> (sampleRate)));
                }

                env *= qualityHigh ? 0.9898f : 0.9868f;
                const auto modalA = advanceSine (phaseA, 130.0 + 410.0 * hashA + 170.0 * env);
                const auto modalB = advanceSine (phaseB, 208.0 + 500.0 * hashB);
                auto strikeEnv = env;
                strikeEnv *= strikeEnv;
                strikeEnv *= strikeEnv;
                const auto resonant = (0.70f * modalA + 0.30f * modalB) * env;
                const auto click = 0.24f * voiceNoise * strikeEnv;
                const auto blended = (auditionSignalTypeIndex == 5)
                    ? (0.70f * delayedSample + 0.30f * resonant + click)
                    : (0.76f * delayedSample + 0.24f * resonant + 0.16f * click);
                return juce::jlimit (-2.0f, 2.0f, blended);
            }
            case 6: // chimes
            {
                const auto voiceNoise = nextAuditionVoiceWhiteNoise (voiceIndex);
                if (cooldown <= 0 && voiceNoise > (qualityHigh ? 0.88f : 0.92f))
                {
                    env = juce::jmax (env, 0.48f + 0.48f * (0.5f + 0.5f * voiceNoise));
                    const auto cooldownSeconds = (qualityHigh ? 0.11f : 0.15f) + 0.18f * hashA;
                    cooldown = static_cast<int> (std::round (cooldownSeconds * static_cast<float> (sampleRate)));
                }

                env *= qualityHigh ? 0.99934f : 0.99886f;
                const auto partialA = advanceSine (phaseA, 520.0 + 1080.0 * hashA);
                const auto partialB = advanceSine (phaseB, 780.0 + 1540.0 * hashB);
                const auto inharmonic = static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * (phaseA * 1.618 + phaseB * 0.337)));
                auto strikeEnv = env;
                strikeEnv *= strikeEnv;
                strikeEnv *= strikeEnv;
                strikeEnv *= strikeEnv;
                const auto resonant = (0.58f * partialA + 0.29f * partialB + 0.13f * inharmonic) * env;
                const auto strike = 0.18f * voiceNoise * strikeEnv;
                return juce::jlimit (-2.0f, 2.0f, 0.64f * delayedSample + 0.36f * resonant + strike);
            }
            default:
                return delayedSample;
        }
    }

    bool isAuditionMultiSourceSignal (int signalIndex) const noexcept
    {
        switch (signalIndex)
        {
            case 3:  // rain
            case 4:  // snow
            case 5:  // bouncing balls
            case 6:  // wind chimes
            case 7:  // crickets
            case 8:  // song birds
            case 9:  // karplus plucks
            case 10: // membrane drops
            case 11: // krell patch
            case 12: // generative arp
                return true;
            default:
                return false;
        }
    }

    int getAuditionVoiceCountForSignal() const noexcept
    {
        if (! isAuditionMultiSourceSignal (auditionSignalTypeIndex))
            return 1;

        switch (auditionSignalTypeIndex)
        {
            case 3: // rain
            case 4: // snow
            case 7: // crickets
            case 8: // song birds
                return qualityHigh ? 7 : 5;
            case 5: // bouncing
                return qualityHigh ? 6 : 4;
            case 6: // chimes
            case 9: // karplus
            case 10: // membrane
            case 12: // arp
                return qualityHigh ? 5 : 4;
            case 11: // krell
                return qualityHigh ? 4 : 3;
            default:
                return 1;
        }
    }

    float getAuditionVoiceSpreadDegrees() const noexcept
    {
        switch (auditionSignalTypeIndex)
        {
            case 3: // rain
            case 4: // snow
            case 5: // bouncing balls
            case 7: // crickets
            case 8: // song birds
                return 172.0f;
            case 6: // chimes
            case 9: // karplus plucks
            case 10: // membrane drops
            case 11: // krell patch
            case 12: // generative arp
                return 156.0f;
            default:
                return 0.0f;
        }
    }

    int getAuditionVoiceDelaySamples (int voiceIndex, int voiceCount) const noexcept
    {
        if (voiceIndex <= 0 || voiceCount <= 1)
            return 0;

        int maxDelayMs = 18;
        switch (auditionSignalTypeIndex)
        {
            case 3: // rain
            case 4: // snow
                maxDelayMs = qualityHigh ? 95 : 70;
                break;
            case 5: // bouncing balls
                maxDelayMs = qualityHigh ? 140 : 95;
                break;
            case 7: // crickets
            case 8: // birds
                maxDelayMs = qualityHigh ? 82 : 56;
                break;
            case 6: // chimes
            case 9: // plucks
            case 10: // membrane
            case 11: // krell
            case 12: // arp
                maxDelayMs = qualityHigh ? 62 : 44;
                break;
            default:
                maxDelayMs = 18;
                break;
        }

        const auto voiceNorm = static_cast<float> (voiceIndex) / static_cast<float> (juce::jmax (1, voiceCount - 1));
        const auto jitterMs = 10.0f * auditionVoiceHashUnit (voiceIndex, 0xA53C9E11u);
        const auto delayMs = juce::jlimit (0.0f, static_cast<float> (maxDelayMs), maxDelayMs * voiceNorm + jitterMs);
        const auto sampleRate = juce::jmax (1.0, currentSampleRate);
        return juce::jlimit (
            0,
            AUDITION_HISTORY_BUFFER_SAMPLES - 1,
            static_cast<int> (std::round (delayMs * static_cast<float> (sampleRate) * 0.001f)));
    }

    float readAuditionHistoryDelayed (int delaySamples) const noexcept
    {
        const auto boundedDelay = juce::jlimit (0, AUDITION_HISTORY_BUFFER_SAMPLES - 1, delaySamples);
        auto readIndex = auditionHistoryWritePos - 1 - boundedDelay;
        while (readIndex < 0)
            readIndex += AUDITION_HISTORY_BUFFER_SAMPLES;
        return auditionHistoryBuffer[static_cast<size_t> (readIndex)];
    }

    void publishAuditionReactiveTelemetry (
        float rms,
        float peak,
        float envFast,
        float envSlow,
        float onset,
        float brightness,
        float rainFadeRate,
        float snowFadeRate,
        float physicsVelocity,
        float physicsCollision,
        float physicsDensity,
        float physicsCoupling,
        float headphoneOutputRms,
        float headphoneOutputPeak,
        float headphoneParity,
        int headphoneFallbackReasonIndex,
        const std::array<float, AUDITION_MAX_VOICES>& sourceEnergy,
        int sourceCount) noexcept
    {
        auditionReactiveRms.store (sanitizeUnitScalar (rms), std::memory_order_relaxed);
        auditionReactivePeak.store (sanitizeUnitScalar (peak), std::memory_order_relaxed);
        auditionReactiveEnvFast.store (sanitizeUnitScalar (envFast), std::memory_order_relaxed);
        auditionReactiveEnvSlow.store (sanitizeUnitScalar (envSlow), std::memory_order_relaxed);
        auditionReactiveOnset.store (sanitizeUnitScalar (onset), std::memory_order_relaxed);
        auditionReactiveBrightness.store (sanitizeUnitScalar (brightness), std::memory_order_relaxed);
        auditionReactiveRainFadeRate.store (sanitizeUnitScalar (rainFadeRate), std::memory_order_relaxed);
        auditionReactiveSnowFadeRate.store (sanitizeUnitScalar (snowFadeRate), std::memory_order_relaxed);
        auditionReactivePhysicsVelocity.store (sanitizeUnitScalar (physicsVelocity), std::memory_order_relaxed);
        auditionReactivePhysicsCollision.store (sanitizeUnitScalar (physicsCollision), std::memory_order_relaxed);
        auditionReactivePhysicsDensity.store (sanitizeUnitScalar (physicsDensity), std::memory_order_relaxed);
        auditionReactivePhysicsCoupling.store (sanitizeUnitScalar (physicsCoupling), std::memory_order_relaxed);
        auditionReactiveHeadphoneOutputRms.store (
            sanitizeUnitScalar (headphoneOutputRms),
            std::memory_order_relaxed);
        auditionReactiveHeadphoneOutputPeak.store (
            sanitizeUnitScalar (headphoneOutputPeak),
            std::memory_order_relaxed);
        auditionReactiveHeadphoneParity.store (
            sanitizeUnitScalar (headphoneParity),
            std::memory_order_relaxed);
        auditionReactiveHeadphoneFallbackReasonIndex.store (
            sanitizeHeadphoneFallbackReasonIndex (headphoneFallbackReasonIndex),
            std::memory_order_relaxed);
        auditionReactiveSourceCount.store (
            sanitizeSourceCount (sourceCount),
            std::memory_order_relaxed);

        for (int i = 0; i < AUDITION_MAX_VOICES; ++i)
        {
            auditionReactiveSourceEnergy[static_cast<size_t> (i)].store (
                sanitizeUnitScalar (sourceEnergy[static_cast<size_t> (i)]),
                std::memory_order_relaxed);
        }
    }

    void resetAuditionReactiveTelemetry() noexcept
    {
        auditionReactiveEnvFastState = 0.0f;
        auditionReactiveEnvSlowState = 0.0f;
        auditionReactiveBrightnessLowpassState = 0.0f;
        auditionPhysicsReactiveInputActive = false;
        auditionPhysicsReactiveVelocityTarget = 0.0f;
        auditionPhysicsReactiveCollisionTarget = 0.0f;
        auditionPhysicsReactiveDensityTarget = 0.0f;
        auditionPhysicsReactiveVelocityState = 0.0f;
        auditionPhysicsReactiveCollisionState = 0.0f;
        auditionPhysicsReactiveDensityState = 0.0f;
        auditionPhysicsReactiveTimbreLowpassState = 0.0f;
        std::array<float, AUDITION_MAX_VOICES> sourceEnergy {};
        publishAuditionReactiveTelemetry (
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None),
            sourceEnergy,
            0);
    }

    void applyAuditionReactiveHeadphoneParity (
        float headphoneOutputRms,
        float headphoneOutputPeak,
        float headphoneParity,
        int headphoneFallbackReasonIndex) noexcept
    {
        const auto parity = sanitizeUnitScalar (headphoneParity, 1.0f);
        const auto scaledRms = juce::jlimit (
            0.0f,
            1.0f,
            auditionReactiveRms.load (std::memory_order_relaxed) * parity);
        const auto scaledPeak = juce::jlimit (
            0.0f,
            1.0f,
            auditionReactivePeak.load (std::memory_order_relaxed) * parity);
        const auto scaledEnvFast = juce::jlimit (
            0.0f,
            1.0f,
            auditionReactiveEnvFast.load (std::memory_order_relaxed) * parity);
        const auto scaledEnvSlow = juce::jlimit (
            0.0f,
            1.0f,
            auditionReactiveEnvSlow.load (std::memory_order_relaxed) * parity);
        const auto onset = auditionReactiveOnset.load (std::memory_order_relaxed);
        const auto brightness = auditionReactiveBrightness.load (std::memory_order_relaxed);
        const auto parityBlend = 0.72f + 0.28f * parity;
        const auto scaledRainFadeRate = juce::jlimit (
            0.0f,
            1.0f,
            auditionReactiveRainFadeRate.load (std::memory_order_relaxed) * parityBlend);
        const auto scaledSnowFadeRate = juce::jlimit (
            0.0f,
            1.0f,
            auditionReactiveSnowFadeRate.load (std::memory_order_relaxed) * parityBlend);

        auditionReactiveRms.store (scaledRms, std::memory_order_relaxed);
        auditionReactivePeak.store (scaledPeak, std::memory_order_relaxed);
        auditionReactiveEnvFast.store (scaledEnvFast, std::memory_order_relaxed);
        auditionReactiveEnvSlow.store (scaledEnvSlow, std::memory_order_relaxed);
        auditionReactiveOnset.store (juce::jlimit (0.0f, 1.0f, onset), std::memory_order_relaxed);
        auditionReactiveBrightness.store (juce::jlimit (0.0f, 1.0f, brightness), std::memory_order_relaxed);
        auditionReactiveRainFadeRate.store (scaledRainFadeRate, std::memory_order_relaxed);
        auditionReactiveSnowFadeRate.store (scaledSnowFadeRate, std::memory_order_relaxed);
        auditionReactiveHeadphoneOutputRms.store (
            sanitizeUnitScalar (headphoneOutputRms),
            std::memory_order_relaxed);
        auditionReactiveHeadphoneOutputPeak.store (
            sanitizeUnitScalar (headphoneOutputPeak),
            std::memory_order_relaxed);
        auditionReactiveHeadphoneParity.store (parity, std::memory_order_relaxed);
        auditionReactiveHeadphoneFallbackReasonIndex.store (
            sanitizeHeadphoneFallbackReasonIndex (headphoneFallbackReasonIndex),
            std::memory_order_relaxed);
    }

    float applyAuditionPhysicsReactiveTimbre (
        float sample,
        float physicsVelocity,
        float physicsCollision,
        float physicsDensity,
        float motionEnergy) noexcept
    {
        const auto couplingBlend = juce::jlimit (
            0.0f,
            1.0f,
            0.44f * physicsVelocity + 0.36f * physicsCollision + 0.20f * physicsDensity);
        if (couplingBlend <= 1.0e-5f)
            return sample;

        const auto lowpassAlpha = juce::jlimit (0.02f, 0.34f, 0.055f + 0.18f * physicsVelocity);
        auditionPhysicsReactiveTimbreLowpassState += (sample - auditionPhysicsReactiveTimbreLowpassState) * lowpassAlpha;
        const auto high = sample - auditionPhysicsReactiveTimbreLowpassState;
        const auto transient = 1.0f + 0.85f * physicsCollision;
        const auto densityBody = 0.92f + 0.28f * physicsDensity;
        float shaped = sample;

        switch (auditionSignalTypeIndex)
        {
            case 3: // rain
                shaped = sample * densityBody
                    + high * (0.18f + 0.44f * physicsVelocity)
                    + high * (0.10f + 0.18f * motionEnergy) * transient;
                break;
            case 4: // snow
                shaped = sample * (0.94f + 0.24f * physicsDensity)
                    + high * (0.06f + 0.16f * physicsVelocity)
                    - high * (0.04f + 0.10f * physicsCollision);
                break;
            case 5: // bouncing
                shaped = std::tanh (sample * (1.0f + 0.56f * physicsCollision + 0.24f * physicsVelocity))
                    + high * (0.10f + 0.16f * physicsVelocity);
                break;
            case 6: // chimes
                shaped = sample * (1.0f + 0.26f * physicsCollision)
                    + high * (0.14f + 0.26f * physicsVelocity + 0.08f * physicsDensity);
                break;
            default:
                shaped = sample * (0.95f + 0.20f * couplingBlend)
                    + high * (0.08f + 0.18f * physicsVelocity);
                break;
        }

        const auto wet = juce::jlimit (
            0.08f,
            0.82f,
            0.20f + 0.52f * couplingBlend + 0.10f * motionEnergy);
        return juce::jlimit (-2.0f, 2.0f, sample + (shaped - sample) * wet);
    }

    float generateAuditionSignalSample() noexcept
    {
        const auto sampleRate = juce::jmax (1.0, currentSampleRate);

        switch (auditionSignalTypeIndex)
        {
            case 0: // Sine 440 Hz
                return advanceAuditionOscillator (440.0, auditionPhasePrimary);
            case 1: // Dual tone 220 + 880 Hz
                return 0.7f * advanceAuditionOscillator (220.0, auditionPhasePrimary)
                     + 0.3f * advanceAuditionOscillator (880.0, auditionPhaseSecondary);
            case 2: // Soft pink-like noise (simple filtered white)
            {
                const auto white = nextAuditionWhiteNoise();
                auditionNoiseOnePole = 0.985f * auditionNoiseOnePole + 0.015f * white;
                return auditionNoiseOnePole;
            }
            case 3: // Rain field with random droplets
            {
                const auto white = nextAuditionWhiteNoise();
                auditionRainBed = 0.9986f * auditionRainBed + 0.0014f * white;
                const auto rainHissRaw = white - auditionRainBed;
                auditionNoiseOnePole = 0.95f * auditionNoiseOnePole + 0.05f * rainHissRaw;

                const auto triggerRateHz = qualityHigh ? 42.0f : 31.0f;
                if (nextAuditionRand01() < triggerRateHz / static_cast<float> (sampleRate))
                {
                    auditionRainDropEnv = juce::jmin (1.0f, auditionRainDropEnv + (0.26f + 0.50f * nextAuditionRand01()));
                    auto randSquared = nextAuditionRand01();
                    randSquared *= randSquared;
                    auditionRainDropFreqHz = 620.0f + (3600.0f * randSquared);
                }

                auditionRainDropPhase += static_cast<double> (auditionRainDropFreqHz) / sampleRate;
                auditionRainDropPhase -= std::floor (auditionRainDropPhase);
                const auto rainPhase = static_cast<float> (juce::MathConstants<double>::twoPi * auditionRainDropPhase);
                const auto rainSine = static_cast<float> (std::sin (rainPhase));
                const auto rainSparkle = rainSine * std::abs (rainSine);
                const auto droplet = (0.72f * rainSine + 0.28f * rainSparkle) * auditionRainDropEnv;
                const auto splash = 0.11f * nextAuditionWhiteNoise() * auditionRainDropEnv;
                auditionRainDropEnv *= qualityHigh ? 0.9942f : 0.9930f;

                return 0.52f * auditionNoiseOnePole + 0.58f * droplet + splash;
            }
            case 4: // Snow drift (soft airy noise)
            {
                const auto white = nextAuditionWhiteNoise();
                auditionSnowBed = 0.99962f * auditionSnowBed + 0.00038f * white;
                const auto airyResidual = white - auditionSnowBed;
                auditionSnowShimmer = 0.9973f * auditionSnowShimmer + 0.0027f * airyResidual;

                auditionSnowFlutterPhase += 0.075 / sampleRate;
                auditionSnowFlutterPhase -= std::floor (auditionSnowFlutterPhase);
                auditionPhaseSecondary += 0.24 / sampleRate;
                auditionPhaseSecondary -= std::floor (auditionPhaseSecondary);

                const auto flutter = 0.86f + 0.14f * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionSnowFlutterPhase));
                const auto shimmerMod = 0.22f + 0.78f * (0.5f + 0.5f * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionPhaseSecondary)));
                const auto airy = 0.66f * auditionSnowBed + 0.34f * (0.86f * auditionSnowShimmer + 0.14f * airyResidual);
                const auto shimmer = 0.12f * auditionSnowShimmer * shimmerMod;
                const auto lowJitterBreath = 0.05f * static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * (auditionSnowFlutterPhase + 0.17 * auditionPhaseSecondary)));

                return airy * flutter + shimmer + lowJitterBreath;
            }
            case 5: // Bouncing balls (clustered impacts)
            {
                bool triggerBounce = false;
                if (auditionBounceCountdownSamples > 0)
                {
                    --auditionBounceCountdownSamples;
                }
                else if (auditionBounceClusterRemaining > 0)
                {
                    triggerBounce = true;
                    --auditionBounceClusterRemaining;
                    if (auditionBounceClusterRemaining > 0)
                    {
                        auditionBounceSpacingSamples = juce::jmax (44.0f, auditionBounceSpacingSamples * (0.58f + 0.08f * nextAuditionRand01()));
                        auditionBounceCountdownSamples = static_cast<int> (std::round (auditionBounceSpacingSamples));
                    }
                    else
                    {
                        auditionBounceCooldownSamples = static_cast<int> (
                            std::round ((0.45f + 0.90f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                    }
                }
                else
                {
                    if (auditionBounceCooldownSamples > 0)
                        --auditionBounceCooldownSamples;

                    if (auditionBounceCooldownSamples <= 0 && nextAuditionRand01() < static_cast<float> (1.4 / sampleRate))
                    {
                        auditionBounceClusterRemaining = 3 + static_cast<int> (nextAuditionRand01() * 6.0f);
                        auditionBounceSpacingSamples = (0.16f + 0.14f * nextAuditionRand01()) * static_cast<float> (sampleRate);
                        triggerBounce = true;
                        --auditionBounceClusterRemaining;
                        if (auditionBounceClusterRemaining > 0)
                            auditionBounceCountdownSamples = static_cast<int> (std::round (auditionBounceSpacingSamples));
                    }
                }

                if (triggerBounce)
                {
                    const auto spacingNorm = juce::jlimit (0.0f, 1.0f, auditionBounceSpacingSamples / (0.36f * static_cast<float> (sampleRate)));
                    const auto impact = 0.24f + 0.76f * spacingNorm;
                    auditionBounceEnv = juce::jmax (auditionBounceEnv, impact);
                    auto randSquared = nextAuditionRand01();
                    randSquared *= randSquared;
                    const auto targetFreq = 130.0f + (680.0f * spacingNorm) + (220.0f * randSquared);
                    auditionBounceFreqHz = 0.55f * auditionBounceFreqHz + 0.45f * targetFreq;
                }

                auditionBouncePhase += static_cast<double> (auditionBounceFreqHz) / sampleRate;
                auditionBouncePhase -= std::floor (auditionBouncePhase);
                const auto bouncePhase = static_cast<float> (juce::MathConstants<double>::twoPi * auditionBouncePhase);
                const auto tonal = (0.76f * static_cast<float> (std::sin (bouncePhase))
                                  + 0.18f * static_cast<float> (std::sin (bouncePhase * 2.35f))
                                  + 0.06f * static_cast<float> (std::sin (bouncePhase * 3.70f)))
                    * auditionBounceEnv;
                auditionNoiseOnePole = 0.90f * auditionNoiseOnePole + 0.10f * nextAuditionWhiteNoise();
                const auto thud = auditionNoiseOnePole * (0.26f * auditionBounceEnv);
                auto impactStrike = auditionBounceEnv;
                impactStrike *= impactStrike;
                impactStrike *= impactStrike;
                const auto impactClick = 0.22f * nextAuditionWhiteNoise() * impactStrike;
                auditionBounceEnv *= qualityHigh ? 0.9960f : 0.9948f;

                return 0.74f * tonal + thud + impactClick;
            }
            case 6: // Wind chimes (metallic resonant pings)
            {
                if (auditionChimeCooldownSamples > 0)
                    --auditionChimeCooldownSamples;

                if (auditionChimeEnv < 1.0e-4f
                    && auditionChimeCooldownSamples <= 0
                    && nextAuditionRand01() < static_cast<float> ((qualityHigh ? 1.05f : 0.80f) / sampleRate))
                {
                    static constexpr std::array<float, 6> kChimeNotes {
                        392.0f, 523.25f, 659.25f, 783.99f, 987.77f, 1174.66f
                    };
                    static constexpr std::array<float, 4> kChimeRatios {
                        1.50f, 1.6666666f, 2.0f, 2.5f
                    };
                    const auto noteIndex = juce::jlimit (
                        0,
                        static_cast<int> (kChimeNotes.size()) - 1,
                        static_cast<int> (nextAuditionRand01() * static_cast<float> (kChimeNotes.size())));
                    const auto ratioIndex = juce::jlimit (
                        0,
                        static_cast<int> (kChimeRatios.size()) - 1,
                        static_cast<int> (nextAuditionRand01() * static_cast<float> (kChimeRatios.size())));
                    auditionChimeFreqA = kChimeNotes[static_cast<size_t> (noteIndex)];
                    auditionChimeFreqB = auditionChimeFreqA * kChimeRatios[static_cast<size_t> (ratioIndex)];
                    auditionChimeEnv = 0.88f + 0.12f * nextAuditionRand01();
                    auditionChimeCooldownSamples = static_cast<int> (
                        std::round ((0.14f + 0.44f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                }

                auditionChimePhaseA += static_cast<double> (auditionChimeFreqA) / sampleRate;
                auditionChimePhaseB += static_cast<double> (auditionChimeFreqB) / sampleRate;
                auditionChimePhaseA -= std::floor (auditionChimePhaseA);
                auditionChimePhaseB -= std::floor (auditionChimePhaseB);
                const auto chimePhaseA = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseA);
                const auto chimePhaseB = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseB);
                const auto body = (0.58f * static_cast<float> (std::sin (chimePhaseA))
                    + 0.26f * static_cast<float> (std::sin (chimePhaseB))
                    + 0.10f * static_cast<float> (std::sin (0.5f * (chimePhaseA + chimePhaseB)))
                    + 0.06f * static_cast<float> (std::sin (1.618f * chimePhaseA + 0.37f * chimePhaseB)))
                    * auditionChimeEnv;
                auto strikeEnv = auditionChimeEnv;
                strikeEnv *= strikeEnv;
                strikeEnv *= strikeEnv;
                strikeEnv *= strikeEnv;
                strikeEnv *= strikeEnv;
                const auto strike = (0.72f * static_cast<float> (std::sin (chimePhaseA * 2.75f))
                    + 0.28f * static_cast<float> (std::sin (chimePhaseB * 1.90f)))
                    * strikeEnv;
                auditionChimeEnv *= qualityHigh ? 0.99976f : 0.99962f;
                auditionChimeShimmer = 0.992f * auditionChimeShimmer + 0.008f * std::abs (body);
                return 0.70f * body + 0.24f * strike + 0.10f * auditionChimeShimmer;
            }
            case 7: // Crickets (narrow-band chirp swarms)
            {
                if (auditionCricketCooldownSamples > 0)
                    --auditionCricketCooldownSamples;

                if (auditionCricketBurstSamples <= 0
                    && auditionCricketCooldownSamples <= 0
                    && nextAuditionRand01() < static_cast<float> (1.0 / sampleRate))
                {
                    auditionCricketBurstSamples = static_cast<int> (
                        std::round ((0.06f + 0.12f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                    auditionCricketCooldownSamples = static_cast<int> (
                        std::round ((0.20f + 0.58f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                    auditionCricketFreqHz = 3200.0f + 3800.0f * nextAuditionRand01();
                    auditionCricketEnv = juce::jmax (auditionCricketEnv, 0.72f + 0.22f * nextAuditionRand01());
                }

                if (auditionCricketBurstSamples > 0)
                {
                    --auditionCricketBurstSamples;
                    auditionCricketEnv = juce::jmin (1.0f, auditionCricketEnv + 0.016f);
                }
                else
                {
                    auditionCricketEnv *= 0.9975f;
                }

                auditionCricketPhase += static_cast<double> (auditionCricketFreqHz) / sampleRate;
                auditionCricketPhase -= std::floor (auditionCricketPhase);
                auditionPhaseSecondary += 34.0 / sampleRate;
                auditionPhaseSecondary -= std::floor (auditionPhaseSecondary);

                const auto cricketCarrier = static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionCricketPhase));
                const auto pulseRaw = 0.5f + 0.5f * static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionPhaseSecondary));
                const auto pulse = pulseRaw * pulseRaw * pulseRaw;
                const auto buzz = 0.18f * nextAuditionWhiteNoise();
                return (0.82f * cricketCarrier + buzz) * auditionCricketEnv * pulse;
            }
            case 8: // Song birds (warbled chirp phrases)
            {
                if (auditionBirdCooldownSamples > 0)
                    --auditionBirdCooldownSamples;

                if (auditionBirdPhraseSamples <= 0
                    && auditionBirdCooldownSamples <= 0
                    && nextAuditionRand01() < static_cast<float> (0.72 / sampleRate))
                {
                    auditionBirdPhraseSamples = static_cast<int> (
                        std::round ((0.16f + 0.34f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                    auditionBirdCooldownSamples = static_cast<int> (
                        std::round ((0.26f + 0.66f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                    auditionBirdFreqA = 880.0f + 1900.0f * nextAuditionRand01();
                    auditionBirdFreqB = auditionBirdFreqA * (1.42f + 0.36f * nextAuditionRand01());
                    auditionBirdEnv = 1.0f;
                }

                if (auditionBirdPhraseSamples > 0)
                    --auditionBirdPhraseSamples;

                auditionBirdWarblePhase += (2.2 + 3.4 * nextAuditionRand01()) / sampleRate;
                auditionBirdWarblePhase -= std::floor (auditionBirdWarblePhase);
                const auto warble = static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionBirdWarblePhase));
                const auto trill = 0.5f + 0.5f * static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionBirdWarblePhase * 7.5));

                const auto freqA = auditionBirdFreqA * (1.0f + 0.18f * warble);
                const auto freqB = auditionBirdFreqB * (1.0f + 0.12f * static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionBirdWarblePhase * 1.7)));
                auditionBirdPhaseA += static_cast<double> (freqA) / sampleRate;
                auditionBirdPhaseB += static_cast<double> (freqB) / sampleRate;
                auditionBirdPhaseA -= std::floor (auditionBirdPhaseA);
                auditionBirdPhaseB -= std::floor (auditionBirdPhaseB);

                const auto birdA = static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionBirdPhaseA));
                const auto birdB = static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionBirdPhaseB));
                const auto whistle = 0.72f * birdA + 0.28f * birdB;

                if (auditionBirdPhraseSamples > 0)
                    auditionBirdEnv *= 0.99935f;
                else
                    auditionBirdEnv *= 0.9958f;

                const auto ambience = 0.06f * auditionSnowShimmer;
                return whistle * auditionBirdEnv * (0.55f + 0.45f * trill) + ambience;
            }
            case 9: // Karplus plucks (physical string model)
            {
                if (auditionKarplusCooldownSamples > 0)
                    --auditionKarplusCooldownSamples;

                if (auditionKarplusCooldownSamples <= 0
                    && nextAuditionRand01() < static_cast<float> (0.85 / sampleRate))
                {
                    static constexpr std::array<float, 10> kPluckNotes {
                        110.0f, 123.47f, 146.83f, 164.81f, 196.0f,
                        220.0f, 246.94f, 293.66f, 329.63f, 392.0f
                    };
                    const auto noteIndex = juce::jlimit (
                        0,
                        static_cast<int> (kPluckNotes.size()) - 1,
                        static_cast<int> (nextAuditionRand01() * static_cast<float> (kPluckNotes.size())));
                    const auto noteHz = kPluckNotes[static_cast<size_t> (noteIndex)] * (0.98f + 0.05f * nextAuditionRand01());
                    auditionKarplusDelaySamples = juce::jlimit (
                        24,
                        kAuditionKarplusMaxDelaySamples - 2,
                        static_cast<int> (std::round (sampleRate / juce::jmax (50.0f, noteHz))));
                    auditionKarplusDamping = qualityHigh ? (0.992f + 0.004f * nextAuditionRand01())
                                                         : (0.986f + 0.004f * nextAuditionRand01());
                    for (int i = 0; i < auditionKarplusDelaySamples; ++i)
                        auditionKarplusDelayLine[static_cast<size_t> (i)] = 0.78f * nextAuditionWhiteNoise();
                    auditionKarplusWriteIndex = 0;
                    auditionKarplusEnv = 1.0f;
                    auditionKarplusCooldownSamples = static_cast<int> (
                        std::round ((0.15f + 0.30f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                }

                const auto delayLength = juce::jlimit (24, kAuditionKarplusMaxDelaySamples - 2, auditionKarplusDelaySamples);
                int readIndex = auditionKarplusWriteIndex - delayLength;
                if (readIndex < 0)
                    readIndex += kAuditionKarplusMaxDelaySamples;
                const int readNextIndex = (readIndex + 1) % kAuditionKarplusMaxDelaySamples;
                const auto delayed = auditionKarplusDelayLine[static_cast<size_t> (readIndex)];
                const auto delayedNext = auditionKarplusDelayLine[static_cast<size_t> (readNextIndex)];
                const auto filtered = 0.5f * (delayed + delayedNext) * auditionKarplusDamping;
                auditionKarplusDelayLine[static_cast<size_t> (auditionKarplusWriteIndex)] = filtered;
                auditionKarplusWriteIndex = (auditionKarplusWriteIndex + 1) % kAuditionKarplusMaxDelaySamples;
                auditionKarplusEnv *= 0.99970f;
                return delayed * auditionKarplusEnv;
            }
            case 10: // Membrane drops (physical modal impacts)
            {
                if (auditionMembraneCooldownSamples > 0)
                    --auditionMembraneCooldownSamples;

                if (auditionMembraneCooldownSamples <= 0
                    && nextAuditionRand01() < static_cast<float> (0.95 / sampleRate))
                {
                    auto randSquared = nextAuditionRand01();
                    randSquared *= randSquared;
                    auditionMembraneFreqA = 120.0f + 260.0f * randSquared;
                    auditionMembraneFreqB = auditionMembraneFreqA * (1.55f + 0.25f * nextAuditionRand01());
                    auditionMembraneEnv = 1.0f;
                    auditionMembraneCooldownSamples = static_cast<int> (
                        std::round ((0.24f + 0.44f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                }

                auditionMembranePhaseA += static_cast<double> (auditionMembraneFreqA) / sampleRate;
                auditionMembranePhaseB += static_cast<double> (auditionMembraneFreqB) / sampleRate;
                auditionMembranePhaseA -= std::floor (auditionMembranePhaseA);
                auditionMembranePhaseB -= std::floor (auditionMembranePhaseB);
                const auto modeA = static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionMembranePhaseA));
                const auto modeB = static_cast<float> (std::sin (
                    juce::MathConstants<double>::twoPi * auditionMembranePhaseB));
                const auto body = (0.70f * modeA + 0.30f * modeB) * auditionMembraneEnv;
                auto strikeEnv = auditionMembraneEnv;
                strikeEnv *= strikeEnv;
                strikeEnv *= strikeEnv;
                const auto strike = 0.28f * nextAuditionWhiteNoise() * strikeEnv;
                auditionMembraneEnv *= 0.99920f;
                return body + strike;
            }
            case 11: // Krell patch (generative synth glide)
            {
                if (auditionKrellStepSamples <= 0)
                {
                    static constexpr std::array<float, 10> kKrellRatios {
                        1.0f, 1.122462f, 1.189207f, 1.334840f, 1.414214f,
                        1.587401f, 1.681793f, 1.887749f, 2.0f, 2.244924f
                    };
                    const auto ratioIndex = juce::jlimit (
                        0,
                        static_cast<int> (kKrellRatios.size()) - 1,
                        static_cast<int> (nextAuditionRand01() * static_cast<float> (kKrellRatios.size())));
                    auditionKrellFreqTarget = 82.41f * kKrellRatios[static_cast<size_t> (ratioIndex)] * (1.0f + 0.45f * nextAuditionRand01());
                    auditionKrellEnv = 0.45f + 0.55f * nextAuditionRand01();
                    auditionKrellStepSamples = static_cast<int> (
                        std::round ((0.16f + 0.72f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                }
                else
                {
                    --auditionKrellStepSamples;
                }

                auditionKrellFreqCurrent += (auditionKrellFreqTarget - auditionKrellFreqCurrent) * 0.0015f;
                auditionKrellPhase += std::max (40.0, static_cast<double> (auditionKrellFreqCurrent)) / sampleRate;
                auditionKrellPhase -= std::floor (auditionKrellPhase);
                auditionPhaseSecondary += 0.18 / sampleRate;
                auditionPhaseSecondary -= std::floor (auditionPhaseSecondary);
                const auto lfo = static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionPhaseSecondary));
                const auto phase = juce::MathConstants<double>::twoPi * auditionKrellPhase;
                const auto carrier = static_cast<float> (std::sin (phase + 0.45 * lfo));
                const auto sub = static_cast<float> (std::sin (phase * 0.5));
                const auto harmonics = static_cast<float> (std::sin (phase * (2.0 + 0.28 * lfo)));
                auditionKrellEnv *= 0.99980f;
                return std::tanh ((0.62f * carrier + 0.26f * sub + 0.18f * harmonics) * (0.65f + auditionKrellEnv));
            }
            case 12: // Generative arp patch
            {
                if (auditionArpGateSamples <= 0)
                {
                    static constexpr std::array<int, 12> kArpSemitones {
                        0, 2, 3, 5, 7, 10, 12, 14, 15, 17, 19, 22
                    };
                    auditionArpStepIndex = (auditionArpStepIndex + 1 + static_cast<int> (nextAuditionRand01() * 3.0f))
                        % static_cast<int> (kArpSemitones.size());
                    const auto semitone = kArpSemitones[static_cast<size_t> (auditionArpStepIndex)];
                    const auto freqBase = 110.0f * std::pow (2.0f, static_cast<float> (semitone) / 12.0f);
                    auditionArpFreqA = freqBase;
                    auditionArpFreqB = freqBase * (1.5f + 0.08f * nextAuditionRand01());
                    auditionArpEnv = 1.0f;
                    auditionArpGateSamples = static_cast<int> (
                        std::round ((0.05f + 0.17f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                }
                else
                {
                    --auditionArpGateSamples;
                }

                auditionArpPhaseA += static_cast<double> (auditionArpFreqA) / sampleRate;
                auditionArpPhaseB += static_cast<double> (auditionArpFreqB) / sampleRate;
                auditionArpPhaseA -= std::floor (auditionArpPhaseA);
                auditionArpPhaseB -= std::floor (auditionArpPhaseB);
                const auto toneA = static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionArpPhaseA));
                const auto toneB = static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionArpPhaseB));
                const auto sparkle = 0.15f * nextAuditionWhiteNoise();
                auditionArpEnv *= (auditionArpGateSamples > 0) ? 0.9968f : 0.9920f;
                return (0.62f * toneA + 0.28f * toneB + sparkle) * auditionArpEnv;
            }
            default:
                return advanceAuditionOscillator (440.0, auditionPhasePrimary);
        }
    }

    void renderInternalAuditionEmitter (int numSamples) noexcept
    {
        if (numSamples <= 0)
            return;

        const auto levelDb = auditionLevelDbForPreset (auditionLevelPresetIndex);
        const auto signalGain = juce::Decibels::decibelsToGain (levelDb);

        float azimuth = 0.0f;
        float elevation = 0.0f;
        float auditionDistanceMeters = 1.0f;
        double orbitHz = 0.0;
        const auto phaseRadians = juce::MathConstants<double>::twoPi * auditionOrbitPhase;

        switch (auditionMotionTypeIndex)
        {
            case 1:
                orbitHz = 0.08;
                azimuth = static_cast<float> (auditionOrbitPhase * 360.0 - 180.0);
                elevation = static_cast<float> (14.0 * std::sin (phaseRadians * 0.75));
                auditionDistanceMeters = 1.05f + 0.12f * static_cast<float> (0.5 + 0.5 * std::cos (phaseRadians * 0.5));
                break;

            case 2:
                // Orbit Fast is an explicit 3D path, not just a flat azimuth sweep.
                orbitHz = 0.20;
                azimuth = static_cast<float> (auditionOrbitPhase * 360.0 - 180.0);
                elevation = static_cast<float> (60.0 * std::sin (phaseRadians * 1.15));
                auditionDistanceMeters = 0.95f + 0.55f * static_cast<float> (0.5 + 0.5 * std::cos (phaseRadians * 0.8));
                break;
            case 3: // figure8_flow
                orbitHz = 0.13;
                azimuth = 168.0f * static_cast<float> (std::sin (phaseRadians) * std::cos (phaseRadians * 0.5));
                elevation = 30.0f * static_cast<float> (std::sin (phaseRadians * 2.0));
                auditionDistanceMeters = 0.85f + 0.92f * static_cast<float> (0.5 + 0.5 * std::cos (phaseRadians * 1.1));
                break;
            case 4: // helix_rise
                orbitHz = 0.17;
                azimuth = static_cast<float> (auditionOrbitPhase * 360.0 - 180.0);
                elevation = -46.0f + 92.0f * static_cast<float> (0.5 + 0.5 * std::sin (phaseRadians * 0.45));
                auditionDistanceMeters = 0.72f + 1.12f * static_cast<float> (0.5 + 0.5 * std::sin (phaseRadians * 1.7 + 0.7));
                break;
            case 5: // wall_ricochet
            {
                const auto sampleRate = juce::jmax (1.0, currentSampleRate);
                const float dt = static_cast<float> (numSamples / sampleRate);
                const float bounds = qualityHigh ? 2.20f : 1.90f;
                auditionWallPosX += auditionWallVelX * dt;
                auditionWallPosZ += auditionWallVelZ * dt;

                bool collision = false;
                auto reflectAxis = [bounds, &collision, this] (float& pos, float& vel) noexcept
                {
                    if (pos > bounds)
                    {
                        pos = bounds - (pos - bounds);
                        vel = -std::abs (vel) * (0.93f + 0.04f * nextAuditionRand01());
                        collision = true;
                    }
                    else if (pos < -bounds)
                    {
                        pos = -bounds + (-bounds - pos);
                        vel = std::abs (vel) * (0.93f + 0.04f * nextAuditionRand01());
                        collision = true;
                    }
                };
                reflectAxis (auditionWallPosX, auditionWallVelX);
                reflectAxis (auditionWallPosZ, auditionWallVelZ);

                if (collision)
                    auditionBounceEnv = juce::jmax (auditionBounceEnv, 0.58f + 0.30f * nextAuditionRand01());

                const float planarDistance = std::sqrt (
                    auditionWallPosX * auditionWallPosX + auditionWallPosZ * auditionWallPosZ);
                azimuth = juce::radiansToDegrees (std::atan2 (auditionWallPosX, -auditionWallPosZ));
                elevation = -22.0f + 56.0f * juce::jlimit (0.0f, 1.0f, auditionBounceEnv)
                    + 12.0f * static_cast<float> (std::sin (phaseRadians * 1.6));
                auditionDistanceMeters = 0.58f + 0.62f * planarDistance;
                break;
            }

            default:
                break;
        }

        if (auditionMotionTypeIndex != 0)
        {
            const auto phase = static_cast<float> (phaseRadians);
            const auto motionTier = (auditionMotionTypeIndex == 1) ? 0.62f
                : (auditionMotionTypeIndex == 2) ? 1.0f
                : (auditionMotionTypeIndex == 3) ? 1.18f
                : (auditionMotionTypeIndex == 4) ? 1.32f
                : 1.50f;
            const auto qualitySpread = qualityHigh ? 1.0f : 0.72f;

            switch (auditionSignalTypeIndex)
            {
                case 3: // rain_sheet
                {
                    const auto sweep = static_cast<float> (std::sin (phase * (0.90f + 0.35f * motionTier)));
                    const auto billow = static_cast<float> (std::sin (phase * (1.85f + 0.20f * motionTier)));
                    azimuth = 158.0f * qualitySpread * sweep + 26.0f * qualitySpread * billow;
                    elevation = -10.0f + 20.0f * qualitySpread
                        * static_cast<float> (std::sin (phase * (1.20f + 0.25f * motionTier)));
                    auditionDistanceMeters = 1.08f + (0.42f + 0.24f * qualitySpread)
                        * static_cast<float> (0.5 + 0.5 * std::cos (phase * (1.35f + 0.25f * motionTier)));
                    break;
                }
                case 4: // snow_cloud
                {
                    const auto cloudA = static_cast<float> (std::sin (phase * (0.34f + 0.12f * motionTier) + 0.6f));
                    const auto cloudB = static_cast<float> (std::sin (phase * (0.58f + 0.07f * motionTier) - 1.1f));
                    const auto drift = 0.55f * cloudA + 0.45f * cloudB;
                    azimuth = 128.0f * qualitySpread * drift;
                    elevation = 16.0f + 30.0f * qualitySpread
                        * (0.45f * cloudB
                           + 0.55f * static_cast<float> (std::sin (phase * (0.42f + 0.09f * motionTier) + 0.9f)));
                    auditionDistanceMeters = 1.28f + (0.42f + 0.20f * qualitySpread)
                        * static_cast<float> (0.5 + 0.5 * std::sin (phase * (0.39f + 0.05f * motionTier) + 0.35f));
                    break;
                }
                case 5: // bounce_cluster
                {
                    if (auditionMotionTypeIndex == 5)
                    {
                        const auto impact = juce::jlimit (0.0f, 1.0f, auditionBounceEnv);
                        elevation = -18.0f + 62.0f * impact
                            + 10.0f * static_cast<float> (std::sin (phase * 2.1f));
                        auditionDistanceMeters += 0.18f * impact;
                        break;
                    }
                    const auto impact = juce::jlimit (0.0f, 1.0f, auditionBounceEnv);
                    const auto cluster = juce::jlimit (0.0f, 1.0f, static_cast<float> (auditionBounceClusterRemaining) / 6.0f);
                    const auto rebound = std::abs (static_cast<float> (std::sin (phase * (1.65f + 0.65f * motionTier))));
                    azimuth = 136.0f * qualitySpread * static_cast<float> (std::sin (phase * (0.95f + 0.45f * motionTier)))
                        + 38.0f * cluster * static_cast<float> (std::sin (phase * (2.60f + 0.35f * motionTier)));
                    elevation = -24.0f + (38.0f * impact + 16.0f * cluster) * rebound;
                    auditionDistanceMeters = 0.96f
                        + 0.72f * (1.0f - impact)
                        + 0.34f * cluster * std::abs (static_cast<float> (std::cos (phase * (1.55f + 0.25f * motionTier))));
                    if (qualityHigh)
                        auditionDistanceMeters += 0.14f * cluster * rebound;
                    break;
                }
                case 6: // chime_constellation
                {
                    const auto chimeA = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseA);
                    const auto chimeB = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseB);
                    const auto shimmer = juce::jlimit (0.0f, 1.0f, auditionChimeShimmer * 2.2f);
                    const auto constellation = static_cast<float> (
                        std::sin (phase * (1.10f + 0.30f * motionTier) + 0.35f * std::sin (chimeA)));
                    azimuth = 138.0f * qualitySpread * constellation
                        + 18.0f * qualitySpread * static_cast<float> (std::sin (chimeB * 0.5f));
                    elevation = 18.0f + 34.0f * qualitySpread * std::abs (static_cast<float> (
                        std::sin (chimeB * 0.45f + phase * (0.60f + 0.18f * motionTier))));
                    auditionDistanceMeters = 0.82f + (0.30f + 0.12f * qualitySpread)
                        * (0.45f + 0.55f * std::abs (static_cast<float> (std::sin (chimeA * 0.5f))));
                    auditionDistanceMeters += 0.12f * shimmer * qualitySpread;
                    break;
                }
                case 7: // crickets
                {
                    const auto chatter = static_cast<float> (std::sin (phase * (1.85f + 0.55f * motionTier)));
                    azimuth = 172.0f * qualitySpread * chatter;
                    elevation = -12.0f + 10.0f * static_cast<float> (std::sin (phase * 2.7f));
                    auditionDistanceMeters = 1.18f + 0.82f * static_cast<float> (
                        0.5f + 0.5f * std::sin (phase * (1.35f + 0.22f * motionTier)));
                    break;
                }
                case 8: // song_birds
                {
                    const auto swirl = static_cast<float> (std::sin (phase * (0.86f + 0.30f * motionTier)));
                    azimuth = 160.0f * qualitySpread * swirl;
                    elevation = 26.0f + 34.0f * qualitySpread * std::abs (static_cast<float> (
                        std::sin (phase * (1.40f + 0.35f * motionTier))));
                    auditionDistanceMeters = 1.05f + 0.96f * static_cast<float> (
                        0.5f + 0.5f * std::cos (phase * (1.05f + 0.18f * motionTier)));
                    break;
                }
                case 9: // karplus_plucks
                {
                    const auto pluckWave = static_cast<float> (std::sin (phase * (1.20f + 0.28f * motionTier)));
                    azimuth = 148.0f * qualitySpread * pluckWave;
                    elevation = -6.0f + 18.0f * static_cast<float> (std::sin (phase * 1.9f));
                    auditionDistanceMeters = 0.92f + 0.84f * static_cast<float> (
                        0.5f + 0.5f * std::cos (phase * (1.25f + 0.24f * motionTier)));
                    break;
                }
                case 10: // membrane_drops
                {
                    const auto throb = std::abs (static_cast<float> (std::sin (phase * (1.45f + 0.35f * motionTier))));
                    azimuth = 164.0f * qualitySpread * static_cast<float> (std::sin (phase * 0.9f));
                    elevation = -18.0f + 32.0f * throb;
                    auditionDistanceMeters = 1.04f + 0.92f * static_cast<float> (
                        0.5f + 0.5f * std::cos (phase * (1.55f + 0.20f * motionTier)));
                    break;
                }
                case 11: // krell_patch
                {
                    const auto glide = static_cast<float> (std::sin (phase * (0.66f + 0.24f * motionTier)));
                    azimuth = 170.0f * qualitySpread * glide;
                    elevation = -4.0f + 40.0f * static_cast<float> (std::sin (phase * 1.25f + 0.6f));
                    auditionDistanceMeters = 0.80f + 1.10f * static_cast<float> (
                        0.5f + 0.5f * std::sin (phase * (1.15f + 0.20f * motionTier) + 0.35f));
                    break;
                }
                case 12: // generative_arp
                {
                    const auto lattice = static_cast<float> (std::sin (phase * (1.45f + 0.34f * motionTier)));
                    azimuth = 158.0f * qualitySpread * lattice;
                    elevation = 4.0f + 28.0f * std::abs (static_cast<float> (
                        std::sin (phase * (2.05f + 0.18f * motionTier))));
                    auditionDistanceMeters = 0.88f + 1.04f * static_cast<float> (
                        0.5f + 0.5f * std::cos (phase * (1.32f + 0.25f * motionTier)));
                    break;
                }
                default:
                    break;
            }

            azimuth = juce::jlimit (-170.0f, 170.0f, azimuth);
            elevation = juce::jlimit (-65.0f, 65.0f, elevation);
            auditionDistanceMeters = juce::jlimit (0.55f, 2.20f, auditionDistanceMeters);
        }

        if (orbitHz > 0.0)
        {
            const auto sampleRate = juce::jmax (1.0, currentSampleRate);
            auditionOrbitPhase += (orbitHz * static_cast<double> (numSamples)) / sampleRate;
            auditionOrbitPhase -= std::floor (auditionOrbitPhase);
        }

        const auto azimuthRadians = juce::degreesToRadians (azimuth);
        const auto elevationRadians = juce::degreesToRadians (elevation);
        const auto cosElevation = std::cos (elevationRadians);
        auditionVisualX.store (std::sin (azimuthRadians) * cosElevation * auditionDistanceMeters, std::memory_order_relaxed);
        auditionVisualY.store (1.2f + std::sin (elevationRadians) * auditionDistanceMeters, std::memory_order_relaxed);
        auditionVisualZ.store (-std::cos (azimuthRadians) * cosElevation * auditionDistanceMeters, std::memory_order_relaxed);

        const auto cloudBoundAvailable = isAuditionCloudBoundModeAvailable();
        const auto requestedVoiceCount = juce::jlimit (1, AUDITION_MAX_VOICES, getAuditionVoiceCountForSignal());
        const auto activeVoices = cloudBoundAvailable ? requestedVoiceCount : 1;
        const auto multiSourceSignal = cloudBoundAvailable && activeVoices > 1;
        const auto spreadDegrees = getAuditionVoiceSpreadDegrees();
        const auto motionSpreadBlend = auditionMotionTypeIndex == 0 ? 1.0f
            : auditionMotionTypeIndex == 1 ? 0.72f
            : auditionMotionTypeIndex == 2 ? 0.62f
            : auditionMotionTypeIndex == 3 ? 0.56f
            : auditionMotionTypeIndex == 4 ? 0.50f
            : 0.42f;
        const auto motionEnergy = auditionMotionTypeIndex == 0 ? 0.0f
            : auditionMotionTypeIndex == 1 ? 0.28f
            : auditionMotionTypeIndex == 2 ? 0.55f
            : auditionMotionTypeIndex == 3 ? 0.72f
            : auditionMotionTypeIndex == 4 ? 0.88f
            : 1.0f;
        const auto physicsVelocityTarget = auditionPhysicsReactiveInputActive ? auditionPhysicsReactiveVelocityTarget : 0.0f;
        const auto physicsCollisionTarget = auditionPhysicsReactiveInputActive ? auditionPhysicsReactiveCollisionTarget : 0.0f;
        const auto physicsDensityTarget = auditionPhysicsReactiveInputActive ? auditionPhysicsReactiveDensityTarget : 0.0f;
        auditionPhysicsReactiveVelocityState += (physicsVelocityTarget - auditionPhysicsReactiveVelocityState) * 0.24f;
        auditionPhysicsReactiveCollisionState += (physicsCollisionTarget - auditionPhysicsReactiveCollisionState) * 0.30f;
        auditionPhysicsReactiveDensityState += (physicsDensityTarget - auditionPhysicsReactiveDensityState) * 0.18f;
        const auto physicsVelocityNorm = juce::jlimit (0.0f, 1.0f, auditionPhysicsReactiveVelocityState);
        const auto physicsCollisionNorm = juce::jlimit (0.0f, 1.0f, auditionPhysicsReactiveCollisionState);
        const auto physicsDensityNorm = juce::jlimit (0.0f, 1.0f, auditionPhysicsReactiveDensityState);
        const auto physicsCouplingNorm = juce::jlimit (
            0.0f,
            1.0f,
            0.44f * physicsVelocityNorm + 0.36f * physicsCollisionNorm + 0.20f * physicsDensityNorm);
        const auto phase = static_cast<float> (phaseRadians);

        std::array<int, AUDITION_MAX_VOICES> voiceDelaySamples {};
        std::array<float, AUDITION_MAX_VOICES> voiceLevelWeights {};
        std::array<double, AUDITION_MAX_VOICES> voiceSquareSum {};
        float voiceWeightSum = 0.0f;

        for (int voice = 0; voice < AUDITION_MAX_VOICES; ++voice)
        {
            if (voice >= activeVoices)
            {
                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                    auditionSmoothedSpeakerGains[static_cast<size_t> (voice)][static_cast<size_t> (spk)].setTargetValue (0.0f);
                continue;
            }

            auto voiceAzimuth = azimuth;
            auto voiceElevation = elevation;
            auto voiceDistanceMeters = auditionDistanceMeters;
            const auto hashA = auditionVoiceHashUnit (voice, 0xA53C9E11u);
            const auto hashB = auditionVoiceHashUnit (voice, 0x3C6EF372u);
            const auto hashC = auditionVoiceHashUnit (voice, 0xBB67AE85u);
            const auto voiceNorm = activeVoices > 1
                ? static_cast<float> (voice) / static_cast<float> (activeVoices - 1)
                : 0.0f;
            const auto ringAzimuth = -180.0f + (360.0f * voiceNorm) + (hashA - 0.5f) * 18.0f;

            if (multiSourceSignal)
            {
                const auto ringRadians = juce::degreesToRadians (ringAzimuth);
                const auto azimuthWobble = spreadDegrees
                    * static_cast<float> (std::sin (phase * (0.65f + 0.22f * hashB)
                                                      + ringRadians * (1.0f + 0.35f * motionEnergy)));
                const auto mixedAzimuth = voiceAzimuth * (1.0f - motionSpreadBlend)
                    + (ringAzimuth + azimuthWobble * (0.28f + 0.42f * motionEnergy)) * motionSpreadBlend;
                voiceAzimuth = wrapAuditionAzimuthDegrees (mixedAzimuth);

                const auto elevationSpread = (auditionSignalTypeIndex == 4 || auditionSignalTypeIndex == 8)
                    ? 44.0f : 30.0f;
                const auto elevationWobble = elevationSpread
                    * static_cast<float> (std::sin (phase * (0.85f + 0.26f * hashC)
                                                      + ringRadians * (0.65f + 0.22f * hashA)));
                voiceElevation = juce::jlimit (
                    -65.0f,
                    65.0f,
                    voiceElevation + (hashB - 0.5f) * elevationSpread * 0.6f
                        + elevationWobble * (0.24f + 0.40f * motionEnergy));

                const auto distanceSpread = 0.26f + 0.40f * hashC;
                const auto distanceWobble = static_cast<float> (std::sin (
                    phase * (0.52f + 0.28f * hashA) + ringRadians * (0.75f + 0.30f * hashB)));
                voiceDistanceMeters = juce::jlimit (
                    0.55f,
                    2.20f,
                    voiceDistanceMeters + distanceSpread * distanceWobble);
            }

            const auto panGains = vbapPanner.calculateGains (voiceAzimuth, voiceElevation);
            const auto distanceGain = distanceAttenuator.calculateGain (voiceDistanceMeters);
            for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
            {
                auditionSmoothedSpeakerGains[static_cast<size_t> (voice)][static_cast<size_t> (spk)].setTargetValue (
                    panGains.gains[static_cast<size_t> (spk)] * distanceGain);
            }

            voiceDelaySamples[static_cast<size_t> (voice)] = getAuditionVoiceDelaySamples (voice, activeVoices);
            voiceLevelWeights[static_cast<size_t> (voice)] = multiSourceSignal
                ? (0.62f + 0.38f * hashB)
                : 1.0f;
            voiceWeightSum += voiceLevelWeights[static_cast<size_t> (voice)];
        }

        if (voiceWeightSum > 0.0f)
        {
            const auto invVoiceWeightSum = 1.0f / voiceWeightSum;
            for (int voice = 0; voice < activeVoices; ++voice)
                voiceLevelWeights[static_cast<size_t> (voice)] *= invVoiceWeightSum;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            auto generated = generateAuditionSignalSample();
            generated = applyAuditionPhysicsReactiveTimbre (
                generated,
                physicsVelocityNorm,
                physicsCollisionNorm,
                physicsDensityNorm,
                motionEnergy);
            tempMonoBuffer[static_cast<size_t> (i)] = generated * signalGain;
        }

        double mixedSquareSum = 0.0;
        double mixedHighSquareSum = 0.0;
        float mixedPeak = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto drySample = tempMonoBuffer[static_cast<size_t> (i)];
            auditionHistoryBuffer[static_cast<size_t> (auditionHistoryWritePos)] = drySample;
            auditionHistoryWritePos = (auditionHistoryWritePos + 1) % AUDITION_HISTORY_BUFFER_SAMPLES;
            float mixedVoiceSample = 0.0f;

            for (int voice = 0; voice < AUDITION_MAX_VOICES; ++voice)
            {
                const auto delayedSample = readAuditionHistoryDelayed (voiceDelaySamples[static_cast<size_t> (voice)]);
                const auto voiceBaseLevel = voiceLevelWeights[static_cast<size_t> (voice)];
                auto& voiceModPhase = auditionVoiceModPhase[static_cast<size_t> (voice)];
                const auto voiceLfoHz = 0.22 + 0.31 * auditionVoiceHashUnit (voice, 0xC2B2AE35u);
                voiceModPhase += voiceLfoHz / juce::jmax (1.0, currentSampleRate);
                voiceModPhase -= std::floor (voiceModPhase);
                const auto modulation = 0.90f + 0.10f * static_cast<float> (
                    std::sin (juce::MathConstants<double>::twoPi * voiceModPhase));
                const auto voiceExcitedSample = (voice < activeVoices && multiSourceSignal)
                    ? renderAuditionVoiceExcitation (voice, activeVoices, delayedSample)
                    : delayedSample;
                const auto voiceSample = voiceExcitedSample * voiceBaseLevel * modulation;
                if (voice < activeVoices)
                {
                    mixedVoiceSample += voiceSample;
                    voiceSquareSum[static_cast<size_t> (voice)] +=
                        static_cast<double> (voiceSample) * static_cast<double> (voiceSample);
                }

                for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                {
                    const auto gain = auditionSmoothedSpeakerGains[static_cast<size_t> (voice)][static_cast<size_t> (spk)].getNextValue();
                    accumBuffer.addSample (spk, i, voiceSample * gain);
                }
            }

            const auto mixedAbs = std::abs (mixedVoiceSample);
            mixedPeak = juce::jmax (mixedPeak, mixedAbs);
            mixedSquareSum += static_cast<double> (mixedVoiceSample) * static_cast<double> (mixedVoiceSample);
            auditionReactiveBrightnessLowpassState += (mixedVoiceSample - auditionReactiveBrightnessLowpassState) * 0.08f;
            const auto highComponent = mixedVoiceSample - auditionReactiveBrightnessLowpassState;
            mixedHighSquareSum += static_cast<double> (highComponent) * static_cast<double> (highComponent);
        }

        const auto invNumSamples = 1.0f / static_cast<float> (numSamples);
        const auto blockRms = juce::jlimit (
            0.0f,
            2.0f,
            std::sqrt (static_cast<float> (mixedSquareSum * static_cast<double> (invNumSamples))));
        const auto blockPeak = juce::jlimit (0.0f, 2.0f, mixedPeak);
        const auto blockHighRms = juce::jlimit (
            0.0f,
            2.0f,
            std::sqrt (static_cast<float> (mixedHighSquareSum * static_cast<double> (invNumSamples))));

        const auto fastAlpha = qualityHigh ? 0.27f : 0.20f;
        const auto slowAlpha = qualityHigh ? 0.08f : 0.06f;
        auditionReactiveEnvFastState += (blockRms - auditionReactiveEnvFastState) * fastAlpha;
        auditionReactiveEnvSlowState += (blockRms - auditionReactiveEnvSlowState) * slowAlpha;
        auditionReactiveEnvFastState = juce::jlimit (0.0f, 2.0f, auditionReactiveEnvFastState);
        auditionReactiveEnvSlowState = juce::jlimit (0.0f, 2.0f, auditionReactiveEnvSlowState);

        auto onset = juce::jlimit (
            0.0f,
            1.0f,
            (auditionReactiveEnvFastState - auditionReactiveEnvSlowState) * 5.0f);
        auto brightness = juce::jlimit (
            0.0f,
            1.0f,
            blockHighRms / juce::jmax (0.001f, blockRms * 1.8f + 0.05f));
        const auto sourceDensityNorm = juce::jlimit (
            0.0f,
            1.0f,
            static_cast<float> (activeVoices) / static_cast<float> (AUDITION_MAX_VOICES));
        const auto coupledDensityNorm = juce::jlimit (
            0.0f,
            1.0f,
            0.70f * sourceDensityNorm + 0.30f * physicsDensityNorm);

        onset = juce::jlimit (
            0.0f,
            1.0f,
            onset + 0.34f * physicsCollisionNorm * (0.40f + 0.60f * physicsVelocityNorm));
        brightness = juce::jlimit (
            0.0f,
            1.0f,
            brightness + 0.28f * physicsVelocityNorm + 0.10f * physicsCollisionNorm);

        auto rainFadeRate = 0.10f
            + 0.45f * auditionReactiveEnvFastState
            + 0.25f * onset
            + 0.10f * brightness
            + 0.10f * motionEnergy
            + 0.16f * physicsVelocityNorm
            + 0.22f * physicsCollisionNorm
            + 0.08f * coupledDensityNorm;
        auto snowFadeRate = 0.12f
            + 0.42f * auditionReactiveEnvSlowState
            + 0.18f * (1.0f - brightness)
            + 0.10f * (1.0f - onset)
            + 0.12f * coupledDensityNorm
            + 0.16f * physicsDensityNorm
            + 0.08f * (1.0f - physicsVelocityNorm)
            + 0.08f * physicsCollisionNorm;

        if (auditionSignalTypeIndex == 3) // rain
        {
            rainFadeRate += 0.20f;
            snowFadeRate *= 0.74f;
        }
        else if (auditionSignalTypeIndex == 4) // snow
        {
            snowFadeRate += 0.20f;
            rainFadeRate *= 0.78f;
        }

        rainFadeRate = juce::jlimit (0.0f, 1.0f, rainFadeRate);
        snowFadeRate = juce::jlimit (0.0f, 1.0f, snowFadeRate);

        std::array<float, AUDITION_MAX_VOICES> sourceEnergy {};
        float maxVoiceRms = 0.0f;
        for (int voice = 0; voice < activeVoices; ++voice)
        {
            sourceEnergy[static_cast<size_t> (voice)] = juce::jlimit (
                0.0f,
                2.0f,
                std::sqrt (static_cast<float> (voiceSquareSum[static_cast<size_t> (voice)] * static_cast<double> (invNumSamples))));
            sourceEnergy[static_cast<size_t> (voice)] = juce::jlimit (
                0.0f,
                2.0f,
                sourceEnergy[static_cast<size_t> (voice)] * (0.88f + 0.24f * physicsCouplingNorm));
            maxVoiceRms = juce::jmax (maxVoiceRms, sourceEnergy[static_cast<size_t> (voice)]);
        }

        if (maxVoiceRms > 1.0e-6f)
        {
            const auto invMaxVoice = 1.0f / maxVoiceRms;
            for (int voice = 0; voice < activeVoices; ++voice)
            {
                sourceEnergy[static_cast<size_t> (voice)] = juce::jlimit (
                    0.0f,
                    1.0f,
                    sourceEnergy[static_cast<size_t> (voice)] * invMaxVoice);
            }
        }

        publishAuditionReactiveTelemetry (
            blockRms,
            blockPeak,
            auditionReactiveEnvFastState,
            auditionReactiveEnvSlowState,
            onset,
            brightness,
            rainFadeRate,
            snowFadeRate,
            physicsVelocityNorm,
            physicsCollisionNorm,
            coupledDensityNorm,
            physicsCouplingNorm,
            0.0f,
            0.0f,
            1.0f,
            static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None),
            sourceEnergy,
            activeVoices);
    }

    static bool isSteamAudioBackendCompiled() noexcept
    {
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        return true;
#else
        return false;
#endif
    }

    bool isSteamAudioBackendAvailable() const noexcept
    {
        return isSteamAudioBackendCompiled() && steamAudioRuntimeReady;
    }

    void setSteamInitStage (SteamInitStage stage, int errorCode) noexcept
    {
        steamInitErrorCode.store (errorCode, std::memory_order_relaxed);
        steamInitStageIndex.store (static_cast<int> (stage), std::memory_order_relaxed);
    }

    void clearSteamInitDiagnosticsStrings()
    {
        const juce::SpinLock::ScopedLockType diagnosticsLock (steamDiagnosticsLock);
        steamRuntimeLibraryPath.clear();
        steamMissingSymbolName.clear();
    }

    void setSteamRuntimeLibraryPathForDiagnostics (const juce::String& libraryPath)
    {
        const juce::SpinLock::ScopedLockType diagnosticsLock (steamDiagnosticsLock);
        steamRuntimeLibraryPath = libraryPath;
    }

    void setSteamMissingSymbolForDiagnostics (const juce::String& symbolName)
    {
        const juce::SpinLock::ScopedLockType diagnosticsLock (steamDiagnosticsLock);
        steamMissingSymbolName = symbolName;
    }

    void initialiseSteamAudioRuntimeIfEnabled()
    {
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        steamAudioRuntimeReady = false;
        steamAudioAvailable.store (false, std::memory_order_relaxed);
        clearSteamInitDiagnosticsStrings();
        setSteamInitStage (SteamInitStage::LoadingLibrary, 0);

        juce::String runtimePath;
        if (const auto* envPath = std::getenv ("LOCUSQ_STEAM_AUDIO_LIB"))
        {
            const auto candidate = juce::String (envPath).trim();
            if (candidate.isNotEmpty())
                runtimePath = candidate;
        }

       #if defined (LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH)
        if (runtimePath.isEmpty())
            runtimePath = juce::String (LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH).trim();
       #endif

        bool libraryOpened = false;
        juce::String loadedLibraryPath;
        juce::String attemptedLibraryPath;
        if (runtimePath.isNotEmpty())
        {
            attemptedLibraryPath = runtimePath;
            libraryOpened = steamAudioLibrary.open (runtimePath);
            if (libraryOpened)
                loadedLibraryPath = runtimePath;
        }

        if (! libraryOpened)
        {
           #if JUCE_MAC
            const juce::String fallbackLibraryName { "libphonon.dylib" };
           #elif JUCE_WINDOWS
            const juce::String fallbackLibraryName { "phonon.dll" };
           #else
            const juce::String fallbackLibraryName { "libphonon.so" };
           #endif

            attemptedLibraryPath = attemptedLibraryPath.isNotEmpty()
                                       ? attemptedLibraryPath + ";" + fallbackLibraryName
                                       : fallbackLibraryName;
            libraryOpened = steamAudioLibrary.open (fallbackLibraryName);
            if (libraryOpened)
                loadedLibraryPath = fallbackLibraryName;
        }

        if (! libraryOpened || steamAudioLibrary.getNativeHandle() == nullptr)
        {
            setSteamRuntimeLibraryPathForDiagnostics (attemptedLibraryPath);
            setSteamInitStage (SteamInitStage::LibraryOpenFailed, 0);
            steamAudioAvailable.store (false, std::memory_order_relaxed);
            return;
        }

        setSteamRuntimeLibraryPathForDiagnostics (loadedLibraryPath);
        setSteamInitStage (SteamInitStage::ResolvingSymbols, 0);

        iplContextCreateFn = reinterpret_cast<IplContextCreateFn> (steamAudioLibrary.getFunction ("iplContextCreate"));
        if (iplContextCreateFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplContextCreate");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        iplContextReleaseFn = reinterpret_cast<IplContextReleaseFn> (steamAudioLibrary.getFunction ("iplContextRelease"));
        if (iplContextReleaseFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplContextRelease");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        iplHRTFCreateFn = reinterpret_cast<IplHRTFCreateFn> (steamAudioLibrary.getFunction ("iplHRTFCreate"));
        if (iplHRTFCreateFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplHRTFCreate");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        iplHRTFReleaseFn = reinterpret_cast<IplHRTFReleaseFn> (steamAudioLibrary.getFunction ("iplHRTFRelease"));
        if (iplHRTFReleaseFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplHRTFRelease");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        iplVirtualSurroundEffectCreateFn = reinterpret_cast<IplVirtualSurroundEffectCreateFn> (steamAudioLibrary.getFunction ("iplVirtualSurroundEffectCreate"));
        if (iplVirtualSurroundEffectCreateFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplVirtualSurroundEffectCreate");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        iplVirtualSurroundEffectReleaseFn = reinterpret_cast<IplVirtualSurroundEffectReleaseFn> (steamAudioLibrary.getFunction ("iplVirtualSurroundEffectRelease"));
        if (iplVirtualSurroundEffectReleaseFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplVirtualSurroundEffectRelease");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        iplVirtualSurroundEffectResetFn = reinterpret_cast<IplVirtualSurroundEffectResetFn> (steamAudioLibrary.getFunction ("iplVirtualSurroundEffectReset"));
        if (iplVirtualSurroundEffectResetFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplVirtualSurroundEffectReset");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        iplVirtualSurroundEffectApplyFn = reinterpret_cast<IplVirtualSurroundEffectApplyFn> (steamAudioLibrary.getFunction ("iplVirtualSurroundEffectApply"));
        if (iplVirtualSurroundEffectApplyFn == nullptr)
        {
            setSteamMissingSymbolForDiagnostics ("iplVirtualSurroundEffectApply");
            setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
            teardownSteamAudioRuntime();
            return;
        }

        IPLContextSettings contextSettings {};
        contextSettings.version = STEAMAUDIO_VERSION;

        setSteamInitStage (SteamInitStage::CreatingContext, 0);
        const auto contextStatus = iplContextCreateFn (&contextSettings, &steamContext);
        if (contextStatus != IPL_STATUS_SUCCESS || steamContext == nullptr)
        {
            setSteamInitStage (SteamInitStage::ContextCreateFailed, static_cast<int> (contextStatus));
            teardownSteamAudioRuntime();
            return;
        }

        IPLAudioSettings audioSettings {};
        audioSettings.samplingRate = juce::jmax (1, static_cast<IPLint32> (std::lround (currentSampleRate)));
        audioSettings.frameSize = juce::jmax (1, static_cast<IPLint32> (currentBlockSize));

        IPLHRTFSettings hrtfSettings {};
        hrtfSettings.type = IPL_HRTFTYPE_DEFAULT;
        hrtfSettings.volume = 1.0f;
        hrtfSettings.normType = IPL_HRTFNORMTYPE_RMS;

        setSteamInitStage (SteamInitStage::CreatingHRTF, 0);
        const auto hrtfStatus = iplHRTFCreateFn (steamContext, &audioSettings, &hrtfSettings, &steamHrtf);
        if (hrtfStatus != IPL_STATUS_SUCCESS || steamHrtf == nullptr)
        {
            setSteamInitStage (SteamInitStage::HRTFCreateFailed, static_cast<int> (hrtfStatus));
            teardownSteamAudioRuntime();
            return;
        }

        IPLVirtualSurroundEffectSettings effectSettings {};
        effectSettings.speakerLayout.type = IPL_SPEAKERLAYOUTTYPE_QUADRAPHONIC;
        effectSettings.speakerLayout.numSpeakers = 0;
        effectSettings.speakerLayout.speakers = nullptr;
        effectSettings.hrtf = steamHrtf;

        setSteamInitStage (SteamInitStage::CreatingVirtualSurround, 0);
        const auto virtualSurroundStatus = iplVirtualSurroundEffectCreateFn (steamContext, &audioSettings, &effectSettings, &steamVirtualSurroundEffect);
        if (virtualSurroundStatus != IPL_STATUS_SUCCESS
            || steamVirtualSurroundEffect == nullptr)
        {
            setSteamInitStage (SteamInitStage::VirtualSurroundCreateFailed, static_cast<int> (virtualSurroundStatus));
            teardownSteamAudioRuntime();
            return;
        }

        steamAudioRuntimeReady = true;
        setSteamInitStage (SteamInitStage::Ready, 0);
        steamAudioAvailable.store (true, std::memory_order_relaxed);
#else
        steamAudioRuntimeReady = false;
        setSteamInitStage (SteamInitStage::NotCompiled, 0);
        steamAudioAvailable.store (false, std::memory_order_relaxed);
#endif
    }

    void teardownSteamAudioRuntime() noexcept
    {
        steamAudioRuntimeReady = false;

#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        if (steamVirtualSurroundEffect != nullptr && iplVirtualSurroundEffectReleaseFn != nullptr)
            iplVirtualSurroundEffectReleaseFn (&steamVirtualSurroundEffect);
        steamVirtualSurroundEffect = nullptr;

        if (steamHrtf != nullptr && iplHRTFReleaseFn != nullptr)
            iplHRTFReleaseFn (&steamHrtf);
        steamHrtf = nullptr;

        if (steamContext != nullptr && iplContextReleaseFn != nullptr)
            iplContextReleaseFn (&steamContext);
        steamContext = nullptr;

        steamAudioLibrary.close();

        iplContextCreateFn = nullptr;
        iplContextReleaseFn = nullptr;
        iplHRTFCreateFn = nullptr;
        iplHRTFReleaseFn = nullptr;
        iplVirtualSurroundEffectCreateFn = nullptr;
        iplVirtualSurroundEffectReleaseFn = nullptr;
        iplVirtualSurroundEffectResetFn = nullptr;
        iplVirtualSurroundEffectApplyFn = nullptr;
#endif

        steamAudioAvailable.store (false, std::memory_order_relaxed);
    }

    struct SpatialProfileResolution
    {
        SpatialOutputProfile profile = SpatialOutputProfile::Auto;
        SpatialProfileStage stage = SpatialProfileStage::Direct;
    };

    static bool isStereoOrBinauralProfile (SpatialOutputProfile profile) noexcept
    {
        switch (profile)
        {
            case SpatialOutputProfile::Stereo20:
            case SpatialOutputProfile::Virtual3dStereo:
            case SpatialOutputProfile::AmbisonicFOA:
            case SpatialOutputProfile::AmbisonicHOA:
                return true;
            default:
                break;
        }

        return false;
    }

    SpatialProfileResolution resolveSpatialProfileForHost (int numOutputChannels) const noexcept
    {
        const auto requested = static_cast<SpatialOutputProfile> (
            juce::jlimit (0, 11, requestedSpatialProfileIndex.load (std::memory_order_relaxed)));

        if (requested == SpatialOutputProfile::Auto)
        {
            if (numOutputChannels >= 13)
                return { SpatialOutputProfile::Surround742, SpatialProfileStage::Direct };
            if (numOutputChannels >= 10)
                return { SpatialOutputProfile::Surround721, SpatialProfileStage::Direct };
            if (numOutputChannels >= 8)
                return { SpatialOutputProfile::Surround521, SpatialProfileStage::Direct };
            if (numOutputChannels >= NUM_SPEAKERS)
                return { SpatialOutputProfile::Quad40, SpatialProfileStage::Direct };
            return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };
        }

        switch (requested)
        {
            case SpatialOutputProfile::Surround742:
                if (numOutputChannels >= 13)
                    return { requested, SpatialProfileStage::Direct };
                if (numOutputChannels >= NUM_SPEAKERS)
                    return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
                return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

            case SpatialOutputProfile::Surround721:
            case SpatialOutputProfile::AtmosBed:
                if (numOutputChannels >= 10)
                    return { requested == SpatialOutputProfile::AtmosBed ? SpatialOutputProfile::AtmosBed
                                                                         : SpatialOutputProfile::Surround721,
                             SpatialProfileStage::Direct };
                if (numOutputChannels >= NUM_SPEAKERS)
                    return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
                return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

            case SpatialOutputProfile::Surround521:
                if (numOutputChannels >= 8)
                    return { requested, SpatialProfileStage::Direct };
                if (numOutputChannels >= NUM_SPEAKERS)
                    return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
                return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

            case SpatialOutputProfile::CodecIAMF:
            case SpatialOutputProfile::CodecADM:
                if (numOutputChannels >= 13)
                    return { SpatialOutputProfile::Surround742, SpatialProfileStage::CodecLayoutPlaceholder };
                if (numOutputChannels >= NUM_SPEAKERS)
                    return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
                return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

            case SpatialOutputProfile::AmbisonicHOA:
                if (numOutputChannels >= 16)
                    return { requested, SpatialProfileStage::Direct };
                if (numOutputChannels >= 4)
                    return { SpatialOutputProfile::AmbisonicFOA, SpatialProfileStage::FallbackQuad };
                return { SpatialOutputProfile::AmbisonicFOA, SpatialProfileStage::AmbiDecodeStereo };

            case SpatialOutputProfile::AmbisonicFOA:
                if (numOutputChannels >= 4)
                    return { requested, SpatialProfileStage::Direct };
                return { requested, SpatialProfileStage::AmbiDecodeStereo };

            case SpatialOutputProfile::Quad40:
                if (numOutputChannels >= NUM_SPEAKERS)
                    return { requested, SpatialProfileStage::Direct };
                return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

            case SpatialOutputProfile::Stereo20:
            case SpatialOutputProfile::Virtual3dStereo:
                return { requested, SpatialProfileStage::Direct };

            case SpatialOutputProfile::Auto:
            default:
                break;
        }

        return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };
    }

    static void encodeAmbisonicFoaProxyFromQuad (float fl, float fr, float rr, float rl,
                                                 float& w, float& x, float& y, float& z) noexcept
    {
        const float sum = fl + fr + rr + rl;
        w = 0.35355339f * sum; // SN3D-style proxy
        x = 0.5f * ((fr + rr) - (fl + rl));
        y = 0.5f * ((fl + fr) - (rl + rr));
        z = 0.0f; // No elevation energy in quad bed proxy.
    }

    static void decodeAmbisonicFoaProxyToStereo (float w, float x, float y, float z,
                                                 float& left, float& right) noexcept
    {
        left = 0.70710678f * w - 0.50f * x + 0.22f * y + 0.08f * z;
        right = 0.70710678f * w + 0.50f * x + 0.22f * y + 0.08f * z;
    }

    inline void renderVirtual3dStereoSample (int sampleIndex, float& left, float& right) const noexcept
    {
        float fl = 0.0f;
        float fr = 0.0f;
        float rr = 0.0f;
        float rl = 0.0f;
        getHeadPoseAdjustedQuadSample (sampleIndex, fl, fr, rr, rl);
        // Simple crossfeed/Haas-style stereo virtualization from quad bed.
        left = (0.74f * fl) + (0.46f * rl) + (0.12f * fr) + (0.08f * rr);
        right = (0.74f * fr) + (0.46f * rr) + (0.12f * fl) + (0.08f * rl);
    }

    inline void writeSurround521Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
    {
        // Order: L R C LFE1 LFE2 Ls Rs TopC
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);
        const float bed = (fl + fr + rr + rl) * 0.25f;

        outputBuffer.setSample (0, sampleIndex, fl * masterGain);
        outputBuffer.setSample (1, sampleIndex, fr * masterGain);
        outputBuffer.setSample (2, sampleIndex, (fl + fr) * 0.70710678f * masterGain);
        outputBuffer.setSample (3, sampleIndex, bed * 0.35f * masterGain);
        outputBuffer.setSample (4, sampleIndex, bed * 0.35f * masterGain);
        outputBuffer.setSample (5, sampleIndex, rl * masterGain);
        outputBuffer.setSample (6, sampleIndex, rr * masterGain);
        outputBuffer.setSample (7, sampleIndex, bed * 0.8f * masterGain);
    }

    inline void writeSurround721Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
    {
        // Order: L R C LFE1 LFE2 Ls Rs Lrs Rrs TopC
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);
        const float bed = (fl + fr + rr + rl) * 0.25f;
        const float lrs = (0.72f * rl) + (0.28f * fl);
        const float rrs = (0.72f * rr) + (0.28f * fr);

        outputBuffer.setSample (0, sampleIndex, fl * masterGain);
        outputBuffer.setSample (1, sampleIndex, fr * masterGain);
        outputBuffer.setSample (2, sampleIndex, (fl + fr) * 0.70710678f * masterGain);
        outputBuffer.setSample (3, sampleIndex, bed * 0.33f * masterGain);
        outputBuffer.setSample (4, sampleIndex, bed * 0.33f * masterGain);
        outputBuffer.setSample (5, sampleIndex, rl * masterGain);
        outputBuffer.setSample (6, sampleIndex, rr * masterGain);
        outputBuffer.setSample (7, sampleIndex, lrs * masterGain);
        outputBuffer.setSample (8, sampleIndex, rrs * masterGain);
        outputBuffer.setSample (9, sampleIndex, bed * 0.8f * masterGain);
    }

    inline void writeSurround742Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
    {
        // Order: L R C LFE1 LFE2 Ls Rs Lrs Rrs TopFL TopFR TopRL TopRR
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);
        const float bed = (fl + fr + rr + rl) * 0.25f;
        const float lrs = (0.72f * rl) + (0.28f * fl);
        const float rrs = (0.72f * rr) + (0.28f * fr);
        const float topFl = (0.70f * fl) + (0.25f * rl);
        const float topFr = (0.70f * fr) + (0.25f * rr);
        const float topRl = (0.78f * rl) + (0.12f * fl);
        const float topRr = (0.78f * rr) + (0.12f * fr);

        outputBuffer.setSample (0, sampleIndex, fl * masterGain);
        outputBuffer.setSample (1, sampleIndex, fr * masterGain);
        outputBuffer.setSample (2, sampleIndex, (fl + fr) * 0.70710678f * masterGain);
        outputBuffer.setSample (3, sampleIndex, bed * 0.30f * masterGain);
        outputBuffer.setSample (4, sampleIndex, bed * 0.30f * masterGain);
        outputBuffer.setSample (5, sampleIndex, rl * masterGain);
        outputBuffer.setSample (6, sampleIndex, rr * masterGain);
        outputBuffer.setSample (7, sampleIndex, lrs * masterGain);
        outputBuffer.setSample (8, sampleIndex, rrs * masterGain);
        outputBuffer.setSample (9, sampleIndex, topFl * masterGain);
        outputBuffer.setSample (10, sampleIndex, topFr * masterGain);
        outputBuffer.setSample (11, sampleIndex, topRl * masterGain);
        outputBuffer.setSample (12, sampleIndex, topRr * masterGain);
    }

    inline void renderStereoDownmixSample (int sampleIndex, float& left, float& right) const noexcept
    {
        float fl = 0.0f;
        float fr = 0.0f;
        float rr = 0.0f;
        float rl = 0.0f;
        getHeadPoseAdjustedQuadSample (sampleIndex, fl, fr, rr, rl);
        // Legacy headphone path: FL+RL -> Left, FR+RR -> Right.
        left = (fl + rl) * 0.707f;
        right = (fr + rr) * 0.707f;
    }

    void resetHeadphoneCompensationState() noexcept
    {
        headphoneCompLowStateLeft = 0.0f;
        headphoneCompLowStateRight = 0.0f;
    }

    void updateHeadphoneCompensationForProfile (HeadphoneDeviceProfile profile) noexcept
    {
        constexpr float pi = 3.14159265358979323846f;
        const float sampleRate = juce::jmax (1.0f, static_cast<float> (currentSampleRate));
        const float lowCutoffHz = 700.0f;
        headphoneCompLowAlpha = juce::jlimit (1.0e-4f, 1.0f, 1.0f - std::exp (-2.0f * pi * lowCutoffHz / sampleRate));

        switch (profile)
        {
            case HeadphoneDeviceProfile::AirPodsPro2:
                headphoneCompLowGain = 0.98f;
                headphoneCompHighGain = 1.03f;
                headphoneCompCrossfeed = 0.015f;
                break;
            case HeadphoneDeviceProfile::AirPodsPro3:
                headphoneCompLowGain = 0.98f;
                headphoneCompHighGain = 1.03f;
                headphoneCompCrossfeed = 0.015f;
                break;
            case HeadphoneDeviceProfile::SonyWH1000XM5:
                headphoneCompLowGain = 1.04f;
                headphoneCompHighGain = 0.97f;
                headphoneCompCrossfeed = 0.020f;
                break;
            case HeadphoneDeviceProfile::CustomSOFA:
                headphoneCompLowGain = 1.00f;
                headphoneCompHighGain = 1.00f;
                headphoneCompCrossfeed = 0.010f;
                break;
            case HeadphoneDeviceProfile::Generic:
            default:
                headphoneCompLowGain = 1.00f;
                headphoneCompHighGain = 1.00f;
                headphoneCompCrossfeed = 0.0f;
                break;
        }
    }

    inline void applyHeadphoneProfileCompensation (float& left, float& right) noexcept
    {
        if (headphoneCompCrossfeed == 0.0f
            && headphoneCompLowGain == 1.0f
            && headphoneCompHighGain == 1.0f)
        {
            return;
        }

        const float inLeft = left;
        const float inRight = right;
        headphoneCompLowStateLeft += headphoneCompLowAlpha * (inLeft - headphoneCompLowStateLeft);
        headphoneCompLowStateRight += headphoneCompLowAlpha * (inRight - headphoneCompLowStateRight);

        const float highLeft = inLeft - headphoneCompLowStateLeft;
        const float highRight = inRight - headphoneCompLowStateRight;
        const float eqLeft = (headphoneCompLowStateLeft * headphoneCompLowGain)
                             + (highLeft * headphoneCompHighGain);
        const float eqRight = (headphoneCompLowStateRight * headphoneCompLowGain)
                              + (highRight * headphoneCompHighGain);

        left = eqLeft + (inRight * headphoneCompCrossfeed);
        right = eqRight + (inLeft * headphoneCompCrossfeed);

        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;
    }

    bool renderSteamBinauralBlock (int numSamples) noexcept
    {
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
        if (! steamAudioRuntimeReady
            || steamVirtualSurroundEffect == nullptr
            || iplVirtualSurroundEffectApplyFn == nullptr
            || numSamples <= 0
            || numSamples > currentBlockSize
            || static_cast<int> (steamBinauralLeft.size()) < numSamples
            || static_cast<int> (steamBinauralRight.size()) < numSamples)
        {
            return false;
        }

        std::fill (steamBinauralLeft.begin(), steamBinauralLeft.begin() + numSamples, 0.0f);
        std::fill (steamBinauralRight.begin(), steamBinauralRight.begin() + numSamples, 0.0f);

        const bool canUseHeadPoseRotation = headPoseInternalBinauralActive
                                            && headPoseValid
                                            && static_cast<int> (headPoseRotatedQuadScratch[0].size()) >= numSamples
                                            && static_cast<int> (headPoseRotatedQuadScratch[1].size()) >= numSamples
                                            && static_cast<int> (headPoseRotatedQuadScratch[2].size()) >= numSamples
                                            && static_cast<int> (headPoseRotatedQuadScratch[3].size()) >= numSamples;

        if (canUseHeadPoseRotation)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float sourceFl = accumBuffer.getSample (0, i);
                const float sourceFr = accumBuffer.getSample (1, i);
                const float sourceRr = accumBuffer.getSample (2, i);
                const float sourceRl = accumBuffer.getSample (3, i);

                for (int targetSpeaker = 0; targetSpeaker < NUM_SPEAKERS; ++targetSpeaker)
                {
                    const auto& mix = headPoseSpeakerMix[static_cast<size_t> (targetSpeaker)];
                    headPoseRotatedQuadScratch[static_cast<size_t> (targetSpeaker)][static_cast<size_t> (i)] =
                        (mix[0] * sourceFl)
                        + (mix[1] * sourceFr)
                        + (mix[2] * sourceRr)
                        + (mix[3] * sourceRl);
                }
            }

            // Steam virtual surround expects quad order FL, FR, RL, RR.
            steamInputChannelPtrs[0] = headPoseRotatedQuadScratch[0].data();
            steamInputChannelPtrs[1] = headPoseRotatedQuadScratch[1].data();
            steamInputChannelPtrs[2] = headPoseRotatedQuadScratch[3].data();
            steamInputChannelPtrs[3] = headPoseRotatedQuadScratch[2].data();
        }
        else
        {
            // Steam virtual surround expects quad order FL, FR, RL, RR.
            steamInputChannelPtrs[0] = const_cast<float*> (accumBuffer.getReadPointer (0));
            steamInputChannelPtrs[1] = const_cast<float*> (accumBuffer.getReadPointer (1));
            steamInputChannelPtrs[2] = const_cast<float*> (accumBuffer.getReadPointer (3));
            steamInputChannelPtrs[3] = const_cast<float*> (accumBuffer.getReadPointer (2));
        }

        steamOutputChannelPtrs[0] = steamBinauralLeft.data();
        steamOutputChannelPtrs[1] = steamBinauralRight.data();

        IPLAudioBuffer inputBuffer {};
        inputBuffer.numChannels = NUM_SPEAKERS;
        inputBuffer.numSamples = numSamples;
        inputBuffer.data = steamInputChannelPtrs.data();

        IPLAudioBuffer outputBuffer {};
        outputBuffer.numChannels = 2;
        outputBuffer.numSamples = numSamples;
        outputBuffer.data = steamOutputChannelPtrs.data();

        IPLVirtualSurroundEffectParams effectParams {};
        effectParams.hrtf = steamHrtf;

        iplVirtualSurroundEffectApplyFn (steamVirtualSurroundEffect, &effectParams, &inputBuffer, &outputBuffer);
        return true;
#else
        juce::ignoreUnused (numSamples);
        return false;
#endif
    }

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
