Title: Three.js JUCE WebView Integration Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# JUCE WebView Integration

## Goal
Keep Three.js runtime logic and JUCE bridge logic decoupled while preserving low-latency updates.

## Existing LocusQ Bridge Surface
Follow established patterns in this repo before introducing new bridge contracts:
- JS to native events: `window.__JUCE__.backend.emitEvent(identifier, payload)`
- Native to JS scene push: `window.updateSceneState(data)`
- Native to JS calibration push: `window.updateCalibrationStatus(status)`

Reuse current entry points unless a migration is explicitly requested.

## Integration Flow
1. Initialize bridge-safe fallback behavior first so browser-only previews still run.
2. Start Three.js scene only after required DOM elements exist.
3. Bind bridge listeners and route payloads into normalized scene state.
4. Render from local state, not directly from bridge callback side effects.

## Safe Host to Scene Pattern
```js
let pendingSceneState = null;

window.updateSceneState = function (data) {
  pendingSceneState = normalizeSceneData(data);
};

function tick() {
  if (pendingSceneState) {
    applySceneState(pendingSceneState);
    pendingSceneState = null;
  }
  animate();
  render();
}
```

This pattern prevents callback bursts from destabilizing render pacing.

## Safe Scene to Host Pattern
- Emit only normalized values and explicit event types.
- Debounce high-frequency UI events before emitting to host.
- Include identifiers that map to existing JUCE parameter or event channels.

Example:
```js
window.__JUCE__.backend.emitEvent("__juce__sliderGain", {
  eventType: "valueChanged",
  value: nextValue
});
```

## Error Handling
- Guard bridge access with capability checks.
- Provide degraded-mode logs when `window.Juce` or `window.__JUCE__` is missing.
- Keep scene operational in local browser preview even when native bridge is absent.

## Bridge Diagnostics Checklist
- Confirm `window.Juce` exists.
- Confirm `window.__JUCE__` exists.
- Confirm `window.__JUCE__.backend` exposes `emitEvent` and `addEventListener`.
- Confirm native side can call JS entry points without exceptions.
- Confirm JS events reach native handlers with expected payload shape.

## Throughput Guidance
- Keep host to JS payloads compact and typed.
- Send only deltas where feasible for frequently changing state.
- Split slow, bulky state updates from fast interaction updates.
