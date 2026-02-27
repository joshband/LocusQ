Title: BL-038 Calibration Threading and Telemetry
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-27

# BL-038 Calibration Threading and Telemetry

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-038 |
| Priority | P1 |
| Status | In Implementation (Owner Z7 intake accepted H2 + C6r/C7 long-run parity semantics; deterministic contract/execute parity and docs freshness are green) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-026 (Done), BL-034 (Done) |
| Blocks | — |

## Objective

Define deterministic calibration threading boundaries and RT-safe telemetry publication rules so runtime state transitions, timeout/error handling, and evidence output are machine-checkable and replay-stable.

## Slice A1 Scope (Docs-Only Contract)

Slice A1 defines contract surfaces only:
1. Thread ownership boundaries and cross-thread handoff rules.
2. RT-safe telemetry publication invariants.
3. Timeout/error taxonomy classes with deterministic escalation behavior.
4. Deterministic evidence schema for future executable slices.

Out of scope for A1:
- Source implementation changes.
- Script or harness changes.
- Runtime performance claims beyond contract definition.

## Traceability Anchors

- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`

## Thread Ownership Contract

| Thread Domain | Owned Responsibilities | Forbidden Responsibilities | Cross-Thread Contract |
|---|---|---|---|
| `audio_rt` | Render audio, read atomically published calibration state snapshot, publish additive telemetry snapshot fields | Blocking waits, mutex lock/unlock, heap allocation, condition-variable wait/notify | Read-only consumption of latest committed telemetry generation ID |
| `calibration_worker` | Capture/analysis lifecycle, timeout supervision, confidence/clipping computation, deterministic state transitions | Audio callback execution, direct UI object mutation | Publishes snapshot updates through lock-free/atomic handoff only |
| `message_ui` | User intent ingestion (start/abort), diagnostics rendering, non-RT status text + controls | Direct mutation of worker-owned analysis internals without handoff protocol | Sends intent tokens (`start`,`abort`,`reset`) and consumes additive telemetry snapshot |
| `io_aux` (optional background) | File/log persistence and non-RT exports | Any direct dependency from `audio_rt` path | Async-only persistence from copied snapshots |

Thread ownership invariants:
- Exactly one owner per mutable lifecycle state variable.
- Shared state crossing into `audio_rt` must be immutable-by-construction once published for that generation.
- UI must never block waiting for worker completion on audio callback path.

## Deterministic State Machine Contract

### Canonical States

| State ID | Owner Thread | Entry Condition | Exit Condition |
|---|---|---|---|
| `cal_idle` | `calibration_worker` | startup or reset complete | `intent_start` accepted |
| `cal_arm_pending` | `calibration_worker` | start accepted, capture arming in progress | capture starts or `start_timeout` |
| `cal_capturing` | `calibration_worker` | capture engine actively collecting frame windows | enough frames captured, `intent_abort`, or `capture_timeout` |
| `cal_analyzing` | `calibration_worker` | analysis started on captured dataset | result committed or `analysis_timeout` |
| `cal_publish_ready` | `calibration_worker` | analysis result validated and ready for publication | telemetry generation committed |
| `cal_complete` | `calibration_worker` | publication commit successful | new `intent_start` or `intent_reset` |
| `cal_aborting` | `calibration_worker` | abort intent accepted during active work | cleanup complete |
| `cal_error` | `calibration_worker` | deterministic error class raised | `intent_reset` accepted |

### Transition Precedence (same-cycle deterministic order)

1. `intent_abort`
2. `timeout_event`
3. `hard_error_event`
4. `intent_start`
5. `normal_progress_event`

Rule:
- If multiple events share the same logical timestamp, apply precedence order above.
- If precedence and timestamp are equal, preserve source event order.

## RT-Safe Telemetry Publication Contract

### Publication Rules

1. `audio_rt` reads only atomically published snapshot payloads.
2. Worker publishes complete snapshot generations; partial generations are never observable.
3. Telemetry fields are additive and backward-compatible (new fields optional for old consumers).
4. All numeric fields consumed by RT/UI surfaces must be finite (`NaN`/`Inf` forbidden).
5. Sequence counters are monotonic (`snapshot_seq` strictly increasing per publish generation).

### Required Telemetry Fields (A1 contract)

| Field | Type | Producer | RT-safe Constraint |
|---|---|---|---|
| `schema` | string | `calibration_worker` | constant token `locusq-calibration-telemetry-v1` |
| `snapshot_seq` | uint64 | `calibration_worker` | monotonic; no regression |
| `state` | enum string | `calibration_worker` | value in canonical state set only |
| `publish_timestamp_ms` | uint64 | `calibration_worker` | monotonic non-decreasing |
| `analysis_confidence` | float | `calibration_worker` | finite, clamped `[0,1]` |
| `clipping_ratio` | float | `calibration_worker` | finite, clamped `[0,1]` |
| `input_route` | enum string | `calibration_worker` | deterministic route token |
| `timeout_class` | enum string | `calibration_worker` | `none` or taxonomy code |
| `error_class` | enum string | `calibration_worker` | `none` or taxonomy code |
| `stale_ms` | uint32 | consumer-calculated | finite, bounded, non-negative |

### Staleness Contract

- `telemetry_stale_warn_ms = 250`
- `telemetry_stale_fail_ms = 1000`
- If `stale_ms > telemetry_stale_fail_ms`, classify as deterministic publication failure (`BL038-A1-FX-006`).

## Timeout and Error Taxonomy Contract

### Timeout Classes

| Class ID | Trigger | Required State Transition | Blocking |
|---|---|---|---|
| `start_timeout` | arming exceeds configured window | `cal_arm_pending -> cal_error` | yes |
| `capture_timeout` | capture duration exceeds max window | `cal_capturing -> cal_error` | yes |
| `analysis_timeout` | analysis exceeds max window | `cal_analyzing -> cal_error` | yes |
| `publish_timeout` | publish handoff fails within window | `cal_publish_ready -> cal_error` | yes |

### Error Classes

| Class ID | Trigger | Required Escalation | Blocking |
|---|---|---|---|
| `route_unavailable` | configured route missing/inactive | set `error_class` and transition to `cal_error` | yes |
| `non_finite_metric` | confidence/clipping metric non-finite | reject generation and transition to `cal_error` | yes |
| `sequence_regression` | published `snapshot_seq` regresses | classify deterministic contract failure | yes |
| `thread_ownership_violation` | state mutated by non-owner domain | classify deterministic contract failure | yes |
| `worker_shutdown_incomplete` | abort/shutdown leaves worker active past timeout | classify runtime execution failure | yes |

## Slice A1 Acceptance IDs

| Acceptance ID | Requirement | Deterministic Threshold / Rule | Evidence Surface |
|---|---|---|---|
| `BL038-A1-001` | Thread ownership boundaries explicit | owner matrix has 4 domains with forbidden responsibilities | runbook + QA parity |
| `BL038-A1-002` | Canonical calibration state machine fixed | exactly 8 state IDs with owner and transitions | runbook + QA parity |
| `BL038-A1-003` | Transition precedence deterministic | precedence list length=5 and ordered | runbook + QA parity |
| `BL038-A1-004` | RT publication invariants explicit | no lock/alloc/wait on `audio_rt`; atomic full-generation publish | runbook + QA parity |
| `BL038-A1-005` | Required telemetry schema explicit | 10 required fields declared with type/constraint | runbook + evidence schema |
| `BL038-A1-006` | Staleness thresholds explicit | warn/fail thresholds declared and bounded | runbook + QA parity |
| `BL038-A1-007` | Timeout/error taxonomy explicit | timeout+error classes mapped to transitions | runbook + QA parity |
| `BL038-A1-008` | Deterministic evidence schema explicit | required files + TSV columns declared | runbook + QA parity |
| `BL038-A1-009` | Acceptance mapping coherent | IDs match across runbook, QA, acceptance_matrix.tsv | evidence packet |
| `BL038-A1-010` | Docs freshness gate pass | `./scripts/validate-docs-freshness.sh` exit `0` | docs_freshness.log |

## Deterministic Evidence Schema (A1)

Evidence root:
- `TestEvidence/bl038_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `threading_telemetry_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

TSV schema requirements:
- `status.tsv` header: `artifact\tvalue`
- `acceptance_matrix.tsv` header: `acceptance_id\tgate\tthreshold\tresult\tartifact\tnote`
- `failure_taxonomy.tsv` header: `failure_id\tcategory\ttrigger\tclassification\tblocking\tseverity`

## Validation Plan (A1)

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| `BL038-A1-doc-freshness` | automated | `./scripts/validate-docs-freshness.sh` | exit `0` |
| `BL038-A1-contract-parity` | manual | compare runbook + QA acceptance IDs and taxonomy IDs | full parity |

Validation status labels:
- `tested` = validation command executed and pass criteria met.
- `partially tested` = command executed but criteria incomplete.
- `not tested` = command not executed.

## Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Thread-role ambiguity causes nondeterministic behavior | High | Med | freeze owner matrix + violation taxonomy |
| Telemetry drift/regression under retries | High | Med | monotonic seq and precedence rules |
| Timeout handling hidden in ad-hoc logic | Med | Med | explicit timeout class table with required transitions |
| Future slices produce incompatible artifacts | Med | Low | deterministic artifact schema declared in A1 |

## Closeout Checklist (Slice A1)

- [x] Thread ownership matrix documented.
- [x] State machine and transition precedence documented.
- [x] RT-safe telemetry publication invariants defined.
- [x] Timeout and error classes defined.
- [x] Acceptance IDs and deterministic evidence schema defined.
- [x] BL-038 QA contract mapped to this runbook.
- [x] Docs freshness validation executed with evidence.

## Slice A1 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl038_slice_a1_contract_20260227T003006Z/status.tsv`
  - `threading_telemetry_contract.md`
  - `acceptance_matrix.tsv`
  - `failure_taxonomy.tsv`
  - `docs_freshness.log`
- Validation:
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - Slice A1 contract definition complete; BL-038 remains in planning pending implementation slices.

### Owner Intake Sync Z1 (2026-02-27)

- Owner packet:
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_z1_20260227T003434Z/status.tsv`
  - `validation_matrix.tsv`
  - `owner_decisions.md`
  - `handoff_resolution.md`
- Owner replay:
  - `./scripts/validate-docs-freshness.sh` => `PASS`
  - `jq empty status.json` => `PASS`
- Disposition:
  - BL-038 remains `In Planning`; A1 contract intake is complete and implementation slices remain pending.

## Slice B1 Lane Bootstrap Contract (2026-02-27)

Slice B1 introduces a deterministic lane bootstrap for contract-level threading/telemetry checks without build coupling.

Canonical lane script:
- `scripts/qa-bl038-calibration-telemetry-lane-mac.sh`

Canonical scenario contract:
- `qa/scenarios/locusq_bl038_calibration_telemetry_suite.json`

Supported lane options:
- `--contract-only`
- `--execute-suite`
- `--runs <N>`
- `--out-dir <path>`
- `--help|-h`

Strict exit semantics:
- `0` = pass
- `1` = lane/contract failure
- `2` = usage/configuration error

### B1 Acceptance IDs

| Acceptance ID | Requirement | Evidence |
|---|---|---|
| `BL038-B1-001` | Scenario and lane schema are parseable and acceptance IDs are declared | `status.tsv` |
| `BL038-B1-002` | Thread/state ownership contract is complete | `status.tsv` |
| `BL038-B1-003` | Telemetry schema contract is complete | `status.tsv` |
| `BL038-B1-004` | Timeout/error classes and failure taxonomy are complete | `status.tsv` |
| `BL038-B1-005` | Replay hash stability contract is enforced for deterministic reruns | `replay_hashes.tsv` |
| `BL038-B1-006` | Artifact schema completeness is enforced | `status.tsv`, `validation_matrix.tsv` |
| `BL038-B1-007` | Hash input contract excludes nondeterministic fields | `scenario_contract.log`, `replay_hashes.tsv` |
| `BL038-B1-008` | Execution mode contract is explicit and preserved | `status.tsv` |

### B1 Deterministic Replay Contract

For `--runs > 1`:
1. Lane executes deterministic per-run directories (`run_01..run_N`).
2. Replay signatures are computed from semantic status/contract/result/taxonomy content only.
3. Replay fails if signature divergence or row drift exceeds scenario thresholds.
4. Aggregate replay outcomes are emitted in run-level `validation_matrix.tsv` and `replay_hashes.tsv`.

### B1 Required Evidence Bundle

Root path:
- `TestEvidence/bl038_slice_b1_lane_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C2 Determinism Soak Contract (2026-02-27)

Slice C2 performs deterministic replay soak for BL-038 threading/telemetry contract checks.

Canonical soak command:
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl038_slice_c2_soak_<timestamp>/contract_runs`

Strict expectations:
1. `signature_divergence = 0`
2. `row_drift = 0`
3. `runtime_execution_failure = 0`
4. `deterministic_contract_failure = 0`
5. `missing_result_artifact = 0`

### C2 Acceptance IDs

| Acceptance ID | Requirement | Evidence |
|---|---|---|
| `BL038-C2-001` | Soak summary is emitted and result is `PASS` for `runs=10` | `soak_summary.tsv` |
| `BL038-C2-002` | Replay hash signatures remain stable across all soak runs | `contract_runs/replay_hashes.tsv`, `contract_runs/status.tsv` |
| `BL038-C2-003` | Replay row signatures remain stable across all soak runs | `contract_runs/replay_hashes.tsv`, `contract_runs/status.tsv` |

### C2 Validation Plan

- `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl038_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C2 Evidence Contract

Required path:
- `TestEvidence/bl038_slice_c2_soak_<timestamp>/`

Required artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C2 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl038_slice_c2_soak_20260227T010825Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `soak_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl038_slice_c2_soak_20260227T010825Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - deterministic soak replay (`runs=10`) is stable with zero signature divergence and zero row drift.

## Slice C3 Replay Sentinel Contract (2026-02-27)

Slice C3 extends deterministic replay validation to a 20-run sentinel packet for BL-038 threading/telemetry contract closeout confidence.

Canonical replay sentinel command:
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c3_replay_sentinel_<timestamp>/contract_runs`

Strict expectations:
1. `signature_divergence = 0`
2. `row_drift = 0`
3. `runtime_execution_failure = 0`
4. `deterministic_contract_failure = 0`
5. `missing_result_artifact = 0`

### C3 Acceptance IDs

| Acceptance ID | Requirement | Evidence |
|---|---|---|
| `BL038-C3-001` | Replay sentinel summary is emitted and result is `PASS` for `runs=20` | `replay_sentinel_summary.tsv` |
| `BL038-C3-002` | Replay hash signatures remain stable across all replay sentinel runs | `contract_runs/replay_hashes.tsv`, `contract_runs/status.tsv` |
| `BL038-C3-003` | Replay row signatures remain stable across all replay sentinel runs | `contract_runs/replay_hashes.tsv`, `contract_runs/status.tsv` |

### C3 Validation Plan

- `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c3_replay_sentinel_<timestamp>/contract_runs`
- `./scripts/validate-docs-freshness.sh`

### C3 Evidence Contract

Required path:
- `TestEvidence/bl038_slice_c3_replay_sentinel_<timestamp>/`

Required artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C3 Execution Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl038_slice_b1_lane_20260227T005557Z/`
  - `TestEvidence/bl038_slice_c2_soak_20260227T010825Z/`
- Evidence packet:
  - `TestEvidence/bl038_slice_c3_replay_sentinel_20260227T012154Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c3_replay_sentinel_20260227T012154Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - replay sentinel (`runs=20`) is stable with zero signature divergence and zero row drift.

## Slice C5 Exit-Semantics Guard Contract (2026-02-27)

Slice C5 adds deterministic guardrails for 20-run replay sentinel closeout and strict usage/exit semantics validation.

Canonical replay guard command:
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c5_semantics_<timestamp>/contract_runs`

Strict expectations:
1. `signature_divergence = 0`
2. `row_drift = 0`
3. `runtime_execution_failure = 0`
4. `deterministic_contract_failure = 0`
5. `missing_result_artifact = 0`
6. `--runs 0` exits with code `2`
7. `--unknown` exits with code `2`

### C5 Acceptance IDs

| Acceptance ID | Requirement | Evidence |
|---|---|---|
| `BL038-C5-001` | Deterministic replay guard emits pass summary at `runs=20` | `replay_sentinel_summary.tsv`, `contract_runs/status.tsv` |
| `BL038-C5-002` | Usage probe `--runs 0` returns strict usage exit code `2` | `exit_semantics_probe.tsv` |
| `BL038-C5-003` | Usage probe `--unknown` returns strict usage exit code `2` | `exit_semantics_probe.tsv` |

### C5 Validation Plan

- `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c5_semantics_<timestamp>/contract_runs`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C5 Evidence Contract

Required path:
- `TestEvidence/bl038_slice_c5_semantics_<timestamp>/`

Required artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C5 Execution Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl038_slice_c4_soak_20260227T013744Z/`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z5_20260227T014558Z/`
- Evidence packet:
  - `TestEvidence/bl038_slice_c5_semantics_20260227T015217Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c5_semantics_20260227T015217Z/contract_runs` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (external non-owned metadata violation)
- Result:
  - deterministic replay and exit-semantics probes pass; slice remains blocked on docs freshness issue outside C5 ownership.

## Slice C5b Execution Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl038_slice_c5_semantics_20260227T015217Z/`
  - `TestEvidence/bl038_slice_c4_soak_20260227T013744Z/`
- Evidence packet:
  - `TestEvidence/bl038_slice_c5b_semantics_20260227T020616Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c5b_semantics_20260227T020616Z/contract_runs` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - C5 shared-files safety reconcile packet is green with deterministic replay and strict usage exits preserved.
  - `SHARED_FILES_TOUCHED` is explicitly `no`.

## Slice C6 Execute-Mode Parity + Exit Guard Contract (2026-02-27)

Slice C6 extends C5b by requiring parity across both lane execution modes while preserving strict usage-exit semantics.

Canonical parity commands:
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_<timestamp>/contract_runs_execute`

Strict expectations:
1. contract-only summary result is `PASS`.
2. execute-suite summary result is `PASS`.
3. Both modes have `signature_divergence=0` and `row_drift=0`.
4. Both modes have `runtime_execution_failure=0`, `deterministic_contract_failure=0`, and `missing_result_artifact=0`.
5. `--runs 0` exits with code `2`.
6. `--unknown` exits with code `2`.

### C6 Acceptance IDs

| Acceptance ID | Requirement | Evidence |
|---|---|---|
| `BL038-C6-001` | mode parity summary shows deterministic pass for both contract-only and execute-suite (`runs=20`) | `mode_parity.tsv`, `replay_sentinel_summary.tsv` |
| `BL038-C6-002` | usage probe `--runs 0` returns strict usage exit code `2` | `exit_semantics_probe.tsv` |
| `BL038-C6-003` | usage probe `--unknown` returns strict usage exit code `2` | `exit_semantics_probe.tsv` |

### C6 Validation Plan

- `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C6 Evidence Contract

Required path:
- `TestEvidence/bl038_slice_c6_mode_parity_<timestamp>/`

Required artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C6 Execution Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl038_slice_c4_soak_20260227T013744Z/`
  - `TestEvidence/bl038_slice_c5b_semantics_20260227T020616Z/`
- Evidence packet:
  - `TestEvidence/bl038_slice_c6_mode_parity_20260227T025802Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_20260227T025802Z/contract_runs_contract` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl038_slice_c6_mode_parity_20260227T025802Z/contract_runs_execute` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (external non-owned metadata violations under `Documentation/Calibration POC/`)
- Result:
  - execute-mode parity and strict usage-exit semantics are deterministic and passing.
  - slice remains blocked on external docs freshness issues outside C6 ownership.

## Slice C6r Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl038_slice_c6r_mode_parity_20260227T031054Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl038_slice_c6r_mode_parity_20260227T031054Z/contract_runs_contract` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl038_slice_c6r_mode_parity_20260227T031054Z/contract_runs_execute` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - execute-mode parity and strict usage-exit semantics remain deterministic and passing.
  - docs freshness gate is green in the C6r post-H2 recheck packet.

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
  - BL-038 remains `In Implementation`; C5b shared-files reconcile packet is accepted and prior external docs blocker is cleared.

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
  - BL-038 remains `In Implementation`; C6r packet is accepted and H2 metadata hygiene closure is integrated.

## Slice C7 Long-Run Parity Sentinel Contract (2026-02-27)

Slice C7 deepens C6r confidence by requiring long-run deterministic parity across both lane modes with strict usage-exit behavior preserved.

Canonical parity commands:
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_<timestamp>/contract_runs_execute`

Strict expectations:
1. contract-only summary result is `PASS`.
2. execute-suite summary result is `PASS`.
3. both modes report `signature_divergence=0` and `row_drift=0`.
4. both modes report zero `runtime_execution_failure`, `deterministic_contract_failure`, and `missing_result_artifact`.
5. `--runs 0` exits with code `2`.
6. `--unknown` exits with code `2`.

### C7 Acceptance IDs

| Acceptance ID | Requirement | Evidence |
|---|---|---|
| `BL038-C7-001` | long-run mode parity summary shows deterministic pass for contract-only and execute-suite (`runs=50`) | `mode_parity.tsv`, `replay_sentinel_summary.tsv` |
| `BL038-C7-002` | usage probe `--runs 0` returns strict usage exit code `2` | `exit_semantics_probe.tsv` |
| `BL038-C7-003` | usage probe `--unknown` returns strict usage exit code `2` | `exit_semantics_probe.tsv` |

### C7 Validation Plan

- `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_<timestamp>/contract_runs_contract`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_<timestamp>/contract_runs_execute`
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

### C7 Evidence Contract

Required path:
- `TestEvidence/bl038_slice_c7_longrun_parity_<timestamp>/`

Required artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs_contract/validation_matrix.tsv`
- `contract_runs_contract/replay_hashes.tsv`
- `contract_runs_contract/failure_taxonomy.tsv`
- `contract_runs_execute/validation_matrix.tsv`
- `contract_runs_execute/replay_hashes.tsv`
- `mode_parity.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C7 Execution Snapshot (2026-02-27)

- Input handoffs resolved:
  - `TestEvidence/bl038_slice_c6r_mode_parity_20260227T031054Z/`
  - `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z7_20260227T032802Z/`
- Evidence packet:
  - `TestEvidence/bl038_slice_c7_longrun_parity_20260227T033937Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs_contract/validation_matrix.tsv`
  - `contract_runs_contract/replay_hashes.tsv`
  - `contract_runs_contract/failure_taxonomy.tsv`
  - `contract_runs_execute/validation_matrix.tsv`
  - `contract_runs_execute/replay_hashes.tsv`
  - `mode_parity.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl038-calibration-telemetry-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_20260227T033937Z/contract_runs_contract` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --execute-suite --runs 50 --out-dir TestEvidence/bl038_slice_c7_longrun_parity_20260227T033937Z/contract_runs_execute` => `PASS`
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --runs 0` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/qa-bl038-calibration-telemetry-lane-mac.sh --unknown` => expected exit `2`, observed `2` (`PASS`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - long-run execute-mode parity and strict usage-exit semantics remain deterministic and passing at 50-run depth.
