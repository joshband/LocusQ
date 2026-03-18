Title: BL-093 Visual DNA Token Adoption and Hierarchy Polish
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# BL-093 Visual DNA Token Adoption and Hierarchy Polish

## Plain-Language Summary

BL-093 in plain terms: take the reviewed `Spatial Atelier / Diagnostic Calm` direction and make it real through design tokens, hierarchy polish, and consistent state styling across plugin and companion. Current state: Done. The 2026-03-17 second-opinion review added one hard prerequisite: reconcile token drift against the live HTML before the token packet is treated as canonical implementation guidance. That reconciliation is now live in the plugin shell, the companion shell, and the updated token packet, with an owner sync packet recorded for promotion and the archive/closeout sync now complete.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin and companion users, design implementers, and QA owners reviewing visual quality at shipping sizes. |
| What is changing? | The reviewed visual DNA becomes concrete semantic colors, typography, spacing, focus states, and hierarchy treatments in shipping UI surfaces. |
| Why is this important? | LocusQ already has a promising tone, but it needs a consistent design system so trust, readiness, and degraded states feel intentional instead of incidental. |
| How will we deliver it? | Adopt the 2026-03-17 token packet, map it to plugin and companion surfaces, and capture readable visual results at target sizes. |
| When is it done? | When plugin and companion share the same semantic state language and remain readable at `1280x720` and the repo's stated minimum sizes. |
| Where is the source of truth? | This runbook plus the design packet under `Documentation/reports/ui-ux-refinement-2026-03-17/`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of priority and sequencing. | `## Status Ledger` |
| Design token packet | Reference token and component guidance to reconcile before implementation. | `Documentation/reports/ui-ux-refinement-2026-03-17/design-tokens.json` |
| Prototype board (HTML) | Shows the intended polished direction. | `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/refinement-prototype.html` |
| Screenshot or capture bundle | Confirms the token system stays readable at target sizes after reconciliation. | `TestEvidence/bl093_visual_token_polish_20260318T042850Z/summary.md` |
| Owner sync packet | Records the promotion decision and final owner closeout checks. | `TestEvidence/bl093_owner_sync_z1_20260318T044057Z/promotion_decision.md` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-093 |
| Priority | P2 |
| Status | Done |
| Track | C - UX Authoring |
| Effort | Medium / M |
| Depends On | BL-089, BL-090, BL-091 |
| Blocks | — |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Adopt the reviewed visual DNA and token system without turning polish into a generic reskin.

Required outputs:

1. reconcile token drift between `design-tokens.json` and live HTML for:
   - background (`#06080B` vs `#0A0A0A`)
   - header height (`44px` vs `40px`)
   - primary font family (`Avenir Next` / `IBM Plex Sans` vs `Inter`)
   - rail width (`348px` vs `356px`)
2. semantic color system for:
   - live
   - ready
   - warn
   - error
   - inactive
3. consistent typography and tabular numeral behavior for metrics and statuses
4. spacing, radii, and card hierarchy alignment across plugin and companion
5. focus-visible, disabled, and degraded-state styling that remains readable under `WKWebView`, `WebView2`, and browser preview
6. one explicit source-of-truth decision for tokens versus live shell values before polish work is promoted

## Implementation Update (2026-03-18)

- `Documentation/reports/ui-ux-refinement-2026-03-17/design-tokens.json` is now the authoritative token packet after reconciliation, with the live shell updated to match the approved background, header height, typography, and rail-width decisions.
- `Source/ui/public/index.html` now reflects the reconciled plugin token layer and semantic state palette.
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift` now reflects the same semantic token system, typography split, and calmer card hierarchy.
- fresh validation captures are bundled under `TestEvidence/bl093_visual_token_polish_20260318T042850Z/`.
- owner sync packet for `Done-candidate` promotion is recorded under `TestEvidence/bl093_owner_sync_z1_20260318T044057Z/`.

## Source Inputs

- `Documentation/reports/ui-ux-refinement-2026-03-17/visual-dna.json`
- `Documentation/reports/ui-ux-refinement-2026-03-17/design-tokens.json`
- `Documentation/reports/ui-ux-refinement-2026-03-17/component-specs.md`
- `Documentation/reports/ui-ux-refinement-2026-03-17/integration-notes.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`
- `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/refinement-prototype.html`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/second-opinion-prototype.html`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`

## Acceptance IDs

- `BL093-A1` Semantic state colors are consistent across plugin and companion.
- `BL093-A2` Token drift between `design-tokens.json` and the live shell is reconciled or explicitly deferred with one documented source of truth.
- `BL093-A3` Visual hierarchy reinforces trust and readiness rather than competing with them.
- `BL093-A4` Typography, spacing, and card hierarchy align with the approved token packet after reconciliation.
- `BL093-A5` Minimum-size readability remains acceptable at reviewed plugin dimensions.
- `BL093-A6` Captured before/after evidence demonstrates polish without hierarchy regression.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Token truth reconciliation between design packet and live shell | `Documentation/reports/ui-ux-refinement-2026-03-17/design-tokens.json`, `Source/ui/public/index.html`, visual packet notes | token drift is resolved or one source is explicitly authoritative |
| B | Plugin token adoption and semantic state styling | plugin WebView shell | renderer, shell, and mode surfaces reflect approved tokens without hierarchy regression |
| C | Companion style alignment, readability pass, and capture refresh | companion UI shell + reports/visuals + evidence captures | Focus/Lab surfaces share semantic state language and before/after captures are archived |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL093-PLUGIN-BUILD | Automated | `cd Source/ui && npm run typecheck && npm run build` | exit 0 |
| BL093-COMPANION-BUILD | Automated | `cd companion && swift build && swift test` | exit 0 |
| BL093-VISUAL-REVIEW | Automated/manual | Playwright/capture review at target sizes | hierarchy and readability remain intact |
| BL093-DOCS | Automated | docs/backlog validation commands | exit 0 |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 1-3 | build + visual review | build logs + screenshots |
| Candidate intake | T2 | 5 or owner-approved equivalent | capture matrix across target sizes | replay summary + image bundle |
| Promotion | T3 | owner-approved equivalent | owner-reviewed polish packet | owner packet + visual evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-093 depends on hierarchy and trust work landing first. It is explicitly a polish lane, not a substitute for the P1 structural changes, and token drift must be reconciled before the design packet can act as canonical implementation guidance.

Reactive, simulation-driven, and temporal experiments remain bounded by `BL-094`; visual polish may clarify those surfaces, but it must not re-promote lab-scoped ideas into first-layer product scope.
