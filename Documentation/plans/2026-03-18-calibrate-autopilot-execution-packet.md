Title: CALIBRATE Autopilot Execution Packet
Document Type: Planning Packet
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE Autopilot Execution Packet

## Purpose

Turn the CALIBRATE automation and personalization direction into an implementation-ready roadmap that follows BL-099 and BL-101 instead of bypassing them.

Primary goals:
- make output and input discovery more automatic
- make topology/device growth registry-driven
- expand headphone personalization into a first-class workflow
- make saved profiles analyzable and comparable
- keep automation honest about what is measured, estimated, generic, stale, or manually overridden

Primary inputs:
- `Documentation/reports/2026-03-18-calibrate-autopilot-concept-brief.md`
- `Documentation/reports/2026-03-18-calibrate-automation-and-headphone-personalization-research-packet.md`
- `Documentation/backlog/bl-101-calibrate-discovery-provenance-and-truthfulness.md`
- `Documentation/backlog/bl-099-headphone-verification-truthfulness-and-compensation-provenance.md`

Validation status: `not tested`

## Planning Summary

This is not one feature.

It is a layered expansion:
1. discovery graph foundation
2. registry-driven topology and device support
3. recipe-driven speaker and headphone workflows
4. saved profile and analysis model
5. richer automated QA and evidence

## Complexity Score

Score: `5/5`

Rationale:
- touches companion, native bridge, UI, persistence, and QA
- spans speakers, headphones, inputs, outputs, topology inference, and profile lifecycle
- requires new registries and session models
- has truthfulness constraints that must remain promotion-safe

## Implementation Strategy

Phased implementation.

This should not be attempted as one broad feature branch. Each wave should leave behind a usable and testable intermediate system.

## Architecture Pillars

### 1. Discovery Graph

Introduce an additive runtime model that tracks:
- discovered outputs
- discovered inputs
- endpoint classes
- topology candidates
- active device/profile assets
- ambiguity edges
- provenance/freshness/override state

Likely touch zones:
- `Source/processor_core/ProcessorCalibrationBridge.cpp`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/ui/src/index.ts`
- companion runtime if richer endpoint metadata is needed

### 2. Registries

Introduce stable registries for:
- topology definitions
- speaker label/position templates
- headphone device identities
- calibration recipe definitions
- verification recipe definitions
- issue categories and recommended actions

Likely first form:
- additive C++ tables plus UI metadata mirrors
- later unify into one registry contract if needed

### 3. Session Model

Make CALIBRATE workflows explicit:
- discovery session
- speaker calibration session
- headphone personalization session
- verification session
- review/analysis session

### 4. Profile Stack

Move toward a profile family rather than one overloaded artifact:
- room profile
- device profile
- personalization profile
- verification report
- analysis report

Near-term rule:
- preserve the current `CalibrationProfile.json` path
- layer richer metadata and references around it rather than breaking compatibility

## Wave Plan

### Wave A: Discovery Graph Foundation

Objective:
create one additive model for automatic output/input/device/topology knowledge

Slices:

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `AUTO-P1` | Define `DiscoveryGraph` runtime contract | docs + bridge payload shape | outputs/inputs/topology/device candidates share one additive structure |
| `AUTO-P2` | Add output-node discovery and host route candidate publication | calibration bridge + UI | `Auto-map Outputs` can explain candidate maps and ambiguity |
| `AUTO-P3` | Add input-node discovery and mic suggestion publication | standalone/native device layer + UI | `Auto-select Input` can suggest likely measurement inputs |
| `AUTO-P4` | Add unresolved-ambiguity surface | UI | users can see what still needs confirmation |

### Wave B: Topology and Device Registries

Objective:
stop encoding growth as one-off dropdown logic

Slices:

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `AUTO-P5` | Define topology registry contract | docs + C++/UI metadata | layout definitions become reusable and visualizable |
| `AUTO-P6` | Define device registry contract | docs + companion/plugin metadata | headphone/speaker endpoint definitions become extensible |
| `AUTO-P7` | Add custom layout descriptor support | docs + UI + persistence follow-on | future user-defined layouts have a stable shape |

### Wave C: Recipe-Driven Flows

Objective:
turn CALIBRATE into guided workflows instead of static cards

Slices:

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `AUTO-P8` | Define speaker recipe model | docs + UI + runtime hooks | stereo/quad/surround recipes can differ without ad-hoc branches |
| `AUTO-P9` | Define headphone personalization recipe model | docs + UI + companion/runtime hooks | acquisition, correction, verification, and save flow are explicit |
| `AUTO-P10` | Add recipe summary and next-action guidance | UI | the panel explains what it will do and what still needs confirmation |

### Wave D: Profile Library and Analysis

Objective:
make calibration assets reusable and diagnosable

Slices:

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `AUTO-P11` | Define profile family metadata | schema/docs + UI library | saved items can distinguish room/device/personalization/verification |
| `AUTO-P12` | Add analysis report model | docs + UI + persistence | repeated-pass and issue findings are saved with the run |
| `AUTO-P13` | Add compare/supersede workflow | UI library + persistence | users can compare or replace prior calibrations intentionally |

### Wave E: Multi-Pass Diagnostics

Objective:
turn calibration quality into actionable feedback

Slices:

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `AUTO-P14` | Add repeated-pass consistency checks | runtime + analysis + UI | speakers/headphones can show run-to-run stability |
| `AUTO-P15` | Add issue taxonomy and recommendation engine | docs + UI | known issues map to plain-language next actions |
| `AUTO-P16` | Add machine-readable analysis outputs | persistence + QA | reports can feed automated and manual review |

## Recommended First Build Order

Short version:
1. Discovery graph for outputs and inputs
2. topology/device registries
3. guided headphone personalization recipe
4. analysis report model

Why:
- it improves automation immediately
- it avoids hardcoding more layouts before the registry exists
- it gives headphone expansion a cleaner foundation

## Likely Write Set

- `Source/processor_core/ProcessorCalibrationBridge.cpp`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `Source/PluginProcessor.cpp`
- `Source/PluginProcessor.h`
- companion endpoint/runtime files as needed
- `Documentation/plans/calibration-profile-schema-v1.md`
- new testing and QA scripts under `scripts/`

## Risks

### High Risk

- discovery logic can overclaim confidence if topology/device heuristics are not clearly bounded
- registry drift between C++ and UI metadata could create mismatched layouts
- profile growth can become incompatible if layered persistence is not carefully additive

### Medium Risk

- richer automation could increase UI complexity unless ambiguity handling stays simple
- standalone vs plugin-host discovery may diverge in capability
- custom layout support could encourage unsupported routing before backend expansion lands

### Low Risk

- additive visualization and library metadata improvements
- analysis copy and recommendation surfacing

## Validation Lanes

Foundational lanes:
- `cd Source/ui && npm run typecheck`
- `cd Source/ui && npm run build`
- `cmake --build build_local --config Release --target LocusQ_Standalone -j 8`

Truthfulness dependencies:
- `./scripts/qa-bl101-calibrate-truthfulness-mac.sh --contract-only`
- `./scripts/qa-bl101-calibrate-truthfulness-mac.sh --execute --runs 3`
- `./scripts/qa-bl099-headphone-truthfulness-mac.sh --contract-only`
- `./scripts/qa-bl099-headphone-truthfulness-mac.sh --execute --runs 3`

Future automation lanes to add:
- `qa-bl102-discovery-graph-mac.sh`
- `qa-bl103-topology-registry-mac.sh`
- `qa-bl104-headphone-personalization-flow-mac.sh`
- `qa-bl105-calibration-analysis-reports-mac.sh`

## Suggested Backlog Split

Recommended follow-on backlog structure:
- `BL-102` Discovery graph and automatic output/input identification
- `BL-103` Topology and device registry expansion
- `BL-104` Headphone personalization workflow expansion
- `BL-105` Calibration analysis and multi-pass diagnostics

If the team prefers fewer items, BL-101 can remain the umbrella and these can be tracked as explicit sub-waves.

## Success Definition

This packet succeeds if CALIBRATE growth from here becomes structured:
- automation is better, but still honest
- new layouts and devices can be added with less UI surgery
- headphone personalization becomes a real workflow
- saved calibration results become diagnosable assets

## Recommended Next Step

Start with `BL-102` style work under the BL-101 umbrella:
- design the `DiscoveryGraph`
- publish output/input candidates
- expose ambiguity and confirmation states in the UI

That is the highest-leverage move because it improves both speaker and headphone setup without pretending that measurement or personalization has already been solved.
