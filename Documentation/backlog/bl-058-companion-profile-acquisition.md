Title: BL-058 Companion Profile Acquisition UI + HRTF Matching
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-058 Companion Profile Acquisition UI + HRTF Matching

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-058 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-057 |
| Blocks | BL-059 |

## Objective

Build guided ear-photo capture UI in companion app (left ear + right ear + frontal). Run MobileNetV3 embedding + cosine similarity against SADIE II subjects. Write selected `subject_id` and `sofa_ref` to `CalibrationProfile.json`. Discard images after embedding.

## Acceptance IDs

- matching completes in <50ms on M-series Mac
- fallback subject used when similarity <0.6
- images not persisted to disk after embedding
- privacy: no network calls

## Validation Plan

QA harness script: `scripts/qa-bl058-companion-profile-acquisition-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl058_*/status.tsv`.
