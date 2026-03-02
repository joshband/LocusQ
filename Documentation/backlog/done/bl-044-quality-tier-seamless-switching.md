Title: BL-044 Quality-Tier Seamless Switching
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-02

# BL-044 Quality-Tier Seamless Switching

## Plain-Language Summary

This runbook tracks **BL-044** (BL-044 Quality-Tier Seamless Switching). Current status: **In Planning**. In plain terms: Eliminate audible discontinuities when switching quality tiers and smoothing-sensitive delay parameters, preserving tonal intent across tier changes.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-044 Quality-Tier Seamless Switching |
| Why is this important? | Eliminate audible discontinuities when switching quality tiers and smoothing-sensitive delay parameters, preserving tonal intent across tier changes. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when required acceptance criteria, validation lanes, and evidence synchronization are all marked pass. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-044-quality-tier-seamless-switching.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Implementation slices table | Explains step-by-step delivery order and boundaries. | `## Implementation Slices` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| ID | BL-044 |
| Priority | P1 |
| Status | In Planning |
| Track | F - Hardening |
| Effort | Med / M |
| Depends On | BL-043 |
| Blocks | — |

## Objective

Eliminate audible discontinuities when switching quality tiers and smoothing-sensitive delay parameters, preserving tonal intent across tier changes.

## Scope

In scope:
- Seamless quality switching for FDN and early reflections.
- Delay parameter smoothing where abrupt changes click.
- Deterministic audio artifact checks during tier transitions.

Out of scope:
- New quality-tier definitions.
- Full reverb rewrite.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | FDN/ER tier-switch transition path | No click/pop on tier changes in validation lane |
| B | Speaker delay smoothing | Delay automation no longer produces discontinuities |
| C | Transition QA lane and evidence | Tier-switch replay passes deterministic thresholds |

## TODOs

- [ ] Implement crossfade or equivalent continuity-preserving tier switch path.
- [ ] Prevent stale/new tap discontinuity in early reflections during tier changes.
- [ ] Smooth speaker-delay transition path to remove automation clicks.
- [ ] Add transition artifact taxonomy and acceptance thresholds.
- [ ] Capture deterministic replay evidence for promotion readiness.

## Validation Plan

- `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
- `./scripts/qa-bl044-quality-switch-lane-mac.sh --runs 5 --out-dir TestEvidence/bl044_<slice>_<timestamp>`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `build.log`
- `transition_runs.tsv`
- `artifact_taxonomy.tsv`
- `quality_switch_contract.md`
- `docs_freshness.log`
