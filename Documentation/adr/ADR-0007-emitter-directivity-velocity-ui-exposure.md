Title: ADR-0007 Emitter Directivity and Initial Velocity UI Exposure
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# ADR-0007: Emitter Directivity and Initial Velocity UI Exposure

## Status
Accepted

## Context

LocusQ already implemented five emitter-control parameters in DSP/runtime paths:

- `emit_dir_azimuth`
- `emit_dir_elevation`
- `phys_vel_x`
- `phys_vel_y`
- `phys_vel_z`

Before Stage 15, these parameters were not fully exposed in the production WebView relay/attachment/UI path. Stage 14 review classified this as a medium-severity gap because users could not directly edit these controls in the production UI without DAW automation lanes.

Stage 15 closed the implementation gap in two steps:

1. Task 15-A wired `emit_dir_azimuth` and `emit_dir_elevation` (`52faafe`).
2. Task 15-B wired `phys_vel_x`, `phys_vel_y`, and `phys_vel_z` (`cee3863`).

This decision record formalizes the architecture contract for v1 so future iterations do not regress these controls back to hidden or automation-only behavior.

## Decision

Expose all five parameters in the production plugin UI path for v1, with no deferral:

1. Maintain relay + attachment bindings in `Source/PluginEditor.h` and `Source/PluginEditor.cpp`.
2. Maintain production control visibility and interaction wiring in `Source/ui/public/index.html` and `Source/ui/public/js/index.js`.
3. Keep existing parameter IDs and ranges stable, including `phys_vel_x/y/z` at `-50..50 m/s`.
4. Treat these controls as first-class production parameters, not dev-only controls.

## Consequences

### Positive

1. Users can set directivity aim and throw velocity directly in the plugin UI.
2. The Stage 14 medium-severity UI parity gap is closed.
3. The decision aligns with `Documentation/invariants.md`, specifically that user-visible controls must affect runtime behavior (or be explicitly documented as deferred/no-op).

### Required Follow-up

1. Update `Documentation/implementation-traceability.md` to reflect production UI binding status for these five parameters (Task 15-E).
2. Add/maintain QA coverage for directivity aim impact on spatial output (targeted in Stage 16-E scenario work).
3. Keep release/checklist docs synchronized with this accepted decision.

### Cost / Risk

1. Additional UI wiring surface increases maintenance burden.
2. Regression risk shifts to UI binding drift, requiring traceability and scenario coverage discipline.

## Related

- `Documentation/invariants.md`
- `Documentation/implementation-traceability.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/full-project-review-2026-02-20.md`
