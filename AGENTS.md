Title: LocusQ Agent Dispatcher
Document Type: Agent Routing Guide
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-18

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

## Automatic Skill Routing (Codex + Claude)
- Both Codex and Claude must auto-select skills when either condition is true:
  - The user explicitly names a skill (for example `$threejs`, `$skill_docs`).
  - The task intent clearly matches a skill description in `SKILLS.md`.
- Selection method:
  1. Route to the phase workflow first (if applicable).
  2. Add the minimal specialist skills needed for the task.
  3. Keep load order: rule -> workflow -> specialist skills.
- Exemption rule: skill/runtime markdown under `.codex/skills/`, `.claude/skills/`, `.codex/workflows/`, `.claude/workflows/`, `.codex/rules/`, and `.claude/rules/` is out of scope for normal docs-hygiene/doc-governance passes unless explicitly requested.
- Specialist trigger priorities:
  - UI/runtime: `juce-webview-runtime`, `threejs`, `reactive-av`, `realtime-dimensional-visualization`
  - simulation/DSP: `simulation-behavior-audio-visual`, `physics-reactive-audio`, `temporal-effects-engineering`
  - format/runtime: `auv3-plugin-lifecycle`, `clap-plugin-lifecycle`, `steam-audio-capi`, `spatial-audio-engineering`
  - docs/governance: `documentation-hygiene-expert`, `skill_docs`
  - companion/calibration: `headtracking-companion-runtime`, `apple-spatial-companion-platform`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`
  - fallback: `skill_troubleshooting`
- When multiple skills apply, state chosen skills and order at task start.
- Canonical matrix: `Documentation/skill-selection-matrix.md`.


## Repo Skill Catalog
`SKILLS.md` is the canonical full skill catalog.
Agents must consider the full catalog there, not only the short trigger list above.

## Phase Discipline
- Enforce one phase at a time.
- Read and update `status.json` during phase work.
- Do not auto-advance phases after one command completes.
- Respect `ui_framework` in `status.json` (`visage` vs `webview`) as a hard gate.

## Core Rules
- Make scoped changes only; avoid unrelated edits.
- Do not revert user work outside requested scope.
- Prefer repository scripts over ad-hoc build flows.
- Backlog automation is draft-only unless explicitly approved: agents may run T1/T2/T3 lanes and draft packets or proposed status diffs, but owner confirmation is required before promotion or archive transitions become authoritative.
- Report validation status explicitly: `tested`, `partially tested`, or `not tested`.

## Documentation Archive Contract
- Use `Documentation/README.md` as the tiered source-of-truth map (`Tier 0..3`).
- Treat `Documentation/exports/` as generated scratch only; it must remain empty/absent at closeout.
- `Documentation/reports/` may hold active non-canonical report artifacts when intentionally referenced by current docs.
- Archive generated bundles under `Documentation/archive/<YYYY-MM-DD>-<slug>/` and record manifests in that archive set.
- When docs are archived or promoted, update both:
  - `Documentation/README.md`
  - `Documentation/standards.md`
- Run `./scripts/validate-docs-freshness.sh` before closeout; this gate now fails if generated top-level docs folders contain files.

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
