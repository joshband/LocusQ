Title: Root Docs Maintenance Loop Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Root Docs Maintenance Loop

## Purpose
Keep root documentation accurate and synchronized with current implementation and governance posture.

## Root Docs In Scope
- `README.md`
- `CHANGELOG.md`
- `AGENTS.md`
- `CODEX.md`
- `CLAUDE.md`
- `SKILLS.md`
- `AGENT_RULE.md`

## Loop
1. Check claims in `README.md` and `CHANGELOG.md` against current code/backlog/evidence.
2. Check routing/skill claims in `AGENTS.md`, `CODEX.md`, `CLAUDE.md`, and `SKILLS.md` for parity.
3. Check rule contract statements in `AGENT_RULE.md` and sync to:
   - `.codex/rules/agent.md`
   - `.claude/rules/agent.md`
4. Record what changed and why in the docs update summary.

## Failure Patterns
- Root docs mention skills/features not present in repository.
- Changelog claims done work without matching evidence/runbook updates.
- Agent-routing docs diverge between root and parity copies.
