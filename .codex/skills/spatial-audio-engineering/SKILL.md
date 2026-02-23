---
name: spatial-audio-engineering
description: Plan, implement, and validate spatial audio systems for LocusQ across stereo/quad/5.1/7.1/7.4.2 speaker layouts, ambisonics (FOA/HOA), binaural/HRTF headphone rendering, and interchange formats (ADM/IAMF). Use when adding spatial renderer features, integrating Steam Audio or open-source spatial DSP libraries, expanding BL-018 layout modes, or creating automated objective plus manual listening QA lanes (including AirPods Pro 2 and Sony WH-1000XM5 constraints).
---

Title: Spatial Audio Engineering Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

# Spatial Audio Engineering

Use this skill for end-to-end spatial audio work in LocusQ: architecture, integration, testing, and evidence.

## Scope
- Speaker layout expansion: stereo, quad, 5.1, 7.1, and visualization targets such as 7.4.2.
- Ambisonic path planning and validation: FOA baseline, HOA-ready interfaces.
- Binaural/headphone rendering: Steam Audio path and deterministic fallback behavior.
- Standards/interchange awareness: ADM/BW64 and IAMF research lanes when format export or interoperability is in scope.
- Device-aware QA constraints for consumer headphones (AirPods Pro 2, Sony WH-1000XM5).

## Workflow
1. Lock the target rendering contract first.
   - Choose one primary output contract per slice: `speaker-layout`, `ambisonic-bus`, or `binaural-headphone`.
   - Keep v1 DSP contracts stable unless backlog item explicitly allows contract changes.
2. Confirm coordinate and channel conventions.
   - Document ambisonic convention assumptions (channel order + normalization) before implementation.
   - For layout visualization tasks, keep renderer layout metadata and UI labels aligned.
3. Select integration strategy.
   - Use native LocusQ DSP path when requirements fit existing contracts.
   - Use Steam Audio for binaural/HRTF path work that needs production-ready runtime fallback behavior.
   - Use open-source research stacks (SAF/libspatialaudio/pyroomacoustics) for prototype or validation harness support, not unscoped production coupling.
4. Implement with realtime safety guarantees.
   - No heap allocation, locks, or blocking I/O in `processBlock()`.
   - Keep processing deterministic for fixed input + fixed parameter/state timeline.
   - Make fallback decisions explicit and observable in scene-state telemetry.
5. Run objective automation lanes.
   - Binaural fallback lane: `scripts/qa-bl009-headphone-contract-mac.sh`.
   - Ambisonic/layout lane: `scripts/qa-bl018-ambisonic-contract-mac.sh`.
   - Combined lane wrapper: `./.codex/skills/spatial-audio-engineering/scripts/run_spatial_lanes.sh`.
6. Run targeted manual listening checks.
   - AirPods Pro 2: validate as stereo endpoint in DAW; do not claim plugin-level head-tracked personalization control.
   - Sony WH-1000XM5: validate downmix/binaural behavior as stereo endpoint unless host/app-specific 360 pipeline is explicitly integrated.
7. Close evidence and routing docs.
   - Update `Documentation/backlog-post-v1-agentic-sprints.md` row status.
   - Update `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md` when acceptance claims change.
   - Update skill routing docs when new specialist behavior is introduced.

## Realtime and Determinism Rules
- Never let device availability decide DSP graph shape at audio-thread time.
- Treat external SDK availability as initialization-time capability with deterministic runtime fallback.
- Keep channel-map transforms and ambisonic decode matrices deterministic and finite.
- Validate non-finite protection (`NaN`/`Inf`) in every new spatial lane.

## Resource Map
- `references/sources.md`: canonical official docs/repos/releases.
- `references/layout-and-codec-notes.md`: channel/layout conventions and codec notes.
- `references/validation-lanes.md`: objective + manual QA protocol.
- `scripts/run_spatial_lanes.sh`: wrapper to execute key spatial QA lanes.

## Deliverables
- List changed files with acceptance mapping.
- Report validation status as `tested`, `partially tested`, or `not tested`.
- If lanes are skipped, state the skipped lane and unresolved risk.
