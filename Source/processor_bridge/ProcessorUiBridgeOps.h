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

juce::String LocusQAudioProcessor::sanitisePresetName (const juce::String& presetName)
{
    return locusq::processor_bridge::sanitisePresetName (presetName);
}

juce::String LocusQAudioProcessor::normalisePresetType (const juce::String& presetType)
{
    return locusq::processor_bridge::normalisePresetType (presetType,
                                                          kEmitterPresetTypeEmitter,
                                                          kEmitterPresetTypeMotion);
}

juce::String LocusQAudioProcessor::normaliseChoreographyPackId (const juce::String& packId)
{
    return locusq::processor_bridge::normaliseChoreographyPackId (packId, kChoreographyPackIds);
}

juce::String LocusQAudioProcessor::normaliseCalibrationTopologyId (const juce::String& topologyId)
{
    return locusq::processor_bridge::normaliseCalibrationTopologyId (
        topologyId,
        kCalibrationTopologyIds,
        [] (int index) { return calibrationTopologyIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::normaliseCalibrationMonitoringPathId (const juce::String& monitoringPathId)
{
    return locusq::processor_bridge::normaliseCalibrationMonitoringPathId (
        monitoringPathId,
        kCalibrationMonitoringPathIds,
        [] (int index) { return calibrationMonitoringPathIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::normaliseCalibrationDeviceProfileId (const juce::String& deviceProfileId)
{
    return locusq::processor_bridge::normaliseCalibrationDeviceProfileId (
        deviceProfileId,
        kCalibrationDeviceProfileIds,
        [] (int index) { return calibrationDeviceProfileIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::inferPresetTypeFromPayload (const juce::var& payload)
{
    return locusq::processor_bridge::inferPresetTypeFromPayload (payload,
                                                                 kEmitterPresetTypeProperty,
                                                                 kEmitterPresetTypeEmitter,
                                                                 kEmitterPresetTypeMotion);
}

juce::String LocusQAudioProcessor::sanitiseEmitterLabel (const juce::String& label)
{
    return locusq::processor_bridge::sanitiseEmitterLabel (label);
}

juce::File LocusQAudioProcessor::getPresetDirectory() const
{
    return locusq::processor_bridge::getUserDataSubdirectory ("Presets");
}

juce::File LocusQAudioProcessor::resolvePresetFileFromOptions (const juce::var& options) const
{
    return locusq::processor_bridge::resolveNamedJsonFileFromOptions (
        options,
        getPresetDirectory(),
        [] (const juce::String& name) { return locusq::processor_bridge::sanitisePresetName (name); });
}

juce::File LocusQAudioProcessor::getCalibrationProfileDirectory() const
{
    return locusq::processor_bridge::getUserDataSubdirectory ("CalibrationProfiles");
}

juce::File LocusQAudioProcessor::resolveCalibrationProfileFileFromOptions (const juce::var& options) const
{
    return locusq::processor_bridge::resolveNamedJsonFileFromOptions (
        options,
        getCalibrationProfileDirectory(),
        [] (const juce::String& name) { return locusq::processor_bridge::sanitisePresetName (name); });
}

juce::String LocusQAudioProcessor::getSnapshotOutputLayout() const
{
    return outputLayoutToString (getBusesLayout().getMainOutputChannelSet());
}

int LocusQAudioProcessor::getSnapshotOutputChannels() const
{
    return locusq::processor_core::readSnapshotOutputChannels (getMainBusNumOutputChannels(),
                                                               getTotalNumOutputChannels());
}

std::array<int, SpatialRenderer::NUM_SPEAKERS> LocusQAudioProcessor::getCurrentCalibrationSpeakerRouting() const
{
    return locusq::processor_core::readCalibrationSpeakerRouting (apvts);
}

int LocusQAudioProcessor::getCurrentCalibrationSpeakerConfigIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (apvts,
                                                               "cal_spk_config",
                                                               0,
                                                               1,
                                                               0);
}

int LocusQAudioProcessor::getCurrentCalibrationTopologyProfileIndex() const
{
    if (apvts.getRawParameterValue ("cal_topology_profile") != nullptr)
    {
        return locusq::processor_core::readDiscreteParameterIndex (
            apvts,
            "cal_topology_profile",
            0,
            static_cast<int> (kCalibrationTopologyIds.size()) - 1,
            1);
    }

    const auto legacyConfig = getCurrentCalibrationSpeakerConfigIndex();
    return legacyConfig == 1 ? 1 : 2;
}

int LocusQAudioProcessor::getCurrentCalibrationMonitoringPathIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (
        apvts,
        "cal_monitoring_path",
        0,
        static_cast<int> (kCalibrationMonitoringPathIds.size()) - 1,
        0);
}

int LocusQAudioProcessor::getCurrentCalibrationDeviceProfileIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (
        apvts,
        "cal_device_profile",
        0,
        static_cast<int> (kCalibrationDeviceProfileIds.size()) - 1,
        0);
}

int LocusQAudioProcessor::getRequiredCalibrationChannelsForTopologyIndex (int topologyIndex) const
{
    return calibrationRequiredChannelsForTopologyIndex (topologyIndex);
}

void LocusQAudioProcessor::applyAutoDetectedCalibrationRoutingIfAppropriate (int outputChannels, bool force)
{
    const auto clampedOutputChannels = juce::jlimit (1, 16, outputChannels);

    std::array<int, SpatialRenderer::NUM_SPEAKERS> autoRouting { 1, 2, 3, 4 };
    int autoSpeakerConfig = 0; // 0 = 4x Mono, 1 = 2x Stereo
    int autoTopologyProfile = topologyProfileForOutputChannels (clampedOutputChannels);

    if (clampedOutputChannels == 1)
    {
        autoSpeakerConfig = 1;
        autoRouting = { 1, 1, 1, 1 };
    }
    else if (clampedOutputChannels == 2)
    {
        autoSpeakerConfig = 1;
        autoRouting = { 1, 2, 1, 2 };
    }
    else if (clampedOutputChannels == 3)
    {
        autoSpeakerConfig = 0;
        autoRouting = { 1, 2, 3, 3 };
    }

    const auto currentRouting = getCurrentCalibrationSpeakerRouting();
    const auto currentSpeakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto currentTopologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto isFactoryMonoRouting = currentSpeakerConfig == 0
                                      && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 3, 4 };
    const auto isFactoryStereoRouting = currentSpeakerConfig == 1
                                        && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 1, 2 };
    const auto isFactoryMonoByChoice = currentSpeakerConfig == 0
                                       && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 1, 2 };
    const auto isFactoryTopologyProfile = currentTopologyProfile == 2 || currentTopologyProfile == 1;
    const auto followsPreviousAuto = hasAppliedAutoDetectedCalibrationRouting
                                     && currentTopologyProfile == lastAutoDetectedTopologyProfile
                                     && currentSpeakerConfig == lastAutoDetectedSpeakerConfig
                                     && currentRouting == lastAutoDetectedSpeakerRouting;

    if (! force
        && ! followsPreviousAuto
        && ! isFactoryMonoRouting
        && ! isFactoryStereoRouting
        && ! isFactoryMonoByChoice
        && ! isFactoryTopologyProfile)
    {
        return;
    }

    if (hasAppliedAutoDetectedCalibrationRouting
        && clampedOutputChannels == lastAutoDetectedOutputChannels
        && autoTopologyProfile == lastAutoDetectedTopologyProfile
        && autoSpeakerConfig == lastAutoDetectedSpeakerConfig
        && autoRouting == lastAutoDetectedSpeakerRouting)
    {
        return;
    }

    setIntegerParameterValueNotifyingHost ("cal_topology_profile", autoTopologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", autoSpeakerConfig);
    setIntegerParameterValueNotifyingHost ("cal_spk1_out", autoRouting[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", autoRouting[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", autoRouting[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", autoRouting[3]);

    hasAppliedAutoDetectedCalibrationRouting = true;
    lastAutoDetectedOutputChannels = clampedOutputChannels;
    lastAutoDetectedTopologyProfile = autoTopologyProfile;
    lastAutoDetectedSpeakerConfig = autoSpeakerConfig;
    lastAutoDetectedSpeakerRouting = autoRouting;
}

void LocusQAudioProcessor::setIntegerParameterValueNotifyingHost (const char* parameterId, int value)
{
    locusq::processor_core::setIntegerParameterValueNotifyingHost (apvts, parameterId, value);
}

void LocusQAudioProcessor::migrateSnapshotLayoutIfNeeded (const juce::ValueTree& restoredState)
{
    int storedOutputChannels = 0;
    if (restoredState.hasProperty (kSnapshotOutputChannelsProperty))
    {
        storedOutputChannels = juce::jlimit (1,
                                             kMaxSnapshotOutputChannels,
                                             static_cast<int> (restoredState.getProperty (kSnapshotOutputChannelsProperty)));
    }
    else if (restoredState.hasProperty (kSnapshotOutputLayoutProperty))
    {
        const auto storedLayout = restoredState.getProperty (kSnapshotOutputLayoutProperty).toString().trim().toLowerCase();
        if (storedLayout == "mono")
            storedOutputChannels = 1;
        else if (storedLayout == "stereo")
            storedOutputChannels = 2;
        else if (storedLayout == "quad")
            storedOutputChannels = SpatialRenderer::NUM_SPEAKERS;
        else if (storedLayout == "surround_5_1")
            storedOutputChannels = 6;
        else if (storedLayout == "surround_5_2_1")
            storedOutputChannels = 8;
        else if (storedLayout == "surround_7_1")
            storedOutputChannels = 8;
        else if (storedLayout == "surround_7_2_1")
            storedOutputChannels = 10;
        else if (storedLayout == "surround_7_1_4")
            storedOutputChannels = 12;
        else if (storedLayout == "surround_7_4_2")
            storedOutputChannels = 13;
        else if (storedLayout == "multichannel")
            storedOutputChannels = juce::jmax (SpatialRenderer::NUM_SPEAKERS, storedOutputChannels);
    }

    const auto currentOutputChannels = juce::jlimit (1,
                                                     kMaxSnapshotOutputChannels,
                                                     getSnapshotOutputChannels());
    const auto isLegacySnapshot = ! restoredState.hasProperty (kSnapshotSchemaProperty);
    const auto hasLayoutMismatch = (storedOutputChannels > 0 && storedOutputChannels != currentOutputChannels);

    if (! isLegacySnapshot && ! hasLayoutMismatch)
        return;

    std::array<int, SpatialRenderer::NUM_SPEAKERS> migratedSpeakerMap { 1, 2, 3, 4 };
    int migratedSpeakerConfig = 0;
    const int migratedTopologyProfile = topologyProfileForOutputChannels (currentOutputChannels);

    if (currentOutputChannels == 1)
    {
        migratedSpeakerMap.fill (1);
        migratedSpeakerConfig = 1;
    }
    else if (currentOutputChannels == 2)
    {
        migratedSpeakerMap = { 1, 2, 1, 2 };
        migratedSpeakerConfig = 1;
    }

    setIntegerParameterValueNotifyingHost ("cal_topology_profile", migratedTopologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", migratedSpeakerConfig);
    setIntegerParameterValueNotifyingHost ("cal_spk1_out", migratedSpeakerMap[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", migratedSpeakerMap[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", migratedSpeakerMap[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", migratedSpeakerMap[3]);
}

juce::String LocusQAudioProcessor::keyframeCurveToString (KeyframeCurve curve)
{
    const auto index = static_cast<size_t> (juce::jlimit (0, static_cast<int> (kCurveNames.size()) - 1, static_cast<int> (curve)));
    return juce::String (kCurveNames[index]);
}

KeyframeCurve LocusQAudioProcessor::keyframeCurveFromVar (const juce::var& value)
{
    if (value.isInt() || value.isInt64() || value.isDouble())
        return static_cast<KeyframeCurve> (juce::jlimit (0, static_cast<int> (kCurveNames.size()) - 1, static_cast<int> (value)));

    const auto text = value.toString().trim();
    for (size_t i = 0; i < kCurveNames.size(); ++i)
    {
        if (text.equalsIgnoreCase (kCurveNames[i]))
            return static_cast<KeyframeCurve> (i);
    }

    return KeyframeCurve::linear;
}

std::optional<juce::var> LocusQAudioProcessor::readJsonFromFile (const juce::File& file)
{
    return locusq::processor_bridge::readJsonFromFile (file);
}

bool LocusQAudioProcessor::writeJsonToFile (const juce::File& file, const juce::var& payload)
{
    return locusq::processor_bridge::writeJsonToFile (file, payload);
}

void LocusQAudioProcessor::applyEmitterLabelToSceneSlotIfAvailable (const juce::String& label)
{
    if (emitterSlotId < 0 || ! sceneGraph.isSlotActive (emitterSlotId))
        return;

    auto data = sceneGraph.getSlot (emitterSlotId).read();
    const auto sanitised = sanitiseEmitterLabel (label);
    std::snprintf (data.label, sizeof (data.label), "%s", sanitised.toRawUTF8());
    sceneGraph.getSlot (emitterSlotId).write (data);
}

juce::var LocusQAudioProcessor::buildEmitterPresetLocked (const juce::String& presetName,
                                                          const juce::String& presetType,
                                                          const juce::String& choreographyPackId,
                                                          bool includeParameters,
                                                          bool includeTimeline) const
{
    juce::var presetVar (new juce::DynamicObject());
    auto* preset = presetVar.getDynamicObject();

    preset->setProperty ("schema", kEmitterPresetSchemaV2);
    preset->setProperty ("name", presetName);
    preset->setProperty (kEmitterPresetTypeProperty, normalisePresetType (presetType));
    preset->setProperty ("savedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    preset->setProperty ("choreographyPackId", normaliseChoreographyPackId (choreographyPackId));

    juce::var layoutVar (new juce::DynamicObject());
    auto* layout = layoutVar.getDynamicObject();
    layout->setProperty ("outputLayout", getSnapshotOutputLayout());
    layout->setProperty ("outputChannels", getSnapshotOutputChannels());
    preset->setProperty (kEmitterPresetLayoutProperty, layoutVar);

    if (includeParameters)
    {
        juce::var parametersVar (new juce::DynamicObject());
        auto* parameters = parametersVar.getDynamicObject();
        for (const auto* parameterId : kEmitterPresetParameterIds)
        {
            if (auto* parameter = apvts.getParameter (parameterId))
                parameters->setProperty (parameterId, parameter->getValue());
        }

        preset->setProperty ("parameters", parametersVar);
    }

    if (includeTimeline)
        preset->setProperty ("timeline", serialiseKeyframeTimelineLocked());

    return presetVar;
}

bool LocusQAudioProcessor::applyEmitterPresetLocked (const juce::var& presetState)
{
    auto* preset = presetState.getDynamicObject();
    if (preset == nullptr)
        return false;

    if (preset->hasProperty ("schema"))
    {
        const auto schema = preset->getProperty ("schema").toString();
        if (schema.isNotEmpty()
            && schema != kEmitterPresetSchemaV1
            && schema != kEmitterPresetSchemaV2)
        {
            return false;
        }
    }

    if (auto* layout = preset->getProperty (kEmitterPresetLayoutProperty).getDynamicObject())
    {
        if (layout->hasProperty ("outputChannels"))
        {
            const auto parsedChannels = static_cast<int> (layout->getProperty ("outputChannels"));
            if (parsedChannels <= 0)
                return false;
        }

        if (layout->hasProperty ("outputLayout")
            && layout->getProperty ("outputLayout").toString().trim().isEmpty())
        {
            return false;
        }
    }

    if (auto* parameters = preset->getProperty ("parameters").getDynamicObject())
    {
        for (const auto* parameterId : kEmitterPresetParameterIds)
        {
            if (parameters->hasProperty (parameterId))
            {
                if (auto* parameter = apvts.getParameter (parameterId))
                {
                    const auto normalized = juce::jlimit (0.0f, 1.0f, static_cast<float> (double (parameters->getProperty (parameterId))));
                    parameter->setValueNotifyingHost (normalized);
                }
            }
        }
    }

    if (preset->hasProperty ("timeline"))
        applyKeyframeTimelineLocked (preset->getProperty ("timeline"));

    {
        const auto choreographyPack = preset->hasProperty ("choreographyPackId")
            ? normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString())
            : juce::String ("custom");
        const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
        choreographyPackState = choreographyPack;
    }

    return true;
}

juce::var LocusQAudioProcessor::buildCalibrationProfileState (const juce::String& profileName,
                                                              const juce::var& validationSummary) const
{
    juce::var profileVar (new juce::DynamicObject());
    auto* profile = profileVar.getDynamicObject();

    profile->setProperty ("schema", kCalibrationProfileSchemaV1);
    profile->setProperty ("name", profileName);
    profile->setProperty ("savedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));

    juce::var contextVar (new juce::DynamicObject());
    auto* context = contextVar.getDynamicObject();
    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPathIndex = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfileIndex = getCurrentCalibrationDeviceProfileIndex();
    context->setProperty ("topologyProfileIndex", topologyIndex);
    context->setProperty ("topologyProfile", calibrationTopologyIdForIndex (topologyIndex));
    context->setProperty ("monitoringPathIndex", monitoringPathIndex);
    context->setProperty ("monitoringPath", calibrationMonitoringPathIdForIndex (monitoringPathIndex));
    context->setProperty ("deviceProfileIndex", deviceProfileIndex);
    context->setProperty ("deviceProfile", calibrationDeviceProfileIdForIndex (deviceProfileIndex));
    context->setProperty ("requiredChannels", getRequiredCalibrationChannelsForTopologyIndex (topologyIndex));
    context->setProperty ("writableChannels", resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting()));
    profile->setProperty ("context", contextVar);

    juce::var controlsVar (new juce::DynamicObject());
    auto* controls = controlsVar.getDynamicObject();
    for (const auto* parameterId : kCalibrationProfileParameterIds)
    {
        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        {
            const auto scaledValue = parameter->convertFrom0to1 (parameter->getValue());
            controls->setProperty (parameterId, scaledValue);
        }
    }
    profile->setProperty ("controls", controlsVar);

    juce::var layoutVar (new juce::DynamicObject());
    auto* layout = layoutVar.getDynamicObject();
    layout->setProperty ("outputLayout", getSnapshotOutputLayout());
    layout->setProperty ("outputChannels", getSnapshotOutputChannels());
    profile->setProperty ("layout", layoutVar);

    if (! validationSummary.isVoid())
        profile->setProperty ("validationSummary", validationSummary);

    return profileVar;
}

bool LocusQAudioProcessor::applyCalibrationProfileState (const juce::var& profileState)
{
    auto* profile = profileState.getDynamicObject();
    if (profile == nullptr)
        return false;

    if (profile->hasProperty ("schema"))
    {
        const auto schema = profile->getProperty ("schema").toString().trim();
        if (schema.isNotEmpty() && schema != kCalibrationProfileSchemaV1)
            return false;
    }

    auto* controls = profile->getProperty ("controls").getDynamicObject();
    if (controls == nullptr)
        return false;

    for (const auto& property : controls->getProperties())
    {
        const auto parameterId = property.name.toString();
        if (parameterId.isEmpty())
            continue;

        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        {
            const auto scaledValue = static_cast<float> (double (property.value));
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (scaledValue));
        }
    }

    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    setIntegerParameterValueNotifyingHost ("cal_spk_config", legacySpeakerConfigForTopologyIndex (topologyIndex));
    setIntegerParameterValueNotifyingHost ("rend_headphone_mode", (monitoringPath == 2 || monitoringPath == 3) ? 1 : 0);
    setIntegerParameterValueNotifyingHost ("rend_headphone_profile", deviceProfile);

    int rendererSpatialProfileIndex = 0;
    switch (topologyIndex)
    {
        case 0: rendererSpatialProfileIndex = 1; break;
        case 1: rendererSpatialProfileIndex = 1; break;
        case 2: rendererSpatialProfileIndex = 2; break;
        case 3: rendererSpatialProfileIndex = 3; break;
        case 4: rendererSpatialProfileIndex = 4; break;
        case 5: rendererSpatialProfileIndex = 4; break;
        case 6: rendererSpatialProfileIndex = 5; break;
        case 7: rendererSpatialProfileIndex = 9; break;
        case 8: rendererSpatialProfileIndex = 6; break;
        case 9: rendererSpatialProfileIndex = 7; break;
        case 10: rendererSpatialProfileIndex = 9; break;
        default: break;
    }
    setIntegerParameterValueNotifyingHost ("rend_spatial_profile", rendererSpatialProfileIndex);

    return true;
}

juce::var LocusQAudioProcessor::listEmitterPresetsFromUI() const
{
    juce::Array<juce::var> presets;
    const auto presetDir = getPresetDirectory();
    if (! presetDir.exists())
        return juce::var (presets);

    juce::Array<juce::File> files;
    presetDir.findChildFiles (files, juce::File::findFiles, false, "*.json");

    for (const auto& file : files)
    {
        juce::var entryVar (new juce::DynamicObject());
        auto* entry = entryVar.getDynamicObject();

        juce::String displayName = file.getFileNameWithoutExtension();
        juce::String choreographyPackId = "custom";
        juce::String presetType = kEmitterPresetTypeEmitter;
        if (const auto payload = readJsonFromFile (file))
        {
            if (auto* preset = payload->getDynamicObject())
            {
                if (preset->hasProperty ("name"))
                    displayName = preset->getProperty ("name").toString();

                if (preset->hasProperty ("choreographyPackId"))
                    choreographyPackId = normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString());

                presetType = inferPresetTypeFromPayload (*payload);
            }
        }

        entry->setProperty ("name", displayName);
        entry->setProperty ("file", file.getFileName());
        entry->setProperty ("path", file.getFullPathName());
        entry->setProperty ("modifiedUtc", file.getLastModificationTime().toISO8601 (true));
        entry->setProperty ("choreographyPackId", choreographyPackId);
        entry->setProperty ("presetType", presetType);
        presets.add (entryVar);
    }

    return juce::var (presets);
}

juce::var LocusQAudioProcessor::saveEmitterPresetFromUI (const juce::var& options)
{
    juce::String requestedName = "Preset";
    juce::String presetType = kEmitterPresetTypeEmitter;
    juce::String choreographyPackId = "custom";
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
        if (optionsObject->hasProperty ("presetType"))
            presetType = optionsObject->getProperty ("presetType").toString();
        if (optionsObject->hasProperty ("choreographyPackId"))
            choreographyPackId = optionsObject->getProperty ("choreographyPackId").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
        requestedName = "Preset_" + juce::String (juce::Time::getCurrentTime().toMilliseconds());

    presetType = normalisePresetType (presetType);
    choreographyPackId = normaliseChoreographyPackId (choreographyPackId);
    const auto includeParameters = presetType == kEmitterPresetTypeEmitter;
    const auto includeTimeline = presetType == kEmitterPresetTypeMotion;

    const auto safeName = sanitisePresetName (requestedName);
    auto presetDir = getPresetDirectory();
    presetDir.createDirectory();
    const auto presetFile = presetDir.getChildFile (safeName + ".json");

    juce::var presetPayload;
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        presetPayload = buildEmitterPresetLocked (requestedName, presetType, choreographyPackId, includeParameters, includeTimeline);
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! writeJsonToFile (presetFile, presetPayload))
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Failed to write preset file.");
        return response;
    }

    result->setProperty (locusq::shared_contracts::bridge_status::kOk, true);
    result->setProperty (locusq::shared_contracts::bridge_status::kName, requestedName);
    result->setProperty (locusq::shared_contracts::bridge_status::kFile, presetFile.getFileName());
    result->setProperty (locusq::shared_contracts::bridge_status::kPath, presetFile.getFullPathName());
    result->setProperty ("choreographyPackId", choreographyPackId);
    result->setProperty ("presetType", presetType);
    return response;
}

juce::var LocusQAudioProcessor::loadEmitterPresetFromUI (const juce::var& options)
{
    const auto presetFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! presetFile.existsAsFile())
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (presetFile);
    if (! payload.has_value())
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset file is invalid JSON.");
        return response;
    }

    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        if (! applyEmitterPresetLocked (*payload))
        {
            result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
            result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset payload is not compatible.");
            return response;
        }
    }

    result->setProperty (locusq::shared_contracts::bridge_status::kOk, true);
    result->setProperty (locusq::shared_contracts::bridge_status::kName, presetFile.getFileNameWithoutExtension());
    result->setProperty (locusq::shared_contracts::bridge_status::kFile, presetFile.getFileName());
    result->setProperty (locusq::shared_contracts::bridge_status::kPath, presetFile.getFullPathName());
    result->setProperty ("presetType", inferPresetTypeFromPayload (*payload));
    if (auto* preset = payload->getDynamicObject(); preset != nullptr
        && preset->hasProperty ("choreographyPackId"))
    {
        result->setProperty ("choreographyPackId",
                             normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString()));
    }
    else
    {
        result->setProperty ("choreographyPackId", "custom");
    }
    return response;
}

juce::var LocusQAudioProcessor::renameEmitterPresetFromUI (const juce::var& options)
{
    const auto sourceFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file not found.");
        return response;
    }

    juce::String requestedName;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("newName"))
            requestedName = optionsObject->getProperty ("newName").toString();
        else if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset name is required.");
        return response;
    }

    const auto safeName = sanitisePresetName (requestedName);
    const auto destinationFile = getPresetDirectory().getChildFile (safeName + ".json");
    const auto samePath = destinationFile.getFullPathName() == sourceFile.getFullPathName();

    if (! samePath && destinationFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset name already exists.");
        return response;
    }

    const auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file is invalid JSON.");
        return response;
    }

    auto updatedPayload = *payload;
    if (auto* preset = updatedPayload.getDynamicObject(); preset != nullptr)
    {
        preset->setProperty ("name", requestedName);
        preset->setProperty ("updatedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    }

    if (! writeJsonToFile (destinationFile, updatedPayload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write preset file.");
        return response;
    }

    if (! samePath)
        sourceFile.deleteFile();

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", destinationFile.getFileName());
    result->setProperty ("path", destinationFile.getFullPathName());
    result->setProperty ("presetType", inferPresetTypeFromPayload (updatedPayload));
    if (auto* preset = updatedPayload.getDynamicObject(); preset != nullptr
        && preset->hasProperty ("choreographyPackId"))
    {
        result->setProperty ("choreographyPackId",
                             normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString()));
    }
    else
    {
        result->setProperty ("choreographyPackId", "custom");
    }
    return response;
}

juce::var LocusQAudioProcessor::deleteEmitterPresetFromUI (const juce::var& options)
{
    const auto presetFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! presetFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file not found.");
        return response;
    }

    if (! presetFile.deleteFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to delete preset file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("file", presetFile.getFileName());
    result->setProperty ("path", presetFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::listCalibrationProfilesFromUI() const
{
    juce::Array<juce::var> profiles;
    const auto profileDir = getCalibrationProfileDirectory();
    if (! profileDir.exists())
        return juce::var (profiles);

    juce::Array<juce::File> files;
    profileDir.findChildFiles (files, juce::File::findFiles, false, "*.json");
    std::sort (files.begin(), files.end(), [] (const juce::File& lhs, const juce::File& rhs)
    {
        return lhs.getLastModificationTime() > rhs.getLastModificationTime();
    });

    for (const auto& file : files)
    {
        juce::var entryVar (new juce::DynamicObject());
        auto* entry = entryVar.getDynamicObject();

        juce::String displayName = file.getFileNameWithoutExtension();
        juce::String topologyId = calibrationTopologyIdForIndex (1);
        juce::String monitoringPathId = calibrationMonitoringPathIdForIndex (0);
        juce::String deviceProfileId = calibrationDeviceProfileIdForIndex (0);
        juce::var validationSummary;

        if (const auto payload = readJsonFromFile (file))
        {
            if (auto* profile = payload->getDynamicObject())
            {
                if (profile->hasProperty ("name"))
                    displayName = profile->getProperty ("name").toString();

                if (auto* context = profile->getProperty ("context").getDynamicObject())
                {
                    if (context->hasProperty ("topologyProfile"))
                        topologyId = normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString());
                    if (context->hasProperty ("monitoringPath"))
                        monitoringPathId = normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString());
                    if (context->hasProperty ("deviceProfile"))
                        deviceProfileId = normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString());
                }

                if (profile->hasProperty ("validationSummary"))
                    validationSummary = profile->getProperty ("validationSummary");
            }
        }

        entry->setProperty ("name", displayName);
        entry->setProperty ("file", file.getFileName());
        entry->setProperty ("path", file.getFullPathName());
        entry->setProperty ("modifiedUtc", file.getLastModificationTime().toISO8601 (true));
        entry->setProperty ("topologyProfile", topologyId);
        entry->setProperty ("monitoringPath", monitoringPathId);
        entry->setProperty ("deviceProfile", deviceProfileId);
        entry->setProperty ("profileTupleKey", topologyId + "::" + monitoringPathId);
        if (! validationSummary.isVoid())
            entry->setProperty ("validationSummary", validationSummary);
        profiles.add (entryVar);
    }

    return juce::var (profiles);
}

juce::var LocusQAudioProcessor::saveCalibrationProfileFromUI (const juce::var& options)
{
    juce::String requestedName;
    juce::var validationSummary;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
        if (optionsObject->hasProperty ("validationSummary"))
            validationSummary = optionsObject->getProperty ("validationSummary");
    }

    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPathIndex = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfileIndex = getCurrentCalibrationDeviceProfileIndex();
    const auto topologyId = calibrationTopologyIdForIndex (topologyIndex);
    const auto monitoringPathId = calibrationMonitoringPathIdForIndex (monitoringPathIndex);
    const auto deviceProfileId = calibrationDeviceProfileIdForIndex (deviceProfileIndex);

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
        requestedName = topologyId + "_" + monitoringPathId + "_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");

    const auto safeName = sanitisePresetName (requestedName);
    auto profileDir = getCalibrationProfileDirectory();
    profileDir.createDirectory();
    const auto profileFile = profileDir.getChildFile (safeName + ".json");
    const auto payload = buildCalibrationProfileState (requestedName, validationSummary);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! writeJsonToFile (profileFile, payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write calibration profile file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    result->setProperty ("topologyProfile", topologyId);
    result->setProperty ("monitoringPath", monitoringPathId);
    result->setProperty ("deviceProfile", deviceProfileId);
    result->setProperty ("profileTupleKey", topologyId + "::" + monitoringPathId);
    if (! validationSummary.isVoid())
        result->setProperty ("validationSummary", validationSummary);
    return response;
}

juce::var LocusQAudioProcessor::loadCalibrationProfileFromUI (const juce::var& options)
{
    const auto profileFile = resolveCalibrationProfileFileFromOptions (options);
    bool enforceTupleMatch = false;
    juce::String expectedTopologyId;
    juce::String expectedMonitoringPathId;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("enforceTupleMatch"))
            enforceTupleMatch = static_cast<bool> (optionsObject->getProperty ("enforceTupleMatch"));
        if (optionsObject->hasProperty ("topologyProfile"))
            expectedTopologyId = optionsObject->getProperty ("topologyProfile").toString();
        else if (optionsObject->hasProperty ("topologyProfileIndex"))
            expectedTopologyId = calibrationTopologyIdForIndex (static_cast<int> (optionsObject->getProperty ("topologyProfileIndex")));

        if (optionsObject->hasProperty ("monitoringPath"))
            expectedMonitoringPathId = optionsObject->getProperty ("monitoringPath").toString();
        else if (optionsObject->hasProperty ("monitoringPathIndex"))
            expectedMonitoringPathId = calibrationMonitoringPathIdForIndex (static_cast<int> (optionsObject->getProperty ("monitoringPathIndex")));
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! profileFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (profileFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file is invalid JSON.");
        return response;
    }

    auto loadedTopologyId = calibrationTopologyIdForIndex (getCurrentCalibrationTopologyProfileIndex());
    auto loadedMonitoringPathId = calibrationMonitoringPathIdForIndex (getCurrentCalibrationMonitoringPathIndex());
    auto loadedDeviceProfileId = calibrationDeviceProfileIdForIndex (getCurrentCalibrationDeviceProfileIndex());
    if (auto* profile = payload->getDynamicObject())
    {
        if (auto* context = profile->getProperty ("context").getDynamicObject())
        {
            if (context->hasProperty ("topologyProfile"))
                loadedTopologyId = normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString());
            if (context->hasProperty ("monitoringPath"))
                loadedMonitoringPathId = normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString());
            if (context->hasProperty ("deviceProfile"))
                loadedDeviceProfileId = normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString());
        }
    }

    if (expectedTopologyId.isEmpty())
        expectedTopologyId = calibrationTopologyIdForIndex (getCurrentCalibrationTopologyProfileIndex());
    if (expectedMonitoringPathId.isEmpty())
        expectedMonitoringPathId = calibrationMonitoringPathIdForIndex (getCurrentCalibrationMonitoringPathIndex());
    expectedTopologyId = normaliseCalibrationTopologyId (expectedTopologyId);
    expectedMonitoringPathId = normaliseCalibrationMonitoringPathId (expectedMonitoringPathId);

    if (enforceTupleMatch
        && (loadedTopologyId != expectedTopologyId
            || loadedMonitoringPathId != expectedMonitoringPathId))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message",
                             "Calibration profile tuple mismatch (profile="
                                 + loadedTopologyId + "/"
                                 + loadedMonitoringPathId + ", current="
                                 + expectedTopologyId + "/"
                                 + expectedMonitoringPathId + ").");
        return response;
    }

    if (! applyCalibrationProfileState (*payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile payload is not compatible.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("name", profileFile.getFileNameWithoutExtension());
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    result->setProperty ("topologyProfile", loadedTopologyId);
    result->setProperty ("monitoringPath", loadedMonitoringPathId);
    result->setProperty ("deviceProfile", loadedDeviceProfileId);
    result->setProperty ("profileTupleKey", loadedTopologyId + "::" + loadedMonitoringPathId);
    if (auto* profile = payload->getDynamicObject())
    {
        if (profile->hasProperty ("name"))
            result->setProperty ("name", profile->getProperty ("name").toString());

        if (auto* context = profile->getProperty ("context").getDynamicObject())
        {
            if (context->hasProperty ("topologyProfile"))
                result->setProperty ("topologyProfile", normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString()));
            if (context->hasProperty ("monitoringPath"))
                result->setProperty ("monitoringPath", normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString()));
            if (context->hasProperty ("deviceProfile"))
                result->setProperty ("deviceProfile", normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString()));
        }

        if (profile->hasProperty ("validationSummary"))
            result->setProperty ("validationSummary", profile->getProperty ("validationSummary"));
    }

    return response;
}

juce::var LocusQAudioProcessor::renameCalibrationProfileFromUI (const juce::var& options)
{
    const auto sourceFile = resolveCalibrationProfileFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    juce::String requestedName;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("newName"))
            requestedName = optionsObject->getProperty ("newName").toString();
        else if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile name is required.");
        return response;
    }

    const auto safeName = sanitisePresetName (requestedName);
    const auto destinationFile = getCalibrationProfileDirectory().getChildFile (safeName + ".json");
    const auto samePath = destinationFile.getFullPathName() == sourceFile.getFullPathName();

    if (! samePath && destinationFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile name already exists.");
        return response;
    }

    const auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file is invalid JSON.");
        return response;
    }

    auto updatedPayload = *payload;
    if (auto* profile = updatedPayload.getDynamicObject(); profile != nullptr)
    {
        profile->setProperty ("name", requestedName);
        profile->setProperty ("updatedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    }

    if (! writeJsonToFile (destinationFile, updatedPayload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write calibration profile file.");
        return response;
    }

    if (! samePath)
        sourceFile.deleteFile();

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", destinationFile.getFileName());
    result->setProperty ("path", destinationFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::deleteCalibrationProfileFromUI (const juce::var& options)
{
    const auto profileFile = resolveCalibrationProfileFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! profileFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    if (! profileFile.deleteFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to delete calibration profile file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::getUIStateFromUI() const
{
    juce::var stateVar (new juce::DynamicObject());
    auto* state = stateVar.getDynamicObject();

    juce::String emitterLabelSnapshot;
    juce::String physicsPresetSnapshot;
    juce::String choreographyPackSnapshot;
    {
        const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
        emitterLabelSnapshot = emitterLabelState;
        physicsPresetSnapshot = physicsPresetState;
        choreographyPackSnapshot = choreographyPackState;
    }

    if (emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId))
    {
        const auto slotData = sceneGraph.getSlot (emitterSlotId).read();
        const auto slotLabel = juce::String::fromUTF8 (slotData.label).trim();
        if (slotLabel.isNotEmpty())
            emitterLabelSnapshot = slotLabel;
    }

    if (physicsPresetSnapshot.isEmpty())
        physicsPresetSnapshot = "off";
    if (choreographyPackSnapshot.isEmpty())
        choreographyPackSnapshot = "custom";

    state->setProperty ("emitterLabel", sanitiseEmitterLabel (emitterLabelSnapshot));
    state->setProperty ("physicsPreset", physicsPresetSnapshot);
    state->setProperty ("choreographyPack", normaliseChoreographyPackId (choreographyPackSnapshot));
    return stateVar;
}

bool LocusQAudioProcessor::setUIStateFromUI (const juce::var& stateVar)
{
    auto* state = stateVar.getDynamicObject();
    if (state == nullptr)
        return false;

    bool changed = false;

    if (state->hasProperty ("emitterLabel"))
    {
        const auto nextLabel = sanitiseEmitterLabel (state->getProperty ("emitterLabel").toString());
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            emitterLabelState = nextLabel;
        }
        emitterLabelRtState.store (std::make_shared<juce::String> (nextLabel));
        applyEmitterLabelToSceneSlotIfAvailable (nextLabel);
        changed = true;
    }

    if (state->hasProperty ("physicsPreset"))
    {
        auto preset = state->getProperty ("physicsPreset").toString().trim().toLowerCase();
        if (preset != "off" && preset != "bounce" && preset != "float" && preset != "orbit")
            preset = "custom";

        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            physicsPresetState = preset;
        }

        changed = true;
    }

    if (state->hasProperty ("choreographyPack"))
    {
        const auto choreographyPack = normaliseChoreographyPackId (state->getProperty ("choreographyPack").toString());
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            choreographyPackState = choreographyPack;
        }
        changed = true;
    }

    return changed;
}
