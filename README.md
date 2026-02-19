Title: LocusQ Root README
Document Type: Project README
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# LocusQ

LocusQ is a spatial audio plugin under APC, built with JUCE and a WebView UI.

## Plain-English Summary

### What is already done
- The core product works: sound placement, room effects, motion/physics, keyframe animation, and calibration logic are implemented.
- The plugin builds and ships on macOS as VST3, AU, and Standalone (universal binary).
- Major UI wiring work is done: controls are connected to real plugin state, not just visuals.
- Performance work closed the earlier allocation warning for the Phase 2.6 full-system run.
- Documentation and decision records are now standardized and synced through a freshness gate.

### What is still open
- Final manual DAW verification is still needed for embedded-host UI behavior (click/edit/drag checks in real host sessions).
- After manual checks, a final full acceptance rerun is needed to lock the closeout snapshot.

### V1 Completion Checklist
- [x] Core feature set implemented (spatialization, room effects, motion/physics, calibration, keyframe animation).
- [x] macOS build/package path complete for VST3/AU/Standalone (universal build).
- [x] Phase 2.6 full-system allocation-free closeout achieved.
- [x] UI control-path recovery implemented through 2.7d (code-side/headless checks passing).
- [x] Pluginval automation-segfault mitigation landed and validated (seeded repro now passes; 10-run stability pass).
- [x] Documentation freshness gate enabled (ADR-0005 + CI/script checks).
- [ ] Complete manual host UI checklist (`TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`).
- [ ] Run final `/test` acceptance rerun (DSP + host + UI matrix) and publish deltas.
- [ ] Close any remaining 2.7d issues found during manual DAW verification.
- [ ] Keep closeout docs synchronized on each final closeout pass (`status.json`, `README.md`, `CHANGELOG.md`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`).
- [ ] Cut v1 release candidate and run final ship smoke checks.

## Current Status (UTC 2026-02-19)

- `current_phase`: `code` (post-ship reopen for Phase `2.7a/2.7b/2.7c/2.7d` UI-runtime recovery; manual host acceptance remains pending)
- `version`: `v0.1.0`
- Implementation plan phases: `2.1-2.6` complete (Phase 2.4 acceptance now closed)
- UI framework: `webview`
- Planning decision package: `complete` (ADR-0002/0003/0004 + scene-state contract)
- Phase 2.5 acceptance: hard-gate `PASS` with one residual warning trend (`locusq_25_cpu_budget_draft`)
- Documentation closeout gate: `active` (ADR-0005; phase-closeout surfaces must sync in one change set)
- Phase 2.7a bootstrap hardening: implemented; host interaction acceptance still pending
- Phase 2.7b implementation: viewport emitter pick/select/move now updates APVTS position state; calibration speaker/meter/profile visuals now consume native status payloads
- Phase 2.7c implementation: tabs/toggles/dropdowns/value steppers now route through relays/attachments, and `emit-label` / physics preset memory persists through native UI-state bridge
- Phase 2.7d implementation: Cartesian viewport drag is now APVTS-backed (`pos_x/pos_y/pos_z` relay/attachment wiring + drag writeback sync), and native timeline time input is finite/clamped on bridge entry
- Pluginval automation guard: mode-transition registration sync prevents stale emitter audio pointer reads during aggressive mode automation.
- Phase 2.7 UI interaction smoke matrix (`/test`): `PASS_WITH_WARNING` with trend deltas published
- Manual host UI checklist staged: `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`

## Completed Phases

- Ideation, planning, and UI design (`v3`) completed.
- Phase 2.1 foundation completed.
- Phase 2.2 spatialization core completed.
- Phase 2.3 room calibration completed.
- Phase 2.4 physics engine completed with acceptance closure evidence.
- Phase 2.5 room acoustics + advanced DSP completed (acceptance suite pass with warning trend).
- Phase 2.6 implemented items: timeline runtime, keyframe editor interactions, preset persistence, and perf telemetry.
- Ship phase completed (local macOS universal package).
- Post-ship integration recovery (`2.7a`) started; bootstrap resiliency patch applied.
- Post-ship integration recovery (`2.7b`) implemented for viewport movement and calibration visualization pathing.
- Post-ship integration recovery (`2.7c`) implemented for rail/header control-path wiring and UI-only state persistence.
- Post-ship integration recovery (`2.7d`) implemented for Cartesian viewport host-interaction closure prep and native timeline-time bridge hardening.

## Build and Load Snapshot

- QA build: `PASS`
- Phase 2.4 deterministic probe + phase-pure suite: `PASS` (`5/5` probe checks; spatial-motion + zero-g suite `2 PASS / 0 WARN / 0 FAIL`)
- Phase 2.6 acceptance suite: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)
- Full-system CPU gate (`locusq_26_full_system_cpu_draft`): `PASS` with allocation-free criteria met (`perf_allocation_free=true`, `perf_total_allocations=0`)
- Host edge matrix (`locusq_26_host_edge_roundtrip_multipass`): `PASS` across `44.1k/256`, `48k/512`, `48k/1024`, `96k/512`
- Plugin build (`LocusQ_VST3`, `LocusQ_Standalone`): `PASS`
- `pluginval` strictness 5 (in-process, skip GUI): `PASS` (exit code 0)
- pluginval automation regression seed (`0x2a331c6`) now passes after fix; 10-run post-fix stability check passed (`10/10`).
- Standalone launch smoke: `PASS`
- Post-2.7b smoke regression suite (`locusq_smoke_suite`): `PASS` (`4 PASS / 0 WARN / 0 FAIL`)
- Post-2.7c smoke regression suite (`locusq_smoke_suite`): `PASS` (`4 PASS / 0 WARN / 0 FAIL`)
- Post-2.7d focused headless checks: `PASS` (JS syntax, `LocusQ_VST3` + `locusq_qa` build, smoke suite, animation smoke, phase 2.6 acceptance suite, host-edge `48k/512`)
- UI interaction smoke matrix refresh: `PASS_WITH_WARNING`; trend deltas published to `qa_output/suite_result.json`

## Distribution Snapshot

- Packaging mode: local macOS release from universal build (`x86_64 arm64`)
- Formats included: `LocusQ.vst3`, `LocusQ.component`, `LocusQ.app`
- Distribution directory: `dist/LocusQ-v0.1.0-macOS`
- Archive: `dist/LocusQ-v0.1.0-macOS.zip`
- Build manifest: `dist/LocusQ-v0.1.0-macOS/BUILD_MANIFEST.md`
- Universal ship checks:
  - `pluginval` on ship VST3: `PASS` (`plugins/LocusQ/TestEvidence/pluginval_ship_universal_stdout.log`)
  - Standalone smoke: `PASS` (`plugins/LocusQ/TestEvidence/standalone_ship_universal_smoke.log`)

See:
- `status.json`
- `TestEvidence/build-summary.md`
- `TestEvidence/test-summary.md`
- `TestEvidence/validation-trend.md`
- `qa_output/suite_result.json`

## Phase 2.6 Closeout

- Full-system and host-edge acceptance gates are closed on hard criteria.
- Allocation-free closeout is complete for the full-system draft scenario.

## Manual-Host-Only Remaining

- Run `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` in target DAW(s) to confirm embedded-host behavior for tabs/toggles/dropdowns/text fields.
- Verify viewport drag in Cartesian mode in-host (confirm APVTS writeback/persistence and no snapback under DAW WebView input routing).
- Verify calibration `START/ABORT` and profile-ready lifecycle against real host audio routing.

## Canonical Documentation

- Plan: `.ideas/plan.md`
- Architecture/spec: `.ideas/architecture.md`, `.ideas/parameter-spec.md`
- Invariants and ADRs: `Documentation/invariants.md`, `Documentation/adr/`
- Docs freshness gate ADR: `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- Scene-state contract: `Documentation/scene-state-contract.md`
- Wiring traceability: `Documentation/implementation-traceability.md`
- Lessons learned: `Documentation/lessons-learned.md`
- Research synthesis and next steps: `Documentation/research/quadraphonic-audio-spatialization-next-steps.md`

## Suggested Next Command

- Run `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` in your DAW session, then run `/test LocusQ full acceptance rerun (DSP + host + UI matrix)` once manual checks pass.
