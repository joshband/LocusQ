Title: BL-047 Spatial Coordinate Contract QA
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-047 Spatial Coordinate Contract QA

## Purpose

Define deterministic QA acceptance mapping for BL-047 Slice A1 so coordinate frame authority, conversion invariants, normalization/clamping, and cross-mode parity evidence remain machine-checkable.

## Linked Contracts

- Runbook: `Documentation/backlog/done/bl-047-spatial-coordinate-contract.md`
- Invariants: `Documentation/invariants.md`
- ADR freshness gate: `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- Artifact retention policy: `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`

## Acceptance ID Catalog (Slice A1)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL047-A1-001` | Coordinate frame authority table defines canonical token and owner surface | runbook + QA parity |
| `BL047-A1-002` | Axis/sign and azimuth direction semantics are explicit | runbook + QA parity |
| `BL047-A1-003` | Conversion invariants list with deterministic thresholds is explicit | runbook + QA parity |
| `BL047-A1-004` | Normalization rules require finite vectors and unit-length tolerance | runbook + QA parity |
| `BL047-A1-005` | Clamping rules for azimuth/elevation/distance are explicit | runbook + QA parity |
| `BL047-A1-006` | NaN/Inf handling contract is explicit and blocking | runbook + QA parity |
| `BL047-A1-007` | Cross-mode parity evidence schema is explicit | runbook + QA parity |
| `BL047-A1-008` | Deterministic A1 artifact schema and TSV columns are explicit | runbook + QA parity |
| `BL047-A1-009` | Acceptance IDs match runbook + QA + acceptance_matrix.tsv | cross-surface parity |
| `BL047-A1-010` | Docs freshness gate passes | `docs_freshness.log` |

## Deterministic Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL047-QA-001` | Coordinate authority completeness | canonical frame tokens + owner surfaces declared |
| `BL047-QA-002` | Axis/sign semantic completeness | +X/+Y/+Z semantics and azimuth direction declared |
| `BL047-QA-003` | Conversion invariant completeness | 4 invariants and thresholds present |
| `BL047-QA-004` | Normalization completeness | finite and unit-norm tolerance requirements present |
| `BL047-QA-005` | Clamp completeness | azimuth/elevation/distance domains present |
| `BL047-QA-006` | Cross-mode parity schema completeness | parity definition + required columns present |
| `BL047-QA-007` | Acceptance parity coherence | A1 acceptance IDs match across runbook/QA/evidence |

## Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity |
|---|---|---|---|---|---|
| `BL047-A1-FX-001` | coordinate_frame_authority_missing | canonical frame token/ownership mapping absent | deterministic_contract_failure | yes | critical |
| `BL047-A1-FX-002` | axis_sign_semantics_drift | axis/sign or azimuth-direction semantics missing/mismatched | deterministic_contract_failure | yes | critical |
| `BL047-A1-FX-003` | conversion_invariant_missing | required conversion invariant or threshold absent | deterministic_contract_failure | yes | major |
| `BL047-A1-FX-004` | normalization_rule_missing | unit-vector or finite-value rule absent | deterministic_contract_failure | yes | major |
| `BL047-A1-FX-005` | clamping_rule_missing | angle/distance clamp rules absent | deterministic_contract_failure | yes | major |
| `BL047-A1-FX-006` | non_finite_classification_missing | NaN/Inf reject class absent | deterministic_contract_failure | yes | major |
| `BL047-A1-FX-007` | cross_mode_parity_schema_missing | mode parity schema/columns absent | deterministic_contract_failure | yes | major |
| `BL047-A1-FX-008` | artifact_schema_incomplete | required evidence files or TSV headers missing | deterministic_evidence_failure | yes | major |
| `BL047-A1-FX-009` | acceptance_parity_drift | acceptance IDs mismatch across runbook/QA/evidence | deterministic_contract_failure | yes | major |
| `BL047-A1-FX-010` | docs_freshness_failure | docs freshness command returns non-zero | deterministic_evidence_failure | yes | major |

## Artifact Schema Contract (Slice A1)

Required evidence bundle path:
- `TestEvidence/bl047_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `spatial_coordinate_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

Required TSV schemas:
- `status.tsv`: `artifact`, `value`, `note`
- `acceptance_matrix.tsv`: `acceptance_id`, `gate`, `threshold`, `result`, `artifact`, `note`
- `failure_taxonomy.tsv`: `failure_id`, `category`, `trigger`, `classification`, `blocking`, `severity`

## Cross-Mode Parity Schema Contract (A1 declaration)

This slice is docs-only; parity execution is deferred to executable slices.
A1 still requires explicit schema declaration:
- required file token: `mode_parity.tsv`
- required columns: `check_id`, `contract_value`, `execute_value`, `expectation`, `result`, `detail`
- required aggregate result row token: `BL047-<slice>-PAR-RESULT`

## A1 Validation Commands

```bash
./scripts/validate-docs-freshness.sh
```

Pass criteria:
- command exit code `0`
- acceptance/failure taxonomy parity across runbook and QA docs
- all required A1 artifacts present in the evidence packet

## Triage Sequence

1. Resolve acceptance-ID parity drift first.
2. Resolve missing conversion/normalization/clamping contract definitions.
3. Resolve missing artifact schema fields before closeout.
4. Resolve docs freshness failure before declaring PASS.

## B1 Executable Lane Contract

Canonical lane script:
- `scripts/qa-bl047-coordinate-contract-lane-mac.sh`

Canonical scenario:
- `qa/scenarios/locusq_bl047_coordinate_contract_suite.json`

### B1 Acceptance Mapping

| Acceptance ID | Lane Check | Contract |
|---|---|---|
| `BL047-B1-001` | `BL047-B1-001_scenario_contract` | scenario contract file exists and declares B1 acceptance IDs |
| `BL047-B1-002` | `BL047-B1-002_coordinate_authority_contract` | coordinate frame authority clauses declared in runbook |
| `BL047-B1-003` | `BL047-B1-003_conversion_norm_parity_contract` | conversion/normalization/parity clauses declared across runbook+QA |
| `BL047-B1-004` | `BL047-B1-004_acceptance_parity` | acceptance ID parity across runbook/QA and replay runs |
| `BL047-B1-005` | `BL047-B1-005_failure_taxonomy_parity` | failure taxonomy parity across runbook/QA and replay runs |
| `BL047-B1-006` | `BL047-B1-006_artifact_schema_contract` | required lane artifacts are declared |
| `BL047-B1-007` | `BL047-B1-007_execution_mode_contract` | mode semantics preserved for `contract_only` and `execute_suite` |

### B1 Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity |
|---|---|---|---|---|---|
| `BL047-B1-FX-001` | scenario_contract_missing | suite contract file missing or malformed | deterministic_contract_failure | yes | major |
| `BL047-B1-FX-002` | coordinate_authority_clause_missing | coordinate authority clauses missing from runbook | deterministic_contract_failure | yes | major |
| `BL047-B1-FX-003` | conversion_norm_parity_clause_missing | conversion/normalization/parity clauses missing | deterministic_contract_failure | yes | major |
| `BL047-B1-FX-004` | acceptance_parity_mismatch | runbook/QA acceptance ID sets diverge | deterministic_contract_failure | yes | major |
| `BL047-B1-FX-005` | failure_taxonomy_parity_mismatch | runbook/QA taxonomy ID sets diverge | deterministic_contract_failure | yes | major |
| `BL047-B1-FX-006` | artifact_schema_clause_missing | required lane artifact declarations missing | deterministic_contract_failure | yes | major |
| `BL047-B1-FX-007` | execution_mode_invalid | mode outside `{contract_only,execute_suite}` | deterministic_contract_failure | yes | major |
| `BL047-B1-FX-008` | replay_hash_divergence | replay combined signature diverges across runs | deterministic_replay_divergence | yes | critical |
| `BL047-B1-FX-009` | replay_row_drift | replay row-signature drifts across runs | deterministic_replay_row_drift | yes | critical |
| `BL047-B1-FX-010` | required_artifact_missing | required lane output artifact absent | missing_result_artifact | yes | major |

### B1 Validation Matrix

```bash
bash -n scripts/qa-bl047-coordinate-contract-lane-mac.sh
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --help
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl047_slice_b1_lane_<timestamp>/contract_runs
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

| Acceptance ID | Lane Check | Contract |
|---|---|---|
| `BL047-C2-001` | `BL047-C2-001_determinism_soak_threshold` | `--runs 10` with `max_signature_divergence=0` and `max_row_drift=0` |
| `BL047-C2-002` | `BL047-C2-002_acceptance_alignment` | scenario/runbook/QA acceptance surfaces remain aligned and deterministic |
| `BL047-C2-003` | `BL047-C2-003_evidence_schema` | required C2 artifact schema is present and machine-readable |

### C2 Validation

```bash
bash -n scripts/qa-bl047-coordinate-contract-lane-mac.sh
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --help
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl047_slice_c2_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

### C2 Evidence Contract

Required files under `TestEvidence/bl047_slice_c2_soak_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C3/C4/C5/D1/D2 Done-Promotion Contract

### Acceptance Mapping

| Acceptance ID | Lane Check | Contract |
|---|---|---|
| `BL047-C3-001` | `BL047-C3-001_replay_sentinel` | `--contract-only --runs 20` with zero signature divergence and zero row drift |
| `BL047-C4-001` | `BL047-C4-001_replay_sentinel_soak` | `--contract-only --runs 50` with zero signature divergence and zero row drift |
| `BL047-C5-001` | `BL047-C5-001_mode_guard_threshold` | mode guard threshold is satisfied at `runs >= 20` |
| `BL047-C5-002` | `BL047-C5-002_execute_mode_alias_contract` | `execute_suite` preserves deterministic contract behavior |
| `BL047-D1-001` | `BL047-D1-001_done_candidate_longrun` | done-candidate long-run threshold is satisfied at `runs >= 75` |
| `BL047-D2-001` | `BL047-D2-001_done_promotion_longrun` | done-promotion long-run threshold is satisfied at `runs >= 100` for contract+execute mode parity |

### Validation Matrix (Done-Promotion)

```bash
bash -n scripts/qa-bl047-coordinate-contract-lane-mac.sh
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --help
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl047_e2e_done_promotion_<timestamp>/c3_contract_runs
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl047_e2e_done_promotion_<timestamp>/c4_contract_runs
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl047_e2e_done_promotion_<timestamp>/contract_runs_contract
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --execute-suite --runs 100 --out-dir TestEvidence/bl047_e2e_done_promotion_<timestamp>/contract_runs_execute
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --runs 0
./scripts/qa-bl047-coordinate-contract-lane-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

### Evidence Contract (Done-Promotion)

Required files under `TestEvidence/bl047_e2e_done_promotion_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `c3_contract_runs/validation_matrix.tsv`
- `c3_contract_runs/replay_hashes.tsv`
- `c3_contract_runs/failure_taxonomy.tsv`
- `c4_contract_runs/validation_matrix.tsv`
- `c4_contract_runs/replay_hashes.tsv`
- `c4_contract_runs/failure_taxonomy.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `contract_runs_execute/failure_taxonomy.tsv`
- `mode_parity.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `promotion_readiness.md`
- `docs_freshness.log`

## B1 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl047_slice_a1_contract_20260227T204927Z/*`
- Evidence bundle:
  - `TestEvidence/bl047_slice_b1_lane_20260227T212806Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl047-coordinate-contract-lane-mac.sh` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl047_slice_b1_lane_20260227T212806Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## C2 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl047_slice_c2_soak_20260227T220609Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `soak_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl047-coordinate-contract-lane-mac.sh` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl047_slice_c2_soak_20260227T220609Z/contract_runs` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## D2 Done-Promotion Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl047_e2e_done_promotion_20260227T223524Z/status.tsv`
  - `validation_matrix.tsv`
  - `c3_contract_runs/validation_matrix.tsv`
  - `c3_contract_runs/replay_hashes.tsv`
  - `c3_contract_runs/failure_taxonomy.tsv`
  - `c4_contract_runs/validation_matrix.tsv`
  - `c4_contract_runs/replay_hashes.tsv`
  - `c4_contract_runs/failure_taxonomy.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `contract_runs_execute/failure_taxonomy.tsv`
  - `mode_parity.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `promotion_readiness.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl047-coordinate-contract-lane-mac.sh` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --help` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl047_e2e_done_promotion_20260227T223524Z/c3_contract_runs` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl047_e2e_done_promotion_20260227T223524Z/c4_contract_runs` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl047_e2e_done_promotion_20260227T223524Z/contract_runs_contract` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --execute-suite --runs 100 --out-dir TestEvidence/bl047_e2e_done_promotion_20260227T223524Z/contract_runs_execute` => PASS
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --runs 0` => exit 2 (PASS)
  - `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --bad-flag` => exit 2 (PASS)
  - `./scripts/validate-docs-freshness.sh` => FAIL (external blocker: BL-049 packet metadata omissions)
- Disposition:
  - Determinism and parity gates are PASS.
  - Ownership safety gate is FAIL due outside-owned paths detected in before/after status delta (`Source/PluginProcessor.cpp`, `TestEvidence/bl049_slice_c5_semantics_20260227_173245/`, `TestEvidence/bl049_slice_c5_semantics_20260227_173357/`).
  - Docs freshness is FAIL due external BL-049 evidence metadata omissions (`TestEvidence/bl049_slice_d1_done_candidate_20260227T224005Z/done_candidate_readiness.md`, `TestEvidence/bl049_slice_d1_done_candidate_20260227T224005Z/lane_notes.md`).
