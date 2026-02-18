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
    // 2. Create attachments BEFORE WebView
    // 3. Create WebBrowserComponent
    // 4. addAndMakeVisible LAST
    //==========================================================================

    // Create parameter attachments
    modeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("mode"), modeRelay);
    bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("bypass"), bypassRelay);

    azimuthAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_azimuth"), azimuthRelay);
    elevationAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_elevation"), elevationRelay);
    distanceAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pos_distance"), distanceRelay);

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

    physEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *audioProcessor.apvts.getParameter ("phys_enable"), physEnableRelay);

    masterGainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_master_gain"), masterGainRelay);
    qualityAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *audioProcessor.apvts.getParameter ("rend_quality"), qualityRelay);

    // Create WebBrowserComponent with platform-aware backend
    DBG ("LocusQ: Creating WebView");
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (juce::File::getSpecialLocation (
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })
            .withOptionsFrom (modeRelay)
            .withOptionsFrom (bypassRelay)
            .withOptionsFrom (azimuthRelay)
            .withOptionsFrom (elevationRelay)
            .withOptionsFrom (distanceRelay)
            .withOptionsFrom (emitGainRelay)
            .withOptionsFrom (spreadRelay)
            .withOptionsFrom (directivityRelay)
            .withOptionsFrom (muteRelay)
            .withOptionsFrom (soloRelay)
            .withOptionsFrom (physEnableRelay)
            .withOptionsFrom (masterGainRelay)
            .withOptionsFrom (qualityRelay));

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

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
