Title: LocusQ Implementation Traceability
Document Type: Traceability Matrix
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-19

# LocusQ Implementation Traceability

This document tracks end-to-end parameter wiring for implementation phases completed during `/impl`.

## Normative References

- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`
- `Documentation/adr/ADR-0001-documentation-governance.md`
- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`

## Scope

- Phase 2.4: Physics engine integration
- Phase 2.5: Room acoustics and advanced DSP integration
- Phase 2.6 (acceptance/tuning): Keyframe timeline editor interactions, preset persistence, and perf telemetry surfacing
- Phase 2.7b: Viewport emitter selection/movement path and calibration visualization state wiring
- Phase 2.7c: UI control wiring/state sync closure (tabs, toggles, dropdowns, text input persistence, transport/keyframe roundtrip)
- Phase 2.7e: Pluginval automation stability guard for mode-transition scene registration
- Phase 2.8: Output layout expansion groundwork (mono/stereo/quad bus-layout acceptance)
- Phase 2.9: QA/CI harness expansion for quad matrix + seeded pluginval stress
- Reference: `.ideas/plan.md`

## Phase 2.4 Parameter Mapping

| Parameter ID | APVTS Definition | DSP / Runtime Read Path | UI Relay / Attachment | Notes |
|---|---|---|---|---|
| `phys_enable` | `Source/PluginProcessor.cpp` (`createParameterLayout`) | `Source/PluginProcessor.cpp` (`publishEmitterState` -> `physicsEngine.setPhysicsEnabled`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Enables/disables per-emitter physics simulation |
| `phys_mass` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.setMass`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Integration mass |
| `phys_drag` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.setDrag`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Velocity damping |
| `phys_elasticity` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.setElasticity`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Collision bounce |
| `phys_gravity` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.setGravity`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Gravity magnitude |
| `phys_gravity_dir` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.setGravity`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Gravity direction mode |
| `phys_friction` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.setFriction`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Tangential damping |
| `phys_vel_x` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.requestThrow`) | Not yet bound in WebView relay | Throw X velocity |
| `phys_vel_y` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.requestThrow`) | Not yet bound in WebView relay | Throw Y velocity (mapped to world Z) |
| `phys_vel_z` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.requestThrow`) | Not yet bound in WebView relay | Throw Z velocity (mapped to world Y) |
| `phys_throw` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (edge-trigger -> `physicsEngine.requestThrow`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | One-shot throw trigger (`btn-throw`) |
| `phys_reset` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (edge-trigger -> `physicsEngine.requestReset`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | One-shot reset trigger (`btn-reset`) |
| `rend_phys_rate` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (Renderer writes SceneGraph global, Emitter reads and applies) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Global simulation tick rate |
| `rend_phys_walls` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (Renderer writes SceneGraph global, Emitter reads and applies) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Global wall-collision enable |
| `rend_phys_pause` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (Renderer writes SceneGraph global, Emitter reads and applies) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Global pause/freeze control |

## Phase 2.4 Acceptance Coverage

- Deterministic behavioral probe target: `qa/physics_probe_main.cpp`
  - Build target: `locusq_physics_probe` (`CMakeLists.txt`)
  - Closeout run artifact: `TestEvidence/locusq_phase_2_4_physics_probe_closeout.log`
  - Coverage: throw + bounce + decay, gravity, drag, elasticity, zero-g drift.
- Audio-domain physics-motion scenario: `qa/scenarios/locusq_24_physics_spatial_motion.json`
  - Closeout run artifact: `TestEvidence/locusq_24_physics_spatial_motion_closeout.log`
  - Coverage: non-finite safety, discontinuity guard, motion-driven flux-rate activity.
- Audio-domain zero-g scenario: `qa/scenarios/locusq_24_physics_zero_g_drift.json`
  - Closeout run artifact: `TestEvidence/locusq_24_physics_zero_g_drift_closeout.log`
  - Coverage: persistent post-throw motion in zero-g (gravity=0, drag=0) with discontinuity/non-finite guards.
- Phase 2.4 suite rollup: `qa/scenarios/locusq_phase_2_4_acceptance_suite.json`
  - Closeout run artifact: `TestEvidence/locusq_phase_2_4_acceptance_suite_closeout.log`
  - Outcome: `2 PASS / 0 WARN / 0 FAIL` (phase-pure scenarios: spatial-motion + zero-g drift).

## Phase 2.5 Parameter Mapping

| Parameter ID | APVTS Definition | DSP / Runtime Read Path | UI Relay / Attachment | Notes |
|---|---|---|---|---|
| `emit_spread` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState`) -> `Source/SpatialRenderer.h` (`SpreadProcessor::apply`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Focused-to-diffuse spread blend |
| `emit_directivity` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState`) -> `Source/SpatialRenderer.h` (`DirectivityFilter::apply`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Cardioid-like directional shaping |
| `emit_dir_azimuth` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState` computes `directivityAim`) -> `Source/SpatialRenderer.h` | Not yet bound in WebView relay | Directivity aim azimuth |
| `emit_dir_elevation` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState` computes `directivityAim`) -> `Source/SpatialRenderer.h` | Not yet bound in WebView relay | Directivity aim elevation |
| `rend_doppler` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setDopplerEnabled`) -> `Source/DopplerProcessor.h` | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Enables doppler processing |
| `rend_doppler_scale` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setDopplerScale`) -> `Source/DopplerProcessor.h` | Not yet bound in WebView relay | Doppler intensity |
| `rend_room_enable` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomEnabled`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Enables room acoustics chain |
| `rend_room_mix` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomMix`) -> `Source/EarlyReflections.h`, `Source/FDNReverb.h` | Not yet bound in WebView relay | Dry/wet mix |
| `rend_room_size` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomSize`) -> `Source/EarlyReflections.h`, `Source/FDNReverb.h` | Not yet bound in WebView relay | Scales delays/room model |
| `rend_room_damping` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomDamping`) -> `Source/EarlyReflections.h`, `Source/FDNReverb.h` | Not yet bound in WebView relay | High-frequency damping |
| `rend_room_er_only` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setEarlyReflectionsOnly`) -> `Source/FDNReverb.h` | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Early reflections only mode |
| `rend_quality` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setQualityTier`) -> `Source/EarlyReflections.h`, `Source/FDNReverb.h` | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Draft/final processing depth |

## Phase 2.6 Parameter Mapping (Acceptance/Tuning)

| Parameter ID | APVTS Definition | DSP / Runtime Read Path | UI Relay / Attachment | Notes |
|---|---|---|---|---|
| `anim_enable` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState`: gates internal timeline application) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Master enable for internal keyframe motion |
| `anim_mode` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState`: DAW automation vs internal timeline) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | `DAW` leaves position params host-driven, `Internal` evaluates `KeyframeTimeline` |
| `anim_loop` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState` -> `KeyframeTimeline::setLooping`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`, `Source/ui/public/index.html`) | Controls timeline wrap behavior |
| `anim_speed` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState` -> `KeyframeTimeline::setPlaybackRate`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Internal playback-rate multiplier |
| `anim_sync` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState` + `getTransportTimeSeconds`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`, `Source/ui/public/index.html`) | Transport-clock sync fallback to internal clock when unavailable |

## Phase 2.6 UI Bridge and Preset Mapping

| Control / API | Bridge Definition | Processor Entry Point | Persistence / Runtime Target | Notes |
|---|---|---|---|---|
| `locusqGetKeyframeTimeline` | `Source/PluginEditor.cpp` | `Source/PluginProcessor.cpp` (`getKeyframeTimelineForUI`) | `Source/KeyframeTimeline.cpp` serialized payload | UI pull of canonical timeline state |
| `locusqSetKeyframeTimeline` | `Source/PluginEditor.cpp` | `Source/PluginProcessor.cpp` (`setKeyframeTimelineFromUI`) | `Source/KeyframeTimeline.cpp` track replacement | UI commit path for add/move/delete/curve edit |
| `locusqSetTimelineTime` | `Source/PluginEditor.cpp` | `Source/PluginProcessor.cpp` (`setTimelineCurrentTimeFromUI`) | Runtime timeline clock | Scrub transport from UI |
| `locusqListEmitterPresets` | `Source/PluginEditor.cpp` | `Source/PluginProcessor.cpp` (`listEmitterPresetsFromUI`) | User data dir `LocusQ/Presets/*.json` | Enumerates saved presets |
| `locusqSaveEmitterPreset` | `Source/PluginEditor.cpp` | `Source/PluginProcessor.cpp` (`saveEmitterPresetFromUI`) | JSON write (`parameters` + `timeline`) | Captures animation + physics-related params + timeline |
| `locusqLoadEmitterPreset` | `Source/PluginEditor.cpp` | `Source/PluginProcessor.cpp` (`loadEmitterPresetFromUI`) | JSON read + APVTS restore + timeline apply | Loads preset and refreshes timeline UI |
| `perfBlockMs` / `perfEmitterMs` / `perfRendererMs` | `Source/ui/public/js/index.js` (`updateSceneState`) | `Source/PluginProcessor.cpp` (`getSceneStateJSON`) | EMA telemetry from `processBlock` | Acceptance/tuning perf visibility in UI |

## Phase 2.7b Viewport + Calibration Mapping

| Control / Payload | Bridge Definition | Processor Runtime Source | UI Runtime Consumer | Notes |
|---|---|---|---|---|
| Viewport emitter pick/select | `Source/ui/public/js/index.js` (`pickEmitterIntersection`, `setSelectedEmitter`) | `Source/PluginProcessor.cpp` (`getSceneStateJSON`, `localEmitterId`) | `Source/ui/public/js/index.js` (`updateSceneState`, `updateEmitterMeshes`) | Selection is now state-backed and local-emitter-aware. |
| Viewport emitter drag movement | `Source/ui/public/js/index.js` (`beginEmitterDrag`, `updateEmitterDrag`) | APVTS position params read in `Source/PluginProcessor.cpp` (`publishEmitterState`) | `Source/ui/public/js/index.js` (`setSliderScaledValue` writes `pos_azimuth`, `pos_elevation`, `pos_distance`) | Drag path now writes through APVTS parameter relays and updates scene snapshot deterministically. |
| Calibration speaker levels | `Source/PluginProcessor.cpp` (`getCalibrationStatus`, `speakerLevels`) | `CalibrationEngine::Progress` + state folding (`playing`/`recording`/`analyzing`/`complete`) | `Source/ui/public/js/index.js` (`getCalibrationSpeakerLevel`, animate loop meter targets) | Calibrate-mode meters/speaker accents are now native-status driven. |
| Calibration profile availability | `Source/PluginProcessor.cpp` (`getCalibrationStatus`, `profileValid`) | `SceneGraph::getRoomProfile()` | `Source/ui/public/js/index.js` (`applyCalibrationStatus`) | Room profile dot/label now reflects true profile publication state. |

## Phase 2.7c UI Control Wiring + State Sync Mapping

| Control / Parameter | JS Binding Path | Relay / Native Bridge | Persistence Path | Notes |
|---|---|---|---|---|
| Mode tabs (`Calibrate`/`Emitter`/`Renderer`) | `Source/ui/public/js/index.js` (`initUIBindings`) | `mode` combo relay/attachment (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`) | APVTS host state (`Source/PluginProcessor.cpp`) | Tab clicks now mutate processor mode deterministically. |
| Emitter label text input (`emit-label`) | `Source/ui/public/js/index.js` (`scheduleUiStateCommit`, `loadUiStateFromNative`) | Native UI bridge (`locusqGetUiState`, `locusqSetUiState`) in `Source/PluginEditor.cpp` | `locusq_ui_state_json` in plugin state (`Source/PluginProcessor.cpp`) | Label edits now roundtrip to scene snapshots and survive save/load. |
| Emitter/physics toggles (`size_link`, `phys_enable`, `emit_mute`, `emit_solo`) | `Source/ui/public/js/index.js` | Toggle relays/attachments (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`) | APVTS host state | Removed visual-only toggle behavior; all mapped to real parameters. |
| Renderer toggles (`rend_doppler`, `rend_air_absorb`, `rend_room_enable`, `rend_room_er_only`, `rend_phys_walls`, `rend_phys_pause`) | `Source/ui/public/js/index.js` | Toggle relays/attachments (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`) | APVTS host state | Renderer rail switches now update DSP/global physics controls. |
| Calibration dropdowns (`cal_spk_config`, `cal_mic_channel`, `cal_spk*_out`, `cal_test_type`) | `Source/ui/public/js/index.js` | Combo/slider relays/attachments (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`) | APVTS host state | Calibration control rail is state-backed and persistent. |
| Position/physics dropdowns (`pos_coord_mode`, `phys_gravity_dir`, `rend_distance_model`, `rend_phys_rate`) | `Source/ui/public/js/index.js` | Combo relays/attachments (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`) | APVTS host state | Dropdown actions now mutate processor/runtime behavior. |
| Transport/keyframe edits | `Source/ui/public/js/index.js` (`renderTimelineLanes`, `commitTimelineToNative`) | Native timeline bridge (`locusqGetKeyframeTimeline`, `locusqSetKeyframeTimeline`, `locusqSetTimelineTime`) | `locusq_timeline_json` state property (`Source/PluginProcessor.cpp`) | Play/stop/rewind and keyframe add/move/delete/curve are persisted. |
| Numeric value steppers (`azimuth`, `elevation`, `distance`, `size`, `gain`, `spread`, `directivity`, `cal_test_level`, `phys_*`, `master gain`, `anim speed`) | `Source/ui/public/js/index.js` (`bindValueStepper`) | Slider relays/attachments (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`) | APVTS host state | Value displays are now interactive controls, not static labels. |

## Phase 2.5 Core Files

- `Source/SpatialRenderer.h`
- `Source/DopplerProcessor.h`
- `Source/DirectivityFilter.h`
- `Source/SpreadProcessor.h`
- `Source/EarlyReflections.h`
- `Source/FDNReverb.h`
- `Source/PluginProcessor.cpp`
- `CMakeLists.txt`

## Phase 2.6 Core Files (Acceptance/Tuning)

- `Source/KeyframeTimeline.h`
- `Source/KeyframeTimeline.cpp`
- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`
- `Source/PhysicsEngine.h`
- `Source/PluginEditor.h`
- `Source/PluginEditor.cpp`
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`

## Phase 2.5 Acceptance Harness Coverage

- `qa/locusq_adapter.h` and `qa/locusq_adapter.cpp` now expose Phase 2.5 controls for room, doppler, directivity, spread, quality tier, and physics-velocity throw inputs.
- `qa/main.cpp` now profiles scenarios with `perf_*` invariants using real block timing/allocation metrics before invariant evaluation.
- Acceptance suite source: `qa/scenarios/locusq_phase_2_5_acceptance_suite.json` (per-scenario outputs under `qa_output/locusq_spatial/`).

## Phase 2.6 Validation Coverage (Acceptance/Tuning)

- `qa/locusq_adapter.h` and `qa/locusq_adapter.cpp` now expose `anim_*` controls (`anim_enable`, `anim_mode`, `anim_loop`, `anim_speed`, `anim_sync`) for spatial regression/scenario use.
- New scenario: `qa/scenarios/locusq_26_animation_internal_smoke.json` validates internal timeline enablement with loop/sync controls and deadline safety.
- Regression guard: `qa/scenarios/locusq_phase_2_5_acceptance_suite.json` is re-run from Phase 2.6 branch to keep Phase 2.5 hard-gate status stable.

## Phase 2.6 Acceptance Closeout Evidence

- Full-system CPU gate scenario: `qa/scenarios/locusq_26_full_system_cpu_draft.json`
  - Run artifact: `TestEvidence/locusq_26_full_system_cpu_draft_phase_2_6c_allocation_free.log`
  - Outcome: hard gates and allocation-free gate pass (`perf_meets_deadline=true`, `perf_avg_block_time_ms=0.304457`, `perf_p95_block_time_ms=0.318466`, `perf_allocation_free=true`).
- Host edge lifecycle scenario: `qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json`
  - Run artifacts:
    - `TestEvidence/locusq_26_host_edge_44k1_256_test_full_acceptance_rerun_20260219T044525Z.log`
    - `TestEvidence/locusq_26_host_edge_48k512_test_full_acceptance_rerun_20260219T044525Z.log`
    - `TestEvidence/locusq_26_host_edge_48k1024_test_full_acceptance_rerun_20260219T044525Z.log`
    - `TestEvidence/locusq_26_host_edge_96k512_test_full_acceptance_rerun_20260219T044525Z.log`
  - Outcome: pass across sample-rate/block-size matrix.
- Suite rollup: `qa/scenarios/locusq_phase_2_6_acceptance_suite.json`
  - Run artifact: `TestEvidence/locusq_phase_2_6_acceptance_suite_phase_2_6c_allocation_free_refresh.log`
  - Outcome: suite pass (`3 PASS / 0 WARN / 0 FAIL`).

## Phase 2.7e Pluginval Stability Guard Coverage

- Registration consistency guard:
  - `Source/PluginProcessor.cpp` (`syncSceneGraphRegistrationForMode`) now owns role-registration transitions and is called from both `prepareToPlay` and `processBlock`.
  - `Source/PluginProcessor.h` declares `syncSceneGraphRegistrationForMode` as private processor lifecycle guard state.
  - `Source/SceneGraph.h` (`unregisterEmitter`) now no-ops safely for already-free slots and only decrements active counters when occupied.
- Deterministic regression proof:
  - Repro before fix: `TestEvidence/pluginval_repro_seed_0x2a331c6.log` (`FAIL`, segfault under automation churn).
  - Crash capture: `TestEvidence/pluginval_lldb_btall_seed_0x2a331c6.log` (top frame in `SpatialRenderer::process`).
  - Deterministic pass after fix: `TestEvidence/pluginval_repro_seed_0x2a331c6_after_fix.log` (`PASS`).
- Stability probe: `TestEvidence/pluginval_postfix_stability_20260219T191544Z_status.tsv` (`10/10 PASS`).

## Phase 2.8 Output Layout Expansion Coverage

- Processor bus-layout validation:
  - `Source/PluginProcessor.cpp` (`isBusesLayoutSupported`) now accepts mono/stereo input with mono/stereo/quad output (`quadraphonic` and `discrete(4)`).
  - Existing stereo/mono render/downmix paths remain unchanged in `Source/SpatialRenderer.h` (`SpatialRenderer::process`).
- Explicit channel-order routing:
  - `Source/SpatialRenderer.h` now defines internal speaker order (`FL, FR, RR, RL`) separately from host quad output order (`FL, FR, RL, RR`) via `kQuadOutputSpeakerOrder = [0,1,3,2]`.
  - Quad rendering now uses deterministic output-channel mapping instead of direct index passthrough.
- Scene-state telemetry:
  - `Source/PluginProcessor.cpp` (`getSceneStateJSON`) now publishes `outputChannels`, `outputLayout`, `rendererOutputMode`, `rendererOutputChannels`, `rendererInternalSpeakers`, and `rendererQuadMap`.
  - `Source/ui/public/js/index.js` surfaces output-layout telemetry in renderer viewport info.
- Regression suites:
  - `qa/scenarios/locusq_phase_2_8_output_layout_mono_suite.json` (1-channel route checks).
  - `qa/scenarios/locusq_phase_2_8_output_layout_stereo_suite.json` (2-channel route checks).
  - `qa/scenarios/locusq_phase_2_8_output_layout_quad_suite.json` (4-channel route checks).
- Focused non-manual verification:
  - Build: `TestEvidence/locusq_build_phase_2_8_quad_layout_20260219T192849Z.log` (`PASS`).
  - Renderer 4-channel scenario: `TestEvidence/locusq_renderer_spatial_output_quad4_20260219T192849Z.log` (`PASS`).
  - Smoke suite 4-channel mode: `TestEvidence/locusq_smoke_suite_quad4_20260219T192849Z.log` (`PASS`, `4 PASS / 0 WARN / 0 FAIL`).
  - JS bridge syntax gate: `TestEvidence/locusq_ui_phase_2_8_layout_js_check_20260219T194047Z.log` (`PASS`).
  - Mapping build refresh: `TestEvidence/locusq_build_phase_2_8_layout_mapping_20260219T194047Z.log` (`PASS`).
  - Mono suite: `TestEvidence/locusq_phase_2_8_output_layout_mono_suite_20260219T194356Z.log` (`PASS`, `3 PASS / 0 WARN / 0 FAIL`).
  - Stereo suite: `TestEvidence/locusq_phase_2_8_output_layout_stereo_suite_20260219T194356Z.log` (`PASS`, `3 PASS / 0 WARN / 0 FAIL`).
  - Quad suite: `TestEvidence/locusq_phase_2_8_output_layout_quad_suite_20260219T194356Z.log` (`PASS`, `3 PASS / 0 WARN / 0 FAIL`).

## Phase 2.9 QA/CI Harness Expansion Coverage

- CI harness workflow expansion:
  - `.github/workflows/qa_harness.yml` (`qa-critical`) now runs explicit quad lanes (`--channels 4`) for:
    - `qa/scenarios/locusq_renderer_spatial_output.json`
    - `qa/scenarios/locusq_smoke_suite.json`
    - `qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json` (`2ch/4ch` matrix at host edge sample-rate/block-size pairs)
    - `qa/scenarios/locusq_26_full_system_cpu_draft.json` (`48k/512` in both `2ch` and `4ch`)
- Seeded pluginval stress lane:
  - `.github/workflows/qa_harness.yml` adds `qa-pluginval-seeded-stress` (macOS).
  - Builds `LocusQ_VST3`, installs `pluginval`, then executes strictness-5 seeded runs for:
    - `0x2a331c6`
    - `0x2a331c7`
    - `0x2a331c8`
    - `0x2a331c9`
    - `0x2a331ca`
  - Per-seed logs and `qa_output/pluginval_seeded_stress/status.tsv` are published as CI artifacts.
- Result gating:
  - Existing `result.json` pass/warn/skip gating remains active.
  - Seeded pluginval lane fails CI on any non-zero seed exit.

## Notes

- Room chain order in renderer: emitter spatialization -> `EarlyReflections` -> `FDNReverb` -> speaker delay/trim -> master gain/output.
- Phase 2.5 acceptance remains closed on hard gates; warning-level trends are tracked independently from the Phase 2.6c allocation-free closeout.
