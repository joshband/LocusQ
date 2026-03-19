Title: Backlog Automation Draft Flow
Document Type: Guide
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# Backlog Automation Draft Flow

## Purpose

Define the safe automation path for backlog `T1`, `T2`, and `T3`.
This is draft automation, not blind promotion.

## Rule

Automation may prepare the packet.
Owner review still confirms the state change.

## Use This Flow When

- the runbook is stable
- validation commands are known
- evidence paths are repo-local
- the item is `draft_only` or `owner_gated_auto`

Do not use it when:
- manual host checks are required
- participant studies are still pending
- signing or external review is still blocking
- the item is `manual_only`

## Canonical Files

- contract: `Documentation/backlog/automation-contracts.json`
- runner: `scripts/backlog-auto-123.py`
- closeout-draft helper: `scripts/backlog-closeout-draft.py`
- packet path: `TestEvidence/<bl_or_hx>_auto_<stage>_<timestamp>/`

## Stage Map

| Stage | Automation may do | Owner still does |
|---|---|---|
| `T1` | run repeatable dev checks and emit evidence | authoritative status changes |
| `T2` | run candidate checks and draft owner inputs | promotion claims |
| `T3` | run promotion checks and draft sync summary | `Done`, archive move, backlog/index/status mutation |

## Required Packet Files

| File | Purpose |
|---|---|
| `status.tsv` | machine-readable packet status |
| `validation_matrix.tsv` | per-command results |
| `promotion_decision.md` | draft promotion note |
| `draft_update_summary.md` | proposed status/doc updates |

Optional when coordination risk is not low:
- `owner_decisions.md`
- `handoff_resolution.md`

## Result Tokens

| Token | Meaning |
|---|---|
| `DRAFT_READY` | checks passed and packet is ready for owner review |
| `BLOCKED` | required checks failed or config is incomplete |
| `MANUAL_ONLY` | item is intentionally excluded from automation |

## Closeout Draft Prep

Example:
- `scripts/backlog-closeout-draft.py --id BL-080 --packet TestEvidence/bl-080_auto_t3_<timestamp>/`

This prepares a closeout diff summary.
It does not move files or mutate authoritative status.
