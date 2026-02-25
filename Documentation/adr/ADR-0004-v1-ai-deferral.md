Title: ADR-0004 V1 AI Deferral
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-25

# ADR-0004: V1 AI Deferral

## Status
Accepted

## Context

Research notes repeatedly prioritize a deterministic, shippable spatial core before adding AI orchestration. Current v1 acceptance still has open system-level gates (CPU/host edge cases), and documentation discipline requires stable architecture before feature-surface expansion.

## Decision

For v1 and current Phase 2.x closure:

1. **No AI orchestration features in critical path** (no automatic scene mutation, no neural acoustic modeling, no generative control execution in runtime path).
2. **AI work is explicitly post-v1** and roadmap-scoped after deterministic core acceptance.
3. Any AI-assistive features must be proposal/preview-first and never bypass deterministic scene-state contracts.

## Rationale

- Protects delivery and acceptance closure from scope creep.
- Preserves deterministic DSP behavior and reproducible validation.
- Aligns research guidance with current implementation priorities.

## Consequences

### Positive
- Clear boundary around v1 priorities.
- Lower schedule and verification risk.
- Better focus on host stability and CPU criteria.

### Costs
- Defers potentially attractive AI-facing differentiation.
- Requires explicit roadmap communication to avoid repeated re-opening.

## Related

- `.ideas/plan.md`
- `Documentation/archive/2026-02-25-research-legacy/quadraphonic-audio-spatialization-next-steps.md`
- `Documentation/invariants.md`
