Title: ADR-0012 Renderer Domain Exclusivity and Matrix Gating
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# ADR-0012: Renderer Domain Exclusivity and Matrix Gating

## Status
Accepted

## Context

LocusQ now spans multiple spatial-output targets:
1. internal binaural headphone rendering,
2. multichannel speaker/AVR rendering,
3. external spatial pipelines.

Without an explicit exclusivity contract, operators can configure ambiguous combinations that risk double-spatialization, hidden fallback, or misleading diagnostics.

## Decision

Adopt a renderer-domain exclusivity contract and matrix-gated legality model:

1. Renderer domain is exclusive and explicit:
   - `InternalBinaural`
   - `Multichannel`
   - `ExternalSpatial`
2. Invalid domain/layout combinations are blocked, not silently auto-corrected.
3. Device profile selection (`generic`, `airpods_pro_2`, `sony_wh1000xm5`, `custom_sofa`) is non-authoritative; it does not auto-switch renderer domain.
4. Head tracking for plugin runtime applies to `InternalBinaural` only and uses bridge telemetry contracts (`BL-017`) with stale-timeout safeguards.
5. Snapshot diagnostics must publish requested/active/stage authority and matrix-rule state so UI and automated lanes can assert legality deterministically.

## Rationale

1. Prevents double-spatialization regressions when moving between headphone and speaker workflows.
2. Makes cross-surface behavior deterministic across CALIBRATE and RENDERER views.
3. Aligns runtime behavior with existing realtime-safe contracts and bridge architecture boundaries.
4. Improves QA traceability by converting implicit fallback behavior into explicit matrix rules.

## Consequences

### Positive

1. Clear operator authority model for AirPods/Sony headphone monitoring versus AVR speaker routing.
2. Deterministic validation lane design for allowed/blocked combinations.
3. Reduced debugging overhead from hidden fallback paths.

### Costs

1. Additional UI contract work for explicit matrix-rule surfacing.
2. Additional self-test lanes to enforce matrix legality.

## Guardrails

1. No heap allocation, locks, or blocking I/O are introduced in `processBlock()`.
2. Matrix legality decisions are made from deterministic runtime state and published as diagnostics.
3. Any matrix-rule changes require synchronized updates to:
   - `Documentation/scene-state-contract.md`
   - `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md`
   - validation lane documentation/evidence.

## Related

- `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md`
- `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md`
- `Documentation/scene-state-contract.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

