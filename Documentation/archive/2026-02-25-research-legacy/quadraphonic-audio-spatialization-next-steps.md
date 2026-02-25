Title: Quadraphonic Audio Spatialization Research Synthesis
Document Type: Research Synthesis
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# Quadraphonic Audio Spatialization Research Synthesis

## Purpose

Create a robust, assistant-friendly plan that can be consumed directly by:

- `skill_plan` for architecture decisions and planning artifacts
- `skill_design` for visual-system decisions
- `skill_impl` for deterministic implementation and acceptance closure

This document reconciles research recommendations with the current LocusQ docs and turns them into prioritized, command-ready next steps with justifications.

## Inputs Reviewed

- `Documentation/archive/2026-02-25-research-legacy/Quadraphonic Audio Spatialization.pdf`
- `tmp/pdfs/quadraphonic_audio_spatialization.txt`
- `Documentation/lessons-learned.md`
- `.ideas/creative-brief.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`

## Research Anchors (Evidence)

| Recommendation | Evidence Anchor |
|---|---|
| Freeze v1 scope and avoid expansion | `tmp/pdfs/quadraphonic_audio_spatialization.txt:6789` |
| Prefer metadata-only scene publishing model | `tmp/pdfs/quadraphonic_audio_spatialization.txt:1526` |
| DAW automation as source of truth | `tmp/pdfs/quadraphonic_audio_spatialization.txt:38`, `tmp/pdfs/quadraphonic_audio_spatialization.txt:962` |
| Shared source-of-truth for UI and audio | `tmp/pdfs/quadraphonic_audio_spatialization.txt:238` |
| Determinism is a hard requirement | `tmp/pdfs/quadraphonic_audio_spatialization.txt:242`, `tmp/pdfs/quadraphonic_audio_spatialization.txt:329` |
| Keep AI out of v1 critical path | `tmp/pdfs/quadraphonic_audio_spatialization.txt:6811` |
| Persistent viewport + adaptive control rail | `tmp/pdfs/quadraphonic_audio_spatialization.txt:159`, `tmp/pdfs/quadraphonic_audio_spatialization.txt:172` |
| Draft/Final should be enhancement, not different behavior | `tmp/pdfs/quadraphonic_audio_spatialization.txt:55`, `tmp/pdfs/quadraphonic_audio_spatialization.txt:76` |

## Decision Resolution Snapshot

1. Routing model contradiction: resolved by `Documentation/adr/ADR-0002-routing-model-v1.md` (metadata canonical + same-block audio fast path with fallback guardrails).

2. Automation authority contradiction: resolved by `Documentation/adr/ADR-0003-automation-authority-precedence.md` (DAW/APVTS base -> timeline rest pose -> physics additive offset).

3. Remaining mismatch to close:
3. Remaining mismatch to close:
- Historical mismatch (resolved): Phase 2.6 progress notes vs acceptance checkbox drift.
- Active mismatch (high priority): host-reported UI interactivity failure despite ship/test status indicating completion.

## Progress Snapshot (2026-02-19)

- `P0-1` completed: v1 scope contract frozen in `.ideas/creative-brief.md`.
- `P0-2` completed: routing model decision recorded in `Documentation/adr/ADR-0002-routing-model-v1.md`.
- `P0-3` completed: automation authority precedence recorded in `Documentation/adr/ADR-0003-automation-authority-precedence.md`.
- `P0-4` completed: scene state contract authored in `Documentation/scene-state-contract.md`.
- `P2-1` completed: v1 AI deferral recorded in `Documentation/adr/ADR-0004-v1-ai-deferral.md`.
- `P1-3` mostly completed: runtime wiring and traceability are implemented; continue regression guard as acceptance closes.
- `P1-1` completed: persistent viewport + adaptive control rail behavior locked in design artifacts and handoff package.
- `P1-2` completed: Draft/Final semantics aligned in latest design style/preview surface.
- `P1-4` completed: Phase 2.6 acceptance gates closed with full-system CPU and host edge-case evidence bundle.
- Remaining emphasis: `P2-2` closeout hygiene and CI-level trend automation.

## Prioritized Execution Matrix (Assistant-Ready)

| ID | Priority | Primary Skill | Objective | Justification | Required Artifacts | Status | Done When |
|---|---|---|---|---|---|---|---|
| P0-1 | Critical | `skill_plan` | Freeze v1 scope contract | Prevent scope and CPU drift before deeper implementation | Update `.ideas/creative-brief.md` with explicit v1 in-scope/out-of-scope section | Done | Scope section exists and is referenced from `.ideas/plan.md` |
| P0-2 | Critical | `skill_plan` | Resolve routing model via ADR | Host compatibility risk was highest unresolved architecture risk | New ADR in `Documentation/adr/` + update `.ideas/architecture.md` | Done | One canonical routing model selected with fallback behavior |
| P0-3 | Critical | `skill_plan` | Resolve automation authority model via ADR | Prevent determinism/recall ambiguity across DAWs | New ADR in `Documentation/adr/` + update `.ideas/architecture.md` and `.ideas/parameter-spec.md` | Done | Precedence rules for DAW automation, timeline, and physics are explicit |
| P0-4 | Critical | `skill_plan` | Define scene-state contract as SSOT | Research identifies this as highest leverage integration boundary | New `Documentation/scene-state-contract.md` | Done | Ownership/thread/serialization contracts documented and cross-linked |
| P1-1 | High | `skill_design` | Lock persistent viewport + adaptive control-rail behavior | Preserves spatial continuity and mental model across modes | Update `Design/HANDOFF.md` and latest `Design/vN-ui-spec.md` | Done | Mode switching changes controls/overlays only, not world/camera continuity |
| P1-2 | High | `skill_design` | Align Draft/Final visual semantics | Avoid user confusion between tiers | Update latest `Design/vN-style-guide.md` + `Design/index.html` notes | Done | Draft/Final visual cues are coherent and non-disruptive |
| P1-3 | High | `skill_impl` | Implement chosen routing + automation precedence | Moves architecture decisions into deterministic runtime behavior | Updates in `Source/` + `Documentation/implementation-traceability.md` | Partial | Parameter/control wiring matches ADR decisions end-to-end |
| P1-4 | High | `skill_impl` | Close Phase 2.6 acceptance with evidence | Plan still has open acceptance gates | Update `.ideas/plan.md` checkboxes, `status.json`, `TestEvidence/*` logs | Done | Full-system and host-edge acceptance criteria are evidenced and tracked |
| P2-1 | Medium | `skill_plan` | Gate AI to post-v1 roadmap | Prevent v1 delivery risk and nondeterministic creep | ADR + explicit roadmap note in `.ideas/plan.md` | Done | AI features moved to post-v1 phases with non-blocking status |
| P2-2 | Medium | `skill_impl` | Enforce closeout hygiene from lessons learned | Prevent recurring documentation/state drift | Updates to `status.json`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md` in each closeout | In Progress | Each `/impl` and `/test` closeout includes synchronized status+evidence updates |

## Skill-Specific Handoff Requirements

## `skill_plan` Handoff

1. Keep P0 decisions locked; reopen only through new ADRs.
2. Keep complexity and strategy in `.ideas/plan.md` aligned with ADR outcomes.
3. Update `status.json` with planning decisions and rationale after each decision package.

## `skill_design` Handoff

1. Preserve a persistent world/viewport across Calibrate/Emitter/Renderer.
2. Use contextual rail/drawer behavior per mode without camera-reset behavior.
3. Reflect Draft/Final as quality cues, not layout shifts.
4. Record approved behavior in `Design/HANDOFF.md` and keep `Design/index.html` aligned.

## `skill_impl` Handoff

1. Implement only after P0 decisions are landed.
2. Keep one-to-one parameter mapping and update `Documentation/implementation-traceability.md`.
3. Run acceptance matrix and log evidence before marking phase tasks complete.
4. Treat `status.json`, `TestEvidence/build-summary.md`, and `TestEvidence/validation-trend.md` updates as mandatory closeout gates.

## Command-Ready Next Steps

1. `/test LocusQ run focused acceptance matrix and publish trend deltas`
2. `/impl LocusQ add qa-harness CI workflow reuse (EchoForm/Memory-Echoes/Monument-Reverb pattern)`
3. Optional planning reopen only if architecture changes: `/plan LocusQ ADR update for new invariant-impacting decision`
4. Optional design reopen only for new interaction scope: `/design LocusQ refine mode rail micro-interactions`

## Integration Recovery Overlay (Host Report, 2026-02-19)

Observed in host (REAPER screenshot + user report):

1. Viewport appears black (no room/emitter render).
2. Controls appear visually present but largely non-functional (tabs, toggles, text/dropdown edits, mode transitions).
3. UX behaves like static HTML with hover-only effects.

Inference:

1. UI boot path likely fails early (for example, `Three.js` / WebGL init failure), which can prevent downstream bindings and bridge listeners from activating.
2. Current acceptance evidence is DSP/host-load strong but under-specifies real interaction viability.
3. Immediate priority is end-to-end interaction integrity, not new DSP feature scope.

## Recovery Priorities (Do This Order)

| ID | Priority | Primary Skill | Objective | Why First | Done When |
|---|---|---|---|---|---|
| R0 | Critical | `skill_plan` | Reopen implementation scope for UI-runtime reconnection | Current shipped state is not interaction-viable in host | Reopen package approved; status and plan point to recovery phases |
| R1 | Critical | `skill_impl` | Bootstrap hardening: decouple control bindings from viewport init | If WebGL fails, controls must still work | Tabs/toggles/dropdowns/keyframe controls work with viewport disabled |
| R2 | Critical | `skill_impl` | Bridge handshake and command-ack path for all user actions | Remove silent no-op behavior | Every control action has success/failure acknowledgment and state reflection |
| R3 | High | `skill_design` + `skill_impl` | Viewport interaction contract (pick/select/move emitter + camera controls) | Core product value depends on spatial manipulation | User can select/move emitter in viewport and see APVTS/state updates |
| R4 | High | `skill_impl` | Calibration UX wiring and visualization validity | Calibrate mode must be operational, not cosmetic | Start/abort/status/progress speaker visualization fully functional |
| R5 | High | `skill_impl` | Renderer/Emitter overlay coherence and mode-state continuity | Prevent mode confusion and stale overlays | Mode transitions preserve viewport/camera and accurate overlay data |
| R6 | High | `skill_test` | New UI acceptance matrix (host interactive) + trend logs | Existing suite does not block on dead UI | UI matrix passes in-host, evidence published with regression trend deltas |

## Command Sequence (APC Workflow)

1. `/plan LocusQ integration-recovery package for UI-runtime reconnection`
2. `/design LocusQ interaction contract for persistent viewport, selection, and mode overlays`
3. `/impl LocusQ R1 bootstrap hardening + bridge command acknowledgments`
4. `/test LocusQ run UI interaction smoke matrix (tabs/toggles/dropdowns/timeline)`
5. `/impl LocusQ R3 viewport emitter pick/move + camera + overlay synchronization`
6. `/test LocusQ run host interaction acceptance matrix and publish trend deltas`
7. `/impl LocusQ R4 calibrate visualization + renderer/emit mode coherence closeout`
8. `/test LocusQ full acceptance rerun (DSP + host + UI matrix) and gate ship`

## New Mandatory Acceptance Gates (Before Any Next Ship)

1. Interaction gate: no "hover-only" state allowed; tabs/toggles/dropdowns must mutate real plugin state.
2. Degraded-mode gate: if viewport fails, control rail remains fully operational and surfaces explicit diagnostic status.
3. Command-ack gate: each UI action emits an acknowledgment (or typed error) within bounded latency.
4. State-coherence gate: UI snapshot, APVTS values, and SceneGraph data must converge deterministically after each action.
5. Evidence gate: `status.json`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, and `TestEvidence/test-summary.md` must include UI matrix results.

## Guardrails

1. No new major features until P0 items are complete.
2. No architecture deviation from invariants/ADRs without a new ADR update.
3. No phase marked complete without synchronized status and evidence updates.
4. No AI orchestration in v1 critical path.

## Cross-References

- `.ideas/creative-brief.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- `Documentation/lessons-learned.md`
- `Documentation/adr/`
- `Documentation/implementation-traceability.md`
- `Documentation/archive/2026-02-25-research-legacy/Quadraphonic Audio Spatialization.pdf`
- `tmp/pdfs/quadraphonic_audio_spatialization.txt`
