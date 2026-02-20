---
name: physics-reactive-audio
description: Design, implement, and validate physics-reactive audio behavior with realtime-safe DSP contracts.
---

Title: Physics-Reactive Audio Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Physics-Reactive Audio

Use this skill when audio behavior is driven by simulation state (for example: fluid dynamics, flocking, crowd behavior, 0G/gravity, drag, collisions, herd fields).

## Workflow
1. Define simulation scope and ownership: what updates on worker thread vs audio thread.
2. Choose model outputs needed by audio (force, speed, density, collision energy, proximity, topology).
3. Define parameter mapping contracts and safety clamps.
4. Implement lock-free or bounded transfer from simulation to DSP.
5. Validate stability, determinism, CPU headroom, and audible behavior.

## Reference Map
- `references/model-selection-guide.md`: Selecting and constraining simulation models.
- `references/realtime-safety.md`: Threading contracts and lock avoidance.
- `references/validation-evidence.md`: QA matrix and evidence requirements.

## Execution Rules
- Never block or allocate on the audio thread.
- Keep simulation update cadence explicit and bounded.
- Quantify worst-case CPU and memory behavior.
- Clamp all mapped values before DSP consumption.
- Add fallback behavior for simulation stalls or invalid state.

## Deliverables
- Threading/dataflow diagram.
- Mapping table from simulation state to DSP parameters.
- Validation summary with `tested`, `partially tested`, or `not tested`.
