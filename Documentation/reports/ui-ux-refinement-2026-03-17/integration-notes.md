Title: LocusQ UI UX Refinement Integration Notes
Document Type: Integration Notes
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# Integration Notes

## Goal

Keep the refinement direction implementable across JUCE, browser preview, and companion-platform constraints.

Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reports/integration-notes-2026-03-17-legacy.md`

## Integration Rules

### Viewport And Bridge

- keep one explicit render-loop owner
- treat the viewport as scene truth, not a diagnostic dumping ground
- publish a small stable bridge: `mode`, `sessionState`, `trustState`, `requestedOutput`, `activeOutput`, `fallbackReason`, `controlOwner`, `trackingState`
- show explicit `booting`, `waiting for native state`, and `degraded` states

### Runtime Parity

- keep the same information hierarchy on WKWebView and WebView2
- do not depend on backend-specific blur, blend, or timing for legibility
- preserve browser-preview fallback when native bridge is absent

### JUCE And Control Mapping

- keep stable control IDs and scene-state keys
- group renderer truth into one authority payload
- keep diagnostic payloads separate from operator-summary payloads
- preserve direct mapping between mode and rail sections

### Format And Platform Notes

- AUv3: show capability limits explicitly instead of hiding unavailable functions
- CLAP: keep the same information architecture as AU/VST3
- Companion: foreground device availability, sync/center, active profile, and capture tray
- Privacy: keep local-processing and no-implicit-transfer rules visible

## Archive Note

The full integration notes are preserved in the archive copy above.
Use this file for current integration constraints and the archive file for detailed rationale.
