Title: LocusQ Invariants
Document Type: Invariant Spec
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-25

# LocusQ Invariants

## Purpose
Define non-negotiable constraints that code and docs must satisfy across implementation phases.

## Source References
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`

## Audio Thread Invariants
- No heap allocation, locks, or blocking I/O inside `processBlock()`.
- Parameter reads must be real-time safe (`getRawParameterValue` / atomic usage).
- Rendering chain must remain deterministic for a given input and parameter state.

## Scene Graph Invariants
- Inter-instance state exchange must remain lock-free.
- Renderer must tolerate inactive/missing emitter slots without faults.
- Physics and spatial state handoff must avoid tearing/glitches.
- UI scene-snapshot transport must be sequence-safe (monotonic acceptance) and degrade safely when snapshots become stale.
- Snapshot payloads must publish finite per-emitter direction/energy fields and room/listener/speaker telemetry required by the production viewport.
- Viewport overlays (selection styling, trails, vectors, RMS rings) are visual-only layers and must never mutate DSP/scene canonical state.

## DSP Chain Invariants (Renderer)
- Order: emitter preprocessing -> panning/spread/directivity/distance -> room chain -> speaker compensation -> master/output.
- Stereo and mono fallback behavior must remain defined and stable.
- Quality-tier switching must preserve tonal intent while changing depth/cost only.

## Device Compatibility Invariants
- Internal spatial scene intent must remain canonical across output layouts; mono/stereo/quad are output mappings, not separate scene models.
- Laptop speaker and headphone operation must not require alternate parameter schemas or separate preset formats.
- Calibration input routing must remain explicit and deterministic for both built-in and external microphone paths.
- Any parameter exposed as user-visible must either affect runtime behavior or be explicitly documented as deferred/no-op.
- Apple PHASE must not be integrated into plugin `processBlock()` runtime; any PHASE experimentation is companion/standalone-only and outside the plugin audio thread path.

## State/Traceability Invariants
- Parameter IDs are stable and spec-aligned.
- Any new parameter must be traceable in `Documentation/implementation-traceability.md`.
- Architectural deviations require a recorded ADR in `Documentation/adr/`.
- New UI self-test assertions that gate shipped behavior must be logged in `TestEvidence/validation-trend.md` and reflected in `status.json`.
