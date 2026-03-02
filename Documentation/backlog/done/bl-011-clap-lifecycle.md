Title: BL-011 CLAP Lifecycle and CI/Host Closeout
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02

# BL-011: CLAP Lifecycle and CI/Host Closeout

## Plain-Language Summary

BL-011 in plain terms: Completed full CLAP plugin format lifecycle — build, install, descriptor validation, clap-validator pass, QA lanes, REAPER discoverability. Current state: Done. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-011: CLAP Lifecycle and CI/Host Closeout |
| Why is this important? | Completed full CLAP plugin format lifecycle — build, install, descriptor validation, clap-validator pass, QA lanes, REAPER discoverability. |
| How will we deliver it? | Use the documented implementation summary and promotion gates in this closeout runbook to confirm what shipped and why it is safe. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-011-clap-lifecycle.md` plus repo-local evidence under `TestEvidence/...`. |

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
| Owner Track | Track A Runtime Formats |

## Objective

Completed full CLAP plugin format lifecycle — build, install, descriptor validation, clap-validator pass, QA lanes, REAPER discoverability. Annex: `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`.

## What Was Built

- CLAP adapter compilation
- Descriptor registration
- Validator pass
- Host discovery verification

## Key Files

- `Source/PluginProcessor.cpp`
- CMake config
- `Documentation/plans/LocusQClapContract.h`

## Evidence References

- BL-011 closeout evidence in annex
- `TestEvidence/build-summary.md`
- ADR: ADR-0009

## Completion Date

2026-02-23


## Governance Retrofit (2026-02-28)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

