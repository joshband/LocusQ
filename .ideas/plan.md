Title: LocusQ Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-02-19

# LocusQ - Implementation Plan

**Complexity Score: 5 / 5**
**Strategy: Phased Implementation (6 phases)**
**UI Framework: WebView (Three.js for 3D)**

---

## Implementation Strategy

Given the extreme complexity, LocusQ requires strict phased implementation with integration checkpoints. Each phase produces a testable, runnable artifact. No phase begins until the previous phase compiles, runs, and passes its acceptance criteria.

### Planning Decision Package (skill_plan, 2026-02-19)

- [x] v1 scope contract frozen in `.ideas/creative-brief.md` (`In Scope` / `Out of Scope`)
- [x] ADR-0002 accepted: v1 routing model (canonical metadata state + same-block audio fast path)
- [x] ADR-0003 accepted: deterministic authority precedence (DAW/APVTS -> timeline rest pose -> physics offset)
- [x] ADR-0004 accepted: AI orchestration deferred from v1 critical path
- [x] ADR-0005 accepted: phase-closeout docs freshness gate (status/evidence/readme/changelog sync contract)
- [x] Scene state contract authored: `Documentation/scene-state-contract.md`

These decisions are mandatory inputs for `skill_design` and `skill_impl` execution and must be treated as normative alongside invariants and parameter/architecture specs.

---

## Phase 2.1: Foundation & Scene Graph
**Goal:** Plugin shell, mode switching, scene graph singleton, basic parameter tree.

### Tasks
- [x] `PluginProcessor.h/cpp` — AudioProcessor with mode parameter, channel config per mode
- [x] `SceneGraph.h/cpp` — Singleton with EmitterSlot array, double-buffered atomic read/write
- [x] `EmitterSlot` struct — Position, size, gain, spread, directivity, velocity, label, color, audio buffer pointer
- [x] Registration/deregistration — `registerEmitter()`, `unregisterEmitter()`, `registerRenderer()`
- [x] Parameter tree — All 76 parameters declared, organized by mode prefix
- [x] Mode gating — Only relevant parameters visible/active per mode
- [x] State serialization — Save/restore mode + parameters to DAW state
- [x] `PluginEditor.h/cpp` — Basic WebView shell, loads placeholder HTML

### Acceptance Criteria
- [x] Plugin loads in DAW without crash
- [x] Mode can be switched between Calibrate/Emitter/Renderer
- [x] Two Emitter instances + one Renderer instance coexist in same DAW session
- [x] Emitter writes to SceneGraph, Renderer reads — verified with debug logging
- [x] Audio passes through Emitter mode (passthrough)

### Estimated Class Count: 5
`PluginProcessor`, `PluginEditor`, `SceneGraph`, `EmitterSlot`, `RoomProfile`

---

## Phase 2.2: Spatialization Core
**Goal:** Renderer produces quad output from emitter positions. Basic panning and distance.

### Tasks
- [x] `VBAPPanner.h/cpp` — 2D VBAP for quad speaker layout (4 speaker pairs)
- [x] `DistanceAttenuator.h/cpp` — InverseSquare/Linear/Log models with reference distance
- [x] `AirAbsorption.h/cpp` — One-pole LPF per emitter, distance-driven cutoff
- [x] `SpatialRenderer.h/cpp` — Orchestrates per-emitter chain, accumulates to 4-ch
- [x] Speaker compensation — Per-speaker delay lines and gain trims
- [x] Emitter audio routing — Emitter publishes audio buffer pointer in SceneGraph slot
- [x] Renderer audio consumption — Reads emitter audio from SceneGraph, applies spatialization
- [x] Smoothing — All gain/position changes use parameter smoothing (no clicks)

### Acceptance Criteria
- [x] Emitter on Track 1 + Renderer on Master → audio comes from correct speaker based on azimuth
- [x] Moving an emitter smoothly pans audio between speakers
- [x] Distance changes produce gain attenuation
- [x] Two emitters produce independently spatialized audio
- [x] No clicks, pops, or artifacts during position changes

### Estimated Class Count: 4
`VBAPPanner`, `DistanceAttenuator`, `AirAbsorption`, `SpatialRenderer`

---

## Phase 2.3: Room Calibration
**Goal:** Calibrate mode measures room and generates a Room Profile.

### Tasks
- [x] `TestSignalGenerator.h/cpp` — Log sweep, pink noise, white noise, impulse generators
- [x] `IRCapture.h/cpp` — Records mic input during test, deconvolves sweep to extract IR
- [x] `RoomAnalyzer.h/cpp` — Extracts delay, level, frequency response, early reflections from IR
- [x] `RoomProfileSerializer.h/cpp` — Save/load Room Profile as JSON
- [x] Calibration state machine — Idle → Measuring (speaker 1-4 sequentially) → Complete
- [x] UI integration — Calibration wizard in WebView (step-by-step guidance)
- [x] Room Profile → SceneGraph — Atomic publish of profile data
- [x] Speaker position visualization — Show measured positions in 3D view

### Acceptance Criteria
- [x] Calibration runs sweep through each speaker sequentially
- [x] Captures IR from mic input for each speaker
- [x] Calculates delay compensation values that match physical speaker distances
- [x] Saves Room Profile to JSON file
- [x] Loads Room Profile in Emitter/Renderer modes
- [x] Speaker delay compensation audibly corrects timing misalignment

### Estimated Class Count: 4
`TestSignalGenerator`, `IRCapture`, `RoomAnalyzer`, `RoomProfileSerializer`

---

## Phase 2.4: Physics Engine
**Goal:** Objects move physically in 3D space with forces, collisions, and damping.

### Tasks
- [x] `PhysicsEngine.h/cpp` — Timer-driven simulation loop (30-240 Hz tick rate)
- [x] `PhysicsBody` struct — Mass, velocity, acceleration, forces, elasticity, drag, friction
- [x] Force integration — Euler integration
- [x] Gravity — Configurable direction and magnitude
- [x] Drag — Velocity-proportional damping
- [x] Wall collision — AABB collision with room boundaries, reflection + elasticity
- [x] Friction — Applied during wall contact, reduces tangential velocity
- [x] Throw trigger — One-shot impulse from initial velocity parameters
- [x] Reset trigger — Returns to keyframed/manual position, zeroes velocity
- [x] Thread safety — Physics thread writes to double-buffered slot, audio thread reads
- [x] Physics pause — Global freeze toggle from Renderer

### Acceptance Criteria
- [x] Throw an object -> it moves, bounces off walls, gradually stops
- [x] Gravity pulls object down (or in configured direction)
- [x] Drag slows motion over time
- [x] Elasticity controls bounce energy retention
- [x] Zero-G mode: object drifts indefinitely (drag = 0, gravity = 0)
- [x] Physics position feeds into spatialization - audio moves with the object
- [x] No glitches or audio artifacts from physics position updates

Acceptance closeout evidence (2026-02-19):
- Deterministic physics probe (`5/5` checks pass): `plugins/LocusQ/TestEvidence/locusq_phase_2_4_physics_probe_closeout.log`
- Physics spatial-motion scenario (`PASS`): `plugins/LocusQ/TestEvidence/locusq_24_physics_spatial_motion_closeout.log`
- Physics zero-g drift scenario (`PASS`): `plugins/LocusQ/TestEvidence/locusq_24_physics_zero_g_drift_closeout.log`
- Phase 2.4 acceptance suite rollup (`2 PASS / 0 WARN / 0 FAIL`): `plugins/LocusQ/TestEvidence/locusq_phase_2_4_acceptance_suite_closeout.log`

### Estimated Class Count: 2
`PhysicsEngine`, `PhysicsBody`

---

## Phase 2.5: Room Acoustics & Advanced DSP
**Goal:** Reverb, early reflections, doppler, directivity, size/spread.

### Tasks
- [x] `EarlyReflections.h/cpp` — Multi-tap early-reflections delay network integrated in renderer
  - Draft: 8 taps per speaker
  - Final: 16 taps per speaker
- [x] `FDNReverb.h/cpp` — Feedback Delay Network integrated in renderer
  - 4x4 Hadamard-like mixing matrix
  - Room-size and damping controls wired
- [x] `DopplerProcessor.h/cpp` — Draft variable-delay doppler processor
  - Reads emitter velocity from physics state
  - Doppler scale parameter wired
- [x] `DirectivityFilter.h/cpp` — Cardioid-like radiation pattern
  - Gain = f(angle between emitter aim and speaker direction)
- [x] `SpreadProcessor.h/cpp` — Focused-to-diffuse distribution blend
  - Point source (spread=0): VBAP
  - Diffuse (spread=1): equal-power quad blend
- [x] Quality tier switching — Draft ↔ Final taps/delay configuration

### Acceptance Criteria
- [x] Early reflections add spatial depth without coloring the sound
- [x] FDN reverb sounds natural and matches room size
- [x] Doppler produces audible pitch shift when object moves fast
- [x] Directivity narrows sound toward aim direction
- [x] Spread smoothly transitions from focused to diffuse
- [x] Draft → Final produces audibly enhanced but tonally consistent result
- [x] CPU usage for full chain < 15% per Renderer instance (Draft mode) (hard gate passed; allocation tracking remains a warning trend)

### Estimated Class Count: 5
`EarlyReflections`, `FDNReverb`, `DopplerProcessor`, `DirectivityFilter`, `SpreadProcessor`

---

## Phase 2.6: Keyframe Animation & Polish
**Goal:** Internal timeline, transport sync, and final integration.

### Tasks
- [x] `KeyframeTimeline.h/cpp` — Multi-track keyframe container
- [x] `KeyframeTrack` — Per-parameter keyframe sequence with interpolation
- [x] Interpolation curves — Linear, EaseIn, EaseOut, EaseInOut, Step
- [x] Transport sync — Read DAW playhead position, sync internal clock
- [x] Loop mode — Configurable loop with optional ping-pong
- [x] Physics + Keyframe interaction — Keyframed position as rest point, physics as offset
- [x] Keyframe editor UI — Timeline component in WebView with drag-to-edit (add/move/delete/curve cycle, transport controls)
- [x] Preset system — Save/load emitter spatial presets (position + animation + physics timeline)
- [x] Performance profiling — CPU measurement telemetry exposed (`perfBlockMs`, `perfEmitterMs`, `perfRendererMs`)
- [x] Edge cases — Plugin removal mid-playback, DAW crash recovery, sample-rate/change host-matrix validation (harness multi-pass roundtrip + pluginval lifecycle + SR/BS matrix)

Status note (2026-02-19): Phase 2.6 acceptance/tuning is closed with allocation-free criteria met in the full-system scenario (`perf_allocation_free=true`, `perf_total_allocations=0`). CPU/deadline thresholds remain passed (`perf_avg_block_time_ms=0.304457`, `perf_p95_block_time_ms=0.318466`, `perf_meets_deadline=true`). Host edge lifecycle matrix remains pass across `44.1k/256`, `48k/512`, `48k/1024`, and `96k/512`, with plugin build/load checks stable.

### Acceptance Criteria
- [x] Keyframes animate position over time, synchronized with DAW transport/internal clock
- [x] Loop plays continuously with smooth wraparound
- [x] Physics forces add to keyframed position (not replace)
- [x] Keyframe editor is usable: add/move/delete keyframes, change curves
- [x] Full system test: 8 Emitters + 1 Renderer, physics active, < 25% total CPU (Draft) and allocation-free pass criteria met

### Estimated Class Count: 3
`KeyframeTimeline`, `KeyframeTrack`, `KeyframeInterpolator`

---

## Phase 2.7: UI-Engine Integration Recovery (Post-Ship Reopen)
**Goal:** Restore real host interactivity by reconnecting WebView controls, viewport runtime, and native command acknowledgments.

### Why This Phase Exists
- Host evidence indicates near-static UI behavior (hover-only) despite successful DSP/build/load gates.
- Current acceptance stack is strong on DSP/host loading but weak on in-host interaction viability.
- No further feature expansion should occur until control-path and viewport-path connectivity are proven.

### Tasks
- [x] Bootstrap hardening:
  - Make control bindings independent from viewport initialization.
  - Add viewport-failure degraded mode with visible diagnostics.
- [x] Bridge handshake and acknowledgments:
  - Add command/ack response contract for tab/mode changes, toggles, dropdowns, text edits, timeline edits, calibration start/abort.
  - Surface explicit errors for rejected/unhandled commands.
- [x] Viewport interactivity:
  - Implement emitter pick/select/move interaction path (raycast or equivalent) wired to APVTS/native state.
  - Keep camera controls operational and bounded (orbit/pan/zoom).
- [x] Calibration interaction and visualization:
  - Validate start/abort controls and status/progress rows against native event stream.
  - Ensure speaker progress and capture meters are state-driven, not cosmetic.
- [x] Mode overlay coherence:
  - Ensure mode tabs, rail content, viewport overlays, and scene counters update from one canonical state source.
- [x] UI acceptance evidence:
  - Add focused UI interaction matrix and publish trend deltas with command logs and pass/fail table.

Status note (2026-02-19): Phase 2.7a bootstrap hardening landed in `Source/ui/public/js/index.js` with resilient startup ordering and guarded optional DOM/viewport access. Phase 2.7b viewport/cali visualization wiring landed with state-backed emitter pick/select/move, APVTS drag updates, local-emitter identity in scene snapshots, and calibration speaker-level/profile visualization sourced from native status payloads (`Source/PluginProcessor.cpp`, `Source/ui/public/js/index.js`). Phase 2.7c wired the remaining control rail paths (tabs/toggles/dropdowns/value steppers) through relays/attachments and added native UI-state persistence (`locusqGetUiState`, `locusqSetUiState`) for emitter label + physics preset memory. Phase 2.7d host-interaction closure prep hardened Cartesian viewport movement by wiring `pos_x/pos_y/pos_z` relays/attachments plus JS writeback during drag, and added finite/clamped guards in `setTimelineCurrentTimeFromUI`. Headless checks are green (JS syntax, VST3+QA build, smoke suite, animation smoke, phase 2.6 acceptance suite, host-edge 48k/512). Manual host verification checklist is staged at `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` and is still pending for final in-host click/edit acceptance.

### Acceptance Criteria
- [ ] Mode tabs function in-host and update processor mode deterministically.
- [ ] Rail controls (toggles/dropdowns/text edits) mutate real plugin state; no silent no-op actions.
- [x] Timeline controls (play/stop/rewind, keyframe add/move/delete/curve) roundtrip to native timeline state.
- [ ] Viewport renders room/emitter scene or explicitly enters degraded mode while keeping rail fully functional.
- [ ] Emitter selection/movement in viewport updates APVTS + scene snapshot consistently.
- [ ] Calibration start/abort/status flow is operational and visually coherent.
- [ ] UI interaction matrix passes in target host(s) with evidence added to `TestEvidence`.

### Manual-Host-Only Remaining (2.7d)
- DAW-embedded tab switching and rail interactions must be manually verified for focus/automation behavior in target hosts.
- Viewport drag in Cartesian mode must be manually verified in host to confirm no snapback/regression under embedded WebView input routing.
- Calibration `START/ABORT` lifecycle must be manually verified with real DAW I/O routing and room-profile persistence.

### Command Sequence (APC)
1. `/plan LocusQ integration-recovery package for UI-runtime reconnection`
2. `/design LocusQ interaction contract for viewport + rail + mode overlays`
3. `/impl LocusQ phase 2.7a bootstrap hardening + bridge ack path`
4. `/test LocusQ UI interaction smoke matrix and trend deltas`
5. `/impl LocusQ phase 2.7b viewport selection/movement + calibration visualization`
6. `/impl LocusQ phase 2.7c ui-control wiring/state sync`
7. `/test LocusQ full acceptance rerun (DSP + host + UI matrix)`

---

## Risk Assessment

### Critical Risk (must solve or project fails)
- **Inter-instance audio sharing:** Emitter must pass its audio buffer to Renderer through the scene graph within a single `processBlock` cycle. This requires that the DAW processes Emitters before the Renderer in the same audio callback. Most DAWs do this naturally (track → master routing), but must be verified per host.
- **Lock-free scene graph:** Any contention on the audio thread causes glitches. Double-buffer atomic swap must be bulletproof.

### High Risk
- **Calibration accuracy:** IR deconvolution and analysis quality directly determines the usefulness of Room Profiles. May need iterative refinement.
- **Physics ↔ audio sync:** Physics runs on a timer thread at a different rate than audio. Position interpolation between physics ticks must be smooth enough to avoid zipper noise.
- **CPU budget:** 8 emitters × full DSP chain + physics + reverb + 3D visualization. Draft mode must stay under 25% CPU on a modern machine.

### Medium Risk
- **WebView ↔ C++ latency:** Scene state serialization to JSON and push to WebView at 30-60fps. If too heavy, may need binary serialization or reduced update rate.
- **DAW compatibility:** Scene graph singleton behavior across DAW hosts (some may use separate processes for plugin scanning).
- **Doppler quality:** Variable-rate delay with fractional interpolation can produce artifacts if not carefully implemented.

### Low Risk
- **VBAP panning:** Well-documented algorithm, straightforward implementation for quad.
- **Distance attenuation:** Simple gain calculations.
- **Parameter system:** Standard JUCE AudioParameterFloat/Bool/Choice, well-supported.

---

## Total Estimated Classes: ~23

| Category | Classes | Count |
|----------|---------|-------|
| Plugin Shell | PluginProcessor, PluginEditor | 2 |
| Scene Graph | SceneGraph, EmitterSlot, RoomProfile | 3 |
| Calibration | TestSignalGenerator, IRCapture, RoomAnalyzer, RoomProfileSerializer | 4 |
| Physics | PhysicsEngine, PhysicsBody | 2 |
| Spatialization | SpatialRenderer, VBAPPanner, DistanceAttenuator, AirAbsorption | 4 |
| Room Acoustics | EarlyReflections, FDNReverb | 2 |
| Advanced DSP | DopplerProcessor, DirectivityFilter, SpreadProcessor | 3 |
| Animation | KeyframeTimeline, KeyframeTrack, KeyframeInterpolator | 3 |
| **Total** | | **23** |

---

## UI Framework Decision

### Decision: WebView

### Rationale
1. **3D Visualization** — Three.js provides WebGL-accelerated 3D rendering with minimal effort. Wireframe rooms, clickable objects, camera orbit, trails, vectors — all achievable in JavaScript with Three.js primitives. Equivalent in native JUCE OpenGL would require writing a complete 3D scene graph from scratch.
2. **Complex Multi-Mode UI** — Calibration wizard, parameter panels, keyframe timeline editor, 3D viewport — these are fundamentally UI-heavy. HTML/CSS/JS excels at layout, theming, and rapid iteration.
3. **Modularity** — The user explicitly requested the 3D layer be "modular and replaceable." WebView naturally isolates the visualization layer behind a message bridge. Swapping Three.js for a different renderer (or even native OpenGL later) only requires reimplementing the JS side.
4. **Interactivity** — Dragging objects in 3D space, scrubbing a timeline, wizard step transitions — all natural in web tech, painful in native JUCE components.
5. **Aesthetic** — Clean minimal wireframe is trivially achievable in Three.js with `THREE.LineSegments`, `THREE.WireframeGeometry`, and `THREE.GridHelper`.

### Trade-offs Accepted
- **Latency:** WebView introduces ~1-2 frames of visual latency vs native rendering. Acceptable for visualization (not audio-critical).
- **Memory:** WebView process overhead (~30-50MB). Acceptable for a complex plugin.
- **Startup:** Initial WebView load is slower than native UI. Mitigated by showing a loading indicator.

---

## Implementation Status Update (2026-02-19)

This plan file was updated to reflect as-built progress so it does not drift from implementation reality.

### Completed and validated
- Phase 2.1 foundation: integrated and build-stable.
- Phase 2.2 spatialization core: integrated and smoke-suite validated.
- Phase 2.3 room calibration: implemented and wired through processor/editor/UI bridge.
- Phase 2.4 physics engine: acceptance criteria closed with deterministic probe + QA suite evidence.
- Phase 2.5 room acoustics and advanced DSP: acceptance suite executed with hard gates passing.
- Phase 2.6 acceptance/tuning: keyframe timeline editor interactions, emitter preset save/load, timeline state persistence, and per-block/per-stage perf telemetry implemented and validated.

### Validation evidence
- Harness ctest: `plugins/LocusQ/TestEvidence/harness_ctest.log` (45/45 pass)
- Phase 2.4 configure/build logs:
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_4_closeout_configure.log`
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_4_closeout_build.log`
- Phase 2.4 deterministic probe: `plugins/LocusQ/TestEvidence/locusq_phase_2_4_physics_probe_closeout.log` (`5/5` checks pass)
- Phase 2.4 physics spatial motion: `plugins/LocusQ/TestEvidence/locusq_24_physics_spatial_motion_closeout.log` (`PASS`)
- Phase 2.4 zero-g drift motion: `plugins/LocusQ/TestEvidence/locusq_24_physics_zero_g_drift_closeout.log` (`PASS`)
- Phase 2.4 acceptance suite: `plugins/LocusQ/TestEvidence/locusq_phase_2_4_acceptance_suite_closeout.log` (`2 PASS / 0 WARN / 0 FAIL`)
- Phase 2.6 QA build: `plugins/LocusQ/TestEvidence/locusq_qa_build_phase_2_6c_allocation_free.log`
- Full-system CPU gate (`8E+1R`, 48k/512): `plugins/LocusQ/TestEvidence/locusq_26_full_system_cpu_draft_phase_2_6c_allocation_free.log` (`PASS`, allocation-free)
- Host edge matrix (multi-pass roundtrip):
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_44k1_256_phase_2_6_acceptance_refresh.log`
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_48k512_phase_2_6_acceptance_refresh.log`
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_48k1024_phase_2_6_acceptance_refresh.log`
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_96k512_phase_2_6_acceptance_refresh.log`
- Phase 2.6 acceptance suite rollup: `plugins/LocusQ/TestEvidence/locusq_phase_2_6_acceptance_suite_phase_2_6c_allocation_free_refresh.log` (`3 PASS / 0 WARN / 0 FAIL`)
- Plugin build/load checks: `plugins/LocusQ/TestEvidence/locusq_build_phase_2_6c_vst3.log`, `plugins/LocusQ/TestEvidence/pluginval_test_full_acceptance_rerun_20260219T044525Z_stdout.log`, `plugins/LocusQ/TestEvidence/standalone_test_full_acceptance_rerun_20260219T044525Z.log`
- pluginval GUI-context success: `plugins/LocusQ/TestEvidence/pluginval_exit_code.txt`, `plugins/LocusQ/TestEvidence/pluginval_stdout.log`
- Additional blocked-run diagnostics: `plugins/LocusQ/TestEvidence/pluginval_blocked_note.txt`

### Current active work
- Execute manual host UI checklist for Phase 2.7 closeout.
- Route follow-on validation to `/test` and CI harness automation integration.
- Maintain ADR/docs freshness gate compliance on each phase closeout.
