#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
LocusQAudioProcessorEditor::LocusQAudioProcessorEditor (LocusQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    DBG ("LocusQ: Editor constructor started");

    //==========================================================================
    // CRITICAL: CREATION ORDER
    // 1. Relays already created (member initialization)
    // 2. Create WebBrowserComponent
    // 3. addAndMakeVisible
    // 4. Create attachments AFTER WebView
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
        .withOptionsFrom (qualityRelay)
        .withOptionsFrom (distanceModelRelay)
        .withOptionsFrom (dopplerRelay)
        .withOptionsFrom (airAbsorbRelay)
        .withOptionsFrom (roomEnableRelay)
        .withOptionsFrom (roomErOnlyRelay)
        .withOptionsFrom (physRateRelay)
        .withOptionsFrom (physWallsRelay)
        .withOptionsFrom (physPauseRelay)
        .withOptionsFrom (vizModeRelay);

    webView = std::make_unique<juce::WebBrowserComponent> (std::move (webViewOptions));

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Create parameter attachments after WebView is alive
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
    qualityAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_quality"), qualityRelay);
    distanceModelAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_distance_model"), distanceModelRelay);
    dopplerAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_doppler"), dopplerRelay);
    airAbsorbAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_air_absorb"), airAbsorbRelay);
    roomEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_room_enable"), roomEnableRelay);
    roomErOnlyAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_room_er_only"), roomErOnlyRelay);
    physRateAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_phys_rate"), physRateRelay);
    physWallsAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_phys_walls"), physWallsRelay);
    physPauseAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_phys_pause"), physPauseRelay);
    vizModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_viz_mode"), vizModeRelay);

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

    // Push scene state JSON to the WebView for 3D viewport updates
    auto json = audioProcessor.getSceneStateJSON();
    webView->evaluateJavascript (
        "if(typeof updateSceneState==='function')updateSceneState(" + json + ");");

    auto calibrationJSON = juce::JSON::toString (audioProcessor.getCalibrationStatus());
    webView->evaluateJavascript (
        "if(typeof updateCalibrationStatus==='function')updateCalibrationStatus(" + calibrationJSON + ");");
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
    auto resourcePath = url.fromFirstOccurrenceOf (
        juce::WebBrowserComponent::getResourceProviderRoot(), false, false);

    if (resourcePath.isEmpty() || resourcePath == "/")
        resourcePath = "/index.html";

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

#if JUCE_DEBUG
    if (resourceData != nullptr)
        DBG ("  -> FOUND (" + juce::String (resourceSize) + " bytes)");
    else
        DBG ("  -> NOT FOUND: " + path);
#endif

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
