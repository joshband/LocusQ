Title: LocusQ v3 UI Parity Checklist
Document Type: Checklist
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# LocusQ v3 UI Parity Checklist

## Purpose
Track implementation parity against:
- `Design/v3-ui-spec.md`
- `Design/v3-style-guide.md`
- `Design/HANDOFF.md`

This checklist is the live implementation tracker. Completed items are `[x]`; open work remains `[ ]`.

## Current Baseline
- Incremental UI route: `Source/ui/public/incremental/index_stage12.html`
- Runtime logic: `Source/ui/public/incremental/js/stage12_ui.js`
- Native bridge hooks: `Source/PluginEditor.cpp`, `Source/PluginProcessor.cpp`
- Latest automated evidence:
  - `TestEvidence/locusq_incremental_stage12_selftest_20260220T175530Z.json`
  - `TestEvidence/ui_pr_gate_20260220T175530Z/status.tsv`

## v3 Contract Status
- [x] v3 design spec and style guide are finalized and approved (`Design/v3-ui-spec.md`, `Design/v3-style-guide.md`).
- [x] Persistent viewport continuity is enforced for mode switches (camera continuity check in Stage 8 self-test).
- [x] Adaptive rail width tokens are enforced (`320px / 280px / 304px`) and validated in Stage 8 self-test.
- [x] Timeline visibility is mode-scoped (Emitter visible; Calibrate/Renderer hidden) and validated in Stage 8 self-test.
- [x] Per-mode rail scroll memory is implemented and validated in Stage 8 self-test.
- [x] Scene status semantics are mode-contextual (`NO PROFILE / MEASURING / PROFILE READY / STABLE / PHYSICS / READY`).
- [x] Draft/Final quality badge behavior is stable and non-layout-shifting.
- [x] Calibrate capture flow has deterministic Start/Abort/Measure Again transitions with explicit automated checks.
- [x] Calibration routing re-detect is bridged native-side and surfaced live in UI status.
- [x] Stage 12 self-test and UI PR gate run as the default fast regression gate on macOS.

## Remaining Parity Work
- [ ] Complete manual DAW acceptance checklist for Stage 11 calibrate workflow (`TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`).
- [ ] Retire obsolete/legacy incremental UI paths after Stage 13 signoff (Stage 9-11 fallback kept intentionally during Stage 12).
- [ ] Re-run/refresh manual DAW host acceptance checklist after parity promotion (`TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`).

## Next Implementation Stages
- [x] Stage 9: Emitter rail parity completion (identity/position/audio/physics/animation/preset groups to final v3 density). Detailed checklist: `Documentation/v3-stage-9-plus-detailed-checklists.md` (`Stage 9 - Emitter Rail Parity Completion`).
- [x] Stage 10: Renderer rail parity completion (system monitoring and spatial/room/global controls at final layout). Detailed checklist: `Documentation/v3-stage-9-plus-detailed-checklists.md` (`Stage 10 - Renderer Rail Parity Completion`).
- [ ] Stage 11: Calibrate workflow parity completion (automated implementation complete; manual DAW checklist follow-up pending). Detailed checklist: `Documentation/v3-stage-9-plus-detailed-checklists.md` (`Stage 11 - Calibrate Workflow Parity Completion`).
- [x] Stage 12: Visual polish and promotion (token-level styling pass, incremental->primary route switch, debug-surface gating, rollback-safe fallback retention). Detailed checklist: `Documentation/v3-stage-9-plus-detailed-checklists.md` (`Stage 12 - Visual Polish And Primary Route Promotion`).
- [ ] Stage 13: Final acceptance sweep (automated gate + manual DAW checklist signoff + doc closeout). Detailed checklist: `Documentation/v3-stage-9-plus-detailed-checklists.md` (`Stage 13 - Final Acceptance Sweep And Closeout`).

## Skills Routing For Remaining Work
- `skill_dream`: no new ideation scope unless concept/parameter set changes.
- `skill_plan`: use for any architecture or phase-boundary replan before major parity expansions.
- `skill_design`: source of truth for final UI behavior and visual contract checks against v3 docs.
- `skill_impl`: implement bridge-safe, framework-compliant UI and processor wiring.
- `threejs`: viewport lifecycle, overlays, interaction, and performance-safe 3D behavior.
- `skill_docs`: keep this checklist, status, and evidence docs synchronized as milestones close.
