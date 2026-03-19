---
Title: LocusQ Choreography Lab Feature Spec
Document Type: Feature Specification
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Choreography Lab Feature Spec

## Architectural Distinction: Timeline vs Choreography Lab

### Timeline (shipped production surface)

The operator *composes* explicitly. Each emitter has per-parameter keyframe tracks with interpolation curves (Linear, EaseIn, EaseOut, EaseInOut, Step). DAW transport sync ties playback to the host playhead. Output is deterministic, host-recallable, and automatable via APVTS. Physics applies as additive offset on top of keyframed rest pose (ADR-0003).

Use the Timeline when: the operator needs to know exactly where each emitter is at every moment in the arrangement.

### Choreography Lab (experimental, BL-094 governed)

The operator *configures rules* and motion emerges. No per-emitter keyframe authoring. The operator sets parameters (formation shape, beat division, audio source, coordination mode) and the system generates positions. Output is non-deterministic per session unless baked.

Lives in Lab by BL-094 governance. Graduates to production via the **keyframe bake export** mechanism.

Use Choreography Lab when: motion should emerge from relationships, patterns, or audio reactivity rather than explicit composition.

### Authority Chain

Choreography Lab output writes to the same `EmitterSlot` position fields via the same worker-thread contract as physics. The full four-layer precedence is:

```
APVTS base state (DAW automation)
  + Timeline keyframe rest pose
    + Choreography Lab generative offset
      + Physics additive offset
        = Final EmitterSlot position → DSP renderer
```

**ADR-0020:** This four-layer chain extends ADR-0003's three-layer model. See `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md`. Choreography Lab output may now write to production `EmitterSlot` instances per ADR-0020 guardrails.

### Graduation Path

Choreography Lab → record session → bake to keyframe tracks in Timeline. Baked output is a normal Timeline asset: deterministic, recallable, hand-editable, physics-offset-compatible. This is the mechanism by which exploratory behavior becomes composed, reproducible content.

---

## Parameter Namespace Conventions

New parameters use the `choro_*` prefix. Parameters marked **[new — pending parameter-spec.md]** require addition to `parameter-spec.md` and `Documentation/implementation-traceability.md` before implementation.

---

## Feature Set

### 1. Formation Patterns *(production-candidate)*

Named geometric arrangements that define a multi-emitter collective rest pose. The operator assigns N emitters to a formation; the formation position set becomes the shared rest pose from which physics and spring offsets apply.

**Built-in formations:**

| Formation | Description | Key parameters |
|---|---|---|
| `line` | Linear arrangement | `axis`, `spacing`, `offset` |
| `arc` | Curved line segment | `radius`, `arc_angle`, `plane` |
| `circle` | Equal-spaced ring | `radius`, `plane`, `phase_offset` |
| `grid` | 2D rectangular array | `rows`, `cols`, `spacing_x`, `spacing_z` |
| `spiral` | Archimedes spiral | `turns`, `spacing`, `height_rise` |
| `sphere_surface` | Equal-area distribution on sphere | `radius`, `point_count` |
| `custom` | User-defined per-slot positions | per-slot Vec3 offsets |

**Formation animation:** formations animate as a unit — rotate, scale, or morph between two formation shapes over time. Morph uses per-slot linear interpolation. Parameters: `choro_formation_morph_rate`, `choro_formation_morph_loop: bool`, `choro_formation_morph_pingpong: bool`. All **[new — pending parameter-spec.md]**.

**Relationship to Timeline formation track:** `choro_formation_morph_*` controls *continuous* animation rate/loop without explicit keyframes — this is the Choreography Lab model. When a Choreography formation session is baked to the Timeline, the result becomes formation-type keyframes whose morph interpolation is governed by the Timeline's standard curve enum (not morph rate). See `timeline-spec.md §Type 3 — Formation Track`.

**DSP hook — spread:** formation spread (average inter-emitter distance / max distance) contributes an **additive offset** to each emitter's `spread` field — it does not override the APVTS `emit_spread` base value. This contribution is summed with all active physics spread contributions, then the total is clamped to [0..1] before being written to `EmitterSlot.spread` by the ChoreographyWorker. Tighter formation = smaller positive offset (more focused). Wider formation = larger positive offset (more diffuse). Write arbitration is identical to the physics spread contract (see physics-simulation-spec.md §DSP Mapping Contract).

---

### 2. Procedural Paths *(production-candidate)*

Single-emitter continuous motion paths defined by mathematical functions. The emitter follows the path indefinitely (looped) or for a configured duration. Physics applies as additive offset on top of the path position (same authority chain).

**Built-in path types:**

| Path | Description | Key parameters |
|---|---|---|
| `lissajous` | `x=A·sin(aωt+δ)`, `y=B·sin(bωt)`, `z=C·sin(cωt)` | frequency ratios `a:b:c`, amplitudes, phase `δ` |
| `orbit` | Circular/elliptical orbit around anchor | `radius_x`, `radius_z`, `height`, `period` |
| `pendulum` | Closed-form analytical pendulum swing | `length`, `amplitude`, `plane` |
| `figure_eight` | Lemniscate curve | `scale`, `plane`, `period` |
| `helix` | Rising/falling spiral | `radius`, `pitch`, `direction` |
| `random_walk` | Bounded Brownian motion | `step_size`, `bounds`, `seed` |

**Pendulum vs Spring Oscillator distinction:** The `pendulum` path type here is a **closed-form analytical solution** — position computed directly from time, immune to numerical drift, unaffected by external disturbances (impulses, attractors, collisions). The physics Spring/Pendulum Oscillator (physics-simulation-spec.md §Spring / Pendulum Oscillator) is a **numerically integrated simulation** that responds to all external forces. They can be layered: use choreography `pendulum` path as rest pose, physics spring as additive offset on top, producing a disturbance-responsive pendulum.

**DSP hook:** path velocity at current position → Doppler shift via existing `EmitterSlot.velocity` field. ChoreographyWorker computes velocity as the positional delta between the current and previous tick positions and writes it to `EmitterSlot.velocity`.

---

### 3. Beat-Sync Choreography *(production-candidate, requires DAW tempo)*

Emitter position changes are quantized to the host beat grid. DAW transport position is read atomically by the ChoreographyWorker; no callback into the DAW on the audio thread.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `choro_beat_division` | enum | `bar, beat, 1/2, 1/4, 1/8, 1/16, 1/32` |
| `choro_beat_mode` | enum | `snap, glide, teleport` |
| `choro_teleport_dip_db` | float | gain dip depth on teleport (default −6 dB) |
| `choro_teleport_decay_ms` | float | gain dip decay time (default 20 ms) |

All **[new — pending parameter-spec.md]**.

**Beat modes:**
- `snap`: position interpolates to next slot over one division, arriving on beat
- `glide`: smooth interpolation, arriving exactly on beat
- `teleport`: instant position change on beat; ChoreographyWorker writes a gain dip envelope to `EmitterSlot.gain` (same lock-free atomic path as physics). Envelope: instant dip to `choro_teleport_dip_db`, exponential decay over `choro_teleport_decay_ms`. Finite-safe: rate-limited to one dip per beat division minimum; gain clamped to [0..1] range after dB conversion. Audio thread reads `EmitterSlot.gain` per-block; no audio-thread involvement in dip computation.

**Pattern sequencer:** per-emitter step sequence of up to 16 steps. Each step: `hold`, `advance` (next formation slot), or `jump` (arbitrary slot index). Steps trigger on each `choro_beat_division` tick.

**DSP hooks:**
- Teleport: gain-dip envelope written to `EmitterSlot.gain` by ChoreographyWorker
- Glide transit velocity → Doppler modulation via `EmitterSlot.velocity`

**Acceptance gate (hard blocking gate — must pass before production promotion):** position snap timing within ±2ms of beat boundary measured across 4 DAW hosts; teleport gain-dip finite-safe under 100 rapid sequential teleports with no NaN or gain > 0dB.

---

### 4. Audio-Reactive Choreography *(Lab → production candidate)*

Emitters move in response to audio features extracted from a designated audio bus.

**Configuration parameters:**

| Parameter | Type | Description |
|---|---|---|
| `choro_audio_src_bus` | int | index of audio bus to analyse (0 = main input) |
| `choro_audio_band_count` | int 1..8 | number of configurable frequency bands for `band_energy[N]` |
| `choro_audio_band_hz[N]` | float array | upper edge frequency of each band in Hz |
| `choro_audio_smooth_ms` | float | feature smoothing time (attack = release) in ms |
| `choro_audio_buffer_len_ms` | float | length of the audio capture ring buffer in ms (default 100ms) |

All **[new — pending parameter-spec.md]**.

**Threading contract:** The audio thread writes per-block feature data into a **lock-free bounded ring buffer** during `processBlock`. The ChoreographyWorker reads from the ring buffer and computes derived features (onset detection, spectral analysis). The audio thread is involved in the production side (writing raw block data); the ChoreographyWorker owns the consumer side (feature computation and position mapping). Neither side blocks. Ring buffer size: pre-allocated at startup; bounded to `ceil(choro_audio_buffer_len_ms / 1000 × sample_rate)` samples per channel.

**Feature extractors** (computed by ChoreographyWorker from ring buffer data):

| Feature | Description | Latency |
|---|---|---|
| `rms` | Short-term RMS level | ≤1 block |
| `peak` | Peak amplitude | ≤1 block |
| `onset` | Transient detection (spectral flux threshold) | ≤10ms |
| `band_energy[N]` | Energy in N configurable frequency bands | ≤1 block |
| `spectral_centroid` | Weighted mean frequency | ≤1 block |
| `spectral_flux` | Frame-to-frame spectral change magnitude | ≤1 block |

**Acceptance gate:** onset detection latency ≤10ms from transient; feature extractor output deterministic for identical input stream replay.

**Reactive behavior mappings** (all follow reactive-av contract: normalize → deadband → curve → smooth → clamp → publish):

| Source feature | Target | Behavioral result |
|---|---|---|
| `rms` | orbit radius | emitter expands outward with loudness |
| `rms` | formation scale | group expands/contracts with mix level |
| `onset` | position jump toward/away center | rhythmic scatter on transients |
| `onset` | formation morph trigger | group snaps to next formation on hit |
| `band_energy[low]` | Y position (height) | bass lifts emitter upward |
| `spectral_centroid` | arc angle | bright sounds move one direction, dark another |
| `spectral_flux` | turbulence strength override | busy signals increase motion chaos |

---

### 5. Multi-Emitter Coordination *(Lab)*

**Leader/Follower:**
One emitter designated leader; up to 7 followers trail with configurable `choro_follow_delay_ms` and `choro_follow_offset: Vec3`. Position history ring buffer pre-allocated at startup: `ceil(choro_follow_delay_max_ms / 1000 × rend_phys_rate)` Vec3 entries per leader slot. Maximum `choro_follow_delay_ms`: 2000ms. Runtime changes to `choro_follow_delay_ms` adjust the read pointer within the pre-allocated buffer only — no reallocation. All **[new — pending parameter-spec.md]**.

**Phase-Offset Ensemble:**
All emitters in a coordination group run the same procedural path or formation animation, each offset in time by `choro_phase_step × slot_index`. Creates wave, canon, and ripple effects across the emitter array.

**Mirror / Symmetry:**
One emitter's position is mirrored across a configurable plane (`XZ`, `XY`, `YZ`, or arbitrary normal + origin). Mirror emitter position computed as geometric reflection by ChoreographyWorker.

**Proximity Coupling:**
When two emitters come within `choro_couple_radius`, gentle mutual attraction pulls them toward their shared midpoint. Blends smoothly with all other forces. Creates "gravitational companionship" without Boids overhead.

---

### 6. Graduation Mechanism — Bake to Timeline *(architectural contract)*

**Record mode:** ChoreographyWorker captures position output into a ring buffer at `rend_phys_rate` Hz during the user-defined transport range.

**Bake parameters:**

| Parameter | Type | Description |
|---|---|---|
| `bake_start` | transport pos | bake range start |
| `bake_end` | transport pos | bake range end |
| `bake_kf_density` | float kf/s | keyframes per second (e.g. 4 = quarter-note at 60bpm) |
| `bake_curve_fit_tolerance` | float meters | max allowable positional error |

All **[new — pending parameter-spec.md]**.

**Bake process:**
1. Captured position data downsampled to `bake_kf_density` keyframes/second
2. Curve-fitted per track to minimize keyframe count while bounding position error within `bake_curve_fit_tolerance`
3. Written as new keyframe tracks in Timeline for each participating emitter
4. Default interpolation: `EaseInOut` (user-adjustable per track after bake)

**Acceptance gate:** reconstructed path RMS error < `bake_curve_fit_tolerance` meters measured across a 30-second bake.

---

## Containment Governance (BL-094)

| Feature | Surface | Graduation trigger |
|---|---|---|
| Formation Patterns | Production-candidate | Mix utility in ≥3 sessions |
| Procedural Paths | Production-candidate | Operator requests in production context |
| Beat-Sync Choreography | Production-candidate | Validates in ≥1 DAW host with tempo sync |
| Audio-Reactive Choreography | Lab | Bake round-trip validates; mix utility demonstrated |
| Multi-Emitter Coordination | Lab | Leader/follower and mirror promote first |
| Bake to Timeline | Architectural bridge (ships with Lab) | Enables all graduation paths |

---

## Visualization

All overlays are visual-only layers and must never mutate DSP or scene canonical state (per `Documentation/invariants.md`).

- **Formation overlays:** wireframe formation shape + slot-to-emitter assignment lines
- **Path previews:** ghost curve in viewport for next N seconds of procedural path motion
- **Beat-sync indicators:** beat grid markers on transport bar; emitter pulse animation on each quantized step
- **Audio-reactive meters:** per-emitter RMS/onset indicator rings; toggle parameter `rend_viz_reactive: bool` **[new — pending parameter-spec.md]**
- **Bake preview:** ghost playback showing keyframe-reduced curve overlaid on original captured motion before commit

---

## Threading Contract (ADR-0020)

ChoreographyWorker is a **logical module colocated in the PhysicsWorker tick** — no new OS thread. Per ADR-0020 §3:

- ChoreographyWorker.compute() runs at the start of each PhysicsWorker tick, before physics integration.
- Outputs a per-emitter `ChoreographyOffset` struct (position, spread_delta, gain_delta, velocity) consumed immediately by the PhysicsWorker to compose the rest pose.
- The single `EmitterSlot` atomic pointer swap per tick (ADR-0002) remains exclusively owned by PhysicsWorker.
- Audio thread writes per-block data to the lock-free bounded `AudioRingBuffer` during `processBlock` — non-blocking, no allocation.
- ChoreographyWorker reads `AudioRingBuffer` on the worker thread — non-blocking consumer side.
- Beat-sync reads DAW transport position atomically — no DAW callback on audio thread.
- Leader/follower history ring buffer pre-allocated at startup — no runtime allocation.
- No choreography logic on audio thread.

---

## ADR Status

**Resolved.** `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md` records the four-layer chain rationale, authority conflict resolution rules (same-field, same-tick ordering), and the colocated-tick worker arbitration decision. Choreography Lab implementation may proceed per ADR-0020 guardrails.

---

## Validation Status

Not tested — spec only.

**Production-candidate acceptance gates (Formations, Procedural Paths, Beat-Sync):**
- Formation morph: position error < 1mm vs analytical target at morph completion
- Procedural path CPU: < 0.5ms/emitter at 240Hz worker rate
- Beat-sync timing: position snap within ±2ms of beat boundary across 4 DAW hosts
- Bake round-trip: reconstructed path RMS error < `bake_curve_fit_tolerance`
- Teleport gain-dip: finite-safe under 100 rapid sequential teleports; no NaN or gain > 0dB

**Lab acceptance gates (Audio-Reactive, Multi-Emitter Coordination):**
- Onset detection latency ≤10ms from transient
- Audio-reactive mapping: deterministic output for identical input stream replay
- Leader/follower delay accuracy: ±1ms at configured `choro_follow_delay_ms`
- No audio-thread contention under all coordination modes at 64 emitters
- No allocation on worker thread or audio thread during operation
