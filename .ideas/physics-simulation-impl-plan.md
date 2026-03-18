---
Title: LocusQ Physics Simulation — First-Pass Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Physics Simulation Feature — First-Pass Action Plan

## Dream Phase Synthesis

**Concept:** Extend LocusQ's isolated per-emitter `PhysicsEngine` into a coordinated multi-emitter scene simulation. A shared `PhysicsWorker` thread replaces per-emitter isolation for features requiring inter-emitter awareness (Boids, collisions, attractors). DSP hooks bridge simulation state to spread/gain/Doppler modulation following the `reactive-av` pipeline contract.

**Scope boundary for first pass:** Tier A only. Tier B (Flow Fields, Environmental Presets, Material Properties, Shockwave) deferred post–Tier-A acceptance gates.

---

## Architecture (Plan Phase)

### Existing Baseline Capabilities (V1 `PhysicsEngine`)

| Feature | Status |
|---|---|
| Per-emitter worker thread, `rend_phys_rate` tick | ✅ present |
| Lock-free double-buffer via atomic `readIndex` | ✅ present |
| Gravity, drag, elasticity, friction, wall collision (hard) | ✅ present |
| `throw` / `reset` triggers | ✅ present |
| `EmitterData` position/velocity/force in `SceneGraph.h` | ✅ present |

### New Architectural Components Required

```
PhysicsScene (new)
├── PhysicsWorker thread (single shared, replaces N per-emitter threads for coordinated features)
│   ├── AttractorSystem [up to 4 sources]
│   ├── BoidsGroup [up to 4 groups × 64 emitters]
│   ├── CollisionSystem [O(n²) ≤64 emitters]
│   ├── SpringSystem [per-emitter optional]
│   ├── TurbulenceSystem [per-emitter independent noise seed]
│   └── AngularPhysicsSystem [per-emitter ang_velocity/ang_drag]
├── PhysicsDSPBridge (new — publishes normalized DSP-mapped values per emitter)
│   └── follows normalize→deadband→curve→smooth→clamp pipeline
└── SoftBoundaryMode enum extension on existing wall handler
```

### Threading Contract

- `PhysicsWorker` owns all coordinated-feature state. Independent emitters (no Boids/collision membership) may stay on their existing `PhysicsEngine` thread.
- Results written to `EmitterSlot` via the existing atomic pointer swap (ADR-0002 preserved).
- `PhysicsDSPBridge` publishes DSP-mapped values to a separate atomic slot consumed read-only by `processBlock()`.
- Stall guard: if worker misses >2 ticks → hold last valid positions and freeze DSP values.

### DSP Mapping Pipeline (per `reactive-av` contract)

```
physics_value → normalize(domain) → deadband(optional) → curve(linear/sigmoid/log)
              → smooth(attack/release) → clamp(0..1) → publish to DSP target
```

### Complexity

**Score: 5/5** — phased implementation required.

---

## Design Phase (UI for New Physics Controls)

The `ui_framework` is `webview`. New controls integrate into the existing **EMITTER** and **SCENE** panels via the established APVTS relay → WebView binding pattern.

### New Control Surfaces

**EMITTER panel additions:**
- Spring sub-section: `phys_spring_enable` toggle, `phys_spring_k` knob, `phys_spring_damp` knob, `phys_spring_anchor_mode` enum picker
- Turbulence row: `phys_turbulence` knob, `phys_turbulence_rate` knob
- Angular row: `phys_ang_enable` toggle, `phys_ang_drag` knob, `phys_ang_impulse_x/y/z` mini-sliders, `phys_ang_throw` / `phys_ang_reset` buttons
- Mass override: `phys_mass_override` knob (0 = use global default)
- Collision radius: `phys_collision_radius` knob (shown when `phys_collide_emitters` is on)

**SCENE panel additions:**
- Attractor strip (×4 slots): position XYZ, strength, falloff picker, radius, `attractor_orbit_stabilize` toggle
- Boids group strip (×4 slots): sep/align/cohesion weight + radius knobs, max_speed, group membership selector
- `phys_collide_emitters` global toggle
- `phys_boundary_mode` enum picker (hard / soft / passthrough)

**Visualization additions (viewport layer — read-only, no DSP mutation):**
- Per-emitter velocity vectors (existing `rend_viz_vectors` extended)
- Flock centroid sphere (toggleable)
- Attractor field radius rings (color-coded by sign)

**Design deliverables:**
- `Design/physics-v1-ui-spec.md` — control placement and layout spec
- `Design/physics-v1-style-guide.md` — visual treatment for attractor/boids overlays
- JS additions to `Source/ui/public/js/index.js` — new relay bindings per APVTS parameter

---

## Implementation Plan (Impl Phase)

Phased, complexity=5. Each phase must pass its acceptance gate before advancing.

---

### Phase P1 — Infrastructure: `PhysicsWorker` & `PhysicsDSPBridge`

**Goal:** Single shared worker loop; DSP bridge with atomic publish; stall guard.

**Files to create/modify:**
- `Source/PhysicsWorker.h/.cpp` — scene-level tick loop, emitter state aggregation, stall detection
- `Source/PhysicsDSPBridge.h` — per-emitter DSP-mapped value struct, atomic slot, normalize→curve→smooth→clamp pipeline
- `Source/PhysicsEngine.h` — mark per-emitter threads as "standalone mode" (no coordinated features)

**Acceptance gate:**
- [ ] Worker ticks at configured `rend_phys_rate` with ≤2-tick stall hold verified deterministically
- [ ] `PhysicsDSPBridge` publishes finite values; NaN/inf injection test passes
- [ ] CPU headroom logged at 64 emitters with zero coordinated features active

---

### Phase P2 — Attractors & Soft Boundary

**Goal:** Force sources, orbital stabilization, soft boundary repulsor.

**Files:**
- `Source/AttractorSystem.h/.cpp` — up to 4 force sources, falloff enum, orbit-stabilize tangential correction
- `Source/PhysicsEngine.h` — extend `SoftBoundary` mode into existing wall handler

**New APVTS params:** `attractor_pos_N_x/y/z`, `attractor_strength_N`, `attractor_falloff_N`, `attractor_radius_N`, `attractor_orbit_stabilize_N` (×4), `phys_boundary_mode`, `phys_soft_boundary_depth`

**DSP hooks wired in `PhysicsDSPBridge`:**
- Attractor proximity → `spread`
- Attractor crossing event → `gain` spike

**Acceptance gate:**
- [ ] Orbital stabilization holds constant radius ±5% under 10-tick soak
- [ ] Soft boundary: emitter never crosses wall surface under adversarial push
- [ ] DSP bridge clamp: no out-of-range values under fuzz input

---

### Phase P3 — Spring / Pendulum Oscillator + Turbulence

**Goal:** Per-emitter spring tether and stochastic force injection.

**Files:**
- `Source/SpringSystem.h` — per-emitter `k`, `damp`, `anchor_mode`, `anchor_pos`
- `Source/TurbulenceSystem.h` — one-pole filtered noise per emitter, independent seeds

**New APVTS params:** `phys_spring_enable`, `phys_spring_k`, `phys_spring_damp`, `phys_spring_anchor_mode`, `phys_spring_anchor_x/y/z`, `phys_turbulence`, `phys_turbulence_rate`

**DSP hooks:**
- Spring oscillation phase → `spread` / `gain` LFO
- Turbulence magnitude → `spread` jitter

**Acceptance gate:**
- [ ] Spring frequency measured vs `ω = √(k/m)` within **2%** (analytical comparison test)
- [ ] Turbulence bounded: max impulse = `turbulence × mass × 9.8` verified at boundary values

---

### Phase P4 — Angular Physics

**Goal:** Dynamic `directivityAim` driven by angular integration.

**Files:**
- `Source/SceneGraph.h` — add `ang_velocity: Vec3`, `ang_drag: float` to `EmitterData`
- `Source/AngularPhysicsSystem.h` — per-emitter angular integration, `attractor_torque_strength`

**New APVTS params:** `phys_ang_enable`, `phys_ang_drag`, `phys_ang_impulse_x/y/z`, `phys_ang_throw`, `phys_ang_reset`, `attractor_torque_strength`

**DSP hook:** aim angle vs speaker angle → cardioid modulation via existing `DirectivityFilter` (rendering path unchanged)

**Acceptance gate:**
- [ ] Aim vector sweep test: expected cardioid curve matches within 5% tolerance
- [ ] `directivityAim` never produces NaN under adversarial impulse sequence

---

### Phase P5 — Boids Flocking

**Goal:** Up to 4 flock groups, 64 emitters max, standard 3-rule boids on worker thread.

**Files:**
- `Source/BoidsSystem.h/.cpp` — separation / alignment / cohesion per group, centroid tracking

**New APVTS params:** per-group `boids_sep_weight`, `boids_align_weight`, `boids_coh_weight`, `boids_sep_radius`, `boids_align_radius`, `boids_cohesion_radius`, `boids_max_speed` (×4 groups)

**DSP hooks:**
- Flock density (neighbor count / max) → `spread`
- Per-emitter velocity magnitude → Doppler (existing velocity field)
- Cohesion-breakup event → gain dip

**Acceptance gate:**
- [ ] CPU headroom at 64 emitters with 4 active groups within worker thread budget
- [ ] Determinism: 3 independent runs from same seed produce identical centroids at tick 100

---

### Phase P6 — Hard-Body Inter-Emitter Collision + Per-Emitter Mass Override

**Goal:** O(n²) impulse resolution; heterogeneous mass support.

**Files:**
- `Source/CollisionSystem.h` — emitter-pair overlap detection, impulse resolution (spec formula)
- `Source/PhysicsEngine.h` — add `phys_mass_override` per emitter slot

**New APVTS params:** `phys_collide_emitters`, `phys_collision_radius`, `phys_collision_gain_scale`, `phys_collision_decay_ms`, `phys_mass_override` (per emitter slot)

**DSP hook:** collision energy → transient gain burst (scaled + decaying envelope)

**Acceptance gate:**
- [ ] Determinism: same initial conditions → same output across 3 runs
- [ ] O(n²) CPU headroom at 64 emitters logged
- [ ] Finite-safe: no energy blow-up under rapid repeated collisions

---

### Phase P7 — WebView Controls + Visualization Overlays

**Goal:** Wire all new APVTS params through WebView relays; add viewport overlays.

**Files:**
- `Source/ui/public/js/index.js` — relay bindings for all Phase P1–P6 parameters
- `Source/ui/public/index.html` — new panel sections (attractor strip, boids strip, spring section, angular row)
- `Source/PluginEditor.h/.cpp` — relay declarations in correct member order (Relays → WebView → Attachments)
- Viewport overlay layer in Three.js scene (velocity vectors, flock centroid sphere, attractor rings)

**Acceptance gate:**
- [ ] WebView 8-point checklist passes (member order, resource provider, backend explicit)
- [ ] All Tier A parameters appear in UI with correct range and label
- [ ] Overlay layer: no DSP/scene state mutation confirmed via RT audit

---

## Parameter Traceability

All new parameters must be logged in `Documentation/implementation-traceability.md` per invariant contract. A new ADR is required if the `EmitterData` struct extension in Phase P4 triggers backward-compatibility consideration for the state schema.

---

## Validation Status

`not tested` — plan only. Tier A acceptance gates above are the promotion criteria per `.ideas/physics-simulation-spec.md`.

---

## Execution Order

**P1 → P2 → P3 → P4 → P5 → P6 → P7**, one phase per session, acceptance gate required before advancing.

**Next command:** Begin Phase P1 with `/impl` — create `Source/PhysicsWorker.h/.cpp` and `Source/PhysicsDSPBridge.h`.
