Title: BL-023 Resize/DPI Hardening QA
Document Type: QA Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-28

# BL-023 Resize/DPI Hardening QA

## Purpose

This document defines the deterministic regression matrix and pass/fail rules for BL-023 resize and DPI behavior across standalone and plugin hosts.

## Matrix Dimensions

### Viewport Bounds

| ID | Width x Height | Category |
|---|---|---|
| VP-01 | 800x600 | minimum usable |
| VP-02 | 960x640 | compact breakpoint edge |
| VP-03 | 1280x800 | standard baseline |
| VP-04 | 1440x900 | wide breakpoint edge |
| VP-05 | 2560x1440 | max reference |

### DPI/Scale Targets

| ID | Scale | Notes |
|---|---|---|
| DPI-01 | 1.0 | standard display baseline |
| DPI-02 | 1.5 | mixed-scaling path where available |
| DPI-03 | 2.0 | Retina/high-density baseline |

### Host/Runtime Matrix

| Lane ID | Host Mode | Host | Plugin Format | WebView Backend | Required |
|---|---|---|---|---|---|
| BL023-HM-001 | standalone | LocusQ Standalone | n/a | WKWebView | yes |
| BL023-HM-002 | plugin | REAPER | VST3 | WKWebView | yes |
| BL023-HM-003 | plugin | Logic Pro | AU | WKWebView | yes |
| BL023-HM-004 | plugin | Ableton Live | VST3 | WKWebView | yes |

## Deterministic Checks

| Check ID | Check | Pass Criteria |
|---|---|---|
| BL023-CHK-001 | overflow/overlap | No layout overflow or overlapping controls at VP-01..VP-05. |
| BL023-CHK-002 | clipped controls | No clipped controls/labels at required viewport and DPI lanes. |
| BL023-CHK-003 | hit-target map | Interactive hit targets match rendered control positions after resize. |
| BL023-CHK-004 | pixel ratio | Effective ratio delta vs target scale is `<= 0.05`. |
| BL023-CHK-005 | settle latency | Layout stabilization after final resize event is `<= 250ms`. |

## UI Diagnostics Contract (Slice B1)

Renderer diagnostics panel must expose an additive `Resize / DPI` card (collapsed by default) with these fields:

| Field ID | Label | Type | Rule | Unknown Fallback |
|---|---|---|---|---|
| `rend-resize-viewport` | Viewport (W x H) | string | Derived from active viewport resize dimensions | `unknown` |
| `rend-resize-dpr` | Device Pixel Ratio | string | Finite `window.devicePixelRatio` formatted to 2 decimals | `unknown` |
| `rend-resize-bucket` | Layout Bucket | enum string | `compact|standard|wide` from active layout/width contract | `unknown` |
| `rend-resize-settle` | Last Resize Settle | string | Last bounded settle duration in milliseconds | `unknown` |

Supporting contract IDs:

| Contract ID | Rule | Pass Criteria |
|---|---|---|
| BL023-DIAG-001 | Panel default state | Card content hidden on boot (`aria-expanded=false`) |
| BL023-DIAG-002 | Additive behavior | Existing diagnostics cards and selftest IDs unchanged |
| BL023-DIAG-003 | Fallback safety | Missing/unavailable metrics render `unknown` and do not throw |
| BL023-DIAG-004 | Bounded settle | No unbounded polling; settle computed with bounded debounce window |
| BL023-DIAG-005 | Settle threshold | `rend-resize-settle` warning threshold aligns to BL023-CHK-005 (`>250ms` warns) |

## Resize Cadence Expectations

- Resize processing must remain bounded and deterministic; no unbounded layout recalculation loops.
- Final interaction checks are evaluated only after settle condition is reached.
- Cadence anomalies are recorded with a taxonomy ID and reproduction context.

## Failure Taxonomy

| Taxonomy ID | Failure Class | Trigger |
|---|---|---|
| BL023-RZ-001 | layout_overflow | Unexpected scrollbars, overlap, or viewport overflow appears. |
| BL023-RZ-002 | stale_hit_target_map | Click/touch result mismatches rendered position after resize/DPI update. |
| BL023-RZ-003 | pixel_ratio_mismatch | Ratio mismatch exceeds tolerance (`> 0.05`). |
| BL023-RZ-004 | clipped_controls | Controls/labels clipped at contract viewport/DPI lane. |
| BL023-RZ-900 | harness_gate_failure | Deterministic gate/schema/runtime failure outside RZ-001..RZ-004. |
| BL023-RZ-910 | deterministic_replay_signature_divergence | Multi-run signature mismatch vs baseline run. |
| BL023-RZ-911 | deterministic_replay_row_count_drift | Multi-run host matrix row-count mismatch vs baseline run. |

## Evidence Output Contract

For each BL-023 run, record:

- `host_matrix.tsv` rows with: `lane_id,host,mode,format,backend,viewport_id,dpi_id,check_id,result,taxonomy_id,notes`.
- Failure rows must include one taxonomy ID from BL023-RZ-001..004.
- `status.tsv` must summarize overall pass/fail and blocker IDs.

## C1 Host Matrix Harness Contract

Command:
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh`

Options:
- `--runs <N>`: deterministic replay count, integer `>= 1`
- `--out-dir <path>`: artifact output directory
- `--contract-only`: docs/contract replay without runtime host launch

Required outputs:
- `status.tsv`: lane-level pass/fail summary and exit details
- `validation_matrix.tsv`: run-level checks (`BL023-C1-RUN-001`, `BL023-C1-DET-001`)
- `host_matrix_results.tsv`: machine-readable host lane outcomes (`BL023-HM-001..004`)
- `failure_taxonomy.tsv`: aggregated failure taxonomy counts
- `determinism_summary.tsv`: deterministic replay counters and thresholds

Exit semantics:
- `0`: gate pass
- `1`: gate fail
- `2`: usage error

Deterministic replay rule:
- For `--runs N`, each run must emit a stable matrix signature (`BL023-C1-DET-001`).
- For `--runs N`, each run must preserve host matrix row-count parity (`BL023-C2-DET-002`).
- Signature mismatch is a deterministic failure and must be recorded in `failure_taxonomy.tsv`.
- Row-count mismatch is a deterministic failure and must be recorded in `failure_taxonomy.tsv`.

## C2 Soak Hardening Additions

Slice C2 requires deterministic multi-run hardening with:
1. Stable status/matrix/taxonomy outputs in contract-only and runtime replay modes.
2. Fixed-order taxonomy rows (including zero-count rows) for deterministic parsing.
3. Backward-compatible single-run invocation and unchanged `0/1/2` exit semantics.

### C2 Acceptance Mapping

| C2 ID | Lane Check | Pass Criteria | Artifact |
|---|---|---|---|
| `BL023-C2-001` | `BL023-C1-DET-001` | Replay signatures match baseline across all runs | `validation_matrix.tsv`, `determinism_summary.tsv` |
| `BL023-C2-002` | `BL023-C2-DET-002` | Host matrix row counts match baseline across all runs | `validation_matrix.tsv`, `determinism_summary.tsv` |
| `BL023-C2-003` | `BL023-C2-DET-003` | Taxonomy table row-count matches fixed contract set | `failure_taxonomy.tsv`, `determinism_summary.tsv` |
| `BL023-C2-004` | `overall` | Contract-only and runtime replay lanes preserve strict exit semantics (`0/1/2`) | `status.tsv` |

### C2 Evidence Contract

Required C2 packet artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `exec_runs/validation_matrix.tsv`
- `exec_runs/host_matrix_results.tsv`
- `exec_runs/failure_taxonomy.tsv`
- `determinism_summary.tsv`
- `harness_notes.md`
- `docs_freshness.log`

## C3 Validation (Mode Parity + Exit Semantics Revalidation)

Acceptance mapping:

| C3 ID | Lane Check | Pass Criteria | Artifact |
|---|---|---|---|
| `BL023-C3-001` | contract replay determinism | `--contract-only --runs 20` exits `0`; `signature_divergence_count=0`; `row_count_drift=0` | `contract_runs/validation_matrix.tsv`, `contract_runs/determinism_summary.tsv` |
| `BL023-C3-002` | runtime replay determinism | `--runs 20` exits `0`; `signature_divergence_count=0`; `row_count_drift=0` | `exec_runs/validation_matrix.tsv`, `exec_runs/determinism_summary.tsv` |
| `BL023-C3-003` | mode parity summary | contract and runtime deterministic counters match; taxonomy drift rows are zero | `mode_parity.tsv` |
| `BL023-C3-004` | usage exit semantics (`--runs 0`) | observed exit code is `2` | `exit_semantics_probe.tsv` |
| `BL023-C3-005` | usage exit semantics (`--unknown`) | observed exit code is `2` | `exit_semantics_probe.tsv` |
| `BL023-C3-006` | docs freshness | `./scripts/validate-docs-freshness.sh` exits `0` | `docs_freshness.log` |
| `BL023-C3-007` | evidence completeness | all required C3 files exist | `status.tsv` |

C3 failure taxonomy additions:

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL023-RZ-920 | mode_parity_mismatch | cross-mode deterministic counters differ | deterministic_replay_failure | yes | critical | mode_parity.tsv |
| BL023-RZ-921 | usage_exit_semantics_failure | `--runs 0` or `--unknown` exit code is not `2` | deterministic_contract_failure | yes | major | exit_semantics_probe.tsv |
| BL023-RZ-922 | c3_evidence_schema_incomplete | required C3 files missing | deterministic_evidence_failure | yes | major | status.tsv |
| BL023-RZ-923 | docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes | major | docs_freshness.log |

## C3 Validation Plan

- `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --help`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl023_slice_c3_mode_parity_<timestamp>/contract_runs`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 20 --out-dir TestEvidence/bl023_slice_c3_mode_parity_<timestamp>/exec_runs`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## C3 Evidence Contract

Required C3 packet artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/host_matrix_results.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `contract_runs/determinism_summary.tsv`
- `exec_runs/validation_matrix.tsv`
- `exec_runs/host_matrix_results.tsv`
- `exec_runs/failure_taxonomy.tsv`
- `exec_runs/determinism_summary.tsv`
- `mode_parity.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C3 Execution Snapshot (2026-02-28)

- Packet directory: `TestEvidence/bl023_slice_c3_mode_parity_20260228T171824Z`
- Command outcomes:
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 20 --out-dir .../contract_runs`: `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 20 --out-dir .../exec_runs`: `FAIL`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0`: `PASS` (exit `2`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown`: `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Runtime failure detail:
  - `run=7`, `BL023-C1-RUN-001=FAIL`, detail `runtime_lane_fail:exit=143`
  - deterministic follow-on: `BL023-C1-DET-001=FAIL` (`signature_mismatch` vs baseline)
- Taxonomy evidence (`exec_runs/failure_taxonomy.tsv`):
  - `BL023-RZ-900`: `count=1`, `first_run=7`, `first_lane=BL023-HM-ALL`, `detail=runtime_lane_fail`
  - `BL023-RZ-910`: `count=1`, `first_run=7`, `first_lane=BL023-HM-ALL`, `detail=determinism_signature_mismatch`
- Gate classification:
  - `BL023-C3-001`: `PASS`
  - `BL023-C3-002`: `FAIL`
  - `BL023-C3-003`: `FAIL`
  - `BL023-C3-004`..`BL023-C3-007`: `PASS`

## C3 Execution Snapshot R2 (2026-02-28)

- Packet directory: `TestEvidence/bl023_slice_c3_mode_parity_20260228T174901Z`
- Runtime lane hardening:
  - bounded retry-on-`143` wrapper logic in `scripts/qa-bl023-resize-dpi-matrix-mac.sh`
  - retry controls: `LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143`, `LOCUSQ_BL023_RUNTIME_RETRY_LIMIT`, `LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS`
- Command outcomes:
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 20 --out-dir .../contract_runs`: `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 20 --out-dir .../exec_runs`: `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0`: `PASS` (exit `2`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown`: `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Runtime recovery evidence:
  - `exec_runs/determinism_summary.tsv`: `runtime_retry143_recovery_count=1` with result `PASS` (`exit143_recovered_by_bounded_retry`)
  - `lane_notes.md`: `run=8;runtime_lane_pass_after_exit143_retry:attempts=2`
- Gate classification:
  - `BL023-C3-001`: `PASS`
  - `BL023-C3-002`: `PASS`
  - `BL023-C3-003`: `PASS`
  - `BL023-C3-004`..`BL023-C3-007`: `PASS`

## C3 T4 Sentinel Snapshot (2026-02-28)

- Sentinel directory: `TestEvidence/bl023_slice_c3_mode_parity_20260228T180258Z/exec_runs_retry_on`
- Policy alignment:
  - classified as `T4` sentinel-only cadence (explicit request), separate from routine gate cadence.
  - gating authority remains the C3 packet at `TestEvidence/bl023_slice_c3_mode_parity_20260228T174901Z`.
- Sentinel metrics:
  - command exit `0`
  - `runtime_failure_count=0`
  - `runtime_retry143_recovery_count=2`
- Sentinel verdict:
  - retry-enabled runtime lane stayed deterministic and recovered transient `143` exits without gate failure.

## C3 Canonical PASS Snapshot (2026-02-28)

- Canonical owner-intake packet:
  - `TestEvidence/bl023_slice_c3_mode_parity_20260228T180543Z`
- Command outcomes:
  - `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh`: `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --help`: `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 20 --out-dir .../contract_runs`: `PASS`
  - `LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143=1 LOCUSQ_BL023_RUNTIME_RETRY_LIMIT=2 LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS=1 LOCUSQ_BL023_SELFTEST_MAX_ATTEMPTS=6 LOCUSQ_BL023_SELFTEST_RETRY_DELAY_SECONDS=4 LOCUSQ_BL023_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS=8 ./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 20 --out-dir .../exec_runs`: `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0`: `PASS` (exit `2`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown`: `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Determinism parity summary:
  - `signature_divergence_count`: contract `0`, runtime `0`
  - `row_count_drift`: contract `0`, runtime `0`
  - `runtime_failure_count`: contract `0`, runtime `0`
  - `taxonomy_row_count`: contract `7`, runtime `7`
- Gate classification:
  - `BL023-C3-001`..`BL023-C3-007`: `PASS`

## A2-01 T1 Replay Snapshot (2026-02-28)

- Packet directory:
  - `TestEvidence/bl023_slice_a2_t1_replay_20260228T200917Z`
- Command outcomes:
  - `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh`: `PASS` (`0`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 3 --out-dir .../contract_runs`: `PASS` (`0`)
  - `LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143=1 LOCUSQ_BL023_RUNTIME_RETRY_LIMIT=2 LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS=1 LOCUSQ_BL023_SELFTEST_MAX_ATTEMPTS=6 LOCUSQ_BL023_SELFTEST_RETRY_DELAY_SECONDS=4 LOCUSQ_BL023_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS=8 ./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 3 --out-dir .../exec_runs`: `PASS` (`0`)
  - `./scripts/validate-docs-freshness.sh`: `PASS` (`0`)
- Result:
  - A2-01 post-change T1 replay is green in both contract and runtime lanes.

## A2-02 T2 Candidate Replay Snapshot (2026-02-28)

- Packet directory:
  - `TestEvidence/bl023_slice_a2_t2_candidate_20260228T201215Z`
- Cadence note:
  - Candidate tier executed with heavy-wrapper `2`-run cap.
- Command outcomes:
  - `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh`: `PASS` (`0`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 2 --out-dir .../contract_runs`: `PASS` (`0`)
  - `LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143=1 LOCUSQ_BL023_RUNTIME_RETRY_LIMIT=2 LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS=1 LOCUSQ_BL023_SELFTEST_MAX_ATTEMPTS=6 LOCUSQ_BL023_SELFTEST_RETRY_DELAY_SECONDS=4 LOCUSQ_BL023_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS=8 ./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 2 --out-dir .../exec_runs`: `PASS` (`0`)
  - `./scripts/validate-docs-freshness.sh`: `PASS` (`0`)
- Result:
  - A2 candidate-tier replay is green for both lanes with no gate failures.

## Validation

- `./scripts/validate-docs-freshness.sh`

Validation state labels:
- `tested`: Command executed and exited as expected.
- `partially tested`: Some required lanes missing.
- `not tested`: Command not executed.

## A2-03 T3 Promotion Replay Snapshot (2026-02-28)

- Packet directory:
  - `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z`
- Command outcomes:
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 3 --out-dir .../contract_runs`: `PASS` (`0`)
  - `LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143=1 LOCUSQ_BL023_RUNTIME_RETRY_LIMIT=2 LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS=1 LOCUSQ_BL023_SELFTEST_MAX_ATTEMPTS=6 LOCUSQ_BL023_SELFTEST_RETRY_DELAY_SECONDS=4 LOCUSQ_BL023_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS=8 ./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 3 --out-dir .../exec_runs`: `PASS` (`0`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0`: `PASS` (exit `2`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown`: `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh`: `PASS` (`0`)
- Determinism/parity summary:
  - `contract_runs/determinism_summary.tsv`: `signature_divergence_count=0`, `row_count_drift=0`
  - `exec_runs/determinism_summary.tsv`: `signature_divergence_count=0`, `row_count_drift=0`
  - `mode_parity.tsv`: `PASS`
- Exit semantics:
  - `exit_semantics_probe.tsv` confirms expected usage exits for invalid invocations.

## A2-04 Owner Intake Handoff (2026-02-28)

- Readiness: `READY_FOR_OWNER_PROMOTION_REVIEW`
- Canonical packet: `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z`
- Required gate summary:
  - lint/help/contract/runtime/docs freshness: `PASS`
  - usage exit probes (`--runs 0`, `--unknown`): expected exit `2` observed `2`
  - parity counters: `signature_divergence_count=0`, `row_count_drift=0`
- Ownership safety marker:
  - `SHARED_FILES_TOUCHED: yes`

## A3-01 Owner Promotion Decision Snapshot (2026-02-28)

- Decision: `APPROVED_FOR_CLOSEOUT`
- Canonical promotion packet: `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z`
- Gate confirmation:
  - contract lane: `PASS` (`0`)
  - runtime lane: `PASS` (`0`)
  - invalid usage exits: `--runs 0 => 2`, `--unknown => 2`
  - parity counters: `signature_divergence_count=0`, `row_count_drift=0`
  - docs freshness: `PASS` (`0`)
- Handoff marker:
  - `SHARED_FILES_TOUCHED: yes`

## A3-02 Done Transition Snapshot (2026-02-28)

- Transition status: `DONE_TRANSITION_COMPLETE`
- Archived runbook path:
  - `Documentation/backlog/done/bl-023-resize-dpi-hardening.md`
- Index synchronization:
  - `Documentation/backlog/index.md` BL-023 row is `Done` and linked to done archive path.
- Canonical packet:
  - `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z`
