Title: LocusQ Master Backlog Index
Document Type: Backlog Index
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-25

# LocusQ Master Backlog Index

## Purpose

Single canonical backlog authority for priority, ordering, status, dependencies, and closeout criteria across all BL/HX work items. This file is a dashboard — detailed execution content lives in individual runbook docs alongside annex plan specs.

## Canonical Contract

1. This file is the single authority for backlog status, ordering, and priority.
2. Individual runbook docs (`Documentation/backlog/bl-XXX-*.md`) carry execution detail, agent prompts, and validation plans.
3. Annex plan specs (`Documentation/plans/*.md`) carry deep architecture; they must not contain authoritative backlog state.
4. Every open item must have a corresponding runbook doc with dependencies, agent mega-prompts, and exit criteria.
5. Any status/priority change must update this file, the runbook's Status Ledger, and evidence surfaces in the same changeset.
6. Intake process for new items uses `Documentation/backlog/_template-intake.md`.

## Layer Model

| Layer | Role | Authority |
|---|---|---|
| Master index (this file) | Priority, sequencing, status, dependencies, dashboard | Authoritative |
| Runbook docs (`Documentation/backlog/bl-XXX-*.md`) | Execution detail, agent prompts, validation plans, evidence contracts | Execution |
| Annex specs (`Documentation/plans/*.md`) | Deep architecture/spec details per BL lane | Supporting |
| Archive (`Documentation/archive/`) | Historical context and extraction source | Reference only |

## Active Queue

| # | ID | Title | Priority | Status | Track | Depends On | Blocks | Runbook |
|--:|-----|-------|----------|--------|-------|------------|--------|---------|
| 1 | BL-013 | HostRunner feasibility promotion | P1 | In Validation | D | BL-012 | — | [bl-013](bl-013-hostrunner-feasibility.md) |
| 2 | BL-026 | CALIBRATE UI/UX v2 multi-topology | P1 | In Implementation (Slices A-C owner-validated; D-E pending) | C | BL-025, BL-009, BL-018 | BL-027, BL-028, BL-029 | [bl-026](bl-026-calibrate-uiux-v2.md) |
| 3 | BL-031 | Tempo-locked visual token scheduler | P1 | In Implementation (Slices A-B validated) | B | BL-016, BL-025 | BL-029 | [bl-031](bl-031-tempo-token-scheduler.md) |
| 4 | HX-02 | Registration lock / memory-order audit | P1 | In Validation (Slices A-B complete) | F | BL-016 | — | [hx-02](hx-02-registration-lock.md) |
| 5 | HX-06 | Recurring RT-safety static audit | P1 | In Validation | F | BL-016 | BL-030 | [hx-06](hx-06-rt-safety-audit.md) |
| 6 | BL-027 | RENDERER UI/UX v2 multi-profile | P2 | In Planning | C | BL-026 | BL-028, BL-029 | [bl-027](bl-027-renderer-uiux-v2.md) |
| 7 | BL-028 | Spatial output matrix enforcement | P2 | In Planning | A+C | BL-017, BL-026, BL-027 | BL-029 | [bl-028](bl-028-spatial-output-matrix.md) |
| 8 | BL-029 | DSP visualization and tooling | P2 | In Implementation (reactive tranche feature lanes advanced, but reliability tranche NO-GO: soak selftests unstable in R1/R2/R3 despite deterministic QA contract and RT/docs green snapshots) | B | BL-025, BL-026, BL-027, BL-028, BL-031 | — | [bl-029](bl-029-dsp-visualization.md) |
| 9 | HX-05 | Payload budget and throttle contract | P2 | Open | F | BL-016, BL-025 | — | [hx-05](hx-05-payload-budget.md) |
| 10 | BL-030 | Release governance and device rerun | P2 | In Validation (Slices A-D complete; release dry-run blocked on RL-01/RL-05/RL-09) | G | BL-024, BL-025, HX-06 | — | [bl-030](bl-030-release-governance.md) |
| 11 | BL-017 | Head-tracked monitoring companion bridge | P2 | In Implementation (Slices A-B validated) | E | BL-009, BL-018 | BL-028 | [bl-017](bl-017-head-tracked-monitoring.md) |
| 12 | BL-020 | Confidence/masking overlay mapping | P2 | Todo | E | BL-014, BL-019 | — | [bl-020](bl-020-confidence-masking.md) |
| 13 | BL-021 | Room-story overlays | P2 | Todo | E | BL-014, BL-015 | — | [bl-021](bl-021-room-story-overlays.md) |
| 14 | BL-023 | Resize/DPI hardening | P2 | Todo | C | BL-025 | — | [bl-023](bl-023-resize-dpi-hardening.md) |
| 15 | BL-032 | Source modularization of PluginProcessor/PluginEditor | P2 | In Planning | F | — | — | [bl-032](bl-032-source-modularization.md) |

## Dependency Graph

```mermaid
graph TD
    subgraph Done
        BL-003[BL-003 Done]
        BL-004[BL-004 Done]
        BL-009[BL-009 Done]
        BL-012[BL-012 Done]
        BL-014[BL-014 Done]
        BL-018[BL-018 Done]
        BL-022[BL-022 Done]
        BL-015[BL-015 Done]
        BL-016[BL-016 Done]
        BL-019[BL-019 Done]
        BL-024[BL-024 Done]
        BL-025[BL-025 Done]
    end

    subgraph "In Validation"
        BL-013[BL-013 HostRunner]
        HX-06[HX-06 RT Audit]
        BL-030[BL-030 Release Gov]
    end

    subgraph "In Planning / Open"
        BL-026[BL-026 Calibrate v2]
        BL-027[BL-027 Renderer v2]
        BL-028[BL-028 Output Matrix]
        BL-029[BL-029 DSP Viz]
        BL-031[BL-031 Tempo Token]
        BL-017[BL-017 Head Track]
        HX-02[HX-02 Reg Lock]
        HX-05[HX-05 Payload]
        BL-032[BL-032 Source Modularization]
    end

    subgraph "Todo"
        BL-020[BL-020 Confidence]
        BL-021[BL-021 Room Story]
        BL-023[BL-023 Resize/DPI]
    end

    BL-014 --> BL-018
    BL-018 --> BL-026
    BL-025 --> BL-026
    BL-009 --> BL-026
    BL-026 --> BL-027
    BL-026 --> BL-028
    BL-027 --> BL-028
    BL-017 --> BL-028
    BL-031 --> BL-029
    BL-025 --> BL-029
    BL-026 --> BL-029
    BL-027 --> BL-029
    BL-028 --> BL-029
    BL-016 --> BL-031
    BL-025 --> BL-031
    BL-016 --> HX-02
    BL-016 --> HX-05
    BL-025 --> HX-05
    BL-016 --> HX-06
    HX-06 --> BL-030
    BL-024 --> BL-030
    BL-025 --> BL-030
    BL-009 --> BL-017
    BL-018 --> BL-017
    BL-003 --> BL-022
    BL-004 --> BL-022
    BL-012 --> BL-013
    BL-014 --> BL-020
    BL-019 --> BL-020
    BL-014 --> BL-021
    BL-015 --> BL-021
    BL-025 --> BL-023
```

## Parallel Agent Tracks

| Track | Name | Scope | Skills |
|---|---|---|---|
| A | Runtime Formats | BL-018, BL-028 | `steam-audio-capi`, `clap-plugin-lifecycle`, `spatial-audio-engineering`, `skill_docs` |
| B | Scene/UI Runtime | BL-031, BL-029, BL-016-adjacent | `juce-webview-runtime`, `reactive-av`, `threejs`, `physics-reactive-audio`, `skill_impl`, `skill_docs` |
| C | UX Authoring | BL-026, BL-027, BL-023, BL-025 regression | `skill_design`, `juce-webview-runtime`, `threejs`, `skill_plan`, `skill_docs` |
| D | QA Platform | BL-012, BL-013, BL-024 cadence | `skill_test`, `skill_testing`, `skill_troubleshooting`, `skill_plan` |
| E | R&D Expansion | BL-017, BL-020, BL-021 | `skill_plan`, `skill_dream`, `reactive-av`, `threejs` |
| F | Hardening | HX-02, HX-05, HX-06, BL-032 | `skill_impl`, `skill_testing`, `juce-webview-runtime`, `skill_docs` |
| G | Release/Governance | BL-030 | `skill_docs`, `skill_plan`, `skill_test`, `skill_ship` |

## Intake Process

1. **Capture** — Create `Documentation/backlog/_intake-YYYY-MM-DD-<slug>.md` using the intake template.
2. **Triage** — Assign BL/HX ID, determine dependencies, set priority, assign to track.
3. **Promote** — Convert to full runbook (`bl-XXX-<slug>.md`), add row to this index.
4. **Archive** — Delete the intake doc after promotion.

## Definition of Ready

1. Objective, dependency gate, owner track, and exit artifact are explicit in the runbook.
2. Annex spec and runbook references are present and linked.
3. Agent mega-prompts (skill-aware + standalone) are defined for each implementation slice.
4. Validation commands and evidence destinations are defined.

## Definition of Done

1. Code/docs changes merged.
2. Required validation commands pass with recorded artifacts.
3. `status.json`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, and this index are synchronized.
4. `./scripts/validate-docs-freshness.sh` passes.

## Sync Contract (ADR-0005 Extended)

Any status change must update in the same changeset:
1. The runbook's Status Ledger
2. This index's dashboard table
3. `status.json`
4. `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md`
5. `README.md` and `CHANGELOG.md` (for Done transitions)

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
| BL-029 | `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md`; `Documentation/plans/bl-029-audition-platform-expansion-plan-2026-02-24.md` |
| BL-031 | `Documentation/plans/bl-031-tempo-locked-visual-token-scheduler-spec-2026-02-24.md` |

## Closed Archive

| ID | Title | Completed | Runbook |
|---|---|---|---|
| BL-001 | README standards and structure | 2026-02-21 | [bl-001](bl-001-readme-standards.md) |
| BL-002 | Physics preset host reversion fix | 2026-02-21 | [bl-002](bl-002-physics-preset-reversion.md) |
| BL-003 | Timeline transport controls restore | 2026-02-21 | [bl-003](bl-003-timeline-transport.md) |
| BL-004 | Keyframe editor gestures in production UI | 2026-02-21 | [bl-004](bl-004-keyframe-gestures.md) |
| BL-005 | Preset save host path fix | 2026-02-21 | [bl-005](bl-005-preset-save-path.md) |
| BL-006 | Motion trail overlays | 2026-02-21 | [bl-006](bl-006-motion-trail-overlays.md) |
| BL-007 | Velocity vector overlays | 2026-02-21 | [bl-007](bl-007-velocity-vector-overlays.md) |
| BL-008 | Audio-reactive RMS overlays | 2026-02-21 | [bl-008](bl-008-rms-overlays.md) |
| BL-009 | Steam headphone contract closeout | 2026-02-23 | [bl-009](bl-009-steam-headphone-contract.md) |
| BL-010 | FDN expansion promotion | 2026-02-23 | [bl-010](bl-010-fdn-expansion.md) |
| BL-011 | CLAP lifecycle and CI/host closeout | 2026-02-23 | [bl-011](bl-011-clap-lifecycle.md) |
| BL-012 | QA harness tranche closeout | 2026-02-24 | [bl-012](bl-012-qa-harness-tranche.md) |
| BL-014 | Listener/speaker/aim/RMS overlay strict closeout | 2026-02-24 | [bl-014](bl-014-overlay-strict-closeout.md) |
| BL-015 | All-emitter realtime rendering closure | 2026-02-23 | [bl-015](bl-015-all-emitter-rendering.md) |
| BL-016 | Visualization transport contract closure | 2026-02-23 | [bl-016](bl-016-transport-contract.md) |
| BL-018 | Spatial format matrix strict closeout | 2026-02-24 | [bl-018](bl-018-spatial-format-matrix.md) |
| BL-019 | Physics interaction lens closure | 2026-02-23 | [bl-019](bl-019-physics-interaction-lens.md) |
| BL-022 | Choreography lane closeout | 2026-02-24 | [bl-022](bl-022-choreography-closeout.md) |
| BL-024 | REAPER host automation baseline | 2026-02-23 | [bl-024](bl-024-reaper-host-automation.md) |
| BL-025 | EMITTER UI/UX v2 deterministic closeout | 2026-02-24 | [bl-025](bl-025-emitter-uiux-v2.md) |
| HX-01 | shared_ptr atomic migration guard | 2026-02-23 | [hx-01](hx-01-shared-ptr-atomic.md) |
| HX-03 | REAPER multi-instance stability lane | 2026-02-23 | [hx-03](hx-03-reaper-multi-instance.md) |
| HX-04 | Scenario coverage audit and drift guard | 2026-02-23 | [hx-04](hx-04-scenario-coverage.md) |
