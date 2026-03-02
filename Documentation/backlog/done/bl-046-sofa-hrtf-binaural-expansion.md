Title: BL-046 SOFA HRTF and Binaural Expansion
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-02

# BL-046 SOFA HRTF and Binaural Expansion

## Plain-Language Summary

This runbook tracks **BL-046** (BL-046 SOFA HRTF and Binaural Expansion). Current status: **In Planning**. In plain terms: Integrate SOFA HRTF loading and stage per-source binaural rendering expansion to improve headphone spatial accuracy and personalization.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-046 SOFA HRTF and Binaural Expansion |
| Why is this important? | Integrate SOFA HRTF loading and stage per-source binaural rendering expansion to improve headphone spatial accuracy and personalization. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when required acceptance criteria, validation lanes, and evidence synchronization are all marked pass. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-046-sofa-hrtf-binaural-expansion.md` plus repo-local evidence under `TestEvidence/...`. |

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
| ID | BL-046 |
| Priority | P1 |
| Status | In Planning |
| Track | A - Runtime Formats |
| Effort | High / L |
| Depends On | BL-045, BL-033 (Done) |
| Blocks | — |

## Objective

Integrate SOFA HRTF loading and stage per-source binaural rendering expansion to improve headphone spatial accuracy and personalization.

## Scope

In scope:
- SOFA ingestion path (libmysofa and validation contract).
- Deterministic fallback matrix for invalid/unavailable SOFA payloads.
- Per-source binaural expansion plan and initial lane contract.

Out of scope:
- Full ambisonics v2 migration.
- Cloud profile sync.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | SOFA loader integration and fallback contract | Custom SOFA profile path validated with fallback-safe behavior |
| B | Per-source binaural pool and routing baseline | Binaural expansion lane passes and preserves RT invariants |
| C | QA + documentation closeout | Promotion packet complete with deterministic artifacts |

## TODOs

- [ ] Integrate SOFA loader path and validate required conventions.
- [ ] Wire CustomSOFA profile into headphone rendering path with deterministic fallback.
- [ ] Add per-source binaural effect pool contract and budget constraints.
- [ ] Add QA scenarios for SOFA load pass/fail and binaural parity.
- [ ] Capture closeout evidence and update runbook status.

## Validation Plan

- `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
- `./scripts/qa-bl046-sofa-binaural-lane-mac.sh --out-dir TestEvidence/bl046_<slice>_<timestamp>`
- `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl046_<slice>_<timestamp>/rt_audit.tsv`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `build.log`
- `qa_lane.log`
- `sofa_load_matrix.tsv`
- `binaural_parity.tsv`
- `rt_audit.tsv`
- `docs_freshness.log`
