---
name: simulation-behavior-audio-visual
description: Design and validate complex simulation-driven audio and visualization behavior for LocusQ (fluid-like fields, crowd/flocking/herd models, interaction forces) with deterministic realtime-safe DSP and synchronized visual contracts.
---

Title: Simulation Behavior Audio-Visual Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Simulation Behavior Audio-Visual

Use this skill when complex simulation behavior (fluid/crowd/flocking/herd/field interactions) drives both sound and visualization.

## Scope
- Simulation model selection and scaling (boids, crowd fields, flow-style dynamics).
- Deterministic worker-thread simulation plus realtime-safe DSP handoff.
- Synchronized simulation-to-audio and simulation-to-visual mapping contracts.
- Failure/fallback behavior when simulation becomes invalid or unstable.

## Workflow
1. Define simulation authority and update cadence.
   - Separate worker-thread simulation ownership from audio-thread consumption.
2. Define mapping contracts before coding.
   - Simulation features -> DSP parameters -> visual channels.
3. Implement bounded transport paths.
   - Lock-free or bounded queues, finite guards, and clamp policies.
4. Add deterministic fallback strategy.
   - Stable degraded behavior for stalls, NaNs, or saturation events.
5. Validate behavior with reproducible scenarios.
   - Check audible results, visual coherence, CPU headroom, and deterministic replay.

## Realtime Rules
- Never allocate or block on audio thread.
- Never let simulation stalls block audio output path.
- Clamp and sanitize all simulation-derived values before DSP and UI use.
- Keep update rate and payload size bounded and measurable.

## Cross-Skill Routing
- Pair with `physics-reactive-audio` for audio-thread DSP ownership decisions.
- Pair with `reactive-av` for visual mapping/smoothing policies.
- Pair with `realtime-dimensional-visualization` for 2D/3D/4D visual storytelling and diagnostics.
- Pair with `skill_testing` for deterministic evidence capture.

## References
- `references/model-families.md`
- `references/realtime-contracts.md`
- `references/prompt-examples.md`

## Deliverables
- Simulation/dataflow diagram and mapping table.
- Validation status: `tested`, `partially tested`, or `not tested`.
- Highest-risk unresolved lane if full validation is not run.
