Title: BL-026 CALIBRATE v1 vs v2 UI/UX Comparison
Document Type: Design Review
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-026 CALIBRATE v1 vs v2 UI/UX Comparison

## Purpose
Provide a side-by-side comparison of CALIBRATE v1 and CALIBRATE v2, including visual evidence, feature-level UX changes, and the next implementation/validation steps.

## Visual Evidence
### v1 (Before)
![CALIBRATE v1 before](../../TestEvidence/bl026_calibrate_v2_20260223T213430Z/calibrate_before_v1.png)

### v2 (After)
![CALIBRATE v2 after](../../TestEvidence/bl026_calibrate_v2_20260223T213430Z/calibrate_after_v2.png)

Image source bundle:
- `TestEvidence/bl026_calibrate_v2_20260223T213430Z/`

## v1 CALIBRATE Features and UX
1. Fixed calibration setup oriented around a limited speaker configuration model (`2x Stereo`/legacy speaker setup semantics).
2. Fixed output routing controls (`SPK1 Out` through `SPK4 Out`) without topology-driven row changes.
3. Core measurement controls only:
- Mic channel
- Test signal type
- Test level
- `START MEASURE`
4. Progress/status list focused on speaker measured/not measured states.
5. No profile library for saving/loading named calibration contexts.
6. No explicit validation chips for mapping/phase/delay/profile/downmix checks.
7. No explicit topology/monitoring/device context displayed in the rail.

## v2 CALIBRATE Features and UX (BL-026)
1. Profile-first setup model:
- Topology selector
- Monitoring path selector
- Device profile selector
- Mic channel
2. Dynamic mapping contract behavior:
- Topology-driven visible map rows
- Routing redetect action
- Custom-map overwrite acknowledgement
- Limited-width mapping acknowledgement when topology exceeds writable routing width
3. Structured run and validation block:
- Deterministic preflight before run
- Start/abort/measure-again lifecycle
- Validation chips for mapping, phase/polarity, delay consistency, profile stage, downmix path
4. Calibration profile library:
- Save
- Load
- Rename
- Delete
- Status feedback for CRUD operations
5. Context chips in header:
- Topology
- Monitoring
- Device
- Mapping coverage
6. Better separation of concerns in rail IA:
- Profile setup
- Output mapping
- Run and validation
- Profile library

## Full Feature Breakdown
| Area | v1 | v2 |
|---|---|---|
| Topology model | Limited legacy setup | Explicit multi-topology model (mono/stereo/quad/surround/ambisonic/binaural/downmix) |
| Monitoring path | Implicit/default | Explicit (`Speakers`, `Stereo Downmix`, `Steam Binaural`, `Virtual Binaural`) |
| Device profile | Not exposed in CALIBRATE | Explicit (`Generic`, `AirPods Pro 2`, `Sony WH-1000XM5`, `Custom SOFA`) |
| Routing matrix | Fixed 4-row mapping | Topology-driven row visibility with routing safety guards |
| Redetect safety | Basic/implicit | Deterministic redetect + explicit overwrite acknowledgement |
| Run preflight | Minimal | Deterministic preflight with topology-width gate |
| Validation surface | Progress-only | Pass/fail-oriented diagnostics chips and state indicators |
| Profile persistence | None | Full calibration profile CRUD flow |
| Self-test coverage | Legacy core checks | Explicit BL-026 lanes `UI-P1-026A..E` |

## Validation Snapshot (Current)
1. Scoped BL-026 production self-test: `PASS`
- Artifact: `TestEvidence/locusq_production_p0_selftest_20260223T212843Z.json`
- Lanes: `UI-P1-026A..E` all pass.
2. REAPER headless smoke: `PASS`
- Artifact: `TestEvidence/reaper_headless_render_20260223T213300Z/status.json`
3. Docs freshness gate: `PASS`
- Artifact: `TestEvidence/bl026_calibrate_v2_20260223T213430Z/docs_freshness.log`
4. Full production self-test currently has an unrelated non-BL-026 failure (`UI-P1-025B`).

## What’s Next (CALIBRATE UI/UX)
1. Manual headphone validation closeout (operator runbook evidence):
- AirPods Pro 2 and Sony WH-1000XM5 path checks
- Binaural vs downmix listening A/B notes
2. BL-026 promotion from `In Validation` to `Done` after manual evidence is attached.
3. Full-suite stabilization alignment with neighboring UX streams (resolve or accept unrelated `UI-P1-025B` before final closure).
4. CALIBRATE-to-RENDERER UX coherence pass with BL-027:
- Shared topology/profile naming and diagnostics semantics
- Cross-panel profile clarity and fallback messaging consistency
5. Optional host-parity expansion for CALIBRATE-specific flows in CLAP runtime validation lane.
