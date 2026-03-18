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
})();
