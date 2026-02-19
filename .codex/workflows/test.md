Title: APC Workflow: Test
Document Type: Workflow
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

﻿---
description: "Run tests on the plugin"
---

Title: Testing Workflow
Document Type: Workflow
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

# Test Phase

**Prerequisites:**
```powershell
. "$PSScriptRoot\..\scripts\state-management.ps1"

$state = Get-PluginState -PluginPath "plugins\$PluginName"

if ($state.current_phase -ne "code_complete" -and $state.current_phase -ne "design_complete") {
    Write-Error "Implementation must be complete first."
    exit 1
}

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

**Execute Skill:**
Load and execute `..\codex\skills\skill_testing\SKILL.md`

**Tests Run:**
- audio-dsp-qa-harness build + self-tests
- plugin QA scenario/suite execution via harness-integrated runner
- parameter/invariant regression checks from QA artifacts
- UI/host compatibility (pluginval + DAW smoke; AppKit/headless pluginval aborts are recorded as `environment_blocked`)
- crash/memory stability follow-up

## Claude Parity Defaults (Mandatory Unless User Overrides)

1. Require a plugin-owned QA folder at `plugins/[Name]/qa/` with at least one suite JSON and multiple scenario JSON files.
2. Require machine-readable test evidence in `plugins/[Name]/qa_output/suite_result.json`.
3. Require summary evidence in `plugins/[Name]/TestEvidence/test-summary.md` and `plugins/[Name]/TestEvidence/harness_ctest.log`.
4. If pluginval cannot run due GUI/AppKit bootstrap issues, mark `environment_blocked` and continue based on harness+QA verdict.
5. Do not mark tests passed unless harness and plugin-owned suite both pass.
6. Record test snapshot/trend data in `plugins/[Name]/TestEvidence/build-summary.md` and `plugins/[Name]/TestEvidence/validation-trend.md`.
7. Confirm failures are triaged against documented invariants/ADRs before closing the test phase.

**Completion:**
```text
Tests complete!
Results: [Pass/Fail count]
Next step: /ship [Name] if all tests passed
```
