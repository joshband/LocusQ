---
Title: LocusQ Physics Simulation Feature Spec
Document Type: Feature Specification
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Physics Simulation Feature Spec

## Architecture

One `PhysicsWorker` thread at `rend_phys_rate` Hz (30–240). Results written to `EmitterSlot` via atomic pointer swap. No physics logic touches the audio thread. All DSP-mapped values clamped before consumption. Stall fallback: if worker misses >2 ticks, last valid positions held; DSP values frozen at last good state.

Physics output is always **additive offset** on top of the keyframe rest pose (ADR-0003 authority precedence preserved).

---

## Parameter Domains

### Existing (V1)
- `phys_enable`, `phys_mass` *(global default; see Per-Emitter Mass Override)*, `phys_drag`, `phys_elasticity`, `phys_gravity`, `phys_gravity_dir`, `phys_friction`, `phys_vel_x/y/z`, `phys_throw`, `phys_reset`
- `rend_phys_rate`, `rend_phys_walls`, `rend_phys_interact`, `rend_phys_pause`

---

## Tier A — Production Extensions

### Attractors & Repulsors

Up to 4 force sources per scene. Each has: `position: Vec3`, `strength: float` (signed: positive = attract, negative = repel), `falloff: enum {1/r, 1/r², constant}`, `radius: float` (hard influence cutoff).

Per tick, each physics-enabled emitter computes net force from all active attractors and adds to its acceleration. Force clamped before position integration.

**Orbital Stabilization:** `attractor_orbit_stabilize: bool`. When enabled, injects a tangential correction force to maintain constant orbital radius. Emitter circles the attractor rather than spiraling in or out.

**DSP hooks:**
- Proximity to attractor → `spread` modulation (closer = tighter focus)
- Attractor crossing event → momentary `gain` spike (configurable level/decay)

---

### Boids Flocking

Up to 4 flock groups, 64 emitters max per group. Each group runs three standard rules on the worker thread:

- **Separation** — steer away from neighbors within `boids_sep_radius`
- **Alignment** — match velocity of neighbors within `boids_align_radius`
- **Cohesion** — steer toward flock centroid within `boids_cohesion_radius`

Per-group APVTS parameters (automatable): `boids_sep_weight`, `boids_align_weight`, `boids_coh_weight`, `boids_sep_radius`, `boids_align_radius`, `boids_cohesion_radius`, `boids_max_speed`.

**DSP hooks:**
- Flock density (neighbor count / max) → `spread` (dense flock = wider spread)
- Per-emitter velocity magnitude → Doppler shift (existing EmitterSlot velocity field)
- Cohesion-breakup event (emitter escaping flock radius) → transient gain dip

**Visualization:**
- Per-emitter velocity vectors (`rend_viz_vectors` extended to show flock alignment direction)
- Flock centroid marker (toggleable sphere in viewport)
- Attractor field radius rings (color-coded by sign)

---

### Hard-Body Inter-Emitter Collision

Each physics-enabled emitter has `phys_collision_radius: float`. When two emitters overlap, impulse resolution is applied to both:

```
relative_vel = v_a - v_b
impulse = -(1 + elasticity) * dot(relative_vel, normal) / (1/mass_a + 1/mass_b)
v_a += impulse / mass_a * normal
v_b -= impulse / mass_b * normal
```

`phys_collide_emitters: bool` — global enable/disable. O(n²) complexity; safe at ≤64 active emitters.

**DSP hook:** collision energy → transient gain burst on both emitters, scaled by `phys_collision_gain_scale`, decaying over `phys_collision_decay_ms`.

---

### Spring / Pendulum Oscillator

Per-emitter spring tethering to a rest position.

**Parameters:**
- `phys_spring_enable: bool`
- `phys_spring_k: float` — stiffness (controls oscillation frequency: `ω = √(k/m)`)
- `phys_spring_damp: float` — 0 = undamped/sustained, 1 = critically damped/settles immediately
- `phys_spring_anchor_mode: enum {rest_pose, fixed_point}`
- `phys_spring_anchor: Vec3` — used when mode is `fixed_point`

Low damping → pendulum-like sustained oscillation after any disturbance. High damping → smooth return to rest. Spring and gravity interact naturally (pendulum hangs below anchor under gravity).

**DSP hook:** oscillation phase → `spread` or `gain` modulation (configurable depth, rate derived from spring natural frequency).

---

### Turbulence / Stochastic Force

`phys_turbulence: float` (0–1). Injects band-limited random impulses per tick — filtered through a one-pole smoother at `phys_turbulence_rate` Hz to produce coherent drift rather than high-frequency jitter. Each emitter gets an independent noise seed.

Turbulence is additive on top of all deterministic forces. Bounded: max impulse magnitude = `phys_turbulence × phys_mass × 9.8` (gravity-equivalent units; turbulence=1 is full chaos at 1g scale).

**DSP hook:** turbulence magnitude → subtle `spread` jitter (±configurable amount).

---

### Angular Physics — Emitter Spin

Extends the existing `directivity.aim: Vec3` field into a dynamically driven domain.

**New EmitterSlot fields:** `ang_velocity: Vec3`, `ang_drag: float`.

**Parameters:**
- `phys_ang_enable: bool`
- `phys_ang_drag: float`
- `phys_ang_impulse_x/y/z: float`
- `phys_ang_throw: trigger` — one-shot angular impulse
- `phys_ang_reset: trigger`
- `attractor_torque_strength: float` — optional torque pulling aim toward attractor direction

**DSP hook:** aim angle relative to each speaker → cardioid gain modulation via existing DirectivityFilter. Angular physics drives the aim input; the rendering path is unchanged.

---

### Soft Boundary Mode

`phys_boundary_mode: enum {hard, soft, passthrough}`.

- `hard`: existing behavior — reflect velocity × elasticity on wall contact.
- `soft`: repulsive force field grows as `1/(dist_to_wall)²` within `phys_soft_boundary_depth` meters. Emitter decelerates and curves away; never crosses wall surface.
- `passthrough`: walls ignored; emitter exits room bounds. Visualization clips to room volume; DSP distance model extrapolates.

---

### Per-Emitter Mass Override

`phys_mass` remains global default. Each emitter slot gains `phys_mass_override: float` (0 = use global default).

When non-zero, the override value is used for: attractor force calculation, spring oscillation frequency, inter-emitter collision impulse resolution, and Boids acceleration response. Heterogeneous mass in flocking produces natural leader/follower dynamics.

---

## Tier B — Lab Experiments

### Flow Fields

A 3D vector field grid fills the room volume. Default resolution 8×8×8; configurable 4→16 per axis. Emitters are advected per tick:

```
velocity += sample_field(position) * phys_field_advection_strength
```

**Field sources:**
- `noise` — Perlin/simplex noise, animated by `phys_field_time_rate`
- `preset` — stored fields: `vortex`, `laminar`, `turbulent`
- `painted` — user draws field vectors in viewport (Lab surface only)

**DSP hooks:**
- Field curl magnitude → air absorption cutoff shift
- Field divergence → reverb send level

---

### Environmental Presets

Composite presets that configure multiple physics + renderer parameters atomically. Lab-resident; individual presets promote to production when mix value is demonstrated.

| Preset | Gravity | Drag | Elasticity | Boundary | Air abs model | Reverb tail |
|---|---|---|---|---|---|---|
| `underwater` | 0.3↓ | 0.6 | 0.2 | soft | steep LPF | long |
| `cathedral` | 1.0↓ | 0.05 | 0.6 | hard | mild | very long |
| `open_field` | 1.0↓ | 0.15 | 0.5 | passthrough | 1/r² | short |
| `anechoic` | 1.0↓ | 0.3 | 0.1 | soft | 1/r | none |
| `zero_g_void` | 0 | 0.001 | 1.0 | soft | minimal | medium |

---

### Material Properties

Per-surface wall material presets. Applied at wall collision events.

| Material | Elasticity | Friction | Absorption | Collision event |
|---|---|---|---|---|
| `concrete` | 0.4 | 0.8 | low | hard thud transient |
| `glass` | 0.85 | 0.1 | minimal | bright click transient |
| `fabric` | 0.1 | 0.9 | high | soft thump transient |
| `foam` | 0.05 | 0.95 | very high | near-silent |

Collision transient: energy × absorption coefficient → gain envelope injection. Finite-safe: clamped and gated.

---

### Pressure / Shockwave Impulse

One-shot radial force from a 3D origin point.

**Parameters:** `shock_trigger` (APVTS one-shot button), `shock_origin: Vec3`, `shock_strength: float`, `shock_radius: float`.

All emitters within `shock_radius` receive outward impulse = `shock_strength / r²` applied in a single worker tick. Triggerable by automation, beat detection, or Lab UI button.

**DSP hook:** impulse energy per emitter → gain transient scaled by proximity to origin.

---

## DSP Mapping Contract

All physics-to-DSP mappings follow the `reactive-av` contract: normalize → optional deadband → curve → smooth (configurable attack/release) → clamp → publish.

| Physics source | Normalization | Curve | DSP target |
|---|---|---|---|
| Attractor proximity | 0..1 (0=at attractor) | sigmoid | `spread` ↑ as distance ↓ |
| Attractor crossing | binary event | gate | `gain` spike → decay |
| Boids density | neighbor count / max | linear | `spread` |
| Boids velocity | 0..max_speed | linear | Doppler shift (velocity field) |
| Inter-emitter collision energy | 0..1 clamped | log | `gain` transient |
| Spring oscillation phase | 0..2π | sine | `spread` or `gain` LFO |
| Turbulence magnitude | 0..1 | linear | `spread` jitter ±depth |
| Angular aim vs speaker angle | 0..π | cardioid | per-speaker gain (DirectivityFilter) |
| Field curl magnitude | 0..1 | linear | air absorption cutoff shift |
| Shockwave impulse energy | 0..1 per emitter | log | `gain` transient |
| Collision material absorption | 0..1 | linear | gain envelope + spectral tilt |

---

## Threading Contract

- All simulation logic runs on `PhysicsWorker` thread.
- Results written to `EmitterSlot` via existing atomic pointer swap (ADR-0002).
- No physics state accessed on audio thread.
- All DSP-mapped values clamped to finite range before audio thread consumption.
- Stall detection: >2 missed ticks → hold last valid positions and DSP values.
- Per `physics-reactive-audio` skill: simulation cadence explicit and bounded; worst-case CPU quantified before ship.

---

## Validation Status

Not tested — spec only. Validation required per tier before promotion.

**Tier A acceptance gates:**
- Worker thread CPU headroom measured at 64 emitters with all Tier A features enabled
- Inter-emitter collision determinism: same initial conditions → same output across 3 runs
- Spring oscillation frequency accuracy: measured vs analytical `ω = √(k/m)` within 2%
- Angular physics aim-vector correctness: sweep test vs expected cardioid modulation
- DSP mapping clamp: no NaN or out-of-range values under adversarial inputs

**Tier B acceptance gates (Lab promotion criteria):**
- Flow field: CPU headroom at 16³ grid resolution with 64 advected emitters
- Environmental presets: atomic parameter switch with no audio-thread glitch
- Material collision transients: finite-safe under rapid repeated collisions
- Shockwave: deterministic energy distribution vs analytical inverse-square
