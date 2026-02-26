Title: Selftest Stability Contract
Document Type: Testing Contract
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-26

# Selftest Stability Contract

## Scope

Defines deterministic output and failure-taxonomy requirements for:
- `scripts/standalone-ui-selftest-production-p0-mac.sh`
- wrappers that consume selftest output (for example `scripts/qa-bl009-headphone-contract-mac.sh`)

## Backward-Compatible Output Keys

The harness must continue emitting these top-level log keys:
- `selftest_ts`
- `app_exec`
- `result_json`
- `timeout_seconds`
- `selftest_scope`
- `result_ready`
- `terminal_failure_reason` (on failure)
- `app_exit_code` (when available)
- `artifact` (on pass)

## Structured Diagnostics

The harness publishes two structured artifacts and logs their paths:
- `attempt_status_table=<path>`: TSV with one row per attempt.
- `metadata_json=<path>`: final-attempt summary metadata.

The harness now also publishes additive runtime-serialization telemetry:
- `lock_path`
- `lock_wait_seconds`
- `lock_wait_result`
- `lock_wait_polls`
- `lock_stale_recovered`
- `lock_stale_recovery_reason`
- `lock_owner_matches_selftest`
- `launch_mode_requested`
- `launch_mode_used`
- `launch_mode_fallback_reason`
- `prelaunch_drain_result`
- `prelaunch_drain_seconds`
- `prelaunch_drain_forced_kill`
- `prelaunch_drain_remaining_pids`
- `prelaunch_drain_term_sent`
- `prelaunch_drain_term_window_seconds`
- `prelaunch_drain_kill_window_seconds`
- `result_after_exit_grace_seconds`
- `result_post_exit_grace_used`
- `result_post_exit_grace_wait_seconds`
- `app_exit_status_source` (additive exit-observation taxonomy)

Attempt status table schema:
- `attempt`
- `status` (`pass|fail`)
- `terminal_failure_reason`
- `app_pid`
- `app_exit_code`
- `app_signal`
- `app_signal_name`
- `result_wait_seconds`
- `result_json`
- `crash_report_path`
- `error_reason`

Final metadata schema (JSON):
- `status`
- `terminalFailureReason`
- `appPid`
- `appExitCode`
- `appSignal`
- `appSignalName`
- `appExitStatusSource`
- `crashReportPath`
- `attemptStatusTable`
- `maxAttempts`
- `attemptsRun`
- `lockPath`
- `lockWaitSeconds`
- `lockWaitResult`
- `lockWaitPolls`
- `lockStaleRecovered`
- `lockStaleRecoveryReason`
- `lockOwnerMatchesSelftest`
- `launchModeRequested`
- `launchModeUsed`
- `launchModeFallbackReason`
- `prelaunchDrainResult`
- `prelaunchDrainSeconds`
- `prelaunchDrainForcedKill`
- `prelaunchDrainRemainingPids`
- `prelaunchDrainTermSent`
- `prelaunchDrainTermWindowSeconds`
- `prelaunchDrainKillWindowSeconds`
- `resultAfterExitGraceSeconds`
- `resultPostExitGraceUsed`
- `resultPostExitGraceWaitSeconds`

## Cleanup Contract

Before every run, the harness must remove:
- target `result_json`
- target run log
- target attempt-status table
- target metadata file
- stale per-attempt files for the same base name

This prevents stale outputs from being misclassified as fresh pass artifacts.

## Retry Contract

Optional retry behavior is controlled by:
- `LOCUSQ_UI_SELFTEST_MAX_ATTEMPTS` (default `1`)
- `LOCUSQ_UI_SELFTEST_RETRY_DELAY_SECONDS` (default `1`)

Rules:
- default `1` preserves legacy single-attempt behavior.
- every attempt must append a row to the attempt status table.
- retries must never convert a hard fail into a silent pass without explicit successful attempt evidence.

## Single-Instance Serialization Contract

The standalone selftest harness must serialize concurrent invocations via a global lock.

Rules:
- lock acquisition is required before per-run cleanup and launch.
- stale lock recovery is allowed only when owner PID is gone or owner PID does not match the selftest harness identity.
- lock age alone must not steal an active lock from a live matching owner process.
- lock wait timeout is a hard fail with deterministic reason `single_instance_lock_timeout`.
- lock telemetry must be emitted in both run log and metadata.

Environment controls (all optional/additive):
- `LOCUSQ_UI_SELFTEST_LOCK_PATH`
- `LOCUSQ_UI_SELFTEST_LOCK_WAIT_TIMEOUT_SECONDS`
- `LOCUSQ_UI_SELFTEST_LOCK_STALE_SECONDS`
- `LOCUSQ_UI_SELFTEST_LOCK_POLL_SECONDS`

Default behavior remains backward-compatible for existing callers.

## Launch Mode Contract

Launch behavior is selectable via:
- `LOCUSQ_UI_SELFTEST_LAUNCH_MODE=direct|open` (default: `direct`)

Rules:
- the chosen mode must be logged and persisted to metadata.
- invalid/unsupported launch mode inputs must not crash the harness; they must resolve deterministically to a valid mode with fallback reason.
- launch-mode reporting is additive and must not break legacy parser fields.

## Prelaunch Process Drain Contract

Before each attempt, harness must verify no stale `LocusQ` process remains.

Rules:
- if residual process(es) remain, harness performs deterministic two-phase drain:
  1. bounded `TERM` window
  2. bounded `KILL` window (only if needed)
- unresolved residual process state is a deterministic hard fail (`prelaunch_process_drain_timeout`).
- drain telemetry is additive and must be emitted to run log and metadata.

Environment controls:
- `LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS`

## Post-Exit Result Grace Contract

To reduce false `app_exited_before_result` classifications caused by near-simultaneous process exit and JSON flush, the harness applies a bounded post-exit grace wait.

Rules:
- if the app process exits during result wait and JSON is still absent, the harness waits an additional bounded grace window before classifying failure.
- default grace window: `3` seconds.
- grace handling is additive telemetry and does not weaken PASS semantics: missing JSON after grace still fails.

Environment controls:
- `LOCUSQ_UI_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS`

## Result JSON Settle Contract

To avoid classifying partially-written payload files as `selftest_payload_not_ok`, the harness applies a bounded JSON settle window after result-file discovery.

Rules:
- once result JSON appears, harness waits for two consecutive stable reads and (when `jq` is available) valid JSON parse.
- settle timeout remains strict: if JSON is still unparseable at timeout, harness fails with `selftest_payload_invalid_json`.
- settle telemetry must be emitted in run log and metadata (`result_json_settle_result`, `result_json_settle_wait_seconds`, `result_json_settle_polls`).

Environment controls:
- `LOCUSQ_UI_SELFTEST_RESULT_JSON_SETTLE_TIMEOUT_SECONDS` (default `2`)
- `LOCUSQ_UI_SELFTEST_RESULT_JSON_SETTLE_POLL_SECONDS` (default `0.1`)

## Failure Taxonomy

Standard terminal reasons:
- `app_exited_before_result`
- `result_json_missing_after_<timeout>s`
- `selftest_payload_not_ok`
- `selftest_payload_invalid_json`
- `single_instance_lock_timeout`
- `prelaunch_process_drain_timeout`
- `launch_mode_failed_<mode>`
- `none` (pass)

Payload-failure taxonomy:
- harness emits additive machine-readable artifact `failure_taxonomy_path` (`*.failure_taxonomy.tsv`) with per-attempt payload diagnostics.
- required fields include: `payload_reason_code`, `payload_failing_check`, `payload_snippet_path`, `payload_pointer_path`, `payload_status`, `payload_ok`.
- `payload_reason_code` examples:
  - `failing_check_assertion`
  - `payload_error_without_check`
  - `payload_ok_false`
  - `invalid_json_payload`
  - `not_payload_failure`
- for payload failures, harness must emit a deterministic snippet artifact (`*.payload_failure_snippet.json`) and record the path.
- `payload_pointer_path` must point to the attempt result JSON used for payload evaluation.

Exit-observation taxonomy (`app_exit_status_source` / `appExitStatusSource`):
- `not_applicable_pass`: pass path; harness validated payload and does not classify process exit as failure data.
- `child_wait`: harness observed child-process exit status via `wait`.
- `external_non_child`: harness launched/observed process outside shell-child ownership; exit code is intentionally left empty.
- `child_pid_missing` / `not_observed`: fallback categories for incomplete observation paths.

Taxonomy consistency rule (Slice Z6):
- `terminal_failure_reason=none` must not rely on synthetic `app_exit_code=127` bookkeeping.
- For pass rows where exit status is not semantically relevant or not wait-observable, `app_exit_code` should be empty and `app_exit_status_source` must explain why.

Wrappers should log failures with at least:
- terminal reason
- exit code
- signal number/name (if present)
- crash report path (if found)
- payload reason code / failing check / snippet path (for payload-classified failures)

## Wrapper Consumption Rules

Wrappers (for example BL-009 contract lane) must:
- parse and preserve `metadata_json`, `attempt_status_table`, and `failure_taxonomy_path` when present
- report structured failure detail in status TSV rather than a log-only fail marker
- keep lane failure strict: structured diagnostics improve triage but do not downgrade fail semantics

## BL-029 Reliability Gate Runner Contract (Slice P4)

`scripts/qa-bl029-reliability-gate-mac.sh` is the deterministic top-level gate for BL-029 reliability closeout.

Required execution order:
1. build standalone
2. BL-029 QA lane
3. BL-029 scoped selftest x10
4. BL-009 scoped selftest x5
5. docs freshness

Required machine-readable outputs:
- `status.tsv`
- `hard_criteria.tsv`
- `failure_taxonomy.tsv`
- `gate_contract.md`

Hard criteria expectations:
- all required stages must execute.
- BL-029 selftest pass count must equal required run count.
- BL-009 selftest pass count must equal required run count.
- docs freshness must pass.

Failure taxonomy expectations:
- include `terminal_failure_reason` distribution.
- include `app_exit_code`, `app_signal`, and `app_signal_name` distributions.
- preserve explicit counts for `app_exited_before_result` when present.

Exit behavior:
- non-zero exit when any hard criterion is unmet.

## BL-029 Reliability Gate CI Wiring Contract (Slice Z5)

Workflow path:
- `.github/workflows/qa_harness.yml`

Hard-gate wiring requirements:
1. CI job `qa-bl029-reliability-gate` runs on `macos-latest` with `needs: qa-critical`.
2. Job executes:
   - `BL029_GATE_OUT_DIR="qa_output/bl029_reliability_gate" ./scripts/qa-bl029-reliability-gate-mac.sh`
3. Job publishes artifacts from `qa_output/bl029_reliability_gate/` (including `status.tsv`, `hard_criteria.tsv`, `failure_taxonomy.tsv`, `gate_contract.md`).
4. Downstream gated macOS validation (`qa-pluginval-seeded-stress`) must depend on `qa-bl029-reliability-gate`, not directly on `qa-critical`.
5. Any non-zero exit from the reliability gate job is a hard CI fail for BL-029 readiness.

## HX-05 Payload Budget Soak Lane Contract (Slice B)

Scope:
- deterministic QA contract for payload-budget enforcement validation
- docs-only specification for implementation slices (no runtime changes in this slice)

Proposed lane identity:
- `HX05-LANE-SOAK`

Proposed lane command contract (for implementation slices to realize):
- `./scripts/qa-hx05-payload-budget-soak-mac.sh`

### Sample Window Schedule

| Window ID | Duration | Included In Scoring | Notes |
|---|---:|---|---|
| `W0_warmup` | 10 s | no | startup warmup only |
| `W1_nominal` | 60 s | yes | baseline steady state |
| `W2_burst` | 30 s | yes | emitter churn / burst pressure |
| `W3_sustained_stress` | 120 s | yes | prolonged pressure and recovery |

### Deterministic Pass/Fail Contract

Required limits for scored windows (`W1..W3`):
- `max(bytes) <= 65536`
- `p95(bytes) <= 32768`
- `max(cadence_hz) <= 60`
- over-soft burst length `<= 8 snapshots` and `<= 500 ms`
- tier transitions conform to policy (`normal`, `degrade_t1`, `degrade_t2_safe`)

The lane must fail if any threshold is violated.

### Required Artifacts

| Artifact | Required Fields |
|---|---|
| `payload_metrics.tsv` | `window_id`, `snapshot_seq`, `utc_ms`, `bytes`, `tier`, `burst_count` |
| `transport_cadence.tsv` | `window_id`, `window_start_ms`, `window_end_ms`, `cadence_hz`, `over_soft_count` |
| `budget_tier_events.tsv` | `snapshot_seq`, `window_id`, `from_tier`, `to_tier`, `reason`, `compliance_streak` |
| `taxonomy_table.tsv` | `failure_code`, `count`, `first_snapshot_seq`, `first_window_id` |
| `status.tsv` | `lane`, `result`, `exit_code`, `timestamp`, `artifact` |

### Failure Taxonomy (HX-05)

Terminal taxonomy values:
- `oversize_hard_limit`
- `oversize_soft_limit`
- `burst_overrun`
- `cadence_violation`
- `degrade_tier_mismatch`
- `none` (pass)

Consistency rule:
- For deterministic replay input, taxonomy counts and first-failure ordering must remain stable.

## HX-05 Payload Budget Soak Harness Contract (Slice C)

Implemented script:
- `scripts/qa-hx05-payload-budget-soak-mac.sh`

### Invocation Contract

Required:
- `--input-dir <path>`

Optional:
- `--out-dir <path>`
- `--label <name>`
- `--help`

Strict exit contract:
- `0`: pass
- `1`: threshold/schema/policy violation
- `2`: usage error

### Deterministic Evaluation Contract

Input artifacts (required):
- `payload_metrics.tsv`
- `transport_cadence.tsv`
- `budget_tier_events.tsv`
- `taxonomy_table.tsv`

Scored windows:
- `W1_nominal`
- `W2_burst`
- `W3_sustained_stress`

Threshold checks:
- `max(bytes) <= 65536`
- nearest-rank `p95(bytes) <= 32768`
- `max(cadence_hz) <= 60`
- burst bounds `<= 8 snapshots` and `<= 500 ms`
- transition policy compatibility for `normal`, `degrade_t1`, `degrade_t2_safe`

### Machine-Readable Outputs

Harness output bundle must include:
- `status.tsv` (`check`, `result`, `detail`, `artifact`)
- `taxonomy_table.tsv` (`failure_code`, `count`, `first_snapshot_seq`, `first_window_id`)
- `qa_lane_contract.md`

Taxonomy ordering (stable):
1. `oversize_hard_limit`
2. `oversize_soft_limit`
3. `cadence_violation`
4. `burst_overrun`
5. `degrade_tier_mismatch`
6. `schema_invalid`
7. `none`
