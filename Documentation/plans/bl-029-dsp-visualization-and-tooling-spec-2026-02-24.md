Title: BL-029 DSP Visualization and Tooling Spec + Implementation Plan
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-03-18

# BL-029 DSP Visualization and Tooling Spec + Implementation Plan

## Purpose

Define the active implementation contract for BL-029.
Keep the runtime visualization program coherent across `CALIBRATE`, `EMITTER`, and `RENDERER`.

Backlog authority:
- `Documentation/backlog/index.md`
- `Documentation/backlog/done/bl-029-dsp-visualization.md`

Legacy deep spec:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24-legacy.md`

Validation status: `not tested`

This is a plan.
It does not claim runtime validation.

## Goal

Move LocusQ from position-only spatial UI to behavior-first spatial instrumentation.

The active BL-029 priorities are:
1. deterministic modulation trace
2. spectral-spatial room view
3. first-order reflection ghosts
4. offline calibration assistant loop

## Scope

### In Scope

- deterministic trace transport and UI targets
- spectral feature publication for emitter visualization
- first-order reflection geometry and ER centroid diagnostics
- offline session export and recommendation apply flow
- host-safe WebView bridge additions

### Out Of Scope

- audio-thread ML inference
- full acoustic simulation
- APVTS ID renames
- new export formats like ADM or IAMF

## Normative Inputs

- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- `Documentation/scene-state-contract.md`
- `Documentation/implementation-traceability.md`
- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Product Intent

Show what was requested.
Show what was actually applied.
Show spectral and reflection behavior in the same spatial frame.
Keep calibration recommendations offline and auditable.

## Mode Ownership

| Mode | Primary Job | Main Runtime Surface |
|---|---|---|
| `CALIBRATE` | export session state and apply recommendations | calibration APIs and bridge functions |
| `EMITTER` | publish motion/audio truth and spectral-reflection metadata | scene snapshot path |
| `RENDERER` | publish applied renderer truth and output energy centroid | renderer parameter update path and diagnostics |

## Priority Contract

### 1. Deterministic Modulation Visualizer

Goal:
- expose base vs applied truth at the DSP usage point

Contract:
- capture at runtime truth, not UI intent
- use lock-free SPSC transport
- keep cadence block-accurate by default
- make overflow behavior explicit

Minimum schema:
- `target`
- `sampleRate`
- `points[]` with:
  - `t`
  - `base`
  - `applied`

### 2. Spectral-Spatial Hybrid Room View

Goal:
- show timbral behavior inside the spatial room view

Feature set:
- `centroidHz`
- `rolloffHz`
- `hfRatio`
- `flux`
- `fluxEma`

Contract:
- compute on the message thread only
- cache per-emitter spectral state
- clamp all values finite before publish

### 3. Reflection Ghost Modeling

Goal:
- visualize first-order image sources and early-reflection bias

Contract:
- rectangular-room first-order ghosts only
- weighted by room settings, damping, brightness, and inverse-distance logic
- publish `reflections[]`, `erCentroid`, and `erShift`
- present this as a perceptual model, not a full acoustic simulation

### 4. Offline Calibration Assistant

Goal:
- export deterministic session bundles and apply deterministic recommendations

Contract:
- no runtime inference in the plugin audio path
- schema-validated session export
- schema-validated recommendation apply path
- heuristic offline analyzer remains the baseline; ML is optional follow-on work

## Implementation Slices

| Slice | Focus | Main Outputs |
|---|---|---|
| A | trace core for emitter truth | trace enum, ring transport, emitter capture |
| B | trace core for renderer truth | requested vs active state traces |
| C | spectral-spatial hybrid | feature cache, flux fields, room styling data |
| D | constellation view | feature-space mode and labels |
| E | reflection ghosts | ghost geometry, ER centroid, shift metrics |
| F | energy centroid integration | output-energy centroid metrics and trace targets |
| G | calibration export/apply | session bundle export and recommendation apply path |
| H | offline analyzer tooling | deterministic analyzer and optional dataset tooling |

## Threading And Runtime Rules

1. No allocation, locks, or blocking I/O in `processBlock()`.
2. Spectral and reflection computations stay on the message-thread snapshot path.
3. Native bridge callbacks must not mutate DSP graph shape directly.
4. All published values must be finite and clamped before UI or DSP use.

## Host And Runtime Contract

Minimum lanes:
- standalone macOS
- REAPER VST3
- REAPER CLAP when CLAP artifacts are enabled

WebView contract:
- bridge timing must stay deterministic
- startup ordering must stay explicit
- failures must degrade with visible status, not silence

## Validation Plan

### Automated

- `cd Source/ui && npm run typecheck`
- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
- `./scripts/standalone-ui-selftest-production-p0-mac.sh`
- `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`

### Planned Assertions

| ID | Check |
|---|---|
| `UI-P2-029A` | trace target selection and schema validity |
| `UI-P2-029B` | base vs applied divergence under internal animation/physics |
| `UI-P2-029C` | spectral payload finite/clamped and smoothed |
| `UI-P2-029D` | reflection payload and ER centroid finite |
| `UI-P2-029E` | calibration export/apply roundtrip |

### Manual

- standalone visual verification for room and constellation modes
- REAPER interaction checks for trace controls and fallback status
- headphone-path sanity check for requested vs active visibility

## Risks

| Risk | Why it matters | Mitigation |
|---|---|---|
| payload growth | UI cadence can regress | cap work and keep stale-mode fallback |
| trace overhead | polling can starve UI thread | bound history and pop count |
| reflection overclaim | users may read ghosts as full acoustics | label as first-order perceptual model |
| calibration drift | recommendation contract can change over time | schema versioning and evidence blocks |

## Exit Criteria

1. All four BL-029 priorities are implemented behind deterministic contracts.
2. Realtime invariants remain intact.
3. Host lanes pass with updated evidence.
4. Scene-state and trace schemas stay synchronized.
5. Tier 0 surfaces are updated when BL-029 acceptance claims change.

## Visual Aid Index

| Artifact | Role |
|---|---|
| `Documentation/backlog/done/bl-029-dsp-visualization.md` | backlog closeout and status context |
| `Documentation/plans/bl-029-cinematic-reactive-preset-language-2026-02-24.md` | supporting preset-language companion spec |
| `Documentation/scene-state-contract.md` | snapshot contract reference |
| archived legacy spec | deep slice and threshold detail |

## Archive Note

The original long-form BL-029 spec was preserved at:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24-legacy.md`

Use the archive copy only when you need the detailed historical slice breakdown.
Use this file for current planning.
