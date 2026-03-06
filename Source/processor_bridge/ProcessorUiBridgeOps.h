#pragma once

// Extracted non-RT UI bridge/calibration/preset orchestration logic from PluginProcessor.cpp.
bool LocusQAudioProcessor::startCalibrationFromUI (const juce::var& options)
{
    const auto snapshotOutputChannels = getSnapshotOutputChannels();
    const auto layoutOutputChannels = static_cast<int> (getBusesLayout().getMainOutputChannelSet().size());
    const auto initialRouting = getCurrentCalibrationSpeakerRouting();
    const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
        snapshotOutputChannels,
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        initialRouting);

    applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, false);

    if (getCurrentMode() != LocusQMode::Calibrate)
    {
        const juce::String message { "Calibration start rejected: mode is not CALIBRATE." };
        calibrationEngine.recordExternalStartFailure ("mode_mismatch", message);
        DBG ("LocusQ: " << message);
        return false;
    }

    const auto state = calibrationEngine.getState();
    if (state == CalibrationEngine::State::Playing
        || state == CalibrationEngine::State::Recording
        || state == CalibrationEngine::State::Analyzing)
    {
        const juce::String message { "Calibration start rejected: calibration engine is already running." };
        calibrationEngine.recordExternalStartFailure ("engine_busy", message);
        DBG ("LocusQ: " << message);
        return false;
    }

    if (state == CalibrationEngine::State::Complete
        || state == CalibrationEngine::State::Error)
    {
        calibrationEngine.abortCalibration();
    }

    int testTypeIndex = static_cast<int> (apvts.getRawParameterValue ("cal_test_type")->load());
    float levelDb     = apvts.getRawParameterValue ("cal_test_level")->load();
    float sweepSecs   = 3.0f;
    float tailSecs    = 1.5f;
    int micChannel    = static_cast<int> (apvts.getRawParameterValue ("cal_mic_channel")->load()) - 1;
    int topologyProfile = getCurrentCalibrationTopologyProfileIndex();
    int monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    int deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    bool allowLimitedMapping = false;
    int speakerCh[4] =
    {
        static_cast<int> (apvts.getRawParameterValue ("cal_spk1_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk2_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk3_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk4_out")->load()) - 1
    };

    if (auto* obj = options.getDynamicObject())
    {
        if (obj->hasProperty ("testType"))
        {
            const auto& value = obj->getProperty ("testType");
            if (value.isString())
                testTypeIndex = toSignalTypeIndex (value.toString());
            else
                testTypeIndex = static_cast<int> (value);
        }

        if (obj->hasProperty ("testLevelDb"))
            levelDb = static_cast<float> (double (obj->getProperty ("testLevelDb")));

        if (obj->hasProperty ("sweepSeconds"))
            sweepSecs = static_cast<float> (double (obj->getProperty ("sweepSeconds")));

        if (obj->hasProperty ("tailSeconds"))
            tailSecs = static_cast<float> (double (obj->getProperty ("tailSeconds")));

        if (obj->hasProperty ("micChannel"))
            micChannel = static_cast<int> (obj->getProperty ("micChannel"));

        if (obj->hasProperty ("topologyProfile"))
        {
            const auto topologyText = normaliseCalibrationTopologyId (obj->getProperty ("topologyProfile").toString());
            const auto topologyIndex = indexOfCaseInsensitive (kCalibrationTopologyIds, topologyText);
            if (topologyIndex >= 0)
                topologyProfile = topologyIndex;
        }

        if (obj->hasProperty ("topologyProfileIndex"))
            topologyProfile = static_cast<int> (obj->getProperty ("topologyProfileIndex"));

        if (obj->hasProperty ("monitoringPath"))
        {
            const auto monitoringText = normaliseCalibrationMonitoringPathId (obj->getProperty ("monitoringPath").toString());
            const auto monitoringIndex = indexOfCaseInsensitive (kCalibrationMonitoringPathIds, monitoringText);
            if (monitoringIndex >= 0)
                monitoringPath = monitoringIndex;
        }

        if (obj->hasProperty ("monitoringPathIndex"))
            monitoringPath = static_cast<int> (obj->getProperty ("monitoringPathIndex"));

        if (obj->hasProperty ("deviceProfile"))
        {
            const auto deviceText = normaliseCalibrationDeviceProfileId (obj->getProperty ("deviceProfile").toString());
            const auto deviceIndex = indexOfCaseInsensitive (kCalibrationDeviceProfileIds, deviceText);
            if (deviceIndex >= 0)
                deviceProfile = deviceIndex;
        }

        if (obj->hasProperty ("deviceProfileIndex"))
            deviceProfile = static_cast<int> (obj->getProperty ("deviceProfileIndex"));

        if (obj->hasProperty ("allowLimitedMapping"))
            allowLimitedMapping = static_cast<bool> (obj->getProperty ("allowLimitedMapping"));

        if (obj->hasProperty ("speakerChannels"))
        {
            const auto channels = obj->getProperty ("speakerChannels");
            if (auto* arr = channels.getArray())
            {
                const auto count = juce::jmin (4, arr->size());
                for (int i = 0; i < count; ++i)
                    speakerCh[i] = static_cast<int> (arr->getReference (i));
            }
        }
    }

    micChannel = juce::jlimit (0, 7, micChannel);
    sweepSecs  = juce::jlimit (0.1f, 30.0f, sweepSecs);
    tailSecs   = juce::jlimit (0.0f, 10.0f, tailSecs);
    topologyProfile = juce::jlimit (0, static_cast<int> (kCalibrationTopologyIds.size()) - 1, topologyProfile);
    monitoringPath = juce::jlimit (0, static_cast<int> (kCalibrationMonitoringPathIds.size()) - 1, monitoringPath);
    deviceProfile = juce::jlimit (0, static_cast<int> (kCalibrationDeviceProfileIds.size()) - 1, deviceProfile);

    for (int& ch : speakerCh)
        ch = juce::jlimit (0, 7, ch);

    const auto requiredChannels = getRequiredCalibrationChannelsForTopologyIndex (topologyProfile);
    const std::array<int, SpatialRenderer::NUM_SPEAKERS> requestedRouting
    {
        speakerCh[0] + 1,
        speakerCh[1] + 1,
        speakerCh[2] + 1,
        speakerCh[3] + 1
    };
    const auto writableChannels = resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        requestedRouting);
    if (requiredChannels > writableChannels && ! allowLimitedMapping)
    {
        const juce::String message = "Calibration start rejected: topology requires "
            + juce::String (requiredChannels)
            + " writable channels but runtime reports "
            + juce::String (writableChannels)
            + ". Enable limited mapping acknowledgement to proceed.";
        calibrationEngine.recordExternalStartFailure ("writable_channel_gate", message);
        DBG ("LocusQ: " << message);
        return false;
    }

    const auto legacySpeakerConfig = legacySpeakerConfigForTopologyIndex (topologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_topology_profile", topologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_monitoring_path", monitoringPath);
    setIntegerParameterValueNotifyingHost ("cal_device_profile", deviceProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", legacySpeakerConfig);

    // Keep renderer diagnostics in sync so CALIBRATE can validate requested vs active
    // headphone/spatial states deterministically.
    const auto headphoneModeIndex = (monitoringPath == 2 || monitoringPath == 3) ? 1 : 0;
    setIntegerParameterValueNotifyingHost ("rend_headphone_mode", headphoneModeIndex);
    setIntegerParameterValueNotifyingHost ("rend_headphone_profile", deviceProfile);

    int rendererSpatialProfileIndex = 0;
    switch (topologyProfile)
    {
        case 0: rendererSpatialProfileIndex = 1; break; // stereo safe
        case 1: rendererSpatialProfileIndex = 1; break; // stereo 2.0
        case 2: rendererSpatialProfileIndex = 2; break; // quad 4.0
        case 3: rendererSpatialProfileIndex = 3; break; // surround 5.2.1
        case 4: rendererSpatialProfileIndex = 4; break; // surround 7.2.1 (7.1)
        case 5: rendererSpatialProfileIndex = 4; break; // surround 7.2.1 (7.1.2 alias target)
        case 6: rendererSpatialProfileIndex = 5; break; // surround 7.4.2
        case 7: rendererSpatialProfileIndex = 9; break; // binaural virtual 3D stereo
        case 8: rendererSpatialProfileIndex = 6; break; // ambisonic FOA
        case 9: rendererSpatialProfileIndex = 7; break; // ambisonic HOA
        case 10: rendererSpatialProfileIndex = 9; break; // downmix target
        default: break;
    }
    setIntegerParameterValueNotifyingHost ("rend_spatial_profile", rendererSpatialProfileIndex);

    if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("cal_mic_channel")))
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (micChannel + 1)));

    const auto started = calibrationEngine.startCalibration (toSignalType (testTypeIndex),
                                                             levelDb,
                                                             sweepSecs,
                                                             tailSecs,
                                                             speakerCh,
                                                             micChannel);

    if (! started)
    {
        const auto startDiagnostics = calibrationEngine.getLastStartDiagnostics();
        DBG ("LocusQ: Calibration start rejected ["
             << startDiagnostics.code
             << "] "
             << startDiagnostics.message);
        return false;
    }

    const auto startDiagnostics = calibrationEngine.getLastStartDiagnostics();
    DBG ("LocusQ: Calibration start accepted (seq="
         << static_cast<int> (startDiagnostics.seq)
         << ", writableChannels="
         << writableChannels
         << ")");
    return true;
}

void LocusQAudioProcessor::abortCalibrationFromUI()
{
    calibrationEngine.abortCalibration();
}

juce::var LocusQAudioProcessor::redetectCalibrationRoutingFromUI()
{
    const auto snapshotOutputChannels = getSnapshotOutputChannels();
    const auto layoutOutputChannels = static_cast<int> (getBusesLayout().getMainOutputChannelSet().size());
    const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
        snapshotOutputChannels,
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting());
    const auto previousSpeakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto previousTopologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto previousRouting = getCurrentCalibrationSpeakerRouting();

    applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, true);

    juce::var resultVar (new juce::DynamicObject());
    auto* result = resultVar.getDynamicObject();
    if (result == nullptr)
        return resultVar;

    result->setProperty ("ok", true);
    result->setProperty ("outputChannels", effectiveWritableChannels);
    result->setProperty ("snapshotOutputChannels", snapshotOutputChannels);
    result->setProperty ("layoutOutputChannels", layoutOutputChannels);
    result->setProperty ("effectiveWritableChannels", effectiveWritableChannels);
    const auto topologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    const auto speakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto requiredChannels = getRequiredCalibrationChannelsForTopologyIndex (topologyProfile);
    const auto writableChannels = resolveCalibrationWritableChannels (
        snapshotOutputChannels,
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting());
    result->setProperty ("speakerConfigIndex", speakerConfig);
    result->setProperty ("previousSpeakerConfigIndex", previousSpeakerConfig);
    result->setProperty ("topologyProfileIndex", topologyProfile);
    result->setProperty ("previousTopologyProfileIndex", previousTopologyProfile);
    result->setProperty ("topologyProfile", calibrationTopologyIdForIndex (topologyProfile));
    result->setProperty ("monitoringPathIndex", monitoringPath);
    result->setProperty ("monitoringPath", calibrationMonitoringPathIdForIndex (monitoringPath));
    result->setProperty ("deviceProfileIndex", deviceProfile);
    result->setProperty ("deviceProfile", calibrationDeviceProfileIdForIndex (deviceProfile));
    result->setProperty ("requiredChannels", requiredChannels);
    result->setProperty ("writableChannels", writableChannels);
    result->setProperty ("mappingLimitedToFirst4", requiredChannels > writableChannels);

    juce::Array<juce::var> routing;
    const auto map = getCurrentCalibrationSpeakerRouting();
    for (const auto channel : map)
        routing.add (juce::jlimit (1, 8, channel));
    result->setProperty ("routing", juce::var (routing));

    juce::Array<juce::var> previousRoutingVar;
    for (const auto channel : previousRouting)
        previousRoutingVar.add (juce::jlimit (1, 8, channel));
    result->setProperty ("previousRouting", juce::var (previousRoutingVar));

    const bool changed = map != previousRouting
                         || topologyProfile != previousTopologyProfile
                         || speakerConfig != previousSpeakerConfig;
    result->setProperty ("changed", changed);

    return resultVar;
}

juce::var LocusQAudioProcessor::getCalibrationStatus() const
{
    auto progress = calibrationEngine.getProgress();
    const auto state = progress.state;
    const auto speakerIndex = juce::jlimit (0, 3, progress.currentSpeaker);
    const auto topologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    const auto outputChannels = getMainBusNumOutputChannels();
    const bool rendererSteamAudioAvailable = spatialRenderer.isSteamAudioAvailable();
    const juce::String rendererSteamAudioInitStage {
        SpatialRenderer::steamAudioInitStageToString (spatialRenderer.getSteamAudioInitStageIndex())
    };
    const int rendererHeadphoneModeRequestedIndex = juce::jlimit (
        0,
        1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_headphone_mode")->load())));
    auto rendererHeadphoneModeActiveIndex = spatialRenderer.getHeadphoneRenderModeActiveIndex();
    if (outputChannels >= 2)
    {
        rendererHeadphoneModeActiveIndex =
            (rendererHeadphoneModeRequestedIndex == static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
             && rendererSteamAudioAvailable)
                ? static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
                : static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    else
    {
        rendererHeadphoneModeActiveIndex = static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    auto headphoneCalibration = buildHeadphoneCalibrationDiagnosticsSnapshot (
        monitoringPath,
        rendererHeadphoneModeRequestedIndex,
        rendererHeadphoneModeActiveIndex,
        outputChannels,
        rendererSteamAudioAvailable,
        rendererSteamAudioInitStage);
    const int rendererHeadphoneProfileRequestedIndex = spatialRenderer.getHeadphoneDeviceProfileRequestedIndex();
    const int rendererHeadphoneProfileActiveIndex = spatialRenderer.getHeadphoneDeviceProfileActiveIndex();
    const bool rendererHeadphoneCalibrationEnabledRequested =
        spatialRenderer.isHeadphoneCalibrationEnabledRequested();
    const int rendererHeadphoneCalibrationEngineRequestedIndex =
        spatialRenderer.getHeadphoneCalibrationEngineRequestedIndex();
    const int rendererHeadphoneCalibrationEngineActiveIndex =
        spatialRenderer.getHeadphoneCalibrationEngineActiveIndex();
    const int rendererHeadphoneCalibrationFallbackReasonIndex =
        spatialRenderer.getHeadphoneCalibrationFallbackReasonIndex();
    const int rendererHeadphoneCalibrationLatencySamples =
        spatialRenderer.getHeadphoneCalibrationLatencySamples();
    auto headphoneVerification = buildHeadphoneVerificationSnapshot (
        rendererHeadphoneProfileRequestedIndex,
        rendererHeadphoneProfileActiveIndex,
        rendererHeadphoneCalibrationEnabledRequested,
        rendererHeadphoneCalibrationEngineRequestedIndex,
        rendererHeadphoneCalibrationEngineActiveIndex,
        rendererHeadphoneCalibrationFallbackReasonIndex,
        rendererHeadphoneCalibrationLatencySamples);
    auto profileSyncSeq = static_cast<juce::int64> (sceneSnapshotSequence);
    {
        const juce::SpinLock::ScopedLockType publishedCalibrationLock (publishedHeadphoneCalibrationLock);
        if (publishedHeadphoneCalibrationDiagnostics.valid)
        {
            profileSyncSeq =
                static_cast<juce::int64> (publishedHeadphoneCalibrationDiagnostics.profileSyncSeq);
            headphoneCalibration.requested = publishedHeadphoneCalibrationDiagnostics.requested;
            headphoneCalibration.active = publishedHeadphoneCalibrationDiagnostics.active;
            headphoneCalibration.stage = publishedHeadphoneCalibrationDiagnostics.stage;
            headphoneCalibration.fallbackReady = publishedHeadphoneCalibrationDiagnostics.fallbackReady;
            headphoneCalibration.fallbackReason = publishedHeadphoneCalibrationDiagnostics.fallbackReason;
        }

        if (publishedHeadphoneVerificationDiagnostics.valid)
        {
            profileSyncSeq =
                static_cast<juce::int64> (publishedHeadphoneVerificationDiagnostics.profileSyncSeq);
            headphoneVerification.profileId = publishedHeadphoneVerificationDiagnostics.profileId;
            headphoneVerification.requestedProfileId =
                publishedHeadphoneVerificationDiagnostics.requestedProfileId;
            headphoneVerification.activeProfileId =
                publishedHeadphoneVerificationDiagnostics.activeProfileId;
            headphoneVerification.requestedEngineId =
                publishedHeadphoneVerificationDiagnostics.requestedEngineId;
            headphoneVerification.activeEngineId =
                publishedHeadphoneVerificationDiagnostics.activeEngineId;
            headphoneVerification.fallbackReasonCode =
                publishedHeadphoneVerificationDiagnostics.fallbackReasonCode;
            headphoneVerification.fallbackTarget =
                publishedHeadphoneVerificationDiagnostics.fallbackTarget;
            headphoneVerification.fallbackReasonText =
                publishedHeadphoneVerificationDiagnostics.fallbackReasonText;
            headphoneVerification.frontBackScore =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedHeadphoneVerificationDiagnostics.frontBackScore,
                    0.0f);
            headphoneVerification.elevationScore =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedHeadphoneVerificationDiagnostics.elevationScore,
                    0.0f);
            headphoneVerification.externalizationScore =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedHeadphoneVerificationDiagnostics.externalizationScore,
                    0.0f);
            headphoneVerification.confidence =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedHeadphoneVerificationDiagnostics.confidence,
                    0.0f);
            headphoneVerification.verificationStage =
                publishedHeadphoneVerificationDiagnostics.verificationStage;
            headphoneVerification.verificationScoreStatus =
                publishedHeadphoneVerificationDiagnostics.verificationScoreStatus;
            headphoneVerification.chainLatencySamples =
                locusq::shared_contracts::headphone_verification::sanitizeLatencySamples (
                    publishedHeadphoneVerificationDiagnostics.chainLatencySamples);
        }
    }
    headphoneVerification.profileId =
        locusq::shared_contracts::headphone_verification::sanitizeProfileId (headphoneVerification.profileId);
    headphoneVerification.requestedProfileId =
        locusq::shared_contracts::headphone_verification::sanitizeProfileId (
            headphoneVerification.requestedProfileId);
    headphoneVerification.activeProfileId =
        locusq::shared_contracts::headphone_verification::sanitizeProfileId (
            headphoneVerification.activeProfileId);
    headphoneVerification.requestedEngineId =
        locusq::shared_contracts::headphone_verification::sanitizeEngineId (
            headphoneVerification.requestedEngineId);
    headphoneVerification.activeEngineId =
        locusq::shared_contracts::headphone_verification::sanitizeEngineId (
            headphoneVerification.activeEngineId);
    headphoneVerification.fallbackReasonCode =
        locusq::shared_contracts::headphone_verification::sanitizeFallbackReasonCode (
            headphoneVerification.fallbackReasonCode);
    headphoneVerification.fallbackTarget =
        locusq::shared_contracts::headphone_verification::sanitizeFallbackTargetForReason (
            headphoneVerification.fallbackReasonCode,
            headphoneVerification.fallbackTarget,
            headphoneVerification.activeEngineId);
    headphoneVerification.fallbackReasonText =
        locusq::shared_contracts::headphone_verification::fallbackReasonTextForCode (
            headphoneVerification.fallbackReasonCode);
    headphoneVerification.verificationStage =
        locusq::shared_contracts::headphone_verification::sanitizeVerificationStage (
            headphoneVerification.verificationStage);
    headphoneVerification.verificationScoreStatus =
        locusq::shared_contracts::headphone_verification::scoreStatusFromStage (
            headphoneVerification.verificationStage);
    headphoneVerification.frontBackScore =
        locusq::shared_contracts::headphone_verification::sanitizeScore (
            headphoneVerification.frontBackScore,
            0.0f);
    headphoneVerification.elevationScore =
        locusq::shared_contracts::headphone_verification::sanitizeScore (
            headphoneVerification.elevationScore,
            0.0f);
    headphoneVerification.externalizationScore =
        locusq::shared_contracts::headphone_verification::sanitizeScore (
            headphoneVerification.externalizationScore,
            0.0f);
    headphoneVerification.confidence =
        locusq::shared_contracts::headphone_verification::sanitizeScore (
            headphoneVerification.confidence,
            0.0f);
    headphoneVerification.chainLatencySamples =
        locusq::shared_contracts::headphone_verification::sanitizeLatencySamples (
            headphoneVerification.chainLatencySamples);
    const auto requiredChannels = getRequiredCalibrationChannelsForTopologyIndex (topologyProfile);
    const auto routing = getCurrentCalibrationSpeakerRouting();
    const auto writableChannels = resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        routing);
    const auto mappingLimitedToFirst4 = requiredChannels > writableChannels;
    const auto startDiagnostics = calibrationEngine.getLastStartDiagnostics();
    const auto checkedRows = juce::jlimit (1, SpatialRenderer::NUM_SPEAKERS, juce::jmin (requiredChannels, writableChannels));
    std::array<bool, 9> seenChannels {};
    bool mappingDuplicateChannels = false;
    bool mappingChannelsInRange = true;

    for (int i = 0; i < checkedRows; ++i)
    {
        const auto routedChannel = juce::jlimit (1, 8, routing[static_cast<size_t> (i)]);
        if (routedChannel < 1 || routedChannel > 8)
        {
            mappingChannelsInRange = false;
            continue;
        }

        if (seenChannels[static_cast<size_t> (routedChannel)])
            mappingDuplicateChannels = true;
        seenChannels[static_cast<size_t> (routedChannel)] = true;
    }
    const bool mappingValid = mappingChannelsInRange && ! mappingDuplicateChannels && ! mappingLimitedToFirst4;

    int completedSpeakers = 0;
    float speakerPhasePercent = 0.0f;
    bool running = false;

    switch (state)
    {
        case CalibrationEngine::State::Idle:
            break;

        case CalibrationEngine::State::Playing:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = juce::jlimit (0.0f, 1.0f, progress.playPercent) * 0.5f;
            break;

        case CalibrationEngine::State::Recording:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = 0.5f + juce::jlimit (0.0f, 1.0f, progress.recordPercent) * 0.45f;
            break;

        case CalibrationEngine::State::Analyzing:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = 0.95f;
            break;

        case CalibrationEngine::State::Complete:
            completedSpeakers = 4;
            speakerPhasePercent = 1.0f;
            break;

        case CalibrationEngine::State::Error:
            completedSpeakers = speakerIndex;
            break;
    }

    auto overallPercent = (state == CalibrationEngine::State::Complete)
                            ? 1.0f
                            : (static_cast<float> (completedSpeakers) + speakerPhasePercent) / 4.0f;
    overallPercent = juce::jlimit (0.0f, 1.0f, overallPercent);

    juce::var statusVar (new juce::DynamicObject());
    auto* status = statusVar.getDynamicObject();

    status->setProperty ("state", toCalibrationStateString (state));
    status->setProperty ("stateCode", static_cast<int> (state));
    status->setProperty ("running", running);
    status->setProperty ("complete", state == CalibrationEngine::State::Complete);
    status->setProperty ("currentSpeaker", speakerIndex + 1);
    status->setProperty ("completedSpeakers", completedSpeakers);
    status->setProperty ("playPercent", juce::jlimit (0.0f, 1.0f, progress.playPercent));
    status->setProperty ("recordPercent", juce::jlimit (0.0f, 1.0f, progress.recordPercent));
    status->setProperty ("overallPercent", overallPercent);
    status->setProperty ("message", progress.message);
    status->setProperty ("startAck", startDiagnostics.accepted);
    status->setProperty ("startSeq", static_cast<int> (startDiagnostics.seq));
    status->setProperty ("startCode", startDiagnostics.code);
    status->setProperty ("startMessage", startDiagnostics.message);
    status->setProperty ("startStateAtRequest", startDiagnostics.stateAtRequest);
    status->setProperty ("startTimestampMs", startDiagnostics.timestampMs);
    status->setProperty ("profileSyncSeq", profileSyncSeq);
    status->setProperty ("topologyProfileIndex", topologyProfile);
    status->setProperty ("topologyProfile", calibrationTopologyIdForIndex (topologyProfile));
    status->setProperty ("monitoringPathIndex", monitoringPath);
    status->setProperty ("monitoringPath", calibrationMonitoringPathIdForIndex (monitoringPath));
    status->setProperty ("deviceProfileIndex", deviceProfile);
    status->setProperty ("deviceProfile", calibrationDeviceProfileIdForIndex (deviceProfile));
    status->setProperty ("headphoneCalibrationSchema", locusq::shared_contracts::headphone_calibration::kSchemaV1);
    status->setProperty ("headphoneCalibrationRequested", headphoneCalibration.requested);
    status->setProperty ("headphoneCalibrationActive", headphoneCalibration.active);
    status->setProperty ("headphoneCalibrationStage", headphoneCalibration.stage);
    status->setProperty ("headphoneCalibrationFallbackReady", headphoneCalibration.fallbackReady);
    status->setProperty ("headphoneCalibrationFallbackReason", headphoneCalibration.fallbackReason);
    status->setProperty ("headphoneVerificationSchema", locusq::shared_contracts::headphone_verification::kSchemaV1);
    status->setProperty ("headphoneVerificationProfileId", headphoneVerification.profileId);
    status->setProperty ("headphoneVerificationRequestedProfileId", headphoneVerification.requestedProfileId);
    status->setProperty ("headphoneVerificationActiveProfileId", headphoneVerification.activeProfileId);
    status->setProperty ("headphoneVerificationRequestedEngineId", headphoneVerification.requestedEngineId);
    status->setProperty ("headphoneVerificationActiveEngineId", headphoneVerification.activeEngineId);
    status->setProperty ("headphoneVerificationFallbackReasonCode", headphoneVerification.fallbackReasonCode);
    status->setProperty ("headphoneVerificationFallbackTarget", headphoneVerification.fallbackTarget);
    status->setProperty ("headphoneVerificationFallbackReasonText", headphoneVerification.fallbackReasonText);
    status->setProperty (
        "headphoneVerificationFrontBackScore",
        locusq::shared_contracts::headphone_verification::sanitizeScore (headphoneVerification.frontBackScore, 0.0f));
    status->setProperty (
        "headphoneVerificationElevationScore",
        locusq::shared_contracts::headphone_verification::sanitizeScore (headphoneVerification.elevationScore, 0.0f));
    status->setProperty (
        "headphoneVerificationExternalizationScore",
        locusq::shared_contracts::headphone_verification::sanitizeScore (
            headphoneVerification.externalizationScore,
            0.0f));
    status->setProperty (
        "headphoneVerificationConfidence",
        locusq::shared_contracts::headphone_verification::sanitizeScore (headphoneVerification.confidence, 0.0f));
    status->setProperty ("headphoneVerificationStage", headphoneVerification.verificationStage);
    status->setProperty ("headphoneVerificationScoreStatus", headphoneVerification.verificationScoreStatus);
    status->setProperty (
        "headphoneVerificationLatencySamples",
        locusq::shared_contracts::headphone_verification::sanitizeLatencySamples (
            headphoneVerification.chainLatencySamples));
    status->setProperty ("requiredChannels", requiredChannels);
    status->setProperty ("writableChannels", writableChannels);
    status->setProperty ("mappingLimitedToFirst4", mappingLimitedToFirst4);
    status->setProperty ("mappingDuplicateChannels", mappingDuplicateChannels);
    status->setProperty ("mappingValid", mappingValid);

    juce::var headphoneCalibrationVar (new juce::DynamicObject());
    if (auto* headphoneContract = headphoneCalibrationVar.getDynamicObject())
    {
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kSchema,
            locusq::shared_contracts::headphone_calibration::kSchemaV1);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kRequested,
            headphoneCalibration.requested);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kActive,
            headphoneCalibration.active);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kStage,
            headphoneCalibration.stage);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kFallbackReady,
            headphoneCalibration.fallbackReady);
        headphoneContract->setProperty (
            locusq::shared_contracts::headphone_calibration::fields::kFallbackReason,
            headphoneCalibration.fallbackReason);
    }
    status->setProperty ("headphoneCalibration", headphoneCalibrationVar);

    if (! running
        && state != CalibrationEngine::State::Complete
        && ! startDiagnostics.accepted
        && startDiagnostics.seq > 0
        && startDiagnostics.message.isNotEmpty())
    {
        status->setProperty ("message", startDiagnostics.message);
    }

    juce::Array<juce::var> speakerLevels;
    speakerLevels.ensureStorageAllocated (4);
    for (int i = 0; i < 4; ++i)
    {
        float level = 0.0f;

        if (state == CalibrationEngine::State::Complete || i < completedSpeakers)
        {
            level = 1.0f;
        }
        else if (running && i == speakerIndex)
        {
            if (state == CalibrationEngine::State::Playing)
                level = juce::jlimit (0.0f, 1.0f, progress.playPercent);
            else if (state == CalibrationEngine::State::Recording)
                level = juce::jlimit (0.0f, 1.0f, progress.recordPercent);
            else if (state == CalibrationEngine::State::Analyzing)
                level = 1.0f;
        }

        speakerLevels.add (juce::jlimit (0.0f, 1.0f, level));
    }
    status->setProperty ("speakerLevels", juce::var (speakerLevels));

    juce::Array<juce::var> speakerRouting;
    speakerRouting.ensureStorageAllocated (4);
    for (const auto channel : routing)
        speakerRouting.add (juce::jlimit (1, 8, channel));
    status->setProperty ("speakerRouting", juce::var (speakerRouting));

    const auto roomProfile = sceneGraph.getRoomProfile();
    status->setProperty ("profileValid", roomProfile != nullptr && roomProfile->valid);
    status->setProperty ("phasePass", state == CalibrationEngine::State::Complete);
    const auto estimatedRt60 = calibrationEngine.getResult().estimatedRT60;
    const bool delayPass = state == CalibrationEngine::State::Complete
                           && std::isfinite (estimatedRt60)
                           && estimatedRt60 > 0.0f;
    status->setProperty ("delayPass", delayPass);

    if (state == CalibrationEngine::State::Complete)
        status->setProperty ("estimatedRT60", estimatedRt60);

    // Companion headphone device status — cached by pollCompanionCalibrationProfileFromDisk().
    // Fields mirror the CalibrationProfile.json schema; verification scores are null until
    // Phase B (Task 17) writes them back.
    {
        juce::var hpDeviceVar (new juce::DynamicObject());
        auto* hpDevice = hpDeviceVar.getDynamicObject();

        hpDevice->setProperty ("device",           cachedCalibrationDevice);
        hpDevice->setProperty ("eq_mode",          cachedCalibrationEqMode);
        hpDevice->setProperty ("hrtf_mode",        cachedCalibrationHrtfMode);
        hpDevice->setProperty ("tracking_enabled", cachedCalibrationTrackingEnabled);
        hpDevice->setProperty ("fir_latency_samples", cachedCalibrationFirLatency);

        // Scores: use JSON null when not yet set (value -1 sentinel).
        if (cachedExternalizationScore >= 0.0f)
            hpDevice->setProperty ("externalization_score",    cachedExternalizationScore);
        else
            hpDevice->setProperty ("externalization_score",    juce::var());

        if (cachedFrontBackConfusionRate >= 0.0f)
            hpDevice->setProperty ("front_back_confusion_rate", cachedFrontBackConfusionRate);
        else
            hpDevice->setProperty ("front_back_confusion_rate", juce::var());

        status->setProperty ("hpDeviceStatus", hpDeviceVar);
    }

    return statusVar;
}

juce::var LocusQAudioProcessor::serialiseKeyframeTimelineLocked() const
{
    juce::var timelineVar (new juce::DynamicObject());
    auto* timeline = timelineVar.getDynamicObject();

    timeline->setProperty ("durationSeconds", keyframeTimeline.getDurationSeconds());
    timeline->setProperty ("looping", keyframeTimeline.isLooping());
    timeline->setProperty ("playbackRate", keyframeTimeline.getPlaybackRate());
    timeline->setProperty ("currentTimeSeconds", keyframeTimeline.getCurrentTimeSeconds());

    juce::Array<juce::var> tracks;

    for (const auto& track : keyframeTimeline.getTracks())
    {
        juce::var trackVar (new juce::DynamicObject());
        auto* trackObject = trackVar.getDynamicObject();
        trackObject->setProperty ("parameterId", track.getParameterId());

        juce::Array<juce::var> keyframes;
        for (const auto& keyframe : track.getKeyframes())
        {
            juce::var keyframeVar (new juce::DynamicObject());
            auto* keyframeObject = keyframeVar.getDynamicObject();
            keyframeObject->setProperty ("timeSeconds", keyframe.timeSeconds);
            keyframeObject->setProperty ("value", keyframe.value);
            keyframeObject->setProperty ("curve", keyframeCurveToString (keyframe.curve));
            keyframes.add (keyframeVar);
        }

        trackObject->setProperty ("keyframes", juce::var (keyframes));
        tracks.add (trackVar);
    }

    timeline->setProperty ("tracks", juce::var (tracks));
    return timelineVar;
}

bool LocusQAudioProcessor::applyKeyframeTimelineLocked (const juce::var& timelineState)
{
    auto* timeline = timelineState.getDynamicObject();
    if (timeline == nullptr)
        return false;

    auto* trackArray = timeline->getProperty ("tracks").getArray();
    if (trackArray == nullptr)
        return false;

    keyframeTimeline.clearTracks();

    for (const auto& trackValue : *trackArray)
    {
        auto* trackObject = trackValue.getDynamicObject();
        if (trackObject == nullptr)
            continue;

        const auto parameterId = trackObject->getProperty ("parameterId").toString().trim();
        if (parameterId.isEmpty())
            continue;

        std::vector<Keyframe> keyframes;
        if (auto* keyframeArray = trackObject->getProperty ("keyframes").getArray())
        {
            keyframes.reserve (static_cast<size_t> (keyframeArray->size()));

            for (const auto& keyframeValue : *keyframeArray)
            {
                auto* keyframeObject = keyframeValue.getDynamicObject();
                if (keyframeObject == nullptr)
                    continue;

                Keyframe keyframe;
                keyframe.timeSeconds = static_cast<double> (keyframeObject->getProperty ("timeSeconds"));
                keyframe.value = static_cast<float> (double (keyframeObject->getProperty ("value")));
                keyframe.curve = keyframeCurveFromVar (keyframeObject->getProperty ("curve"));
                keyframes.push_back (keyframe);
            }
        }

        if (! keyframes.empty())
        {
            KeyframeTrack track { parameterId };
            track.setKeyframes (std::move (keyframes));
            keyframeTimeline.addOrReplaceTrack (std::move (track));
        }
    }

    if (timeline->hasProperty ("durationSeconds"))
        keyframeTimeline.setDurationSeconds (static_cast<double> (timeline->getProperty ("durationSeconds")));

    if (timeline->hasProperty ("looping"))
        keyframeTimeline.setLooping (static_cast<bool> (timeline->getProperty ("looping")));

    if (timeline->hasProperty ("playbackRate"))
        keyframeTimeline.setPlaybackRate (static_cast<float> (double (timeline->getProperty ("playbackRate"))));

    if (timeline->hasProperty ("currentTimeSeconds"))
        keyframeTimeline.setCurrentTimeSeconds (static_cast<double> (timeline->getProperty ("currentTimeSeconds")));

    if (! keyframeTimeline.hasAnyTrack())
        initialiseDefaultKeyframeTimeline();

    return true;
}

juce::var LocusQAudioProcessor::getKeyframeTimelineForUI() const
{
    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    return serialiseKeyframeTimelineLocked();
}

bool LocusQAudioProcessor::setKeyframeTimelineFromUI (const juce::var& timelineState)
{
    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    return applyKeyframeTimelineLocked (timelineState);
}

bool LocusQAudioProcessor::setTimelineCurrentTimeFromUI (double timeSeconds)
{
    if (! std::isfinite (timeSeconds))
        return false;

    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    const auto clamped = juce::jlimit (0.0,
                                       juce::jmax (0.0, keyframeTimeline.getDurationSeconds()),
                                       timeSeconds);
    keyframeTimeline.setCurrentTimeSeconds (clamped);
    return true;
}
