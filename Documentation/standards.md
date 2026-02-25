Title: LocusQ Documentation Standards
Document Type: Standard
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-25

# Documentation Standards

## Scope
Applies to human-authored markdown in:
- repository root (`AGENTS.md`, `AGENT_RULE.md`, `CLAUDE.md`, `SKILLS.md`)
- `.codex/`
- `.claude/`
- `.ideas/`
- `Design/`
- `Documentation/`
- `TestEvidence/`

Generated markdown under `qa_output/` is exempt.

## Required Metadata Header
Every in-scope markdown file must include, in this order, at the top of file:
1. `Title`
2. `Document Type`
3. `Author`
4. `Created Date`
5. `Last Modified Date`

## Naming Conventions
- Use lowercase kebab-case for new docs, except canonical legacy files already in use.
- ADR names must follow: `ADR-XXXX-kebab-case.md` (zero-padded index).
- Keep versioned design docs in `Design/` as `vN-ui-spec.md` and `vN-style-guide.md`.

## Folder Placement
- Concepts/specs: `.ideas/`
- UI design artifacts: `Design/`
- Stable reference docs/ADRs/invariants/traceability: `Documentation/`
- Execution runbooks: `Documentation/runbooks/`
- Validation artifacts and run logs: `TestEvidence/`

## Source-Of-Truth Tiering
- Tier 0 canonical docs are listed in `Documentation/README.md` and are the only authority for status/closeout claims.
- Tier 1 docs are active execution specs and may drive implementation detail, but they must not supersede Tier 0 status surfaces.
- Tier 2 docs are historical/research references and are non-authoritative.
- Tier 3 docs are archived artifacts under `Documentation/archive/`.

## Master Backlog Contract
1. `Documentation/backlog/index.md` is the single backlog authority for priority, ordering, and state.
2. Individual runbook docs in `Documentation/backlog/` carry execution detail, agent mega-prompts, validation plans, and evidence contracts (`bl-XXX-*.md` for open work, `done/*.md` for completed work).
3. Plan docs under `Documentation/plans/` carry deep architecture content but must not become competing backlog ledgers.
4. New backlog items enter via `Documentation/backlog/_template-intake.md` and are promoted to full runbooks using `Documentation/backlog/_template-runbook.md`.
5. The legacy files `Documentation/backlog-post-v1-agentic-sprints.md` and `Documentation/runbooks/backlog-execution-runbooks.md` are superseded and retained as Tier 2 reference only.

## Cross-Reference Requirements
When code behavior changes, updated docs must reference:
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`
- relevant `Documentation/adr/*.md`

## Status And Task Hygiene
- Keep task checkboxes current in `.ideas/plan.md`.
- Keep phase status aligned in `status.json`.
- Avoid duplicative “summary” docs when a canonical file already exists.

## Phase Closeout Freshness Gate
Per `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`, any phase closeout that changes acceptance/status claims must update this canonical bundle in the same change set:
- `status.json`
- `Documentation/backlog-post-v1-agentic-sprints.md`
- `README.md`
- `CHANGELOG.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

## Validation Logging
- Snapshot: update `TestEvidence/build-summary.md` after meaningful build/test runs.
- Trend: append an entry to `TestEvidence/validation-trend.md` for each meaningful run.

## API Documentation
- Doxygen is the preferred API doc generator for C++ source comments.
- Use Doxygen-style comments (`/** ... */`) for public classes and nontrivial methods.
- Recommended command: `doxygen Documentation/Doxyfile`

## Minimalism Rule
Prefer updating canonical docs over creating new files. New docs require a clear owner and purpose.

## Artifact Tracking Rule
Apply artifact tracking and retention policy from `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`:
1. classify by artifact class first;
2. keep generated/heavy artifacts local-only by default;
3. track only canonical decision-grade evidence.

## Archival Rule
When documentation bloat or ambiguity appears:
1. Classify docs into Tier 0-3 (per `Documentation/README.md`).
2. Move generated snapshots and one-off operational bundles into `Documentation/archive/<YYYY-MM-DD>-<slug>/`.
3. Keep top-level generated scratch directory `Documentation/exports/` empty or absent; archive its outputs instead.
4. Keep `Documentation/reports/` for active report artifacts that are intentionally referenceable from current docs.
5. Keep historical docs in-place only if active docs/status surfaces still reference them; otherwise archive them.
6. Update `Documentation/README.md` in the same change to reflect any tier changes.
7. Run `./scripts/validate-docs-freshness.sh` after archival edits.
8. Keep only active research under `Documentation/research/`, index it in `Documentation/research/README.md`, and move superseded research to `Documentation/archive/<YYYY-MM-DD>-<slug>/`.

## Tier Promotion Snapshot (2026-02-24)
1. Tier 1 execution specs now include `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md`.
2. Tier 1 execution specs now include `Documentation/plans/bl-031-tempo-locked-visual-token-scheduler-spec-2026-02-24.md`.
2. Tier 1 execution specs include `Documentation/runbooks/backlog-execution-runbooks.md` as the procedural companion to the master backlog.
