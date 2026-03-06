#include "../SpatialRenderer.h"

bool SpatialRenderer::isSteamAudioAvailable() const noexcept
{
    return steamAudioAvailable.load (std::memory_order_relaxed);
}

bool SpatialRenderer::isSteamAudioCompiled() const noexcept
{
    return locusq::spatial_steam_backend::isSteamAudioBackendCompiled();
}

int SpatialRenderer::getSteamAudioInitStageIndex() const noexcept
{
    return steamInitStageIndex.load (std::memory_order_relaxed);
}

int SpatialRenderer::getSteamAudioInitErrorCode() const noexcept
{
    return steamInitErrorCode.load (std::memory_order_relaxed);
}

juce::String SpatialRenderer::getSteamAudioRuntimeLibraryPath() const
{
    const juce::SpinLock::ScopedLockType diagnosticsLock (steamDiagnosticsLock);
    return steamRuntimeLibraryPath;
}

juce::String SpatialRenderer::getSteamAudioMissingSymbolName() const
{
    const juce::SpinLock::ScopedLockType diagnosticsLock (steamDiagnosticsLock);
    return steamMissingSymbolName;
}

const char* SpatialRenderer::steamAudioInitStageToString (int stageIndex) noexcept
{
    return locusq::spatial_steam_backend::steamInitStageToString (stageIndex);
}

bool SpatialRenderer::renderVirtualSurroundForMonitoring (const float* const* quadChannels,
                                                          float* outL,
                                                          float* outR,
                                                          int numSamples,
                                                          const IPLCoordinateSpace3* listenerOrientation) noexcept
{
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
    if (! steamAudioRuntimeReady
        || steamVirtualSurroundEffect == nullptr
        || iplVirtualSurroundEffectApplyFn == nullptr
        || quadChannels == nullptr
        || outL == nullptr
        || outR == nullptr
        || numSamples <= 0
        || numSamples > currentBlockSize)
    {
        return false;
    }

    if (quadChannels[0] == nullptr
        || quadChannels[1] == nullptr
        || quadChannels[2] == nullptr
        || quadChannels[3] == nullptr)
    {
        return false;
    }

    if (static_cast<int> (monitoringHeadPoseRotatedQuadScratch_[0].size()) < numSamples
        || static_cast<int> (monitoringHeadPoseRotatedQuadScratch_[1].size()) < numSamples
        || static_cast<int> (monitoringHeadPoseRotatedQuadScratch_[2].size()) < numSamples
        || static_cast<int> (monitoringHeadPoseRotatedQuadScratch_[3].size()) < numSamples)
    {
        return false;
    }

    // Avoid in-place aliasing between quad input pointers and output L/R when
    // monitoring is applied to the same host buffer.
    std::copy_n (quadChannels[0], numSamples, monitoringHeadPoseRotatedQuadScratch_[0].data());
    std::copy_n (quadChannels[1], numSamples, monitoringHeadPoseRotatedQuadScratch_[1].data());
    std::copy_n (quadChannels[2], numSamples, monitoringHeadPoseRotatedQuadScratch_[2].data());
    std::copy_n (quadChannels[3], numSamples, monitoringHeadPoseRotatedQuadScratch_[3].data());

    if (listenerOrientation != nullptr)
    {
        ListenerOrientation monitoringOrientation {};
        if (tryBuildListenerOrientationFromCoordinateSpace (*listenerOrientation, monitoringOrientation))
        {
            std::array<std::array<float, NUM_SPEAKERS>, NUM_SPEAKERS> monitoringSpeakerMix {};
            locusq::spatial_headphone_pose::buildSpeakerMixFromOrientation (monitoringOrientation,
                                                                            monitoringSpeakerMix);

            // Host quad order in this monitoring path is FL, FR, RL, RR.
            // buildSpeakerMixFromOrientation() expects/source-indexes FL, FR, RR, RL,
            // so source channel mapping is adapted per-sample below.
            auto* rotatedFl = monitoringHeadPoseRotatedQuadScratch_[0].data();
            auto* rotatedFr = monitoringHeadPoseRotatedQuadScratch_[1].data();
            auto* rotatedRl = monitoringHeadPoseRotatedQuadScratch_[2].data();
            auto* rotatedRr = monitoringHeadPoseRotatedQuadScratch_[3].data();

            for (int i = 0; i < numSamples; ++i)
            {
                const float sourceFl = rotatedFl[i];
                const float sourceFr = rotatedFr[i];
                const float sourceRl = rotatedRl[i];
                const float sourceRr = rotatedRr[i];

                const float targetFl = (monitoringSpeakerMix[0][0] * sourceFl)
                                       + (monitoringSpeakerMix[0][1] * sourceFr)
                                       + (monitoringSpeakerMix[0][2] * sourceRr)
                                       + (monitoringSpeakerMix[0][3] * sourceRl);
                const float targetFr = (monitoringSpeakerMix[1][0] * sourceFl)
                                       + (monitoringSpeakerMix[1][1] * sourceFr)
                                       + (monitoringSpeakerMix[1][2] * sourceRr)
                                       + (monitoringSpeakerMix[1][3] * sourceRl);
                const float targetRr = (monitoringSpeakerMix[2][0] * sourceFl)
                                       + (monitoringSpeakerMix[2][1] * sourceFr)
                                       + (monitoringSpeakerMix[2][2] * sourceRr)
                                       + (monitoringSpeakerMix[2][3] * sourceRl);
                const float targetRl = (monitoringSpeakerMix[3][0] * sourceFl)
                                       + (monitoringSpeakerMix[3][1] * sourceFr)
                                       + (monitoringSpeakerMix[3][2] * sourceRr)
                                       + (monitoringSpeakerMix[3][3] * sourceRl);

                rotatedFl[i] = targetFl;
                rotatedFr[i] = targetFr;
                rotatedRl[i] = targetRl;
                rotatedRr[i] = targetRr;
            }
        }
    }

    monitoringInputPtrs_[0] = monitoringHeadPoseRotatedQuadScratch_[0].data();
    monitoringInputPtrs_[1] = monitoringHeadPoseRotatedQuadScratch_[1].data();
    monitoringInputPtrs_[2] = monitoringHeadPoseRotatedQuadScratch_[2].data();
    monitoringInputPtrs_[3] = monitoringHeadPoseRotatedQuadScratch_[3].data();

    monitoringOutputPtrs_[0] = outL;
    monitoringOutputPtrs_[1] = outR;

    IPLAudioBuffer inputBuffer {};
    inputBuffer.numChannels = NUM_SPEAKERS;
    inputBuffer.numSamples = numSamples;
    inputBuffer.data = monitoringInputPtrs_.data();

    IPLAudioBuffer outputBuffer {};
    outputBuffer.numChannels = 2;
    outputBuffer.numSamples = numSamples;
    outputBuffer.data = monitoringOutputPtrs_.data();

    IPLVirtualSurroundEffectParams effectParams {};
    effectParams.hrtf = steamHrtf;

    iplVirtualSurroundEffectApplyFn (steamVirtualSurroundEffect,
                                     &effectParams,
                                     &inputBuffer,
                                     &outputBuffer);
    return true;
#else
    juce::ignoreUnused (quadChannels, outL, outR, numSamples, listenerOrientation);
    return false;
#endif
}

bool SpatialRenderer::isSteamAudioBackendAvailable() const noexcept
{
    return locusq::spatial_steam_backend::isSteamAudioBackendCompiled() && steamAudioRuntimeReady;
}

void SpatialRenderer::setSteamInitStage (SpatialRenderer::SteamInitStage stage, int errorCode) noexcept
{
    locusq::spatial_steam_backend::setSteamInitStage (steamInitStageIndex,
                                                      steamInitErrorCode,
                                                      stage,
                                                      errorCode);
}

void SpatialRenderer::clearSteamInitDiagnosticsStrings()
{
    locusq::spatial_steam_backend::clearSteamDiagnosticsStrings (steamDiagnosticsLock,
                                                                 steamRuntimeLibraryPath,
                                                                 steamMissingSymbolName);
}

void SpatialRenderer::setSteamRuntimeLibraryPathForDiagnostics (const juce::String& libraryPath)
{
    locusq::spatial_steam_backend::setSteamRuntimeLibraryPathForDiagnostics (steamDiagnosticsLock,
                                                                             steamRuntimeLibraryPath,
                                                                             libraryPath);
}

void SpatialRenderer::setSteamMissingSymbolForDiagnostics (const juce::String& symbolName)
{
    locusq::spatial_steam_backend::setSteamMissingSymbolForDiagnostics (steamDiagnosticsLock,
                                                                        steamMissingSymbolName,
                                                                        symbolName);
}

void SpatialRenderer::initialiseSteamAudioRuntimeIfEnabled()
{
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
    steamAudioRuntimeReady = false;
    steamAudioAvailable.store (false, std::memory_order_relaxed);
    clearSteamInitDiagnosticsStrings();
    setSteamInitStage (SteamInitStage::LoadingLibrary, 0);

    juce::StringArray runtimeCandidates;

    // Prefer bundle-local runtime locations first to avoid host permission
    // issues when the repo lives under user-protected directories.
    const auto executableFile = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    const auto executableDir = executableFile.getParentDirectory();
    locusq::spatial_steam_backend::appendSteamRuntimeCandidates (runtimeCandidates, executableDir);

    juce::String loadedLibraryPath;
    juce::String attemptedLibraryPath;
    const bool libraryOpened = locusq::spatial_steam_backend::tryOpenSteamRuntimeLibrary (
        steamAudioLibrary,
        runtimeCandidates,
        attemptedLibraryPath,
        loadedLibraryPath);

    if (! libraryOpened || steamAudioLibrary.getNativeHandle() == nullptr)
    {
        setSteamRuntimeLibraryPathForDiagnostics (attemptedLibraryPath);
        setSteamInitStage (SteamInitStage::LibraryOpenFailed, 0);
        steamAudioAvailable.store (false, std::memory_order_relaxed);
        return;
    }

    setSteamRuntimeLibraryPathForDiagnostics (loadedLibraryPath);
    setSteamInitStage (SteamInitStage::ResolvingSymbols, 0);

    const auto resolveSymbolOrFail = [this] (auto& fnOut, const char* symbolName) -> bool
    {
        if (locusq::spatial_steam_backend::resolveRequiredSymbol (steamAudioLibrary, symbolName, fnOut))
            return true;

        setSteamMissingSymbolForDiagnostics (symbolName);
        setSteamInitStage (SteamInitStage::SymbolsMissing, 0);
        teardownSteamAudioRuntime();
        return false;
    };

    if (! resolveSymbolOrFail (iplContextCreateFn, "iplContextCreate")) return;
    if (! resolveSymbolOrFail (iplContextReleaseFn, "iplContextRelease")) return;
    if (! resolveSymbolOrFail (iplHRTFCreateFn, "iplHRTFCreate")) return;
    if (! resolveSymbolOrFail (iplHRTFReleaseFn, "iplHRTFRelease")) return;
    if (! resolveSymbolOrFail (iplVirtualSurroundEffectCreateFn, "iplVirtualSurroundEffectCreate")) return;
    if (! resolveSymbolOrFail (iplVirtualSurroundEffectReleaseFn, "iplVirtualSurroundEffectRelease")) return;
    if (! resolveSymbolOrFail (iplVirtualSurroundEffectResetFn, "iplVirtualSurroundEffectReset")) return;
    if (! resolveSymbolOrFail (iplVirtualSurroundEffectApplyFn, "iplVirtualSurroundEffectApply")) return;

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

    // TODO(Task 13): SOFA HRTF swap hook.
    //
    // When hp_hrtf_mode == "sofa" and a sofa_ref path is available (delivered
    // from CalibrationProfile.json via pollCompanionCalibrationProfileFromDisk),
    // replace the DEFAULT HRTF type with a SOFA-backed one:
    //
    //   const auto sofaAbsPath = juce::File (
    //       juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
    //           .getChildFile ("LocusQ/sofa")
    //           .getChildFile (sofaRefRelativePath)).getFullPathName().toStdString();
    //
    //   // Load via locusq::dsp::loadSofaFile() from Source/dsp/SofaHrtfLoader.h
    //   // (include that header only from an isolated .cpp, not here).
    //   // On success (result.valid == true), set:
    //   //   hrtfSettings.type       = IPL_HRTFTYPE_SOFA;
    //   //   hrtfSettings.sofaFileName = sofaAbsPath.c_str();  // phonon.h field
    //   // On failure, fall through to IPL_HRTFTYPE_DEFAULT (current behaviour).
    //
    // Prerequisite: SpatialRenderer needs a member `juce::String pendingSofaRef`
    // populated by the processor when the CalibrationProfile changes, plus a
    // `bool pendingHrtfIsSofa` flag, both written from the message thread and
    // read here on the audio thread under a memory_order_relaxed atomic or
    // equivalent lock strategy consistent with HX-06 RT-safety audit.
    //
    // The full wiring is deferred to a follow-up task because it requires
    // cross-thread state (sofa_ref string) to be communicated safely to the
    // Steam Audio init path which runs on the audio thread.
    // See: Source/dsp/SofaHrtfLoader.h for the loader infrastructure.

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
    const auto virtualSurroundStatus = iplVirtualSurroundEffectCreateFn (steamContext,
                                                                         &audioSettings,
                                                                         &effectSettings,
                                                                         &steamVirtualSurroundEffect);
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

void SpatialRenderer::teardownSteamAudioRuntime() noexcept
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

bool SpatialRenderer::renderSteamBinauralBlock (int numSamples) noexcept
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
