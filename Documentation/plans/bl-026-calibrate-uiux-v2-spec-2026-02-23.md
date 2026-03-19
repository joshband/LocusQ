Title: BL-026 Calibrate View V2 Multi-Topology UI/UX Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# BL-026 Calibrate View V2 Multi-Topology UI/UX Spec

## Status

Approved. This is the active BL-026 execution contract.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23-legacy.md`

Backlog authority:
- `Documentation/backlog/index.md`

Companion specs:
- BL-025 emitter v2
- BL-027 renderer v2

## Goal

Make CALIBRATE profile-first.
Operators should choose a topology, monitoring path, and device profile, then run, validate, and save a repeatable calibration without losing routing safety or host reliability.

## Scope

- Support mono, stereo, quad, surround, ambisonic, binaural, and downmix-validation flows.
- Keep `start`, `abort`, and `measure again` deterministic.
- Persist calibration profiles by topology plus monitoring tuple.
- Preserve existing `cal_*` contracts where possible.

## Core Contracts

| Area | Contract |
|---|---|
| Profile authority | Every run is tagged with `topology`, `monitoring path`, and `device profile`. |
| Mapping | Topology controls output rows. Redetect never silently overwrites custom routes. |
| Validation | The UI must show pass/fail for map, phase/polarity, delay, and profile activation. |
| Host parity | Standalone, REAPER VST3, and REAPER CLAP keep the same payload semantics. |
| Profile library | Save/load/rename/delete preserves metadata and last validation result. |

## Delivery Order

### Slice A

- Rebuild CALIBRATE into staged cards: Profile, Mapping, Mic/Stimulus, Run, Validation, Library.
- Add compact status chips and responsive layout tokens.

### Slice B

- Add `cal_topology_profile`.
- Add the alias table to renderer profile strings.
- Publish resolved topology in status payloads.

### Slice C

- Replace fixed `SPK1..SPK4` rows with topology-driven mapping rows.
- Keep auto-routing as bootstrap only.
- Block run start when custom mapping is invalid or wider than the active runtime supports.

### Slice D

- Surface headphone and spatial activation diagnostics in the validation block.
- Show exact fallback or init stage when requested mode is not active.

### Slice E

- Add profile CRUD with inline naming.
- Persist the topology plus monitoring plus device tuple and the last validation summary.

## Validation Plan

### Automated

- `cd Source/ui && npm run typecheck`
- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
- `UI-P1-026A..E`
- `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`
- `./scripts/validate-docs-freshness.sh`

### Manual

- AirPods Pro 2 headphone validation
- Sony WH-1000XM5 headphone validation
- binaural vs `stereo_downmix` A/B checks
- multi-topology host session review for map accuracy and validation clarity
- profile library CRUD across at least two topology/device tuples

## Evidence Bundle

Canonical closeout bundle:
- `TestEvidence/bl026_calibrate_v2_<timestamp>/`

Required artifacts:
- `status.tsv`
- `report.md`
- `ui_selftest_production.json`
- `reaper_headless_status.json`
- `docs_freshness.log`
- `manual_headphone_checks.md`

## Risks

- Topology sprawl can confuse the panel. Keep the flow staged and profile-first.
- Auto-detect can override operator intent. Require explicit overwrite for custom maps.
- Compact layouts can hide validation state. Keep status chips and pass/fail rows visible.
- Host/runtime mismatches must stay visible inside the validation block.

## Handoff Requirements

- Share one alias dictionary with BL-027.
- Share `requested`, `active`, and `stage` chip semantics with BL-027.
- Keep BL-025 visual language compatible where cards and chips overlap.

## Exit Criteria

- `UI-P1-026A..E` pass.
- BL-025, BL-019, and BL-022 assertions stay green.
- REAPER host smoke lane passes with fresh evidence.
- Manual headphone checks are logged.
- Docs freshness passes.
- Backlog, status, and evidence surfaces stay synchronized.

## Visual Aid Index

| Artifact | Use |
|---|---|
| Backlog row | Current authority and closure state. |
| Self-test lanes | Control, routing, and validation proof. |
| Evidence bundle | Host matrix, manual checks, and docs-freshness logs. |

## Archive Note

The original long-form plan is preserved in the archive copy above.
Use this active file for execution decisions and the archive file for deep implementation detail.
