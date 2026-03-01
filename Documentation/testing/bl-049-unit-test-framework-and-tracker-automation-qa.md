Title: BL-049 Unit Test Framework and Tracker Automation QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-049 Unit Test Framework and Tracker Automation QA Contract

## Purpose

Define deterministic QA checks and machine-readable evidence schema for BL-049 Slice A1 unit-test framework, tracker automation schema, flake classification policy, and CI artifact determinism.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/done/bl-049-unit-test-framework-and-tracker-automation.md`

Traceability anchors:
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`

## Deterministic QA Checks (A1)

### Framework Architecture Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL049-QA-001 | Framework layer contract | all required framework layers are explicitly declared |
| BL049-QA-002 | Lifecycle determinism contract | setup/run/teardown state contract exists with deterministic order |
| BL049-QA-003 | Aggregation determinism contract | result aggregation inputs and order rules are explicit |

### Tracker Schema Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL049-QA-004 | Tracker required fields | required field set is complete and typed |
| BL049-QA-005 | Enum domain determinism | tracker enum values are constrained and explicit |
| BL049-QA-006 | Sequence monotonicity contract | `payload_seq` monotonic rule and rollback failure behavior are explicit |

### Flake and CI Artifact Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL049-QA-007 | Flake classification completeness | required flake classes and mapping policy are explicit |
| BL049-QA-008 | Flake threshold contract | recovered flake threshold and unrecovered blocking rule are explicit |
| BL049-QA-009 | CI artifact determinism contract | required deterministic artifacts and row/column determinism rules are explicit |

## Acceptance Matrix Contract (Slice A1)

Required acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-A1-001 | Framework layer contract completeness | all five required framework layers declared |
| BL049-A1-002 | Deterministic lifecycle semantics | lifecycle states/order contract defined |
| BL049-A1-003 | Tracker schema completeness | required tracker fields and enums defined |
| BL049-A1-004 | Tracker sequence determinism | `payload_seq` monotonic and rollback blocking rule defined |
| BL049-A1-005 | Flake classification taxonomy completeness | all five flake classes and mapping policy defined |
| BL049-A1-006 | Flake thresholds explicit | recovered flake ratio threshold and unrecovered blocking rule defined |
| BL049-A1-007 | Deterministic CI artifact contract completeness | required artifact list and deterministic row/column/hash rules defined |
| BL049-A1-008 | Evidence schema completeness | all required A1 evidence files declared |
| BL049-A1-009 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

Required `acceptance_matrix.tsv` columns:
- `acceptance_id`
- `gate`
- `threshold`
- `measured_value`
- `result`
- `evidence_path`

## Failure Taxonomy Contract (Slice A1)

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL049-FX-001 | framework_layer_contract_missing | required framework layer missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL049-FX-002 | lifecycle_semantics_incomplete | lifecycle order/state contract missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL049-FX-003 | tracker_schema_required_field_missing | required tracker field absent | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL049-FX-004 | tracker_sequence_non_monotonic | `payload_seq` rollback detected | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL049-FX-005 | tracker_enum_domain_invalid | enum value outside declared domain | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL049-FX-006 | flake_taxonomy_incomplete | required flake classification missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL049-FX-007 | flake_threshold_policy_missing | flake threshold policy undefined | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL049-FX-008 | ci_artifact_contract_incomplete | required CI artifact contract incomplete | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL049-FX-009 | evidence_schema_incomplete | required A1 evidence artifact missing | deterministic_evidence_failure | yes | major | status.tsv |
| BL049-FX-010 | docs_freshness_gate_failure | docs freshness exits non-zero | governance_failure | yes | major | docs_freshness.log |

Required `failure_taxonomy.tsv` columns:
- `failure_id`
- `category`
- `trigger`
- `classification`
- `blocking`
- `severity`
- `expected_artifact`

## Validation (A1)

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (A1)

Required files under `TestEvidence/bl049_slice_a1_contract_<timestamp>/`:
- `status.tsv`
- `unit_test_tracker_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## B1 Acceptance Matrix (Lane Bootstrap)

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-B1-001 | Runbook B1 contract completeness | runbook includes `Validation Plan (B1)` + `Evidence Contract (B1)` + script reference |
| BL049-B1-002 | QA B1 contract completeness | QA includes `B1 Validation` + `B1 Evidence Contract` + script reference |
| BL049-B1-003 | Runbook artifact schema alignment | required B1 artifact list includes replay hashes/taxonomy outputs |
| BL049-B1-004 | QA artifact schema alignment | required B1 artifact list matches runbook |
| BL049-B1-005 | Runbook B1 taxonomy completeness | B1 taxonomy IDs `BL049-FX-101..108` declared |
| BL049-B1-006 | QA B1 taxonomy completeness | B1 taxonomy IDs `BL049-FX-101..108` declared |
| BL049-B1-007 | A1 handoff continuity | A1 input handoff is explicitly resolved in B1 snapshot |
| BL049-B1-008 | Lane mode and exit semantics | script supports `--contract-only`/`--execute-suite` and exits `0/1/2` |

## B1 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| BL049-FX-101 | lane_contract_missing | B1 contract sections absent in runbook/QA | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| BL049-FX-102 | lane_artifact_schema_mismatch | runbook and QA artifact schema mismatch | deterministic_contract_failure | yes | validation_matrix.tsv |
| BL049-FX-103 | lane_replay_signature_drift | replay signature divergence for equal contract input | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| BL049-FX-104 | lane_replay_row_drift | replay row signature divergence for equal contract input | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| BL049-FX-105 | lane_mode_exit_contract_invalid | script mode/usage exit semantics missing or invalid | deterministic_contract_failure | yes | status.tsv |
| BL049-FX-106 | lane_a1_handoff_unresolved | A1 input handoff not linked in B1 packet | deterministic_contract_failure | yes | lane_notes.md |
| BL049-FX-107 | lane_evidence_schema_incomplete | required B1 artifact missing | deterministic_evidence_failure | yes | status.tsv |
| BL049-FX-108 | docs_freshness_gate_failure | docs freshness exits non-zero | governance_failure | yes | docs_freshness.log |

## B1 Validation

- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl049_slice_b1_lane_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## B1 Evidence Contract

Required files under `TestEvidence/bl049_slice_b1_lane_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## B1 Execution Snapshot (2026-02-27)

- Input handoff resolved:
  - `TestEvidence/bl049_slice_a1_contract_20260227T204255Z/status.tsv`
- Validation commands:
  - `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl049_slice_b1_lane_20260227T212400Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Evidence path: `TestEvidence/bl049_slice_b1_lane_20260227T212400Z/`
- Determinism results:
  - `runs_observed=3`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`

## C2 Soak Determinism Contract

Purpose:
- Prove BL-049 lane replay determinism at soak depth (`runs=10`) with stable taxonomy and governance gate.

Validation matrix:
- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl049_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C2 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-C2-001 | lane syntax validity | shell syntax check exits `0` |
| BL049-C2-002 | lane CLI contract validity | `--help` exits `0` and emits usage |
| BL049-C2-003 | deterministic 10-run replay | contract-only replay exits `0` and emits run artifacts |
| BL049-C2-004 | replay count integrity | `replay_hashes.tsv` has exactly 10 data rows |
| BL049-C2-005 | soak drift/taxonomy stability | signature drift, row drift, contract fail rows, taxonomy nonzero rows all equal `0` |
| BL049-C2-006 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

### C2 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL049-FX-201 | soak_replay_count_mismatch | replay rows differ from requested `runs=10` | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-202 | soak_signature_or_row_drift | replay signature or row signature diverges during soak | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-203 | soak_contract_or_taxonomy_nonzero | contract validation fails or taxonomy emits nonzero deterministic failure rows | deterministic_contract_failure | yes | major | contract_runs/validation_matrix.tsv |
| BL049-FX-204 | soak_artifact_or_docs_gate_failure | required evidence artifact missing or docs freshness validation fails | deterministic_evidence_failure | yes | major | status.tsv |

### C2 Evidence Contract

Required path:
- `TestEvidence/bl049_slice_c2_soak_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C2 Execution Snapshot (2026-02-27)

- Input handoff resolved:
  - `TestEvidence/bl049_slice_b1_lane_20260227T212400Z/status.tsv`
- Validation commands:
  - `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl049_slice_c2_soak_20260227T220458Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Evidence path:
  - `TestEvidence/bl049_slice_c2_soak_20260227T220458Z/`
- Determinism readout:
  - `runs_observed=10`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `contract_fail_rows=0`
  - `taxonomy_nonzero_rows=0`

## C3 Replay Sentinel Contract

Purpose:
- Prove BL-049 deterministic replay stability at 20-run sentinel depth.

Validation matrix:
- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl049_slice_c3_replay_sentinel_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C3 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-C3-001 | lane syntax validity | shell syntax check exits `0` |
| BL049-C3-002 | lane CLI contract validity | `--help` exits `0` and emits usage |
| BL049-C3-003 | deterministic 20-run replay | contract-only replay exits `0` and emits run artifacts |
| BL049-C3-004 | replay count integrity | `replay_hashes.tsv` has exactly 20 data rows |
| BL049-C3-005 | drift/taxonomy stability | signature drift, row drift, contract fail rows, taxonomy nonzero rows all equal `0` |
| BL049-C3-006 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

### C3 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL049-FX-301 | c3_replay_count_mismatch | replay rows differ from requested `runs=20` | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-302 | c3_signature_or_row_drift | replay signature or row signature diverges during C3 sentinel | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-303 | c3_contract_or_taxonomy_nonzero | contract validation fails or taxonomy emits nonzero deterministic failure rows | deterministic_contract_failure | yes | major | contract_runs/validation_matrix.tsv |
| BL049-FX-304 | c3_artifact_or_docs_gate_failure | required evidence artifact missing or docs freshness validation fails | deterministic_evidence_failure | yes | major | status.tsv |

### C3 Evidence Contract

Required path:
- `TestEvidence/bl049_slice_c3_replay_sentinel_<timestamp>/`

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

- Validation commands:
  - `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl049_slice_c3_replay_sentinel_20260227T222544Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Evidence path:
  - `TestEvidence/bl049_slice_c3_replay_sentinel_20260227T222544Z/`
- Determinism readout:
  - `runs_observed=20`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `contract_fail_rows=0`
  - `taxonomy_nonzero_rows=0`

## C4 Soak Escalation Contract

Purpose:
- Prove BL-049 deterministic replay stability at 50-run soak depth.

Validation matrix:
- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl049_slice_c4_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C4 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-C4-001 | lane syntax validity | shell syntax check exits `0` |
| BL049-C4-002 | lane CLI contract validity | `--help` exits `0` and emits usage |
| BL049-C4-003 | deterministic 50-run replay | contract-only replay exits `0` and emits run artifacts |
| BL049-C4-004 | replay count integrity | `replay_hashes.tsv` has exactly 50 data rows |
| BL049-C4-005 | drift/taxonomy stability | signature drift, row drift, contract fail rows, taxonomy nonzero rows all equal `0` |
| BL049-C4-006 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

### C4 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL049-FX-401 | c4_replay_count_mismatch | replay rows differ from requested `runs=50` | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-402 | c4_signature_or_row_drift | replay signature or row signature diverges during C4 soak | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-403 | c4_contract_or_taxonomy_nonzero | contract validation fails or taxonomy emits nonzero deterministic failure rows | deterministic_contract_failure | yes | major | contract_runs/validation_matrix.tsv |
| BL049-FX-404 | c4_artifact_or_docs_gate_failure | required evidence artifact missing or docs freshness validation fails | deterministic_evidence_failure | yes | major | status.tsv |

### C4 Evidence Contract

Required path:
- `TestEvidence/bl049_slice_c4_soak_<timestamp>/`

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

- Validation commands:
  - `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl049_slice_c4_soak_20260227T223044Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Evidence path:
  - `TestEvidence/bl049_slice_c4_soak_20260227T223044Z/`
- Determinism readout:
  - `runs_observed=50`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `contract_fail_rows=0`
  - `taxonomy_nonzero_rows=0`

## C5 Execute-Mode Parity Contract

Purpose:
- Prove parity between `--contract-only` and `--execute-suite` and enforce strict usage exits.

Validation matrix:
- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl049_slice_c5_semantics_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl049_slice_c5_semantics_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --bad-flag` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C5 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-C5-001 | lane syntax validity | shell syntax check exits `0` |
| BL049-C5-002 | lane CLI contract validity | `--help` exits `0` and emits usage |
| BL049-C5-003 | contract-only replay stability | contract-only replay exits `0` with zero drift/fail counts |
| BL049-C5-004 | execute-suite replay stability | execute-suite replay exits `0` with zero drift/fail counts |
| BL049-C5-005 | mode parity | replay hashes are identical across both modes |
| BL049-C5-006 | usage probe runs0 | `--runs 0` exits `2` |
| BL049-C5-007 | usage probe bad-flag | `--bad-flag` exits `2` |
| BL049-C5-008 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

### C5 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL049-FX-501 | c5_mode_parity_drift | contract-only vs execute-suite replay hashes diverge | deterministic_mode_parity_failure | yes | critical | mode_parity.tsv |
| BL049-FX-502 | c5_contract_mode_drift | contract-only mode reports signature/row/fail drift | deterministic_replay_failure | yes | critical | contract_runs_contract/replay_hashes.tsv |
| BL049-FX-503 | c5_execute_mode_drift | execute-suite mode reports signature/row/fail drift | deterministic_replay_failure | yes | critical | contract_runs_execute/replay_hashes.tsv |
| BL049-FX-504 | c5_usage_exit_semantics_invalid | `--runs 0` or `--bad-flag` exits are not `2` | deterministic_contract_failure | yes | major | exit_semantics_probe.tsv |
| BL049-FX-505 | c5_artifact_or_docs_gate_failure | required evidence artifact missing or docs freshness validation fails | deterministic_evidence_failure | yes | major | status.tsv |

### C5 Evidence Contract

Required path:
- `TestEvidence/bl049_slice_c5_semantics_<timestamp>/`

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

- Validation commands:
  - `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl049_slice_c5_semantics_20260227T223839Z/contract_runs_contract` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl049_slice_c5_semantics_20260227T223839Z/contract_runs_execute` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --bad-flag` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Evidence path:
  - `TestEvidence/bl049_slice_c5_semantics_20260227T223839Z/`
- Semantics readout:
  - `contract_runs_observed=20`
  - `execute_runs_observed=20`
  - `overall_mode_parity=PASS`
  - `runs0_exit=2`
  - `badflag_exit=2`

## D1 Done-Candidate Sentinel Contract

Purpose:
- Establish done-candidate confidence with 75-run deterministic replay and strict usage exits.

Validation matrix:
- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl049_slice_d1_done_candidate_<timestamp>/contract_runs`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --bad-flag` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### D1 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-D1-001 | lane syntax validity | shell syntax check exits `0` |
| BL049-D1-002 | lane CLI contract validity | `--help` exits `0` |
| BL049-D1-003 | deterministic 75-run replay | contract-only replay exits `0` and emits run artifacts |
| BL049-D1-004 | replay count integrity | `replay_hashes.tsv` has exactly 75 data rows |
| BL049-D1-005 | replay/taxonomy stability | signature drift, row drift, contract fail rows, taxonomy nonzero rows all equal `0` |
| BL049-D1-006 | usage exit semantics | `--runs 0` and `--bad-flag` exits are `2` |
| BL049-D1-007 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

### D1 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL049-FX-601 | d1_replay_count_or_drift_failure | replay row count mismatch or replay drift in 75-run sentinel | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-602 | d1_contract_or_taxonomy_nonzero | contract validation fails or taxonomy emits nonzero deterministic failure rows | deterministic_contract_failure | yes | major | contract_runs/validation_matrix.tsv |
| BL049-FX-603 | d1_usage_exit_semantics_invalid | `--runs 0` or `--bad-flag` exits are not `2` | deterministic_contract_failure | yes | major | exit_semantics_probe.tsv |
| BL049-FX-604 | d1_done_candidate_readiness_fail | done-candidate readiness memo evaluates FAIL | deterministic_readiness_failure | yes | major | done_candidate_readiness.md |
| BL049-FX-605 | d1_artifact_or_docs_gate_failure | required artifact missing or docs freshness fails | deterministic_evidence_failure | yes | major | status.tsv |

### D1 Evidence Contract

Required path:
- `TestEvidence/bl049_slice_d1_done_candidate_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `drift_summary.tsv`
- `exit_semantics_probe.tsv`
- `done_candidate_readiness.md`
- `docs_freshness.log`

### D1 Execution Snapshot (2026-02-27)

- Validation commands:
  - `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl049_slice_d1_done_candidate_20260227T224327Z/contract_runs` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --bad-flag` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Evidence path:
  - `TestEvidence/bl049_slice_d1_done_candidate_20260227T224327Z/`
- Determinism readout:
  - `runs_observed=75`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `contract_fail_rows=0`
  - `taxonomy_nonzero_rows=0`
  - `runs0_exit=2`
  - `badflag_exit=2`

## D2 Done-Promotion Sentinel Contract

Purpose:
- Establish done-promotion confidence with 100-run deterministic replay and strict usage exits.

Validation matrix:
- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl049_slice_d2_done_promotion_<timestamp>/contract_runs`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --bad-flag` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### D2 Acceptance Matrix Contract

| acceptance_id | gate | threshold |
|---|---|---|
| BL049-D2-001 | lane syntax validity | shell syntax check exits `0` |
| BL049-D2-002 | lane CLI contract validity | `--help` exits `0` |
| BL049-D2-003 | deterministic 100-run replay | contract-only replay exits `0` and emits run artifacts |
| BL049-D2-004 | replay count integrity | `replay_hashes.tsv` has exactly 100 data rows |
| BL049-D2-005 | replay/taxonomy stability | signature drift, row drift, contract fail rows, taxonomy nonzero rows all equal `0` |
| BL049-D2-006 | usage exit semantics | `--runs 0` and `--bad-flag` exits are `2` |
| BL049-D2-007 | docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

### D2 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL049-FX-701 | d2_replay_count_or_drift_failure | replay row count mismatch or replay drift in 100-run sentinel | deterministic_replay_failure | yes | critical | contract_runs/replay_hashes.tsv |
| BL049-FX-702 | d2_contract_or_taxonomy_nonzero | contract validation fails or taxonomy emits nonzero deterministic failure rows | deterministic_contract_failure | yes | major | contract_runs/validation_matrix.tsv |
| BL049-FX-703 | d2_usage_exit_semantics_invalid | `--runs 0` or `--bad-flag` exits are not `2` | deterministic_contract_failure | yes | major | exit_semantics_probe.tsv |
| BL049-FX-704 | d2_promotion_readiness_fail | promotion readiness memo evaluates FAIL | deterministic_readiness_failure | yes | major | promotion_readiness.md |
| BL049-FX-705 | d2_artifact_or_docs_gate_failure | required artifact missing or docs freshness fails | deterministic_evidence_failure | yes | major | status.tsv |

### D2 Evidence Contract

Required path:
- `TestEvidence/bl049_slice_d2_done_promotion_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `drift_summary.tsv`
- `exit_semantics_probe.tsv`
- `promotion_readiness.md`
- `docs_freshness.log`

### D2 Execution Snapshot (2026-02-27)

- Validation commands:
  - `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl049_slice_d2_done_promotion_20260227T224446Z/contract_runs` => `PASS`
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --bad-flag` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Evidence path:
  - `TestEvidence/bl049_slice_d2_done_promotion_20260227T224446Z/`
- Determinism readout:
  - `runs_observed=100`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `contract_fail_rows=0`
  - `taxonomy_nonzero_rows=0`
  - `runs0_exit=2`
  - `badflag_exit=2`
  - `promotion_readiness=PASS`
