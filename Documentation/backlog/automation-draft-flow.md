Title: Backlog Automation Draft Flow
Document Type: Guide
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# Backlog Automation Draft Flow

## Purpose

Define the safe automation path for backlog `T1`, `T2`, and `T3`.
This guide covers draft-only automation, not blind promotion.

## Rule

Automation may prepare the packet.
Owner review still confirms the state change.

## When To Use

Use this flow when:
- the backlog item already has a stable runbook,
- validation commands are known,
- evidence paths are repo-local,
- the item is marked `draft_only` or `owner_gated_auto`.

Do not use this flow when:
- manual host checks are still required,
- participant study work is still pending,
- platform signing or external review is still blocking,
- the item is marked `manual_only`.

## Contract

Machine-readable eligibility lives in:
- `Documentation/backlog/automation-contracts.json`

Current runner:
- `scripts/backlog-auto-123.py`
- `scripts/backlog-closeout-draft.py`

Packet output:
- `TestEvidence/<bl_or_hx>_auto_<stage>_<timestamp>/`

## Stage Map

| Stage | What automation may do | What stays owner-only |
|---|---|---|
| `T1` | run repeatable dev checks, emit compact evidence | any authoritative status move |
| `T2` | run candidate checks, emit blocker summary, draft owner inputs | any promotion claim |
| `T3` | run promotion checks, assemble promotion packet, draft sync summary | `Done`, archive move, backlog/index/status mutation |

## Required Packet Files

| File | Purpose |
|---|---|
| `status.tsv` | machine-readable packet status |
| `validation_matrix.tsv` | per-command results |
| `promotion_decision.md` | draft promotion note |
| `draft_update_summary.md` | proposed status/doc updates |
| `owner_decisions.md` | optional; add when coordination risk is not low |
| `handoff_resolution.md` | optional; add when coordination risk is not low |

## Result Tokens

| Token | Meaning |
|---|---|
| `DRAFT_READY` | checks passed and packet is ready for owner review |
| `BLOCKED` | one or more required checks failed or config is incomplete |
| `MANUAL_ONLY` | item is intentionally excluded from automation |

## Closeout Draft Prep

Use:
- `scripts/backlog-closeout-draft.py --id BL-080 --packet TestEvidence/bl-080_auto_t3_<timestamp>/`

This prepares a closeout diff summary.
It does not move files or change authoritative status.

## Pilot Set

Use the first rollout on:
- `BL-080`
- `BL-089`
- `BL-090`
- `BL-091`
- `BL-092`
- `BL-093`
- `BL-094`

These items are close enough to closeout to benefit from draft packet automation without risking misleading state changes.
