Title: LocusQ CALIBRATE Review and Redesign Spec
Document Type: Design Review Report
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# LocusQ CALIBRATE Review and Redesign Spec

## Purpose

Review the current `CALIBRATE` surface.
Answer the product questions directly.
Turn the findings into a concrete redesign direction.

Validation status: `not tested`

This is a design and code review artifact.
It does not claim runtime or build validation.

## Review Status Legend

- `[KEEP]` keep and protect
- `[TIGHTEN]` keep, but clarify or simplify
- `[NEXT]` highest-value redesign move
- `[LATER]` useful follow-on, not first wave

## Executive Call

`CALIBRATE` has a strong foundation.
It already knows about topology, monitoring path, device profile, routing safety, profile activation, and headphone status.

The main problem is not missing ambition.
It is mismatch between what the panel appears to support and what it fully supports today.

The corrective move is:
- split speaker calibration from headphone personalization,
- make automation visible and honest,
- demote compatibility controls,
- and stop presenting partial wide-layout support like complete support.

## Direct Answers

### Can LocusQ calibrate headphones, not just speakers?

Yes, but not in the same way.

Current truth:
- speaker calibration uses output mapping plus mic-driven measurement,
- headphone calibration uses `CalibrationProfile.json` handoff, headphone EQ/FIR/HRTF fields, and profile-activation validation,
- headphone device detection lives in the companion,
- headphone listening verification exists, but the panel is still speaker-first.

Implication:
- LocusQ supports headphone personalization and headphone compensation,
- but the `CALIBRATE` UI does not yet present that as a first-class guided workflow.

### Does Output Mapping include all speaker/headphone/listening arrangements?

No.

Current truth:
- the panel lists many topologies,
- but routing is hard-limited to `4` routable rows and `8` assignable outputs,
- wider layouts become read-only or limited-mapping cases.

Implication:
- stereo and quad are the most truthful current fits,
- 5.1, 7.1, 7.1.2, 7.4.2, and ambisonic are not fully expressed as operator-complete calibration flows in this panel,
- headphone listening arrangements are represented indirectly through `Monitoring Path` and `Device Profile`, not through a dedicated listening-mode surface.

### Should Profile Setup be more automated?

Yes.

Current truth:
- topology and routing can be auto-derived from host output width,
- headphone device profile can be auto-seeded by the companion,
- user profile matching can be auto-derived from companion capture,
- mic/input detection is still manual in the panel.

Best next move:
- automate detection,
- show what was detected,
- explain confidence and source,
- and keep manual override easy.

### What is Legacy Config for?

It is mainly a compatibility alias.

Current truth:
- `Legacy Config` maps old `4x Mono` / `2x Stereo` behavior onto the newer topology model,
- and can still drive topology changes.

Product reading:
- it is no longer a primary operator concept,
- it is a migration and compatibility concept.

### What is Redetect Routing for?

It re-bootstrap maps from host output layout.

Current truth:
- it does not identify speaker labels in the room,
- it does not identify the active headphone model,
- it does not identify the preferred mic/input,
- it does not discover all possible listening arrangements.

Better label:
- `Auto-map Outputs`
- or `Detect Host Output Layout`

## Current-State Review

## What Works

- `[KEEP]` profile-first framing
  Reason: topology, monitoring path, and device profile already exist as explicit inputs.
- `[KEEP]` validation block
  Reason: map, phase, delay, profile activation, and downmix checks create a real trust surface.
- `[KEEP]` redetect overwrite guard
  Reason: custom routing is protected from silent replacement.
- `[KEEP]` headphone device status card
  Reason: it proves the system already has headphone-specific truth.

## Pressure Points

| Area | Status | Why it matters |
|---|---|---|
| Speaker vs headphone hierarchy | `[NEXT]` | two different jobs are mixed into one speaker-shaped flow |
| Wide topology truthfulness | `[NEXT]` | panel implies more complete support than it actually exposes |
| Legacy Config prominence | `[TIGHTEN]` | compatibility detail is stealing main-surface attention |
| Automation visibility | `[NEXT]` | helpful auto-behavior exists, but users cannot read it clearly |
| Input discovery | `[NEXT]` | output mapping is surfaced; input choice remains a raw manual field |
| Progress copy | `[TIGHTEN]` | fixed `SPK1..SPK4` language is wrong for headphone-oriented runs |

## Information-Architecture Problems

### 1. One panel is carrying two jobs

Job A:
- measure and align speakers in a room.

Job B:
- personalize and verify headphone playback.

Those jobs share some metadata.
They should not share the same primary mental model.

### 2. Output Mapping is overloaded

It currently mixes:
- active topology,
- compatibility mode,
- host auto-detection,
- manual routing,
- and partial support warnings.

That is too much responsibility for one card.

### 3. Automation is implicit instead of legible

The product already auto-does several things:
- host-width routing bootstrap,
- topology inference from output width,
- headphone profile seeding from companion detection,
- profile handoff from companion to plugin.

But the panel does not say:
- what was detected,
- where it came from,
- when it last changed,
- or what is still manual.

## Redesign Direction

## Core Principle

Keep one `CALIBRATE` mode.
Inside it, split the work into two explicit tracks:

1. `Speaker Setup`
2. `Headphone Personalization`

Do not hide one inside the other.

## Proposed Card Structure

### Card 1: Setup Mode

Replace the current top card with:
- `Calibration Target`
- `Speaker Room`
- `Headphones`

Behavior:
- this selection changes the card stack below,
- not the global plugin mode.

### Card 2A: Speaker Setup

Show only when target is `Speaker Room`.

Fields:
- Topology
- Detected Host Output Layout
- Writable Outputs
- Mic/Input
- Auto-map Outputs
- Manual Routing

Rules:
- `Legacy Config` hidden by default
- `Legacy Config` available only in `Advanced`
- explicit note when topology exceeds current writable routing width

### Card 2B: Headphone Personalization

Show only when target is `Headphones`.

Fields:
- Detected Device
- Monitoring Path
- Headphone Profile
- User HRTF Profile
- Head Tracking
- Verification State

Actions:
- `Refresh Device`
- `Load Profile`
- `Open Companion`
- `Run Listening Check`

Rules:
- no speaker mapping rows shown here,
- no `SPK1..SPK4` progress copy here,
- profile activation becomes the main trust block.

### Card 3: Automation Summary

New compact summary card.

Show:
- `Outputs: detected from host`
- `Headphones: detected from companion`
- `Input: manual`
- `Profile: loaded from CalibrationProfile.json`

Each row should show:
- source,
- status,
- last change,
- override state.

### Card 4: Run and Validation

Shared shell.
Different copy by target.

Speaker target:
- map
- polarity/phase
- delay
- profile activation when headphone monitor path is involved

Headphone target:
- requested vs active profile
- EQ/FIR active state
- HRTF active state
- tracking active state
- listening verification scores

### Card 5: Library

Keep this.
But rename to:
- `Calibration Profiles`

Add tuple badges:
- `target`
- `topology`
- `monitoring`
- `device`

## Rename Recommendations

| Current Label | Proposed Label | Why |
|---|---|---|
| `Profile Setup` | `Calibration Target` + context card | current label is too broad |
| `Output Mapping` | `Speaker Routing` | more truthful and narrower |
| `Legacy Config` | `Legacy Routing Alias` | compatibility term, not operator term |
| `REDETECT` | `AUTO-MAP OUTPUTS` | describes actual behavior |
| `Device Profile` | `Headphone Profile` when in headphone flow | more explicit |
| `Run and Validation` | `Measure and Verify` | more operator-readable |

## Automation Contract

## What should be automatic now

| Surface | Recommendation | Why |
|---|---|---|
| output topology seed | auto-detect | host already exposes enough width/layout info |
| output map seed | auto-detect | existing redetect path already does this |
| headphone device seed | auto-detect | companion already does this |
| headphone profile seed | auto-apply from companion profile | existing handoff already supports it |
| user HRTF profile | auto-suggest | companion matching pipeline already exists |

## What should stay manual now

| Surface | Recommendation | Why |
|---|---|---|
| final speaker channel map | manual override always available | host ordering may not match room reality |
| mic/input channel | manual | current plugin panel does not have reliable input-device identity |
| topology override | manual | operator intent matters more than host guess |
| profile confirmation | manual confirm after auto-suggest | avoid silent wrong personalization |

## What should be automated later

| Surface | Recommendation | Why |
|---|---|---|
| standalone input-device identity | `[LATER]` | needs stronger host/runtime plumbing |
| speaker semantic labeling | `[LATER]` | requires more than width detection |
| endpoint-type switching | `[LATER]` | useful, but must avoid surprise state changes |

## Better User Flows

### Speaker Room Flow

1. Choose `Speaker Room`.
2. Read detected output layout.
3. Press `Auto-map Outputs` if needed.
4. Confirm or edit routing.
5. Choose input/mic.
6. Run measurement.
7. Review map, phase, and delay.
8. Save profile.

### Headphone Flow

1. Choose `Headphones`.
2. Read detected headphone device.
3. Confirm monitoring path.
4. Load or refresh `CalibrationProfile.json`.
5. Review active EQ/HRTF/tracking state.
6. Run listening verification.
7. Save or update profile.

## Product Truthfulness Rules

### Rule 1

Do not present a topology as fully calibratable if the current surface cannot route and validate it end-to-end.

### Rule 2

Do not use speaker-shaped copy in headphone-only verification flows.

### Rule 3

Every auto-detected value must show:
- source,
- confidence or scope,
- and manual override state.

### Rule 4

Compatibility controls must live behind `Advanced`.

## First-Wave Implementation Priorities

1. `[NEXT]` Split `CALIBRATE` into `Speaker Room` and `Headphones` target flows.
2. `[NEXT]` Rename `REDETECT` to `AUTO-MAP OUTPUTS` and add explicit source text.
3. `[NEXT]` Move `Legacy Config` behind `Advanced`.
4. `[NEXT]` Add an automation summary row for outputs, headphone detection, and profile source.
5. `[NEXT]` Rewrite validation/progress copy so headphone flows are not described as `SPK1..SPK4`.
6. `[NEXT]` Mark wide-topology support honestly as `limited`, `preview`, or `not fully routable here`.
7. `[LATER]` Add standalone input-device auto-identification if the runtime can expose it safely.

## Suggested Backlog Slices

| Candidate | Focus |
|---|---|
| `CAL-A1` | split speaker vs headphone target flow in `CALIBRATE` |
| `CAL-A2` | automation summary and source-of-truth labels |
| `CAL-A3` | rename and demote legacy routing controls |
| `CAL-A4` | headphone-first validation copy and listening-check card |
| `CAL-A5` | wider-topology truthfulness pass and guardrail language |
| `CAL-A6` | standalone input detection feasibility study |

## Visual Aid Index

| Artifact | Role |
|---|---|
| `Source/ui/public/index.html` | current `CALIBRATE` card hierarchy |
| `Source/ui/src/index.ts` | current topology, mapping, redetect, and validation behavior |
| `Documentation/plans/2026-02-27-calibration-system-design.md` | canonical calibration architecture |
| `Documentation/plans/calibration-profile-schema-v1.md` | headphone/user profile handoff contract |
| `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md` | active BL-026 calibration UI contract |

## Closeout

The current `CALIBRATE` panel is directionally right.
It already contains the right raw concepts.

The next improvement is structural, not cosmetic.
Make speaker calibration and headphone personalization feel like two clear operator jobs inside one trusted mode.
