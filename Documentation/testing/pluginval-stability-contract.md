Title: Pluginval Stability Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-030 RL-06 Pluginval Reliability Harness (Slice H3)

## Purpose

Define the deterministic replay contract for RL-06 (`pluginval`) so repeated runs are machine-readable and directly classifiable.

## Canonical Command

- `./scripts/qa-bl030-pluginval-stability-mac.sh --runs 5 --out-dir TestEvidence/bl030_rl06_pluginval_h3_<timestamp>`

## Inputs

- `--runs <N>`: number of pluginval invocations (default `5`)
- `--out-dir <path>`: evidence output directory
- `--plugin-path <path>`: plugin under test (default `build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3`)
- `--pluginval-bin <path>`: pluginval executable path
- `--timeout-ms <N>`: pluginval timeout (default `30000`)

## Required Outputs

- `status.tsv`
- `replay_runs.tsv`
- `failure_taxonomy.tsv`
- `harness_contract.md`
- `command_transcript.log`

## `replay_runs.tsv` Schema

Columns (exact order):
- `run_index`
- `timestamp_utc`
- `exit_code`
- `terminal_reason`
- `classification`
- `crash_report_present`
- `crash_report_path`
- `plugin_path`
- `log_path`

## Failure Classification

- `pass`: exit code `0`.
- `transient_runtime_abort`: abort signatures (`ABRT`, `Abort trap`, exit `134` style failures).
- `deterministic_timeout`: timeout signatures.
- `deterministic_failure`: all other non-zero failures.

## Crash Report Detection

For each run, the harness probes for new crash reports after run start in:
- `~/Library/Logs/DiagnosticReports`
- `/Library/Logs/DiagnosticReports`

Matched names:
- `*pluginval*.crash`
- `*pluginval*.ips`

## Exit Semantics

- `0`: all required runs passed.
- `1`: preflight invalid (missing plugin/pluginval) or one/more runs failed.
