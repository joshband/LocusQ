Title: LocusQ Style Guide v3
Document Type: Style Guide
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# LocusQ - Style Guide v3

**Theme:** Studio Instrument / Deterministic Spatial Console
**Design Focus:** Preserve spatial continuity while adapting control density by mode.

---

## Color System

Core palette remains from v2 for visual continuity.

- Background: `#0A0A0A`
- Surface: `#141414`
- Elevated Surface: `#1E1E1E`
- Border: `#2A2A2A`
- Gold Accent: `#D4A847`
- Primary Text: `#E0E0E0`
- Secondary Text: `#AAAAAA`
- Dim Text: `#666666`
- Success: `#44AA66`
- Warning: `#AA4444`

Emitter palette remains the v2 desaturated 16-color set.

---

## Typography

- Base stack: `'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif`
- Header logo: `14px`, weight `700`, letter spacing `2px`
- Mode/status pills: `9px`, weight `600`, uppercase, tracking `1.2px-1.5px`
- Rail labels: `10px-11px`, medium/high contrast hierarchy

---

## Adaptive Rail Tokens

```css
:root {
  --rail-width-calibrate: 320px;
  --rail-width-emitter: 280px;
  --rail-width-renderer: 304px;
  --rail-width: var(--rail-width-emitter);
}

body.mode-calibrate { --rail-width: var(--rail-width-calibrate); }
body.mode-emitter   { --rail-width: var(--rail-width-emitter); }
body.mode-renderer  { --rail-width: var(--rail-width-renderer); }
```

Rail width transition:

- Duration: `180ms`
- Curve: `ease`
- Constraint: no viewport jump or camera reset side effects

---

## Motion Rules

1. Mode switch motion is structural, not cinematic.
2. Allowed: rail width interpolation, timeline collapse/expand, status-pill crossfade.
3. Disallowed: camera tweening or recentering triggered by mode tab changes.
4. Keep total UI mode transition under `220ms` perceived latency.

---

## Status Semantics

- `STABLE`: neutral idle state.
- `PHYSICS`: dynamic motion active in emitter context.
- `MEASURING`: active calibration operation.
- `READY` / `PROFILE READY`: renderer/calibration completion state.
- `NO PROFILE`: calibrate warning state.

Status badge style is informational, not dominant; no full-screen alerts.

---

## Spatial Continuity Cues

1. Add persistent micro-badge (`VIEWPORT LOCK`) in header to communicate continuity model.
2. Keep world wireframe present across all modes.
3. Use mode-specific overlay contrast changes (not world replacement) to indicate context.

---

## Interaction Density by Mode

- Calibrate: highest setup density, wider rail.
- Emitter: performance manipulation density, compact rail.
- Renderer: monitoring/system density, medium rail.

The user should perceive one unified instrument with context rails, not three unrelated pages.

---

## QA Visual Checklist

- [x] No visible camera jump when switching tabs.
- [x] Rail width adjusts by mode and remains readable.
- [x] Timeline only appears in emitter mode.
- [x] Status and quality indicators remain stable under mode changes.
- [x] Viewport remains primary visual anchor at all times.
