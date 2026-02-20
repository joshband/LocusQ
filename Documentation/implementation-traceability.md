Title: LocusQ Implementation Traceability
Document Type: Traceability Matrix
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-20

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
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Scope

- Phase 2.4: Physics engine integration
- Phase 2.5: Room acoustics and advanced DSP integration
- Phase 2.6 (acceptance/tuning): Keyframe timeline editor interactions, preset persistence, and perf telemetry surfacing
- Phase 2.7b: Viewport emitter selection/movement path and calibration visualization state wiring
- Phase 2.7c: UI control wiring/state sync closure (tabs, toggles, dropdowns, text input persistence, transport/keyframe roundtrip)
- Phase 2.7e: Pluginval automation stability guard for mode-transition scene registration
- Phase 2.8: Output layout expansion groundwork (mono/stereo/quad bus-layout acceptance)
- Phase 2.9: QA/CI harness expansion for quad matrix + seeded pluginval stress
- Phase 2.10: Renderer CPU guardrails (activity culling + high-emitter budget protection)
- Phase 2.11: Preset/snapshot layout compatibility hardening (metadata versioning + migration checks)
- Phase 2.11b: Snapshot migration matrix expansion (mono/stereo/quad runtime suites + extended metadata emulation modes)
- Phase 2.12: Device-profile contract alignment and drift closure planning
- Reference: `.ideas/plan.md`

## Stage 14 Drift Ledger (Open)

| Parameter / Contract Surface | Implementation State | Documentation State | Next Action |
|---|---|---|---|
| `room_profile` | Runtime/internal status concept; not APVTS parameter | Previously documented as global parameter without APVTS caveat | Keep as internal runtime state with explicit non-APVTS note in `.ideas/parameter-spec.md` |
| `cal_state` | Runtime/internal status concept; not APVTS parameter | Previously documented as calibrate parameter without APVTS caveat | Keep as internal runtime state with explicit non-APVTS note in `.ideas/parameter-spec.md` |
| `emit_dir_azimuth` / `emit_dir_elevation` | DSP/runtime active | Not exposed in Stage 12 incremental control UI | Add Stage 14 binding or mark intentional defer with ADR-linked note |
| `phys_vel_x` / `phys_vel_y` / `phys_vel_z` | DSP/runtime active (throw request inputs) | Not exposed in Stage 12 incremental control UI | Add Stage 14 binding or mark intentional defer with ADR-linked note |

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
| `rend_phys_interact` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`processBlock` renderer global + `publishEmitterState` interaction force path) and `Source/PhysicsEngine.h` (`setInteractionForce`) | Bound in Stage 12 incremental UI (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/incremental/js/stage12_ui.js`) | Enables global soft inter-emitter interaction force for physics-enabled emitters |
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
| `rend_doppler_scale` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setDopplerScale`) -> `Source/DopplerProcessor.h` | Bound in Stage 12 incremental UI (`Source/ui/public/incremental/js/stage12_ui.js`) | Doppler intensity |
| `rend_room_enable` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomEnabled`) | Bound (`Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`) | Enables room acoustics chain |
| `rend_room_mix` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomMix`) -> `Source/EarlyReflections.h`, `Source/FDNReverb.h` | Bound in Stage 12 incremental UI (`Source/ui/public/incremental/js/stage12_ui.js`) | Dry/wet mix |
| `rend_room_size` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomSize`) -> `Source/EarlyReflections.h`, `Source/FDNReverb.h` | Bound in Stage 12 incremental UI (`Source/ui/public/incremental/js/stage12_ui.js`) | Scales delays/room model |
| `rend_room_damping` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`updateRendererParameters`) -> `Source/SpatialRenderer.h` (`setRoomDamping`) -> `Source/EarlyReflections.h`, `Source/FDNReverb.h` | Bound in Stage 12 incremental UI (`Source/ui/public/incremental/js/stage12_ui.js`) | High-frequency damping |
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

## Phase 2.7 Host Bridge Recovery (Module/Backend Compatibility)

| Surface | Prior State | Implemented Recovery | Evidence |
|---|---|---|---|
| Web UI bootstrap (`index.html`) | module script path (`type=\"module\"`) could fail in-host | switched to non-module script load chain (`check_native_interop.js` -> `js/juce/index.js` -> `js/index.js`) | `Source/ui/public/index.html` |
| JUCE frontend binding (`js/juce/index.js`) | ES module import/export path | converted to global `window.Juce` bridge export for in-host compatibility | `Source/ui/public/js/juce/index.js` |
| App binding (`js/index.js`) | direct ES module import of JUCE bridge | consumes global `window.Juce` with explicit guard | `Source/ui/public/js/index.js` |
| Editor backend selection (`PluginEditor.cpp`) | forced `webview2` backend on all platforms | platform-aware backend: `webview2` only on Windows; default backend on macOS/Linux | `Source/PluginEditor.cpp` |
| CMake backend flags (`CMakeLists.txt`) | `NEEDS_WEB_BROWSER=FALSE` on Apple | `NEEDS_WEB_BROWSER=TRUE` on Apple (`WKWebView` path enabled) | `CMakeLists.txt` |

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

## Phase 2.10 Renderer CPU Guardrail Coverage

- Renderer hot-path guardrails:
  - `Source/SpatialRenderer.h` now performs a two-pass render:
    - pass 1: select top-priority emitters by `emit_gain * distance attenuation` under a hard per-block budget (`MAX_RENDER_EMITTERS_PER_BLOCK = 8`)
    - pass 2: process selected emitters only, with near-silent activity culling (`ACTIVITY_PEAK_GATE_LINEAR`) before expensive spatial stages
  - Per-block guardrail stats are captured in atomics for non-audio telemetry reads:
    - `lastEligibleEmitterCount`
    - `lastProcessedEmitterCount`
    - `lastBudgetCulledEmitterCount`
    - `lastActivityCulledEmitterCount`
    - `lastGuardrailActive`
- Scene-state telemetry surfacing:
  - `Source/PluginProcessor.cpp` (`getSceneStateJSON`) now includes:
    - `rendererEligibleEmitters`
    - `rendererProcessedEmitters`
    - `rendererCulledBudget`
    - `rendererCulledActivity`
    - `rendererGuardrailActive`
- QA harness high-emitter coverage:
  - `qa/locusq_adapter.h` / `qa/locusq_adapter.cpp` expands `qa_emitter_instances` ceiling from `8` to `16`.
  - Existing scenario normalized values were remapped to preserve previous emitter counts:
    - `qa/scenarios/locusq_26_full_system_cpu_draft.json` (`8` emitters preserved)
    - `qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json` (`5` emitters preserved)
  - New focused guardrail scenario:
    - `qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json` (`16` emitters stress)
  - New rollup suite:
    - `qa/scenarios/locusq_phase_2_9_renderer_cpu_suite.json`
- Focused non-manual verification (UTC 2026-02-19):
  - Build refresh: `TestEvidence/locusq_build_phase_2_9_renderer_cpu_guard_20260219T194552Z.log` (`PASS`)
  - Baseline CPU gate (`8` emitters): `TestEvidence/locusq_26_full_system_cpu_draft_phase_2_9_guardrail_20260219T194552Z.log` (`PASS`, `perf_avg_block_time_ms=0.304505`, `perf_p95_block_time_ms=0.323633`, `perf_allocation_free=true`)
  - High-emitter guardrail (`16` emitters): `TestEvidence/locusq_29_renderer_guardrail_high_emitters_20260219T194552Z.log` (`PASS`, `perf_avg_block_time_ms=0.412833`, `perf_p95_block_time_ms=0.433221`, `perf_allocation_free=true`)
  - Guardrail suite rollup: `TestEvidence/locusq_phase_2_9_renderer_cpu_suite_20260219T194552Z.log` (`PASS`, `2 PASS / 0 WARN / 0 FAIL`)
  - Smoke regression: `TestEvidence/locusq_smoke_suite_phase_2_9_guardrail_20260219T194552Z.log` (`PASS`, `4 PASS / 0 WARN / 0 FAIL`)

## Phase 2.10b Renderer CPU Trend Expansion Coverage

- Trend scenario/suite additions:
  - `qa/scenarios/locusq_210b_renderer_guardrail_high_emitters_final_quality.json`
  - `qa/scenarios/locusq_phase_2_10b_renderer_cpu_trend_suite.json`
  - `locusq_phase_2_10b_renderer_cpu_trend_suite` rolls up:
    - `locusq_26_full_system_cpu_draft`
    - `locusq_29_renderer_guardrail_high_emitters`
    - `locusq_210b_renderer_guardrail_high_emitters_final_quality`
- CI matrix expansion:
  - `.github/workflows/qa_harness.yml` (`qa-critical`) now runs `phase_2_10b_renderer_cpu_trend_suite` at:
    - `48k/512` in `2ch` and `4ch`
    - `96k/512` in `2ch` and `4ch` (macOS lane)
- Focused non-manual verification (UTC 2026-02-19):
  - Build refresh: `TestEvidence/locusq_build_phase_2_10b_renderer_cpu_trend_20260219T202603Z.log` (`PASS`)
  - Draft high-emitter matrix (`16` emitters):
    - `TestEvidence/locusq_29_renderer_guardrail_high_emitters_48k512_ch2_phase_2_10b_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.067616`, `perf_p95_block_time_ms=0.073208`, `perf_allocation_free=true`)
    - `TestEvidence/locusq_29_renderer_guardrail_high_emitters_48k512_ch4_phase_2_10b_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.0717895`, `perf_p95_block_time_ms=0.0769191`, `perf_allocation_free=true`)
    - `TestEvidence/locusq_29_renderer_guardrail_high_emitters_96k512_ch2_phase_2_10b_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.0678394`, `perf_p95_block_time_ms=0.074208`, `perf_allocation_free=true`)
    - `TestEvidence/locusq_29_renderer_guardrail_high_emitters_96k512_ch4_phase_2_10b_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.0725433`, `perf_p95_block_time_ms=0.0794191`, `perf_allocation_free=true`)
  - Final-quality high-emitter matrix (`16` emitters):
    - `TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_48k512_ch2_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.068104`, `perf_p95_block_time_ms=0.074083`, `perf_total_allocations=0`)
    - `TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_48k512_ch4_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.0715641`, `perf_p95_block_time_ms=0.0775831`, `perf_total_allocations=0`)
    - `TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_96k512_ch2_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.0675089`, `perf_p95_block_time_ms=0.076041`, `perf_total_allocations=0`)
    - `TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_96k512_ch4_20260219T202524Z.log` (`PASS`, `perf_avg_block_time_ms=0.0717689`, `perf_p95_block_time_ms=0.0775861`, `perf_total_allocations=0`)
  - Trend suite matrix:
    - `TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_48k512_ch2_20260219T202524Z.log` (`PASS`, `3 PASS / 0 WARN / 0 FAIL`)
    - `TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_48k512_ch4_20260219T202524Z.log` (`PASS`, `3 PASS / 0 WARN / 0 FAIL`)
    - `TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_96k512_ch2_20260219T202524Z.log` (`PASS`, `3 PASS / 0 WARN / 0 FAIL`)
    - `TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_96k512_ch4_20260219T202524Z.log` (`PASS`, `3 PASS / 0 WARN / 0 FAIL`)

## Phase 2.11 Preset/Snapshot Layout Compatibility Coverage

- Host-snapshot metadata + migration:
  - `Source/PluginProcessor.cpp` (`getStateInformation`) now persists:
    - `locusq_snapshot_schema` (`locusq-state-v2`)
    - `locusq_output_layout`
    - `locusq_output_channels`
  - `Source/PluginProcessor.cpp` (`setStateInformation`) now applies `migrateSnapshotLayoutIfNeeded` after restore to remap calibration speaker outputs for legacy/mismatched layout snapshots.
- Preset schema hardening:
  - `Source/PluginProcessor.cpp` (`buildEmitterPresetLocked`) now writes `schema=locusq-emitter-preset-v2` with `layout` payload (`outputLayout`, `outputChannels`).
  - `Source/PluginProcessor.cpp` (`applyEmitterPresetLocked`) now accepts `v1` + `v2` schemas and validates optional layout metadata without breaking legacy preset loads.
- QA migration emulation + scenarios:
  - `qa/locusq_adapter.h` / `qa/locusq_adapter.cpp` adds `qa_snapshot_migration_mode` for state-roundtrip mutation modes:
    - `0.0` passthrough
    - `0.25` strip snapshot layout metadata (legacy emulation)
    - `0.5` force mono layout metadata
    - `0.75` force stereo layout metadata
    - `1.0` force quad layout metadata
  - New scenarios/suites:
    - `qa/scenarios/locusq_211_snapshot_migration_legacy_layout.json`
    - `qa/scenarios/locusq_211_snapshot_migration_layout_mismatch_stereo.json`
    - `qa/scenarios/locusq_phase_2_11_snapshot_migration_suite.json`
    - `qa/scenarios/locusq_211_snapshot_migration_layout_mismatch_mono_runtime.json`
    - `qa/scenarios/locusq_211_snapshot_migration_layout_mismatch_quad_runtime.json`
    - `qa/scenarios/locusq_phase_2_11b_snapshot_migration_mono_suite.json`
    - `qa/scenarios/locusq_phase_2_11b_snapshot_migration_stereo_suite.json`
    - `qa/scenarios/locusq_phase_2_11b_snapshot_migration_quad_suite.json`
- Focused non-manual verification (UTC 2026-02-19):
  - QA build: `TestEvidence/locusq_qa_build_phase_2_11_snapshot_migration_20260219T194406Z.log` (`PASS`)
  - Stereo migration suite: `TestEvidence/locusq_phase_2_11_snapshot_migration_suite_stereo_20260219T194406Z.log` (`PASS`, `2 PASS / 0 WARN / 0 FAIL`)
  - Quad legacy migration scenario: `TestEvidence/locusq_211_snapshot_migration_legacy_layout_quad4_20260219T194406Z.log` (`PASS`)
  - Matrix configure + QA build refresh:
    - `TestEvidence/locusq_configure_phase_2_11b_snapshot_migration_matrix_20260219T202551Z.log` (`PASS`)
    - `TestEvidence/locusq_qa_build_phase_2_11b_snapshot_migration_matrix_20260219T202551Z.log` (`PASS`)
  - Matrix suites:
    - `TestEvidence/locusq_phase_2_11b_snapshot_migration_mono_suite_20260219T202742Z.log` (`PASS`, `2 PASS / 0 WARN / 0 FAIL`)
    - `TestEvidence/locusq_phase_2_11b_snapshot_migration_stereo_suite_20260219T202742Z.log` (`PASS`, `2 PASS / 0 WARN / 0 FAIL`)
    - `TestEvidence/locusq_phase_2_11b_snapshot_migration_quad_suite_20260219T202742Z.log` (`PASS`, `2 PASS / 0 WARN / 0 FAIL`)

## Incremental Stage 2 UI Shell Mapping

Scope: focused incremental WebView shell aligned to `Design/v3-ui-spec.md` and `Design/v3-style-guide.md` with a minimal verified control set before reintroducing full-density panels.

| Parameter ID / Contract | Stage 2 UI Path | Bridge Path | Notes |
|---|---|---|---|
| `mode` | `Source/ui/public/incremental/index_stage2.html` (mode tabs) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getComboBoxState("mode")` + `setChoiceIndexSafe(...)` | Drives mode tab state, adaptive rail width class, timeline visibility, and per-mode scroll restore. |
| `rend_quality` | `Source/ui/public/incremental/index_stage2.html` (`#quality-badge`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getComboBoxState("rend_quality")` | Badge click toggles Draft/Final; live relay value keeps badge/status synchronized. |
| `size_link` | `Source/ui/public/incremental/index_stage2.html` (`#toggle-size-link`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getToggleState("size_link")` | Uses native checkbox input path to avoid overlay hit-target ambiguity from the legacy shell. |
| `size_uniform` | `Source/ui/public/incremental/index_stage2.html` (`#slider-size-uniform`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getSliderState("size_uniform")` + `setNormalisedValue(...)` | Slider readout displays normalized/scaled values from relay state. |
| `phys_enable` | `Source/ui/public/incremental/index_stage2.html` (`#toggle-phys-enable`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getToggleState("phys_enable")` | Emitter-mode status badge uses this state (`STABLE` vs `PHYSICS`). |
| `anim_enable` | `Source/ui/public/incremental/index_stage2.html` (`#toggle-anim-enable`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getToggleState("anim_enable")` | Incremental animation control-path verification in reduced UI context. |
| `anim_mode` | `Source/ui/public/incremental/index_stage2.html` (`#choice-anim-mode`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getComboBoxState("anim_mode")` + native choice fallback (`locusqGetChoiceItems`) | Dropdown keeps working even when relay choices arrive late. |
| `anim_loop` | `Source/ui/public/incremental/index_stage2.html` (`#toggle-anim-loop`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getToggleState("anim_loop")` | Included in Stage 2 to validate emitter timeline toggle path before full timeline editor reintegration. |
| `anim_sync` | `Source/ui/public/incremental/index_stage2.html` (`#toggle-anim-sync`) + `Source/ui/public/incremental/js/stage2_ui.js` | `Juce.getToggleState("anim_sync")` | Included in Stage 2 with direct checkbox binding path. |
| Viewport continuity invariant | `Source/ui/public/incremental/js/stage2_ui.js` (`createSceneApp`, `applyMode`) | C++ push hooks unchanged: `window.updateSceneState(...)`, `window.updateCalibrationStatus(...)` | One Three.js scene/camera/render loop owner; mode switches do not recreate scene or reset orbit state. |

## Incremental Stage 3 Emitter Audio Block Mapping

Scope: extend the verified Stage 2 shell with one fuller Emitter block while preserving direct relay binding behavior and viewport continuity.

| Parameter ID / Contract | Stage 3 UI Path | Bridge Path | Notes |
|---|---|---|---|
| `emit_mute` | `Source/ui/public/incremental/index_stage3.html` (`#toggle-emit-mute`) + `Source/ui/public/incremental/js/stage3_ui.js` | `Juce.getToggleState("emit_mute")` | Toggle is wired through the shared `bindToggle` path and updates Stage 3 status text. |
| `emit_gain` | `Source/ui/public/incremental/index_stage3.html` (`#slider-emit-gain`) + `Source/ui/public/incremental/js/stage3_ui.js` | `Juce.getSliderState("emit_gain")` + `setNormalisedValue(...)` | Slider uses normalized UI range while relay handles native scaled value mapping. |
| `phys_gravity_dir` | `Source/ui/public/incremental/index_stage3.html` (`#choice-phys-gravity-dir`) + `Source/ui/public/incremental/js/stage3_ui.js` | `Juce.getComboBoxState("phys_gravity_dir")` + native choice fetch fallback (`locusqGetChoiceItems`) | Dropdown keeps values even when relay choices arrive late. |
| Emitter Audio status reflection | `Source/ui/public/incremental/index_stage3.html` (`#status-emitter-audio`) + `Source/ui/public/incremental/js/stage3_ui.js` (`updateEmitterAudioStatus`) | Derived from relay states (`emit_mute`, `emit_gain`, `phys_gravity_dir`) | Status line updates on change events and heartbeat to show live bridge state. |
| Default incremental route | `Source/PluginEditor.cpp` (`incremental/index.html` resource mapping, window title tag) | BinaryData entries generated from `CMakeLists.txt` Stage 3 files | Stage 3 is now the default incremental shell (`[incremental-stage3]`) while Stage 1/2 paths remain available. |

## Incremental Stage 4 Emitter Audio Extended Mapping

Scope: extend Stage 3 emitter audio controls with additional working relay-bound controls while keeping the same verified binding path and continuity behavior.

| Parameter ID / Contract | Stage 4 UI Path | Bridge Path | Notes |
|---|---|---|---|
| `emit_solo` | `Source/ui/public/incremental/index_stage4.html` (`#toggle-emit-solo`) + `Source/ui/public/incremental/js/stage4_ui.js` | `Juce.getToggleState("emit_solo")` | Uses shared `bindToggle` path to keep parity with other working toggles. |
| `emit_spread` | `Source/ui/public/incremental/index_stage4.html` (`#slider-emit-spread`) + `Source/ui/public/incremental/js/stage4_ui.js` | `Juce.getSliderState("emit_spread")` + `setNormalisedValue(...)` | Adds audio spread control in the incremental shell with live status reflection. |
| `emit_directivity` | `Source/ui/public/incremental/index_stage4.html` (`#slider-emit-directivity`) + `Source/ui/public/incremental/js/stage4_ui.js` | `Juce.getSliderState("emit_directivity")` + `setNormalisedValue(...)` | Adds directivity control in the same status-backed block. |
| Emitter Audio status reflection (extended) | `Source/ui/public/incremental/index_stage4.html` (`#status-emitter-audio`) + `Source/ui/public/incremental/js/stage4_ui.js` (`updateEmitterAudioStatus`) | Derived from relay states (`emit_mute`, `emit_solo`, `emit_gain`, `emit_spread`, `emit_directivity`, `phys_gravity_dir`) | Status line now tracks all Stage 4 emitter-audio controls in one live readout. |
| Default incremental route | `Source/PluginEditor.cpp` (`incremental/index.html` resource mapping, window title tag) | BinaryData entries generated from `CMakeLists.txt` Stage 4 files | Stage 4 is now the default incremental shell (`[incremental-stage4]`) while Stage 1/2/3 remain available by route. |

## Incremental Stage 4 Self-Test Automation

Scope: add repeatable, non-coordinate Stage 4 UI interaction checks that run against live standalone WebView bindings.

| Artifact / Hook | Location | Purpose | Notes |
|---|---|---|---|
| Stage 4 self-test runner script | `scripts/standalone-ui-selftest-stage4-mac.sh` | Launch standalone in self-test mode, wait for pass/fail JSON, enforce exit code | Uses `LOCUSQ_UI_SELFTEST=1` and `LOCUSQ_UI_SELFTEST_RESULT_PATH` to enable deterministic automation runs. |
| Self-test mode URL gate | `Source/PluginEditor.cpp` (`isUiSelfTestEnabled`, initial URL query append) | Adds `selftest=1` query only when explicitly requested | Keeps normal UI sessions unchanged. |
| Self-test result export path | `Source/PluginEditor.cpp` (`getUiSelfTestResultFile`, timer polling for `window.__LQ_SELFTEST_RESULT__`) | Writes machine-readable result JSON from live WebView runtime | Polling finalizes when JS reports `status=pass|fail`. |
| Live control checks | `Source/ui/public/incremental/js/stage4_ui.js` (`runIncrementalStage4SelfTest`) | Verifies DOM-to-relay behavior for `emit_mute`, `emit_solo`, `emit_gain`, `emit_spread`, `emit_directivity`, `phys_gravity_dir` plus status reflection | Restores original values after checks to avoid persistent state drift. |

## Incremental Stage 4 UI PR Gate (Self-Test Default)

Scope: make the non-coordinate Stage 4 self-test the default automated UI gate and keep legacy coordinate smoke optional.

| Artifact / Hook | Location | Purpose | Notes |
|---|---|---|---|
| Stage 4 self-test argument handling | `scripts/standalone-ui-selftest-stage4-mac.sh` | Accept either `.app` bundle path or direct executable path | Allows one consistent invocation shape across manual and gate flows. |
| Default UI gate sequencing | `scripts/ui-pr-gate-mac.sh` | Runs `ui_stage4_selftest` by default, with smoke lane opt-in via `UI_PR_GATE_WITH_SMOKE=1` | Prevents false negatives from legacy coordinate smoke during incremental UI bring-up. |
| Gate evidence | `TestEvidence/ui_pr_gate_20260220T024215Z/status.tsv` | Records pass/fail state for self-test and optional lanes | Latest recorded run: `ui_stage4_selftest=PASS`, smoke/Appium skipped by default. |

## Incremental Stage 5 Renderer Core Mapping

Scope: extend Stage 4 with a focused renderer block that preserves direct relay bindings and status reflection discipline.

| Parameter ID / Contract | Stage 5 UI Path | Bridge Path | Notes |
|---|---|---|---|
| `rend_master_gain` | `Source/ui/public/incremental/index_stage5.html` (`#slider-rend-master-gain`) + `Source/ui/public/incremental/js/stage5_ui.js` | `Juce.getSliderState("rend_master_gain")` + `setNormalisedValue(...)` | Renderer master-gain path added as a normalized slider with live status output. |
| `rend_distance_model` | `Source/ui/public/incremental/index_stage5.html` (`#choice-rend-distance-model`) + `Source/ui/public/incremental/js/stage5_ui.js` | `Juce.getComboBoxState("rend_distance_model")` + native choice fetch fallback (`locusqGetChoiceItems`) | Uses canonical fallback labels and late-choice hydration path. |
| `rend_doppler` | `Source/ui/public/incremental/index_stage5.html` (`#toggle-rend-doppler`) + `Source/ui/public/incremental/js/stage5_ui.js` | `Juce.getToggleState("rend_doppler")` | Toggle is wired with the same proven `bindToggle` path as emitter controls. |
| `rend_room_enable` | `Source/ui/public/incremental/index_stage5.html` (`#toggle-rend-room-enable`) + `Source/ui/public/incremental/js/stage5_ui.js` | `Juce.getToggleState("rend_room_enable")` | Room enable route is reflected in renderer status and heartbeat snapshot. |
| `rend_phys_rate` | `Source/ui/public/incremental/index_stage5.html` (`#choice-rend-phys-rate`) + `Source/ui/public/incremental/js/stage5_ui.js` | `Juce.getComboBoxState("rend_phys_rate")` + native choice fetch fallback (`locusqGetChoiceItems`) | Choice list remains stable even when relay properties arrive late. |
| `rend_viz_mode` | `Source/ui/public/incremental/index_stage5.html` (`#choice-rend-viz-mode`) + `Source/ui/public/incremental/js/stage5_ui.js` | `Juce.getComboBoxState("rend_viz_mode")` + native choice fetch fallback (`locusqGetChoiceItems`) | Completes focused renderer control set for Stage 5. |
| Renderer status reflection | `Source/ui/public/incremental/index_stage5.html` (`#status-renderer-core`) + `Source/ui/public/incremental/js/stage5_ui.js` (`updateRendererCoreStatus`) | Derived from relay states (`rend_master_gain`, `rend_distance_model`, `rend_doppler`, `rend_room_enable`, `rend_phys_rate`, `rend_viz_mode`) | Live status now mirrors the full Stage 5 renderer control subset. |
| Default incremental route | `Source/PluginEditor.cpp` (`incremental/index.html` resource mapping, window title tag) | BinaryData entries generated from `CMakeLists.txt` Stage 5 files | Stage 5 is now the default incremental shell (`[incremental-stage5]`) while Stage 2/3/4 routes remain available. |

## Incremental Stage 5 Self-Test and Gate

Scope: extend deterministic non-coordinate automation to cover the Stage 5 renderer subset and make Stage 5 the gate default.

| Artifact / Hook | Location | Purpose | Notes |
|---|---|---|---|
| Stage 5 self-test runner script | `scripts/standalone-ui-selftest-stage5-mac.sh` | Launch standalone in self-test mode and enforce pass/fail JSON | Accepts `.app` or direct executable path. |
| Stage 5 self-test runtime | `Source/ui/public/incremental/js/stage5_ui.js` (`runIncrementalStage5SelfTest`) | Verifies emitter + renderer control paths and status reflection in one run | Adds checks for `rend_master_gain`, `rend_distance_model`, `rend_doppler`, `rend_room_enable`, `rend_phys_rate`, `rend_viz_mode`. |
| UI PR gate default | `scripts/ui-pr-gate-mac.sh` | Runs `ui_stage5_selftest` by default, with smoke/Appium optional | Latest gate evidence: `TestEvidence/ui_pr_gate_20260220T025111Z/status.tsv` (`PASS`). |

## Incremental Stage 6 Calibrate Core Mapping

Scope: extend Stage 5 with a focused calibrate control block while preserving the same direct relay binding and status reflection pattern.

| Parameter ID / Contract | Stage 6 UI Path | Bridge Path | Notes |
|---|---|---|---|
| `cal_spk_config` | `Source/ui/public/incremental/index_stage6.html` (`#choice-cal-spk-config`) + `Source/ui/public/incremental/js/stage6_ui.js` | `Juce.getComboBoxState("cal_spk_config")` + native choice fetch fallback (`locusqGetChoiceItems`) | Uses canonical fallback choices (`4x Mono`, `2x Stereo`) with late-choice hydration. |
| `cal_mic_channel` | `Source/ui/public/incremental/index_stage6.html` (`#slider-cal-mic-channel`) + `Source/ui/public/incremental/js/stage6_ui.js` | `Juce.getSliderState("cal_mic_channel")` + `setNormalisedValue(...)` | Integer-backed mic-channel route is exposed via normalized slider binding. |
| `cal_test_level` | `Source/ui/public/incremental/index_stage6.html` (`#slider-cal-test-level`) + `Source/ui/public/incremental/js/stage6_ui.js` | `Juce.getSliderState("cal_test_level")` + `setNormalisedValue(...)` | Calibrate test-level path added as a focused slider in the incremental shell. |
| `cal_test_type` | `Source/ui/public/incremental/index_stage6.html` (`#choice-cal-test-type`) + `Source/ui/public/incremental/js/stage6_ui.js` | `Juce.getComboBoxState("cal_test_type")` + native choice fetch fallback (`locusqGetChoiceItems`) | Uses fallback choices (`Sweep`, `Pink`, `White`, `Impulse`) with relay-first behavior. |
| Calibrate status reflection | `Source/ui/public/incremental/index_stage6.html` (`#status-calibrate-core`) + `Source/ui/public/incremental/js/stage6_ui.js` (`updateCalibrateCoreStatus`) | Derived from relay states (`cal_spk_config`, `cal_mic_channel`, `cal_test_level`, `cal_test_type`) | Live status mirrors Stage 6 calibrate controls in one compact line. |
| Default incremental route | `Source/PluginEditor.cpp` (`incremental/index.html` resource mapping, window title tag) | BinaryData entries generated from `CMakeLists.txt` Stage 6 files | Stage 6 is now the default incremental shell (`[incremental-stage6]`) while Stage 2/3/4/5 remain available by route. |

## Incremental Stage 6 Self-Test and Gate

Scope: extend deterministic automation coverage to include the Stage 6 calibrate subset and promote Stage 6 as the default UI gate.

| Artifact / Hook | Location | Purpose | Notes |
|---|---|---|---|
| Stage 6 self-test runner script | `scripts/standalone-ui-selftest-stage6-mac.sh` | Launch standalone in self-test mode and enforce pass/fail JSON | Accepts `.app` or direct executable path. |
| Stage 6 self-test runtime | `Source/ui/public/incremental/js/stage6_ui.js` (`runIncrementalStage6SelfTest`) | Verifies calibrate + emitter + renderer control paths and status reflection | Adds checks for `cal_spk_config`, `cal_mic_channel`, `cal_test_level`, `cal_test_type` on top of Stage 5 coverage. |
| UI PR gate default | `scripts/ui-pr-gate-mac.sh` | Runs `ui_stage6_selftest` by default, with smoke/Appium optional | Latest gate evidence: `TestEvidence/ui_pr_gate_20260220T030133Z/status.tsv` (`PASS`). |

## Incremental Stage 7 Calibrate Speaker Output Routing Mapping

Scope: extend Stage 6 with explicit calibrate speaker output routing sliders while preserving the same direct relay binding and status reflection pattern.

| Parameter ID / Contract | Stage 7 UI Path | Bridge Path | Notes |
|---|---|---|---|
| `cal_spk1_out` | `Source/ui/public/incremental/index_stage7.html` (`#slider-cal-spk1-out`) + `Source/ui/public/incremental/js/stage7_ui.js` | `Juce.getSliderState("cal_spk1_out")` + `setNormalisedValue(...)` | Adds normalized slider routing for speaker output 1. |
| `cal_spk2_out` | `Source/ui/public/incremental/index_stage7.html` (`#slider-cal-spk2-out`) + `Source/ui/public/incremental/js/stage7_ui.js` | `Juce.getSliderState("cal_spk2_out")` + `setNormalisedValue(...)` | Adds normalized slider routing for speaker output 2. |
| `cal_spk3_out` | `Source/ui/public/incremental/index_stage7.html` (`#slider-cal-spk3-out`) + `Source/ui/public/incremental/js/stage7_ui.js` | `Juce.getSliderState("cal_spk3_out")` + `setNormalisedValue(...)` | Adds normalized slider routing for speaker output 3. |
| `cal_spk4_out` | `Source/ui/public/incremental/index_stage7.html` (`#slider-cal-spk4-out`) + `Source/ui/public/incremental/js/stage7_ui.js` | `Juce.getSliderState("cal_spk4_out")` + `setNormalisedValue(...)` | Adds normalized slider routing for speaker output 4. |
| Calibrate status reflection | `Source/ui/public/incremental/index_stage7.html` (`#status-calibrate-core`) + `Source/ui/public/incremental/js/stage7_ui.js` (`updateCalibrateCoreStatus`) | Derived from relay states (`cal_spk_config`, `cal_mic_channel`, `cal_spk1_out`, `cal_spk2_out`, `cal_spk3_out`, `cal_spk4_out`, `cal_test_level`, `cal_test_type`) | Live status now includes `Out ch1/ch2/ch3/ch4` routing values. |
| Default incremental route | `Source/PluginEditor.cpp` (`incremental/index.html` resource mapping, window title tag) | BinaryData entries generated from `CMakeLists.txt` Stage 7 files | Stage 7 is now the default incremental shell (`[incremental-stage7]`); Stage 2-6 routes remain addressable. |

## Incremental Stage 7 Self-Test and Gate

Scope: extend deterministic automation coverage to include the Stage 7 speaker output routing subset and promote Stage 7 as the default UI gate.

| Artifact / Hook | Location | Purpose | Notes |
|---|---|---|---|
| Stage 7 self-test runner script | `scripts/standalone-ui-selftest-stage7-mac.sh` | Launch standalone in self-test mode and enforce pass/fail JSON | Accepts `.app` or direct executable path. |
| Stage 7 self-test runtime | `Source/ui/public/incremental/js/stage7_ui.js` (`runIncrementalStage7SelfTest`) | Verifies calibrate + emitter + renderer control paths and status reflection | Adds checks for `cal_spk1_out`, `cal_spk2_out`, `cal_spk3_out`, `cal_spk4_out` on top of Stage 6 coverage. |
| UI PR gate default | `scripts/ui-pr-gate-mac.sh` | Runs `ui_stage7_selftest` by default, with smoke/Appium optional | Latest gate evidence: `TestEvidence/ui_pr_gate_20260220T031226Z/status.tsv` (`PASS`). |

## Incremental Stage 8 Calibrate Capture/Progress Mapping

Scope: extend Stage 7 with explicit calibrate capture/progress controls and status reflection while preserving direct relay bindings and the persistent viewport shell.

| Parameter ID / Contract | Stage 8 UI Path | Bridge Path | Notes |
|---|---|---|---|
| Capture start/abort action | `Source/ui/public/incremental/index_stage8.html` (`#btn-cal-measure`) + `Source/ui/public/incremental/js/stage8_ui.js` (`handleCalibrationMeasureClick`) | Native functions `locusqStartCalibration` / `locusqAbortCalibration` via `Juce.getNativeFunction(...)` | Button text/state follows calibration runtime (`START MEASURE`, `ABORT`, `MEASURE AGAIN`). |
| Capture progress reflection | `Source/ui/public/incremental/index_stage8.html` (`#status-cal-progress`, `#cal-progress-bar`) + `Source/ui/public/incremental/js/stage8_ui.js` (`updateCalibrateCaptureStatus`) | `window.updateCalibrationStatus(...)` payload from processor | Reflects `state` + `overallPercent` in text and progress bar width. |
| Capture message reflection | `Source/ui/public/incremental/index_stage8.html` (`#status-cal-message`) + `Source/ui/public/incremental/js/stage8_ui.js` (`updateCalibrateCaptureStatus`) | `window.updateCalibrationStatus(...)` payload (`message`) | Shows live phase guidance (`playing`, `recording`, `analyzing`, complete). |
| Per-speaker calibration status rows | `Source/ui/public/incremental/index_stage8.html` (`#cal-spk1-status..#cal-spk4-status`, dots) + `Source/ui/public/incremental/js/stage8_ui.js` (`setCalSpeakerRow`) | `window.updateCalibrationStatus(...)` payload (`currentSpeaker`, `completedSpeakers`, `playPercent`, `recordPercent`) | Adds row-by-row status semantics (`Not measured`, active phase, `Measured`). |
| Default incremental route | `Source/PluginEditor.cpp` (`incremental/index.html` resource mapping, window title tag) | BinaryData entries generated from `CMakeLists.txt` Stage 8 files | Stage 8 is now the default incremental shell (`[incremental-stage8]`); Stage 2-7 routes remain addressable. |

## Incremental Stage 8 Self-Test and Gate

Scope: extend deterministic automation coverage with capture/progress status checks and promote Stage 8 as the default UI gate.

| Artifact / Hook | Location | Purpose | Notes |
|---|---|---|---|
| Stage 8 self-test runner script | `scripts/standalone-ui-selftest-stage8-mac.sh` | Launch standalone in self-test mode and enforce pass/fail JSON | Accepts `.app` or direct executable path. |
| Stage 8 self-test runtime | `Source/ui/public/incremental/js/stage8_ui.js` (`runIncrementalStage8SelfTest`) | Verifies calibrate + emitter + renderer controls and capture/progress status reflection | Adds checks for `cal_capture_running_status` and `cal_capture_complete_status` in addition to Stage 7 coverage. |
| UI PR gate default | `scripts/ui-pr-gate-mac.sh` | Runs `ui_stage8_selftest` by default, with smoke/Appium optional | Latest gate evidence: `TestEvidence/ui_pr_gate_20260220T033031Z/status.tsv` (`PASS`). |

## Notes

- Room chain order in renderer: emitter spatialization -> `EarlyReflections` -> `FDNReverb` -> speaker delay/trim -> master gain/output.
- Phase 2.5 acceptance remains closed on hard gates; warning-level trends are tracked independently from the Phase 2.6c allocation-free closeout.
