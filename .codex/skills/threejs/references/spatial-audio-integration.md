Title: Three.js Spatial Audio Integration Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Spatial Audio Integration (JUCE + WebView + Three.js)

## Goal
Drive a spatial-audio engine from a 3D UI while keeping the audio thread real-time safe.

## Architecture Contract
- Treat Three.js as the visualization and interaction surface.
- Treat JUCE DSP as the source of truth for audio state.
- Exchange compact event payloads over the WebView bridge.
- Keep audio-thread mutation lock-free or bounded and deterministic.

## Coordinate Mapping
Define one canonical coordinate system and convert at boundaries.

Recommended default:
- UI world: `x` right, `y` up, `z` forward.
- Audio world: azimuth and elevation in degrees, distance in meters.

Conversion guidance:
- `azimuthDeg = atan2(z, x) * 180 / PI`
- `distanceM = sqrt(x*x + y*y + z*z)`
- `elevationDeg = atan2(y, sqrt(x*x + z*z)) * 180 / PI`

Clamp on ingest:
- Azimuth: `[-180, 180]`
- Elevation: `[-90, 90]`
- Distance: project-specific min/max, never negative.

## State Model
Keep separate models for:
- `visualState`: fast, frame-rate updates for mesh transforms.
- `controlState`: debounced and normalized transport payloads.
- `dspState`: smoothed values consumed by audio processing.

Never write raw pointer deltas directly into DSP parameters.

## Transport Payload Shape
Prefer explicit and versioned payloads:

```json
{
  "eventType": "spatialUpdate",
  "schema": 1,
  "emitterId": 3,
  "position": { "x": 1.25, "y": 0.35, "z": -2.10 },
  "spherical": { "azimuthDeg": -59.2, "elevationDeg": 8.4, "distanceM": 2.47 },
  "tMs": 1700000000000
}
```

Send only what changed for high-rate interactions.

## Update-Rate Strategy
- UI interaction sample rate can be high, but bridge sends should be throttled.
- Use trailing debounce or fixed-rate throttling (for example 30 to 60 Hz).
- Apply parameter smoothing in DSP to remove zipper noise.

Two-stage smoothing:
1. Bridge throttling reduces event burst pressure.
2. DSP ramping smooths audible transitions.

## JUCE Thread-Safety Rules
- Do not allocate memory on the audio thread.
- Do not call locks that can block the audio callback.
- Use atomics, lock-free queues, or double-buffer snapshots for control updates.
- Parse and validate JSON off the audio thread, then publish compact structs.

## Emitter and Listener Model
- Keep stable integer emitter IDs across UI and DSP.
- Define listener pose explicitly: position plus forward/up vectors or yaw/pitch/roll.
- Reject events for unknown emitter IDs unless creation is intentional.
- Apply bounds checks before committing state.

## Bridge API Pattern
Use existing LocusQ style contracts where possible:
- JS to native: `window.__JUCE__.backend.emitEvent(identifier, payload)`
- Native to JS full-state refresh: `window.updateSceneState(data)`

Recommended split:
- High-rate control updates: per-emitter delta events.
- Lower-rate sync updates: canonical full scene snapshots from native.

## Apple Spatial Audio Notes
- Treat Apple Spatial Audio as a renderer target, not only a UI feature.
- Keep channel-layout metadata explicit when running as AU or standalone on Apple platforms.
- Prefer platform-native layout descriptors and verify host-reported bus layouts at runtime.
- Keep head-tracking hooks optional and feature-gated when no tracking source is available.

## Atmos and Bed/Object Strategy
- Model two paths explicitly:
- Bed path: fixed speaker layout channels.
- Object path: emitter metadata plus renderer mapping.
- Keep UI emitter IDs stable so object metadata remains traceable.
- Provide fallback behavior when object rendering is unavailable:
- Downroute objects to bed channels with documented priority rules.

## 7.4.2 and Layout-Naming Ambiguity
- Treat `7.4.2` as a project-defined layout string, because naming conventions vary by pipeline.
- Store layout as explicit speaker labels and order, not only as shorthand text.
- Validate every payload against the active layout map before committing to DSP state.

Example explicit contract:
```json
{
  "layoutId": "7.4.2",
  "channels": ["L","R","C","LFE1","Ls","Rs","Lrs","Rrs","Tfl","Tfr","Tbl","Tbr","LFE2"]
}
```

If your target stack defines `7.4.2` differently, replace channel labels and keep the same explicit-map approach.

## Binaural Monitoring Support
- Expose a renderer mode switch: `speaker`, `binaural`, or `auto`.
- Keep binaural settings independent from speaker-bed routing so A/B is deterministic.
- Apply HRTF rendering only on the monitoring path unless the export path explicitly requires it.
- Add per-emitter binaural metadata only if your renderer supports it.
- Persist binaural mode and key HRTF options in plugin state so session recall is stable.

## Validation Checklist
- Dragging emitter updates both mesh position and audible position.
- Fast drags do not crackle or produce zipper artifacts.
- Listener rotation updates binaural/spatial cues consistently.
- UI and DSP remain in sync after rapid mode changes.
- Reopen/reload preserves emitter IDs and spatial mapping.
- Apple host reports expected channel layout and route.
- Atmos object fallback behavior is deterministic when object rendering is unavailable.
- 7.4.2 channel map matches documented speaker-label order.
- Binaural mode null-test and level matching pass against speaker mode reference.

## Debug Checklist
- Log one normalized spatial event per throttle tick in debug mode.
- Verify conversion parity by round-tripping cartesian and spherical values.
- Capture per-block DSP timing to detect control-queue pressure.
- Confirm fallback browser mode runs without native bridge errors.
