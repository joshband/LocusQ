Title: ADR-0019 Custom SOFA Profile Readiness and Fallback Contract
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# ADR-0019: Custom SOFA Profile Readiness and Fallback Contract

## Status
Accepted

## Context

LocusQ exposes `custom_sofa` in the headphone profile catalog and diagnostics, while
runtime integration includes partial/deferred SOFA wiring hooks in the render path.
Without an explicit readiness contract, operators can interpret profile selection as
guaranteed custom-HRTF activation when runtime prerequisites are missing.

## Decision

Adopt a capability-gated readiness and fallback contract for `custom_sofa`:

1. `custom_sofa` remains a valid requested profile ID in public contracts.
2. Active use of `custom_sofa` requires all of:
   - compatible monitoring/render mode,
   - valid SOFA reference token/path semantics,
   - runtime HRTF integration readiness for the current build/host context.
3. If any prerequisite is missing, runtime must deterministically downgrade to safe
   fallback target (default: `generic`) with explicit diagnostics/fallback reason.
4. UI and QA lanes must treat requested profile and active profile as separate
   authorities; requested `custom_sofa` does not imply active `custom_sofa`.
5. Promotion claims for custom-SOFA readiness must include deterministic evidence from
   dedicated profile-governance and fallback-behavior lanes.

## Rationale

1. Preserves truthful operator diagnostics during staged rollout.
2. Prevents silent mismatch between requested profile and active renderer behavior.
3. Enables incremental implementation without contract ambiguity.

## Consequences

### Positive

1. Clear distinction between profile request semantics and activation readiness.
2. Better reliability of profile governance telemetry and fallback reporting.
3. Cleaner future promotion path from placeholder/deferred wiring to full readiness.

### Costs

1. Additional governance/evidence work for readiness promotion.
2. More explicit QA matrix rows for fallback taxonomy coverage.

## Guardrails

1. Fallback behavior must be deterministic and finite, with no crash or non-finite audio output.
2. Fallback reason/target diagnostics must publish in every snapshot that includes
   profile governance fields.
3. Any profile-catalog or fallback-taxonomy change must update:
   - `Documentation/scene-state-contract.md`
   - `Documentation/spatial-audio-profiles-usage.md`
   - relevant BL-009/BL-033/BL-034 evidence contracts.

## Related

- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/adr/ADR-0012-renderer-domain-exclusivity-and-matrix-gating.md`
- `Documentation/scene-state-contract.md`
- `Source/PluginProcessor.cpp`
- `Source/SpatialRenderer.h`
