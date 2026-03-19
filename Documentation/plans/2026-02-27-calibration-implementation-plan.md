Title: LocusQ Calibration System Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# LocusQ Calibration System Implementation Plan

## Purpose

Keep one active implementation plan for the calibration program.
This file is the short execution surface for BL-054 through BL-061.

Design authority:
- `Documentation/plans/2026-02-27-calibration-system-design.md`

Schema authority:
- `Documentation/plans/calibration-profile-schema-v1.md`

Legacy deep task script:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-02-27-calibration-implementation-plan-legacy.md`

Validation status: `not tested`

This is an implementation plan.
It does not claim current runtime validation.

## Goal

Turn the old calibration stub into a measurable calibration system with:
- model-based headphone EQ,
- FIR-based monitoring expansion,
- companion profile handoff,
- device preset support,
- and a listening-test loop that can prove improvement.

## Scope

### In Scope

- PEQ cascade and preset-driven compensation
- FIR engine and latency-safe switching
- `CalibrationProfile.json` handoff
- AirPods Pro 1/2/3 and WH-1000XM5 device-profile flow
- companion profile acquisition and matching
- Phase B listening harness and analysis

### Out Of Scope

- productizing every prototype detail in `Documentation/Calibration POC/`
- making Phase B evidence itself the implementation authority
- replacing the approved architecture doc

## Architecture Summary

Two tracks remain:

| Track | Role | Primary Outputs |
|---|---|---|
| Plugin DSP track | turn monitoring compensation into a real DSP chain | PEQ, FIR, latency reporting, state migration |
| Companion/profile track | build the user-profile and device-profile handoff path | profile schema, device detection, matching, handoff, listening harness |

Integration point:
- `CalibrationProfile.json` links companion-side profile selection to plugin-side monitoring behavior.

## Current Program Map

| BL | Focus | Active Reference |
|---|---|---|
| BL-054 | PEQ cascade RT integration | this plan + system design |
| BL-055 | FIR convolution engine | this plan + system design |
| BL-056 | calibration state migration and latency contract | this plan + schema |
| BL-057 | device preset library | this plan + system design |
| BL-058 | companion profile acquisition | this plan + schema |
| BL-059 | calibration profile integration handoff | this plan + schema |
| BL-060 | Phase B listening harness | this plan + methodology + POC tools |
| BL-061 | interpolation and crossfade follow-on | this plan + BL-060 gate |

## Implementation Order

### Phase A: Device and PEQ Foundation

1. Extend device-profile coverage.
   Output:
   - AirPods Pro 1/2/3 support
   - WH-1000XM5 preset path
2. Replace the simple compensation stub with a preset-driven PEQ cascade.
   Output:
   - bounded band count
   - RT-safe coefficient publication
   - deterministic profile switching

Primary backlog lanes:
- BL-054
- BL-057

### Phase B: FIR And Monitoring Path

1. Land the FIR engine contract.
2. Separate direct-path versus partitioned-path behavior honestly.
3. Report latency correctly.
4. keep state migration and fallback behavior deterministic.

Primary backlog lanes:
- BL-055
- BL-056

### Phase C: Companion Profile Handoff

1. finalize `CalibrationProfile.json`
2. detect supported device mode
3. match user profile
4. write the handoff contract
5. surface plugin-side read and application behavior

Primary backlog lanes:
- BL-058
- BL-059

### Phase D: Listening Validation

1. render or prepare evaluation stimuli
2. run the listening harness
3. analyze improvement
4. write verification fields back into the calibration profile or evidence surfaces as required

Primary backlog lanes:
- BL-060
- BL-061

## Deliverables

| Deliverable | Owner Surface | Notes |
|---|---|---|
| approved architecture | `Documentation/plans/2026-02-27-calibration-system-design.md` | canonical design authority |
| profile schema | `Documentation/plans/calibration-profile-schema-v1.md` | schema authority |
| device preset path | code + BL-057 evidence | avoid duplicating preset tables in docs |
| PEQ implementation | code + BL-054 evidence | active plan should describe contract, not inline full code |
| FIR implementation | code + BL-055 evidence | keep truthfulness aligned with actual implementation |
| handoff contract | BL-058/059 docs + schema | `CalibrationProfile.json` is the integration boundary |
| listening harness | BL-060 docs + `Documentation/Calibration POC/locusq_spatial_prototype/tools/` | POC tools stay reference-only |

## Key Decisions

### 1. Two-Profile Model Stays Separate

Keep:
- user profile / HRTF choice
- headphone profile / model EQ

Do not collapse them into one blob.

### 2. `CalibrationProfile.json` Is The Handoff Contract

The companion owns profile creation.
The plugin owns runtime application.

### 3. Listening Validation Is A Gate, Not Decoration

BL-060 is not a side quest.
It is the proof lane for whether the calibration system is improving real outcomes.

### 4. POC Material Stays Supporting

`Documentation/Calibration POC/` may support implementation and listening work.
It is not status authority.
Promote truth into backlog docs, ADRs, code, or evidence instead of expanding the POC surface.

## Risks

| Risk | Why it matters | Mitigation |
|---|---|---|
| DSP truth drift | docs can overstate FIR or latency behavior | keep BL-055 evidence objective and current |
| schema drift | companion and plugin can diverge | keep schema authority in one file and update both sides together |
| profile ambiguity | user/HRTF and headphone/EQ concerns can blur | keep the two-profile model explicit |
| POC leakage | prototype notes can start acting like live product docs | keep active plan short and point to archive or evidence for detail |
| listening-gate drift | implementation can look "done" before perceptual proof exists | keep BL-060 gate visible in backlog and evidence surfaces |

## Evidence And Validation Pointers

Use these as the decision surfaces:
- `Documentation/backlog/index.md`
- relevant BL runbooks under `Documentation/backlog/`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

Use these as supporting inputs:
- `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`
- `Documentation/Calibration POC/locusq_spatial_prototype/`

## Visual Aid Index

| Visual/Artifact | Role |
|---|---|
| `Documentation/plans/2026-02-27-calibration-system-design.md` diagrams | architecture reference |
| `Documentation/Calibration POC/locusq_spatial_prototype/tools/render.py` | offline reference renderer |
| `Documentation/Calibration POC/locusq_spatial_prototype/tools/listening_test.py` | listening harness reference |
| `Documentation/Calibration POC/locusq_spatial_prototype/tools/analyze_results.py` | analysis reference |

## Active Checklist

- [ ] BL-054 truth remains aligned with the actual PEQ implementation.
- [ ] BL-055 truth remains aligned with the actual FIR implementation.
- [ ] BL-056 schema and latency reporting stay synchronized.
- [ ] BL-057 device preset scope stays explicit and bounded.
- [ ] BL-058 and BL-059 keep companion and plugin contracts aligned.
- [ ] BL-060 remains the visible perceptual validation gate.
- [ ] BL-061 stays conditional on BL-060 outcome.

## Archive Note

The original 2026-02-27 worker-style plan was preserved for traceability at:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-02-27-calibration-implementation-plan-legacy.md`

Use the archive copy only when you need the historical step-by-step worker breakdown.
Use this file for current planning.
