Title: ADR-0018 Temporal Effects Realtime Architecture Contract
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# ADR-0018: Temporal Effects Realtime Architecture Contract

## Status
Accepted

## Context

BL-050 and BL-068 establish temporal-effects expansion (delay/echo/looper/
frippertronics-style behavior) as active high-risk work. These features are sensitive
to runaway feedback, allocation pressure, and transport/recall nondeterminism.

The repository needs a clear architecture contract before deeper implementation to
avoid unsafe ad-hoc temporal DSP growth.

## Decision

Adopt a realtime-safe temporal-effects architecture contract:

1. Delay/loop buffers are preallocated, bounded, and sample-rate aware.
2. Feedback paths must enforce explicit gain ceilings plus finite-output guardrails.
3. Transport/tempo/recall behavior is deterministic for identical host state and timeline input.
4. Automation transitions into temporal parameters must be zipper-safe/click-safe.
5. Temporal effects remain compatible with existing spatial-render and FIR/headphone
   lanes; no bypass of canonical output safety contracts is permitted.

## Rationale

1. Converts a high-risk implementation area into explicit non-negotiable rules.
2. Aligns temporal expansion with existing realtime invariants and QA discipline.
3. Reduces regression probability during BL-050/BL-068 execution.

## Consequences

### Positive

1. Shared safety baseline for future delay/looper/frippertronics work.
2. Easier code review and QA gate design for temporal DSP slices.
3. Stronger protection against runaway or non-finite render behavior.

### Costs

1. Additional upfront implementation rigor and validation work.
2. Potentially slower feature iteration due to guardrail enforcement.

## Guardrails

1. Temporal DSP paths must remain lock-free, allocation-free, and non-blocking in audio callbacks.
2. Any temporal-state telemetry exported for UI must remain additive/backward-compatible.
3. Any change to temporal contract semantics requires synchronized updates to:
   - `Documentation/plans/bl-068-temporal-effects-core-spec-2026-03-01.md`
   - `Documentation/backlog/bl-068-temporal-effects-delay-echo-looper-frippertronics.md`
   - `Documentation/invariants.md` (if invariant semantics are expanded).

## Related

- `Documentation/backlog/bl-050-high-rate-delay-and-fir-hardening.md`
- `Documentation/backlog/bl-068-temporal-effects-delay-echo-looper-frippertronics.md`
- `Documentation/plans/bl-068-temporal-effects-core-spec-2026-03-01.md`
- `Documentation/invariants.md`
