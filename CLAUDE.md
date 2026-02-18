# CLAUDE.md

## Purpose
Assistant operating contract for the `audio-plugin-coder` repository.
Use this file for behavior and quality rules. Use `AGENTS.md` for command routing.

## Priority Order
1. User request
2. Safety and correctness
3. This file
4. `AGENTS.md`
5. Workflow and skill files under `.codex/`
6. Existing code conventions

## Default Mode
- Execute directly with minimal, targeted edits.
- Prefer repository scripts and workflow contracts over ad-hoc commands.
- Do not revert unrelated user changes.
- Validate with the smallest meaningful checks first.

## Command Routing
If input starts with a slash command, route to:
- `/dream [PluginName]` -> `.codex/workflows/dream.md`
- `/plan [PluginName]` -> `.codex/workflows/plan.md`
- `/design [PluginName]` -> `.codex/workflows/design.md`
- `/impl [PluginName]` -> `.codex/workflows/impl.md`
- `/test [PluginName]` -> `.codex/workflows/test.md`
- `/ship [PluginName]` -> `.codex/workflows/ship.md`
- `/status [PluginName]` -> `.codex/workflows/status.md`
- `/resume [PluginName]` -> `.codex/workflows/resume.md`
- `/new [PluginName]` -> `.codex/workflows/new.md`

For clear natural-language intent, map to the same workflows.

## Required Loading Sequence
For workflow execution, always load in this order:
1. `.codex/rules/agent.md`
2. Selected file in `.codex/workflows/`
3. Referenced skill file in `.codex/skills/`

## Phase Discipline
- Enforce one phase at a time.
- Read `plugins/[PluginName]/status.json` before phase work.
- Update `status.json` as phase state changes.
- Do not auto-advance to the next phase.
- Stop after completing the requested command output.

## Framework Discipline
`ui_framework` in `status.json` is binding:
- `visage`: do not generate WebView-only UI implementation.
- `webview`: generate WebView-compatible UI paths and integration.
- `pending`: block framework-specific implementation until planning resolves it.

## Expected Project Layout
Per plugin, keep work inside:
- `plugins/[PluginName]/.ideas/`
- `plugins/[PluginName]/Design/`
- `plugins/[PluginName]/Source/`
- `plugins/[PluginName]/status.json`

Keep build artifacts and shipping assets in repository-level build and dist paths.

## Quality Contract
Every response and code change should be:
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
- Keep rule parity through `AGENT_RULE.md` and run `./scripts/sync-agent-contract.sh` after contract edits.
