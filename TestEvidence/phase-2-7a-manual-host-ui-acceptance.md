Title: LocusQ Phase 2.7a Manual Host UI Acceptance
Document Type: Test Evidence Checklist
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# Phase 2.7a Manual Host UI Acceptance

## Purpose
Capture in-host DAW UI checks required to close Phase 2.7 acceptance items that are not provable through harness/pluginval alone.

## Scope
- Plugin: `LocusQ`
- Host: operator-selected DAW session(s) on macOS
- Phase gate: `.ideas/plan.md` Phase 2.7 acceptance criteria

## Normative References
- `.ideas/plan.md`
- `.ideas/architecture.md`
- `Documentation/invariants.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`

## Result Legend
- `PASS`: behavior matches expected result
- `FAIL`: behavior diverges or is non-functional
- `N/A`: not applicable in current host/session configuration
- `PENDING`: not yet executed in a manual DAW host session

## Operator Run Context (Fill Before Running)

| Field | Value |
|---|---|
| Operator | |
| Run Date (UTC) | |
| Host (DAW) | |
| Host Version | |
| macOS Version | |
| Plugin Version | `v0.1.0` |
| Plugin Format Under Test | |
| Plugin Binary Path | |
| Session Sample Rate / Block Size | |
| Notes | |

## Manual Host UI Checklist (2.7 Closeout Sheet)

| ID | Check | Steps | Expected Result | Manual Result | Run Date (UTC) | Host (DAW) | Host Version | Prefilled Objective Observation (from existing evidence) | Evidence Paths |
|---|---|---|---|---|---|---|---|---|---|
| UI-01 | Mode tab interaction | Click `Calibrate`, `Emitter`, `Renderer` tabs repeatedly | Active tab and rail panel switch correctly with no stuck state | `PENDING` | | | | 2.7c control-path/state-sync closure states tabs are routed through relays/attachments; focused smoke suite is green. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-02 | Quality badge toggle | Click `DRAFT/FINAL` badge | Badge toggles and renderer quality state updates with no JS freeze | `PENDING` | | | | JS syntax gate passed after 2.7c/2.7d refresh and smoke checks are green; no manual DAW click-path evidence yet. | `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_js_check.log`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-03 | Rail toggle controls | Toggle `Mute`, `Solo`, `Animation`, `Loop`, `Sync` | Switches change visual state and persist correctly after tab changes | `PENDING` | | | | 2.7c notes record rail/header controls now routed via relays/attachments with state-sync closure. | `plugins/LocusQ/status.json` |
| UI-04 | Dropdown controls | Change `Mode`, `Physics preset`, `Animation source` dropdowns | Selection changes apply immediately and do not break UI | `PENDING` | | | | 2.7c notes include dropdown relay/attachment routing plus physics preset persistence via native UI-state bridge. | `plugins/LocusQ/status.json` |
| UI-05 | Text input edit | Edit emitter label text box | Text entry accepts changes and preserves edited value | `PENDING` | | | | 2.7c notes explicitly mention `emit-label` persistence through native UI-state bridge. | `plugins/LocusQ/status.json` |
| UI-06 | Timeline transport | Use rewind/stop/play controls | Timeline time updates correctly; play/stop behavior is coherent | `PENDING` | | | | 2.7d notes confirm timeline bridge hardening (reject non-finite time and clamp to duration); animation smoke passes. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_animation_smoke.log` |
| UI-07 | Timeline keyframe editing | Add/move/delete keyframes; cycle curve on dbl-click | Keyframe edits are reflected visually and remain stable after mode switches | `PENDING` | | | | Phase 2.6 acceptance suite is green in focused 2.7d rerun, but in-host keyframe gesture path still requires manual DAW confirmation. | `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_phase_2_6_acceptance_suite.log` |
| UI-08 | Viewpoint controls | Use P/T/F/S view buttons and orbit/pan/zoom in viewport | Camera controls respond or viewport degrades gracefully without disabling rail | `PENDING` | | | | 2.7b/2.7d notes describe emitter pick/select/move and Cartesian drag writeback via APVTS; host-edge proxy scenario passes at `48k/512`. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_host_edge_48k512.log` |
| UI-09 | Viewport degraded-path resilience | If viewport fails, continue interacting with rail controls | Rail remains fully interactive with visible degraded message | `PENDING` | | | | 2.7a runtime hardening explicitly moved viewport/WebGL failures into degraded mode while keeping rail bindings active. | `plugins/LocusQ/TestEvidence/build-summary.md` |
| UI-10 | Calibration start/abort | In `Calibrate`, press `START MEASURE`, then abort | Start/abort controls work and status text/meter update coherently | `PENDING` | | | | 2.7b notes confirm calibration speaker/meter/profile visualization now consumes native status payloads; manual host interaction still pending. | `plugins/LocusQ/status.json` |
| UI-11 | Profile/status indicators | Observe `scene-status`, viewport info, profile indicators across modes | Status labels update consistently and match current mode/action | `PENDING` | | | | 2.7b calibration/profile status path was wired and 2.7d smoke remains pass; DAW-session indicator behavior still requires manual evidence. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-12 | Persistence sanity | Save/load preset after edits (timeline + toggles) | Loaded state restores expected settings without UI lockup | `PENDING` | | | | Preset persistence is listed as implemented in Phase 2.6 and current acceptance suite rerun is pass; in-host save/load UX still needs manual verification. | `plugins/LocusQ/README.md`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_phase_2_6_acceptance_suite.log` |

## Pass/Fail Rollup (Fill After Running)

| Verdict Bucket | Count | Checklist IDs |
|---|---|---|
| PASS | | |
| FAIL | | |
| N/A | | |
| PENDING | 12 | UI-01..UI-12 |

- Final manual host UI verdict (manual-only): `PENDING`
- Ready for 2.7 closeout when: no `FAIL`, all non-`N/A` checks are `PASS`, and host/version fields are populated.

## Notes

- If any check fails, include exact control ID/location and a short repro sequence.
- Capture evidence for each executed row; minimum is one screenshot or clip path per failed row.
- Recommended: capture one screenshot per functional section (`tabs`, `timeline`, `viewport`, `calibration`).
- This sheet intentionally pre-fills only objective, non-manual observations from existing logs; it does not claim DAW click-path completion.
