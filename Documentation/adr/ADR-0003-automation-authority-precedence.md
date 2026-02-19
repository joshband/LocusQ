Title: ADR-0003 Automation Authority Precedence
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# ADR-0003: Automation Authority Precedence

## Status
Accepted

## Context

Research guidance emphasizes DAW automation authority and deterministic behavior. Current design includes DAW automation, internal timeline playback, and physics-driven motion. Without explicit precedence, behavior and recall can diverge across hosts and sessions.

## Decision

Adopt this precedence contract for spatial parameters:

1. **DAW/APVTS parameter state is base authority** for current block.
2. **Internal timeline** (`anim_enable && anim_mode == Internal`) evaluates deterministic time and provides the **rest pose** for animated tracks.
3. **Physics applies additive offset** on top of the current rest pose.
4. **Manual UI edits** are parameter writes into APVTS and are treated as base-state changes; host automation remains authoritative where active.

This precedence applies at block evaluation boundaries and must be deterministic for identical input/state.

## Rationale

- Aligns with DAW recall expectations and host automation semantics.
- Preserves existing Phase 2.6 timeline + physics design intent.
- Maintains deterministic layering for reproducible sessions.

## Consequences

### Positive
- Clear conflict resolution among automation, timeline, and physics.
- Improves portability and predictable session reload behavior.
- Reduces ambiguity in QA acceptance criteria.

### Costs
- Requires explicit documentation and implementation traceability.
- Future alternate blend modes require additional ADRs.

## Guardrails

1. Time source and interpolation mode must be deterministic for internal timeline evaluation.
2. Physics reset and pause semantics must preserve stable rest-pose behavior.
3. Any precedence changes require ADR update before phase closeout.

## Related

- `Documentation/scene-state-contract.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`
