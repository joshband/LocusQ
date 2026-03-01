Title: ADR-0002 Routing Model V1
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-03-01

# ADR-0002: Routing Model V1

## Status
Accepted

## Context

Research recommendations prioritize metadata-only inter-instance state exchange for host robustness. Current implementation and acceptance flows are built around a single-process `SceneGraph` that carries emitter metadata and an ephemeral per-block copied mono audio snapshot consumed by a renderer instance in the same callback cycle.

This is the highest-impact architectural contradiction and must be explicitly resolved for planning and implementation continuity.

## Decision

For v1, adopt a **single-process shared-scene routing model** with:

1. **Emitter metadata as canonical shared state** (position, size, gain, spread, directivity, velocity, labels, flags).
2. **Ephemeral emitter audio snapshot as v1 fast path** for renderer-side spatialization in the same process block.
3. **No IPC** and no cross-process transport in v1.
4. **Fallback behavior**: when fast-path assumptions are invalid in a host/runtime configuration, renderer must fail-safe (no crash, no non-finite output) and surface degraded behavior; metadata-only/bus-oriented fallback remains a planned evolution path.

## Rationale

- Preserves current validated implementation path and avoids destabilizing mid-phase refactor.
- Keeps deterministic low-latency behavior and avoids extra routing complexity during Phase 2.6 closure.
- Explicitly records constraints so host edge-case validation can target them.

## Consequences

### Positive
- Minimal re-architecture risk while acceptance gates are still open.
- Maintains current performance characteristics and QA scenario compatibility.
- Clear contract for renderer consumption window and lifetime.

### Costs
- Host execution-order assumptions remain critical risk.
- Ephemeral snapshot lifetime and buffer bounds require strict invariants and validation discipline.

## Guardrails

1. Snapshot validity is limited to the current `processBlock` cycle.
2. Audio thread remains lock-free and allocation-free.
3. Host edge-case matrix must continuously validate no-crash/no-non-finite behavior.

## Related

- `Documentation/scene-state-contract.md`
- `Documentation/invariants.md`
- `.ideas/architecture.md`
- `.ideas/plan.md`
