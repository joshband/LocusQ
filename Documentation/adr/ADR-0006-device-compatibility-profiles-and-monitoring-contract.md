Title: ADR-0006 Device Compatibility Profiles and Monitoring Contract
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# ADR-0006: Device Compatibility Profiles and Monitoring Contract

## Status
Accepted

## Context

LocusQ v1 was designed around quad speaker workflows, but current project goals now require reliable operation on:

1. laptop speakers,
2. built-in or external mic calibration inputs,
3. headphones.

Implementation already supports mono/stereo/quad output layouts in processor/runtime, but the product contract and release gates did not explicitly require portable device-profile validation.

## Decision

Adopt an explicit device-profile contract for v1 closeout and near-term phases:

1. **Quad Studio Profile (reference):**
   - quad host output layout (`quadraphonic` or `discrete(4)`)
   - canonical speaker-map behavior remains normative.
2. **Laptop Speaker Profile (portable):**
   - stereo host output layout with deterministic downmix from canonical scene/render state.
3. **Headphone Profile (portable):**
   - stereo host output layout suitable for headphone monitoring.
   - advanced personalized binaural/HRTF remains post-v1.
4. **Mic Input Profiles:**
   - calibration is valid with built-in or external microphones through explicit channel selection (`cal_mic_channel`).

Release/closeout gating must include manual checks covering quad (when available), laptop stereo playback, and headphone playback.

## Rationale

- Aligns product behavior with requested real-world usage without re-architecting DSP core contracts.
- Preserves one canonical scene/automation model and avoids profile-specific parameter forks.
- Converts implicit behavior into explicit testable acceptance gates.

## Consequences

### Positive
- Clear expectation for portable monitoring behavior before release.
- Reduced ambiguity in documentation and acceptance criteria.
- Better continuity between quad-first design and everyday laptop workflows.

### Costs
- Additional manual acceptance burden until automation coverage expands.
- Some deferred parameters/features need explicit no-op/deferred labeling until implemented.

## Guardrails

1. Output-layout switching must not alter parameter IDs or preset schema semantics.
2. Stereo/headphone paths must remain deterministic and finite under existing QA scenarios.
3. Any non-functional user-exposed control must be documented as deferred/no-op until implemented.

## Related

- `Documentation/invariants.md`
- `.ideas/creative-brief.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
