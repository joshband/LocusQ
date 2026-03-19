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

    const int rendererSpatialProfileIndex =
        locusq::shared_contracts::calibration_registry::topologyRendererSpatialProfileIndexForIndex (topologyProfile);
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

juce::var LocusQAudioProcessor::applyBestCalibrationOutputMapFromUI()
{
    const auto snapshotOutputChannels = getSnapshotOutputChannels();
    const auto layoutOutputChannels = static_cast<int> (getBusesLayout().getMainOutputChannelSet().size());
    const auto previousSpeakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto previousTopologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto previousRouting = getCurrentCalibrationSpeakerRouting();
    const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
        snapshotOutputChannels,
        layoutOutputChannels,
        lastAutoDetectedOutputChannels,
        previousRouting);

    const auto clampedOutputChannels = juce::jlimit (1, 16, effectiveWritableChannels);
    std::array<int, SpatialRenderer::NUM_SPEAKERS> autoRouting { 1, 2, 3, 4 };

    if (clampedOutputChannels == 1)
        autoRouting = { 1, 1, 1, 1 };
    else if (clampedOutputChannels == 2)
        autoRouting = { 1, 2, 1, 2 };
    else if (clampedOutputChannels == 3)
        autoRouting = { 1, 2, 3, 3 };

    setIntegerParameterValueNotifyingHost ("cal_spk1_out", autoRouting[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", autoRouting[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", autoRouting[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", autoRouting[3]);

    juce::var resultVar (new juce::DynamicObject());
    auto* result = resultVar.getDynamicObject();
    if (result == nullptr)
        return resultVar;

    result->setProperty ("ok", true);
    result->setProperty ("action", "apply_best_output_map");
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

    const bool changed = map != previousRouting;
    result->setProperty ("changed", changed);
    result->setProperty (
        "message",
        changed
            ? "Applied best output map from current writable calibration routes."
            : "Best output map is already active.");

    return resultVar;
}

juce::var LocusQAudioProcessor::applyBestCalibrationTopologyFromUI()
{
    auto resultVar = redetectCalibrationRoutingFromUI();
    if (auto* result = resultVar.getDynamicObject())
    {
        result->setProperty ("action", "apply_best_topology");
        result->setProperty (
            "message",
            static_cast<bool> (result->getProperty ("changed"))
                ? "Applied best topology and aligned the host-derived output map."
                : "Best topology and host-derived output map are already active.");
    }
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
    const auto snapshotOutputChannels = getSnapshotOutputChannels();
    const auto layoutOutputChannels = static_cast<int> (getBusesLayout().getMainOutputChannelSet().size());
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
    PublishedHeadphoneCalibrationDiagnostics publishedCalibration;
    PublishedHeadphoneVerificationDiagnostics publishedVerification;
    if (copyPublishedHeadphoneDiagnosticsSnapshot (publishedCalibration, publishedVerification))
    {
        if (publishedCalibration.valid)
        {
            profileSyncSeq = static_cast<juce::int64> (publishedCalibration.profileSyncSeq);
            headphoneCalibration.requested = publishedCalibration.requested.toString();
            headphoneCalibration.active = publishedCalibration.active.toString();
            headphoneCalibration.stage = publishedCalibration.stage.toString();
            headphoneCalibration.fallbackReady = publishedCalibration.fallbackReady;
            headphoneCalibration.fallbackReason = publishedCalibration.fallbackReason.toString();
        }

        if (publishedVerification.valid)
        {
            profileSyncSeq = static_cast<juce::int64> (publishedVerification.profileSyncSeq);
            headphoneVerification.profileId = publishedVerification.profileId.toString();
            headphoneVerification.requestedProfileId =
                publishedVerification.requestedProfileId.toString();
            headphoneVerification.activeProfileId =
                publishedVerification.activeProfileId.toString();
            headphoneVerification.requestedEngineId =
                publishedVerification.requestedEngineId.toString();
            headphoneVerification.activeEngineId =
                publishedVerification.activeEngineId.toString();
            headphoneVerification.fallbackReasonCode =
                publishedVerification.fallbackReasonCode.toString();
            headphoneVerification.fallbackTarget =
                publishedVerification.fallbackTarget.toString();
            headphoneVerification.fallbackReasonText =
                publishedVerification.fallbackReasonText.toString();
            headphoneVerification.frontBackScore =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedVerification.frontBackScore,
                    0.0f);
            headphoneVerification.elevationScore =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedVerification.elevationScore,
                    0.0f);
            headphoneVerification.externalizationScore =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedVerification.externalizationScore,
                    0.0f);
            headphoneVerification.confidence =
                locusq::shared_contracts::headphone_verification::sanitizeScore (
                    publishedVerification.confidence,
                    0.0f);
            headphoneVerification.verificationStage =
                publishedVerification.verificationStage.toString();
            headphoneVerification.verificationScoreStatus =
                publishedVerification.verificationScoreStatus.toString();
            headphoneVerification.scoreProvenance =
                publishedVerification.scoreProvenance.toString();
            headphoneVerification.compensationLabel =
                publishedVerification.compensationLabel.toString();
            headphoneVerification.compensationProvenance =
                publishedVerification.compensationProvenance.toString();
            headphoneVerification.chainLatencySamples =
                locusq::shared_contracts::headphone_verification::sanitizeLatencySamples (
                    publishedVerification.chainLatencySamples);
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
        locusq::shared_contracts::headphone_verification::scoreStatusFromProvenance (
            headphoneVerification.scoreProvenance);
    headphoneVerification.scoreProvenance =
        locusq::shared_contracts::headphone_verification::sanitizeProvenance (
            headphoneVerification.scoreProvenance);
    headphoneVerification.compensationProvenance =
        locusq::shared_contracts::headphone_verification::sanitizeProvenance (
            headphoneVerification.compensationProvenance);
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

    auto buildCalibrationRegistryCatalog = []() -> juce::var
    {
        juce::var catalogVar (new juce::DynamicObject());
        auto* catalog = catalogVar.getDynamicObject();
        if (catalog == nullptr)
            return catalogVar;

        juce::Array<juce::var> topologies;
        topologies.ensureStorageAllocated (static_cast<int> (locusq::shared_contracts::calibration_registry::kTopologyCatalog.size()));
        for (const auto& entry : locusq::shared_contracts::calibration_registry::kTopologyCatalog)
        {
            juce::var topologyVar (new juce::DynamicObject());
            if (auto* topology = topologyVar.getDynamicObject())
            {
                topology->setProperty ("id", entry.id);
                topology->setProperty ("label", entry.label);
                topology->setProperty ("shortLabel", entry.shortLabel);
                topology->setProperty ("requiredChannels", entry.requiredChannels);
                topology->setProperty ("headphoneTarget", entry.headphoneTarget);
                topology->setProperty ("rendererSpatialProfileIndex", entry.rendererSpatialProfileIndex);
                topology->setProperty ("legacySpeakerConfigIndex", entry.legacySpeakerConfigIndex);
                topology->setProperty ("capabilityNote", entry.capabilityNote);

                juce::Array<juce::var> previewSpeakerPositions;
                for (int previewIndex = 0; previewIndex < entry.previewSpeakerPositionCount; ++previewIndex)
                {
                    const auto base = previewIndex * 3;
                    juce::var previewVar (new juce::DynamicObject());
                    if (auto* preview = previewVar.getDynamicObject())
                    {
                        preview->setProperty ("x", entry.previewSpeakerPositions[static_cast<size_t> (base)]);
                        preview->setProperty ("y", entry.previewSpeakerPositions[static_cast<size_t> (base + 1)]);
                        preview->setProperty ("z", entry.previewSpeakerPositions[static_cast<size_t> (base + 2)]);
                    }
                    previewSpeakerPositions.add (previewVar);
                }
                topology->setProperty ("previewSpeakerPositions", previewSpeakerPositions);

                juce::Array<juce::var> roleLabels;
                for (int roleIndex = 0; roleIndex < entry.roleLabelCount; ++roleIndex)
                    roleLabels.add (entry.roleLabels[static_cast<size_t> (roleIndex)]);
                topology->setProperty ("roleLabels", roleLabels);

                juce::Array<juce::var> aliases;
                for (int aliasIndex = 0; aliasIndex < entry.aliasCount; ++aliasIndex)
                    aliases.add (entry.aliases[static_cast<size_t> (aliasIndex)]);
                topology->setProperty ("aliases", aliases);
            }
            topologies.add (topologyVar);
        }
        catalog->setProperty ("topologies", topologies);

        juce::Array<juce::var> monitoringPaths;
        monitoringPaths.ensureStorageAllocated (static_cast<int> (locusq::shared_contracts::calibration_registry::kMonitoringPathCatalog.size()));
        for (const auto& entry : locusq::shared_contracts::calibration_registry::kMonitoringPathCatalog)
        {
            juce::var pathVar (new juce::DynamicObject());
            if (auto* path = pathVar.getDynamicObject())
            {
                path->setProperty ("id", entry.id);
                path->setProperty ("label", entry.label);
                path->setProperty ("targetId", entry.targetId);
                path->setProperty ("headphoneTarget", entry.headphoneTarget);
                path->setProperty ("capabilityNote", entry.capabilityNote);

                juce::Array<juce::var> aliases;
                for (int aliasIndex = 0; aliasIndex < entry.aliasCount; ++aliasIndex)
                    aliases.add (entry.aliases[static_cast<size_t> (aliasIndex)]);
                path->setProperty ("aliases", aliases);
            }
            monitoringPaths.add (pathVar);
        }
        catalog->setProperty ("monitoringPaths", monitoringPaths);

        juce::Array<juce::var> deviceProfiles;
        deviceProfiles.ensureStorageAllocated (static_cast<int> (locusq::shared_contracts::calibration_registry::kDeviceProfileCatalog.size()));
        for (const auto& entry : locusq::shared_contracts::calibration_registry::kDeviceProfileCatalog)
        {
            juce::var profileVar (new juce::DynamicObject());
            if (auto* profile = profileVar.getDynamicObject())
            {
                profile->setProperty ("id", entry.id);
                profile->setProperty ("label", entry.label);
                profile->setProperty ("family", entry.family);
                profile->setProperty ("familyLabel", entry.familyLabel);
                profile->setProperty ("companionProfileCapable", entry.companionProfileCapable);
                profile->setProperty ("personalizedHrtfCapable", entry.personalizedHrtfCapable);
                profile->setProperty ("headTrackingCapable", entry.headTrackingCapable);
                profile->setProperty ("capabilityNote", entry.capabilityNote);

                juce::Array<juce::var> aliases;
                for (int aliasIndex = 0; aliasIndex < entry.aliasCount; ++aliasIndex)
                    aliases.add (entry.aliases[static_cast<size_t> (aliasIndex)]);
                profile->setProperty ("aliases", aliases);
            }
            deviceProfiles.add (profileVar);
        }
        catalog->setProperty ("deviceProfiles", deviceProfiles);
        catalog->setProperty ("version", "bl103-waveA-v1");
        return catalogVar;
    };

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
    status->setProperty ("registryCatalog", buildCalibrationRegistryCatalog());
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
    status->setProperty ("headphoneVerificationScoreProvenance", headphoneVerification.scoreProvenance);
    status->setProperty ("headphoneVerificationCompensationLabel", headphoneVerification.compensationLabel);
    status->setProperty ("headphoneVerificationCompensationProvenance", headphoneVerification.compensationProvenance);
    status->setProperty (
        "headphoneVerificationLatencySamples",
        locusq::shared_contracts::headphone_verification::sanitizeLatencySamples (
            headphoneVerification.chainLatencySamples));
    status->setProperty ("requiredChannels", requiredChannels);
    status->setProperty ("writableChannels", writableChannels);
    status->setProperty ("mappingLimitedToFirst4", mappingLimitedToFirst4);
    status->setProperty ("mappingDuplicateChannels", mappingDuplicateChannels);
    status->setProperty ("mappingValid", mappingValid);
    const auto layoutInputChannels = static_cast<int> (getBusesLayout().getMainInputChannelSet().size());
    const bool hostInputVisible = layoutInputChannels > 0;
    const int selectedMicChannel = juce::jlimit (
        1,
        8,
        static_cast<int> (apvts.getRawParameterValue ("cal_mic_channel")->load()));
    const bool selectedMicVisible = hostInputVisible && selectedMicChannel <= layoutInputChannels;

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

    const auto makeBl101Descriptor = [] (const juce::String& source,
                                         const juce::String& provenance,
                                         const juce::String& detail,
                                         bool manualOverride = false,
                                         std::optional<double> ageMs = std::nullopt,
                                         std::optional<double> staleAfterMs = std::nullopt)
    {
        juce::var descriptorVar (new juce::DynamicObject());
        if (auto* descriptor = descriptorVar.getDynamicObject())
        {
            descriptor->setProperty ("source", source);
            descriptor->setProperty ("provenance", provenance);
            descriptor->setProperty ("detail", detail);
            descriptor->setProperty ("manualOverride", manualOverride);

            if (ageMs.has_value())
                descriptor->setProperty ("ageMs", *ageMs);
            if (staleAfterMs.has_value())
                descriptor->setProperty ("staleAfterMs", *staleAfterMs);

            const bool isStale = ageMs.has_value()
                && staleAfterMs.has_value()
                && std::isfinite (*ageMs)
                && std::isfinite (*staleAfterMs)
                && *staleAfterMs >= 0.0
                && *ageMs >= *staleAfterMs;
            descriptor->setProperty ("isStale", isStale);
        }
        return descriptorVar;
    };

    {
        juce::var provenanceVar (new juce::DynamicObject());
        if (auto* provenance = provenanceVar.getDynamicObject())
        {
            provenance->setProperty (
                "topology",
                makeBl101Descriptor ("runtime_state",
                                     "inferred",
                                     "Reflects the current CALIBRATE topology selection in runtime state."));
            provenance->setProperty (
                "monitoringPath",
                makeBl101Descriptor ("runtime_state",
                                     "inferred",
                                     "Reflects the current CALIBRATE monitoring path in runtime state."));
            provenance->setProperty (
                "deviceProfile",
                makeBl101Descriptor ("runtime_state",
                                     "inferred",
                                     "Reflects the current CALIBRATE device-profile selection in runtime state."));
            provenance->setProperty (
                "routing",
                makeBl101Descriptor ("host_auto",
                                     mappingValid ? "detected" : "inferred",
                                     mappingDuplicateChannels
                                         ? "Routing contains duplicate channels and requires operator correction."
                                         : (mappingLimitedToFirst4
                                             ? "Host output layout exposes only a limited writable calibration map."
                                             : "Routing is derived from the current host output layout and writable-channel visibility.")));
            provenance->setProperty (
                "profile",
                makeBl101Descriptor (roomProfile != nullptr && roomProfile->valid ? "runtime_active" : "unknown",
                                     roomProfile != nullptr && roomProfile->valid ? "inferred" : "unavailable",
                                     roomProfile != nullptr && roomProfile->valid
                                         ? "A room profile is available to the runtime."
                                         : "No active room profile is currently available."));
            provenance->setProperty (
                "headphoneVerify",
                makeBl101Descriptor ("runtime_active",
                                     "estimated",
                                     "Current headphone verification metrics are runtime estimates unless stronger evidence is published by BL-099/BL-060 follow-on lanes."));
        }
        status->setProperty ("calAutomationProvenance", provenanceVar);
    }

    {
        juce::var discoveryGraphVar (new juce::DynamicObject());
        if (auto* discoveryGraph = discoveryGraphVar.getDynamicObject())
        {
            discoveryGraph->setProperty ("schema", "locusq-calibrate-discovery-graph-v1");

            const auto topologyLabelForId = [] (const juce::String& topologyId)
            {
                const auto normalized = topologyId.trim().toLowerCase();
                if (normalized == "mono") return juce::String ("Mono");
                if (normalized == "stereo") return juce::String ("Stereo");
                if (normalized == "quad") return juce::String ("Quad");
                if (normalized == "surround_51") return juce::String ("5.1");
                if (normalized == "surround_71") return juce::String ("7.1");
                if (normalized == "surround_712") return juce::String ("7.1.2");
                if (normalized == "surround_742") return juce::String ("7.4.2 / Atmos-style");
                if (normalized == "binaural") return juce::String ("Binaural / Headphone");
                if (normalized == "ambisonic_1st") return juce::String ("Ambisonic 1st Order");
                if (normalized == "ambisonic_3rd") return juce::String ("Ambisonic 3rd Order");
                if (normalized == "downmix_stereo") return juce::String ("Multichannel -> Stereo Downmix");
                return topologyId;
            };
            const auto topologyRolesForId = [] (const juce::String& topologyId)
            {
                const auto normalized = topologyId.trim().toLowerCase();
                if (normalized == "mono") return juce::StringArray { "Main" };
                if (normalized == "stereo") return juce::StringArray { "L", "R" };
                if (normalized == "quad") return juce::StringArray { "FL", "FR", "RL", "RR" };
                if (normalized == "surround_51") return juce::StringArray { "L", "R", "C", "LFE", "Ls", "Rs" };
                if (normalized == "surround_71") return juce::StringArray { "L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs" };
                if (normalized == "surround_712") return juce::StringArray { "L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs", "TopL", "TopR" };
                if (normalized == "surround_742") return juce::StringArray { "L", "R", "C", "LFE1", "LFE2", "Ls", "Rs", "Lrs", "Rrs", "TopFL", "TopFR", "TopRL", "TopRR" };
                if (normalized == "binaural") return juce::StringArray { "Left", "Right" };
                if (normalized == "ambisonic_1st") return juce::StringArray { "W", "X", "Y", "Z" };
                if (normalized == "ambisonic_3rd")
                {
                    juce::StringArray labels;
                    for (int acn = 0; acn < 16; ++acn)
                        labels.add ("ACN" + juce::String (acn));
                    return labels;
                }
                if (normalized == "downmix_stereo") return juce::StringArray { "Downmix L", "Downmix R" };
                return juce::StringArray { "Ch 1", "Ch 2", "Ch 3", "Ch 4" };
            };
            const auto splitLayoutTokens = [] (const juce::String& layoutText)
            {
                juce::StringArray tokens;
                tokens.addTokens (layoutText, " ", "");
                tokens.removeEmptyStrings();
                return tokens;
            };
            const auto autoRoutingForWritableChannels = [] (int writableChannelCount)
            {
                const auto clampedOutputChannels = juce::jlimit (1, 16, writableChannelCount);
                std::array<int, SpatialRenderer::NUM_SPEAKERS> autoRouting { 1, 2, 3, 4 };

                if (clampedOutputChannels == 1)
                    autoRouting = { 1, 1, 1, 1 };
                else if (clampedOutputChannels == 2)
                    autoRouting = { 1, 2, 1, 2 };
                else if (clampedOutputChannels == 3)
                    autoRouting = { 1, 2, 3, 3 };

                return autoRouting;
            };
            const auto addRoleAssignments = [&] (juce::DynamicObject* candidate,
                                                 const juce::StringArray& labels,
                                                 int mappedAssignments,
                                                 auto&& outputChannelResolver,
                                                 auto&& preferredOutputResolver,
                                                 const juce::String& provenance,
                                                 const juce::String& detail,
                                                 const juce::String& blockedReason,
                                                 const juce::String& preferredOutputProvenance,
                                                 const juce::String& preferredOutputDetail)
            {
                if (candidate == nullptr)
                    return;

                juce::Array<juce::var> assignments;
                const auto assignmentCount = static_cast<int> (labels.size());
                int mappedCount = 0;
                for (int index = 0; index < assignmentCount; ++index)
                {
                    juce::var assignmentVar (new juce::DynamicObject());
                    if (auto* assignment = assignmentVar.getDynamicObject())
                    {
                        const bool isMapped = index < mappedAssignments;
                        const int outputChannel = isMapped ? juce::jmax (0, outputChannelResolver (index)) : 0;
                        const int preferredOutputChannel = juce::jmax (0, preferredOutputResolver (labels[index], index));
                        assignment->setProperty ("label", labels[index]);
                        assignment->setProperty ("outputChannel", outputChannel);
                        assignment->setProperty ("mapped", isMapped && outputChannel > 0);
                        assignment->setProperty ("blocked", ! isMapped || outputChannel <= 0);
                        assignment->setProperty ("blockedReason", ! isMapped || outputChannel <= 0 ? blockedReason : juce::String());
                        assignment->setProperty ("provenance", provenance);
                        assignment->setProperty ("detail", detail);
                        assignment->setProperty ("preferredOutputChannel", preferredOutputChannel);
                        assignment->setProperty ("preferredOutputProvenance", preferredOutputChannel > 0 ? preferredOutputProvenance : juce::String());
                        assignment->setProperty ("preferredOutputDetail", preferredOutputChannel > 0 ? preferredOutputDetail : juce::String());
                    }
                    if (const auto* assignment = assignmentVar.getDynamicObject())
                        if (static_cast<bool> (assignment->getProperty ("mapped")))
                            ++mappedCount;
                    assignments.add (assignmentVar);
                }

                candidate->setProperty ("roleAssignments", juce::var (assignments));
                candidate->setProperty ("roleAssignmentProvenance", provenance);
                candidate->setProperty ("roleAssignmentDetail", detail);
                candidate->setProperty ("roleIntentMappedCount", mappedCount);
                candidate->setProperty ("roleIntentTotalCount", assignmentCount);
                candidate->setProperty ("roleIntentBlockedCount", juce::jmax (0, assignmentCount - mappedCount));
                candidate->setProperty ("roleIntentComplete", mappedCount >= assignmentCount);
            };

            const auto snapshotOutputLayout = outputLayoutToString (getBusesLayout().getMainOutputChannelSet());
            const auto snapshotOutputRoles = splitLayoutTokens (snapshotOutputLayout);
            const auto findPreferredOutputChannelForRole = [&] (const juce::String& roleLabel, int fallbackIndex)
            {
                const auto normalizedRole = roleLabel.trim().toLowerCase();
                for (int roleIndex = 0; roleIndex < snapshotOutputRoles.size(); ++roleIndex)
                {
                    if (snapshotOutputRoles[roleIndex].trim().toLowerCase() == normalizedRole)
                        return roleIndex + 1;
                }

                if (snapshotOutputChannels > 0 && fallbackIndex >= 0 && fallbackIndex < snapshotOutputChannels)
                    return fallbackIndex + 1;
                if (layoutOutputChannels > 0 && fallbackIndex >= 0 && fallbackIndex < layoutOutputChannels)
                    return fallbackIndex + 1;
                return 0;
            };
            const auto autoTopologyId = calibrationTopologyIdForIndex (lastAutoDetectedTopologyProfile);
            const auto selectedTopologyId = calibrationTopologyIdForIndex (topologyProfile);
            const bool hostOutputVisible = snapshotOutputChannels > 0 || layoutOutputChannels > 0;
            const bool topologyMismatch = selectedTopologyId != autoTopologyId;
            const bool limitedCandidate = requiredChannels > writableChannels;
            const auto currentAutoRouting = autoRoutingForWritableChannels (writableChannels);

            juce::Array<juce::var> outputCandidates;
            {
                auto addOutputCandidate = [&] (const juce::String& id,
                                               const juce::String& label,
                                               int channelCount,
                                               int candidateWritableChannels,
                                               int rank,
                                               double confidence,
                                               const juce::String& detail,
                                               const juce::String& provenance,
                                               bool manualOverride = false)
                {
                    juce::var candidateVar (new juce::DynamicObject());
                    if (auto* candidate = candidateVar.getDynamicObject())
                    {
                        candidate->setProperty ("id", id);
                        candidate->setProperty ("kind", "output");
                        candidate->setProperty ("label", label);
                        candidate->setProperty ("layout", snapshotOutputLayout);
                        candidate->setProperty ("channelCount", channelCount);
                        candidate->setProperty ("layoutChannelCount", layoutOutputChannels);
                        candidate->setProperty ("writableChannels", candidateWritableChannels);
                        candidate->setProperty ("rank", rank);
                        candidate->setProperty ("confidence", confidence);
                        candidate->setProperty ("selected", rank == 1);
                        candidate->setProperty ("candidateVisible", channelCount > 0);
                        candidate->setProperty ("needsConfirmation", mappingDuplicateChannels || candidateWritableChannels < channelCount);
                        candidate->setProperty (
                            "descriptor",
                            makeBl101Descriptor ("host_auto",
                                                 provenance,
                                                 detail,
                                                 manualOverride));
                        if (id == "host_writable_output")
                        {
                            const auto topologyRoles = topologyRolesForId (selectedTopologyId);
                            const auto requiredRoleCount = getRequiredCalibrationChannelsForTopologyIndex (topologyProfile);
                            addRoleAssignments (candidate,
                                                topologyRoles,
                                                juce::jmin (requiredRoleCount, SpatialRenderer::NUM_SPEAKERS),
                                                [&] (int index)
                                                {
                                                    return juce::jlimit (1, 8, currentAutoRouting[static_cast<size_t> (index)]);
                                                },
                                                [&] (const juce::String& roleLabel, int index)
                                                {
                                                    return findPreferredOutputChannelForRole (roleLabel, index);
                                                },
                                                limitedCandidate ? "generic" : "inferred",
                                                limitedCandidate
                                                    ? "Role guesses preserve the current topology selection but collapse onto the limited writable calibration map."
                                                    : "Role guesses preserve the current topology selection and follow the best writable calibration map.",
                                                "limited_writable_map",
                                                snapshotOutputRoles.isEmpty() ? "inferred" : "detected",
                                                snapshotOutputRoles.isEmpty()
                                                    ? "Preferred reroute target is inferred from host output width order."
                                                    : "Preferred reroute target comes from the host output role layout.");
                        }
                        else if (id == "host_main_output")
                        {
                            auto fallbackOutputLabels = snapshotOutputRoles;
                            if (fallbackOutputLabels.isEmpty())
                            {
                                for (int channelIndex = 1; channelIndex <= juce::jmax (1, channelCount); ++channelIndex)
                                    fallbackOutputLabels.add ("Out " + juce::String (channelIndex));
                            }
                            addRoleAssignments (candidate,
                                                fallbackOutputLabels,
                                                channelCount,
                                                [] (int index) { return index + 1; },
                                                [] (const juce::String&, int index) { return index + 1; },
                                                snapshotOutputRoles.isEmpty() ? "generic" : "detected",
                                                snapshotOutputRoles.isEmpty()
                                                    ? "Host output layout did not expose named speaker roles, so generic output channels are shown."
                                                    : "Role guesses come directly from the host output layout token order.",
                                                "host_output_unmapped",
                                                snapshotOutputRoles.isEmpty() ? "inferred" : "detected",
                                                snapshotOutputRoles.isEmpty()
                                                    ? "Preferred reroute target follows generic host output ordering."
                                                    : "Preferred reroute target comes directly from the host output layout.");
                        }
                    }
                    outputCandidates.add (candidateVar);
                };

                if (! hostOutputVisible)
                {
                    addOutputCandidate ("host_main_output",
                                        "Host Main Output",
                                        0,
                                        0,
                                        1,
                                        0.15,
                                        "Host output layout has not published usable channel visibility yet.",
                                        "unavailable");
                }
                else
                {
                    addOutputCandidate ("host_writable_output",
                                        "Writable Calibration Routes",
                                        writableChannels,
                                        writableChannels,
                                        1,
                                        0.92,
                                        "Current writable calibration routes are the strongest output candidate for measurement.",
                                        "detected");

                    if (snapshotOutputChannels != writableChannels || layoutOutputChannels != writableChannels)
                    {
                        addOutputCandidate ("host_main_output",
                                            "Host Main Output",
                                            snapshotOutputChannels,
                                            writableChannels,
                                            2,
                                            0.70,
                                            "Host output layout exposes more visible channels than the current writable calibration map.",
                                            "inferred");
                    }
                }
            }
            discoveryGraph->setProperty ("outputCandidates", juce::var (outputCandidates));

            juce::Array<juce::var> inputCandidates;
            {
                auto addInputCandidate = [&] (int candidateMicChannel,
                                              int rank,
                                              bool candidateSelected,
                                              bool candidateVisible,
                                              double confidence,
                                              const juce::StringArray& reasonCodeValues,
                                              const juce::StringArray& blockedByValues,
                                              const juce::String& confirmationPrompt,
                                              const juce::String& detail,
                                              const juce::String& provenance)
                {
                    juce::var candidateVar (new juce::DynamicObject());
                    if (auto* candidate = candidateVar.getDynamicObject())
                    {
                        juce::Array<juce::var> reasonCodes;
                        juce::Array<juce::var> blockedBy;
                        for (const auto& reason : reasonCodeValues)
                            reasonCodes.add (reason);
                        for (const auto& blocked : blockedByValues)
                            blockedBy.add (blocked);

                        candidate->setProperty ("id", "host_input_ch_" + juce::String (candidateMicChannel));
                        candidate->setProperty ("kind", "input");
                        candidate->setProperty ("label", "Host Input CH " + juce::String (candidateMicChannel));
                        candidate->setProperty ("channelCount", layoutInputChannels);
                        candidate->setProperty ("candidateMicChannel", candidateMicChannel);
                        candidate->setProperty ("selectedMicChannel", selectedMicChannel);
                        candidate->setProperty ("selected", candidateSelected);
                        candidate->setProperty ("selectedMicVisible", selectedMicVisible);
                        candidate->setProperty ("candidateVisible", candidateVisible);
                        candidate->setProperty ("recommendedMicChannel", hostInputVisible ? 1 : 0);
                        candidate->setProperty ("rank", rank);
                        candidate->setProperty ("confidence", confidence);
                        candidate->setProperty ("needsConfirmation", ! candidateVisible || ! selectedMicVisible);
                        candidate->setProperty ("reasonCodes", juce::var (reasonCodes));
                        candidate->setProperty ("confirmationPrompt", confirmationPrompt);
                        candidate->setProperty ("blockedBy", juce::var (blockedBy));
                        candidate->setProperty (
                            "descriptor",
                            makeBl101Descriptor ("host_auto",
                                                 provenance,
                                                 detail,
                                                 candidateSelected && candidateMicChannel != 1));
                    }
                    inputCandidates.add (candidateVar);
                };

                if (! hostInputVisible)
                {
                    addInputCandidate (selectedMicChannel,
                                       1,
                                       true,
                                       false,
                                       0.10,
                                       { "host_input_unavailable" },
                                       { "host_input_visibility" },
                                       "Wait for the host to expose an input bus before trusting mic discovery.",
                                       "Host input bus has not published usable channel visibility yet.",
                                       "unavailable");
                }
                else
                {
                    for (int micChannel = 1; micChannel <= layoutInputChannels; ++micChannel)
                    {
                        const bool candidateSelected = micChannel == selectedMicChannel;
                        juce::StringArray reasonCodes;
                        if (micChannel == 1)
                            reasonCodes.add ("auto_ranked_best");
                        if (candidateSelected)
                            reasonCodes.add ("selected_mic_addressable");
                        else
                            reasonCodes.add ("alternate_visible_input");
                        if (layoutInputChannels == 1)
                            reasonCodes.add ("single_input_visible");
                        reasonCodes.add ("host_input_visible");

                        const auto detail = candidateSelected
                            ? ("Host input bus reports "
                                + juce::String (layoutInputChannels)
                                + " visible channel(s); selected mic channel "
                                + juce::String (selectedMicChannel)
                                + " is currently addressable.")
                            : ("Host input bus reports "
                                + juce::String (layoutInputChannels)
                                + " visible channel(s); mic channel "
                                + juce::String (micChannel)
                                + " is available as an alternate measurement input.");

                        addInputCandidate (micChannel,
                                           micChannel == 1 ? 1 : (candidateSelected ? 2 : 10 + micChannel),
                                           candidateSelected,
                                           true,
                                           micChannel == 1 ? 0.82 : (candidateSelected ? 0.78 : 0.64),
                                           reasonCodes,
                                           {},
                                           juce::String(),
                                           detail,
                                           "detected");
                    }

                    if (! selectedMicVisible)
                    {
                        addInputCandidate (selectedMicChannel,
                                           99,
                                           true,
                                           false,
                                           0.52,
                                           { "selected_mic_out_of_range" },
                                           { "selected_mic_visibility" },
                                           "Choose a mic channel that is visible in the current host input width.",
                                           "Selected mic channel falls outside the currently visible host input width.",
                                           "inferred");
                    }
                }
            }
            discoveryGraph->setProperty ("inputCandidates", juce::var (inputCandidates));

            juce::Array<juce::var> topologyCandidates;
            {
                juce::var autoCandidateVar (new juce::DynamicObject());
                if (auto* candidate = autoCandidateVar.getDynamicObject())
                {
                    candidate->setProperty ("id", autoTopologyId);
                    candidate->setProperty ("kind", "topology_candidate");
                    candidate->setProperty ("label", topologyLabelForId (autoTopologyId));
                    candidate->setProperty ("requiredChannels", getRequiredCalibrationChannelsForTopologyIndex (lastAutoDetectedTopologyProfile));
                    candidate->setProperty ("writableChannels", writableChannels);
                    candidate->setProperty ("rank", 1);
                    candidate->setProperty ("confidence", hostOutputVisible ? 0.88 : 0.20);
                    candidate->setProperty ("selected", autoTopologyId == selectedTopologyId);
                    candidate->setProperty ("needsConfirmation", topologyMismatch || limitedCandidate);
                    candidate->setProperty (
                        "descriptor",
                        makeBl101Descriptor ("host_auto",
                                             hostOutputVisible ? "detected" : "unavailable",
                                             hostOutputVisible
                                                 ? ("Auto-map suggests "
                                                     + topologyLabelForId (autoTopologyId)
                                                     + " from the current host output width.")
                                                 : "Auto topology candidate is waiting for a usable host output report."));
                    addRoleAssignments (candidate,
                                        topologyRolesForId (autoTopologyId),
                                        juce::jmin (getRequiredCalibrationChannelsForTopologyIndex (lastAutoDetectedTopologyProfile),
                                                    SpatialRenderer::NUM_SPEAKERS),
                                        [&] (int index)
                                        {
                                            return juce::jlimit (1, 8, currentAutoRouting[static_cast<size_t> (index)]);
                                        },
                                        [&] (const juce::String& roleLabel, int index)
                                        {
                                            return findPreferredOutputChannelForRole (roleLabel, index);
                                        },
                                        getRequiredCalibrationChannelsForTopologyIndex (lastAutoDetectedTopologyProfile) > writableChannels ? "generic" : "inferred",
                                        getRequiredCalibrationChannelsForTopologyIndex (lastAutoDetectedTopologyProfile) > writableChannels
                                            ? "Role guesses follow the best topology candidate, but the current writable calibration map cannot represent every role yet."
                                            : "Role guesses are inferred from the best topology candidate and current writable calibration map.",
                                        "limited_writable_map",
                                        snapshotOutputRoles.isEmpty() ? "inferred" : "detected",
                                        snapshotOutputRoles.isEmpty()
                                            ? "Preferred reroute target is inferred from host output width order."
                                            : "Preferred reroute target comes from the host output role layout.");
                }
                topologyCandidates.add (autoCandidateVar);
            }
            if (selectedTopologyId != autoTopologyId)
            {
                juce::var selectedCandidateVar (new juce::DynamicObject());
                if (auto* candidate = selectedCandidateVar.getDynamicObject())
                {
                    candidate->setProperty ("id", selectedTopologyId);
                    candidate->setProperty ("kind", "topology_candidate");
                    candidate->setProperty ("label", topologyLabelForId (selectedTopologyId));
                    candidate->setProperty ("requiredChannels", requiredChannels);
                    candidate->setProperty ("writableChannels", writableChannels);
                    candidate->setProperty ("rank", 2);
                    candidate->setProperty ("confidence", 0.55);
                    candidate->setProperty ("selected", true);
                    candidate->setProperty ("needsConfirmation", true);
                    candidate->setProperty (
                        "descriptor",
                        makeBl101Descriptor ("manual_override",
                                             "inferred",
                                             "Current topology selection differs from the latest host-derived topology candidate.",
                                             true));
                    addRoleAssignments (candidate,
                                        topologyRolesForId (selectedTopologyId),
                                        juce::jmin (requiredChannels, SpatialRenderer::NUM_SPEAKERS),
                                        [&] (int index)
                                        {
                                            return juce::jlimit (1, 8, currentAutoRouting[static_cast<size_t> (index)]);
                                        },
                                        [&] (const juce::String& roleLabel, int index)
                                        {
                                            return findPreferredOutputChannelForRole (roleLabel, index);
                                        },
                                        requiredChannels > writableChannels ? "generic" : "inferred",
                                        requiredChannels > writableChannels
                                            ? "Role guesses preserve the manual topology selection, but the current writable calibration map cannot represent every role yet."
                                            : "Role guesses preserve the manual topology selection and follow the best writable calibration map.",
                                        "limited_writable_map",
                                        snapshotOutputRoles.isEmpty() ? "inferred" : "detected",
                                        snapshotOutputRoles.isEmpty()
                                            ? "Preferred reroute target is inferred from host output width order."
                                            : "Preferred reroute target comes from the host output role layout.");
                }
                topologyCandidates.add (selectedCandidateVar);
            }
            discoveryGraph->setProperty ("topologyCandidates", juce::var (topologyCandidates));

            juce::Array<juce::var> confirmationItems;
            const bool hasBlockingConfirmation = mappingDuplicateChannels || limitedCandidate || topologyMismatch;
            if (mappingDuplicateChannels)
                confirmationItems.add ("Resolve duplicate output routing before trusting auto-map.");
            if (limitedCandidate)
                confirmationItems.add ("Confirm limited mapping or choose a topology that fits writable outputs.");
            if (topologyMismatch)
                confirmationItems.add ("Confirm whether CALIBRATE should follow the host-derived topology or keep the manual selection.");
            if (hostInputVisible && ! selectedMicVisible)
                confirmationItems.add ("Selected mic channel is outside the visible host input width; choose an addressable input before measuring.");
            if (confirmationItems.isEmpty())
                confirmationItems.add ("No confirmation needed. Auto-map and topology candidate are aligned.");
            discoveryGraph->setProperty ("needsConfirmation", juce::var (confirmationItems));

            juce::var summaryVar (new juce::DynamicObject());
            if (auto* summary = summaryVar.getDynamicObject())
            {
                summary->setProperty (
                    "outputHeadline",
                    hostOutputVisible
                        ? ("Host main output · "
                            + juce::String (snapshotOutputChannels)
                            + " visible / "
                            + juce::String (writableChannels)
                            + " writable")
                        : "Host main output awaiting channel visibility");
                summary->setProperty (
                    "inputHeadline",
                    hostInputVisible
                        ? ("Host main input · "
                            + juce::String (layoutInputChannels)
                            + " visible · Mic "
                            + juce::String (selectedMicChannel)
                            + (selectedMicVisible ? " selected" : " needs review"))
                        : "Host main input awaiting channel visibility");
                summary->setProperty (
                    "topologyHeadline",
                    "Best candidate: " + topologyLabelForId (autoTopologyId));
                summary->setProperty (
                    "ambiguityHeadline",
                    ! hasBlockingConfirmation
                        ? "No ambiguity detected."
                        : (juce::String (confirmationItems.size()) + " confirmation item(s) require review."));
            }
            discoveryGraph->setProperty ("summary", summaryVar);
        }
        status->setProperty ("discoveryGraph", discoveryGraphVar);
    }

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
        hpDevice->setProperty ("profile_source",   cachedCalibrationProfileSource);

        const auto currentUtcMs = juce::Time::currentTimeMillis();
        std::optional<double> profileAgeMs = std::nullopt;
        constexpr double kProfileStaleAfterMs = 300000.0;
        if (cachedCalibrationProfileUpdatedAtUtcMs > 0 && currentUtcMs >= cachedCalibrationProfileUpdatedAtUtcMs)
            profileAgeMs = static_cast<double> (currentUtcMs - cachedCalibrationProfileUpdatedAtUtcMs);

        // Scores: use JSON null when not yet set (value -1 sentinel).
        if (cachedExternalizationScore >= 0.0f)
            hpDevice->setProperty ("externalization_score",    cachedExternalizationScore);
        else
            hpDevice->setProperty ("externalization_score",    juce::var());

        if (cachedFrontBackConfusionRate >= 0.0f)
            hpDevice->setProperty ("front_back_confusion_rate", cachedFrontBackConfusionRate);
        else
            hpDevice->setProperty ("front_back_confusion_rate", juce::var());

        hpDevice->setProperty (
            "provenance",
            makeBl101Descriptor ("companion_profile",
                                 cachedCalibrationHeadphoneProvenance,
                                 "Device status comes from the latest companion CalibrationProfile.json handoff.",
                                 false,
                                 profileAgeMs,
                                 kProfileStaleAfterMs));
        hpDevice->setProperty (
            "verification_provenance",
            makeBl101Descriptor ("companion_profile",
                                 cachedCalibrationVerificationProvenance,
                                 "Verification fields loaded from CalibrationProfile.json remain weaker than direct measurement unless a stronger provenance source is published.",
                                 false,
                                 profileAgeMs,
                                 kProfileStaleAfterMs));

        status->setProperty ("hpDeviceStatus", hpDeviceVar);
    }

    return statusVar;
}

juce::var LocusQAudioProcessor::serialiseKeyframeTimelineLocked() const
{
    juce::var timelineVar (new juce::DynamicObject());
    auto* timeline = timelineVar.getDynamicObject();

    timeline->setProperty ("durationSeconds",
                           keyframeTimelinePublishedDurationSeconds.load (std::memory_order_acquire));
    timeline->setProperty ("looping",
                           keyframeTimelinePublishedLooping.load (std::memory_order_acquire));
    timeline->setProperty ("playbackRate",
                           keyframeTimelinePublishedPlaybackRate.load (std::memory_order_acquire));
    timeline->setProperty ("currentTimeSeconds",
                           keyframeTimelinePublishedCurrentTimeSeconds.load (std::memory_order_acquire));

    juce::Array<juce::var> tracks;

    for (const auto& track : keyframeTimelineState.getTracks())
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

    keyframeTimelineState.clearTracks();

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
            keyframeTimelineState.addOrReplaceTrack (std::move (track));
        }
    }

    if (timeline->hasProperty ("durationSeconds"))
        keyframeTimelineState.setDurationSeconds (static_cast<double> (timeline->getProperty ("durationSeconds")));

    if (timeline->hasProperty ("looping"))
        keyframeTimelineState.setLooping (static_cast<bool> (timeline->getProperty ("looping")));

    if (timeline->hasProperty ("playbackRate"))
        keyframeTimelineState.setPlaybackRate (static_cast<float> (double (timeline->getProperty ("playbackRate"))));

    if (timeline->hasProperty ("currentTimeSeconds"))
        keyframeTimelineState.setCurrentTimeSeconds (static_cast<double> (timeline->getProperty ("currentTimeSeconds")));

    if (! keyframeTimelineState.hasAnyTrack())
        initialiseDefaultKeyframeTimeline (keyframeTimelineState);

    publishKeyframeTimelineStateToRtLocked();

    return true;
}

juce::var LocusQAudioProcessor::getKeyframeTimelineForUI() const
{
    const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
    return serialiseKeyframeTimelineLocked();
}

bool LocusQAudioProcessor::setKeyframeTimelineFromUI (const juce::var& timelineState)
{
    const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
    return applyKeyframeTimelineLocked (timelineState);
}

bool LocusQAudioProcessor::setTimelineCurrentTimeFromUI (double timeSeconds)
{
    if (! std::isfinite (timeSeconds))
        return false;

    const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
    const auto clamped = juce::jlimit (0.0,
                                       juce::jmax (0.0, keyframeTimelineState.getDurationSeconds()),
                                       timeSeconds);
    keyframeTimelineState.setCurrentTimeSeconds (clamped);
    publishKeyframeTimelineStateToRtLocked();
    return true;
}
