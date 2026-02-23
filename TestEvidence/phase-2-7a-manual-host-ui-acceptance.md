Title: LocusQ Phase 2.7a Manual Host UI Acceptance
Document Type: Test Evidence Checklist
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-23

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

## P0 Host UI Rerun Handoff (2026-02-21)

- Production UI entrypoint now defaults to `Source/ui/public/index.html` in host/runtime.
- Incremental stage12 remains available for self-test/debug only.
- Manual rerun required to close P0 host UI defects:
  - `UI-04` physics preset stickiness (`Orbit`/`Float`/`Bounce` must not auto-revert to `Custom`).
  - `UI-06` timeline transport controls (`rewind/stop/play`) present and functional.
  - `UI-07` timeline keyframe gestures (add/move/delete/curve cycle) functional and persistent.
  - `UI-12` preset `SAVE`/`LOAD` behavior visibly functional in host session.

## P0 Blocker-Only Rerun Quick Sheet (2026-02-21)

1. Install latest local build before DAW run:
   - `./scripts/build-and-install-mac.sh`
2. Open a fresh DAW session (recommended baseline: Reaper `48kHz / 512`).
3. Execute only blocker rows in this order: `UI-04` -> `UI-06` -> `UI-07` -> `UI-12`.
4. For each blocker row, capture one screenshot/clip and add an evidence path in the checklist table.
5. If all four blocker rows pass, update rollup counts and final verdict at the bottom of this file.

### Blocker Row Focus

| ID | Manual Focus | Pass Signal |
|---|---|---|
| UI-04 | Change `Physics preset` to `Orbit`, `Float`, `Bounce`; switch tabs and return | Preset stays selected and does not auto-revert to `Custom` |
| UI-06 | Use `rewind`, `play`, `stop` buttons in timeline rail | Controls are visible and transport state/time changes coherently |
| UI-07 | Add keyframe, drag to new time/value, delete keyframe, double-click for curve cycle | Gesture path is functional and edits persist after mode/tab switches |
| UI-12 | Edit a control state, click `SAVE`, then `LOAD` | Saved preset appears in selector and restores edited state |

### Blocker-Only Rerun Result (2026-02-21)

- `UI-04`: `PASS`
- `UI-06`: `PASS`
- `UI-07`: `PASS`
- `UI-12`: `PASS`
- Net: blocker rerun is fully green; manual 2.7 closeout blockers are cleared.

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
| Run Date (UTC) | 2026-02-21T09:26:24Z |
| Host (DAW) | Reaper |
| Host Version | v7.61 |
| macOS Version | Tahoe 26.3 |
| Plugin Version | `v1.0.0-ga` |
| Plugin Format Under Test | VST3 |
| Plugin Binary Path | /Users/artbox/Library/Audio/Plug-Ins/VST3/LocusQ.vst3/Contents/MacOS/LocusQ |
| Session Sample Rate / Block Size | 48kHz / 512 |
| Notes | Blocker-only rerun on UI-04/UI-06/UI-07/UI-12 after fresh local build/install; UI-06/UI-07 now pass in host. |

## Manual Host UI Checklist (2.7 Closeout Sheet)

| ID | Check | Steps | Expected Result | Manual Result | Run Date (UTC) | Host (DAW) | Host Version | Prefilled Objective Observation (from existing evidence) | Evidence Paths |
|---|---|---|---|---|---|---|---|---|---|
| UI-01 | Mode tab interaction | Click `Calibrate`, `Emitter`, `Renderer` tabs repeatedly | Active tab and rail panel switch correctly with no stuck state | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | Operator PASS in host session; mode tab switching stable. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-02 | Quality badge toggle | Click `DRAFT/FINAL` badge | Badge toggles and renderer quality state updates with no JS freeze | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS with hit-target usability note: clicks near badge text can miss; cursor reposition improves registration. | `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_js_check.log`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-03 | Rail toggle controls | Toggle `Mute`, `Solo`, `Animation`, `Loop`, `Sync` | Switches change visual state and persist correctly after tab changes | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | UI state PASS. Audio response for these controls requires Renderer instance on main channel (expected design behavior). | `plugins/LocusQ/status.json` |
| UI-04 | Dropdown controls | Change `Mode`, `Physics preset`, `Animation source` dropdowns | Selection changes apply immediately and do not break UI | `PASS` | 2026-02-21T09:10:44Z | Reaper | v7.61 | `Physics preset` is now sticky across `Off`/`Bounce`/`Float`/`Orbit`/`Custom` and no longer auto-reverts to `Custom`; operator noted `Mode`/`Animation Source` visibly affect motion but behavior correctness still needs product-level confirmation. | operator manual rerun notes (2026-02-21) |
| UI-05 | Text input edit | Edit emitter label text box | Text entry accepts changes and preserves edited value | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session; emitter label text entry and persistence behavior confirmed. | `plugins/LocusQ/status.json` |
| UI-06 | Timeline transport | Use rewind/stop/play controls | Timeline time updates correctly; play/stop behavior is coherent | `PASS` | 2026-02-21T09:26:24Z | Reaper | v7.61 | PASS on rerun: rewind/play/stop are now coherent in host. Prior 2026-02-21T09:10:44Z fail is superseded. | operator manual rerun notes (2026-02-21) |
| UI-07 | Timeline keyframe editing | Add/move/delete keyframes; cycle curve on dbl-click | Keyframe edits are reflected visually and remain stable after mode switches | `PASS` | 2026-02-21T09:26:24Z | Reaper | v7.61 | PASS on rerun: keyframe/timeline controls are visible and gesture path is functional in host. Prior 2026-02-21T09:10:44Z fail is superseded. | operator manual rerun notes (2026-02-21) |
| UI-08 | Viewpoint controls | Use P/T/F/S view buttons and orbit/pan/zoom in viewport | Camera controls respond or viewport degrades gracefully without disabling rail | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session for available viewpoint controls. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_host_edge_48k512.log` |
| UI-09 | Viewport degraded-path resilience | If viewport fails, continue interacting with rail controls | Rail remains fully interactive with visible degraded message | `N/A` | 2026-02-20T22:11:16Z | Reaper | v7.61 | N/A: degraded viewport message/state was not observed during this run. | `plugins/LocusQ/TestEvidence/build-summary.md` |
| UI-10 | Calibration start/abort | In `Calibrate`, press `START MEASURE`, then abort | Start/abort controls work and status text/meter update coherently | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session; start/abort path behaved coherently. | `plugins/LocusQ/status.json` |
| UI-11 | Profile/status indicators | Observe `scene-status`, viewport info, profile indicators across modes | Status labels update consistently and match current mode/action | `PASS` | 2026-02-20T22:11:16Z | Reaper | v7.61 | PASS in host session; status indicators tracked mode/action changes. | `plugins/LocusQ/status.json`; `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log` |
| UI-12 | Persistence sanity | Save/load preset after edits (timeline + toggles) | Loaded state restores expected settings without UI lockup | `PASS` | 2026-02-21T09:10:44Z | Reaper | v7.61 | PASS on core requirement: `SAVE` adds preset and `LOAD` restores state; operator notes preset rename/delete affordances are not exposed in current UI (possible UX follow-up). | operator manual rerun notes (2026-02-21) |

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
| PASS | 16 | UI-01, UI-02, UI-03, UI-04, UI-05, UI-06, UI-07, UI-08, UI-10, UI-11, UI-12, DEV-01, DEV-02, DEV-03, DEV-04, DEV-05 |
| FAIL | 0 | |
| N/A | 2 | UI-09, DEV-06 |
| PENDING | 0 | |

- Final manual host UI verdict (manual-only): `PASS`
- 2.7 closeout condition is now satisfied for this checklist (`no FAIL`, all non-`N/A` checks `PASS`, host/version fields populated).

## Notes

- If any check fails, include exact control ID/location and a short repro sequence.
- Capture evidence for each executed row; minimum is one screenshot or clip path per failed row.
- Recommended: capture one screenshot per functional section (`tabs`, `timeline`, `viewport`, `calibration`).
- This sheet intentionally pre-fills only objective, non-manual observations from existing logs; it does not claim DAW click-path completion.
- Operator summary from latest blocker rerun: timeline transport and keyframe editing now pass in host; blocker defects are resolved.

## BL-024 Manual Runbook Evidence Row (2026-02-23)

| Field | Value |
|---|---|
| Operator | Josh Band |
| Run Date (UTC) | 2026-02-23T03:04:49Z |
| Host (DAW) | REAPER |
| Host Version | v7.61 |
| Output Device Path | Stereo monitor path (`1/2ch`, `512spls`, `48kHz`) |
| Session Bootstrap | `qa/reaper/reascripts/LocusQ_Create_Manual_QA_Session.lua` |
| Checklist Source | `Documentation/testing/reaper-manual-qa-session.md` |

| Checklist Block | Result | Evidence |
|---|---|---|
| Bootstrap creates synth + LocusQ routing session | `PASS` | ReaScript console output confirms `Synth FX: VSTi: ReaSynth (Cockos)` and `LocusQ FX: VST3: LocusQ`; automation artifact `TestEvidence/reaper_headless_render_20260223T030321Z/bootstrap_status.json` (`sendCreated=true`, `midiClipCreated=true`). |
| Headphone mode path (`stereo_downmix` vs `steam_binaural`) remains stable | `PASS` | BL-009 deterministic contract remains green (`TestEvidence/bl009_headphone_contract_20260223T020702Z/status.tsv`) and BL-024 host lane ran `3/3` clean strict passes (`TestEvidence/bl024_reaper_automation_20260223T030210Z/status.tsv`). |
| Transport/keyframe/preset interaction checks | `PASS` | Prior manual host checklist rows remain green: `UI-06`, `UI-07`, `UI-12` (`TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` section “Manual Host UI Checklist”). |

- BL-024 manual runbook row verdict: `PASS` (host session + deterministic automation evidence).
