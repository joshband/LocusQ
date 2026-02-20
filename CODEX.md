Title: LocusQ Codex Contract
Document Type: Agent Contract
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-20

# CODEX.md

Codex-specific guidance for this repository (`gpt-5.3-codex` and later).

Start with `AGENTS.md`, then apply the constraints below.

## Agent Role
Use APC workflows to implement and validate LocusQ while preserving phase discipline and `status.json` integrity.

## Codex 5.3 Operating Defaults
- Use high reasoning for architecture/phase transitions and multi-file DSP changes.
- Use standard reasoning for focused implementation/testing tasks.
- Prefer concise, deterministic command output and minimal diff scope.

## Execution Contract
1. Multi-agent watchdog/bootstrap is optional in this repo. Do not run `./scripts/codex-session-bootstrap.sh` unless explicitly requested.
2. Route commands through `.codex/workflows/` as defined in `AGENTS.md`.
3. Always load `.codex/rules/agent.md` first for phase work.
4. Read `status.json` before edits.
5. Honor `ui_framework` gate (`visage`/`webview`) and do not mix UI paradigms.
6. Do not auto-advance to the next phase after finishing one command.

## Build/Test Policy
- Prefer project scripts and validators over ad-hoc command chains.
- Run the smallest meaningful validation first, then broaden.
- Report validation as `tested`, `partially tested`, or `not tested`.

## High-Value Paths
- Workflows: `.codex/workflows/`
- Rules: `.codex/rules/`
- Skills: `.codex/skills/`
- Troubleshooting DB: `.codex/troubleshooting/known-issues.yaml`
- Plugin state: `status.json`
- Implementation: `Source/`
- Design/notes: `.ideas/`, `Documentation/`, `TestEvidence/`

## Guardrails
- Keep diffs scoped; avoid opportunistic refactors.
- Do not modify generated artifacts unless requested.
- Do not commit secrets or local machine credentials.
