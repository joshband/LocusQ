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
    if (w === 0 || h === 0) return; // not yet laid out — ResizeObserver will fire again
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

    const selectedId = this._getSelectedId ? this._getSelectedId() : null;
    for (const em of emitters) {
      const pos2d = this._project(em, camera, w, h);
      if (!pos2d) continue;
      const dimmed = selectedId !== null && em.id !== selectedId;
      this._drawEmitterAtmo(ctx, em, pos2d, dimmed);
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

  _drawEmitterAtmo(ctx, em, pos, dimmed = false) {
    ctx.save();
    if (dimmed) ctx.globalAlpha = 0.35;

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

    ctx.restore();
  }
}

window.AtmoOverlay = AtmoOverlay;

// Self-test
if (typeof location !== "undefined" && new URLSearchParams(location.search).has("selftest")) {
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
