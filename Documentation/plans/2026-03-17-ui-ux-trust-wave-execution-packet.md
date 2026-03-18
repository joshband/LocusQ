Title: UI/UX Trust Wave Execution Packet
Document Type: Planning Packet
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# UI/UX Trust Wave Execution Packet

## Purpose

Lock the immediate execution plan for the top three UI/UX backlog items:

- `BL-091 Companion Focus/Lab Trust Flow`
- `BL-090 Plugin Authority-First Shell and Renderer Hierarchy`
- `BL-089 Render Trust Contract and Requested-vs-Active Language`

This packet converts the 2026-03-17 design review, refinement pass, backlog prioritization, and Claude second-opinion review into a buildable wave with explicit write ownership, sequencing, and promotion gates.

## Scope Baseline

- The companion monitor is still one dense telemetry-first surface inside `companion/Sources/LocusQHeadTrackingCompanion/main.swift`.
- The plugin shell header still shows `scene-status`, `viewport-lock`, `room-profile`, and `quality-badge` at the same hierarchy level in `Source/ui/public/index.html`.
- `RENDERER` still exposes nine default-visible card groups in the live shell.
- Requested/active truth and fallback language are split across plugin and companion surfaces rather than frozen as one operator contract.
- `BL-095` now governs FIR-engine truthfulness follow-up, so BL-089 must avoid finalizing engine-specific trust phrases that could conflict with its outcome.

## Entry Conditions

| Item | Can Start Now? | Promotion Gate |
|---|---|---|
| `BL-091` | Yes | Focus copy and trust states must reconcile with BL-089 before promotion |
| `BL-090` | Yes | Authority card language and degraded-state wording must reconcile with BL-089 before promotion |
| `BL-089` | Yes | Engine-specific requested/active copy must remain consistent with BL-095 before promotion |
| `BL-093` | No | Wait until BL-091, BL-090, and BL-089 land and token drift is reconciled |

## Ownership and Write Sets

| Pod | Primary BL | Primary Files | Ownership Rule |
|---|---|---|---|
| Companion Trust Pod | `BL-091` | `companion/Sources/LocusQHeadTrackingCompanion/main.swift` | Own structure, Focus/Lab split, readiness funnel, capture tray, and companion operator copy |
| Plugin Shell Pod | `BL-090` | `Source/ui/public/index.html`, `Source/ui/src/index.ts` | Own shell hierarchy, header compression, renderer default-visible reduction, and contextual badge placement |
| Trust Copy Pod | `BL-089` | `Documentation/backlog/bl-089-render-trust-contract-and-requested-active-language.md`, then targeted plugin/companion state-text hooks | Freeze vocabulary first; integrate late enough to avoid churn while BL-091 and BL-090 move structure |

Shared-surface rule:

- `BL-091` and `BL-090` may start immediately because their primary write sets are disjoint.
- `BL-089` starts with docs-first copy normalization, then lands targeted code edits after the structural shells exist.
- Any UI text living inside a `BL-091` or `BL-090` patch should use temporary plain-English placeholders only if they already match the BL-089 vocabulary table.

## Wave Plan

### Wave 1A: BL-091 Companion Focus/Lab Skeleton

Objective: make the companion default to trust-first operation without losing diagnostic depth.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `BL091-S1` | Introduce `Focus` and `Lab` shell split | monitor HTML structure around `#metrics`, status/header area, and section grouping in `main.swift` | default launch surface is `Focus`; dense telemetry moves behind `Lab` without being deleted |
| `BL091-S2` | Build the 5-step readiness funnel with `Plugin Ack` and synthetic-mode warning | `updateStatus`, `updateMetrics`, readiness/state formatting, plugin ingest readout, new Focus status components | `Device -> Motion -> Synced -> Sending -> Plugin Ack` is visible; `mode=synthetic` produces a clear Focus warning |
| `BL091-S3` | Move sync/orientation/profile essentials into Focus | `syncButton`, `syncHint`, axis-flip controls, profile summary and apply/capture section | `Center / Sync`, axis orientation, active profile summary, match percentage, capture tray, and privacy note are first-layer content |
| `BL091-S4` | Retain deep diagnostics in `Lab` and clean up operator text | transport/orientation/motion/stabilization/profile telemetry rows plus state-label formatting | raw internal strings are not shown in Focus; Lab retains packet age, seq, jitter, and matcher detail |

Validation lanes:

- `cd companion && swift build && swift test`
- visual/runtime review of Focus-first launch
- visual/runtime review of Lab telemetry retention

## Wave 1B: BL-090 Plugin Shell Compression

Objective: reduce header and renderer competition so the viewport and authority card lead.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `BL090-S1` | Compress the header while preserving the boot shell and quality badge | header markup/CSS in `index.html`; badge/status logic in `index.ts` | header keeps brand, mode tabs, session pill, trust badge, and quality badge only |
| `BL090-S2` | Remove non-global header truths and relocate them contextually | `viewport-lock`, `room-profile`, EMITTER contextual strip, renderer room section | `viewport-lock` is EMITTER-only; `room-profile` is no longer a persistent header signal |
| `BL090-S3` | Reduce `RENDERER` to four default-visible items | renderer card markup in `index.html`; renderer-state rendering/toggles in `index.ts` | default-visible items are `Authority`, `Spatialization`, `Room`, and collapsed `Lab`; diagnostics/internals move out of the main stack |
| `BL090-S4` | Preserve authority-first degraded behavior and capture the new hierarchy | fallback note, authority-state rendering, shell screenshots/captures | boot shell remains intact; degraded/fallback states still render honestly after the hierarchy change |

Validation lanes:

- `cd Source/ui && npm run typecheck`
- `cd Source/ui && npm run build`
- browser preview or Playwright capture of the compressed shell

## Wave 1C: BL-089 Trust Vocabulary and Parity

Objective: freeze the shared operator language and reconcile plugin/companion copy after the new shells exist.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `BL089-S1` | Freeze the state and copy table in docs | `Documentation/backlog/bl-089-render-trust-contract-and-requested-active-language.md` | requested/active/why/profile source/control owner mappings are explicit and approved |
| `BL089-S2` | Normalize plugin authority/fallback language | `Source/ui/src/index.ts` renderer authority/fallback helpers; targeted `index.html` labels if needed | plugin never leaks raw fallback or authority-state tokens and stale-pose reasons are plain English |
| `BL089-S3` | Normalize companion readiness/profile language | `main.swift` `readinessLabel`, `updateStatus`, `updateMetrics`, Focus summary copy | companion Focus never leaks raw readiness/state tokens and synthetic mode is named plainly |
| `BL089-S4` | Final parity pass and BL-095 truth check | plugin + companion screenshots/review plus any engine-specific text references | equivalent states use equivalent language; FIR-engine phrasing does not overstep BL-095 truth constraints |

Validation lanes:

- plugin/companion visual parity review
- browser preview capture for plugin authority states
- companion runtime review for Focus copy states

## Recommended Sequencing

1. Start `BL091-S1` and `BL090-S1` in parallel.
2. Land `BL089-S1` immediately as the vocabulary baseline.
3. Move to `BL091-S2` and `BL090-S2` once the structural shells are in place.
4. Land `BL091-S3` and `BL090-S3` next; these are the highest-value operator-visible outcomes.
5. Reconcile copy with `BL089-S2` and `BL089-S3`.
6. Close with `BL091-S4`, `BL090-S4`, and `BL089-S4` capture/parity review.

## Immediate Non-Goals

- No new top-level plugin mode.
- No CLAP-specific or AUv3-specific alternate information architecture.
- No new diagnostic surfaces outside the existing plugin `Lab` concept and companion `Lab`.
- No token-polish work that changes structure before `BL-093`.

## Promotion Gates

| Gate | Requirement |
|---|---|
| `G1` | Companion Focus shows synthetic mode, readiness funnel, Plugin Ack, orientation controls, and active profile summary without raw runtime strings |
| `G2` | Plugin header is compressed and `RENDERER` is reduced to four default-visible items while keeping boot shell and quality badge |
| `G3` | Requested/active/fallback/profile-source/control-owner language is consistent across plugin and companion |
| `G4` | Any engine-specific trust text remains truthful relative to BL-095 |
| `G5` | Fresh captures exist for companion Focus/Lab and plugin RENDERER after the hierarchy changes |

## Source Inputs

- `Documentation/backlog/bl-091-companion-focus-lab-trust-flow.md`
- `Documentation/backlog/bl-090-plugin-authority-first-shell-and-renderer-hierarchy.md`
- `Documentation/backlog/bl-089-render-trust-contract-and-requested-active-language.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`

## Validation Status

`not tested` — planning packet only.
