Title: BL-079 APVTS Parameter Grouping and Host Hierarchy
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-07
Last Modified Date: 2026-03-19

# BL-079: APVTS Parameter Grouping and Host Hierarchy

## Plain-Language Summary

BL-079 in plain terms: organize LocusQ's parameter list into sensible host-visible groups so DAWs can show cleaner sections like Calibration, Emitter, and Renderer without breaking existing sessions, automation IDs, or the WebView bridge. Current state: In Validation. The grouped APVTS tree is implemented locally, the clean-checkout replay passes, REAPER VST3 host verification now passes, and REAPER AU still shows a flat-order mismatch that blocks promotion.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and maintainers working in DAWs that surface parameter groups. |
| What is changing? | The APVTS parameter layout now exposes grouped host hierarchy instead of one flat list. |
| Why is this important? | It improves host-side organization and reduces parameter-navigation friction without changing the canonical IDs other systems depend on. |
| How will we deliver it? | Keep IDs/defaults/order stable, build the grouped JUCE tree in `ProcessorParameterLayout.cpp`, then validate parity and host behavior. |
| When is it done? | When grouped layout builds cleanly, preserves ID/order parity, and focused promotion validation confirms no host/runtime regressions. |
| Where is the source of truth? | This runbook, `Documentation/backlog/index.md`, `Documentation/architecture-code-review-2026-03-06.md`, and evidence updates in `TestEvidence/`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status legend + progress snapshot | Fast read on done vs remaining work without relying on color. | `## Status Legend`, `## Progress Snapshot` |
| Group hierarchy table | Makes the new host-visible APVTS structure easy to scan. | `## Implementation Slices` |
| Validation table | Clarifies what counts as implementation-complete vs promotion-ready. | `## Validation Plan` |

## Status Legend

- `[DONE]` completed slice or milestone; use `~~strikethrough~~` on the slice name when appropriate.
- `[ACTIVE]` current focus with meaningful remaining work.
- `[NEXT]` next recommended slice after the active one.
- `[QUEUED]` planned but not current focus.
- `[BLOCKED]` waiting on dependency or failing validation lane.
- Portable markdown only: no HTML/CSS color.
- If exact time or tokens were not logged, use `not logged` or `n/a`.

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-079 |
| Priority | P2 |
| Status | In Validation |
| Owner Track | F - Hardening |
| Depends On | BL-032 |
| Blocks | — |
| Annex Spec | `(no annex spec — self-contained runbook)` |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Progress Snapshot

| Item | Status | Priority | Estimate | Actual / Time | Tokens | Updated | Where | Remaining |
|---|---|---|---|---|---|---|---|---|
| `~~Slice A~~` grouped APVTS tree implementation | `[DONE]` | P2 | Small | focused refactor completed 2026-03-07 | `n/a` | 2026-03-07 | `Source/processor_core/ProcessorParameterLayout.cpp` | none |
| `~~Slice B~~` promotion validation | `[DONE]` | P2 | Small | fresh clean-checkout replay passed 2026-03-19 | `n/a` | 2026-03-19 | `build_bl079_check2`, `TestEvidence/bl079_validation_20260319T030000Z` | none |
| Host smoke follow-up | `[ACTIVE]` | P2 | Small | REAPER VST3 gate PASS; REAPER AU gate FAIL on flat-order check (`Mode` at idx `1`, expected `0`) | `n/a` | 2026-03-19 | `TestEvidence/bl079_param_group_host_gate_*_20260319T042105Z` | resolve AU host ordering mismatch or narrow the acceptance contract honestly |

## Objective

Expose LocusQ's APVTS as a grouped host hierarchy so DAWs that understand parameter groups can present a cleaner tree, while preserving the plugin's compatibility contracts: stable parameter IDs, stable defaults, and unchanged flattened ordering for legacy automation and QA lanes that still depend on raw indices.

## Scope & Non-Scope

**In scope:**
- `Source/processor_core/ProcessorParameterLayout.cpp`
- JUCE `AudioProcessorParameterGroup` hierarchy for Global, Calibration, Emitter, and Renderer
- ID/default/order parity checks against the pre-grouping layout
- Architecture/backlog/status/evidence synchronization

**Out of scope:**
- Renaming or removing parameter IDs
- Changing parameter defaults or ranges
- WebView/UI runtime changes
- New automation lanes or host-specific UI redesign

## Architecture Context

- Invariants: `Documentation/invariants.md`
- ADRs: `Documentation/adr/ADR-0003-automation-authority-precedence.md`, `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- Architecture review authority: `Documentation/architecture-code-review-2026-03-06.md`
- Related implementation lane: `Documentation/backlog/bl-039-parameter-relay-spec-generation.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Replace the flat APVTS vector with a grouped JUCE parameter tree while preserving parameter identity and order. | `Source/processor_core/ProcessorParameterLayout.cpp` | BL-032/W0-A extracted the layout authority into its own file | 4 top-level groups + 11 nested subgroups land with `90/90` ID parity and unchanged flattened order |
| B | Promotion validation and host verification for the new grouped layout. | `build_local`, representative host/QA surfaces, docs/evidence surfaces | Slice A complete | build is green, parity evidence is recorded, and clean-checkout follow-up confirms no host/runtime regressions |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL079-BUILD | Automated | `cmake --build build_local --config Release --target LocusQ locusq_qa -- -j8` | Exit 0 |
| BL079-PARITY | Automated | local source-parity script comparing pre/post W1-D IDs and order | `90/90` IDs match and flattened order is unchanged |
| BL079-QA-FOLLOWUP | Automated | `build_local/locusq_qa_artefacts/Release/locusq_qa --spatial ...` representative scenario replay in a clean checkout | scenario result emits normally and does not regress |
| BL079-HOST-SMOKE | Automated + manual follow-up | `scripts/reaper-param-group-host-gate-mac.sh --format VST3` and `scripts/reaper-param-group-host-gate-mac.sh --format AU` | VST3 and AU both satisfy the host gate, or any format-specific mismatch is recorded honestly and handled before promotion |

## Validation Snapshot (2026-03-19)

- `cmake -S . -B build_bl079_check -DBUILD_LOCUSQ_QA=ON -DLOCUSQ_ENABLE_STEAM_AUDIO=OFF -DCMAKE_BUILD_TYPE=Release` -> `PASS`
- `cmake --build build_bl079_check --config Release --target LocusQ locusq_qa -j8` -> `PASS`
- `build_bl079_check/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> `FAIL`
- top finding: `locusq_rt_safety_emitter (FAIL) [allocation_free] perf_allocation_free=false (expected: true)`
- evidence root: `TestEvidence/bl079_validation_20260319T014000Z`
- `cmake -S . -B build_bl079_check2 -DBUILD_LOCUSQ_QA=ON -DCMAKE_BUILD_TYPE=Release` -> `PASS`
- `cmake --build build_bl079_check2 --config Release --target LocusQ locusq_qa -j8` -> `PASS`
- `build_bl079_check2/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> `PASS_WITH_WARNING`
- top finding: `locusq_emitter_passthrough (WARN) [rms_level] rms=-29.667015 dBFS (range: -10.000000 to -5.000000)`
- `scripts/reaper-param-group-host-gate-mac.sh --format VST3` -> `PASS`
- VST3 host gate: `param_count=215`, all required group-boundary names present, `Mode` at index `0`, renderer section ordered after emitter identity
- evidence root: `TestEvidence/bl079_param_group_host_gate_vst3_20260319T042105Z`
- `scripts/reaper-param-group-host-gate-mac.sh --format AU` -> `FAIL`
- AU host gate: `param_count=214`, all required names present, but `Mode` is at index `1` instead of `0`
- evidence root: `TestEvidence/bl079_param_group_host_gate_au_20260319T042105Z`
- disposition: shared RT-safety blocker is cleared; VST3 host verification is now green, but BL-079 remains in validation because AU host ordering does not yet satisfy the same flat-order contract
- evidence root: `TestEvidence/bl079_validation_20260319T030000Z`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 1-3 | build + source-parity checks | build logs, parity summary |
| Candidate intake | T2 | 5 (if a dedicated QA lane is added) | representative QA replay | replay summary + blocker notes |
| Promotion | T3 | owner-approved equivalent | host/QA parity packet | owner packet or promotion note |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Legacy automation or QA depends on flattened parameter indices | High | Medium | Keep declaration order unchanged and verify parity against the pre-grouping source |
| Host-specific parameter-tree presentation differs across formats | High | Medium | Treat format-specific host evidence as a real gate, not a documentation afterthought |
| AU host ordering differs even when required names are present | High | Medium | Keep BL-079 in validation until the AU mismatch is fixed or the acceptance contract is narrowed honestly |
| Concurrent editor/toolchain work muddies validation signal | Medium | High | Keep promotion follow-up in a clean checkout or separate worktree |

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Build summary update | `TestEvidence/build-summary.md` | command, result, scope |
| Validation trend entry | `TestEvidence/validation-trend.md` | timestamp, command, result, notes |
| Status sync | `status.json` | phase note, timestamp |

## Closeout Checklist

- [x] Grouped APVTS implementation landed
- [x] Architecture review updated
- [x] Backlog index row added
- [x] Build/parity evidence recorded
- [x] Clean-checkout configure/build replay captured
- [x] Clean-checkout QA replay captured with smoke lane green
- [x] Representative VST3 host parameter-view gate captured
- [ ] Representative AU host parameter-view gate reconciled
- [ ] Promotion decision recorded
