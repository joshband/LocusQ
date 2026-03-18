Title: BL-038 Calibration Threading and Telemetry QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-038 Calibration Threading and Telemetry QA Contract

## Status

Active QA contract.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/bl-038-calibration-threading-and-telemetry-qa-legacy.md`

## Goal

Keep BL-038 threading ownership, RT-safe telemetry publication, timeout/error handling, and replay lanes deterministic.

## Linked Authority

- `Documentation/backlog/bl-038-calibration-threading-and-telemetry.md`
- `Documentation/invariants.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`

## Core Checks

| Area | Contract |
|---|---|
| A1 parity | `BL038-A1-001..010` keep ownership, state, telemetry, thresholds, taxonomy, artifact schema, and docs freshness aligned. |
| A1 threading | `BL038-QA-001..005` keeps owner exclusivity, handoff policy, tie-breaks, finiteness, and monotonic sequence rules explicit. |
| A1 timing | `BL038-QA-006..009` covers timeout/error classes and stale-state handling. |
| B1 lane | `BL038-B1-001..008` keeps the executable lane schema and replay contract auditable. |
| B1 classes | `deterministic_contract_failure`, `runtime_execution_failure`, `missing_result_artifact`, `deterministic_replay_divergence`, `deterministic_replay_row_drift`. |
| C2/C3/C5 | soak and sentinel runs preserve replay stability and exit semantics at `10`, `20`, and the guard cases. |

## Validation Plan

- `./scripts/validate-docs-freshness.sh`
- `node --check Source/ui/src/index.ts`
- `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl038_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c3_replay_sentinel_<timestamp>/contract_runs`

## Evidence Bundle

Primary bundle roots:
- `TestEvidence/bl038_slice_a1_contract_<timestamp>/`
- `TestEvidence/bl038_slice_b1_lane_<timestamp>/`
- `TestEvidence/bl038_slice_c2_soak_<timestamp>/`
- `TestEvidence/bl038_slice_c3_replay_sentinel_<timestamp>/`

Required files:
- `status.tsv`
- `acceptance_matrix.tsv` or `validation_matrix.tsv`
- `failure_taxonomy.tsv`
- `replay_hashes.tsv`
- `lane_notes.md` or `soak_summary.tsv`
- `docs_freshness.log`

## Risks

- Ownership drift can turn into hidden RT regressions.
- Telemetry schema gaps can break the downstream replay parser.
- Stale thresholds can make warnings look like passes.

## Archive Note

The verbose original runbook is preserved in the archive copy above.
Use this active file for current QA decisions and the archive file for deep detail.
