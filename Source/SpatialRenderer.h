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
#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
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
    enum class HeadphoneRenderMode : int
    {
        StereoDownmix = 0,
        SteamBinaural = 1
    };
    enum class HeadphoneDeviceProfile : int
    {
        Generic = 0,
        AirPodsPro2 = 1,
        SonyWH1000XM5 = 2,
        CustomSOFA = 3
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
        resetHeadphoneCompensationState();
        updateHeadphoneCompensationForProfile (HeadphoneDeviceProfile::Generic);
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
        resetHeadphoneCompensationState();

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
        const auto clamped = juce::jlimit (0, 3, profileIndex);
        if (requestedHeadphoneProfileIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedHeadphoneProfileIndex.store (clamped, std::memory_order_relaxed);
    }

    void setSpatialOutputProfile (int profileIndex)
    {
        const auto clamped = juce::jlimit (0, 11, profileIndex);
        if (requestedSpatialProfileIndex.load (std::memory_order_relaxed) == clamped)
            return;

        requestedSpatialProfileIndex.store (clamped, std::memory_order_relaxed);
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

    static const char* headphoneDeviceProfileToString (int profileIndex) noexcept
    {
        switch (juce::jlimit (0, 3, profileIndex))
        {
            case static_cast<int> (HeadphoneDeviceProfile::AirPodsPro2): return "airpods_pro_2";
            case static_cast<int> (HeadphoneDeviceProfile::SonyWH1000XM5): return "sony_wh1000xm5";
            case static_cast<int> (HeadphoneDeviceProfile::CustomSOFA): return "custom_sofa";
            case static_cast<int> (HeadphoneDeviceProfile::Generic):
            default: break;
        }

        return "generic";
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
            juce::jlimit (0, 3, requestedHeadphoneProfileIndex.load (std::memory_order_relaxed)));
        const auto steamBackendAvailable = isSteamAudioBackendAvailable();
        const bool profileAllowsHeadphoneRender = isStereoOrBinauralProfile (activeSpatialProfile)
                                                  || numOutputChannels <= 2;
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

        const bool steamRenderedThisBlock = (profileAllowsHeadphoneRender
                                             && numOutputChannels >= 2
                                             && activeHeadphoneMode == HeadphoneRenderMode::SteamBinaural
                                             && renderSteamBinauralBlock (numSamples));

        if (activeHeadphoneMode == HeadphoneRenderMode::SteamBinaural && ! steamRenderedThisBlock)
            activeHeadphoneMode = HeadphoneRenderMode::StereoDownmix;

        activeHeadphoneModeIndex.store (static_cast<int> (activeHeadphoneMode), std::memory_order_relaxed);
        activeHeadphoneProfileIndex.store (activeHeadphoneProfileIndexValue, std::memory_order_relaxed);
        steamAudioAvailable.store (steamBackendAvailable, std::memory_order_relaxed);

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

                if (steamRenderedThisBlock && activeHeadphoneMode == HeadphoneRenderMode::SteamBinaural)
                {
                    left = steamBinauralLeft[static_cast<size_t> (i)];
                    right = steamBinauralRight[static_cast<size_t> (i)];
                }
                else if (activeSpatialProfile == SpatialOutputProfile::Virtual3dStereo)
                {
                    renderVirtual3dStereoSample (i, left, right);
                }
                else if (activeSpatialProfile == SpatialOutputProfile::AmbisonicFOA
                         || activeSpatialProfile == SpatialOutputProfile::AmbisonicHOA)
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
                    decodeAmbisonicFoaProxyToStereo (w, x, y, z, left, right);
                }
                else
                {
                    renderStereoDownmixSample (i, left, right);
                }

                applyHeadphoneProfileCompensation (left, right);
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
    std::atomic<int> requestedHeadphoneModeIndex { static_cast<int> (HeadphoneRenderMode::StereoDownmix) };
    std::atomic<int> activeHeadphoneModeIndex { static_cast<int> (HeadphoneRenderMode::StereoDownmix) };
    std::atomic<int> requestedHeadphoneProfileIndex { static_cast<int> (HeadphoneDeviceProfile::Generic) };
    std::atomic<int> activeHeadphoneProfileIndex { static_cast<int> (HeadphoneDeviceProfile::Generic) };
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
    float headphoneCompLowAlpha = 0.0f;
    float headphoneCompLowGain = 1.0f;
    float headphoneCompHighGain = 1.0f;
    float headphoneCompCrossfeed = 0.0f;
    float headphoneCompLowStateLeft = 0.0f;
    float headphoneCompLowStateRight = 0.0f;
    int lastAppliedHeadphoneProfileIndex = -1;

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

    //==========================================================================
    // Coordinate helpers
    //==========================================================================

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
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);
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
        // Legacy headphone path: FL+RL -> Left, FR+RR -> Right.
        left = (accumBuffer.getSample (0, sampleIndex) + accumBuffer.getSample (3, sampleIndex)) * 0.707f;
        right = (accumBuffer.getSample (1, sampleIndex) + accumBuffer.getSample (2, sampleIndex)) * 0.707f;
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

        // Steam virtual surround expects quad order FL, FR, RL, RR.
        steamInputChannelPtrs[0] = const_cast<float*> (accumBuffer.getReadPointer (0));
        steamInputChannelPtrs[1] = const_cast<float*> (accumBuffer.getReadPointer (1));
        steamInputChannelPtrs[2] = const_cast<float*> (accumBuffer.getReadPointer (3));
        steamInputChannelPtrs[3] = const_cast<float*> (accumBuffer.getReadPointer (2));
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
