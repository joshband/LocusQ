Title: BL-054 PEQ Cascade RT Integration
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-054 PEQ Cascade RT Integration

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-054 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-052 |
| Blocks | BL-056 |

## Objective

Integrate `PeqBiquadCascade` (8-band RBJ, already implemented) into the monitoring chain after Steam Audio binaural output. Coefficient updates via off-thread atomic swap. Load preset from `CalibrationProfile.json` on profile change.

## Acceptance IDs

- PEQ applies in processBlock with no allocation
- coefficients swap atomically on non-RT thread
- bypass path produces identical output to no-PEQ path

## Validation Plan

QA harness script: `scripts/qa-bl054-peq-cascade-rt-integration-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl054_*/status.tsv`.
