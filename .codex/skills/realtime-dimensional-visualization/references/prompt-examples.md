Title: Realtime Visualization Prompt Examples

Use these prompts for beautiful, high-clarity realtime 2D/3D/4D UI slices.

## Design Prompt

```text
/design LocusQ Visualization Slice A
Load: $realtime-dimensional-visualization, $skill_design, $threejs

Goal:
- Define an intentional visual language for plugin telemetry (not generic defaults).
- Deliver 2D overview + 3D spatial context + 4D time-history interaction concept.

Requirements:
- Clear hierarchy for control/state/diagnostics.
- Semantic color + purposeful typography + motion that explains state transitions.
- Host-aware frame-budget targets and graceful quality tiers.
```

## Implementation Prompt

```text
/impl LocusQ Visualization Slice B
Load: $realtime-dimensional-visualization, $threejs, $reactive-av, $juce-webview-runtime

Goal:
- Implement data-to-encoding mappings with smoothing/deadband controls.
- Build performant render-loop behavior for plugin-host constraints.

Checks:
- Deterministic response for deterministic playback input.
- Stable, legible diagnostics under high update rate.
- Quality-tier fallback for heavy post effects and dense scene modes.
```

## Validation Prompt

```text
/test LocusQ Visualization Candidate Gate
Load: $realtime-dimensional-visualization, $skill_testing

Run:
- clarity/legibility checklist under fast-state updates
- frame-time budget lane across quality tiers
- interaction latency and jitter checks
- replay-parity checks for visual determinism
```
