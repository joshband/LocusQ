---
name: juce-webview-runtime
description: Advanced JUCE WebView runtime integration, host compatibility, debugging, and QA guidance for plugin UIs.
---

Title: JUCE WebView Runtime Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-03-01

# JUCE WebView Runtime

Use this skill for deep runtime issues in JUCE WebView plugin UIs (interop glitches, input hit-testing issues, host-specific behavior, callback timing, and bridge reliability).

## Workflow
1. Establish host/runtime matrix (DAW, plugin format, OS, JUCE version).
2. Confirm bridge contract and startup ordering.
3. Instrument JS/native boundaries and timeout paths.
4. Reproduce with a minimal host scenario.
5. Apply fix with explicit fallback behavior.
6. Validate across targeted host matrix and document residual risks.

## Reference Map
- `references/host-behavior-matrix.md`: Host-specific behavior tracking template.
- `references/bridge-contract-and-fallbacks.md`: JS/native contract and fallback rules.
- `references/debugging-playbook.md`: Diagnostic steps and capture checklist.
- `references/qa-and-evidence.md`: Acceptance matrix and evidence requirements.

## Backend Scope: WKWebView vs WebView2
- `WKWebView` (Apple) and `WebView2` (Windows) must be treated as distinct runtime targets.
- Differences that matter for bug triage and QA:
  - Process/runtime model and crash isolation behavior.
  - JS/native bridge entry points and callback timing semantics.
  - Input focus/hit-testing behavior for embedded plugin UIs.
  - Storage/session/profile behavior and persistence scope.
- Every runtime bug write-up must include backend, host, plugin format, and reproducible steps.

## Execution Rules
- Preserve relay/attachment/member-order constraints in JUCE editor classes.
- Treat bridge timeouts and callback ordering as first-class failure modes.
- Keep browser-preview fallbacks functional for non-host testing.
- Add explicit status/error feedback in UI for native call failures.
- Never ship host-specific hacks without documenting scope and rationale.

## Startup Hydration And Sync Contract
- Validate startup sequencing for runtime states that depend on hardware readiness.
- Prefer explicit UI gating (ready/not-ready + manual sync) over implicit startup assumptions.
- Capture bridge ordering and timeout behavior when state transitions involve async device availability.

## Deliverables
- Repro steps (host + format + exact action path).
- Root-cause summary across JS and native sides.
- Validation status as `tested`, `partially tested`, or `not tested`.
