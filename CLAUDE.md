Title: LocusQ Claude Contract
Document Type: Agent Contract
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-01

# CLAUDE.md

## Purpose
Claude-specific operating contract for the standalone `LocusQ` repository.
Use this file for behavior and quality rules. Use `AGENTS.md` for routing.

## Priority Order
1. User request.
2. `AGENTS.md`.
3. This file.
4. Workflow and skill files under `.codex/`.
5. Existing code conventions.

If directives conflict, preserve build/test stability, phase discipline, and state/evidence contract integrity.

## Default Mode
- Execute directly with minimal, targeted edits.
- Prefer repository scripts and workflow contracts over ad-hoc commands.
- Do not revert unrelated user changes.
- Validate with the smallest meaningful checks first.

## Required Loading Sequence
1. `.codex/rules/agent.md`
2. Selected file in `.codex/workflows/`
3. Referenced skill file in `.codex/skills/`

## Automatic Skill Selection
- Automatically load skills when:
  - The user explicitly names a skill token (for example `$threejs`, `$skill_docs`), or
  - The request clearly matches a specialist skill intent.
- Routing order:
  1. Phase workflow skill.
  2. Minimal specialist skills required by the task.
- Specialist routing defaults:
  - WebView runtime/bridge/host interop issues -> `juce-webview-runtime`.
  - Audio-reactive or physics-reactive visualization behavior -> `reactive-av`.
  - Realtime 2D/3D/4D information visualization and beautiful operator-facing UI direction -> `realtime-dimensional-visualization`.
  - Complex simulation-driven audio+visual behavior (fluid/crowd/flocking/herd) -> `simulation-behavior-audio-visual`.
  - Physics/simulation to DSP/audio behavior -> `physics-reactive-audio`.
  - Delay/echo/looper/frippertronics-style temporal DSP work -> `temporal-effects-engineering`.
  - AUv3 format lifecycle, app-extension boundaries, and AUv3 host-validation lanes -> `auv3-plugin-lifecycle`.
  - CLAP format integration/migration and host/CI validation lanes -> `clap-plugin-lifecycle`.
  - Steam Audio C API runtime loading/lifecycle/fallback work -> `steam-audio-capi`.
  - Spatial audio layout/ambisonic/binaural architecture and QA -> `spatial-audio-engineering`.
  - SDLC documentation cleanup/de-bloat/freshness remediation with ADR alignment -> `documentation-hygiene-expert` (pair `skill_docs` for governance sync).
  - Git artifact hygiene (tracked ignored paths, stale archives, history-bloat audits, pre-commit/CI guards) -> `documentation-hygiene-expert`.
  - Documentation governance metadata, ADR/invariant traceability, and root routing-contract parity -> `skill_docs`.
  - API documentation cleanup or stale code-comment hygiene -> `documentation-hygiene-expert` (pair `skill_impl` when behavior-level edits are required).
  - Companion readiness/sync/axis runtime diagnostics -> `headtracking-companion-runtime`.
  - AirPods companion Apple API/capture/privacy contract work (BL-057/BL-058) -> `apple-spatial-companion-platform`.
  - HRTF/FIR/interpolation parity and crossfade validation -> `hrtf-rendering-validation-lab`.
  - Blind listening protocol and statistical gate decisions -> `perceptual-listening-harness`.
  - 3D scene/render architecture and performance -> `threejs`.
  - Unresolved failures and diagnostics -> `skill_troubleshooting`.
- If multiple skills apply, declare selected skills and execution order in the response.
- Reference matrix: `Documentation/skill-selection-matrix.md`.

## Skill Catalog Scope
Claude must consider the full repo skill catalog:
- `skill_dream`, `skill_plan`, `skill_design`, `skill_impl`, `skill_test`, `skill_ship`
- `skill_docs`, `skill_debug`, `skill_testing`, `skill_troubleshooting`
- `documentation-hygiene-expert`
- `juce-webview-windows`, `juce-webview-runtime`
- `threejs`, `reactive-av`, `realtime-dimensional-visualization`, `simulation-behavior-audio-visual`, `physics-reactive-audio`, `temporal-effects-engineering`
- `auv3-plugin-lifecycle`, `clap-plugin-lifecycle`, `steam-audio-capi`, `spatial-audio-engineering`
- `headtracking-companion-runtime`, `apple-spatial-companion-platform`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`

Canonical paths and trigger guidance: `SKILLS.md` and `Documentation/skill-selection-matrix.md`.

## Phase Discipline
- Enforce one phase at a time.
- Read `status.json` before phase work.
- Update `status.json` as phase state changes.
- Do not auto-advance to the next phase.
- Stop after completing the requested command output.

## Framework Discipline
`ui_framework` in `status.json` is binding:
- `visage`: do not generate WebView-only UI implementation.
- `webview`: generate WebView-compatible UI paths and integration.
- `pending`: block framework-specific implementation until planning resolves it.

## Spec/Invariant/ADR Discipline
- Treat `.ideas/architecture.md`, `.ideas/parameter-spec.md`, `.ideas/plan.md`, `Documentation/invariants.md`, and `Documentation/adr/*.md` as normative references.
- Do not ship code that conflicts with documented invariants or ADR decisions.
- If a change must override an invariant/ADR, record the decision in a new ADR before closing the task.

## Expected Project Layout
Keep work inside:
- `.ideas/`
- `Design/`
- `Source/`
- `status.json`

Keep build artifacts and shipping assets in repository build/dist paths.

## Quality Contract
- Clear: explicit assumptions and scope boundaries.
- Accurate: verify claims against repository sources.
- Concise: high signal, no filler.
- Actionable: concrete next steps and outcomes.
- Defensible: key decisions include tradeoffs.

## Output Contract
For non-trivial tasks, use this response shape:
1. Recommendation or result
2. Key reasoning
3. Files changed
4. Validation status
5. Risks or follow-ups

For simple tasks, use one short paragraph or up to three bullets.

## Validation Rules
- Run targeted checks first; broaden only if needed.
- If checks are skipped, state exactly why.
- Report status as one of: `tested`, `partially tested`, `not tested`.

## Troubleshooting Rules
- Check known issues first: `.codex/troubleshooting/known-issues.yaml`.
- Reuse documented fixes when a match exists.
- If an issue is new and persistent, document it in troubleshooting artifacts.

## Documentation Hygiene
- Keep this file aligned with `AGENTS.md` and `.codex/workflows/*`.
- When workflow/skill behavior changes, update this file in the same change set.
- Enforce markdown metadata (`Title`, `Document Type`, `Author`, `Created Date`, `Last Modified Date`) for human-authored docs in root, `.codex/`, `.claude/`, `.ideas/`, `Design/`, `Documentation/`, and `TestEvidence/`.
- Treat skill/runtime markdown under `.codex/skills/`, `.claude/skills/`, `.codex/workflows/`, `.claude/workflows/`, `.codex/rules/`, and `.claude/rules/` as runtime-standard surfaces; exclude them from normal documentation-hygiene and `skill_docs` passes unless explicitly requested.
- Track validation snapshots/trends in `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md`.
