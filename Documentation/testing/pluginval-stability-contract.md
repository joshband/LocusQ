Title: Pluginval Stability Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-26

# BL-030 RL-06 Pluginval Reliability Harness (Slice L3)

## Purpose

Define deterministic RL-06 pluginval replay behavior with strict pass/fail thresholding, machine-readable per-run terminal taxonomy (including command/exit/signal), bounded deterministic retry/backoff, and backward-compatible lane CLI behavior.

## Canonical Command

- `./scripts/qa-bl030-pluginval-stability-mac.sh --runs 10 --out-dir TestEvidence/bl030_rl06_stability_l3_<timestamp>`

## Inputs

- `--runs <N>`: number of pluginval invocations (default `5`)
- `--out-dir <path>`: evidence output directory
- `--plugin-path <path>`: plugin under test (default `build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3`)
- `--pluginval-bin <path>`: pluginval executable path
- `--timeout-ms <N>`: pluginval timeout (default `30000`)
- `--max-retries-per-run <N>`: bounded retry budget per run (default `0`)
- `--retry-backoff-ms <N>`: fixed deterministic retry backoff (default `250`)

## Required Outputs (L3)

- `status.tsv`
- `validation_matrix.tsv`
- `pluginval_runs.tsv`
- `failure_taxonomy.tsv`
- `rl06_hardening_notes.md`
- `command_transcript.log`

Compatibility outputs retained:
- `replay_runs.tsv`
- `harness_contract.md`

Owner replay packet normally also captures:
- `docs_freshness.log` (from `./scripts/validate-docs-freshness.sh`)

## `pluginval_runs.tsv` Schema

Columns (exact order):
- `run_index`
- `timestamp_utc`
- `exit_code`
- `terminal_reason`
- `terminal_reason_class`
- `terminal_signal_number`
- `classification`
- `crash_report_present`
- `crash_report_path`
- `plugin_path`
- `log_path`
- `run_attempts`
- `max_attempts`
- `pluginval_command`

## Terminal Reason Contract

`terminal_reason` tokens:
- `pass`
- `pluginval_failed_abrt`
- `pluginval_failed_signal`
- `pluginval_failed_timeout`
- `pluginval_failed_exit`

`terminal_reason_class` tokens:
- `pass`
- `abrt`
- `signal`
- `timeout`
- `exit`

`classification` tokens:
- `pass`
- `transient_runtime_abort`
- `transient_runtime_signal`
- `deterministic_timeout`
- `deterministic_failure`

## Failure Taxonomy Contract

`failure_taxonomy.tsv` dimensions:
- `terminal_reason`
- `terminal_reason_class`
- `terminal_signal_number`
- `classification`
- `crash_report_present`
- `exit_code`

## Threshold and Exit Semantics

Strict threshold:
- RL-06 pass requires `passes == runs`.
- Any failed run yields lane failure.

Exit codes:
- `0`: all required runs passed.
- `1`: preflight invalid or one/more runs failed.
- `2`: invocation/schema errors (invalid CLI args/values).

## Deterministic Retry/Backoff Contract

- Retry loop is bounded: `max_attempts_per_run = max_retries_per_run + 1`.
- No unbounded loops are allowed.
- Backoff is fixed and deterministic (`retry_backoff_ms`), applied only between failed attempts when retries remain.
- Final run taxonomy in `pluginval_runs.tsv` reflects the terminal attempt for each run while preserving total attempts executed.

## Crash Report Detection

Per run, the harness probes for newly created crash reports under:
- `~/Library/Logs/DiagnosticReports`
- `/Library/Logs/DiagnosticReports`

Patterns:
- `*pluginval*.crash`
- `*pluginval*.ips`
