Title: Tracker Issue Draft - BL-070 Coherent Audio Snapshot and Telemetry Seqlock Contract
Document Type: Tracker Issue Draft
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-070 Tracker Issue Draft

## Proposed Title

BL-070: coherent audio snapshot and telemetry seqlock contract

## Summary

Harden snapshot publication/consumption so audio pointer/sample-count reads and telemetry handoff are sequence-consistent and race-free.

## Evidence

- `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md` (Findings #2 and #3)
- `Source/SceneGraph.h:135`
- `Source/SceneGraph.h:141`
- `Source/SpatialRenderer.h:1418`
- `Source/SpatialRenderer.h:1419`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1082`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1083`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1128`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1607`

## Acceptance Checklist

- [ ] Snapshot API returns coherent tuple from one publication epoch.
- [ ] Telemetry publication/read path is race-free under concurrent polling.
- [ ] Stress lane demonstrates deterministic bridge payloads.
- [ ] Concurrency report artifacts are attached to promotion packet.
