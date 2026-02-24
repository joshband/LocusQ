Title: LocusQ Post-v1 Master Backlog
Document Type: Backlog and Execution Spec
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-24

# LocusQ Post-v1 Master Backlog

> **DEPRECATED (2026-02-24):** This file is superseded by `Documentation/backlog/index.md` as the single backlog authority. Individual runbook docs in `Documentation/backlog/` now carry execution detail, agent prompts, and validation plans. This file is retained as Tier 2 reference only. Do not update this file for new status changes.

## Purpose
Provide one canonical backlog authority for ordering, status, dependencies, and closeout criteria across BL/HX work items.

## Scope
This file is the master backlog only. Deep design and implementation details live in referenced annex plans and runbooks.

## Canonical Backlog Contract
1. This file is the single authority for backlog status, ordering, and priority.
2. `Documentation/plans/*.md` are specialist annexes; they must not contain authoritative backlog state.
3. `Documentation/plans/2026-02-20-full-project-review.md` is extraction input only, not execution authority.
4. Execution steps and command-level procedures live in `Documentation/runbooks/backlog-execution-runbooks.md`.
5. Every open item in this file must include dependencies, owner track, and required exit artifact.
6. Any status/priority change must update this file and evidence surfaces in the same change set.

## Master + Annex Model
| Layer | Role | Authority |
|---|---|---|
| Master backlog (this file) | Priority, sequencing, status, dependencies, ownership | Authoritative |
| Annex plans (`Documentation/plans/*.md`) | Deep architecture/spec details per BL lane | Supporting |
| Runbooks (`Documentation/runbooks/backlog-execution-runbooks.md`) | Actionable procedures, validation commands, evidence paths | Supporting |
| Archived reviews (`Documentation/archive/...`) | Historical context and extraction source | Reference only |

## Material Preservation Map
| Backlog ID | Primary Annex Docs |
|---|---|
| BL-011 | `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`; `Documentation/plans/LocusQClapContract.h` |
| BL-013 | `Documentation/plans/bl-013-hostrunner-feasibility-2026-02-23.md` |
| BL-017 | `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md` |
| BL-024 | `Documentation/plans/reaper-host-automation-plan-2026-02-22.md` |
| BL-025 | `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md` |
| BL-026 | `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`; `Documentation/plans/bl-026-calibrate-v1-v2-uiux-comparison-2026-02-23.md` |
| BL-027 | `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md` |
| BL-028 | `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md` |
| BL-029 | `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md` |
| BL-031 | `Documentation/plans/bl-031-tempo-locked-visual-token-scheduler-spec-2026-02-24.md` |

## Extraction Coverage From Full Review Plan
Source: `Documentation/plans/2026-02-20-full-project-review.md`

| Review Bundle | Backlog Landing |
|---|---|
| Stage 15 parameter exposure + ADR/traceability | Closed archive + governance surfaces (already landed) |
| Stage 16-A/E QA gap scenarios | HX-04 parity guard (landed, recurring) |
| Stage 16-B RT safety static audit | HX-06 recurring hardening lane |
| Stage 16-C research integration | BL-017/028/029 annex planning chain |
| Stage 16-D viewport scope policy | ADR-0008 + BL-026/027/029 coordination |
| Stage 17 release/device rerun discipline | BL-030 release governance lane |
| Tempo-lock user proposal (2026-02-24) | BL-031 tempo-token scheduler lane |

## Status Snapshot (2026-02-24)
- `Done`: BL-001..BL-011, BL-014, BL-015, BL-016, BL-019, BL-024, BL-025
- `In Progress`: none
- `In Validation`: BL-012, BL-013, BL-018, BL-022
- `In Planning`: BL-017, BL-026, BL-027, BL-028, BL-029, BL-030, BL-031
- `Todo`: BL-020, BL-021, BL-023
- `HX Open`: HX-02, HX-05, HX-06

## Structured TODO Registry (Canonical Queue)
| Order | Priority | ID | State | Depends On | Owner Track | Exit Artifact |
|---:|---|---|---|---|---|---|
| 1 | P1 | BL-018 | In Validation | BL-014 stable | Track A Runtime Formats | Strict warning-free profile-matrix rerun |
| 2 | P1 | BL-022 | In Validation | BL-003, BL-004 done | Track C UX Authoring | Choreography lane closeout evidence and BL-025 regression guard |
| 3 | P1 | BL-012 | In Validation | none | Track D QA Platform | Tranche closeout + embedded HX-04 parity evidence |
| 4 | P1 | BL-013 | In Validation | BL-012 | Track D QA Platform | HostRunner feasibility promotion decision with rerun evidence |
| 5 | P1 | BL-026 | In Planning | BL-025 baseline, BL-009/BL-018 diagnostics stable | Track C UX Authoring | Slice A-E implementation + `UI-P1-026A..E` evidence |
| 6 | P1 | BL-031 | In Planning (spec complete) | BL-016 baseline, BL-025 baseline | Track B Scene/UI Runtime | Tempo-token scheduler slices A-D + `UI-P2-031A..D` rhythm-lock evidence |
| 7 | P1 | HX-02 | Open | BL-016 baseline | Track F Hardening | Registration lock/memory-order audit and fixes |
| 8 | P1 | HX-06 | Open | BL-016 baseline | Track F Hardening | Recurring RT-safety static-audit lane + CI/report integration |
| 9 | P2 | BL-027 | In Planning | BL-026 | Track C UX Authoring | First renderer-v2 slice + validation promotion |
| 10 | P2 | BL-028 | In Planning | BL-017, BL-026, BL-027 | Track A + Track C | Matrix legality enforcement + requested/active/stage parity evidence |
| 11 | P2 | BL-029 | In Planning | BL-025, BL-026, BL-027, BL-028, BL-031 | Track B Scene/UI Runtime | `UI-P2-029A..E` lanes + schema sync |
| 12 | P2 | HX-05 | Open | BL-016, BL-025 | Track F Hardening | Payload budget + throttle contract enforcement evidence |
| 13 | P2 | BL-030 | In Planning | BL-024 baseline, BL-025 baseline, HX-06 active | Track G Release/Governance | Release/device-rerun checklist + synchronized closeout evidence |
| 14 | P2 | BL-017 | In Planning | BL-009, BL-018 | Track E R&D Expansion | Slice-A bridge implementation + deterministic contract lane |
| 15 | P2 | BL-020 | Todo | BL-014, BL-019 | Track E R&D Expansion | Confidence/masking mapping and validation matrix |
| 16 | P2 | BL-021 | Todo | BL-014, BL-015 | Track E R&D Expansion | Room-story overlays with deterministic payload contracts |
| 17 | P2 | BL-023 | Todo | BL-025 baseline | Track C UX Authoring | Resize/DPI hardening evidence matrix |

## Dependency and Ordering Rules
1. Keep BL-016 transport contract stable before promoting BL-014 and BL-019-dependent lanes.
2. Keep BL-025 as UX baseline gate before BL-023 and BL-026 execution.
3. Promote BL-018 after BL-014 strict closeout only.
4. Keep BL-017 behind BL-009 and BL-018 stability.
5. Keep BL-027 behind BL-026 shared profile alias and diagnostics contracts.
6. Keep BL-028 matrix enforcement aligned with BL-017/026/027 contracts.
7. Keep BL-031 ahead of BL-029 promotion so rhythm-locked visual timing authority is deterministic.
8. Keep BL-029 behind BL-025/026/027/028/031 contract stability.
9. Keep HX-05 tied to BL-016 and BL-025 closeout.
10. Keep HX-06 active before BL-030 release-governance promotion.
11. Preserve HX-04 parity guard in every BL-012 rerun.

## Parallel AI Agent Tracks
| Track | Scope | Skills |
|---|---|---|
| Track A Runtime Formats | BL-018, BL-028 format/routing contracts | `steam-audio-capi`, `clap-plugin-lifecycle`, `spatial-audio-engineering`, `skill_docs` |
| Track B Scene and UI Runtime | BL-031, BL-029, BL-016-adjacent runtime behavior, and BL-014 regression maintenance | `juce-webview-runtime`, `reactive-av`, `threejs`, `physics-reactive-audio`, `skill_impl`, `skill_docs` |
| Track C UX Authoring | BL-026, BL-027, BL-022, BL-023, and BL-025 regression maintenance | `skill_design`, `juce-webview-runtime`, `threejs`, `skill_plan`, `skill_docs` |
| Track D QA Platform | BL-012, BL-013, BL-024 cadence | `skill_test`, `skill_testing`, `skill_troubleshooting`, `skill_plan` |
| Track E R&D Expansion | BL-017, BL-020, BL-021 | `skill_plan`, `skill_dream`, `reactive-av`, `threejs` |
| Track F Hardening | HX-02, HX-05, HX-06 | `skill_impl`, `skill_testing`, `juce-webview-runtime`, `skill_docs` |
| Track G Release and Governance | BL-030 closeout synchronization | `skill_docs`, `skill_plan`, `skill_test`, `skill_ship` |

## Runbook References
- Primary execution runbook: `Documentation/runbooks/backlog-execution-runbooks.md`
- Annex specs: see Material Preservation Map above.
- Historical extraction source: `Documentation/plans/2026-02-20-full-project-review.md`

## Closed Archive
| ID | Summary | Status |
|---|---|---|
| BL-001 | README standards and structure | Done (2026-02-21) |
| BL-002 | Physics preset host reversion fix | Done (2026-02-21) |
| BL-003 | Timeline transport controls restore | Done (2026-02-21) |
| BL-004 | Keyframe editor gestures in production UI | Done (2026-02-21) |
| BL-005 | Preset save host path fix | Done (2026-02-21) |
| BL-006 | Motion trail overlays | Done (2026-02-21) |
| BL-007 | Velocity vector overlays | Done (2026-02-21) |
| BL-008 | Audio-reactive RMS overlays | Done (2026-02-21) |
| BL-009 | Steam headphone contract closeout | Done (2026-02-23) |
| BL-010 | FDN expansion promotion | Done (2026-02-23) |
| BL-011 | CLAP lifecycle and CI/host closeout | Done (2026-02-23) |
| BL-014 | Listener/speaker/aim/RMS overlay strict closeout | Done (2026-02-24) |
| BL-015 | All-emitter realtime rendering closure | Done (2026-02-23) |
| BL-016 | Visualization transport contract closure | Done (2026-02-23) |
| BL-019 | Physics interaction lens closure | Done (2026-02-23) |
| BL-025 | EMITTER UI/UX v2 deterministic closeout | Done (2026-02-24) |
| HX-01 | shared_ptr atomic migration guard | Done (2026-02-23) |
| HX-03 | REAPER multi-instance stability lane | Done (2026-02-23) |
| HX-04 | Scenario coverage audit and drift guard | Done (2026-02-23) |

## Definition of Ready
1. Objective, dependency gate, owner track, and exit artifact are explicit.
2. Annex plan and runbook references are present.
3. Validation commands and evidence destinations are defined.

## Definition of Done
1. Code/docs changes merged.
2. Required validation commands pass with recorded artifacts.
3. `status.json`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, and this backlog are synchronized when claims change.
4. `./scripts/validate-docs-freshness.sh` passes.

## Maintenance Rule
Any backlog decision made in plan, implementation, validation, or ADR updates must be reflected here in the same change set.
