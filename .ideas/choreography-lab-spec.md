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

Choreography Lab output writes to the same `EmitterSlot` position fields via the same worker-thread/additive-offset contract as physics. Layering precedence:

```
APVTS base state (DAW automation)
  + Timeline keyframe rest pose
    + Choreography Lab generative offset
      + Physics additive offset
        = Final EmitterSlot position → DSP renderer
```

### Graduation Path

Choreography Lab → record session → bake to keyframe tracks in Timeline. Baked output is a normal Timeline asset: deterministic, recallable, hand-editable. This is the mechanism by which exploratory behavior becomes composed, reproducible content.

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
| `custom` | User-defined per-slot positions | per-slot `Vec3` offsets |

**Formation animation:** formations animate as a unit — rotate, scale, or morph between two formation shapes over time. Morph uses per-slot linear interpolation between source and target positions. Parameters: `formation_morph_rate`, `formation_morph_loop: bool`, `formation_morph_pingpong: bool`.

**DSP hook:** formation spread (average inter-emitter distance / max distance) → per-emitter `spread` parameter (tighter formation = more focused individual sources).

---

### 2. Procedural Paths *(production-candidate)*

Single-emitter continuous motion paths defined by mathematical functions. The emitter follows the path indefinitely (looped) or for a configured duration. Physics applies as additive offset on top of the path position (same authority chain).

**Built-in path types:**

| Path | Description | Key parameters |
|---|---|---|
| `lissajous` | Parametric: `x=A·sin(aωt+δ)`, `y=B·sin(bωt)`, `z=C·sin(cωt)` | frequency ratios `a:b:c`, amplitudes `A/B/C`, phase `δ` |
| `orbit` | Circular/elliptical orbit around anchor | `radius_x`, `radius_z`, `height`, `period` |
| `pendulum` | Analytical gravity-driven pendulum swing | `length`, `amplitude`, `plane` |
| `figure_eight` | Lemniscate curve | `scale`, `plane`, `period` |
| `helix` | Rising/falling spiral | `radius`, `pitch`, `direction` |
| `random_walk` | Bounded Brownian motion | `step_size`, `bounds`, `seed` |

**DSP hook:** path velocity at current position → Doppler shift via existing EmitterSlot `velocity` field.

---

### 3. Beat-Sync Choreography *(production-candidate, requires DAW tempo)*

Emitter position changes are quantized to the host beat grid.

**Parameters:**
- `choro_beat_division: enum {bar, beat, 1/2, 1/4, 1/8, 1/16, 1/32}`
- `choro_beat_mode: enum {snap, glide, teleport}`
  - `snap`: position jumps to next slot on beat, interpolated over one division
  - `glide`: smooth interpolation to next position, arriving exactly on beat
  - `teleport`: instant position change on beat with a brief gain-dip mask to hide artifact

**Pattern sequencer:** per-emitter step sequence of up to 16 steps. Each step: `hold` (same position), `advance` (next formation slot), or `jump` (arbitrary slot index). Steps trigger on each `choro_beat_division` tick.

**DSP hooks:**
- Beat-coincident position change → gain dip envelope (masks teleport artifact)
- Glide transit velocity → Doppler modulation during move

---

### 4. Audio-Reactive Choreography *(Lab → production candidate)*

Emitters move in response to audio features extracted from a designated audio bus. Each emitter can be assigned an audio source and a reactive behavior mapping.

**Feature extractors** (run on worker thread; features transferred to Choreography worker via lock-free ring buffer — never touches audio thread):

| Feature | Description | Update rate |
|---|---|---|
| `rms` | Short-term RMS level | per block |
| `peak` | Peak amplitude | per block |
| `onset` | Transient detection (spectral flux threshold) | event-driven |
| `band_energy[N]` | Energy in N configurable frequency bands | per block |
| `spectral_centroid` | Weighted mean frequency | per block |
| `spectral_flux` | Frame-to-frame spectral change magnitude | per block |

**Reactive behavior mappings:**

| Source feature | Target parameter | Behavioral result |
|---|---|---|
| `rms` | orbit radius | emitter expands outward with loudness |
| `rms` | formation scale | group expands/contracts with mix level |
| `onset` | position jump (toward/away from center) | rhythmic scatter on transients |
| `onset` | formation morph trigger | group snaps to next formation on hit |
| `band_energy[low]` | Y position (height) | bass lifts emitter upward |
| `spectral_centroid` | arc angle | bright sounds move one direction, dark another |
| `spectral_flux` | turbulence strength override | busy signals increase motion chaos |

All mappings follow the `reactive-av` contract: normalize → optional deadband → curve → smooth (configurable attack/release) → clamp → publish.

---

### 5. Multi-Emitter Coordination *(Lab)*

Behaviors that define relationships *between* emitters rather than independent per-emitter rules.

**Leader/Follower:**
One emitter designated leader; up to 7 followers trail behind with configurable `choro_follow_delay_ms` and `choro_follow_offset: Vec3`. Each follower copies the leader's position history at the configured lag. Creates natural echo/shadow motion for stereo or quad scenes.

**Phase-Offset Ensemble:**
All emitters in a coordination group run the same procedural path or formation animation, each offset in time by `choro_phase_step × slot_index`. Creates wave, canon, and ripple effects across the emitter array.

**Mirror / Symmetry:**
One emitter's position is mirrored across a configurable plane (`XZ`, `XY`, `YZ`, or arbitrary normal + origin). Mirror emitter position computed as geometric reflection. Useful for symmetric stereo pairs and quad mirror arrangements.

**Proximity Coupling:**
When two emitters come within `choro_couple_radius`, a gentle mutual attraction pulls them toward their shared midpoint. Blends smoothly with all other forces. Creates "gravitational companionship" without full Boids complexity.

---

### 6. Graduation Mechanism — Bake to Timeline *(architectural contract)*

The critical bridge between Lab exploration and production composition.

**Record mode:** Choreography Lab output is captured into a position buffer at `rend_phys_rate` Hz during a user-defined transport range.

**Bake process:**
1. Captured position data downsampled to `bake_kf_density` keyframes/second
2. Curve-fitted per track to minimize keyframe count while bounding position error within `bake_curve_fit_tolerance` meters
3. Written as new keyframe tracks in the Timeline for each participating emitter
4. Interpolation curve type set to `EaseInOut` by default (user-adjustable per track after bake)

**Bake parameters:**
- `bake_start`, `bake_end` — transport range (DAW timeline positions)
- `bake_kf_density: float` — keyframes per second (e.g. 4 = quarter-note at 60bpm)
- `bake_curve_fit_tolerance: float` — max allowable position error in meters

**Result:** The choreography session becomes a deterministic, host-recallable, automatable Timeline asset. Operator can hand-edit individual keyframes, add physics offset on top, and treat it as normal composed content.

---

## Containment Governance (BL-094)

Per BL-094, each feature must pass the placement test before earning a new top-level surface:

| Feature | Placement | Current surface | Graduation trigger |
|---|---|---|---|
| Formation Patterns | Timeline extension (replaces per-emitter manual keyframing of group arrangements) | Production-candidate | Mix utility demonstrated in ≥3 sessions |
| Procedural Paths | Timeline extension (path = implicit keyframe track) | Production-candidate | Operator requests in production context |
| Beat-Sync Choreography | Timeline extension (quantized keyframe steps) | Production-candidate | Validates in ≥1 DAW host with tempo sync |
| Audio-Reactive Choreography | Lab (non-deterministic until baked; does not change next 10s of action directly) | Lab | Bake round-trip validates; mix utility demonstrated |
| Multi-Emitter Coordination | Lab (relational rules; experimental) | Lab | Leader/follower and mirror promote first |
| Bake to Timeline | Architectural bridge | Ships with Lab | Enables all graduation paths |

---

## Visualization

**Formation overlays:** wireframe formation shape rendered in viewport when formation mode is active. Slot-to-emitter assignment lines.

**Path previews:** procedural path drawn as a ghost curve in the viewport for the next N seconds of motion.

**Beat-sync indicators:** beat grid markers on the transport bar; emitter pulse animation on each quantized step.

**Audio-reactive meters:** per-emitter RMS/onset indicator rings (overlay toggle `rend_viz_reactive`).

**Bake preview:** before committing bake, a ghost playback shows the keyframe-reduced curve overlaid on the original captured motion.

All overlays are visual-only layers and must never mutate DSP or scene canonical state (per `Documentation/invariants.md`).

---

## Threading Contract

- Feature extraction runs on `PhysicsWorker` or a dedicated `ChoreographyWorker` thread.
- Audio features transferred from audio thread via lock-free ring buffer (never blocking, bounded size).
- Choreography position outputs written to `EmitterSlot` via atomic pointer swap.
- No choreography logic on audio thread.
- Beat-sync uses DAW transport position read atomically; no callback into DAW on audio thread.

---

## Validation Status

Not tested — spec only.

**Production-candidate acceptance gates (Formations, Procedural Paths, Beat-Sync):**
- Formation morph: position error < 1mm vs analytical target at morph completion
- Procedural path CPU overhead: < 0.5ms/emitter at 240Hz worker rate
- Beat-sync timing accuracy: position snap within ±2ms of beat boundary across 4 DAW hosts
- Bake round-trip fidelity: reconstructed path RMS error < `bake_curve_fit_tolerance` meters

**Lab acceptance gates (Audio-Reactive, Multi-Emitter Coordination):**
- Feature extractor latency: onset detection within 10ms of transient
- Audio-reactive mapping: deterministic output for identical input stream replay
- Leader/follower delay accuracy: ±1ms at configured `choro_follow_delay_ms`
- No audio-thread contention under all coordination modes at 64 emitters
