Title: LocusQ Claude Contract
Document Type: Agent Contract
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-19

# CLAUDE.md

## Purpose
Claude-specific operating contract for the standalone `LocusQ` repository.
Use this file for behavior and quality rules. Use `AGENTS.md` for routing.

## Priority Order
1. User request.
2. Safety and correctness.
3. This file.
4. `AGENTS.md`.
5. `CODEX.md`.
6. Workflow and skill files under `.codex/`.
7. Existing code conventions.

## Default Mode
- Execute directly with minimal, targeted edits.
- Prefer repository scripts and workflow contracts over ad-hoc commands.
- Do not revert unrelated user changes.
- Validate with the smallest meaningful checks first.

## Required Loading Sequence
1. `.codex/rules/agent.md`
2. Selected file in `.codex/workflows/`
3. Referenced skill file in `.codex/skills/`

## Phase Discipline
- Enforce one phase at a time.
- Read `status.json` before phase work.
- Update `status.json` as phase state changes.
- Do not auto-advance to the next phase.
- Stop after completing the requested command output.

## Framework Discipline
`ui_framework` in `status.json` is binding:
- `visage`: do not generate WebView-only UI implementation.
- `webview`: generate WebView-compatible UI paths and integration.
- `pending`: block framework-specific implementation until planning resolves it.

## Spec/Invariant/ADR Discipline
- Treat `.ideas/architecture.md`, `.ideas/parameter-spec.md`, `.ideas/plan.md`, `Documentation/invariants.md`, and `Documentation/adr/*.md` as normative references.
- Do not ship code that conflicts with documented invariants or ADR decisions.
- If a change must override an invariant/ADR, record the decision in a new ADR before closing the task.

## Expected Project Layout
Keep work inside:
- `.ideas/`
- `Design/`
- `Source/`
- `status.json`

Keep build artifacts and shipping assets in repository build/dist paths.

## Quality Contract
- Clear: explicit assumptions and scope boundaries.
- Accurate: verify claims against repository sources.
- Concise: high signal, no filler.
- Actionable: concrete next steps and outcomes.
- Defensible: key decisions include tradeoffs.

## Output Contract
For non-trivial tasks, use this response shape:
1. Recommendation or result
2. Key reasoning
3. Files changed
4. Validation status
5. Risks or follow-ups

For simple tasks, use one short paragraph or up to three bullets.

## Validation Rules
- Run targeted checks first; broaden only if needed.
- If checks are skipped, state exactly why.
- Report status as one of: `tested`, `partially tested`, `not tested`.

## Troubleshooting Rules
- Check known issues first: `.codex/troubleshooting/known-issues.yaml`.
- Reuse documented fixes when a match exists.
- If an issue is new and persistent, document it in troubleshooting artifacts.

## Documentation Hygiene
- Keep this file aligned with `AGENTS.md` and `.codex/workflows/*`.
- When workflow/skill behavior changes, update this file in the same change set.
- Enforce markdown metadata (`Title`, `Document Type`, `Author`, `Created Date`, `Last Modified Date`) for human-authored docs in root, `.codex/`, `.claude/`, `.ideas/`, `Design/`, `Documentation/`, and `TestEvidence/`.
- Track validation snapshots/trends in `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md`.
