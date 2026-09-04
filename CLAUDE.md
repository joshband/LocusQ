Title: LocusQ Claude Contract
Document Type: Agent Contract
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-09-04

# CLAUDE.md

## Purpose
Claude-specific operating contract for the standalone `LocusQ` repository.
Use this file for Claude-surface specifics (skill routing, loading sequence, response shaping). Use `AGENTS.md` for routing and `AGENT_RULE.md` for the canonical behavior/quality rules — this file does not restate those.

## Priority Order
See `AGENT_RULE.md` (Priority Order) for the canonical instruction hierarchy — this file does not restate it.

## Default Mode
- Execute directly with minimal, targeted edits.
- Do not revert unrelated user changes.
- See `AGENT_RULE.md` (Build and Validation) for script-preference and smallest-check-first validation ordering — this file does not restate them.

## Required Loading Sequence
See `AGENT_RULE.md` (Required Load Sequence) — this file does not restate it.

## Automatic Skill Selection
- Automatically load skills when:
  - The user explicitly names a skill token (for example `$threejs`, `$skill_docs`), or
  - The request clearly matches a specialist skill intent.
- Routing order:
  1. Phase workflow skill.
  2. Minimal specialist skills required by the task.
- Specialist routing defaults are canonical in `SKILLS.md` and `Documentation/skill-selection-matrix.md`.
- Short routing guide:
  - UI/runtime -> `juce-webview-runtime`, `threejs`, `reactive-av`, `realtime-dimensional-visualization`
  - DSP/simulation -> `simulation-behavior-audio-visual`, `physics-reactive-audio`, `temporal-effects-engineering`
  - format/spatial -> `auv3-plugin-lifecycle`, `clap-plugin-lifecycle`, `steam-audio-capi`, `spatial-audio-engineering`
  - docs/governance -> `documentation-hygiene-expert`, `skill_docs`
  - companion/calibration -> `headtracking-companion-runtime`, `apple-spatial-companion-platform`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`
  - fallback -> `skill_troubleshooting`
- If multiple skills apply, declare selected skills and execution order in the response.
- Reference matrix: `Documentation/skill-selection-matrix.md`.

## Skill Catalog Scope
Claude must consider the full repo skill catalog, not only the short list above.
Canonical paths and triggers: `SKILLS.md` and `Documentation/skill-selection-matrix.md`.

## Phase Discipline
See `AGENT_RULE.md` (Routing Contract, State Contract) for phase-at-a-time enforcement, `status.json` read/update timing, and the no-auto-advance rule — this file does not restate them.

## Backlog Automation
See `AGENT_RULE.md` (Routing Contract) for the draft-only backlog automation rule — this file does not restate it.

## Framework Discipline
See `AGENT_RULE.md` (Framework Gate) for the binding `ui_framework` behavior — this file does not restate it.

## Spec/Invariant/ADR Discipline
See `AGENT_RULE.md` (Spec/Invariant/ADR Contract) — this file does not restate it.

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
See `AGENT_RULE.md` (Build and Validation) for check-ordering and the `tested` / `partially tested` / `not tested` vocabulary — this file does not restate them.

## Troubleshooting Rules
See `AGENT_RULE.md` (Troubleshooting Contract) — this file does not restate it.

## Documentation Hygiene
- Keep this file aligned with `AGENTS.md` and `.codex/workflows/*`.
- When workflow/skill behavior changes, update this file in the same change set.
- See `AGENT_RULE.md` (Documentation Contract) for metadata requirements, the skill/runtime markdown exemption, and validation-log locations — this file does not restate them.
