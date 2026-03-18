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

class StateRingManager {
  /**
   * @param {HTMLElement} container - positioned ancestor of atmo-canvas (viewport-shell)
   */
  constructor(container) {
    this._container = container;
    this._rings = new Map(); // emitterId → { g, base, segs }
    this._svg = this._createSVGLayer(container);
  }

  _createSVGLayer(container) {
    const svg = document.createElementNS(SVG_NS, "svg");
    svg.style.cssText = "position:absolute;inset:0;z-index:11;pointer-events:none;overflow:visible;";
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

    // Remove rings for departed emitters (collect first, delete after — safe for JavaScriptCore)
    const departed = [];
    for (const [id] of this._rings) {
      if (!activeIds.has(id)) departed.push(id);
    }
    for (const id of departed) {
      this._rings.get(id).g.remove();
      this._rings.delete(id);
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

  _createRingGroup(_emitterId) {
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

// Self-test
if (typeof location !== "undefined" && new URLSearchParams(location.search).has("selftest")) {
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
