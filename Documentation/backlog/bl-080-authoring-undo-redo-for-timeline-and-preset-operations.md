Title: BL-080 Authoring Undo/Redo for Timeline and Preset Operations
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-07
Last Modified Date: 2026-03-18

# BL-080: Authoring Undo/Redo for Timeline and Preset Operations

## Plain-Language Summary

BL-080 adds a real undo/redo history for the authoring flows the DAW host does not cover well on its own: manual keyframe edits, choreography/timeline changes, and preset save/load/rename/delete actions coming from the WebView UI. Current state: Done-candidate. The processor-side history engine, WebView controls, and standalone validation lane are now green, with the rebuilt standalone production self-test explicitly passing `UI-W3A-01` and `UI-W3A-02`; the remaining step is owner promotion and closeout sync.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Operators authoring motion/keyframe content in the WebView UI, plus maintainers validating preset/timeline state safety. |
| What is changing? | The processor now stores undo/redo history snapshots for timeline and preset operations, and the WebView exposes undo/redo buttons, status, and keyboard shortcuts. |
| Why is this important? | DAW undo covers APVTS automation well, but custom keyframe/preset workflows had no authoring recovery path before this lane. |
| How will we deliver it? | Add a processor-owned snapshot/file history engine, route native preset/timeline mutations through it, then expose undo/redo controls and validation hooks in the WebView. |
| When is it done? | When the rebuilt standalone app replays the production self-test with both embedded `UI-W3A-01` / `UI-W3A-02` checks passing, and the owner promotion note records that evidence. |
| Where is the source of truth? | This runbook, `Documentation/architecture-code-review-2026-03-06.md`, `Documentation/backlog/index.md`, and evidence under `TestEvidence/`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status legend + progress snapshot | Fast read on what is implemented vs what remains blocked. | `## Status Legend`, `## Progress Snapshot` |
| Implementation metrics table | Shows the scope of the history/runtime/UI changes without opening every file. | `## Implementation Metrics` |
| Validation table | Separates green build evidence from the blocked standalone replay. | `## Validation Plan` |

## Status Legend

- `[DONE]` completed slice or milestone; use `~~strikethrough~~` on the slice name when appropriate.
- `[ACTIVE]` current focus with meaningful remaining work.
- `[NEXT]` next recommended slice after the active one.
- `[QUEUED]` planned but not current focus.
- `[BLOCKED]` waiting on a failing validation lane or dependency outside the slice itself.
- Portable markdown only: no HTML/CSS color.
- If exact time or tokens were not logged, use `not logged` or `n/a`.

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-080 |
| Priority | P3 |
| Status | Done-candidate |
| Owner Track | F - Hardening |
| Depends On | BL-070, BL-074 |
| Blocks | — |
| Annex Spec | `(no annex spec — self-contained runbook)` |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Progress Snapshot

| Item | Status | Priority | Estimate | Actual / Time | Tokens | Updated | Where | Remaining |
|---|---|---|---|---|---|---|---|---|
| `~~Slice A~~` processor-owned authoring history core | `[DONE]` | P3 | Medium | focused local slice completed 2026-03-07 | `n/a` | 2026-03-07 | `Source/PluginProcessor.h`, `Source/processor_core/ProcessorAuthoringHistory.cpp`, `Source/processor_core/ProcessorPresetManager.cpp` | none |
| `~~Slice B~~` native bridge + WebView undo/redo controls | `[DONE]` | P3 | Medium | focused local slice completed 2026-03-07 | `n/a` | 2026-03-07 | `Source/editor_webview/EditorWebViewRuntime.h`, `Source/ui/src/index.ts`, `Source/ui/public/index.html` | none |
| `~~Slice C~~` production validation + CALIBRATE replay cleanup | `[DONE]` | P3 | Small | focused validation completed 2026-03-18 | `n/a` | 2026-03-18 | `build_local`, `TestEvidence/locusq_production_p0_selftest_20260318T022435Z.json` | none |
| Slice D owner promotion and archive sync | `[NEXT]` | P3 | Small | not started | `n/a` | 2026-03-18 | backlog/status/evidence sync surfaces | record promotion decision and archive/closeout sync |

## Objective

Add deterministic undo/redo coverage for keyframe timeline edits plus preset lifecycle operations initiated from the WebView UI, while preserving the existing processor authority model, preset-file contract, and native UI-state persistence behavior.

## Scope & Non-Scope

**In scope:**
- processor-owned authoring snapshot/history model
- keyframe timeline undo/redo commits
- preset save/load/rename/delete history entries
- native undo/redo status + action bindings
- WebView undo/redo buttons, shortcuts, and self-test hooks
- roadmap/backlog/status/evidence synchronization

**Out of scope:**
- DAW host undo integration redesign
- a broad `juce::UndoManager` retrofit
- unrelated CALIBRATE topology alias cleanup beyond what is needed to unblock W3-A validation
- mode-transition crossfade (`W3-B`)

## Architecture Context

- Invariants: `Documentation/invariants.md`
- ADRs: `Documentation/adr/ADR-0003-automation-authority-precedence.md`, `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- Architecture review authority: `Documentation/architecture-code-review-2026-03-06.md`
- Related backlog lanes: `Documentation/backlog/bl-070-coherent-audio-snapshot-and-telemetry-seqlock-contract.md`, `Documentation/backlog/done/bl-074-webview-runtime-reliability-diagnostics-strict-gesture-and-degraded-mode.md`

## Implementation Metrics

| File | Delta | Current Size / Note |
|---|---|---|
| `Source/processor_core/ProcessorAuthoringHistory.cpp` | new file | `340` LOC new processor history unit |
| `Source/PluginProcessor.h` | `+60 / -0` | `507` LOC with authoring-history contracts and storage |
| `Source/processor_core/ProcessorPresetManager.cpp` | `+115 / -27` | `710` LOC with preset operations routed through history snapshots |
| `Source/editor_webview/EditorWebViewRuntime.h` | `+19 / -1` | `761` LOC with undo/redo/status native bindings |
| `Source/ui/src/index.ts` | `+666 / -40` | `14424` LOC with authoring-history UI, shortcuts, and self-test hooks |
| `Source/ui/public/index.html` | `+186 / -4` | `2475` LOC with timeline undo/redo controls and production-shell markup |
| `CMakeLists.txt` | `+3 / -0` | adds `ProcessorAuthoringHistory.cpp` to plugin + QA builds |

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Add processor-owned authoring state snapshots plus undo/redo stacks for timeline and preset actions. | `Source/PluginProcessor.h`, `Source/processor_core/ProcessorAuthoringHistory.cpp`, `Source/processor_core/ProcessorPresetManager.cpp` | W2-A processor seams are already in place | undo/redo entries can capture/apply authoring state and preset-file state without changing parameter IDs or preset schema |
| B | Expose undo/redo through the native bridge and WebView controls. | `Source/editor_webview/EditorWebViewRuntime.h`, `Source/ui/src/index.ts`, `Source/ui/public/index.html` | Slice A complete | buttons, shortcuts, and status wiring exist; generated bundle embeds `UI-W3A-01` and `UI-W3A-02` |
| C | Replay rebuilt standalone validation and clear the CALIBRATE blocker so the new W3-A checks execute end-to-end. | `build_local`, `TestEvidence/` | Slices A and B complete | production self-test reaches and records `UI-W3A-01` and `UI-W3A-02` without the earlier CALIBRATE alias failure |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL080-TS | Automated | `cd Source/ui && npm run typecheck` | Exit 0 |
| BL080-BUNDLE | Automated | `cd Source/ui && npm run build` | Exit 0 and `Source/ui/generated/index.js` refreshes cleanly |
| BL080-BUILD | Automated | `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -- -j8` | Exit 0 |
| BL080-BUNDLE-MARKERS | Automated | `rg -o "UI-W3A-[0-9]+" Source/ui/generated/index.js | sort -u` | emits `UI-W3A-01` and `UI-W3A-02` |
| BL080-SELFTEST | Automated | `./scripts/standalone-ui-selftest-production-p0-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app` | `PASS`; rebuilt standalone replay records `UI-W3A-01` and `UI-W3A-02` in `TestEvidence/locusq_production_p0_selftest_20260318T022435Z.json` |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 1 | typecheck + bundle + standalone/QA build | build logs + bundle marker proof |
| Candidate intake | T2 | 3-5 | rebuilt standalone self-test once the CALIBRATE blocker is cleared | self-test JSON + failure taxonomy |
| Promotion | T3 | owner-approved equivalent | rebuilt standalone replay + focused follow-up on preset/timeline checks | owner packet or promotion note |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Undo/redo restores state the host does not expect | High | Medium | keep history scoped to authoring-state snapshots and preset-file changes, not raw APVTS automation rewrites |
| Preset save/rename/delete undo corrupts file-library state | High | Medium | capture file snapshots before/after each file operation and restore atomically through the processor helper |
| WebView buttons/shortcuts drift from processor history truth | Medium | Medium | drive button enabled/disabled state from native `locusqGetAuthoringHistoryStatus` response only |
| Production self-test regresses before W3-A checks in future UI work | Medium | Medium | keep the rebuilt standalone production self-test in the dev loop and retain the passing BL-080 evidence artifact as the baseline |

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Build summary update | `TestEvidence/build-summary.md` | command, result, scope |
| Validation trend entry | `TestEvidence/validation-trend.md` | timestamp, command, result, notes |
| Standalone replay artifact | `TestEvidence/locusq_production_p0_selftest_<timestamp>.json` | status, failure/check details |
| Status sync | `status.json` | timestamped implementation note |

## Closeout Checklist

- [x] Processor authoring-history core landed
- [x] Native undo/redo/status bridge landed
- [x] WebView undo/redo controls and shortcuts landed
- [x] Generated bundle embeds `UI-W3A-01` and `UI-W3A-02`
- [x] Architecture review updated
- [x] Backlog index row added
- [x] Build/typecheck evidence recorded
- [x] Rebuilt standalone self-test reaches and records `UI-W3A-01`
- [x] Rebuilt standalone self-test reaches and records `UI-W3A-02`
- [ ] Promotion decision recorded
