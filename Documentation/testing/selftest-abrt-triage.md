Title: Selftest ABRT Triage Guide
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# Selftest ABRT Triage Guide

## Purpose

Provide deterministic classification for Standalone selftest `SIGABRT` failures and a reproducible, minimal command surface for owner-side debugging.

## Scope

Read-only runtime probe workflow for:
- `scripts/diagnose-selftest-abrt-mac.sh`
- `scripts/standalone-ui-selftest-production-p0-mac.sh`

No plugin/source mutation is performed by the probe.

## Required Captures Per Run

- selftest command exit code
- app exit code and signal
- newest crash report path (`~/Library/Logs/DiagnosticReports/LocusQ*.ips|*.crash`)
- main-thread crash frames summary
- phase classification

## Phase Classification Contract

Classification outputs:
- `pre_result_emit`
- `appkit_registration`
- `post_ui_bootstrap`
- `unknown`

Deterministic mapping:
1. `appkit_registration`:
   - main-thread frames contain AppKit registration/bootstrap symbols (for example `_RegisterApplication`, `+[NSApplication sharedApplication]`, `_NSInitializeAppContext`).
2. `post_ui_bootstrap`:
   - main-thread frames contain WebView/UI bootstrap symbols (for example `WKWebView`, `WebKit`, `JavaScriptCore`, JUCE web-browser bridge symbols).
3. `pre_result_emit`:
   - terminal reason indicates result file was not emitted before process exit/timeout (for example `app_exited_before_result`, `result_json_missing_after_*`, `selftest_payload_not_ok`).
4. `unknown`:
   - no deterministic match.

## Probe Outputs

Per probe invocation output directory includes:
- `crash_taxonomy.tsv` (machine-readable)
- `triage_summary.md` (human summary)
- `crash_stack_samples.md` (run-by-run frame samples)
- `repro_commands.md` (minimal repro commands)
- `probe.log`

## Standard Commands

BL-029 lane:

```bash
./scripts/diagnose-selftest-abrt-mac.sh --scope bl029 --runs 5
```

BL-009 lane variant:

```bash
./scripts/diagnose-selftest-abrt-mac.sh --scope bl009 --runs 5 --bl009 1
```

## Owner Handoff Expectations

When probe lane fails, handoff must include:
- top phase classification
- top terminal failure reason
- minimal reproducible command
- representative crash report path
- main-thread frame excerpt
