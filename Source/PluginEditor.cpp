#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cstdlib>

namespace
{
bool isUiSelfTestEnabled()
{
    if (const auto* flag = std::getenv ("LOCUSQ_UI_SELFTEST"))
    {
        const auto value = juce::String (flag).trim().toLowerCase();
        if (value.isNotEmpty() && value != "0" && value != "false" && value != "off")
            return true;
    }

    return false;
}

bool isUiSelfTestBl009Enabled()
{
    if (const auto* flag = std::getenv ("LOCUSQ_UI_SELFTEST_BL009"))
    {
        const auto value = juce::String (flag).trim().toLowerCase();
        if (value.isNotEmpty() && value != "0" && value != "false" && value != "off")
            return true;
    }

    return false;
}

bool isUiSelfTestBl011Enabled()
{
    if (const auto* flag = std::getenv ("LOCUSQ_UI_SELFTEST_BL011"))
    {
        const auto value = juce::String (flag).trim().toLowerCase();
        if (value.isNotEmpty() && value != "0" && value != "false" && value != "off")
            return true;
    }

    return false;
}

juce::String getUiSelfTestScope()
{
    if (const auto* scope = std::getenv ("LOCUSQ_UI_SELFTEST_SCOPE"))
    {
        const auto value = juce::String (scope).trim().toLowerCase();
        juce::String sanitized;
        sanitized.preallocateBytes (value.getNumBytesAsUTF8());

        for (const auto ch : value)
        {
            if (juce::CharacterFunctions::isLetterOrDigit (ch) || ch == '_' || ch == '-')
                sanitized << ch;
        }

        if (sanitized.isNotEmpty())
            return sanitized;
    }

    return {};
}

int getUiSelfTestTimeoutTicks()
{
    // BL-009/BL-011 opt-in checks execute additional deterministic lanes and can
    // exceed the default timeout budget under production self-test load.
    if (isUiSelfTestBl009Enabled() || isUiSelfTestBl011Enabled())
        return 900; // ~30 seconds at 30 Hz

    return 300; // ~10 seconds at 30 Hz
}

bool shouldUseIncrementalUi()
{
    if (const auto* variant = std::getenv ("LOCUSQ_UI_VARIANT"))
    {
        const auto value = juce::String (variant).trim().toLowerCase();
        if (value == "incremental" || value == "stage12")
            return true;
        if (value == "production" || value == "index")
            return false;
    }

    if (isUiSelfTestEnabled())
        return true;

    return false;
}

juce::String getInitialUiResourcePath()
{
    return shouldUseIncrementalUi() ? "/incremental/index.html"
                                    : "/index.html";
}

juce::String getStandaloneWindowTitle()
{
    const auto uiLabel = shouldUseIncrementalUi() ? "incremental-stage12"
                                                  : "production-ui";
    return juce::String (JucePlugin_Name)
        + " v" + juce::String (JucePlugin_VersionString)
        + " [" + uiLabel + "]";
}

juce::File getUiSelfTestResultFile()
{
    if (const auto* path = std::getenv ("LOCUSQ_UI_SELFTEST_RESULT_PATH"))
    {
        const auto configuredPath = juce::String (path).trim();
        if (configuredPath.isNotEmpty())
            return juce::File (configuredPath);
    }

    return juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getChildFile ("locusq_incremental_ui_selftest_result.json");
}
} // namespace

//==============================================================================
LocusQAudioProcessorEditor::LocusQAudioProcessorEditor (LocusQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
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
    auto webViewOptions = juce::WebBrowserComponent::Options{};

#if JUCE_WINDOWS
    webViewOptions = webViewOptions
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder (juce::File::getSpecialLocation (
                    juce::File::SpecialLocationType::tempDirectory)));
#else
    webViewOptions = webViewOptions
        .withBackend (juce::WebBrowserComponent::Options::Backend::defaultBackend);
#endif

    webViewOptions = webViewOptions
        .withNativeIntegrationEnabled()
        .withNativeFunction ("locusqStartCalibration",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var options = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.startCalibrationFromUI (options));
                             })
        .withNativeFunction ("locusqAbortCalibration",
                             [this] (const juce::Array<juce::var>&,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 audioProcessor.abortCalibrationFromUI();
                                 completion (true);
                             })
        .withNativeFunction ("locusqRedetectCalibrationRouting",
                             [this] (const juce::Array<juce::var>&,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.redetectCalibrationRoutingFromUI());
                             })
        .withNativeFunction ("locusqGetKeyframeTimeline",
                             [this] (const juce::Array<juce::var>&,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.getKeyframeTimelineForUI());
                             })
        .withNativeFunction ("locusqSetKeyframeTimeline",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 if (args.isEmpty())
                                 {
                                     completion (false);
                                     return;
                                 }

                                 completion (audioProcessor.setKeyframeTimelineFromUI (args[0]));
                             })
        .withNativeFunction ("locusqSetTimelineTime",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 if (args.isEmpty())
                                 {
                                     completion (false);
                                     return;
                                 }

                                 completion (audioProcessor.setTimelineCurrentTimeFromUI (static_cast<double> (args[0])));
                             })
        .withNativeFunction ("locusqListEmitterPresets",
                             [this] (const juce::Array<juce::var>&,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.listEmitterPresetsFromUI());
                             })
        .withNativeFunction ("locusqSaveEmitterPreset",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var options = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.saveEmitterPresetFromUI (options));
                             })
        .withNativeFunction ("locusqLoadEmitterPreset",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var options = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.loadEmitterPresetFromUI (options));
                             })
        .withNativeFunction ("locusqRenameEmitterPreset",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var options = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.renameEmitterPresetFromUI (options));
                             })
        .withNativeFunction ("locusqDeleteEmitterPreset",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var options = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.deleteEmitterPresetFromUI (options));
                             })
        .withNativeFunction ("locusqGetUiState",
                             [this] (const juce::Array<juce::var>&,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.getUIStateFromUI());
                             })
        .withNativeFunction ("locusqSetUiState",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 if (args.isEmpty())
                                 {
                                     completion (false);
                                     return;
                                 }

                                 completion (audioProcessor.setUIStateFromUI (args[0]));
                             })
        .withNativeFunction ("locusqGetChoiceItems",
                             [this] (const juce::Array<juce::var>& args,
                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 juce::Array<juce::var> values;

                                 if (! args.isEmpty() && args[0].isString())
                                 {
                                     const auto parameterId = args[0].toString();
                                     if (auto* parameter = audioProcessor.apvts.getParameter (parameterId))
                                     {
                                         if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (parameter))
                                         {
                                             for (const auto& item : choice->choices)
                                                 values.add (item);
                                         }
                                     }
                                 }

                                 completion (juce::var (values));
                             })
        .withNativeFunction ("locusqWriteUiSelfTestResult",
                             [] (const juce::Array<juce::var>& args,
                                 juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const auto resultFile = getUiSelfTestResultFile();
                                 resultFile.getParentDirectory().createDirectory();

                                 juce::var payload (new juce::DynamicObject());
                                 if (auto* object = payload.getDynamicObject())
                                 {
                                     object->setProperty ("timestampUtc", juce::Time::getCurrentTime().toISO8601 (true));
                                     object->setProperty ("selftestEnabled", isUiSelfTestEnabled());
                                     object->setProperty ("payload", args.isEmpty() ? juce::var() : args[0]);
                                 }

                                 const auto json = juce::JSON::toString (payload, true);
                                 const auto writeOk = resultFile.replaceWithText (json);

                                 juce::var response (new juce::DynamicObject());
                                 if (auto* object = response.getDynamicObject())
                                 {
                                     object->setProperty ("ok", writeOk);
                                     object->setProperty ("path", resultFile.getFullPathName());
                                 }

                                 completion (response);
                             })
        .withResourceProvider ([this] (const auto& url) { return getResource (url); })
        .withOptionsFrom (modeRelay)
        .withOptionsFrom (bypassRelay)
        .withOptionsFrom (calSpkConfigRelay)
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

    const auto initialResourcePath = getInitialUiResourcePath();
    const auto normalizedInitialPath = (initialResourcePath == "/") ? "/index.html" : initialResourcePath;
    const auto cacheBust = juce::String (juce::Time::getCurrentTime().toMilliseconds());
    auto initialUrl = juce::WebBrowserComponent::getResourceProviderRoot()
        + normalizedInitialPath + "?cb=" + cacheBust;

    if (isUiSelfTestEnabled())
        initialUrl += "&selftest=1";

    if (isUiSelfTestBl009Enabled())
        initialUrl += "&selftest_bl009=1";

    if (isUiSelfTestBl011Enabled())
        initialUrl += "&selftest_bl011=1";

    const auto selfTestScope = getUiSelfTestScope();
    if (selfTestScope.isNotEmpty())
        initialUrl += "&selftest_scope=" + selfTestScope;

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
    if (webView)
        webView->setBounds (getLocalBounds());
}

//==============================================================================
void LocusQAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr) return;

    updateStandaloneWindowTitle();

    // Push scene state JSON to the WebView for 3D viewport updates
    auto json = audioProcessor.getSceneStateJSON();
    webView->evaluateJavascript (
        "if(typeof updateSceneState==='function')updateSceneState(" + json + ");");

    auto calibrationJSON = juce::JSON::toString (audioProcessor.getCalibrationStatus());
    webView->evaluateJavascript (
        "if(typeof updateCalibrationStatus==='function')updateCalibrationStatus(" + calibrationJSON + ");");

    if (! runtimeProbeDone)
    {
        ++runtimeProbeTicks;

        if (runtimeProbeTicks >= 60)
        {
            runtimeProbeDone = true;

            const auto probeFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                .getChildFile ("locusq_webview_runtime_probe.json");

            const juce::String probeScript = R"JS(
                (() => {
                    const badge = document.getElementById('quality-badge');
                    return {
                        hasJuce: typeof window.Juce !== 'undefined',
                        hasBackend: typeof window.__JUCE__ !== 'undefined' && !!window.__JUCE__.backend,
                        hasUpdateSceneState: typeof window.updateSceneState === 'function',
                        hasUpdateCalibrationStatus: typeof window.updateCalibrationStatus === 'function',
                        modeTabCount: document.querySelectorAll('.mode-tab').length,
                        bodyClass: document.body ? document.body.className : '',
                        qualityText: badge ? badge.textContent : ''
                    };
                })()
            )JS";

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

    if (isUiSelfTestEnabled() && ! uiSelfTestResultWritten)
    {
        ++uiSelfTestPollTicks;
        const auto uiSelfTestTimeoutTicks = getUiSelfTestTimeoutTicks();

        if (uiSelfTestPollTicks >= 30 && ! uiSelfTestProbeInFlight && (uiSelfTestPollTicks % 6) == 0)
        {
            uiSelfTestProbeInFlight = true;
            const auto resultFile = getUiSelfTestResultFile();

            const juce::String selfTestPollScript = R"JS(
                (() => {
                    const value = window.__LQ_SELFTEST_RESULT__;
                    if (!value || typeof value !== 'object')
                        return {
                            ready: false,
                            status: 'missing',
                            search: String(window.location && window.location.search ? window.location.search : ''),
                            href: String(window.location && window.location.href ? window.location.href : ''),
                            hasUpdateSceneState: typeof window.updateSceneState === 'function',
                            hasUpdateCalibrationStatus: typeof window.updateCalibrationStatus === 'function',
                            scriptSrcs: Array.from(document.scripts || []).map(s => String(s.src || '')),
                            bootErrors: Array.isArray(window.__LQ_BOOT_ERRORS__) ? window.__LQ_BOOT_ERRORS__ : []
                        };

                    const status = String(value.status || '');
                    if (status !== 'pass' && status !== 'fail')
                        return {
                            ready: false,
                            status,
                            search: String(window.location && window.location.search ? window.location.search : ''),
                            href: String(window.location && window.location.href ? window.location.href : ''),
                            hasUpdateSceneState: typeof window.updateSceneState === 'function',
                            hasUpdateCalibrationStatus: typeof window.updateCalibrationStatus === 'function',
                            scriptSrcs: Array.from(document.scripts || []).map(s => String(s.src || '')),
                            bootErrors: Array.isArray(window.__LQ_BOOT_ERRORS__) ? window.__LQ_BOOT_ERRORS__ : []
                        };

                    return {
                        ready: true,
                        status,
                        ok: !!value.ok,
                        result: value
                    };
                })()
            )JS";

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
        window->setName (getStandaloneWindowTitle());
        standaloneWindowTitleUpdated = true;
    }
}

//==============================================================================
// RESOURCE PROVIDER IMPLEMENTATION
//==============================================================================

const char* LocusQAudioProcessorEditor::getMimeForExtension (const juce::String& extension)
{
    static const std::unordered_map<juce::String, const char*> mimeMap =
    {
        { "html", "text/html" },
        { "css",  "text/css" },
        { "js",   "text/javascript" },
        { "mjs",  "text/javascript" },
        { "json", "application/json" },
        { "png",  "image/png" },
        { "jpg",  "image/jpeg" },
        { "svg",  "image/svg+xml" }
    };

    auto it = mimeMap.find (extension.toLowerCase());
    if (it != mimeMap.end())
        return it->second;

    return "text/plain";
}

std::optional<juce::WebBrowserComponent::Resource> LocusQAudioProcessorEditor::getResource (
    const juce::String& url)
{
    auto resourcePath = url.trim();

    const auto stripKnownPrefix = [&resourcePath] (const juce::String& prefix)
    {
        if (resourcePath.startsWithIgnoreCase (prefix))
            resourcePath = resourcePath.substring (prefix.length());
    };

    stripKnownPrefix ("juce://juce.backend");
    stripKnownPrefix ("https://juce.backend");
    stripKnownPrefix ("http://juce.backend");

    while (resourcePath.startsWith ("//"))
        resourcePath = resourcePath.substring (1);

    if (resourcePath.isEmpty())
        resourcePath = "/";

    if (! resourcePath.startsWithChar ('/'))
        resourcePath = "/" + resourcePath;

    resourcePath = resourcePath.upToFirstOccurrenceOf ("?", false, false);
    resourcePath = resourcePath.upToFirstOccurrenceOf ("#", false, false);
    if (resourcePath.isEmpty() || resourcePath == "/")
        resourcePath = "/index.html";

    const auto logDirectory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("LocusQ");
    logDirectory.createDirectory();
    const auto resourceLogFile = logDirectory.getChildFile ("resource_requests.log");
    resourceLogFile.appendText (juce::Time::getCurrentTime().toISO8601 (true)
        + " request url=" + url + " path=" + resourcePath + "\n");

#if JUCE_DEBUG
    DBG ("LocusQ resource requested: " + resourcePath);
#endif

    // Map URL paths to BinaryData resources
    const char* resourceData = nullptr;
    int resourceSize = 0;
    juce::String mimeType;

    auto path = resourcePath.substring (1); // Remove leading slash

    if (path == "index.html")
    {
        resourceData = BinaryData::index_html;
        resourceSize = BinaryData::index_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "js/index.js")
    {
        resourceData = BinaryData::index_js;
        resourceSize = BinaryData::index_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "js/three.min.js")
    {
        resourceData = BinaryData::three_min_js;
        resourceSize = BinaryData::three_min_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "js/juce/index.js")
    {
        // JUCE frontend library
        resourceData = BinaryData::index_js2;
        resourceSize = BinaryData::index_js2Size;
        mimeType = "text/javascript";
    }
    else if (path == "js/juce/check_native_interop.js")
    {
        resourceData = BinaryData::check_native_interop_js;
        resourceSize = BinaryData::check_native_interop_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "poc/index.html")
    {
        resourceData = BinaryData::index_poc_html;
        resourceSize = BinaryData::index_poc_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/index.html")
    {
        resourceData = BinaryData::index_stage12_html;
        resourceSize = BinaryData::index_stage12_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage1.html")
    {
        resourceData = BinaryData::index_poc_html;
        resourceSize = BinaryData::index_poc_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage2.html")
    {
        resourceData = BinaryData::index_stage2_html;
        resourceSize = BinaryData::index_stage2_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage3.html")
    {
        resourceData = BinaryData::index_stage3_html;
        resourceSize = BinaryData::index_stage3_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage4.html")
    {
        resourceData = BinaryData::index_stage4_html;
        resourceSize = BinaryData::index_stage4_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage5.html")
    {
        resourceData = BinaryData::index_stage5_html;
        resourceSize = BinaryData::index_stage5_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage6.html")
    {
        resourceData = BinaryData::index_stage6_html;
        resourceSize = BinaryData::index_stage6_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage7.html")
    {
        resourceData = BinaryData::index_stage7_html;
        resourceSize = BinaryData::index_stage7_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage8.html")
    {
        resourceData = BinaryData::index_stage8_html;
        resourceSize = BinaryData::index_stage8_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage9.html")
    {
        resourceData = BinaryData::index_stage9_html;
        resourceSize = BinaryData::index_stage9_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage10.html")
    {
        resourceData = BinaryData::index_stage10_html;
        resourceSize = BinaryData::index_stage10_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage11.html")
    {
        resourceData = BinaryData::index_stage11_html;
        resourceSize = BinaryData::index_stage11_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/stage12.html")
    {
        resourceData = BinaryData::index_stage12_html;
        resourceSize = BinaryData::index_stage12_htmlSize;
        mimeType = "text/html";
    }
    else if (path == "incremental/js/stage2_ui.js")
    {
        resourceData = BinaryData::stage2_ui_js;
        resourceSize = BinaryData::stage2_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage3_ui.js")
    {
        resourceData = BinaryData::stage3_ui_js;
        resourceSize = BinaryData::stage3_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage4_ui.js")
    {
        resourceData = BinaryData::stage4_ui_js;
        resourceSize = BinaryData::stage4_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage5_ui.js")
    {
        resourceData = BinaryData::stage5_ui_js;
        resourceSize = BinaryData::stage5_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage6_ui.js")
    {
        resourceData = BinaryData::stage6_ui_js;
        resourceSize = BinaryData::stage6_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage7_ui.js")
    {
        resourceData = BinaryData::stage7_ui_js;
        resourceSize = BinaryData::stage7_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage8_ui.js")
    {
        resourceData = BinaryData::stage8_ui_js;
        resourceSize = BinaryData::stage8_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage9_ui.js")
    {
        resourceData = BinaryData::stage9_ui_js;
        resourceSize = BinaryData::stage9_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage10_ui.js")
    {
        resourceData = BinaryData::stage10_ui_js;
        resourceSize = BinaryData::stage10_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage11_ui.js")
    {
        resourceData = BinaryData::stage11_ui_js;
        resourceSize = BinaryData::stage11_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "incremental/js/stage12_ui.js")
    {
        resourceData = BinaryData::stage12_ui_js;
        resourceSize = BinaryData::stage12_ui_jsSize;
        mimeType = "text/javascript";
    }
    else if (path == "poc/js/poc_ui.js")
    {
        resourceData = BinaryData::poc_ui_js;
        resourceSize = BinaryData::poc_ui_jsSize;
        mimeType = "text/javascript";
    }

#if JUCE_DEBUG
    if (resourceData != nullptr)
        DBG ("  -> FOUND (" + juce::String (resourceSize) + " bytes)");
    else
        DBG ("  -> NOT FOUND: " + path);
#endif

    resourceLogFile.appendText (juce::Time::getCurrentTime().toISO8601 (true)
        + " result path=" + path + " found=" + juce::String (resourceData != nullptr ? "1" : "0")
        + " size=" + juce::String (resourceSize) + "\n");

    if (resourceData != nullptr && resourceSize > 0)
    {
        std::vector<std::byte> data (static_cast<size_t> (resourceSize));
        std::memcpy (data.data(), resourceData, static_cast<size_t> (resourceSize));

        return juce::WebBrowserComponent::Resource {
            std::move (data),
            mimeType
        };
    }

    return std::nullopt;
}
