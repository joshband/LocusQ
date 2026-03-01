Title: ADR-0017 AUv3 App-Extension Boundary and Lifecycle Contract
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# ADR-0017: AUv3 App-Extension Boundary and Lifecycle Contract

## Status
Accepted

## Context

BL-067 introduces AUv3 support planning with app-extension lifecycle constraints and
cross-format parity obligations. AUv3 is materially different from desktop plugin
formats because of extension boundaries, sandbox behavior, and host lifecycle variance.

A dedicated architecture decision is needed so AUv3 enablement does not leak format-
specific behavior into core DSP runtime paths.

## Decision

Adopt the following AUv3 boundary contract:

1. AUv3 is treated as a format adapter and lifecycle shell around the same canonical
   DSP/runtime core used by AU/VST3/CLAP.
2. AUv3-specific services (packaging, entitlement, extension lifecycle handling) stay
   outside DSP process paths.
3. Runtime behavior must not branch on host name; capability/state detection drives
   deterministic fallback behavior.
4. Lifecycle transitions (cold start, reload, suspend/resume, state restore) must be
   validated as explicit acceptance lanes before promotion.
5. AUv3 promotion is blocked unless parity evidence shows no new regression against
   AU/VST3/CLAP core contracts.

## Rationale

1. Protects realtime-safe DSP boundaries while adding format reach.
2. Prevents format-fragmented logic and hard-to-debug host divergence.
3. Keeps AUv3 adoption testable and evidence-driven rather than aspirational.

## Consequences

### Positive

1. Clear architecture boundary for AUv3 implementation work.
2. Predictable parity expectations across all supported formats.
3. Better release confidence for host lifecycle behavior.

### Costs

1. Additional validation lanes and evidence burden for lifecycle matrices.
2. More explicit packaging/signing process documentation overhead.

## Guardrails

1. No AUv3 integration may introduce heap allocation/locks/blocking in audio callbacks.
2. Extension boundary violations (app-only services crossing into DSP paths) are hard-stop blockers.
3. Any AUv3-specific runtime contract change must update:
   - `Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md`
   - `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md`
   - `ARCHITECTURE.md`.

## Related

- `Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md`
- `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md`
- `Documentation/invariants.md`
- `ARCHITECTURE.md`
