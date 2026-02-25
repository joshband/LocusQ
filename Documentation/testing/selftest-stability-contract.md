Title: Selftest Stability Contract
Document Type: Testing Contract
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

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
- `launch_mode_requested`
- `launch_mode_used`
- `launch_mode_fallback_reason`
- `prelaunch_drain_result`
- `prelaunch_drain_seconds`
- `prelaunch_drain_forced_kill`
- `prelaunch_drain_remaining_pids`

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
- `launchModeRequested`
- `launchModeUsed`
- `launchModeFallbackReason`
- `prelaunchDrainResult`
- `prelaunchDrainSeconds`
- `prelaunchDrainForcedKill`
- `prelaunchDrainRemainingPids`

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
- stale lock recovery is allowed only when owner PID is gone or lock age exceeds configured stale window.
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
- if residual process(es) remain, harness tries bounded drain and bounded force-kill once.
- unresolved residual process state is a deterministic hard fail (`prelaunch_process_drain_timeout`).
- drain telemetry is additive and must be emitted to run log and metadata.

Environment controls:
- `LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS`

## Failure Taxonomy

Standard terminal reasons:
- `app_exited_before_result`
- `result_json_missing_after_<timeout>s`
- `selftest_payload_not_ok`
- `single_instance_lock_timeout`
- `prelaunch_process_drain_timeout`
- `launch_mode_failed_<mode>`
- `none` (pass)

Wrappers should log failures with at least:
- terminal reason
- exit code
- signal number/name (if present)
- crash report path (if found)

## Wrapper Consumption Rules

Wrappers (for example BL-009 contract lane) must:
- parse and preserve `metadata_json` and `attempt_status_table` when present
- report structured failure detail in status TSV rather than a log-only fail marker
- keep lane failure strict: structured diagnostics improve triage but do not downgrade fail semantics
