Title: LocusQ Implementation Traceability
Document Type: Traceability Matrix
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-26

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
- Phase 2.13: Visualization transport contract hardening (sequence guard + stale fallback + smoothing)
- Phase 2.14: Production viewport multi-emitter + listener/speaker overlays (BL-015/BL-014), RMS telemetry overlays (BL-008), and visual overlays (BL-006/BL-007)
- BL-011 Slice 2: CLAP runtime lifecycle telemetry surfaced to scene snapshots + optional production self-test lane
- BL-026 Slice A: CALIBRATE topology profile selector + alias dictionary wiring refresh
- BL-026 Slice C: CALIBRATE profile library tuple-scoped save/recall contract
- BL-026 Slice D+E: CALIBRATE deterministic diagnostics cards + host resize contract hardening
- BL-028 Slice B1: Spatial output matrix deterministic QA lane scaffold (scenario suite + lane runner + acceptance parity artifacts)
- BL-028 Slice B2: Spatial output matrix QA reliability hardening (replay controls + hash divergence gate + deterministic taxonomy)
- BL-028 Slice C1: Native spatial output matrix state publication in scene-state contract (requested/active/rule/fallback/reason/status)
- BL-029 Slice B1: Renderer audition binding resolver metadata bridge for additive cross-mode control telemetry
- BL-029 Slice G6: Cinematic reactive preset language pre-code design/plan contract
- BL-029 Slice Z3: Reactive UI/QA consolidation for defensive cloud/reactive consumption and fallback robustness
- BL-030 Slice B: Device rerun matrix contract for DEV-01..DEV-06 release-governance lane
- BL-030 Slice C: CI release-governance workflow for automated gate lanes on tag/manual dispatch
- BL-030 Slice D: First release-governance dry-run baseline execution + blocked-gate capture contract
- BL-031 Slice D: Deterministic tempo-ramp token scheduler QA lane (scenario + lane script + monotonicity evidence)
- BL-032 Slice A: Processor/editor modularization boundary map contract for parallel no-overlap extraction
- BL-032 Slice B: Processor native extraction of non-RT core/bridge/shared helper responsibilities
- BL-032 Slice C1: Editor shell/webview extraction of runtime lifecycle and resource-provider orchestration
- BL-033 Slice D1: Headphone calibration core QA lane scaffold (scenario contract + lane script + docs parity artifacts)
- BL-033 Slice D2: Headphone calibration core QA closeout expansion (diagnostics consistency + replay hash determinism + strict artifact schema completeness)
- BL-033 Slice Z1: Owner integration replay and promotion decision packet (A1/B1/D1 handoff reconciliation)
- BL-033 Slice Z8: Owner replay closeout update (Z2/Z5/Z6/Z7 unblock reconciliation + status promotion)
- BL-033 Slice Z11: Owner integration update after Z9/Z10 reconciliation (Done-candidate promotion decision)
- Reference: `.ideas/plan.md`

## Stage 14 Drift Ledger (Open)

| Parameter / Contract Surface | Implementation State | Documentation State | Next Action |
|---|---|---|---|
| `room_profile` | Runtime/internal status concept; not APVTS parameter | Previously documented as global parameter without APVTS caveat | Keep as internal runtime state with explicit non-APVTS note in `.ideas/parameter-spec.md` |
| `cal_state` | Runtime/internal status concept; not APVTS parameter | Previously documented as calibrate parameter without APVTS caveat | Keep as internal runtime state with explicit non-APVTS note in `.ideas/parameter-spec.md` |
| `emit_dir_azimuth` / `emit_dir_elevation` | DSP/runtime active; relay/attachment/UI now bound | Exposed in production UI (`val-dir-azimuth`, `val-dir-elevation`) | Resolved in Stage 15 (`dirAzimuthRelay`/`dirElevationRelay`, attachments, `sliderStates` + `bindValueStepper`) |
| `phys_vel_x` / `phys_vel_y` / `phys_vel_z` | DSP/runtime active (throw request inputs); relay/attachment/UI now bound | Exposed in production UI (`val-vel-x`, `val-vel-y`, `val-vel-z`) | Resolved in Stage 15 (`physVelXRelay`/`physVelYRelay`/`physVelZRelay`, attachments, `sliderStates` + `bindValueStepper`) |

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
| `phys_vel_x` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.requestThrow`) | Bound (`Source/PluginEditor.h`: `physVelXRelay`; `Source/PluginEditor.cpp`: `physVelXAttachment`; `Source/ui/public/js/index.js`: `sliderStates.phys_vel_x` + `bindValueStepper("val-vel-x", ...)`; `Source/ui/public/index.html`: `#val-vel-x`) | Throw X velocity |
| `phys_vel_y` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.requestThrow`) | Bound (`Source/PluginEditor.h`: `physVelYRelay`; `Source/PluginEditor.cpp`: `physVelYAttachment`; `Source/ui/public/js/index.js`: `sliderStates.phys_vel_y` + `bindValueStepper("val-vel-y", ...)`; `Source/ui/public/index.html`: `#val-vel-y`) | Throw Y velocity (mapped to world Z) |
| `phys_vel_z` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`physicsEngine.requestThrow`) | Bound (`Source/PluginEditor.h`: `physVelZRelay`; `Source/PluginEditor.cpp`: `physVelZAttachment`; `Source/ui/public/js/index.js`: `sliderStates.phys_vel_z` + `bindValueStepper("val-vel-z", ...)`; `Source/ui/public/index.html`: `#val-vel-z`) | Throw Z velocity (mapped to world Y) |
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
| `emit_dir_azimuth` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState` computes `directivityAim`) -> `Source/SpatialRenderer.h` | Bound (`Source/PluginEditor.h`: `dirAzimuthRelay`; `Source/PluginEditor.cpp`: `dirAzimuthAttachment`; `Source/ui/public/js/index.js`: `sliderStates.emit_dir_azimuth` + `bindValueStepper("val-dir-azimuth", ...)`; `Source/ui/public/index.html`: `#val-dir-azimuth`) | Directivity aim azimuth |
| `emit_dir_elevation` | `Source/PluginProcessor.cpp` | `Source/PluginProcessor.cpp` (`publishEmitterState` computes `directivityAim`) -> `Source/SpatialRenderer.h` | Bound (`Source/PluginEditor.h`: `dirElevationRelay`; `Source/PluginEditor.cpp`: `dirElevationAttachment`; `Source/ui/public/js/index.js`: `sliderStates.emit_dir_elevation` + `bindValueStepper("val-dir-elevation", ...)`; `Source/ui/public/index.html`: `#val-dir-elevation`) | Directivity aim elevation |
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

## Phase 2.13 Visualization Transport Contract Hardening (BL-016)

| Contract Surface | Runtime Publisher | UI Consumer | Behavior |
|---|---|---|---|
| Snapshot transport metadata (`snapshotSchema`, `snapshotSeq`, `snapshotPublishedAtUtcMs`, `snapshotCadenceHz`, `snapshotStaleAfterMs`) | `Source/PluginProcessor.cpp` (`getSceneStateJSON`) | `Source/ui/public/js/index.js` (`window.updateSceneState`) | Native snapshot payload now carries deterministic sequence/cadence/stale metadata for visualization transport. |
| Sequence guard | `Source/PluginProcessor.cpp` (`sceneSnapshotSequence`) | `Source/ui/public/js/index.js` (`parseSnapshotSequence`, guard in `updateSceneState`) | Out-of-order snapshots are rejected (`incomingSeq <= lastAcceptedSeq`). |
| Stale snapshot fallback | `Source/PluginProcessor.cpp` (`snapshotStaleAfterMs`) | `Source/ui/public/js/index.js` (`updateSceneTransportHealth`, `applySceneStatusBadge`) | UI enters warning state (`STALE SNAPSHOT`) and reduced visual confidence when snapshots exceed timeout budget. |
| Snapshot smoothing | `Source/PluginProcessor.cpp` (`snapshotCadenceHz`) | `Source/ui/public/js/index.js` (`getSceneSmoothingAlpha`, `animate`, `updateEmitterMeshes`) | Emitters interpolate toward latest snapshot targets while preserving native snapshot authority. |
| Contract specification | `Documentation/scene-state-contract.md` | `Documentation/invariants.md` | BL-016 transport contract is now documented as an explicit interface contract. |

## Phase 2.14 Viewport Multi-Emitter + Overlay Expansion (BL-015/BL-014/BL-008/BL-006/BL-007)

| Feature Surface | Runtime Publisher | UI Consumer | Behavior |
|---|---|---|---|
| All-emitter realtime rendering + selection styling (BL-015) | `Source/PluginProcessor.cpp` (`getSceneStateJSON`, emitter array publication) | `Source/ui/public/js/index.js` (`updateEmitterMeshes`) | All emitters in snapshot are rendered continuously; selected emitter keeps focus styling while non-selected emitters remain visible with transparent/dashed treatment. |
| Per-emitter direction + energy overlays (BL-015/BL-014) | `Source/PluginProcessor.cpp` (per-emitter `directivity`, `aimX/Y/Z`, `rms`, `rmsDb`) | `Source/ui/public/js/index.js` (`setArrowFromVector`, emitter aim arrows, emitter energy rings) | Direction and RMS overlays are driven from native snapshot fields with clamped visual response. |
| Per-speaker RMS telemetry overlays (BL-008) | `Source/PluginProcessor.cpp` (`speakerRms`, `speakers[].rms`) | `Source/ui/public/js/index.js` (`updateSpeakerTargetsFromScene`, animate speaker meter/ring response) | Speaker meter/ring visuals are audio-reactive and ordered by per-speaker RMS telemetry from native snapshots. |
| Listener/speaker room overlays (BL-014) | `Source/PluginProcessor.cpp` (`roomProfileValid`, `roomDimensions`, `listener`, `speakerRms`, `speakers`) | `Source/ui/public/js/index.js` (`updateSpeakerTargetsFromScene`, `updateListenerTargetFromScene`, speaker/listener energy visuals) | Viewport listener/headphone and speaker overlays track scene telemetry with smoothing and safe defaults. |
| Trail/vector toggles + trail-length control (BL-006/BL-007) | APVTS parameters (`rend_viz_trails`, `rend_viz_vectors`, `rend_viz_trail_len`) surfaced through relay state | `Source/ui/public/js/index.js` (toggle bindings + animate visibility gates) | Motion trails and velocity vectors are controlled by renderer UI toggles and remain visual-only overlays. |
| Steam headphone runtime diagnostics (BL-009 closeout support) | `Source/SpatialRenderer.h` (`SteamInitStage`, init failure/error tracking) + `Source/PluginProcessor.cpp` (`getSceneStateJSON`) | `Source/ui/public/js/index.js` (`runProductionP0SelfTest`, renderer status line) | Scene snapshots now expose deterministic Steam init stage/error/path data (`rendererSteamAudioInitStage`, `rendererSteamAudioInitErrorCode`, `rendererSteamAudioRuntimeLib`, `rendererSteamAudioMissingSymbol`). |
| Renderer UI/UX v2 structural shell + fallback-safe authority chips (BL-027 Slice A) | Existing renderer scene-state payload from `Source/PluginProcessor.cpp` (requested/active/profile/stage/output fields); additive-only consumption with no schema changes | `Source/ui/public/index.html` (v2 IA card shell) + `Source/ui/public/js/index.js` (`updateRendererPanelShell`, chip state helpers, defensive payload guards) | Renderer rail now exposes profile authority, output summary, diagnostics cards, and deterministic fallback-safe states when renderer payload is absent/partial while preserving legacy control IDs and runtime behavior. |
| Steam diagnostics card collapse + fielded fallback contract (BL-027 Slice C) | Existing Steam scene-state diagnostics from `Source/PluginProcessor.cpp` (`rendererSteamAudioCompiled`, `rendererSteamAudioAvailable`, `rendererSteamAudioInitStage`, `rendererSteamAudioInitErrorCode`, `rendererSteamAudioRuntimeLib`, `rendererSteamAudioMissingSymbol`) plus optional additive per-field tokens when present | `Source/ui/public/index.html` (`rend-steam-toggle`, `rend-steam-content`, `rend-steam-hrtf-status`, `rend-steam-binaural-tier`, `rend-steam-reflections-state`, `rend-steam-convolution-method`, `rend-steam-last-error`) + `Source/ui/public/js/index.js` (`setRendererSteamDiagnosticsExpanded`, `getRendererSteamDiagnosticsState`, `updateRendererPanelShell`) | Steam diagnostics are now collapsible (collapsed by default), expose five runtime fields with deterministic unknown/unavailable fallbacks for missing/partial payloads, preserve existing BL-029 IDs (`rend-steam-chip`, `rend-steam-detail`), and keep renderer UI behavior additive and non-breaking. |
| Ambisonic diagnostics card collapse + fielded fallback contract (BL-027 Slice D) | Existing Ambisonic scene-state diagnostics from `Source/PluginProcessor.cpp` (`rendererAmbiCompiled`, `rendererAmbiActive`, `rendererAmbiMaxOrder`, `rendererAmbiStage`, `rendererAmbiDecodeLayout`) plus optional additive ambisonic diagnostic tokens when present | `Source/ui/public/index.html` (`rend-ambi-toggle`, `rend-ambi-content`, `rend-ambi-order`, `rend-ambi-channel-count`, `rend-ambi-decoder-state`, `rend-ambi-decoder-type`) + `Source/ui/public/js/index.js` (`setRendererAmbiDiagnosticsExpanded`, `getRendererAmbiDiagnosticsState`, `updateRendererPanelShell`) | Ambisonic diagnostics are now collapsible (collapsed by default), expose order/channel-count/decoder-state/decoder-type fields with deterministic `Unknown` fallbacks for missing scalars, preserve existing BL-029 IDs (`rend-ambi-chip`, `rend-ambi-detail`), and keep renderer UI behavior additive and non-breaking. |
| Cross-panel profile coherence finalization (BL-027 Slice E) | `Source/PluginProcessor.cpp` (`getSceneStateJSON`: additive `profileSyncSeq`; `getCalibrationStatus`: additive `profileSyncSeq`) + `Source/PluginEditor.cpp` (`timerCallback` single-shot scene+calibration JS dispatch) | `Source/ui/public/js/index.js` (`updateSceneState`, `updateCalibrationStatus`, `updateRendererPanelShell`, `applyCalibrationStatus`) | Profile/status updates now use monotonic sequence guards and scene-authoritative calibration display fallbacks so CALIBRATE/RENDERER chips do not regress to stale labels under rapid switch bursts; behavior remains additive/backward-compatible. |
| Audition-cloud metadata bridge (BL-029 Slice C/F) | `Source/PluginProcessor.cpp` (`getSceneStateJSON`; derived from `rend_audition_*`, audition visual state, and deterministic seed hashing) | `Source/ui/public/js/index.js` (`window.updateSceneState`; additive consumer path) | Scene snapshots publish additive `rendererAuditionCloud` metadata with backward compatibility (`enabled`, `pattern`, `mode`, `emitterCount`, `pointCount`, `spreadMeters`, `seed`, `pulseHz`, `coherence`, `emitters[]` where each source has `id`, `weight`, `localOffsetX/Y/Z`, `phase`, `activity`) so UI can render concurrent multi-source audition clouds while preserving legacy single-glyph fallback. |
| Audition binding resolver metadata bridge (BL-029 Slice B1) | `Source/PluginProcessor.cpp` (`getSceneStateJSON`; renderer-owned requested/resolved mode resolver using existing audition/emitter/choreography/physics runtime state) | `Source/ui/public/js/index.js` (`window.updateSceneState`; optional additive consumer) | Scene snapshots now include additive resolver telemetry (`rendererAuditionSourceMode`, `rendererAuditionRequestedMode`, `rendererAuditionResolvedMode`, `rendererAuditionBindingTarget`, `rendererAuditionBindingAvailable`, `rendererAuditionSeed`, `rendererAuditionTransportSync`, `rendererAuditionDensity`, `rendererAuditionReactivity`, `rendererAuditionFallbackReason`) with deterministic fallback semantics for `bound_emitter`, `bound_choreography`, and `bound_physics` while preserving legacy single/cloud behavior. |
| Audition reactive envelope + physics coupling bridge (BL-029 Slice G1/G5/R1/P6) | `Source/SpatialRenderer.h` (`setAuditionPhysicsReactiveInput`, `publishAuditionReactiveTelemetry`, `getAuditionReactiveSnapshot`; centralized unit-range sanitize + bounded source-count guards + additive geometry derivation) + `Source/PluginProcessor.cpp` (`processBlock` physics summary extraction + `getSceneStateJSON` defensive sanitize/fallback publication) | `Source/ui/public/js/index.js` (`window.updateSceneState`; optional additive consumer for rain/snow fade drivers and physics-reactive morphing) | Scene snapshots include additive `rendererAuditionReactive` telemetry with strict `[0..1]` scalar bounds, fixed-capacity source-energy bounds, deterministic neutral fallback (`reactive_payload_missing` / `reactive_payload_invalid`), explicit audition fallback reasons for visual/cloud/reactive defensive branches, and additive geometry metadata (`geometryScale`, `geometryWidth`, `geometryDepth`, `geometryHeight`, `precipitationFade`, `collisionBurst`, `densitySpread`) derived from existing reactive + physics runtime state. |
| Audition cinematic preset language contract (BL-029 Slice G6) | `Documentation/plans/bl-029-cinematic-reactive-preset-language-2026-02-24.md` (v1->v3 dictionary, mapping tables, acceptance thresholds) | `Documentation/testing/bl-029-audition-platform-qa.md` (G6 acceptance-to-lane mapping + triage contract) | Pre-code expansion design authority for cinematic preset growth: explicit feature mapping, deterministic acceptance IDs, and docs-gated QA alignment before source-level expansion. |
| Audition reactive UI hardening + deterministic self-test scope (BL-029 Slice Z3) | `Source/ui/public/js/index.js` (`window.updateSceneState` startup queueing, defensive cloud transform parsing, pattern-reactive geometry clamps, `selftest_scope=bl029` checks) + `Source/ui/public/index.html` (existing audition renderer controls reused) | `Documentation/testing/bl-029-audition-platform-qa.md` (`UI-P1-029A/B/C` acceptance contract) | UI now defends against additive/missing payloads without startup throws, keeps single-glyph fallback when cloud payload is invalid, and exposes deterministic Z3 acceptance IDs: `UI-P1-029A` (schema-additive transform bounds), `UI-P1-029B` (fallback robustness), `UI-P1-029C` (binaural parity telemetry bounds). |
| CLAP lifecycle/runtime diagnostics (BL-011 Slice 2) | `Source/PluginProcessor.h` / `Source/PluginProcessor.cpp` (`clap_properties` bridge + `getClapRuntimeDiagnostics` + scene snapshot fields) | `Source/ui/public/js/index.js` (`runProductionP0SelfTest` optional `UI-P2-011`) + `Source/PluginEditor.cpp` (`selftest_bl011` query flag wiring) | Scene snapshots now expose deterministic CLAP status and lifecycle telemetry (`clapBuildEnabled`, `clapPropertiesAvailable`, `clapIsPluginFormat`, `clapLifecycleStage`, `clapRuntimeMode`, `clapVersion`) for host-format triage and closeout evidence. |
| Automated production self-test coverage | Production self-test script (`scripts/standalone-ui-selftest-production-p0-mac.sh`) | `Source/ui/public/js/index.js` (`runProductionP0SelfTest`) | Added P1 assertions: `UI-P1-015`, `UI-P1-014`, `UI-P1-008`, `UI-P1-006`, `UI-P1-007`; optional deterministic diagnostics lanes: `UI-P1-009` (`selftest_bl009=1`) and `UI-P2-011` (`selftest_bl011=1`). |

## HX-05 Payload Budget Contract (Slice A)

| Contract Surface | Source of Truth | Acceptance ID | Evidence Contract |
|---|---|---|---|
| Scene-state payload/throughput hard limits (`bytes/update`, cadence caps, burst policy, degradation tiers) | `Documentation/scene-state-contract.md` (HX-05 section) + `Documentation/backlog/done/hx-05-payload-budget.md` (Authoritative Budget Contract table) | `HX05-AC-001` | `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/budget_contract.md` |
| Additive schema guidance for optional HX-05 telemetry (`snapshotPayloadBytes`, `snapshotBudgetTier`, `snapshotBurstCount`, `snapshotBudgetPolicyVersion`) | `Documentation/scene-state-contract.md` (HX-05 Additive Schema Guidance) | `HX05-AC-002` | `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/budget_contract.md` |
| Acceptance criteria + measurable enforcement checks linked to artifact schemas | `Documentation/backlog/done/hx-05-payload-budget.md` (Acceptance Criteria + Enforcement Checks tables) | `HX05-AC-003` | `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/status.tsv` |
| Documentation hygiene gate for contract-only slice | `scripts/validate-docs-freshness.sh` | `HX05-AC-004` | `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/docs_freshness.log` |

## HX-05 Payload Budget QA Lane Contract (Slice B)

| Contract Surface | Source of Truth | Acceptance ID | Evidence Contract |
|---|---|---|---|
| Deterministic soak lane window schedule (`W0` warmup, `W1` nominal, `W2` burst, `W3` sustained stress) | `Documentation/backlog/done/hx-05-payload-budget.md` (Slice B QA Lane Spec) + `Documentation/testing/selftest-stability-contract.md` (HX-05 Payload Budget Soak Lane Contract) | `HX05-B-AC-001` | `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/qa_lane_contract.md` |
| Payload-budget lane artifact schemas (`payload_metrics.tsv`, `transport_cadence.tsv`, `budget_tier_events.tsv`, `taxonomy_table.tsv`, `status.tsv`) | `Documentation/backlog/done/hx-05-payload-budget.md` (Artifact Schema Contract) + `Documentation/testing/selftest-stability-contract.md` (Required Artifacts) | `HX05-B-AC-002` | `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/qa_lane_contract.md` |
| Failure taxonomy contract for budget regressions (`oversize_hard_limit`, `oversize_soft_limit`, `burst_overrun`, `cadence_violation`, `degrade_tier_mismatch`) | `Documentation/backlog/done/hx-05-payload-budget.md` (Failure Taxonomy) + `Documentation/testing/selftest-stability-contract.md` (Failure Taxonomy HX-05) | `HX05-B-AC-003` | `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/taxonomy_table.tsv` |
| Slice B acceptance-ID traceability mapping present in this matrix | `Documentation/backlog/done/hx-05-payload-budget.md` (Slice B acceptance table) + `Documentation/implementation-traceability.md` (this section) | `HX05-B-AC-004` | `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/status.tsv` |
| Slice B docs-only validation hygiene gate | `scripts/validate-docs-freshness.sh` | `HX05-B-AC-005` | `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/docs_freshness.log` |

## HX-05 Payload Budget Soak Harness (Slice C)

| Contract Surface | Source of Truth | Acceptance ID | Evidence Contract |
|---|---|---|---|
| Deterministic soak evaluator implementation (`--input-dir`, strict `0/1/2` exits, machine-readable outputs) | `scripts/qa-hx05-payload-budget-soak-mac.sh` + `Documentation/backlog/done/hx-05-payload-budget.md` (Slice C Soak Harness) | `HX05-C-AC-001` | `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/status.tsv` |
| Required artifact schema validation before threshold scoring | `scripts/qa-hx05-payload-budget-soak-mac.sh` (`schema_validation`) + `Documentation/testing/selftest-stability-contract.md` (Slice C) | `HX05-C-AC-002` | `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/status.tsv` |
| Deterministic threshold + policy enforcement (`max`, `p95`, cadence, burst, tier transitions) | `scripts/qa-hx05-payload-budget-soak-mac.sh` evaluator + `Documentation/backlog/done/hx-05-payload-budget.md` (Slice A+B thresholds) | `HX05-C-AC-003` | `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/taxonomy_table.tsv` |
| Fixture replay contract (PASS fixture -> exit 0, FAIL fixture -> exit 1) | `scripts/qa-hx05-payload-budget-soak-mac.sh` + fixture protocol in `Documentation/backlog/done/hx-05-payload-budget.md` | `HX05-C-AC-004` | `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/pass_fixture_result.tsv`, `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/fail_fixture_result.tsv` |
| Slice C docs freshness hygiene gate | `scripts/validate-docs-freshness.sh` | `HX05-C-AC-005` | `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/docs_freshness.log` |

## BL-032 Processor Native Extraction (Slice B)

| Contract Surface | Source of Truth | Slice B Mapping | Evidence Contract |
|---|---|---|---|
| Shared bridge response key contract | `Source/shared_contracts/BridgeStatusContract.h` | Additive canonical non-RT response keys (`ok`, `message`, `name`, `file`, `path`) consumed by processor bridge glue | `TestEvidence/bl032_slice_b_native_extract_<timestamp>/moved_symbols.tsv` |
| Processor-core parameter read/write helper contract | `Source/processor_core/ProcessorParameterReaders.h` | Snapshot channel resolution + calibration routing/indices + integer host-notify writes delegated from `PluginProcessor` | `TestEvidence/bl032_slice_b_native_extract_<timestamp>/module_migration_map.md` |
| Processor-bridge utility contract | `Source/processor_bridge/ProcessorBridgeUtilities.h` | Preset/profile sanitization + normalization + options-to-file resolution + JSON IO delegated from `PluginProcessor` | `TestEvidence/bl032_slice_b_native_extract_<timestamp>/moved_symbols.tsv` |
| PluginProcessor facade parity contract | `Source/PluginProcessor.cpp` | Existing public method signatures preserved while non-RT helpers route through extracted modules | `TestEvidence/bl032_slice_b_native_extract_<timestamp>/qa_smoke.log` |
| Slice B validation gates | `Documentation/backlog/bl-032-source-modularization.md` | Build/smoke/docs pass with RT static-audit blocker (`non_allowlisted=80`) | `TestEvidence/bl032_slice_b_native_extract_<timestamp>/status.tsv`, `TestEvidence/bl032_slice_b_native_extract_<timestamp>/rt_audit.tsv` |

## BL-032 Editor Shell/WebView Extraction (Slice C1)

| Contract Surface | Source of Truth | Slice C1 Mapping | Evidence Contract |
|---|---|---|---|
| WebView runtime configuration contract | `Source/editor_webview/EditorWebViewRuntime.h` | UI self-test env parsing, initial URL/title derivation, backend options, native bridge registration extracted from `PluginEditor.cpp` | `TestEvidence/bl032_slice_c1_editor_extract_<timestamp>/module_move_map.md` |
| Resource-provider lifecycle contract | `Source/editor_webview/EditorWebViewRuntime.h` | Embedded BinaryData resource dispatch moved out of `PluginEditor` member methods into module function `getResource` | `TestEvidence/bl032_slice_c1_editor_extract_<timestamp>/guardrail_report.tsv` |
| Editor shell orchestration helper contract | `Source/editor_shell/EditorShellHelpers.h` | Deterministic scene/calibration JS push, resize notification, runtime probe/selftest script sources extracted from monolithic editor file | `TestEvidence/bl032_slice_c1_editor_extract_<timestamp>/module_move_map.md` |
| PluginEditor facade parity contract | `Source/PluginEditor.cpp` + `Source/PluginEditor.h` | Relay/attachment ownership and order preserved while runtime/resource concerns delegate to module helpers | `TestEvidence/bl032_slice_c1_editor_extract_<timestamp>/selftest_runs.tsv` |
| Slice C1 validation gates | `Documentation/backlog/bl-032-source-modularization.md` | Build/selftest/docs pass, guardrail residual fails on pre-existing Processor line-count threshold (`BL032-G-001`) | `TestEvidence/bl032_slice_c1_editor_extract_<timestamp>/status.tsv`, `TestEvidence/bl032_slice_c1_editor_extract_<timestamp>/blocker_taxonomy.tsv` |

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

## HX-04 Scenario Coverage Drift Guard

- Required scenario manifest: `qa/scenarios/locusq_hx04_required_scenarios.json`
  - Declares required component-level parity for `AirAbsorption`, `CalibrationEngine`, and directivity (`emit_dir` + aim).
- Dedicated parity suite: `qa/scenarios/locusq_hx04_component_parity_suite.json`
  - Executes `locusq_air_absorption_distance`, `locusq_calibration_sweep_capture`, `locusq_emit_dir_spatial_effect`, and `locusq_directivity_aim` under fixed runtime config.
- Audit command: `scripts/qa-hx04-scenario-audit.sh`
  - Validates manifest presence, scenario membership, required suite wiring, and suite execution output.
  - Emits deterministic artifact bundle under `TestEvidence/hx04_scenario_audit_<timestamp>/` with `status.tsv` + `coverage_matrix.tsv`.
- BL-012 lane wiring: `scripts/qa-bl012-harness-backport-tranche1-mac.sh`
  - Runs HX-04 audit by default (`LQ_BL012_RUN_HX04_AUDIT=1`) so tranche-1 backport validation fails loudly on scenario parity drift.
- Latest evidence:
  - `TestEvidence/hx04_scenario_audit_20260223T035254Z/status.tsv`
  - `TestEvidence/bl012_harness_backport_20260223T035318Z/status.tsv`

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

## BL-026 Slice A CALIBRATE Topology Profile Wiring

Scope: align CALIBRATE topology profile choices across APVTS, production UI, and runtime alias normalization without changing calibration engine state transitions.

| Parameter ID / Contract | Implementation Path | Bridge Path | Notes |
|---|---|---|---|
| `cal_topology_profile` APVTS choice expansion | `Source/PluginProcessor.cpp` (`createParameterLayout`, `normaliseCalibrationTopologyId`, `topologyProfileForOutputChannels`, calibration profile apply/start mapping) | APVTS `AudioParameterChoice` index -> emitted topology IDs in calibration status/profile payloads | Canonical IDs now include `mono`, `stereo`, `quad`, `surround_51`, `surround_71`, `surround_712`, `surround_742`, `binaural`, `ambisonic_1st`, `ambisonic_3rd`, `downmix_stereo` with legacy alias compatibility preserved. |
| CALIBRATE topology selector UI | `Source/ui/public/index.html` (`#cal-topology`) | `bindSelectToComboState("cal-topology", comboStates.cal_topology_profile)` in `Source/ui/public/js/index.js` | Dropdown choices and default (`Stereo`) now match APVTS order for deterministic combo index mapping. |
| Topology alias dictionary + runtime resolution | `Source/ui/public/js/index.js` (`calibrationTopologyAliasDictionary`, `resolveCalibrationTopologyId`, `getCalibrationTopologyIndex`) | Processor-reported `topologyProfile` IDs and legacy IDs map to canonical client IDs before status/render use | UI status chips, preview speaker positions, required-channel checks, and BL-026 self-test topology switching now resolve through one canonical dictionary. |

## BL-026 Slice C CALIBRATE Profile Library Save/Recall

Scope: enforce tuple-scoped profile persistence and guarded profile recall by `topologyProfile` + `monitoringPath` in CALIBRATE without changing audio-thread behavior.

| Parameter / Contract Surface | Implementation Path | Bridge Path | Notes |
|---|---|---|---|
| Profile tuple metadata publication | `Source/PluginProcessor.cpp` (`listCalibrationProfilesFromUI`, `saveCalibrationProfileFromUI`, `loadCalibrationProfileFromUI`) | Native responses now include `topologyProfile`, `monitoringPath`, `deviceProfile`, `profileTupleKey` | Profile CRUD results now expose tuple identity to UI for deterministic filtering/selection. |
| Tuple-derived default profile naming | `Source/PluginProcessor.cpp` (`saveCalibrationProfileFromUI`) + `Source/ui/public/js/index.js` (`buildDefaultCalibrationProfileName`, `saveCalibrationProfile`) | Empty profile names auto-resolve to `<topology>_<monitor>_<timestamp>` | Save flow no longer depends on manual name entry for unique, tuple-identifiable profile artifacts. |
| Tuple-scoped profile listing in CALIBRATE | `Source/ui/public/js/index.js` (`refreshCalibrationProfileList`, `profileEntryMatchesCalibrationTuple`) | Current `cal_topology_profile` + `cal_monitoring_path` selection gates displayed profile options | CALIBRATE list now hides non-matching tuple profiles and reports hidden-count status message. |
| Guarded tuple-matched profile recall | `Source/PluginProcessor.cpp` (`loadCalibrationProfileFromUI`) + `Source/ui/public/js/index.js` (`loadCalibrationProfile`) | UI passes `enforceTupleMatch=true` + expected tuple; processor rejects mismatched tuple payloads | Prevents accidental load of incompatible topology/monitoring profile combinations. |

## BL-026 Slice D+E CALIBRATE Diagnostics + Host/Resize Reliability

Scope: harden CALIBRATE validation diagnostics with deterministic states (`PASS`/`FAIL`/`UNTESTED`) and close host resize regressions while preserving calibration lifecycle behavior.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Deterministic diagnostics card state model | `Source/ui/public/index.html` (`.cal-diagnostic-card`, `#cal-validation-*-chip`, `#cal-validation-*-detail`) + `Source/ui/public/js/index.js` (`normaliseCalibrationDiagnosticState`, `setCalibrationValidationState`, `applyCalibrationStatus`) | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` (`UI-P1-026D`, `UI-P1-026E`) | CALIBRATE diagnostics now resolve to explicit `PASS`, `FAIL`, or `UNTESTED` only; prior `PENDING/WARN` chip semantics removed from diagnostic-card contract. |
| Profile activation and downmix diagnostics details | `Source/ui/public/js/index.js` (`applyCalibrationStatus`) | BL-026 scoped self-test + manual QA doc `Documentation/testing/bl-026-calibrate-uiux-v2-qa.md` | Card detail copy now publishes deterministic requested vs active mode/profile context and fallback reason text when downmix fallback is active. |
| Host resize bridge reliability | `Source/PluginEditor.cpp` (`resized`, `window.__LocusQHostResized`) + `Source/ui/public/js/index.js` (`syncResponsiveLayoutMode`, `window.__LocusQHostResized`) | BL-026 scoped self-test layout assertions (`compact`, `tight`) | Native resize now explicitly nudges WebView runtime to recompute responsive classes + canvas sizing, guarding hosts that under-report DOM resize callbacks. |
| Compact/tight CALIBRATE control usability | `Source/ui/public/index.html` (calibrate-specific `body.layout-compact` / `body.layout-tight` rules) + `Source/ui/public/js/index.js` (`UI-P1-026E` layout checks) | BL-026 scoped self-test (`UI-P1-026E`) and BL-029 regression guard self-tests | CALIBRATE controls/cards remain visible and non-clipped across narrow-panel breakpoints without altering start/abort button behavior. |

## BL-028 Slice B1 Spatial Output Matrix QA Lane Scaffold

Scope: convert BL-028 A1 planning contracts into an executable deterministic lane scaffold without modifying source-renderer behavior.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| BL-028 matrix suite contract | `qa/scenarios/locusq_bl028_output_matrix_suite.json` | `./scripts/qa-bl028-output-matrix-lane-mac.sh` (`matrix_contract_eval`) | Scenario suite now defines deterministic matrix case rows (`SOM-028-01..11` + fail-safe case), enum constraints, thresholds, artifact schema, and acceptance IDs `BL028-A1-001..006`. |
| Executable lane runner + artifacts | `scripts/qa-bl028-output-matrix-lane-mac.sh` | `TestEvidence/bl028_slice_b1_<timestamp>/status.tsv`, `qa_lane.log`, `scenario_result.log`, `matrix_report.tsv`, `acceptance_parity.tsv` | Lane builds required targets, runs suite, evaluates deterministic contract rows, and emits machine-readable PASS/FAIL artifacts. |
| Acceptance-ID direct check mapping | `scripts/qa-bl028-output-matrix-lane-mac.sh` (`BL028-A1-001_matrix_legality`, `BL028-A1-002_fallback_contract`, `BL028-A1-003_diagnostics_schema`, `BL028-A1-004_status_text_map`, `BL028-A1-005_lane_thresholds`, `BL028-A1-006_acceptance_parity`) | `status.tsv` + `acceptance_parity.tsv` | Each BL-028 A1 acceptance ID now has an explicit lane check identifier and deterministic artifact pointer. |
| QA documentation synchronization | `Documentation/testing/bl-028-spatial-output-matrix-qa.md` + `Documentation/backlog/done/bl-028-spatial-output-matrix.md` | `./scripts/validate-docs-freshness.sh` | Command contract, artifact schema, thresholds, and acceptance mapping are documented and parity-checked across runbook/spec/qa/lane surfaces. |

## BL-028 Slice B2 Spatial Output Matrix QA Reliability Hardening

Scope: make BL-028 lane replay-stable and promotion-ready using deterministic replay controls and strict exit semantics, without changing source DSP/runtime code.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Lane replay controls (`--out-dir`, `--runs`) | `scripts/qa-bl028-output-matrix-lane-mac.sh` | `./scripts/qa-bl028-output-matrix-lane-mac.sh --runs 1` and `--runs 5` | Adds explicit CLI controls while retaining `BL028_OUT_DIR`/`BL028_RUNS` environment compatibility for backward callers. |
| Replay-run capture and hash signatures | `scripts/qa-bl028-output-matrix-lane-mac.sh` (`replay_runs.tsv`, `replay_hashes.tsv`) | `TestEvidence/bl028_slice_b2_<timestamp>/replay_runs.tsv`, `replay_hashes.tsv` | Per-run result summaries and signatures are machine-readable; run signatures are compared against run-1 baseline for determinism enforcement. |
| Deterministic divergence and transient taxonomy gate | `scripts/qa-bl028-output-matrix-lane-mac.sh` (`deterministic_replay_divergence`, `deterministic_contract_failure`, `transient_runtime_failure`, `transient_result_missing`) | `status.tsv` + `reliability_decision.md` | Replay lane fails if divergence/transient counts exceed scenario contract thresholds, preserving strict exit semantics (`0` only when all checks pass). |
| QA contract and artifact schema updates | `Documentation/testing/bl-028-spatial-output-matrix-qa.md` | `./scripts/validate-docs-freshness.sh` | QA spec now codifies replay contract, failure taxonomy, new artifacts, and B2 pass thresholds while preserving existing B1 artifacts/schema surfaces. |

## BL-028 Slice C1 Native Matrix State Publication

Scope: publish deterministic native BL-028 matrix diagnostics directly from renderer/runtime state so matrix behavior is observable in scene snapshots without introducing new audio-thread coupling.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Deterministic matrix snapshot derivation | `Source/PluginProcessor.cpp` (`buildRendererMatrixSnapshot`, profile/layout/domain mapping helpers) | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` + `./scripts/qa-bl028-output-matrix-lane-mac.sh --runs 3` | Matrix rule/fallback/reason/status values are derived from existing requested/active spatial profile indices, stage, headphone mode, and output-channel topology only; no new cross-thread dependency added. |
| Additive scene-state matrix payload publication | `Source/PluginProcessor.cpp` (`getSceneStateJSON`) | `TestEvidence/bl028_slice_c1_<timestamp>/qa_smoke.log` + `matrix_lane.log` | Scene snapshots now include additive `rendererMatrix*` fields (`requested/active domain/layout`, `rule`, `reason`, `fallback`, `status text`, `event seq`) plus grouped `rendererMatrix{requestedDomain,activeDomain,requestedLayout,activeLayout,ruleId,ruleState,fallbackMode,reasonCode,statusText}` mirror keys, preserving backward compatibility for existing consumers. |
| Contract documentation for matrix payload | `Documentation/scene-state-contract.md` (Spatial output matrix diagnostics section + additive rule) | `./scripts/validate-docs-freshness.sh` | Interface contract now explicitly defines field enums and compatibility expectations for BL-028 matrix diagnostics. |
| RT-safety guardrail continuity | `Source/PluginProcessor.cpp` (message-thread JSON composition only; no process-block changes) | `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl028_slice_c1_<timestamp>/rt_audit.tsv` | Changes avoid audio-thread locks/allocations and keep existing process-block behavior unchanged. |

## BL-030 Slice B Device Rerun Matrix Contract

Scope: define deterministic release-governance validation rows for device-profile reruns (`DEV-01..DEV-06`) with explicit pass criteria, evidence paths, and constrained `N/A` policy.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Device rerun matrix authority (`DEV-01..DEV-06`) | `Documentation/runbooks/device-rerun-matrix.md` | Executed under `TestEvidence/bl030_release_governance_<timestamp>/device_matrix_results.tsv` | Matrix defines required rows, deterministic run order, and result ledger schema. |
| Spatial profile/device checks (`quad`, `stereo`, `headphone generic`, `headphone Steam`) | `Documentation/runbooks/device-rerun-matrix.md` (`DEV-01..DEV-04`) | Scripted lanes: `scripts/qa-bl018-profile-matrix-strict-mac.sh`, `scripts/qa-bl009-headphone-contract-mac.sh`, `scripts/qa-bl009-headphone-profile-contract-mac.sh` plus manual notes | Aligns release rerun checks to ADR-0006 device-profile contract and BL-009/BL-018 deterministic evidence lanes. |
| Calibration mic rerun checks (`built-in`, `external`) | `Documentation/runbooks/device-rerun-matrix.md` (`DEV-05`, `DEV-06`) | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` + manual calibration notes | External mic row permits `N/A` only for explicit hardware-unavailable waiver; all other rows prohibit `N/A`. |
| Release-gate `N/A` governance | `Documentation/runbooks/device-rerun-matrix.md` (`Execution Policy`, `N/A Policy` column) | Release checklist gate `RL-05` in `Documentation/runbooks/release-checklist-template.md` | Removes implicit skip behavior by requiring row-level `N/A` policy and written waiver artifacts where allowed. |

## BL-030 Slice C CI Release-Governance Integration Contract

Scope: automate repeatable release-gate execution on tag/manual triggers while preserving explicit operator ownership of hardware-dependent matrix rows.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Release-governance CI trigger and lane | `.github/workflows/release-governance.yml` (`workflow_dispatch`, tag `push`) | Local wiring checks in `TestEvidence/bl030_slice_c_20260224T203848Z/ci_integration.log` + YAML parse proof | Workflow defines deterministic entrypoints for release gate automation (`tag` and manual dispatch). |
| Automated gates in CI (`build`, `ctest`, `docs freshness`, `production self-test`, `pluginval`) | `.github/workflows/release-governance.yml` step chain under `release-governance-automated` | `TestEvidence/bl030_slice_c_20260224T203848Z/status.tsv` | Encodes BL-030 Slice C gate set with explicit failure semantics for non-zero exits. |
| Optional CLAP gate policy | `.github/workflows/release-governance.yml` (`enable_clap_gate` input + conditional step) | Static wiring proof (`clap-info`, `clap-validator` step presence) | CLAP validation is explicit and operator-controlled for scope alignment; disabled by default for non-CLAP release lanes. |
| Manual gate surfacing (device matrix ownership) | `.github/workflows/release-governance.yml` (`manual_required_gates.txt`) | Uploaded artifact `locusq-release-governance-gates` | Keeps `RL-05` manual/hardware contract explicit in CI output, preventing implicit automation claims. |

## BL-030 Slice D Dry-Run Baseline Contract

Scope: execute first end-to-end release-governance checklist run, produce deterministic evidence artifacts, and capture blocked release gates without mutating gate semantics.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Dry-run checklist execution bundle | `TestEvidence/bl030_release_governance_20260224T204022Z/release_checklist_run.md` | `TestEvidence/bl030_release_governance_20260224T204022Z/status.tsv` | Captures full `RL-01..RL-10` results, N/A justifications, and release decision (`BLOCKED`) in one deterministic bundle. |
| Device matrix dry-run ledger and waiver handling | `TestEvidence/bl030_release_governance_20260224T204022Z/device_matrix_results.tsv` + DEV note/waiver files | `RL-05` row in dry-run report | Preserves explicit row outcomes (`FAIL` for pending manual rows, `DEV-06` allowed `N/A` with waiver) instead of implicit skips. |
| Automated gate evidence capture during dry-run | `gate_rl03_selftest.log`, `gate_rl04_reaper_smoke.log`, `gate_rl06_pluginval_{vst3,au}.log`, `gate_rl08_docs_freshness.log`, `release_artifact_manifest.tsv` | Gate rows `RL-03`, `RL-04`, `RL-06`, `RL-08`, `RL-10` marked `PASS` in bundle | Confirms release-governance checklist commands are executable on current baseline and produces reusable artifact-path contract. |
| Blocked-gate reporting contract | `release_checklist_run.md` (`Blocking gates: RL01, RL05, RL09`) | `BL030-SliceD-release-decision` row in `status.tsv` | Provides explicit release-block reasons for owner triage instead of soft warnings or ambiguous closeout state. |

## BL-031 Slice D Deterministic Tempo-Ramp QA Contract

Scope: deliver deterministic BL-031 token scheduler validation under fixed tempo, tempo ramps, transport transitions, and missing-host-time fallback without modifying audio-thread implementation surfaces.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| BL-031 deterministic scenario contract | `qa/scenarios/locusq_bl031_tempo_ramp_suite.json` | `scripts/qa-bl031-tempo-token-lane-mac.sh` (`scenario_exec`, `scenario_status`) | Declares hard-fail QA invariants plus `bl031_contract_checks` (source tokens, simulation defaults, acceptance IDs, deterministic tempo/transport cases). |
| Lane runner and machine-readable outputs | `scripts/qa-bl031-tempo-token-lane-mac.sh` | `TestEvidence/bl031_slice_d_<timestamp>/status.tsv`, `qa_lane.log`, `scenario_result.log` | Executes build + scenario and emits deterministic lane check rows (`PASS`/`FAIL`) for promotion gating. |
| Token monotonicity and bounded-capacity evidence | `scripts/qa-bl031-tempo-token-lane-mac.sh` (embedded deterministic model aligned to scheduler boundary logic) | `TestEvidence/bl031_slice_d_<timestamp>/token_monotonicity.tsv` + `token_summary.json` | Validates `UI-P2-031A..D` acceptance IDs: non-decreasing PPQ token ordering, fixed-tempo beat spacing, ramp density growth, stop/resume behavior, and zero-token fallback when host time is unavailable. |
| QA lane and acceptance documentation | `Documentation/testing/bl-031-tempo-token-scheduler-qa.md` | `./scripts/validate-docs-freshness.sh` | Documents command contract, acceptance IDs, expected metrics, and deterministic failure triage flow for BL-031 Slice D closeout. |

## BL-032 Slice A Processor/Editor Module Boundary Map

Scope: establish deterministic module ownership/dependency contracts for `PluginProcessor`/`PluginEditor` decomposition so implementation slices can run in parallel without file collisions.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Target module boundary set (`processor_core`, `processor_bridge`, `editor_shell`, `editor_webview`, `shared_contracts`) | `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md` (Target Module Boundary Map) + `Documentation/backlog/bl-032-source-modularization.md` (Acceptance IDs) | `./scripts/validate-docs-freshness.sh` | Satisfies `BL032-A-001` by fixing the module taxonomy and preventing ad-hoc tranche splits. |
| Owned files (current/planned) and public interface contracts per module | `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md` (module sections) | `TestEvidence/bl032_slice_a_boundary_map_<timestamp>/boundary_map.md` | Satisfies `BL032-A-002` with deterministic ownership and interface declarations prior to source extraction. |
| Forbidden dependency and one-way layering rules | `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md` (Dependency Rules) | `TestEvidence/bl032_slice_a_boundary_map_<timestamp>/module_dependency_matrix.tsv` | Satisfies `BL032-A-003` by encoding disallowed reverse edges and editor/processor isolation constraints. |
| Tranche migration order + no-overlap slice ownership plan (`A/B/C`) | `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md` (Slice Ownership Plan + Migration Sequence) + runbook slice table | `TestEvidence/bl032_slice_a_boundary_map_<timestamp>/slice_ownership_plan.tsv` | Satisfies `BL032-A-004` and `BL032-A-005`; B and C file ownership lists are mutually exclusive for parallel worker safety. |
| Acceptance-ID cross-reference parity | `Documentation/backlog/bl-032-source-modularization.md`, `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md`, this section | `TestEvidence/bl032_slice_a_boundary_map_<timestamp>/status.tsv` | Satisfies `BL032-A-006` with explicit tri-doc acceptance linkage and evidence pointers. |

## BL-033 Slice D1 Headphone Core QA Lane Scaffold

Scope: establish deterministic BL-033 headphone-core QA lane scaffolding and cross-document acceptance parity before source-level calibration slices are merged.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| BL-033 suite contract scaffold (`BL033-D1-001`, `BL033-D1-002`, `BL033-D1-003`) | `qa/scenarios/locusq_bl033_headphone_core_suite.json` | `scripts/qa-bl033-headphone-core-lane-mac.sh --contract-only` (`BL033-D1-001_contract_schema`, `BL033-D1-002_diagnostics_fields`, `BL033-D1-003_artifact_schema`) | Scenario now declares deterministic acceptance IDs, diagnostics-field contract, thresholds, artifact schema, and failure taxonomy for headphone-core lanes. |
| Lane runner strict status + taxonomy output (`BL033-D1-005`, `BL033-D1-006`) | `scripts/qa-bl033-headphone-core-lane-mac.sh` | `TestEvidence/bl033_headphone_core_<timestamp>/status.tsv` + `taxonomy_table.tsv` | Script enforces strict exit semantics with machine-readable status rows and explicit execution-mode contract (`contract_only` vs `execute_suite`). |
| Acceptance parity mapping (`BL033-D1-004`) | `scripts/qa-bl033-headphone-core-lane-mac.sh` + `Documentation/backlog/bl-033-headphone-calibration-core.md` + `Documentation/testing/bl-033-headphone-core-qa.md` + this section | `TestEvidence/bl033_headphone_core_<timestamp>/acceptance_parity.tsv` | Lane verifies each BL-033 D1 acceptance ID is present across runbook, QA doc, traceability, scenario, and script surfaces before promotion. |
| QA contract publication and command contract sync | `Documentation/testing/bl-033-headphone-core-qa.md` | `./scripts/validate-docs-freshness.sh` | Testing guide codifies lane commands, artifact schema, deterministic thresholds, and triage order aligned to runbook + scenario contracts. |

## BL-033 Slice D2 Headphone Core QA Closeout Expansion

Scope: expand BL-033 lane from scaffold coverage to closeout-grade reliability checks with deterministic multi-run replay validation and strict artifact schema enforcement.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Diagnostics requested/active/stage/fallback consistency (`BL033-D2-001`) | `qa/scenarios/locusq_bl033_headphone_core_suite.json` (`diagnostics_consistency_contract`) + `scripts/qa-bl033-headphone-core-lane-mac.sh` (`BL033-D2-001_diagnostics_consistency`) | `./scripts/qa-bl033-headphone-core-lane-mac.sh --contract-only --runs 3` and `--execute-suite --runs 5` | Lane enforces pairwise requested/active contracts plus required stage/fallback fields before accepting closeout promotion. |
| Replay determinism hash and row stability (`BL033-D2-002`) | `scripts/qa-bl033-headphone-core-lane-mac.sh` multi-run branch (`validation_matrix.tsv`, `replay_hashes.tsv`, replay thresholds) + scenario `replay_contract` | `./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --runs 5` | Multi-run closeout computes per-run signatures and row signatures, then fails when divergence exceeds configured thresholds. |
| Strict artifact schema completeness (`BL033-D2-003`) | `qa/scenarios/locusq_bl033_headphone_core_suite.json` (`artifact_schema`, `artifact_schema_execute_additions`, `artifact_schema_multi_run`) + lane checks in single/multi-run paths | `status.tsv` rows `BL033-D2-003_artifact_schema_complete` + replay `taxonomy_table.tsv` | Enforces required artifact presence for both single-run and replay closeout outputs with non-zero exit on missing artifacts. |
| D2 documentation and parity surface alignment | `Documentation/testing/bl-033-headphone-core-qa.md` + this traceability section + scenario + lane script | `acceptance_parity.tsv` + `./scripts/validate-docs-freshness.sh` | D2 acceptance IDs remain parity-tracked across owned surfaces while preserving D1 runbook-linked parity behavior. |

## BL-033 Slice Z1 Owner Integration Replay

Scope: owner-authoritative reconciliation of A1/B1/D1 handoffs with fresh replay evidence and promotion-state decisioning.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Handoff intake and shared-file safety verification | `TestEvidence/bl033_slice_a1_processor_contract_20260225T232640Z/status.tsv`, `TestEvidence/bl033_slice_b1_renderer_chain_20260225T232819Z/status.tsv`, `TestEvidence/bl033_slice_d1_qa_contract_20260225T232722Z/status.tsv` | `TestEvidence/bl033_owner_sync_z1_20260226T000200Z/handoff_resolution.md` | All three worker handoffs reported `SHARED_FILES_TOUCHED: no`; owner intake preserved per-slice outcomes and blockers. |
| Owner replay build/smoke/headphone-contract checks | Existing BL-033 source surfaces (`Source/PluginProcessor*`, `Source/SpatialRenderer.h`, `Source/headphone_core/*`, `Source/headphone_dsp/*`) | `TestEvidence/bl033_owner_sync_z1_20260226T000200Z/status.tsv` (`build`, `qa_smoke`, `qa_bl009_headphone_contract`) | Replay confirms build + smoke + BL-009 lane pass on current branch despite prior worker bundle failures. |
| D1 lane determinism replay (`execute-suite` x3) | `scripts/qa-bl033-headphone-core-lane-mac.sh` + `qa/scenarios/locusq_bl033_headphone_core_suite.json` | `TestEvidence/bl033_owner_sync_z1_20260226T000200Z/validation_matrix.tsv` + `qa_lane.log` | All three owner replay runs passed with zero warnings and stable acceptance-parity checks. |
| Promotion blocker contract (`RT` + docs freshness) | `scripts/rt-safety-audit.sh` output + docs freshness gate output | `TestEvidence/bl033_owner_sync_z1_20260226T000200Z/rt_audit.tsv`, `docs_freshness.log`, `owner_decisions.md` | Decision remains blocked while `non_allowlisted=94` and prior-worker evidence markdown metadata debt keep required gates red. |

## BL-033 Slice Z8 Owner Replay Closeout Update

Scope: reconcile post-Z1 unblock slices (Z2/Z5/Z6/Z7), re-run owner validation with hardened lane contract, and publish promotion-state decision.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| RT gate reconciliation intake | `scripts/rt-safety-allowlist.txt` delta + `Documentation/backlog/hx-06-rt-safety-audit.md` update from Z2 | `TestEvidence/bl033_rt_gate_z2_20260226T003240Z/rt_after.tsv` | Z2 moved RT audit from `non_allowlisted=94` to `non_allowlisted=0`; owner replay confirms this gate remains green. |
| Docs freshness blocker closure intake | Root doc metadata sync from Z5 (`README.md`, `CHANGELOG.md`) | `TestEvidence/bl033_root_docs_z5_20260226T004444Z/status.tsv` | Resolves prior freshness blocker caused by date mismatch with `status.json`. |
| Lane hardening deterministic multi-run contract | `scripts/qa-bl033-headphone-core-lane-mac.sh` (`--runs`) + QA doc sync | `TestEvidence/bl033_lane_hardening_z6_20260226T004506Z/status.tsv` + `.../exec_runs/validation_matrix.tsv` | Confirms backward-compatible single-run behavior and deterministic multi-run output matrix for owner replay automation. |
| Owner replay closeout gates | Existing BL-033 source/runtime surfaces with no new code edits in Z8 | `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/status.tsv`, `validation_matrix.tsv`, `rt_audit.tsv`, `docs_freshness.log` | All required Z8 gates pass (`build`, `smoke`, BL-033 lane x3, BL-009 lane, RT audit, status JSON, docs freshness); BL-033 advanced to `In Validation`. |

## BL-033 Slice Z11 Owner Sync + Promotion Decision

Scope: integrate post-D2/Z9/Z10 branch state and publish owner-authoritative BL-033 promotion posture using a full replay on current branch state.

| Contract Surface | Implementation Path | Validation Path | Notes |
|---|---|---|---|
| Z9 RT gate reconciliation intake | `TestEvidence/bl033_rt_gate_z9_20260226T010610Z/*` (`rt_before`, `rt_after`, blocker resolution) | `TestEvidence/bl033_owner_sync_z11_20260225_200647/rt_audit.tsv` | Confirms historical drift closure is preserved on current branch (`non_allowlisted=0`). |
| Z10 evidence hygiene intake | `TestEvidence/bl033_evidence_hygiene_z10_20260226T010548Z/*` | `TestEvidence/bl033_owner_sync_z11_20260225_200647/07_docs_freshness.log` | Confirms metadata hygiene debt no longer blocks docs freshness gate. |
| D2 deterministic replay closure | Existing BL-033 lane + scenario contracts (`--execute-suite --runs 5`) | `TestEvidence/bl033_owner_sync_z11_20260225_200647/lane_runs/validation_matrix.tsv` + `replay_hashes.tsv` | Replay hash and row signatures remain stable across all five runs. |
| Owner integration decision packet | Owned docs/status sync + Z11 evidence bundle (`owner_decisions.md`, `handoff_resolution.md`) | `TestEvidence/bl033_owner_sync_z11_20260225_200647/status.tsv` + `validation_matrix.tsv` | Owner decision is upgraded from `In Validation` to `Done-candidate` on fully green required gates. |

## Notes

- Room chain order in renderer: emitter spatialization -> `EarlyReflections` -> `FDNReverb` -> speaker delay/trim -> master gain/output.
- Phase 2.5 acceptance remains closed on hard gates; warning-level trends are tracked independently from the Phase 2.6c allocation-free closeout.
