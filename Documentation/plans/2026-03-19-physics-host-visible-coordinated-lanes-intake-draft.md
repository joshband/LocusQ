---
Title: Physics Host-Visible Coordinated Lane Validation (Boids Multi-Instance)
Document Type: Draft Backlog Intake
Author: APC Codex
Created Date: 2026-03-19
Last Modified Date: 2026-03-19
---

# Physics Host-Visible Coordinated Lane Validation (Boids Multi-Instance)

## Problem Statement

The boids coordinated motion host lane is blocked specifically in REAPER multi-instance/multi-track scenarios. The controlled processor-path probes (`locusq_physics_runtime_boids_probe`) are green, proving that flock-driven coordinated motion and density-driven spread work in the production `PluginProcessor` path. However, the REAPER host-facing acceptance lane remains unvalidated: the Lua gate script and supporting multi-track RPP project have not yet been executed in a real REAPER environment to prove the feature survives the actual plugin-host lifecycle.

This distinct replay-cost profile and novel host-facing interaction pattern cannot be confidently closed under the umbrella C2 host-proof goal without specific targeted work. Treating this as a proposed net-new intake clarifies the boundary and allows independent diagnostic effort if needed.

## Current Evidence

### What is proven:
- **Runtime processor-path probe:** `locusq_physics_runtime_boids_probe` PASS in the production processor path for flock-driven coordinated motion, multi-emitter flocking, and density-driven spread behavior. The runtime probe proves:
  - Emitter 0 and Emitter 1 in the same flock group converge to a bounded mean distance.
  - Spread modulation responds to flock density.
  - Motion remains finite and stable across a fixed replay window.
  - Settlement metrics (`settleMeanDistance`, `settleRangeDistance`) are bounded across 3 independent reruns.

- **Subsystem integration tests:** The boids subsystem (`Source/BoidsSystem.h/.cpp`) and shared-worker activation path have been validated at the isolated probe level (`locusq_physics_tier_a_probe` PASS 15/15).

- **Proof matrix status:** Boids control family marked as `runtime-proven` with remaining gap noted as "Add proof for the non-motion hooks such as breakup-driven gain behavior."

### What is not yet proven:
- **Host acceptance lane:** The REAPER multi-instance boids gate script (`qa/reaper/reascripts/LocusQ_PhysicsBoidsSpread_Gate.lua`) and supporting shell wrapper have been drafted but have not yet been executed in a live REAPER environment.
- **Multi-track RPP project:** A preloaded REAPER project file with 2+ LocusQ tracks configured for boids flocking does not yet exist or has not been tested with the gate script.
- **DAW automation lifecycle:** Evidence does not yet cover whether boids flocking parameters (flock-group membership, separation/alignment/cohesion weights, flock radii, max speed) survive the full DAW-host lifecycle (load, play, record, unload).
- **Inter-instance state:** No evidence yet that multiple plugin instances in REAPER correctly share the process-wide `PhysicsWorker` state for coordinated flocking.

## Scope of Work

The following activities are needed to close this intake:

1. **Verify boids Lua gate script in REAPER**
   - Load the gate script into REAPER's ReaScript environment.
   - Confirm the script correctly reads boids parameters from LocusQ plugin instances.
   - Ensure the script can trigger flock motion and capture spread/motion metrics from multiple tracks.

2. **Create or validate multi-track RPP project**
   - If not already present: create a REAPER project file (`LocusQ-Loaded-Track1-Boids.RPP` or similar) with 2 LocusQ instances on separate tracks.
   - Configure both instances with boids group membership and reasonable flocking parameters.
   - Save the project in a known location for automated gate use.

3. **Run the boids host gate script in a headless REAPER session**
   - Execute `scripts/reaper-phys-boids-spread-gate-mac.sh` (or equivalent) using headless REAPER with BlackHole audio device.
   - Capture the following metrics:
     - Baseline spread (both emitters, no flock interaction).
     - Active-flock spread (both emitters in same flock group).
     - Settlement and convergence distances across the replay window.
     - Gate pass/fail status from the Lua script.
   - Log all results to `TestEvidence/reaper_phys_boids_spread_gate_<timestamp>/`.

4. **Investigate any boids-specific host blockers**
   - If the gate fails, diagnose whether the failure is:
     - Parameter registration or wiring (boids params not reaching REAPER/plugin).
     - Process-wide worker state not shared correctly between instances.
     - Lua script error or incomplete spread/motion capture.
     - Replay stability issue (metrics vary beyond expected tolerance).
   - Possibly add temporary per-session diagnostic logging to `qa/reaper/reascripts/LocusQ_PhysicsBoidsSpread_Gate.lua` to surface shared-state details.

5. **Robotic or screenshot evidence (if diagnostics reveal subtle issues)**
   - May require screen recording or manual REAPER inspection to confirm visual/audible coordinated motion.
   - May need to exercise boids parameters in real-time (change flock group, separation weight, etc.) while gate is running to verify responsiveness.

## Acceptance Gate

This intake is closed when:

1. `scripts/reaper-phys-boids-spread-gate-mac.sh` (or equivalent host gate wrapper) runs clean with exit code 0.
2. The Lua gate script (`LocusQ_PhysicsBoidsSpread_Gate.lua`) reports all internal gates passing:
   - `gate_a`: baseline spread low with flock disabled.
   - `gate_b`: active-flock spread non-zero and higher than baseline.
   - `gate_c`: settlement metrics bounded (mean/range within expected tolerances).
   - `gate_d`: parameter consumption verified (flock params read by both instances).
3. Test evidence logged to `TestEvidence/reaper_phys_boids_spread_gate_<timestamp>/run.log` and summary JSON.
4. Proof matrix row updated from `runtime-proven` to `host-proven`.

## Why Deferred from C2

The original umbrella host-proof goal (C2 / Lane H2) assumed boids validation could be addressed with the general multi-instance host closure. However, the evidence shows a distinct boundary:

- **Controlled processor-path probes are green:** The runtime probe proves the behavior works in the production plugin path in isolation.
- **REAPER multi-instance host lane is untested:** Unlike attractor and collision transient lanes (which have host gates already passing), the boids lane has not yet been validated in a live DAW environment.
- **Replay-cost profile differs:** Boids require sustained inter-emitter coordination across the full replay window, not just a single transient event. This makes them more sensitive to timing/scheduling anomalies in the host environment.
- **No canonical backlog item owns this:** The physics closeout plan (C2) intentionally focused on the most critical host acceptance lanes. Boids is important for feature completeness but not a blocker for the main C2 goal; separating it as a dedicated intake allows proportional diagnostic effort.

## Proposed Priority

**Lab/validation lane; not a ship blocker.**

- The runtime-proven status confirms the feature works in the controlled processor path.
- DAW integration testing is valuable for shipping confidence but not required for Tier A acceptance gates to be considered closed.
- If promoted to user-facing in Tier A, the host acceptance would become a ship blocker; otherwise, it remains a quality/confidence deliverable.

## Related Artifacts

- Runtime probe: `qa/physics_runtime_boids_probe_main.cpp` (PASS)
- Proof matrix: `.ideas/physics-simulation-impl-plan.md` (Boids row, line ~351)
- Lua gate script (draft): `qa/reaper/reascripts/LocusQ_PhysicsBoidsSpread_Gate.lua`
- Shell wrapper (draft): `scripts/reaper-phys-boids-spread-gate-mac.sh` (if present; may be `reaper-phys-boids-spread-gate-mac.sh` or follow naming pattern)
- Physics closeout amendment: `.ideas/physics-end-to-end-closeout-plan.md` (section: "Original C2 / Lane H2 boids host lane...")
