Title: BL-056 Calibration State Migration + Latency Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-056 Calibration State Migration + Latency Contract

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-056 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-054, BL-055 |
| Blocks | BL-059 |

## Objective

Bump plugin `state_version`, serialize new headphone calibration parameters in getStateInformation/setStateInformation. Regenerate golden state snapshots. Ensure reported latency resets to 0 on bypass.

## Acceptance IDs

- state migration is idempotent (old state loads cleanly)
- golden snapshots regenerated and committed
- latency = 0 when calibration is bypassed

## Validation Plan

QA harness script: `scripts/qa-bl056-calibration-state-migration-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl056_*/status.tsv`.
