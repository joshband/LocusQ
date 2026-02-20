Title: LocusQ v3 Stage 9+ Detailed Checklists
Document Type: Stage Checklist
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# LocusQ v3 Stage 9+ Detailed Checklists

## Purpose
Define detailed implementation, testing, and documentation checklists for Stages 9 through 13 so parity work can be executed in small, verifiable increments without losing contract alignment.

This document is the execution companion to:
- `Documentation/v3-ui-parity-checklist.md` (high-level parity tracker)
- `Design/v3-ui-spec.md` (UI behavior contract)
- `Design/v3-style-guide.md` (visual/token contract)

## Normative Inputs
- `.ideas/creative-brief.md`
- `.ideas/parameter-spec.md`
- `.ideas/architecture.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- `Documentation/implementation-traceability.md`
- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Stage 9 - Emitter Rail Parity Completion

### Objective
Complete emitter-rail parity in incremental UI beyond Stage 8 subset: identity, position, size depth, physics controls, animation speed, and preset lifecycle controls.

### Task Checklist
- [x] `S9-T1` Add emitter identity controls to incremental UI surface (`emit_label`, `emit_color`) and live status/readout updates.
- [x] `S9-T2` Add emitter position controls (`pos_coord_mode`, `pos_azimuth`, `pos_elevation`, `pos_distance`) plus Cartesian sync readout (`pos_x`, `pos_y`, `pos_z`).
- [x] `S9-T3` Extend size controls with linked/unlinked behavior coverage (`size_uniform`, `size_link`, `size_width`, `size_depth`, `size_height`).
- [x] `S9-T4` Add physics preset + advanced controls (`phys_mass`, `phys_drag`, `phys_elasticity`, `phys_gravity`, `phys_gravity_dir`, `phys_friction`, `phys_throw`, `phys_reset`).
- [x] `S9-T5` Extend animation controls (`anim_speed`) and ensure timeline transport controls reflect live state (`play/stop/rewind`, loop/sync continuity).
- [x] `S9-T6` Add preset lifecycle controls wired to native functions (`locusqListEmitterPresets`, `locusqSaveEmitterPreset`, `locusqLoadEmitterPreset`).
- [x] `S9-T7` Add Stage 9 self-test script and default gate wiring (`scripts/standalone-ui-selftest-stage9-mac.sh`, `scripts/ui-pr-gate-mac.sh`).
- [x] `S9-T8` Update resource routing and default stage metadata (`Source/PluginEditor.cpp`, staged incremental index/js assets, window title tag).

### Acceptance Checklist
- [x] Every added control mutates real APVTS/native state (no cosmetic-only controls).
- [x] Every added control shows status/readout reflection in Stage 9 UI diagnostics/status rows.
- [x] Stage 9 self-test returns `ok=true`.
- [x] UI PR gate defaults to Stage 9 self-test and passes.

### Expected Evidence
- `TestEvidence/locusq_incremental_stage9_selftest_<timestamp>.json`
- `TestEvidence/ui_pr_gate_<timestamp>/status.tsv`

### Codex Mega-Prompts

`S9-T1`
```text
Use $skill_impl and $skill_docs.
Implement Stage 9 Task S9-T1 in LocusQ incremental UI.
Goal: add emitter identity controls (emit_label, emit_color) with live bridge/state reflection.
Edit only:
- Source/ui/public/incremental/index_stage9.html (or stage8 copy promoted to stage9)
- Source/ui/public/incremental/js/stage9_ui.js
- Source/PluginEditor.cpp if new resource paths are needed
Requirements:
- bridge-safe control wiring through existing JUCE relay APIs
- no regression to existing Stage 8 controls
- status/readout text updates on valueChanged/propertiesChanged
Validation:
- run Stage 9 self-test command
- report tested/partially tested/not tested with artifact path(s)
```

`S9-T2-S9-T6`
```text
Use $skill_impl, $threejs, and $skill_docs.
Implement Stage 9 Tasks S9-T2 through S9-T6.
Goal: close emitter parity gaps (position/size/physics/animation/presets) in incremental UI.
Required parameter coverage:
- pos_coord_mode, pos_azimuth, pos_elevation, pos_distance, pos_x, pos_y, pos_z
- size_uniform, size_link, size_width, size_depth, size_height
- phys_mass, phys_drag, phys_elasticity, phys_gravity, phys_gravity_dir, phys_friction, phys_throw, phys_reset
- anim_speed
- emitter preset native lifecycle calls
Constraints:
- preserve viewport continuity invariants from Design/v3-ui-spec.md
- no allocations/locks in audio thread code paths
Validation:
- add/update Stage 9 self-test steps for every new control
- run UI PR gate and attach artifact paths
```

`S9-T7-S9-T8`
```text
Use $skill_impl and $skill_docs.
Complete Stage 9 automation and stage promotion tasks.
Deliverables:
- scripts/standalone-ui-selftest-stage9-mac.sh
- scripts/ui-pr-gate-mac.sh updated to stage9 default
- Source/PluginEditor.cpp default incremental route/tag updated to stage9
- resource provider mapping for stage9 index/js
Validation:
- run scripts/ui-pr-gate-mac.sh <path-to-standalone-app>
- publish status.tsv and self-test JSON paths
```

## Stage 10 - Renderer Rail Parity Completion

### Objective
Complete renderer-rail parity to final monitoring/system density with full spatial, room, speaker, and global physics controls.

### Task Checklist
- [x] `S10-T1` Expand renderer panel structure to match final sections: Scene, Master, Speakers, Spatialization, Room, Physics (Global), Visualization.
- [x] `S10-T2` Add missing renderer parameter controls and status updates:
  - speaker trims/delays (`rend_spk1_gain`..`rend_spk4_gain`, `rend_spk1_delay`..`rend_spk4_delay`)
  - distance refs (`rend_distance_ref`, `rend_distance_max`)
  - doppler/air (`rend_doppler_scale`, `rend_air_absorb`)
  - room depth (`rend_room_mix`, `rend_room_size`, `rend_room_damping`, `rend_room_er_only`)
  - global physics (`rend_phys_walls`, `rend_phys_pause`)
  - visualization (`rend_viz_trails`, `rend_viz_trail_len`, `rend_viz_vectors`, `rend_viz_grid`, `rend_viz_labels`)
- [x] `S10-T3` Ensure scene list and output telemetry remain coherent with renderer mode status semantics (`READY`, output layout/channel route text).
- [x] `S10-T4` Add Stage 10 self-test coverage for every new renderer control and status string.
- [x] `S10-T5` Promote gate default to Stage 10 self-test.

### Acceptance Checklist
- [x] Renderer controls are bridge-backed and reflected in status readouts.
- [x] Scene/output monitoring text remains consistent with processor snapshot fields.
- [x] Stage 10 self-test `ok=true`.
- [x] UI PR gate passes with Stage 10 as default.

### Expected Evidence
- `TestEvidence/locusq_incremental_stage10_selftest_<timestamp>.json`
- `TestEvidence/ui_pr_gate_<timestamp>/status.tsv`

### Captured Evidence (UTC 2026-02-20)
- `TestEvidence/locusq_build_incremental_stage10_20260220T173255Z.log`
- `TestEvidence/locusq_incremental_stage10_selftest_20260220T173332Z.json`
- `TestEvidence/ui_pr_gate_20260220T173332Z/status.tsv`
- `TestEvidence/locusq_incremental_stage10_resource_probe_20260220T173344Z.log`

### Codex Mega-Prompts

`S10-T1-S10-T2`
```text
Use $skill_impl and $skill_docs.
Implement Stage 10 renderer parity tasks in incremental UI.
Goal: bring renderer rail to full control density using real APVTS parameters.
Update:
- Source/ui/public/incremental/index_stage10.html
- Source/ui/public/incremental/js/stage10_ui.js
- Source/PluginEditor.cpp relay/attachment/resource mappings if needed
Control coverage must include:
- speaker gains/delays, distance ref/max, doppler scale, air absorb,
  room mix/size/damping/ER-only, walls/pause, viz toggles/trail length
Validation:
- add self-test assertions for set/value/status reflection on each new control
- run Stage 10 self-test and capture JSON evidence
```

`S10-T3-S10-T5`
```text
Use $skill_impl, $skill_docs, and $skill_plan if architecture rework is needed.
Complete Stage 10 telemetry coherence and gate promotion.
Requirements:
- renderer status lines reflect scene snapshot output layout/channel route
- no regression in mode-switch continuity/rail width/timeline visibility
- ui-pr-gate defaults to Stage 10 self-test
Provide:
- exact artifact paths
- tested/partially tested/not tested statement
```

## Stage 11 - Calibrate Workflow Parity Completion

### Objective
Complete calibrate operator flow parity: setup, capture lifecycle, routing re-detect, measure-again behavior, and profile lifecycle messaging.

### Task Checklist
- [x] `S11-T1` Complete calibrate setup density and readouts (speaker config, mic routing, SPK output mapping, test type/level).
- [x] `S11-T2` Harden capture flow state machine UI transitions:
  - `START MEASURE` -> `ABORT` while active
  - `ABORT` -> idle state reset
  - complete -> `MEASURE AGAIN` with deterministic reset behavior
- [x] `S11-T3` Ensure routing re-detect UX reflects native results (current map, auto-detected map, output channels).
- [x] `S11-T4` Ensure calibration messages/status strings are sanitized (ASCII-safe, no mojibake artifacts).
- [x] `S11-T5` Add explicit profile lifecycle status semantics (`NO PROFILE`, `MEASURING`, `PROFILE READY`) in both rail and viewport overlay.
- [x] `S11-T6` Add Stage 11 self-test coverage for start/abort/measure-again/re-detect and speaker progress row transitions.
- [x] `S11-T7` Promote gate default to Stage 11 self-test.

### Acceptance Checklist
- [ ] Manual checklist steps 1-11 for calibration flow are passable without hidden state.
- [x] `Measure Again` reliably resets progress/status and starts a new run.
- [x] Re-detect routing status updates on every invocation.
- [x] Stage 11 self-test `ok=true`.

### Expected Evidence
- `TestEvidence/locusq_incremental_stage11_selftest_<timestamp>.json`
- `TestEvidence/ui_pr_gate_<timestamp>/status.tsv`
- manual checklist artifacts under `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` follow-up entries

### Captured Evidence (UTC 2026-02-20)
- `TestEvidence/locusq_build_incremental_stage11_20260220T174725Z.log`
- `TestEvidence/locusq_incremental_stage11_selftest_20260220T174757Z.json`
- `TestEvidence/ui_pr_gate_20260220T174757Z/status.tsv`
- `TestEvidence/locusq_incremental_stage11_resource_probe_20260220T174808Z.log`

### Codex Mega-Prompts

`S11-T1-S11-T4`
```text
Use $skill_impl and $skill_docs.
Implement Stage 11 calibrate parity tasks.
Focus:
- full setup controls and statuses
- deterministic start/abort/measure-again behavior
- routing re-detect reflection from native bridge
- status/message sanitization to prevent mojibake
Files:
- Source/ui/public/incremental/index_stage11.html
- Source/ui/public/incremental/js/stage11_ui.js
- Source/PluginProcessor.cpp and Source/PluginEditor.cpp only if native/status payload gaps are found
Validation:
- Stage 11 self-test must verify steps for start->abort, abort->idle, measure-again reset, and re-detect status updates
```

`S11-T5-S11-T7`
```text
Use $skill_impl, $skill_docs, and $skill_plan if state contract changes are required.
Finish Stage 11 status semantics and gate promotion.
Requirements:
- status badges and viewport overlays follow v3 semantics
- self-test and ui-pr-gate move to stage11 default
- no regression to stage9/stage10 controls
Return:
- changed files
- test commands and artifact paths
```

## Stage 12 - Visual Polish And Primary Route Promotion

### Objective
Complete style parity pass and promote incremental route to the primary production UI entry path while preserving rollback safety.

### Task Checklist
- [x] `S12-T1` Apply final style-guide parity for typography, spacing rhythm, section density, and status/quality visual stability.
- [x] `S12-T2` Hide or gate debug-only diagnostics sections in non-selftest runs.
- [x] `S12-T3` Promote Stage 12 assets as primary incremental route (`/incremental/index.html` -> Stage 12 files; update title tag/version suffix).
- [x] `S12-T4` Keep previous stage assets available as fallback targets (no destructive cleanup until Stage 13 signoff).
- [x] `S12-T5` Create Stage 12 self-test with full coverage union from Stages 9-11.
- [x] `S12-T6` Promote UI PR gate default to Stage 12 self-test.

### Acceptance Checklist
- [x] Visual parity review passes against `Design/v3-style-guide.md`.
- [ ] Primary route loads Stage 12 assets in standalone and DAW hosts (standalone verified; manual DAW rerun deferred to Stage 13).
- [x] Stage 12 self-test `ok=true`.
- [x] Gate passes with Stage 12 default.

### Expected Evidence
- `TestEvidence/locusq_incremental_stage12_selftest_<timestamp>.json`
- `TestEvidence/ui_pr_gate_<timestamp>/status.tsv`
- updated resource-request probe logs for stage12 assets

### Captured Evidence (UTC 2026-02-20)
- `TestEvidence/locusq_build_incremental_stage12_20260220T175454Z.log`
- `TestEvidence/locusq_incremental_stage12_selftest_20260220T175530Z.json`
- `TestEvidence/ui_pr_gate_20260220T175530Z/status.tsv`
- `TestEvidence/locusq_incremental_stage12_resource_probe_20260220T175539Z.log`

### Codex Mega-Prompts

`S12-T1-S12-T3`
```text
Use $skill_impl, $skill_design, and $skill_docs.
Execute Stage 12 visual polish and primary-route promotion.
Goals:
- strict v3 style-guide parity
- promote Stage 12 as default incremental route in PluginEditor/resource provider
- preserve viewport continuity invariants and mode contracts
Files likely touched:
- Source/ui/public/incremental/index_stage12.html
- Source/ui/public/incremental/js/stage12_ui.js
- Source/PluginEditor.cpp
- scripts/ui-pr-gate-mac.sh
Validation:
- run Stage 12 self-test + ui-pr-gate
- include artifacts
```

`S12-T4-S12-T6`
```text
Use $skill_impl and $skill_docs.
Finalize Stage 12 fallback safety and automation gate migration.
Requirements:
- keep stage9-11 assets callable for rollback
- default gate and window tag clearly indicate stage12
- no regressions in existing pass criteria
Output:
- patch + command log + evidence paths
```

## Stage 13 - Final Acceptance Sweep And Closeout

### Objective
Run full acceptance sweep across automation and manual host checks, then close documentation/status surfaces for parity signoff.

### Task Checklist
- [x] `S13-T1` Run Stage 12 UI self-test and UI PR gate (default settings).
- [x] `S13-T2` Run targeted QA matrix for non-UI regressions required by parity promotion (smoke + acceptance suites + host edge).
- [x] `S13-T3` Run plugin host validation (`pluginval` and standalone smoke) on promoted artifacts.
- [ ] `S13-T4` Execute and record manual DAW UI acceptance checklist rerun.
- [x] `S13-T5` Update canonical closeout bundle per ADR-0005:
  - `status.json`
  - `README.md`
  - `CHANGELOG.md`
  - `TestEvidence/build-summary.md`
  - `TestEvidence/validation-trend.md`
- [ ] `S13-T6` Mark Stage 9-13 checkboxes complete in parity docs only after evidence is attached.

### Acceptance Checklist
- [x] Automated UI gate is PASS on promoted stage.
- [ ] Manual DAW checklist is PASS.
- [x] No new hard failures in non-UI acceptance suites.
- [x] Closeout docs freshness gate passes.

### Expected Evidence
- Latest UI self-test JSON + gate status TSV
- QA suite logs referenced in `TestEvidence/build-summary.md`
- `scripts/validate-docs-freshness.sh` PASS output

### Captured Evidence (UTC 2026-02-20)
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/status.tsv`
- `TestEvidence/locusq_incremental_stage12_selftest_20260220T180204Z.json`
- `TestEvidence/ui_pr_gate_20260220T180214Z/status.tsv`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_smoke_suite.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_phase_2_5_acceptance_suite.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_phase_2_6_acceptance_suite.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_44k1_256.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_48k512.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_48k1024.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_96k512.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/pluginval_strict5_skip_gui.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/standalone_open_smoke.log`
- `./scripts/validate-docs-freshness.sh` -> PASS (`0 warning(s)`)
- Manual DAW rerun remains pending in `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`

### Codex Mega-Prompts

`S13-T1-S13-T4`
```text
Use $skill_impl, $skill_test, and $skill_docs.
Execute Stage 13 acceptance sweep for LocusQ parity promotion.
Run:
- Stage 12 standalone self-test
- ui-pr-gate
- required QA scenario matrix
- pluginval + standalone smoke
- manual DAW checklist handoff/update
Deliver:
- concise pass/fail summary
- exact evidence artifact paths
- blocker list if any check fails
```

`S13-T5-S13-T6`
```text
Use $skill_docs and $skill_plan.
Perform Stage 13 documentation closeout and parity signoff.
Must update:
- status.json
- README.md
- CHANGELOG.md
- TestEvidence/build-summary.md
- TestEvidence/validation-trend.md
- Documentation/v3-ui-parity-checklist.md
- Documentation/v3-stage-9-plus-detailed-checklists.md
Run:
- ./scripts/validate-docs-freshness.sh
Return:
- tested/partially tested/not tested
- complete list of changed files
- remaining deferred items (if any)
```

## Stage 14 - Device Profiles, Comprehensive Review, and Release Decision

### Objective
Close remaining spec/implementation drift, validate laptop-speaker/mic/headphone usability, complete comprehensive architecture/code/design/QA review, and produce explicit release decision output.

### Task Checklist
- [x] `S14-T1` Align `.ideas` and ADR/invariant contracts to current implementation state and portable-device requirement.
- [x] `S14-T2` Refresh implementation traceability for Stage 12 renderer bindings and deferred parameter exposure.
- [ ] `S14-T3` Execute comprehensive review pass:
  - architecture review (`.ideas`, ADR, invariants, routing/state contracts)
  - code review (processor/editor/UI bridge/scripts)
  - design review (v3 visual/interaction parity in real host context)
  - QA review (manual + automated evidence coherence)
- [ ] `S14-T4` Complete manual DAW rerun with explicit rows for:
  - laptop speaker playback
  - headphone playback
  - built-in mic calibration route
  - external mic calibration route (if available)
- [ ] `S14-T5` Resolve deferred/runtime gap for `rend_phys_interact` (implement or retain explicit no-op/defer contract).
- [ ] `S14-T6` Decide and document release state:
  - `hold` (gates pending)
  - `draft-pre-release` (state lock)
  - `ga` (all gates complete)
- [ ] `S14-T7` Run docs freshness gate and synchronize closeout bundle if phase/release status changes.

### Acceptance Checklist
- [ ] No undocumented spec/implementation drifts remain.
- [ ] Manual DAW checklist includes portable device profile signoff rows.
- [ ] Comprehensive review findings are recorded and prioritized.
- [ ] Release decision and rationale are explicit in docs/status.
- [ ] Docs freshness gate passes.

### Expected Evidence
- `Documentation/stage14-review-release-checklist.md`
- updated `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`
- optional release evidence (tag/release notes/artifacts) when selected

### Codex Mega-Prompts

`S14-T1-S14-T2`
```text
Use $skill_plan, $skill_docs, $skill_dream, $skill_design, and $threejs.
Implement Stage 14 contract-alignment tasks.
Deliverables:
- .ideas contract updates (creative-brief, architecture, parameter-spec, plan)
- ADR/invariant updates for device profiles and deferred/no-op parameter contract
- traceability refresh for renderer control bindings and known open drifts
Run:
- ./scripts/validate-docs-freshness.sh
Return:
- changed files
- tested/partially tested/not tested
- explicit open drifts that remain
```

`S14-T3-S14-T7`
```text
Use $skill_docs and $skill_ship (plus $skill_test for validation reruns).
Execute Stage 14 comprehensive review + release decision flow.
Requirements:
- findings-first review output for architecture/code/design/QA
- manual DAW checklist updated with laptop speakers/mic/headphones rows
- release decision documented (hold/draft-pre-release/ga) with gate rationale
- closeout bundle updated if status/release state changes
Validation:
- run docs freshness gate
- include evidence artifact paths
```
