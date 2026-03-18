Title: BL-092 Cross-Format Capability Messaging Parity
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# BL-092 Cross-Format Capability Messaging Parity

## Plain-Language Summary

BL-092 in plain terms: keep LocusQ feeling like one instrument across AU, VST3, CLAP, AUv3, browser preview, `WKWebView`, and `WebView2`, while explaining limited capabilities honestly instead of silently hiding them. Current state: Done. The 2026-03-17 second-opinion review reinforced that AUv3 should not get a separate UI worldview, and the current plugin shell now carries the preview/degraded/AUv3 language path with representative native and preview captures, an owner packet, and a completed archive/closeout sync.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, release owners, QA maintainers, and cross-format/runtime implementers. |
| What is changing? | Capability messaging, boot/degraded states, and limited-feature wording across plugin formats and WebView runtimes are normalized. |
| Why is this important? | AUv3 extension limits, backend timing differences, and browser-preview fallbacks can otherwise create fragmented UX or misleading claims. |
| How will we deliver it? | Define one IA and one copy system, then implement explicit limited-capability and degraded-state surfaces for each supported runtime context. |
| When is it done? | When the same core navigation and trust model hold everywhere, and any limitation is explained explicitly in plain language. |
| Where is the source of truth? | This runbook, `Documentation/backlog/index.md`, BL-067/BL-074/BL-011 artifacts, and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of scope, priority, and dependencies. | `## Status Ledger` |
| Runtime matrix visual | Summarizes the parity goal across backends and formats. | `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/format-runtime-parity.svg` |
| Validation table | Makes cross-format proof requirements explicit. | `## Validation Plan` |
| Trust-wave validation bundle | Fresh browser-preview and native-shell captures used for parity review. | `TestEvidence/ui_ux_trust_wave_validation_20260318T023805Z/summary.md` |
| Owner sync packet | Records the promotion decision and final owner closeout checks. | `TestEvidence/ui_ux_trust_wave_owner_sync_z1_20260318T040618Z/promotion_decision.md` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-092 |
| Priority | P2 |
| Status | Done |
| Track | A - Runtime Formats |
| Effort | Medium / M |
| Depends On | BL-067 (In Validation), BL-074 (Done), BL-089 |
| Blocks | — |
| Default Replay Tier | T1 |
| Heavy Lane Budget | High-cost wrapper (format/back-end matrix) |

## Objective

Keep one information architecture across supported runtime contexts while making capability limits honest and explicit.

Required messaging rules:

1. AU, VST3, and CLAP share the same mode model and top-level navigation.
2. AUv3 uses the same shell but may display explicit limited-capability notices in `Why this changed` and `Control owner`.
3. CLAP does not get a separate diagnostic worldview or alternate information architecture.
4. `WKWebView` and `WebView2` share the same hierarchy and degraded-state semantics.
5. Browser preview remains able to show shell structure and honest “native unavailable” states.
6. No format or backend gets a separate worldview unless a true workflow difference requires it.

## Source Inputs

- `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`
- `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/format-runtime-parity.svg`
- `Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md`
- `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`
- `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md`
- `TestEvidence/clap-validation-report-2026-02-22.md`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`

## Acceptance IDs

- `BL092-A1` AU, VST3, and CLAP retain the same top-level UI information architecture.
- `BL092-A2` AUv3-limited capabilities use explicit plain-language notices in `Why this changed` and `Control owner` rather than silent omission.
- `BL092-A3` Boot, degraded, and native-unavailable states are coherent across `WKWebView`, `WebView2`, and browser preview.
- `BL092-A4` Backend-specific timing or native-bridge absence does not remove first-layer truth surfaces.
- `BL092-A5` CLAP does not introduce a separate diagnostic worldview for equivalent states.
- `BL092-A6` Cross-format screenshots or captures demonstrate parity of structure, not perfect pixel identity.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Capability matrix and copy contract | runbook + UI copy maps + format/runtime notes | approved matrix for AU/VST3/CLAP/AUv3/WebView/browser preview |
| B | Plugin shell limited-capability and degraded-state surfaces | plugin shell / bridge UI paths | all limited states are explicit and plain-language |
| C | Capture parity and backend review | format/runtime matrix + visual evidence | representative captures show one coherent IA |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL092-FORMAT-BUILD | Automated | representative AU/CLAP/AUv3 builds and existing lane scripts | builds/lane entrypoints stay green |
| BL092-BROWSER-PREVIEW | Automated | preview `Source/ui/public/index.html` and capture degraded/native-unavailable states | shell remains reviewable without native services |
| BL092-RUNTIME-CAPTURE | Automated/manual | `WKWebView` and `WebView2` representative visual capture | equivalent states use equivalent structure and messaging |
| BL092-DOCS | Automated | docs/backlog validation commands | exit 0 |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 1-3 | format build/lane spot checks + UI captures | logs + screenshots |
| Candidate intake | T2 | 2-5 (heavy-wrapper capped as needed) | cross-format parity review | replay summary + capture matrix |
| Promotion | T3 | owner-approved heavy-wrapper equivalent | owner parity packet | owner packet + deterministic evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

This runbook translates the refinement-pass and second-opinion format/runtime guidance into a concrete backlog lane. It does not change the underlying format-enablement contracts owned by BL-011 and BL-067.
