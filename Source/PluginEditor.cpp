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
        .withResourceProvider ([] (const auto& url) { return locusq::editor_webview::getResource (url); });

    webViewOptions = parameterBridgeRelays.addToOptions (std::move (webViewOptions));

    webView = std::make_unique<juce::WebBrowserComponent> (std::move (webViewOptions));

    // Create parameter attachments before exposing/loading the WebView.
    parameterBridgeAttachments.bindToParameters (audioProcessor.apvts, parameterBridgeRelays);

    addAndMakeVisible (*webView);

    const auto initialResourcePath = locusq::editor_webview::getInitialUiResourcePath (runtimeConfig);
    const auto initialUrl = locusq::editor_webview::makeInitialUrl (runtimeConfig);

    DBG ("LocusQ: Loading UI path " + initialResourcePath);
    webView->goToURL (initialUrl);

    // Keep a responsive editor timer, but tier expensive bridge work under BL-097.
    startTimerHz (kEditorTimerHz);

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

    ++bridgeTickCount;
    updateStandaloneWindowTitle();
    if ((bridgeTickCount % kCalibrationProfilePollIntervalTicks) == 0)
        audioProcessor.pollCompanionCalibrationProfileFromDisk();
    audioProcessor.applyPendingCompanionCalibrationProfileReload();

    pushBridgePayloadsIfDue();

    // BL-045 Slice C: push drift telemetry at ~500ms intervals (every 15 ticks at 30Hz).
    if (++driftTelemetryTickCount >= kDriftTelemetryIntervalTicks)
    {
        driftTelemetryTickCount = 0;
        const bool refSet = audioProcessor.yawReferenceSet.load (std::memory_order_relaxed);
        const float rawYaw = audioProcessor.lastHeadTrackYawDeg.load (std::memory_order_relaxed);
        const float refYaw = audioProcessor.yawReferenceDeg.load (std::memory_order_relaxed);
        const float driftDeg = refSet ? std::abs (rawYaw - refYaw) : 0.0f;
        locusq::editor_shell::pushHeadTrackDrift (*webView, driftDeg, refSet);
    }

    if (! runtimeProbeDone)
    {
        ++runtimeProbeTicks;

        if (runtimeProbeTicks >= 60)
        {
            runtimeProbeDone = true;

            auto diagnosticsDirectory = locusq::processor_bridge::getUserDataSubdirectory ("Diagnostics");
            diagnosticsDirectory.createDirectory();
            const auto probeFile = diagnosticsDirectory.getChildFile ("locusq_webview_runtime_probe.json");

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

void LocusQAudioProcessorEditor::pushBridgePayloadsIfDue()
{
    if (webView == nullptr)
        return;

    const bool publishSceneNow = (bridgeTickCount % kStructuralScenePublishIntervalTicks) == 0;
    const bool publishCalibrationNow = (bridgeTickCount % kCalibrationStatusPublishIntervalTicks) == 0;

    if (! publishSceneNow && ! publishCalibrationNow)
        return;

    juce::String sceneJSON;
    if (publishSceneNow)
        sceneJSON = audioProcessor.getSceneStateJSON();

    juce::String calibrationJSON;
    if (publishCalibrationNow)
    {
        calibrationJSON = juce::JSON::toString (audioProcessor.getCalibrationStatus());
        if (calibrationJSON == lastCalibrationPayload && ! publishSceneNow)
            return;
        lastCalibrationPayload = calibrationJSON;
    }

    if (publishSceneNow && publishCalibrationNow)
    {
        locusq::editor_shell::pushSceneAndCalibrationUpdate (*webView, sceneJSON, calibrationJSON);
        return;
    }

    if (publishSceneNow)
    {
        locusq::editor_shell::pushSceneUpdate (*webView, sceneJSON);
        return;
    }

    locusq::editor_shell::pushCalibrationUpdate (*webView, calibrationJSON);
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
