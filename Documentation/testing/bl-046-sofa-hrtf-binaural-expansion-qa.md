Title: BL-046 SOFA HRTF and Binaural Expansion QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-046 SOFA HRTF and Binaural Expansion QA Contract

## Purpose

Define deterministic QA checks and evidence schema for BL-046 Slice A1 contract authority covering SOFA ingest, HRTF selection, fallback behavior, and replay determinism.

## Linked Contract

Primary runbook authority:
- `Documentation/backlog/done/bl-046-sofa-hrtf-binaural-expansion.md`

Traceability anchors:
- `.ideas/parameter-spec.md`
- `.ideas/architecture.md`
- `Documentation/invariants.md`

## Deterministic QA Checks

### SOFA Ingest Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL046-QA-001` | SOFA field coverage | required ingest fields and fallback values declared |
| `BL046-QA-002` | SOFA bounds/finite coverage | finite-only and numeric bounds explicit for sample rate, IR length, source count, ear count |
| `BL046-QA-003` | SOFA ingest order | canonical ingest sequence is explicit and deterministic |

### HRTF Selection and Fallback Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL046-QA-004` | Selection precedence contract | precedence tiers declared with deterministic ordering |
| `BL046-QA-005` | Selection tie-break contract | lexicographic tie-break rule explicit |
| `BL046-QA-006` | Fallback token completeness | required fallback reason tokens declared and mapped |
| `BL046-QA-007` | Fail-closed policy | invalid SOFA/profile paths deterministically route to safe fallback |

### Replay and Evidence Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| `BL046-QA-008` | Replay hash contract | required deterministic hash input set defined |
| `BL046-QA-009` | Replay trace contract | required trace outputs defined and deterministic equality rule explicit |
| `BL046-QA-010` | Artifact schema contract | required files and TSV column contracts fully declared |

## Acceptance Matrix Contract (Slice A1)

Required acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| `BL046-A1-001` | SOFA ingest contract completeness | required ingest fields/ranges/order all explicit |
| `BL046-A1-002` | HRTF selection precedence defined | precedence tiers and deterministic tie-break explicit |
| `BL046-A1-003` | Fallback policy completeness | required fallback tokens and fail-closed mapping explicit |
| `BL046-A1-004` | Finite/range constraints defined | finite-only and bounds contract explicit for ingest payload |
| `BL046-A1-005` | Replay determinism contract completeness | required hash inputs and deterministic traces declared |
| `BL046-A1-006` | Failure taxonomy completeness | required BL046-FX IDs present |
| `BL046-A1-007` | Artifact schema completeness | required A1 artifacts and TSV columns declared |
| `BL046-A1-008` | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exit code `0` |

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
| `BL046-FX-001` | sofa_contract_incomplete | required SOFA ingest field/range/order missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL046-FX-002` | sofa_convention_invalid | SOFA convention not supported | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL046-FX-003` | sofa_dimension_invalid | ear/source/IR dimensions outside contract | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL046-FX-004` | sofa_non_finite_ir | non-finite IR sample detected | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL046-FX-005` | sofa_digest_mismatch | digest mismatch for requested profile | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL046-FX-006` | hrtf_selection_nondeterministic | same inputs resolve different profile | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL046-FX-007` | fallback_policy_incomplete | fallback token set or mapping incomplete | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL046-FX-008` | replay_identity_incomplete | required replay hash inputs missing | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL046-FX-009` | replay_trace_divergence | identical inputs produce divergent traces | deterministic_replay_failure | yes | critical | acceptance_matrix.tsv |
| `BL046-FX-010` | artifact_schema_incomplete | required artifacts or TSV columns missing | deterministic_evidence_failure | yes | major | status.tsv |
| `BL046-FX-011` | docs_freshness_gate_failure | docs freshness script exits non-zero | governance_failure | yes | major | docs_freshness.log |

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
- `TestEvidence/bl046_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `sofa_binaural_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Validation

- `./scripts/validate-docs-freshness.sh`

## Triage Sequence

1. Resolve acceptance-ID parity drift first.
2. Resolve SOFA ingest/selection/fallback contract gaps second.
3. Resolve artifact schema gaps before closeout.
4. Resolve docs freshness failures before marking PASS.

## Slice A1 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl046_slice_a1_contract_20260227T203920Z/status.tsv`
  - `sofa_binaural_contract.md`
  - `acceptance_matrix.tsv`
  - `failure_taxonomy.tsv`
  - `docs_freshness.log`
- Validation:
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## B1 Lane Bootstrap Contract

### B1 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL046-B1-001` | `BL046-B1-001_scenario_schema` | scenario id is `locusq_bl046_sofa_binaural_suite` |
| `BL046-B1-002` | `BL046-B1-002_acceptance_alignment` | IDs `BL046-B1-001..008` all present in scenario contract |
| `BL046-B1-003` | `BL046-B1-003_hash_input_contract` | deterministic hash include fields complete |
| `BL046-B1-004` | `BL046-B1-004_fallback_contract` | required fallback tokens complete |
| `BL046-B1-005` | `BL046-B1-005_artifact_schema_complete` | artifact schema includes status/validation/replay/taxonomy TSVs |
| `BL046-B1-006` | `BL046-B1-006_runbook_alignment` | runbook contains B1 validation + evidence contract and lane references |
| `BL046-B1-007` | `BL046-B1-007_qa_alignment` | QA runbook contains B1 validation + evidence contract and lane references |
| `BL046-B1-008` | `BL046-B1-008_mode_semantics` | script exposes `--contract-only`/`--execute-suite` and strict `0/1/2` exits |

### B1 Validation

```bash
bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl046_slice_b1_lane_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

### B1 Evidence Contract

Required files under `TestEvidence/bl046_slice_b1_lane_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## B1 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl046_slice_b1_lane_20260227T204705Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl046_slice_b1_lane_20260227T204705Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## C2 Determinism Soak Contract

### C2 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL046-C2-001` | `BL046-C2-001_soak_contract_thresholds` | `--runs 10` with `max_signature_divergence=0` and `max_row_drift=0` |
| `BL046-C2-002` | `BL046-C2-002_acceptance_alignment` | IDs `BL046-C2-001..003` present across scenario/runbook/QA |
| `BL046-C2-003` | `BL046-C2-003_evidence_schema` | required C2 artifact schema present |

### C2 Validation

```bash
bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl046_slice_c2_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

### C2 Evidence Contract

Required files under `TestEvidence/bl046_slice_c2_soak_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C3 Replay Sentinel Contract

### C3 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL046-C3-001` | `BL046-C3-001_replay_sentinel_thresholds` | `--runs 20` with `max_signature_divergence=0` and `max_row_drift=0` |
| `BL046-C3-002` | `BL046-C3-002_acceptance_alignment` | IDs `BL046-C3-001..003` present across scenario/runbook/QA |
| `BL046-C3-003` | `BL046-C3-003_evidence_schema` | required C3 artifact schema present |

### C3 Validation

```bash
bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl046_slice_c3_replay_sentinel_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

### C3 Evidence Contract

Required files under `TestEvidence/bl046_slice_c3_replay_sentinel_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C4 Long-Run Replay Sentinel + Mode Parity Contract

### C4 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL046-C4-001` | `BL046-C4-001_longrun_thresholds` | `--runs 50` with `max_signature_divergence=0` and `max_row_drift=0` |
| `BL046-C4-002` | `BL046-C4-002_mode_parity_and_usage_guards` | contract-only and execute-suite parity rows match, usage guards return exit `2` |
| `BL046-C4-003` | `BL046-C4-003_evidence_schema` | required C4 artifact schema present |

### C4 Validation

```bash
bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl046_slice_c4_longrun_mode_parity_<timestamp>/contract_runs_contract
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl046_slice_c4_longrun_mode_parity_<timestamp>/contract_runs_execute
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --runs 0
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

### C4 Evidence Contract

Required files under `TestEvidence/bl046_slice_c4_longrun_mode_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `soak_summary.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## D1 Done-Candidate Long-Run Contract

### D1 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL046-D1-001` | `BL046-D1-001_done_candidate_thresholds` | `--runs 75` with `max_signature_divergence=0` and `max_row_drift=0` |
| `BL046-D1-002` | `BL046-D1-002_mode_parity_and_usage_guards` | contract-only and execute-suite parity rows match, usage guard returns exit `2` |
| `BL046-D1-003` | `BL046-D1-003_evidence_schema` | required D1 artifact schema present |

### D1 Validation

```bash
bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl046_slice_d1_done_candidate_<timestamp>/contract_runs_contract
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --execute-suite --runs 75 --out-dir TestEvidence/bl046_slice_d1_done_candidate_<timestamp>/contract_runs_execute
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --runs 0
./scripts/validate-docs-freshness.sh
```

### D1 Evidence Contract

Required files under `TestEvidence/bl046_slice_d1_done_candidate_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `soak_summary.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## D2 Done-Promotion Contract

### D2 Acceptance Mapping

| Acceptance ID | Lane Check | Threshold |
|---|---|---|
| `BL046-D2-001` | `BL046-D2-001_done_promotion_thresholds` | `--runs 100` with `max_signature_divergence=0` and `max_row_drift=0` |
| `BL046-D2-002` | `BL046-D2-002_mode_parity_and_usage_guards` | contract-only and execute-suite parity rows match, usage guard returns exit `2` |
| `BL046-D2-003` | `BL046-D2-003_evidence_schema` | required D2 artifact schema present including `promotion_readiness.md` |

### D2 Validation

```bash
bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl046_slice_d2_done_promotion_<timestamp>/contract_runs_contract
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --execute-suite --runs 100 --out-dir TestEvidence/bl046_slice_d2_done_promotion_<timestamp>/contract_runs_execute
./scripts/qa-bl046-sofa-binaural-lane-mac.sh --runs 0
./scripts/validate-docs-freshness.sh
```

### D2 Evidence Contract

Required files under `TestEvidence/bl046_slice_d2_done_promotion_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `soak_summary.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `promotion_readiness.md`
- `lane_notes.md`
- `docs_freshness.log`

## C2 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl046_slice_c2_soak_20260227T215044Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `soak_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl046_slice_c2_soak_20260227T215044Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## C3 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl046_slice_c3_replay_sentinel_20260227T220511Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl046_slice_c3_replay_sentinel_20260227T220511Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## C4 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl046_slice_c4_longrun_mode_parity_20260227T223236Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `soak_summary.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl046_slice_c4_longrun_mode_parity_20260227T223236Z/contract_runs_contract` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl046_slice_c4_longrun_mode_parity_20260227T223236Z/contract_runs_execute` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --runs 0` => `PASS (exit 2 expected)`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --bad-flag` => `PASS (exit 2 expected)`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## D1 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl046_slice_d1_done_candidate_20260227T222910Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `soak_summary.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl046_slice_d1_done_candidate_20260227T222910Z/contract_runs_contract` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --execute-suite --runs 75 --out-dir TestEvidence/bl046_slice_d1_done_candidate_20260227T222910Z/contract_runs_execute` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --runs 0` => `PASS (exit 2 expected)`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## D2 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl046_slice_d2_done_promotion_20260227T222959Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `soak_summary.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `promotion_readiness.md`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl046-sofa-binaural-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl046_slice_d2_done_promotion_20260227T222959Z/contract_runs_contract` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --execute-suite --runs 100 --out-dir TestEvidence/bl046_slice_d2_done_promotion_20260227T222959Z/contract_runs_execute` => `PASS`
  - `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --runs 0` => `PASS (exit 2 expected)`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## Owner-Ready Verification Z16 Snapshot (2026-02-27)

- Input packets:
  - `TestEvidence/bl046_e2e_owner_ready_20260227T223310Z/*`
  - `TestEvidence/bl046_slice_d2_done_promotion_20260227T222959Z/*`
- Z16 output packet:
  - `TestEvidence/bl046_owner_ready_z16_20260227T225448Z/status.tsv`
  - `validation_matrix.tsv`
  - `owner_ready_decision.md`
  - `blocker_taxonomy.tsv`
  - `handoff_index.tsv`
  - `docs_freshness.log`
- Verification outcomes:
  - D2 evidence completeness contract: `PASS` (`missing_count=0`)
  - D2 lane aggregate status: `PASS`
  - Owner-ready recommendation token: `Done-candidate`
  - Docs freshness gate: `PASS`
- QA disposition:
  - Owner-ready bundle is normalized to current branch head with zero blockers for owner intake.
