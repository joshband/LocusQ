---
Title: BL-022 Choreography Lane Closeout
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02
---

# BL-022: Choreography Lane Closeout

## Plain-Language Summary

This runbook tracks **BL-022** (BL-022: Choreography Lane Closeout). Current status: **Done (2026-02-24 closeout evidence refresh)**. In plain terms: Finalize choreography pack closure — validate that the keyframe timeline and transport system supports choreography workflows (multi-track keyframe sequences, preset-driven motion paths) while maintaining BL-025 EMITTER UI/UX v2 stability.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-022: Choreography Lane Closeout |
| Why is this important? | Finalize choreography pack closure — validate that the keyframe timeline and transport system supports choreography workflows (multi-track keyframe sequences, preset-driven motion paths) while maintaining BL-025 EMITTER UI/UX v2 stability. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-022-choreography-closeout.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Implementation slices table | Explains step-by-step delivery order and boundaries. | `## Implementation Slices` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Done (2026-02-24 closeout evidence refresh) |
| Owner Track | Track C — UX Authoring |
| Depends On | BL-003, BL-004 |
| Blocks | — |
| Annex Spec | (inline — choreography is part of timeline/keyframe systems) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Low | S | Choreography validation lane |
| B | Low | S | BL-025 regression guard rerun |

## Objective

Finalize choreography pack closure — validate that the keyframe timeline and transport system supports choreography workflows (multi-track keyframe sequences, preset-driven motion paths) while maintaining BL-025 EMITTER UI/UX v2 stability.

## Scope & Non-Scope

**In scope:**
- Choreography-specific validation (keyframe sequencing, transport sync, preset recall)
- BL-025 regression guard rerun
- Evidence capture

**Out of scope:**
- New choreography features
- Timeline UI redesign (that's future work)
- Physics-driven choreography (that's BL-019/020 territory)

## Architecture Context

- Keyframe timeline: `Source/KeyframeTimeline.cpp` / `.h` — multi-track animation system
- Transport controls: restored in BL-003, gestures added in BL-004
- EMITTER UI: redesigned in BL-025 with new control rail structure
- Bridge: WebView <-> native bridge handles timeline commands via `Source/PluginEditor.cpp`
- Invariants: Scene Graph (snapshot transport is sequence-safe)

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Choreography validation | `Source/KeyframeTimeline.cpp`, `Source/ui/public/js/index.js` | BL-003, BL-004 done | Choreography lane passes |
| B | BL-025 regression guard | `Source/ui/public/js/index.js` | Slice A done | BL-025 self-test green |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/test BL-022 Slice A: Choreography lane validation
Load: $skill_test, $juce-webview-runtime

Objective: Validate choreography workflows — multi-track keyframe sequences,
transport sync, preset-driven motion paths.

Constraints:
- Timeline transport must remain deterministic
- Keyframe editing gestures (BL-004) must still work
- Do not modify timeline source code

Validation:
- Run production self-test focusing on timeline/keyframe assertions
- Manual verification: create keyframe sequence, play, verify motion path
- Verify preset recall preserves keyframe data

Evidence:
- TestEvidence/bl022_choreography_<timestamp>/choreography_lane.log
- TestEvidence/bl022_choreography_<timestamp>/status.tsv
```

### Slice A — Standalone Fallback Prompt

```
You are validating BL-022 Slice A for LocusQ.

PROJECT CONTEXT:
- Keyframe system: Source/KeyframeTimeline.cpp/.h — multi-track, supports add/delete/drag/select
- Transport: play/pause/stop/scrub restored in BL-003
- Gestures: click-to-add, drag-to-move, selection, delete, multi-select from BL-004
- WebView bridge: Source/PluginEditor.cpp handles timeline native bridge calls
- UI runtime: Source/ui/public/js/index.js has timeline rendering and interaction code

TASK:
1. Build and launch standalone: cmake --build build --target LocusQ_Standalone
2. Verify timeline transport works (play, pause, stop, scrub)
3. Create multi-track keyframe sequence (at least 3 keyframes on 2 tracks)
4. Play back and verify emitter follows motion path
5. Save preset, reload, verify keyframe data persists
6. Run production self-test lane for timeline assertions
7. Capture evidence

CONSTRAINTS:
- Do not modify source code — validation only
- Transport must be deterministic (same input = same playback)

EVIDENCE:
- TestEvidence/bl022_choreography_<timestamp>/choreography_lane.log
- TestEvidence/bl022_choreography_<timestamp>/status.tsv
```

### Slice B — Skill-Aware Prompt

```
/test BL-022 Slice B: BL-025 regression guard
Load: $skill_test, $juce-webview-runtime

Objective: Confirm BL-025 EMITTER UI/UX v2 remains stable after choreography validation.

Validation:
- Run BL-025 self-test lane (UI-P1-025A..E)
- Run production self-test
- Verify no regressions in emitter panel controls

Evidence:
- TestEvidence/bl022_choreography_<timestamp>/bl025_regression.log
```

### Slice B — Standalone Fallback Prompt

```
You are validating BL-022 Slice B for LocusQ — regression guard for BL-025.

TASK:
1. Run production self-test: the standalone app's built-in self-test mechanism
2. Focus on EMITTER panel assertions (parameter rail, emitter selector, directivity, velocity, preset)
3. Verify no regressions from choreography validation work
4. Capture results

EVIDENCE:
- TestEvidence/bl022_choreography_<timestamp>/bl025_regression.log
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-022-choreo | Mixed | Self-test + manual choreography workflow | Timeline assertions pass, motion paths play correctly |
| BL-022-bl025 | Automated | BL-025 self-test lane | All BL-025 assertions green |
| BL-022-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| BL-025 regression from choreography changes | Med | Low | Slice B guards against this |
| Timeline transport timing drift | Med | Low | Verify against scene-state-contract timing guarantees |

## Failure & Rollback Paths

- If choreography lane fails: isolate to keyframe vs transport vs preset subsystem
- If BL-025 regression detected: compare against BL-025 closeout evidence, identify delta
- If both fail: check recent commits for cross-cutting changes

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Choreography + BL-025 lane log | `TestEvidence/bl022_validation_<timestamp>/selftest.log` | production self-test output |
| Spatial smoke regression | `TestEvidence/bl022_validation_<timestamp>/qa_smoke_spatial.log` | suite summary |
| Status TSV | `TestEvidence/bl022_validation_<timestamp>/status.tsv` | lane, result, timestamp |
| Validation trend | `TestEvidence/validation-trend.md` | date, lane, result, notes |

## Latest Closeout Evidence (2026-02-24)

- Bundle: `TestEvidence/bl022_validation_20260224T184032Z/`
- Production self-test: `TestEvidence/locusq_production_p0_selftest_20260224T184037Z.json`
- Key checks: `UI-P1-022` pass, `UI-P1-025A..E` pass
- Spatial smoke: `TestEvidence/bl022_validation_20260224T184032Z/qa_smoke_spatial.log` (`4 PASS / 0 WARN / 0 FAIL`)

## Closeout Checklist

- [x] Choreography validation lane passes
- [x] BL-025 regression guard green
- [x] Evidence captured at designated paths
- [x] status.json updated
- [x] Documentation/backlog/index.md row updated
- [x] TestEvidence surfaces updated
- [x] ./scripts/validate-docs-freshness.sh passes


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-022-choreography-closeout.md` |

### Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | Legacy closeout documented | `Evidence References` |
| Lane replay/parity | Legacy closeout documented | `Evidence References` |
| RT safety | Legacy closeout documented | `Evidence References` |
| Docs freshness | Legacy closeout documented | `Evidence References` |
| Status schema | Legacy closeout documented | `Evidence References` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | Required for modern promotions; legacy packets may predate marker | `Evidence References` |

### Backlog/Status Sync Checklist

- [x] Runbook archived under `Documentation/backlog/done/`
- [x] Backlog index links the done runbook
- [x] Historical evidence references retained
- [ ] Legacy packet retrofitted to modern owner packet template (`_template-promotion-decision.md`) where needed
- [ ] Legacy closeout fully normalized to modern checklist fields where needed
