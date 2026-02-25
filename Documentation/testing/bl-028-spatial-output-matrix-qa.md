Title: BL-028 Spatial Output Matrix QA Contract
Document Type: Testing Specification
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-028 Deterministic QA Lane Contract (Slice A1)

## Purpose
Define deterministic, implementation-ready validation requirements for BL-028 spatial output matrix legality, mismatch fallback behavior, diagnostics publication, and user-facing status text parity.

## Linked Contracts
- Runbook: `Documentation/backlog/done/bl-028-spatial-output-matrix.md`
- Spec: `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md`

## Acceptance IDs

| Acceptance ID | QA Enforcement |
|---|---|
| BL028-A1-001 | Matrix legality scenarios validate all required layout/domain combinations. |
| BL028-A1-002 | Mismatch scenarios validate fallback mode precedence + fail-safe route selection. |
| BL028-A1-003 | Diagnostics payload schema assertions validate required fields and enums. |
| BL028-A1-004 | Status text assertions validate reason-code text mapping parity. |
| BL028-A1-005 | Lane output artifacts and thresholds are deterministic and machine-readable. |
| BL028-A1-006 | Acceptance IDs appear unchanged in runbook/spec/qa docs. |

## Lane Command Contract (Slice B1 Implemented)

Primary lane command:
```bash
./scripts/qa-bl028-output-matrix-lane-mac.sh
```

Lane controls (Slice B2 reliability hardening):
```bash
./scripts/qa-bl028-output-matrix-lane-mac.sh --out-dir TestEvidence/bl028_slice_b2_<timestamp> --runs 5
```

Environment-compatible controls remain supported:
- `BL028_OUT_DIR`
- `BL028_RUNS`

Scenario suite:
```bash
qa/scenarios/locusq_bl028_output_matrix_suite.json
```

Direct runner command used by lane:
```bash
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_bl028_output_matrix_suite.json
```

## Scenario Set (Deterministic)

| Scenario ID | Coverage | Expected Result |
|---|---|---|
| `bl028_legal_binaural_stereo` | InternalBinaural + stereo_2_0 (+/- head tracking) | ALLOW |
| `bl028_block_binaural_multichannel` | InternalBinaural + {quad,5.1,7.1,7.4.2} | BLOCK + fallback |
| `bl028_legal_multichannel_quad` | Multichannel + quad_4_0 | ALLOW |
| `bl028_legal_multichannel_5_1` | Multichannel + surround_5_1 | ALLOW |
| `bl028_legal_multichannel_7_1` | Multichannel + surround_7_1 | ALLOW |
| `bl028_legal_multichannel_7_4_2` | Multichannel + immersive_7_4_2 | ALLOW |
| `bl028_block_multichannel_stereo` | Multichannel + stereo_2_0 | BLOCK + fallback |
| `bl028_block_ht_multichannel` | Multichannel + head tracking enabled | BLOCK + fallback |
| `bl028_legal_external_spatial_bed` | ExternalSpatial + multichannel bed | ALLOW |
| `bl028_block_external_spatial_stereo` | ExternalSpatial + stereo_2_0 | BLOCK + fallback |

## Artifact Schema (Required)

### 1) `status.tsv`
Columns:
- `check`
- `result` (`PASS`/`FAIL`)
- `detail`
- `artifact`

### 2) `qa_lane.log`
Columns:
- Human-readable per-step execution log emitted by lane script.

### 3) `scenario_result.log`
Key-value ledger produced by lane script:
- `scenario_id`
- `result_status`
- `summary_passed`
- `summary_failed`
- `summary_total`
- `summary_warned`
- `result_json`

### 4) `matrix_report.tsv`
Columns:
- `case_id`
- `acceptance_id`
- `requested_domain`
- `host_layout`
- `head_tracking`
- `expected_decision`
- `expected_rule_id`
- `expected_fallback_mode`
- `expected_fail_safe_route`
- `expected_reason_code`
- `expected_status_text`
- `row_result`
- `row_detail`

### 5) `acceptance_parity.tsv`
Columns:
- `acceptance_id`
- `runbook_count`
- `spec_count`
- `qa_doc_count`
- `scenario_count`
- `lane_script_count`
- `matrix_case_count`
- `mapped_check`
- `result`

### 6) `replay_runs.tsv`
Columns:
- `run`
- `qa_exit`
- `result_status`
- `warnings`
- `passed`
- `failed`
- `total`
- `result_json`
- `result_sha256`
- `matrix_sha256`
- `combined_signature`
- `baseline_match`
- `run_result`
- `failure_class`

### 7) `replay_hashes.tsv`
Columns:
- `run`
- `result_json_sha256`
- `matrix_report_sha256`
- `combined_signature`
- `baseline_signature`
- `signature_match`

### 8) `reliability_decision.md`
Required sections:
- final verdict (`PASS`/`FAIL`)
- replay divergence metrics and thresholds
- transient failure metrics and thresholds
- deterministic vs transient failure taxonomy counts

## Deterministic Pass/Fail Thresholds

| Metric | Threshold | Result Rule |
|---|---|---|
| `suite_status` | `PASS` | FAIL if not `PASS` |
| `suite_warnings` | `0` | FAIL if `> 0` |
| `matrix_accuracy` | `1.0` | FAIL if `< 1.0` |
| `fallback_accuracy` | `1.0` | FAIL if `< 1.0` |
| `diagnostics_schema_min_fields` | `11` | FAIL if `< 11` |
| `status_text_map_min_entries` | `7` | FAIL if `< 7` |
| `max_signature_divergence` | `0` | FAIL if `> 0` |
| `max_transient_failures` | `0` | FAIL if `> 0` |
| `run_failure_count` | `0` | FAIL if `> 0` |

## Replay Contract (Slice B2)

1. Lane supports `--runs <N>` with default from scenario replay contract and strict integer validation.
2. Lane captures per-run suite results and per-run signatures in `replay_runs.tsv`.
3. Lane computes and compares run signatures (result summary + matrix hash) against run 1 baseline.
4. Lane fails when signature divergence exceeds contract threshold.
5. Lane exit code is `0` only when all build/suite/acceptance/replay checks pass.

## Failure Taxonomy (Deterministic vs Transient)

| Failure Class | Category | Trigger |
|---|---|---|
| `deterministic_replay_divergence` | deterministic | run signature differs from baseline beyond contract |
| `deterministic_contract_failure` | deterministic | suite status/warnings violate configured thresholds |
| `transient_runtime_failure` | transient | QA runner exits non-zero for a replay run |
| `transient_result_missing` | transient | expected result JSON missing after runner exit 0 |

## Non-Determinism Guardrails
1. No randomness in scenario generation.
2. Fixed scenario ordering and expected outputs.
3. Explicit enum/value assertions for rule IDs, fallback modes, and reason codes.
4. Replay of same scenario set must produce identical `matrix_report.tsv` rows.
5. Replay signatures in `replay_hashes.tsv` must match the baseline run under strict contract.

## Acceptance ID Check Mapping (Executable)

| Acceptance ID | Lane Check ID | Artifact |
|---|---|---|
| BL028-A1-001 | `BL028-A1-001_matrix_legality` | `matrix_report.tsv` |
| BL028-A1-002 | `BL028-A1-002_fallback_contract` | `matrix_report.tsv` |
| BL028-A1-003 | `BL028-A1-003_diagnostics_schema` | `status.tsv` |
| BL028-A1-004 | `BL028-A1-004_status_text_map` | `status.tsv` |
| BL028-A1-005 | `BL028-A1-005_lane_thresholds` | `status.tsv` |
| BL028-A1-006 | `BL028-A1-006_acceptance_parity` | `acceptance_parity.tsv` |

## Cross-Document ID Parity Check
The lane enforces that IDs `BL028-A1-001..006` remain present in runbook + spec + QA + scenario + lane script surfaces.
