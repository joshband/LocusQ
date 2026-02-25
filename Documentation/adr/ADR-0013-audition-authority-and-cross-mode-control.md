Title: ADR-0013 Audition Authority and Cross-Mode Control
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# ADR-0013: Audition Authority and Cross-Mode Control

## Status
Accepted

## Context
LocusQ users need Audition to work as a high-quality standalone showcase and diagnostic source while still supporting emitter-like, choreography-linked, and physics-reactive behavior.

There is a design tension:
1. Move Audition ownership into EMITTER mode for authoring ergonomics.
2. Keep Audition in RENDERER mode for correctness of final spatial output path.

Existing contracts:
1. ADR-0011 introduced standalone renderer audition sources.
2. ADR-0012 enforces renderer domain exclusivity and matrix gating.

## Decision
1. Audition rendering authority remains in the Renderer domain.
2. Emitter/Choreography/Physics panels may expose "Audition This" controls, but those controls are proxy writers to renderer audition state.
3. Audition behavior may bind to emitter/choreography/physics runtime state through additive binding metadata, not by moving DSP ownership out of renderer.
4. Audition must support deterministic standalone operation with no DAW signal present.
5. All new audition metadata remains additive and backward-compatible in scene-state payloads.

## Rationale
1. Renderer ownership preserves final-path correctness for spatial profile, headphone mode, room path, and output matrix behavior.
2. Proxy controls provide authoring convenience without splitting authority across modes.
3. Deterministic standalone behavior improves demos, regression testing, and user onboarding.
4. Additive metadata preserves existing sessions and UI fallback behavior.

## Consequences

### Positive
1. One DSP authority for audition signal generation and spatialization.
2. Cross-mode UX can improve without architecture fork.
3. Stronger deterministic QA lanes for standalone showcases.
4. Better alignment with existing renderer diagnostics and BL-029 contracts.

### Costs
1. Additional control-mapping logic for cross-mode proxy actions.
2. More complex selftest coverage for binding/fallback semantics.

## Guardrails
1. No allocations, locks, or blocking operations introduced on audio thread.
2. Cross-mode controls must never bypass renderer gating or profile contracts.
3. Binding target failures must degrade deterministically and report explicit status.
4. Default behavior remains conservative (`rend_audition_enable=off`).

## Validation Implications
Required validation includes:
1. Scoped selftest lanes for BL-029 audition bindings.
2. RT audit pass with current allowlist.
3. QA smoke pass with bound and unbound audition source modes.
4. Scene-state schema compatibility checks for additive fields.

## Related
1. `Documentation/adr/ADR-0011-standalone-renderer-audition-source.md`
2. `Documentation/adr/ADR-0012-renderer-domain-exclusivity-and-matrix-gating.md`
3. `Documentation/plans/bl-029-audition-platform-expansion-plan-2026-02-24.md`
4. `Documentation/scene-state-contract.md`
