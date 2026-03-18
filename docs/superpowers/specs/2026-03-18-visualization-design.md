---
Title: LocusQ Visualization Design Spec
Document Type: Design Spec
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# LocusQ Visualization Design Spec

## Overview

This spec defines the complete visual language for the LocusQ plugin viewport — covering the tier system, atmosphere layers, physics/choreography/audio reactive states, multi-instance sharing, the emitter state ring, and the selection transition. All elements are designed for the WebView viewport using SVG/Canvas/CSS animation.

The governing principle: **atmosphere is the primary face of the plugin. Analytical information is a precision layer that surfaces only on selection.**

---

## 1. Tier System

The viewport operates in two tiers per emitter at all times.

### Tier 1 — Default (all unselected emitters)
- Emitter dot
- Spread aura
- Audio-reactive atmosphere layers (A, B, C — see §3)
- Physics-reactive atmosphere layers (core: A, B, C, E when active; optional: D, F — see §4)
- Choreography-reactive atmosphere layers (all six when active — see §5)
- Emitter state ring (see §6)
- No analytical elements visible

### Tier 2 — Selected (one emitter at a time)
Tier 1 elements remain, plus:
- Selection ring
- HUD overlay: spread %, gain dB, active modes
- Force vectors (gravity, velocity, attractor)
- Directivity lobe
- Boids neighborhood ring (analytical detail)
- Collision halo
- Spring coil indicator (when spring active)
- Attractor crosshair

Unselected emitters dim to 35% opacity when any emitter is selected.

---

## 2. Multi-Instance Shared Viewport

When multiple LocusQ instances are active on different tracks, each instance's viewport shows ghost representations of emitters from other tracks.

### Ghost emitter form
- **Shape:** spread ring only — no solid dot anchor
- **Color:** per-track unique hue (user-assignable or auto-assigned)
- **Opacity:** ~20–25% ring opacity; faint track-ID label above ring
- A global toggle (layer panel) shows/hides all ghost emitters

### Ghost emitter interaction
| Interaction | Behavior |
|---|---|
| Hover | Tooltip: track name + emitter ID |
| Single click | Ring pulses + brief highlight |
| Double-click | Raise focus to that track's plugin window |

### Color language — track identity
Track color is assigned per-instance and applies consistently to ghost rings, automation lane indicators, and (future) cross-instance influence lines. Current track emitters remain full-brightness white dots.

**Auto-assignment:** instances are assigned a hue round-robin from a fixed 6-color palette in DAW track order: `#5b8fff` (blue), `#ff7b5b` (coral), `#5bffb8` (teal), `#ffcc44` (yellow), `#cc5bff` (purple), `#ff5b8f` (pink). User-assigned colors override the auto value and persist in plugin state.

---

## 3. Audio-Reactive Atmosphere

These layers respond to the audio signal of the emitter's track. Three are always-on; two are feature-gated; one is opt-in.

| Layer | Name | Mapping | Activation |
|---|---|---|---|
| A | Breathe | RMS → aura radius + opacity | Always on |
| B | Spectral heatmap | Spectral centroid → hue; band energy → saturation | Always on |
| C | Transient rings | Onset detection → concentric ring spawn; amplitude → opacity | Always on |
| D | Motion trail | Emitter velocity → trail length; RMS → opacity | Feature-gated: physics/timeline active |
| E | Beat phase arc | DAW BPM → arc rotation speed; downbeat → radial bloom | Feature-gated: beat-sync active |
| F | Energy cloud | Spectral flux → particle density; RMS → orbit radius | Opt-in via layer panel |

A, B, and C operate on different timescales (macro envelope, mid-rate tonal, fast transient) and do not compete visually.

---

## 4. Physics-Reactive Atmosphere

These layers express active physics simulation states through atmosphere — no analytical elements. All are feature-gated.

| Layer | Name | Trigger | Character |
|---|---|---|---|
| A | Boids neighborhood ring | Boids mode active | Soft ring at perception radius; neighbor emitters as tiny orbiting dots at actual angles |
| B | Attractor field tint | Attractor force > threshold | Aura blooms violet `rgba(180,100,255)`; dot hue shifts toward that same violet; strength = normalized force magnitude |
| C | Collision flash | Emitter–emitter collision | Radial burst ~120ms; dot momentarily swells; intensity = collision impulse |
| D | Spring coil | Spring constraint active | Two nested rings oscillate at spring ω; dot micro-oscillates (optional) |
| E | Wall bounce ripple | Wall collision | Ripple from contact point; wall briefly glows ~200ms; intensity = velocity at impact |
| F | Drag smear | Drag > 0.4 + velocity > threshold | Aura stretches asymmetrically in direction of motion (optional) |

**Core set:** A + B + C + E
**Optional:** D + F

### Conflict resolution
- B (attractor tint) overrides spectral heatmap hue when attractor force is strong
- C (collision flash) takes priority over E (wall ripple) if both fire simultaneously
- §3-D (motion trail) and §4-F (drag smear) share the smear channel — use whichever is stronger

---

## 5. Choreography-Reactive Atmosphere

These layers express active Choreography Lab states. All are feature-gated; all six are enabled.

| Layer | Name | Trigger | Color | Character |
|---|---|---|---|---|
| A | Formation lines | Formation pattern active | Green | Faint lines to sibling emitters at actual angles; soft ring marks formation boundary; dot tints green |
| B | Procedural path trail | Path running | Blue | Ghost dots trail behind; dot tints blue; trail length = path speed |
| C | Beat-sync arc | Beat-sync mode active | Amber | Outer arc = 1 beat; inner arc = ½ beat; downbeat radial bloom |
| D | Pattern morph crossfade | Morph in progress | Violet→cyan | Two rings crossfade between outgoing/incoming pattern colors; dot hue interpolates |
| E | Audio deviation trail | Audio-reactive deviation active | Orange | Trail color shifts orange; aura pulses; intensity = deviation magnitude |
| F | Bake preview ghost | Bake preview mode | Dim white | Hollow ghost dot traces projected path; breadcrumb dots mark path; disappears on bake complete |

### Color language — mode identity
| Color | Mode |
|---|---|
| Green `rgba(120,220,150)` | Formation lock (matches §6 Choreography quadrant color) |
| Blue `rgba(100,160,255)` | Procedural path (distinct from §6 physics blue which is `rgba(160,210,255)` — lower saturation, lighter value) |
| Amber | Beat-sync |
| Violet→cyan | Pattern morph transition |
| Orange | Audio-reactive deviation |
| Dim white | Bake preview ghost (§5 layer F only) |
| Violet pulse | Attractor force (physics — see §4 layer B) |

Note: multi-instance ghost emitters use per-track unique hues (not dim white) — see §2.

---

## 6. Emitter State Ring

A single SVG ring sits just outside the spread aura, between the atmosphere and any outer effects. It is always a complete circle; modes add luminance to their quadrant — inactive quadrants revert to dim base (~5% opacity), never go dark.

### Quadrant map

| Quadrant | Mode | Color | Animation |
|---|---|---|---|
| 12 o'clock → 3 | Physics | Blue `rgba(160,210,255)` | Breathes with simulation activity |
| 3 → 6 | Choreography | Green `rgba(120,220,150)` | Breathes with pattern state |
| 6 → 9 | Beat-sync | Amber `rgba(255,200,70)` | Sweep dot rotates at tempo; arc pulses on beat |
| 9 → 12 | Timeline | Violet `rgba(200,150,255)` | Steady; flashes on keyframe hit |

### Rules
- The ring is always thinner than the atmosphere layers so dot + atmosphere remain the dominant read
- A segment appears only when its mode is active
- The beat sweep dot (6→9 quadrant) is the only element on the ring that moves; all other animations are opacity/brightness only
- On Tier 2 selection, ring segments brighten slightly but do not transform or change character
- Base state (no modes active): full ring at ~5% opacity — boundary marker only

---

## 7. Selection Transition

### Timing

| Phase | Window | Events |
|---|---|---|
| Response | 0–80ms | Dot brightens + swells to 1.4×; aura begins expanding; other emitters begin dimming |
| Ring | 80–200ms | Selection ring bounce-in (overshoot + settle at cubic-bezier(0.22,1,0.36,1)); dot settles; aura settles |
| Analytical | 150–350ms | Directivity lobe, collision halo, force vectors fade in; HUD labels fade up with 4px drift; state ring brightens |
| Deselect | 150ms | Analytical layers fade out; ring shrinks out; others restore to full opacity; dot/aura return to Tier 1 |

### Rules
- Ring arrives first — it anchors the selection read before analytical information appears
- Analytical fade-in may begin at 150ms and overlap with the ring settle phase (150–200ms); the ring does not need to fully complete before analytical elements appear
- HUD labels stagger in with 20ms offsets to avoid simultaneous pop
- Deselect is intentionally fast (150ms) — no lingering chrome
- Only one emitter can be in Tier 2 at a time; clicking a second emitter deselects the current one first (crossfade ~80ms)

---

## 8. Layer Panel

A global toggle panel (accessible via the viewport header) controls:

| Toggle | Controls |
|---|---|
| Ghost emitters | Show/hide all cross-instance ghost rings |
| Analytical layer | Show/hide Tier 2 analytical elements globally |
| Atmosphere layers | Individual toggles for each audio-reactive layer |
| Physics atmosphere | Individual toggles for physics layers |
| Choreography atmosphere | Individual toggles for choreography layers |

Feature-gated layers (D/E audio, all physics, all choreography) are not shown in the layer panel unless their corresponding mode is enabled in the PHYSICS or CHOREOGRAPHY panel.

---

## 9. Implementation Notes

### Rendering approach
- WebView-based Canvas/SVG — no JUCE/Visage direct rendering for atmosphere
- Atmosphere layers drawn on Canvas; state ring and selection ring drawn as SVG
- HUD labels as positioned HTML elements (CSS animation for entry)
- Physics/choreography state data flows from the audio thread via the existing WebView bridge (lock-free message queue)

### Data sources per layer

| Layer | Data source | Update rate |
|---|---|---|
| Aura breathe | RMS from AudioRingBuffer | ~60fps smoothed |
| Spectral heatmap | Per-band energy from AudioRingBuffer | ~30fps |
| Transient rings | Onset events from ChoreographyWorker (AudioRingBuffer consumer) — single amplitude value per event; ring opacity decays by fixed time constant on renderer side, no per-frame data needed | Event-driven |
| State ring segments | Per-quadrant: Physics (12–3) from PhysicsWorker mode flags; Choreography (3–6) from ChoreographyWorker pattern state; Beat-sync (6–9) from DAW transport beat phase; Timeline (9–12) from Timeline track active flag + keyframe events. All delivered through the combined worker tick message queue. | On change |
| Beat sweep dot | DAW transport BPM + beat phase | ~60fps |
| Formation lines | ChoreographyWorker emitter positions | ~60fps |
| Ghost emitters | TBD — cross-instance IPC mechanism unspecified; see §10 | ~30fps (target) |

### Performance budget
- All Tier 1 atmosphere: target < 1ms/frame GPU composite (Chrome/WKWebView)
- Particle cloud (F) is opt-in specifically because it exceeds budget at high emitter counts
- Ghost emitters are culled at > 8 per viewport to prevent overdraw

---

## 10. Open Items

- Cross-instance influence lines (bidirectional boids/attractor connections between ghost emitters) — deferred to Choreography Lab CL-P6
- Cross-instance IPC mechanism — IPC contract for delivering ghost emitter positions from other instances is unspecified; must be resolved before §9 ghost emitter data source can be implemented
- Spatial feedback loop visualization (physics-to-audio response indicator) — deferred to physics-reactive-audio skill scope
- Timeline scrub preview — handled by bake-preview ghost (§5 layer F)
- Track color assignment UX — deferred to EMITTER panel design phase

---

## References

- `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md`
- `.ideas/choreography-lab-spec.md`
- `.ideas/physics-simulation-spec.md`
- `.ideas/timeline-spec.md`
- `Documentation/invariants.md`
