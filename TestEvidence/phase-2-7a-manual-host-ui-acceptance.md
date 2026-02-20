Title: LocusQ Phase 2.7a Manual Host UI Acceptance
Document Type: Test Evidence Checklist
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-20

# Phase 2.7a Manual Host UI Acceptance

## Purpose
Capture in-host DAW UI checks required to close Phase 2.7 acceptance items that are not provable through harness/pluginval alone.

## Scope
- Plugin: `LocusQ`
- Host: operator-selected DAW session(s) on macOS
- Phase gate: `.ideas/plan.md` Phase 2.7 acceptance criteria

## Bridge-Fix Rerun Status (2026-02-19)

- A host interaction bridge fix landed after the first manual run:
  - module-based JS loading removed for in-host compatibility (`index.html`, `js/juce/index.js`, `js/index.js`)
  - macOS WebBrowser backend/build flags corrected (`Source/PluginEditor.cpp`, `CMakeLists.txt`)
- Non-manual acceptance rerun after the fix is green with warn-only residuals (`qa_output/suite_result.json`).
- Manual DAW checklist rows below still reflect the last executed manual run and now require rerun/signoff in host.

## Stage 13 Manual Rerun Handoff (2026-02-20)

- Automated Stage 13 sweep passed (`TestEvidence/stage13_acceptance_sweep_20260220T180204Z/status.tsv`).
- Stage 12 self-test/gate and non-UI parity suites are green on promoted artifacts.
- Manual DAW rerun is still required to close Stage 13:
  - fill operator context table
  - rerun checklist rows UI-01 through UI-12 in target DAW
  - execute portable-device rows DEV-01 through DEV-06 (laptop speakers/mic/headphones)
- update rollup and final manual verdict

## Stage 17-A Portable Acceptance Rerun Handoff (2026-02-20)

- Stage 17-A prerequisite build completed:
  - command: `./scripts/build-and-install-mac.sh`
  - result: `PASS`
  - evidence: `TestEvidence/stage17a_portable_acceptance_20260220T231840Z/build_and_install.log`
- Manual rerun required to close Stage 17-A gate:
  - rerun portable-device rows `DEV-01..DEV-06` in Standalone + Reaper.
  - focus on headphone and laptop speaker profiles (ADR-0006 contract gate).
  - if any DEV row fails, block GA and file a defect issue with repro + evidence path.
- Current Stage 17-A status: `PENDING_OPERATOR_RERUN`.

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
| Operator | Josh Band |
| Run Date (UTC) | 2026-02-20T21:42:35Z |
| Host (DAW) | Reaper |
| Host Version | v7.61 |
| macOS Version | Tahoe 26.3 |
| Plugin Version | `v0.1.0` |
| Plugin Format Under Test | VST3 |
| Plugin Binary Path | /Users/artbox/Library/Audio/Plug-Ins/VST3/LocusQ.vst3/Contents/MacOS/LocusQ |
| Session Sample Rate / Block Size | 48kHz / 512 |
| Notes | |

## Manual Host UI Checklist (2.7 Closeout Sheet)

| ID | Check | Steps | Expected Result | Manual Result | Run Date (UTC) | Host (DAW) | Host Version | Prefilled Objective Observation (from existing evidence) | Evidence Paths |
|---|---|---|---|---|---|---|---|---|---|
| UI-01 | Mode tab interaction | Click `Calibrate`, `Emitter`, `Renderer` tabs repeatedly | Active tab and rail panel switch correctly with no stuck state | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | Operator PASS in host session; mode tab switching stable. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-02 | Quality badge toggle | Click `DRAFT/FINAL` badge | Badge toggles and renderer quality state updates with no JS freeze | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS with hit-target usability note: clicks near badge text can miss; cursor reposition improves registration. | `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_js_check.log`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-03 | Rail toggle controls | Toggle `Mute`, `Solo`, `Animation`, `Loop`, `Sync` | Switches change visual state and persist correctly after tab changes | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | UI state PASS. Audio response for these controls requires Renderer instance on main channel (expected design behavior). | `plugins/LocusQ/status.json` |
| UI-04 | Dropdown controls | Change `Mode`, `Physics preset`, `Animation source` dropdowns | Selection changes apply immediately and do not break UI | `FAIL` | 2026-02-20T22:11:16Z | Reaper | v7.61 | `Physics preset` selections (e.g., `Orbit`) quickly revert to `Custom`; `Animation Source` and `Mode` remain sticky. | `plugins/LocusQ/status.json` |
| UI-05 | Text input edit | Edit emitter label text box | Text entry accepts changes and preserves edited value | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session; emitter label text entry and persistence behavior confirmed. | `plugins/LocusQ/status.json` |
| UI-06 | Timeline transport | Use rewind/stop/play controls | Timeline time updates correctly; play/stop behavior is coherent | `FAIL` | 2026-02-20T22:11:16Z | Reaper | v7.61 | FAIL: timeline transport controls are not present in plugin UI, so rewind/stop/play cannot be executed manually. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_animation_smoke.log` |
| UI-07 | Timeline keyframe editing | Add/move/delete keyframes; cycle curve on dbl-click | Keyframe edits are reflected visually and remain stable after mode switches | `FAIL` | 2026-02-20T22:11:16Z | Reaper | v7.61 | FAIL: keyframe editing controls are not present in current plugin UI, so manual keyframe gesture path cannot be executed. | `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_phase_2_6_acceptance_suite.log` |
| UI-08 | Viewpoint controls | Use P/T/F/S view buttons and orbit/pan/zoom in viewport | Camera controls respond or viewport degrades gracefully without disabling rail | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session for available viewpoint controls. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_host_edge_48k512.log` |
| UI-09 | Viewport degraded-path resilience | If viewport fails, continue interacting with rail controls | Rail remains fully interactive with visible degraded message | `N/A` | 2026-02-20T22:11:16Z | Reaper | v7.61 | N/A: degraded viewport message/state was not observed during this run. | `plugins/LocusQ/TestEvidence/build-summary.md` |
| UI-10 | Calibration start/abort | In `Calibrate`, press `START MEASURE`, then abort | Start/abort controls work and status text/meter update coherently | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session; start/abort path behaved coherently. | `plugins/LocusQ/status.json` |
| UI-11 | Profile/status indicators | Observe `scene-status`, viewport info, profile indicators across modes | Status labels update consistently and match current mode/action | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session; status indicators tracked mode/action changes. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-12 | Persistence sanity | Save/load preset after edits (timeline + toggles) | Loaded state restores expected settings without UI lockup | `FAIL` | 2026-02-20T22:11:16Z | Reaper | v7.61 | FAIL: preset `SAVE` button appears non-functional in host session (no visible save effect). | `plugins/LocusQ/README.md`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_phase_2_6_acceptance_suite.log` |

## Portable Device Profile Checklist (Stage 14 Addendum)

| ID | Check | Steps | Expected Result | Manual Result | Run Date (UTC) | Host (DAW/Standalone) | Host Version | Notes | Evidence Paths |
|---|---|---|---|---|---|---|---|---|---|
| DEV-01 | Standalone laptop speaker playback | Launch standalone, play reference material through built-in speakers | Audible output is stable, no UI/input lockup, no non-finite artifacts | `PASS` | 2026-02-20T22:08:23Z | Standalone | v0.1.0 | Operator confirmed completion in built-in speaker profile with stable playback. | |
| DEV-02 | Standalone headphone playback | Connect headphones and replay same material | Audible output is stable and level controls remain responsive | `PASS` | 2026-02-20T22:08:23Z | Standalone | v0.1.0 | Operator reported PASS during headphone profile run. | |
| DEV-03 | DAW laptop speaker playback | Run plugin in host session and monitor via built-in speakers | In-host playback remains stable with working controls and expected monitoring behavior | `PASS` | 2026-02-20T22:08:23Z | Reaper | v7.61 | Operator reported PASS in DAW speaker profile. | |
| DEV-04 | DAW headphone playback | Monitor same host session via headphones | In-host playback remains stable with no transport/control regressions | `PASS` | 2026-02-20T22:08:23Z | Reaper | v7.61 | PASS after DAW routing/setup adjustment; emitter controls verified effective in correct mode/path. | |
| DEV-05 | Built-in mic calibration route | In Calibrate mode, select built-in mic channel and run start/abort cycle | Calibration status transitions remain coherent and abort path is deterministic | `PASS` | 2026-02-20T22:08:23Z | Reaper | v7.61 | PASS (operator-confirmed). Built-in mic start/abort transitions remained coherent. | |
| DEV-06 | External mic calibration route (if available) | Select external mic channel and run start/abort cycle | Calibration status transitions remain coherent and routing reflects expected input | `N/A` | 2026-02-20T22:08:23Z | Reaper | v7.61 | External mic unavailable for this run. | |

## Pass/Fail Rollup (Fill After Running)

| Verdict Bucket | Count | Checklist IDs |
|---|---|---|
| PASS | 12 | UI-01, UI-02, UI-03, UI-05, UI-08, UI-10, UI-11, DEV-01, DEV-02, DEV-03, DEV-04, DEV-05 |
| FAIL | 4 | UI-04, UI-06, UI-07, UI-12 |
| N/A | 2 | UI-09, DEV-06 |
| PENDING | 0 | |

- Final manual host UI verdict (manual-only): `FAIL`
- Ready for 2.7 closeout when: no `FAIL`, all non-`N/A` checks are `PASS`, and host/version fields are populated.

## Notes

- If any check fails, include exact control ID/location and a short repro sequence.
- Capture evidence for each executed row; minimum is one screenshot or clip path per failed row.
- Recommended: capture one screenshot per functional section (`tabs`, `timeline`, `viewport`, `calibration`).
- This sheet intentionally pre-fills only objective, non-manual observations from existing logs; it does not claim DAW click-path completion.
- Operator summary from this run: most controls remained non-interactive (hover cursor/visual only), while dropdown and emitter-label text input paths were functional.
