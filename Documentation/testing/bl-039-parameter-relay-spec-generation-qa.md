Title: BL-039 Parameter Relay Spec Generation QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-039 Parameter Relay Spec Generation QA Contract

## Purpose

Define deterministic QA checks and machine-readable evidence schema for BL-039 Slice A1 contract authority and Slice B1 drift-lane bootstrap replay checks.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/bl-039-parameter-relay-spec-generation.md`

Traceability anchors:
- `.ideas/parameter-spec.md`
- `.ideas/architecture.md`
- `Documentation/invariants.md`

## Deterministic QA Checks

### Schema Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL039-QA-001 | Canonical field set completeness | all required schema fields defined |
| BL039-QA-002 | Canonical type/rule validity | all field types/rules explicitly defined |
| BL039-QA-003 | Key uniqueness rule | unique (`apvts_param_id`, `relay_param_id`, `ui_binding_id`) |

### Ordering Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL039-QA-004 | Mode scope ordering | `global, calibrate, emitter, renderer` rank order enforced |
| BL039-QA-005 | Lexicographic tie-breaks | ASCII sort by `apvts_param_id`, then `relay_param_id`, then `ui_binding_id` |
| BL039-QA-006 | Ordinal determinism | ordinal contiguous from `0` and equal to sorted row index |

### Drift Detection Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL039-QA-007 | Content hash determinism | stable `spec_content_sha256` for unchanged inputs |
| BL039-QA-008 | Schema hash determinism | stable `schema_definition_sha256` for unchanged schema contract |
| BL039-QA-009 | Ordering fingerprint determinism | stable `ordering_fingerprint_sha256` for unchanged inputs |

### Replay Artifact Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL039-QA-010 | Replay artifact presence | all required replay artifacts exist |
| BL039-QA-011 | Replay artifact schema | required columns present in `relay_hashes.tsv` and `relay_drift_report.tsv` |
| BL039-QA-012 | Acceptance parity | BL039 A1 acceptance IDs match across backlog/qa/evidence |

## Acceptance Matrix Contract (Slice A1)

Required acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| BL039-A1-001 | Canonical schema completeness | 100% required fields/types/rules defined |
| BL039-A1-002 | Deterministic ordering contract | sort precedence + ordinal rule explicitly defined |
| BL039-A1-003 | Key uniqueness + ordinal contiguity | 0 duplicate keys; 0 ordinal gaps/regressions |
| BL039-A1-004 | Drift detection contract | hash set + normalization + drift rules explicitly defined |
| BL039-A1-005 | Replay artifact schema contract | required artifacts and required columns explicitly defined |
| BL039-A1-006 | Failure taxonomy coverage | deterministic and evidence failure classes fully mapped |
| BL039-A1-007 | Backlog/QA acceptance parity | all A1 acceptance IDs present in both docs |
| BL039-A1-008 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exit code `0` |

Required `acceptance_matrix.tsv` columns:
- `acceptance_id`
- `gate`
- `threshold`
- `measured_value`
- `result`
- `evidence_path`

## Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL039-FX-001 | schema_missing_required_field | required schema field absent | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL039-FX-002 | schema_type_or_rule_mismatch | field type/rule mismatch vs canonical contract | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL039-FX-003 | duplicate_relay_key | duplicate (`apvts_param_id`, `relay_param_id`, `ui_binding_id`) key | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL039-FX-004 | non_deterministic_ordering | sorted row order differs for same input | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL039-FX-005 | ordinal_gap_or_regression | ordinal not contiguous from `0` | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL039-FX-006 | drift_hash_mismatch | deterministic hash mismatch for unchanged inputs | deterministic_drift_failure | yes | critical | acceptance_matrix.tsv |
| BL039-FX-007 | replay_artifact_schema_incomplete | required replay file/columns missing | deterministic_evidence_failure | yes | major | status.tsv |
| BL039-FX-008 | acceptance_id_parity_failure | acceptance IDs out of sync across runbook/qa/evidence | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL039-FX-009 | docs_freshness_gate_failure | docs freshness script non-zero exit | governance_failure | yes | major | docs_freshness.log |

## Replay Artifact Schema (Execution Slices B/C)

Required replay bundle path:
- `TestEvidence/bl039_slice_b_or_c_<timestamp>/`

Required files:
- `status.tsv`
- `parameter_relay_spec.tsv`
- `relay_generation_manifest.json`
- `relay_hashes.tsv`
- `relay_drift_report.tsv`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`

Required `relay_hashes.tsv` columns:
- `hash_name`, `hash_value`, `input_signature`, `result`

Required `relay_drift_report.tsv` columns:
- `drift_check_id`, `baseline_value`, `candidate_value`, `result`, `classification`

## Slice A1 Evidence Bundle Requirements

Required path:
- `TestEvidence/bl039_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `parameter_relay_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Validation Command (Slice A1)

- `./scripts/validate-docs-freshness.sh`

Validation status labels:
- `tested` = command executed, expected exit code observed.
- `partially tested` = command executed with partial artifacts or warnings.
- `not tested` = command not executed.

## Slice B1 Drift Lane Contract

Lane script:
- `scripts/qa-bl039-parameter-relay-drift-mac.sh`

Supported options:
- `--contract-only` (default)
- `--execute-suite` (alias mode for lane parity, no build/runtime execution in B1)
- `--runs <N>`
- `--out-dir <path>`
- `--help|-h`

Strict exit semantics:
- `0` = pass
- `1` = lane/contract failure
- `2` = usage/configuration error

## Slice B1 Acceptance Matrix Contract

Required acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| BL039-B1-001 | schema contract | canonical schema clauses present |
| BL039-B1-002 | ordering contract | sort precedence + ordinal clauses present |
| BL039-B1-003 | drift hash contract | deterministic hash clauses present |
| BL039-B1-004 | replay hash stability | signature divergence `= 0` across runs |
| BL039-B1-005 | replay row stability | row drift `= 0` across runs |
| BL039-B1-006 | artifact schema completeness | required artifacts and required columns present |
| BL039-B1-007 | execution mode contract | mode declared and machine-auditable |

Required `validation_matrix.tsv` columns:
- `run_index`
- `gate_id`
- `gate`
- `threshold`
- `measured_value`
- `result`
- `artifact`

Required `replay_hashes.tsv` columns:
- `run_index`
- `hash_name`
- `hash_value`
- `input_signature`
- `result`
- `artifact`

## Slice B1 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL039-B1-FX-001 | schema_contract_missing | required schema clauses absent | deterministic_contract_failure | yes | major | validation_matrix.tsv |
| BL039-B1-FX-002 | ordering_contract_missing | ordering/ordinal clauses absent | deterministic_contract_failure | yes | major | validation_matrix.tsv |
| BL039-B1-FX-003 | drift_hash_contract_missing | deterministic hash clauses absent | deterministic_contract_failure | yes | major | validation_matrix.tsv |
| BL039-B1-FX-004 | acceptance_parity_mismatch | acceptance IDs differ across backlog/qa contracts | deterministic_contract_failure | yes | major | validation_matrix.tsv |
| BL039-B1-FX-005 | taxonomy_parity_mismatch | failure taxonomy IDs differ across backlog/qa contracts | deterministic_contract_failure | yes | major | validation_matrix.tsv |
| BL039-B1-FX-006 | replay_hash_divergence | combined replay hash mismatch across runs | deterministic_replay_divergence | yes | critical | replay_hashes.tsv |
| BL039-B1-FX-007 | replay_row_drift | row-signature mismatch across runs | deterministic_replay_row_drift | yes | critical | replay_hashes.tsv |
| BL039-B1-FX-008 | missing_required_artifact | required artifact missing | missing_result_artifact | yes | major | status.tsv |
| BL039-B1-FX-009 | runtime_tool_missing | required command unavailable | runtime_execution_failure | yes | major | status.tsv |
| BL039-B1-FX-010 | usage_error | invalid argument/configuration usage | usage_error | yes | major | status.tsv |

## Slice B1 Validation Commands

- `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl039_slice_b1_lane_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## Slice B1 Evidence Bundle Requirements

Required path:
- `TestEvidence/bl039_slice_b1_lane_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice B1 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl039_slice_b1_lane_20260227T005455Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl039_slice_b1_lane_20260227T005455Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## Slice C2 Soak Replay Contract

Purpose:
- Prove deterministic replay/hash/order behavior across 10 contract-only runs with stable exit semantics.

Validation matrix:
- `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl039_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C2 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL039-C2-001 | lane syntax validity | shell syntax check exit `0` |
| BL039-C2-002 | lane CLI contract validity | `--help` exit `0` and usage emitted |
| BL039-C2-003 | replay soak determinism | `signature_divergence_count=0` and `row_drift_count=0` over `runs=10` |
| BL039-C2-004 | failure taxonomy stability | deterministic failure classes all `0` |
| BL039-C2-005 | soak artifact schema completeness | all required C2 artifacts present |
| BL039-C2-006 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exit `0` |

### C2 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL039-C2-FX-001 | soak_signature_divergence | replay signature mismatch across soak runs | deterministic_replay_divergence | yes | critical | contract_runs/replay_hashes.tsv |
| BL039-C2-FX-002 | soak_row_drift | replay row-signature mismatch across soak runs | deterministic_replay_row_drift | yes | critical | contract_runs/replay_hashes.tsv |
| BL039-C2-FX-003 | soak_artifact_missing | required C2 artifact missing | missing_result_artifact | yes | major | status.tsv |
| BL039-C2-FX-004 | soak_usage_or_runtime_error | usage/runtime error during soak | runtime_execution_failure | yes | major | status.tsv |

### C2 Evidence Contract

Required path:
- `TestEvidence/bl039_slice_c2_soak_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `drift_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C2 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl039_slice_c2_soak_20260227T010751Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `drift_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl039_slice_c2_soak_20260227T010751Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## Slice C3 Replay Sentinel Contract

Purpose:
- Prove sustained deterministic replay/hash/order behavior across 20 contract-only runs.

Validation matrix:
- `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl039_slice_c3_replay_sentinel_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C3 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL039-C3-001 | replay sentinel run-count contract | `runs=20` and sentinel status emitted |
| BL039-C3-002 | sentinel drift summary contract | `signature_divergence_count=0`, `row_drift_count=0`, `run_failure_count=0` |
| BL039-C3-003 | replay hash sentinel stability | all 20 runs preserve baseline combined signature |
| BL039-C3-004 | failure taxonomy sentinel stability | deterministic/runtime/missing counts all `0` |
| BL039-C3-005 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exit `0` |

### C3 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL039-C3-FX-001 | replay_sentinel_signature_divergence | combined replay signature mismatch across 20 runs | deterministic_replay_divergence | yes | critical | contract_runs/replay_hashes.tsv |
| BL039-C3-FX-002 | replay_sentinel_row_drift | row-signature mismatch across 20 runs | deterministic_replay_row_drift | yes | critical | contract_runs/replay_hashes.tsv |
| BL039-C3-FX-003 | replay_sentinel_artifact_missing | required C3 artifact missing | missing_result_artifact | yes | major | status.tsv |
| BL039-C3-FX-004 | replay_sentinel_runtime_or_usage_error | runtime/usage failure during sentinel replay | runtime_execution_failure | yes | major | status.tsv |

### C3 Evidence Contract

Required path:
- `TestEvidence/bl039_slice_c3_replay_sentinel_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `drift_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C3 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl039_slice_b1_lane_20260227T005455Z/*`
  - `TestEvidence/bl039_slice_c2_soak_20260227T010751Z/*`
- Evidence bundle:
  - `TestEvidence/bl039_slice_c3_replay_sentinel_20260227T012211Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `drift_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl039_slice_c3_replay_sentinel_20260227T012211Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## Slice C4 Replay Sentinel Soak Contract

Purpose:
- Prove sustained deterministic replay/hash/order behavior across 50 contract-only runs and publish a stable drift rollup.

Validation matrix:
- `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl039_slice_c4_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C4 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL039-C4-001 | replay soak run-count contract | `runs=50` and C4 soak status emitted |
| BL039-C4-002 | soak drift summary contract | `signature_divergence_count=0`, `row_drift_count=0`, `run_failure_count=0` |
| BL039-C4-003 | replay hash soak stability | all 50 runs preserve baseline combined signature |
| BL039-C4-004 | failure taxonomy soak stability | deterministic/runtime/missing counts all `0` |
| BL039-C4-005 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exit `0` |

### C4 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL039-C4-FX-001 | replay_soak_signature_divergence | combined replay signature mismatch across 50 runs | deterministic_replay_divergence | yes | critical | contract_runs/replay_hashes.tsv |
| BL039-C4-FX-002 | replay_soak_row_drift | row-signature mismatch across 50 runs | deterministic_replay_row_drift | yes | critical | contract_runs/replay_hashes.tsv |
| BL039-C4-FX-003 | replay_soak_artifact_missing | required C4 artifact missing | missing_result_artifact | yes | major | status.tsv |
| BL039-C4-FX-004 | replay_soak_runtime_or_usage_error | runtime/usage failure during soak replay | runtime_execution_failure | yes | major | status.tsv |

### C4 Evidence Contract

Required path:
- `TestEvidence/bl039_slice_c4_soak_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `drift_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C4 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl039_slice_c3_replay_sentinel_20260227T012211Z/*`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z4_20260227T013040Z/*`
- Evidence bundle:
  - `TestEvidence/bl039_slice_c4_soak_20260227T013914Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `drift_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl039_slice_c4_soak_20260227T013914Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## Slice C5 Execute-Mode Parity Guard Contract

Purpose:
- Prove `--contract-only` and `--execute-suite` alias-mode parity and strict usage exit semantics.

Validation matrix:
- `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl039_slice_c5_semantics_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl039_slice_c5_semantics_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C5 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL039-C5-001 | execute-mode alias parity contract | contract vs execute mode parity checks all `PASS` |
| BL039-C5-002 | contract-only replay determinism | `signature_divergence_count=0` and `row_drift_count=0` for `runs=20` |
| BL039-C5-003 | execute-suite replay determinism | `signature_divergence_count=0` and `row_drift_count=0` for `runs=20` |
| BL039-C5-004 | mode parity artifact contract | `mode_parity.tsv` machine-readable with `PASS` parity rows |
| BL039-C5-005 | strict usage exit semantics | negative probe `--runs 0` exits with code `2` |
| BL039-C5-006 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exit `0` |

### C5 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL039-C5-FX-001 | mode_alias_parity_mismatch | contract vs execute mode parity check mismatch | deterministic_contract_failure | yes | critical | mode_parity.tsv |
| BL039-C5-FX-002 | contract_mode_signature_divergence | combined or row signature divergence in contract mode runset | deterministic_replay_divergence | yes | critical | contract_runs_contract/replay_hashes.tsv |
| BL039-C5-FX-003 | execute_mode_signature_divergence | combined or row signature divergence in execute mode runset | deterministic_replay_divergence | yes | critical | contract_runs_execute/replay_hashes.tsv |
| BL039-C5-FX-004 | strict_exit_semantics_violation | negative usage probe does not return exit `2` | usage_error | yes | major | exit_semantics_probe.tsv |
| BL039-C5-FX-005 | c5_artifact_missing | required C5 artifact missing | missing_result_artifact | yes | major | status.tsv |

### C5 Evidence Contract

Required path:
- `TestEvidence/bl039_slice_c5_semantics_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `drift_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C5 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl039_slice_c4_soak_20260227T013914Z/*`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z5_20260227T014558Z/*`
- Evidence bundle:
  - `TestEvidence/bl039_slice_c5_semantics_20260227T015405Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `drift_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl039_slice_c5_semantics_20260227T015405Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl039_slice_c5_semantics_20260227T015405Z/contract_runs_execute` => PASS
  - negative probe `./scripts/qa-bl039-parameter-relay-drift-mac.sh --runs 0` => exit `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => FAIL (external metadata failure outside C5 ownership scope)

### C5b Recheck Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl039_slice_c4_soak_20260227T013914Z/*`
  - `TestEvidence/bl039_slice_c5_semantics_20260227T015405Z/*`
  - `TestEvidence/docs_hygiene_hrtf_h1_20260227T020511Z/*`
- Evidence bundle:
  - `TestEvidence/bl039_slice_c5b_semantics_20260227T025259Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `drift_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl039_slice_c5b_semantics_20260227T025259Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl039_slice_c5b_semantics_20260227T025259Z/contract_runs_execute` => PASS
  - negative probe `./scripts/qa-bl039-parameter-relay-drift-mac.sh --runs 0` => exit `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => FAIL (global metadata failures under `Documentation/Calibration POC/*`, outside C5b ownership)

## Slice C5c Execute-Mode Parity Recheck Contract (Post-H2)

Purpose:
- Re-run C5 execute-mode parity semantics after H2 docs hygiene intake and require a fully green packet.

Acceptance mapping:
- Inherits C5 acceptance IDs `BL039-C5-001` through `BL039-C5-006`.
- PASS requires all inherited C5 gates plus C5c evidence bundle completeness.

Validation matrix:
- `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl039_slice_c5c_semantics_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl039_slice_c5c_semantics_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

Required evidence bundle:
- `TestEvidence/bl039_slice_c5c_semantics_<timestamp>/status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `drift_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C6 Long-Run Execute-Mode Parity Sentinel Contract

Purpose:
- Raise execute-mode parity confidence with 50-run deterministic replay for contract-only and execute-suite aliases.

Validation matrix:
- `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl039_slice_c6_longrun_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl039_slice_c6_longrun_parity_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl039-parameter-relay-drift-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C6 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL039-C6-001 | long-run execute-mode parity contract | parity checks all `PASS` for contract-only and execute-suite at `runs=50` |
| BL039-C6-002 | contract-only long-run determinism | `signature_divergence_count=0`, `row_drift_count=0`, `run_failure_count=0` |
| BL039-C6-003 | execute-suite long-run determinism | `signature_divergence_count=0`, `row_drift_count=0`, `run_failure_count=0` |
| BL039-C6-004 | long-run mode parity artifact contract | `mode_parity.tsv` present with `BL039-C6-PAR-RESULT=PASS` |
| BL039-C6-005 | strict usage exit semantics | negative probe `--runs 0` exits with code `2` |
| BL039-C6-006 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exit `0` |

### C6 Artifact Contract

Required bundle:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `drift_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C5c Recheck Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl039_slice_c5b_semantics_20260227T025259Z/*`
  - `TestEvidence/docs_hygiene_calibration_poc_h2_20260227T030945Z/*`
- Evidence bundle:
  - `TestEvidence/bl039_slice_c5c_semantics_20260227T031036Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `drift_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl039_slice_c5c_semantics_20260227T031036Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl039_slice_c5c_semantics_20260227T031036Z/contract_runs_execute` => PASS
  - negative probe `./scripts/qa-bl039-parameter-relay-drift-mac.sh --runs 0` => exit `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => PASS

### C6 Long-Run Sentinel Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl039_slice_c5c_semantics_20260227T031036Z/*`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z7_20260227T032802Z/*`
- Evidence bundle:
  - `TestEvidence/bl039_slice_c6_longrun_parity_20260227T033754Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `drift_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl039-parameter-relay-drift-mac.sh` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --help` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl039_slice_c6_longrun_parity_20260227T033754Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl039-parameter-relay-drift-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl039_slice_c6_longrun_parity_20260227T033754Z/contract_runs_execute` => PASS
  - negative probe `./scripts/qa-bl039-parameter-relay-drift-mac.sh --runs 0` => exit `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => PASS
