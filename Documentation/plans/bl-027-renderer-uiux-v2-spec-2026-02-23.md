Title: BL-027 Renderer View V2 Multi-Profile UI/UX Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-027 Renderer View V2 Multi-Profile UI/UX Spec

## Purpose
Define RENDERER v2 as a profile-authoritative monitoring and diagnostics surface that clearly represents requested vs active render topology, fallback stages, and output routing while preserving deterministic runtime behavior in JUCE WebView hosts.

## Backlog Link
- Backlog ID: `BL-027`
- Canonical backlog file: `Documentation/backlog-post-v1-agentic-sprints.md`

## Companion Specs
1. EMITTER v2 foundation:
- `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md`
2. CALIBRATE v2 topology/profile workflow:
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`

## Implementation Status Ledger (2026-02-23)

### Complete (Planning Artifacts)
1. Full BL-027 v2 spec authored with IA, interaction contracts, and slice-based implementation plan.
2. Dependency/handoff assumptions with BL-025 and BL-026 documented in this file and main backlog.

### In Review
1. Final naming of profile chips and alias labels across CALIBRATE and RENDERER.
2. Exact self-test lane naming and thresholds for renderer compact-layout assertions.

### Remaining
1. Implement Slices A-E.
2. Add renderer v2 self-test lanes (`UI-P2-027A..E`) to production lane.
3. Promote BL-027 to `In Validation` after first slice evidence.

## Problem Statement
Current RENDERER controls are functional, but the panel is section-flat and does not make profile authority explicit. Key diagnostics exist in scene snapshots and viewport overlay strings, yet they are not surfaced as structured, scan-friendly controls near the corresponding settings. This increases operator uncertainty around fallback conditions and weakens cross-panel coherence with CALIBRATE v2 profile workflows.

## Current-State Strengths (Preserve)
1. Renderer parameter pathway is deterministic and already maps runtime controls into spatial renderer state each block.
- Reference: `Source/PluginProcessor.cpp:558`
- Reference: `Source/PluginProcessor.cpp:571`
- Reference: `Source/PluginProcessor.cpp:576`
2. Scene snapshot already publishes requested/active/stage diagnostics for spatial profile, headphone mode/profile, Steam init, and ambisonic status.
- Reference: `Source/PluginProcessor.cpp:1027`
- Reference: `Source/PluginProcessor.cpp:1031`
- Reference: `Source/PluginProcessor.cpp:1081`
- Reference: `Source/PluginProcessor.cpp:1270`
- Reference: `Documentation/scene-state-contract.md:132`
3. UI already computes and displays fallback-aware renderer summary text in viewport overlay.
- Reference: `Source/ui/public/js/index.js:5791`
- Reference: `Source/ui/public/js/index.js:5813`
- Reference: `Source/ui/public/js/index.js:5849`
4. Viewport has established listener/speaker and energy telemetry rendering that can anchor renderer diagnostics.
- Reference: `Source/ui/public/js/index.js:4193`
- Reference: `Source/ui/public/js/index.js:4235`
- Reference: `Source/ui/public/js/index.js:6535`
5. Spatial profile/headphone enums and string contracts already exist in renderer core for deterministic aliasing.
- Reference: `Source/SpatialRenderer.h:60`
- Reference: `Source/SpatialRenderer.h:516`
- Reference: `Source/SpatialRenderer.h:538`

## Current-State Gaps (Address)
1. `Spatial Profile` exists in APVTS/runtime contracts but is not exposed as a renderer panel control in current HTML/JS bindings.
- Reference: `Source/PluginProcessor.cpp:2800`
- Reference: `Source/ui/public/index.html:1116`
- Reference: `Source/ui/public/js/index.js:322`
2. Renderer IA is not profile-first; topology authority is split across fields without explicit requested vs active ownership line.
- Reference: `Source/ui/public/index.html:1098`
3. Diagnostics are packed into one overlay string instead of structured chips and section-level statuses.
- Reference: `Source/ui/public/js/index.js:5864`
4. Speaker section is fixed to four rows and does not reflect active output layout/profile context.
- Reference: `Source/ui/public/index.html:1109`
- Reference: `Source/PluginProcessor.cpp:1100`
5. Scene list actions (`S`/`M`) are terse and do not communicate local/remote authority constraints clearly.
- Reference: `Source/ui/public/js/index.js:6391`
6. Compact-window readability can degrade due to dense section stack and lack of renderer-specific responsive grouping.
- Reference: `Source/ui/public/index.html:1098`
- Reference: `Source/ui/public/index.html:1140`

## Design Goals
1. Make render topology/profile authority explicit and visible at all times.
2. Surface deterministic `requested -> active -> stage` diagnostics in structured UI, not only overlay text.
3. Unify profile semantics with CALIBRATE v2 alias dictionary.
4. Improve speaker/output routing clarity for mono/stereo/quad/surround/ambisonic/headphone contexts.
5. Preserve existing realtime-safe DSP and snapshot contracts.
6. Keep renderer panel actionable in compact host sizes.

## Non-Goals
1. No rewrite of core spatial DSP algorithms in BL-027.
2. No breaking rename of existing APVTS parameter IDs in this tranche.
3. No mandatory implementation of codec output formats beyond existing placeholder stages.
4. No head-tracking companion bridge implementation in this task (BL-017 scope).

## RENDERER V2 IA Redesign
Order in rail:
1. `Profile Authority`
- `Spatial Profile` (auto/stereo/quad/surround/ambisonic/atmos/virtual/codec placeholders).
- `Headphone Mode` and `Headphone Device Profile`.
- Chips: `Requested`, `Active`, `Stage`.
- Output summary readback (`layout`, `channels`, `route`).
2. `Output and Speakers`
- Active output map summary.
- Dynamic speaker rows for visible active path with trim/delay controls and status tags.
- Quad mapping and fallback indicators where relevant.
3. `Spatialization`
- Distance model, reference/max distance, doppler, air absorption.
4. `Room Acoustics`
- Enable, mix, size, damping, ER-only with clear state chips.
5. `Diagnostics`
- Steam init status (`compiled`, `available`, `stage`, `error`).
- Ambisonic status (`compiled`, `active`, `order`, `stage`).
- Physics lens quick controls and diagnostic mix.
6. `Scene Monitor`
- Emitter list with explicit `Solo`/`Mute` labels and local-authority cues.

## Renderer Profile Matrix Contract
Renderer v2 uses operator-facing labels mapped deterministically to core strings:
1. `Auto` -> `auto`
2. `Stereo 2.0` -> `stereo_2_0`
3. `Quad 4.0` -> `quad_4_0`
4. `Surround 5.2.1` -> `surround_5_2_1`
5. `Surround 7.2.1` -> `surround_7_2_1`
6. `Surround 7.4.2` -> `surround_7_4_2`
7. `Ambisonic FOA` -> `ambisonic_foa`
8. `Ambisonic HOA` -> `ambisonic_hoa`
9. `Atmos Bed` -> `atmos_bed`
10. `Virtual 3D Stereo` -> `virtual_3d_stereo`
11. `Codec IAMF` -> `codec_iamf`
12. `Codec ADM` -> `codec_adm`

Stage mapping:
1. `direct`
2. `fallback_stereo`
3. `fallback_quad`
4. `ambi_decode_stereo`
5. `codec_layout_placeholder`

Note:
- All operator-facing labels must display resolved active value and stage when active differs from requested.
- Backward-compatible label aliases are allowed for operator familiarity:
  - `5.1` label -> `surround_5_2_1`
  - `7.1.2` label -> `surround_7_2_1`

## Interaction Contracts

### Profile Authority Contract
1. Changing `Spatial Profile`, `Headphone Mode`, or `Headphone Profile` updates requested state immediately.
2. UI always shows both requested and active states and highlights divergence.
3. Stage chip is mandatory whenever active path is non-direct.

### Diagnostics Contract
1. Steam diagnostics must surface exact init stage and error code when binaural request cannot activate.
2. Ambisonic diagnostics must show `compiled`, `active`, and `order` with stage.
3. Missing diagnostics fields must degrade gracefully and never block control interaction.

### Output Routing Contract
1. Output summary reflects negotiated host layout and `rendererOutputChannels`.
2. Dynamic speaker rows follow active output path and avoid stale static labels.
3. Quad order remap visibility must be explicit when active mode is `quad_map_first4`.

### Scene Monitor Contract
1. Scene list labels actions as `Solo`/`Mute` (not cryptic single-letter buttons).
2. Local-only control constraints remain enforced and visibly indicated.
3. Renderer scene list selection must not mutate emitter transport/timeline state.

### Cross-Panel Sync Contract
1. RENDERER and CALIBRATE use one shared profile alias dictionary.
2. Profile changes in one panel are reflected in the other through scene-state/APVTS synchronization.
3. Diagnostics chip semantics (`requested`, `active`, `stage`) remain identical across panels.

## Visual Language and UX Rules
1. Reuse BL-025/BL-026 status-chip grammar and section rhythm.
2. Keep one primary emphasis cluster at top: `Profile Authority`.
3. Present fallback and failure states with compact chips before verbose details.
4. Keep advanced diagnostics collapsible by default.
5. Ensure compact layout keeps `Profile Authority` and `Diagnostics` summaries visible without scrolling to panel bottom.

## Technical Implementation Plan (Patch Slices)

### Slice A: IA and profile-authority shell
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
Changes:
1. Rebuild renderer panel into staged cards (Profile, Output/Speakers, Spatialization, Room, Diagnostics, Scene).
2. Add requested/active/stage chips and output summary line.
Acceptance:
- Renderer panel clearly exposes profile authority and fallback stage at first glance.

### Slice B: Spatial profile control binding and alias table
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Source/SpatialRenderer.h`
- `Source/PluginProcessor.cpp`
Changes:
1. Add visible `Spatial Profile` dropdown bound to renderer APVTS state.
2. Add shared UI alias dictionary aligned to renderer core string mapping.
Acceptance:
- Spatial profile request is controllable from renderer UI and reflected in snapshot diagnostics.

### Slice C: Dynamic output/speaker presentation
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Source/PluginProcessor.cpp`
Changes:
1. Replace fixed static speaker rows with dynamic view tied to active output profile/layout.
2. Surface output route/readback including fallback mode hints.
Acceptance:
- Speaker/output section matches active rendering context and avoids misleading static rows.

### Slice D: Diagnostics cards and failure-stage visibility
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Documentation/scene-state-contract.md`
Changes:
1. Add dedicated Steam and Ambisonic diagnostics rows/cards.
2. Surface deterministic stage/error details inline when mismatch/fallback occurs.
Acceptance:
- Operators can identify exact failure stage without parsing long overlay text.

### Slice E: Scene monitor polish and cross-panel coherence
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
Changes:
1. Convert scene actions to explicit labels and authority cues.
2. Confirm CALIBRATE/RENDERER shared profile semantics in UI copy and chips.
Acceptance:
- Scene monitor actions are explicit, and cross-panel profile semantics are consistent.

## Validation Plan

### Automated
1. Syntax/build:
- `node --check Source/ui/public/js/index.js`
- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
2. Production self-test lane extensions:
- `UI-P2-027A`: renderer spatial profile control binding and requested/active chip contract.
- `UI-P2-027B`: fallback stage visibility contract (`direct` vs fallback enums).
- `UI-P2-027C`: Steam binaural request failure-stage surfacing contract.
- `UI-P2-027D`: ambisonic diagnostics visibility and consistency contract.
- `UI-P2-027E`: compact renderer layout visibility/non-clipping contract.
3. Host/runtime lanes:
- `./scripts/standalone-ui-selftest-production-p0-mac.sh`
- `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`
- `./scripts/validate-docs-freshness.sh`

### Manual (Operator Required)
1. Host visual verification of renderer compact/medium/wide panel states.
2. Profile switching checks:
- stereo
- quad
- surround
- ambisonic
- headphone binaural/downmix
3. Headphone verification with:
- AirPods Pro 2
- Sony WH-1000XM5
4. Confirm CALIBRATE panel reflects renderer profile changes and vice versa.

## Risks and Mitigations
1. Risk: profile complexity overwhelms renderer panel.
- Mitigation: strict profile-first IA with collapsible advanced diagnostics.
2. Risk: mismatch between UI alias labels and runtime strings.
- Mitigation: single alias dictionary sourced from renderer enum/string contract.
3. Risk: dynamic speaker/output views diverge from host routing truth.
- Mitigation: derive display from snapshot `rendererOutputChannels` and negotiated layout only.
4. Risk: compact layout regressions.
- Mitigation: dedicated renderer compact lane (`UI-P2-027E`) and host spot-check checklist.

## BL-026/BL-025 Handoff Requirements
1. Preserve BL-026 shared profile dictionary and diagnostics chip semantics.
2. Preserve BL-025 visual language tokens (chips, hierarchy, authority cues).
3. Keep BL-019 visual diagnostics behavior stable while rearranging renderer controls.

## Exit Criteria
1. BL-027 self-test lanes pass (`UI-P2-027A..E`).
2. Existing BL-025/BL-026/BL-019 assertions remain green.
3. Host smoke lane passes with fresh artifact.
4. Backlog/status/evidence surfaces synchronized.
5. Docs freshness gate passes.

## Deliverables
1. This full BL-027 spec.
2. Backlog row/status ledger updates reflecting `spec complete`.
3. Initial implementation PR plan for Slices A-E with acceptance checks.
