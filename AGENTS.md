Title: LocusQ Agent Dispatcher
Document Type: Agent Routing Guide
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-09-04

# AGENTS.md

## Intent
Repository-level operating contract for AI coding agents in the standalone `LocusQ` plugin repository.

## Repo Snapshot
- Path: `/Users/artbox/Documents/Repos/LocusQ`
- Stack: JUCE 8/C++ plugin, APC workflow contracts, local QA/test evidence tracking
- Canonical routing target: `.codex/`
- Plugin state file: `status.json`

## Instruction Priority
See `AGENT_RULE.md` (Priority Order) for the canonical instruction hierarchy — this file does not restate it.

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

See `AGENT_RULE.md` (Required Load Sequence) for the rules -> workflow -> skill load order — this file does not restate it.

## Automatic Skill Routing (Codex + Claude)
- Both Codex and Claude must auto-select skills when either condition is true:
  - The user explicitly names a skill (for example `$threejs`, `$skill_docs`).
  - The task intent clearly matches a skill description in `SKILLS.md`.
- Selection method:
  1. Route to the phase workflow first (if applicable).
  2. Add the minimal specialist skills needed for the task.
  3. Keep load order: rule -> workflow -> specialist skills.
- Specialist trigger priorities:
  - UI/runtime: `juce-webview-runtime`, `threejs`, `reactive-av`, `realtime-dimensional-visualization`
  - simulation/DSP: `simulation-behavior-audio-visual`, `physics-reactive-audio`, `temporal-effects-engineering`
  - format/runtime: `auv3-plugin-lifecycle`, `clap-plugin-lifecycle`, `steam-audio-capi`, `spatial-audio-engineering`
  - docs/governance: `documentation-hygiene-expert`, `skill_docs`
  - companion/calibration: `headtracking-companion-runtime`, `apple-spatial-companion-platform`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`
  - fallback: `skill_troubleshooting`
- When multiple skills apply, state chosen skills and order at task start.
- Canonical matrix: `Documentation/skill-selection-matrix.md`.
- See `AGENT_RULE.md` (Documentation Contract) for the skill/runtime markdown exemption from docs-hygiene/doc-governance passes — this file does not restate it.


## Repo Skill Catalog
`SKILLS.md` is the canonical full skill catalog.
Agents must consider the full catalog there, not only the short trigger list above.

## Phase Discipline
See `AGENT_RULE.md` (Routing Contract, State Contract, Framework Gate) for phase-at-a-time enforcement, `status.json` read/update timing, the no-auto-advance rule, and the `ui_framework` gate — this file does not restate them.

## Core Rules
- Do not revert user work outside requested scope.
- See `AGENT_RULE.md` for scoped-edit discipline, script-preference, draft-only backlog automation, and the `tested` / `partially tested` / `not tested` validation vocabulary — this file does not restate them.

## Documentation Archive Contract
- When docs are archived or promoted, update both:
  - `Documentation/README.md`
  - `Documentation/standards.md`
- See `AGENT_RULE.md` (Documentation Contract) for source-of-truth tiers, `Documentation/exports/` and `Documentation/reports/` handling, archive-bundle naming, and the `validate-docs-freshness.sh` closeout gate — this file does not restate them.

## Root Docs Sync
When execution posture, routing, or acceptance claims change, keep these root docs aligned in the same change set:
- `README.md`
- `CHANGELOG.md`
- `AGENTS.md`
- `CODEX.md`
- `CLAUDE.md`
- `SKILLS.md`
- `AGENT_RULE.md` (then sync with: `cp AGENT_RULE.md .codex/rules/agent.md && cp AGENT_RULE.md .claude/rules/agent.md`)

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
