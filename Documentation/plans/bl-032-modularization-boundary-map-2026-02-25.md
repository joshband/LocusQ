Title: BL-032 Modularization Boundary Map
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-03-18

# BL-032 Modularization Boundary Map

## Status

Approved. This is the active boundary-map contract for BL-032 Slice A.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-032-modularization-boundary-map-2026-02-25-legacy.md`

## Goal

Define deterministic module boundaries so processor and editor decomposition can proceed without file-collision risk.

## Scope

- Freeze the five-module target set.
- Define ownership, public interfaces, and forbidden dependencies.
- Keep slice A documentation-only.
- Preserve the no-overlap rule for implementation slices B and C.

## Core Contracts

| Area | Contract |
|---|---|
| Module set | `shared_contracts`, `processor_core`, `processor_bridge`, `editor_shell`, `editor_webview`. |
| Dependency direction | `shared_contracts` feeds the other modules. Reverse edges are forbidden. |
| Ownership | Each module declares current and planned files plus public interfaces. |
| Migration order | `shared_contracts -> processor_core -> processor_bridge -> editor_shell -> editor_webview`. |
| Slice safety | B and C must not touch each other’s exclusive files. |

## Delivery Order

### A

- Publish the boundary map.
- Freeze acceptance IDs.
- Keep the work planning-only.

### B

- Extract `shared_contracts`.
- Move runtime orchestration to `processor_core`.
- Move bridge adapters to `processor_bridge`.

### C

- Move editor relay and polling to `editor_shell`.
- Move WebView bootstrap and dispatch to `editor_webview`.
- Keep `PluginProcessor.cpp` and `PluginEditor.cpp` as thin facades.

## Validation Plan

- `BL032-A-001..006` remain cross-referenced in runbook, plan, and traceability docs.
- Slice A remains documentation-only.
- B and C preserve their exclusive file sets.
- Dependency rules remain one-way.

## Risks

- A reverse dependency can sneak in during the split.
- B/C overlap can break parallel implementation.
- `PluginProcessor` and `PluginEditor` can grow back into monoliths if facade boundaries are not kept thin.

## Visual Aid Index

| Artifact | Use |
|---|---|
| Boundary map table | Module ownership and dependency clarity. |
| Slice ownership table | Parallel-safety contract for B/C. |
| Traceability matrix | Acceptance-to-evidence linkage. |

## Archive Note

The original long-form plan is preserved in the archive copy above.
Use this active file for implementation sequencing and the archive file for the complete boundary map detail.
