Title: LocusQ Build Summary (Acceptance Closeout)
Document Type: Build Summary
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-20

# LocusQ Build Summary (Acceptance Closeout)

Date (UTC): `2026-02-20`

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

## Stage 14 Contract Alignment Addendum (UTC 2026-02-20)

15. Documentation freshness gate after Stage 14 contract/drift updates

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Stage 14 docs touched in this pass:
- `.ideas/creative-brief.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/invariants.md`
- `Documentation/implementation-traceability.md`
- `Documentation/stage14-review-release-checklist.md`

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

## Pluginval Automation Segfault Mitigation Snapshot (UTC 2026-02-19)

31. Reproduce pluginval automation crash with deterministic seed

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --random-seed 0x2a331c6 build/LocusQ_artefacts/VST3/LocusQ.vst3
```

Result: `FAIL` (`exit 9`, `Segmentation fault: 11`)

32. Capture crash backtrace (`lldb`, same seed)

```sh
lldb --batch -o "run" -k "thread backtrace all" -- /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --random-seed 0x2a331c6 build/LocusQ_artefacts/VST3/LocusQ.vst3
```

Result: `FAIL`, crash in `SpatialRenderer::process` while reading emitter audio during automation.

33. Implement mitigation and rebuild

```sh
cmake --build build --target LocusQ_VST3 locusq_qa -j 8
```

Result: `PASS`

Mitigation summary:
- Added `syncSceneGraphRegistrationForMode()` in processor runtime.
- Mode transitions now proactively unregister incompatible scene roles (Emitter/Renderer) before processing.
- Hardened `SceneGraph::unregisterEmitter()` against double-unregister state drift.

34. Re-run deterministic seed after fix

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --random-seed 0x2a331c6 build/LocusQ_artefacts/VST3/LocusQ.vst3
```

Result: `PASS` (`exit 0`)

35. Post-fix stability probe

```sh
for i in $(seq 1 10); do /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests build/LocusQ_artefacts/VST3/LocusQ.vst3; done
```

Result: `PASS` (`10/10`)

36. Post-fix QA regression refresh

```sh
./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json
./build/locusq_qa_artefacts/locusq_qa --spatial --sample-rate 48000 --block-size 512 qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json
./build/locusq_qa_artefacts/locusq_qa --spatial --sample-rate 48000 --block-size 512 qa/scenarios/locusq_26_full_system_cpu_draft.json
```

Result: `PASS`
- `locusq_smoke_suite`: `PASS`
- `locusq_phase_2_6_acceptance_suite`: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_5_acceptance_suite`: `PASS` (`9 PASS / 0 WARN / 0 FAIL`)
- `locusq_26_host_edge_roundtrip_multipass` (`48k/512`): `PASS`
- `locusq_26_full_system_cpu_draft` (`48k/512`): `PASS` (`perf_allocation_free=true`)

Artifacts:
- `plugins/LocusQ/TestEvidence/pluginval_repro_seed_0x2a331c6.log`
- `plugins/LocusQ/TestEvidence/pluginval_lldb_btall_seed_0x2a331c6.log`
- `plugins/LocusQ/TestEvidence/pluginval_repro_seed_0x2a331c6_after_fix.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_status.tsv`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run1.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run2.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run3.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run4.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run5.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run6.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run7.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run8.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run9.log`
- `plugins/LocusQ/TestEvidence/pluginval_postfix_stability_20260219T191544Z_run10.log`
- `plugins/LocusQ/TestEvidence/locusq_smoke_suite_pluginval_fix_20260219T191606Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_6_acceptance_suite_pluginval_fix_20260219T191606Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_5_acceptance_suite_pluginval_fix_20260219T191606Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_host_edge_48k512_pluginval_fix_20260219T191606Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_full_system_48k512_pluginval_fix_20260219T191606Z.log`

## Continue Checkpoint Snapshot (UTC 2026-02-19)

37. Rebuild core plugin + QA target

```sh
cmake --build build --target LocusQ_VST3 locusq_qa -j 8
```

Result: `PASS`

38. Smoke suite rerun (Emitter adapter)

```sh
./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json
```

Result: `PASS` (`4 PASS / 0 WARN / 0 FAIL`)

39. Docs freshness gate check

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_build_continue_20260219T192122Z.log`
- `plugins/LocusQ/TestEvidence/locusq_smoke_continue_20260219T192136Z.log`

## Phase 2.8 Output Layout Expansion Snapshot (UTC 2026-02-19)

40. Build plugin + QA targets after bus-layout expansion

```sh
cmake --build build --target LocusQ_VST3 locusq_qa -j 8
```

Result: `PASS`

41. Renderer spatial-output scenario in 4-channel runtime mode

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_renderer_spatial_output.json --channels 4
```

Result: `PASS`
- `non_finite`: `PASS`
- `clipping`: `PASS`
- `rms_energy`: `PASS`

42. Smoke-suite regression in 4-channel runtime mode

```sh
./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json --channels 4
```

Result: `PASS` (`4 PASS / 0 WARN / 0 FAIL`)

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_build_phase_2_8_quad_layout_20260219T192849Z.log`
- `plugins/LocusQ/TestEvidence/locusq_renderer_spatial_output_quad4_20260219T192849Z.log`
- `plugins/LocusQ/TestEvidence/locusq_smoke_suite_quad4_20260219T192849Z.log`

## Phase 2.8 Output Mapping + Telemetry Snapshot (UTC 2026-02-19)

43. JS syntax gate after output-layout telemetry UI wiring

```sh
node --input-type=module --check < Source/ui/public/js/index.js
```

Result: `PASS`

44. Build plugin + QA targets after explicit channel-map routing changes

```sh
cmake --build build --target LocusQ_VST3 locusq_qa -j 8
```

Result: `PASS`

45. Output-layout mono regression suite

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_mono_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)

46. Output-layout stereo regression suite

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_stereo_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)

47. Output-layout quad regression suite

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_quad_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_ui_phase_2_8_layout_js_check_20260219T194047Z.log`
- `plugins/LocusQ/TestEvidence/locusq_build_phase_2_8_layout_mapping_20260219T194047Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_8_output_layout_mono_suite_20260219T194356Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_8_output_layout_stereo_suite_20260219T194356Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_8_output_layout_quad_suite_20260219T194356Z.log`

## Phase 2.9 QA/CI Harness Expansion Snapshot (UTC 2026-02-19)

48. Verify CI workflow now includes explicit quad matrix lanes and seeded pluginval stress job

```sh
rg -n "qa-pluginval-seeded-stress|--channels 4|ci_phase_2_6_host_edge_.*_ch4|pluginval_seeded_stress/status.tsv" .github/workflows/qa_harness.yml
```

Result: `PASS` (quad matrix and seeded pluginval stress definitions present in workflow)

49. Validate docs freshness after phase 2.9 sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Artifacts:
- `plugins/LocusQ/.github/workflows/qa_harness.yml`

## Phase 2.10 Renderer CPU Guardrails Snapshot (UTC 2026-02-19)

50. Build plugin + QA targets after renderer guardrail/culling changes

```sh
cmake --build build --target LocusQ_VST3 locusq_qa -j 8
```

Result: `PASS`

51. Full-system CPU baseline regression (`8` emitters, `48k/512`)

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512
```

Result: `PASS`
- `perf_avg_block_time_ms=0.304505`
- `perf_p95_block_time_ms=0.323633`
- `perf_meets_deadline=true`
- `perf_allocation_free=true`

52. High-emitter guardrail stress (`16` emitters, `48k/512`)

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json --sample-rate 48000 --block-size 512
```

Result: `PASS`
- `perf_avg_block_time_ms=0.412833`
- `perf_p95_block_time_ms=0.433221`
- `perf_meets_deadline=true`
- `perf_allocation_free=true`

53. Renderer CPU guardrail suite rollup

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_9_renderer_cpu_suite.json --sample-rate 48000 --block-size 512
```

Result: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)

54. Smoke-suite regression after guardrail changes

```sh
./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json
```

Result: `PASS` (`4 PASS / 0 WARN / 0 FAIL`)

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_build_phase_2_9_renderer_cpu_guard_20260219T194552Z.log`
- `plugins/LocusQ/TestEvidence/locusq_26_full_system_cpu_draft_phase_2_9_guardrail_20260219T194552Z.log`
- `plugins/LocusQ/TestEvidence/locusq_29_renderer_guardrail_high_emitters_20260219T194552Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_9_renderer_cpu_suite_20260219T194552Z.log`
- `plugins/LocusQ/TestEvidence/locusq_smoke_suite_phase_2_9_guardrail_20260219T194552Z.log`

## Phase 2.11 Preset/Snapshot Migration Hardening Snapshot (UTC 2026-02-19)

55. Build QA target for snapshot-migration hardening checks

```sh
cmake --build build --target locusq_qa -j 1
```

Result: `PASS`

56. Run stereo migration suite (legacy + layout-mismatch checkpoint scenarios)

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_11_snapshot_migration_suite.json
```

Result: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)

57. Run quad legacy migration scenario (`--channels 4`)

```sh
./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_211_snapshot_migration_legacy_layout.json --channels 4
```

Result: `PASS`
- `non_finite=0`
- `peak_level=-50.1919`
- `rms_energy=-65.785`

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_qa_build_phase_2_11_snapshot_migration_20260219T194406Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_11_snapshot_migration_suite_stereo_20260219T194406Z.log`
- `plugins/LocusQ/TestEvidence/locusq_211_snapshot_migration_legacy_layout_quad4_20260219T194406Z.log`

## Phase 2.10b Renderer CPU Trend Expansion Snapshot (UTC 2026-02-19)

58. Build plugin + QA targets in local workspace for 2.10b trend matrix

```sh
cmake --build build_local --target LocusQ_VST3 locusq_qa -j 8
```

Result: `PASS`

59. Draft high-emitter guardrail matrix (`16` emitters): `48k/512` + `96k/512`, `2ch` + `4ch`

```sh
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 48000 --block-size 512 qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 48000 --block-size 512 --channels 4 qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 96000 --block-size 512 qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 96000 --block-size 512 --channels 4 qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json
```

Result: `PASS` (all 4 runs)
- `48k/512 ch2`: `perf_avg_block_time_ms=0.067616`, `perf_p95_block_time_ms=0.073208`, `perf_allocation_free=true`
- `48k/512 ch4`: `perf_avg_block_time_ms=0.0717895`, `perf_p95_block_time_ms=0.0769191`, `perf_allocation_free=true`
- `96k/512 ch2`: `perf_avg_block_time_ms=0.0678394`, `perf_p95_block_time_ms=0.074208`, `perf_allocation_free=true`
- `96k/512 ch4`: `perf_avg_block_time_ms=0.0725433`, `perf_p95_block_time_ms=0.0794191`, `perf_allocation_free=true`

60. Final-quality high-emitter guardrail matrix (`16` emitters): `48k/512` + `96k/512`, `2ch` + `4ch`

```sh
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 48000 --block-size 512 qa/scenarios/locusq_210b_renderer_guardrail_high_emitters_final_quality.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 48000 --block-size 512 --channels 4 qa/scenarios/locusq_210b_renderer_guardrail_high_emitters_final_quality.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 96000 --block-size 512 qa/scenarios/locusq_210b_renderer_guardrail_high_emitters_final_quality.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 96000 --block-size 512 --channels 4 qa/scenarios/locusq_210b_renderer_guardrail_high_emitters_final_quality.json
```

Result: `PASS` (all 4 runs)
- `48k/512 ch2`: `perf_avg_block_time_ms=0.068104`, `perf_p95_block_time_ms=0.074083`, `perf_total_allocations=0`
- `48k/512 ch4`: `perf_avg_block_time_ms=0.0715641`, `perf_p95_block_time_ms=0.0775831`, `perf_total_allocations=0`
- `96k/512 ch2`: `perf_avg_block_time_ms=0.0675089`, `perf_p95_block_time_ms=0.076041`, `perf_total_allocations=0`
- `96k/512 ch4`: `perf_avg_block_time_ms=0.0717689`, `perf_p95_block_time_ms=0.0775861`, `perf_total_allocations=0`

61. 2.10b trend-suite matrix (`48k/512` + `96k/512`, `2ch` + `4ch`)

```sh
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 48000 --block-size 512 qa/scenarios/locusq_phase_2_10b_renderer_cpu_trend_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 48000 --block-size 512 --channels 4 qa/scenarios/locusq_phase_2_10b_renderer_cpu_trend_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 96000 --block-size 512 qa/scenarios/locusq_phase_2_10b_renderer_cpu_trend_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 96000 --block-size 512 --channels 4 qa/scenarios/locusq_phase_2_10b_renderer_cpu_trend_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL` in each run)

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_build_phase_2_10b_renderer_cpu_trend_20260219T202603Z.log`
- `plugins/LocusQ/TestEvidence/locusq_29_renderer_guardrail_high_emitters_48k512_ch2_phase_2_10b_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_29_renderer_guardrail_high_emitters_48k512_ch4_phase_2_10b_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_29_renderer_guardrail_high_emitters_96k512_ch2_phase_2_10b_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_29_renderer_guardrail_high_emitters_96k512_ch4_phase_2_10b_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_48k512_ch2_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_48k512_ch4_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_96k512_ch2_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_210b_renderer_guardrail_high_emitters_final_quality_96k512_ch4_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_48k512_ch2_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_48k512_ch4_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_96k512_ch2_20260219T202524Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_10b_renderer_cpu_trend_suite_96k512_ch4_20260219T202524Z.log`

## Phase 2.11b Snapshot Migration Matrix Expansion Snapshot (UTC 2026-02-19)

62. Reconfigure local QA build for standalone `LocusQ` repo path

```sh
cmake -S . -B build -DBUILD_LOCUSQ_QA=ON -DJUCE_DIR=/Users/artbox/Documents/Repos/audio-plugin-coder/_tools/JUCE -DQA_HARNESS_DIR=/Users/artbox/Documents/Repos/audio-dsp-qa-harness -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Result: `PASS`

63. Rebuild `locusq_qa` after snapshot-migration matrix mode expansion

```sh
cmake --build build --target locusq_qa -j 1
```

Result: `PASS`

64. Run mono runtime migration suite (`2.11b`)

```sh
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11b_snapshot_migration_mono_suite.json
```

Result: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)

65. Run stereo runtime migration suite (`2.11b`)

```sh
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11b_snapshot_migration_stereo_suite.json
```

Result: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)

66. Run quad runtime migration suite (`2.11b`)

```sh
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11b_snapshot_migration_quad_suite.json
```

Result: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)

Artifacts:
- `plugins/LocusQ/TestEvidence/locusq_configure_phase_2_11b_snapshot_migration_matrix_20260219T202551Z.log`
- `plugins/LocusQ/TestEvidence/locusq_qa_build_phase_2_11b_snapshot_migration_matrix_20260219T202551Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_11b_snapshot_migration_mono_suite_20260219T202742Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_11b_snapshot_migration_stereo_suite_20260219T202742Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_11b_snapshot_migration_quad_suite_20260219T202742Z.log`

## Non-Manual Acceptance Matrix Rerun Snapshot (UTC 2026-02-19)

Run ID: `test_non_manual_acceptance_matrix_rerun_20260219T202424Z`

67. Run harness sanity (configure + build + ctest)

```sh
cmake -S /Users/artbox/Documents/Repos/audio-dsp-qa-harness -B /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test
ctest --test-dir /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test --output-on-failure
```

Result: `PASS` (`45/45`)

68. Recover local plugin build after stale cache path mismatch and build QA + host targets

```sh
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --target LocusQ_VST3 LocusQ_Standalone locusq_qa -j 8
```

Result: `PASS`

69. Run non-manual QA acceptance matrix + expansion suites

```sh
./build/locusq_qa_artefacts/Release/locusq_qa qa/scenarios/locusq_smoke_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_animation_internal_smoke.json --sample-rate 48000 --block-size 512
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json --sample-rate 48000 --block-size 512
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 44100 --block-size 256
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 512
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 1024
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 96000 --block-size 512
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_mono_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_stereo_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_quad_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_9_renderer_cpu_suite.json --sample-rate 48000 --block-size 512
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11_snapshot_migration_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_211_snapshot_migration_legacy_layout.json --channels 4
```

Result: `FAIL` (blocking regression in `phase_2_5`), with additional non-blocking `2.8` warnings
- `locusq_phase_2_5_acceptance_suite`: `8 PASS / 0 WARN / 1 FAIL`
- `locusq_phase_2_6_acceptance_suite`: `3 PASS / 0 WARN / 0 FAIL`
- `locusq_phase_2_8_output_layout_mono_suite`: `3 PASS / 0 WARN / 0 FAIL`
- `locusq_phase_2_8_output_layout_stereo_suite`: `2 PASS / 1 WARN / 0 FAIL`
- `locusq_phase_2_8_output_layout_quad_suite`: `2 PASS / 1 WARN / 0 FAIL`
- `locusq_phase_2_9_renderer_cpu_suite`: `2 PASS / 0 WARN / 0 FAIL`
- `locusq_phase_2_11_snapshot_migration_suite`: `2 PASS / 0 WARN / 0 FAIL`

70. Recheck blocking regression for deterministic triage

```sh
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_25_room_depth_no_coloring.json
```

Result: `FAIL` (reproducible)
- `locusq_25_room_depth_no_coloring`: `signal_present` failed (`rms=-81.8862`, min `-80.0`)

71. Host validation (`pluginval` + standalone open smoke)

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 30000 build/LocusQ_artefacts/Release/VST3/LocusQ.vst3
open build/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS`
- pluginval: `SUCCESS`, exit code `0`
- standalone open: exit code `0`

Artifacts:
- `plugins/LocusQ/TestEvidence/test_non_manual_acceptance_matrix_rerun_20260219T202424Z_status.tsv`
- `plugins/LocusQ/TestEvidence/harness_configure_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/harness_build_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/harness_ctest_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_reconfigure_clean_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_reconfigure_policy_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_plugin_build_vst3_standalone_final_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_5_acceptance_suite_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_25_room_depth_no_coloring_recheck_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_8_output_layout_stereo_suite_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_8_output_layout_quad_suite_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_9_renderer_cpu_suite_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/locusq_phase_2_11_snapshot_migration_suite_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`
- `plugins/LocusQ/TestEvidence/pluginval_test_non_manual_acceptance_matrix_rerun_20260219T202424Z_stdout.log`
- `plugins/LocusQ/TestEvidence/pluginval_test_non_manual_acceptance_matrix_rerun_20260219T202424Z_stderr.log`
- `plugins/LocusQ/TestEvidence/pluginval_test_non_manual_acceptance_matrix_rerun_20260219T202424Z_exit_code.txt`
- `plugins/LocusQ/TestEvidence/standalone_test_non_manual_acceptance_matrix_rerun_20260219T202424Z.log`

## Non-Manual Acceptance Matrix Post-Fix Snapshot (UTC 2026-02-19)

Run ID: `test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z`

72. Verify phase 2.5 blocker scenario + suite after room-depth gain retune

```sh
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_25_room_depth_no_coloring.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json
```

Result: `PASS`
- `locusq_25_room_depth_no_coloring`: `rms_energy=-78.2862` (`min=-80.0`)
- `locusq_phase_2_5_acceptance_suite`: `9 PASS / 0 WARN / 0 FAIL`

73. Re-run non-manual acceptance matrix (harness sanity + plugin build + QA suites + host validation)

```sh
cmake -S /Users/artbox/Documents/Repos/audio-dsp-qa-harness -B /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test
ctest --test-dir /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test --output-on-failure
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --target LocusQ_VST3 LocusQ_Standalone locusq_qa -j 8
./build/locusq_qa_artefacts/Release/locusq_qa qa/scenarios/locusq_smoke_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_stereo_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_quad_suite.json
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_9_renderer_cpu_suite.json --sample-rate 48000 --block-size 512
./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11_snapshot_migration_suite.json
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 30000 build/LocusQ_artefacts/Release/VST3/LocusQ.vst3
open build/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS_WITH_WARNING`
- Hard-fail suites/scenarios: all `PASS`
- Remaining soft warnings:
  - `locusq_phase_2_8_output_layout_stereo_suite`: `2 PASS / 1 WARN / 0 FAIL`
  - `locusq_phase_2_8_output_layout_quad_suite`: `2 PASS / 1 WARN / 0 FAIL`
- Perf/host snapshot:
  - `qa_full_system_48k512`: `perf_avg_block_time_ms=0.0669724`, `perf_p95_block_time_ms=0.0832101`, `perf_allocation_free=true`
  - `qa_guardrail_48k512`: `perf_avg_block_time_ms=0.0850263`, `perf_p95_block_time_ms=0.113294`, `perf_allocation_free=true`
  - `pluginval` strictness 5: `SUCCESS`
  - standalone open smoke: `PASS`

Artifacts:
- `plugins/LocusQ/TestEvidence/test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z_status.tsv`
- `plugins/LocusQ/TestEvidence/harness_configure_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/harness_build_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/harness_ctest_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/locusq_reconfigure_policy_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/locusq_plugin_build_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_5_suite_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_6_suite_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_stereo_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_quad_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_9_suite_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_11_suite_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/pluginval_strict5_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/standalone_open_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`

## Full Acceptance Rerun Bridge-Fix Snapshot (UTC 2026-02-19)

Run ID: `test_full_acceptance_rerun_bridge_fix_20260219T212613Z`

74. Rebuild plugin + QA targets after host-bridge triage fix

```sh
cmake -S . -B build_local -DBUILD_LOCUSQ_QA=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build_local --target LocusQ_VST3 LocusQ_AU LocusQ_Standalone locusq_qa --config Release -j 4
```

Result: `PASS`

75. Execute full acceptance non-manual matrix (harness + suites + perf/host + pluginval + standalone)

```sh
cmake -S /Users/artbox/Documents/Repos/audio-dsp-qa-harness -B /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test -j 4
ctest --test-dir /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_test --output-on-failure
./build_local/locusq_qa_artefacts/Release/locusq_qa qa/scenarios/locusq_smoke_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_stereo_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_quad_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_9_renderer_cpu_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11_snapshot_migration_suite.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial --sample-rate 48000 --block-size 512 qa/scenarios/locusq_26_full_system_cpu_draft.json
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 30000 build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3
open -g build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS_WITH_WARNING`
- Harness: `PASS` (`45/45`) on immediate retry (`performance_profiler_test` transient on first ctest pass).
- Core suites: `PASS`
  - `locusq_smoke_suite`: `4 PASS / 0 WARN / 0 FAIL`
  - `locusq_phase_2_5_acceptance_suite`: `9 PASS / 0 WARN / 0 FAIL`
  - `locusq_phase_2_6_acceptance_suite`: `3 PASS / 0 WARN / 0 FAIL`
  - `locusq_phase_2_9_renderer_cpu_suite`: `2 PASS / 0 WARN / 0 FAIL`
  - `locusq_phase_2_11_snapshot_migration_suite`: `2 PASS / 0 WARN / 0 FAIL`
- Warn-only suites (non-blocking):
  - `locusq_phase_2_8_output_layout_stereo_suite`: `2 PASS / 1 WARN / 0 FAIL`
  - `locusq_phase_2_8_output_layout_quad_suite`: `2 PASS / 1 WARN / 0 FAIL`
- Perf snapshot:
  - `qa_full_system_48k512`: `perf_avg_block_time_ms=0.0687689`, `perf_p95_block_time_ms=0.0904191`, `perf_allocation_free=true`
- Host snapshot:
  - `pluginval` strictness 5 (`skip GUI`): `SUCCESS`
  - `pluginval` strictness 5 (`with GUI`): `SUCCESS` (Editor Automation completed)
  - standalone open smoke: `PASS`

Artifacts:
- `plugins/LocusQ/TestEvidence/test_full_acceptance_rerun_bridge_fix_20260219T212613Z_status.tsv`
- `plugins/LocusQ/TestEvidence/harness_configure_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/harness_build_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/harness_ctest_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/harness_ctest_retry_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_smoke_suite_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_5_suite_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_6_suite_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_stereo_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_quad_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_9_suite_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_11_suite_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_full_system_48k512_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_host_edge_roundtrip_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/pluginval_strict5_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/pluginval_strict5_with_gui_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/standalone_open_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`

## Incremental Stage 2 Shell (UTC 2026-02-20)

76. Rebuild standalone with incremental Stage 2 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

77. Standalone launch/title smoke after Stage 2 routing

```sh
open build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
pgrep -x LocusQ | wc -l
osascript # query front window title/geometry
```

Result: `PASS` (`process_count=1`, window title `LocusQ v0.1.0 [incremental-stage2]`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage2_20260220T015353Z.log`
- `TestEvidence/locusq_standalone_incremental_stage2_smoke_20260220T015702Z.log`
- `TestEvidence/locusq_incremental_stage2_window_20260220T020057Z.png`

## Incremental Stage 3 Emitter Audio Block (UTC 2026-02-20)

78. Rebuild standalone with incremental Stage 3 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

79. Standalone smoke after Stage 3 default-route switch

```sh
osascript -e 'tell application "LocusQ" to quit' || true
pkill -x LocusQ || true
open -g build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
pgrep -x LocusQ | wc -l
osascript # optional window title probe (may require Accessibility permission)
```

Result: `PASS` (`process_count=1`; window-title probe unavailable in this run due OS automation visibility)

80. Resource-provider probe for active Stage 3 payload

```sh
rg -n "incremental/index.html|incremental/js/stage3_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `17624`; `incremental/js/stage3_ui.js` loaded with size `48326`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage3_20260220T020821Z.log`
- `TestEvidence/locusq_standalone_incremental_stage3_smoke_20260220T020854Z.log`
- `TestEvidence/locusq_incremental_stage3_resource_probe_20260220T020938Z.log`

## Incremental Stage 4 Emitter Audio Extended (UTC 2026-02-20)

81. Rebuild standalone with incremental Stage 4 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

82. Standalone smoke after Stage 4 default-route switch

```sh
osascript -e 'tell application "LocusQ" to quit' || true
pkill -x LocusQ || true
open -g build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
pgrep -x LocusQ | wc -l
osascript # optional window title probe (may require Accessibility permission)
```

Result: `PASS` (`process_count=1`; window-title probe unavailable in this run due OS automation visibility)

83. Resource-provider probe for active Stage 4 payload

```sh
rg -n "incremental/index.html|incremental/js/stage4_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `18284`; `incremental/js/stage4_ui.js` loaded with size `50147`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage4_20260220T021738Z.log`
- `TestEvidence/locusq_standalone_incremental_stage4_smoke_20260220T021804Z.log`
- `TestEvidence/locusq_incremental_stage4_resource_probe_20260220T021814Z.log`

## Incremental Stage 4 UI Self-Test Automation (UTC 2026-02-20)

84. Rebuild standalone with Stage 4 self-test hooks

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

85. Run automated Stage 4 UI interaction self-test

```sh
scripts/standalone-ui-selftest-stage4-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage4_selftestclean_20260220T023706Z.log`
- `TestEvidence/locusq_incremental_stage4_selftest_20260220T023730Z.json`
- `TestEvidence/locusq_incremental_stage4_selftest_20260220T023730Z.run.log`

## Incremental Stage 4 UI PR Gate Default Self-Test (UTC 2026-02-20)

86. Re-run Stage 4 self-test using `.app` path input

```sh
scripts/standalone-ui-selftest-stage4-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

87. Run UI PR gate with default self-test-first sequencing

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage4_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

Artifacts:
- `TestEvidence/locusq_incremental_stage4_selftest_20260220T024215Z.json`
- `TestEvidence/locusq_incremental_stage4_selftest_20260220T024215Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T024215Z/status.tsv`

## Incremental Stage 5 Renderer Core + Automation (UTC 2026-02-20)

88. Rebuild standalone with incremental Stage 5 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

89. Run automated Stage 5 self-test against standalone app path

```sh
scripts/standalone-ui-selftest-stage5-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

90. Run UI PR gate with Stage 5 self-test default

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage5_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

91. Resource-provider probe for active Stage 5 payload

```sh
rg -n "incremental/index.html|incremental/js/stage5_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `19808`; `incremental/js/stage5_ui.js` loaded with size `72020`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage5_20260220T025040Z.log`
- `TestEvidence/locusq_incremental_stage5_selftest_20260220T025103Z.json`
- `TestEvidence/locusq_incremental_stage5_selftest_20260220T025103Z.run.log`
- `TestEvidence/locusq_incremental_stage5_selftest_20260220T025111Z.json`
- `TestEvidence/locusq_incremental_stage5_selftest_20260220T025111Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T025111Z/status.tsv`
- `TestEvidence/locusq_incremental_stage5_resource_probe_20260220T025122Z.log`

## Incremental Stage 6 Calibrate Core + Automation (UTC 2026-02-20)

92. Rebuild standalone with incremental Stage 6 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

93. Run automated Stage 6 self-test against standalone app path

```sh
scripts/standalone-ui-selftest-stage6-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

94. Run UI PR gate with Stage 6 self-test default

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage6_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

95. Resource-provider probe for active Stage 6 payload

```sh
rg -n "incremental/index.html|incremental/js/stage6_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `20797`; `incremental/js/stage6_ui.js` loaded with size `82163`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage6_20260220T030058Z.log`
- `TestEvidence/locusq_incremental_stage6_selftest_20260220T030123Z.json`
- `TestEvidence/locusq_incremental_stage6_selftest_20260220T030123Z.run.log`
- `TestEvidence/locusq_incremental_stage6_selftest_20260220T030133Z.json`
- `TestEvidence/locusq_incremental_stage6_selftest_20260220T030133Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T030133Z/status.tsv`
- `TestEvidence/locusq_incremental_stage6_resource_probe_20260220T030144Z.log`

## Incremental Stage 7 Calibrate Speaker Output Routing + Automation (UTC 2026-02-20)

96. Rebuild standalone with incremental Stage 7 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

97. Run automated Stage 7 self-test against standalone app path

```sh
scripts/standalone-ui-selftest-stage7-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

98. Run UI PR gate with Stage 7 self-test default

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage7_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

99. Resource-provider probe for active Stage 7 payload

```sh
rg -n "incremental/index.html|incremental/js/stage7_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `21540`; `incremental/js/stage7_ui.js` loaded with size `89844`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage7_20260220T031152Z.log`
- `TestEvidence/locusq_incremental_stage7_selftest_20260220T031217Z.json`
- `TestEvidence/locusq_incremental_stage7_selftest_20260220T031217Z.run.log`
- `TestEvidence/locusq_incremental_stage7_selftest_20260220T031226Z.json`
- `TestEvidence/locusq_incremental_stage7_selftest_20260220T031226Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T031226Z/status.tsv`
- `TestEvidence/locusq_incremental_stage7_resource_probe_20260220T031234Z.log`

## Incremental Stage 8 Calibrate Capture/Progress + Automation (UTC 2026-02-20)

100. Rebuild standalone with incremental Stage 8 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

101. Run automated Stage 8 self-test against standalone app path

```sh
scripts/standalone-ui-selftest-stage8-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

102. Run UI PR gate with Stage 8 self-test default

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage8_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

103. Resource-provider probe for active Stage 8 payload

```sh
rg -n "incremental/index.html|incremental/js/stage8_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `24647`; `incremental/js/stage8_ui.js` loaded with size `98723`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage8_20260220T032953Z.log`
- `TestEvidence/locusq_incremental_stage8_selftest_20260220T033017Z.json`
- `TestEvidence/locusq_incremental_stage8_selftest_20260220T033017Z.run.log`
- `TestEvidence/locusq_incremental_stage8_selftest_20260220T033031Z.json`
- `TestEvidence/locusq_incremental_stage8_selftest_20260220T033031Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T033031Z/status.tsv`
- `TestEvidence/locusq_incremental_stage8_resource_probe_20260220T033040Z.log`

## Stage 9+ Checklist Planning Snapshot (UTC 2026-02-20)

104. Validate docs freshness after creating Stage 9+ detailed parity checklist documentation

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Artifacts:
- Updated docs:
  - `Documentation/v3-stage-9-plus-detailed-checklists.md`
  - `Documentation/v3-ui-parity-checklist.md`
  - `Documentation/README.md`

## Incremental Stage 10 Renderer Parity + Automation (UTC 2026-02-20)

105. Rebuild standalone with incremental Stage 10 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

106. Run automated Stage 10 self-test against standalone app path

```sh
scripts/standalone-ui-selftest-stage10-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

107. Run UI PR gate with Stage 10 self-test default

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage10_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

108. Resource-provider probe for active Stage 10 payload

```sh
rg -n "incremental/index.html|incremental/js/stage10_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `36723`; `incremental/js/stage10_ui.js` loaded with size `194438`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage10_20260220T173255Z.log`
- `TestEvidence/locusq_incremental_stage10_selftest_20260220T173332Z.json`
- `TestEvidence/locusq_incremental_stage10_selftest_20260220T173332Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T173332Z/status.tsv`
- `TestEvidence/locusq_incremental_stage10_resource_probe_20260220T173344Z.log`

## Incremental Stage 11 Calibrate Workflow Parity + Automation (UTC 2026-02-20)

109. Rebuild standalone with incremental Stage 11 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

110. Run automated Stage 11 self-test against standalone app path

```sh
scripts/standalone-ui-selftest-stage11-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

111. Run UI PR gate with Stage 11 self-test default

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage11_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

112. Resource-provider probe for active Stage 11 payload

```sh
rg -n "incremental/index.html|incremental/js/stage11_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `37293`; `incremental/js/stage11_ui.js` loaded with size `201827`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage11_20260220T174725Z.log`
- `TestEvidence/locusq_incremental_stage11_selftest_20260220T174757Z.json`
- `TestEvidence/locusq_incremental_stage11_selftest_20260220T174757Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T174757Z/status.tsv`
- `TestEvidence/locusq_incremental_stage11_resource_probe_20260220T174808Z.log`

## Incremental Stage 12 Visual Polish + Primary Route Promotion (UTC 2026-02-20)

113. Rebuild standalone with incremental Stage 12 WebView resources

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

114. Run automated Stage 12 self-test against standalone app path

```sh
scripts/standalone-ui-selftest-stage12-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

115. Run UI PR gate with Stage 12 self-test default

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage12_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

116. Resource-provider probe for active Stage 12 payload

```sh
rg -n "incremental/index.html|incremental/js/stage12_ui.js" "$HOME/Library/LocusQ/resource_requests.log"
```

Result: `PASS` (`incremental/index.html` loaded with size `37563`; `incremental/js/stage12_ui.js` loaded with size `202265`)

Artifacts:
- `TestEvidence/locusq_build_incremental_stage12_20260220T175454Z.log`
- `TestEvidence/locusq_incremental_stage12_selftest_20260220T175530Z.json`
- `TestEvidence/locusq_incremental_stage12_selftest_20260220T175530Z.run.log`
- `TestEvidence/ui_pr_gate_20260220T175530Z/status.tsv`
- `TestEvidence/locusq_incremental_stage12_resource_probe_20260220T175539Z.log`

## Stage 13 Final Acceptance Sweep Snapshot (UTC 2026-02-20)

117. Run Stage 12 standalone self-test on promoted app

```sh
scripts/standalone-ui-selftest-stage12-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

118. Run Stage 12 UI PR gate on promoted app

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage12_selftest=PASS`; `ui_smoke_fast_gate=SKIP`; `ui_regression_appium=SKIP`)

119. Run targeted non-UI parity matrix (smoke + acceptance suites + host edge)

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa qa/scenarios/locusq_smoke_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 44100 --block-size 256
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 512
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 1024
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 96000 --block-size 512
```

Result: `PASS`
- `locusq_smoke_suite`: `4 PASS / 0 WARN / 0 FAIL`
- `locusq_phase_2_5_acceptance_suite`: `9 PASS / 0 WARN / 0 FAIL`
- `locusq_phase_2_6_acceptance_suite`: `3 PASS / 0 WARN / 0 FAIL`
- host edge roundtrip: `PASS` across `44.1k/256`, `48k/512`, `48k/1024`, `96k/512`

120. Run host validation on promoted artifacts (`pluginval` + standalone smoke)

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3
open -g build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`pluginval SUCCESS`; standalone process launch observed)

121. Run docs freshness gate after Stage 13 closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Artifacts:
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/status.tsv`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/ui_stage12_selftest.log`
- `TestEvidence/locusq_incremental_stage12_selftest_20260220T180204Z.json`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/ui_pr_gate_stage12.log`
- `TestEvidence/ui_pr_gate_20260220T180214Z/status.tsv`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_smoke_suite.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_phase_2_5_acceptance_suite.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_phase_2_6_acceptance_suite.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_44k1_256.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_48k512.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_48k1024.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/qa_host_edge_96k512.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/pluginval_strict5_skip_gui.log`
- `TestEvidence/stage13_acceptance_sweep_20260220T180204Z/standalone_open_smoke.log`

## QA Contract-Pack Backport (UTC 2026-02-20)

122. Build LocusQ QA target after runtime-config + contract-pack wiring

```sh
cmake --build build_local --config Release --target locusq_qa -j 8
```

Result: `PASS`

123. Run new contract-pack suite (spatial adapter)

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_contract_pack_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)

124. Run docs freshness gate after QA/doc sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Artifacts:
- `TestEvidence/locusq_qa_contract_pack_build_20260220T185755Z.log`
- `TestEvidence/locusq_contract_pack_suite_20260220T185830Z.log`

## Stage 14 Install Automation + Host Cache Hygiene (UTC 2026-02-20)

125. Run canonical macOS build/install automation with host-cache hygiene enabled

```sh
LOCUSQ_REAPER_AUTO_QUIT=0 ./scripts/build-and-install-mac.sh
```

Result: `PASS`
- VST3/AU build + install completed
- AU registrar refresh executed
- REAPER cache rows for `LocusQ` were pruned with timestamped backups
- Installed AU/VST3 binary hashes match build artefacts

126. Verify REAPER cache files no longer carry stale `LocusQ` registry rows

```sh
for f in "$HOME/Library/Application Support/REAPER"/reaper-vstplugins*.ini "$HOME/Library/Application Support/REAPER"/reaper-auplugins*.ini "$HOME/Library/Application Support/REAPER"/reaper-recentfx.ini "$HOME/Library/Application Support/REAPER"/reaper-fxtags.ini; do rg -n "LocusQ|LcQd|Nfld" "$f"; done
```

Result: `PASS` (no matching rows in scanned cache files)

127. Run docs freshness gate after Stage 14 review/install automation sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Artifacts:
- `TestEvidence/locusq_build_install_mac_20260220T190808Z.log`
