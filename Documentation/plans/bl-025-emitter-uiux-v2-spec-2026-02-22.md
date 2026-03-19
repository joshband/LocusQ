Title: BL-025 Emitter View V2 UI/UX Consolidation Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-03-18

# BL-025 Emitter View V2 UI/UX Consolidation Spec

## Status

Approved. This is the active BL-025 execution contract.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22-legacy.md`

## Goal

Make EMITTER easier to author and harder to misuse.
Keep position, motion, presets, and diagnostics readable in compact WebView hosts.

## Scope

- position mode clarity
- motion source consolidation
- preset lifecycle
- local vs remote authority
- emitter-facing diagnostics

Out of scope:
- DSP math changes
- cross-instance coordination
- breaking preset-format migration

## Core Contracts

| Area | Contract |
|---|---|
| Position | Spherical and Cartesian views both exist, but only the active mode is editable. |
| Viewport sync | Drag updates both coordinate forms without breaking timeline tracks. |
| Motion | `Motion Source` owns transport context and loop/sync semantics. |
| Presets | Save/load/rename/delete are inline and deterministic. |
| Authority | Remote emitters show read-only controls and an explicit scope badge. |

## Delivery Order

### Slice 1

- Rebuild the rail into clearer sections.
- Add responsive layout tokens.

### Slice 2

- Add Cartesian rows.
- Add mode-aware editability for position controls.

### Slice 3

- Unify motion transport.
- Remove duplicated loop and sync semantics.

### Slice 4

- Add typed emitter and motion presets.
- Replace prompt-based naming with inline naming and native rename/delete.

### Slice 5

- Add local vs remote authority cues.
- Collapse diagnostics by default.

## Validation Plan

### Automated

- `cd Source/ui && npm run typecheck`
- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
- `UI-P1-025A..C`
- `./scripts/standalone-ui-selftest-production-p0-mac.sh`
- `build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
- `./scripts/validate-docs-freshness.sh`

### Manual

- standalone emitter authoring pass
- host DAW prompt and input-behavior spot-check
- narrow and wide resize review
- preset lifecycle review

## Risks

- host-specific WebView event quirks
- timeline coupling regressions after motion unification
- preset labels drifting away from action semantics

## Exit Criteria

- BL-025 self-test lanes pass.
- BL-019 and BL-022 assertions remain green.
- Docs freshness passes.
- Backlog, status, and evidence stay synchronized.

## Visual Aid Index

| Artifact | Use |
|---|---|
| Active chips and tables | Fast state readback during implementation. |
| Archived legacy spec | Resize captures and preset-lifecycle detail. |

## Archive Note

The original long-form spec is preserved in the archive copy above.
Use this active file as the canonical execution surface.
