Title: Realtime 2D/3D/4D Visualization Contract

## Dimensional Layers
- 2D: controls, overlays, compact diagnostics.
- 3D: scene geometry, emitter/listener relationships, spatial context.
- 4D: explicit time-aware layer (history trails, time windows, scrub/replay views).

## Performance Contract
- Define frame budget targets per quality tier.
- Bound history window size and update cadence.
- Measure memory and update jitter for long sessions.

## Determinism Contract
- Fixed telemetry replay should produce stable visual outputs.
- Timestamp handling must avoid drift under variable host cadence.
- Stale telemetry should render explicit stale-state indicators.
