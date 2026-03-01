Title: Simulation Audio-Visual Prompt Examples

Use these prompts when simulation behavior drives both DSP and visuals.

## Plan Slice Prompt

```text
/plan LocusQ Simulation Slice A
Load: $simulation-behavior-audio-visual, $physics-reactive-audio, $reactive-av

Goal:
- Choose simulation family (flocking/crowd/flow/hybrid) and define authority model.
- Produce mapping contracts: simulation -> DSP params -> visualization channels.

Constraints:
- Worker-thread simulation authority only.
- Audio-thread consumption stays lock-free/bounded.
```

## Implementation Slice Prompt

```text
/impl LocusQ Simulation Slice B
Load: $simulation-behavior-audio-visual, $skill_impl, $realtime-dimensional-visualization

Goal:
- Implement bounded simulation transport and deterministic fallback behavior.
- Add synchronized visual channels for motion energy, density, and collision stress.

Checks:
- Stalls/NaNs never block or destabilize audio path.
- All simulation-derived values are clamped/sanitized.
- Visual updates remain coherent at target frame budget.
```

## Validation Prompt

```text
/test LocusQ Simulation Candidate Gate
Load: $simulation-behavior-audio-visual, $skill_testing

Run:
- Deterministic replay scenarios for each model family
- CPU/memory budget lanes
- Audio artifact checks + visual coherence checks
- Fallback behavior drills (stall, saturation, invalid-state injection)
```
