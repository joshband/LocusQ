Title: LocusQ Creative Brief
Document Type: Creative Brief
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-02-19

# LocusQ - Creative Brief

**Alternate Name:** AetherQuad (reserved)
**Tagline:** "Place your sound."
**Category:** Quadraphonic 3D Spatial Audio Tool
**Format:** VST3 (single binary, three operational modes)

---

## Hook

LocusQ turns your DAW into a quadraphonic spatial mixing environment. Calibrate your room, place sounds as physical objects in 3D space, animate them with keyframes or physics, and render to four speakers with real-time visual feedback. One plugin, three modes, infinite spatial control.

## Vision

Most spatial audio tools treat panning as an afterthought — a knob or an X/Y pad bolted onto a stereo bus. LocusQ treats space as a first-class dimension of your mix. Every track becomes a physical object with position, size, velocity, and material properties inside a calibrated representation of your actual room.

LocusQ is a single-binary plugin with three internal modes:

### Mode 1: Calibrate
A room profiling tool that measures your physical quadraphonic setup. Using a measurement microphone and test signals sent to four speakers (4x mono or 2x stereo pairs via a Focusrite 8i6 or similar interface), it captures:
- Speaker positions and relative distances
- Room dimensions and basic reflection characteristics
- Frequency response variations per speaker/position
- Delay compensation values

The result is a **Room Profile** — a portable file that defines the spatial field for all subsequent work. Calibration is a prerequisite; Emitter and Renderer modes require a loaded Room Profile.

### Mode 2: Emitter (Per-Track Instance)
Placed on individual audio tracks or buses. Each Emitter instance:
- Positions its track's audio as a 3D object in the calibrated space
- Exposes position (azimuth, elevation, distance), size (width, depth, height), and gain
- Supports DAW-native automation and a built-in keyframe timeline for spatial animation
- Offers physics-driven motion: throw, bounce, orbit, drift with configurable mass, drag, elasticity, gravity
- Publishes its object state to a shared scene graph accessible by the Renderer
- Provides a local 3D wireframe view showing this object relative to the room and speakers

### Mode 3: Renderer (Master Bus Instance)
A single Renderer instance lives on the master or quad output bus. It:
- Aggregates all active Emitter instances into a unified scene graph
- Performs the final spatialization: distance attenuation, HRTF-optional processing, speaker feed generation
- Applies room acoustics (early reflections, basic reverb derived from Room Profile)
- Manages inter-object interactions (occlusion, proximity effects)
- Provides the master 3D visualization showing all objects, their trajectories, and the full spatial field
- Supports two quality tiers:
  - **Draft:** Real-time, simplified propagation, "feels right" physics — for interactive mixing
  - **Final:** Offline render, higher simulation fidelity, denser reflections — for export/bounce

### Mode 4 (Future): Physics Sandbox
Advanced physics behaviors beyond basic throw/bounce:
- Fluid/gas dynamics simulation (sound drifting through currents)
- Zero-gravity environments
- Flocking/swarm/herd behaviors (multiple objects moving as a group)
- Material properties affecting bounce, absorption, diffusion
- Environmental presets (underwater, cathedral, open field, etc.)

*Note: Core physics (throw, bounce, gravity, drag) ships in v1. Advanced behaviors are stretch goals.*

## Architecture Philosophy

- **Monolithic binary, modular internals:** One plugin, three modes. Internally separated into: Calibration Module, Scene Graph, Physics Engine, Renderer, UI Layer.
- **Shared scene via memory:** Emitters and Renderer communicate through shared memory within the DAW process — no IPC, no sockets, no files. Robust across DAW hosts.
- **Two-tier quality:** Draft for real-time work, Final for renders. Final is an enhancement of Draft, not a different algorithm — same spatial character, higher fidelity.
- **Visualization-first UI:** Clean minimal wireframe. Room, speakers, objects, motion vectors. Clarity over flash. Built on JUCE OpenGL initially, but the 3D layer is modular/replaceable.

## Target User

- Electronic musicians and sound designers working in quadraphonic
- Immersive audio artists and installers
- Experimental composers wanting physical metaphors for spatial mixing
- Anyone with 4 speakers and a desire to place sound in real space

## Technical Context

- **Interface:** Focusrite 8i6 (or any 4+ output interface)
- **Speaker Config:** 4 identical powered monitors, arranged in quad (or user-defined placement)
- **Channels:** 4x mono or 2x stereo pairs
- **Measurement Mic:** Any calibration mic (user-supplied)
- **DAW:** Any VST3 host (Ableton, Reaper, Bitwig, etc.)
- **Framework:** JUCE 8, WebView UI path, C++ DSP

## V1 Scope Contract (2026-02-19)

### In Scope (Must Ship In V1)

1. Single plugin binary with three modes: Calibrate, Emitter, Renderer.
2. Deterministic quad spatial core: panning, distance attenuation, spread/directivity, room chain, quality tiers.
3. Room calibration v1 for delay/trim and usable Room Profile persistence.
4. Physics v1 (deterministic throw/bounce/drag/gravity) integrated with spatial motion.
5. Internal keyframe timeline and transport-aware behavior with documented authority precedence.
6. Full-system acceptance evidence for CPU/deadline and host edge-case stability.

### Out of Scope (Deferred Post-V1)

1. AI orchestration and autonomous scene mutation.
2. Neural acoustic modeling / neural IR generation.
3. Advanced output formats and binaural/HRTF expansion beyond current v1 plan.
4. Cross-emitter complex interaction systems (flocking/swarm/fluid) beyond preset-level motion behavior.
5. Sensor-driven calibration dependencies (for example LiDAR) as shipping requirements.

## Competitive Landscape

| Tool | Difference from LocusQ |
|------|----------------------|
| Dolby Atmos Renderer | Enterprise, 7.1.4+, no physics, no calibration |
| IEM Plugin Suite | Academic, Ambisonics-focused, no integrated calibration |
| SPAT Revolution | Expensive, standalone app, not a plugin |
| Envelop for Live | Ableton-only, ambisonics, no physics |
| **LocusQ** | **Quad-focused, single plugin, physics engine, room calibration, visual, affordable** |

## Success Criteria (v1.0)

1. Room calibration produces accurate speaker delay/level compensation
2. Emitter instances reliably share state with Renderer across DAW hosts
3. Real-time Draft mode runs at < 5% CPU per Emitter instance
4. Physics-driven motion (throw, bounce, gravity, drag) feels musical and controllable
5. 3D visualization is responsive, clear, and accurately represents the spatial field
6. Keyframe animation exports/imports and syncs with DAW transport
7. In-host UI interaction is fully functional (tabs, controls, timeline, calibration actions) with explicit degraded-mode behavior if viewport initialization fails
