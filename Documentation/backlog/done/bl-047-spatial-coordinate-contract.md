Title: BL-047 Spatial Coordinate Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-02

# BL-047 Spatial Coordinate Contract

## Plain-Language Summary

This runbook tracks **BL-047** (BL-047 Spatial Coordinate Contract). Current status: **In Planning**. In plain terms: Document and enforce a single canonical coordinate-system contract across renderer math, head-tracking transforms, and viewport representation to prevent silent axis/sign drift.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-047 Spatial Coordinate Contract |
| Why is this important? | Document and enforce a single canonical coordinate-system contract across renderer math, head-tracking transforms, and viewport representation to prevent silent axis/sign drift. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when required acceptance criteria, validation lanes, and evidence synchronization are all marked pass. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-047-spatial-coordinate-contract.md` plus repo-local evidence under `TestEvidence/...`. |

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
| ID | BL-047 |
| Priority | P1 |
| Status | In Planning |
| Track | E - R&D Expansion |
| Effort | Low / S |
| Depends On | BL-018 (Done), BL-045 |
| Blocks | — |

## Objective

Document and enforce a single canonical coordinate-system contract across renderer math, head-tracking transforms, and viewport representation to prevent silent axis/sign drift.

## Scope

In scope:
- Canonical axis/orientation definitions and conversion rules.
- Assertion/test points for azimuth/elevation/forward conventions.
- Contract docs + QA checks.

Out of scope:
- Spatial algorithm redesign.
- UI style changes.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Coordinate contract doc + acceptance IDs | Contract published and cross-referenced |
| B | Native/UI assertion and parity checks | Convention mismatches produce deterministic failure codes |
| C | QA lane closeout | Coordinate parity lane passes on target profiles |

## TODOs

- [ ] Publish canonical coordinate system and transform mapping contract.
- [ ] Add deterministic assertions for forward/up/right conversions.
- [ ] Add azimuth/elevation parity checks for renderer and UI diagnostics.
- [ ] Add taxonomy for coordinate mismatch failures.
- [ ] Capture evidence and owner-ready closeout packet.

## Validation Plan

- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
- `./scripts/qa-bl047-coordinate-contract-lane-mac.sh --out-dir TestEvidence/bl047_<slice>_<timestamp>`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `build.log`
- `qa_lane.log`
- `coordinate_parity.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`
