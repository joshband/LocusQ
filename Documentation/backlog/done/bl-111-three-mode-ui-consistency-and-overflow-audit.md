Title: BL-111 Three-Mode UI Consistency and Overflow Audit
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-20
Last Modified Date: 2026-03-20

# BL-111: Three-Mode UI Consistency and Overflow Audit

## Plain-Language Summary

BL-111 gives `CALIBRATE`, `EMITTER`, and `RENDERER` one focused visual QA lane. The goal is to catch and fix the specific problems users feel at launch size: overlapping text, clipped content, awkward wrapping, inconsistent helper surfaces, and mode-to-mode visual drift.

Current state: **Done** (2026-03-20).

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, UI maintainers, and QA owners reviewing launch-size usability. |
| What is changing? | LocusQ gets one dedicated audit for three-mode consistency, overflow defects, and helper-surface behavior. |
| Why is this important? | The current review set is strategic, but it does not yet tightly audit cropping, overlap, tooltip/popover/toast placement, inline helper fit, or cross-mode visual consistency. |
| How will we deliver it? | Create a capture-backed defect matrix for all three modes, then land only the verified layout/copy fixes needed to resolve the findings. |
| When is it done? | Done means each mode has been reviewed at realistic launch sizes, defects are capture-backed, fixes are landed, and before/after evidence is stored under `TestEvidence/`. |
| Where is the source of truth? | `Documentation/backlog/bl-111-three-mode-ui-consistency-and-overflow-audit.md`, `Documentation/reports/2026-03-20-locusq-three-mode-ui-consistency-audit-packet.md`, and `TestEvidence/bl111_<slice>_<timestamp>/`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast scan of lane scope and dependencies | `## Status Ledger` |
| Progress snapshot | Shows audit-now vs fix-later sequencing | `## Progress Snapshot` |
| Slice table | Keeps audit, fix, and evidence work separate | `## Implementation Slices` |
| Defect matrix | Tracks overlap/crop/consistency findings | future `TestEvidence/bl111_*` bundle |

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | **Done** (2026-03-20) |
| Owner Track | C - UX Authoring |
| Depends On | BL-090 (Done), BL-093 (Done), BL-094 (Done) |
| Blocks | BL-107 visual foundation productionization quality signoff |
| Annex Spec | `Documentation/reports/2026-03-20-locusq-three-mode-ui-consistency-audit-packet.md` |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Automation Contract

| Field | Value |
|---|---|
| Automation Mode | `draft_only` unless owner-approved otherwise |
| Stage Cap | `T1` / `T2` / `T3` |
| Owner Approval Required For | `Done`, archive move, status/index transition |
| Runner Output | `DRAFT_READY`, `BLOCKED`, `MANUAL_ONLY` |

## Progress Snapshot

| Item | Status | Updated | Where | Remaining |
|---|---|---|---|---|
| Macro trust-wave reviews exist | `[DONE]` | 2026-03-20 | March 17 review set | none |
| Tight three-mode audit packet | `[DONE]` | 2026-03-20 | `Documentation/reports/2026-03-20-locusq-three-mode-ui-consistency-audit-packet.md` | none |
| Capture-backed defect matrix | `[DONE]` | 2026-03-20 | `TestEvidence/bl111_ui_consistency_audit_20260320T020500Z/` | none |
| Scoped UI fix pass | `[DONE]` | 2026-03-19 | `Source/ui/public/index.html` | none |
| Disclosure and rhythm fix pass | `[DONE]` | 2026-03-19 | `Source/ui/public/index.html` | none |
| Renderer authority condensation pass | `[DONE]` | 2026-03-19 | `Source/ui/public/index.html` | none |
| Renderer authority polish pass | `[DONE]` | 2026-03-20 | `Source/ui/public/index.html` | none |
| Post-fix standalone rerun | `[DONE]` | 2026-03-19 | `TestEvidence/locusq_production_p0_selftest_20260319T235656Z.json` + `TestEvidence/standalone_ui_smoke_20260319T234726Z/` | owner closeout/admin only unless more renderer polish is desired |

## Objective

Audit and tighten the three top-level plugin modes at real launch sizes.
This is a product-fit and usability lane, not a broad redesign.

## Scope

### In scope

- `CALIBRATE`, `EMITTER`, and `RENDERER` rail consistency
- clipping, overlap, truncation, and awkward wrapping defects
- tooltip, popover-like helper, toast, snackbar-like notice, and inline helper behavior
- consistency of color, text, sizing, spacing, and card rhythm across modes

### Out of scope

- broad information architecture reinvention
- companion UX work
- new visualization primitives unrelated to verified UI defects

## Architecture Context

- Invariants: `Documentation/invariants.md` - truthful UI status, bounded fallback language, stable shell behavior
- ADRs: `Documentation/adr/ADR-0021-smart-brevity-documentation-contract.md`
- Architecture: `Source/ui/public/index.html`, `Source/ui/src/index.ts`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Capture-backed audit of the three modes at realistic sizes | review packet + `TestEvidence/bl111_*` | runbook exists | defect matrix is explicit |
| B | Apply only the verified layout/copy/surface fixes | `Source/ui/public/index.html`, `Source/ui/src/index.ts` | Slice A complete | verified defects are resolved |
| C | Refresh screenshots and evidence | `TestEvidence/bl111_*` | Slice B complete | before/after proof is reviewable |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL111-VISUAL | Manual / capture-backed | review `CALIBRATE`, `EMITTER`, `RENDERER` at launch, compact, and tight layouts | each defect is captured with mode, size, and severity |
| BL111-UI | Automated | `cd Source/ui && npm run typecheck && npm run build` | exit 0 after any fixes |
| BL111-STANDALONE | Automated/manual | targeted standalone visual check after fixes | no previously captured critical overlap/crop issues remain |
| BL111-DOCS | Automated | `./scripts/export-backlog-summaries.py`, `./scripts/export-backlog-summaries.py --check`, `./scripts/validate-backlog-plain-language.sh`, `./scripts/validate-backlog-redundancy.py`, `./scripts/validate-docs-freshness.sh`, `jq empty status.json` | exit 0 |

## Replay Cadence

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | T1 | 3 | defect matrix + targeted UI proof |
| Candidate | T2 | 5 | before/after image bundle |
| Promotion | T3 | 10 or owner-approved equivalent | owner-reviewed visual packet |

## Risks

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| The lane turns into a vague redesign | Med | High | keep scope on verified defects only |
| Real issues are described but not captured | High | Med | require screenshot/crop-backed defect entries |
| Mode-specific fixes create new cross-mode inconsistency | High | Med | review all three modes together before closing |

## Evidence Bundle

| Artifact | Path | Notes |
|---|---|---|
| `status.tsv` | `TestEvidence/bl111_ui_consistency_audit_20260320T020500Z/status.tsv` | machine-readable packet status |
| `defect_matrix.tsv` | `TestEvidence/bl111_ui_consistency_audit_20260320T020500Z/defect_matrix.tsv` | mode/size/severity/fix tracking |
| `summary.md` | `TestEvidence/bl111_ui_consistency_audit_20260320T020500Z/summary.md` | human-readable findings summary |
| UI fix slice B build proof | `Source/ui/public/index.html` | landed CALIBRATE acknowledgment coupling, shared mode-help triggers, denser-safe renderer authority layout, and timeline readability relief |
| UI fix slice C build proof | `Source/ui/public/index.html` | landed a collapsed CALIBRATE automation drawer, calmer top-of-rail support notes, and an explicit EMITTER identity help surface |
| UI fix slice D build proof | `Source/ui/public/index.html` | landed renderer authority-card condensation by keeping requested/active/output summary visible and moving `Why / Source / Owner` into disclosure |
| UI fix slice E build proof | `Source/ui/public/index.html` | tightened renderer authority-card summary spacing/typography and shortened the disclosure label to reduce scan cost at launch width |
| Reassessment summary | `TestEvidence/bl111_ui_consistency_reassessment_20260319T235900Z_summary.md` | fresh CALIBRATE launch-size proof closes F3, and the remaining renderer question is polish-level only |
| Post-fix rerun summary | `TestEvidence/bl111_postfix_visual_rerun_20260319T230000Z/summary.md` | historical note for the earlier aborted selftest rerun |
| Selftest recovery proof | `TestEvidence/locusq_production_p0_selftest_20260320T000110Z.json` | canonical production selftest stayed green after the renderer authority polish slice |
| Smoke rerun proof | `TestEvidence/standalone_ui_smoke_20260320T000150Z/` | all 6 standalone smoke probes stayed green after the renderer authority polish slice |
| CALIBRATE closeout capture | `TestEvidence/locusq_production_p0_selftest_20260319T235656Z.window.png` | fresh launch-size CALIBRATE capture after the latest shell changes |
| screenshots | `TestEvidence/bl111_<slice>_<timestamp>/` | before/after crops and full-window captures |

## Closeout Checklist

- [x] Defect matrix captured
- [x] Verified fixes landed
- [x] Before/after evidence stored under `TestEvidence/...`
- [x] `Documentation/backlog/index.md` updated when state changes
- [x] `status.json` updated when state changes
- [x] `TestEvidence/build-summary.md` updated when required
- [x] `TestEvidence/validation-trend.md` updated when required
- [x] `./scripts/validate-docs-freshness.sh` passes
