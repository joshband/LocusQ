Title: BL-026 Calibrate View V2 Multi-Topology UI/UX Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-026 Calibrate View V2 Multi-Topology UI/UX Spec

## Purpose
Define a CALIBRATE v2 redesign that supports multiple monitoring/output configurations (mono, stereo, quad, surround, binaural/headphone, ambisonic, and downmix paths) while preserving deterministic calibration contracts and WebView host reliability.

## Backlog Link
- Proposed Backlog ID: `BL-026`
- Canonical backlog file: `Documentation/backlog-post-v1-agentic-sprints.md`

## Companion Specs
1. Existing paired spec:
- `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md`
2. Planned follow-on spec (next tranche):
- `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md` (authored; implementation pending)

## Problem Statement
Current CALIBRATE UX is functional for baseline 4-speaker workflows, but it is not profile-driven and does not scale clearly to modern output topologies (headphones/binaural, surround variants, ambisonic, and downmix-validation paths). Operators cannot reliably calibrate and store multiple audio configuration profiles from one coherent workflow.

## Current-State Strengths (Preserve)
1. Native bridge lifecycle for start/abort/status is production wired.
- Reference: `Source/PluginEditor.cpp:122`
- Reference: `Source/PluginEditor.cpp:572`
- Reference: `Source/ui/public/js/index.js:5058`
2. Calibration state machine is explicit and deterministic (`Idle/Playing/Recording/Analyzing/Complete/Error`).
- Reference: `Source/CalibrationEngine.h:57`
- Reference: `Source/PluginProcessor.cpp:14`
- Reference: `Source/PluginProcessor.cpp:1453`
3. Auto-detected calibration routing already exists and should remain authoritative for safe defaults.
- Reference: `Source/PluginProcessor.cpp:1835`
- Reference: `Source/PluginProcessor.cpp:1431`
4. Renderer already exposes headphone/spatial profile diagnostics that CALIBRATE can consume as validation signals.
- Reference: `Documentation/scene-state-contract.md:145`
- Reference: `Source/PluginProcessor.cpp:1082`
- Reference: `Source/PluginProcessor.cpp:1092`

## Current-State Gaps (Address)
1. CALIBRATE panel topology model is limited (`4x Mono` / `2x Stereo`) with fixed `SPK1..SPK4` rows.
- Reference: `Source/ui/public/index.html:879`
- Reference: `Source/PluginProcessor.cpp:1832`
2. No CALIBRATE-side monitoring profile selection for headphone/device-specific paths.
- Headphone profile exists under Renderer only.
- Reference: `Source/ui/public/index.html:1119`
3. Multi-topology outputs are available in renderer profile contracts but not represented in calibration IA.
- Reference: `Source/PluginProcessor.cpp:2800`
- Reference: `Documentation/scene-state-contract.md:146`
4. Workflow status is progress-centric but not validation-centric.
- Lacks explicit pass/fail blocks for channel map, polarity/phase, and profile activation checks.
- Reference: `Source/ui/public/js/index.js:5917`
5. No profile library dedicated to storing calibration results per topology+monitoring path.
6. Downmix and virtualization verification paths are implicit; no guided validation UI.

## Design Goals
1. Make CALIBRATE profile-driven rather than single-session-driven.
2. Support topology calibration presets for:
- mono, stereo, quadraphonic, surround (`5.1`, `7.1.2`, `7.4.2`), ambisonic, binaural/headphone, and multichannel downmix-to-stereo validation.
3. Keep calibration lifecycle deterministic and RT-safe.
4. Preserve host reliability across WebView runtimes (Standalone, REAPER, host matrix).
5. Minimize cognitive load with staged workflow and explicit status.
6. Keep compatibility with existing v1 parameter/runtime contracts whenever possible.

## Non-Goals
1. No rewrite of core DSP rendering algorithms in this slice.
2. No mandatory break to current `cal_*` parameter IDs.
3. No hard dependency on external companion apps for baseline calibration flow.

## CALIBRATE V2 IA Redesign
Order in rail:
1. `Profile Setup`
- `Topology` (speaker/format target)
- `Monitoring Path` (speakers, headphones, downmix verification, virtual binaural)
- `Device Profile` (generic, AirPods Pro 2, Sony WH-1000XM5, custom SOFA)
2. `Output Mapping`
- Dynamic channel map matrix based on selected topology.
- Redetect routing action and per-channel confidence/status.
3. `Mic and Stimulus`
- Mic channel/input source.
- Test signal type and level.
- Safety gates (clipping/noise floor indicators).
4. `Measurement Run`
- Start/abort/measure-again transport.
- Per-stage progress with explicit phase labels.
5. `Validation`
- Channel assignment check.
- Phase/polarity check.
- Distance/delay sanity.
- Headphone/spatial profile activation diagnostics.
6. `Calibration Profiles Library`
- Save/load/rename/delete per topology+monitoring path.
- Dirty-state signaling and last validation result.

## Topology Matrix Contract
CALIBRATE v2 will present operator-facing topology labels with deterministic mapping to current runtime profiles:
1. `Mono` -> calibration map size `1`; renderer fallback/stereo-safe.
2. `Stereo` -> map size `2`; aligns with `Stereo 2.0`.
3. `Quadraphonic` -> map size `4`; aligns with `Quad 4.0`.
4. `5.1` -> topology profile target; renderer profile alias to current surround contract.
5. `7.1.2` -> topology profile target; renderer profile alias/fallback contract if unsupported.
6. `7.4.2 / Atmos-style` -> aligns with existing `Surround 7.4.2` / `Atmos Bed` paths.
7. `Ambisonic (FOA/HOA)` -> validation path bound to ambisonic diagnostics.
8. `Binaural / Headphone` -> binds monitoring path + headphone profile diagnostics.
9. `Multichannel -> Stereo Downmix` -> explicit validation profile (speaker route + headphone/downmix verification).

Note:
- Where naming differs between operator-facing topology labels and current renderer profile enum labels, UI must preserve a deterministic alias table and expose active resolved target in status.

## Interaction Contracts

### Profile Authority Contract
1. Every calibration run is tagged with `(topology, monitoring path, device profile)`.
2. Profile context changes set dirty-state and require explicit save/apply.
3. Loading a profile restores all relevant calibration controls atomically.

### Mapping Contract
1. Output matrix row count follows selected topology and available output channels.
2. Redetect routing must never silently overwrite non-default custom maps unless user confirms.
3. Invalid mappings are blocked before run start.

### Measurement Contract
1. `START` runs only when mode is `Calibrate` and mapping is valid.
2. `ABORT` is always available during active phases.
3. Stage progress mirrors native state machine without inferred hidden states.

### Headphone/Spatial Validation Contract
1. If monitoring path is headphone/virtual, CALIBRATE validates:
- `rendererHeadphoneModeRequested/Active`
- `rendererHeadphoneProfileRequested/Active`
- Steam init diagnostics when binaural requested.
2. Spatial topology checks consume:
- `rendererSpatialProfileRequested/Active/Stage`
- Ambisonic diagnostics fields when relevant.

### Downmix Validation Contract
1. Downmix mode performs deterministic A/B validation:
- source topology render
- expected stereo downmix behavior
2. UI reports pass/fail with bounded tolerances and explicit fallback stage reporting.

## Visual Language and UX Rules
1. Keep CALIBRATE visual language aligned with EMITTER v2 chips/status/section rhythm.
2. Use one primary CTA in run section (`START` / `ABORT` / `MEASURE AGAIN`).
3. Add compact status chips:
- `Topology`
- `Monitoring`
- `Profile`
- `Validation`
4. Keep advanced diagnostics collapsed by default.
5. Ensure compact-window behavior preserves full visibility of run status and validation summary blocks.

## Technical Implementation Plan (Patch Slices)

### Slice A: IA + profile scaffolding
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
Changes:
1. Rebuild CALIBRATE panel into staged cards (Profile, Mapping, Mic/Stimulus, Run, Validation, Library).
2. Add status chips and responsive tokens aligned to EMITTER v2 conventions.
Acceptance:
- CALIBRATE panel remains readable/actionable at compact and wide widths.

### Slice B: Topology profile model
Files:
- `Source/PluginProcessor.cpp`
- `Source/PluginProcessor.h`
- `Source/PluginEditor.cpp`
- `Source/PluginEditor.h`
- `Source/ui/public/js/index.js`
Changes:
1. Add calibration topology profile parameter/state (`cal_topology_profile`, alias table to renderer profiles).
2. Expose resolved topology in calibration status payload.
Acceptance:
- Topology selection is deterministic and persisted.

### Slice C: Dynamic mapping matrix + routing safety
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Source/PluginProcessor.cpp`
Changes:
1. Replace fixed `SPK1..SPK4` rows with topology-driven map rows.
2. Keep existing auto-routing logic as default bootstrap path.
3. Add explicit overwrite confirmation behavior for custom maps.
Acceptance:
- Redetect is deterministic, and custom maps are protected.

### Slice D: Headphone/spatial validation integration
Files:
- `Source/ui/public/js/index.js`
- `Source/PluginProcessor.cpp`
- `Documentation/scene-state-contract.md`
Changes:
1. Add CALIBRATE validation block for headphone and spatial profile activation diagnostics.
2. Surface exact fallback/init stage reason when requested mode/profile is not active.
Acceptance:
- Headphone/spatial validation shows deterministic pass/fail with stage detail.

### Slice E: Calibration profile library
Files:
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Source/PluginProcessor.cpp`
- `Source/PluginProcessor.h`
Changes:
1. Add save/load/rename/delete for calibration profiles with inline naming.
2. Persist profile metadata including topology+monitoring+device tuple and last validation summary.
Acceptance:
- Operators can switch calibration contexts without manual reconfiguration.

## Validation Plan

### Automated
1. Syntax/build:
- `node --check Source/ui/public/js/index.js`
- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
2. Production self-test lane extensions:
- `UI-P1-026A`: topology profile switch and matrix row-count contract.
- `UI-P1-026B`: routing redetect/custom-map protection contract.
- `UI-P1-026C`: run lifecycle (`start/abort/measure-again`) determinism.
- `UI-P1-026D`: headphone/spatial profile activation diagnostics in CALIBRATE validation block.
- `UI-P1-026E`: downmix validation path contract.
3. Host automation:
- `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`

### Manual (Operator Required)
1. Headphone listening validation for:
- AirPods Pro 2
- Sony WH-1000XM5
2. Binaural/spatial audibility A/B checks (`stereo_downmix` vs binaural path).
3. Multi-topology session verification in host UI for map correctness and validation summary clarity.

## Risks and Mitigations
1. Risk: topology sprawl creates confusing IA.
- Mitigation: profile-first flow and strict staged grouping.
2. Risk: host/runtime mismatch between requested and active profiles.
- Mitigation: deterministic diagnostics surfaced in CALIBRATE validation block.
3. Risk: routing auto-detect overwrites operator intent.
- Mitigation: protect custom routes and require explicit overwrite action.
4. Risk: expanded UI regresses compact layout behavior.
- Mitigation: responsive contract lane and minimum-height assertions.

## Renderer V2 Handoff Requirements
CALIBRATE v2 must hand clean contracts to RENDERER v2:
1. Shared profile dictionary and alias table (single source of truth).
2. Shared diagnostics chips semantics (requested vs active vs fallback stage).
3. Shared profile library metadata schema for cross-panel coherence.

## Exit Criteria
1. BL-026 self-test lanes pass (`UI-P1-026A..E`).
2. Existing BL-025/BL-019/BL-022 assertions remain green.
3. REAPER headless host lane passes with fresh evidence.
4. Manual headphone listening checks logged as operator evidence.
5. Backlog/status/evidence docs synchronized.

## Deliverables
1. This CALIBRATE v2 spec.
2. Backlog row(s) for BL-026 and BL-027.
3. Follow-on RENDERER v2 spec (`BL-027`) in the same plans folder.
