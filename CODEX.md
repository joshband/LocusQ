Title: LocusQ Codex Contract
Document Type: Agent Contract
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-09-04

# CODEX.md

Codex-specific guidance for this repository (`gpt-5.3-codex` and later).

Start with `AGENTS.md` for routing and `AGENT_RULE.md` for canonical behavior/quality rules, then apply the Codex-specific constraints below.

## Agent Role
Use APC workflows to implement and validate LocusQ while preserving phase discipline and `status.json` integrity.

## Codex 5.3 Operating Defaults
- Use high reasoning for architecture/phase transitions and multi-file DSP changes.
- Use standard reasoning for focused implementation/testing tasks.
- Prefer concise, deterministic command output and minimal diff scope.

## Execution Contract
1. Multi-agent watchdog/bootstrap is optional in this repo. Do not run `./scripts/codex-session-bootstrap.sh` unless explicitly requested.
2. Route commands through `.codex/workflows/` as defined in `AGENTS.md`.
3. See `AGENT_RULE.md` (Required Load Sequence, State Contract, Framework Gate, Routing Contract) for the rules-file load order, the `status.json` read requirement, the `ui_framework` gate, the no-auto-advance rule, and draft-only backlog automation — this file does not restate them.

## Automatic Skill Selection
- Auto-select skills whenever user names a skill token or intent clearly matches a skill.
- Apply this routing order:
  1. Phase workflow skill (dream/plan/design/impl/test/ship).
  2. Specialist skill(s), minimal set only.
- Specialist routing defaults are canonical in `SKILLS.md` and `Documentation/skill-selection-matrix.md`.
- Short routing guide:
  - UI/runtime -> `juce-webview-runtime`, `threejs`, `reactive-av`, `realtime-dimensional-visualization`
  - DSP/simulation -> `simulation-behavior-audio-visual`, `physics-reactive-audio`, `temporal-effects-engineering`
  - format/spatial -> `auv3-plugin-lifecycle`, `clap-plugin-lifecycle`, `steam-audio-capi`, `spatial-audio-engineering`
  - docs/governance -> `documentation-hygiene-expert`, `skill_docs`
  - companion/calibration -> `headtracking-companion-runtime`, `apple-spatial-companion-platform`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`
  - fallback -> `skill_troubleshooting`
- For overlapping intents, compose skills in that order and announce selected skills.
- Reference matrix: `Documentation/skill-selection-matrix.md`.

## Skill Catalog Scope
Codex must consider the full repo skill catalog, not only the short list above.
Canonical paths and triggers: `SKILLS.md` and `Documentation/skill-selection-matrix.md`.

## Build/Test Policy
See `AGENT_RULE.md` (Build and Validation) for validation ordering and the `tested` / `partially tested` / `not tested` vocabulary — this file does not restate them.

## Documentation Hygiene Policy
- Do not treat archived docs (`Documentation/archive/`) as status authority unless explicitly re-promoted.
- Keep generated doc outputs out of top-level source docs:
  - `Documentation/reports/`
  - `Documentation/exports/`
- Archive generated bundles under `Documentation/archive/<YYYY-MM-DD>-<slug>/` with a manifest.
- See `AGENT_RULE.md` (Documentation Contract) for tiered documentation authority, the skill/runtime markdown exemption, and the `validate-docs-freshness.sh` closeout gate — this file does not restate them.

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
