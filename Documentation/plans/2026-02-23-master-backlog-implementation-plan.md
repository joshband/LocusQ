Title: Master Backlog System Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# Master Backlog System Implementation Plan

## Purpose

Document the steady-state implementation contract for the backlog system.
This file is no longer a migration work script.

Current backlog authority:
- `Documentation/backlog/index.md`

Backlog standards:
- `Documentation/standards.md`
- `Documentation/backlog/runbook-authoring-guide.md`

Legacy migration script:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-02-23-master-backlog-implementation-plan-legacy.md`

Validation status: `not tested`

This is a governance implementation plan.
It does not claim fresh migration execution.

## Goal

Keep the backlog system stable, layered, and easy to maintain.

The active backlog system should provide:
- one canonical index,
- one runbook per BL/HX lane,
- stable annex specs,
- and synchronized status/evidence surfaces.

## Architecture Summary

The backlog system has three active layers:

| Layer | Role | Authority |
|---|---|---|
| `Documentation/backlog/index.md` | priority, status, dependency dashboard | canonical |
| `Documentation/backlog/*.md` and `done/*.md` | execution and closeout detail | runbook authority |
| `Documentation/plans/*.md` | deep design and implementation detail | supporting only |

## What Must Stay True

### 1. One Backlog Authority

Keep `Documentation/backlog/index.md` as the only backlog status authority.

Do not let these become parallel status ledgers:
- `Documentation/plans/`
- `Documentation/testing/`
- `Documentation/reports/`
- historical review packets

### 2. One Runbook Per Lane

Every active BL/HX lane should have:
- a canonical runbook,
- current status,
- dependencies,
- evidence pointers,
- and a readable summary for humans and agents.

### 3. Annex Specs Stay Supporting

Plan docs can hold deep architecture.
They should not become competing dashboards or change logs.

### 4. Done Work Moves Cleanly

When a lane becomes done:
- move it into `Documentation/backlog/done/`,
- update the backlog index,
- sync `status.json`,
- sync `TestEvidence/build-summary.md`,
- sync `TestEvidence/validation-trend.md`.

## Maintenance Actions

| Action | Trigger | Primary Surface |
|---|---|---|
| add a new backlog item | new intake promoted | `Documentation/backlog/index.md` + new runbook |
| change status or priority | phase or owner decision changes | index + runbook + status/evidence surfaces |
| compact a verbose runbook | reading tax or duplication appears | runbook itself, not a new summary doc |
| add or revise a deep spec | architecture changes | `Documentation/plans/*.md` |
| archive superseded support docs | active surface becomes noisy | `Documentation/archive/<date>-<slug>/` |

## Readiness And Done Contracts

### Definition Of Ready

A backlog lane is ready when:
1. scope is named clearly,
2. dependencies are explicit,
3. validation expectations are stated,
4. ownership is clear enough to avoid parallel-write confusion.

### Definition Of Done

A backlog lane is done when:
1. the runbook is updated or archived under `done/`,
2. the backlog index is synchronized,
3. status/evidence surfaces are synchronized,
4. freshness and readability gates pass.

## Sync Contract

For any meaningful backlog state change, update in the same change set:
- `Documentation/backlog/index.md`
- the affected runbook
- `status.json`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

Update `README.md` and `CHANGELOG.md` too when the change alters user-facing or governance-facing project posture.

## Risks

| Risk | Why it matters | Mitigation |
|---|---|---|
| plan/report drift | supporting docs can look authoritative | keep status truth in the index and runbooks |
| backlog verbosity creep | runbooks can become mini-archives | compact in place and archive deep history |
| stale machine outputs | agents/scripts lose trust in exports | keep backlog summary artifacts fresh |
| archive avoidance | active surfaces stay noisy too long | archive superseded packets aggressively |

## Evidence And Validation

Key validation commands:
- `./scripts/validate-backlog-plain-language.sh`
- `./scripts/validate-backlog-redundancy.py`
- `./scripts/export-backlog-summaries.py --check`
- `./scripts/validate-docs-freshness.sh`

Key evidence surfaces:
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

## Visual Aid Index

| Artifact | Role |
|---|---|
| `Documentation/backlog/index.md` dashboard table | active backlog view |
| `Documentation/backlog/index.md` dependency graph | dependency reference |
| `Documentation/backlog/backlog-summary-schema.md` | machine-readable summary contract |

## Archive Note

The original 2026-02-23 migration-style implementation script was preserved at:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-02-23-master-backlog-implementation-plan-legacy.md`

Use the archive copy only if you need the historical batch-by-batch migration procedure.
Use this file for the active backlog-system implementation contract.
