Title: BL-090 Plugin Authority-First Shell and Renderer Hierarchy
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# BL-090 Plugin Authority-First Shell and Renderer Hierarchy

## Plain-Language Summary

BL-090 in plain terms: tighten the LocusQ plugin shell so the viewport stays primary, the header stops shouting too many truths at once, and `RENDERER` becomes an authority-first trust surface instead of the busiest rail. Current state: Done. The 2026-03-17 second-opinion review sharpened this from a general clean-up into a more specific reduction, and that structure is now live in the shipping shell with fresh native and preview captures, an owner promotion packet, and a completed archive/closeout sync.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin operators, maintainers, QA/release owners, and design/implementation agents. |
| What is changing? | The plugin shell hierarchy: header status density, renderer top-card structure, and diagnostics exposure across modes. |
| Why is this important? | The current risk is not weak design language; it is equal visual weight across too many controls and statuses. |
| How will we deliver it? | Rebuild the shell hierarchy around one session pill, one trust badge, one top authority card, and collapsed diagnostics drawers. |
| When is it done? | When `RENDERER` answers “what is leaving the system right now?” faster than any diagnostics cluster competes with it. |
| Where is the source of truth? | This runbook, `Documentation/backlog/index.md`, the 2026-03-17 design reports, and repo-local visual evidence under `Documentation/reports/visuals/...` and `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of priority and dependencies. | `## Status Ledger` |
| Hierarchy rules | Makes the shell compression goals explicit. | `## Objective` |
| Refinement prototype board | Shows the intended authority-first direction. | `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/refinement-prototype.html` |
| Existing current-state capture | Anchors the delta against the current shell. | `Documentation/images/readme/locusq-state-renderer.png` |
| Trust-wave validation bundle | Fresh standalone and browser-preview captures of the compressed shell. | `TestEvidence/ui_ux_trust_wave_validation_20260318T023805Z/summary.md` |
| Owner sync packet | Records the promotion decision and final owner closeout checks. | `TestEvidence/ui_ux_trust_wave_owner_sync_z1_20260318T040618Z/promotion_decision.md` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-090 |
| Priority | P1 |
| Status | Done |
| Track | C - UX Authoring |
| Effort | Medium / M |
| Depends On | BL-040 (Done), BL-027 (Done), BL-074 (Done) |
| Blocks | BL-093 |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Implement an authority-first plugin shell with these hierarchy rules:

1. The viewport remains the primary spatial truth surface.
2. The boot shell remains intact as the startup trust surface; hierarchy work must not remove or genericize it.
3. The header shows:
   - brand
   - mode tabs
   - one session pill
   - one trust badge
   - one quality badge
4. Persistent header items that are not globally meaningful must move out:
   - `room-profile` leaves the header and belongs to `RENDERER`
   - `viewport-lock` leaves the header and becomes EMITTER-contextual
5. `RENDERER` gets one always-visible top authority card containing:
   - requested path
   - active path
   - fallback reason
   - control owner
6. Default-visible `RENDERER` surface is capped at four items:
   - authority card
   - spatialization card
   - room card
   - collapsed `Lab` drawer
7. Diagnostics, parity counters, and engine-internal cards move to drawers or `Lab` and default closed.
8. Each mode leads with one loud question:
   - `CALIBRATE`: are we ready to measure and apply?
   - `EMITTER`: what is selected and how is it moving or sounding?
   - `RENDERER`: what is leaving the system right now?
9. BL-089 operator-language rules must be reflected before promotion.

## Source Inputs

- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`
- `Documentation/reports/visuals/ui-ux-review-2026-03-17/plugin-wireframe.svg`
- `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/refinement-prototype.html`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/render-trust-ladder.svg`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/second-opinion-prototype.html`
- `Documentation/reports/ui-ux-refinement-2026-03-17/component-specs.md`
- `Documentation/reports/ui-ux-refinement-2026-03-17/design-tokens.json`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`

## Acceptance IDs

- `BL090-A1` The shell header contains at most one session pill, one persistent trust badge, and one quality badge.
- `BL090-A2` `room-profile` and `viewport-lock` are removed from the persistent header; `viewport-lock` is only shown in EMITTER context.
- `BL090-A3` `RENDERER` presents one always-visible top authority card and no more than four default-visible items overall.
- `BL090-A4` Renderer diagnostics, parity counters, and engine-internal cards are available but collapsed into `Lab` or secondary drawers by default.
- `BL090-A5` The boot shell and draft/final quality badge remain intact as intentional product signals.
- `BL090-A6` Browser preview, runtime WebView, and captured screenshots preserve the same hierarchy.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Header compression while preserving boot shell and quality badge | `Source/ui/public/index.html`, `Source/ui/src/index.ts` | header obeys session-pill + trust-badge + quality-badge rule and removes non-global badges |
| B | Renderer authority card and default-visible reduction to four items | plugin WebView shell + supporting scene-state UI plumbing | top authority card becomes the loudest renderer truth and default `RENDERER` count is capped |
| C | Cross-mode hierarchy cleanup, contextual EMITTER viewport-lock placement, and visual capture refresh | plugin shell + report visuals | all three modes reflect the one-question rule, EMITTER owns viewport-lock contextually, and fresh captures are archived |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL090-UI-TYPECHECK | Automated | `cd Source/ui && npm run typecheck` | exit 0 |
| BL090-UI-BUILD | Automated | `cd Source/ui && npm run build` | exit 0 |
| BL090-STANDALONE | Automated | representative standalone/WebView selftest | no regression in boot/degraded behavior |
| BL090-VISUAL-CAPTURE | Automated/manual | Playwright or capture harness screenshot refresh | captured shell matches accepted hierarchy |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 1-3 | typecheck/build + focused visual review | build logs + preview captures |
| Candidate intake | T2 | 5 or owner-approved equivalent | selftest + visual capture matrix | replay summary + screenshot bundle |
| Promotion | T3 | 10 or owner-approved equivalent | owner-reviewed shell hierarchy packet | owner packet + deterministic evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

This runbook implements the plugin-side hierarchy recommendations from the 2026-03-17 design review, refinement pass, and second-opinion report; it does not redefine the global backlog lifecycle contract.

Reactive, simulation-driven, and temporal follow-on work inside the plugin shell is now governed by `BL-094`, which requires those expansions to extend existing cards, overlays, or lab drawers before any new top-level surface is proposed.
