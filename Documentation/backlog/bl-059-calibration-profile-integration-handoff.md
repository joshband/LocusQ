Title: BL-059 CalibrationProfile Integration Handoff
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-059 CalibrationProfile Integration Handoff

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-059 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-052, BL-053, BL-054, BL-055, BL-056, BL-057, BL-058 |
| Blocks | BL-060 |

## Objective

Wire `CalibrationProfile.json` from companion to plugin state end-to-end. Primitive fields → APVTS parameters. Blob fields (sofa_ref, hp_fir_taps) → base64 in state. Plugin reloads profile on file change without glitches.

## Acceptance IDs

- profile load/unload cycle is stable (no glitches)
- SOFA swap is atomic
- APVTS params update on profile change
- smoke test `qa-bl059-calibration-integration-smoke-mac.sh` exits 0

## Validation Plan

QA harness script: `scripts/qa-bl059-calibration-integration-smoke-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl059_*/status.tsv`.
