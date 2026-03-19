Title: BL-089 Render Trust Contract and Requested-vs-Active Language
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# BL-089 Render Trust Contract and Requested-vs-Active Language

## Status
Done. The shared render-trust language contract landed in plugin and companion surfaces, parity evidence was captured, and owner closeout sync completed.

## Plain-Language Summary
BL-089 made LocusQ honest about render state. Operators now see what they requested, what is actually active, why anything changed, where the active profile came from, who owns the decision, and whether quality-style claims are measured, estimated, generic, or unavailable.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | One additive render-trust language contract for plugin and companion. |
| Why | Prevented over-claiming, hidden fallback behavior, and drift between plugin and companion trust language. |
| Who | Plugin operators, companion users, QA owners, and UI/runtime maintainers. |
| When | Done on 2026-03-18 after parity evidence and owner sync passed. |
| Where | [`Documentation/backlog/done/bl-089-render-trust-contract-and-requested-active-language.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-089-render-trust-contract-and-requested-active-language.md), `TestEvidence/...`, and the 2026-03-17 UI/UX report set. |
| How | Shared requested-vs-active fields, plain-language fallback rules, and deterministic degraded-state behavior across plugin and companion. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final trust contract, gates, and evidence map | This runbook |
| UI and report detail | Full design, copy, and validation packet | Archived legacy copy |

## Core Contract
- `Requested` and `Active` always appear together where fallback is possible.
- `Why this changed` is required when requested and active diverge.
- First-layer trust copy stays plain-language, not internal enums or debug labels.
- Evidence-status language must disclose measured vs estimated vs generic vs unavailable claims.
- Plugin and companion must use the same trust vocabulary for equivalent states.

## Key Gates
- Plugin and companion parity evidence passes.
- Fallback and degraded states remain explicit.
- Owner sync packet closes the trust-wave lane cleanly.
- Follow-on truthfulness lanes stay separate instead of hiding in this closeout.

## Evidence Pointers
| Signal | Path |
|---|---|
| Trust-wave validation bundle | `TestEvidence/ui_ux_trust_wave_validation_20260318T023805Z/summary.md` |
| Owner sync packet | `TestEvidence/ui_ux_trust_wave_owner_sync_z1_20260318T040618Z/promotion_decision.md` |
| Related report set | `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`, `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`, `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Trust field contract | Done | Requested, active, reason, source, owner, and evidence status normalized. |
| Plugin and companion rollout | Done | Shared language shipped on both sides. |
| Owner promotion | Done | Validation and closeout sync completed. |

## Archive Note
Full historical material is preserved at [`bl-089-render-trust-contract-and-requested-active-language-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-089-render-trust-contract-and-requested-active-language-legacy.md).
