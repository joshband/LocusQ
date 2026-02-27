Title: BL-050 High-Rate Delay and FIR Hardening
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-050 High-Rate Delay and FIR Hardening

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-050 |
| Priority | P2 |
| Status | In Planning |
| Track | F - Hardening |
| Effort | Med / M |
| Depends On | BL-043, BL-046 |
| Blocks | — |

## Objective

Harden high-sample-rate behavior by expanding delay headroom and defining a path from direct FIR convolution toward partitioned FIR for scalability.

## Scope

In scope:
- Delay max bounds suitable for 192kHz operation.
- FIR path profiling and partitioned FIR migration contract.
- Deterministic high-rate stress validation.

Out of scope:
- Full ambisonics architecture shift.
- New UI features.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Delay-range hardening for high-rate operation | Delay range validation passes at 192kHz |
| B | Partitioned FIR migration contract and prototype lane | FIR scalability metrics captured |
| C | High-rate soak and evidence closeout | High-rate replay lane passes deterministically |

## TODOs

- [ ] Validate and adjust delay max bounds for 192kHz support.
- [ ] Define partitioned FIR migration contract (latency/cpu/quality bounds).
- [ ] Add high-rate soak lane for delay + FIR paths.
- [ ] Add deterministic failure taxonomy for high-rate regressions.
- [ ] Capture promotion evidence and update runbook status.

## Validation Plan

- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
- `./scripts/qa-bl050-highrate-lane-mac.sh --out-dir TestEvidence/bl050_<slice>_<timestamp>`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `build.log`
- `highrate_matrix.tsv`
- `fir_profile.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`
