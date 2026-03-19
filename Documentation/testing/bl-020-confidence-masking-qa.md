Title: BL-020 Confidence Masking QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-19

# BL-020 Confidence/Masking QA Contract

## Purpose

Define the deterministic QA contract for BL-020 confidence/masking overlays.
The contract keeps the overlay replayable, fail-safe, and easy to audit.

## Linked Contracts

- Runbook: `Documentation/backlog/bl-020-confidence-masking.md`
- Invariants: `Documentation/invariants.md`
- Lane harness: `scripts/qa-bl020-confidence-masking-lane-mac.sh`
- Scenario contract: `qa/scenarios/locusq_bl020_confidence_masking_suite.json`

## Core Contract

### Overlay Rules

| Rule | Contract |
|---|---|
| Required fields | Required keys must be present with the declared contract types. |
| Numeric bounds | Normalized confidence/masking values must stay finite in `[0,1]`. |
| Formula | `combined_confidence = 0.40*distance_confidence + 0.30*(1.0-occlusion_probability) + 0.20*hrtf_match_quality + 0.10*(1.0-masking_index)` |
| Formula tolerance | Absolute delta `<= 0.01`. |
| Bucket mapping | `low < 0.40`, `mid < 0.80`, `high >= 0.80`. |
| Fallbacks | Invalid rows must emit a deterministic reason token. |
| Sequence | `snapshotSeq` must be non-decreasing. |
| Degradation | Base emitter rendering stays available; overlay layer degrades independently. |

### Acceptance IDs

| acceptance_id | gate | threshold |
|---|---|---|
| BL020-A1-001 | Required field/type validity | 100% active rows valid |
| BL020-A1-002 | Numeric range + finiteness | 0 pre-clamp violations |
| BL020-A1-003 | Combined formula conformance | max abs delta `<= 0.01` |
| BL020-A1-004 | Bucket mapping determinism | 100% row match |
| BL020-A1-005 | Fallback token determinism | 100% fallback rows tokenized |
| BL020-A1-006 | Snapshot sequence monotonicity | 0 regressions |
| BL020-A1-007 | QA artifact schema completeness | all required artifacts + columns present |

### Failure Taxonomy

| failure_id | category | trigger | classification | blocking | severity |
|---|---|---|---|---|---|
| BL020-FX-001 | schema_missing_required_field | missing key/type mismatch | deterministic_contract_failure | yes | major |
| BL020-FX-002 | value_out_of_range_or_non_finite | value out of range or NaN/Inf | deterministic_contract_failure | yes | major |
| BL020-FX-003 | combined_confidence_formula_mismatch | formula delta exceeds tolerance | deterministic_contract_failure | yes | major |
| BL020-FX-004 | overlay_bucket_mismatch | threshold bucket mismatch | deterministic_contract_failure | yes | major |
| BL020-FX-005 | fallback_reason_missing_or_invalid | fallback token absent/invalid | deterministic_contract_failure | yes | major |
| BL020-FX-006 | snapshot_sequence_non_monotonic | sequence regression | deterministic_contract_failure | yes | critical |
| BL020-FX-007 | artifact_schema_incomplete | missing required artifact/columns | deterministic_evidence_failure | yes | major |

## Evidence Contract

Required evidence root:
- `TestEvidence/bl020_<slice>_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

Required TSV columns:
- `status.tsv`: lane id, result, evidence path, blocker summary
- `validation_matrix.tsv`: run id, command, exit code, result, log path
- `replay_hashes.tsv`: run id, signature hash, row hash, drift flags
- `failure_taxonomy.tsv`: failure id, category, trigger, classification, blocking, severity, expected artifact

## Replay Tiers

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | T1 | 3 | `validation_matrix.tsv` + replay summary |
| Candidate | T2 | 5 | stable replay summary + taxonomy |
| Promotion | T3 | 10 | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20 | explicit parity or soak evidence |

Cost rule:
- Diagnose the failing run index first.
- Do not repeat full multi-run sweeps blindly.
- Heavy wrappers should use targeted reruns before broader sweeps.

## Validation Commands

Current lane contract:

```bash
bash -n scripts/qa-bl020-confidence-masking-lane-mac.sh
./scripts/qa-bl020-confidence-masking-lane-mac.sh --help
./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs <N> --out-dir TestEvidence/bl020_<slice>_<timestamp>/contract_runs
./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs <N> --out-dir TestEvidence/bl020_<slice>_<timestamp>/execute_runs
./scripts/qa-bl020-confidence-masking-lane-mac.sh --runs 0
./scripts/qa-bl020-confidence-masking-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

Use `N = 3`, `5`, or `20` based on the active replay tier.

## C4 Validation Contract

C4 Evidence Contract requires 20-run sentinel evidence across both modes:

```bash
./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/contract_runs
./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/execute_runs
```

C4 gate criteria:
- `replay_hash_drift_count=0` across all 20 runs
- zero failing validation rows in either mode
- `mode_parity.tsv` confirms cross-mode doc/scenario hash stability
- `exit_semantics_probe.tsv` confirms `--runs 0` exits 2 and `--unknown-flag` exits 2

## Current Signal

| Item | Status |
|---|---|
| Runbook authority | `Documentation/backlog/bl-020-confidence-masking.md` |
| Current technical state | `In Validation` |
| Promotion state | Owner promotion review pending |
| Latest high-signal evidence | `TestEvidence/bl020_slice_c4b_mode_parity_20260228T202240Z/` |
| Latest parity evidence | `TestEvidence/bl020_slice_c4_mode_parity_20260228T175923Z/` |
| Latest native bridge evidence | `TestEvidence/bl020_slice_c1_native_20260226T174052Z/` |

## Milestone Snapshot

| Milestone | Packet | Result | Why it matters |
|---|---|---|---|
| A1 contract | `TestEvidence/bl020_slice_a1_contract_20260226T170007Z/` | PASS | Contract surface defined the overlay rules. |
| B1 lane | `TestEvidence/bl020_slice_b1_lane_20260226T172017Z/` | PASS | Replay contract and evidence schema held. |
| C1 native bridge | `TestEvidence/bl020_slice_c1_native_20260226T174052Z/` | PASS then RT-blocked in owner review | Additive native bridge was accepted, then blocked by RT findings at the time. |
| C3 re-verify | `TestEvidence/bl020_slice_c3_reverify_20260226T194955Z/` | PASS | C1 became green end-to-end after RT reconciliation. |
| C4 parity | `TestEvidence/bl020_slice_c4_mode_parity_20260228T175923Z/` | PASS | 20-run parity and exit semantics held. |
| C4b non-interference | `TestEvidence/bl020_slice_c4b_mode_parity_20260228T202240Z/` | PASS | The parity contract stayed stable under the follow-up check. |

## Notes

- Keep this file short.
- Detailed packet chronology stays in `TestEvidence/`.
- Add new history only when it changes the active contract.
