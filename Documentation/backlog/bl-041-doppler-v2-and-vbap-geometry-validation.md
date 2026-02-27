Title: BL-041 Doppler v2 and VBAP Geometry Validation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-27

# BL-041 Doppler v2 and VBAP Geometry Validation

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-041 |
| Priority | P2 |
| Status | In Implementation (Owner Z7 intake accepted H2 + C3c packet; C4 long-run replay sentinel and mode parity packet now green at 50-run depth) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-036 |
| Blocks | — |
| Slice A1 Type | Docs only |

## Objective

Define a deterministic contract for Doppler v2 and VBAP geometry validity so interpolation behavior, boundary continuity, and failure handling are replay-stable and machine-auditable before runtime implementation.

## Slice A1 Contract Authority

Slice A1 is documentation-only and defines the normative contract for later implementation and QA lane slices.

### 1) Doppler v2 Signal Contract

Deterministic Doppler fields and bounds:

| Field | Type | Rule | Fallback |
|---|---|---|---|
| `relative_velocity_mps` | float | finite-only; clamped to `[-200.0, 200.0]` | `0.0` |
| `distance_m` | float | finite-only; clamped to `[0.1, 1000.0]` | `1.0` |
| `doppler_ratio` | float | finite-only; clamped to `[0.25, 4.0]` | `1.0` |
| `delay_samples` | float | finite-only; clamped to `[0.0, max_delay_samples]` | `0.0` |
| `interp_mode` | enum | `cubic_hermite` for v2, explicit v1 legacy `linear` accepted only in compatibility mode | `linear` (compatibility mode only) |

Normative per-block order:
1. Validate finite-only inputs (`relative_velocity_mps`, `distance_m`, timing terms).
2. Clamp to contract bounds.
3. Compute target `doppler_ratio` and `delay_samples`.
4. Apply deterministic smoothing constraints.
5. Perform interpolation with contract-selected mode.

### 2) Doppler Smoothing and Continuity Contract

| Metric | Threshold | Window |
|---|---|---|
| `ratio_delta_per_block_abs_max` | `<=0.02` | per block |
| `delay_delta_samples_abs_max` | `<=1.5` | per block |
| `continuity_pitch_step_cents_abs_max` | `<=10` | 256-block window |
| `continuity_energy_jump_db_abs_max` | `<=1.0` | 256-block window |

Deterministic rules:
- Smoothing must be monotonic toward target when target is constant.
- Same input sequence must yield identical smoothed `doppler_ratio` and `delay_samples` traces.
- Non-finite intermediate smoothing state is treated as contract failure and fail-closed to fallback.

### 3) VBAP Geometry Validity Contract

| Field | Type | Rule | Fallback |
|---|---|---|---|
| `speaker_triplet_id` | string | deterministic sorted identifier for active triplet | `none` |
| `triplet_area` | float | finite-only; must be `> area_epsilon` | `0.0` |
| `gain_vector` | vec3 | finite-only; each gain clamped `[0.0, 1.0]` | `[0,0,0]` |
| `gain_sum` | float | normalized to `1.0 +/- 1e-4` | `1.0` by normalized fallback |
| `inside_simplex` | bool | deterministic barycentric validity flag | `false` |

Geometry thresholds:
- `area_epsilon = 1.0e-6`
- `boundary_crossfade_deg = 2.0`
- `boundary_gain_jump_abs_max = 0.05` across adjacent triplet transition

Deterministic rules:
- Triplet selection tie-break is deterministic by sorted triplet identifier.
- Boundary transitions must crossfade with fixed window and no randomization.
- Degenerate triplets are rejected deterministically and mapped to taxonomy.

### 4) Degradation and Fallback Policy

Required deterministic fallback reason tokens:
- `none`
- `doppler_non_finite_input`
- `doppler_non_finite_state`
- `doppler_ratio_out_of_bounds`
- `vbap_degenerate_triplet`
- `vbap_invalid_gain_vector`
- `vbap_boundary_discontinuity`
- `geometry_payload_incomplete`

Fallback policy:
1. Doppler invalid state -> use neutral ratio `1.0` and preserve timing continuity via clamped delay.
2. VBAP invalid geometry -> deterministic nearest-valid triplet fallback; if unavailable, deterministic stereo-safe collapse.
3. Any fallback must emit taxonomy token and remain replay-stable for identical inputs.

### 5) Deterministic Replay Contract

Required hash inputs:
- `schema_version`
- `sample_rate`
- `block_size`
- `interp_mode`
- `smoothing_thresholds`
- `geometry_thresholds`
- `speaker_layout_signature`
- `velocity_distance_sequence_signature`

Determinism requirement:
- Identical hash inputs must produce identical sequence outputs for:
  - `doppler_ratio_trace`
  - `delay_samples_trace`
  - `speaker_triplet_trace`
  - `fallback_reason_trace`

### 6) Acceptance IDs and Measurable Pass/Fail Thresholds

| Acceptance ID | Gate | Pass Threshold |
|---|---|---|
| BL041-A1-001 | Doppler input/bounds contract completeness | required fields, ranges, and fallback clauses all explicit |
| BL041-A1-002 | Doppler smoothing thresholds defined | all smoothing/continuity thresholds declared with numeric limits |
| BL041-A1-003 | VBAP geometry validity thresholds defined | area/boundary/gain thresholds declared with deterministic rules |
| BL041-A1-004 | Deterministic tie-break and transition policy | triplet tie-break + boundary crossfade rules explicit |
| BL041-A1-005 | Replay contract completeness | required hash inputs + deterministic equality requirement explicit |
| BL041-A1-006 | Failure taxonomy completeness | all required BL041-FX IDs defined |
| BL041-A1-007 | Artifact schema completeness | required artifacts and TSV column contracts declared |
| BL041-A1-008 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

### 7) QA Artifact Schema Contract (A1)

Required evidence path:
- `TestEvidence/bl041_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `doppler_vbap_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

Required `acceptance_matrix.tsv` columns:
- `acceptance_id`
- `gate`
- `threshold`
- `measured_value`
- `result`
- `evidence_path`

Required `failure_taxonomy.tsv` columns:
- `failure_id`
- `category`
- `trigger`
- `classification`
- `blocking`
- `severity`
- `expected_artifact`

### 8) Failure Taxonomy (Authoritative)

| Failure ID | Category | Trigger | Classification | Blocking |
|---|---|---|---|---|
| BL041-FX-001 | doppler_contract_incomplete | required Doppler field/range/fallback clause missing | deterministic_contract_failure | yes |
| BL041-FX-002 | doppler_smoothing_threshold_missing | smoothing/continuity thresholds absent | deterministic_contract_failure | yes |
| BL041-FX-003 | doppler_non_finite_state | non-finite Doppler input/intermediate/state detected | deterministic_contract_failure | yes |
| BL041-FX-004 | vbap_geometry_contract_incomplete | geometry validity thresholds/rules missing | deterministic_contract_failure | yes |
| BL041-FX-005 | vbap_degenerate_triplet | triplet area `<= area_epsilon` | deterministic_contract_failure | yes |
| BL041-FX-006 | vbap_gain_normalization_failure | finite/normalized gain contract violated | deterministic_contract_failure | yes |
| BL041-FX-007 | boundary_continuity_violation | boundary gain jump exceeds threshold | deterministic_contract_failure | yes |
| BL041-FX-008 | replay_identity_incomplete | deterministic hash input set incomplete | deterministic_contract_failure | yes |
| BL041-FX-009 | replay_trace_divergence | identical inputs produce divergent output/fallback traces | deterministic_replay_failure | yes |
| BL041-FX-010 | artifact_schema_incomplete | required artifact or required columns missing | deterministic_evidence_failure | yes |

## Traceability References

- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`

## TODOs (Slice A1)

- [x] Define Doppler v2 deterministic data/bounds contract.
- [x] Define smoothing and continuity thresholds with explicit numeric limits.
- [x] Define VBAP geometry validity thresholds and deterministic boundary behavior.
- [x] Define fallback/degradation policy tokens and fail-closed behavior.
- [x] Define acceptance IDs and measurable pass/fail thresholds.
- [x] Define failure taxonomy with blocking classification.
- [x] Define replay artifact schema and required evidence files.
- [x] Capture A1 evidence bundle and docs freshness validation log.

## Validation Plan (A1)

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (A1)

- `status.tsv`
- `doppler_vbap_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Slice B1 Lane Bootstrap (Contract Harness + Scenario Scaffold)

B1 scope is contract-only and additive:
- Add deterministic BL-041 lane harness script:
  - `scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- Add BL-041 scenario scaffold:
  - `qa/scenarios/locusq_bl041_doppler_vbap_suite.json`
- Align runbook + QA docs to B1 acceptance IDs and artifact schema.
- Keep B1 source-free (no `Source/*` or `Source/ui/*` edits).

### B1 Acceptance IDs

| Acceptance ID | Requirement | Pass Threshold |
|---|---|---|
| BL041-B1-001 | Scenario schema contract exists | scenario id + required lane schema fields present |
| BL041-B1-002 | B1 acceptance alignment | IDs `BL041-B1-001..008` present in scenario + runbook + QA docs |
| BL041-B1-003 | Deterministic hash input contract complete | all required hash include fields present; nondeterministic excludes declared |
| BL041-B1-004 | Fallback token contract complete | required fallback tokens present and deterministic |
| BL041-B1-005 | Artifact schema contract complete | `status.tsv`, `validation_matrix.tsv`, `replay_hashes.tsv`, `failure_taxonomy.tsv` declared |
| BL041-B1-006 | Runbook alignment | B1 validation/evidence sections reference lane script + scenario |
| BL041-B1-007 | QA alignment | QA doc contains B1 matrix + validation + evidence contract |
| BL041-B1-008 | Mode/exit semantics explicit | lane script exposes `--contract-only`/`--execute-suite` and strict `0/1/2` exits |

### B1 Failure Taxonomy Additions

| Failure ID | Category | Trigger | Classification | Blocking |
|---|---|---|---|---|
| BL041-FX-101 | lane_contract_schema_missing | scenario id/schema fields missing | deterministic_contract_failure | yes |
| BL041-FX-102 | lane_acceptance_alignment_missing | B1 IDs missing in scenario/runbook/QA | deterministic_contract_failure | yes |
| BL041-FX-103 | lane_hash_input_contract_missing | required deterministic hash inputs missing | deterministic_contract_failure | yes |
| BL041-FX-104 | lane_fallback_contract_missing | required fallback tokens missing | deterministic_contract_failure | yes |
| BL041-FX-105 | lane_replay_signature_drift | replay signatures diverge for equal inputs | deterministic_replay_failure | yes |
| BL041-FX-106 | lane_replay_row_drift | replay row signatures diverge for equal inputs | deterministic_replay_failure | yes |
| BL041-FX-107 | lane_artifact_schema_incomplete | required artifact files missing | deterministic_evidence_failure | yes |

## Validation Plan (B1)

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl041_slice_b1_lane_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (B1)

Evidence bundle path:
- `TestEvidence/bl041_slice_b1_lane_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C2 Determinism Soak (10-Run Contract Replay)

C2 scope is additive and contract-only:
- Keep B1 checks intact.
- Execute deterministic replay on the same contract lane with `--runs 10`.
- Require stable replay hashes, stable validation rows, and stable taxonomy across all runs.

### C2 Acceptance IDs

| Acceptance ID | Requirement | Pass Threshold |
|---|---|---|
| BL041-C2-001 | C2 soak replay contract declared | `required_runs=10`, `max_signature_divergence=0`, `max_row_drift=0` |
| BL041-C2-002 | C2 acceptance alignment | IDs `BL041-C2-001..005` present in scenario + runbook + QA docs |
| BL041-C2-003 | C2 evidence schema complete | required C2 artifacts declared in scenario contract |
| BL041-C2-004 | Runbook C2 alignment | `Validation Plan (C2)` + `Evidence Contract (C2)` + `--runs 10` explicitly documented |
| BL041-C2-005 | QA C2 alignment | QA runbook contains C2 validation/evidence + replay/taxonomy checks |

### C2 Failure Taxonomy Additions

| Failure ID | Category | Trigger | Classification | Blocking |
|---|---|---|---|---|
| BL041-FX-201 | lane_c2_contract_missing | C2 soak fields/thresholds missing from scenario contract | deterministic_contract_failure | yes |
| BL041-FX-202 | lane_c2_acceptance_alignment_missing | C2 IDs missing across scenario/runbook/QA | deterministic_contract_failure | yes |
| BL041-FX-203 | lane_c2_evidence_schema_incomplete | C2 required evidence list incomplete | deterministic_evidence_failure | yes |
| BL041-FX-204 | lane_c2_signature_drift | replay signatures drift across 10-run soak | deterministic_replay_failure | yes |
| BL041-FX-205 | lane_c2_row_drift | validation row signature drift across 10-run soak | deterministic_replay_failure | yes |
| BL041-FX-206 | lane_c2_taxonomy_drift | non-zero or unstable taxonomy rows across replay | deterministic_replay_failure | yes |

## Validation Plan (C2)

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl041_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (C2)

Evidence bundle path:
- `TestEvidence/bl041_slice_c2_soak_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C3 Replay Sentinel + Mode Parity (20-Run Contract/Execute Replay)

C3 scope is additive and deterministic:
- Keep B1 and C2 checks intact.
- Run both `--contract-only` and `--execute-suite` in 20-run replay mode.
- Require parity across modes for deterministic signature/row behavior.
- Probe invalid run count (`--runs 0`) and require strict exit `2`.

### C3 Acceptance IDs

| Acceptance ID | Requirement | Pass Threshold |
|---|---|---|
| BL041-C3-001 | C3 replay sentinel contract declared | `required_runs=20`, `max_signature_divergence=0`, `max_row_drift=0` |
| BL041-C3-002 | C3 acceptance alignment | IDs `BL041-C3-001..006` present in scenario + runbook + QA docs |
| BL041-C3-003 | C3 evidence schema complete | required C3 artifacts declared in scenario contract |
| BL041-C3-004 | Runbook C3 alignment | `Validation Plan (C3)` + `Evidence Contract (C3)` + parity/exit probe artifacts explicitly documented |
| BL041-C3-005 | QA C3 alignment | QA runbook contains C3 validation/evidence + mode parity and exit-semantics checks |
| BL041-C3-006 | Mode and exit semantics explicit | script preserves `--contract-only` and `--execute-suite`; invalid `--runs` exits `2` |

### C3 Failure Taxonomy Additions

| Failure ID | Category | Trigger | Classification | Blocking |
|---|---|---|---|---|
| BL041-FX-301 | lane_c3_contract_missing | C3 replay sentinel thresholds missing from scenario contract | deterministic_contract_failure | yes |
| BL041-FX-302 | lane_c3_acceptance_alignment_missing | C3 IDs missing across scenario/runbook/QA | deterministic_contract_failure | yes |
| BL041-FX-303 | lane_c3_evidence_schema_incomplete | C3 required evidence list incomplete | deterministic_evidence_failure | yes |
| BL041-FX-304 | lane_c3_mode_parity_failure | contract-only/execute replay signatures or rows diverge | deterministic_replay_failure | yes |
| BL041-FX-305 | lane_c3_exit_semantics_failure | `--runs 0` does not return exit code `2` | deterministic_contract_failure | yes |
| BL041-FX-306 | lane_c3_taxonomy_drift | non-zero or unstable taxonomy rows across parity replay | deterministic_replay_failure | yes |

## Validation Plan (C3)

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl041_slice_c3_replay_mode_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl041_slice_c3_replay_mode_parity_<timestamp>/contract_runs_execute`
- Negative probe: `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (C3)

Evidence bundle path:
- `TestEvidence/bl041_slice_c3_replay_mode_parity_<timestamp>/`

Required files:
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

## Slice C4 Long-Run Replay Sentinel + Mode Parity (50-Run Contract/Execute Replay)

C4 scope increases deterministic replay depth while keeping C3 semantics intact:
- Keep B1/C2/C3 checks intact with no script/source edits.
- Run both `--contract-only` and `--execute-suite` in 50-run replay mode.
- Require zero cross-mode signature and row mismatch across all 50 runs.
- Preserve strict usage semantics (`--runs 0` must exit `2`) and docs freshness gate.

### C4 Acceptance IDs

| Acceptance ID | Requirement | Pass Threshold |
|---|---|---|
| BL041-C4-001 | Contract-only long-run sentinel | `--contract-only --runs 50` completes with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-C4-002 | Execute-suite long-run sentinel | `--execute-suite --runs 50` completes with `signature_drift_count=0`, `row_drift_count=0` |
| BL041-C4-003 | Cross-mode parity | `cross_mode_signature_mismatch_count=0` and `cross_mode_row_mismatch_count=0` |
| BL041-C4-004 | Taxonomy stability | contract/execute taxonomy rows remain none-only (`nonzero_rows=0`) |
| BL041-C4-005 | Exit semantics guard | `--runs 0` returns exit code `2` deterministically |
| BL041-C4-006 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |
| BL041-C4-007 | C4 evidence schema complete | all required C4 files emitted under evidence root |

### C4 Failure Taxonomy Additions

| Failure ID | Category | Trigger | Classification | Blocking |
|---|---|---|---|---|
| BL041-FX-401 | lane_c4_contract_longrun_drift | contract-only 50-run signature/row drift detected | deterministic_replay_failure | yes |
| BL041-FX-402 | lane_c4_execute_longrun_drift | execute-suite 50-run signature/row drift detected | deterministic_replay_failure | yes |
| BL041-FX-403 | lane_c4_mode_parity_failure | contract/execute replay hashes diverge by run | deterministic_replay_failure | yes |
| BL041-FX-404 | lane_c4_taxonomy_drift | non-zero or unstable taxonomy rows across long-run replay | deterministic_replay_failure | yes |
| BL041-FX-405 | lane_c4_exit_semantics_failure | invalid `--runs` probe does not return exit `2` | deterministic_contract_failure | yes |
| BL041-FX-406 | lane_c4_docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes |
| BL041-FX-407 | lane_c4_evidence_schema_incomplete | required C4 artifact files missing | deterministic_evidence_failure | yes |

## Validation Plan (C4)

- `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl041_slice_c4_longrun_mode_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl041_slice_c4_longrun_mode_parity_<timestamp>/contract_runs_execute`
- Negative probe: `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (C4)

Evidence bundle path:
- `TestEvidence/bl041_slice_c4_longrun_mode_parity_<timestamp>/`

Required files:
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

## Slice B1 Execution Snapshot (2026-02-27)

- Input handoff resolved:
  - `TestEvidence/bl041_slice_a1_contract_20260227T010932Z/status.tsv`
  - `TestEvidence/bl041_slice_a1_contract_20260227T010932Z/acceptance_matrix.tsv`
  - `TestEvidence/bl041_slice_a1_contract_20260227T010932Z/failure_taxonomy.tsv`
- Evidence packet:
  - `TestEvidence/bl041_slice_b1_lane_20260227T011936Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 3 --out-dir .../contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Replay determinism summary:
  - `runs_observed=3`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`
- Result:
  - B1 bootstrap lane is replay-stable and contract-aligned; BL-041 remains in implementation for future runtime slices.

## Slice C2 Execution Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl041_slice_b1_lane_20260227T011936Z/contract_runs/validation_matrix.tsv`
  - `TestEvidence/bl041_slice_b1_lane_20260227T011936Z/contract_runs/replay_hashes.tsv`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z4_20260227T013040Z/bl041_recheck/status.tsv`
- Evidence packet:
  - `TestEvidence/bl041_slice_c2_soak_20260227T014141Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `soak_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 10 --out-dir .../contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Replay determinism summary:
  - `runs_observed=10`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `contract_fail_rows=0`
  - `taxonomy_nonzero_rows=0`
- Result:
  - C2 deterministic soak contract is replay-stable for 10 runs with no taxonomy drift.

## Slice C3 Execution Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl041_slice_c2_soak_20260227T014141Z/status.tsv`
  - `TestEvidence/bl041_slice_c2_soak_20260227T014141Z/soak_summary.tsv`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z5_20260227T014558Z/bl041_recheck/status.tsv`
- Evidence packet:
  - `TestEvidence/bl041_slice_c3_replay_mode_parity_20260227T015445Z/status.tsv`
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
- Validation:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (expected exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (out-of-scope metadata issue)
- Replay sentinel and mode parity summary:
  - `contract_runs_observed=20`
  - `execute_runs_observed=20`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
- Blocker:
  - `Documentation/research/HRTF and Personalized Headphone Calibration.md` missing required metadata fields; docs freshness gate remains red.

### Owner Intake Sync Z6 (2026-02-27)

- Owner packet:
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z6_20260227T021108Z/status.tsv`
  - `validation_matrix.tsv`
  - `owner_decisions.md`
  - `handoff_resolution.md`
- Owner replay:
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z6_20260227T021108Z/bl041_recheck` => `PASS`
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 3 --out-dir TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z6_20260227T021108Z/bl040_recheck` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
  - `jq empty status.json` => `PASS`
- Disposition:
  - BL-041 remains `In Implementation`; C3 replay sentinel + mode parity packet is accepted with deterministic contract/execute parity and strict usage-exit semantics.

## Slice C3b Recheck Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl041_slice_c2_soak_20260227T014141Z/status.tsv`
  - `TestEvidence/bl041_slice_c3_replay_mode_parity_20260227T015445Z/status.tsv`
  - `TestEvidence/docs_hygiene_hrtf_h1_20260227T020511Z/status.tsv`
- Evidence packet:
  - `TestEvidence/bl041_slice_c3b_replay_mode_parity_20260227T025246Z/status.tsv`
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
- Validation:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (expected exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (out-of-scope metadata issues)
- Replay sentinel and mode parity summary:
  - `contract_runs_observed=20`
  - `execute_runs_observed=20`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
- Blocker:
  - Docs freshness currently fails on out-of-scope files under `Documentation/Calibration POC/*` missing required metadata headers.

## Slice C3c Recheck Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl041_slice_c3b_replay_mode_parity_20260227T025246Z/status.tsv`
  - `TestEvidence/docs_hygiene_calibration_poc_h2_*/status.tsv`
- Evidence packet:
  - `TestEvidence/bl041_slice_c3c_replay_mode_parity_20260227T031142Z/status.tsv`
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
- Validation:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 20 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 20 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (expected exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Replay sentinel and mode parity summary:
  - `contract_runs_observed=20`
  - `execute_runs_observed=20`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=PASS`

### Owner Intake Sync Z7 (2026-02-27)

- Owner packet:
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z7_20260227T032802Z/status.tsv`
  - `validation_matrix.tsv`
  - `owner_decisions.md`
  - `handoff_resolution.md`
- Owner replay:
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z7_20260227T032802Z/bl041_recheck` => `PASS`
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 3 --out-dir TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z7_20260227T032802Z/bl040_recheck` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
  - `jq empty status.json` => `PASS`
- Disposition:
  - BL-041 remains `In Implementation`; C3c packet is accepted and H2 metadata hygiene closure is integrated.

## Slice C4 Long-Run Replay Sentinel + Mode Parity Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl041_slice_c3c_replay_mode_parity_20260227T031142Z/status.tsv`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z7_20260227T032802Z/status.tsv`
- Evidence packet:
  - `TestEvidence/bl041_slice_c4_longrun_mode_parity_20260227T033844Z/status.tsv`
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
- Validation:
  - `bash -n scripts/qa-bl041-doppler-vbap-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --contract-only --runs 50 --out-dir .../contract_runs_contract` => `PASS`
  - `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --execute-suite --runs 50 --out-dir .../contract_runs_execute` => `PASS`
  - Negative probe `./scripts/qa-bl041-doppler-vbap-lane-mac.sh --runs 0` => `PASS` (expected exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Long-run parity summary:
  - `contract_runs_observed=50`
  - `execute_runs_observed=50`
  - `signature_drift_count(contract/execute)=0/0`
  - `row_drift_count(contract/execute)=0/0`
  - `cross_mode_signature_mismatch_count=0`
  - `cross_mode_row_mismatch_count=0`
  - `mode_parity_gate=PASS`
  - `exit_semantics_gate=PASS`
  - `docs_freshness_gate=PASS`
