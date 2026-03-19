// selection-manager.js
// Manages emitter selection state. Detects clicks on projected emitter dots,
// triggers Tier 1→2 transition: selection ring SVG + HUD label fade-in.
// Deselection: click canvas background, or click another emitter.
"use strict";

const HIT_RADIUS_PX = 18; // click within 18px of projected dot to select
const SVG_NS = "http://www.w3.org/2000/svg";

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

    // Dim unselected emitters via dataset on container
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
      const g = this._selectionRing.querySelector("g");
      if (g) g.setAttribute("transform", `translate(${pos.x},${pos.y})`);
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
    const gain = Math.max(0, Number(em.gain) || 0.7);
    const gainDb = gain > 0 ? (20 * Math.log10(gain)).toFixed(1) : "-\u221e";

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

// Self-test
if (typeof location !== "undefined" && new URLSearchParams(location.search).has("selftest")) {
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
