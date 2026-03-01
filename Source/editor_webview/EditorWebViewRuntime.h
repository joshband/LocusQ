#pragma once

#include "BinaryData.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <unordered_map>
#include <vector>

namespace locusq::editor_webview
{
struct RuntimeConfig
{
    bool selfTestEnabled = false;
    bool selfTestBl009Enabled = false;
    bool selfTestBl011Enabled = false;
    bool useIncrementalUi = false;
    juce::String selfTestScope;
    int selfTestTimeoutTicks = 600;
};

inline bool readFeatureFlag (const char* envName)
{
    if (const auto* flag = std::getenv (envName))
    {
        const auto value = juce::String (flag).trim().toLowerCase();
        return value.isNotEmpty() && value != "0" && value != "false" && value != "off";
    }

    return false;
}

inline juce::String readSelfTestScope()
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

inline RuntimeConfig makeRuntimeConfig()
{
    RuntimeConfig config;
    config.selfTestEnabled = readFeatureFlag ("LOCUSQ_UI_SELFTEST");
    config.selfTestBl009Enabled = readFeatureFlag ("LOCUSQ_UI_SELFTEST_BL009");
    config.selfTestBl011Enabled = readFeatureFlag ("LOCUSQ_UI_SELFTEST_BL011");
    config.selfTestScope = readSelfTestScope();

    if (const auto* variant = std::getenv ("LOCUSQ_UI_VARIANT"))
    {
        const auto value = juce::String (variant).trim().toLowerCase();
        if (value == "incremental" || value == "stage12")
            config.useIncrementalUi = true;
        else if (value == "production" || value == "index")
            config.useIncrementalUi = false;
    }
    else
    {
        config.useIncrementalUi = config.selfTestEnabled;
    }

    if (config.selfTestScope.isNotEmpty())
        config.selfTestTimeoutTicks = 1200; // ~40s @ 30Hz
    else if (config.selfTestBl009Enabled || config.selfTestBl011Enabled)
        config.selfTestTimeoutTicks = 900; // ~30s @ 30Hz

    return config;
}

inline juce::String getInitialUiResourcePath (const RuntimeConfig& config)
{
    return config.useIncrementalUi ? "/incremental/index.html"
                                   : "/index.html";
}

inline juce::String getStandaloneWindowTitle (const RuntimeConfig& config)
{
    const auto uiLabel = config.useIncrementalUi ? "incremental-stage12"
                                                  : "production-ui";
    return juce::String (JucePlugin_Name)
        + " v" + juce::String (JucePlugin_VersionString)
        + " [" + uiLabel + "]";
}

inline juce::String makeInitialUrl (const RuntimeConfig& config)
{
    const auto initialResourcePath = getInitialUiResourcePath (config);
    const auto normalizedInitialPath = (initialResourcePath == "/") ? "/index.html" : initialResourcePath;
    const auto cacheBust = juce::String (juce::Time::getCurrentTime().toMilliseconds());

    auto initialUrl = juce::WebBrowserComponent::getResourceProviderRoot()
        + normalizedInitialPath + "?cb=" + cacheBust;

    if (config.selfTestEnabled)
        initialUrl += "&selftest=1";

    if (config.selfTestBl009Enabled)
        initialUrl += "&selftest_bl009=1";

    if (config.selfTestBl011Enabled)
        initialUrl += "&selftest_bl011=1";

    if (config.selfTestScope.isNotEmpty())
        initialUrl += "&selftest_scope=" + config.selfTestScope;

    return initialUrl;
}

inline juce::File getUiSelfTestResultFile()
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

inline juce::WebBrowserComponent::Options makeBaseWebViewOptions()
{
    auto options = juce::WebBrowserComponent::Options{};

#if JUCE_WINDOWS
    options = options
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder (juce::File::getSpecialLocation (
                    juce::File::SpecialLocationType::tempDirectory)));
#else
    options = options.withBackend (juce::WebBrowserComponent::Options::Backend::defaultBackend);
#endif

    return options.withNativeIntegrationEnabled();
}

template <typename ProcessorType>
inline juce::WebBrowserComponent::Options withNativeBindings (
    juce::WebBrowserComponent::Options options,
    ProcessorType& audioProcessor,
    const RuntimeConfig& runtimeConfig)
{
    return std::move (options)
        .withNativeFunction ("locusqStartCalibration",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.startCalibrationFromUI (opt));
                             })
        .withNativeFunction ("locusqAbortCalibration",
                             [&audioProcessor] (const juce::Array<juce::var>&,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 audioProcessor.abortCalibrationFromUI();
                                 completion (true);
                             })
        .withNativeFunction ("locusqRedetectCalibrationRouting",
                             [&audioProcessor] (const juce::Array<juce::var>&,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.redetectCalibrationRoutingFromUI());
                             })
        .withNativeFunction ("locusqListCalibrationProfiles",
                             [&audioProcessor] (const juce::Array<juce::var>&,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.listCalibrationProfilesFromUI());
                             })
        .withNativeFunction ("locusqSaveCalibrationProfile",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.saveCalibrationProfileFromUI (opt));
                             })
        .withNativeFunction ("locusqLoadCalibrationProfile",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.loadCalibrationProfileFromUI (opt));
                             })
        .withNativeFunction ("locusqRenameCalibrationProfile",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.renameCalibrationProfileFromUI (opt));
                             })
        .withNativeFunction ("locusqDeleteCalibrationProfile",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.deleteCalibrationProfileFromUI (opt));
                             })
        .withNativeFunction ("locusqGetKeyframeTimeline",
                             [&audioProcessor] (const juce::Array<juce::var>&,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.getKeyframeTimelineForUI());
                             })
        .withNativeFunction ("locusqSetKeyframeTimeline",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
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
                             [&audioProcessor] (const juce::Array<juce::var>& args,
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
                             [&audioProcessor] (const juce::Array<juce::var>&,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.listEmitterPresetsFromUI());
                             })
        .withNativeFunction ("locusqSaveEmitterPreset",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.saveEmitterPresetFromUI (opt));
                             })
        .withNativeFunction ("locusqLoadEmitterPreset",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.loadEmitterPresetFromUI (opt));
                             })
        .withNativeFunction ("locusqRenameEmitterPreset",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.renameEmitterPresetFromUI (opt));
                             })
        .withNativeFunction ("locusqDeleteEmitterPreset",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::var opt = args.isEmpty() ? juce::var() : args[0];
                                 completion (audioProcessor.deleteEmitterPresetFromUI (opt));
                             })
        .withNativeFunction ("locusqSetForwardYaw",
                             [&audioProcessor] (const juce::Array<juce::var>&,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 // BL-045-C: capture current raw yaw as the re-center reference.
                                 const float rawYaw = audioProcessor.lastHeadTrackYawDeg.load (
                                     std::memory_order_relaxed);
                                 audioProcessor.setYawReference (rawYaw);
                                 completion (true);
                             })
        .withNativeFunction ("locusqGetUiState",
                             [&audioProcessor] (const juce::Array<juce::var>&,
                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 completion (audioProcessor.getUIStateFromUI());
                             })
        .withNativeFunction ("locusqSetUiState",
                             [&audioProcessor] (const juce::Array<juce::var>& args,
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
                             [&audioProcessor] (const juce::Array<juce::var>& args,
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
                             [selfTestEnabled = runtimeConfig.selfTestEnabled]
                             (const juce::Array<juce::var>& args,
                              juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const auto resultFile = getUiSelfTestResultFile();
                                 resultFile.getParentDirectory().createDirectory();

                                 juce::var payload (new juce::DynamicObject());
                                 if (auto* object = payload.getDynamicObject())
                                 {
                                     object->setProperty ("timestampUtc", juce::Time::getCurrentTime().toISO8601 (true));
                                     object->setProperty ("selftestEnabled", selfTestEnabled);
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
                             });
}

inline std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url)
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

    const char* resourceData = nullptr;
    int resourceSize = 0;
    juce::String mimeType;

    const auto path = resourcePath.substring (1);

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
} // namespace locusq::editor_webview
