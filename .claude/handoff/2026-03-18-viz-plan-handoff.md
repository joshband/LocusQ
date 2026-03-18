---
Title: Session Handoff — Visualization Foundation Plan Ready to Execute
Document Type: Session Handoff
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Session Handoff — 2026-03-18

## Branch
`feat/physics-choreography-timeline-specs`

## What was completed this session

### 1. ADR-0020 — Four-Layer Authority Chain (previously committed)
- `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md` — accepted
- `Documentation/adr/ADR-0003-automation-authority-precedence.md` — status updated (superseded by ADR-0020 §Decision)
- `Source/PhysicsEngine.h` — `standaloneMode` atomic bool + `setStandaloneMode()`/`isStandaloneMode()` added

### 2. Implementation plans (all in `.ideas/`)
- `.ideas/choreography-lab-impl-plan.md` — 8-phase CL plan
- `.ideas/timeline-impl-plan.md` — 6-phase TL plan gated on CL
- Spec files updated to remove "Pending ADR" markers

### 3. Visualization design spec + plan (brainstormed and reviewed this session)
- `.ideas/2026-03-18-visualization-design.md` — full visual language spec (passed 3-pass review)
- `.ideas/2026-03-18-visualization-foundation.md` — Plan 1 of 3 implementation plan (passed 2-pass review)

## What was NOT yet started (next action)

**Execute visualization foundation plan** using `superpowers:subagent-driven-development`.

The plan is at `.ideas/2026-03-18-visualization-foundation.md` and is fully reviewed and ready.

### Plan summary — 7 tasks, 6 new files

| Task | File | Status |
|---|---|---|
| 1 | `Source/ui/public/incremental/index_stage13.html` | ⬜ pending |
| 2 | `Source/ui/public/incremental/js/viz-data-schema.js` | ⬜ pending |
| 3 | `Source/ui/public/incremental/js/atmo-overlay.js` | ⬜ pending |
| 4 | `Source/ui/public/incremental/js/state-ring.js` | ⬜ pending |
| 5 | `Source/ui/public/incremental/js/selection-manager.js` | ⬜ pending |
| 6 | `Source/ui/public/incremental/js/stage13_ui.js` + patch `stage12_ui.js` | ⬜ pending |
| 7 | Integration polish (layer panel, dimming) | ⬜ pending |

### Key architectural facts for the implementer

- **Existing viewport**: Three.js 3D (`createSceneApp` in `stage12_ui.js`). The `viewport-shell` div contains a `#viewport-canvas` (Three.js) and `.viewport-overlay` (text label).
- **Overlay approach**: Add `<canvas id="atmo-canvas">` absolutely positioned over Three.js canvas (`pointer-events:none`, `z-index:10`). State rings use a separate `<svg>` overlay at `z-index:11`.
- **Stage pattern**: Each stage is a standalone `index_stageN.html` + `js/stageN_ui.js`. Stage 13 loads stage12_ui.js + new modules via `<script defer>`.
- **Critical stage12 patches required** (Task 6):
  1. Add `getCamera() { return state.camera; }` to the `createSceneApp` **return object literal** (NOT inside `init()`)
  2. Add `window.__locusq_sceneApp = sceneApp` after `createSceneApp(...)` call
  3. Add `window.__locusq_onSceneUpdate` hook in `updateSceneState` function (NOT in `heartbeat`)
- **WKWebView compat**: Never animate SVG `r` attribute via CSS keyframes. Use `transform:scale()` on `<g>` wrapper instead.
- **Canvas resize**: Use `ctx.setTransform(dpr,0,0,dpr,0,0)` (absolute), NOT `ctx.scale()` (additive — causes drift).
- **setTimeout ordering**: `stage13_ui.js` uses `DOMContentLoaded` + `setTimeout(initViz,0)` to guarantee stage12 initializes first. Do NOT remove the setTimeout.
- **Y/Z swap**: JUCE convention maps Y↔Z. Use `new THREE.Vector3(em.x, em.z, em.y)` for projection.

### Untracked files also on branch (not committed yet — Physics P1 work in progress)
- `Source/PhysicsDSPBridge.h`
- `Source/PhysicsWorker.h`

## Visualization design decisions (locked — do not revisit)

**Two-tier system:**
- Tier 1 (all unselected): dot + spread aura + audio/physics/choro atmosphere + state ring
- Tier 2 (selected): + selection ring + HUD (spread%, gain dB) + force vectors + directivity lobe + collision halo

**Audio atmosphere (always-on A/B/C):**
- A: RMS → aura breathe
- B: Spectral centroid → hue; band energy → saturation
- C: Onset events → transient rings (amplitude at spawn, renderer-side time decay, no per-frame data)

**State ring:** One SVG ring, four fixed quadrants: Physics (12→3, blue), Choreography (3→6, green), Beat-sync (6→9, amber), Timeline (9→12, violet). Plans 2/3 activate segments.

**Selection transition:** 350ms in — dot swells (0–80ms) → ring bounce-in (80–200ms, `cubic-bezier(0.22,1,0.36,1)`) → analytical layers fade up (150–350ms). 150ms out. Only one emitter in Tier 2 at a time.

**Multi-instance ghosts (Plan 1 NOT in scope):** IPC mechanism unspecified (§10 open item).

## Spec and plan locations
- Spec: `.ideas/2026-03-18-visualization-design.md`
- Plan: `.ideas/2026-03-18-visualization-foundation.md`

## Memory updated
- `feedback_brainstorm_output_location.md` — brainstorm outputs go to `.ideas/`, not `docs/superpowers/`

## Next command to run in a new session
```
use appropriate skills → execute visualization foundation plan
```
Or directly: invoke `superpowers:subagent-driven-development` with plan at `.ideas/2026-03-18-visualization-foundation.md`.
