#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "editor_shell/EditorShellHelpers.h"
#include "editor_webview/EditorWebViewRuntime.h"

//==============================================================================
LocusQAudioProcessorEditor::LocusQAudioProcessorEditor (LocusQAudioProcessor& p)
    : AudioProcessorEditor (&p),
      runtimeConfig (locusq::editor_webview::makeRuntimeConfig()),
      audioProcessor (p)
{
    DBG ("LocusQ: Editor constructor started");

    //==========================================================================
    // CRITICAL: CREATION ORDER
    // 1. Relays already created (member initialization)
    // 2. Create WebBrowserComponent
    // 3. Create attachments (before browser visibility/navigation)
    // 4. addAndMakeVisible + goToURL
    //==========================================================================

    // Create WebBrowserComponent with platform-aware backend
    DBG ("LocusQ: Creating WebView");
    auto webViewOptions = locusq::editor_webview::makeBaseWebViewOptions();
    webViewOptions = locusq::editor_webview::withNativeBindings (std::move (webViewOptions),
                                                                  audioProcessor,
                                                                  runtimeConfig);
    webViewOptions = std::move (webViewOptions)
        .withResourceProvider ([] (const auto& url) { return locusq::editor_webview::getResource (url); })
        .withOptionsFrom (modeRelay)
        .withOptionsFrom (bypassRelay)
        .withOptionsFrom (calSpkConfigRelay)
        .withOptionsFrom (calTopologyProfileRelay)
        .withOptionsFrom (calMonitoringPathRelay)
        .withOptionsFrom (calDeviceProfileRelay)
        .withOptionsFrom (calMicChannelRelay)
        .withOptionsFrom (calSpk1OutRelay)
        .withOptionsFrom (calSpk2OutRelay)
        .withOptionsFrom (calSpk3OutRelay)
        .withOptionsFrom (calSpk4OutRelay)
        .withOptionsFrom (calTestLevelRelay)
        .withOptionsFrom (calTestTypeRelay)
        .withOptionsFrom (azimuthRelay)
        .withOptionsFrom (elevationRelay)
        .withOptionsFrom (distanceRelay)
        .withOptionsFrom (posXRelay)
        .withOptionsFrom (posYRelay)
        .withOptionsFrom (posZRelay)
        .withOptionsFrom (coordModeRelay)
        .withOptionsFrom (sizeLinkRelay)
        .withOptionsFrom (sizeUniformRelay)
        .withOptionsFrom (emitGainRelay)
        .withOptionsFrom (spreadRelay)
        .withOptionsFrom (directivityRelay)
        .withOptionsFrom (muteRelay)
        .withOptionsFrom (soloRelay)
        .withOptionsFrom (emitColorRelay)
        .withOptionsFrom (physEnableRelay)
        .withOptionsFrom (physMassRelay)
        .withOptionsFrom (physDragRelay)
        .withOptionsFrom (physElasticityRelay)
        .withOptionsFrom (physGravityRelay)
        .withOptionsFrom (physGravityDirRelay)
        .withOptionsFrom (physFrictionRelay)
        .withOptionsFrom (physThrowRelay)
        .withOptionsFrom (physResetRelay)
        .withOptionsFrom (animEnableRelay)
        .withOptionsFrom (animModeRelay)
        .withOptionsFrom (animLoopRelay)
        .withOptionsFrom (animSpeedRelay)
        .withOptionsFrom (animSyncRelay)
        .withOptionsFrom (masterGainRelay)
        .withOptionsFrom (spk1GainRelay)
        .withOptionsFrom (spk2GainRelay)
        .withOptionsFrom (spk3GainRelay)
        .withOptionsFrom (spk4GainRelay)
        .withOptionsFrom (spk1DelayRelay)
        .withOptionsFrom (spk2DelayRelay)
        .withOptionsFrom (spk3DelayRelay)
        .withOptionsFrom (spk4DelayRelay)
        .withOptionsFrom (qualityRelay)
        .withOptionsFrom (distanceModelRelay)
        .withOptionsFrom (headphoneModeRelay)
        .withOptionsFrom (headphoneProfileRelay)
        .withOptionsFrom (auditionEnableRelay)
        .withOptionsFrom (auditionSignalRelay)
        .withOptionsFrom (auditionMotionRelay)
        .withOptionsFrom (auditionLevelRelay)
        .withOptionsFrom (distanceRefRelay)
        .withOptionsFrom (distanceMaxRelay)
        .withOptionsFrom (dopplerRelay)
        .withOptionsFrom (dopplerScaleRelay)
        .withOptionsFrom (airAbsorbRelay)
        .withOptionsFrom (roomEnableRelay)
        .withOptionsFrom (roomMixRelay)
        .withOptionsFrom (roomSizeRelay)
        .withOptionsFrom (roomDampingRelay)
        .withOptionsFrom (roomErOnlyRelay)
        .withOptionsFrom (physRateRelay)
        .withOptionsFrom (physWallsRelay)
        .withOptionsFrom (physInteractRelay)
        .withOptionsFrom (physPauseRelay)
        .withOptionsFrom (vizModeRelay)
        .withOptionsFrom (vizTrailsRelay)
        .withOptionsFrom (vizTrailLenRelay)
        .withOptionsFrom (vizVectorsRelay)
        .withOptionsFrom (vizPhysicsLensRelay)
        .withOptionsFrom (vizDiagMixRelay)
        .withOptionsFrom (vizGridRelay)
        .withOptionsFrom (vizLabelsRelay);

    webView = std::make_unique<juce::WebBrowserComponent> (std::move (webViewOptions));

    // Create parameter attachments before exposing/loading the WebView.
    modeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("mode"), modeRelay);
    bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("bypass"), bypassRelay);

    calSpkConfigAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_spk_config"), calSpkConfigRelay);
    calTopologyProfileAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_topology_profile"), calTopologyProfileRelay);
    calMonitoringPathAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_monitoring_path"), calMonitoringPathRelay);
    calDeviceProfileAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_device_profile"), calDeviceProfileRelay);
    calMicChannelAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_mic_channel"), calMicChannelRelay);
    calSpk1OutAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_spk1_out"), calSpk1OutRelay);
    calSpk2OutAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_spk2_out"), calSpk2OutRelay);
    calSpk3OutAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_spk3_out"), calSpk3OutRelay);
    calSpk4OutAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_spk4_out"), calSpk4OutRelay);
    calTestLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_test_level"), calTestLevelRelay);
    calTestTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("cal_test_type"), calTestTypeRelay);

    azimuthAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_azimuth"), azimuthRelay);
    elevationAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_elevation"), elevationRelay);
    distanceAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_distance"), distanceRelay);
    posXAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_x"), posXRelay);
    posYAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_y"), posYRelay);
    posZAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_z"), posZRelay);
    coordModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_coord_mode"), coordModeRelay);

    sizeLinkAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("size_link"), sizeLinkRelay);
    sizeUniformAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("size_uniform"), sizeUniformRelay);

    emitGainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_gain"), emitGainRelay);
    spreadAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_spread"), spreadRelay);
    directivityAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_directivity"), directivityRelay);
    muteAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_mute"), muteRelay);
    soloAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_solo"), soloRelay);
    emitColorAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_color"), emitColorRelay);
    dirAzimuthAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_dir_azimuth"), dirAzimuthRelay);
    dirElevationAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("emit_dir_elevation"), dirElevationRelay);

    physEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_enable"), physEnableRelay);
    physMassAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_mass"), physMassRelay);
    physDragAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_drag"), physDragRelay);
    physElasticityAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_elasticity"), physElasticityRelay);
    physGravityAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_gravity"), physGravityRelay);
    physGravityDirAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_gravity_dir"), physGravityDirRelay);
    physFrictionAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_friction"), physFrictionRelay);
    physThrowAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_throw"), physThrowRelay);
    physResetAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_reset"), physResetRelay);
    physVelXAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_vel_x"), physVelXRelay);
    physVelYAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_vel_y"), physVelYRelay);
    physVelZAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_vel_z"), physVelZRelay);

    animEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("anim_enable"), animEnableRelay);
    animModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("anim_mode"), animModeRelay);
    animLoopAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("anim_loop"), animLoopRelay);
    animSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("anim_speed"), animSpeedRelay);
    animSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("anim_sync"), animSyncRelay);

    masterGainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_master_gain"), masterGainRelay);
    spk1GainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk1_gain"), spk1GainRelay);
    spk2GainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk2_gain"), spk2GainRelay);
    spk3GainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk3_gain"), spk3GainRelay);
    spk4GainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk4_gain"), spk4GainRelay);
    spk1DelayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk1_delay"), spk1DelayRelay);
    spk2DelayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk2_delay"), spk2DelayRelay);
    spk3DelayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk3_delay"), spk3DelayRelay);
    spk4DelayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_spk4_delay"), spk4DelayRelay);
    qualityAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_quality"), qualityRelay);
    distanceModelAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_distance_model"), distanceModelRelay);
    headphoneModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_headphone_mode"), headphoneModeRelay);
    headphoneProfileAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_headphone_profile"), headphoneProfileRelay);
    auditionEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_audition_enable"), auditionEnableRelay);
    auditionSignalAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_audition_signal"), auditionSignalRelay);
    auditionMotionAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_audition_motion"), auditionMotionRelay);
    auditionLevelAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_audition_level"), auditionLevelRelay);
    distanceRefAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_distance_ref"), distanceRefRelay);
    distanceMaxAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_distance_max"), distanceMaxRelay);
    dopplerAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_doppler"), dopplerRelay);
    dopplerScaleAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_doppler_scale"), dopplerScaleRelay);
    airAbsorbAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_air_absorb"), airAbsorbRelay);
    roomEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_room_enable"), roomEnableRelay);
    roomMixAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_room_mix"), roomMixRelay);
    roomSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_room_size"), roomSizeRelay);
    roomDampingAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_room_damping"), roomDampingRelay);
    roomErOnlyAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_room_er_only"), roomErOnlyRelay);
    physRateAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_phys_rate"), physRateRelay);
    physWallsAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_phys_walls"), physWallsRelay);
    physInteractAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_phys_interact"), physInteractRelay);
    physPauseAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_phys_pause"), physPauseRelay);
    vizModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_mode"), vizModeRelay);
    vizTrailsAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_trails"), vizTrailsRelay);
    vizTrailLenAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_trail_len"), vizTrailLenRelay);
    vizVectorsAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_vectors"), vizVectorsRelay);
    vizPhysicsLensAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_physics_lens"), vizPhysicsLensRelay);
    vizDiagMixAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_diag_mix"), vizDiagMixRelay);
    vizGridAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_grid"), vizGridRelay);
    vizLabelsAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_labels"), vizLabelsRelay);

    addAndMakeVisible (*webView);

    const auto initialResourcePath = locusq::editor_webview::getInitialUiResourcePath (runtimeConfig);
    const auto initialUrl = locusq::editor_webview::makeInitialUrl (runtimeConfig);

    DBG ("LocusQ: Loading UI path " + initialResourcePath);
    webView->goToURL (initialUrl);

    // Start timer for scene state updates (~30fps)
    startTimerHz (30);

    setSize (1200, 800);
    DBG ("LocusQ: Editor constructor completed");
}

LocusQAudioProcessorEditor::~LocusQAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void LocusQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void LocusQAudioProcessorEditor::resized()
{
    if (webView == nullptr)
        return;

    const auto bounds = getLocalBounds();
    webView->setBounds (bounds);
    locusq::editor_shell::notifyHostResized (*webView, bounds.getWidth(), bounds.getHeight());
}

//==============================================================================
void LocusQAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr) return;

    updateStandaloneWindowTitle();

    // Push scene + calibration payloads in one JS evaluation so panel updates stay
    // deterministic under rapid profile switch bursts.
    auto sceneJSON = audioProcessor.getSceneStateJSON();
    auto calibrationJSON = juce::JSON::toString (audioProcessor.getCalibrationStatus());
    locusq::editor_shell::pushSceneAndCalibrationUpdate (*webView, sceneJSON, calibrationJSON);

    if (! runtimeProbeDone)
    {
        ++runtimeProbeTicks;

        if (runtimeProbeTicks >= 60)
        {
            runtimeProbeDone = true;

            const auto probeFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                .getChildFile ("locusq_webview_runtime_probe.json");

            const auto probeScript = locusq::editor_shell::getRuntimeProbeScript();

            juce::Component::SafePointer<LocusQAudioProcessorEditor> safeThis (this);
            webView->evaluateJavascript (probeScript,
                [safeThis, probeFile] (juce::WebBrowserComponent::EvaluationResult result)
                {
                    if (safeThis == nullptr)
                        return;

                    juce::var payloadVar (new juce::DynamicObject());
                    if (auto* payload = payloadVar.getDynamicObject())
                    {
                        payload->setProperty ("timestampUtc", juce::Time::getCurrentTime().toISO8601 (true));

                        if (const auto* value = result.getResult())
                        {
                            payload->setProperty ("status", "ok");
                            payload->setProperty ("result", *value);
                        }
                        else if (const auto* error = result.getError())
                        {
                            juce::String typeText = "unknown";
                            switch (error->type)
                            {
                                case juce::WebBrowserComponent::EvaluationResult::Error::Type::javascriptException: typeText = "javascriptException"; break;
                                case juce::WebBrowserComponent::EvaluationResult::Error::Type::unsupportedReturnType: typeText = "unsupportedReturnType"; break;
                                case juce::WebBrowserComponent::EvaluationResult::Error::Type::unknown: default: break;
                            }

                            juce::var errorVar (new juce::DynamicObject());
                            if (auto* errorObj = errorVar.getDynamicObject())
                            {
                                errorObj->setProperty ("type", typeText);
                                errorObj->setProperty ("message", error->message);
                            }

                            payload->setProperty ("status", "error");
                            payload->setProperty ("error", errorVar);
                        }
                        else
                        {
                            payload->setProperty ("status", "no_result");
                        }
                    }

                    probeFile.replaceWithText (juce::JSON::toString (payloadVar, true));
                });
        }
    }

    if (runtimeConfig.selfTestEnabled && ! uiSelfTestResultWritten)
    {
        ++uiSelfTestPollTicks;
        const auto uiSelfTestTimeoutTicks = runtimeConfig.selfTestTimeoutTicks;

        if (uiSelfTestPollTicks >= 30 && ! uiSelfTestProbeInFlight && (uiSelfTestPollTicks % 6) == 0)
        {
            uiSelfTestProbeInFlight = true;
            const auto resultFile = locusq::editor_webview::getUiSelfTestResultFile();
            const auto selfTestPollScript = locusq::editor_shell::getSelfTestPollScript();

            juce::Component::SafePointer<LocusQAudioProcessorEditor> safeThis (this);
            webView->evaluateJavascript (selfTestPollScript,
                [safeThis, resultFile, uiSelfTestTimeoutTicks] (juce::WebBrowserComponent::EvaluationResult result)
                {
                    if (safeThis == nullptr)
                        return;

                    safeThis->uiSelfTestProbeInFlight = false;

                    const auto* value = result.getResult();
                    if (value == nullptr)
                        return;

                    const auto* root = value->getDynamicObject();
                    if (root == nullptr)
                        return;

                    if (! static_cast<bool> (root->getProperty ("ready")))
                    {
                        if (safeThis->uiSelfTestPollTicks >= uiSelfTestTimeoutTicks)
                        {
                            juce::var payloadVar (new juce::DynamicObject());
                            if (auto* payload = payloadVar.getDynamicObject())
                            {
                                payload->setProperty ("timestampUtc", juce::Time::getCurrentTime().toISO8601 (true));
                                payload->setProperty ("selftestEnabled", true);
                                payload->setProperty ("status", "fail");
                                payload->setProperty ("ok", false);

                                juce::var resultVar (new juce::DynamicObject());
                                if (auto* result = resultVar.getDynamicObject())
                                {
                                    result->setProperty ("status", root->getProperty ("status"));
                                    result->setProperty ("error", "ui_selftest_timeout_before_pass_or_fail");
                                    result->setProperty ("search", root->getProperty ("search"));
                                    result->setProperty ("href", root->getProperty ("href"));
                                    result->setProperty ("hasUpdateSceneState", root->getProperty ("hasUpdateSceneState"));
                                    result->setProperty ("hasUpdateCalibrationStatus", root->getProperty ("hasUpdateCalibrationStatus"));
                                    result->setProperty ("scriptSrcs", root->getProperty ("scriptSrcs"));
                                    result->setProperty ("bootErrors", root->getProperty ("bootErrors"));
                                }

                                payload->setProperty ("result", resultVar);
                            }

                            resultFile.getParentDirectory().createDirectory();
                            const auto writeOk = resultFile.replaceWithText (juce::JSON::toString (payloadVar, true));
                            safeThis->uiSelfTestResultWritten = writeOk;
                        }

                        return;
                    }

                    juce::var payloadVar (new juce::DynamicObject());
                    if (auto* payload = payloadVar.getDynamicObject())
                    {
                        payload->setProperty ("timestampUtc", juce::Time::getCurrentTime().toISO8601 (true));
                        payload->setProperty ("selftestEnabled", true);
                        payload->setProperty ("status", root->getProperty ("status"));
                        payload->setProperty ("ok", root->getProperty ("ok"));
                        payload->setProperty ("result", root->getProperty ("result"));
                    }

                    resultFile.getParentDirectory().createDirectory();
                    const auto writeOk = resultFile.replaceWithText (juce::JSON::toString (payloadVar, true));
                    safeThis->uiSelfTestResultWritten = writeOk;
                });
        }
    }
}

void LocusQAudioProcessorEditor::updateStandaloneWindowTitle()
{
    if (standaloneWindowTitleUpdated)
        return;

    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        window->setName (locusq::editor_webview::getStandaloneWindowTitle (runtimeConfig));
        standaloneWindowTitleUpdated = true;
    }
}
