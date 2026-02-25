Title: BL-032 Source Modularization of PluginProcessor/PluginEditor
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-032: Source Modularization of PluginProcessor/PluginEditor

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Planning |
| Owner Track | Track F — Hardening |
| Depends On | — |
| Blocks | — |
| Annex Spec | (inline) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Extract non-RT-safe orchestration/editor bridge logic into focused helpers |
| B | Med | M | Extract processor responsibilities into domain modules and adapter boundaries |
| C | Low | S | Add structure checks, ownership docs, and merge-risk guardrails |

## Objective

Reduce maintenance and merge risk caused by oversized monolithic translation units by decomposing `Source/PluginProcessor.cpp` and `Source/PluginEditor.cpp` into coherent, testable modules with explicit ownership boundaries while preserving current behavior and realtime safety constraints.

## Scope & Non-Scope

**In scope:**
- Introduce targeted `.h/.cpp` units for cohesive responsibilities now embedded in monolithic processor/editor files
- Move UI-bridge, payload shaping, and orchestration code out of giant files while keeping behavior parity
- Add lightweight governance checks to prevent file-size regression and responsibility drift

**Out of scope:**
- Feature redesigns or UI behavior changes
- DSP algorithm changes not required for decomposition
- Broad namespace/API overhauls beyond modularization needs

## Architecture Context

- Core touchpoints currently concentrated in `Source/PluginProcessor.cpp` and `Source/PluginEditor.cpp`
- Invariants: `Documentation/invariants.md` (RT safety and scene-state synchronization)
- Architecture: `.ideas/architecture.md` (mode separation and bridge contracts)
- Existing backlog alignment: hardening lane prioritizes reliability, mergeability, and structural safeguards

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Responsibility map + extraction plan | `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`, `Documentation/plans/` | BL-032 approved for implementation | Module map documented and agreed |
| B | Incremental extraction | `Source/**/*.h`, `Source/**/*.cpp`, build files as required | Slice A complete | Core responsibilities moved with behavior parity and passing validation lanes |
| C | Structural guardrails | `scripts/`, `Documentation/backlog/`, `TestEvidence/` | Slice B complete | CI/local checks enforce decomposition thresholds and documentation updated |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-032-build | Automated | `cmake --build build --target LocusQ_Standalone -j 8` | Exit 0 |
| BL-032-selftest | Automated | `ctest --test-dir build --output-on-failure` | Exit 0 with no regressions |
| BL-032-structure | Automated | `python3 scripts/check-source-structure.py` | Exit 0 and thresholds respected |
| BL-032-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Extraction introduces behavior drift | High | Med | Use slice-by-slice parity checks and focused regression lanes |
| RT safety regressions from refactor | High | Low | Keep processBlock-critical logic constraints explicit; audit moved code paths |
| Build graph churn from new units | Med | Med | Stage additions incrementally and verify each build configuration |

## Closeout Checklist

- [ ] Slice A-C completed with evidence
- [ ] `PluginProcessor.cpp` and `PluginEditor.cpp` reduced to bounded orchestration roles
- [ ] Validation lanes pass with recorded evidence
- [ ] `status.json` and `Documentation/backlog/index.md` synchronized
- [ ] `./scripts/validate-docs-freshness.sh` passes
