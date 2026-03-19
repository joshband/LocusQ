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

// Self-test (runs only when URL contains ?selftest)
if (typeof location !== "undefined" && new URLSearchParams(location.search).has("selftest")) {
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
