---
name: clap-plugin-lifecycle
description: Plan and execute CLAP plugin support for LocusQ end-to-end (architecture, JUCE integration, implementation, QA, CI, and ship readiness). Use when working BL-011 tasks, adding `clap-juce-extensions`, defining CLAP adapter/runtime contracts, validating CLAP host compatibility, or preparing CLAP distribution and release evidence.
---

Title: CLAP Plugin Lifecycle Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

# CLAP Plugin Lifecycle

Use this skill for BL-011 and any CLAP platform-expansion work in LocusQ.

## Scope
- Add CLAP output support via `clap-juce-extensions` while preserving VST3/AU behavior.
- Keep CLAP logic inside an adapter boundary; keep core spatial DSP format-agnostic.
- Enforce lock-free and deterministic rules from `Documentation/plans/LocusQClapContract.h`.
- Add host-validation and CI lanes for CLAP artifacts and report explicit evidence.

## Workflow
1. Establish baseline and constraints.
   - Confirm BL-011 scope in `Documentation/backlog-post-v1-agentic-sprints.md`.
   - Confirm current plugin formats and build graph in `CMakeLists.txt`.
   - Treat CLAP capability negotiation as runtime data, never host-name branching.
2. Integrate build-system support.
   - Add `clap-juce-extensions` wiring with explicit feature gates.
   - Preserve existing VST3/AU/Standalone targets and packaging behavior.
   - Discover CLAP target names with `cmake --build <build-dir> --target help` before hard-coding commands.
3. Implement adapter and contract path.
   - Translate CLAP events into internal voice/modulation events at adapter boundary.
   - Keep core renderer and engine free of CLAP headers/host structs.
   - Use fixed-capacity structures and lock-free SPSC telemetry for DSP->UI exchange.
4. Add deterministic validation lanes.
   - Prefer `clap-validator` and `clap-info` when available.
   - Keep existing `pluginval` and harness scenarios green to guard regressions.
   - Record explicit pass/fail artifacts in `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md`.
5. Complete docs and ship readiness.
   - Update backlog status for BL-011 only when acceptance criteria are met.
   - Update routing docs when CLAP specialist behavior changes (`AGENTS.md`, `SKILLS.md`, `Documentation/skill-selection-matrix.md`).
   - Include release note coverage for new CLAP format output and validation lanes.

## Realtime + Determinism Rules
- Never allocate, lock, or perform blocking I/O in `processBlock()`.
- Make runtime capability negotiation immutable per session.
- Keep voice-slot assignment deterministic under overflow/voice-steal conditions.
- Apply sample-accurate modulation in frame order with deterministic overflow policy.
- Keep UI optional: DSP correctness must not depend on UI availability.

## Reference Map
- `references/sources.md`: canonical local and upstream CLAP sources.
- `references/bl011-playbook.md`: execution playbook and file-level change map.
- `references/validation-and-ship.md`: CLAP validation matrix and closeout checklist.

## Deliverables
- List changed files with rationale and BL-011 acceptance mapping.
- Report validation as `tested`, `partially tested`, or `not tested`.
- If any lane is skipped, state why and identify the highest-risk unresolved gap.
