---
name: auv3-plugin-lifecycle
description: Plan and execute AUv3 plugin support for LocusQ end-to-end (app-extension architecture, JUCE integration boundaries, implementation, QA, CI, and shipping evidence) while preserving VST3/AU/CLAP parity.
---

Title: AUv3 Plugin Lifecycle Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# AUv3 Plugin Lifecycle

Use this skill when work includes AUv3 format enablement, extension-safe runtime behavior, or AUv3 host validation.

## Scope
- AUv3 target architecture and packaging boundaries (main app + extension).
- JUCE/AUv3 integration choices and capability constraints relative to AU/VST3/CLAP.
- Extension-safe contracts for state, assets, and sandboxed file access.
- Host and regression validation lanes specific to AUv3 behavior.

## Workflow
1. Establish format boundary and acceptance criteria first.
   - Confirm AUv3 scope for the current slice and keep non-AUv3 formats green.
2. Lock extension-safe architecture.
   - Keep DSP deterministic and format-agnostic.
   - Keep app-only services outside extension audio path.
3. Implement build and target wiring.
   - Add AUv3 target/config with explicit feature gates.
   - Preserve AU/VST3/CLAP outputs and packaging.
4. Validate runtime constraints.
   - Check state restore, automation, channel layout behavior, and extension lifecycle transitions.
5. Run deterministic QA and host checks.
   - Record explicit pass/fail evidence for AUv3 plus non-AUv3 regression lanes.
6. Complete ship readiness and routing updates.
   - Update routing docs and release notes when AUv3 capability claims change.

## Realtime and Extension Rules
- No allocation, locking, or blocking I/O in `processBlock()`.
- No app-UI dependencies in extension DSP path.
- Treat extension lifecycle events as capability changes, never host-name branches.
- Keep fallback behavior deterministic when extension services are unavailable.

## Cross-Skill Routing
- Pair with `spatial-audio-engineering` for spatial renderer/layout contracts.
- Pair with `juce-webview-runtime` for host/runtime UI bridge behavior.
- Pair with `clap-plugin-lifecycle` when maintaining cross-format parity decisions.
- Pair with `skill_testing` for replay-tier and evidence packet execution.

## References
- `references/platform-boundaries.md`
- `references/validation-and-ship.md`
- `references/prompt-examples.md`

## Deliverables
- File-level change list with AUv3 acceptance mapping.
- Explicit validation status: `tested`, `partially tested`, or `not tested`.
- Highest-risk unresolved gap if any lane is skipped.
