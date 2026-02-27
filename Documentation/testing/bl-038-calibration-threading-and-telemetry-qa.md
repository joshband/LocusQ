Title: BL-038 Calibration Threading and Telemetry QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-038 Calibration Threading and Telemetry QA Contract

## Purpose

Define deterministic QA acceptance mapping for BL-038 Slice A1 so threading boundaries, RT-safe telemetry publication, timeout/error handling, and evidence schema remain machine-checkable.

## Linked Contracts

- Runbook: `Documentation/backlog/bl-038-calibration-threading-and-telemetry.md`
- Invariants: `Documentation/invariants.md`
- ADR freshness gate: `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`

## Acceptance ID Catalog (Slice A1)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL038-A1-001` | Thread ownership matrix defines owner + forbidden responsibilities for all declared domains | runbook + QA parity |
| `BL038-A1-002` | Canonical state machine includes exactly 8 states and explicit ownership | runbook + QA parity |
| `BL038-A1-003` | Transition precedence order is fixed and deterministic | precedence table parity |
| `BL038-A1-004` | RT publication invariants forbid lock/alloc/wait on audio thread | publication contract parity |
| `BL038-A1-005` | Required telemetry schema defines 10 required fields with type/constraints | schema table parity |
| `BL038-A1-006` | Staleness thresholds explicitly bounded | threshold parity |
| `BL038-A1-007` | Timeout and error taxonomies are explicit and mapped to transitions | taxonomy parity |
| `BL038-A1-008` | Deterministic artifact schema defines required files and TSV columns | artifact schema parity |
| `BL038-A1-009` | Acceptance IDs appear across runbook + QA + evidence matrix | cross-surface parity |
| `BL038-A1-010` | Docs freshness gate passes | `docs_freshness.log` |

## Deterministic Threading Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL038-QA-001` | Owner exclusivity | each mutable lifecycle state has one owner domain |
| `BL038-QA-002` | Cross-thread handoff policy | all worker->RT publication uses full-generation atomic handoff |
| `BL038-QA-003` | Transition tie-break deterministic | precedence list applied for same-cycle event collisions |
| `BL038-QA-004` | Telemetry finiteness/range | confidence and clipping are finite and bounded `[0,1]` |
| `BL038-QA-005` | Sequence monotonicity contract | `snapshot_seq` monotonic non-regressing |

## Timeout/Error Determinism Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL038-QA-006` | Timeout class coverage | start/capture/analysis/publish timeout classes all defined |
| `BL038-QA-007` | Error class coverage | route, non-finite metric, sequence, ownership, shutdown classes defined |
| `BL038-QA-008` | Escalation mapping | each timeout/error class maps to deterministic state outcome |
| `BL038-QA-009` | Staleness classification | stale thresholds map to warn/fail classes and deterministic handling |

## Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity |
|---|---|---|---|---|---|
| `BL038-A1-FX-001` | thread_ownership_violation | non-owner thread mutates owner-owned lifecycle state | deterministic_contract_failure | yes | critical |
| `BL038-A1-FX-002` | transition_precedence_drift | same-cycle event ordering not resolved by declared precedence | deterministic_contract_failure | yes | major |
| `BL038-A1-FX-003` | rt_safety_violation | lock/alloc/blocking operation required on `audio_rt` publication path | deterministic_contract_failure | yes | critical |
| `BL038-A1-FX-004` | telemetry_schema_missing_field | required telemetry field absent or typed inconsistently | deterministic_contract_failure | yes | major |
| `BL038-A1-FX-005` | telemetry_value_non_finite | non-finite metric value published | deterministic_contract_failure | yes | major |
| `BL038-A1-FX-006` | telemetry_stale_timeout | `stale_ms` exceeds fail threshold (`>1000`) without deterministic classification | deterministic_runtime_failure | yes | major |
| `BL038-A1-FX-007` | sequence_regression | `snapshot_seq` regresses between publishes | deterministic_contract_failure | yes | critical |
| `BL038-A1-FX-008` | timeout_taxonomy_incomplete | required timeout class missing from contract | deterministic_contract_failure | yes | major |
| `BL038-A1-FX-009` | error_taxonomy_incomplete | required error class missing from contract | deterministic_contract_failure | yes | major |
| `BL038-A1-FX-010` | artifact_schema_incomplete | required evidence files or TSV columns missing | deterministic_evidence_failure | yes | major |
| `BL038-A1-FX-011` | acceptance_parity_drift | acceptance ID mismatch across runbook/QA/evidence | deterministic_contract_failure | yes | major |
| `BL038-A1-FX-012` | docs_freshness_failure | docs freshness gate returns non-zero | deterministic_evidence_failure | yes | major |

## Artifact Schema Contract (Slice A1)

Required evidence bundle path:
- `TestEvidence/bl038_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `threading_telemetry_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

Required schemas:
- `status.tsv`: `artifact`, `value`
- `acceptance_matrix.tsv`: `acceptance_id`, `gate`, `threshold`, `result`, `artifact`, `note`
- `failure_taxonomy.tsv`: `failure_id`, `category`, `trigger`, `classification`, `blocking`, `severity`

## A1 Validation Commands

```bash
./scripts/validate-docs-freshness.sh
```

Pass criteria:
- command exit code `0`
- runbook/QA acceptance ID parity preserved
- deterministic failure taxonomy IDs explicitly present

## Triage Sequence

1. Resolve acceptance-ID parity drift before any executable slice work.
2. Resolve missing timeout/error taxonomy IDs before contract promotion.
3. Resolve artifact schema gaps before slice closeout.
4. Resolve docs freshness violations before declaring PASS.

## B1 Executable Lane Contract

Canonical lane script:
- `scripts/qa-bl038-calibration-telemetry-lane-mac.sh`

Canonical scenario:
- `qa/scenarios/locusq_bl038_calibration_telemetry_suite.json`

### B1 Acceptance Mapping

| Acceptance ID | Lane Check | Contract |
|---|---|---|
| `BL038-B1-001` | `BL038-B1-001_contract_schema` | Scenario/lane schema parseable and acceptance IDs declared |
| `BL038-B1-002` | `BL038-B1-002_thread_state_contract` | Canonical thread ownership + state set contract declared |
| `BL038-B1-003` | `BL038-B1-003_telemetry_schema_contract` | Required telemetry field schema and constraints declared |
| `BL038-B1-004` | `BL038-B1-004_timeout_error_taxonomy_contract` | Timeout/error class and failure taxonomy contract declared |
| `BL038-B1-005` | `BL038-B1-005_replay_hash_stability` | Replay signatures stable across deterministic reruns |
| `BL038-B1-006` | `BL038-B1-006_artifact_schema_complete` | Required artifact schema present for selected mode |
| `BL038-B1-007` | `BL038-B1-007_hash_input_contract` | Hash input includes semantic rows and excludes nondeterministic fields |
| `BL038-B1-008` | `BL038-B1-008_execution_mode_contract` | `contract_only`/`execute_suite` mode semantics preserved |

### B1 Failure Taxonomy Classes

| failure_class | Deterministic Meaning |
|---|---|
| `deterministic_contract_failure` | Scenario/schema/threshold contract mismatch |
| `runtime_execution_failure` | Runner or tooling returned non-zero |
| `missing_result_artifact` | Required lane artifact missing |
| `deterministic_replay_divergence` | Replay signature mismatch beyond threshold |
| `deterministic_replay_row_drift` | Replay semantic row signature drift beyond threshold |

### B1 Validation Matrix

```bash
bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl038_slice_b1_lane_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

### B1 Artifact Contract

Required bundle:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C2 Determinism Soak Contract

### C2 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL038-C2-001` | `BL038-C2-001_soak_summary` | `result=PASS` with `runs=10` |
| `BL038-C2-002` | `BL038-B1-005_replay_hash_stability` | `signature_divergence=0` |
| `BL038-C2-003` | `BL038-B1-007_hash_input_contract` + replay rows | `row_drift=0` |

### C2 Validation Matrix

```bash
bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl038_slice_c2_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

### C2 Artifact Contract

Required bundle:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C2 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl038_slice_c2_soak_20260227T010825Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `soak_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl038_slice_c2_soak_20260227T010825Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## C3 Replay Sentinel Contract

### C3 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL038-C3-001` | `BL038-C3-001_replay_sentinel_summary` | `result=PASS` with `runs=20` |
| `BL038-C3-002` | `BL038-B1-005_replay_hash_stability` | `signature_divergence=0` |
| `BL038-C3-003` | `BL038-B1-007_hash_input_contract` + replay rows | `row_drift=0` |

### C3 Validation Matrix

```bash
bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c3_replay_sentinel_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

### C3 Artifact Contract

Required bundle:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C3 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl038_slice_c3_replay_sentinel_20260227T012154Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c3_replay_sentinel_20260227T012154Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## C5 Exit-Semantics Guard Contract

### C5 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL038-C5-001` | `BL038-C5-001_replay_sentinel_guard` | `result=PASS` with `runs=20` |
| `BL038-C5-002` | `BL038-C5-002_exit_semantics_runs_zero` | command `--runs 0` exits `2` |
| `BL038-C5-003` | `BL038-C5-003_exit_semantics_unknown_arg` | command `--unknown` exits `2` |

### C5 Validation Matrix

```bash
bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c5_semantics_<timestamp>/contract_runs
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown
./scripts/validate-docs-freshness.sh
```

### C5 Artifact Contract

Required bundle:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C5 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl038_slice_c5_semantics_20260227T015217Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c5_semantics_20260227T015217Z/contract_runs` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (PASS)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => FAIL (external non-owned metadata violation)

## C6 Execute-Mode Parity + Exit Guard Contract

### C6 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL038-C6-001` | `BL038-C6-PAR-*` mode parity checks | contract-only + execute-suite replay summaries both PASS at `runs=20` with zero deterministic failure counters |
| `BL038-C6-002` | `BL038-C6-VAL-005` usage probe | command `--runs 0` exits `2` |
| `BL038-C6-003` | `BL038-C6-VAL-006` usage probe | command `--unknown` exits `2` |

### C6 Validation Matrix

```bash
bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_<timestamp>/contract_runs_contract
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_<timestamp>/contract_runs_execute
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown
./scripts/validate-docs-freshness.sh
```

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
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C6 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl038_slice_c6_mode_parity_20260227T025802Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_20260227T025802Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_20260227T025802Z/contract_runs_execute` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (PASS)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => FAIL (external non-owned metadata violations under `Documentation/Calibration POC/`)

## C6r Post-H2 Recheck Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl038_slice_c6r_mode_parity_20260227T031054Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c6r_mode_parity_20260227T031054Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl038_slice_c6r_mode_parity_20260227T031054Z/contract_runs_execute` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (PASS)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => PASS

## C7 Long-Run Parity Sentinel Contract

### C7 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL038-C7-001` | `BL038-C7-PAR-*` long-run parity checks | contract-only + execute-suite replay summaries both PASS at `runs=50` with zero deterministic failure counters |
| `BL038-C7-002` | `BL038-C7-VAL-005` usage probe | command `--runs 0` exits `2` |
| `BL038-C7-003` | `BL038-C7-VAL-006` usage probe | command `--unknown` exits `2` |

### C7 Validation Matrix

```bash
bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_<timestamp>/contract_runs_contract
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_<timestamp>/contract_runs_execute
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0
./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown
./scripts/validate-docs-freshness.sh
```

### C7 Artifact Contract

Required bundle:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C7 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl038_slice_c7_longrun_parity_20260227T033937Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_20260227T033937Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_20260227T033937Z/contract_runs_execute` => PASS
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (PASS)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (PASS)
  - `./scripts/validate-docs-freshness.sh` => PASS
