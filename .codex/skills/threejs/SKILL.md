---
name: threejs
description: Build, integrate, and troubleshoot Three.js interfaces for web-based plugin UIs. Use when tasks involve Three.js scene setup, cameras, lighting, materials, animation loops, shaders, pointer interactions, postprocessing, WebGL performance and memory issues, or 3D spatial-audio UI integration with JUCE WebView bridges, including Apple Spatial Audio, Atmos workflows, custom layouts such as 7.4.2, and binaural monitoring support.
---

Title: Three.js Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Three.js

Use this skill to produce production-grade Three.js code with deterministic lifecycle, explicit cleanup, and stable frame-time behavior.

## Workflow
1. Confirm runtime constraints first: host type, render surface size, target frame rate, and interaction model.
2. Choose architecture before coding: vanilla Three.js module, framework wrapper, or hybrid integration.
3. Implement a minimal scene lifecycle first: init, resize, tick, render, dispose.
4. Add interaction and bridge wiring only after the baseline scene is stable.
5. Add postprocessing and visual polish only after performance and memory are measured.
6. Validate with concrete evidence and report residual risks.

## Reference Map
- `references/scene-architecture.md`: Use for scene structure, render-loop ownership, and teardown template.
- `references/juce-webview-integration.md`: Use for JUCE WebView bridge patterns and C++ to JS state updates.
- `references/spatial-audio-integration.md`: Use for 3D emitter and listener modeling, coordinate mapping, and audio-thread-safe transport.
- `references/sdk-api-oss-research-landscape.md`: Use for SDK/API selection, GitHub toolchain discovery, and research/project starting points.
- `references/performance-and-debugging.md`: Use for frame-time budgeting, GPU memory hygiene, and diagnostics flow.

## Execution Rules
- Start with one camera, one renderer, and one explicit animation loop owner.
- Treat resize and device pixel ratio changes as first-class lifecycle events.
- Dispose geometry, material, texture, and render target resources during teardown.
- Keep scene state updates unidirectional when integrating host bridge events.
- Gate expensive features behind capability checks and quality tiers.
- Preserve existing bridge event names and JS entry points unless a migration is requested.
- When fixing bugs, capture a reproducible case and include before and after evidence.

## Deliverables
- List changed files and the reason each change was required.
- Report validation status explicitly as `tested`, `partially tested`, or `not tested`.
- If validation is skipped, state why and identify the highest-risk unresolved area.
