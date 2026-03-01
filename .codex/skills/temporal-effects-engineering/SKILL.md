---
name: temporal-effects-engineering
description: Design, implement, and validate temporal DSP effects for LocusQ (delay/echo/feedback networks/looper/frippertronics-style layering) with realtime safety, deterministic behavior, and plugin-host automation fidelity.
---

Title: Temporal Effects Engineering Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Temporal Effects Engineering

Use this skill for time-domain effects such as delay, echo, loopers, feedback matrices, and frippertronics-style evolving layers.

## Scope
- Delay/echo architectures (single, multi-tap, ping-pong, networked feedback).
- Looper and layered-repeat behavior with deterministic transport semantics.
- Frippertronics-style long feedback/evolution design with guardrails.
- Host automation, parameter smoothing, and recall-safe state contracts.

## Workflow
1. Lock temporal contract first.
   - Define buffer ownership, max delay/loop length, and transport/tempo behavior.
2. Design feedback safety model.
   - Clamp gain, define saturation/protection, and set runaway prevention policy.
3. Implement realtime-safe buffers and transitions.
   - Preallocate memory and define zipper-free parameter transitions.
4. Bind host/state semantics.
   - Ensure deterministic automation behavior and session recall.
5. Validate with stress and soak lanes.
   - Verify latency, drift, runaway, click/zipper risk, and CPU bounds.

## Realtime Rules
- No allocation, locks, or blocking I/O in `processBlock()`.
- All loop/delay writes and reads must be bounded and finite-safe.
- Feedback paths require explicit safety ceiling and deterministic fallback.
- Transport-dependent behavior must be deterministic for identical timeline input.

## Cross-Skill Routing
- Pair with `spatial-audio-engineering` when delay/loop behaviors interact with 3D routing paths.
- Pair with `skill_testing` for harness-first evidence and replay-tier execution.
- Pair with `realtime-dimensional-visualization` when visual feedback/loop state is a first-class UI output.

## References
- `references/loop-feedback-contracts.md`
- `references/qa-lanes.md`
- `references/prompt-examples.md`

## Deliverables
- File-level change map with temporal-contract rationale.
- Validation status: `tested`, `partially tested`, or `not tested`.
- Highest-risk unresolved behavior if any validation lane is skipped.
