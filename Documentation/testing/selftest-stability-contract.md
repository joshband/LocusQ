Title: Selftest Stability Contract
Document Type: Testing Contract
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-03-18

# Selftest Stability Contract

## Purpose

Define the stable output, retry, locking, and failure rules for the standalone selftest harness and wrappers that consume it.

## Authority

- primary script: `scripts/standalone-ui-selftest-production-p0-mac.sh`
- example downstream consumer: `scripts/qa-bl009-headphone-contract-mac.sh`

## Core Contract

### Required top-level log keys

- `selftest_ts`
- `app_exec`
- `result_json`
- `timeout_seconds`
- `selftest_scope`
- `result_ready`
- `terminal_failure_reason` on failure
- `app_exit_code` when available
- `artifact` on pass

### Required structured artifacts

| Artifact | Purpose | Required fields |
|---|---|---|
| `attempt_status_table=<path>` | one row per attempt | `attempt`, `status`, `terminal_failure_reason`, `app_pid`, `app_exit_code`, `app_signal`, `app_signal_name`, `result_wait_seconds`, `result_json`, `crash_report_path`, `error_reason` |
| `metadata_json=<path>` | final summary metadata | final status, exit data, retry settings, lock telemetry, launch telemetry, drain telemetry, grace/settle telemetry |

### Additive telemetry families

| Family | Required examples |
|---|---|
| lock telemetry | `lock_path`, `lock_wait_seconds`, `lock_wait_result`, `lock_stale_recovered` |
| launch telemetry | `launch_mode_requested`, `launch_mode_used`, `launch_mode_fallback_reason`, `launch_ready_delay_seconds` |
| drain telemetry | `prelaunch_drain_result`, `prelaunch_drain_seconds`, `prelaunch_drain_remaining_pids` |
| result-grace telemetry | `result_after_exit_grace_seconds`, `result_post_exit_grace_used`, `result_post_exit_grace_wait_seconds` |
| retry telemetry | `max_attempts_configured`, `retry_delay_seconds_configured`, `auto_assertion_retry_applied`, `auto_assertion_retry_reason` |

## Safety Rules

### Cleanup

Before every run, the harness must remove:
- target `result_json`
- target run log
- target attempt-status table
- target metadata file
- stale per-attempt files for the same base name

### Retry

| Env var | Default | Rule |
|---|---|---|
| `LOCUSQ_UI_SELFTEST_MAX_ATTEMPTS` | `1` | preserves legacy single-attempt behavior unless explicitly raised |
| `LOCUSQ_UI_SELFTEST_RETRY_DELAY_SECONDS` | `1` | bounded retry delay |
| `LOCUSQ_UI_SELFTEST_AUTO_ASSERTION_RETRY_ENABLED` | `1` | enables bounded additive auto-profile behavior |
| `LOCUSQ_UI_SELFTEST_AUTO_ASSERTION_RETRY_MAX_ATTEMPTS` | `2` | hard cap for auto-profile retries |
| `LOCUSQ_UI_SELFTEST_AUTO_ASSERTION_RETRY_DELAY_SECONDS` | `2` | bounded additive retry delay |

Retry rules:
- every attempt must append a row to the attempt table
- retries must never turn a hard fail into a silent pass
- explicit caller-provided max attempts always wins
- effective retry behavior must be logged and written to metadata

### Single-instance serialization

Rules:
- global lock is required before cleanup and launch
- stale lock recovery is allowed only when the owner PID is gone or mismatched
- lock age alone must not steal a live matching lock
- lock wait timeout is a hard fail: `single_instance_lock_timeout`

Environment controls:
- `LOCUSQ_UI_SELFTEST_LOCK_PATH`
- `LOCUSQ_UI_SELFTEST_LOCK_WAIT_TIMEOUT_SECONDS`
- `LOCUSQ_UI_SELFTEST_LOCK_STALE_SECONDS`
- `LOCUSQ_UI_SELFTEST_LOCK_POLL_SECONDS`

### Launch mode

| Env var | Allowed values | Default |
|---|---|---|
| `LOCUSQ_UI_SELFTEST_LAUNCH_MODE` | `direct`, `open` | `direct` |

Rules:
- chosen mode must be logged and persisted
- invalid values must fall back deterministically to a valid mode
- launch-mode reporting is additive only

### Prelaunch process drain

Rules:
- no stale `LocusQ` process may remain before launch
- drain is two-phase: bounded `TERM`, then bounded `KILL` only if needed
- zero remaining PIDs must stay stable for consecutive polls before launch
- unresolved residual process state is a hard fail: `prelaunch_process_drain_timeout`

Environment controls:
- `LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS`
- `LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_STABLE_POLLS`
- `LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_STABLE_POLL_SECONDS`

### Post-exit result grace

Rules:
- if the app exits before JSON appears, wait a bounded grace window before classifying failure
- default grace window is `3` seconds
- missing JSON after grace still fails

Environment controls:
- `LOCUSQ_UI_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS`
- `LOCUSQ_UI_SELFTEST_RESULT_AFTER_EXIT_GRACE_POLL_SECONDS`

### Result JSON settle

Rules:
- after result discovery, require two consecutive stable reads
- when `jq` is available, require valid JSON parse
- if settle timeout expires, fail with `selftest_payload_invalid_json`
- if `jq` is unavailable, fail with `selftest_payload_parser_unavailable`

Environment controls:
- `LOCUSQ_UI_SELFTEST_RESULT_JSON_SETTLE_TIMEOUT_SECONDS` default `2`
- `LOCUSQ_UI_SELFTEST_RESULT_JSON_SETTLE_POLL_SECONDS` default `0.1`

### Targeted assertion retry

Known targeted checks:
- `UI-P1-029B`
- `UI-07`
- `UI-P1-025E`

Rules:
- applies only when max attempts was not explicitly overridden and auto retry is enabled
- may add `+1` retry per targeted failure up to the hard cap
- strict fail behavior stays intact if the issue persists

Environment control:
- `LOCUSQ_UI_SELFTEST_TARGETED_CHECK_MAX_ATTEMPTS` default `4`

## Failure Contract

| Failure | Meaning |
|---|---|
| `single_instance_lock_timeout` | lock wait exceeded bounded timeout |
| `prelaunch_process_drain_timeout` | stale process drain failed |
| `app_exited_before_result` | app exited and no valid result arrived after grace |
| `selftest_payload_invalid_json` | result file appeared but never settled into valid JSON |
| `selftest_payload_parser_unavailable` | JSON parser unavailable, so result could not be trusted |
| `selftest_payload_not_ok` | payload assertions failed |

## Evidence Expectations

Every stable selftest packet should make it easy to answer:
- what attempt ran
- whether retries were applied
- whether lock or drain handling intervened
- which launch mode was used
- why the final result passed or failed

## Archive Note

The long telemetry-by-telemetry narrative was removed from the active contract on purpose. If the full legacy wording is needed again, preserve or restore it from the matching archive copy under `Documentation/archive/2026-03-18-doc-surface-consolidation/`.
