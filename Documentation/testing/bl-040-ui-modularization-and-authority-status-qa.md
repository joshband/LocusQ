Title: BL-040 UI Modularization and Authority Status QA
Document Type: QA Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-040 UI Modularization and Authority Status QA

## Purpose

Define deterministic QA contract checks for BL-040 authority-state provenance, stale/lock/fallback signaling, and replay stability.

## Linked Contracts

- Runbook: `Documentation/backlog/bl-040-ui-modularization-and-authority-status.md`
- Invariants: `Documentation/invariants.md`
- Scene-state authority context: `Documentation/scene-state-contract.md`

## A1 Acceptance Matrix

| Acceptance ID | Gate | Deterministic Rule | Evidence Signal |
|---|---|---|---|
| `BL040-A1-001` | provenance_precedence | precedence list exactly 5 entries in defined order | runbook + QA parity |
| `BL040-A1-002` | status_classes | exactly 5 status classes with enter/exit rules | runbook + QA parity |
| `BL040-A1-003` | stale_thresholds | `stale_warn_ms=500`, `stale_fail_ms=1500` | threshold rows |
| `BL040-A1-004` | required_fields | required additive field table complete | field schema table |
| `BL040-A1-005` | module_boundaries | module domains exactly: bridge/state/controls/viewport/selftest | module table |
| `BL040-A1-006` | replay_seq_contract | monotonic replay sequence rule explicit | replay contract section |
| `BL040-A1-007` | failure_taxonomy | failure table contains `BL040-FX-001..007` | taxonomy table |
| `BL040-A1-008` | evidence_schema | required A1 files listed | evidence contract section |
| `BL040-A1-009` | acceptance_parity | same IDs in runbook + QA + evidence matrix | parity rows |

## Authority-State Taxonomy

| Failure ID | Class | Blocking | Required Classification |
|---|---|---|---|
| `BL040-FX-001` | missing_required_authority_field | yes | deterministic_contract_failure |
| `BL040-FX-002` | invalid_authority_source | yes | deterministic_contract_failure |
| `BL040-FX-003` | status_class_transition_invalid | yes | deterministic_contract_failure |
| `BL040-FX-004` | stale_threshold_violation | yes | deterministic_contract_failure |
| `BL040-FX-005` | lock_reason_missing | yes | deterministic_contract_failure |
| `BL040-FX-006` | replay_sequence_non_monotonic | yes | deterministic_contract_failure |
| `BL040-FX-007` | artifact_schema_incomplete | yes | deterministic_evidence_failure |

## Replay Determinism Contract

For runtime slices (`--runs N`, `N>=3`):
1. Replay signatures for authority status rows must match baseline (`divergence=0`).
2. Row count/order must match baseline (`row_drift=0`).
3. `authorityReplaySeq` must be monotonic non-decreasing.
4. Any violation maps to `BL040-FX-003` or `BL040-FX-006`.

Expected machine-readable artifacts for runtime slices:
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`

## A1 Validation

```bash
./scripts/validate-docs-freshness.sh
```

Pass criteria:
- Exit code `0`
- Acceptance parity complete across runbook + QA + evidence

## A1 Evidence Contract

Required files under `TestEvidence/bl040_slice_a1_contract_<timestamp>/`:
- `status.tsv`
- `ui_authority_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## B1 Acceptance Matrix (UI Diagnostics Bootstrap)

| Acceptance ID | Gate | Deterministic Rule | Evidence Signal |
|---|---|---|---|
| `BL040-B1-001` | authority_card_presence | `rend-auth-*` card + field IDs present in HTML | harness validation rows |
| `BL040-B1-002` | authority_card_default_collapsed | toggle defaults collapsed (`aria-expanded=false`, content hidden) | harness validation rows |
| `BL040-B1-003` | payload_detection_contract | additive payload detection + fallback defaults are present in JS | harness validation rows |
| `BL040-B1-004` | toggle_wiring_contract | deterministic expand/collapse setter + bootstrap bind | harness validation rows |
| `BL040-B1-005` | status_chip_mapping | status classes map to deterministic chip states | harness validation rows |
| `BL040-B1-006` | availability_summary_contract | diagnostics summary includes Authority present/missing | harness validation rows |
| `BL040-B1-007` | replay_determinism | replay signature/row signature drift = `0` for equal contract runs | `replay_hashes.tsv` |
| `BL040-B1-008` | artifact_schema | required B1 artifact files exist | owner packet + artifact listing |

## B1 Taxonomy

| Failure ID | Class | Blocking | Required Classification |
|---|---|---|---|
| `BL040-FX-101` | authority_diag_card_missing | yes | deterministic_contract_failure |
| `BL040-FX-102` | authority_toggle_contract_invalid | yes | deterministic_contract_failure |
| `BL040-FX-103` | authority_fallback_contract_invalid | yes | deterministic_contract_failure |
| `BL040-FX-104` | authority_chip_mapping_invalid | yes | deterministic_contract_failure |
| `BL040-FX-105` | authority_replay_signature_drift | yes | deterministic_contract_failure |
| `BL040-FX-106` | authority_artifact_schema_incomplete | yes | deterministic_evidence_failure |

## B1 Harness Contract

Script:
- `scripts/qa-bl040-ui-authority-diagnostics-mac.sh`

Required arguments:
- `--runs <N>`
- `--out-dir <path>`
- `--contract-only`

Exit semantics:
- `0`: pass
- `1`: gate fail
- `2`: usage/config error

Required outputs:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`

## B1 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl040_slice_b1_ui_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

## B1 Evidence Contract

Required files under `TestEvidence/bl040_slice_b1_ui_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C2 Replay Sentinel Contract

Purpose:
- Confirm high-repeat replay determinism for existing BL-040 authority diagnostics contract lane.
- Confirm failure taxonomy stability during repeated contract-only runs.

Sentinel gate rules:
1. Run count must be exactly `10` for C2 replay soak.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed).
5. All required validation commands must exit `0`.

## C2 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl040_slice_c2_ui_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

## C2 Evidence Contract

Required files under `TestEvidence/bl040_slice_c2_ui_soak_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C3 Replay Sentinel Contract

Purpose:
- Extend sentinel depth from C2 (`runs=10`) to C3 (`runs=20`) while preserving existing harness behavior and strict exit semantics.
- Keep deterministic replay and taxonomy gates machine-readable for owner intake.

C3 sentinel gate rules:
1. Run count must be exactly `20`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Required validation commands must all exit `0`.

## C3 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl040_slice_c3_ui_replay_sentinel_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

## C3 Evidence Contract

Required files under `TestEvidence/bl040_slice_c3_ui_replay_sentinel_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C4 Replay Sentinel Soak Contract

Purpose:
- Extend sentinel depth from C3 (`runs=20`) to C4 soak (`runs=50`) while preserving deterministic signatures/taxonomy and strict harness exit semantics.
- Validate replay stability for extended deterministic soak windows before owner intake.

C4 sentinel gate rules:
1. Run count must be exactly `50`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Required validation commands must all exit `0`.

## C4 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl040_slice_c4_ui_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

## C4 Evidence Contract

Required files under `TestEvidence/bl040_slice_c4_ui_soak_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C5 Exit-Semantics Guard Contract

Purpose:
- Preserve C3/C4 deterministic replay sentinel behavior at `runs=20`.
- Add strict usage-exit guard probes to prove deterministic `exit 2` behavior for invalid usage paths.

C5 guard gate rules:
1. Run count must be exactly `20`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Negative probe `--runs 0` must exit `2`.
6. Negative probe `--bad-flag` must exit `2`.
7. Required validation commands must execute and be captured in machine-readable evidence.

## C5 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl040_slice_c5_ui_semantics_<timestamp>/contract_runs
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

## C5 Evidence Contract

Required files under `TestEvidence/bl040_slice_c5_ui_semantics_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `exit_semantics_probe.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C5b Exit-Semantics Guard Recheck Contract

Purpose:
- Re-run C5 UI semantics guard after docs hygiene handoff and publish a clean deterministic packet when freshness gate permits.

C5b guard gate rules:
1. Run count must be exactly `20`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Negative probe `--runs 0` must exit `2`.
6. Negative probe `--bad-flag` must exit `2`.
7. Required validation commands must execute and be captured in machine-readable evidence.

## C5b Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl040_slice_c5b_ui_semantics_<timestamp>/contract_runs
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

## C5b Evidence Contract

Required files under `TestEvidence/bl040_slice_c5b_ui_semantics_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `exit_semantics_probe.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C5c Exit-Semantics Guard Recheck Contract (Post-H2)

Purpose:
- Re-run C5 semantics guard after H2 metadata hygiene handoff and publish deterministic packet with refreshed docs freshness status.

C5c guard gate rules:
1. Run count must be exactly `20`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Negative probe `--runs 0` must exit `2`.
6. Negative probe `--bad-flag` must exit `2`.
7. Required validation commands must execute and be captured in machine-readable evidence.

## C5c Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl040_slice_c5c_ui_semantics_<timestamp>/contract_runs
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

## C5c Evidence Contract

Required files under `TestEvidence/bl040_slice_c5c_ui_semantics_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `exit_semantics_probe.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C5b Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl040_slice_c4_ui_soak_20260227T013820Z/*`
  - `TestEvidence/bl040_slice_c5_ui_semantics_20260227T015533Z/*`
  - `TestEvidence/docs_hygiene_hrtf_h1_20260227T020511Z/*`
- Evidence bundle:
  - `TestEvidence/bl040_slice_c5b_ui_semantics_20260227T025301Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `ui_diagnostics_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `ui_diagnostics_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `node --check Source/ui/public/js/index.js` => PASS
  - `bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl040_slice_c5b_ui_semantics_20260227T025301Z/contract_runs` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0` (expect exit `2`) => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag` (expect exit `2`) => PASS
  - `./scripts/validate-docs-freshness.sh` => FAIL (external metadata failures in `Documentation/Calibration POC/*.md` outside C5b ownership)

## C5c Execution Snapshot (2026-02-27)

- Evidence path: `TestEvidence/bl040_slice_c5c_ui_semantics_20260227T031110Z/`
- Validation outcomes:
  - `node --check Source/ui/public/js/index.js` => PASS
  - `bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 20 --out-dir .../contract_runs` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0` (expect exit `2`) => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag` (expect exit `2`) => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS
- Determinism/taxonomy/semantics summary:
  - `runs_observed=20`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`
  - `usage_probe_runs0_exit=2`
  - `usage_probe_badflag_exit=2`

## C6 Long-Run Exit-Semantics Sentinel Contract

Purpose:
- Extend BL-040 UI authority diagnostics sentinel confidence to `runs=50` with strict usage-exit semantics checks.

C6 guard gate rules:
1. Run count must be exactly `50`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Negative probe `--runs 0` must exit `2`.
6. Negative probe `--bad-flag` must exit `2`.
7. Required validation commands must execute and be captured in machine-readable evidence.

## C6 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl040_slice_c6_ui_longrun_<timestamp>/contract_runs
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

## C6 Evidence Contract

Required files under `TestEvidence/bl040_slice_c6_ui_longrun_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `exit_semantics_probe.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## C6 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl040_slice_c5c_ui_semantics_20260227T031110Z/*`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z7_20260227T032802Z/*`
- Evidence bundle:
  - `TestEvidence/bl040_slice_c6_ui_longrun_20260227T033800Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `ui_diagnostics_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `ui_diagnostics_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `node --check Source/ui/public/js/index.js` => PASS
  - `bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl040_slice_c6_ui_longrun_20260227T033800Z/contract_runs` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0` (expect exit `2`) => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag` (expect exit `2`) => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS
- Determinism/taxonomy/semantics summary:
  - `runs_observed=50`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`
  - `usage_probe_runs0_exit=2`
  - `usage_probe_badflag_exit=2`

## D1 Done-Candidate Long-Run Sentinel Contract

Purpose:
- Publish BL-040 done-candidate readiness packet with extended deterministic UI authority diagnostics soak depth and strict usage-exit semantics.

D1 guard gate rules:
1. Run count must be exactly `75`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Negative probe `--runs 0` must exit `2`.
6. Negative probe `--bad-flag` must exit `2`.
7. Required validation commands must execute and be captured in machine-readable evidence.

## D1 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl040_slice_d1_done_candidate_<timestamp>/contract_runs
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

## D1 Evidence Contract

Required files under `TestEvidence/bl040_slice_d1_done_candidate_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `exit_semantics_probe.tsv`
- `ui_diagnostics_notes.md`
- `docs_freshness.log`

## D1 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl040_slice_c6_ui_longrun_20260227T033800Z/*`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z8_20260227T042149Z/*`
- Evidence bundle:
  - `TestEvidence/bl040_slice_d1_done_candidate_20260227T183452Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `ui_diagnostics_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `ui_diagnostics_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `node --check Source/ui/public/js/index.js` => PASS
  - `bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 75 --out-dir TestEvidence/bl040_slice_d1_done_candidate_20260227T183452Z/contract_runs` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0` (expect exit `2`) => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag` (expect exit `2`) => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS
- Determinism/taxonomy/semantics summary:
  - `runs_observed=75`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`
  - `usage_probe_runs0_exit=2`
  - `usage_probe_badflag_exit=2`

## D2 Done Promotion Sentinel Contract

Purpose:
- Promote BL-040 from D1 done-candidate to done state using an extended deterministic authority diagnostics sentinel with strict usage exits.

D2 guard gate rules:
1. Run count must be exactly `100`.
2. `signature_drift_count` must be `0`.
3. `row_drift_count` must be `0`.
4. `taxonomy_nonzero_rows` must be `0` (only `none` row allowed in taxonomy output).
5. Negative probe `--runs 0` must exit `2`.
6. Negative probe `--bad-flag` must exit `2`.
7. Required validation commands must execute and be captured in machine-readable evidence.

## D2 Validation

```bash
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl040_slice_d2_done_promotion_<timestamp>/contract_runs
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0
./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag
./scripts/validate-docs-freshness.sh
```

## D2 Evidence Contract

Required files under `TestEvidence/bl040_slice_d2_done_promotion_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `ui_diagnostics_summary.tsv`
- `exit_semantics_probe.tsv`
- `promotion_readiness.md`
- `docs_freshness.log`

## D2 Execution Snapshot (2026-02-27)

- Input handoffs:
  - `TestEvidence/bl040_slice_d1_done_candidate_20260227T183452Z/*`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z9_20260227T195521Z/*`
- Evidence bundle:
  - `TestEvidence/bl040_slice_d2_done_promotion_20260227T201804Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `ui_diagnostics_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `promotion_readiness.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `node --check Source/ui/public/js/index.js` => PASS
  - `bash -n scripts/qa-bl040-ui-authority-diagnostics-mac.sh` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --help` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --contract-only --runs 100 --out-dir TestEvidence/bl040_slice_d2_done_promotion_20260227T201804Z/contract_runs` => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --runs 0` (expect exit `2`) => PASS
  - `./scripts/qa-bl040-ui-authority-diagnostics-mac.sh --bad-flag` (expect exit `2`) => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS
- Determinism/taxonomy/semantics summary:
  - `runs_observed=100`
  - `signature_drift_count=0`
  - `row_drift_count=0`
  - `taxonomy_nonzero_rows=0`
  - `usage_probe_runs0_exit=2`
  - `usage_probe_badflag_exit=2`
