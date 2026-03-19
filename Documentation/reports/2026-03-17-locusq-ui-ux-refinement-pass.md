Title: LocusQ UI/UX Refinement Pass
Document Type: Design Review Addendum
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# LocusQ UI/UX Refinement Pass

## Purpose

Turn the base review into a shipping-oriented refinement packet.

Base review:
- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`

Validation status: `not tested`

This is a design addendum.
It does not claim runtime, build, or listening validation.

## Priority Snapshot

| Area | Status | Priority | Decision |
|---|---|---|---|
| Plugin viewport-first shell | `[KEEP]` | high | protect it |
| Renderer hierarchy | `[NEXT]` | highest | compress to trust-first flow |
| Companion first-run flow | `[NEXT]` | highest | launch into `Focus` |
| Requested vs active language | `[NEXT]` | high | make truth language explicit |
| Cross-format UI parity | `[KEEP]` | high | one product, same information architecture |
| Reactive/simulation visuals | `[TIGHTEN]` | medium | keep as contained overlays or lab detail |
| Temporal/simulation expansion | `[DEFERRED]` | medium | do not create a new top-level surface |

## Refined Thesis

The main product question is:

How quickly can LocusQ tell the operator what is true, what changed, and what still needs action?

That leads to a simple role split:
- plugin = scene truth
- companion = device trust and profile acquisition
- `Lab` = evidence, parity, and experiments

If a feature does not strengthen one of those jobs, it is scope debt.

## Visual Direction

Recommended direction: `On-brand`

Working style:
- `Spatial Atelier / Diagnostic Calm`

Why:
- keeps the boutique studio tone,
- improves live/degraded state clarity,
- supports dense spatial tooling without becoming a science dashboard,
- and stays plausible across WebView, AU, VST3, CLAP, and AUv3.

## Plugin Tightening

### `RENDERER` Must Become The Trust Surface

Keep visible:
- requested path
- active path
- fallback reason
- control owner

Move behind `Lab`:
- parity counters
- engine internals
- secondary summaries

### Header Must Carry Fewer Truths

Keep only:
- one session pill
- one structural trust badge

Move the rest into mode-local cards.

### Experiments Must Stay Inside Existing Metaphors

Good:
- new motion families inside `EMITTER`
- temporal behavior inside the existing timeline strip
- optional viewport overlays

Bad:
- a new top-level `SIMULATION` mode
- a second diagnostic viewport
- format-specific pages for backend internals

## Companion Tightening

### Default To `Focus`

`Focus` should answer:
1. is the device supported and available?
2. is streaming safe to start?
3. do I need to center or sync?
4. which profile is active?
5. what capture step is next?

### Keep `Lab`, But Contain It

`Lab` still matters for:
- readiness gating,
- stale-pose behavior,
- axis sanity,
- packet age,
- matcher confidence.

It just should not lead.

### Keep Privacy Near Capture

The capture flow should stay explicit about:
- local processing,
- no implicit upload,
- and clear fallback when confidence is insufficient.

## Truth Language Rule

Never show a personalized or advanced render claim without also showing whether it is actually active.

Minimum shipping contract:

| Field | Visible? | Why |
|---|---|---|
| Requested | yes | shows operator intent |
| Active | yes | shows runtime truth |
| Why this changed | yes when degraded | keeps fallback honest |
| Profile source | yes | distinguishes local/device/generic paths |
| Listening evidence | lab only | useful, but not headline UI |

## Visual Aid Index

| Visual | Role |
|---|---|
| `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/trust-state-ladder.svg` | trust hierarchy |
| `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/focus-lab-operating-model.svg` | companion mode split |
| `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/format-runtime-parity.svg` | parity framing |
| `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/refinement-prototype.html` | interactive prototype reference |
| `Documentation/reports/ui-ux-refinement-2026-03-17/design-tokens.json` | token reference |
| `Documentation/reports/ui-ux-refinement-2026-03-17/visual-dna.json` | visual DNA reference |
| `Documentation/reports/ui-ux-refinement-2026-03-17/component-specs.md` | component-level reference |

## Recommended Order

1. Tighten `RENDERER`.
2. Make companion `Focus` first.
3. Normalize requested/active/fallback language.
4. Hold reactive, simulation, and temporal expansion inside current metaphors.

