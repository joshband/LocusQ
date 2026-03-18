Title: LocusQ UI/UX Second-Opinion Review
Document Type: Design Review Report
Author: Claude Sonnet 4.6 (second-opinion pass)
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# LocusQ UI/UX Second-Opinion Review

## Purpose

Stress-test the first design review and refine it where it was too soft or too vague.

Inputs, read as inputs and not as authority:
- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`

Validation status: `not tested`

This is a design review.
It does not claim builds, tests, or runtime validation.

## Executive Call

The first review was directionally right.
It correctly identified density in `RENDERER`, header truth overload, and the need for companion `Focus` versus `Lab`.

Where it was weak:
- it sometimes said "tighten" when it should have said "remove",
- it did not name enough concrete UI removals,
- and it missed a few source-visible risks.

## Strong Agreements

| Recommendation | Agreement | Why |
|---|---|---|
| Focus/Lab split for companion | strong | highest-value change in the product |
| Authority card at top of `RENDERER` | strong | requested/active/why/owner must be obvious |
| One loud question per mode | strong | best way to keep the shell disciplined |
| Lab drawer for diagnostics | strong | diagnostics remain useful without leading |
| No new top-level mode for experiments | strong | scope guardrail |
| Honest cross-format messaging | strong | one product, not multiple UI worldviews |

## Concrete Corrections

### 1. Axis Flip Belongs In `Focus`

The earlier review treated axis flip like expert-only detail.
That is too aggressive.

Reason:
- a wrong axis breaks first-run trust,
- so correction controls must be reachable without entering `Lab`.

Call:
- keep axis orientation controls in `Focus`,
- but present them compactly.

### 2. Keep The Quality Badge

The header quality badge is not noise.
It signals draft versus final scene intent.

Call:
- keep it,
- and let it be visually stronger in `draft` than in `final`.

### 3. Surface Synthetic Mode In `Focus`

This was the biggest miss.

If the companion is running in `synthetic` mode, the operator must see that immediately.
Otherwise a fake motion feed can look like a working device.

Call:
- show a clear `synthetic mode` warning in `Focus`.

### 4. Make `RENDERER` Cuts Specific

The earlier review said "merge."
That was not specific enough.

Default-visible target:
1. Authority card
2. Spatialization card
3. Room card
4. collapsed `Lab`

Remove from default:
- Scene Monitor
- parity counters
- secondary routing summary
- backend and engine internals

### 5. Put Stale Pose Reason In `Why Changed`

If active render output degraded because pose went stale, the authority surface should say so plainly.

Example:
- `No pose received for 2.4 seconds`

That belongs in the top-level truth surface, not only in `Lab`.

### 6. Add `Plugin Ack` To The Readiness Funnel

The funnel should not stop at "sending."
Operators need confirmation that the plugin received the stream.

Call:
- `Device -> Motion -> Synced -> Sending -> Plugin Ack`

### 7. Show Match Score In The Active Profile Summary

The score should not live only in the capture tray.

Better:
- `Active: Personalized (Match 94%)`

### 8. Acknowledge Token Drift

The design-token system and the current HTML implementation still diverge.

Call:
- do not treat the token file as fully implemented truth until those values reconcile.

## What The Earlier Review Missed

| Item | Severity | Why |
|---|---|---|
| Synthetic mode visibility | high | silent first-run failure risk |
| Token drift | medium | token system and implementation disagree |
| Axis flip tiering | medium | too hidden for a first-run corrective |
| Plugin Ack in readiness funnel | medium | trust loop stays open otherwise |
| Stale pose reason in top-level truth | medium | degradation needs plain-English cause |
| Quality badge value | low | small, but product-meaningful |
| Match score in active profile | low | useful operator confidence signal |

## Final Direction

Use the original review as the base.
Use these second-opinion calls as overrides where they are more specific.

Priority order:
1. compress `RENDERER` to four default-visible items,
2. make companion `Focus` tell the truth faster,
3. surface `synthetic mode`, `Plugin Ack`, and stale-pose reasons,
4. reconcile token drift before treating the token files as fully authoritative.

## Visual Aid Index

| Visual | Role |
|---|---|
| `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/render-trust-ladder.svg` | trust hierarchy override |
| `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/companion-focus-lab-hierarchy.svg` | companion hierarchy override |
| `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/scope-boundary.svg` | scope tightening visual |
| `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/second-opinion-prototype.html` | prototype reference |

