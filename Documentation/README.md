Title: LocusQ Documentation Index
Document Type: Documentation Index
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-28


# Documentation Index

## Purpose
Keep documentation lean, canonical, and current while preserving traceability from specs to code to validation evidence.
Backlog model: `Documentation/backlog/index.md` is the single backlog catalog authority; individual runbook docs in `Documentation/backlog/` hold execution detail and agent prompts; lifecycle templates (`_template-intake.md`, `_template-runbook.md`, `_template-promotion-decision.md`, `_template-closeout.md`) are the required process contract for future and remaining backlog work; annex plan specs in `Documentation/plans/` hold deep architecture.

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
- `Documentation/backlog/index.md`
- `Documentation/skill-selection-matrix.md`
- `Documentation/spatial-audio-profiles-usage.md`
- `Documentation/adr/` (all ADR files)
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

### Tier 1: Active Execution Specs (current cycle, non-canonical status authority)
Used for implementation planning and validation flow, but status truth remains Tier 0:
- `Documentation/backlog/*.md` (individual runbook docs with agent prompts and validation plans)
- `Documentation/backlog/_template-promotion-decision.md` (owner/orchestrator promotion packet template)
- `Documentation/backlog/_template-intake.md` (new backlog intake contract)
- `Documentation/backlog/_template-runbook.md` (execution/runbook contract)
- `Documentation/backlog/_template-closeout.md` (done transition contract)
- `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md`
- `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md`
- `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md`
- `Documentation/plans/bl-029-audition-platform-expansion-plan-2026-02-24.md`
- `Documentation/plans/bl-031-tempo-locked-visual-token-scheduler-spec-2026-02-24.md`
- `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`
- `Documentation/reports/` (active report artifacts and companion data/visual bundles)
- `Documentation/testing/`
- `Documentation/testing/production-selftest-and-reaper-headless-smoke-guide.md`

### Tier 2: Reference (historical/research context, not authoritative)
Reference-only docs are retained for traceability but are not status authority:
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-2026-02-20.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-design-2026-02-20.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/stage14-comprehensive-review-2026-02-20.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/stage14-review-release-checklist.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/v3-ui-parity-checklist.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/v3-stage-9-plus-detailed-checklists.md`
- `Documentation/research/README.md` (active research index)
- `Documentation/research/` (active research files only)
- `Documentation/archive/2026-02-25-research-legacy/`
- `Documentation/archive/2026-02-24-multi-agent-thread-watchdog/`
- `Documentation/lessons-learned.md`
- `Documentation/plans/2026-02-20-full-project-review.md`
- `Documentation/archive/2026-02-23-clap-reference-bundle/`
- `Documentation/backlog-post-v1-agentic-sprints.md` (superseded by `Documentation/backlog/index.md`)
- `Documentation/runbooks/backlog-execution-runbooks.md` (superseded by individual runbook docs)

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
8. Archived superseded quadraphonic and harness research artifacts into `Documentation/archive/2026-02-25-research-legacy/`; retained only active calibration research under `Documentation/research/`.

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
- Latest acceptance-claim sync: `BL-019` is `Done (2026-02-23)` with refreshed physics-lens evidence (`TestEvidence/locusq_production_p0_selftest_20260223T171542Z.json`, `TestEvidence/locusq_smoke_suite_spatial_bl019_20260223T121613.log`, `TestEvidence/validate_docs_freshness_bl019_20260223T122029_postsync.log`) and synchronized Tier 0 status/evidence surfaces (`status.json`, `Documentation/backlog-post-v1-agentic-sprints.md`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, `README.md`, `CHANGELOG.md`).
- HX-04 closure sync: deterministic scenario-audit evidence is captured at `TestEvidence/hx04_scenario_audit_20260223T172312Z/status.tsv` with BL-012 embedded enforcement evidence at `TestEvidence/bl012_harness_backport_20260223T172301Z/status.tsv`.
