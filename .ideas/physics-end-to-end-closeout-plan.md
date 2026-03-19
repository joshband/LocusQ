---
Title: LocusQ Physics End-to-End Closeout Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-20
---

# Physics End-to-End Closeout Plan

## Current Status Note (2026-03-20)

This plan is now a historical umbrella closeout plan, not the canonical authority for current backlog state.

What changed:
- `BL-102` is now `Done` in the canonical backlog/runbook surfaces and should no longer be read as an open blocker inside this plan.
- The broad Tier A runtime/host proof surface is materially stronger than this document's original 2026-03-18 snapshot.
- The main remaining open thread from this plan is the formal physics DAW-automation verification and closeout pass, which now needs its own canonical backlog owner rather than living only as an unchecked section here.

Canonical authority:
- `Documentation/backlog/index.md`
- `Documentation/backlog/done/bl-102-physics-host-visible-coordinated-lanes-and-reaper-multi-instance-boids-truthfulness.md`
- `.ideas/physics-simulation-impl-plan.md`
- `.ideas/physics-daw-automation-spec.md`

Historical-plan note:
- the old C4 task list below is no longer fully current as written
- `.ideas/parameter-spec.md` and `Documentation/implementation-traceability.md` already include the DAW automation mirror/freeze rows
- the former BL-106-specific gap is now closed by the canonical wrapper/evidence packet and synchronized truth surfaces

## Scope

Closes the remaining gaps across three specs in one ordered sequence:

- `.ideas/physics-simulation-spec.md` — Tier A completion
- `.ideas/physics-simulation-impl-plan.md` — R1–R4 blockers
- `.ideas/physics-daw-automation-spec.md` — all acceptance gates

**Tier B (Flow Fields, Environmental Presets, Material Properties, Shockwave) remains explicitly deferred.** This plan does not touch Tier B.

---

## Current Truth Summary (historical baseline from 2026-03-18)

| Area | State |
|---|---|
| Tier A subsystem headers (P1–P6) | All present |
| Tier A WebView UI + relays (P7) | Complete |
| Tier A standalone probe (P8) | 15/15 PASS |
| Tier A runtime probes (R2/R3) | All green — production processor path validated |
| Split-brain authority (R1) | Historical baseline only — later closed in the implementation-plan truth surface |
| Host-facing acceptance | Historical baseline only — later expanded materially, including the boids host lane that became `BL-102` |
| DAW automation code | Structurally complete — parameters registered, freeze logic, AsyncUpdater |
| DAW automation acceptance gates | Historical baseline only — still the main remaining formal verification gap |
| Docs/spec alignment (R4) | Historical baseline only — partially improved later, but not fully closed here |

---

## Phase C1 — Authority Migration Closeout

**Goal:** Make `PhysicsWorker` the unambiguous sole motion authority for all coordinated Tier A paths. Remove the split-brain contract so `PhysicsEngine` is no longer a parallel integration sink for any coordinated feature.

**What to do:**

1. Audit every code path inside `PluginProcessor` where `physicsEngine.*` writes are gated by `coordinatedWorkerActive`. Confirm that each guarded path has a corresponding worker-side owner.
2. For any coordinated path that still falls back to `physicsEngine` for non-motion concerns (parameter reads, state queries), either move ownership to `PhysicsWorker` or document the explicit carve-out.
3. Remove or permanently zero out `physicsEngine` gravity, interaction force, wall collision, and throw/reset for emitters in coordinated mode. These are already gated; verify they cannot accumulate stale state on the legacy side across blocks.
4. Add a debug assertion that fires if both the worker and the legacy engine produce non-zero motion for the same emitter slot in the same block — use this to smoke out any remaining dual-integration.

**Files in scope:**
- `Source/PluginProcessor.cpp` — the `coordinatedWorkerActive` gating block
- `Source/PhysicsEngine.h` — `standaloneMode` semantics
- `Source/PhysicsWorker.h` / `Source/PhysicsSharedRuntime.h` — confirm no emitter path bypasses worker for coordinated features

**Acceptance gate:**
- [x] No emitter in coordinated mode receives non-zero position/velocity contribution from `PhysicsEngine` in the same block
- [x] Debug assertion does not fire under a 30-second soak with all Tier A features simultaneously active at 64 emitters
- [x] Existing runtime probes remain green after the change (re-run full suite)

---

## Phase C2 — Host-Facing Acceptance Lanes

**Goal:** Add at least two REAPER-runnable host acceptance lanes beyond the collision transient lane that already exists. This closes the gap between "runtime probe passes" and "plugin survives real DAW host lifecycle."

**Required lanes (choose two minimum; three recommended):**

### Lane H1 — Attractor Spread in Host
Verify attractor proximity produces audible spread modulation inside REAPER. A scripted scenario places one emitter, activates one attractor, records `phys_out_spread_mod_0` for 4 bars, and checks that the recorded values exceed a minimum spread threshold. Gate: mean recorded spread > 0.1 with attractor active; < 0.01 with attractor disabled.

### Lane H2 — Boids Coordinated Motion in Host
Verify flock membership produces coordinated motion inside REAPER. Two emitters, one flock group, 4-bar recording. Gate: emitters converge (distance decreases from initial) and `phys_out_spread_mod_*` values show density-driven spread > 0.1 at closest approach.

### Lane H3 — Spring/Turbulence Spread in Host
Verify spring tether and turbulence each produce non-zero spread output in REAPER when activated in isolation. Gate: mean recorded `phys_out_spread_mod_0` > 0.05 for each feature independently.

**Script location:** `scripts/reaper-phys-<lane>-gate-mac.sh` (follow the same pattern as `reaper-phys-collision-transient-gate-mac.sh`)

**Acceptance gate:**
- [x] Lane H1 PASS in REAPER
- [x] One of Lane H2 or H3 PASS in REAPER
- [x] No plugin crash or parameter registration failure across any lane's host lifecycle (load → play → stop → unload)

---

## Phase C3 — DAW Automation Verification

**Goal:** Formally verify all 12 acceptance gates from `physics-daw-automation-spec.md`.

**Gates to verify (in order):**

| Gate | Verification method |
|---|---|
| `phys_out_spread_mod_N` and `phys_out_gain_mod_N` registered for all 8 slots | pluginval parameter scan — check all 16 appear in parameter list |
| `phys_frozen_N` registered for all 8 slots | Same pluginval scan — check all 8 appear |
| `last_frozen_state[8]` array maintained (transition detection) | Code review — already confirmed in `PluginProcessor.h:400`; log this as code-verified |
| LIVE path: physics atomic values appear in DAW automation lane during recording | REAPER: record `phys_out_spread_mod_0` with physics active; confirm non-flat curve |
| FROZEN path: DAW playback drives DSP; physics updates silently | REAPER: freeze slot, draw automation, confirm DSP follows drawn curve |
| LIVE→FROZEN snapshot: no value jump (\|pre−post\| < 0.01) | REAPER: toggle freeze mid-recording, inspect automation discontinuity |
| FROZEN→LIVE transition: DSP switches within one `processBlock` call | Code review of transition logic — already confirmed; log as code-verified |
| No `setValueNotifyingHost` in `processBlock` | `grep -n setValueNotifyingHost PluginProcessor.cpp` — already confirmed clean; log as code-verified |
| AsyncUpdater dispatches host notification from message thread | Code review — `handleAsyncUpdate` confirmed at line 1708; log as code-verified |
| `gainTransient` flows regardless of freeze state | REAPER: freeze slot, induce collision, confirm `phys_out_transient_N` still pulses |
| All 24 parameters in `parameter-spec.md` and `implementation-traceability.md` | Doc audit — see Phase C4 |
| DAW automation lane names correct in Logic and REAPER | Load plugin in REAPER, open automation lane picker, verify display names match spec |

**Acceptance gate:**
- [x] pluginval scan: all 32 DAW automation parameters present
- [x] REAPER: LIVE recording lane captures non-flat spread/gain curves
- [ ] REAPER: FROZEN lane follows drawn automation
- [ ] REAPER: `gainTransient` pulses when frozen
- [x] REAPER: automation lane display names match `physics-daw-automation-spec.md §UI Surface`
- [x] No `setValueNotifyingHost` in `processBlock` — confirmed clean
- [x] Code-reviewed gates logged as verified

---

## Phase C4 — Documentation Truth Pass

**Goal:** Align all spec, traceability, and parameter-spec surfaces with the actual implementation state. Exit R4.

**Tasks:**

1. **`parameter-spec.md`** — Add all 32 DAW automation parameters (`phys_out_spread_mod_N`, `phys_out_gain_mod_N`, `phys_out_transient_N`, `phys_frozen_N` for N = 0..7). These are absent from parameter-spec.md per the spec's own acceptance gate.

2. **`Documentation/implementation-traceability.md`** — Add entries for all Tier A parameters that were registered in P1–P6 but not yet traced: spring, turbulence, angular, boids, collision, attractor, boundary mode families. Add the 32 DAW automation parameters. Add the 15 runtime probes as evidence entries.

3. **`physics-simulation-spec.md`** — Update the `Validation Status` section from `partially tested` to reflect the current runtime probe evidence. List which Tier A acceptance gates are now closed (spring frequency, angular aim, collision determinism, turbulence bound) via the P8 + runtime probe suite.

4. **`physics-simulation-impl-plan.md`** — Tag every Tier A control in the proof matrix as one of `runtime-proven`, `host-proven`, or `partially proven` based on this closeout. Add the C2 host lane results as new evidence rows. Mark R1–R4 as closed after their respective phases pass.

5. **`physics-daw-automation-spec.md`** — Update `Validation Status` from `Not tested — spec only` to `tested` or `partially tested` once C3 gates close.

6. **`status.json`** — Add a `physics_closeout` block under `notes` recording the closeout date, which proof tiers are complete, and any deferred items.

**Acceptance gate:**
- [x] `parameter-spec.md` contains all 32 DAW automation parameters
- [x] `implementation-traceability.md` traces all Tier A control families and all 32 DAW automation parameters
- [x] `physics-simulation-spec.md` validation status updated with accurate gate results
- [x] `physics-simulation-impl-plan.md` proof matrix fully tagged; R1–R4 marked closed
- [x] `physics-daw-automation-spec.md` validation status reflects C3 results
- [x] `status.json` physics closeout entry present

---

## Execution Order

```
C1 → C2 → C3 → C4
```

C1 must complete first: the runtime probes in C2/C3 depend on a clean authority boundary. C2 and C3 can run in parallel once C1 passes. C4 is last — it codifies evidence produced by C1–C3.

---

## Deferred Items (Explicit Non-Scope)

The following are out of scope for this closeout. Do not implement until Tier A is promoted to complete.

- Tier B: Flow Fields, Environmental Presets, Material Properties, Shockwave
- Second host DAW (Logic) for DAW automation gates — REAPER is sufficient for this closeout; Logic can be a follow-on
- AUv3-specific host acceptance lanes
- Perceptual listening study for physics-driven spatial spread/gain changes

---

## Validation Status

`partially tested — C1/C2/C4 complete; C3 mostly verified (FROZEN drawn-automation and gainTransient-when-frozen are code-reviewed but not headless-tested)` — historical umbrella plan only. Do not use this document alone to infer current backlog state.

## Planning Amendment (2026-03-19)

The original `Phase C2 / Lane H2` boids host lane was scoped as part of the general physics closeout. Current evidence shows that was too broad. The controlled processor/debug lanes are green, while the REAPER duplicate-track host lane remains specifically blocked. Treat that as a **proposed net-new backlog intake** rather than burying it inside the umbrella closeout plan.

Current routing recommendation:
- keep `C2` as the umbrella host-proof goal
- continue `H1` and transient-style host lanes under the main closeout
- split the boids-specific REAPER multi-instance blocker into the draft intake at `Documentation/plans/2026-03-19-physics-host-visible-coordinated-lanes-intake-draft.md`

Why:
- it has a distinct replay-cost profile
- it likely needs temporary diagnostics and possibly screenshot/robotic evidence
- no currently open canonical BL cleanly owns this boundary

## 2026-03-20 Reconciliation

The 2026-03-19 amendment is now historical:
- the boids-specific host blocker was promoted into canonical backlog item `BL-102`
- `BL-102` is now `Done`
- this document should no longer be used to argue that the boids host lane remains open

Remaining honest takeaway:
- keep this file as historical planning context
- the former DAW-automation closeout thread now lives in done-state under `Documentation/backlog/done/bl-106-physics-daw-automation-verification-and-closeout.md`
- any new host-breadth, stability-margin, or follow-on automation work should be opened as a new backlog lane instead of being inferred from this umbrella plan
