Title: LocusQ Documentation Standards
Document Type: Standard
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-19

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
- Validation artifacts and run logs: `TestEvidence/`

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
