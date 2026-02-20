Title: LocusQ DSP Architecture Specification
Document Type: Architecture Specification
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-02-20

# LocusQ - DSP Architecture Specification

**Version:** v0.1 (Planning)
**Complexity Score:** 5/5 (Research-grade)

---

## System Overview

LocusQ is a single VST3 binary containing three operational modes that share a common plugin shell. The critical architectural challenge is **inter-instance communication**: multiple plugin instances (Emitters) must publish spatial object data to a shared scene graph that a single Renderer instance consumes — all within the same DAW process, on the audio thread, with zero allocation and deterministic timing.

```
┌─────────────────────────────────────────────────────────┐
│                     DAW Process                         │
│                                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ Emitter  │  │ Emitter  │  │ Emitter  │   (N tracks) │
│  │ Instance │  │ Instance │  │ Instance │              │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘              │
│       │              │              │                    │
│       ▼              ▼              ▼                    │
│  ┌──────────────────────────────────────┐               │
│  │        Shared Scene Graph            │  (singleton)  │
│  │   Lock-free ring buffer per slot     │               │
│  └──────────────────┬───────────────────┘               │
│                     │                                    │
│                     ▼                                    │
│  ┌──────────────────────────────────────┐               │
│  │         Renderer Instance            │  (master bus) │
│  │  Reads scene → Spatializes → 4ch out │               │
│  └──────────────────────────────────────┘               │
│                                                         │
│  ┌──────────────────────────────────────┐               │
│  │       Calibrate Instance             │  (standalone) │
│  │  Mic in → Analysis → Room Profile    │               │
│  └──────────────────────────────────────┘               │
└─────────────────────────────────────────────────────────┘
```

---

## Decision Alignment (2026-02-19)

This architecture is aligned to the planning decision package:

- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/adr/ADR-0004-v1-ai-deferral.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/scene-state-contract.md`

Key implications:

1. v1 routing remains single-process `SceneGraph` with metadata as canonical shared state and ephemeral same-block audio fast path.
2. Authority precedence is explicit: DAW/APVTS base -> internal timeline rest pose (when enabled) -> physics additive offset.
3. AI orchestration is out of v1 critical path.
4. Phase closeout requires synchronized updates to status/evidence/readme/changelog surfaces.

---

## Core Components

### 1. Plugin Shell (`PluginProcessor` / `PluginEditor`)

The outermost JUCE layer. A single `AudioProcessor` class that delegates to one of three internal modules based on the `mode` parameter.

**Responsibilities:**
- Parameter tree creation (all 76 parameters registered, visibility gated by mode)
- Mode switching logic
- Audio I/O channel configuration (varies by mode)
- State save/restore (DAW preset serialization)
- WebView bridge setup (parameter sync, scene data to UI)

**Channel Configurations:**
| Mode | Input | Output | Notes |
|------|-------|--------|-------|
| Calibrate | 1 (mic) | 4 (speakers) | Mono mic route (external or built-in) -> quad test signals |
| Emitter | 1-2 (track audio) | 1-2 (passthrough) | Audio passes through; spatial state published to scene graph |
| Renderer | 1-2 (host bus) + scene metadata/audio fast path | 1 / 2 / 4 (host layout) | Internal quad reference render with host-layout output mapping (mono/stereo/quad) |

### Device Compatibility Contract (Stage 14 Planning)

Renderer behavior is profile-oriented and must preserve one canonical scene contract:

1. **Quad Studio Profile:** quad output layout (`quadraphonic` or `discrete(4)`) with explicit speaker mapping (`FL, FR, RL, RR`).
2. **Laptop Speaker Profile:** stereo host output layout with deterministic downmix from the same internal scene/render state.
3. **Headphone Profile:** stereo host output layout suitable for headphones; advanced personalized binaural/HRTF remains post-v1.

Calibration input routing remains channel-driven (`cal_mic_channel`) and must be usable with both built-in and external microphones.

### 2. Scene Graph (`SceneGraph` — Singleton)

The central nervous system. A process-wide singleton that all instances register with.

**Data Structure:**
```
SceneGraph (singleton, one per DAW process)
├── RoomProfile          (shared calibration data)
├── EmitterSlot[0..255]  (lock-free, atomic)
│   ├── active: bool
│   ├── position: Vec3 (x, y, z)
│   ├── size: Vec3 (w, d, h)
│   ├── gain: float
│   ├── spread: float
│   ├── directivity: float + aim Vec3
│   ├── velocity: Vec3 (for doppler)
│   ├── label: char[32]
│   ├── color: uint8
│   ├── audioBuffer: float* (ephemeral emitter block pointer; v1 fast path)
│   ├── bufferSize: int
│   └── sampleRate: double
├── RendererRegistered: bool
├── PhysicsState[0..255]
│   ├── position: Vec3
│   ├── velocity: Vec3
│   └── forces: Vec3
└── GlobalClock: uint64 (sample counter for sync)
```

Per ADR-0002, metadata fields in each emitter slot are the canonical shared state. `audioBuffer` is an optional same-block fast path and never the cross-block source of truth.

**Thread Safety Strategy:**
- Each `EmitterSlot` uses a **double-buffer with atomic swap**. Emitter writes to back buffer, atomically swaps pointer. Renderer reads front buffer. No locks on audio thread.
- Registration/deregistration (instance creation/destruction) uses a **spinlock** — rare operation, acceptable.
- `RoomProfile` is read-only after loading; written only by Calibrate mode on a background thread with atomic pointer swap.
- `audioBuffer` pointers are only valid during the current `processBlock` call. Renderer must consume within the same audio callback cycle.
- If fast-path assumptions are invalid in a host/runtime context, behavior must degrade safely and be captured by host-edge validation evidence.

**Instance Discovery:**
- On construction, each plugin instance calls `SceneGraph::getInstance()` (Meyer's singleton, process-local).
- Emitters call `registerEmitter(this)` → assigned a slot ID.
- Renderer calls `registerRenderer(this)` → becomes the scene consumer.
- On destruction: `unregisterEmitter(slotId)` / `unregisterRenderer()`.

### 3. Calibration Module (`CalibrationEngine`)

Runs only in Calibrate mode. Performs room measurement and generates a Room Profile.

**Signal Chain:**
```
Test Signal Generator → Output to Speaker N → [Room] → Mic Input → Analysis Engine
```

**Sub-components:**

#### 3a. Test Signal Generator
- Generates logarithmic sine sweeps (20Hz–20kHz), pink noise, white noise, or impulses
- Routes signal to one speaker at a time (sequential measurement)
- Level controlled by `cal_test_level`

#### 3b. Impulse Response Capture
- Records mic input during/after each test signal
- For sweeps: deconvolves recorded signal against original sweep to extract IR
- For impulses: direct capture with noise floor gating
- Stores per-speaker impulse responses

#### 3c. Analysis Engine
From the captured IRs, extracts:
- **Delay**: Time-of-arrival difference between speakers → distance estimation
- **Level**: RMS energy difference → gain trim calculation
- **Frequency Response**: FFT of IR → per-speaker EQ curve
- **Early Reflections**: First N peaks after direct sound in IR → reflection timing map
- **Room Dimensions**: Estimated from reflection patterns (basic shoebox model)

#### 3d. Room Profile Generator
- Packages all analysis results into a `RoomProfile` struct
- Serializes to JSON file for save/load
- Publishes to SceneGraph for Emitters and Renderer to use

**Room Profile Schema:**
```
RoomProfile
├── speakers[4]
│   ├── position: Vec3
│   ├── distance: float (meters)
│   ├── angle: float (degrees)
│   ├── height: float (meters)
│   ├── delayCompensation: float (ms)
│   ├── gainTrim: float (dB)
│   └── frequencyResponse: float[256] (magnitude, 256 bands)
├── room
│   ├── dimensions: Vec3 (W, D, H meters)
│   ├── earlyReflections[4][8] (per speaker, up to 8 reflections)
│   │   ├── time: float (ms)
│   │   ├── level: float (dB)
│   │   └── direction: Vec3
│   └── estimatedRT60: float (seconds)
├── listener
│   └── position: Vec3 (derived center point)
└── metadata
    ├── createdAt: string
    ├── sampleRate: double
    └── interface: string
```

### 4. Physics Engine (`PhysicsEngine`)

Simulates object motion in 3D space. Runs on a **dedicated timer thread** at a configurable tick rate (30–240 Hz), separate from the audio thread.

**Simulation Loop (per tick):**
```
For each active emitter with phys_enable:
  1. Read current forces (gravity, drag, user impulses)
  2. Apply forces: acceleration = force / mass
  3. Integrate velocity: velocity += acceleration * dt
  4. Apply drag: velocity *= (1 - drag * dt)
  5. Integrate position: position += velocity * dt
  6. Collision detection against room boundaries
     - If collision: reflect velocity * elasticity, apply friction
  7. Write updated position to EmitterSlot (atomic)
```

**Force Types (v1):**
- Gravity (constant, configurable direction and magnitude)
- Drag (velocity-proportional damping)
- Friction (applied during wall contact)
- Impulse (one-shot "throw" from UI or parameter trigger)

**Force Types (v2 stretch):**
- Attractors/repulsors (inter-object forces)
- Flow fields (fluid/gas simulation)
- Flocking (Boids algorithm: separation, alignment, cohesion)

**Physics ↔ Audio Thread Contract:**
- Physics writes position to a double-buffered slot (atomic swap)
- Audio thread reads latest position each `processBlock`
- No mutex, no allocation on either thread
- Physics tick rate is independent of audio buffer size

### 5. Spatialization Renderer (`SpatialRenderer`)

The DSP core of Renderer mode. Reads all emitter states from the scene graph, renders in a quad reference domain, and maps to host output layout (`1ch/2ch/4ch`).

**Processing Chain (per audio block):**
```
For each active EmitterSlot:
  1. Read position, size, gain, spread, directivity from scene graph
  2. Calculate per-speaker gains (VBAP or distance-based panning)
  3. Apply distance attenuation (selected model)
  4. Apply air absorption (low-pass filter, cutoff inversely proportional to distance)
  5. Apply doppler shift (if enabled, using velocity from physics)
  6. Apply directivity pattern (cardioid-like gain shaping based on aim vs speaker angle)
  7. Apply size/spread (decorrelation + multi-point source distribution)
  8. Sum into quad-reference accumulation buffer

After all emitters:
  9. Apply room acoustics (early reflections + late reverb from Room Profile)
  10. Apply per-speaker delay compensation (from calibration)
  11. Apply per-speaker gain trim
  12. Apply master gain
  13. Map to host output layout (mono/stereo/quad)
```

**Sub-components:**

#### 5a. VBAP Panner (Vector Base Amplitude Panning)
- Quad layout: 4 speakers define 4 triangular regions (with virtual height)
- For each emitter position, find the enclosing triangle
- Calculate gain weights using inverse matrix method
- Handles edge cases: object directly on a speaker, object outside speaker array

#### 5b. Distance Attenuation
- Three models: `1/r^2` (inverse square), `1/r` (linear), `log(r)` (logarithmic)
- Reference distance and max distance parameters
- Smooth interpolation to avoid clicks on rapid position changes

#### 5c. Air Absorption Filter
- Simple one-pole low-pass filter per emitter
- Cutoff frequency decreases with distance
- Coefficient: `cutoff = 20000 / (1 + distance * absorptionFactor)`

#### 5d. Doppler Processor
- Reads velocity from physics engine
- Calculates relative velocity toward/away from each speaker
- Applies pitch shift via variable-rate delay line (fractional delay interpolation)
- Scale factor exaggerates or reduces effect

#### 5e. Room Acoustics Processor
- **Early Reflections:** FIR tapped delay line using reflection data from Room Profile
  - Draft: 4-8 taps per speaker
  - Final: 16-32 taps per speaker with frequency-dependent absorption
- **Late Reverb:** Feedback delay network (FDN) with Hadamard mixing matrix
  - 4-channel FDN matches quad output naturally
  - RT60 and damping derived from Room Profile
  - Draft: 4x4 FDN
  - Final: 8x8 FDN with modulated delays

#### 5f. Speaker Compensation
- Per-speaker delay lines (from calibration or manual)
- Per-speaker gain trims
- Applied as final processing stage before output

### 6. Keyframe Animation System (`KeyframeTimeline`)

Internal animation engine that complements DAW automation under the ADR-0003 precedence contract.

**Data Model:**
```
KeyframeTimeline
├── tracks[]
│   ├── parameterId: string (e.g., "pos_azimuth")
│   ├── keyframes[]
│   │   ├── time: double (seconds or beats)
│   │   ├── value: float
│   │   └── curve: enum (Linear / EaseIn / EaseOut / EaseInOut / Step)
│   └── interpolate(time) → float
├── duration: double
├── looping: bool
├── playbackRate: float
└── transportSync: bool
```

**Evaluation:**
- On each `processBlock`, advance internal clock (optionally synced to DAW transport)
- Evaluate all active keyframe tracks at current time
- Authority precedence is deterministic (ADR-0003): DAW/APVTS base -> internal timeline rest pose (when enabled) -> physics additive offset.

### 7. UI Layer (WebView + Three.js)

**Architecture:**
```
┌─────────────────────────────────────┐
│          WebView (HTML/JS)          │
│  ┌───────────────────────────────┐  │
│  │   Three.js 3D Viewport        │  │
│  │   - Room wireframe             │  │
│  │   - Speaker positions          │  │
│  │   - Emitter objects            │  │
│  │   - Motion trails              │  │
│  │   - Velocity vectors           │  │
│  │   - Interactive drag/rotate    │  │
│  └───────────────────────────────┘  │
│  ┌───────────────────────────────┐  │
│  │   Control Panel (mode-specific)│  │
│  │   - Calibrate: wizard steps    │  │
│  │   - Emitter: position/physics  │  │
│  │   - Renderer: master controls  │  │
│  └───────────────────────────────┘  │
│  ┌───────────────────────────────┐  │
│  │   Keyframe Editor (Emitter)   │  │
│  │   - Timeline with curves       │  │
│  │   - Scrub / playback controls  │  │
│  └───────────────────────────────┘  │
└────────────────┬────────────────────┘
                 │ window.__JUCE__
                 ▼
┌─────────────────────────────────────┐
│     C++ Backend (PluginEditor)      │
│  - Parameter bridge                 │
│  - Scene graph snapshot → JSON      │
│  - Physics state → JSON             │
│  - Room profile → JSON              │
│  - Keyframe data I/O                │
└─────────────────────────────────────┘
```

**Data Flow (C++ → WebView):**
- Timer-driven (30-60 fps): C++ serializes current scene state to JSON, pushes to WebView via `evaluateJavascript`
- Scene snapshot includes: all emitter positions/sizes/velocities, room bounds, speaker positions
- Lightweight delta encoding for efficiency (only send changed values)

**Data Flow (WebView → C++):**
- User drags object in 3D viewport → JS sends new position via `window.__JUCE__` bridge
- Parameter changes in control panel → bridge call to `setParameterValue`
- Keyframe edits → serialized keyframe data sent to C++ timeline

### UI Runtime Resilience Contract (2026-02-19 Recovery Update)

To prevent "static UI" failure modes in-host, UI runtime must follow a deterministic bootstrap contract:

```
BOOT_START
  -> DOM_READY
  -> BRIDGE_READY
  -> CONTROL_BINDINGS_READY
  -> VIEWPORT_READY (optional)
  -> RUNNING
```

Rules:

1. `CONTROL_BINDINGS_READY` must not depend on `VIEWPORT_READY`.
2. Viewport failures must downgrade to `RUNNING_DEGRADED` with visible diagnostics, not abort initialization.
3. Any fatal bridge error must surface a typed error in UI overlay and test evidence logs.

### Command / Acknowledgment Path (UI <-> C++)

Each interactive control must use a command path with acknowledgment:

1. UI emits command (`set mode`, `toggle mute`, `set position`, `start calibration`, `set keyframe`).
2. C++ validates and applies command.
3. C++ returns explicit ack (`ok`, `rejected`, `error`) with reason code.
4. UI reflects committed state from canonical snapshot (not optimistic-only local state).

This removes silent no-op behavior when bridge calls fail or are dropped.

### Interaction Ownership Model

1. Control rail owns parameter intent (APVTS-facing).
2. Viewport owns spatial manipulation intent (selection/move/orbit intent).
3. Canonical state remains C++ (`PluginProcessor` + SceneGraph contract); WebView renders snapshots.
4. UI local state is cache only and must reconcile to snapshot after every ack or periodic refresh.

### Additional Invariants for Host Interactivity

1. Tabs, toggles, dropdowns, text edits, and timeline actions remain interactive even if WebGL is unavailable.
2. Mode switching updates both UI shell and processor mode parameter in bounded time.
3. Emitter selection/movement updates APVTS + SceneGraph consistently (no one-way visual-only updates).
4. Calibration controls (`start/abort`) must drive live state rows and progress meters from native status events.
5. Renderer overlays (counts, perf values, quality badge) must be sourced from native snapshots only.

---

## Processing Chain Summary

### Calibrate Mode
```
[Test Signal Gen] → Speaker Output (one at a time)
Mic Input → [IR Capture] → [Analysis] → [Room Profile]
```

### Emitter Mode
```
Audio Input → [Passthrough to Output]
                ↓ (side-chain)
         [Read parameters + physics position]
                ↓
         [Publish to SceneGraph EmitterSlot]
                ↓
         [Update WebView visualization]
```

Note: Emitters do NOT spatialize audio themselves. They pass audio through unchanged and publish their spatial state. The Renderer does all spatialization.

### Renderer Mode
```
For each EmitterSlot in SceneGraph:
  [Read emitter audio + spatial state]
       ↓
  [VBAP Panning] → 4 gain coefficients
       ↓
  [Distance Attenuation]
       ↓
  [Air Absorption Filter]
       ↓
  [Doppler Shift] (if enabled)
       ↓
  [Directivity Shaping]
       ↓
  [Size/Spread Decorrelation]
       ↓
  [Accumulate into 4-ch bus]

[4-ch Accumulation]
     ↓
[Early Reflections (FIR tap delay)]
     ↓
[Late Reverb (FDN)]
     ↓
[Speaker Delay Compensation]
     ↓
[Speaker Gain Trim]
     ↓
[Master Gain]
     ↓
[Host Output Layout (mono/stereo/quad)]
```

---

## Parameter → Component Mapping

### Calibrate Mode
| Parameter | Component | Function |
|-----------|-----------|----------|
| `cal_mic_channel` | IR Capture | Selects input channel for recording |
| `cal_spk_config` | Test Signal Gen | Routes test signals to correct outputs |
| `cal_spk[1-4]_out` | Test Signal Gen | Maps speakers to physical output channels |
| `cal_test_level` | Test Signal Gen | Sets amplitude of measurement signals |
| `cal_test_type` | Test Signal Gen | Selects sweep/noise/impulse type |

### Emitter Mode
| Parameter | Component | Function |
|-----------|-----------|----------|
| `pos_*` | SceneGraph write | Sets object position in 3D space |
| `size_*` | SceneGraph write | Defines object spatial extent |
| `emit_gain` | SceneGraph write | Published gain for Renderer to apply |
| `emit_spread` | SceneGraph write | Point source vs diffuse ratio |
| `emit_directivity` | SceneGraph write | Radiation pattern shape |
| `phys_*` | Physics Engine | Configures forces, mass, drag, elasticity |
| `anim_*` | Keyframe Timeline | Controls internal animation playback |

### Renderer Mode
| Parameter | Component | Function |
|-----------|-----------|----------|
| `rend_distance_model` | Distance Attenuation | Selects falloff curve |
| `rend_doppler*` | Doppler Processor | Enables/scales pitch shifting |
| `rend_air_absorb` | Air Absorption Filter | Enables HF rolloff with distance |
| `rend_room_*` | Room Acoustics | Controls reflection/reverb processing |
| `rend_spk[1-4]_gain` | Speaker Compensation | Per-speaker output trim |
| `rend_spk[1-4]_delay` | Speaker Compensation | Per-speaker delay alignment |
| `rend_master_gain` | Master Output | Final output level |
| `rend_quality` | All DSP components | Switches Draft/Final processing depth |
| `rend_phys_*` | Physics Engine (global) | Simulation rate, wall collision, pause |
| `rend_viz_*` | WebView (UI only) | Camera, trails, labels, grid |

---

## Complexity Assessment

**Score: 5 / 5 (Research-grade)**

**Rationale:**
1. **Inter-instance communication** — Process-wide singleton scene graph with lock-free audio-thread-safe data sharing. Novel architecture not covered by standard JUCE patterns.
2. **Real-time physics engine** — Separate simulation thread with deterministic timing, collision detection, force integration, all feeding into audio processing without locks.
3. **Multi-channel spatialization** — VBAP, distance models, doppler, air absorption, directivity — each a non-trivial DSP component, composed into a per-emitter chain.
4. **Room acoustics** — Calibration-driven early reflections (FIR) + FDN reverb. Two quality tiers with different computation budgets.
5. **Room calibration** — IR capture, deconvolution, delay/level/frequency analysis, room geometry estimation.
6. **3D visualization** — Real-time WebGL rendering of a dynamic scene with interactive object manipulation.
7. **Keyframe animation** — Internal timeline with interpolation curves, transport sync, and physics interaction.
8. **Scale** — 76 parameters, 3 operational modes, 7+ major subsystems.

---

## Dependencies

### Required JUCE Modules
- `juce_audio_basics` — Audio buffer management, MIDI
- `juce_audio_processors` — AudioProcessor base class, parameter system
- `juce_audio_formats` — WAV writing for calibration IR export
- `juce_audio_utils` — Audio device management (calibration)
- `juce_dsp` — FFT, FIR filters, oscillators, convolution
- `juce_gui_basics` — Base UI framework
- `juce_gui_extra` — WebBrowserComponent
- `juce_opengl` — Potential fallback for native 3D (may not be needed if WebGL is sufficient)
- `juce_data_structures` — ValueTree for state management

### External Dependencies (bundled or header-only)
- **Three.js** (JS, bundled inline) — WebGL 3D rendering
- **GLM or custom** (C++, header-only) — 3D math (Vec3, Mat4, quaternions) — or use JUCE's built-in Point/Vector if sufficient

### No External Dependencies Required For
- Physics engine (custom, lightweight)
- VBAP (custom implementation, well-documented algorithm)
- Room calibration (JUCE DSP FFT + custom analysis)
- Keyframe system (custom)

---

## As-Built Delta (2026-02-18)

Implementation has progressed beyond this planning draft.

### Landed through Phase 2.3
- Scene graph integration and tri-mode processor path are live.
- Spatial renderer support classes are present (`VBAPPanner`, `DistanceAttenuator`, `AirAbsorption`, `SpatialRenderer`).
- Room calibration support classes are present (`TestSignalGenerator`, `IRCapture`, `RoomAnalyzer`, `RoomProfileSerializer`, `CalibrationEngine`).
- WebView UI bridge now exposes calibration start/abort/status plumbing.

### QA integration corrections
- LocusQ scenario stimuli were normalized to canonical harness IDs/variants.
- `locusq_qa` runner factory was corrected to honor wrapped DUT factories from the scenario executor, restoring proper stimulus injection and scenario parameter application.

### Validation snapshot
- Harness ctest: 45/45 pass.
- LocusQ smoke suite: 4/4 pass.
- pluginval: GUI-context success evidence retained; restricted/headless context remains AppKit/HIServices-blocked before plugin load.

## Normative References

- `Documentation/scene-state-contract.md`
- `Documentation/invariants.md`
- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/adr/ADR-0004-v1-ai-deferral.md`
