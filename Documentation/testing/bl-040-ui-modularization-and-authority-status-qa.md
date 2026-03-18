Title: BL-040 UI Modularization and Authority Status QA
Document Type: QA Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-040 UI Modularization and Authority Status QA

## Status

Active. This is the short QA contract for BL-040 authority-state and replay checks.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/bl-040-ui-modularization-and-authority-status-qa-legacy.md`

## Goal

Keep authority provenance, stale/lock/fallback signaling, and replay semantics machine-checkable.
Keep the UI diagnostics contract deterministic across runs.

## Linked Authority

- Runbook: `Documentation/backlog/bl-040-ui-modularization-and-authority-status.md`
- Contracts: `Documentation/invariants.md`, `Documentation/scene-state-contract.md`

## Core Contracts

| Area | Contract |
|---|---|
| A1 authority | Precedence, status classes, stale thresholds, required fields, module boundaries, replay sequence, and taxonomy IDs are explicit. |
| B1 bootstrap | Authority diagnostic card presence, default collapsed state, payload detection, toggle wiring, and chip mapping are explicit. |
| C5 semantics | Negative probes must exit `2` and deterministic replay must remain stable. |
| C6 long-run | `runs=50` soak stays stable with no signature or row drift. |
| D1 readiness | `runs=75` done-candidate packet keeps the same semantics and evidence shape. |

## Validation Plan

### A1/B1

- `BL040-A1-001..009`
- `BL040-FX-001..007`
- `BL040-B1-001..008`
- `BL040-FX-101..106`
- Scripts: `node --check Source/ui/src/index.ts`, `bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh`, `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help`

### C5/C6/D1

- C5: `runs=20`, `--runs 0` exits `2`, `--bad-flag` exits `2`
- C6: `runs=50`, same exit semantics
- D1: `runs=75`, same exit semantics

### Shared Outputs

- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `exit_semantics_probe.tsv`
- `docs_freshness.log`

## Evidence Bundle

Common packet roots:
- `TestEvidence/bl040_slice_a1_contract_<timestamp>/`
- `TestEvidence/bl040_slice_b1_ui_<timestamp>/`
- `TestEvidence/bl040_slice_c5_ui_semantics_<timestamp>/`
- `TestEvidence/bl040_slice_c6_ui_longrun_<timestamp>/`
- `TestEvidence/bl040_slice_d1_done_candidate_<timestamp>/`

## Risks

- Alias drift can break authority-state parity.
- Compact UI changes can hide diagnostics if chips are not preserved.
- Exit-semantics probes can regress if scripts change usage handling.

## Archive Note

The full historical snapshots, soak runs, and replay packets live in the archive copy above.
Use this active file as the short contract and the archive file for legacy detail.
