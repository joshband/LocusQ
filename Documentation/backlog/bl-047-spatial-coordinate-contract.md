Title: BL-047 Spatial Coordinate Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-047 Spatial Coordinate Contract

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
