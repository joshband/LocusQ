Title: CALIBRATE Autopilot Concept Brief
Document Type: Concept Brief
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE Autopilot Concept Brief

## Status

Concept brief.
Validation status: `not tested`.

## Hook

`CALIBRATE Autopilot` is the version of CALIBRATE that can meet a user where they are:
- one pair of headphones
- a laptop plus interface
- a stereo pair and measurement mic
- a wide speaker room
- a mixed device stack with saved profiles and partial detection

The goal is not “automate everything blindly.”

The goal is:
- detect as much as we honestly can
- tell the user what we know, how we know it, and what is still unknown
- guide calibration and personalization in the fewest manual steps possible
- make it easy to add new device and layout definitions without rewriting the whole panel

## Product Character

`CALIBRATE` should feel like an intelligent operator tool, not a bag of hidden heuristics.

It should behave like:
1. a discovery console
2. a routing and topology assistant
3. a speaker measurement workstation
4. a headphone personalization workflow
5. a profile and analysis library

That means the panel should stop acting like one monolithic calibration wizard and instead become a guided orchestration surface with explicit truth contracts.

## Core Promise

Users should be able to:
1. attach any speaker or headphone configuration
2. let LocusQ discover what it can
3. confirm or correct only the parts that remain ambiguous
4. run the right calibration or personalization recipe
5. save a profile with provenance
6. compare runs, diagnose issues, and improve results over time

## Experience Pillars

### 1. Honest Automation

Every auto-filled field must answer:
- what was detected
- what was inferred
- what remains unknown
- what the user changed manually
- whether the information may be stale

### 2. Any Configuration Growth Path

The system must be extensible enough for:
- stereo
- quad
- surround and immersive speaker layouts
- binaural and virtual binaural monitoring
- generic headphones
- named companion-detected devices
- imported and custom SOFA/HRTF paths
- future user-authored or vendor-authored definitions

### 3. Unified But Not Conflated

Speaker calibration and headphone personalization should live in one home, but not share one misleading flow.

The operator should always see which domain they are in:
- `Speaker Room`
- `Headphones`
- later: `Custom Layout` or `Verification Lab`

### 4. Analysis That Helps

Calibration should not end with “pass/fail.”

It should surface:
- repeated-pass consistency
- left/right asymmetry
- duplicate routing or missing channels
- SNR and clipping issues
- polarity and phase problems
- profile fallback or generic compensation
- recommended next actions

### 5. Profile-Centric Growth

Profiles should become reusable assets rather than opaque side effects.

The system should support:
- save
- load
- compare
- supersede
- annotate
- export/import
- preserve provenance and issue history

## Interaction Model

### Surface 1: Discovery

Purpose:
- detect outputs, inputs, companion devices, current topology candidates, and active profile assets

Key ideas:
- `Auto-map Outputs`
- `Auto-select Input`
- `Detected Devices`
- `Topology Candidates`
- `Manual Corrections`

### Surface 2: Target Flow

Purpose:
- route the user into the right guided flow

Paths:
- `Speaker Room`
- `Headphones`
- `Verification`
- future: `Custom Layout`

### Surface 3: Recipe

Purpose:
- apply the correct calibration or personalization strategy for the chosen target

Examples:
- speaker sweep recipe
- repeated-pass speaker consistency recipe
- headphone profile verification recipe
- binaural fallback verification recipe
- imported SOFA verification recipe

### Surface 4: Analysis

Purpose:
- explain result quality and next actions

Outputs:
- issue labels
- confidence summaries
- run-to-run comparison
- suggested fixes

### Surface 5: Library

Purpose:
- manage saved speaker, headphone, and mixed profiles

Capabilities:
- search
- filter by target/device/layout
- show provenance
- promote active profile
- compare revisions

## Architecture Concept

`CALIBRATE Autopilot` should be built on five reusable layers.

### Layer A: Discovery Graph

Nodes:
- outputs
- inputs
- device endpoints
- companion device candidates
- topology candidates
- active profiles
- unresolved ambiguities

Edges:
- derived from
- confirmed by
- conflicts with
- overridden by
- expires at

### Layer B: Registries

Required registries:
- topology registry
- device profile registry
- calibration recipe registry
- verification recipe registry
- issue taxonomy registry

The registry approach is the key to “support any configuration” without hardcoding every case in the panel.

### Layer C: Runtime Sessions

Distinct session types:
- speaker calibration session
- headphone personalization session
- verification session
- analysis/review session

### Layer D: Profile Stack

Separate but linkable assets:
- `DeviceProfile`
- `RoomProfile`
- `PersonalizationProfile`
- `VerificationProfile`
- `AnalysisReport`

### Layer E: Truth Contract

Every surface continues to honor BL-099 and BL-101 semantics:
- source
- provenance
- age/freshness
- manual override
- generic vs measured vs estimated

## Headphone Personalization Expansion

This is the largest product opportunity.

The current flow is mostly:
- detect device
- show profile state
- verify runtime activation

The expanded flow should become:
1. identify device and profile candidate
2. identify personalization source
3. resolve compensation chain
4. resolve HRTF source
5. resolve tracking readiness
6. run verification recipe
7. analyze issues
8. save or revise profile

Later growth:
- left/right asymmetry support
- multiple personal profiles per headphone
- custom import with provenance
- perceptual check bundles
- multi-pass verification history

## Speaker Automation Expansion

Speaker calibration should become more automatic without pretending certainty.

Better automation targets:
- host-derived output map
- input/mic suggestion
- topology candidate ranking
- duplicate/overlap detection
- channel naming and visual layout
- repeated-pass diagnostics
- saved-room comparison

## Visual Direction

The long-term UI should feel like:
- a mission-control panel for discovery and calibration
- not a basic form
- not a hidden expert-only matrix

High-level visual guidance:
- topology and device state should be visibly represented
- ambiguous detection should be visually different from confirmed state
- profiles should look like assets with identity and history
- issue analysis should feel actionable, not punitive

## Success Definition

This concept is successful if future CALIBRATE work can say:
- “we can add a new layout without inventing new UI semantics”
- “we can add a new headphone source without weakening truth language”
- “the user can calibrate more systems with less manual setup”
- “profiles are reusable, comparable, and diagnosable”

## Recommended Follow-On

Use this concept brief as the product and architecture north star for:
- discovery graph design
- topology/device registry design
- CALIBRATE automation wave planning
- richer headphone personalization and analysis work
