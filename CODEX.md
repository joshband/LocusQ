Title: LocusQ Codex Contract
Document Type: Agent Contract
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-03-01

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
  - Realtime 2D/3D/4D information visualization and beautiful operator-facing UI direction -> `realtime-dimensional-visualization`.
  - Complex simulation-driven audio+visual behavior (fluid/crowd/flocking/herd) -> `simulation-behavior-audio-visual`.
  - Physics/simulation-driven DSP behavior -> `physics-reactive-audio`.
  - Delay/echo/looper/frippertronics-style temporal DSP work -> `temporal-effects-engineering`.
  - AUv3 format lifecycle, app-extension boundaries, and AUv3 host lanes -> `auv3-plugin-lifecycle`.
  - CLAP format integration/lifecycle/host lanes -> `clap-plugin-lifecycle`.
  - Steam Audio C API runtime/lifecycle/fallback behavior -> `steam-audio-capi`.
  - Spatial layout/ambisonic/binaural architecture and QA -> `spatial-audio-engineering`.
  - SDLC documentation cleanup/de-bloat/freshness remediation with ADR alignment -> `documentation-hygiene-expert` (pair `skill_docs` for governance sync).
  - Git artifact hygiene (tracked ignored paths, stale archives, history-bloat audits, pre-commit/CI guards) -> `documentation-hygiene-expert`.
  - Documentation governance metadata, ADR/invariant traceability, and root routing-contract parity -> `skill_docs`.
  - API documentation cleanup or stale code-comment hygiene -> `documentation-hygiene-expert` (pair `skill_impl` when behavior-level edits are required).
  - Companion readiness/sync/axis runtime diagnostics -> `headtracking-companion-runtime`.
  - AirPods companion Apple API/capture/privacy contract work (BL-057/BL-058) -> `apple-spatial-companion-platform`.
  - HRTF/FIR/interpolation parity and crossfade validation -> `hrtf-rendering-validation-lab`.
  - Blind listening protocol and statistical gate decisions -> `perceptual-listening-harness`.
  - Core 3D scene/render integration -> `threejs`.
  - Unknown/failing behavior -> `skill_troubleshooting`.
- For overlapping intents, compose skills in that order and announce selected skills.
- Reference matrix: `Documentation/skill-selection-matrix.md`.

## Skill Catalog Scope
Codex must consider the full repo skill catalog (not only specialist skills):
- `skill_dream`, `skill_plan`, `skill_design`, `skill_impl`, `skill_test`, `skill_ship`
- `skill_docs`, `skill_debug`, `skill_testing`, `skill_troubleshooting`
- `documentation-hygiene-expert`
- `juce-webview-windows`, `juce-webview-runtime`
- `threejs`, `reactive-av`, `realtime-dimensional-visualization`, `simulation-behavior-audio-visual`, `physics-reactive-audio`, `temporal-effects-engineering`
- `auv3-plugin-lifecycle`, `clap-plugin-lifecycle`, `steam-audio-capi`, `spatial-audio-engineering`
- `headtracking-companion-runtime`, `apple-spatial-companion-platform`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`

Canonical paths and trigger guidance: `SKILLS.md` and `Documentation/skill-selection-matrix.md`.

## Build/Test Policy
- Prefer project scripts and validators over ad-hoc command chains.
- Run the smallest meaningful validation first, then broaden.
- Report validation as `tested`, `partially tested`, or `not tested`.

## Documentation Hygiene Policy
- Follow tiered documentation authority in `Documentation/README.md`.
- Treat skill/runtime markdown under `.codex/skills/`, `.claude/skills/`, `.codex/workflows/`, `.claude/workflows/`, `.codex/rules/`, and `.claude/rules/` as Codex/Claude runtime-standard surfaces; do not include them in normal documentation-hygiene or `skill_docs` passes unless explicitly requested.
- Do not treat archived docs (`Documentation/archive/`) as status authority unless explicitly re-promoted.
- Keep generated doc outputs out of top-level source docs:
  - `Documentation/reports/`
  - `Documentation/exports/`
- Archive generated bundles under `Documentation/archive/<YYYY-MM-DD>-<slug>/` with a manifest.
- Before closeout, run `./scripts/validate-docs-freshness.sh` (includes guardrails for populated generated doc folders).

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
