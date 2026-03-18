Title: LocusQ UI UX Refinement Integration Notes
Document Type: Integration Notes
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# Integration Notes

## three.js + WKWebView

- Keep one explicit render-loop owner for the viewport.
- Treat the viewport as scene truth, not as a dumping ground for every diagnostic.
- Publish a small, stable state bridge:
  - `mode`
  - `sessionState`
  - `trustState`
  - `requestedOutput`
  - `activeOutput`
  - `fallbackReason`
  - `controlOwner`
  - `trackingState`
- Preserve a browser-preview path when the native bridge is absent.
- Keep startup hydration honest:
  - show `booting`
  - show `waiting for native state`
  - show `degraded` when native services do not arrive

## WebView2 Parity

- Avoid relying on backend-specific blur, blend, or input timing behavior for core legibility.
- Use identical information hierarchy on Windows and Apple targets.
- Treat bridge delay and callback ordering as UX concerns, not only engineering concerns.
- Prefer explicit skeleton and error surfaces over optimistic silent retries.

## JUCE Layout Metadata And Control Mapping

- Keep stable control IDs and scene-state keys; do not rename user-facing concepts per format.
- Group renderer truth into one authority payload so the UI can render a single top card.
- Keep diagnostic payloads separate from operator-summary payloads.
- Preserve direct mapping between mode and rail sections:
  - `calibrate`
  - `emitter`
  - `renderer`

## AUv3 Extension Notes

- No plugin UI path should assume app-only services are available inside the extension.
- File import/export and companion-owned capture need explicit extension-safe wording and fallback behavior.
- If a capability is extension-limited, show `Limited in AUv3` with the reason rather than removing the affordance silently.
- Keep DSP and renderer trust semantics format-agnostic; only capability messaging changes.

## CLAP Parity Notes

- Maintain the same top-level navigation and copy as AU/VST3.
- Let host-specific differences appear only in capability or automation metadata, never in information architecture.
- Do not create a separate CLAP diagnostics worldview inside the plugin UI.

## Companion Apple Platform Notes

- Focus mode should foreground:
  - device availability
  - authorization or support state
  - sync / center
  - active profile
  - capture tray
- Privacy contract should remain explicit:
  - local processing
  - no unintended persistence
  - no implicit network transfer
- Use plain-language fallback reasons for profile matching confidence failures.

## HRTF, Steam Audio, And Listening-Gate Notes

- Always show `Requested` and `Active` together.
- When the active path differs from the requested path, show `Why this changed`.
- Steam Audio availability is diagnostic context, not the user-facing headline.
- Personalization claims must stay modest until promotion criteria from listening evidence are met.
- Listening evidence belongs in `Lab` as supporting trust, not as decorative marketing.

## Reactive, Simulation, And Temporal Notes

- Reactive visuals need a documented mapping contract and smoothing policy.
- Simulation-driven audio should remain attached to emitter authoring or renderer lab overlays, not a new top-level mode.
- Temporal behaviors should extend the existing timeline strip and authoring metaphors rather than spawning a separate surface.
- Any future fluid/flocking field needs a bounded fallback and a visible off state.
