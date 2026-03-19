Title: Backlog Runbook Authoring Guide
Document Type: Guide
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-18

# Backlog Runbook Authoring Guide

## Purpose

Keep backlog docs short, current, and easy to parse for both humans and automation.

## Scope

Applies to:
- `Documentation/backlog/bl-*.md`
- `Documentation/backlog/hx-*.md`
- `Documentation/backlog/done/*.md`
- `Documentation/backlog/_template-*.md`

## Required Sections

Every runbook must include:
1. `## Plain-Language Summary`
2. `## 6W Snapshot (Who/What/Why/How/When/Where)`
3. `## Visual Aid Index`

## Smart Brevity Rules

Per `Documentation/adr/ADR-0021-smart-brevity-documentation-contract.md`:

1. Lead with the current decision or current need.
2. Keep sentences short.
3. Keep most paragraphs to `1-3` sentences.
4. Prefer tables and bullets over long narrative.
5. Use concrete labels:
   - status
   - dependency
   - evidence path
   - next action
6. Replace repeated governance prose with pointers to:
   - `Documentation/backlog/index.md`
   - `Documentation/standards.md`

## Anti-Duplication Rules

1. Keep `Plain-Language Summary` short.
2. Do not repeat the full `Objective` text in the summary.
3. Use `Objective` for intent, not status history.
4. Use one canonical pointer instead of restating global policy.
5. Keep replay history in milestone tables, evidence packets, or archive bundles, not long diary sections.
6. Use `Governance Alignment` only for item-specific exceptions.

## Automation Rules

1. Default mode is `draft_only`.
2. Automation may run T1, T2, and T3 for packet drafting.
3. Automation must not finalize promotion, archive moves, or status transitions by default.
4. Use `MANUAL_ONLY` for participant studies, signing, host inventory, or other human-gated lanes.
5. Use `DRAFT_READY`, `BLOCKED`, or `MANUAL_ONLY` as the result token.

## Visual Rules

1. Tables are the default visual.
2. Add diagrams only when they reduce ambiguity.
3. Add screenshots or charts only when they improve decision quality.
4. Link visual evidence to repo-local artifacts under `TestEvidence/...`.

## Status Formatting

1. Use portable markdown only.
2. Prefer `[DONE]`, `[ACTIVE]`, `[NEXT]`, `[QUEUED]`, `[DEFERRED]`, `[BLOCKED]`.
3. Use `~~strikethrough~~` only when it improves scan speed.
4. If time or token telemetry is unknown, write `not logged` or `n/a`.

## Done-Runbook Rule

For files under `Documentation/backlog/done/`:
- `Status` should read `Done`.
- Historical detail belongs in evidence, milestone tables, or archive, not the main status line.

## Authoring Workflow

1. Scaffold new work:
   - `./scripts/new-backlog-item.py --id BL-078 --title "Example Item" --priority P1 --track "Track E - R&D Expansion"`
2. Compact verbose runbooks when needed:
   - `./scripts/backlog-compact-runbooks.py --mode p0p1-open --apply`
   - `./scripts/backlog-compact-runbooks.py --mode remaining-open --apply`
   - `./scripts/backlog-compact-runbooks.py --mode done-all --apply`
3. Validate before closeout:
   - `./scripts/validate-backlog-plain-language.sh`
   - `./scripts/validate-backlog-redundancy.py`
   - `./scripts/export-backlog-summaries.py --check`
   - `./scripts/validate-docs-freshness.sh`
4. Refresh machine-readable summaries after backlog changes:
   - `./scripts/export-backlog-summaries.py`

## Quick Review Checklist

- [ ] Summary is readable by non-technical readers.
- [ ] 6W answers are current.
- [ ] Visual Aid Index points to real sections or artifacts.
- [ ] Repeated boilerplate is replaced by pointers.
- [ ] Status semantics match folder semantics.
- [ ] Validation commands and evidence paths are explicit.
