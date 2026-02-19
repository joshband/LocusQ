Title: APC Skill: test
Document Type: Skill Specification
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

---
name: skill_test
description: "Testing phase entrypoint that routes APC plugin validation through the testing workflow."
---

Title: Testing Entrypoint Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

# Test - Plugin Testing & Validation

**Trigger:** `/test [PluginName]`
**Phase:** Testing (can run after Implementation)
**Primary Skill:** `.codex\skills\skill_testing\SKILL.md`

---

## EXECUTION

When invoked, execute the complete workflow from:
**`.codex\skills\skill_testing\SKILL.md`**

This workflow is **harness-first** and must resolve:
- `$env:APC_DSP_QA_HARNESS_PATH` or
- `~/Documents/Repos/audio-dsp-qa-harness`

Default parity gates:
- plugin-owned QA scenarios under `plugins/[Name]/qa/`
- suite output at `plugins/[Name]/qa_output/suite_result.json`
- test evidence summary at `plugins/[Name]/TestEvidence/test-summary.md`
- pluginval environment-block classification when AppKit bootstrap prevents validation
- final PASS only after harness + plugin suite both pass

## WORKFLOW GATES

See `.codex\workflows\test.md` for:
- Prerequisites (requires completed Implementation phase)
- Test procedures
- Validation criteria

## PARAMETERS

- `PluginName` - Name of existing plugin to test

## OUTPUT

- Test results
- Validation report
- Updates `plugins/[Name]/status.json` with test status
- If pluginval is blocked by headless/AppKit startup failure, record `validation.pluginval_results.environment_blocked=true` instead of failing `/test`
- Updates `plugins/[Name]/TestEvidence/build-summary.md` and `plugins/[Name]/TestEvidence/validation-trend.md` with snapshot + trend entries

## TEST TYPES

- Harness build/self-test validation
- Scenario/suite-based DSP invariants and regressions
- UI/DSP integration verification
- DAW compatibility check
