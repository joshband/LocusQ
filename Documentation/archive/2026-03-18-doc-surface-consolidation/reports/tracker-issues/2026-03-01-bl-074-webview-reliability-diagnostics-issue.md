Title: Tracker Issue Draft - BL-074 WebView Runtime Reliability Diagnostics
Document Type: Tracker Issue Draft
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-074 Tracker Issue Draft

## Proposed Title

BL-074: WebView runtime reliability diagnostics (strict gesture + degraded mode)

## Summary

Raise UI runtime trust by failing strict self-tests on gesture fallback paths and surfacing binding/native-call failures through deterministic degraded-mode diagnostics.

## Evidence

- `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md` (Findings #8, #14, #15, #16)
- `Source/ui/public/js/index.js:6689`
- `Source/ui/public/js/index.js:6700`
- `Source/ui/public/js/index.js:6743`
- `Source/ui/public/js/index.js:6804`
- `Source/editor_webview/EditorWebViewRuntime.h:386`
- `Source/ui/public/js/index.js:5670`
- `Source/ui/public/js/index.js:5681`

## Acceptance Checklist

- [ ] CI strict-gesture mode fails when fallback mutation path is used.
- [ ] Critical startup binding failures force degraded mode.
- [ ] Native timeline call failures increment visible diagnostics counters.
- [ ] Reliability diagnostics artifacts are present in promotion packet.
