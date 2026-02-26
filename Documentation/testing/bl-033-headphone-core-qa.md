Title: BL-033 Headphone Calibration Core QA Contract
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-26

# BL-033 Headphone Calibration Core QA Contract (D1 + D2)

## Purpose
Define the deterministic QA lane contract for BL-033 Slice D1 scaffold and Slice D2 closeout reliability checks so headphone calibration validation can run with strict machine-readable evidence.

## Linked Contracts
- Runbook: `Documentation/backlog/bl-033-headphone-calibration-core.md`
- Spec: `Documentation/plans/bl-033-headphone-calibration-core-spec-2026-02-25.md`
- Scenario: `qa/scenarios/locusq_bl033_headphone_core_suite.json`
- Lane Script: `scripts/qa-bl033-headphone-core-lane-mac.sh`

## Acceptance IDs

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL033-D1-001` | Scenario + lane schema is parseable and complete | `status.tsv` (`BL033-D1-001_contract_schema`) |
| `BL033-D1-002` | Required diagnostics field list is declared | `status.tsv` (`BL033-D1-002_diagnostics_fields`) |
| `BL033-D1-003` | Artifact schema contract is explicit and machine-readable | `status.tsv` (`BL033-D1-003_artifact_schema`) |
| `BL033-D1-004` | Acceptance-ID parity is enforced across runbook/qa/trace/scenario/lane | `acceptance_parity.tsv` + `status.tsv` (`BL033-D1-004_acceptance_parity`) |
| `BL033-D1-005` | Lane thresholds are deterministic and valid | `status.tsv` (`BL033-D1-005_lane_thresholds`) |
| `BL033-D1-006` | Execution mode semantics are explicit (`contract_only` vs `execute_suite`) | `status.tsv` (`BL033-D1-006_execution_mode`) |
| `BL033-D2-001` | Diagnostics requested/active/stage/fallback contract consistency is enforced | `status.tsv` (`BL033-D2-001_diagnostics_consistency`) |
| `BL033-D2-002` | Replay determinism contract is enforced with hash and row-stability thresholds | `replay_hashes.tsv` + `status.tsv` (`BL033-D2-002_replay_hash_stability`) |
| `BL033-D2-003` | Strict artifact schema completeness is enforced for single-run and multi-run modes | `status.tsv` (`BL033-D2-003_artifact_schema_complete`) |

## Lane Command Contract

Contract-only scaffold mode (default):
```bash
./scripts/qa-bl033-headphone-core-lane-mac.sh --contract-only --out-dir TestEvidence/bl033_headphone_core_<timestamp>
```

Full suite execution mode (for merged source slices):
```bash
./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --out-dir TestEvidence/bl033_headphone_core_<timestamp>
```

Deterministic multi-run scaffold mode:
```bash
./scripts/qa-bl033-headphone-core-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl033_headphone_core_<timestamp>/contract_runs
```

Deterministic multi-run closeout mode:
```bash
./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --runs 5 --out-dir TestEvidence/bl033_headphone_core_<timestamp>/exec_runs
```

Optional controls:
- `--scenario <path>` to override scenario path.
- `--qa-bin <path>` to override QA runner path.
- `--runs <N>` to execute deterministic `run_01..run_N` directories (`N >= 1`).
- `--skip-build` to skip build in `--execute-suite` mode.

## Scenario Contract Summary

Scenario file: `qa/scenarios/locusq_bl033_headphone_core_suite.json`

Declared deterministic contract keys:
1. `bl033_contract_checks.acceptance_ids`
2. `bl033_contract_checks.required_diagnostics_fields`
3. `bl033_contract_checks.thresholds`
4. `bl033_contract_checks.artifact_schema`
5. `bl033_contract_checks.failure_taxonomy`
6. `bl033_contract_checks.check_rows`

## Artifact Schema

Required lane outputs under `<out-dir>`:
1. `status.tsv`
2. `qa_lane.log`
3. `scenario_contract.log`
4. `scenario_result.log`
5. `acceptance_parity.tsv`
6. `taxonomy_table.tsv`

Optional output in `--execute-suite` mode:
1. `scenario_result.json`
2. `build.log`

Additional output when `--runs > 1`:
1. `validation_matrix.tsv` (aggregate per-run machine-readable summary)
2. `replay_hashes.tsv` (run-level deterministic hash/signature ledger)
3. `acceptance_parity.tsv` (baseline parity snapshot for replay set)
4. `taxonomy_table.tsv` (aggregate failure taxonomy)
5. `<out-dir>/run_01..run_N/` (each run preserves standard lane artifacts)

## Deterministic Thresholds

| Metric | Threshold | Result Rule |
|---|---|---|
| `suite_status` | `PASS` | FAIL if suite status differs in execute mode |
| `max_warnings` | `0` | FAIL if warnings exceed threshold in execute mode |
| `acceptance_parity_failures` | `0` | FAIL if any acceptance ID missing from required surfaces |
| `run_failure_count` | `0` | FAIL if any run exits non-zero or reports `lane_result != PASS` when `--runs > 1` |
| `max_signature_divergence` | `0` | FAIL if replay signature divergence exceeds threshold |
| `max_row_drift` | `0` | FAIL if replay row-signature drift exceeds threshold |

## Failure Taxonomy

| Failure Class | Category | Trigger |
|---|---|---|
| `deterministic_contract_failure` | deterministic | Scenario/schema/threshold mismatch |
| `runtime_execution_failure` | runtime | Build or QA runner non-zero exit |
| `missing_result_artifact` | runtime | Expected suite result artifact not found |
| `acceptance_parity_failure` | deterministic | Acceptance ID missing from one or more required surfaces |
| `diagnostics_consistency_failure` | deterministic | Diagnostics consistency contract check fails |
| `replay_determinism_failure` | deterministic | Replay hash/row signatures diverge beyond threshold |

## Validation Commands

```bash
bash -n scripts/qa-bl033-headphone-core-lane-mac.sh
./scripts/qa-bl033-headphone-core-lane-mac.sh --help
./scripts/qa-bl033-headphone-core-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl033_slice_d2_qa_closeout_<timestamp>/contract_runs
./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --runs 5 --out-dir TestEvidence/bl033_slice_d2_qa_closeout_<timestamp>/exec_runs
./scripts/validate-docs-freshness.sh
```

## Triage Sequence

1. If schema checks fail, fix scenario/lane/doc contract before running execute mode.
2. If parity fails, restore missing acceptance IDs in runbook/qa/trace/scenario/lane surfaces.
3. If execute mode fails, inspect `build.log`, `scenario_run.log`, and `scenario_result.log`.
4. If replay closeout mode fails, inspect `replay_hashes.tsv` for `signature_match`/`row_match` drift first.
5. Inspect `validation_matrix.tsv` and per-run `run_XX/status.tsv` for diagnostics/artifact check failures.
6. Use `taxonomy_table.tsv` to classify deterministic vs runtime failures before promotion decisions.
