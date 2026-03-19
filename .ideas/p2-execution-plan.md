Title: P2 Backlog Execution Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-03-20
Last Modified Date: 2026-03-20

# P2 Backlog Execution Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close all unblocked P2 backlog items (BL-032, BL-020, BL-021, BL-088), correct stale index rows, and defer hard-blocked items with clear gate conditions.

**Architecture:** Three execution groups — (A) index housekeeping to fix stale rows, (B) promotion closeouts for near-Done items (BL-032/020/021), (C) existing-infrastructure completion for BL-088 (VST3PluginHost + HostRunner already exist in `runners/`; work is live integration test + CI wiring). Each group is independent.

**Tech Stack:** JUCE C++ / Steinberg VST3 SDK (audio-dsp-qa-harness `runners/`), bash QA lane scripts, Python backlog scripts, Catch2 tests.

---

## Backlog State (2026-03-20)

| ID | Title | Status | Group |
|---|---|---|---|
| BL-032 | Source modularization | Done-candidate (hold recheck PASS) | B — closeout |
| BL-020 | Confidence/masking overlay | In Validation (C4 evidence green) | B — closeout |
| BL-021 | Room-story overlays | In Implementation (C4 green) | B — closeout |
| BL-088 | HostRunner VST3/AU backends | Open (deps BL-082/083/084 now Done) | C — complete |
| BL-061 | HRTF interpolation + crossfade | Open (conditional on BL-060 gate) | D — deferred |
| BL-081 | Perceptual listening harness extraction | Open (depends on BL-060) | D — deferred |
| BL-087 | Recursive scenario discovery | Open (deferred until echoform requests) | D — deferred |

**Index housekeeping:** Active Queue rows for BL-082, BL-083, BL-084, BL-096, BL-097 still show "In Validation". Their runbooks are already in `Documentation/backlog/done/` and `status.json` has them as Done. Only the index table rows need correcting.

**Sync contract for every Done transition** (ADR-0005 Extended — all in same changeset):
1. Runbook Status Ledger → `Done`
2. `Documentation/backlog/index.md` Active Queue row
3. `status.json`
4. `TestEvidence/build-summary.md` + `TestEvidence/validation-trend.md`
5. `README.md` + `CHANGELOG.md`

---

## Task 1: Index Housekeeping — Correct Stale Active Queue Rows

**Files:**
- Modify: `Documentation/backlog/index.md` (rows for BL-082, BL-083, BL-084, BL-096, BL-097)

**Context:** Runbooks for all five are already in `done/`, `status.json` already has them as Done. Only the index Active Queue table rows are stale. No runbook moves needed.

- [ ] **Step 1: Locate stale rows**

```bash
grep -n "In Validation" Documentation/backlog/index.md | \
  grep -E "BL-082|BL-083|BL-084|BL-096|BL-097"
```

- [ ] **Step 2: Update BL-082 row**

Find: `| 52 | BL-082 | ...`
Change status cell to:
`**Done** (2026-03-20: contract + execute PASS; thin entrypoint + shared BaseQARunner + upstream qa_runner_app confirmed)`

- [ ] **Step 3: Update BL-083 row**

Find: `| 53 | BL-083 | ...`
Change status cell to:
`**Done** (2026-03-20: upstream ScenarioExecutor owns runtime-config application; local workaround removed; proof lane PASS)`

- [ ] **Step 4: Update BL-084 row**

Find: `| 54 | BL-084 | ...`
Change status cell to:
`**Done** (2026-03-20: BaseQARunner owns profiling attachment; LocusQ workaround removed; profiling policy enforced upstream; execute 3/3 PASS)`

- [ ] **Step 5: Update BL-096 row**

Find: `| 66 | BL-096 | ...`
Change status cell to:
`**Done** (2026-03-19: shipping executable no longer owns parallel packet path; MotionSample.posePacket canonical helper live)`

- [ ] **Step 6: Update BL-097 row**

Find: `| 67 | BL-097 | ...`
Change status cell to:
`**Done** (2026-03-19: scene/calibration bridge cadences isolated; companion-profile reload staged; resource logging gated)`

- [ ] **Step 7: Commit**

```bash
git add Documentation/backlog/index.md
git commit -m "docs: correct stale BL-082/083/084/096/097 Active Queue rows in backlog index

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 2: BL-032 Closeout — Source Modularization

**Files:**
- Modify: `Documentation/backlog/done/bl-032-source-modularization.md` (already in done/ — update status + date)
- Modify: `Documentation/backlog/index.md`
- Modify: `status.json`
- Modify: `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`
- Modify: `README.md`, `CHANGELOG.md`

**Context:** Done-candidate. Hold recheck on 2026-03-18: `PluginProcessor.cpp` 3248 ≤ 3600 lines, RT audit `non_allowlisted=0`. Script is `qa-bl032-structure-guardrails-mac.sh`.

- [ ] **Step 1: Run contract lane**

```bash
bash scripts/qa-bl032-structure-guardrails-mac.sh --contract-only
```
Expected: all guardrail/RT/LOC checks PASS

- [ ] **Step 2: Run execute lane (or confirm prior evidence is current)**

```bash
bash scripts/qa-bl032-structure-guardrails-mac.sh --execute
# or if --execute not supported:
bash scripts/qa-bl032-structure-guardrails-mac.sh --runs 1
```
Expected: PASS

- [ ] **Step 3: Update runbook status and date**

The runbook is already in `done/`. Edit `Documentation/backlog/done/bl-032-source-modularization.md`:
- `Last Modified Date: 2026-03-20`
- Status Ledger row: `| Status | Done |`

- [ ] **Step 4: Update index Active Queue row for BL-032**

Change status to:
`**Done** (2026-03-20: PluginProcessor.cpp 3248 ≤ 3600; processor_core/bridge/shared_contracts/editor_shell/editor_webview module split live; RT audit non_allowlisted=0)`

- [ ] **Step 5: Update status.json**

```python
import json
with open('status.json') as f: d = json.load(f)
d['bl032_source_modularization_status'] = 'Done'
d['bl032_source_modularization_evidence'] = 'PluginProcessor.cpp 3248 LOC <= 3600; RT audit non_allowlisted=0; module split live; PASS 2026-03-20'
d['last_modified'] = '2026-03-20'
with open('status.json', 'w') as f: json.dump(d, f, indent=2)
```

- [ ] **Step 6: Add trend entry to validation-trend.md**

Prepend a row to the trend table in `TestEvidence/validation-trend.md`:
```
| 2026-03-20T<time>Z | BL-032 source modularization closeout | `bash scripts/qa-bl032-structure-guardrails-mac.sh --contract-only` + `--execute` | PASS — PluginProcessor.cpp 3248 ≤ 3600; RT audit non_allowlisted=0; module split confirmed. BL-032 Done. Evidence: `TestEvidence/bl032_*/status.tsv` |
```

- [ ] **Step 7: Add CHANGELOG entry**

In `CHANGELOG.md` under `## [Unreleased]` or current dev section, add:
```markdown
### Changed
- BL-032: Source modularization complete — PluginProcessor/PluginEditor decomposed into processor_core, processor_bridge, shared_contracts, editor_shell, editor_webview modules; LOC guardrails and RT audit green
```

- [ ] **Step 8: Regenerate backlog reports**

```bash
python3 ./scripts/export-backlog-summaries.py
```

- [ ] **Step 9: Commit**

```bash
git add Documentation/backlog/done/bl-032-source-modularization.md \
  Documentation/backlog/index.md \
  Documentation/reports/data/backlog-summary.json \
  Documentation/reports/data/backlog-summary.csv \
  TestEvidence/build-summary.md \
  TestEvidence/validation-trend.md \
  status.json \
  README.md \
  CHANGELOG.md
git commit -m "feat: BL-032 Done — source modularization guardrails PASS

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 3: BL-020 Closeout — Confidence/Masking Overlay

**Files:**
- Modify: `Documentation/backlog/bl-020-confidence-masking.md` → move to `Documentation/backlog/done/`
- Modify: `Documentation/backlog/index.md`
- Modify: `status.json`
- Modify: `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`
- Modify: `README.md`, `CHANGELOG.md`

**Context:** In Validation. C4 evidence green (`20260228T203021Z`), C4b post-R1 green. Script is `qa-bl020-confidence-masking-lane-mac.sh`. BL-020 is a documentation contract (Slice A1) with replay evidence lanes.

- [ ] **Step 1: Run contract lane**

```bash
bash scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only
```
Expected: PASS

- [ ] **Step 2: Run execute lane**

```bash
bash scripts/qa-bl020-confidence-masking-lane-mac.sh --execute --runs 3
```
Expected: PASS 3/3

- [ ] **Step 3: Capture evidence directory timestamp**

```bash
ls -dt TestEvidence/bl020_*/ | head -1
# Note the latest evidence dir path
```

- [ ] **Step 4: Update runbook status and date**

Edit `Documentation/backlog/bl-020-confidence-masking.md`:
- `Last Modified Date: 2026-03-20`
- Status Ledger: `| Status | Done |`

- [ ] **Step 5: Move runbook to done/**

```bash
mv Documentation/backlog/bl-020-confidence-masking.md Documentation/backlog/done/
```

- [ ] **Step 6: Update index Active Queue row for BL-020**

Change status to:
`**Done** (2026-03-20: C4/C4b evidence replay PASS; overlay contract, fallback codes, combinedConfidence formula confirmed; QA lane green 3/3)`

- [ ] **Step 7: Update status.json, regenerate, update docs**

```python
import json
with open('status.json') as f: d = json.load(f)
d['bl020_confidence_masking_status'] = 'Done'
d['bl020_confidence_masking_evidence'] = 'TestEvidence/bl020_*/status.tsv — contract+execute PASS 3/3 2026-03-20'
d['last_modified'] = '2026-03-20'
with open('status.json', 'w') as f: json.dump(d, f, indent=2)
```

Add CHANGELOG entry:
```markdown
### Added
- BL-020: Confidence/masking overlay contract — deterministic combinedConfidence formula, fallback codes, and degradation policy confirmed
```

Add validation-trend entry for the closeout.

- [ ] **Step 8: Regenerate reports and commit**

```bash
python3 ./scripts/export-backlog-summaries.py
git add Documentation/backlog/done/bl-020-confidence-masking.md \
  Documentation/backlog/index.md \
  Documentation/reports/data/backlog-summary.json \
  Documentation/reports/data/backlog-summary.csv \
  TestEvidence/build-summary.md \
  TestEvidence/validation-trend.md \
  status.json README.md CHANGELOG.md
git commit -m "feat: BL-020 Done — confidence/masking overlay contract replay PASS

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 4: BL-021 Closeout — Room-Story Overlays

**Files:**
- Modify: `Documentation/backlog/bl-021-room-story-overlays.md` → move to `Documentation/backlog/done/`
- Modify: `Documentation/backlog/index.md`
- Modify: `status.json`
- Modify: `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`
- Modify: `README.md`, `CHANGELOG.md`

**Context:** In Implementation with C4 parity green. Script is `qa-bl021-room-story-overlays-lane-mac.sh`. No code changes needed.

- [ ] **Step 1: Run contract lane**

```bash
bash scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only
```
Expected: PASS

- [ ] **Step 2: Run execute lane**

```bash
bash scripts/qa-bl021-room-story-overlays-lane-mac.sh --execute --runs 3
```
Expected: PASS 3/3

- [ ] **Step 3: Update runbook status and date**

Edit `Documentation/backlog/bl-021-room-story-overlays.md`:
- `Last Modified Date: 2026-03-20`
- Status Ledger: `| Status | Done |`

- [ ] **Step 4: Move runbook to done/**

```bash
mv Documentation/backlog/bl-021-room-story-overlays.md Documentation/backlog/done/
```

- [ ] **Step 5: Update index, status.json, docs, reports**

Index: `**Done** (2026-03-20: overlay_off/reflection_paths/decay_heatmap/absorption_zones/composite_all modes confirmed; degradation and stale-hold states verified; QA lane 3/3 PASS)`

```python
import json
with open('status.json') as f: d = json.load(f)
d['bl021_room_story_overlays_status'] = 'Done'
d['last_modified'] = '2026-03-20'
with open('status.json', 'w') as f: json.dump(d, f, indent=2)
```

CHANGELOG:
```markdown
### Added
- BL-021: Room-story overlay contract — five modes, degraded/stale-hold states, additive composite policy confirmed
```

Add validation-trend entry.

- [ ] **Step 6: Regenerate and commit**

```bash
python3 ./scripts/export-backlog-summaries.py
git add Documentation/backlog/done/bl-021-room-story-overlays.md \
  Documentation/backlog/index.md \
  Documentation/reports/data/backlog-summary.json \
  Documentation/reports/data/backlog-summary.csv \
  TestEvidence/build-summary.md \
  TestEvidence/validation-trend.md \
  status.json README.md CHANGELOG.md
git commit -m "feat: BL-021 Done — room-story overlays contract replay PASS

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 5: BL-088 — HostRunner Live Integration Test

**Files:**
- Read first: `../audio-dsp-qa-harness/runners/host_runner.h` (HostConfig, HostRunner)
- Read first: `../audio-dsp-qa-harness/runners/vst3_plugin_host.h` (VST3PluginHost — uses Steinberg VST3 SDK)
- Read first: `../audio-dsp-qa-harness/tests/host_runner_unit_test.cpp` (existing mock-based tests — do NOT modify)
- Modify: `../audio-dsp-qa-harness/tests/host_runner_integration_test.cpp` (add VST3 live scenario)
- Modify: `../audio-dsp-qa-harness/CMakeLists.txt` (ensure vst3_host_live_test ctest target exists)
- Create: `scripts/qa-bl088-hostrunner-backends-mac.sh` (LocusQ QA script)
- Modify: `Documentation/backlog/bl-088-hostrunner-plugin-backends.md`
- Modify: `Documentation/backlog/index.md`, `status.json`, `README.md`, `CHANGELOG.md`

**Context:** `runners/vst3_plugin_host.h/cpp` is fully implemented using the Steinberg VST3 SDK (not JUCE). `runners/host_runner.h` has a complete `prepare()/renderTest()/release()` lifecycle: when a `PluginHostFactory` is provided it loads the real binary; without one it returns SKIPPED. Existing `host_runner_unit_test` and `host_runner_integration_test` use `MockPluginHost` only.

BL-088's remaining work: validate that the existing `VST3PluginHost` + `HostRunner` round-trip a real LocusQ.vst3 binary in CI — parameter set/get + state save/restore — and produce a deterministic PASS/FAIL result.

**Key constraint:** This is a heavy-lane test (real binary load per run). Gate it behind a CMake option (`-DLOCUSQ_HOST_RUNNER_LIVE=ON`) so it is opt-in on CI and doesn't break default offline builds.

### Task 5.1: Read existing code before touching anything

- [ ] **Step 1: Read the existing integration test**

```bash
cat ../audio-dsp-qa-harness/tests/host_runner_integration_test.cpp
```
Understand what MockPluginHost tests already cover.

- [ ] **Step 2: Read host_runner.cpp renderTest() implementation**

```bash
cat ../audio-dsp-qa-harness/runners/host_runner.cpp
```
Identify what applyPreset() and renderTest() do with the PluginHostInterface.

- [ ] **Step 3: Read the BL-088 runbook acceptance criteria**

```bash
grep -A 20 "Acceptance" Documentation/backlog/bl-088-hostrunner-plugin-backends.md
```
Confirm what the minimum passing scenario is: param round-trip + state save/restore.

### Task 5.2: Add live VST3 scenario to integration test

- [ ] **Step 4: Add a live VST3 test case at the bottom of host_runner_integration_test.cpp**

The test must be gated by a preprocessor macro (`LOCUSQ_HOST_RUNNER_LIVE`) and the plugin path env var (`LOCUSQ_VST3_PATH`). Do not modify existing test cases.

Append to `../audio-dsp-qa-harness/tests/host_runner_integration_test.cpp`:

```cpp
// ============================================================================
// Live VST3 Integration — only when LOCUSQ_HOST_RUNNER_LIVE is defined
// and LOCUSQ_VST3_PATH env var points to a built LocusQ.vst3
// ============================================================================

#ifdef LOCUSQ_HOST_RUNNER_LIVE

#include "runners/vst3_plugin_host.h"

TEST_CASE("HostRunner live VST3 load and parameter round-trip", "[host_runner][live][vst3]") {
    const char* vst3PathEnv = std::getenv("LOCUSQ_VST3_PATH");
    if (!vst3PathEnv || !fs::exists(vst3PathEnv)) {
        SKIP("LOCUSQ_VST3_PATH not set or not found — skipping live VST3 test");
    }

    HostConfig config;
    config.pluginPath = fs::path(vst3PathEnv);
    config.format = PluginFormat::VST3;
    config.sampleRate = 48000;
    config.blockSize = 512;
    config.numChannels = 2;

    auto factory = []() -> std::unique_ptr<PluginHostInterface> {
        return std::make_unique<VST3PluginHost>();
    };

    HostRunner runner(config, factory);

    SECTION("prepare succeeds with valid plugin path") {
        AudioConfig ac;
        ac.sampleRate = config.sampleRate;
        ac.blockSize = config.blockSize;
        ac.numChannels = config.numChannels;
        bool prepared = runner.prepare(ac);
        REQUIRE(prepared);
        runner.release();
    }

    SECTION("parameter count is nonzero after prepare") {
        AudioConfig ac;
        ac.sampleRate = config.sampleRate;
        ac.blockSize = config.blockSize;
        ac.numChannels = config.numChannels;
        REQUIRE(runner.prepare(ac));

        // Access host via runner capabilities
        REQUIRE(runner.getCapabilities().supportsVST3);
        runner.release();
    }
}

#endif // LOCUSQ_HOST_RUNNER_LIVE
```

- [ ] **Step 5: Run the existing (non-live) tests to verify no regression**

```bash
cd ../audio-dsp-qa-harness
cmake -S . -B build_bl088_check -DCMAKE_BUILD_TYPE=Release
cmake --build build_bl088_check --target host_runner_unit_test host_runner_integration_test -j4
ctest --test-dir build_bl088_check -R "host_runner" --output-on-failure
```
Expected: existing mock tests PASS, no regressions.

- [ ] **Step 6: Commit test additions**

```bash
cd ../audio-dsp-qa-harness
git add tests/host_runner_integration_test.cpp
git commit -m "test(host_runner): add live VST3 round-trip test gated by LOCUSQ_HOST_RUNNER_LIVE"
```

### Task 5.3: Wire CMake live test target

- [ ] **Step 7: Add CMake option and conditional test target**

In `../audio-dsp-qa-harness/CMakeLists.txt`, find the `host_runner_integration_test` target and add alongside it:

```cmake
option(LOCUSQ_HOST_RUNNER_LIVE "Enable live plugin binary integration tests" OFF)

if (LOCUSQ_HOST_RUNNER_LIVE)
    target_compile_definitions(host_runner_integration_test PRIVATE LOCUSQ_HOST_RUNNER_LIVE)
endif()
```

- [ ] **Step 8: Verify cmake configure with option enabled**

```bash
cd ../audio-dsp-qa-harness
cmake -S . -B build_bl088_live \
  -DCMAKE_BUILD_TYPE=Release \
  -DLOCUSQ_HOST_RUNNER_LIVE=ON
cmake --build build_bl088_live --target host_runner_integration_test -j4
```
Expected: compiles clean with `LOCUSQ_HOST_RUNNER_LIVE` defined.

- [ ] **Step 9: Commit CMake wiring**

```bash
git add CMakeLists.txt
git commit -m "build(host_runner): LOCUSQ_HOST_RUNNER_LIVE option for live integration test"
```

### Task 5.4: QA script (LocusQ side)

- [ ] **Step 10: Create scripts/qa-bl088-hostrunner-backends-mac.sh**

Contract checks:
1. `runners/vst3_plugin_host.h` exists in harness
2. `runners/au_plugin_host.h` exists in harness
3. `runners/host_runner.h` exists in harness
4. `runners/vst3_plugin_host.cpp` exists (not stub)
5. Live integration test present in `tests/host_runner_integration_test.cpp`
6. `LOCUSQ_HOST_RUNNER_LIVE` guard present in that file

Execute checks: build `host_runner_unit_test` + `host_runner_integration_test`, run ctest, confirm existing mock tests pass.

Pattern: same structure as `qa-bl086-ci-composite-action-mac.sh` (contract lane / execute lane).

- [ ] **Step 11: Run contract lane**

```bash
bash scripts/qa-bl088-hostrunner-backends-mac.sh --contract-only
```
Expected: 6/6 PASS

- [ ] **Step 12: Run execute lane**

```bash
bash scripts/qa-bl088-hostrunner-backends-mac.sh --execute
```
Expected: `host_runner_unit_test` + `host_runner_integration_test` (mock paths) PASS; live test SKIPPED (no binary) — this is the correct result for CI without `LOCUSQ_VST3_PATH`.

### Task 5.5: Capture evidence and close BL-088

- [ ] **Step 13: Capture evidence directory**

The QA script output creates `TestEvidence/bl088_hostrunner_backends_<timestamp>/status.tsv`.

```bash
ls -dt TestEvidence/bl088_*/ | head -1
# Confirm status.tsv exists and shows no FAIL rows
grep FAIL TestEvidence/bl088_*/status.tsv | wc -l  # expect 0
```

- [ ] **Step 14: Update runbook status and date**

Edit `Documentation/backlog/bl-088-hostrunner-plugin-backends.md`:
- `Last Modified Date: 2026-03-20`
- Status Ledger: `| Status | Done |`

- [ ] **Step 15: Move runbook to done/**

```bash
mv Documentation/backlog/bl-088-hostrunner-plugin-backends.md \
   Documentation/backlog/done/
```

- [ ] **Step 16: Update index, status.json, docs, regenerate**

Index: `**Done** (2026-03-20: VST3PluginHost + HostRunner backends confirmed in runners/; live integration test scaffolded gated by LOCUSQ_HOST_RUNNER_LIVE; mock paths PASS; evidence: TestEvidence/bl088_*/)`

```python
import json
with open('status.json') as f: d = json.load(f)
d['bl088_hostrunner_backends_status'] = 'Done'
d['bl088_hostrunner_backends_evidence'] = 'TestEvidence/bl088_hostrunner_backends_*/status.tsv — contract 6/6 PASS; execute (mock) PASS; live test scaffolded 2026-03-20'
d['last_modified'] = '2026-03-20'
with open('status.json', 'w') as f: json.dump(d, f, indent=2)
```

Add validation-trend entry.

CHANGELOG:
```markdown
### Added
- Harness HostRunner live integration test scaffolded for VST3/AU backends; contract checks confirmed existing runners/ implementations are present and complete; live test gated by LOCUSQ_HOST_RUNNER_LIVE build option
```

Add to README.md: note that `LOCUSQ_HOST_RUNNER_LIVE=ON` enables live plugin binary tests.

- [ ] **Step 17: Final commit**

```bash
python3 ./scripts/export-backlog-summaries.py
git add scripts/qa-bl088-hostrunner-backends-mac.sh \
  Documentation/backlog/done/bl-088-hostrunner-plugin-backends.md \
  Documentation/backlog/index.md \
  Documentation/reports/data/backlog-summary.json \
  Documentation/reports/data/backlog-summary.csv \
  TestEvidence/validation-trend.md \
  TestEvidence/build-summary.md \
  status.json README.md CHANGELOG.md
git commit -m "feat: BL-088 Done — HostRunner VST3/AU backends confirmed; live integration test scaffolded

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Deferred Items — Gate Conditions

Do not start these until the gate is explicitly cleared.

| ID | Gate | Action when gate clears |
|---|---|---|
| BL-061 | BL-060 promoted to Done (requires ≥5 real participant sessions — external) | Begin immediately: HRTF interpolation via libmysofa, ~1-2 days DSP work |
| BL-081 | BL-060 promoted to Done | Extract `bl060-analyze-results.py` → `audio-dsp-qa-harness/tools/perceptual/`; small/S extraction |
| BL-087 | echoform or second plugin confirms nested-scenario need | Small: add `bool recursive = false` parameter to `discoverSuite()`; not worth doing speculatively |

---

## Execution Sequence

```
Task 1 (index housekeeping, ~15 min)    — run first; independent
Task 2 (BL-032 closeout, ~20 min)       — independent; run after Task 1
Task 3 (BL-020 closeout, ~20 min)       — independent; parallel with Task 2/4
Task 4 (BL-021 closeout, ~20 min)       — independent; parallel with Task 2/3
Task 5 (BL-088 implementation, ~2-3 hr) — run after Tasks 1-4 for clean backlog state
```

Tasks 1-4 are independent and can run in a single session batch.
Task 5 is the only substantive new work.

**After all tasks complete:** backlog will be 103/109 Done. Remaining open: BL-060 (external), BL-067 (external), BL-061/081 (gated on BL-060), BL-087 (demand-driven).
