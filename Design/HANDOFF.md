Title: LocusQ Design Handoff v3
Document Type: Design Handoff
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# LocusQ Design Handoff v3

## Finalized Version

- Approved design package: `v3`
- Primary spec: `Design/v3-ui-spec.md`
- Primary style guide: `Design/v3-style-guide.md`
- Browser preview (latest): `Design/index.html`
- Versioned preview snapshot: `Design/v3-test.html`

## Control Placement Summary

### Header

1. Logo (left)
2. Mode tabs: `Calibrate | Emitter | Renderer`
3. Scene status pill
4. Viewport continuity pill (`VIEWPORT LOCK`)
5. Room profile indicator
6. Draft/Final quality badge

### Main Area

1. Persistent 3D viewport (left, primary)
2. Mode-adaptive right control rail
3. Timeline strip (Emitter mode only)

### Rail Panels

- `calibrate`: speaker routing, mic, test type/level, capture meter, progress status.
- `emitter`: object identity/position/size/audio/physics/animation/presets.
- `renderer`: scene list, master/speaker controls, spatialization, room, global physics.

## Adaptive Rail Contract

- Width tokens:
  - Calibrate: `320px`
  - Emitter: `280px`
  - Renderer: `304px`
- Width transitions in `180ms` ease.
- Rail scroll position is remembered per mode.
- Panel switch changes controls only; viewport state persists.

## Typography + Color

- Font stack: `Inter` + system fallback.
- Base palette: dark neutral + gold accent (`#D4A847`).
- Status accents: success `#44AA66`, warning `#AA4444`.
- Emitter palette: v2 desaturated 16-color set retained.

## Implementation Notes

### Files Updated For Enforcement

- `Source/ui/public/index.html`
  - mode-aware rail width tokens
  - body mode classes
  - viewport lock indicator
  - rail width transition
- `Source/ui/public/js/index.js`
  - mode-shell application helper
  - per-mode rail scroll memory + restore
  - mode switch flow preserving viewport state

### Behavior Expectations

1. Mode change does not mutate camera orbit target/zoom.
2. Timeline remains emitter-only.
3. Status semantics remain mode-contextual and non-disruptive.
4. Draft/Final remains semantic state, not layout mode.

## QA Checks To Run During Impl/Test

1. Rapid mode switching (10+ toggles) keeps camera stable.
2. Rail scroll restore works per mode after deep scrolling.
3. Rail width transition does not clip controls.
4. Emitter timeline reappears with prior lane selection after mode return.

## Follow-on

- If additional rail density is needed, add per-mode collapsed section memory (next optional UI refinement) without altering viewport continuity invariants.
