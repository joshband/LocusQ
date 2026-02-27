Title: BL-046 SOFA HRTF and Binaural Expansion
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-046 SOFA HRTF and Binaural Expansion

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
