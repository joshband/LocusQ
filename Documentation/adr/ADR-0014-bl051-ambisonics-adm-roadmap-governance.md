Title: ADR-0014 BL-051 Ambisonics + ADM Roadmap Governance
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# ADR-0014: BL-051 Ambisonics + ADM Roadmap Governance

## Status

Accepted (planning governance baseline for BL-051).

## Context

BL-051 defines a roadmap for:
- Ambisonics intermediate-bus adoption.
- ADM/IAMF interoperability and export-readiness sequencing.

Roadmap work must progress without destabilizing active production execution lanes. A decision authority is required to govern migration phases, risk handling, and rollout gating before implementation decomposition.

## Decision

1. Ambisonics intermediate bus is adopted as the canonical internal roadmap direction for advanced spatial interchange in BL-051 scope.
2. ADM/IAMF capabilities are introduced through phase-gated roadmap slices with deterministic evidence contracts.
3. Phase advancement requires explicit pass/fail criteria, replayable artifacts, and rollback triggers.
4. No production-lane default behavior changes are permitted under BL-051 until prototype/parity contracts are validated and owner intake criteria are met.

## Consequences

Positive:
- Unified roadmap direction across spatial rendering and delivery interoperability work.
- Lower integration risk through deterministic phase gates and explicit rollback criteria.
- Clear ownership boundaries between planning governance and execution lanes.

Tradeoffs:
- Additional upfront governance/documentation work before implementation starts.
- Requires strict artifact discipline for each phase transition.

## Rollback and Guardrails

1. Any deterministic gate failure in a migration phase blocks advancement and triggers rollback to the previous accepted phase.
2. Compatibility regressions against current stereo/surround baseline are hard-stop blockers.
3. Phase contracts must remain docs-first until owner intake authorizes execution-lane work.

## Alternatives Considered

1. Direct implementation without phase-gated roadmap governance.
- Rejected due to elevated risk of cross-lane regressions and unclear rollback posture.

2. Keep current delivery model only, defer ambisonics/ADM roadmap indefinitely.
- Rejected due to strategic capability gap and unresolved interoperability planning debt.
