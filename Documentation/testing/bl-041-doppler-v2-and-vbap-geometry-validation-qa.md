Title: BL-041 Doppler v2 and VBAP Geometry Validation QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-041 Doppler v2 and VBAP Geometry Validation QA Contract

## Purpose

Define deterministic QA checks and machine-readable evidence schema for BL-041 Slice A1 Doppler/VBAP contract authority.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/bl-041-doppler-v2-and-vbap-geometry-validation.md`

Traceability anchors:
- `.ideas/parameter-spec.md`
- `.ideas/architecture.md`
- `Documentation/invariants.md`

## Deterministic QA Checks

### Doppler Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL041-QA-001 | Doppler field coverage | required fields/ranges/fallbacks declared |
| BL041-QA-002 | Smoothing threshold coverage | all ratio/delay/continuity thresholds declared |
| BL041-QA-003 | Non-finite fail-closed behavior | finite-only + fallback policy explicit |

### VBAP Geometry Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL041-QA-004 | Geometry threshold coverage | `area_epsilon`, boundary window, gain jump limits defined |
| BL041-QA-005 | Deterministic triplet tie-break | sorted deterministic tie-break rule explicit |
| BL041-QA-006 | Boundary continuity policy | fixed crossfade and discontinuity threshold explicit |

### Replay and Evidence Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL041-QA-007 | Replay hash contract | required deterministic hash input set defined |
| BL041-QA-008 | Replay equality contract | identical input => identical trace output rule explicit |
| BL041-QA-009 | Artifact schema contract | required files and TSV columns fully declared |

## Acceptance Matrix Contract (Slice A1)

Required acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| BL041-A1-001 | Doppler input/bounds contract completeness | required fields, ranges, and fallback clauses all explicit |
| BL041-A1-002 | Doppler smoothing thresholds defined | all smoothing/continuity thresholds declared with numeric limits |
| BL041-A1-003 | VBAP geometry validity thresholds defined | area/boundary/gain thresholds declared with deterministic rules |
| BL041-A1-004 | Deterministic tie-break and transition policy | triplet tie-break + boundary crossfade rules explicit |
| BL041-A1-005 | Replay contract completeness | required hash inputs + deterministic equality requirement explicit |
| BL041-A1-006 | Failure taxonomy completeness | all required BL041-FX IDs defined |
| BL041-A1-007 | Artifact schema completeness | required artifacts and TSV column contracts declared |
| BL041-A1-008 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exit code `0` |

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
| BL041-FX-001 | doppler_contract_incomplete | required Doppler field/range/fallback clause missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL041-FX-002 | doppler_smoothing_threshold_missing | smoothing/continuity thresholds absent | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL041-FX-003 | doppler_non_finite_state | non-finite Doppler input/intermediate/state detected | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL041-FX-004 | vbap_geometry_contract_incomplete | geometry validity thresholds/rules missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL041-FX-005 | vbap_degenerate_triplet | triplet area `<= area_epsilon` | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL041-FX-006 | vbap_gain_normalization_failure | finite/normalized gain contract violated | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL041-FX-007 | boundary_continuity_violation | boundary gain jump exceeds threshold | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| BL041-FX-008 | replay_identity_incomplete | deterministic hash input set incomplete | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| BL041-FX-009 | replay_trace_divergence | identical inputs produce divergent output/fallback traces | deterministic_replay_failure | yes | critical | acceptance_matrix.tsv |
| BL041-FX-010 | artifact_schema_incomplete | required artifact or required columns missing | deterministic_evidence_failure | yes | major | status.tsv |
| BL041-FX-011 | docs_freshness_gate_failure | docs freshness script exits non-zero | governance_failure | yes | major | docs_freshness.log |

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
- `TestEvidence/bl041_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `doppler_vbap_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Validation

- `./scripts/validate-docs-freshness.sh`

## B1 Acceptance Matrix (Lane Bootstrap)

| acceptance_id | gate | threshold |
|---|---|---|
| BL041-B1-001 | Scenario schema contract | `id=locusq_bl041_doppler_vbap_suite` and required B1 schema fields present |
| BL041-B1-002 | Acceptance alignment | IDs `BL041-B1-001..008` present in scenario + runbook + QA docs |
| BL041-B1-003 | Deterministic hash input contract | all required include fields present and deterministic excludes declared |
| BL041-B1-004 | Fallback token contract | required fallback reason tokens present |
| BL041-B1-005 | Artifact schema contract | required lane artifacts declared and emitted |
| BL041-B1-006 | Runbook alignment | runbook includes B1 validation + evidence contract references |
| BL041-B1-007 | QA alignment | QA doc includes B1 validation + evidence contract references |
| BL041-B1-008 | Mode and exit semantics | script `--contract-only`/`--execute-suite` and strict exits `0/1/2` |

## B1 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| BL041-FX-101 | lane_contract_schema_missing | scenario id/schema fields missing | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| BL041-FX-102 | lane_acceptance_alignment_missing | B1 IDs missing in scenario/runbook/QA | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| BL041-FX-103 | lane_hash_input_contract_missing | deterministic hash input set incomplete | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| BL041-FX-104 | lane_fallback_contract_missing | required fallback tokens missing | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| BL041-FX-105 | lane_replay_signature_drift | replay signature diverges for equal inputs | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| BL041-FX-106 | lane_replay_row_drift | replay row signature diverges for equal inputs | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| BL041-FX-107 | lane_artifact_schema_incomplete | required artifact missing | deterministic_evidence_failure | yes | status.tsv |
| BL041-FX-108 | docs_freshness_gate_failure | docs freshness exits non-zero | governance_failure | yes | docs_freshness.log |

## B1 Validation

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl041_slice_b1_lane_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## B1 Evidence Contract

Required files under `TestEvidence/bl041_slice_b1_lane_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C2 Acceptance Matrix (Determinism Soak)

| acceptance_id | gate | threshold |
|---|---|---|
| BL041-C2-001 | C2 soak replay contract | `required_runs=10`, `max_signature_divergence=0`, `max_row_drift=0` |
| BL041-C2-002 | C2 acceptance alignment | IDs `BL041-C2-001..005` present in scenario + runbook + QA docs |
| BL041-C2-003 | C2 evidence schema | required C2 artifacts declared in scenario contract |
| BL041-C2-004 | Runbook C2 alignment | runbook contains `Validation Plan (C2)` + `Evidence Contract (C2)` + `--runs 10` |
| BL041-C2-005 | QA C2 alignment | QA runbook contains `C2 Validation` + `C2 Evidence Contract` + `--runs 10` |

## C2 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| BL041-FX-201 | lane_c2_contract_missing | C2 soak fields/thresholds missing | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| BL041-FX-202 | lane_c2_acceptance_alignment_missing | C2 IDs missing in scenario/runbook/QA | deterministic_contract_failure | yes | contract_runs/validation_matrix.tsv |
| BL041-FX-203 | lane_c2_evidence_schema_incomplete | C2 required evidence rows missing | deterministic_evidence_failure | yes | validation_matrix.tsv |
| BL041-FX-204 | lane_c2_signature_drift | replay signature drift across 10-run soak | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| BL041-FX-205 | lane_c2_row_drift | replay row signature drift across 10-run soak | deterministic_replay_failure | yes | contract_runs/replay_hashes.tsv |
| BL041-FX-206 | lane_c2_taxonomy_drift | taxonomy non-zero or unstable across soak runs | deterministic_replay_failure | yes | contract_runs/failure_taxonomy.tsv |
| BL041-FX-207 | docs_freshness_gate_failure | docs freshness exits non-zero | governance_failure | yes | docs_freshness.log |

## C2 Validation

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl041_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## C2 Evidence Contract

Required files under `TestEvidence/bl041_slice_c2_soak_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C3 Acceptance Matrix (Replay Sentinel + Mode Parity)

| acceptance_id | gate | threshold |
|---|---|---|
| BL041-C3-001 | C3 replay sentinel contract | `required_runs=20`, `max_signature_divergence=0`, `max_row_drift=0` |
| BL041-C3-002 | C3 acceptance alignment | IDs `BL041-C3-001..006` present in scenario + runbook + QA docs |
| BL041-C3-003 | C3 evidence schema | required C3 artifacts declared in scenario contract |
| BL041-C3-004 | Runbook C3 alignment | runbook contains `Validation Plan (C3)` + `Evidence Contract (C3)` + parity/exit-probe artifacts |
| BL041-C3-005 | QA C3 alignment | QA runbook contains C3 validation + mode parity + exit semantics checks |
| BL041-C3-006 | Mode and exit semantics | script preserves contract/execute semantics; invalid `--runs` exits `2` |

## C3 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| BL041-FX-301 | lane_c3_contract_missing | C3 replay sentinel thresholds missing | deterministic_contract_failure | yes | contract_runs_contract/validation_matrix.tsv |
| BL041-FX-302 | lane_c3_acceptance_alignment_missing | C3 IDs missing in scenario/runbook/QA | deterministic_contract_failure | yes | validation_matrix.tsv |
| BL041-FX-303 | lane_c3_evidence_schema_incomplete | C3 required evidence rows missing | deterministic_evidence_failure | yes | validation_matrix.tsv |
| BL041-FX-304 | lane_c3_mode_parity_failure | contract-only and execute replay drift | deterministic_replay_failure | yes | mode_parity.tsv |
| BL041-FX-305 | lane_c3_exit_semantics_failure | negative probe exit code != 2 | deterministic_contract_failure | yes | exit_semantics_probe.tsv |
| BL041-FX-306 | lane_c3_taxonomy_drift | non-zero/unstable taxonomy rows across parity replay | deterministic_replay_failure | yes | soak_summary.tsv |
| BL041-FX-307 | docs_freshness_gate_failure | docs freshness exits non-zero | governance_failure | yes | docs_freshness.log |

## C3 Validation

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl041_slice_c3_replay_mode_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl041_slice_c3_replay_mode_parity_<timestamp>/contract_runs_execute`
- Negative probe: `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## C3 Evidence Contract

Required files under `TestEvidence/bl041_slice_c3_replay_mode_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `soak_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C4 Acceptance Matrix (Long-Run Replay Sentinel + Mode Parity)

| acceptance_id | gate | threshold |
|---|---|---|
| BL041-C4-001 | Contract-only long-run sentinel | `--contract-only --runs 50` with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-C4-002 | Execute-suite long-run sentinel | `--execute-suite --runs 50` with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-C4-003 | Cross-mode parity | `cross_mode_signature_mismatch_count=0` and `cross_mode_row_mismatch_count=0` |
| BL041-C4-004 | Taxonomy stability | contract/execute `taxonomy_nonzero_rows=0` |
| BL041-C4-005 | Exit semantics guard | `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` exits `2` |
| BL041-C4-006 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |
| BL041-C4-007 | Evidence schema completeness | all required C4 artifacts emitted |

## C4 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| BL041-FX-401 | lane_c4_contract_longrun_drift | contract-only 50-run replay drift | deterministic_replay_failure | yes | contract_runs_contract/replay_hashes.tsv |
| BL041-FX-402 | lane_c4_execute_longrun_drift | execute-suite 50-run replay drift | deterministic_replay_failure | yes | contract_runs_execute/replay_hashes.tsv |
| BL041-FX-403 | lane_c4_mode_parity_failure | contract/execute parity mismatch | deterministic_replay_failure | yes | mode_parity.tsv |
| BL041-FX-404 | lane_c4_taxonomy_drift | non-zero/unstable taxonomy rows | deterministic_replay_failure | yes | soak_summary.tsv |
| BL041-FX-405 | lane_c4_exit_semantics_failure | negative probe exit code != 2 | deterministic_contract_failure | yes | exit_semantics_probe.tsv |
| BL041-FX-406 | lane_c4_docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes | docs_freshness.log |
| BL041-FX-407 | lane_c4_evidence_schema_incomplete | required evidence rows/files missing | deterministic_evidence_failure | yes | status.tsv |

## C4 Validation

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl041_slice_c4_longrun_mode_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl041_slice_c4_longrun_mode_parity_<timestamp>/contract_runs_execute`
- Negative probe: `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## C4 Evidence Contract

Required files under `TestEvidence/bl041_slice_c4_longrun_mode_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `soak_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## D1 Acceptance Matrix (Done-Candidate Long-Run Mode Parity)

| acceptance_id | gate | threshold |
|---|---|---|
| BL041-D1-001 | Contract-only done-candidate sentinel | `--contract-only --runs 75` with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-D1-002 | Execute-suite done-candidate sentinel | `--execute-suite --runs 75` with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-D1-003 | Cross-mode parity at D1 depth | `cross_mode_signature_mismatch_count=0` and `cross_mode_row_mismatch_count=0` |
| BL041-D1-004 | Taxonomy stability at D1 depth | contract/execute `taxonomy_nonzero_rows=0` |
| BL041-D1-005 | Exit semantics guard | `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` exits `2` |
| BL041-D1-006 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |
| BL041-D1-007 | Evidence schema completeness | all required D1 artifacts emitted |

## D1 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| BL041-FX-501 | lane_d1_contract_longrun_drift | contract-only 75-run replay drift | deterministic_replay_failure | yes | contract_runs_contract/replay_hashes.tsv |
| BL041-FX-502 | lane_d1_execute_longrun_drift | execute-suite 75-run replay drift | deterministic_replay_failure | yes | contract_runs_execute/replay_hashes.tsv |
| BL041-FX-503 | lane_d1_mode_parity_failure | contract/execute parity mismatch | deterministic_replay_failure | yes | mode_parity.tsv |
| BL041-FX-504 | lane_d1_taxonomy_drift | non-zero/unstable taxonomy rows | deterministic_replay_failure | yes | soak_summary.tsv |
| BL041-FX-505 | lane_d1_exit_semantics_failure | negative probe exit code != 2 | deterministic_contract_failure | yes | exit_semantics_probe.tsv |
| BL041-FX-506 | lane_d1_docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes | docs_freshness.log |
| BL041-FX-507 | lane_d1_evidence_schema_incomplete | required evidence rows/files missing | deterministic_evidence_failure | yes | status.tsv |

## D1 Validation

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl041_slice_d1_done_candidate_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 75 --out-dir TestEvidence/bl041_slice_d1_done_candidate_<timestamp>/contract_runs_execute`
- Negative probe: `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## D1 Evidence Contract

Required files under `TestEvidence/bl041_slice_d1_done_candidate_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `soak_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## D2 Acceptance Matrix (Done Promotion Mode-Parity)

| acceptance_id | gate | threshold |
|---|---|---|
| BL041-D2-001 | Contract-only done-promotion sentinel | `--contract-only --runs 100` with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-D2-002 | Execute-suite done-promotion sentinel | `--execute-suite --runs 100` with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-D2-003 | Cross-mode parity at D2 depth | `cross_mode_signature_mismatch_count=0` and `cross_mode_row_mismatch_count=0` |
| BL041-D2-004 | Taxonomy stability at D2 depth | contract/execute `taxonomy_nonzero_rows=0` |
| BL041-D2-005 | Exit semantics guard | `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` exits `2` |
| BL041-D2-006 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |
| BL041-D2-007 | Evidence schema completeness | all required D2 artifacts emitted |

## D2 Taxonomy

| failure_id | category | trigger | classification | blocking | expected_artifact |
|---|---|---|---|---|---|
| BL041-FX-601 | lane_d2_contract_longrun_drift | contract-only 100-run replay drift | deterministic_replay_failure | yes | contract_runs_contract/replay_hashes.tsv |
| BL041-FX-602 | lane_d2_execute_longrun_drift | execute-suite 100-run replay drift | deterministic_replay_failure | yes | contract_runs_execute/replay_hashes.tsv |
| BL041-FX-603 | lane_d2_mode_parity_failure | contract/execute parity mismatch | deterministic_replay_failure | yes | mode_parity.tsv |
| BL041-FX-604 | lane_d2_taxonomy_drift | non-zero/unstable taxonomy rows | deterministic_replay_failure | yes | soak_summary.tsv |
| BL041-FX-605 | lane_d2_exit_semantics_failure | negative probe exit code != 2 | deterministic_contract_failure | yes | exit_semantics_probe.tsv |
| BL041-FX-606 | lane_d2_docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes | docs_freshness.log |
| BL041-FX-607 | lane_d2_evidence_schema_incomplete | required evidence rows/files missing | deterministic_evidence_failure | yes | status.tsv |

## D2 Validation

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl041_slice_d2_done_promotion_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 100 --out-dir TestEvidence/bl041_slice_d2_done_promotion_<timestamp>/contract_runs_execute`
- Negative probe: `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## D2 Evidence Contract

Required files under `TestEvidence/bl041_slice_d2_done_promotion_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `soak_summary.tsv`
- `exit_semantics_probe.tsv`
- `promotion_readiness.md`
- `docs_freshness.log`

Validation status labels:
- `tested` = command executed and expected exit observed.
- `partially tested` = command executed with warnings or incomplete artifacts.
- `not tested` = command not executed.

## C2 Execution Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl041_slice_c2_soak_20260227T014141Z/`
- Validation results:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 10 --out-dir .../contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Determinism results:
  - `runs_observed=10`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`

## C3 Execution Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl041_slice_c3_replay_mode_parity_20260227T015445Z/`
- Validation results:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (out-of-scope metadata issue)
- Mode parity and sentinel results:
  - `contract_runs_observed=20`
  - `execute_runs_observed=20`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=FAIL`

## C3b Recheck Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl041_slice_c3b_replay_mode_parity_20260227T025246Z/`
- Validation results:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (out-of-scope metadata issues)
- Mode parity and sentinel results:
  - `contract_runs_observed=20`
  - `execute_runs_observed=20`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=FAIL`

## C3c Recheck Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl041_slice_c3c_replay_mode_parity_20260227T031142Z/`
- Validation results:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Mode parity and sentinel results:
  - `contract_runs_observed=20`
  - `execute_runs_observed=20`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=PASS`

## C4 Long-Run Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl041_slice_c4_longrun_mode_parity_20260227T033844Z/`
- Validation results:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 50 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 50 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Long-run parity and sentinel results:
  - `contract_runs_observed=50`
  - `execute_runs_observed=50`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=PASS`

## D1 Done-Candidate Long-Run Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl041_slice_d1_done_candidate_20260227T183530Z/`
- Validation results:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 75 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 75 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Done-candidate parity and sentinel results:
  - `contract_runs_observed=75`
  - `execute_runs_observed=75`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=PASS`

## D2 Done Promotion Mode-Parity Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl041_slice_d2_done_promotion_20260227T201910Z/`
- Validation results:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 100 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 100 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Done-promotion parity and sentinel results:
  - `contract_runs_observed=100`
  - `execute_runs_observed=100`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=PASS`
