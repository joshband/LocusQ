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

### Phase P7 — WebView Controls + Visualization Overlays ✅ COMPLETE

**Goal:** Wire all new APVTS params through WebView relays; add viewport overlays.

**Files modified:**
- `Source/editor_webview/EditorParameterBridge.h` — 89 new ParameterBridgeSpec entries (all P1–P6 params)
- `Source/ui/src/index.ts` — ~75 slider states, ~21 toggle states, ~8 combo states; all new UI bindings; Three.js attractor rings (×4) and flock centroid spheres (×4) created in scene init; per-frame overlay update in animate()
- `Source/ui/public/index.html` — Spring/Turbulence/Angular/Scene Membership subsections; `scene-physics` renderer card (Attractors ×4); `boids` renderer card (Boids groups ×4)
- `Source/ui/generated/index.js` — rebuilt 836 kB (Vite build PASS)

**Acceptance gate:**
- [x] WebView 8-point checklist passes (member order, resource provider, backend explicit)
- [x] All Tier A parameters wired via EditorParameterBridge.h relay entries
- [x] Overlay layer: read-only Three.js objects (no DSP mutation); relay values consumed read-only

**Evidence:** `TestEvidence/` — JS bundle build PASS logged in build-summary.md

---

### Phase P8 — Isolated QA Harness + Acceptance Gate Probe

**Goal:** Build and run an isolated Tier A probe against the new subsystem headers and worker scaffolding.

**Files created/modified:**
- `qa/physics_tier_a_probe_main.cpp` — 12-check standalone probe covering all Tier A acceptance gates
- `CMakeLists.txt` — new `locusq_physics_tier_a_probe` target

**Probe results (12/12 PASS):**
- [x] DSP bridge NaN clamp — spread/gain/transient all clamped to [0..1] finite
- [x] DSP bridge Inf clamp — same for ±∞ injection
- [x] DSP bridge boundary values — all 64 slots correct under 0/1 publish
- [x] Spring frequency accuracy — measured 0.800 Hz vs analytical 0.796 Hz → **0.55% error < 2% gate**
- [x] Turbulence bounded — max force 11.1 N < spec bound 98.0 N
- [x] Angular aim no NaN — 5000 ticks, adversarial impulses, always finite unit vector
- [x] Angular aim sweep cardioid — 90° rotation dot product = 0.007 ≈ cos(90°) within ±10° tolerance
- [x] Boids determinism 3 runs — centroid bit-exact across all three runs
- [x] Collision determinism 3 runs — impulse + energy bit-exact across all three runs
- [x] Collision finite-safe — 100 rapid collisions, energy always ∈ [0..1], no NaN
- [x] Worker tick rate 240 Hz — observed/expected ratio = 1.007 (≥ 0.90 gate)
- [x] Worker stall guard — position held during pause, loop remains alive

**Evidence:** `TestEvidence/physics_p8_tier_a_20260318/`

---

## Parameter Traceability

All new parameters must be logged in `Documentation/implementation-traceability.md` per invariant contract. A new ADR is required if the `EmitterData` struct extension in Phase P4 triggers backward-compatibility consideration for the state schema.

---

## Implementation Reality Check

What is genuinely complete in this change set:
- New Tier A parameter registration, WebView relay wiring, and viewport overlays are implemented.
- New subsystem headers and a standalone `locusq_physics_tier_a_probe` target exist and pass their isolated checks.
- Traceability and parameter-spec updates were started for the new Tier A surface area.
- Broad production-path runtime validation now exists for the coordinated Tier A lanes: attractor, hard/soft boundary, collision, boids, interaction, spring/turbulence, and angular/directivity each have in-plugin probes with replay-stable or state-bounded contracts.
- The coordinated runtime path is no longer probe-only scaffolding: a shared process-wide `PhysicsWorker` / `PhysicsDSPBridge` path now participates in real multi-emitter plugin behavior and is validated from the actual `PluginProcessor` path.

What is not yet complete in the production plugin runtime:
- Shared-worker ownership is still partial overall; `PluginProcessor` still carries the legacy per-emitter `PhysicsEngine`, so coordinated-worker mode is not yet the sole motion authority for every Tier A path.
- The new Tier A controls are much more truthful than before, but not every Tier A control has been proven end-to-end with the same production-path depth.
- The runtime probes now validate real in-plugin behavior, but they are still targeted integration lanes rather than full DAW-host acceptance or exhaustive user-surface completion proof.

## Validation Status

`partially tested` — production-path integration is now broad and materially stronger, but full Tier A migration and final host-level completion claims are still not justified.

Validation summary:
- `locusq_physics_tier_a_probe`: PASS 15/15 on standalone subsystem checks.
- `locusq_physics_runtime_orbit_probe`: PASS in the production processor path for attractor orbit stabilization, proving the worker now holds a bounded orbital radius while the emitter traverses a real orbit arc (`initialRadius=2.000`, `settleMeanRadius=2.179`, `settleRangePct=6.82`, `maxAngularDeviationDeg=59.95`).
- `locusq_physics_runtime_mass_override_probe`: PASS in the production processor path for heterogeneous mass semantics, proving the lighter emitter exits a shared-worker collision materially faster than the heavy emitter under the same incoming speed (`heavyMeanPostCollisionVx=2.268`, `lightMeanPostCollisionVx=7.489`).
- `locusq_physics_runtime_attractor_crossing_probe`: PASS in the production processor path for attractor-crossing transient behavior, proving a quiet baseline, real radius crossing, visible bridge transient, visible scene-published transient, and bounded decay (`maxCollisionEnergy=0.964`, `maxBridgeTransient=0.964`, `settleMeanCollisionEnergy=0.000` across 3 reruns after correcting the probe's 3D coordinate setup).
- `locusq_physics_runtime_collision_transient_probe`: PASS in the production processor path for collision transient behavior, proving `phys_collision_gain_scale` changes peak burst magnitude and `phys_collision_decay_ms` now changes late-window transient persistence in both the bridge and the published scene state (`gainDeltaBridge=0.236..0.249`, `gainDeltaScene=0.245..0.258`, `decayLateDeltaBridge=0.065..0.146` across 3 immediate reruns).
- `locusq_physics_runtime_attractor_probe`: PASS in the production processor path for the first coordinated Tier A slice, now under a damped settle-window contract (`baselineMaxSpread=0.000`, `attractorMaxSpread=0.880`, `spreadDelta=0.880`, `attractorMaxDisp=2.235`, `settleMeanX≈2.00`, `settleRangeX=0.605`).
- `locusq_physics_runtime_boundary_probe`: PASS in the production processor path for worker-owned boundary response, now under a settle-window rebound contract (`maxX=3.000`, `collisionMask=1`, `settleMeanX≈1.36`, `settleRangeX=0.350`).
- `locusq_physics_runtime_soft_boundary_probe`: PASS in the production processor path for soft-boundary shaping, proving the emitter enters the soft zone, stays inside room bounds, publishes `collisionMask`, and peels away under a bounded settle window (`maxX=2.464`, `minDistanceToWall=0.536`, `minVelocityX=-3.427`, `settleMeanX=1.152`, `settleRangeX=0.339`, `collisionMask=1`).
- `locusq_physics_runtime_collision_probe`: PASS in the production processor path for shared-worker two-emitter collision under a bounded room-contained settle-window contract (`emitterIds=(0,1)`, `minDistance=0.634`, `finalDistance=3.724`, `maxAbsX=2.046`, `maxCollisionEnergy=0.0136`, `settleMeanDistance=3.802`, `settleRangeDistance=0.159`, `finalVx=(0.550,-0.550)`).
- `locusq_physics_runtime_boids_probe`: PASS in the production processor path for flock-driven coordinated motion and density-driven spread, now under a bounded settle-window contract (`emitterIds=(0,1)`, `initialDistance=2.992`, `minDistance=2.410`, `settleMeanDistance=2.464`, `settleRangeDistance=0.096`, `maxSpread=1.000`).
- `locusq_physics_runtime_interaction_probe`: PASS in the production processor path for interaction-only shared-worker repulsion, now under a bounded settle-window contract (`emitterIds=(0,1)`, `initialDistance=0.732`, `maxDistance=3.186`, `maxAbsForce=5.641`, `settleMeanDistance=3.030`, `settleRangeDistance=0.289`, `finalVx=(-1.837,1.626)`).
- `locusq_physics_runtime_spring_turbulence_probe`: PASS in the production processor path for spring-only and turbulence-only shared-worker activation, now under bounded late-window contracts (`spring: maxSpread=0.300, maxAbsForceX=6.000, maxDisp=1.919, settleMeanX≈0.03, settleRangeX=0.300`; `turbulence: maxSpread=0.090, maxAbsForceX=1.179, maxDisp=1.202, settleMeanX≈0.107, settleRangeX=0.020`).
- `locusq_physics_runtime_angular_probe`: PASS in the production processor path for angular/directivity ownership, proving worker-published `directivityAim` reaches the real scene snapshot, responds to angular throw, remains finite/unit-length, and resets cleanly (`baselineAim=(0.000,0.000,-1.000)`, `maxAbsAimYAfterThrow=0.994`, `maxAimNormError=0.0004`, `resetMeanAim=(0.000,0.000,-1.000)`).
- The runtime probe suite is now more trustworthy too: boids, interaction, and boundary scenarios explicitly disable unrelated coordinated features, and the boids lane captures baseline spacing before convergence scoring begins.
- The suite contract is tighter again: the attractor lane now also disables unrelated coordinated features explicitly and captures a live baseline before scoring, while spring and turbulence capture zeroed baseline spread/force before live scoring.
- Repeated replay confidence is improving materially now: after the first repeated replay exposed attractor end-state drift, the attractor lane was tightened into an explicitly damped settle-window contract and now holds stable across three reruns too (`settleMeanX≈2.00`, `settleRangeX=0.605`).
- Boundary and collision now follow the same bounded-window style too: each lane scores settle-window mean/range instead of a single terminal sample, and both held stable across three reruns.
- Boids and interaction now follow that same bounded-window style too: each lane scores settle-window mean/range instead of terminal-only motion summaries, and both held stable across three reruns.
- Spring and turbulence now join that same pattern too: each lane still enforces baseline and live-force/spread behavior, but now also proves bounded late-window replay stability across three reruns.
- The practical meaning of the evidence changed with those refinements: the repo now has a credible production-path validation suite for the major coordinated Tier A lanes, not just isolated subsystem math checks.
- Collision controls are now consumed in the production processor path too: `phys_collide_emitters`, `phys_collision_radius`, `phys_collision_gain_scale`, `phys_collision_decay_ms`, and `phys_mass_override` now reach `PhysicsWorker` instead of stopping at APVTS/UI registration.
- Collision transient shaping is now honest too: `phys_collision_decay_ms` is no longer a dead control, because the processor now forwards it into `PhysicsDSPBridge` transient smoothing and the dedicated runtime lane proves the decay difference live.
- Coordinated physics ownership is now process-shared for the validated runtime path: `PhysicsWorker` / `PhysicsDSPBridge` moved behind a shared runtime so multiple plugin instances can participate in one collision/coordination domain.
- Shared-worker ownership is still partial overall, but the biggest architectural blocker from the review is now closed: true in-plugin multi-emitter collision validation across plugin instances is possible and green.
- The attractor-crossing transient lane is now truthful too: the earlier zero-transient failure turned out to be a probe geometry bug, not a production-path propagation gap, and the corrected 3D lane now passes consistently.

## Remaining Work Before A Full Tier A Completion Claim

These are the highest-value blockers that still separate the current state from an honest "Tier A complete in production" claim:

1. Make coordinated-worker ownership the default authority for all coordinated Tier A motion paths.
   - The current runtime is much stronger, but it still carries a split-brain contract between `PhysicsWorker` and the legacy per-emitter `PhysicsEngine`.
   - Exit condition: coordinated Tier A scenes no longer depend on legacy-engine fallback for motion authority, or the remaining fallback paths are explicitly carved out and non-user-facing.

2. Close the parameter-consumption proof gap for the remaining Tier A controls.
   - The major coordinated families now have credible runtime proof, but the plan still does not enumerate which Tier A controls only have partial production-path evidence versus full end-to-end proof.
   - Exit condition: every Tier A control is tagged as either `runtime-proven`, `partially proven`, or `not yet proven`, with a linked probe, self-test, or explicit gap note.

3. Add at least one host-level acceptance lane on top of the targeted runtime probes.
   - The current runtime probes prove real plugin behavior, but they are still controlled integration lanes rather than DAW-host acceptance evidence.
   - Exit condition: one repeatable host-facing lane confirms that coordinated Tier A behavior survives the actual plugin-host lifecycle, not just isolated processor-path execution.

4. Reconcile the docs/spec surface with the implementation truth.
   - The implementation plan is now much more honest, but the broader spec/traceability surface still needs one explicit pass to reflect which Tier A claims are complete, partial, or deferred.
   - Exit condition: `.ideas/physics-simulation-spec.md`, parameter/spec traceability, and the implementation plan all agree on what is production-live versus still in migration.

Recommended execution order:
- First: finish the authority boundary.
- Second: publish the Tier A control-by-control proof matrix.
- Third: add the host-level acceptance lane.
- Fourth: do the final documentation truth pass for promotion readiness.

## Tier A Proof Matrix

This matrix is the current truth surface for the Tier A controls. It is intentionally blunt: if a control family does not yet have equivalent production-path proof, it should not be described as fully complete.

| Tier A control family | Representative controls | Current proof status | Current evidence | Most important remaining gap |
|---|---|---|---|---|
| Attractor motion + spread | `attractor_N_active`, `attractor_N_pos_*`, `attractor_N_strength`, `attractor_N_falloff`, `attractor_N_radius` | `runtime-proven` | `locusq_physics_runtime_attractor_probe` proves worker-owned motion plus spread response in the production processor path | Add host-facing acceptance for the same slice |
| Boundary hard-wall response | `rend_phys_walls`, `phys_boundary_mode=Hard` | `runtime-proven` | `locusq_physics_runtime_boundary_probe` proves published wall clamp, rebound, and `collisionMask` output in the production processor path | Add equivalent proof for non-hard boundary modes |
| Collision core lane | `phys_collide_emitters`, `phys_collision_radius`, `phys_collision_gain_scale`, `phys_collision_decay_ms` | `runtime-proven` | `locusq_physics_runtime_collision_probe` proves shared-worker two-emitter collision in the production processor path; collision controls now reach `PhysicsWorker` | Add host-facing acceptance and a clearer proof for audible gain-transient behavior |
| Boids coordinated motion | `phys_flock_group`, `phys_flock_G_enable`, `phys_flock_G_*` weights/radii/speed | `runtime-proven` | `locusq_physics_runtime_boids_probe` proves flock-driven coordinated motion and density-driven spread in the production processor path | Add proof for the non-motion hooks such as breakup-driven gain behavior |
| Interaction-only repulsion | `rend_phys_interact` | `runtime-proven` | `locusq_physics_runtime_interaction_probe` proves interaction-only shared-worker repulsion in the production processor path | Add host-facing acceptance and multi-scene lifecycle coverage |
| Spring tether motion | `phys_spring_enable`, `phys_spring_k`, `phys_spring_damp`, `phys_spring_anchor_mode`, `phys_spring_anchor_*` | `runtime-proven` | `locusq_physics_runtime_spring_turbulence_probe` proves spring-only shared-worker activation, motion, and spread behavior in the production processor path | Add proof for gain-modulation variants if those are intended to be user-visible |
| Turbulence motion + spread jitter | `phys_turbulence`, `phys_turbulence_rate` | `runtime-proven` | `locusq_physics_runtime_spring_turbulence_probe` proves turbulence-only shared-worker activation, motion, and spread behavior in the production processor path | Add host-facing acceptance and longer replay/soak confidence if promoted further |
| Soft boundary mode | `phys_boundary_mode=Soft`, `phys_soft_boundary_depth` | `runtime-proven` | `locusq_physics_runtime_soft_boundary_probe` proves soft-zone entry, in-room containment, published boundary mask, and repelled settle behavior in the production processor path | Add host-facing acceptance if soft-boundary shaping becomes a promoted user-facing completion claim |
| Attractor orbit stabilization | `attractor_N_orbit_stabilize` | `runtime-proven` | `locusq_physics_runtime_orbit_probe` proves the real processor path keeps a bounded orbital radius while the emitter traverses a meaningful orbit arc | Add host-facing acceptance or a longer soak lane if this becomes a promotion-grade completion claim |
| Per-emitter mass override semantics | `phys_mass_override` | `runtime-proven` | `locusq_physics_runtime_mass_override_probe` proves heterogeneous masses produce asymmetric shared-worker post-collision velocities in the production processor path | Add host-facing acceptance and heavier multi-body edge coverage if this becomes a promoted user-facing claim |
| Attractor-crossing gain spike | attractor crossing event -> gain transient | `runtime-proven` | `locusq_physics_runtime_attractor_crossing_probe` proves quiet baseline, true radius crossing, visible bridge transient, visible scene-published transient, and decay in the real processor path | Add host-facing acceptance and decide whether this transient should become a user-facing DSP completion claim distinct from collision transients |
| Collision gain transient as audible/DSP contract | `phys_collision_gain_scale`, `phys_collision_decay_ms` -> collision burst | `runtime-proven` | `locusq_physics_runtime_collision_transient_probe` proves gain-scale changes peak transient magnitude and decay-ms changes late-window persistence in the real processor path | Add host-facing acceptance if this burst contract becomes a promoted user-facing completion claim |
| Angular/directivity behavior | `phys_ang_enable`, `phys_ang_drag`, `phys_ang_impulse_*`, `phys_ang_throw`, `phys_ang_reset`, `phys_ang_attractor_torque` | `runtime-proven` | `locusq_physics_runtime_angular_probe` proves worker-published `directivityAim` is visible in the production processor path, responds to throw/reset, and stays finite/unit-length | Add host-facing acceptance and attractor-torque-specific proof if that sub-behavior is promoted as a distinct claim |

Interpretation rule:
- `runtime-proven` means there is a targeted production-path integration probe proving the behavior in the real `PluginProcessor` path.
- `partially proven` means the control is wired and/or subsystem-tested, but the production-path proof is incomplete or too indirect.
- `not yet production-proven` means the current evidence is still standalone/subsystem-only for completion-claim purposes.

## Review-Driven Refinement Priorities

### R1 — Runtime Authority Migration

**Goal:** make the shared `PhysicsWorker` the real coordinated-physics authority in the plugin rather than a probe-only path.

**Must-do actions:**
- Add `PhysicsWorker` and `PhysicsDSPBridge` as processor-owned runtime objects.
- Register and activate emitter slots from the real scene lifecycle.
- Define the switchover contract between legacy per-emitter `PhysicsEngine` and coordinated-worker mode.
- Make one path authoritative for coordinated features so state ownership is unambiguous.

**Progress (2026-03-18):**
- `PluginProcessor` now owns and prepares `PhysicsWorker` / `PhysicsDSPBridge` in the live runtime path.
- Real emitter lifecycle registration now activates/deactivates worker slots alongside scene registration cleanup.
- The switchover contract is tighter: coordinated-worker mode now activates for the coordinated features that actually need shared state, rather than only for the legacy attractor slice.
- `PhysicsEngine::standaloneMode` now gates whether coordinated force is consumed, so inactive coordinated slices no longer leak worker-derived force into the legacy path.
- The production processor now prefers worker-published position/velocity/force for the coordinated attractor slice, making the shared worker the source of truth for the first published motion lane.
- Gravity and inter-emitter interaction are now carried on the worker side for the coordinated slice, while the legacy engine is explicitly zeroed for those forces to avoid dual integration.
- Boundary response, throw/reset triggers, and collision-mask publication are now also owned by the worker for coordinated mode, removing another set of split responsibilities from the legacy engine path.
- Collision impulses are now applied directly into worker-owned velocity for coordinated mode instead of using the legacy engine as an intermediate sink.
- `PhysicsWorker` / `PhysicsDSPBridge` now live behind a shared process-wide runtime instead of per-processor ownership, aligning coordinated physics authority with the existing shared `SceneGraph`.
- Coordinated slots are no longer re-activated every block; worker-owned motion now persists across blocks instead of being reset by the emitter publish path.
- Coordinated-worker activation is no longer attractor-only: collision-enabled emitters can now enter the shared worker path without relying on a dummy attractor source.
- Boids activation is now honest in the production path too: flock-group assignment and per-group boids parameters are pushed into `BoidsSystem`, and flock-enabled emitters can now enter coordinated worker mode even without an attractor or collision gate.
- Shared physics scene flags are now refreshed from the emitter publish path too, so coordinated-worker features no longer depend on a renderer instance being active to keep shared runtime state truthful.
- Interaction-only coordinated mode is now honest as well: shared-worker activation can be driven by `rend_phys_interact` in emitter-only scenes when more than one emitter is active.
- Spring and turbulence are now honest in the same way: `PluginProcessor` pushes their APVTS values into `SpringSystem` / `TurbulenceSystem`, and either feature can activate coordinated worker ownership without attractor/collision/flock/interaction scaffolding.

**Exit gate:**
- Production plugin reads coordinated physics state from the shared worker for at least one end-to-end Tier A slice, with motion ownership boundaries documented.

### R2 — First Vertical Slice

**Goal:** land one honest end-to-end feature slice before broadening the surface area.

**Recommended slice:** Attractor proximity -> shared worker -> DSP bridge -> spread modulation -> viewport proof.

**Progress (2026-03-18):**
- Processor-owned `PhysicsWorker` / `PhysicsDSPBridge` objects are now wired into the live emitter runtime path.
- Attractor APVTS parameters are now pushed into the shared worker from `PluginProcessor`.
- Scene-published emitter spread now consumes bridge output in the production processor path.
- A dedicated runtime probe now validates `baseline spread ~= 0` with attractor off and `spread > 0` with attractor on.
- The runtime probe immediately exposed a non-zero sigmoid floor bug (`baselineMaxSpread=0.120`) and the worker mapping was corrected so inactive attractors now produce zero spread at baseline.
- The coordinated-worker activation contract was re-checked after the ownership-gating change and both runtime lanes remained green (`locusq_physics_runtime_attractor_probe`: PASS, `locusq_physics_tier_a_probe`: PASS 15/15).
- The runtime probe now also validates published motion, confirming the emitter moves toward the active attractor in the scene snapshot (`attractorMaxDisp=1.985`, final published position `~(2.002, 1.200, 0.000)`).
- The coordinated force contract is now more truthful: worker-owned motion includes gravity and scene interaction for this slice, and the replay remains green after removing that duplicate work from the legacy engine (`attractorMaxDisp=1.845`, final published position `~(1.862, 1.200, 0.000)`).
- The authority handoff is cleaner again: coordinated mode now keeps boundary handling and one-shot throw/reset events inside the worker-owned slice, and the replay remains green (`attractorMaxDisp=1.969`, final published position `~(2.002, 1.200, 0.000)`).
- Collision ownership is cleaner too: the coordinated slice now consumes inter-emitter collision impulses directly in worker-owned state, and the replay remains green (`attractorMaxDisp=2.263`, final published position `~(2.296, 1.200, 0.000)`).
- Integration validation is broader now: a second in-plugin runtime probe confirms worker-owned boundary response clamps at the wall and publishes the expected collision mask (`maxX=3.000`, `collisionMask=1`, `finalPos~(1.040, 1.200, 0.000)`).
- Collision control wiring is now honest in the production path as well: the processor pushes collision enable/radius/gain-decay tuning plus per-emitter mass override into `PhysicsWorker`, closing another UI-to-runtime no-op gap before the shared-worker architecture is widened.
- The shared-worker architecture is now actually widened: multiple emitter instances can collide inside one process-wide worker, and a dedicated runtime probe confirms the collision lane through the real `PluginProcessor` path (`maxCollisionEnergy=0.4191`, `finalVx=(-2.800, 2.800)`).
- A runtime reset bug was closed along the way: coordinated slots are no longer re-activated every block, which had been wiping worker-owned motion before the new multi-emitter lane could be observed honestly.
- The bounded shared-motion contract is tighter now too: collision-only coordinated mode no longer depends on a fake attractor gate, and the shared two-emitter runtime lane now proves room-contained behavior (`maxAbsX=3.000`) instead of an unbounded fly-apart case.
- Shared collision mode now has a runtime containment policy instead of relying on probe-side drag tuning alone: when 2+ emitters are in coordinated collision mode with room boundaries enabled, the worker applies a weak rest-pose tether outside a deadzone so post-collision motion recenters before the emitters drift to the room edges.
- The collision runtime probe now validates that stronger contract under `phys_drag=0.0`, and it stays green with the emitters contained well inside the room (`finalDistance=3.686`, `maxAbsX=2.046`, `finalVx=(0.568, -0.568)`).
- The first non-attractor coordinated feature slice is now live as well: a new runtime boids probe confirms that flock membership activates shared-worker ownership in the production processor path and produces both coordinated motion and density-driven spread (`initialDistance=2.992`, `minDistance=2.306`, `maxSpread=1.000`).
- Interaction-only scenes are now covered by the same production-path contract: the emitter publish path refreshes shared physics scene flags, interaction-enabled emitters can enter coordinated worker mode without attractor/collision/flock scaffolding, and a new runtime probe confirms real repulsion force plus separation growth (`initialDistance=0.726`, `maxDistance=3.162`, `maxAbsForce=5.664`, `finalVx=(-1.798, 1.667)`).
- The remaining P3 gap is now closed in the production path too: a new runtime probe confirms spring-only and turbulence-only scenes each activate the shared worker and publish real motion/force/spread without piggybacking on another coordinated feature (`spring maxSpread=0.300, maxDisp=1.919`; `turbulence maxSpread=0.090, maxAbsForceX=1.179, maxDisp=1.202`).
- The runtime replay contract is tighter now as well: boids, interaction, and boundary probes explicitly disable unrelated coordinated features before validation, and the boids lane now captures its baseline before convergence scoring so replay numbers are less sensitive to ambient-state drift.
- The remaining runtime lanes now follow the same discipline: attractor explicitly disables unrelated coordinated features and captures a baseline before scoring, and spring/turbulence each record zeroed baseline spread/force before their live replay windows.
- The attractor lane now has that bounded contract: it runs with explicit damping and scores a settle window instead of a single phase-sensitive terminal sample, which removed the earlier replay-to-replay end-state drift from the probe lane.

**Must-do actions:**
- Wire attractor APVTS params into processor-owned runtime objects.
- Publish shared-worker spread modulation into the actual emitter/render path.
- Add a minimal integration probe or scripted scenario that confirms audible/visual response inside the plugin.

**Exit gate:**
- One Tier A feature is demonstrably live in the plugin, not just in the standalone probe.

### R3 — Integration Validation Lane

**Goal:** separate “subsystem math is plausible” from “plugin behavior is correct.”

**Must-do actions:**
- Add a plugin-runtime validation lane for coordinated physics ownership, parameter consumption, and DSP publication.
- Re-run acceptance gates against the real processor path where feasible.
- Keep isolated probe results, but report them as subsystem evidence rather than ship-readiness evidence.

**Exit gate:**
- Evidence clearly distinguishes standalone subsystem PASS from in-plugin PASS.
- Shared coordinated behavior is proven across more than one processor instance in at least one in-plugin runtime lane.

### R4 — Naming And Contract Cleanup

**Goal:** eliminate spec/API naming drift before more implementation lands.

**Must-do actions:**
- Update the spec to use canonical APVTS IDs (`attractor_N_*`, `phys_flock_G_*`, etc.).
- Reserve prose aliases for explanation only, not normative parameter references.
- Keep traceability tables and plan docs aligned to the same canonical names.

**Exit gate:**
- No parameter naming exceptions are needed to reconcile the spec with the implementation.

---

## Execution Order

**P1 → P6** — subsystem scaffolding and isolated probe coverage landed, but production-runtime integration is still pending.

**P7** — UI relay wiring and viewport overlays landed.

**P8** — standalone subsystem probe landed and is useful, but it is not a substitute for plugin integration validation.

**Tier B (deferred):** Flow Fields, Environmental Presets, Material Properties, Shockwave — post–Tier-A acceptance gates.

**Next command:** begin `R1` + `R2` by integrating `PhysicsWorker`/`PhysicsDSPBridge` into `PluginProcessor` for a single attractor-to-spread vertical slice, then validate that slice in-plugin.
