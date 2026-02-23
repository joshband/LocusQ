Title: LocusQ Documentation Index
Document Type: Documentation Index
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-23

# Documentation Index

## Purpose
Keep documentation lean, canonical, and current while preserving traceability from specs to code to validation evidence.

## Source-of-Truth Tiers

### Tier 0: Canonical (authoritative)
Only these documents are normative for implementation, release status, and closeout claims:
- `README.md`
- `CHANGELOG.md`
- `status.json`
- `Documentation/standards.md`
- `Documentation/invariants.md`
- `Documentation/scene-state-contract.md`
- `Documentation/implementation-traceability.md`
- `Documentation/backlog-post-v1-agentic-sprints.md`
- `Documentation/skill-selection-matrix.md`
- `Documentation/spatial-audio-profiles-usage.md`
- `Documentation/adr/` (all ADR files)
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

### Tier 1: Active Execution Specs (current cycle, non-canonical status authority)
Used for implementation planning and validation flow, but status truth remains Tier 0:
- `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md`
- `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`
- `Documentation/reports/` (active report artifacts and companion data/visual bundles)
- `Documentation/testing/`

### Tier 2: Reference (historical/research context, not authoritative)
Reference-only docs are retained for traceability but are not status authority:
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-2026-02-20.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-design-2026-02-20.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/stage14-comprehensive-review-2026-02-20.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/stage14-review-release-checklist.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/v3-ui-parity-checklist.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/v3-stage-9-plus-detailed-checklists.md`
- `Documentation/research/`
- `Documentation/multi-agent-thread-watchdog.md`
- `Documentation/lessons-learned.md`
- `Documentation/plans/2026-02-20-full-project-review.md`
- `Documentation/archive/2026-02-23-clap-reference-bundle/`

### Tier 3: Archived (generated and operational snapshots)
- `Documentation/archive/`
- Archived docs are discoverable and preserved, but excluded from source-of-truth decisions unless explicitly re-promoted.

## 2026-02-23 Archival Pass
Completed:
1. Moved generated exports and an operational report snapshot out of top-level `Documentation/` into:
- `Documentation/archive/2026-02-23-ops-artifacts/exports/`
- `Documentation/archive/2026-02-23-ops-artifacts/reports-final/`
2. Tightened this index so source-of-truth is explicitly Tier 0.
3. Archived historical full-review and checklist bundles into:
- `Documentation/archive/2026-02-23-historical-review-bundles/`
4. Rewired active references (backlog/README/status/evidence/docs) to archived paths.
5. Enforced docs-freshness guard for scratch output: populated top-level `Documentation/exports/` fails closeout checks.
6. Re-promoted the 2026-02-23 executive-brief report set into active `Documentation/reports/` for non-archived access.
7. Archived CLAP reference markdown/PDF artifacts into `Documentation/archive/2026-02-23-clap-reference-bundle/` and promoted one canonical BL-011 CLAP closeout plan.

## Normative Inputs For Implementation
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- Relevant `Documentation/adr/*.md`

## Exclusions
- `qa_output/` markdown reports are generated artifacts and are not manually normalized for metadata.

## Compliance Snapshot (2026-02-23)
- All non-generated markdown files are metadata-compliant.
- Generated markdown under `qa_output/` remains intentionally unmanaged.
- Root `README.md` and `CHANGELOG.md` are canonical Tier 0 surfaces.
- Top-level `Documentation/exports/` remains scratch-only and is blocked by closeout guard checks.
- `Documentation/reports/` is an active non-canonical report surface (archive snapshots remain under `Documentation/archive/`).
- Phase closeout updates are gated by ADR-0005 and validated via `scripts/validate-docs-freshness.sh`.
- Latest acceptance-claim sync: `BL-015` is `Done (2026-02-23)` with refreshed all-emitter baseline evidence (`TestEvidence/locusq_production_p0_selftest_20260223T034704Z.json`, `TestEvidence/locusq_smoke_suite_spatial_bl015_20260223T034751Z.log`) and synchronized Tier 0 status/evidence surfaces (`status.json`, `Documentation/backlog-post-v1-agentic-sprints.md`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, `README.md`, `CHANGELOG.md`).
