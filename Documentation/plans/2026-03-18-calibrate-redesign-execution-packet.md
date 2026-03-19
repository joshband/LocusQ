Title: CALIBRATE Redesign Execution Packet
Document Type: Planning Packet
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE Redesign Execution Packet

## Status

Active.
This is the short execution contract for the first CALIBRATE redesign wave.

Validation status: `not tested`

Primary inputs:
- `Documentation/reports/2026-03-18-calibrate-review-and-redesign-spec.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/2026-02-27-calibration-system-design.md`
- `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md`

Legacy deep packet:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-03-18-calibrate-redesign-execution-packet-legacy.md`

## Goal

Make CALIBRATE easier to trust.

First-wave outcomes:
- separate speaker calibration from headphone personalization
- show where automation values came from
- demote legacy controls
- stop overstating wide-layout support

## Scope

Current product mismatch:
- one top card still mixes speaker topology, headphone personalization, device/profile state, and mic/input
- output routing is still limited to four routable rows
- wide layouts appear more complete than the live panel can honestly support
- headphone profile logic is real, but the panel still reads like a speaker-first workflow
- `Legacy Config` still looks more primary than it should

Out of scope:
- no routing expansion beyond four calibration channels
- no new standalone input-device discovery contract
- no companion capture pipeline rewrite
- no new personalization DSP
- no privacy or retention policy change

## Core Contracts

### Execution Principles

1. Keep one `CALIBRATE` mode.
2. Split operator jobs inside the mode.
3. Make automation visible and attributable.
4. Do not claim wide-layout calibration that the UI cannot execute honestly.
5. Preserve current backend and profile contracts unless a slice explicitly changes them.

### Entry Conditions

| Item | Start Now | Notes |
|---|---|---|
| UI hierarchy split | yes | front-end first |
| automation summary | yes | built on existing payloads |
| legacy-control demotion | yes | compatibility-safe cleanup |
| requested vs active trust pass | yes | wording and provenance work |
| wide-layout capability expansion | no | separate follow-on |
| standalone input auto-identification | no | needs feasibility proof |

### Ownership

| Pod | Focus | Primary Files |
|---|---|---|
| CAL Shell | hierarchy, cards, labels, visibility | `Source/ui/public/index.html`, `Source/ui/src/index.ts` |
| Calibration Bridge | source-of-truth payloads | `Source/processor_bridge/ProcessorUiBridgeOps.h`, `Source/processor_core/ProcessorCalibrationBridge.cpp` |
| Companion Truth | device/profile provenance wording | `companion/Sources/LocusQHeadTrackingCompanion/main.swift`, `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift` |
| Docs/Governance | plan/report/status sync | `Documentation/plans/*.md`, `Documentation/reports/*.md`, `status.json` |

Shared-surface rule:
- UI hierarchy can land before payload enrichment.
- payload enrichment should land before final copy polish
- wider-than-quad capability work stays out of this packet

## Delivery Order

### Wave A: Split The Operator Jobs

Goal:
- make the panel answer one job at a time

Required slices:
- `CAL-A1`: add `Calibration Target` with `Speaker Room` vs `Headphones`
- `CAL-A2`: re-sequence cards by target flow
- `CAL-A3`: remove speaker-shaped progress copy from the headphone path

Exit:
- users can tell which flow they are in without reading implementation detail

### Wave B: Make Automation Legible

Goal:
- show where values came from and what is still manual

Required slices:
- `CAL-B1`: add automation summary card
- `CAL-B2`: enrich payload semantics for host, companion, persisted, and manual values
- `CAL-B3`: clarify requested vs active wording, including stale and override states

Exit:
- users can read output source, headphone source, profile source, and active-state differences quickly

### Wave C: Demote Compatibility Controls

Goal:
- reduce false primary actions

Required slices:
- `CAL-C1`: rename `REDETECT` to a truthful host-output bootstrap label
- `CAL-C2`: move `Legacy Config` behind `Advanced`
- `CAL-C3`: keep overwrite and limited-mapping safety wording explicit

Exit:
- legacy and compatibility controls stop competing with the main calibration path

### Wave D: Truthfulness Pass

Goal:
- fix capability claims before any larger layout expansion

Required slices:
- `CAL-D1`: mark wide layouts as limited where needed
- `CAL-D2`: align monitoring-path copy with real matrix behavior
- `CAL-D3`: update docs and reports to match the live truth

Exit:
- the panel and docs no longer imply full wide-layout calibration support

## Promotion Gates

| Gate | Requirement |
|---|---|
| `G1` | `CALIBRATE` clearly separates `Speaker Room` and `Headphones` |
| `G2` | automation summary exposes source-of-truth for outputs, headphones, and profile handoff |
| `G3` | `Legacy Config` is no longer default-visible as a peer control |
| `G4` | button and status copy match requested-vs-active behavior |
| `G5` | wide-layout and binaural wording is more truthful than the current surface |

## Validation Plan

Core UI/runtime:
- `cd Source/ui && npm run typecheck`
- `cd Source/ui && npm run build`
- `cmake --build build_local --config Release --target LocusQ_Standalone -j 8`

Calibration confidence:
- `scripts/standalone-ui-selftest-production-p0-mac.sh`
- `./scripts/qa-bl009-headphone-contract-mac.sh`

Companion confidence:
- `cd companion && swift build`
- `cd companion && swift test`

## Touch Forecast

Likely code files:
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/processor_core/ProcessorCalibrationBridge.cpp`

Likely companion files:
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`
- `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift`

Likely docs:
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/reports/2026-03-18-calibrate-review-and-redesign-spec.md`
- `status.json`

## Visual Aid Index

| Artifact | Role |
|---|---|
| redesign review | redesign rationale and UI findings |
| calibration system design | active design authority |
| BL-026 spec | CALIBRATE UI baseline |
| archive copy | original long execution packet |

## Archive Note

The long-form execution packet was preserved at:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-03-18-calibrate-redesign-execution-packet-legacy.md`

Use the archive copy for the original staging narrative.
Use this file as the active execution surface.
