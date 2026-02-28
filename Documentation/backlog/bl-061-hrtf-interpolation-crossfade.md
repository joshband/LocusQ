Title: BL-061 HRTF Interpolation + Crossfade (Phase C, conditional)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-061 HRTF Interpolation + Crossfade (Phase C, conditional)

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-061 |
| Priority | P2 |
| Status | Open (conditional on BL-060 gate pass) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-060 gate pass |
| Blocks | — |

## Objective

Replace nearest-neighbor HRIR selection with `libmysofa` continuous azimuth/elevation interpolation. Add crossfaded filter updates to eliminate zipper artifacts when source direction changes during head movement.

## Acceptance IDs

- interpolated HRTF changes produce no audible zipper
- crossfade duration ≤ 10ms
- no RT allocation during direction update
- libmysofa version pinned in CMakeLists.txt

## Validation Plan

QA harness script: `scripts/qa-bl061-hrtf-interpolation-crossfade-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl061_*/status.tsv`.
