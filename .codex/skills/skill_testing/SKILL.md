---
name: skill_testing
description: "Detailed DSP QA harness-first testing workflow for APC plugins."
---

Title: SKILL: TESTING (DSP QA HARNESS FIRST)
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-01


# SKILL: TESTING (DSP QA HARNESS FIRST)

**Goal:** Run APC testing through the `audio-dsp-qa-harness` workflow first, then host/plugin checks.

## Claude Parity Defaults (Mandatory Unless User Overrides)

1. Generate and preserve plugin-owned QA artifacts under `plugins/[Name]/qa/`, not only harness-side temporary scenarios.
2. Always emit `plugins/[Name]/qa_output/suite_result.json` for machine-readable pass/fail.
3. Always write `plugins/[Name]/TestEvidence/test-summary.md` with harness, QA runner, pluginval, and DAW smoke outcomes.
4. Treat pluginval AppKit bootstrap aborts as environment issues; attach `exit_code` and `crash_report_path` evidence.
5. Final `/test` verdict is PASS only when harness sanity and plugin-owned suite both pass.

## HARNESS LOCATION

Resolve harness path in this order:

1. `$env:APC_DSP_QA_HARNESS_PATH`
2. `Join-Path $HOME "Documents/Repos/audio-dsp-qa-harness"`

Fail fast if the directory is missing.

```powershell
$HarnessPath = if ($env:APC_DSP_QA_HARNESS_PATH) {
    $env:APC_DSP_QA_HARNESS_PATH
} else {
    Join-Path $HOME "Documents/Repos/audio-dsp-qa-harness"
}

if (-not (Test-Path $HarnessPath)) {
    Write-Error "audio-dsp-qa-harness not found. Set APC_DSP_QA_HARNESS_PATH or install at $HarnessPath"
    exit 1
}
```

## TASKS

1. **Harness Sanity:** Build and validate harness locally when needed.
   - `cmake -S "$HarnessPath" -B "$HarnessPath/build_test" -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5`
   - `cmake --build "$HarnessPath/build_test"`
   - `ctest --test-dir "$HarnessPath/build_test" --output-on-failure`
2. **Plugin QA Runner Execution:** Prefer plugin-integrated QA binaries generated from harness templates (`simple_scenario_runner.cpp` or `suite_runner.cpp`).
   - Candidate executables: `*${PluginName}*qa*`, `*${PluginName}*suite*qa*` under `build/`.
   - Run suite-first if available; otherwise run scenario runner.
3. **Scenario Source:** Prefer plugin-owned suites/scenarios under `plugins/[Name]/qa/`.
   - If missing, bootstrap from harness examples in `$HarnessPath/scenarios/examples/` and templates in `$HarnessPath/examples/templates/`.
4. **Artifacts & Verdict:** Collect pass/fail from QA output (`result.json`, summary files, WAV artifacts), then update `status.json` test flags.
5. **Host Validation:** Run pluginval/DAW compatibility checks after harness pass for packaging confidence.
   - macOS note: `pluginval` is an AppKit app and must run in a GUI login session.
   - If it aborts with `SIGABRT` and stack frames around `HIServices _RegisterApplication` / `NSApplication sharedApplication`,
     treat this as an environment bootstrap failure (headless/sandbox context), not a plugin regression.
   - In this case, do not fail `/test`. Mark `validation.pluginval_results.environment_blocked = true`, set `blocked_reason`,
     record `exit_code`/`crash_report_path` if available, and keep `validation.tests_passed` tied to harness+QA verdict.
   - Retry from a normal Terminal session or launch via:
     - `open -na /Applications/pluginval.app --args --strictness-level 5 --timeout-ms 30000 --validate "<path-to-plugin.vst3>"`
6. **Crash Analysis:** If crash reported, read `Documents/APC_CRASH_REPORT.txt` and correlate with QA artifacts.

## Backlog Replay Tier Discipline
When testing backlog lanes, enforce replay tiers from `Documentation/backlog/index.md`:
- `T1` dev loop default,
- `T2` candidate gate,
- `T3` promotion gate,
- `T4` sentinel only when explicitly requested.

For heavy wrappers, prefer targeted reruns before broad sweeps and record any owner-approved cadence override in evidence notes.
