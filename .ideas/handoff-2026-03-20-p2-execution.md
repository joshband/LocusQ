Title: Session Handoff — P2 Execution (2026-03-20)
Document Type: Session Handoff
Author: APC Codex
Created Date: 2026-03-20
Last Modified Date: 2026-03-20

# Session Handoff — P2 Execution

## Where We Are

- **Branch:** `main`
- **Backlog:** 99/109 Done (BL-082, BL-083, BL-084 closed this session)
- **status.json last_modified:** 2026-03-20
- **Build:** clean (build_local)
- **Backlog summaries:** fresh (`./scripts/export-backlog-summaries.py --check` PASS)

## Immediate Next Action

Execute the P2 plan using **`superpowers:subagent-driven-development`**.

Plan file: `.ideas/p2-execution-plan.md`

Invoke the skill, point it at the plan, let it dispatch one subagent per task with review between tasks.

## The Plan (5 Tasks)

Tasks 1-4 are independent closeouts. Task 5 is the only new implementation.

| Task | Item | Type | Est |
|---|---|---|---|
| 1 | Index housekeeping — fix stale BL-082/083/084/096/097 Active Queue rows | docs only | ~15 min |
| 2 | BL-032 closeout — source modularization Done | QA replay + sync | ~20 min |
| 3 | BL-020 closeout — confidence/masking overlay Done | QA replay + sync | ~20 min |
| 4 | BL-021 closeout — room-story overlays Done | QA replay + sync | ~20 min |
| 5 | BL-088 — HostRunner live integration test | new code in harness | ~2-3 hrs |

**After all tasks:** 103/109 Done.

## Key Facts Each Subagent Needs

### Task 1 (index only)
- BL-082/083/084/096/097 runbooks are **already in** `Documentation/backlog/done/` and `status.json` already has them Done
- Only fix: Active Queue rows in `Documentation/backlog/index.md` (5 rows showing "In Validation")
- No mv, no status.json change needed — just edit the table cells

### Tasks 2/3/4 (closeout pattern)
Correct QA script names (non-obvious names — use these exactly):
- BL-032: `bash scripts/qa-bl032-structure-guardrails-mac.sh --contract-only`
- BL-020: `bash scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only` + `--execute --runs 3`
- BL-021: `bash scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only` + `--execute --runs 3`

BL-032 runbook is already in `done/` (moved earlier, Done-candidate status) — edit in place, don't move again.
BL-020 and BL-021 runbooks are still in `Documentation/backlog/` — edit status + move to `done/`.

**Done-transition sync contract (ADR-0005 Extended) — all in same commit:**
1. Runbook Status Ledger → Done + Last Modified Date → 2026-03-20
2. `Documentation/backlog/index.md` Active Queue row
3. `status.json` (add `bl0XX_<slug>_status: 'Done'` key + update `last_modified`)
4. `TestEvidence/build-summary.md` + `TestEvidence/validation-trend.md` (add trend row)
5. `README.md` + `CHANGELOG.md` (add entry)
6. Regenerate: `python3 ./scripts/export-backlog-summaries.py`

### Task 5 (BL-088 — HostRunner)
**Critical facts:**
- Harness location: `../audio-dsp-qa-harness/` (sibling repo)
- Existing runners are in `../audio-dsp-qa-harness/runners/` — NOT `lib/host_runner/`
- `VST3PluginHost` uses **Steinberg VST3 SDK** (not JUCE) — `pluginterfaces/vst/ivstaudioprocessor.h` etc.
- `HostRunner` is fully implemented; `renderTest()` works when a `PluginHostFactory` is provided
- Existing tests: `tests/host_runner_unit_test.cpp` (mock-based) + `tests/host_runner_integration_test.cpp` — do NOT modify existing test cases
- `RunnerCapabilities` fields: `supportsDryCapture`, `supportsParameterSweep`, `requiresExternalBinary`, `supportsMIDI`, `supportsAutomation`, `supportsStateRoundtrip`, **`supportsVST3`**, `supportsAU`, `supportsCLAP` — use `supportsVST3` not `supportsParameterControl` (doesn't exist)
- Live test must be gated by `#ifdef LOCUSQ_HOST_RUNNER_LIVE` + `LOCUSQ_VST3_PATH` env var
- CMake option: `-DLOCUSQ_HOST_RUNNER_LIVE=ON` to enable live tests
- Evidence dir: `TestEvidence/bl088_hostrunner_backends_<timestamp>/status.tsv`
- QA script to create: `scripts/qa-bl088-hostrunner-backends-mac.sh` (6 contract checks + execute lane)

**Do NOT embed backlog IDs in source code comments, variable names, or struct names.** (Repo rule)

## Deferred Items (do not start)

| ID | Gate |
|---|---|
| BL-061 | BL-060 Done (needs ≥5 real participants — external) |
| BL-081 | BL-060 Done |
| BL-087 | echoform confirms nested-scenario need |

## Repo Health

```
git status:  clean (after last commit 6ba7b718)
open P1:     BL-060 (external), BL-067 (external)
open P2:     BL-020, BL-021, BL-032, BL-061, BL-081, BL-087, BL-088
boids issue: boids spread dark in REAPER host; isolated runtime probe passes —
             narrowed to shared-lifecycle boundary; not a P1 blocker
```

## Commands to Run at Session Start

```bash
# Verify clean state
git status
git log --oneline -3

# Confirm backlog is current
python3 ./scripts/export-backlog-summaries.py --check

# Then invoke: superpowers:subagent-driven-development
# pointing at: .ideas/p2-execution-plan.md
```
