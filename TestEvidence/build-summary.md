Title: LocusQ Build Summary (Acceptance Closeout)
Document Type: Build Summary
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-19

# LocusQ Build Summary (Acceptance Closeout)

Date (UTC): `2026-02-19`

## Commands Run

1. Build QA executable for closeout rerun

```sh
cmake --build plugins/LocusQ/build --target locusq_qa -j 8
```

Result: `PASS`

2. Run full-system CPU gate scenario (`8 Emitters + 1 Renderer`, Draft, 48k/512)

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512
```

Result: `WARN` (hard gates pass; soft allocation warning)
- `perf_avg_block_time_ms=0.428113`
- `perf_meets_deadline=true`
- `perf_p95_block_time_ms=0.449794`
- `perf_allocation_free=false` (soft warning)

3. Run host edge-case lifecycle matrix (`multi-pass roundtrip`) across SR/BS variants

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 44100 --block-size 256
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 512
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 1024
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 96000 --block-size 512
```

Result: `PASS` (all four runs)

4. Run Phase 2.6 acceptance suite rollup

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json
```

Result: `PASS` with warning (`3` scenarios: `2 PASS`, `1 WARN`, `0 FAIL`, `0 ERROR`)

5. Build plugin targets (VST3 + Standalone) after closeout validation

```sh
cmake --build plugins/LocusQ/build --target LocusQ_VST3 LocusQ_Standalone -j 8
```

Result: `PASS` (warnings only)

6. Validate VST3 with `pluginval`

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3
```

Result: `PASS` (`exit code 0`)

7. Standalone launch smoke

```sh
open plugins/LocusQ/build/LocusQ_artefacts/Standalone/LocusQ.app
```

Result: `PASS`

8. WebView checklist validation (`manual fallback`; PowerShell unavailable in this environment)
- Verified against source/CMake:
  - embedded web resources in `CMakeLists.txt`
  - explicit WebView2 backend + user-data folder
  - native integration + resource provider
  - relay/attachment creation order
  - resource-root URL load path
  - relay/member declaration order (`Relays -> WebView -> Attachments`)
- Result: `PASS`

## Phase 2.4 Acceptance Closeout Addendum (UTC 2026-02-19)

1. Reconfigure QA build graph (new probe target)

```sh
cmake -S plugins/LocusQ -B plugins/LocusQ/build -DBUILD_LOCUSQ_QA=ON -DJUCE_DIR=/Users/artbox/Documents/Repos/audio-plugin-coder/_tools/JUCE
```

Result: `PASS`

2. Build Phase 2.4 probe + QA targets

```sh
cmake --build plugins/LocusQ/build --target locusq_physics_probe locusq_qa -j 8
```

Result: `PASS`

3. Run deterministic physics probe

```sh
./plugins/LocusQ/build/locusq_physics_probe_artefacts/locusq_physics_probe
```

Result: `PASS` (`5/5` checks)

4. Run Phase 2.4 physics spatial-motion scenario

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_24_physics_spatial_motion.json
```

Result: `PASS`

5. Run Phase 2.4 zero-g drift scenario

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_24_physics_zero_g_drift.json
```

Result: `PASS`

6. Run Phase 2.4 suite rollup

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_4_acceptance_suite.json
```

Result: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)

## Artifacts

- QA build log: `plugins/LocusQ/TestEvidence/locusq_qa_build_phase_2_6_acceptance_refresh.log`
- Full-system CPU gate log: `plugins/LocusQ/TestEvidence/locusq_phase_2_6_full_system_48k512_phase_2_6_acceptance_refresh.log`
- Host edge matrix logs:
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_44k1_256_phase_2_6_acceptance_refresh.log`
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_48k512_phase_2_6_acceptance_refresh.log`
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_48k1024_phase_2_6_acceptance_refresh.log`
  - `plugins/LocusQ/TestEvidence/locusq_phase_2_6_host_edge_96k512_phase_2_6_acceptance_refresh.log`
- Phase 2.6 suite rollup log: `plugins/LocusQ/TestEvidence/locusq_phase_2_6_acceptance_suite_run_phase_2_6_acceptance_refresh.log`
- Plugin build log: `plugins/LocusQ/TestEvidence/locusq_build_phase_2_6_acceptance_refresh.log`
- Phase 2.4 configure log: `plugins/LocusQ/TestEvidence/locusq_phase_2_4_closeout_configure.log`
- Phase 2.4 build log: `plugins/LocusQ/TestEvidence/locusq_phase_2_4_closeout_build.log`
- Phase 2.4 probe log: `plugins/LocusQ/TestEvidence/locusq_phase_2_4_physics_probe_closeout.log`
- Phase 2.4 scenario log: `plugins/LocusQ/TestEvidence/locusq_24_physics_spatial_motion_closeout.log`
- Phase 2.4 zero-g scenario log: `plugins/LocusQ/TestEvidence/locusq_24_physics_zero_g_drift_closeout.log`
- Phase 2.4 suite log: `plugins/LocusQ/TestEvidence/locusq_phase_2_4_acceptance_suite_closeout.log`
- pluginval logs:
  - `plugins/LocusQ/TestEvidence/pluginval_phase_2_6_acceptance_refresh_stdout.log`
  - `plugins/LocusQ/TestEvidence/pluginval_phase_2_6_acceptance_refresh_stderr.log`
  - `plugins/LocusQ/TestEvidence/pluginval_phase_2_6_acceptance_refresh_exit_code.txt`
- Standalone smoke log: `plugins/LocusQ/TestEvidence/standalone_launch_smoke_phase_2_6_acceptance_refresh.log`

## Notes

- Phase 2.6 acceptance gates are now closed on hard criteria.
- Phase 2.4 acceptance gates are now closed (`probe + scenario + suite all pass`).
- Historical allocation warning from early Phase 2.6 closeout is superseded by the Phase 2.6c allocation-free rerun.

## Ship Addendum (UTC 2026-02-19)

9. Configure universal macOS release build

```sh
cmake -S plugins/LocusQ -B plugins/LocusQ/build_ship_universal -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DJUCE_DIR=/Users/artbox/Documents/Repos/audio-plugin-coder/_tools/JUCE
```

Result: `PASS`

10. Build release plugin formats for ship

```sh
cmake --build plugins/LocusQ/build_ship_universal --target LocusQ_VST3 LocusQ_AU LocusQ_Standalone -j 8
```

Result: `PASS`

11. Verify universal architecture

```sh
lipo -archs plugins/LocusQ/build_ship_universal/LocusQ_artefacts/Release/VST3/LocusQ.vst3/Contents/MacOS/LocusQ
lipo -archs plugins/LocusQ/build_ship_universal/LocusQ_artefacts/Release/AU/LocusQ.component/Contents/MacOS/LocusQ
lipo -archs plugins/LocusQ/build_ship_universal/LocusQ_artefacts/Release/Standalone/LocusQ.app/Contents/MacOS/LocusQ
```

Result: `PASS` (`x86_64 arm64` for all three formats)

12. Package local macOS distribution

```sh
ditto -c -k --sequesterRsrc --keepParent dist/LocusQ-v0.1.0-macOS dist/LocusQ-v0.1.0-macOS.zip
```

Result: `PASS`

13. Validate universal VST3 with `pluginval`

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build_ship_universal/LocusQ_artefacts/Release/VST3/LocusQ.vst3
```

Result: `PASS` (`exit code 0`)

14. Standalone universal smoke

```sh
open plugins/LocusQ/build_ship_universal/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (process observed/terminated)

Ship artifacts:
- Distribution dir: `dist/LocusQ-v0.1.0-macOS`
- Archive: `dist/LocusQ-v0.1.0-macOS.zip`
- Build manifest: `dist/LocusQ-v0.1.0-macOS/BUILD_MANIFEST.md`

## Phase 2.7a UI Runtime Recovery Snapshot (UTC 2026-02-19)

15. Module syntax check for resilient WebView bootstrap patch

```sh
node --input-type=module --check < plugins/LocusQ/Source/ui/public/js/index.js
```

Result: `PASS`

16. Rebuild VST3 after WebView runtime hardening

```sh
cmake --build plugins/LocusQ/build --target LocusQ_VST3 -j 8
```

Result: `PASS` (existing compile warnings unchanged; artifact rebuilt)

Scope of patch:
- `Source/ui/public/js/index.js` now initializes UI bindings and parameter listeners before viewport init.
- Viewport/WebGL init failures enter explicit degraded mode instead of aborting full UI interactivity.
- Optional DOM and viewport references are guarded in mode switching and scene updates.
- Ship logs:
  - `plugins/LocusQ/TestEvidence/locusq_ship_universal_configure.log`
  - `plugins/LocusQ/TestEvidence/locusq_ship_universal_build.log`
  - `plugins/LocusQ/TestEvidence/locusq_ship_package.log`
  - `plugins/LocusQ/TestEvidence/pluginval_ship_universal_stdout.log`
  - `plugins/LocusQ/TestEvidence/pluginval_ship_universal_stderr.log`
  - `plugins/LocusQ/TestEvidence/pluginval_ship_universal_exit_code.txt`
  - `plugins/LocusQ/TestEvidence/standalone_ship_universal_smoke.log`

## Phase 2.7 UI Interaction Smoke Matrix Snapshot (UTC 2026-02-19)

17. Harness sanity (configure/build/ctest)

```sh
cmake -S "$HARNESS_PATH" -B "$HARNESS_PATH/build_test" -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build "$HARNESS_PATH/build_test"
ctest --test-dir "$HARNESS_PATH/build_test" --output-on-failure
```

Result: `PASS` (`45/45`)

18. Build plugin QA runner for matrix execution

```sh
cmake --build plugins/LocusQ/build --target locusq_qa -j 8
```

Result: `PASS`

19. Execute UI interaction smoke matrix (DSP/host proxy scenarios)

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_animation_internal_smoke.json
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 44100 --block-size 256
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 512
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 1024
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 96000 --block-size 512
```

Result: `PASS_WITH_WARNING`
- `locusq_smoke_suite`: `PASS` (`4 PASS / 0 WARN / 0 FAIL`)
- `locusq_26_animation_internal_smoke`: `PASS`
- `locusq_phase_2_6_acceptance_suite`: `WARN` (`2 PASS / 1 WARN / 0 FAIL`)
- `locusq_26_full_system_cpu_draft`: `WARN` (hard gates pass; allocation warning retained)
- Host-edge matrix runs (`44.1k/256`, `48k/512`, `48k/1024`, `96k/512`): `PASS`

20. Host validation checks

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3
open plugins/LocusQ/build/LocusQ_artefacts/Standalone/LocusQ.app
```

Result: `PASS`
- pluginval exit code: `0`
- Standalone smoke: `open` exit code `0`

Artifacts:
- `plugins/LocusQ/qa_output/suite_result.json`
- `plugins/LocusQ/TestEvidence/suite_result_baseline_test_phase_2_7_ui_matrix.json`
- `plugins/LocusQ/TestEvidence/test-summary.md`
- `plugins/LocusQ/TestEvidence/locusq_smoke_suite_emitter_run_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/locusq_26_animation_smoke_run_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_6_acceptance_suite_run_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/locusq_26_full_system_48k512_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_44k1_256_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_48k512_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_48k1024_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_96k512_test_phase_2_7_ui_matrix.log`
- `plugins/LocusQ/TestEvidence/pluginval_test_phase_2_7_ui_matrix_stdout.log`
- `plugins/LocusQ/TestEvidence/pluginval_test_phase_2_7_ui_matrix_stderr.log`
- `plugins/LocusQ/TestEvidence/pluginval_test_phase_2_7_ui_matrix_exit_code.txt`
- `plugins/LocusQ/TestEvidence/standalone_launch_smoke_test_phase_2_7_ui_matrix.log`

## Phase 2.7a Manual Host Checklist Handoff (UTC 2026-02-19)

21. Prepared manual in-host DAW acceptance checklist for final 2.7a closeout

Artifact:
- `plugins/LocusQ/TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`

Status:
- Checklist created and linked in `status.json`, `.ideas/plan.md`, and `README.md`.
- Awaiting user-run DAW pass/fail entries to convert from pending to closed.

## Phase 2.7b Viewport + Calibration Wiring Snapshot (UTC 2026-02-19)

22. WebView runtime module syntax check (post-2.7b patch)

```sh
node --input-type=module --check < plugins/LocusQ/Source/ui/public/js/index.js
```

Result: `PASS`

23. Rebuild VST3 after 2.7b scene/calibration payload and viewport interaction updates

```sh
cmake --build plugins/LocusQ/build --target LocusQ_VST3 -j 8
```

Result: `PASS`

24. Run smoke regression suite after 2.7b implementation

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json
```

Result: `PASS` (`4 PASS / 0 WARN / 0 FAIL`)

Scope of patch:
- `plugins/LocusQ/Source/ui/public/js/index.js`
  - Added emitter ray-pick/select + drag movement path in viewport.
  - Drag updates now write through APVTS position parameters (`pos_azimuth`, `pos_elevation`, `pos_distance`) using JUCE slider ranges/skew.
  - Selection ring now tracks selected emitter deterministically (local-emitter aware).
  - Calibration speaker colors/meters now consume native calibration status data, replacing cosmetic-only behavior in calibrate mode.
- `plugins/LocusQ/Source/PluginProcessor.cpp`
  - Added `localEmitterId` in scene payload for UI ownership-aware selection.
  - Added calibration payload fields `speakerLevels` and `profileValid` for state-driven visualization.
- `plugins/LocusQ/Source/ui/public/index.html`
  - Added selected-state styling for renderer scene list rows.

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_ui_phase_2_7b_js_check.log`
- `plugins/LocusQ/TestEvidence/locusq_build_phase_2_7b_vst3.log`
- `plugins/LocusQ/TestEvidence/locusq_smoke_suite_phase_2_7b.log`

## Phase 2.6c Allocation-Free Closeout Snapshot (UTC 2026-02-19)

25. Rebuild QA target for allocation-free rerun

```sh
cmake --build plugins/LocusQ/build --target locusq_qa -j 8
```

Result: `PASS`

26. Run full-system CPU gate scenario (`8 Emitters + 1 Renderer`, Draft, 48k/512)

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512
```

Result: `PASS`
- `perf_avg_block_time_ms=0.304457`
- `perf_p95_block_time_ms=0.318466`
- `perf_meets_deadline=true`
- `perf_allocation_free=true`
- `perf_total_allocations=0`

27. Run Phase 2.6 acceptance suite rollup

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_qa_build_phase_2_6c_allocation_free.log`
- `plugins/LocusQ/TestEvidence/locusq_26_full_system_cpu_draft_phase_2_6c_allocation_free.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_6_acceptance_suite_phase_2_6c_allocation_free.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_6_acceptance_suite_phase_2_6c_allocation_free_refresh.log`

## Acceptance Rebaseline Snapshot (UTC 2026-02-19)

Run ID: `test_acceptance_rebaseline_20260219T053302Z`

28. Rebaseline matrix execution (harness + suites + perf + host-edge)

Result: `PASS` on all scenario/suite/perf steps
- Harness ctest: `45/45` pass
- Phase 2.5 suite: `PASS` (`9 PASS / 0 WARN / 0 FAIL`)
- Phase 2.6 suite: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)
- Full-system CPU (`48k/512`): `PASS`
  - `perf_avg_block_time_ms=0.307458`
  - `perf_p95_block_time_ms=0.321793`
  - `perf_meets_deadline=true`
  - `perf_allocation_free=true`
- Host-edge matrix (`44.1k/256`, `48k/512`, `48k/1024`, `96k/512`): `PASS`

29. Plugin host-validation check (`pluginval`)

Result: `FLAKY` in this run set
- First run: `FAIL` (`exit 9`, segfault during automation sub-block test)
  - `plugins/LocusQ/TestEvidence/pluginval_test_acceptance_rebaseline_20260219T053302Z.log`
- Immediate retry: `PASS` (`exit 0`)
  - `plugins/LocusQ/TestEvidence/pluginval_test_acceptance_rebaseline_retry_20260219T053459Z.log`

Artifacts:
- `plugins/LocusQ/TestEvidence/test_acceptance_rebaseline_20260219T053302Z_status.tsv`
- `plugins/LocusQ/TestEvidence/harness_configure_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/harness_build_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/harness_ctest_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_qa_build_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_5_acceptance_suite_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_6_acceptance_suite_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_full_system_cpu_draft_48k512_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_44k1_256_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_48k512_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_48k1024_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_96k512_test_acceptance_rebaseline_20260219T053302Z.log`
- `plugins/LocusQ/TestEvidence/locusq_plugin_build_vst3_standalone_test_acceptance_rebaseline_20260219T053302Z.log`

## Phase 2.7d Host-Interaction Closure Prep Snapshot (UTC 2026-02-19)

28. WebView runtime JS syntax check (post-2.7d patch)

```sh
node --input-type=module --check < plugins/LocusQ/Source/ui/public/js/index.js
```

Result: `PASS`

29. Rebuild VST3 + QA runner after relay/bridge hardening

```sh
cmake --build plugins/LocusQ/build --target LocusQ_VST3 locusq_qa -j 8
```

Result: `PASS` (existing warnings unchanged)

30. Focused headless scenario checks

```sh
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_animation_internal_smoke.json
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 512
```

Result: `PASS`
- `locusq_smoke_suite`: `4 PASS / 0 WARN / 0 FAIL`
- `locusq_26_animation_internal_smoke`: `PASS` (`perf_avg_block_time_ms=0.0534585`, `perf_meets_deadline=true`)
- `locusq_phase_2_6_acceptance_suite`: `3 PASS / 0 WARN / 0 FAIL`
- `locusq_26_host_edge_roundtrip_multipass` (`48k/512`): `PASS`

Scope of patch:
- `plugins/LocusQ/Source/PluginEditor.h` / `plugins/LocusQ/Source/PluginEditor.cpp`
  - Added Web relays + attachments for `pos_x`, `pos_y`, `pos_z`.
- `plugins/LocusQ/Source/ui/public/js/index.js`
  - Viewport drag now writes both spherical and Cartesian APVTS coordinates.
- `plugins/LocusQ/Source/PluginProcessor.cpp`
  - `setTimelineCurrentTimeFromUI` now rejects non-finite input and clamps to timeline duration.

Manual-host-only remainder:
- DAW-embedded verification remains required for tabs/toggles/text/dropdowns focus/automation behavior, Cartesian viewport drag feel/persistence, and calibration start/abort lifecycle.

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_js_check.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_build.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_smoke.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_animation_smoke.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_phase_2_6_acceptance_suite.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_7d_host_edge_48k512.log`
