Title: BL-037 Emitter Snapshot CPU Budget QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-037 Emitter Snapshot CPU Budget QA Contract

## Status

Active QA contract.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/bl-037-emitter-snapshot-cpu-budget-qa-legacy.md`

## Goal

Keep BL-037 snapshot cadence, CPU budget, replay stability, and failure taxonomy deterministic.

## Linked Authority

- `Documentation/backlog/bl-037-emitter-snapshot-cpu-budget.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`

## Core Checks

| Area | Contract |
|---|---|
| A1 cadence | `BL037-QA-001..008` define snapshot cadence, late-join window, CPU budget, replay identity, fallback taxonomy, and artifact schema. |
| A1 acceptance | `BL037-A1-001..008` stay parity-locked with the runbook and evidence packet. |
| A1 taxonomy | `BL037-FX-001..008` covers contract, replay, non-finite metric, and evidence failures. |
| B1 lane | `BL037-B1-001..008` keeps the lane contract, outputs, and replay hash stability machine-auditable. |
| B1 taxonomy | `BL037-B1-FX-001..020` covers preflight, contract, lane, replay, and artifact failures. |
| C2/C3/C4 | soak and sentinel runs stay deterministic at `10`, `20`, and `50` runs. |

## Validation Plan

- `./scripts/validate-docs-freshness.sh`
- `bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh`
- `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help`
- `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl037_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c3_replay_sentinel_<timestamp>/contract_runs`
- `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl037_slice_c4_soak_<timestamp>/contract_runs`

## Evidence Bundle

Primary bundle roots:
- `TestEvidence/bl037_slice_a1_contract_<timestamp>/`
- `TestEvidence/bl037_slice_b1_lane_<timestamp>/`
- `TestEvidence/bl037_slice_c2_soak_<timestamp>/`
- `TestEvidence/bl037_slice_c3_replay_sentinel_<timestamp>/`
- `TestEvidence/bl037_slice_c4_soak_<timestamp>/`

Required files:
- `status.tsv`
- `acceptance_matrix.tsv` or `validation_matrix.tsv`
- `failure_taxonomy.tsv`
- `replay_hashes.tsv`
- `lane_notes.md` or `soak_summary.tsv`
- `docs_freshness.log`

## Risks

- Replay drift can hide behind passing syntax/help checks.
- Evidence schema drift can break machine parsing.
- Large soak runs can regress if the taxonomy gets out of sync with the lane contract.

## Archive Note

The verbose original runbook is preserved in the archive copy above.
Use this active file for current QA decisions and the archive file for deep detail.
