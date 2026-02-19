Title: LocusQ UI Specification v3
Document Type: UI Specification
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# LocusQ - UI Specification v3

**Window:** Responsive desktop baseline (1200 x 800 reference)
**Framework:** WebView + Three.js
**Primary Objective:** Enforce persistent viewport continuity and adaptive right control rail per mode.

---

## Scope

This version is a targeted design hardening pass for:

1. Persistent viewport behavior across Calibrate, Emitter, and Renderer modes.
2. Adaptive mode-specific control rail behavior without scene/camera discontinuity.
3. Draft/Final visual semantics that do not change layout meaning.

This does not introduce new DSP features.

---

## Continuity Contract (Viewport)

### Required Invariants

1. Camera orbit state (`theta`, `phi`, `radius`) is retained when switching modes.
2. Orbit target and user camera navigation context are retained when switching modes.
3. Viewpoint shortcut selection (Perspective/Top/Front/Side) remains stable until user changes it.
4. Mode switch must not recreate or re-seed the 3D world; only mode overlays and object visibility states may change.
5. Timeline lane highlight geometry is mode-scoped:
   - Emitter mode: visible by selected lane.
   - Calibrate/Renderer: hidden.

### Explicit Non-Goals

1. No camera auto-jump on mode change.
2. No mode-specific camera presets applied automatically.
3. No scene-clear/fade-to-black transition between mode tabs.

---

## Adaptive Rail Contract (Per Mode)

Single right rail container with mode-specific panel/content and width profile.

### Rail Width Tokens

- `--rail-width-calibrate`: `320px`
- `--rail-width-emitter`: `280px`
- `--rail-width-renderer`: `304px`

### Rail Behavior Rules

1. Rail content panel changes with mode (`data-panel="calibrate|emitter|renderer"`).
2. Rail width animates smoothly (`~180ms`) when mode changes.
3. Rail scroll position is remembered per mode and restored when returning to that mode.
4. Panel swap must not trigger viewport reset or camera state mutation.
5. Timeline visibility remains mode-dependent:
   - Emitter: visible.
   - Calibrate/Renderer: collapsed.

---

## Mode Matrix

| Mode | Viewport World | Timeline | Rail Panel | Rail Width | Scene Status |
|---|---|---|---|---|---|
| Calibrate | Persistent world, calibration overlays active | Hidden | Calibration setup + capture status | 320px | `NO PROFILE` / `MEASURING` / `PROFILE READY` |
| Emitter | Persistent world, selection ring + lane highlights | Visible | Object, audio, physics, animation, presets | 280px | `STABLE` / `PHYSICS` |
| Renderer | Persistent world, aggregate monitoring | Hidden | Scene list, master/speakers/spatial/room/global physics | 304px | `READY` |

---

## Interaction Sequence (Mode Switch)

On mode switch event:

1. Save outgoing rail scroll position to mode-keyed cache.
2. Activate new mode tab and panel.
3. Update document mode class (`mode-calibrate`, `mode-emitter`, `mode-renderer`).
4. Restore incoming rail scroll from cache.
5. Toggle timeline visibility based on mode.
6. Update scene-status badge and mode-scoped viewport overlays.
7. Recompute layout (`resize`) without mutating camera orbit state.

---

## Control Mapping Notes

The adaptive rail uses existing parameter/command controls (no parameter ID changes):

- Mode: `mode`
- Animation source: `anim_mode`
- Animation toggles: `anim_enable`, `anim_loop`, `anim_sync`
- Renderer quality badge: `rend_quality`
- Physics toggle path: `phys_enable` (+ preset UX)

---

## Acceptance Checklist

- [x] Switching mode does not reset camera orbit or zoom.
- [x] Switching mode swaps only overlays/rail/timeline visibility.
- [x] Rail width adapts per mode using design tokens.
- [x] Rail remembers per-mode scroll state.
- [x] Emitter timeline remains available only in Emitter mode.
- [x] Scene status semantics are mode-consistent and non-disruptive.
- [x] Draft/Final badge toggles quality semantics without layout shift.
- [x] Design preview and production WebView shell remain aligned.

---

## v2 -> v3 Delta

1. Promoted viewport persistence from implicit behavior to explicit invariants.
2. Added mode-tokenized adaptive rail widths.
3. Added per-mode rail scroll memory requirement.
4. Added deterministic mode-switch sequence contract for implementation handoff.
