---
Title: LocusQ Visualization Foundation — Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Visualization Foundation Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the two-tier emitter visualization system on top of the existing Three.js viewport — a 2D Canvas atmosphere overlay, multi-emitter dot projection, audio-reactive atmosphere layers A/B/C, the four-quadrant state ring, and the Tier 1→2 selection transition.

**Architecture:** A transparent `<canvas id="atmo-canvas">` is layered absolutely over `#viewport-canvas` (pointer-events: none). Each frame, emitter 3D positions are projected to 2D screen coords using `THREE.Vector3.project()`. The atmosphere canvas draws aura, audio-reactive layers, and state rings per emitter. Selection is detected via pointer distance tests on projected coords; selected emitters enter Tier 2 (SVG selection ring + HUD labels faded in via CSS animation). Audio-reactive data (`rms`, `spectralCentroid`, `bandEnergy`, onset events) arrives via the existing JUCE WebView bridge in the scene payload; fields default to neutral values until the C++ audio pipeline is wired.

**Tech Stack:** Vanilla JS, Canvas 2D API, SVG, CSS animation, Three.js (existing — for 3D→2D projection only), JUCE WebView bridge (existing)

**Spec:** `.ideas/2026-03-18-visualization-design.md`

**Plan series:** This is Plan 1 of 3. Plan 2 (physics-reactive atmosphere, state ring physics/beat segments) requires `PhysicsWorker` wired. Plan 3 (choreography-reactive atmosphere, state ring choreography/timeline segments) requires `ChoreographyWorker` wired.

---

## Scope Boundaries

**In scope (Plan 1):**
- Stage 13 incremental HTML/JS
- 2D atmosphere Canvas overlay infrastructure
- Multi-emitter dot rendering (all emitters in `scene.emitters`, not just the local one)
- 3D→2D position projection with viewport resize tracking
- Spread aura (base — driven by `emitter.spread`)
- Audio-reactive layer A — Breathe (RMS → aura radius/opacity)
- Audio-reactive layer B — Spectral heatmap (centroid → hue, band energy → saturation)
- Audio-reactive layer C — Transient rings (onset events → ring spawn)
- Emitter state ring: base dim ring (no colored segments — those come in Plans 2/3)
- Selection: click detection, Tier 1→2 transition (ring bounce-in, HUD labels, dim others)
- Layer panel: atmosphere and analytical visibility toggles

**Not in scope (Plan 1):**
- Audio layers D (motion trail), E (beat arc), F (energy cloud)
- Physics-reactive atmosphere (Plan 2)
- Choreography-reactive atmosphere (Plan 3)
- State ring colored segments for physics/choro/beat/timeline (Plans 2/3)
- Multi-instance ghost emitters (IPC mechanism unspecified — §10 of spec)
- C++ AudioRingBuffer / audio analysis (wired in CL-P1; JS side stubs for now)

---

## File Structure

| File | Action | Responsibility |
|---|---|---|
| `Source/ui/public/incremental/index_stage13.html` | Create | Stage 13 HTML — copies stage12 structure, adds `atmo-canvas` overlay and layer panel |
| `Source/ui/public/incremental/js/stage13_ui.js` | Create | Stage 13 entry — imports stage12 runtime, wires `AtmoOverlay` and `SelectionManager` |
| `Source/ui/public/incremental/js/atmo-overlay.js` | Create | 2D Canvas atmosphere renderer — projects emitters, draws aura/layers/rings per frame |
| `Source/ui/public/incremental/js/state-ring.js` | Create | SVG state ring per emitter — manages quadrant segments (base ring only in Plan 1) |
| `Source/ui/public/incremental/js/selection-manager.js` | Create | Click detection, Tier 1→2 state machine, HUD label lifecycle |
| `Source/ui/public/incremental/js/viz-data-schema.js` | Create | Defines and validates the audio/physics/choro data fields on `scene.emitters[n]`; provides default values for all stub fields |

### Data schema contract (viz-data-schema.js)

Each `emitter` object in `scene.emitters` is extended with these optional fields (all defaulted when absent):

```js
// Audio (populated by C++ audio bridge — CL-P1 onwards)
emitter.audioRms          // float 0..1 — RMS level, default 0.25 (mid breathe)
emitter.spectralCentroid  // float 0..1 — normalized spectral centroid, default 0.5
emitter.bandEnergy        // { low, mid, high } floats 0..1, default { low:0.3, mid:0.3, high:0.2 }

// Onset events (populated by C++ audio bridge — CL-P1 onwards)
// scene.onsetEvents: Array<{ emitterId: number, amplitude: float 0..1 }>
// New entries each scene tick; consumed and cleared by AtmoOverlay

// Physics mode flags (populated by PhysicsWorker — Plan 2)
emitter.physicsActive     // bool, default false
emitter.boidsActive       // bool, default false
emitter.attractorForce    // float 0..1, default 0
emitter.collisionEvent    // bool (pulse), default false
emitter.wallCollisionEvent // bool (pulse), default false
emitter.springActive      // bool, default false
emitter.springOmega       // float Hz, default 0
emitter.dragLevel         // float 0..1, default 0

// Choreography mode flags (populated by ChoreographyWorker — Plan 3)
emitter.choreographyActive  // bool, default false
emitter.formationActive     // bool, default false
emitter.pathActive          // bool, default false
emitter.beatSyncActive      // bool, default false
emitter.timelineActive      // bool, default false
```

---

## Task 1: Stage 13 HTML — overlay canvas and layer panel

**Files:**
- Create: `Source/ui/public/incremental/index_stage13.html`

Add a second canvas (`atmo-canvas`) absolutely positioned over `viewport-canvas`, and a layer panel control strip. Base this on `index_stage12.html` — copy it and make targeted additions only.

- [ ] **Step 1.1: Copy stage12 HTML as starting point**

```bash
cp Source/ui/public/incremental/index_stage12.html \
   Source/ui/public/incremental/index_stage13.html
```

- [ ] **Step 1.2: Update title and script references**

In `index_stage13.html`, change:
```html
<!-- FROM -->
<title>LocusQ Incremental UI Stage 12</title>
<script src="/js/juce/check_native_interop.js?cb=12"></script>
<script defer src="/js/three.min.js"></script>
<script defer src="/incremental/js/stage12_ui.js?cb=12"></script>

<!-- TO -->
<title>LocusQ Incremental UI Stage 13</title>
<script src="/js/juce/check_native_interop.js?cb=13"></script>
<script defer src="/js/three.min.js"></script>
<script defer src="/incremental/js/stage13_ui.js?cb=13"></script>
```

- [ ] **Step 1.3: Add `atmo-canvas` overlay in viewport-shell**

Find the `<canvas id="viewport-canvas">` element and add the atmosphere canvas immediately after it:
```html
<canvas id="viewport-canvas"></canvas>
<canvas id="atmo-canvas" aria-hidden="true"></canvas>
```

Add CSS for `atmo-canvas`:
```css
#atmo-canvas {
  position: absolute;
  top: 0; left: 0;
  width: 100%; height: 100%;
  pointer-events: none;
  z-index: 10;
}
```

- [ ] **Step 1.4: Add layer panel strip above the viewport info overlay**

Add inside `.viewport-shell`, after `#atmo-canvas`:
```html
<div class="layer-panel" id="layer-panel" aria-label="Layer visibility">
  <button class="layer-btn active" data-layer="atmosphere" title="Toggle atmosphere layers">ATM</button>
  <button class="layer-btn active" data-layer="analytical" title="Toggle analytical overlay">ANA</button>
</div>
```

Add CSS:
```css
.layer-panel {
  position: absolute;
  top: 8px; right: 8px;
  display: flex; gap: 4px;
  z-index: 20;
}
.layer-btn {
  font-size: 9px; font-family: var(--font);
  padding: 3px 6px; border-radius: 3px;
  background: rgba(0,0,0,0.55);
  border: 1px solid rgba(255,255,255,0.12);
  color: rgba(255,255,255,0.5);
  cursor: pointer; letter-spacing: 0.06em;
}
.layer-btn.active { color: rgba(255,255,255,0.85); border-color: rgba(255,255,255,0.3); }
```

- [ ] **Step 1.5: Verify HTML renders without errors**

Open `http://localhost:5173/incremental/index_stage13.html` (or the project's dev server) and confirm:
- No console errors
- Viewport renders (Three.js 3D scene visible)
- `atmo-canvas` element exists with `z-index: 10`
- Layer panel buttons visible top-right

- [ ] **Step 1.6: Commit**

```bash
git add Source/ui/public/incremental/index_stage13.html
git commit -m "feat(viz): add stage13 HTML with atmo-canvas overlay and layer panel"
```

---

## Task 2: viz-data-schema.js — data contract and defaults

**Files:**
- Create: `Source/ui/public/incremental/js/viz-data-schema.js`

- [ ] **Step 2.1: Write viz-data-schema.js**

```js
// viz-data-schema.js
// Defines and normalises the visualization data fields expected on each emitter object.
// All fields default to neutral stub values when absent (C++ bridge not yet wired).
// Plans 2/3 wire physics/choreography fields when their workers are active.

"use strict";

const VIZ_DEFAULTS = {
  audioRms: 0.25,
  spectralCentroid: 0.5,
  bandEnergy: { low: 0.3, mid: 0.3, high: 0.2 },
  physicsActive: false,
  boidsActive: false,
  attractorForce: 0,
  collisionEvent: false,
  wallCollisionEvent: false,
  springActive: false,
  springOmega: 0,
  dragLevel: 0,
  choreographyActive: false,
  formationActive: false,
  pathActive: false,
  beatSyncActive: false,
  timelineActive: false,
};

/**
 * Returns a normalised emitter viz data object.
 * Merges incoming emitter fields with defaults; validates numeric ranges.
 */
function normaliseEmitterViz(emitter) {
  const d = VIZ_DEFAULTS;
  return {
    id: Number(emitter.id) || 0,
    x: Number(emitter.x) || 0,
    y: Number(emitter.y) || 0,
    z: Number(emitter.z) || 0,
    spread: Math.max(0, Math.min(1, Number(emitter.spread) || 0.5)),
    gain: Math.max(0, Math.min(1, Number(emitter.gain) || 0.7)),
    audioRms: Math.max(0, Math.min(1, Number(emitter.audioRms ?? d.audioRms))),
    spectralCentroid: Math.max(0, Math.min(1, Number(emitter.spectralCentroid ?? d.spectralCentroid))),
    bandEnergy: {
      low:  Math.max(0, Math.min(1, Number((emitter.bandEnergy || d.bandEnergy).low))),
      mid:  Math.max(0, Math.min(1, Number((emitter.bandEnergy || d.bandEnergy).mid))),
      high: Math.max(0, Math.min(1, Number((emitter.bandEnergy || d.bandEnergy).high))),
    },
    physicsActive:      Boolean(emitter.physicsActive      ?? d.physicsActive),
    boidsActive:        Boolean(emitter.boidsActive        ?? d.boidsActive),
    attractorForce:     Math.max(0, Math.min(1, Number(emitter.attractorForce ?? d.attractorForce))),
    collisionEvent:     Boolean(emitter.collisionEvent     ?? d.collisionEvent),
    wallCollisionEvent: Boolean(emitter.wallCollisionEvent ?? d.wallCollisionEvent),
    springActive:       Boolean(emitter.springActive       ?? d.springActive),
    springOmega:        Math.max(0, Number(emitter.springOmega ?? d.springOmega)),
    dragLevel:          Math.max(0, Math.min(1, Number(emitter.dragLevel ?? d.dragLevel))),
    choreographyActive: Boolean(emitter.choreographyActive ?? d.choreographyActive),
    formationActive:    Boolean(emitter.formationActive    ?? d.formationActive),
    pathActive:         Boolean(emitter.pathActive         ?? d.pathActive),
    beatSyncActive:     Boolean(emitter.beatSyncActive     ?? d.beatSyncActive),
    timelineActive:     Boolean(emitter.timelineActive     ?? d.timelineActive),
  };
}

/**
 * Consumes onset events from scene payload for a given emitter.
 * Returns array of amplitude values (0..1) for new onset rings to spawn.
 * scene.onsetEvents: Array<{ emitterId: number, amplitude: number }>
 */
function consumeOnsetEvents(scene, emitterId) {
  if (!Array.isArray(scene.onsetEvents)) return [];
  const out = [];
  const remaining = [];
  for (const ev of scene.onsetEvents) {
    if (Number(ev.emitterId) === emitterId) {
      out.push(Math.max(0, Math.min(1, Number(ev.amplitude) || 0.5)));
    } else {
      remaining.push(ev);
    }
  }
  scene.onsetEvents = remaining;
  return out;
}

window.VizDataSchema = { normaliseEmitterViz, consumeOnsetEvents, VIZ_DEFAULTS };
```

- [ ] **Step 2.2: Write unit tests inline**

Add a self-test block at the bottom of `viz-data-schema.js`:
```js
// Self-test (runs only when URL contains ?selftest)
if (new URLSearchParams(location.search).has("selftest")) {
  function assert(cond, msg) { if (!cond) throw new Error("FAIL: " + msg); }

  const e1 = normaliseEmitterViz({});
  assert(e1.audioRms === 0.25, "default audioRms");
  assert(e1.spread === 0.5, "default spread");
  assert(e1.bandEnergy.low === 0.3, "default bandEnergy.low");
  assert(e1.physicsActive === false, "default physicsActive");

  const e2 = normaliseEmitterViz({ audioRms: 1.5, spread: -0.1 });
  assert(e2.audioRms === 1.0, "audioRms clamped to 1");
  assert(e2.spread === 0, "spread clamped to 0");

  const scene = { onsetEvents: [{ emitterId: 1, amplitude: 0.8 }, { emitterId: 2, amplitude: 0.5 }] };
  const hits = consumeOnsetEvents(scene, 1);
  assert(hits.length === 1 && hits[0] === 0.8, "consumeOnsetEvents returns matching event");
  assert(scene.onsetEvents.length === 1 && scene.onsetEvents[0].emitterId === 2, "consumeOnsetEvents removes consumed event");

  console.log("[viz-data-schema] self-test PASS");
}
```

- [ ] **Step 2.3: Run self-test**

Open in browser: `http://localhost:5173/incremental/js/viz-data-schema.js?selftest`

Expected console output: `[viz-data-schema] self-test PASS`

- [ ] **Step 2.4: Commit**

```bash
git add Source/ui/public/incremental/js/viz-data-schema.js
git commit -m "feat(viz): add viz-data-schema with emitter field contract and defaults"
```

---

## Task 3: atmo-overlay.js — 2D Canvas atmosphere renderer

**Files:**
- Create: `Source/ui/public/incremental/js/atmo-overlay.js`

This module owns the `atmo-canvas` 2D context. Each animation frame it clears the canvas, iterates `emitters`, projects each to 2D, and draws: spread aura, audio breathe (A), spectral hue (B), active onset rings (C).

- [ ] **Step 3.1: Write AtmoOverlay class skeleton**

```js
// atmo-overlay.js
// 2D atmosphere canvas renderer. Draws per-emitter atmosphere layers each rAF.
// Depends on: viz-data-schema.js (VizDataSchema), THREE (global, for projection)
"use strict";

class AtmoOverlay {
  /**
   * @param {HTMLCanvasElement} atmoCanvas - the overlay canvas
   * @param {HTMLCanvasElement} threeCanvas - the Three.js canvas (for size reference)
   * @param {Function} getCamera - returns the current THREE.PerspectiveCamera
   * @param {Function} getEmitters - returns Array of normalised emitter viz objects
   */
  constructor(atmoCanvas, threeCanvas, getCamera, getEmitters) {
    this._canvas = atmoCanvas;
    this._threeCanvas = threeCanvas;
    this._getCamera = getCamera;
    this._getEmitters = getEmitters;
    this._ctx = atmoCanvas.getContext("2d");
    this._rafId = 0;
    this._running = false;
    this._layers = { atmosphere: true, analytical: true };

    // Active onset rings per emitter: Map<emitterId, Array<{r, opacity, amplitude}>>
    this._onsetRings = new Map();

    // Resize observer to keep atmo-canvas in sync with three-canvas
    if (typeof ResizeObserver !== "undefined") {
      this._resizeObserver = new ResizeObserver(() => this._syncSize());
      this._resizeObserver.observe(threeCanvas);
    }
    this._syncSize();
  }

  setLayerVisible(layer, visible) {

    this._layers[layer] = visible;
  }

  /**
   * Spawn a transient ring for an emitter (audio-reactive layer C).
   * @param {number} emitterId
   * @param {number} amplitude 0..1
   */
  spawnOnsetRing(emitterId, amplitude) {
    if (!this._onsetRings.has(emitterId)) this._onsetRings.set(emitterId, []);
    this._onsetRings.get(emitterId).push({ r: 0, opacity: Math.max(0.3, amplitude * 0.8), amplitude });
  }

  start() {
    if (this._running) return;
    this._running = true;
    this._frame();
  }

  stop() {
    this._running = false;
    if (this._rafId) cancelAnimationFrame(this._rafId);
    this._rafId = 0;
  }

  dispose() {
    this.stop();
    if (this._resizeObserver) this._resizeObserver.disconnect();
  }

  _syncSize() {
    const w = this._threeCanvas.clientWidth || this._threeCanvas.width;
    const h = this._threeCanvas.clientHeight || this._threeCanvas.height;
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    this._canvas.width = Math.round(w * dpr);
    this._canvas.height = Math.round(h * dpr);
    this._canvas.style.width = w + "px";
    this._canvas.style.height = h + "px";
    // Use setTransform (absolute) not scale() (additive) — prevents scale drift on repeated resizes
    this._ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this._w = w;
    this._h = h;
    this._dpr = dpr;
  }

  _frame() {
    if (!this._running) return;
    this._draw();
    this._rafId = requestAnimationFrame(() => this._frame());
  }

  _draw() {
    const ctx = this._ctx;
    const w = this._w, h = this._h;
    ctx.clearRect(0, 0, w, h);

    if (!this._layers.atmosphere) return;

    const emitters = this._getEmitters();
    const camera = this._getCamera();
    if (!emitters || !camera || !window.THREE) return;

    for (const em of emitters) {
      const pos2d = this._project(em, camera, w, h);
      if (!pos2d) continue;
      this._drawEmitterAtmo(ctx, em, pos2d, w, h);
    }
  }

  /**
   * Project a 3D emitter position to 2D canvas coordinates.
   * Returns null if behind camera.
   */
  _project(em, camera, w, h) {
    const v = new window.THREE.Vector3(em.x, em.z, em.y); // JUCE Y↔Z convention
    v.project(camera);
    if (v.z > 1) return null; // behind camera
    return {
      x: (v.x * 0.5 + 0.5) * w,
      y: (-v.y * 0.5 + 0.5) * h,
    };
  }

  _drawEmitterAtmo(ctx, em, pos, w, h) {
    const { x, y } = pos;
    const baseRadius = Math.max(20, em.spread * 80);

    // ── Layer A: Breathe (RMS → aura radius and opacity) ──
    const breatheScale = 0.7 + em.audioRms * 0.6;      // 0.7 at silence, 1.3 at peak
    const breatheOpacity = 0.08 + em.audioRms * 0.18;  // 0.08..0.26
    const auraR = baseRadius * breatheScale;
    const grad = ctx.createRadialGradient(x, y, 0, x, y, auraR);
    grad.addColorStop(0, `rgba(255,255,255,${breatheOpacity})`);
    grad.addColorStop(1, "rgba(255,255,255,0)");
    ctx.beginPath();
    ctx.arc(x, y, auraR, 0, Math.PI * 2);
    ctx.fillStyle = grad;
    ctx.fill();

    // ── Layer B: Spectral heatmap (centroid → hue, band energy → saturation) ──
    const hue = Math.round(em.spectralCentroid * 240);  // 0 (red/bass) → 240 (blue/highs)
    const saturation = Math.round((em.bandEnergy.mid + em.bandEnergy.high) * 50); // 0..50%
    const lightness = 50;
    const heatOpacity = 0.06 + em.bandEnergy.high * 0.12;
    const heatR = auraR * 0.75;
    const heatGrad = ctx.createRadialGradient(x, y, 0, x, y, heatR);
    heatGrad.addColorStop(0, `hsla(${hue},${saturation}%,${lightness}%,${heatOpacity})`);
    heatGrad.addColorStop(1, `hsla(${hue},${saturation}%,${lightness}%,0)`);
    ctx.beginPath();
    ctx.arc(x, y, heatR, 0, Math.PI * 2);
    ctx.fillStyle = heatGrad;
    ctx.fill();

    // ── Layer C: Transient rings — advance and draw active onset rings ──
    const rings = this._onsetRings.get(em.id);
    if (rings && rings.length > 0) {
      const dt = 0.016; // ~60fps nominal
      for (let i = rings.length - 1; i >= 0; i--) {
        const ring = rings[i];
        ring.r += 1.8;                  // expand ~108px/s at 60fps
        ring.opacity -= dt * 1.6;       // fade in ~625ms (fixed time constant)
        if (ring.opacity <= 0) {
          rings.splice(i, 1);
          continue;
        }
        ctx.beginPath();
        ctx.arc(x, y, ring.r + baseRadius * 0.4, 0, Math.PI * 2);
        ctx.strokeStyle = `rgba(255,255,255,${ring.opacity})`;
        ctx.lineWidth = 1.5;
        ctx.stroke();
      }
    }

    // ── Emitter dot (always drawn last so it sits on top of aura) ──
    ctx.beginPath();
    ctx.arc(x, y, 3.5, 0, Math.PI * 2);
    ctx.fillStyle = "rgba(255,255,255,0.95)";
    ctx.shadowColor = "rgba(255,255,255,0.5)";
    ctx.shadowBlur = 8;
    ctx.fill();
    ctx.shadowBlur = 0;
  }
}

window.AtmoOverlay = AtmoOverlay;
```

- [ ] **Step 3.2: Write self-test for projection logic**

Add at end of `atmo-overlay.js`:
```js
// Self-test
if (new URLSearchParams(location.search).has("selftest")) {
  function assert(cond, msg) { if (!cond) throw new Error("FAIL: " + msg); }

  // Test onset ring lifecycle
  const mockCanvas = document.createElement("canvas");
  mockCanvas.width = 200; mockCanvas.height = 200;
  const overlay = new AtmoOverlay(mockCanvas, mockCanvas, () => null, () => []);
  overlay.spawnOnsetRing(1, 0.8);
  const rings = overlay._onsetRings.get(1);
  assert(rings && rings.length === 1, "spawnOnsetRing creates ring");
  assert(Math.abs(rings[0].opacity - 0.64) < 0.01, "ring opacity = amplitude * 0.8");

  // Test layer toggle
  overlay.setLayerVisible("atmosphere", false);
  assert(overlay._layers.atmosphere === false, "layer toggle");

  overlay.dispose();
  console.log("[atmo-overlay] self-test PASS");
}
```

- [ ] **Step 3.3: Run self-test**

In browser: load `index_stage13.html?selftest`

Expected console: `[atmo-overlay] self-test PASS`

- [ ] **Step 3.4: Commit**

```bash
git add Source/ui/public/incremental/js/atmo-overlay.js
git commit -m "feat(viz): add AtmoOverlay 2D canvas renderer with layers A/B/C"
```

---

## Task 4: state-ring.js — SVG state ring per emitter

**Files:**
- Create: `Source/ui/public/incremental/js/state-ring.js`

Each emitter gets one SVG `<circle>` base ring drawn in an absolute-positioned SVG overlay. Colored quadrant segments are stubs (opacity 0) in Plan 1 — Plans 2/3 activate them.

- [ ] **Step 4.1: Write StateRingManager class**

```js
// state-ring.js
// Manages per-emitter SVG state rings. Each ring is a <circle> element
// in a shared <svg> overlay. In Plan 1, only the base dim ring is drawn.
// Plans 2/3 activate the four colored quadrant segments.
"use strict";

// Quadrant arc config — each segment is a stroke-dasharray on a circle (r=36, circumference=226.2)
const RING_R = 36;
const RING_C = 2 * Math.PI * RING_R; // 226.2
const SEG_LEN = RING_C / 4 - 4;     // ~52.6 (gap of 2px each side)
const SEG_GAP = RING_C - SEG_LEN;

// Quadrant offsets (stroke-dashoffset). SVG starts at 3 o'clock; we want 12 o'clock start.
// Rotate SVG -90deg to map 12 o'clock to start.
const SEGMENTS = {
  physics:       { color: "rgba(160,210,255,0.55)", offset: 0 },          // 12→3
  choreography:  { color: "rgba(120,220,150,0.55)", offset: -RING_C/4 },  // 3→6
  beat:          { color: "rgba(255,200,70,0.55)",  offset: -RING_C/2 },  // 6→9
  timeline:      { color: "rgba(200,150,255,0.5)",  offset: -3*RING_C/4 },// 9→12
};

const SVG_NS = "http://www.w3.org/2000/svg";
const SVG_SIZE = RING_R * 2 + 10; // 82px — ring + margin

class StateRingManager {
  /**
   * @param {HTMLElement} container - positioned ancestor of atmo-canvas (viewport-shell)
   */
  constructor(container) {
    this._container = container;
    this._rings = new Map(); // emitterId → { svg, base, segments }
    this._svg = this._createSVGLayer(container);
  }

  _createSVGLayer(container) {
    const svg = document.createElementNS(SVG_NS, "svg");
    svg.style.cssText = "position:absolute;inset:0;width:100%;height:100%;pointer-events:none;z-index:11;overflow:visible;";
    container.appendChild(svg);
    return svg;
  }

  /**
   * Update ring positions and segment visibility for all emitters.
   * Call each frame after AtmoOverlay._draw().
   * @param {Array} emitters - normalised emitter viz objects
   * @param {Function} project - (em) → {x,y} | null
   */
  update(emitters, project) {
    const activeIds = new Set(emitters.map(e => e.id));

    // Remove rings for departed emitters
    for (const [id, ring] of this._rings) {
      if (!activeIds.has(id)) {
        ring.g.remove();
        this._rings.delete(id);
      }
    }

    for (const em of emitters) {
      const pos = project(em);
      if (!pos) {
        if (this._rings.has(em.id)) this._rings.get(em.id).g.style.display = "none";
        continue;
      }

      if (!this._rings.has(em.id)) {
        this._rings.set(em.id, this._createRingGroup(em.id));
      }

      const ring = this._rings.get(em.id);
      ring.g.style.display = "";
      ring.g.setAttribute("transform", `translate(${pos.x},${pos.y})`);

      // Update segment visibility (Plan 1: all segments at opacity 0)
      this._updateSegments(ring, em);
    }
  }

  _createRingGroup(emitterId) {
    const g = document.createElementNS(SVG_NS, "g");

    // Base dim ring (always visible)
    const base = document.createElementNS(SVG_NS, "circle");
    base.setAttribute("r", RING_R);
    base.setAttribute("fill", "none");
    base.setAttribute("stroke", "rgba(255,255,255,0.05)");
    base.setAttribute("stroke-width", "1.5");
    g.appendChild(base);

    // Four quadrant segments (opacity 0 in Plan 1)
    const segs = {};
    for (const [key, cfg] of Object.entries(SEGMENTS)) {
      const seg = document.createElementNS(SVG_NS, "circle");
      seg.setAttribute("r", RING_R);
      seg.setAttribute("fill", "none");
      seg.setAttribute("stroke", cfg.color);
      seg.setAttribute("stroke-width", "2");
      seg.setAttribute("stroke-dasharray", `${SEG_LEN} ${SEG_GAP}`);
      seg.setAttribute("stroke-dashoffset", cfg.offset);
      seg.setAttribute("stroke-linecap", "round");
      seg.style.transform = "rotate(-90deg)";
      seg.style.transformOrigin = "0 0";
      seg.style.opacity = "0"; // activated in Plans 2/3
      g.appendChild(seg);
      segs[key] = seg;
    }

    this._svg.appendChild(g);
    return { g, base, segs };
  }

  _updateSegments(ring, em) {
    // Plan 1: all segments remain at 0 opacity (no physics/choro data yet)
    // Plans 2/3 will call ring.segs.physics.style.opacity = em.physicsActive ? "1" : "0"; etc.
    void ring; void em;
  }

  dispose() {
    this._svg.remove();
    this._rings.clear();
  }
}

window.StateRingManager = StateRingManager;
```

- [ ] **Step 4.2: Write self-test**

Add at end of `state-ring.js`:
```js
if (new URLSearchParams(location.search).has("selftest")) {
  function assert(cond, msg) { if (!cond) throw new Error("FAIL: " + msg); }
  const div = document.createElement("div");
  div.style.position = "relative";
  document.body.appendChild(div);

  const mgr = new StateRingManager(div);
  assert(div.querySelector("svg") !== null, "SVG overlay appended to container");

  const em = { id: 1, x: 0, y: 0, z: 0, physicsActive: false };
  mgr.update([em], () => ({ x: 100, y: 100 }));
  const g = div.querySelector("svg g");
  assert(g !== null, "ring group created for emitter");

  mgr.update([], () => null);
  assert(div.querySelector("svg g") === null, "ring group removed when emitter departs");

  mgr.dispose();
  console.log("[state-ring] self-test PASS");
}
```

- [ ] **Step 4.3: Run self-test and commit**

Load `index_stage13.html?selftest` — confirm `[state-ring] self-test PASS`.

```bash
git add Source/ui/public/incremental/js/state-ring.js
git commit -m "feat(viz): add StateRingManager SVG state ring with Plan 1 base ring"
```

---

## Task 5: selection-manager.js — click detection and Tier 1→2 transition

**Files:**
- Create: `Source/ui/public/incremental/js/selection-manager.js`

Click detection uses pointer distance to projected emitter positions (not Three.js raycasting — emitters are dots, not large meshes). Tier 2 state: selection ring SVG + HUD labels appended to viewport-shell.

- [ ] **Step 5.1: Write SelectionManager**

```js
// selection-manager.js
// Manages emitter selection state. Detects clicks on projected emitter dots,
// triggers Tier 1→2 transition: selection ring SVG + HUD label fade-in.
// Deselection: click canvas background, or click another emitter.
"use strict";

const HIT_RADIUS_PX = 18; // click within 18px of projected dot to select
const SVG_NS = "http://www.w3.org/2000/svg";
const DIM_OPACITY = "0.35";

class SelectionManager {
  /**
   * @param {HTMLElement} container - viewport-shell
   * @param {Function} getProjectedPositions - () → Map<emitterId, {x,y}>
   * @param {Function} getEmitters - () → Array of normalised emitter viz objects
   */
  constructor(container, getProjectedPositions, getEmitters) {
    this._container = container;
    this._getPos = getProjectedPositions;
    this._getEmitters = getEmitters;
    this._selectedId = null;
    this._selectionRing = null;
    this._hudEl = null;
    this._layers = { analytical: true };

    this._handleClick = this._handleClick.bind(this);
    container.addEventListener("click", this._handleClick);
  }

  setLayerVisible(layer, visible) {
    this._layers[layer] = visible;
    if (layer === "analytical" && this._hudEl) {
      this._hudEl.style.display = visible ? "" : "none";
    }
  }

  getSelectedId() { return this._selectedId; }

  _handleClick(ev) {
    // Ignore clicks on layer panel buttons
    if (ev.target.closest(".layer-panel")) return;

    const rect = this._container.getBoundingClientRect();
    const cx = ev.clientX - rect.left;
    const cy = ev.clientY - rect.top;
    const positions = this._getPos();
    let closest = null;
    let closestDist = HIT_RADIUS_PX;

    for (const [id, pos] of positions) {
      const d = Math.hypot(pos.x - cx, pos.y - cy);
      if (d < closestDist) { closestDist = d; closest = id; }
    }

    if (closest !== null) {
      this._select(closest);
    } else {
      this._deselect();
    }
  }

  _select(emitterId) {
    if (this._selectedId === emitterId) return;
    this._deselect();
    this._selectedId = emitterId;

    const pos = this._getPos().get(emitterId);
    if (!pos) return;

    // Dim unselected emitters via CSS class on container
    this._container.dataset.selectedEmitter = emitterId;

    // Selection ring (SVG, bounces in)
    this._selectionRing = this._createSelectionRing(pos);
    this._container.appendChild(this._selectionRing);

    // HUD labels (HTML, fades in with stagger)
    if (this._layers.analytical) {
      this._hudEl = this._createHUD(emitterId, pos);
      this._container.appendChild(this._hudEl);
    }
  }

  _deselect() {
    this._selectedId = null;
    delete this._container.dataset.selectedEmitter;
    if (this._selectionRing) { this._selectionRing.remove(); this._selectionRing = null; }
    if (this._hudEl) { this._hudEl.remove(); this._hudEl = null; }
  }

  /** Update selection ring + HUD position after emitter moves */
  update() {
    if (this._selectedId === null) return;
    const pos = this._getPos().get(this._selectedId);
    if (!pos) { this._deselect(); return; }
    if (this._selectionRing) {
      this._selectionRing.setAttribute("transform", `translate(${pos.x},${pos.y})`);
    }
    if (this._hudEl) {
      this._hudEl.style.left = pos.x + "px";
      this._hudEl.style.top = pos.y + "px";
    }
  }

  _createSelectionRing(pos) {
    // SVG fragment — ring bounce-animates in via scale() on the <g> wrapper.
    // Animating `r` on <circle> is NOT used — it is not supported as a CSS property
    // in WKWebView (the plugin runtime). scale() on <g> has universal support.
    const svg = document.createElementNS(SVG_NS, "svg");
    svg.style.cssText = "position:absolute;inset:0;width:100%;height:100%;pointer-events:none;z-index:15;overflow:visible;";

    const g = document.createElementNS(SVG_NS, "g");
    g.setAttribute("transform", `translate(${pos.x},${pos.y})`);
    // Bounce-in via scale on the group — overshoot then settle
    g.style.cssText = "animation: selRingIn 0.35s cubic-bezier(0.22,1,0.36,1) both; transform-origin: 0 0;";

    const ring = document.createElementNS(SVG_NS, "circle");
    ring.setAttribute("r", "48");
    ring.setAttribute("fill", "none");
    ring.setAttribute("stroke", "rgba(255,255,255,0.35)");
    ring.setAttribute("stroke-width", "1");
    g.appendChild(ring);
    svg.appendChild(g);

    // Inject keyframes if not already present
    if (!document.getElementById("sel-ring-kf")) {
      const style = document.createElement("style");
      style.id = "sel-ring-kf";
      style.textContent = `
        @keyframes selRingIn {
          0%   { opacity:0; transform:scale(0.6); }
          60%  { opacity:1; transform:scale(1.08); }
          80%  { transform:scale(0.96); }
          100% { opacity:1; transform:scale(1.0); }
        }
        @keyframes hudIn {
          0%   { opacity:0; transform:translateY(4px); }
          100% { opacity:1; transform:translateY(0); }
        }
      `;
      document.head.appendChild(style);
    }

    return svg;
  }

  _createHUD(emitterId, pos) {
    const emitters = this._getEmitters();
    const em = emitters.find(e => e.id === emitterId) || {};
    const spreadPct = Math.round((em.spread || 0) * 100);
    const gainDb = ((em.gain || 0.7) > 0 ? 20 * Math.log10(em.gain || 0.7) : -Infinity).toFixed(1);

    const hud = document.createElement("div");
    hud.style.cssText = `
      position:absolute; left:${pos.x}px; top:${pos.y}px;
      pointer-events:none; z-index:16; white-space:nowrap;
      font:9px/1 var(--font,monospace); color:rgba(255,255,255,0.7);
    `;
    hud.innerHTML = `
      <span style="position:absolute;top:-28px;left:12px;animation:hudIn 0.25s ease-out 0.2s both">
        <span style="color:rgba(255,255,255,0.35)">spread </span>${spreadPct}%
      </span>
      <span style="position:absolute;top:-14px;right:-56px;animation:hudIn 0.25s ease-out 0.22s both">
        <span style="color:rgba(255,255,255,0.35)">gain </span>${gainDb} dB
      </span>
    `;
    return hud;
  }

  dispose() {
    this._container.removeEventListener("click", this._handleClick);
    this._deselect();
  }
}

window.SelectionManager = SelectionManager;
```

- [ ] **Step 5.2: Add CSS for unselected emitter dimming**

In `index_stage13.html`, add:
```css
/* When an emitter is selected, dim the atmo-canvas
   (individual emitter dimming is handled in AtmoOverlay._drawEmitterAtmo) */
.viewport-shell[data-selected-emitter] #atmo-canvas {
  /* atmo-overlay.js checks selectedId and dims non-selected emitters inline */
}
```

Also update `AtmoOverlay._drawEmitterAtmo` to accept a `selectedId` parameter and reduce opacity for unselected emitters:

In `atmo-overlay.js`, update `_draw()`:
```js
_draw() {
  // ...existing code...
  const selectedId = this._getSelectedId ? this._getSelectedId() : null;
  for (const em of emitters) {
    const pos2d = this._project(em, camera, w, h);
    if (!pos2d) continue;
    const dimmed = selectedId !== null && em.id !== selectedId;
    this._drawEmitterAtmo(ctx, em, pos2d, w, h, dimmed);
  }
}
```

Update `_drawEmitterAtmo` signature:
```js
_drawEmitterAtmo(ctx, em, pos, w, h, dimmed = false) {
  ctx.save();
  if (dimmed) ctx.globalAlpha = 0.35;
  // ...existing drawing code...
  ctx.restore();
}
```

- [ ] **Step 5.2b: Write SelectionManager self-test**

Add at end of `selection-manager.js`:
```js
if (new URLSearchParams(location.search).has("selftest")) {
  function assert(cond, msg) { if (!cond) throw new Error("FAIL: " + msg); }

  const container = document.createElement("div");
  container.style.cssText = "position:relative;width:400px;height:300px;";
  document.body.appendChild(container);

  const positions = new Map([[1, { x: 100, y: 100 }], [2, { x: 300, y: 200 }]]);
  const emitters = [
    { id: 1, spread: 0.5, gain: 0.8 },
    { id: 2, spread: 0.3, gain: 0.6 },
  ];
  const mgr = new SelectionManager(container, () => positions, () => emitters);

  // Simulate click on emitter 1 (within HIT_RADIUS_PX)
  const rect = { left: 0, top: 0 };
  container.getBoundingClientRect = () => rect;
  container.dispatchEvent(Object.assign(new MouseEvent("click"), { clientX: 102, clientY: 101 }));
  assert(mgr.getSelectedId() === 1, "click within hit radius selects emitter");
  assert(container.dataset.selectedEmitter === "1", "dataset.selectedEmitter set");

  // Simulate click on emitter 2
  container.dispatchEvent(Object.assign(new MouseEvent("click"), { clientX: 300, clientY: 200 }));
  assert(mgr.getSelectedId() === 2, "clicking another emitter switches selection");

  // Simulate click on background (far from any emitter)
  container.dispatchEvent(Object.assign(new MouseEvent("click"), { clientX: 10, clientY: 10 }));
  assert(mgr.getSelectedId() === null, "click on background deselects");
  assert(!container.dataset.selectedEmitter, "dataset.selectedEmitter cleared on deselect");

  mgr.dispose();
  console.log("[selection-manager] self-test PASS");
}
```

- [ ] **Step 5.2c: Run self-test**

Load `index_stage13.html?selftest` — confirm `[selection-manager] self-test PASS`.

- [ ] **Step 5.3: Run visual integration test**

Load `index_stage13.html` in browser.
- Click on an emitter dot → selection ring should bounce in, HUD labels should fade up
- Click background → deselects, ring disappears in ~150ms
- Click second emitter → first deselects, second selects

- [ ] **Step 5.4: Commit**

```bash
git add Source/ui/public/incremental/js/selection-manager.js
git commit -m "feat(viz): add SelectionManager with Tier 1→2 transition and HUD labels"
```

---

## Task 6: stage13_ui.js — wire everything together

**Files:**
- Create: `Source/ui/public/incremental/js/stage13_ui.js`

Stage 13 is a thin wrapper that copies stage12's IIFE, adds script imports for the new modules, and wires `AtmoOverlay`, `StateRingManager`, and `SelectionManager` into the existing `createSceneApp` rAF loop.

- [ ] **Step 6.1: Write stage13_ui.js**

```js
// stage13_ui.js
// Stage 13 — Visualization Foundation.
// Layers AtmoOverlay, StateRingManager, and SelectionManager onto stage12.
// All stage12 logic is preserved via script tag; this file adds viz wiring only.
"use strict";

// Note: stage12_ui.js is NOT imported here — index_stage13.html loads it via its own
// <script defer> tag before this file. Both scripts run in the same window scope.
// This file patches the window after stage12 initialises.

(function () {
  // Defer until DOMContentLoaded (stage12 IIFE also defers to DOMContentLoaded)
  document.addEventListener("DOMContentLoaded", () => {
    // Give stage12 one tick to initialise
    setTimeout(initViz, 0);
  });

  // IMPORTANT: stage12's IIFE also registers a DOMContentLoaded listener.
  // With `defer`, both listeners are registered before the event fires and
  // both run synchronously in script order. The setTimeout(initViz, 0) defers
  // initViz to the next macrotask — after stage12's DOMContentLoaded handler
  // completes — so window.__locusq_sceneApp is guaranteed to be set when initViz
  // runs. Do NOT remove this setTimeout.
  function initViz() {
    const atmoCanvas = document.getElementById("atmo-canvas");
    const threeCanvas = document.getElementById("viewport-canvas");
    const viewportShell = threeCanvas ? threeCanvas.closest(".viewport-shell") : null;

    if (!atmoCanvas || !threeCanvas || !viewportShell) {
      console.warn("[stage13] required DOM elements missing — viz not initialised");
      return;
    }

    // Access the sceneApp and camera created by stage12.
    // stage12 exposes sceneApp on window for inter-stage use.
    const sceneApp = window.__locusq_sceneApp;
    if (!sceneApp) {
      console.warn("[stage13] sceneApp not exposed by stage12 — camera unavailable");
    }

    // Projected position cache (updated each frame)
    const projectedPositions = new Map();

    function getCamera() {
      return sceneApp ? sceneApp.getCamera() : null;
    }

    function getNormalisedEmitters() {
      const scene = window.__locusq_latestScene;
      if (!scene || !Array.isArray(scene.emitters)) return [];
      return scene.emitters.map(e => window.VizDataSchema.normaliseEmitterViz(e));
    }

    const overlay = new window.AtmoOverlay(
      atmoCanvas,
      threeCanvas,
      getCamera,
      getNormalisedEmitters
    );

    // Expose selectedId getter to overlay for dimming
    overlay._getSelectedId = () => selection.getSelectedId();

    const stateRings = new window.StateRingManager(viewportShell);
    const selection = new window.SelectionManager(
      viewportShell,
      () => projectedPositions,
      getNormalisedEmitters
    );

    // Patch AtmoOverlay._draw to also update state rings and selection
    const originalDraw = overlay._draw.bind(overlay);
    overlay._draw = function () {
      originalDraw();
      const emitters = getNormalisedEmitters();
      const camera = getCamera();
      if (camera && window.THREE) {
        const w = overlay._w, h = overlay._h;
        projectedPositions.clear();
        for (const em of emitters) {
          const pos = overlay._project(em, camera, w, h);
          if (pos) projectedPositions.set(em.id, pos);
        }
      }
      stateRings.update(getNormalisedEmitters(), em => projectedPositions.get(em.id) || null);
      selection.update();
    };

    // Consume onset events each scene update
    window.__locusq_onSceneUpdate = function (scene) {
      const emitters = getNormalisedEmitters();
      for (const em of emitters) {
        const onsets = window.VizDataSchema.consumeOnsetEvents(scene, em.id);
        for (const amplitude of onsets) {
          overlay.spawnOnsetRing(em.id, amplitude);
        }
      }
    };

    // Layer panel wiring
    document.querySelectorAll(".layer-btn").forEach(btn => {
      btn.addEventListener("click", () => {
        const layer = btn.dataset.layer;
        const nowActive = !btn.classList.contains("active");
        btn.classList.toggle("active", nowActive);
        overlay.setLayerVisible(layer, nowActive);
        selection.setLayerVisible(layer, nowActive);
      });
    });

    overlay.start();

    // Expose for stage14+ and testing
    window.__locusq_viz = { overlay, stateRings, selection };

    console.log("[stage13] visualization foundation initialised");
  }
})();
```

> **Note:** Stage12 must expose two globals for stage13 to use:
> - `window.__locusq_sceneApp` — the sceneApp object (add `sceneApp.getCamera = () => state.camera` and `window.__locusq_sceneApp = sceneApp` at the end of stage12's `createSceneApp` init)
> - `window.__locusq_latestScene` — set to `runtime.latestScene` each heartbeat (add `window.__locusq_latestScene = runtime.latestScene` in the heartbeat function)

- [ ] **Step 6.2: Patch stage12 to expose required globals**

**Patch 1 — expose `getCamera` on the returned sceneApp object.**

Find the `return {` block at the end of `createSceneApp` (after `init()` is called) and add `getCamera` to the returned object literal:
```js
return {
  resize,
  dispose,
  setPendingScene(payload) { state.pendingScene = payload; },
  setLane(lane, visible) { setLaneHighlight(lane, visible); },
  setViewPreset(preset) { applyViewPreset(preset); },
  getViewPreset() { return state.viewPreset; },
  getOrbitState() { return { theta: state.orbit.theta, phi: state.orbit.phi, radius: state.orbit.radius }; },
  getCamera() { return state.camera; },   // ← ADD THIS
};
```

**Patch 2 — expose sceneApp on window** (add after `const sceneApp = createSceneApp(...)`):
```js
window.__locusq_sceneApp = sceneApp;
```

**Patch 3 — fire onset/scene update hook from `updateSceneState`, not `heartbeat`.**

Find `function updateSceneState` (the function that receives new scene data from the C++ bridge, called at line ~4526 with `runtime.latestScene = data`). Add at the end of that function:
```js
window.__locusq_latestScene = runtime.latestScene;
if (window.__locusq_onSceneUpdate && runtime.latestScene) {
  window.__locusq_onSceneUpdate(runtime.latestScene);
}
```

> **Why `updateSceneState` and not `heartbeat`:** `heartbeat` is a 350ms liveness ping — it does not receive new scene payloads. `updateSceneState` is the handler that receives each new scene message from the C++ bridge. Hooking `heartbeat` would delay onset events by up to 350ms and miss events received between ticks.

- [ ] **Step 6.3: Load order — add stage13 script tags to index_stage13.html**

In `index_stage13.html`, update the `<head>` script block:
```html
<script src="/js/juce/check_native_interop.js?cb=13"></script>
<script defer src="/js/three.min.js"></script>
<script defer src="/incremental/js/viz-data-schema.js?cb=13"></script>
<script defer src="/incremental/js/atmo-overlay.js?cb=13"></script>
<script defer src="/incremental/js/state-ring.js?cb=13"></script>
<script defer src="/incremental/js/selection-manager.js?cb=13"></script>
<script defer src="/incremental/js/stage12_ui.js?cb=13"></script>
<script defer src="/incremental/js/stage13_ui.js?cb=13"></script>
```

Order matters: viz modules before stage12 (they register globals), stage12 before stage13.

- [ ] **Step 6.4: Integration smoke test**

Load `index_stage13.html` with the plugin connected or in preview mode:
- [ ] Emitter dots appear as white dots on the 3D scene
- [ ] Spread aura glows around each dot
- [ ] Aura breathes with simulated RMS (default 0.25 = subtle)
- [ ] Clicking a dot shows selection ring bounce-in + HUD labels (spread %, gain dB)
- [ ] ATM/ANA toggle buttons show/hide atmosphere and analytical layers
- [ ] Console shows `[stage13] visualization foundation initialised`
- [ ] No console errors

- [ ] **Step 6.5: Write and run stage13 self-test**

Add to `stage13_ui.js`:
```js
// Self-test registration
window.runIncrementalStage13SelfTest = async function () {
  const results = [];
  function check(cond, name) {
    results.push({ name, pass: !!cond });
    if (!cond) console.error("[stage13 self-test] FAIL:", name);
  }

  check(!!window.__locusq_viz, "viz system initialised");
  check(!!window.__locusq_viz.overlay, "AtmoOverlay present");
  check(!!window.__locusq_viz.stateRings, "StateRingManager present");
  check(!!window.__locusq_viz.selection, "SelectionManager present");

  // VizDataSchema smoke test
  const e = window.VizDataSchema.normaliseEmitterViz({});
  check(e.audioRms === 0.25, "VizDataSchema defaults applied");

  const passed = results.every(r => r.pass);
  window.__LQ_SELFTEST_RESULT__ = { status: passed ? "pass" : "fail", results };
  console.log(`[stage13 self-test] ${passed ? "PASS" : "FAIL"} (${results.filter(r=>r.pass).length}/${results.length})`);
  return passed;
};
```

Load `index_stage13.html?selftest` — confirm `[stage13 self-test] PASS`.

- [ ] **Step 6.6: Commit**

```bash
git add Source/ui/public/incremental/js/stage13_ui.js \
        Source/ui/public/incremental/js/stage12_ui.js
git commit -m "feat(viz): wire stage13 viz foundation — AtmoOverlay + StateRingManager + SelectionManager"
```

---

## Task 7: Layer panel and emitter dimming — integration polish

**Files:**
- Modify: `Source/ui/public/incremental/index_stage13.html` (CSS only)
- Modify: `Source/ui/public/incremental/js/atmo-overlay.js` (dim logic)

- [ ] **Step 7.1: Verify dimming in AtmoOverlay**

Confirm that `_drawEmitterAtmo(ctx, em, pos, w, h, dimmed=false)` sets `ctx.globalAlpha = 0.35` for non-selected emitters when any emitter is selected. Open browser, select an emitter, confirm unselected dots visibly dim.

- [ ] **Step 7.2: Layer panel — atmosphere toggle hides all aura layers**

Confirm clicking `ATM` hides spread aura, breathe, spectral heatmap, and rings. `ANA` toggle hides HUD labels only (selection ring stays — it's a selection affordance, not an analytical element).

- [ ] **Step 7.3: Final integration commit**

```bash
git add Source/ui/public/incremental/index_stage13.html \
        Source/ui/public/incremental/js/atmo-overlay.js
git commit -m "feat(viz): finalize layer panel toggles and emitter dimming on selection"
```

---

## Validation Status

- `tested` — self-tests for viz-data-schema, atmo-overlay, state-ring modules
- `partially tested` — stage13 integration (manual smoke test; no automated E2E)
- `not tested` — C++ audio bridge data fields (stubs only; wired in CL-P1)

---

## Follow-On Plans

| Plan | Gating dependency | Adds |
|---|---|---|
| Plan 2: Physics-reactive atmosphere | PhysicsWorker wired (physics impl plan P3) | Boids ring, attractor tint, collision flash, wall ripple; state ring physics (12→3) and beat (6→9) segments; audio trail (§3-D) and beat arc (§3-E) |
| Plan 3: Choreography-reactive atmosphere | ChoreographyWorker wired (CL-P2+) | All six choreography layers; state ring choreography (3→6) and timeline (9→12) segments; audio data bridge (CL-P1 AudioRingBuffer) replaces RMS stub |

---

## References

- Spec: `docs/superpowers/specs/2026-03-18-visualization-design.md`
- `Source/ui/public/incremental/index_stage12.html` — base HTML structure
- `Source/ui/public/incremental/js/stage12_ui.js` — runtime to patch
- `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md`
- `.ideas/choreography-lab-impl-plan.md` — CL-P1 AudioRingBuffer (unblocks audio data bridge)
