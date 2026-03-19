Title: LocusQ UI UX Refinement Component Specs
Document Type: Design Specification
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# Component Specs

## Goal

Translate the UI refinement direction into a small, stable component contract for the plugin and the Head-Tracking Companion.

Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reports/component-specs-2026-03-17-legacy.md`

## Core Rules

- the viewport stays primary
- each mode answers one dominant question
- `Requested` versus `Active` must stay visible for trust
- diagnostics are available, but not the default reading path
- the companion defaults to `Focus`; `Lab` is opt-in

## Required Outputs

| Surface | Required Component Contract |
|---|---|
| Plugin shell | compact header, mode tabs, session truth, one trust badge |
| Renderer | always-visible authority card with `Requested`, `Active`, `Why this changed`, `Control owner` |
| Mode rails | task-first cards with diagnostics behind drawers |
| Viewport | scene truth, selection context, degraded-mode badge, optional overlays default off |
| Companion Focus | readiness, sync/center, active profile, capture tray |
| Companion Lab | packet age, axis sanity, matcher scores, fallback reason, debug views |
| Capture tray | left/right/front slots, privacy note, capture-quality hint, apply/retake flow |

## Accessibility And Runtime Rules

- state must be readable without color alone
- keyboard order follows task order
- keep stable control IDs and scene-state keys
- preserve browser-preview behavior without native bridge
- keep AUv3, CLAP, WKWebView, and WebView2 on the same information architecture

## Archive Note

The full component packet is preserved in the archive copy above.
Use this file for the active component contract and the archive file for detailed component taxonomy.
