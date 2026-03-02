Title: BL-014 Listener/Speaker/Aim/RMS Overlay Strict Closeout
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02

# BL-014: Listener/Speaker/Aim/RMS Overlay Strict Closeout

## Plain-Language Summary

BL-014 in plain terms: Finalized all viewport overlay confidence lanes (listener position, speaker positions, aim direction, RMS rings) with strict deterministic evidence. Current state: Done. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-014: Listener/Speaker/Aim/RMS Overlay Strict Closeout |
| Why is this important? | Finalized all viewport overlay confidence lanes (listener position, speaker positions, aim direction, RMS rings) with strict deterministic evidence. |
| How will we deliver it? | Use the documented implementation summary and promotion gates in this closeout runbook to confirm what shipped and why it is safe. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-014-overlay-strict-closeout.md` plus repo-local evidence under `TestEvidence/...`. |

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
| Completed | 2026-02-24 |
| Owner Track | Track B Scene/UI Runtime |

## Objective

Finalized all viewport overlay confidence lanes (listener position, speaker positions, aim direction, RMS rings) with strict deterministic evidence.

## What Was Built

- Deterministic overlay rendering contracts
- Strict self-test assertions for each overlay type
- Evidence bundle with probe-level granularity

## Key Files

- `Source/ui/public/js/index.js`
- `Source/SceneGraph.h`
- `Source/PluginProcessor.cpp`

## Evidence References

- `TestEvidence/locusq_production_p0_selftest_20260224T032239Z.json`
- `TestEvidence/locusq_smoke_suite_spatial_bl014_20260224T032355Z.log`

## Completion Date

2026-02-24


## Governance Retrofit (2026-02-28)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

