#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace locusq::editor_shell
{
inline void pushSceneAndCalibrationUpdate (juce::WebBrowserComponent& webView,
                                           const juce::String& sceneJson,
                                           const juce::String& calibrationJson)
{
    webView.evaluateJavascript (
        "(()=>{const __scene=" + sceneJson + ";const __cal=" + calibrationJson
            + ";if(typeof updateSceneState==='function')updateSceneState(__scene);"
              "if(typeof updateCalibrationStatus==='function')updateCalibrationStatus(__cal);})();");
}

inline void notifyHostResized (juce::WebBrowserComponent& webView,
                               const int width,
                               const int height)
{
    webView.evaluateJavascript (
        "if(typeof window.__LocusQHostResized==='function')window.__LocusQHostResized("
            + juce::String (width) + "," + juce::String (height) + ");");
}

// BL-045 Slice C: push drift telemetry to the WebView at 500ms intervals.
inline void pushHeadTrackDrift (juce::WebBrowserComponent& webView,
                                float driftDeg,
                                bool referenceSet)
{
    const juce::String js =
        "(()=>{"
        "const d={type:'headTrackDrift',"
        "driftDeg:" + juce::String (driftDeg, 2) + ","
        "referenceSet:" + (referenceSet ? "true" : "false") + "};"
        "if(typeof window.updateHeadTrackDrift==='function')window.updateHeadTrackDrift(d);"
        "})();";
    webView.evaluateJavascript (js);
}

inline juce::String getRuntimeProbeScript()
{
    return R"JS(
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
}

inline juce::String getSelfTestPollScript()
{
    return R"JS(
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
}
} // namespace locusq::editor_shell

