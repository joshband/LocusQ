Title: LocusQ Invariants
Document Type: Invariant Spec
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

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

## DSP Chain Invariants (Renderer)
- Order: emitter preprocessing -> panning/spread/directivity/distance -> room chain -> speaker compensation -> master/output.
- Stereo and mono fallback behavior must remain defined and stable.
- Quality-tier switching must preserve tonal intent while changing depth/cost only.

## State/Traceability Invariants
- Parameter IDs are stable and spec-aligned.
- Any new parameter must be traceable in `Documentation/implementation-traceability.md`.
- Architectural deviations require a recorded ADR in `Documentation/adr/`.
