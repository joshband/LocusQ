Title: HX-03 REAPER Multi-Instance Stability Lane
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02

# HX-03: REAPER Multi-Instance Stability Lane

## Plain-Language Summary

HX-03 in plain terms: Validated multi-instance stability in REAPER — multiple LocusQ instances in the same project with shared SceneGraph singleton. Current state: Done. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | HX-03: REAPER Multi-Instance Stability Lane |
| Why is this important? | Validated multi-instance stability in REAPER — multiple LocusQ instances in the same project with shared SceneGraph singleton. |
| How will we deliver it? | Use the documented implementation summary and promotion gates in this closeout runbook to confirm what shipped and why it is safe. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/hx-03-reaper-multi-instance.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Done |
| Completed | 2026-02-23 |
| Owner Track | Track F Hardening |

## Objective

Validated multi-instance stability in REAPER — multiple LocusQ instances in the same project with shared SceneGraph singleton.

## What Was Built

- Multi-instance stability test lane
- Shared SceneGraph concurrent access validation

## Key Files

- Validation scripts
- `Source/SceneGraph.h`

## Evidence References

- `TestEvidence/build-summary.md`
- REAPER multi-instance smoke results

## Completion Date

2026-02-23


## Governance Retrofit (2026-02-28)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

