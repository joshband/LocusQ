Title: BL-042 QA CI Regression Gates QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-042 QA CI Regression Gates QA Contract

## Purpose

Define deterministic QA contract checks and machine-readable evidence schema for BL-042 Slice A1 CI regression gate authority.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/bl-042-qa-ci-regression-gates.md`

Traceability anchors:
- `.ideas/parameter-spec.md`
- `.ideas/architecture.md`
- `Documentation/invariants.md`

## Deterministic QA Checks

### Gate Coverage and Order

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL042-QA-001` | Mandatory gate coverage | all required gates declared (`build/smoke/selftest/rt/docs/schema`) |
| `BL042-QA-002` | Deterministic gate order | fixed gate order declared and machine-checkable |
| `BL042-QA-003` | Strict exit semantics | wrapper semantics explicitly constrained to `0/1/2` |

### Gate Command Contracts

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL042-QA-004` | Build gate contract | canonical command + success criteria explicit |
| `BL042-QA-005` | Smoke gate contract | canonical command + success criteria explicit |
| `BL042-QA-006` | Selftest gate contract | canonical command + payload/success criteria explicit |
| `BL042-QA-007` | RT gate contract | command + `non_allowlisted=0` success criteria explicit |
| `BL042-QA-008` | Docs/schema gate contract | docs freshness + status schema commands/success criteria explicit |

### Evidence and Taxonomy Contracts

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL042-QA-009` | Acceptance matrix schema | required `acceptance_matrix.tsv` columns declared |
| `BL042-QA-010` | Failure taxonomy schema | required `failure_taxonomy.tsv` columns and `BL042-FX-*` mapping declared |
| `BL042-QA-011` | Evidence bundle completeness | required A1 files declared and path contract explicit |

## Acceptance Matrix Contract (Slice A1)

Required acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| `BL042-A1-001` | Gate coverage completeness | all mandatory CI gates declared |
| `BL042-A1-002` | Deterministic gate order | explicit fixed order declared |
| `BL042-A1-003` | Build gate contract | canonical build command and success criteria explicit |
| `BL042-A1-004` | Smoke gate contract | canonical smoke command and success criteria explicit |
| `BL042-A1-005` | Selftest gate contract | canonical selftest command and success criteria explicit |
| `BL042-A1-006` | RT gate contract | RT command and `non_allowlisted=0` criteria explicit |
| `BL042-A1-007` | Docs/schema gate contract | docs freshness + schema command contracts explicit |
| `BL042-A1-008` | Exit semantics contract | strict `0/1/2` semantics declared |
| `BL042-A1-009` | Failure taxonomy completeness | required `BL042-FX-*` map declared |
| `BL042-A1-010` | Artifact schema completeness | required files and TSV column contracts declared |
| `BL042-A1-011` | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

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
| `BL042-FX-001` | gate_set_incomplete | required gate missing from CI contract | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL042-FX-002` | gate_order_missing_or_nondeterministic | gate order absent/ambiguous | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL042-FX-003` | build_gate_failure | build command fails or target missing | runtime_gate_failure | yes | critical | status.tsv |
| `BL042-FX-004` | smoke_gate_failure | smoke command fails or smoke artifact missing | runtime_gate_failure | yes | critical | status.tsv |
| `BL042-FX-005` | selftest_gate_failure | selftest exits non-zero or payload contract missing | runtime_gate_failure | yes | critical | status.tsv |
| `BL042-FX-006` | rt_gate_failure | RT audit fails or `non_allowlisted>0` | runtime_gate_failure | yes | critical | status.tsv |
| `BL042-FX-007` | docs_gate_failure | docs freshness exits non-zero | governance_gate_failure | yes | major | docs_freshness.log |
| `BL042-FX-008` | schema_gate_failure | `jq empty status.json` exits non-zero | governance_gate_failure | yes | major | status.tsv |
| `BL042-FX-009` | exit_semantics_contract_failure | wrapper exits outside `0/1/2` | deterministic_contract_failure | yes | major | status.tsv |
| `BL042-FX-010` | artifact_schema_incomplete | required evidence file/column missing | deterministic_evidence_failure | yes | major | status.tsv |

Required `failure_taxonomy.tsv` columns:
- `failure_id`
- `category`
- `trigger`
- `classification`
- `blocking`
- `severity`
- `expected_artifact`

## Evidence Bundle (Slice A1)

Required output path:
- `TestEvidence/bl042_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `ci_regression_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Validation

- `./scripts/validate-docs-freshness.sh`

## Acceptance Matrix (B1 Lane Bootstrap)

| acceptance_id | gate | threshold |
|---|---|---|
| `BL042-B1-001` | Contract surface availability | runbook + QA contract docs present and parseable |
| `BL042-B1-002` | Gate declaration coverage | required gate tokens (`ci_gate_build`..`ci_gate_schema`) present |
| `BL042-B1-003` | A1 acceptance parity | `BL042-A1-001..011` present in both contract surfaces |
| `BL042-B1-004` | A1 taxonomy parity | `BL042-FX-001..010` present in both contract surfaces |
| `BL042-B1-005` | Artifact schema completeness | lane emits `status/validation/replay/taxonomy` outputs |
| `BL042-B1-006` | Replay hash stability | 3-run replay has zero signature and row drift |
| `BL042-B1-007` | Execution mode contract | script accepts `--contract-only` and `--execute-suite` |
| `BL042-B1-008` | Usage error semantics | invalid invocation exits `2` |

## B1 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| `BL042-FX-101` | lane_contract_surface_missing | runbook/QA contract doc missing | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| `BL042-FX-102` | lane_gate_contract_missing | required gate tokens missing | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| `BL042-FX-103` | lane_acceptance_parity_missing | A1 acceptance IDs missing/incomplete | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| `BL042-FX-104` | lane_taxonomy_parity_missing | A1 taxonomy IDs missing/incomplete | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| `BL042-FX-105` | lane_replay_signature_drift | replay signature mismatch for equal inputs | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| `BL042-FX-106` | lane_replay_row_drift | replay row signature mismatch for equal inputs | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| `BL042-FX-107` | lane_artifact_schema_incomplete | required artifact missing | deterministic_evidence_failure | yes | status.tsv |

## B1 Validation

- `bash -n scripts/qa-bl042-ci-regression-gates-lane-mac.sh`
- `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --help`
- `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl042_slice_b1_lane_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## B1 Evidence Contract

Required files under `TestEvidence/bl042_slice_b1_lane_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## B1 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl042_slice_b1_lane_20260227T205414Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl042-ci-regression-gates-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl042_slice_b1_lane_20260227T205414Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - B1 deterministic lane bootstrap is stable and machine-readable for owner intake.

## Acceptance Matrix (C2 Determinism Soak)

| acceptance_id | gate | threshold |
|---|---|---|
| `BL042-C2-001` | Contract replay depth | `--contract-only --runs 10` executes fully |
| `BL042-C2-002` | Replay signature stability | 10-run signature drift count is `0` |
| `BL042-C2-003` | Replay row stability | 10-run row drift count is `0` |
| `BL042-C2-004` | Failure taxonomy stability | all failure-class counts remain `0` |
| `BL042-C2-005` | Governance gate | docs freshness exits `0` |

## C2 Validation

- `bash -n scripts/qa-bl042-ci-regression-gates-lane-mac.sh`
- `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --help`
- `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl042_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## C2 Evidence Contract

Required files under `TestEvidence/bl042_slice_c2_soak_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C2 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl042_slice_c2_soak_20260227T214654Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `soak_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl042-ci-regression-gates-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl042-ci-regression-gates-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl042_slice_c2_soak_20260227T214654Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - C2 deterministic soak passed 10/10 with zero signature drift, zero row drift, and zero taxonomy failures.

## Z16 Reconcile Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl042_e2e_reconcile_z16_20260227T225524Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `execute_runs/validation_matrix.tsv`
  - `exit_semantics_probe.tsv`
  - `blocker_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Gate replay result:
  - `--contract-only --runs 20` => PASS (deterministic, zero drift)
  - `--execute-suite --runs 3` => FAIL (all failures at `ci_gate_rt`)
  - Execute-suite deterministic blockers: `non_allowlisted=8` in every replay run.
- Exit semantics:
  - `--runs 0` => exit `2` (PASS)
  - `--unknown-flag` => exit `2` (PASS)
