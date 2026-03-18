Title: BL-027 Renderer View V2 Multi-Profile UI/UX Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# BL-027 Renderer View V2 Multi-Profile UI/UX Spec

## Status

Approved. This is the active BL-027 execution contract.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23-legacy.md`

Companion specs:
- BL-025 emitter v2
- BL-026 calibrate v2

## Goal

Make renderer profile authority obvious.
Show `requested`, `active`, and `stage` together.
Keep routing, diagnostics, and scene controls compact and deterministic in JUCE WebView hosts.

## Scope

- Profile authority
- Dynamic output and speaker presentation
- Diagnostics chips and failure-stage visibility
- Scene monitor labeling
- BL-026 alias coherence

Out of scope:
- DSP algorithm rewrites
- breaking APVTS renames
- head-tracking bridge work
- codec completion beyond existing placeholders

## Core Contracts

| Area | Contract |
|---|---|
| Requested state | `Spatial Profile`, `Headphone Mode`, and `Headphone Profile` update immediately. |
| Active state | UI always shows requested vs active values together. |
| Stage | `stage` is mandatory whenever active differs from requested. |
| Alias dictionary | Operator-facing names must match BL-026. |
| Scene actions | Controls read `Solo` and `Mute`, not terse one-letter buttons. |

## Delivery Order

### Slice A

- Rebuild the panel into staged cards: Profile Authority, Output/Speakers, Spatialization, Room, Diagnostics, Scene.

### Slice B

- Bind `Spatial Profile`.
- Add the shared alias table.

### Slice C

- Replace fixed speaker rows with active-layout-driven output rows.
- Surface fallback-aware route readback.

### Slice D

- Split overlay-only diagnostics into explicit Steam and Ambisonic cards.
- Show deterministic stage and error detail inline.

### Slice E

- Tighten scene labels and authority cues.
- Keep cross-panel copy aligned with BL-026.

## Validation Plan

### Automated

- `cd Source/ui && npm run typecheck`
- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
- `UI-P2-027A..E`
- `./scripts/standalone-ui-selftest-production-p0-mac.sh`
- `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`
- `./scripts/validate-docs-freshness.sh`

### Manual

- compact, medium, and wide host layout review
- stereo, quad, surround, and ambisonic profile switching
- headphone binaural vs downmix checks
- AirPods Pro 2 and Sony WH-1000XM5 verification
- CALIBRATE/RENDERER cross-panel sync review

## Risks

- Alias drift between BL-026 and BL-027
- compact-window clipping if diagnostics stay too dense
- host-specific WebView refresh or selection quirks

## Exit Criteria

- `UI-P2-027A..E` pass.
- BL-025, BL-026, and BL-019 assertions remain green.
- Host smoke lane passes with fresh evidence.
- Backlog, status, and evidence surfaces stay synchronized.
- Docs freshness passes.

## Visual Aid Index

| Artifact | Use |
|---|---|
| Active chips and tables | Quick requested/active/stage readback. |
| Archived legacy spec | Screenshots, overlay examples, and deep fallback cases. |

## Archive Note

The original long-form spec is preserved in the archive copy above.
Use this active file as the canonical execution surface.
