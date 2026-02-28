Title: BL-055 FIR Convolution Engine
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-055 FIR Convolution Engine

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-055 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | — |
| Blocks | BL-056 |

## Objective

Integrate `FirEngineManager` (DirectFirConvolver ≤256 taps / PartitionedFftConvolver >256 taps, already implemented) into the monitoring chain after PEQ. Atomic engine swap on tap-count change. Report latency via `setLatencySamples()`.

## Acceptance IDs

- direct engine introduces 0 latency
- partitioned engine latency = nextPow2(blockSize)
- engine swap is glitch-free
- `setLatencySamples()` called on every engine change

## Validation Plan

QA harness script: `scripts/qa-bl055-fir-convolution-engine-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl055_*/status.tsv`.
