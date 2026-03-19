Title: LocusQ UI/UX Design Review
Document Type: Design Review Report
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# LocusQ UI/UX Design Review

## Purpose

Assess the current plugin and companion UI.
Focus on trust, density, and scope control.

Companion docs:
- refinement addendum: `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`
- second opinion: `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`

Validation status: `not tested`

This is a design review.
It does not claim runtime or build validation.

## Review Status Legend

- `[KEEP]` keep and protect
- `[TIGHTEN]` keep, but reduce noise
- `[NEXT]` highest-value change
- `[DEFERRED]` useful later, not now

## Executive Call

LocusQ already has a strong product core.
The viewport-first shell, the three-mode model, and the companion trust workflow are real strengths.

The main design risk is not weak styling.
It is equal visual weight across too many truths.

The main corrective move is simple:
- make the primary task obvious,
- move diagnostics behind `Lab`,
- keep one dominant question per mode,
- and make the companion feel like a confidence tool before it feels like a console.

## What Works

### Plugin

- `[KEEP]` persistent viewport
  Reason: it makes the product feel like one instrument.
- `[KEEP]` `CALIBRATE`, `EMITTER`, `RENDERER`
  Reason: the mode names describe operator jobs.
- `[KEEP]` emitter timeline
  Reason: it gives LocusQ an authoring identity.
- `[KEEP]` studio-console visual direction
  Reason: the product already feels intentional.

### Companion

- `[KEEP]` readiness and send-gate model
  Reason: it implies a real workflow.
- `[KEEP]` pose visualization
  Reason: hidden sensor state becomes legible.
- `[KEEP]` profile acquisition flow
  Reason: capture, matching, privacy, and apply steps already form a credible tool.

## Pressure Points

| Area | Status | Why it matters |
|---|---|---|
| Header truth density | `[TIGHTEN]` | too many micro-statuses compete at once |
| `RENDERER` mode | `[NEXT]` | highest clutter risk; too much state-about-state |
| Companion first-run hierarchy | `[NEXT]` | telemetry appears before trust |
| Fallback and authority copy | `[NEXT]` | wording can still sound system-first instead of operator-first |
| Diagnostics exposure | `[TIGHTEN]` | rich diagnostics are good; permanent exposure is not |

## Density Snapshot

| Surface | Cards/Sections | Control Rows | Buttons | Selects | Toggles | Status Chips |
|---|---:|---:|---:|---:|---:|---:|
| LocusQ `CALIBRATE` rail | 5 | 16 | 8 | 7 | 0 | 10 |
| LocusQ `EMITTER` rail | 6 | 37 | 15 | 8 | 12 | 6 |
| LocusQ `RENDERER` rail | 9 | 29 | 11 | 7 | 20 | 14 |
| Companion dashboard | 5 telemetry sections | 73 metric rows | 11 capture buttons | n/a | 4 sync/axis buttons | 1 major status pill |

Interpretation:
- `EMITTER` is dense, but most of that density is productive authoring density.
- `RENDERER` is the real cleanup target.
- Companion telemetry is acceptable only if it stops leading the experience.

Visual:
- `Documentation/reports/visuals/ui-ux-review-2026-03-17/mode-density-metrics.svg`

## Design Rules

### 1. One Loud Question Per Mode

| Mode | Loud Question | Quiet By Default |
|---|---|---|
| `CALIBRATE` | Are we ready to measure and apply? | secondary routing detail |
| `EMITTER` | What is selected and how is it moving or sounding? | raw simulation internals |
| `RENDERER` | What is leaving the system right now? | parity and backend detail |

### 2. Keep One Global Truth In The Header

Recommended header order:
1. logo
2. mode tabs
3. session/trust pill
4. one structural trust badge
5. utility/settings

Move the rest into mode-local cards.

### 3. Collapse Diagnostics Harder

- keep one-line summaries visible,
- keep full diagnostics behind `Lab`,
- and never give diagnostics the same hierarchy as the primary action.

## Surface Calls

### Plugin

#### `RENDERER`

- `[NEXT]` merge trust into one top authority card
- `[NEXT]` always show:
  - requested path
  - active path
  - why this changed
  - control owner
- `[NEXT]` move parity counters, engine internals, and scene-monitor detail into `Lab`

#### `CALIBRATE`

- `[KEEP]` readiness framing
- `[TIGHTEN]` reduce secondary status clutter

#### `EMITTER`

- `[KEEP]` authoring-first identity
- `[TIGHTEN]` keep simulation and reactive detail inside existing drawers or overlays

### Companion

- `[NEXT]` default to `Focus`
- `[NEXT]` move dense telemetry to `Lab`
- `[KEEP]` keep privacy framing near capture
- `[KEEP]` keep profile acquisition as a guided operator flow

## Copy Rule

Use operator language first.

| Avoid | Prefer |
|---|---|
| `fallback_stage` | `Why this changed` |
| `authority` | `Control owner` |
| raw profile alias terms | `Requested Profile` / `Active Profile` |
| generic degraded-state jargon | plain-English reason text |

## Visual Aid Index

| Visual | Role |
|---|---|
| `Documentation/images/readme/locusq-state-calibrate.png` | current `CALIBRATE` shell reference |
| `Documentation/images/readme/locusq-state-emitter.png` | current `EMITTER` shell reference |
| `Documentation/images/readme/locusq-state-renderer.png` | current `RENDERER` shell reference |
| `Documentation/reports/visuals/ui-ux-review-2026-03-17/mode-density-metrics.svg` | density snapshot |
| `Documentation/reports/visuals/ui-ux-review-2026-03-17/scope-compass.svg` | scope tightening visual |
| `Documentation/reports/visuals/ui-ux-review-2026-03-17/plugin-wireframe.svg` | plugin shell direction |
| `Documentation/reports/visuals/ui-ux-review-2026-03-17/companion-wireframe.svg` | companion direction |

## Immediate Next Actions

1. Compress the `RENDERER` hierarchy.
2. Make the companion launch into `Focus`.
3. Normalize requested-versus-active copy.
4. Keep new experiments inside existing mode metaphors.

