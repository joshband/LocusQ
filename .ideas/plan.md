# LocusQ - Implementation Plan

**Complexity Score: 5 / 5**
**Strategy: Phased Implementation (6 phases)**
**UI Framework: WebView (Three.js for 3D)**

---

## Implementation Strategy

Given the extreme complexity, LocusQ requires strict phased implementation with integration checkpoints. Each phase produces a testable, runnable artifact. No phase begins until the previous phase compiles, runs, and passes its acceptance criteria.

---

## Phase 2.1: Foundation & Scene Graph
**Goal:** Plugin shell, mode switching, scene graph singleton, basic parameter tree.

### Tasks
- [ ] `PluginProcessor.h/cpp` — AudioProcessor with mode parameter, channel config per mode
- [ ] `SceneGraph.h/cpp` — Singleton with EmitterSlot array, double-buffered atomic read/write
- [ ] `EmitterSlot` struct — Position, size, gain, spread, directivity, velocity, label, color, audio buffer pointer
- [ ] Registration/deregistration — `registerEmitter()`, `unregisterEmitter()`, `registerRenderer()`
- [ ] Parameter tree — All 76 parameters declared, organized by mode prefix
- [ ] Mode gating — Only relevant parameters visible/active per mode
- [ ] State serialization — Save/restore mode + parameters to DAW state
- [ ] `PluginEditor.h/cpp` — Basic WebView shell, loads placeholder HTML

### Acceptance Criteria
- [ ] Plugin loads in DAW without crash
- [ ] Mode can be switched between Calibrate/Emitter/Renderer
- [ ] Two Emitter instances + one Renderer instance coexist in same DAW session
- [ ] Emitter writes to SceneGraph, Renderer reads — verified with debug logging
- [ ] Audio passes through Emitter mode (passthrough)

### Estimated Class Count: 5
`PluginProcessor`, `PluginEditor`, `SceneGraph`, `EmitterSlot`, `RoomProfile`

---

## Phase 2.2: Spatialization Core
**Goal:** Renderer produces quad output from emitter positions. Basic panning and distance.

### Tasks
- [ ] `VBAPPanner.h/cpp` — 2D VBAP for quad speaker layout (4 speaker pairs)
- [ ] `DistanceAttenuator.h/cpp` — InverseSquare/Linear/Log models with reference distance
- [ ] `AirAbsorption.h/cpp` — One-pole LPF per emitter, distance-driven cutoff
- [ ] `SpatialRenderer.h/cpp` — Orchestrates per-emitter chain, accumulates to 4-ch
- [ ] Speaker compensation — Per-speaker delay lines and gain trims
- [ ] Emitter audio routing — Emitter publishes audio buffer pointer in SceneGraph slot
- [ ] Renderer audio consumption — Reads emitter audio from SceneGraph, applies spatialization
- [ ] Smoothing — All gain/position changes use parameter smoothing (no clicks)

### Acceptance Criteria
- [ ] Emitter on Track 1 + Renderer on Master → audio comes from correct speaker based on azimuth
- [ ] Moving an emitter smoothly pans audio between speakers
- [ ] Distance changes produce gain attenuation
- [ ] Two emitters produce independently spatialized audio
- [ ] No clicks, pops, or artifacts during position changes

### Estimated Class Count: 4
`VBAPPanner`, `DistanceAttenuator`, `AirAbsorption`, `SpatialRenderer`

---

## Phase 2.3: Room Calibration
**Goal:** Calibrate mode measures room and generates a Room Profile.

### Tasks
- [ ] `TestSignalGenerator.h/cpp` — Log sweep, pink noise, white noise, impulse generators
- [ ] `IRCapture.h/cpp` — Records mic input during test, deconvolves sweep to extract IR
- [ ] `RoomAnalyzer.h/cpp` — Extracts delay, level, frequency response, early reflections from IR
- [ ] `RoomProfileSerializer.h/cpp` — Save/load Room Profile as JSON
- [ ] Calibration state machine — Idle → Measuring (speaker 1-4 sequentially) → Complete
- [ ] UI integration — Calibration wizard in WebView (step-by-step guidance)
- [ ] Room Profile → SceneGraph — Atomic publish of profile data
- [ ] Speaker position visualization — Show measured positions in 3D view

### Acceptance Criteria
- [ ] Calibration runs sweep through each speaker sequentially
- [ ] Captures IR from mic input for each speaker
- [ ] Calculates delay compensation values that match physical speaker distances
- [ ] Saves Room Profile to JSON file
- [ ] Loads Room Profile in Emitter/Renderer modes
- [ ] Speaker delay compensation audibly corrects timing misalignment

### Estimated Class Count: 4
`TestSignalGenerator`, `IRCapture`, `RoomAnalyzer`, `RoomProfileSerializer`

---

## Phase 2.4: Physics Engine
**Goal:** Objects move physically in 3D space with forces, collisions, and damping.

### Tasks
- [ ] `PhysicsEngine.h/cpp` — Timer-driven simulation loop (30-240 Hz tick rate)
- [ ] `PhysicsBody` struct — Mass, velocity, acceleration, forces, elasticity, drag, friction
- [ ] Force integration — Euler or Verlet integration (Verlet preferred for stability)
- [ ] Gravity — Configurable direction and magnitude
- [ ] Drag — Velocity-proportional damping
- [ ] Wall collision — AABB collision with room boundaries, reflection + elasticity
- [ ] Friction — Applied during wall contact, reduces tangential velocity
- [ ] Throw trigger — One-shot impulse from initial velocity parameters
- [ ] Reset trigger — Returns to keyframed/manual position, zeroes velocity
- [ ] Thread safety — Physics thread writes to double-buffered slot, audio thread reads
- [ ] Physics pause — Global freeze toggle from Renderer

### Acceptance Criteria
- [ ] Throw an object → it moves, bounces off walls, gradually stops
- [ ] Gravity pulls object down (or in configured direction)
- [ ] Drag slows motion over time
- [ ] Elasticity controls bounce energy retention
- [ ] Zero-G mode: object drifts indefinitely (drag = 0, gravity = 0)
- [ ] Physics position feeds into spatialization — audio moves with the object
- [ ] No glitches or audio artifacts from physics position updates

### Estimated Class Count: 2
`PhysicsEngine`, `PhysicsBody`

---

## Phase 2.5: Room Acoustics & Advanced DSP
**Goal:** Reverb, early reflections, doppler, directivity, size/spread.

### Tasks
- [ ] `EarlyReflections.h/cpp` — FIR tapped delay line from Room Profile reflection data
  - Draft: 4-8 taps per speaker
  - Final: 16-32 taps with frequency-dependent absorption per tap
- [ ] `FDNReverb.h/cpp` — Feedback Delay Network, 4x4 (Draft) / 8x8 (Final)
  - Hadamard mixing matrix
  - RT60 and damping from Room Profile
  - Per-channel modulated delays for diffusion
- [ ] `DopplerProcessor.h/cpp` — Variable-rate delay line with fractional interpolation
  - Reads velocity from physics/animation
  - Calculates per-speaker relative velocity
  - Scale factor parameter
- [ ] `DirectivityFilter.h/cpp` — Cardioid-like radiation pattern
  - Gain = f(angle between emitter aim and speaker direction)
  - Smooth interpolation for rotation
- [ ] `SpreadProcessor.h/cpp` — Decorrelation + multi-point distribution
  - Point source (spread=0): single position VBAP
  - Diffuse (spread=1): decorrelated signal to all speakers
  - Intermediate: blend of focused and diffuse
- [ ] Quality tier switching — Draft ↔ Final changes processing depth

### Acceptance Criteria
- [ ] Early reflections add spatial depth without coloring the sound
- [ ] FDN reverb sounds natural and matches room size
- [ ] Doppler produces audible pitch shift when object moves fast
- [ ] Directivity narrows sound toward aim direction
- [ ] Spread smoothly transitions from focused to diffuse
- [ ] Draft → Final produces audibly enhanced but tonally consistent result
- [ ] CPU usage for full chain < 15% per Renderer instance (Draft mode)

### Estimated Class Count: 5
`EarlyReflections`, `FDNReverb`, `DopplerProcessor`, `DirectivityFilter`, `SpreadProcessor`

---

## Phase 2.6: Keyframe Animation & Polish
**Goal:** Internal timeline, transport sync, and final integration.

### Tasks
- [ ] `KeyframeTimeline.h/cpp` — Multi-track keyframe container
- [ ] `KeyframeTrack` — Per-parameter keyframe sequence with interpolation
- [ ] Interpolation curves — Linear, EaseIn, EaseOut, EaseInOut, Step
- [ ] Transport sync — Read DAW playhead position, sync internal clock
- [ ] Loop mode — Configurable loop with optional ping-pong
- [ ] Physics + Keyframe interaction — Keyframed position as rest point, physics as offset
- [ ] Keyframe editor UI — Timeline component in WebView with drag-to-edit
- [ ] Preset system — Save/load emitter spatial presets (position + animation + physics)
- [ ] Performance profiling — CPU measurement per component, optimization pass
- [ ] Edge cases — Plugin removal mid-playback, DAW crash recovery, sample rate changes

### Acceptance Criteria
- [ ] Keyframes animate position over time, synchronized with DAW transport
- [ ] Loop plays continuously with smooth wraparound
- [ ] Physics forces add to keyframed position (not replace)
- [ ] Keyframe editor is usable: add/move/delete keyframes, change curves
- [ ] Full system test: 8 Emitters + 1 Renderer, physics active, < 25% total CPU (Draft)

### Estimated Class Count: 3
`KeyframeTimeline`, `KeyframeTrack`, `KeyframeInterpolator`

---

## Phase 2.7: Output Format Expansion (Headphones, Stereo, Binaural)
**Goal:** Support headphone monitoring, stereo-only output, and binaural rendering.

### Tasks
- [ ] `OutputFormatManager.h/cpp` — Manages output mode: Quad / Stereo / Binaural / 5.1.2 / 7.1.4
- [ ] `BinauralRenderer.h/cpp` — HRTF convolution engine for headphone output
  - MIT KEMAR or SADIE II free HRTF dataset (embedded or loadable)
  - Per-emitter HRTF pair selection based on azimuth/elevation
  - Crossfade between HRTF filters during movement
- [ ] `StereoDownmixer.h/cpp` — Quad-to-stereo fold-down with adjustable width
- [ ] `rend_output_format` parameter — Choice: Quad / Stereo / Binaural / 5.1.2 / 7.1.4
- [ ] `rend_hrtf_set` parameter — Choice of HRTF dataset
- [ ] Channel configuration update — `isBusesLayoutSupported()` adapts to output format
- [ ] WebView UI — Output format selector in Renderer rail

### Acceptance Criteria
- [ ] Stereo output produces a convincing spatial image on headphones
- [ ] Binaural HRTF accurately places sounds in perceived 3D space
- [ ] Switching between Quad and Binaural preserves spatial intent
- [ ] No clicks or artifacts during format switching

### Estimated Class Count: 3
`OutputFormatManager`, `BinauralRenderer`, `StereoDownmixer`

---

## Phase 2.8: Immersive Format Support (5.1.2, 7.1.4, Atmos Bed)
**Goal:** Extend spatialization to height-enabled speaker layouts and Dolby Atmos-compatible output.

### Tasks
- [ ] `SpeakerLayout.h/cpp` — Configurable speaker layout definitions (Quad, 5.1, 5.1.2, 7.1.4)
- [ ] VBAP extension — 3D VBAP with triangulated speaker triplets for height layouts
- [ ] `AtmosBedRenderer.h/cpp` — Outputs channel-bed audio compatible with DAW Atmos renderers
  - Outputs positioned audio as multi-channel bed (no Dolby license required)
  - Compatible with Logic Pro Atmos, Nuendo, Reaper ATMOS plugin
- [ ] ADM metadata export — Optional BWF/ADM object position metadata for offline render
- [ ] Layout calibration — Extend calibration to support > 4 speakers
- [ ] `rend_speaker_layout` parameter — Choice of speaker configuration

### Acceptance Criteria
- [ ] 5.1.2 output correctly places sounds with height information
- [ ] 7.1.4 output distributes full 3D spatial field
- [ ] Audio routed through Logic Pro / Reaper Atmos renderer produces correct spatialization
- [ ] No Dolby licensing required for channel-bed output path

### Notes
- Dolby Atmos Renderer SDK licensing ($$$) is **not required** for channel-bed output
- Object-based Atmos (`.atmos` master) requires Dolby Production Suite — this is a DAW-side tool
- LocusQ outputs correctly positioned channel-bed audio; the DAW's Atmos renderer handles encoding

### Estimated Class Count: 3
`SpeakerLayout`, `AtmosBedRenderer`, `ADMExporter`

---

## Phase 2.9: Headphone Spatial Audio SDK Integration
**Goal:** Support Apple Spatial Audio (AirPods Pro) and Sony 360 Reality Audio headphone tracking.

### Tasks
- [ ] `HeadTrackingBridge.h/cpp` — Abstract interface for head tracking data input
- [ ] `AppleSpatialAudioBridge.h/cpp` — macOS/iOS: CoreAudio `AUSpatialMixer` or `PHASEEngine` integration
  - Reads head orientation from AirPods Pro 2/3 via CoreMotion
  - Adjusts listener orientation in scene graph based on head tracking
  - Outputs Spatial Audio compatible channel layout
- [ ] `Sony360RABridge.h/cpp` — Sony 360 Reality Audio SDK integration (if SDK available)
  - Reads head tracking data from WH-1000XM5 via Bluetooth LE
  - Maps to listener orientation in scene graph
- [ ] Platform gating — Apple bridge only built on macOS; Sony bridge cross-platform
- [ ] `rend_headtracking` parameter — Toggle: Off / Apple / Sony / Generic
- [ ] `rend_headtracking_smoothing` parameter — Smoothing time for head movement
- [ ] CMake platform guards — Conditional compilation per platform

### Acceptance Criteria
- [ ] AirPods Pro head rotation is reflected in spatialization in real-time
- [ ] Sound field stays stable when head turns (world-locked)
- [ ] Smooth interpolation prevents artifacts during rapid head movement
- [ ] Graceful degradation when no head tracking device is connected

### Platform Requirements
- Apple Spatial Audio: macOS 12+, CoreAudio, CoreMotion frameworks
- Sony 360 RA: Sony 360 Reality Audio SDK (availability TBD)
- Fallback: Manual head orientation parameter for non-tracked headphones

### Estimated Class Count: 3
`HeadTrackingBridge`, `AppleSpatialAudioBridge`, `Sony360RABridge`

---

## Phase 2.10: QA Harness Integration
**Goal:** Integrate joshband/audio-dsp-qa-harness for automated DSP validation and regression testing.

### Tasks
- [ ] Add `audio-dsp-qa-harness` as submodule or CMake dependency
- [ ] `tests/` directory — Automated test suite using QA harness
- [ ] Test fixtures — Pre-defined scene configurations (single emitter, multi-emitter, physics)
- [ ] Spatial accuracy tests — Verify VBAP gain coefficients against known positions
- [ ] Latency measurement — Round-trip latency through Emitter → SceneGraph → Renderer chain
- [ ] CPU profiling tests — Automated benchmarks for Draft/Final mode at 8/16/32 emitters
- [ ] Regression suite — Capture audio output snapshots, diff against baseline
- [ ] CI integration — GitHub Actions workflow runs harness on push
- [ ] Room calibration validation — Test IR analysis against synthetic IRs with known properties

### Acceptance Criteria
- [ ] `ctest` runs full suite and reports pass/fail
- [ ] Spatial panning tests verify correct speaker gains within 1% tolerance
- [ ] Performance regression detected if CPU exceeds baseline by > 10%
- [ ] CI pipeline runs on push, blocks merge on failure

### Estimated Class Count: 0 (test-only, no production classes)

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

## Total Estimated Classes: ~32

| Category | Classes | Count | Phase |
|----------|---------|-------|-------|
| Plugin Shell | PluginProcessor, PluginEditor | 2 | 2.1 |
| Scene Graph | SceneGraph, EmitterSlot, RoomProfile | 3 | 2.1 |
| Spatialization | SpatialRenderer, VBAPPanner, DistanceAttenuator, AirAbsorption | 4 | 2.2 |
| Calibration | TestSignalGenerator, IRCapture, RoomAnalyzer, RoomProfileSerializer | 4 | 2.3 |
| Physics | PhysicsEngine, PhysicsBody | 2 | 2.4 |
| Room Acoustics | EarlyReflections, FDNReverb | 2 | 2.5 |
| Advanced DSP | DopplerProcessor, DirectivityFilter, SpreadProcessor | 3 | 2.5 |
| Animation | KeyframeTimeline, KeyframeTrack, KeyframeInterpolator | 3 | 2.6 |
| Output Formats | OutputFormatManager, BinauralRenderer, StereoDownmixer | 3 | 2.7 |
| Immersive | SpeakerLayout, AtmosBedRenderer, ADMExporter | 3 | 2.8 |
| Head Tracking | HeadTrackingBridge, AppleSpatialAudioBridge, Sony360RABridge | 3 | 2.9 |
| **Total** | | **32** |

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
