Title: BL-045 Head Tracking Fidelity v1.1 — Architecture Spec
Document Type: Annex Spec
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-18

# BL-045 Head Tracking Fidelity v1.1 — Architecture Spec

## Status

Approved. This is the active execution contract for BL-045.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-045-head-tracking-fidelity-v11-spec-2026-02-26-legacy.md`

Depends on:
- BL-017
- BL-034

## Goal

Ship reliable head-tracking across companion, bridge, renderer, and QA.
The plan fixes packet size drift, adds motion-rate metadata, smooths pose updates, and gives operators a simple re-center control with drift readback.

## Scope

- Slice A: companion packet v2 plus bridge decode compatibility.
- Slice B: audio-thread-safe interpolation, bounded prediction, sensor-switch smoothing.
- Slice C: `Set Forward` UX, drift telemetry, and QA lane.

## Core Contracts

| Contract | Rule |
|---|---|
| Packet v1 | Accept `version = 1` at `36` bytes and map new fields to defaults. |
| Packet v2 | Encode `version = 2` at `52` bytes with angular velocity and sensor location flags. |
| Snapshot layout | `HeadTrackingPoseSnapshot` and `PoseSnapshot` both expand to `48` bytes. |
| Runtime safety | Interpolation stays allocation-free and branch-light on the audio thread. |
| Re-center | `yawReferenceDeg` is session-scoped and must not persist to state XML. |

## Implementation Order

### Slice A

- Extend `MotionSample`, `PosePacket`, and `TrackerApp.swift`.
- Fix bridge size checks and version dispatch in `HeadTrackingBridge.h`.
- Mirror the new layout in `SpatialRenderer.h`.

### Slice B

- Add `Source/HeadPoseInterpolator.h`.
- Wire ingest and interpolated pose reads into `PluginProcessor.cpp`.
- Cap prediction to `50 ms` and clamp quaternion math for stability.

### Slice C

- Add `setForwardYaw` handling in the web message path.
- Publish drift telemetry every `500 ms`.
- Add the compact `Set Forward` plus drift strip in the renderer panel.

## Validation And Evidence

- `swift build -c release` in `companion/` passes.
- `cmake --build build_local --target LocusQ_Standalone locusq_qa -j8` passes.
- `packet_v2_compat = PASS`.
- `headtracking_latency.tsv` shows mean jitter below `1.5 ms`.
- `recenter_drift_metrics.tsv` shows all BL045-C checks PASS.

## Risks

- `sensorLocation` is unavailable on older OS versions; guard with `#available`.
- Snapshot size drift can break static asserts; update both C++ buffers together.
- High angular velocity can destabilize prediction; clamp the horizon and angle.
- Re-center commands must stay off the audio thread.

## Visual Aid Index

| Artifact | Use |
|---|---|
| Legacy archive copy | Full packet tables, math derivations, and file map. |
| QA lane script | Repeatable BL-045 validation. |
| Drift telemetry | Operator-facing re-center confidence signal. |

## Archive Note

The verbose original plan is preserved in the archive copy above.
Use this active file for execution decisions and the archive file for legacy detail.
