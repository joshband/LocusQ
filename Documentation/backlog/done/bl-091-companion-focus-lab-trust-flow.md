Title: BL-091 Companion Focus/Lab Trust Flow
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# BL-091 Companion Focus/Lab Trust Flow

## Plain-Language Summary

BL-091 in plain terms: reorganize the Head-Tracking Companion so it launches as a confidence tool first and a lab console second. Current state: Done. The 2026-03-17 second-opinion review raised the urgency of this lane, and the Focus/Lab split is now live with synthetic-mode disclosure, Plugin Ack, and first-layer orientation/profile controls; the live Focus/Lab captures and owner packet are now in place, and the archive/closeout step is complete.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Companion users, support/QA owners, and maintainers of the Apple head-tracking and profile-acquisition path. |
| What is changing? | The companion gets a `Focus` default surface and a separate `Lab` surface for deep telemetry and diagnostics. |
| Why is this important? | Today the companion behaves like a diagnostics cockpit first, which slows first-run confidence and hides the one answer most users need: “Can I trust this right now?” |
| How will we deliver it? | Build a readiness ladder, trust summary, capture tray, and explicit Lab boundary, then validate that runtime diagnostics remain available without dominating first launch. |
| When is it done? | When the default companion flow answers device readiness, sync state, active profile, and next capture step before it asks the user to parse dense telemetry. |
| Where is the source of truth? | This runbook, `Documentation/backlog/index.md`, the 2026-03-17 UI/UX reports, and evidence under `Documentation/reports/visuals/...` plus `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast priority/dependency scan. | `## Status Ledger` |
| Focus/Lab operating model | Shows the intended information hierarchy. | `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/focus-lab-operating-model.svg` |
| Companion wireframe | Anchors the current reviewed concept. | `Documentation/reports/visuals/ui-ux-review-2026-03-17/companion-wireframe.svg` |
| Validation table | Clarifies how runtime truth and UX containment are both checked. | `## Validation Plan` |
| Trust-wave validation bundle | Fresh `Focus` and `Lab` captures from the live companion surface. | `TestEvidence/ui_ux_trust_wave_validation_20260318T023805Z/summary.md` |
| Owner sync packet | Records the promotion decision and final owner closeout checks. | `TestEvidence/ui_ux_trust_wave_owner_sync_z1_20260318T040618Z/promotion_decision.md` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-091 |
| Priority | P1 |
| Status | Done |
| Track | C - UX Authoring |
| Effort | Medium / M |
| Depends On | BL-045 (Done), BL-058 (Done), BL-072 (Done) |
| Blocks | BL-093 |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Create a trust-first companion information architecture with these rules:

1. Default launch surface is `Focus`.
2. `Focus` must show, in order:
   - readiness funnel (`Device -> Motion -> Synced -> Sending -> Plugin Ack`)
   - synthetic-mode warning whenever `mode=synthetic`
   - `Center / Sync` plus compact axis orientation controls
   - active profile summary with match percentage and profile source
   - capture tray
   - apply action plus privacy note
   - collapsed `Lab` drawer
3. `Lab` remains available for:
   - packet age
   - effective rate
   - axis sanity
   - tri-view diagnostics
   - matcher confidence
   - stale-pose and fallback detail
4. Raw runtime state strings never appear in `Focus`; BL-089 plain-language mappings are required for any operator-facing state.
5. Privacy and local-processing cues stay near capture and apply actions.
6. The companion continues to expose enough truth for BL-058-class diagnostics without turning those metrics into the first-run homepage.

## Source Inputs

- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`
- `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/focus-lab-operating-model.svg`
- `Documentation/reports/visuals/ui-ux-review-2026-03-17/companion-wireframe.svg`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/companion-focus-lab-hierarchy.svg`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/second-opinion-prototype.html`
- `Documentation/reports/ui-ux-refinement-2026-03-17/component-specs.md`
- `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`
- `Documentation/testing/bl-045-headtracking-fidelity-qa.md`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`

## Acceptance IDs

- `BL091-A1` Default companion launch view is `Focus`, not dense telemetry.
- `BL091-A2` The readiness funnel contains five steps, including `Plugin Ack`.
- `BL091-A3` Synthetic mode is visibly disclosed in `Focus` whenever active.
- `BL091-A4` `Center / Sync` and compact axis orientation controls appear above raw metrics.
- `BL091-A5` Active profile summary includes requested/active truth where needed, match percentage, profile source, and fallback reason in plain language.
- `BL091-A6` Capture tray keeps privacy/local-processing language adjacent to capture/apply actions.
- `BL091-A7` `Lab` retains tri-view diagnostics, packet-age truth, matcher detail, and stale-pose detail without blocking the focus flow.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Focus/Lab IA contract and ordered content map, including synthetic-mode, axis, and Plugin Ack rules | runbook + companion UI structure | approved `Focus` vs `Lab` content split reflects the five-step readiness loop |
| B | Focus default surface, readiness funnel, trust summary, active profile summary, and capture tray | `companion/Sources/LocusQHeadTrackingCompanion/main.swift` and companion UI assets | first-run flow answers trust questions before telemetry and exposes no raw state strings |
| C | Lab containment and diagnostic carryover | companion diagnostic views and toggles | deep telemetry remains intact but secondary |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL091-COMPANION-BUILD | Automated | `cd companion && swift build && swift test` | exit 0 |
| BL091-FOCUS-REVIEW | Visual/runtime review | capture focus-first launch and profile flow | first screen communicates readiness and next action |
| BL091-LAB-REVIEW | Visual/runtime review | capture lab diagnostics state | packet age, tri-view, and matcher detail remain available |
| BL091-TRUST-PARITY | Focused QA | compare companion trust summary to BL-089 contract | equivalent states use approved copy |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 1-3 | companion build/test + focused runtime review | logs + screenshots |
| Candidate intake | T2 | 5 or owner-approved equivalent | readiness and profile-flow replay checks | replay summary + capture bundle |
| Promotion | T3 | 10 or owner-approved equivalent | owner-reviewed Focus/Lab packet | owner packet + evidence bundle |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

This runbook implements the companion-side hierarchy recommendations from the 2026-03-17 design review, refinement pass, and second-opinion report; it relies on the existing head-tracking and profile-acquisition runtime contracts rather than redefining them.

Reactive, simulation-driven, and temporal follow-on ideas now route through `BL-094`, which keeps `Focus` action-first and reserves experimental metrics or behaviors for explicit `Lab` containment unless they change the operator's next 10 seconds of action.
