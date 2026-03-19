Title: CALIBRATE Redesign Execution Packet
Document Type: Planning Packet
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE Redesign Execution Packet

## Purpose

Lock the first implementation wave for the `CALIBRATE` redesign.
Convert the 2026-03-18 review findings into a buildable plan.

Primary goals:
- separate speaker calibration from headphone personalization,
- make automation legible,
- make layout support more truthful,
- and reduce legacy control clutter.

Primary inputs:
- `Documentation/reports/2026-03-18-calibrate-review-and-redesign-spec.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/2026-02-27-calibration-system-design.md`
- `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md`

Validation status: `not tested`

This is a planning packet.
It does not claim runtime or build validation.

## Scope Baseline

- `CALIBRATE` currently mixes speaker topology, headphone monitoring, device profile, and mic/input into one top card.
- Output routing is still limited to four routable rows.
- Wide layouts are listed in the UI but not fully operable end to end from the current panel.
- Headphone profile logic is real in the companion and plugin, but the panel still reads like a speaker-first workflow.
- `Legacy Config` still appears as a primary control even though it mostly serves compatibility.

## Review Status Legend

- `[NEXT]` first-wave item
- `[LATER]` important, but not wave one
- `[DEFERRED]` explicitly out of this packet

## Execution Principles

1. Keep one `CALIBRATE` mode.
2. Split operator jobs inside the mode.
3. Make automation visible and attributable.
4. Do not claim full wide-layout calibration if the UI cannot execute it honestly.
5. Preserve current backend/profile contracts unless a slice explicitly changes them.

## Entry Conditions

| Item | Can Start Now? | Notes |
|---|---|---|
| UI hierarchy split | Yes | front-end only if labels and visibility are staged carefully |
| automation summary | Yes | uses existing data already exposed by host and companion |
| legacy-control demotion | Yes | should remain compatibility-safe |
| requested/active trust pass | Yes | depends on existing validation payloads |
| wide-layout capability expansion | No | treat as separate follow-on after truthfulness pass |
| standalone input-device auto-identification | No | needs feasibility work before UX promises |

## Ownership and Write Sets

| Pod | Focus | Primary Files | Ownership Rule |
|---|---|---|---|
| CAL Shell Pod | hierarchy, cards, labels, status sequencing | `Source/ui/public/index.html`, `Source/ui/src/index.ts` | own card order, section labels, visibility logic, and trust-summary placement |
| Calibration Bridge Pod | explicit source-of-truth payloads for automation and status | `Source/processor_bridge/ProcessorUiBridgeOps.h`, `Source/processor_core/ProcessorCalibrationBridge.cpp` | add or clarify status payload fields without changing DSP graph shape |
| Companion Truth Pod | headphone/device/profile provenance and readiness wording | `companion/Sources/LocusQHeadTrackingCompanion/main.swift`, `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift` | expose clearer source/state distinctions; do not widen privacy scope |
| Docs/Governance Pod | update design authority and closeout wording | `Documentation/plans/*.md`, `Documentation/reports/*.md`, `status.json` | keep planning/review artifacts synchronized and concise |

Shared-surface rule:
- UI hierarchy work can start before payload enrichment.
- Payload enrichment should land before final copy polish.
- Capability expansion for layouts wider than quad is not part of this packet.

## Wave Plan

### Wave 1A: Split The Operator Jobs

Objective:
Make `CALIBRATE` answer one clear question at a time.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-A1` | Add explicit `Calibration Target` choice: `Speaker Room` vs `Headphones` | `index.html`, `index.ts` | top of panel selects target flow without adding a new global plugin mode |
| `CAL-A2` | Re-sequence cards by target flow | `index.html`, `index.ts` | speaker flow shows topology/routing/mic first; headphone flow shows device/profile/HRTF/tracking first |
| `CAL-A3` | Remove speaker-shaped run copy from headphone flow | `index.ts` | no `SPK1..SPK4` progress/status copy when headphone flow is active |

Why first:
- this resolves the biggest product mismatch without requiring new DSP behavior.

### Wave 1B: Make Automation Legible

Objective:
Show where key values came from and what is still manual.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-B1` | Add automation summary card | `index.html`, `index.ts` | panel shows output source, headphone source, profile source, and input state |
| `CAL-B2` | Enrich payload/source-of-truth semantics | `ProcessorUiBridgeOps.h`, `ProcessorCalibrationBridge.cpp`, `main.swift` if needed | UI can distinguish host-derived, companion-derived, persisted, and manual values |
| `CAL-B3` | Clarify requested vs active wording and stale/override states | `index.ts`, optional companion wording | users can read what was requested, what is active, and why they differ |

Why second:
- the underlying system already has most of this information.
- the main gap is trust messaging.

### Wave 1C: Demote Compatibility Controls

Objective:
Reduce confusion from legacy controls and narrow labels to what the UI really does.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-C1` | Rename `REDETECT` to `AUTO-MAP OUTPUTS` or equivalent | `index.html`, `index.ts` | button label and status copy match actual host-output bootstrap behavior |
| `CAL-C2` | Move `Legacy Config` behind `Advanced` | `index.html`, `index.ts` | legacy alias is not a default-visible primary control |
| `CAL-C3` | Keep overwrite and limited-mapping acknowledgements explicit | `index.ts` | current routing safety contracts remain intact after hierarchy cleanup |

Why third:
- this is high-value UX cleanup with low backend risk.

### Wave 1D: Truthfulness Pass For Layout Support

Objective:
Make capability claims honest before any wider-layout implementation work.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-D1` | Mark wide layouts as `limited` where appropriate | `index.ts`, `index.html` | 5.1/7.1/7.4.2/ambisonic flows are not presented as fully calibratable if only partially routable |
| `CAL-D2` | Align monitoring-path copy with actual matrix behavior | `index.ts`, docs | binaural/headphone choices do not imply unsupported multichannel legality |
| `CAL-D3` | Update planning/report docs to match live truth | `Documentation/plans/*.md`, `Documentation/reports/*.md` | docs stop overstating current wide-layout calibration capability |

Why fourth:
- operator trust improves immediately.
- deeper layout support can come later on a truthful baseline.

## Recommended Sequencing

1. Start `CAL-A1` and `CAL-A2`.
2. Land `CAL-B1` in parallel if the UI can consume current payloads cleanly.
3. Add `CAL-B2` once the new summary card is ready to show source distinctions.
4. Land `CAL-C1` and `CAL-C2` after the new hierarchy settles.
5. Close with `CAL-D1..D3` as the truthfulness and docs pass.

## First-Wave Non-Goals

- No expansion beyond four routable calibration channels in this packet.
- No new standalone input-device discovery contract yet.
- No rewrite of the companion capture/matching pipeline.
- No new headphone personalization DSP beyond current profile-driven behavior.
- No change to privacy/retention rules for capture assets.

## Follow-On Work

| Candidate | Status | Why deferred |
|---|---|---|
| wider-than-quad calibration routing expansion | `[LATER]` | needs real backend and QA lane expansion, not just UI work |
| richer headtracking readiness states in `CALIBRATE` | `[LATER]` | useful, but secondary to target-flow split |
| standalone input-device auto-identification | `[LATER]` | needs host/runtime feasibility proof |
| parity/lab surfacing for HRTF validation evidence | `[LATER]` | operator UI should not overfit lab detail in wave one |

## Promotion Gates

| Gate | Requirement |
|---|---|
| `G1` | `CALIBRATE` clearly separates `Speaker Room` and `Headphones` flows |
| `G2` | automation summary exposes source-of-truth for outputs, headphones, and profile handoff |
| `G3` | `Legacy Config` is no longer default-visible as a peer to topology and routing |
| `G4` | button/copy language matches actual host-output bootstrap and requested-vs-active behavior |
| `G5` | wide-layout and binaural wording is more truthful than the current surface |

## File Touch Forecast

### Likely Wave-One Code Files

- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/processor_core/ProcessorCalibrationBridge.cpp`

### Likely Wave-One Companion Files

- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`
- `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift`

### Likely Wave-One Docs

- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/reports/2026-03-18-calibrate-review-and-redesign-spec.md`
- `status.json`

## Validation Lanes

### Core UI/Runtime

- `cd Source/ui && npm run typecheck`
- `cd Source/ui && npm run build`
- `cmake --build build_local --config Release --target LocusQ_Standalone -j 8`

### Calibration/Rendering Confidence

- `scripts/standalone-ui-selftest-production-p0-mac.sh`
- `./scripts/qa-bl009-headphone-contract-mac.sh`

### Companion Confidence

- `cd companion && swift build`
- `cd companion && swift test`

## Success Definition

The first wave succeeds if:
- users can tell whether they are calibrating speakers or personalizing headphones,
- automation is visible and understandable,
- compatibility controls are demoted,
- and the UI stops implying more wide-layout calibration support than it can actually deliver today.

## Source Inputs

- `Documentation/reports/2026-03-18-calibrate-review-and-redesign-spec.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md`
- `Documentation/plans/2026-02-27-calibration-system-design.md`
- `Documentation/plans/calibration-profile-schema-v1.md`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/processor_core/ProcessorCalibrationBridge.cpp`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`

