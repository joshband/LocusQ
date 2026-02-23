Title: ADR-0008 Viewport Scope v1 vs Post-v1
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# ADR-0008: Viewport Scope v1 vs Post-v1

## Status
Accepted

## Context

Section 7 of `.ideas/architecture.md` specifies a Three.js viewport contract including:
room wireframe + grid, speaker cones/positions, draggable emitter objects, motion
trails, velocity vectors, and orbit-style interaction.

Stage 16-D compared that spec against production UI code in
`Source/ui/public/index.html` and `Source/ui/public/js/index.js`.

The production UI already includes foundational viewport behavior (room/grid scene,
speaker markers, draggable emitters, and orbit/navigation controls), but advanced
telemetry visuals are incomplete.

## Decision

For v1, keep the current control panel + foundational viewport as the required scope and
defer non-critical viewport telemetry features to post-v1.

Feature classification:

| Feature | Implementation status (2026-02-20) | Scope |
| --- | --- | --- |
| Room wireframe + grid | Present | v1-required |
| Speaker positions/cones | Present (marker meshes, not literal cone geometry) | v1-required |
| Draggable emitters | Present | v1-required |
| Orbit/drag viewport controls | Present | v1-required |
| Motion trails | Absent | Post-v1 |
| Velocity vectors | Absent | Post-v1 |

## Rationale

1. v1 audio behavior and parameter control do not depend on motion-trail/vector rendering.
2. Foundational viewport interaction is already implemented and adequate for v1 spatial
   context and control verification.
3. Deferring trail/vector telemetry reduces release risk while preserving a documented
   path for enhancement work.

## Consequences

### Positive

1. Keeps release scope focused on deterministic DSP/runtime behavior and control integrity.
2. Converts ambiguous viewport expectations into an explicit accepted contract.
3. Prevents late-cycle UI polish work from destabilizing Stage 16/17 closeout.

### Costs

1. Users will not see trajectory or velocity overlays in v1.
2. Post-v1 work must add trail/vector visuals without regressing UI responsiveness.

## Guardrails

1. Any post-v1 viewport enhancement must preserve existing parameter IDs, relay contracts,
   and deterministic control behavior.
2. If motion trails or velocity vectors become release blockers, this ADR must be revised
   before GA signoff.
3. Documentation and traceability records must remain synchronized with this scope decision.

## Related

- `.ideas/architecture.md`
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-2026-02-20.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/adr/ADR-0007-emitter-directivity-velocity-ui-exposure.md`
