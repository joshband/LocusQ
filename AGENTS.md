Title: LocusQ Agent Dispatcher
Document Type: Agent Routing Guide
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-20

# AGENTS.md

## Intent
Repository-level operating contract for AI coding agents in the standalone `LocusQ` plugin repository.

## Repo Snapshot
- Path: `/Users/artbox/Documents/Repos/LocusQ`
- Stack: JUCE 8/C++ plugin, APC workflow contracts, local QA/test evidence tracking
- Canonical routing target: `.codex/`
- Plugin state file: `status.json`

## Instruction Priority
1. User request in current session.
2. This `AGENTS.md`.
3. `CODEX.md` (Codex) or `CLAUDE.md` (Claude).
4. `.codex/rules/agent.md` plus selected workflow/skill docs.
5. Existing repository conventions and scripts.

If instructions conflict, preserve build/test stability and phase/state contracts.

## Command Routing
Slash-command routing:
- `/dream [PluginName]` -> `.codex/workflows/dream.md`
- `/plan [PluginName]` -> `.codex/workflows/plan.md`
- `/design [PluginName]` -> `.codex/workflows/design.md`
- `/impl [PluginName]` -> `.codex/workflows/impl.md`
- `/test [PluginName]` -> `.codex/workflows/test.md`
- `/ship [PluginName]` -> `.codex/workflows/ship.md`
- `/status [PluginName]` -> `.codex/workflows/status.md`
- `/resume [PluginName]` -> `.codex/workflows/resume.md`
- `/new [PluginName]` -> `.codex/workflows/new.md`

Default `[PluginName]` to `LocusQ` when omitted.

Load order for phase execution:
1. `.codex/rules/agent.md`
2. Selected workflow in `.codex/workflows/`
3. Referenced skill in `.codex/skills/`

## Phase Discipline
- Enforce one phase at a time.
- Read and update `status.json` during phase work.
- Do not auto-advance phases after one command completes.
- Respect `ui_framework` in `status.json` (`visage` vs `webview`) as a hard gate.

## Core Rules
- Make scoped changes only; avoid unrelated edits.
- Do not revert user work outside requested scope.
- Prefer repository scripts over ad-hoc build flows.
- Report validation status explicitly: `tested`, `partially tested`, or `not tested`.

## Multi-Agent Runtime (Codex, Optional)
- Disabled by default for normal Codex sessions in this repo.
- Do not run watchdog/bootstrap/thread-heartbeat flows automatically.
- Use only when explicitly requested for parallel-agent experiments or diagnostics.
- Optional session bootstrap:
  - `./scripts/codex-session-bootstrap.sh`
- Optional thread contract updates:
  - `./scripts/codex-init --thread-id <id> --task "<task>" --expected-outputs "<artifact1|artifact2>" --timeout-minutes <N> --owner <name> --role <worker|coordinator>`
- Optional heartbeat updates:
  - `./scripts/codex-init --heartbeat-only --thread-id <id> --status "WORKING <step>" --last-artifact <path-or-commit>`
- Optional closeout gate:
  - `./scripts/thread-watchdog`
- Keep all scripts/docs/artifacts for future exploration, but treat them as opt-in tooling.

## High-Value Commands
```bash
./scripts/validate-docs-freshness.sh
```

Optional multi-agent tooling:
```bash
./scripts/codex-init --help
./scripts/thread-watchdog
```

## Handoff Checklist
- Changed files are listed and scoped to request.
- Validation commands/results are reported or explicitly skipped.
- `status.json` and phase docs are updated when phase work is performed.
