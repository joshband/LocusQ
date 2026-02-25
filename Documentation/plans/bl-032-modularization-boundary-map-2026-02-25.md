Title: BL-032 Modularization Boundary Map
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-032 Modularization Boundary Map (Slice A)

## Purpose
Define deterministic module boundaries for `PluginProcessor` and `PluginEditor` decomposition so Slice B and Slice C can execute in parallel without file-collision risk.

## Authority
- Backlog runbook: `Documentation/backlog/bl-032-source-modularization.md`
- Traceability matrix: `Documentation/implementation-traceability.md`
- Invariants: `Documentation/invariants.md`
- Architecture baseline: `.ideas/architecture.md`

## Acceptance IDs (Normative)

| Acceptance ID | Contract Statement |
|---|---|
| BL032-A-001 | Target module set is fixed to `processor_core`, `processor_bridge`, `editor_shell`, `editor_webview`, `shared_contracts`. |
| BL032-A-002 | Each module declares current/planned ownership and explicit public interfaces. |
| BL032-A-003 | Forbidden dependencies are explicit and enforce one-way layering. |
| BL032-A-004 | Migration order is deterministic and tranche-gated. |
| BL032-A-005 | Slice ownership plan A/B/C has zero file overlap between active implementation slices. |
| BL032-A-006 | Acceptance IDs are cross-referenced in runbook, plan, and implementation traceability docs. |

## Target Module Boundary Map

### 1) `shared_contracts`
- Migration order: 1
- Current owned files:
  - `Source/SharedPtrAtomicContract.h`
  - contract constants/types currently embedded in `Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`
- Planned owned files:
  - `Source/shared_contracts/SceneStateContract.h`
  - `Source/shared_contracts/CalibrationContract.h`
  - `Source/shared_contracts/TopologyContract.h`
  - `Source/shared_contracts/BridgeStatusContract.h`
- Public interfaces:
  - POD contract structs/enums for scene-state and bridge metadata
  - string-key constants for JSON field publication/consumption
- Forbidden dependencies:
  - must not include `PluginProcessor.h`, `PluginEditor.h`, or UI JS/WebView resources
  - must not depend on `processor_core`, `processor_bridge`, `editor_shell`, `editor_webview`

### 2) `processor_core`
- Migration order: 2
- Current owned files:
  - runtime orchestration sections currently in `Source/PluginProcessor.cpp`
  - renderer/physics orchestration inputs from `Source/SpatialRenderer.h`, `Source/PhysicsEngine.h`, `Source/SceneGraph.h`, `Source/KeyframeTimeline.h`
- Planned owned files:
  - `Source/processor_core/ProcessorCoreRuntime.h`
  - `Source/processor_core/ProcessorCoreRuntime.cpp`
  - `Source/processor_core/ProcessorStatePublisher.h`
  - `Source/processor_core/ProcessorStatePublisher.cpp`
  - `Source/processor_core/ProcessorParameterReaders.h`
  - `Source/processor_core/ProcessorParameterReaders.cpp`
- Public interfaces:
  - deterministic process/update entrypoints used by `PluginProcessor`
  - typed runtime snapshots consumed by `processor_bridge`
- Forbidden dependencies:
  - must not include `PluginEditor.h`
  - must not include WebView bridge symbols or DOM/selftest concerns

### 3) `processor_bridge`
- Migration order: 3
- Current owned files:
  - bridge/orchestration functions currently in `Source/PluginProcessor.cpp` and declarations in `Source/PluginProcessor.h`
  - calibration/head-tracking/preset bridge helpers currently coupled in processor file
  - `Source/HeadTrackingBridge.h`, `Source/RoomProfileSerializer.h` consumption sites
- Planned owned files:
  - `Source/processor_bridge/ProcessorSceneStateBridge.h`
  - `Source/processor_bridge/ProcessorSceneStateBridge.cpp`
  - `Source/processor_bridge/ProcessorCalibrationBridge.h`
  - `Source/processor_bridge/ProcessorCalibrationBridge.cpp`
  - `Source/processor_bridge/ProcessorPresetBridge.h`
  - `Source/processor_bridge/ProcessorPresetBridge.cpp`
- Public interfaces:
  - message-thread bridge APIs consumed by `PluginEditor`
  - scene-state publishing adapter over `processor_core` snapshots
- Forbidden dependencies:
  - must not depend on `editor_shell` or `editor_webview`
  - must not perform direct DOM/resource path logic

### 4) `editor_shell`
- Migration order: 4
- Current owned files:
  - relay/attachment registration and timer polling sections currently in `Source/PluginEditor.h` and `Source/PluginEditor.cpp`
- Planned owned files:
  - `Source/editor_shell/EditorRelayRegistry.h`
  - `Source/editor_shell/EditorRelayRegistry.cpp`
  - `Source/editor_shell/EditorSnapshotPoller.h`
  - `Source/editor_shell/EditorSnapshotPoller.cpp`
  - `Source/editor_shell/EditorLayoutBridge.h`
  - `Source/editor_shell/EditorLayoutBridge.cpp`
- Public interfaces:
  - editor-to-processor relay wiring and bounded polling APIs
  - shell status model consumed by `editor_webview`
- Forbidden dependencies:
  - must not include `Source/ui/public/js/index.js` or web resource constants directly
  - must not call renderer internals (`SpatialRenderer.h`) directly

### 5) `editor_webview`
- Migration order: 5
- Current owned files:
  - WebView bootstrap/dispatch logic currently in `Source/PluginEditor.cpp`
  - UI resources in `Source/ui/public/index.html` and `Source/ui/public/js/index.js`
- Planned owned files:
  - `Source/editor_webview/WebViewBridgeBindings.h`
  - `Source/editor_webview/WebViewBridgeBindings.cpp`
  - `Source/editor_webview/WebViewBootstrap.h`
  - `Source/editor_webview/WebViewBootstrap.cpp`
  - `Source/editor_webview/WebViewSelfTestBridge.h`
  - `Source/editor_webview/WebViewSelfTestBridge.cpp`
- Public interfaces:
  - native function registration/dispatch surface for UI runtime
  - webview lifecycle hooks and selftest query adapter
- Forbidden dependencies:
  - must not include `PluginProcessor.h` directly (consume via `editor_shell` contract)
  - must not read/write DSP runtime state directly

## Dependency Rules (Normative)

Allowed dependency direction:
`shared_contracts -> (none)`
`processor_core -> shared_contracts`
`processor_bridge -> processor_core, shared_contracts`
`editor_shell -> processor_bridge, shared_contracts`
`editor_webview -> editor_shell, shared_contracts`

Forbidden reverse edges:
- `processor_core` must not depend on `processor_bridge`, `editor_shell`, `editor_webview`.
- `processor_bridge` must not depend on `editor_shell`, `editor_webview`.
- `editor_shell` must not depend on `editor_webview` internals.
- `shared_contracts` must not depend on any module.

## Slice Ownership Plan (No-Overlap)

| Slice | Primary Modules | Owned Files (exclusive for slice duration) | Explicitly Not Owned |
|---|---|---|---|
| A (this slice) | Planning/docs only | `Documentation/backlog/bl-032-source-modularization.md`, `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md`, `Documentation/implementation-traceability.md`, `TestEvidence/bl032_slice_a_boundary_map_<timestamp>/` | `Source/*`, `scripts/*`, `.github/workflows/*` |
| B | `shared_contracts`, `processor_core`, `processor_bridge` | `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/shared_contracts/*`, `Source/processor_core/*`, `Source/processor_bridge/*` | `Source/PluginEditor.cpp`, `Source/PluginEditor.h`, `Source/editor_shell/*`, `Source/editor_webview/*`, `Source/ui/public/*` |
| C | `editor_shell`, `editor_webview` | `Source/PluginEditor.cpp`, `Source/PluginEditor.h`, `Source/editor_shell/*`, `Source/editor_webview/*`, `Source/ui/public/index.html`, `Source/ui/public/js/index.js` | `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/shared_contracts/*`, `Source/processor_core/*`, `Source/processor_bridge/*` |

`BL032-A-005` fails if any B/C worker edits files from the other slice's exclusive list.

## Deterministic Migration Sequence

1. Freeze this boundary map as authority for implementation tranches.
2. Extract `shared_contracts` first to remove implicit processor/editor coupling.
3. Move processor runtime logic into `processor_core` and keep `PluginProcessor` as thin orchestrator.
4. Move scene/calibration/preset/head-tracking bridge surfaces into `processor_bridge`.
5. Move editor relay/polling/orchestration to `editor_shell`.
6. Move WebView bootstrapping + JS bridge dispatch to `editor_webview`.
7. Keep `PluginProcessor.cpp` and `PluginEditor.cpp` as facade entrypoints only.

`BL032-A-004` requires this order to be followed for merge sequencing and acceptance replay.

## Traceability Links

| Acceptance ID | Runbook Source | Plan Source | Traceability Source |
|---|---|---|---|
| BL032-A-001 | `Documentation/backlog/bl-032-source-modularization.md` | this file | `Documentation/implementation-traceability.md` (BL-032 Slice A section) |
| BL032-A-002 | `Documentation/backlog/bl-032-source-modularization.md` | this file | `Documentation/implementation-traceability.md` |
| BL032-A-003 | `Documentation/backlog/bl-032-source-modularization.md` | this file | `Documentation/implementation-traceability.md` |
| BL032-A-004 | `Documentation/backlog/bl-032-source-modularization.md` | this file | `Documentation/implementation-traceability.md` |
| BL032-A-005 | `Documentation/backlog/bl-032-source-modularization.md` | this file | `Documentation/implementation-traceability.md` |
| BL032-A-006 | `Documentation/backlog/bl-032-source-modularization.md` | this file | `Documentation/implementation-traceability.md` |

## Slice A Exit Criteria

- [x] Module boundary map published for all five target modules.
- [x] Public interfaces and forbidden dependencies documented per module.
- [x] Deterministic migration sequence and no-overlap slice ownership plan documented.
- [x] Acceptance IDs `BL032-A-001..006` cross-referenced across runbook/plan/traceability.
