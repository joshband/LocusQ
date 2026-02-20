Title: Three.js Scene Architecture Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Scene Architecture

## Goal
Build a scene lifecycle that is easy to reason about and easy to tear down without leaks.

## Choose an Integration Shape
- Use vanilla Three.js when host integration and deterministic control matter more than JSX ergonomics.
- Use React Three Fiber only when the target codebase is already React-heavy and reconciliation overhead is acceptable.
- Use a hybrid pattern when most UI is DOM and only one isolated viewport needs Three.js.

## Minimum Lifecycle Contract
Implement these functions explicitly:
- `init(canvas, opts)`
- `resize(width, height, dpr)`
- `tick(deltaSeconds)`
- `render()`
- `dispose()`

Never hide lifecycle ownership across multiple modules.

## Baseline Module Template
```js
import * as THREE from "three";

export function createSceneApp(canvas) {
  const state = {
    scene: null,
    camera: null,
    renderer: null,
    clock: new THREE.Clock(),
    rafId: 0,
    running: false,
  };

  function init() {
    state.scene = new THREE.Scene();
    state.camera = new THREE.PerspectiveCamera(60, 1, 0.1, 1000);
    state.camera.position.set(0, 2, 6);

    state.renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
    state.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  }

  function resize(width, height, dpr = window.devicePixelRatio || 1) {
    if (!state.renderer || !state.camera || width <= 0 || height <= 0) return;
    state.renderer.setPixelRatio(Math.min(dpr, 2));
    state.renderer.setSize(width, height, false);
    state.camera.aspect = width / height;
    state.camera.updateProjectionMatrix();
  }

  function tick(deltaSeconds) {
    void deltaSeconds;
    // Update animation, simulation, and interaction state.
  }

  function render() {
    state.renderer.render(state.scene, state.camera);
  }

  function frame() {
    if (!state.running) return;
    const dt = state.clock.getDelta();
    tick(dt);
    render();
    state.rafId = window.requestAnimationFrame(frame);
  }

  function start() {
    if (state.running) return;
    state.running = true;
    state.clock.start();
    frame();
  }

  function stop() {
    state.running = false;
    if (state.rafId) window.cancelAnimationFrame(state.rafId);
    state.rafId = 0;
  }

  function dispose() {
    stop();
    state.scene?.traverse(obj => {
      if (obj.geometry && typeof obj.geometry.dispose === "function") obj.geometry.dispose();
      if (obj.material) {
        const list = Array.isArray(obj.material) ? obj.material : [obj.material];
        list.forEach(mat => mat?.dispose && mat.dispose());
      }
    });
    state.renderer?.dispose();
  }

  init();
  return { resize, start, stop, dispose };
}
```

## Scene Data Flow
- Keep host data ingestion separate from render code.
- Normalize incoming payloads once, then write to scene state.
- Apply visual changes in `tick` or a dedicated update function, not directly inside bridge callbacks.

## Camera and Controls
- Keep one source of truth for camera target and spherical state.
- Clamp phi, distance, or dolly bounds to avoid invalid transforms.
- Disable conflicting interactions while drag operations are active.

## Material and Lighting Defaults
- Start with low-cost materials during logic development.
- Move to physically based materials only after geometry and interaction are stable.
- Keep shadows off by default and re-enable only if the visual delta justifies the cost.

## Teardown Checklist
- Cancel animation frame loop.
- Remove event listeners from canvas and window.
- Dispose render targets, textures, geometries, and materials.
- Null references to large scene objects and arrays.
