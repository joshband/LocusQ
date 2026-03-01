Title: Information Visualization Mappings

## Mapping Contract Template
- Signal source: where the data originates.
- Transform: normalization, smoothing, deadband, clamp.
- Visual channel: position, color, scale, opacity, trail, label.
- Range semantics: thresholds and alert behavior.
- Failure semantics: missing/stale/invalid display behavior.

## Recommended Realtime Channels
- Position for spatial location.
- Color for categorical mode/state.
- Scale/intensity for magnitude.
- Trail/history window for time evolution (4D framing: value over time).
- Overlay labels for confidence/fallback reason.
