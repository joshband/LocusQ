Title: Three.js Performance and Debugging Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Performance and Debugging

## Frame-Time Budget
- 60 FPS target: 16.67 ms total frame time.
- 120 FPS target: 8.33 ms total frame time.
- Reserve headroom for host UI and bridge processing, not only GPU draw time.

## Performance First Pass
1. Disable expensive effects first: shadows, SSAO, bloom, high sample counts.
2. Measure baseline with static camera and idle scene.
3. Add one feature at a time and record the frame-time delta.
4. Keep a simple quality tier switch for lower-power systems.

## Renderer and Scene Hygiene
- Clamp device pixel ratio to a sane max for embedded UIs.
- Avoid allocating objects inside hot per-frame loops.
- Reuse vectors, rays, and temp arrays.
- Batch materials and geometries where visual parity allows.
- Prefer instancing for repeated meshes.

## Memory Leak Checklist
- Dispose geometry, material, texture, and render targets.
- Dispose postprocessing passes and composer targets.
- Remove event listeners during teardown.
- Cancel animation loops on unmount or hidden state.
- Null large references so GC can reclaim memory.

## Interaction Debugging
- Visualize raycast hits and drag planes when pointer bugs occur.
- Log mode transitions and interaction state gates.
- Ensure one interaction mode owns pointer events at a time.

## Bridge Debugging
- Verify payload shape before applying state mutations.
- Reject malformed payloads with explicit console warnings.
- Separate host event handling from scene mutation for traceability.

## Fast Instrumentation Snippets
```js
console.table(renderer.info.render);
console.table(renderer.info.memory);
```

```js
const t0 = performance.now();
// mutate scene
const t1 = performance.now();
console.log("scene update ms", (t1 - t0).toFixed(3));
```

## Regression Checklist Before Handoff
- Resize behavior correct across small and large viewport sizes.
- Camera remains stable under rapid interaction and mode switches.
- No uncaught errors when bridge is unavailable.
- Baseline frame time and memory footprint captured.
- Teardown and re-init cycle tested at least once.
