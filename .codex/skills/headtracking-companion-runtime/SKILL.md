---
name: headtracking-companion-runtime
description: Validate and harden LocusQ companion runtime behavior for head-tracking readiness, sync/center gating, frame mapping, axis sanity, and deterministic operator diagnostics.
---

Title: Headtracking Companion Runtime Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Headtracking Companion Runtime

Use this skill for companion runtime stability and diagnostics work tied to BL-053/BL-058 class issues.

## Scope
- Readiness state machine behavior:
  - `disabled_disconnected`
  - `active_not_ready`
  - `active_ready`
- Sync/center gating and startup baseline behavior.
- Axis/frame sanity (yaw/pitch/roll), sensor-location transitions, and forward-vector correctness.
- Companion-to-plugin observability checks (packet freshness, sequence continuity, active pose age).

## Workflow
1. Confirm readiness contract first.
   - Verify the companion starts in a safe, non-streaming state when devices are not ready.
   - Verify send gate remains closed until explicit sync/center action in ready state.
2. Validate frame contract and axis semantics.
   - Canonical frame: `+X right`, `+Y up`, `-Z ahead`.
   - Ensure principal-axis sweeps show dominant expected movement.
3. Validate pose freshness behavior.
   - Stale packets must not keep driving orientation visuals.
   - Explicit stale fallback orientation should be observable.
4. Validate relaunch and sensor transitions.
   - Confirm reconnection does not silently preserve bad startup offsets.
   - Confirm sensor-location switch is visible and deterministic.
5. Capture deterministic evidence packet.

## Mandatory Evidence Packet
- `status.tsv`
- `results.tsv`
- `axis_sweeps.md`
- `readiness_gate.md`
- `runtime_snapshot.md` (if runtime telemetry capture is part of the run)

## Cross-Skill Routing
- Pair with `threejs` for visualization/render-loop issues.
- Pair with `juce-webview-runtime` for bridge/startup hydration ordering faults.
- Pair with `spatial-audio-engineering` when plugin-side orientation math must be validated in parallel.
- Pair with `skill_troubleshooting` for unresolved regressions or inversion/drift loops.

## References
- `references/state-machine-and-gating.md`
- `references/axis-sweep-and-frame-contract.md`
- `references/evidence-packet-contract.md`

## Deliverables
- Changed files with rationale.
- Explicit statement of which readiness/axis checks passed or failed.
- Validation status: `tested`, `partially tested`, or `not tested`.
