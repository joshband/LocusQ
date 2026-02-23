Title: BL-025 Emitter View V2 UI/UX Consolidation Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-23

# BL-025 Emitter View V2 UI/UX Consolidation Spec

## Purpose
Define a comprehensive redesign and implementation plan for the EMITTER workflow so motion authoring, physics behavior, timeline choreography, presets, and viewport diagnostics are clearer, more deterministic, and faster to operate in host WebView contexts.

## Backlog Link
- Backlog ID: `BL-025`
- Canonical backlog file: `Documentation/backlog-post-v1-agentic-sprints.md`

## Implementation Status Ledger (2026-02-23)

### Complete (Evidence-Backed)
1. Slice-level BL-025 architecture is shipped in the production UI path (`Slice 1..5` in this spec).
2. Transport/timeline regressions called out in manual QA are currently passing:
- `UI-06`: PASS
- `UI-07`: PASS
3. BL-025 responsive/authority/preset self-test lanes are passing in isolated runs:
- `UI-P1-025A`
- `UI-P1-025B`
- `UI-P1-025C`
- `UI-P1-025D`
- `UI-P1-025E`
4. Host spot-check automation for this lane has a passing artifact in current backlog cycle.

Primary evidence:
- `Documentation/testing/bl-025-emitter-resize-manual-qa-2026-02-23.md`
- `TestEvidence/locusq_production_p0_selftest_20260223T010422Z.json`
- `TestEvidence/reaper_headless_render_20260223T010307Z/status.json`

### In Review
1. Preset lifecycle operator clarity (not functional correctness) remains under UX review for closeout language alignment (`SAVE/LOAD/RENAME/DELETE` discoverability and naming flow confidence).
2. Final closure wording for BL-025 to `Done` is pending synchronized updates across backlog + evidence docs.
3. BL-023 dependency handoff notes need to remain explicit in backlog sequencing.

### Remaining to Close BL-025
1. Refresh one deterministic production self-test run and keep artifact pointer current in backlog closeout notes.
2. Perform one host-side manual preset lifecycle spot-check and record short result row in `Documentation/testing/bl-025-emitter-resize-manual-qa-2026-02-23.md`.
3. Move BL-025 from `In Progress` to `Done` only after docs/evidence/status synchronization is complete.

## Problem Statement
The current EMITTER surface is functionally rich, but interaction ownership is fragmented across Physics/Animation/Timeline/Choreography/Preset sections, with duplicated semantics and weak state visibility. This slows authoring, increases operator uncertainty, and raises regression risk across hosts.

## Current-State Strengths (Preserve)
1. Deterministic keyframe timeline editing exists and is production wired.
- Reference: `Source/ui/public/js/index.js:1660`
2. Choreography packs generate usable motion motifs and synchronize spherical/cartesian tracks.
- Reference: `Source/ui/public/js/index.js:1054`
- Reference: `Source/ui/public/js/index.js:1310`
- Reference: `Source/ui/public/js/index.js:1347`
3. Physics visualization is expressive (force, velocity, collision pulse, trajectory).
- Reference: `Source/ui/public/js/index.js:4937`
- Reference: `Source/ui/public/js/index.js:5112`
4. UI/native state persistence for emitter label, physics preset, and choreography pack already exists.
- Reference: `Source/ui/public/js/index.js:918`
- Reference: `Source/ui/public/js/index.js:942`

## Current-State Gaps (Address)
1. Position mode mismatch.
- UI exposes `Spherical/Cartesian`, but only spherical inspector fields are visible.
- Reference: `Source/ui/public/index.html:597`
- Engine supports both coordinate representations.
- Reference: `Source/PluginProcessor.cpp:700`
- Reference: `Source/PluginProcessor.cpp:775`
2. Motion control fragmentation and duplicated loop/sync semantics.
- Reference: `Source/ui/public/index.html:521`
- Reference: `Source/ui/public/index.html:661`
- Reference: `Source/ui/public/js/index.js:600`
3. Preset model ambiguity (`SAVE PACK` vs `SAVE/LOAD`), host prompt fragility, weak lifecycle management.
- Reference: `Source/ui/public/index.html:680`
- Reference: `Source/ui/public/index.html:694`
- Reference: `Source/ui/public/js/index.js:1491`
- Reference: `Source/ui/public/js/index.js:3837`
4. Multi-emitter authoring scope is implicit.
- Local-only drag enforcement exists but is not surfaced in EMITTER UX.
- Reference: `Source/ui/public/js/index.js:2123`
- Reference: `Source/PluginProcessor.cpp:1256`
5. Diagnostics controls affecting emitter workflow are separated into Renderer telemetry section.
- Reference: `Source/ui/public/index.html:746`
- Reference: `Source/ui/public/js/index.js:4324`
6. Fixed dimensions and non-responsive layout increase failure risk on resize and compact plugin windows.
- Reference: `Source/ui/public/index.html:37`
- Reference: `Source/ui/public/index.html:42`
- Reference: `Source/ui/public/index.html:192`

## Design Goals
1. One authoritative motion mental model.
2. Position mode clarity with explicit control visibility.
3. Preset lifecycle reliability in host-constrained WebViews.
4. Immediate visibility into local-vs-remote edit authority.
5. Lower visual entropy and faster scanning.
6. Retain existing DSP/scene-state contracts unless explicitly versioned.

## Non-Goals
1. No change to v1 DSP math contracts for renderer core behavior.
2. No cross-instance coordination changes in this slice.
3. No mandatory migration of existing presets; backward compatibility is required.

## IA Redesign (Emitter V2)
Order in rail:
1. `Emitter Identity`
- Label, Color, Mute, Solo, `Local/Remote` scope chip.
2. `Position`
- Segmented control: `Spherical | Cartesian`.
- Spherical fields: azimuth/elevation/distance.
- Cartesian fields: x/y/z.
- Always show world readback line for operator confidence.
3. `Audio Shape`
- Gain, Spread, Directivity, Aim azimuth/elevation.
4. `Motion`
- `Motion Source`: `Static | Physics | Timeline | Choreography`.
- Shared Transport: rewind/stop/play, loop, sync, speed.
- Source-specific sub-panels appear contextually.
5. `Presets`
- Typed tabs: `Emitter Presets | Motion Presets`.
- Actions: Save, Load, Rename, Delete.
- Inline naming field replaces modal prompt dependency.
6. `Diagnostics` (collapsed by default)
- Physics lens quick toggle + diagnostic mix for emitter editing context.

## Visual Language Simplification
1. Typography hierarchy
- Section headers 12/600 uppercase.
- Field labels 12/500 sentence case.
- Values 12/600 tabular.
2. Accent discipline
- Gold only for active/armed/selected states.
- Secondary controls in neutral tones.
3. Control consistency
- All booleans use uniform toggle rows.
- All numeric controls use a shared editable control pattern (stepper + drag/scroll affordance).
4. Density tuning
- Increase row height from 28 to 32 in EMITTER panel only.
- Group spacing increased between major sections.
5. Status chips
- Add compact chips for `Motion Source`, `Loop`, `Sync`, `Preset Dirty`.

## Interaction Contracts

### Position Mode Contract
1. If mode is `Spherical`, spherical fields are editable and cartesian fields are read-only mirrored.
2. If mode is `Cartesian`, cartesian fields are editable and spherical fields are read-only mirrored.
3. Viewport drag always writes both forms through existing parameter mapping and does not break timeline tracks.
- Reference mapping: `Source/ui/public/js/index.js:2172`

### Motion Contract
1. Single transport controls playback for timeline/choreography-driven motion.
2. `Motion Source` determines which sub-controls are active.
3. Loop/sync are single-source-of-truth controls.
4. Manual transport no longer silently changes unrelated controls without explicit visual state update.

### Preset Contract
1. `Emitter Preset` includes emitter/audio/physics settings.
2. `Motion Preset` includes timeline/choreography metadata.
3. Save naming is inline text input first; no hard dependency on `prompt`.
4. List rows carry type badges and choreography badges.
5. Rename/delete operations are available with deterministic native responses.

### Multi-Emitter Scope Contract
1. If selected emitter is not `localEmitterId`, inspector controls become read-only with explicit `Remote emitter` badge.
2. Viewport selection remains possible for awareness and renderer context.
3. Local-only editing restrictions remain unchanged at engine layer.

### Diagnostics Contract
1. Emitter diagnostics quick controls reflect renderer telemetry state.
2. Advanced telemetry remains in Renderer panel; EMITTER exposes only authoring-relevant subset.

## Technical Implementation Plan (Patch Slices)

### Slice 1: IA + responsive layout foundation
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
Changes:
1. Rebuild EMITTER panel section order and add semantic wrappers.
2. Add responsive tokens for rail/timeline heights and compact width behavior.
3. Add motion status chips and local/remote badge scaffolding.
Acceptance:
- Layout renders correctly at narrow and wide plugin sizes.
- Timeline remains visible and stable in emitter mode.

### Slice 2: Position mode clarity
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
Changes:
1. Add cartesian inspector rows (`X`, `Y`, `Z`) with binding to `pos_x/pos_y/pos_z`.
2. Add mode-aware enable/disable/read-only behavior for dual coordinate groups.
3. Ensure mirrored readback updates from scene snapshots.
Acceptance:
- Switching `Spherical/Cartesian` updates visible editable rows instantly.
- Viewport drag keeps both coordinate forms synchronized.

### Slice 3: Unified motion subsystem
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
Changes:
1. Add `Motion Source` selection and context panels.
2. Unify loop/sync/transport control ownership.
3. Remove duplicated loop semantics in separate sections.
Acceptance:
- Transport behavior deterministic and source-aware.
- No hidden state flips without UI reflection.

### Slice 4: Preset manager lifecycle
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`
Changes:
1. Add typed preset model (`emitter` vs `motion`) and list badges.
2. Add rename/delete native calls and UI actions.
3. Replace modal prompt path with inline name input primary path.
Acceptance:
- Save/load/rename/delete deterministic in host WebView.
- Existing preset compatibility retained.

### Slice 5: Diagnostics + authority UX polish
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
Changes:
1. Add local/remote emitter badge and disable editable controls for remote scope.
2. Add emitter-level diagnostics quick controls and collapse advanced by default.
Acceptance:
- Operators can always identify edit authority.
- Diagnostics do not clutter default authoring flow.

## Validation Plan

### Automated
1. Syntax/build guard:
- `node --check Source/ui/public/js/index.js`
- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
2. Production UI self-test extension lanes:
- Add `UI-P1-025A` position mode visibility/editability.
- Add `UI-P1-025B` unified motion transport semantics.
- Add `UI-P1-025C` preset rename/delete lifecycle.
3. Regression suites:
- `./scripts/standalone-ui-selftest-production-p0-mac.sh`
- `build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`

### Manual
1. EMITTER authoring run in Standalone.
2. Host DAW run for modal/input quirks in WebView runtime.
3. Verify resize behavior at minimum/maximum practical host window sizes.

## Risks and Mitigations
1. Risk: host-specific UI event behavior regression.
- Mitigation: keep `juce-webview-runtime` fallback patterns and add explicit error/status messages.
2. Risk: timeline coupling regressions after motion unification.
- Mitigation: preserve existing timeline serialization contract and add dedicated self-test lanes.
3. Risk: preset migration confusion.
- Mitigation: non-breaking schema with default type inference and backward compatibility.

## Exit Criteria
1. BL-025 self-test lanes pass.
2. Existing BL-019 and BL-022 assertions remain green.
3. Docs freshness gate passes.
4. Backlog/status/evidence synced with artifacts.

## Deliverables
1. This spec document.
2. Backlog row and kickoff entry for BL-025.
3. Status tracking fields in `status.json`.
