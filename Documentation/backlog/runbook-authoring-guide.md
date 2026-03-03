Title: Backlog Runbook Authoring Guide
Document Type: Guide
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-03

# Backlog Runbook Authoring Guide

## Purpose

Keep backlog docs readable, non-duplicative, and machine-friendly for humans, scripts, and coding agents.

## Scope

Applies to:
- `Documentation/backlog/bl-*.md`
- `Documentation/backlog/hx-*.md`
- `Documentation/backlog/done/*.md`
- backlog lifecycle templates under `Documentation/backlog/_template-*.md`

## Required Core Sections

Every runbook must include:
1. `## Plain-Language Summary`
2. `## 6W Snapshot (Who/What/Why/How/When/Where)`
3. `## Visual Aid Index`

## Anti-Duplication Rules

1. Keep `Plain-Language Summary` short (2-3 sentences) and avoid repeating the full `Objective` text verbatim.
2. Use `Objective` for implementation intent; use `Plain-Language Summary` for non-technical framing.
3. Prefer one canonical contract pointer over duplicated contract prose:
- lifecycle/cadence/handoff: `Documentation/backlog/index.md`
- standards/metadata/readability: `Documentation/standards.md`
4. Use `Handoff Return Contract` only when item-specific fields differ from canonical owner sync contract.
5. Use `Governance Alignment` only for item-specific exceptions; do not restate global policy.

## Visual Guidance

1. Tables are default visual format.
2. Add mermaid diagrams only when the behavior/sequence is hard to understand in text.
3. Add screenshots/charts only when they materially improve decision quality.
4. Link visual evidence to repo-local artifacts under `TestEvidence/...`.

## Done-Runbook Status Hygiene

For files under `Documentation/backlog/done/`:
1. `Status` in `Status Ledger` should read `Done` (optionally with short historical qualifier).
2. Keep pre-closeout historical context in execution/timeline sections, not in primary status state.

## Authoring Workflow

1. Scaffold new items:
   - `./scripts/new-backlog-item.py --id BL-078 --title "Example Item" --priority P1 --track "Track E - R&D Expansion"`
2. If a runbook becomes verbose, compact it:
   - `./scripts/backlog-compact-runbooks.py --mode p0p1-open --apply`
   - `./scripts/backlog-compact-runbooks.py --mode remaining-open --apply`
   - `./scripts/backlog-compact-runbooks.py --mode done-all --apply`
3. Validate before closeout:
   - `./scripts/validate-backlog-plain-language.sh`
   - `./scripts/validate-backlog-redundancy.py`
   - `./scripts/export-backlog-summaries.py --check`
   - `./scripts/validate-docs-freshness.sh`
4. Refresh machine-readable backlog summaries when runbooks/index change:
   - `./scripts/export-backlog-summaries.py`
5. Install local hook to auto-refresh summaries on staged backlog changes:
   - `./scripts/install-git-hygiene-hooks.sh`
   - Hook behavior: `.githooks/pre-commit` runs `./scripts/export-backlog-summaries.py` and stages summary artifacts when `Documentation/backlog/**` is staged.
6. Use schema contract for automation/parser updates:
   - `Documentation/backlog/backlog-summary-schema.md`

## Quick Review Checklist

- [ ] Summary is readable by non-technical stakeholders.
- [ ] 6W answers are concrete and current.
- [ ] Visual Aid Index points to real sections/artifacts.
- [ ] Repeated boilerplate is replaced by canonical pointers.
- [ ] Status semantics match folder semantics (`done/` implies `Done`).
- [ ] Validation commands and evidence paths are explicit.
