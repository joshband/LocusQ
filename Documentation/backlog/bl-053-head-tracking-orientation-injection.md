Title: BL-053 Head Tracking Orientation Injection
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-053 Head Tracking Orientation Injection

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-053 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-052, BL-045 |
| Blocks | BL-059 |

## Objective

Inject head pose quaternion from the companion bridge into `SteamAudioVirtualSurround` as an `IPLCoordinateSpace3`. Apply yaw offset from `CalibrationProfile.json`. Fallback to identity when companion is disconnected.

## Acceptance IDs

- head rotation updates HRTF direction within one processBlock
- null fallback is silent (no glitch)
- yaw offset is applied correctly

## Validation Plan

QA harness script: `scripts/qa-bl053-head-tracking-orientation-injection-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl053_*/status.tsv`.
