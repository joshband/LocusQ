Title: ADR-0008 Viewport Scope v1 vs Post-v1
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-03-01

# ADR-0008: Viewport Scope v1 vs Post-v1

## Status
Accepted

## Context

Section 7 of `.ideas/architecture.md` specifies a Three.js viewport contract including:
room wireframe + grid, speaker cones/positions, draggable emitter objects, motion
trails, velocity vectors, and orbit-style interaction.

Stage 16-D compared that spec against production UI code in
`Source/ui/public/index.html` and `Source/ui/public/js/index.js`.

The production UI now includes foundational viewport behavior (room/grid scene,
speaker markers, draggable emitters, and orbit/navigation controls) plus telemetry
overlays for trails and velocity vectors.

## Decision

For v1, keep the current control panel + foundational viewport as the required scope,
with telemetry overlays (trails and velocity vectors) treated as implemented diagnostics
behind renderer visualization controls.

Feature classification:

| Feature | Implementation status (2026-03-01) | Scope |
| --- | --- | --- |
| Room wireframe + grid | Present | v1-required |
| Speaker positions/cones | Present (marker meshes, not literal cone geometry) | v1-required |
| Draggable emitters | Present | v1-required |
| Orbit/drag viewport controls | Present | v1-required |
| Motion trails | Present (toggle + runtime trail history) | v1-implemented |
| Velocity vectors | Present (toggle + per-emitter velocity arrow) | v1-implemented |

## Rationale

1. v1 scope now matches production behavior: foundational scene controls plus telemetry
   overlays are both present.
2. Telemetry controls are optional and bounded, so operators can disable overlays without
   affecting canonical DSP/control behavior.
3. Updating this ADR removes stale guidance that would otherwise misclassify implemented
   functionality as deferred work.

## Consequences

### Positive

1. Keeps release scope focused on deterministic DSP/runtime behavior and control integrity.
2. Converts ambiguous viewport expectations into an explicit accepted contract.
3. Prevents late-cycle UI polish work from destabilizing Stage 16/17 closeout.

### Costs

1. Overlay rendering adds incremental scene-visualization cost and must stay performance-safe.
2. Future telemetry enhancements should remain additive and preserve existing control IDs and toggles.

## Guardrails

1. Telemetry overlays must preserve existing parameter IDs, relay contracts, and deterministic
   control behavior.
2. Any regression that removes trails/vectors from production behavior requires either a fix
   or an ADR update in the same change set.
3. Documentation and traceability records must remain synchronized with this scope decision.

## Related

- `.ideas/architecture.md`
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-2026-02-20.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/adr/ADR-0007-emitter-directivity-velocity-ui-exposure.md`
