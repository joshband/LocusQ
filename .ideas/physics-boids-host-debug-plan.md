---
Title: Physics Boids Host Debug Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-03-19
Last Modified Date: 2026-03-20
---

## 2026-03-20 Reconciliation — Resolved by BL-102

This plan is now historical. All investigation goals are complete.

**Resolution:** The REAPER multi-instance boids spread lane failure was diagnosed and fixed as `BL-102`. The root cause was skewed boids-parameter normalization in the REAPER Lua gate script. After correcting the parameter write, both the duplicate-track and prepared-dual REAPER session shapes pass (`active_peak_spread=1.000`). The controlled two-instance processor debug probe also confirms the full boids chain — density, bridge, APVTS, async pending/published, and scene — all reach `1.000` on both instances.

**Canonical authority:** `Documentation/backlog/done/bl-102-physics-host-visible-coordinated-lanes-and-reaper-multi-instance-boids-truthfulness.md`

This file is retained as planning context only. Do not use it to infer open work.

# Physics Boids Host Debug Plan

## Goal

Resolve the gap between the passing runtime boids lane and the failing REAPER boids-spread host lane.

Plain-language summary:
- The boids system works in the real processor path.
- REAPER can see the attractor spread lane and the transient lanes.
- REAPER still cannot see boids-driven spread in the dual-instance host scenario.
- The next work should target the host/shared-lifecycle boundary, not the core boids algorithm.

## Complexity Score

**Score: 4/5**

**Rationale:**
- This is not a simple parameter wiring issue anymore.
- The runtime probe already proves boids motion and spread on the current build.
- The failing path depends on multi-instance shared-worker behavior, host lifecycle timing, and host-visible mirror publication.
- The likely fix will require careful instrumentation across processor, shared runtime, and host observation surfaces without breaking realtime safety.

## Architecture View

### Core Components Involved

- `PluginProcessor`
  - reads APVTS boids params
  - assigns flock groups
  - activates coordinated worker mode
  - publishes host-visible spread mirrors
- `PhysicsSharedRuntime`
  - owns the shared `PhysicsWorker` / `PhysicsDSPBridge`
  - is responsible for cross-instance coordination
- `PhysicsWorker`
  - builds the boids snapshot
  - computes flock steering
  - publishes `spreadMod`
- `PhysicsDSPBridge`
  - smooths and clamps spread values
- REAPER host gate
  - duplicates tracks
  - applies boids params to two instances
  - reads `phys_out_spread_mod_N`

### Processing Chain

```text
REAPER track params
→ APVTS boids params in each PluginProcessor
→ shared-worker scene activation / emitter-group assignment
→ BoidsSystem snapshot + steering
→ PhysicsWorker spreadMod publication
→ PhysicsDSPBridge smoothing/clamp
→ phys_out_spread_mod_N host-visible mirror
→ REAPER gate observation
```

### Current Truth

Runtime path:
- PASS
- `locusq_physics_runtime_boids_probe` reports `maxSpread=1.000`

Host path:
- FAIL twice
- REAPER sees:
  - dual-instance parameter registration
  - quiet baseline
- REAPER does not see:
  - any spread rise after boids enable

## Hypothesis Matrix

### Hypothesis 1: Shared-worker membership is not forming in the host scenario

What it would mean:
- duplicated REAPER tracks do not end up active in the same coordinated flock scene
- boids runtime probe still passes because it controls processor lifecycle more tightly

Signals to capture:
- active emitter slot ids per instance
- shared runtime acquired/released counts
- `boidsSystem.setEmitterGroup(...)` values per instance
- coordinated activation decision per instance
- active coordinated slot count

### Hypothesis 2: Boids spread is real internally but not reaching the host-visible spread mirror in multi-instance scenes

What it would mean:
- boids force/spread exists in `PhysicsWorker`
- `PhysicsDSPBridge` may publish it
- `phys_out_spread_mod_N` host-notify path may not reflect the right slot or scene timing

Signals to capture:
- worker-side `boidsSpread`
- bridge-side `dspValues.spreadMod`
- raw APVTS `phys_out_spread_mod_N`
- async host-notify pending/published values

### Hypothesis 3: The host gate config still does not match the runtime boids probe closely enough

What it would mean:
- the host lane is asking boids to appear in a scene that differs materially from the passing runtime probe

Signals to capture:
- exact param dump for both instances after gate setup
- confirmed `mode=Emitter`
- confirmed `phys_flock_group=Group 1`
- confirmed `phys_flock_0_enable=1`
- confirmed positions and velocities match intended setup

### Hypothesis 4: REAPER track duplication creates a lifecycle/copy state issue

What it would mean:
- duplicated plugin instances may inherit state that prevents correct shared-worker registration
- a pre-built two-instance project could behave differently from track duplication

Signals to capture:
- whether a manually prepared two-instance RPP behaves the same as runtime duplication
- whether instance order or registration sequence changes the result

## Implementation Strategy

### Phase 1: Instrumentation

- Add temporary test-only diagnostics for the boids host lane:
  - active emitter slot id
  - assigned flock group
  - coordinated-mode enabled flag
  - worker-side boids density/spread
  - bridge-side spread value
  - host-published spread mirror value
- Prefer test-only readback or status JSON over ad-hoc console guessing.

Exit condition:
- we can say exactly where the boids signal disappears:
  - before worker activation
  - inside worker/bridge
  - between bridge and host mirror
  - inside REAPER lifecycle/setup

### Phase 2: Reproduce Under Controlled Host Variants

- Keep the current duplicate-track gate.
- Add one comparison variant:
  - a prebuilt two-instance REAPER project, or
  - a gate path that reopens with two explicit prepared instances before playback
- Compare:
  - duplicated-track path
  - prebuilt-two-instance path

Exit condition:
- we know whether duplication is part of the bug.

### Phase 3: Fix the Narrowest Proven Boundary

If shared-worker activation is missing:
- fix registration/activation sequencing across instances

If worker/bridge spread exists but mirror is dark:
- fix host-visible spread publication for multi-instance scenes

If host setup is mismatched:
- align the boids REAPER gate with the runtime probe contract

Exit condition:
- the REAPER boids-spread gate shows non-zero active spread after boids enable.

### Phase 4: Harden and Promote

- rerun the boids host gate at least 3 times
- rerun runtime boids probe to confirm no regression
- sync plan/evidence/status truth surfaces

Exit condition:
- boids host lane moves from failing repro to repeatable PASS, or remains FAIL with a deeper narrowed diagnosis.

## Parameter Mapping

| Parameter | Component | Function | Current host risk |
|---|---|---|---|
| `phys_flock_group` | `PluginProcessor` → `BoidsSystem` | assigns emitter to flock group | group may not apply correctly across duplicated host instances |
| `phys_flock_0_enable` | `BoidsSystem` | enables group behavior | host lane may set it correctly but shared worker may not reflect it |
| `phys_flock_0_sep_weight` | `BoidsSystem` | separation steering weight | likely low risk once group activation is proven |
| `phys_flock_0_align_weight` | `BoidsSystem` | alignment steering weight | likely low risk once group activation is proven |
| `phys_flock_0_coh_weight` | `BoidsSystem` | cohesion steering weight | likely low risk once group activation is proven |
| `phys_flock_0_*_radius` | `BoidsSystem` | flock neighborhood radii | low-to-medium risk; scene config mismatch could suppress visible flocking |
| `phys_out_spread_mod_N` | `PluginProcessor` host mirror | DAW-visible spread observation | current highest-risk boundary |

## Dependencies

**Required JUCE / runtime surfaces:**
- `juce_audio_processors`
- `juce_core`
- current APVTS + AsyncUpdater host notification path

**Repo dependencies:**
- `PhysicsSharedRuntime`
- `PhysicsWorker`
- `PhysicsDSPBridge`
- REAPER headless gate wrappers/scripts

## Risk Assessment

**High Risk**
- Multi-instance shared-worker activation differs between runtime probes and real REAPER duplication
- Host-visible spread mirroring may be correct for single-instance scenes but incomplete for shared multi-instance scenes

**Medium Risk**
- Gate setup may still miss one material parameter or lifecycle step
- Fixing host visibility could accidentally perturb freeze semantics or automation expectations

**Low Risk**
- Runtime boids core algorithm appears healthy on the current build
- Existing attractor spread host lane proves the spread mirror pattern is viable in REAPER

## Recommended First Actions

1. Add a boids-specific test readback surface that reports, per instance:
   - emitter slot id
   - flock group
   - coordinated-mode active
   - worker boids density/spread
   - bridge spread
   - published host spread
2. Teach the REAPER boids gate to write those diagnostics into `status.json`.
3. Run the gate once with duplicated-track setup and once with a prebuilt two-instance project.
4. Fix the first boundary where the signal disappears.

## Success Criteria

- REAPER boids host gate reports:
  - `gate_a_param_reg=true`
  - `gate_b_quiet_baseline=true`
  - `active_peak_spread > 0.20`
  - `gate_c_boids_visible=true`
- runtime boids probe remains PASS on the same build
- evidence docs explain whether the resolution was:
  - shared-worker activation
  - spread host mirror publication
  - host setup mismatch

## Continuation Recommendation

Best next step: implement Phase 1 instrumentation first.

Why:
- we already have enough failing host evidence
- another blind gate tweak is unlikely to teach us much
- the missing information is internal state at the host/runtime boundary

## Backlog Routing Amendment (2026-03-19)

### Decision

Treat this as a **net-new backlog intake**, not as an amendment to one of the currently open BL items.

### Why A Net-New Item Is The Better Fit

- There is no active open BL that cleanly owns "physics host-visible coordinated behavior truthfulness in REAPER multi-instance scenes."
- The closest historical items are already closed and too broad or too tooling-focused:
  - `BL-024` proved the REAPER automation baseline.
  - `HX-03` covered REAPER multi-instance stability.
  - `BL-100` delivered downstream desktop/operator tooling, not this product-runtime bug.
- The current blocker is not generic physics completion anymore. It is a narrowed host/runtime boundary issue with its own replay cost, evidence needs, and likely temporary diagnostics.

### Proposed Backlog Shape

**Recommended draft ID:** `BL-102`

**Working title:** `Physics host-visible coordinated lanes and REAPER multi-instance boids truthfulness`

**Recommended priority:** `P1`

**Recommended track:** `E`

### Scope Of The Proposed BL

- Close the boids REAPER host lane gap without weakening the already-green runtime probes.
- Compare duplicated-track REAPER setup versus a prebuilt two-instance project.
- Use REAPER headless + Lua as the primary deterministic lane.
- Add screenshots and desktop/robotic automation only where Lua cannot reveal UI-only or host-only state.
- Promote the boids host lane from failing repro to repeatable PASS, or document the narrower host-specific root cause honestly.

### Explicit Non-Scope

- Reworking the boids core algorithm
- Broad physics Tier A completion claims
- General-purpose REAPER automation framework redesign
- New UI product features unrelated to host-observation truth

### Relationship To Existing Physics Closeout Work

- Keep the broad physics closeout plan as the umbrella for Tier A truthfulness.
- Split this REAPER boids host-lane blocker into its own backlog-shaped intake so it has:
  - a single owner boundary
  - its own replay/cost plan
  - explicit success criteria
  - room for temporary diagnostics and host-only tooling

### Recommended Immediate Follow-On

1. Draft the intake as a proposed new backlog item.
2. Leave `Documentation/backlog/index.md` unchanged until owner confirmation.
3. Continue implementation/debugging against this plan meanwhile, with evidence feeding back into the draft intake.
