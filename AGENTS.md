Title: APC Codex Dispatcher (LocusQ Bridge)
Document Type: Agent Routing Guide
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

# APC Codex Dispatcher (LocusQ Bridge)

Use this guide when working in this repository with Codex models (including `gpt-5.3-codex` with `reasoning effort: xhigh`).

This repository is also linked at `audio-plugin-coder/plugins/LocusQ`. APC workflow and skill content is available locally in `.codex/`.

## Command Routing

If the user input starts with a slash command, route it to the matching APC workflow:

- `/dream [PluginName]` -> `.codex/workflows/dream.md`
- `/plan [PluginName]` -> `.codex/workflows/plan.md`
- `/design [PluginName]` -> `.codex/workflows/design.md`
- `/impl [PluginName]` -> `.codex/workflows/impl.md`
- `/test [PluginName]` -> `.codex/workflows/test.md`
- `/ship [PluginName]` -> `.codex/workflows/ship.md`
- `/status [PluginName]` -> `.codex/workflows/status.md`
- `/resume [PluginName]` -> `.codex/workflows/resume.md`
- `/new [PluginName]` -> `.codex/workflows/new.md`

Always load `.codex/rules/agent.md` first, then the selected workflow file, then any referenced skill file from `.codex/skills/`.

Default `[PluginName]` to `LocusQ` when omitted.

## Phase Discipline

- Enforce one phase at a time.
- Read and update `status.json` during phase execution.
- Do not jump to the next phase automatically after finishing one command.
- For framework-specific work, honor `ui_framework` in `status.json` (`visage` vs `webview`).

## Natural Language Mapping

When the user does not type slash commands, map clear intents to the same workflows:

- "start a plugin / ideate / brainstorm" -> dream
- "plan architecture" -> plan
- "design UI" -> design
- "implement/build code" -> impl
- "run tests/validate" -> test
- "package/release/ship" -> ship
- "what is the status" -> status
- "continue where we left off" -> resume
- "standardize docs / ADR / invariants / traceability" -> load `.codex/skills/docs/SKILL.md` in addition to the active phase workflow

## Skill Location

Use APC Codex skill content from:

- `.codex/skills/`
- `.codex/rules/`
- `.codex/guides/`
- `.codex/troubleshooting/`
- `.codex/templates/`
