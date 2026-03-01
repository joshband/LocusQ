Title: LocusQ Documentation Index
Document Type: Documentation Index
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-01


# Documentation Index

## Purpose
Keep documentation lean, canonical, and current while preserving traceability from specs to code to validation evidence.
Backlog model: `Documentation/backlog/index.md` is the single backlog catalog authority; individual runbook docs in `Documentation/backlog/` hold execution detail and agent prompts; lifecycle templates (`_template-intake.md`, `_template-runbook.md`, `_template-promotion-decision.md`, `_template-closeout.md`) are the required process contract for future and remaining backlog work; annex plan specs in `Documentation/plans/` hold deep architecture.

## Source-of-Truth Tiers

### Tier 0: Canonical (authoritative)
Only these documents are normative for implementation, release status, and closeout claims:
- `README.md`
- `CHANGELOG.md`
- `ARCHITECTURE.md`
- `status.json`
- `Documentation/standards.md`
- `Documentation/invariants.md`
- `Documentation/scene-state-contract.md`
- `Documentation/implementation-traceability.md`
- `Documentation/backlog/index.md`
- `Documentation/skill-selection-matrix.md`
- `Documentation/spatial-audio-profiles-usage.md`
- `Documentation/adr/` (all ADR files)
- `Documentation/adr-index.md`
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
- `Documentation/archive/2026-03-01-architecture-review-consolidation/`
- `Documentation/archive/2026-03-01-build-summary-compaction/`
- `Documentation/archive/2026-03-01-validation-trend-compaction/`
- `Documentation/lessons-learned.md`
- `Documentation/plans/2026-02-20-full-project-review.md`
- `Documentation/archive/2026-02-23-clap-reference-bundle/`
- `Documentation/backlog-post-v1-agentic-sprints.md` (superseded by `Documentation/backlog/index.md`)
- `Documentation/runbooks/backlog-execution-runbooks.md` (superseded by individual runbook docs)

### Tier 3: Archived (generated and operational snapshots)
- `Documentation/archive/`
- Archived docs are discoverable and preserved, but excluded from source-of-truth decisions unless explicitly re-promoted.

## Freshness Ownership Contract (2026-03-01)

Default ownership/cadence for critical docs:

| Surface | Role | Default Owner | Review Cadence | Trigger Conditions |
|---|---|---|---|---|
| `Documentation/backlog/index.md` | backlog/status authority | active lane owner + owner/orchestrator | on each state transition | intake, promotion, done/archive moves |
| `status.json` | runtime/phase authority | active lane owner + owner/orchestrator | on each phase/status update | phase change, release posture change, authoritative gate decision |
| `README.md` | operator-facing project contract | documentation hygiene owner | weekly and before release decisions | behavior/routing/posture claim changes |
| `CHANGELOG.md` | chronological change contract | implementation owner + documentation hygiene owner | per meaningful merged change set | added/changed/fixed user-visible or governance behavior |
| `AGENTS.md`, `CODEX.md`, `CLAUDE.md`, `SKILLS.md`, `AGENT_RULE.md` | routing/governance contract | docs governance owner | when skill/routing/execution posture changes | new skill, trigger change, contract override |
| `TestEvidence/build-summary.md` | closeout snapshot authority | docs governance owner | after meaningful validation/governance passes | closeout or governance reconciliation updates |
| `TestEvidence/validation-trend.md` | run-history trend authority | docs governance owner | after meaningful validation/governance passes | new gate runs, reconciliations, release readiness checks |

Escalation path:
- If a trigger condition is missed or ownership is unclear, escalate to the owner/orchestrator.
- Record resolution and new owner/timing in:
  - `TestEvidence/build-summary.md`
  - `TestEvidence/validation-trend.md`

## Archive Governance Snapshot (2026-03-01)
- Top-level generated scratch remains constrained:
  - `Documentation/exports/` must stay empty/absent at closeout.
  - `Documentation/reports/` is active non-canonical report surface only.
- Historical narratives and one-off bundles are preserved under `Documentation/archive/`.
- Legacy backlog companions (`Documentation/backlog-post-v1-agentic-sprints.md`, `Documentation/runbooks/backlog-execution-runbooks.md`) remain Tier 2 reference-only.

## Normative Inputs For Implementation
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- Relevant `Documentation/adr/*.md`

## Exclusions
- `qa_output/` markdown reports are generated artifacts and are not manually normalized for metadata.
- Skill/runtime markdown under `.codex/skills/`, `.claude/skills/`, `.codex/workflows/`, `.claude/workflows/`, `.codex/rules/`, and `.claude/rules/` follows Codex/Claude runtime standards and is excluded from documentation-hygiene/skill_docs normalization unless explicitly requested.

## Compliance Contract (Current)
- All non-generated markdown files are metadata-compliant.
- Generated markdown under `qa_output/` remains intentionally unmanaged.
- Root `README.md` and `CHANGELOG.md` are canonical Tier 0 surfaces.
- Top-level `Documentation/exports/` remains scratch-only and is blocked by closeout guard checks.
- `Documentation/reports/` is an active non-canonical report surface (archive snapshots remain under `Documentation/archive/`).
- Phase closeout updates are gated by ADR-0005 and validated via `scripts/validate-docs-freshness.sh`.
- Current acceptance/status claims must resolve through:
  - `Documentation/backlog/index.md`
  - `status.json`
  - `TestEvidence/build-summary.md`
  - `TestEvidence/validation-trend.md`

## Closeout Sync Snapshot (2026-02-28)

- BL-023 done-transition sync recorded:
  - open runbook moved to `Documentation/backlog/done/bl-023-resize-dpi-hardening.md`
  - master index row updated at `Documentation/backlog/index.md`
  - canonical promotion packet remains `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z/`

## Architecture Consolidation Snapshot (2026-03-01)

- `ARCHITECTURE.md` is the active architecture source-of-truth document.
- Prior standalone architecture review docs were archived to:
  - `Documentation/archive/2026-03-01-architecture-review-consolidation/reviews/2026-02-26-full-architecture-review.md`
  - `Documentation/archive/2026-03-01-architecture-review-consolidation/reviews/LocusQ Repo Review 02262026.md`

## Build Summary Compaction Snapshot (2026-03-01)

- `TestEvidence/build-summary.md` was compacted to current governance and closeout highlights for faster review.
- Full historical narrative was archived to:
  - `Documentation/archive/2026-03-01-build-summary-compaction/build-summary-legacy-2026-03-01.md`

## Validation Trend Compaction Snapshot (2026-03-01)

- `TestEvidence/validation-trend.md` was compacted to a recent high-signal trend window for faster review.
- Full historical trend chronology was archived to:
  - `Documentation/archive/2026-03-01-validation-trend-compaction/validation-trend-legacy-2026-03-01.md`
