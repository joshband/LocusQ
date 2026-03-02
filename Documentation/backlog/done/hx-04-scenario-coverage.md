Title: HX-04 Scenario Coverage Audit and Drift Guard
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02

# HX-04: Scenario Coverage Audit and Drift Guard

## Plain-Language Summary

HX-04 in plain terms: Completed deterministic scenario coverage audit across AirAbsorption, CalibrationEngine, and directivity paths with embedded BL-012 enforcement. Current state: Done. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | HX-04: Scenario Coverage Audit and Drift Guard |
| Why is this important? | Completed deterministic scenario coverage audit across AirAbsorption, CalibrationEngine, and directivity paths with embedded BL-012 enforcement. |
| How will we deliver it? | Use the documented implementation summary and promotion gates in this closeout runbook to confirm what shipped and why it is safe. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/hx-04-scenario-coverage.md` plus repo-local evidence under `TestEvidence/...`. |

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
| Owner Track | Track D QA Platform |

## Objective

Completed deterministic scenario coverage audit across AirAbsorption, CalibrationEngine, and directivity paths with embedded BL-012 enforcement.

## What Was Built

- Scenario parity audit framework
- Coverage matrix with pass/warn status
- BL-012 embedded enforcement

## Key Files

- `Documentation/testing/hx-04-scenario-coverage-audit-2026-02-23.md`

## Evidence References

- `TestEvidence/hx04_scenario_audit_20260223T172312Z/status.tsv`
- `TestEvidence/bl012_harness_backport_20260223T172301Z/status.tsv`

## Completion Date

2026-02-23


## Governance Retrofit (2026-02-28)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

