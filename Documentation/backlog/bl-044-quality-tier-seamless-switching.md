Title: BL-044 Quality-Tier Seamless Switching
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-044 Quality-Tier Seamless Switching

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
