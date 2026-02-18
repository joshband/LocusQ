# AGENT_RULE.md

## Purpose
Canonical APC agent rule contract.

This file is the single source for:
- `.codex/rules/agent.md`
- `.claude/rules/agent.md`

Do not edit those copies directly. After editing this file, run:
`./scripts/sync-agent-contract.sh` (or `pwsh ./scripts/sync-agent-contract.ps1`)

## Priority Order
1. User request
2. Safety and correctness
3. `AGENTS.md` routing rules
4. This file
5. Active workflow and skill instructions
6. Existing repository conventions

## Routing Contract
- Slash commands and natural-language intent map through `AGENTS.md`.
- Enforce one phase at a time.
- Do not auto-advance to the next phase when a phase completes.

## Required Load Sequence
For phase execution, always load in this order:
1. This rule file (`rules/agent.md`)
2. Matching workflow in `../workflows/`
3. Referenced skill in `../skills/`

## State Contract
Before phase work:
- Read `plugins/[PluginName]/status.json`.
- Validate prerequisites with `scripts/state-management.ps1` and `Test-PluginState`.

During phase work:
- Keep edits in phase scope.
- Update state through state-management functions.

After phase work:
- Validate required artifacts exist.
- Stop and report next expected command.

## Framework Gate
`ui_framework` in `status.json` is binding:
- `visage`: avoid WebView-only implementation outputs.
- `webview`: use WebView-compatible UI and integration paths.
- `pending`: block framework-specific implementation until resolved in planning.

## Build and Validation
- Prefer repository workflows/scripts over ad-hoc command sequences.
- Run the smallest meaningful validation first, then broaden as needed.
- If validation is skipped, state why.
- Report status as: `tested`, `partially tested`, or `not tested`.

## Troubleshooting Contract
- Check known issues first: `../troubleshooting/known-issues.yaml`.
- Reuse documented resolutions when a match exists.
- If failures repeat, capture issue details and document the fix.

## Response Quality
- Lead with result/recommendation.
- Keep reasoning concise and explicit.
- Reference changed files and validation status.
