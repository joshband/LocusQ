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

## Automatic Skill Selection
- Auto-select skills whenever user names a skill token or intent clearly matches a skill.
- Apply this routing order:
  1. Phase workflow skill (dream/plan/design/impl/test/ship).
  2. Specialist skill(s), minimal set only.
- Specialist routing defaults:
  - WebView host/runtime/interop defects -> `juce-webview-runtime`.
  - Audio-reactive or physics-reactive visuals -> `reactive-av`.
  - Physics/simulation-driven DSP behavior -> `physics-reactive-audio`.
  - Core 3D scene/render integration -> `threejs`.
  - Unknown/failing behavior -> `skill_troubleshooting`.
- For overlapping intents, compose skills in that order and announce selected skills.
- Reference matrix: `Documentation/skill-selection-matrix.md`.

## Skill Catalog Scope
Codex must consider the full repo skill catalog (not only specialist skills):
- `skill_dream`, `skill_plan`, `skill_design`, `skill_impl`, `skill_test`, `skill_ship`
- `skill_docs`, `skill_debug`, `skill_testing`, `skill_troubleshooting`
- `juce-webview-windows`, `juce-webview-runtime`
- `threejs`, `reactive-av`, `physics-reactive-audio`

Canonical paths and trigger guidance: `SKILLS.md` and `Documentation/skill-selection-matrix.md`.

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
