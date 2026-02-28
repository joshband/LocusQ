Title: BL-060 Phase B Listening Test Harness + Evaluation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-060 Phase B Listening Test Harness + Evaluation

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-060 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-059 |
| Blocks | BL-061 (conditional) |

## Objective

Execute Phase B 2×2 blind listening test (generic vs personalized HRTF × no EQ vs WH-1000XM5 EQ) across ≥5 participants × ≥10 scenes. Run statistical analysis. Gate: ≥20% mean externalization improvement OR p<0.05 localization gain.

## Acceptance IDs

- ≥5 participants complete full session
- analysis script exits 0
- Phase B gate result recorded in `verification` fields of CalibrationProfile
- result documented in TestEvidence

## Validation Plan

QA harness script: `scripts/qa-bl060-phase-b-listening-test-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl060_*/status.tsv`.
