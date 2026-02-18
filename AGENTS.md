# APC Codex Dispatcher (LocusQ Bridge)

Use this guide when working in this repository with Codex models (including `gpt-5.3-codex` with `reasoning effort: xhigh`).

This repository is also linked at `audio-plugin-coder/plugins/LocusQ`. Load APC workflow and skill content from:

- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/rules/`
- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/`
- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/skills/`

## Command Routing

If the user input starts with a slash command, route it to the matching APC workflow:

- `/dream [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/dream.md`
- `/plan [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/plan.md`
- `/design [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/design.md`
- `/impl [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/impl.md`
- `/test [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/test.md`
- `/ship [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/ship.md`
- `/status [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/status.md`
- `/resume [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/resume.md`
- `/new [PluginName]` -> `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/workflows/new.md`

Always load `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/rules/agent.md` first, then the selected workflow file, then any referenced skill file from `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/skills/`.

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

## Skill Location

Use APC Codex skill content from:

- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/skills/`
- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/rules/`
- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/guides/`
- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/troubleshooting/`
- `/Users/artbox/Documents/Repos/audio-plugin-coder/.codex/templates/`
