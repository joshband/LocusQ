Title: BL-041 Doppler v2 and VBAP Geometry Validation QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-041 Doppler v2 and VBAP Geometry Validation QA Contract

## Purpose

Define the deterministic QA contract for BL-041 Doppler v2 and VBAP geometry validation.
This file is now a compact support surface.
The detailed execution chronology lives in the done runbook and evidence packets.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/done/bl-041-doppler-v2-and-vbap-geometry-validation.md`

Traceability anchors:
- `.ideas/parameter-spec.md`
- `.ideas/architecture.md`
- `Documentation/invariants.md`

Current posture:
- `Done`
- Current support role: retained QA contract and evidence index
- Current highest-signal evidence: `TestEvidence/bl041_slice_d2_done_promotion_20260227T201910Z/`

## Core QA Contract

### A1 Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL041-QA-001 | Doppler field coverage | required fields, ranges, and fallback rules are explicit |
| BL041-QA-002 | Doppler smoothing thresholds | all smoothing and continuity thresholds are explicit |
| BL041-QA-003 | Non-finite fail-closed behavior | finite-only and fallback policy are explicit |
| BL041-QA-004 | Geometry threshold coverage | `area_epsilon`, boundary window, and gain limits are explicit |
| BL041-QA-005 | Deterministic triplet tie-break | sorted tie-break rule is explicit |
| BL041-QA-006 | Boundary continuity policy | crossfade and discontinuity thresholds are explicit |
| BL041-QA-007 | Replay hash contract | deterministic hash input set is explicit |
| BL041-QA-008 | Replay equality contract | identical input yields identical trace output |
| BL041-QA-009 | Artifact schema contract | required files and TSV columns are explicit |

### Stage Tiers

| Tier | Runs | Purpose | Evidence Shape |
|---|---:|---|---|
| A1 | docs-only | contract authority | `status.tsv`, `acceptance_matrix.tsv`, `failure_taxonomy.tsv`, `docs_freshness.log` |
| B1 | 3 | lane bootstrap | `validation_matrix.tsv`, `contract_runs/*`, `lane_notes.md`, `docs_freshness.log` |
| C2 | 10 | determinism soak | `validation_matrix.tsv`, `contract_runs/*`, `soak_summary.tsv`, `lane_notes.md`, `docs_freshness.log` |
| C3 | 20 | replay sentinel + mode parity | `validation_matrix.tsv`, `contract_runs_contract/*`, `contract_runs_execute/*`, `mode_parity.tsv`, `exit_semantics_probe.tsv`, `lane_notes.md`, `docs_freshness.log` |
| C4 | 50 | long-run sentinel + mode parity | same shape as C3, scaled to 50 runs |
| D1 | 75 | done-candidate long-run parity | same shape as C3, scaled to 75 runs |
| D2 | 100 | done-promotion parity | same shape as C3, scaled to 100 runs |

## Required Evidence Shape

Required output paths:
- `TestEvidence/bl041_slice_a1_contract_<timestamp>/`
- `TestEvidence/bl041_slice_b1_lane_<timestamp>/`
- `TestEvidence/bl041_slice_c2_soak_<timestamp>/`
- `TestEvidence/bl041_slice_c3_replay_mode_parity_<timestamp>/`
- `TestEvidence/bl041_slice_c4_longrun_mode_parity_<timestamp>/`
- `TestEvidence/bl041_slice_d1_done_candidate_<timestamp>/`
- `TestEvidence/bl041_slice_d2_done_promotion_<timestamp>/`

Common files across tiers:
- `status.tsv`
- `validation_matrix.tsv`
- `docs_freshness.log`

Tier-specific files:
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `soak_summary.tsv`
- `mode_parity.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `promotion_readiness.md`

Required TSV columns:
- `acceptance_matrix.tsv`: `acceptance_id`, `gate`, `threshold`, `measured_value`, `result`, `evidence_path`
- `failure_taxonomy.tsv`: `failure_id`, `category`, `trigger`, `classification`, `blocking`, `severity`, `expected_artifact`

## Validation Commands

Current contract checks:
```bash
./scripts/validate-docs-freshness.sh
```

Tier runner pattern:
```bash
bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh
./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help
./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs <N> --out-dir TestEvidence/bl041_slice_<tier>_<timestamp>/contract_runs
./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs <N> --out-dir TestEvidence/bl041_slice_<tier>_<timestamp>/contract_runs_execute
./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0
./scripts/validate-docs-freshness.sh
```

## Milestone Snapshot

| Tier | Date | Result | Why it matters | Evidence |
|---|---|---|---|---|
| A1 | 2026-02-27 | PASS | Contract, taxonomy, and evidence schema were defined. | `TestEvidence/bl041_slice_a1_contract_20260227T010932Z/` |
| B1 | 2026-02-27 | PASS | Bootstrap lane was parseable and aligned. | `TestEvidence/bl041_slice_b1_lane_20260227T204705Z/` |
| C2 | 2026-02-27 | PASS | 10-run soak stayed deterministic. | `TestEvidence/bl041_slice_c2_soak_20260227T014141Z/` |
| C3 | 2026-02-27 | PASS | 20-run replay sentinel and mode parity held. | `TestEvidence/bl041_slice_c3c_replay_mode_parity_20260227T031142Z/` |
| C4 | 2026-02-27 | PASS | 50-run long-run parity held. | `TestEvidence/bl041_slice_c4_longrun_mode_parity_20260227T033844Z/` |
| D1 | 2026-02-27 | PASS | 75-run done-candidate parity held. | `TestEvidence/bl041_slice_d1_done_candidate_20260227T183530Z/` |
| D2 | 2026-02-27 | PASS | 100-run done-promotion parity held. | `TestEvidence/bl041_slice_d2_done_promotion_20260227T201910Z/` |

## Latest Evidence

| Item | Value |
|---|---|
| Highest-signal packet | `TestEvidence/bl041_slice_d2_done_promotion_20260227T201910Z/` |
| Latest validation result | `PASS` |
| Latest docs freshness | `PASS` |
| Latest parity summary | `contract_runs_observed=100`, `execute_runs_observed=100`, `signature_drift_count=0/0`, `row_drift_count=0/0`, `cross_mode_signature_mismatch_count=0`, `cross_mode_row_mismatch_count=0` |

## Validation Status Labels

- `tested` = command executed and expected exit observed.
- `partially tested` = command executed with warnings or incomplete artifacts.
- `not tested` = command not executed.

## Notes

This file intentionally omits the long stage-by-stage replay diary.
Use the done runbook and the evidence directories above for deep chronology.
