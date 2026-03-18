Title: LocusQ Test Evidence Index
Document Type: Evidence Index
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# Test Evidence Index

## Purpose

Keep `TestEvidence/` readable.
This folder is not one flat authority surface.

Use it in this order:
- `build-summary.md` for the current governance snapshot
- `validation-trend.md` for the recent run window
- referenced decision packets for item-specific proof

## Classification

| Class | Role | Examples |
|---|---|---|
| canonical | Tier 0 authority surfaces | `build-summary.md`, `validation-trend.md` |
| active decision packet | current promotion, closeout, or validation packet | `bl057_promotion_t3_closeout/`, `bl076_promotion_t3_closeout/`, `ui_ux_trust_wave_owner_sync_z1_20260318T040618Z/` |
| supporting reference | still referenced by current status or older closeout chains | `phase-2-7a-manual-host-ui-acceptance.md`, `test-summary.md`, `clap-validation-report-2026-02-22.md` |
| debug or replay exhaust | machine-generated support files, not active authority by themselves | `locusq_production_p0_selftest_*.meta.json`, `*.attempts.tsv`, `*.run.log`, `*.payload_failure_snippet.json` |
| archive candidate | historical packet family no longer needed in active closeout decisions | older timestamped packet directories after their decision is absorbed elsewhere |

## Current Rules

- Keep `build-summary.md` short and summary-first.
- Keep `validation-trend.md` as a short recent window, not a full lifetime log.
- Prefer packet directories over loose top-level files for new evidence.
- Treat repeated selftest snippet families as debug support, not decision-grade documentation.
- Keep root-level loose files only when they are still referenced by canonical docs or `status.json`.

## Packet Shape

Preferred active packet shape:
- `status.tsv`
- `report.md` or `promotion_decision.md`
- one primary machine-readable result file
- optional logs only when they materially help triage

## Archive Direction

Use `Documentation/archive/<YYYY-MM-DD>-<slug>/testevidence/` for:
- superseded build-summary and validation-trend copies
- retired evidence summaries
- historical decision packets that no longer need an active path

Do not mass-move timestamped files that are still referenced by their own packet metadata unless the whole packet family moves together.
