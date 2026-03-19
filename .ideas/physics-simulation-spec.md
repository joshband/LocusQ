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

Physics output is always **additive offset** on top of the composed rest pose. The composed rest pose is:
```
APVTS base state + Timeline keyframe offset + Choreography Lab generative offset
```
Physics sits on top of this three-part rest pose. This four-layer authority chain is defined in **ADR-0020** (`Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md`), which supersedes ADR-0003's three-layer model.

## Current Implementation Posture

This document remains the target architecture/specification, not a statement that the full shared-worker runtime path is already authoritative in the shipping plugin.

As of 2026-03-18:
- Tier A parameter registration, traceability, UI relay wiring, and a standalone subsystem probe exist.
- New subsystem headers (`PhysicsWorker`, `PhysicsDSPBridge`, attractor/spring/turbulence/angular/boids/collision systems) exist.
- The production plugin runtime still centers on the legacy per-emitter `PhysicsEngine` path in `PluginProcessor`.

Interpretation rule:
- Treat this spec as the intended contract for runtime integration work.
- Treat runtime-completion claims as invalid unless confirmed in Tier 0 status/evidence surfaces and the real processor path.

---

## Parameter Namespace Conventions

New parameters in this spec belong to the following reserved prefixes (per parameter-spec.md Note 7):
- `phys_flock_*` — flocking/Boids parameters
- `phys_ang_*` — angular physics parameters
- `phys_field_*` — flow field parameters
- `phys_mat_*` — material property parameters
- `phys_preset_*` — environmental preset parameters

Parameters marked **[new — pending parameter-spec.md]** require addition to `parameter-spec.md` and `Documentation/implementation-traceability.md` before implementation.

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
- Proximity to attractor → `spread` modulation (closer = tighter focus); additive contribution within the multi-source spread chain (see DSP Mapping Contract)
- Attractor crossing event → momentary `gain` spike (configurable level/decay); written to `EmitterSlot.gain` by PhysicsWorker

---

### Boids Flocking

Up to 4 flock groups, 64 emitters max per group. Each group runs three standard rules on the worker thread:

- **Separation** — steer away from neighbors within `phys_flock_sep_radius`
- **Alignment** — match velocity of neighbors within `phys_flock_align_radius`
- **Cohesion** — steer toward flock centroid within `phys_flock_coh_radius`

Per-group APVTS parameters (automatable):

| Parameter | Type | Description |
|---|---|---|
| `phys_flock_sep_weight` | float 0..1 | separation rule weight |
| `phys_flock_align_weight` | float 0..1 | alignment rule weight |
| `phys_flock_coh_weight` | float 0..1 | cohesion rule weight |
| `phys_flock_sep_radius` | float meters | separation influence radius |
| `phys_flock_align_radius` | float meters | alignment influence radius |
| `phys_flock_coh_radius` | float meters | cohesion influence radius |
| `phys_flock_max_speed` | float m/s | per-emitter speed cap |

All parameters **[new — pending parameter-spec.md]** under the reserved `phys_flock_*` namespace.

**DSP hooks:**
- Flock density (neighbor count / max) → `spread` (additive contribution)
- Per-emitter velocity magnitude → Doppler shift (existing EmitterSlot `velocity` field)
- Cohesion-breakup event (emitter escaping `phys_flock_coh_radius`) → transient gain dip written to `EmitterSlot.gain`

**Visualization:**
- Per-emitter velocity vectors (`rend_viz_vectors` extended to show flock alignment)
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

`phys_collide_emitters: bool` — global enable/disable. O(n²) complexity; bounded safe at ≤64 active emitters.

**DSP hook:** collision energy written to `EmitterSlot.gain` by PhysicsWorker as a transient burst envelope. Envelope shape: instant-on, exponential decay. Parameters: `phys_collision_gain_scale: float` (peak dB), `phys_collision_decay_ms: float`. Both **[new — pending parameter-spec.md]**. Finite-safe: energy clamped to [0..1] before gain computation; minimum decay enforced to prevent runaway.

**Acceptance gate — shockwave + collision concurrency:** Under simultaneous shockwave impulse and inter-emitter collision in the same worker tick, output must be deterministic across 3 runs with identical initial state.

---

### Spring / Pendulum Oscillator

Per-emitter spring tethering to a rest position. Distinct from the Choreography `pendulum` path type: this is a numerically integrated simulation that responds to external disturbances (impulses, attractor forces, inter-emitter collisions). The Choreography pendulum is a closed-form analytical path immune to disturbances. They can be layered: choreography pendulum as rest pose, physics spring as additive offset on top.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `phys_spring_enable` | bool | per-emitter enable |
| `phys_spring_k` | float | stiffness; controls oscillation frequency `ω = √(k/m)` |
| `phys_spring_damp` | float 0..1 | 0 = undamped, 1 = critically damped |
| `phys_spring_anchor_mode` | enum | `rest_pose` or `fixed_point` |
| `phys_spring_anchor` | Vec3 | used when mode is `fixed_point` |

All **[new — pending parameter-spec.md]**.

Low damping → sustained pendulum-like oscillation after any disturbance. High damping → smooth return to rest. Spring and gravity interact naturally.

**DSP hook:** oscillation phase → `spread` or `gain` modulation (configurable depth; rate derived from spring natural frequency); written by PhysicsWorker via `EmitterSlot`.

---

### Turbulence / Stochastic Force

`phys_turbulence: float` (0–1) **[new — pending parameter-spec.md]**. Injects band-limited random impulses per tick — filtered through a one-pole smoother at `phys_turbulence_rate` Hz to produce coherent drift rather than jitter. Each emitter gets an independent noise seed.

Bounded: max impulse magnitude = `phys_turbulence × phys_mass × 9.8`. Additive on top of all deterministic forces.

**DSP hook:** turbulence magnitude → `spread` jitter (±configurable depth); additive contribution.

---

### Angular Physics — Emitter Spin

Extends the existing `EmitterSlot.directivityAim: Vec3` field into a dynamically driven domain.

**Internal PhysicsEngine state only** (not promoted to EmitterSlot): `ang_velocity: Vec3`, `ang_drag: float`. These are worker-thread-local values. The PhysicsWorker writes only the resulting `directivityAim` Vec3 to `EmitterSlot` via the existing atomic swap — no new EmitterSlot fields required.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `phys_ang_enable` | bool | enable angular physics |
| `phys_ang_drag` | float 0..1 | angular velocity decay per tick |
| `phys_ang_impulse_x/y/z` | float | angular impulse components |
| `phys_ang_throw` | trigger | apply one-shot angular impulse |
| `phys_ang_reset` | trigger | zero angular velocity, restore default aim |
| `phys_ang_attractor_torque` | float | torque pulling aim toward nearest active attractor direction |

All **[new — pending parameter-spec.md]** under the reserved `phys_ang_*` namespace.

**DSP hook:** aim direction relative to each speaker → cardioid gain modulation via existing DirectivityFilter. Angular physics drives the `directivityAim` input; no changes to the rendering path.

---

### Soft Boundary Mode

`phys_boundary_mode: enum {hard, soft, passthrough}` **[new — pending parameter-spec.md]**.

- `hard`: existing behavior — reflect velocity × elasticity on wall contact.
- `soft`: repulsive force field grows as `1/(dist_to_wall)²` within `phys_soft_boundary_depth` meters. Emitter decelerates and curves away; never crosses wall surface.
- `passthrough`: walls ignored; emitter exits room bounds. Visualization clips to room volume; DSP distance model extrapolates.

---

### Per-Emitter Mass Override

`phys_mass` remains global default. Each emitter slot gains `phys_mass_override: float` **[new — pending parameter-spec.md]** (0 = use global default).

When non-zero, used for: attractor force calculation, spring oscillation frequency (`ω = √(k/m_override)`), inter-emitter collision impulse resolution, and Boids acceleration response. Heterogeneous mass in flocking produces natural leader/follower dynamics.

---

## Tier B — Lab Experiments

### Flow Fields

A 3D vector field grid fills the room volume. Default resolution 8×8×8; configurable 4→16 per axis via `phys_field_resolution: int` **[new — pending parameter-spec.md]**. Emitters are advected per tick:

```
velocity += sample_field(position) * phys_field_advection_strength
```

**Field sources:** `noise` (Perlin/simplex, animated by `phys_field_time_rate`), `preset` (vortex, laminar, turbulent), `painted` (Lab-only via viewport).

**DSP hooks** (require new parameters before implementation):
- Field curl magnitude → air absorption cutoff shift: `phys_field_air_cutoff_shift: float` (Hz offset) **[new — pending parameter-spec.md]**
- Field divergence → reverb send level: `phys_field_reverb_send: float` (0..1) **[new — pending parameter-spec.md]**

---

### Environmental Presets

Composite presets that atomically configure existing physics parameters. Air absorption model and reverb tail columns map to new parameters **[pending parameter-spec.md]**: `phys_preset_air_model: enum {inverse_r, inverse_r2, lp_steep}` and `phys_preset_reverb_tail: float` (seconds). Lab-resident; promote individually when mix value is demonstrated.

| Preset | Gravity | Drag | Elasticity | Boundary | Air model | Reverb tail |
|---|---|---|---|---|---|---|
| `underwater` | 0.3↓ | 0.6 | 0.2 | soft | `lp_steep` | 3.0s |
| `cathedral` | 1.0↓ | 0.05 | 0.6 | hard | `inverse_r` | 5.0s |
| `open_field` | 1.0↓ | 0.15 | 0.5 | passthrough | `inverse_r2` | 0.5s |
| `anechoic` | 1.0↓ | 0.3 | 0.1 | soft | `inverse_r` | 0.0s |
| `zero_g_void` | 0 | 0.001 | 1.0 | soft | `inverse_r` | 1.5s |

---

### Material Properties

Per-surface wall material presets. Applied at wall collision events.

| Material | Elasticity | Friction | Absorption | Collision transient |
|---|---|---|---|---|
| `concrete` | 0.4 | 0.8 | low | hard thud |
| `glass` | 0.85 | 0.1 | minimal | bright click |
| `fabric` | 0.1 | 0.9 | high | soft thump |
| `foam` | 0.05 | 0.95 | very high | near-silent |

**Collision transient threading:** transient energy = collision_energy × (1 - absorption_coefficient). PhysicsWorker writes this value to `EmitterSlot.gain` as a per-block gain envelope (same lock-free atomic path as position). Audio thread reads `EmitterSlot.gain` per-block; no synthesis on audio thread. Finite-safe: energy clamped to [0..1]; minimum transient decay enforced.

---

### Pressure / Shockwave Impulse

One-shot radial force from a 3D origin point.

| Parameter | Type | Description |
|---|---|---|
| `shock_trigger` | trigger | one-shot APVTS button |
| `shock_origin` | Vec3 | origin in room space |
| `shock_strength` | float | peak impulse magnitude |
| `shock_radius` | float meters | influence boundary |

All **[new — pending parameter-spec.md]**. Emitters within `shock_radius` receive outward impulse = `shock_strength / r²` applied in a single worker tick. Rate-limited: minimum 100ms between triggers.

**DSP hook:** impulse energy per emitter (clamped) → gain transient written to `EmitterSlot.gain` by PhysicsWorker.

**Concurrency acceptance gate:** deterministic output across 3 runs with identical initial state when shockwave and inter-emitter collision resolve in the same tick.

---

## DSP Mapping Contract

All physics-to-DSP mappings follow the `reactive-av` contract: normalize → optional deadband → curve → smooth (configurable attack/release) → clamp → publish.

**Spread write arbitration:** multiple physics sources (attractor proximity, Boids density, spring phase, turbulence) each contribute an additive delta to `EmitterSlot.spread`. The PhysicsWorker sums all active contributions, then clamps the total to [0..1] before writing. No source overrides the APVTS `emit_spread` base value — all physics contributions are additive offsets layered on top of the base, clamped to the valid range. Choreography formation spread follows the same additive-offset rule (see choreography-lab-spec.md §1).

| Physics source | Normalization | Curve | DSP target |
|---|---|---|---|
| Attractor proximity | 0..1 (0=at attractor) | sigmoid | `spread` additive offset |
| Attractor crossing | binary event | gate | `gain` spike → decay |
| Boids density | neighbor count / max | linear | `spread` additive offset |
| Boids velocity | 0..max_speed | linear | Doppler (velocity field) |
| Cohesion-breakup event | binary event | gate | `gain` dip → decay |
| Inter-emitter collision energy | 0..1 clamped | log | `gain` transient envelope |
| Spring oscillation phase | 0..2π | sine | `spread` or `gain` additive offset |
| Turbulence magnitude | 0..1 | linear | `spread` jitter ±depth |
| Angular aim vs speaker angle | 0..π | cardioid | per-speaker gain (DirectivityFilter) |
| Field curl magnitude | 0..1 | linear | air absorption cutoff shift |
| Field divergence | 0..1 | linear | reverb send level |
| Shockwave impulse energy | 0..1 per emitter | log | `gain` transient envelope |
| Collision material absorption | 0..1 | linear | `gain` transient envelope |

---

## Threading Contract

- All simulation logic runs on `PhysicsWorker` thread at `rend_phys_rate` Hz.
- Results written to `EmitterSlot` via existing atomic pointer swap (ADR-0002).
- Angular physics: `ang_velocity` and `ang_drag` are PhysicsWorker-internal state; only `directivityAim` is written to `EmitterSlot`.
- All DSP-mapped values (gain, spread, velocity, directivityAim) clamped to finite range before audio thread reads them.
- Stall detection: >2 missed ticks → hold last valid EmitterSlot state.
- Per `physics-reactive-audio` skill: worst-case CPU must be measured at 64 emitters with all Tier A features enabled simultaneously before ship.

---

## ADR Status

**Resolved.** `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md` supersedes ADR-0003's three-layer model, records the four-layer chain rationale, authority-conflict resolution rules, and the colocated-tick arbitration decision.

---

## Validation Status

`partially tested` — the spec has isolated subsystem evidence behind parts of Tier A, but the full production-runtime architecture described here is not yet the authoritative plugin path.

**Tier A acceptance gates:**
- Worker thread CPU headroom: all Tier A features enabled simultaneously at 64 emitters, measured at 240Hz rate
- Inter-emitter collision determinism: identical initial conditions → identical output across 3 runs
- Spring oscillation frequency accuracy: measured vs analytical `ω = √(k/m)` within 2%
- Angular physics aim sweep: full yaw/pitch/roll sweep produces expected cardioid modulation curve
- DSP mapping clamp: no NaN or out-of-range values under adversarial inputs (NaN position, zero mass, overlapping emitters)
- Shockwave + collision concurrency: deterministic output across 3 runs with simultaneous events in same tick

**Tier B acceptance gates (Lab promotion criteria):**
- Flow field: CPU headroom at 16³ grid resolution with 64 advected emitters
- Flow field DSP hooks: `phys_field_air_cutoff_shift` and `phys_field_reverb_send` parameters added to parameter-spec.md before implementation
- Environmental presets: atomic parameter switch produces no audio-thread glitch (tested in pluginval)
- Material collision transients: finite-safe under 100 rapid repeated collisions in 1 second
- Shockwave: measured energy distribution matches analytical inverse-square within 5% at 8 sample points
