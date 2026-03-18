Title: BL-048 Cross-Platform Shipping Hardening QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-048 Cross-Platform Shipping Hardening QA Contract

## Purpose

Define deterministic QA checks for BL-048 A1 cross-platform shipping-hardening contracts, including platform matrix rules, signing/notarization/packaging gate semantics, and deterministic release evidence schema.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/done/bl-048-cross-platform-shipping-hardening.md`

Traceability anchors:
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`

## A1 Deterministic QA Checks

### Platform Matrix Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL048-QA-001` | Matrix column completeness | all required matrix columns explicitly defined |
| `BL048-QA-002` | Unique active platform rows | no duplicate active `platform_id` rows |
| `BL048-QA-003` | Deterministic ordering contract | `platform_id` ASCII ordering rule explicitly defined |

### Signing and Notarization Gate Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL048-QA-004` | macOS signing gate definition | gate includes deterministic verification evidence expectation |
| `BL048-QA-005` | macOS notarization/stapling gate definition | gate includes notarization + stapling + verification evidence expectation |
| `BL048-QA-006` | Windows signing gate definition | gate includes deterministic Authenticode verification evidence expectation |

### Packaging and Release Evidence Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL048-QA-007` | Packaging manifest schema | required `packaging_manifest.tsv` columns explicitly defined |
| `BL048-QA-008` | Checksum schema | required `checksums.tsv` columns explicitly defined |
| `BL048-QA-009` | Deterministic release artifact set | required release artifacts listed and machine-parseable |

### Governance Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL048-QA-010` | Acceptance parity | `BL048-A1-001..008` present in backlog + QA + A1 evidence matrix |
| `BL048-QA-011` | Failure taxonomy parity | `BL048-FX-001..010` present in backlog + QA + evidence taxonomy |
| `BL048-QA-012` | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

## A1 Acceptance Matrix Contract

Required acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| `BL048-A1-001` | Platform matrix completeness | 100% required matrix columns/rules defined |
| `BL048-A1-002` | Deterministic matrix ordering | `platform_id` ASCII ordering and uniqueness contract defined |
| `BL048-A1-003` | macOS signing gate definition | deterministic signature verification expectations defined |
| `BL048-A1-004` | macOS notarization + stapling gate definition | deterministic notarization/stapling verification expectations defined |
| `BL048-A1-005` | Windows signing gate definition | deterministic Authenticode verification expectations defined |
| `BL048-A1-006` | Packaging + checksum gate definition | manifest/checksum schema contract defined |
| `BL048-A1-007` | Backlog/QA acceptance parity | identical A1 ID set across backlog + QA |
| `BL048-A1-008` | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exit `0` |

Required `acceptance_matrix.tsv` columns:
- `acceptance_id`
- `gate`
- `threshold`
- `measured_value`
- `result`
- `evidence_path`

## A1 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| `BL048-FX-001` | platform_matrix_missing_column | required platform matrix column absent | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL048-FX-002` | platform_matrix_duplicate_row | duplicate active `platform_id` row | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL048-FX-003` | matrix_ordering_non_deterministic | row ordering differs for unchanged input | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL048-FX-004` | macos_signing_gate_undefined | macOS signing gate rule/evidence missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL048-FX-005` | macos_notarization_gate_undefined | notarization/stapling gate rule/evidence missing | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL048-FX-006` | windows_signing_gate_undefined | Windows signing gate rule/evidence missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL048-FX-007` | packaging_checksum_schema_incomplete | manifest/checksum schema missing required fields | deterministic_evidence_failure | yes | major | acceptance_matrix.tsv |
| `BL048-FX-008` | acceptance_id_parity_failure | A1 IDs drift between backlog and QA docs | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL048-FX-009` | release_artifact_missing | required deterministic release artifact missing | deterministic_evidence_failure | yes | major | status.tsv |
| `BL048-FX-010` | docs_freshness_gate_failure | docs freshness validation non-zero exit | governance_failure | yes | major | docs_freshness.log |

## Deterministic Release Evidence Contract (Implementation Slices)

Required bundle path:
- `TestEvidence/bl048_slice_<slice>_<timestamp>/`

Required deterministic artifacts:
- `status.tsv`
- `release_gate_matrix.tsv`
- `platform_matrix.tsv`
- `signing_verification.tsv`
- `notarization_stapling.tsv`
- `packaging_manifest.tsv`
- `checksums.tsv`
- `failure_taxonomy.tsv`

Required schema columns:
- `release_gate_matrix.tsv`: `gate_id`, `gate`, `threshold`, `measured_value`, `result`, `artifact`
- `packaging_manifest.tsv`: `platform_id`, `artifact_path`, `artifact_type`, `version`, `size_bytes`, `sha256`
- `checksums.tsv`: `artifact_path`, `sha256`, `hash_algorithm`, `result`

## Slice A1 Validation

- `./scripts/validate-docs-freshness.sh`

Validation status labels:
- `tested` = command executed, expected exit code observed.
- `partially tested` = command executed with incomplete/blocked evidence.
- `not tested` = command not executed.

## Slice A1 Evidence Contract

Required path:
- `TestEvidence/bl048_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `shipping_hardening_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Slice B1 Deterministic Lane Contract

Lane script:
- `scripts/qa-bl048-shipping-hardening-lane-mac.sh`

Supported options:
- `--contract-only`
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
| `BL048-B1-001` | platform-matrix contract clause presence | required matrix schema/invariant clauses present |
| `BL048-B1-002` | signing/notarization gate clause presence | required macOS and Windows gate clauses present |
| `BL048-B1-003` | release evidence schema clause presence | required evidence schema anchor clauses present |
| `BL048-B1-004` | acceptance parity clause presence | A1 acceptance parity clauses present in QA runbook |
| `BL048-B1-005` | failure taxonomy parity clause presence | A1 failure taxonomy parity clauses present in QA runbook |
| `BL048-B1-006` | replay signature stability | `signature_drift_count=0` across `runs=3` |
| `BL048-B1-007` | replay row stability | `row_drift_count=0` across `runs=3` |
| `BL048-B1-008` | docs freshness gate | `./scripts/validate-docs-freshness.sh` exit `0` |

Required `validation_matrix.tsv` columns:
- `run`
- `check_id`
- `result`
- `detail`
- `artifact`

Required `replay_hashes.tsv` columns:
- `run`
- `signature`
- `baseline_signature`
- `signature_match`
- `row_signature`
- `baseline_row_signature`
- `row_match`

## Slice B1 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| `BL048-B1-FX-001` | contract_clause_missing | required file/pattern missing in backlog or QA docs | deterministic_contract_failure | yes | major | contract_runs/validation_matrix.tsv |
| `BL048-B1-FX-002` | replay_signature_divergence | replay signature mismatch across repeated runs | deterministic_replay_divergence | yes | critical | contract_runs/replay_hashes.tsv |
| `BL048-B1-FX-003` | replay_row_divergence | replay row-signature mismatch across repeated runs | deterministic_replay_row_drift | yes | critical | contract_runs/replay_hashes.tsv |
| `BL048-B1-FX-004` | missing_required_artifact | required lane artifact missing | deterministic_evidence_failure | yes | major | status.tsv |
| `BL048-B1-FX-005` | usage_error | invalid argument/configuration invocation | usage_error | yes | major | status.tsv |

## Slice B1 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl048_slice_b1_lane_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## Slice C2 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl048_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## Slice B1 Evidence Bundle Contract

Required path:
- `TestEvidence/bl048_slice_b1_lane_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C2 Evidence Bundle Contract

Required path:
- `TestEvidence/bl048_slice_c2_soak_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C3-C7 / D1-D2 Validation Contract

Acceptance ID continuity:
- C3->D2 retains `BL048-B1-001..008` as mandatory deterministic acceptance anchors.

Additional executable checks:
- mode parity hash checks (contract vs execute) at C6r/C7/D1/D2.
- usage/exit semantics checks with expected exit code `2` for invalid invocation.
- long-run drift summaries at C7/D1/D2.

### C3 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl048_slice_c3_replay_sentinel_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C4 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl048_slice_c4_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C5 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl048_slice_c5_exit_semantics_<timestamp>/contract_runs`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --unknown` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C6r Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl048_slice_c6r_mode_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl048_slice_c6r_mode_parity_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --unknown` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C7 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl048_slice_c7_longrun_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl048_slice_c7_longrun_parity_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### D1 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl048_slice_d1_done_candidate_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --execute-suite --runs 75 --out-dir TestEvidence/bl048_slice_d1_done_candidate_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### D2 Validation Commands

- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl048_slice_d2_done_promotion_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --execute-suite --runs 100 --out-dir TestEvidence/bl048_slice_d2_done_promotion_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## Slice C3-C7 / D1-D2 Evidence Bundle Contract

- C3/C4:
  - `status.tsv`, `validation_matrix.tsv`, `contract_runs/validation_matrix.tsv`, `contract_runs/replay_hashes.tsv`, `contract_runs/failure_taxonomy.tsv`, `replay_sentinel_summary.tsv`, `lane_notes.md`, `docs_freshness.log`.
- C5:
  - C3/C4 bundle plus `exit_semantics_probe.tsv`.
- C6r:
  - `status.tsv`, `validation_matrix.tsv`,
  - `contract_runs_contract/validation_matrix.tsv`, `contract_runs_contract/replay_hashes.tsv`, `contract_runs_contract/failure_taxonomy.tsv`,
  - `contract_runs_execute/validation_matrix.tsv`, `contract_runs_execute/replay_hashes.tsv`,
  - `mode_parity.tsv`, `replay_sentinel_summary.tsv`, `exit_semantics_probe.tsv`, `lane_notes.md`, `docs_freshness.log`.
- C7/D1:
  - `status.tsv`, `validation_matrix.tsv`,
  - `contract_runs_contract/validation_matrix.tsv`, `contract_runs_contract/replay_hashes.tsv`, `contract_runs_contract/failure_taxonomy.tsv`,
  - `contract_runs_execute/validation_matrix.tsv`, `contract_runs_execute/replay_hashes.tsv`,
  - `mode_parity.tsv`, `drift_summary.tsv`, `exit_semantics_probe.tsv`, `lane_notes.md`, `docs_freshness.log`.
- D2:
  - D1 bundle plus `promotion_readiness.md`.

## Slice B1 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl048_slice_a1_contract_20260227T203929Z/*`
- Evidence bundle:
  - `TestEvidence/bl048_slice_b1_lane_20260227T212253Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh` => PASS
  - `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl048_slice_b1_lane_20260227T212253Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS
- Determinism summary:
  - `runs_observed=3`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`

## Slice C2 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl048_slice_c2_soak_20260227T220243Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh` => PASS
  - `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl048_slice_c2_soak_20260227T220243Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS
- Determinism summary:
  - `runs_observed=10`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`

## Slice C3-C7 / D1-D2 Execution Snapshots (2026-02-27)

- C3 replay sentinel:
  - `TestEvidence/bl048_slice_c3_replay_sentinel_20260227T222616Z/*`
  - Validation PASS; replay sentinel summary PASS.
- C4 replay soak:
  - `TestEvidence/bl048_slice_c4_soak_20260227T222617Z/*`
  - Validation PASS; replay sentinel summary PASS.
- C5 exit-semantics guard:
  - `TestEvidence/bl048_slice_c5_exit_semantics_20260227T222618Z/*`
  - Validation PASS; `--runs 0` and `--unknown` both exit `2`.
- C6r execute-mode parity:
  - `TestEvidence/bl048_slice_c6r_mode_parity_20260227T222619Z/*`
  - Contract + execute validation PASS; mode parity PASS; exit probes PASS.
- C7 long-run parity sentinel:
  - `TestEvidence/bl048_slice_c7_longrun_parity_20260227T222620Z/*`
  - Contract + execute validation PASS; mode parity PASS; drift summary PASS.
- D1 done-candidate parity:
  - `TestEvidence/bl048_slice_d1_done_candidate_20260227T222621Z/*`
  - Contract + execute validation PASS; mode parity PASS; drift summary PASS.
- D2 done-promotion parity:
  - `TestEvidence/bl048_slice_d2_done_promotion_20260227T222622Z/*`
  - Contract + execute validation PASS; mode parity PASS; drift summary PASS; promotion readiness documented; docs freshness PASS.

## Slice Z16b Ownership-Safety Reconcile Validation Snapshot (2026-02-27)

Reconcile objective:
- Produce a single ownership-safe E2E packet from C3/C4/C5/C6r/C7/D1/D2 with deterministic parity, deterministic usage exits, and explicit done-promotion readiness recommendation.

Validation commands and outcomes:
- `bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh` => PASS
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help` => PASS
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl048_e2e_promotion_z16b_20260227T225426Z/contract_runs_contract` => PASS
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --execute-suite --runs 100 --out-dir TestEvidence/bl048_e2e_promotion_z16b_20260227T225426Z/contract_runs_execute` => PASS
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --runs 0` => expected exit `2` observed
- `./scripts/qa-bl048-shipping-hardening-lane-mac.sh --unknown` => expected exit `2` observed
- `./scripts/validate-docs-freshness.sh` => PASS

Evidence bundle:
- `TestEvidence/bl048_e2e_promotion_z16b_20260227T225426Z/status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `drift_summary.tsv`
- `exit_semantics_probe.tsv`
- `promotion_readiness.md`
- `ownership_safety_check.tsv`
- `blocker_taxonomy.tsv`
- `docs_freshness.log`

Z16b contract verdict:
- `RESULT=PASS`
- `promotion_recommendation=Done-candidate`
- `blockers=none`
- `shared_files_touched=no`
