Title: LocusQ Build Summary (Acceptance Closeout)
Document Type: Build Summary
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-24

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
- `Documentation/archive/2026-02-23-historical-review-bundles/stage14-review-release-checklist.md`

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
  - `Documentation/archive/2026-02-23-historical-review-bundles/v3-stage-9-plus-detailed-checklists.md`
  - `Documentation/archive/2026-02-23-historical-review-bundles/v3-ui-parity-checklist.md`
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

## Stage 14 `rend_phys_interact` Runtime + Stage 12 Binding (UTC 2026-02-20)

128. Rebuild standalone after runtime/UI interaction binding

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

129. Run Stage 12 standalone self-test on rebuilt app

```sh
scripts/standalone-ui-selftest-stage12-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`status=pass`, `ok=true`)

130. Run Stage 12 UI PR gate (self-test default lane)

```sh
scripts/ui-pr-gate-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS` (`ui_stage12_selftest=PASS`)

131. Verify installed AU/VST3 binaries match current build artifacts

```sh
shasum/stat verification for:
- build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3/Contents/MacOS/LocusQ
- build_local/LocusQ_artefacts/Release/AU/LocusQ.component/Contents/MacOS/LocusQ
- ~/Library/Audio/Plug-Ins/VST3/LocusQ.vst3/Contents/MacOS/LocusQ
- ~/Library/Audio/Plug-Ins/Components/LocusQ.component/Contents/MacOS/LocusQ
```

Result: `PASS` (`match_vst3=true`, `match_au=true`)

132. Run docs freshness gate after Stage 14 drift-resolution updates

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

Artifacts:
- `TestEvidence/locusq_incremental_stage12_selftest_20260220T193020Z.json`
- `TestEvidence/locusq_incremental_stage12_selftest_20260220T193031Z.json`
- `TestEvidence/ui_pr_gate_20260220T193031Z/status.tsv`

## Stage 15 Manual DAW Acceptance (UTC 2026-02-20)

133. Execute portable-device DEV acceptance rows (`DEV-01..DEV-06`) with operator-in-the-loop run

```sh
manual operator execution:
- DEV-01 standalone laptop speakers
- DEV-02 standalone headphones
- DEV-03 DAW (Reaper) laptop speakers
- DEV-04 DAW (Reaper) headphones
- DEV-05 DAW (Reaper) built-in mic calibrate start/abort
- DEV-06 external mic calibrate start/abort (if available)
```

Result: `PASS_WITH_NA`
- `DEV-01..DEV-05`: `PASS`
- `DEV-06`: `N/A` (external mic unavailable during this run)

Artifacts:
- `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`

## Stage 17-A Portable Device Acceptance Repeat (UTC 2026-02-20)

134. Run fresh macOS build/install prerequisite for portable-profile rerun

```sh
./scripts/build-and-install-mac.sh
```

Result: `PASS`
- AU/VST3 rebuild + install succeeded
- AudioComponentRegistrar refresh succeeded
- REAPER cache prune/backup steps completed
- Installed binary hashes match build artefacts

135. Stage 17-A manual portable-device rerun gate handoff (`DEV-01..DEV-06`)

```sh
manual operator rerun required:
- DEV-01 standalone laptop speakers
- DEV-02 standalone headphones
- DEV-03 DAW (Reaper) laptop speakers
- DEV-04 DAW (Reaper) headphones
- DEV-05 DAW (Reaper) built-in mic calibrate start/abort
- DEV-06 external mic calibrate start/abort (if available)
```

Result: `PENDING_OPERATOR_RERUN`
- Stage 17-A remains open until manual rows are re-executed with Stage 16 hardening in place.
- If any DEV row fails, block GA and file a defect issue with repro + evidence path.

Artifacts:
- `TestEvidence/stage17a_portable_acceptance_20260220T231840Z/build_and_install.log`
- `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`

## P0 Runtime Patch Addendum (UTC 2026-02-21)

1. Rebuild plugin targets after production UI baseline switch + physics preset hardening

```sh
cmake --build build_local --config Release --target LocusQ_VST3 LocusQ_Standalone -j 8
```

Result: `PASS` (warnings only; no compile/link errors)

2. Stage12 incremental self-test gate (fallback path verification)

```sh
./scripts/ui-pr-gate-mac.sh
```

Result: `PASS` (`ui_stage12_selftest=PASS`)

3. Production standalone smoke automation

```sh
./scripts/standalone-ui-smoke-mac.sh
```

Result: `FAIL` (`0/6` visual deltas; script click-coordinate assumptions no longer match current production layout)

4. QA smoke suite (Spatial adapter)

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
```

Result: `WARN` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)

5. QA Phase 2.6 acceptance suite (Spatial adapter)

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)

### Artifacts (2026-02-21)

- UI PR gate status: `TestEvidence/ui_pr_gate_20260221T001557Z/status.tsv`
- Stage12 self-test JSON: `TestEvidence/locusq_incremental_stage12_selftest_20260221T001557Z.json`
- Production standalone smoke summary: `TestEvidence/standalone_ui_smoke_20260221T001608Z/summary.tsv`
- Smoke suite log (spatial): `TestEvidence/locusq_smoke_suite_spatial_p0_20260221T001710Z.log`
- Phase 2.6 acceptance suite log (spatial): `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_p0_20260221T001710Z.log`

### Notes

- Non-spatial adapter execution for `locusq_phase_2_6_acceptance_suite` produces runner-prep `ERROR`; use `--spatial` for this suite.
- Manual host DAW rerun remains required for closure of checklist rows `UI-04`, `UI-06`, `UI-07`, `UI-12`.

## P0 Runtime Patch Addendum II (UTC 2026-02-21)

1. Production P0 self-test on production route (`UI-04/UI-06/UI-07/UI-12`)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `FAIL` (initial run: `UI-12` save completion timeout)

2. Production P0 self-test rerun after preset-save selftest harness fix

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`)

3. Shared gate rerun after production self-test closure

```sh
./scripts/ui-pr-gate-mac.sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./scripts/validate-docs-freshness.sh
```

Result:
- `ui-pr-gate`: `PASS` (`ui_stage12_selftest=PASS`)
- `smoke suite (spatial)`: `WARN` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)
- `phase 2.6 suite (spatial)`: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)
- `docs freshness`: `PASS` (`0 warning(s)`)

### Artifacts (2026-02-21)

- Production self-test fail JSON: `TestEvidence/locusq_production_p0_selftest_20260221T005050Z.json`
- Production self-test pass JSON: `TestEvidence/locusq_production_p0_selftest_20260221T005145Z.json`
- Shared gate status: `TestEvidence/ui_pr_gate_20260221T005211Z/status.tsv`
- Smoke suite log (spatial rerun): `TestEvidence/locusq_smoke_suite_spatial_p0_20260221T005221Z.log`
- Phase 2.6 suite log (spatial rerun): `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_p0_20260221T005221Z.log`

### Notes

- P0 automated gates now pass for BL-002/BL-003/BL-004/BL-005.
- Manual DAW checklist rerun remains the only exit gate for moving BL-002 through BL-005 to `Done`.

## P1 BL-016 Kickoff Addendum (UTC 2026-02-21)

1. Rebuild standalone after scene-snapshot transport contract implementation

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS` (warnings only; no compile/link errors)

2. Production regression self-test after transport hardening

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`)

3. Shared gate rerun

```sh
./scripts/ui-pr-gate-mac.sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./scripts/validate-docs-freshness.sh
```

Result:
- `ui-pr-gate`: `PASS` (`ui_stage12_selftest=PASS`)
- `smoke suite (spatial)`: `WARN` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)
- `phase 2.6 suite (spatial)`: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)
- `docs freshness`: `PASS` (`0 warning(s)`)

### Artifacts (2026-02-21)

- Production self-test JSON: `TestEvidence/locusq_production_p0_selftest_20260221T013140Z.json`
- Shared gate status: `TestEvidence/ui_pr_gate_20260221T013208Z/status.tsv`
- Smoke suite log (spatial rerun): `TestEvidence/locusq_smoke_suite_spatial_bl016_20260221T013152Z.log`
- Phase 2.6 suite log (spatial rerun): `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_bl016_20260221T013152Z.log`

## P1 BL-015/BL-014/BL-006/BL-007 Validation Addendum (UTC 2026-02-21)

1. Production self-test expansion run 1 (`UI-P1-015` failure capture)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `FAIL` (`UI-P1-015`: synthetic multi-emitter meshes were not created)

2. Production self-test expansion run 2 (`UI-P1-015` failure capture)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `FAIL` (`UI-P1-015`: synthetic multi-emitter meshes were not created)

3. Production self-test expansion run 3 (`UI-P1-015` failure capture)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `FAIL` (`UI-P1-015`: synthetic multi-emitter meshes were not created)

4. Production self-test rerun after synthetic-scene hold/fallback hardening

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`)

5. Shared gate rerun

```sh
./scripts/ui-pr-gate-mac.sh
```

Result: `PASS` (`ui_stage12_selftest=PASS`)

6. QA rerun (spatial adapter) + docs freshness

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./scripts/validate-docs-freshness.sh
```

Result:
- `smoke suite (spatial)`: `WARN` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)
- `phase 2.6 suite (spatial)`: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)
- `docs freshness`: `PASS` (`0 warning(s)`)

### Artifacts (2026-02-21)

- Production self-test fail JSON #1: `TestEvidence/locusq_production_p0_selftest_20260221T014405Z.json`
- Production self-test fail JSON #2: `TestEvidence/locusq_production_p0_selftest_20260221T014527Z.json`
- Production self-test fail JSON #3: `TestEvidence/locusq_production_p0_selftest_20260221T014616Z.json`
- Production self-test pass JSON: `TestEvidence/locusq_production_p0_selftest_20260221T014724Z.json`
- Shared gate status: `TestEvidence/ui_pr_gate_20260221T014738Z/status.tsv`
- Smoke suite log (spatial rerun): `TestEvidence/locusq_smoke_suite_spatial_p1_20260221T014756Z.log`
- Phase 2.6 suite log (spatial rerun): `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_p1_20260221T014756Z.log`
- Docs freshness log: `TestEvidence/validate_docs_freshness_p1_20260221T014756Z.log`

### Notes

- Manual host DAW checks remain deferred by user direction; this addendum closes automated validation coverage for the P1 viewport slice.

## P1 BL-008 Audio-Reactive Telemetry Addendum (UTC 2026-02-21)

1. Rebuild standalone after BL-008 self-test assertion addition (`UI-P1-008`)

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS` (warnings only; no compile/link errors)

2. Production self-test rerun with BL-008 assertion active

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`; `UI-P1-008` check included)

3. Shared gate rerun

```sh
./scripts/ui-pr-gate-mac.sh
```

Result: `PASS` (`ui_stage12_selftest=PASS`)

4. QA + docs freshness rerun

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./scripts/validate-docs-freshness.sh
```

Result:
- `smoke suite (spatial)`: `WARN` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)
- `phase 2.6 suite (spatial)`: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)
- `docs freshness`: `PASS` (`0 warning(s)`)

### Artifacts (2026-02-21)

- Production self-test pass JSON (with `UI-P1-008`): `TestEvidence/locusq_production_p0_selftest_20260221T031550Z.json`
- Shared gate status: `TestEvidence/ui_pr_gate_20260221T031609Z/status.tsv`
- Smoke suite log (spatial rerun): `TestEvidence/locusq_smoke_suite_spatial_bl008_20260221T0316Z.log`
- Phase 2.6 suite log (spatial rerun): `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_bl008_20260221T0316Z.log`
- Docs freshness log: `TestEvidence/validate_docs_freshness_bl008_20260221T0316Z.log`

## Resume Baseline Realignment Addendum (UTC 2026-02-21)

1. Production self-test baseline rerun (pre-patch)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `FAIL` both runs (shared timeout: headphone mode request; artifacts below).

2. Scope-alignment patch rebuild

```sh
node --check Source/ui/public/js/index.js
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS` (syntax/build; warnings unchanged).

3. Production self-test rerun (post-patch)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`).

4. Shared gate + spatial suites + docs freshness rerun

```sh
./scripts/ui-pr-gate-mac.sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
./scripts/validate-docs-freshness.sh
```

Result:
- `UI PR gate`: `PASS` (`ui_stage12_selftest=PASS`)
- `smoke suite (spatial)`: `WARN` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)
- `phase 2.6 suite (spatial)`: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)
- `docs freshness`: `PASS` (`0 warning(s)`)

### Artifacts (2026-02-21 resume rerun)

- Pre-patch self-test fail #1: `TestEvidence/locusq_production_p0_selftest_20260221T083613Z.json`
- Pre-patch self-test fail #2: `TestEvidence/locusq_production_p0_selftest_20260221T083632Z.json`
- Post-patch self-test pass: `TestEvidence/locusq_production_p0_selftest_20260221T083905Z.json`
- Shared gate status: `TestEvidence/ui_pr_gate_20260221T083926Z/status.tsv`

## Manual Host Blocker Rerun Addendum (UTC 2026-02-21)

1. Install latest local plugin build before manual DAW rerun

```sh
./scripts/build-and-install-mac.sh
```

Result: `PASS` (VST3/AU install hashes match local build; AU registrar refreshed; REAPER cache rows pruned).

2. Manual host blocker rerun (`UI-04`, `UI-06`, `UI-07`, `UI-12`)

Result: `PARTIAL_PASS`
- `UI-04`: `PASS` (physics preset stickiness restored in host)
- `UI-06`: `FAIL` (transport controls do not behave coherently; rewind/play/stop behavior remains broken)
- `UI-07`: `FAIL` (timeline/keyframe bar is vertically clipped/squished in host, blocking gesture path)
- `UI-12`: `PASS` (`SAVE` creates preset and `LOAD` restores state; rename/delete remains UX gap)

### Artifacts (manual blocker rerun)

- Checklist + operator observations: `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`
- Host screenshot evidence for `UI-07`: chat attachment `Image #1` (2026-02-21)

## Manual Blocker Follow-Up Closeout Addendum (UTC 2026-02-21)

1. Transport/layout patch automated validation rerun

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
./scripts/ui-pr-gate-mac.sh
```

Result: `PASS`
- production self-test: `PASS` (`UI-06` and `UI-07` checks pass)
- shared UI PR gate: `PASS` (`ui_stage12_selftest=PASS`)

2. Manual host blocker follow-up confirmation

Result: `PASS`
- `UI-06`: `PASS`
- `UI-07`: `PASS`
- blocker defects are cleared for manual P0 closeout.

3. Docs freshness gate after manual closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

### Artifacts (manual blocker follow-up closeout)

- Production self-test pass JSON: `TestEvidence/locusq_production_p0_selftest_20260221T092320Z.json`
- Shared gate status TSV: `TestEvidence/ui_pr_gate_20260221T092337Z/status.tsv`
- Updated manual checklist: `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`

## P1 BL-006/BL-007/BL-008 Closeout Addendum (UTC 2026-02-21)

1. Coordinator closeout sync for completed viewport overlays slice

Result: `PASS`
- `BL-006` motion trails: closeout on automated evidence (`UI-P1-006` pass)
- `BL-007` velocity vectors: closeout on automated evidence (`UI-P1-007` pass)
- `BL-008` RMS telemetry overlays: closeout on automated evidence (`UI-P1-008` pass)
- backlog/status/evidence records synchronized to `Done`.

2. Docs freshness gate after closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

### Artifacts (BL-006/BL-007/BL-008 closeout)

- Production self-test (includes `UI-P1-006`/`UI-P1-007`/`UI-P1-008`): `TestEvidence/locusq_production_p0_selftest_20260221T092320Z.json`
- Shared UI gate status: `TestEvidence/ui_pr_gate_20260221T092337Z/status.tsv`
- Smoke suite log (spatial): `TestEvidence/locusq_smoke_suite_spatial_bl008_20260221T0316Z.log`
- Phase 2.6 suite log (spatial): `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_bl008_20260221T0316Z.log`
- Docs freshness log reference: `TestEvidence/validation-trend.md`

## P1 BL-009 Steam Binaural Addendum (UTC 2026-02-21)

1. Steam-enabled standalone build validation

```sh
cmake -S . -B build_local -DCMAKE_BUILD_TYPE=Release -DLOCUSQ_ENABLE_STEAM_AUDIO=ON
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS` (Steam-enabled runtime path compiles and links for standalone target).

2. Production self-test baseline after BL-009 integration patch set

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`).

3. BL-009 opt-in assertion run (`UI-P1-009`)

```sh
LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`; `UI-P1-009` pass).

4. Shared UI gate rerun after BL-009 integration

```sh
./scripts/ui-pr-gate-mac.sh
```

Result: `PASS` (`ui_stage12_selftest=PASS`).

5. Docs freshness check (post-BL-009 sync)

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## BL-016 Transport Contract Closeout Refresh (UTC 2026-02-23)

1. Production transport-contract regression self-test

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/locusq_production_p0_selftest_20260223T025859Z.json`
- run log: `TestEvidence/locusq_production_p0_selftest_20260223T025859Z.run.log`
- status: `status=pass`, `ok=true`

2. BL-016 required smoke suite rerun (Spatial adapter)

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
```

Result: `PASS_WITH_WARNING` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)
- artifact: `TestEvidence/locusq_smoke_suite_spatial_bl016_20260223T025916Z.log`
- warn source: `locusq_emitter_passthrough` soft-warn invariant `rms_level` (`rms_energy=-29.667 dB`) from `qa/scenarios/locusq_emitter_passthrough.json`
- note: warn-only baseline retained; hard-fail invariants passed

3. BL-016 companion phase 2.6 acceptance suite rerun (Spatial adapter)

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
```

Result: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)
- artifact: `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_bl016_20260223T030005Z.log`

4. Docs freshness gate after BL-016 closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_bl016_20260223T030010Z.log`

## BL-024 REAPER Automation Lane Hardening (UTC 2026-02-23)

1. Initial wrapper run after strictness/retry patchset

```sh
LQ_BL024_SKIP_INSTALL=1 LQ_BL024_HEADLESS_RUNS=1 ./scripts/qa-bl024-reaper-automation-lane-mac.sh
```

Result: `FAIL`
- artifact: `TestEvidence/bl024_reaper_automation_20260223T024628Z/`
- failure signature: `renderExitCode=0` with `renderOutputDetected=false`; prompted render-target/auto-quit follow-up hardening.

2. Hardened headless smoke rerun (timeouts + isolated REAPER instance + auto-bootstrap render target pinning)

```sh
LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC=30 LQ_REAPER_RENDER_TIMEOUT_SEC=45 ./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap
```

Result: `PASS`
- artifact: `TestEvidence/reaper_headless_render_20260223T025610Z/status.json`
- key checks: `requireLocusQ=true`, `locusqFxFound=true`, `renderOutputDetected=true`, `renderAttempts=1`.

3. Wrapper lane rerun with hardened headless path

```sh
LQ_BL024_SKIP_INSTALL=1 LQ_BL024_HEADLESS_RUNS=1 LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC=30 LQ_REAPER_RENDER_TIMEOUT_SEC=45 ./scripts/qa-bl024-reaper-automation-lane-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl024_reaper_automation_20260223T025619Z/status.tsv`
- report: `TestEvidence/bl024_reaper_automation_20260223T025619Z/report.md`
- note: this rerun validated the hardened path before full acceptance pack.

4. Docs freshness gate after BL-024 hardening sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

5. BL-024 acceptance pack (`3/3` strict runs with install enabled)

```sh
LQ_BL024_HEADLESS_RUNS=3 LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC=45 LQ_REAPER_RENDER_TIMEOUT_SEC=90 ./scripts/qa-bl024-reaper-automation-lane-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl024_reaper_automation_20260223T030210Z/status.tsv`
- report: `TestEvidence/bl024_reaper_automation_20260223T030210Z/report.md`
- install lane: `build_install=pass`
- strict host checks: `headless_run_1..3=pass` with `locusqFxFound=true` and `renderOutputDetected=true`.

6. Manual runbook evidence row update

```sh
TestEvidence/phase-2-7a-manual-host-ui-acceptance.md
```

Result: `PASS`
- new section: `BL-024 Manual Runbook Evidence Row (2026-02-23)`.
- includes host-session row plus deterministic automation references for bootstrap/routing and headphone mode stability checks.

7. Docs freshness gate after BL-024 In Validation sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## HX-03 REAPER Multi-Instance Stability Lane (UTC 2026-02-23)

1. Run deterministic clean/warm cache multi-instance host lane

```sh
LQ_HX03_INSTANCE_COUNT=3 LQ_HX03_START_STAGGER_SEC=1 LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC=45 LQ_REAPER_RENDER_TIMEOUT_SEC=90 ./scripts/qa-hx03-reaper-multi-instance-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/hx03_reaper_multi_instance_20260223T031450Z/status.tsv`
- report: `TestEvidence/hx03_reaper_multi_instance_20260223T031450Z/report.md`
- clean phase: `instance_1..3=pass`, `new_reports=0`, `reaper_processes_clean`
- warm phase: `instance_1..3=pass`, `new_reports=0`, `reaper_processes_clean`
- strict checks: all per-instance artifacts reported `locusqFxFound=true` and `renderOutputDetected=true`

2. BL-024 closure gate update after HX-03 pass

```sh
Documentation/backlog-post-v1-agentic-sprints.md
status.json
```

Result: `PASS`
- BL-024 moved to `Done (2026-02-23)` with HX-03 evidence linked.
- HX-03 marked `Done (2026-02-23)` and retained as recurring regression lane.

3. Docs freshness gate after HX-03/BL-024 closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## BL-012 Harness Backport Tranche 1 (UTC 2026-02-23)

1. Initial tranche-1 lane run (pre-fix)

```sh
./scripts/qa-bl012-harness-backport-tranche1-mac.sh
```

Result: `FAIL`
- artifact: `TestEvidence/bl012_harness_backport_20260223T032829Z/status.tsv`
- failure signature: runtime probe expected `locusq_state_roundtrip_contract/result.json`; actual result is nested (`pass_2/result.json`) for multi-pass state-roundtrip scenario output layout.

2. Runtime probe hardening for nested scenario result paths

```sh
scripts/qa-bl012-harness-backport-tranche1-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl012_harness_backport_20260223T032945Z/status.tsv`
- report: `TestEvidence/bl012_harness_backport_20260223T032945Z/report.md`
- assertions:
  - harness sanity configure/build/ctest `PASS`
  - contract coverage `PASS` (`latency=1`, `smoothing=1`, `state=1`, `contract_scenarios=3`)
  - runtime-config precedence probe `PASS` (`sample_rate=48000`, `block_size=512`, `channels=2` across all contract scenarios despite conflicting CLI flags)
  - perf probe `PASS` (`perf_metric_count=4` without forcing `--profile`)
  - canonical `qa_output/suite_result.json` sync `PASS`

3. BL-012 state promotion sync

```sh
Documentation/backlog-post-v1-agentic-sprints.md
status.json
```

Result: `PASS`
- BL-012 moved to `In Validation` with tranche-1 lane artifacts linked.

## BL-010 FDN Expansion Promotion Closeout (UTC 2026-02-23)

1. Verify existing BL-010 validation bundle remains fully green

```sh
awk -F'\t' 'NR==1 {next} {if ($2 != 0) {bad=1}} END {exit bad}' TestEvidence/bl010_validation_20260222T191102Z/status.tsv
```

Result: `PASS`
- all recorded BL-010 steps remain `exit_code=0` (`qa_210c` matrix, phase `2.10b` suite, seeded `pluginval`, RT-safety audit, deterministic hash compare, docs gate).
- canonical artifact set remains: `TestEvidence/bl010_validation_20260222T191102Z/`.

2. Docs freshness gate after BL-010 promotion/state sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

3. Final docs freshness gate after `status.json` BL-010 state/timestamp sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## BL-022 Choreography Packs Multipass Validation (UTC 2026-02-22)

1. Syntax/build guard before choreography closeout

```sh
node --check Source/ui/public/js/index.js
cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8
```

Result: `PASS`

2. Production self-test lane rerun (includes `UI-P1-022`)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/locusq_production_p0_selftest_20260222T213205Z.json`
- `UI-P1-022` detail: `orbit choreography pack apply/save/load verified`.

3. Spatial smoke regression check after choreography patch

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
```

Result: `PASS_WITH_WARNING` (`3 PASS / 1 WARN / 0 FAIL`; existing warn-only baseline retained).

4. Documentation freshness gate after backlog/evidence sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

5. Documentation freshness gate after final status/backlog sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

6. Final documentation freshness confirmation

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

BL-022 evidence bundle:
- `TestEvidence/bl022_validation_20260222T213204Z/status.tsv`
- `TestEvidence/bl022_validation_20260222T213204Z/selftest_production_p0.log`
- `TestEvidence/bl022_validation_20260222T213204Z/qa_smoke_spatial.log`
- `TestEvidence/bl022_validation_20260222T213204Z/docs_freshness.log`
- `TestEvidence/bl022_validation_20260222T213204Z/docs_freshness_postsync.log`
- `TestEvidence/bl022_validation_20260222T213204Z/docs_freshness_final.log`

## BL-019 Physics Interaction Lens Validation (UTC 2026-02-22)

1. Compile and syntax guard for production UI + QA host

```sh
node --check Source/ui/public/js/index.js
cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8
```

Result: `PASS`

2. Production self-test with BL-019 assertions

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
jq '.result.checks[] | select(.id=="UI-P1-019")' TestEvidence/locusq_production_p0_selftest_20260222T211349Z.json
```

Result: `PASS`
- `status=pass`, `ok=true`.
- `UI-P1-019`: `pass=true`, details `physics lens overlays verified (force/collision/trajectory)`.
- artifact: `TestEvidence/locusq_production_p0_selftest_20260222T211349Z.json`.

3. Docs freshness gate after BL-019 state/evidence sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## BL-017 Planning Kickoff (UTC 2026-02-22)

1. Companion bridge planning artifact authored

- `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md`
- scope includes:
  - PHASE exclusion inside plugin process callback
  - companion IPC bridge contract (`seq`, timestamps, pose fields, stale handling)
  - RT-safe plugin integration slices (A/B/C)
  - deterministic QA lane and manual AirPods validation path

2. Backlog/status sync

- `Documentation/backlog-post-v1-agentic-sprints.md`:
  - BL-017 moved to `In Planning (2026-02-22)`
- `status.json` notes updated for BL-017 kickoff state

3. Docs freshness gate

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## P1 BL-009 Audio-Path Contract Addendum (UTC 2026-02-22)

1. BL-009 QA adapter + Steam include-path build verification

```sh
cmake --build build_local --config Release --target locusq_qa -j 8
```

Result: `PASS`.
- `locusq_qa` now compiles in Steam-enabled lanes with SDK include/runtime macros propagated from `CMakeLists.txt`.

2. BL-009 deterministic headphone-path contract lane

```sh
./scripts/qa-bl009-headphone-contract-mac.sh
```

Result: `PASS`.
- Downmix reference determinism: stable hash across reruns.
- Steam-request determinism: stable hash across reruns.
- `UI-P1-009` scene-state check: `request=steam_binaural active=steam_binaural steamAvailable=true steamCompiled=true stage=ready err=0`.
- Cross-mode contract: hashes diverge when `steamAvailable=true`.

3. Docs freshness gate after BL-009 automation sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

### Artifacts (BL-009 audio-path contract)

- `TestEvidence/bl009_headphone_contract_20260222T193857Z/status.tsv`
- `TestEvidence/bl009_headphone_contract_20260222T193857Z/report.md`
- `TestEvidence/bl009_headphone_contract_20260222T193857Z/ui_selftest_bl009.json`

## P2 BL-010 Modulated FDN Validation Bundle (UTC 2026-02-22)

1. Isolated BL-010 configure/build lane (`build_bl010`, Steam disabled to avoid cross-lane Steam include coupling in `build_local` QA target)

```sh
cmake -S . -B build_bl010 -DCMAKE_BUILD_TYPE=Release -DBUILD_LOCUSQ_QA=ON -DLOCUSQ_ENABLE_STEAM_AUDIO=OFF -DLOCUSQ_ENABLE_CLAP=OFF -DQA_HARNESS_DIR=/Users/artbox/Documents/Repos/audio-dsp-qa-harness -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build_bl010 --config Release --target locusq_qa LocusQ_VST3 -j 8
```

Result: `PASS`.
- Built artifacts:
  - `build_bl010/locusq_qa_artefacts/Release/locusq_qa`
  - `build_bl010/LocusQ_artefacts/Release/VST3/LocusQ.vst3`

2. New BL-010 deterministic scenario matrix (`locusq_210c_fdn_modulated_deterministic`)

```sh
./build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_210c_fdn_modulated_deterministic.json --sample-rate 48000 --block-size 512
./build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_210c_fdn_modulated_deterministic.json --sample-rate 96000 --block-size 512 --channels 4
```

Result: `PASS`.
- 48k/512: `rt60=4.24773`, `perf_avg_block_time_ms=0.062053`, `perf_meets_deadline=true`.
- 96k/512/4ch: `rt60=4.26626`, `perf_avg_block_time_ms=0.0636573`, `perf_meets_deadline=true`.

3. Existing renderer CPU trend suite regression matrix

```sh
./build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_10b_renderer_cpu_trend_suite.json --sample-rate 48000 --block-size 512
./build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_10b_renderer_cpu_trend_suite.json --sample-rate 96000 --block-size 512 --channels 4
```

Result: `PASS`.
- Both runs: `3 PASS / 0 WARN / 0 FAIL`.

4. Determinism replay check (same scenario, repeated render hash)

```sh
./build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_210c_fdn_modulated_deterministic.json --sample-rate 48000 --block-size 512
./build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_210c_fdn_modulated_deterministic.json --sample-rate 48000 --block-size 512
shasum -a 256 <run_a_wet.wav> <run_b_wet.wav>
```

Result: `PASS`.
- `match=true` (`run_a_sha256 == run_b_sha256`).

5. Seeded host regression check

```sh
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --random-seed 0x2a331c6 build_bl010/LocusQ_artefacts/Release/VST3/LocusQ.vst3
```

Result: `PASS` (`SUCCESS`).

6. RT-safety audit grep

```sh
rg -n "new |std::vector|\\.push_back|\\.resize|std::string\\(|juce::Logger|std::cout" Source/PluginProcessor.cpp Source/SpatialRenderer.h Source/FDNReverb.h
```

Result: `PASS` (audit completed; matches are confined to setup/state paths, not `FDNReverb::process`).

7. Docs freshness gate after BL-010 sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

### Artifacts (BL-010)

- Validation folder: `TestEvidence/bl010_validation_20260222T191102Z`
- Command rollup: `TestEvidence/bl010_validation_20260222T191102Z/status.tsv`
- Determinism hash report: `TestEvidence/bl010_validation_20260222T191102Z/qa_210c_determinism_hashes.txt`

## P2 BL-011 CLAP External Validation (UTC 2026-02-22)

1. Verify CLAP tooling availability

```sh
command -v clap-info
command -v clap-validator
```

Result: `PASS`.
- `clap-info` available at `/Users/artbox/.local/bin/clap-info` (`0.9.0`).
- `clap-validator` available at `/Users/artbox/.local/bin/clap-validator` (`0.3.2`).

2. Descriptor/introspection snapshot

```sh
clap-info build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
```

Result: `PASS`.
- `plugin_count=1`
- `id=com.noizefield.locusq`
- `name=LocusQ`
- `version=1.0.0`

3. External validator suite

```sh
clap-validator validate build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
clap-validator validate --json build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
```

Result: `FAIL`.
- `21 tests run, 15 passed, 1 failed, 5 skipped, 0 warnings`.
- Failing check: `param-set-wrong-namespace`
- Failure detail: parameter values changed after `CLAP_EVENT_PARAM_VALUE` events with mismatching namespace ID.

4. Remediation and revalidation

```sh
cmake --build build_local --config Release --target LocusQ_CLAP -j 8
clap-validator validate build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
clap-validator validate --json build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
```

Result: `PASS`.
- Code fix in `Source/PluginProcessor.cpp`: CLAP instance now skips activation-time `emit_color` host parameter seeding.
- Revalidation summary: `21 tests run, 16 passed, 0 failed, 5 skipped, 0 warnings`.
- Previously failing check `param-set-wrong-namespace` now `PASSED`.

### Artifacts (BL-011 external validation)

- Report: `TestEvidence/clap-validation-report-2026-02-22.md`
- Logs: `TestEvidence/clap_validation_20260222T181504Z/clap-info.json`, `TestEvidence/clap_validation_20260222T181504Z/clap-validator.txt`, `TestEvidence/clap_validation_20260222T181504Z/clap-validator.json`, `TestEvidence/clap_validation_20260222T181504Z/clap-validator-quiet.json`, `TestEvidence/clap_validation_20260222T181504Z/clap-validator-quiet.cleaned.json`
- Binary evidence: `TestEvidence/clap_validation_20260222T181504Z/artifact_stat.txt`, `TestEvidence/clap_validation_20260222T181504Z/artifact_sha256.txt`
- Revalidation logs: `TestEvidence/clap_validation_20260222T182619Z/clap-info.json`, `TestEvidence/clap_validation_20260222T182619Z/clap-validator.txt`, `TestEvidence/clap_validation_20260222T182619Z/clap-validator.json`, `TestEvidence/clap_validation_20260222T182619Z/clap-validator-quiet.json`, `TestEvidence/clap_validation_20260222T182619Z/clap-validator-quiet.cleaned.json`
- Revalidation binary evidence: `TestEvidence/clap_validation_20260222T182619Z/artifact_stat.txt`, `TestEvidence/clap_validation_20260222T182619Z/artifact_sha256.txt`
- Added metadata header to `Documentation/plans/CLAP_References.md`.
- Updated `scripts/validate-docs-freshness.sh` to prune `third_party/` vendor docs from repo-owned metadata checks.

### Artifacts (BL-009 validation)

- BL-009 opt-in self-test pass JSON: `TestEvidence/locusq_production_p0_selftest_20260221T102014Z.json`
- Baseline production self-test pass JSON: `TestEvidence/locusq_production_p0_selftest_20260221T102031Z.json`
- Shared gate status TSV: `TestEvidence/ui_pr_gate_20260221T102057Z/status.tsv`

## P1 BL-009 Diagnostics Closeout Addendum (UTC 2026-02-21)

1. Rebuild after Steam diagnostic telemetry patch set

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`.

2. BL-009 opt-in self-test with diagnostics

```sh
LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS` (`status=pass`, `ok=true`).
- `UI-P1-009` detail confirms active Steam path on this host:
  - `request=steam_binaural`
  - `active=steam_binaural`
  - `steamAvailable=true`
  - `steamCompiled=true`
  - `stage=ready`
  - `err=0`
  - `lib=/Users/artbox/Documents/Repos/LocusQ/third_party/steam-audio/sdk/steamaudio/lib/osx/libphonon.dylib`

### Artifacts (BL-009 diagnostics)

- Diagnostics self-test pass JSON: `TestEvidence/locusq_production_p0_selftest_20260221T104708Z.json`
- Intermediate failure history (for traceability): `TestEvidence/locusq_production_p0_selftest_20260221T104429Z.json`, `TestEvidence/locusq_production_p0_selftest_20260221T104557Z.json`

## P2 BL-011 Slice 1 CLAP Build Scaffolding (UTC 2026-02-22)

1. CLAP-enabled configure with pinned `clap-juce-extensions` commit

```sh
cmake -S . -B build_local -DCMAKE_BUILD_TYPE=Release -DLOCUSQ_ENABLE_CLAP=ON -DLOCUSQ_CLAP_FETCH=ON -DLOCUSQ_CLAP_JUCE_EXTENSIONS_TAG=02f91b7988298f7f1f05c706da16e1d9da852a87 -DBUILD_LOCUSQ_QA=ON
```

Result: `PASS`.
- CLAP helper loaded successfully.
- Generated target: `LocusQ_CLAP`.

2. CLAP target build

```sh
cmake --build build_local --config Release --target LocusQ_CLAP -j 8
```

Result: `PASS`.

3. CLAP artifact verification

```sh
ls -la build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap/Contents/MacOS
file build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap/Contents/MacOS/LocusQ
```

Result: `PASS`.
- Output bundle: `build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap`
- Binary detected: `Mach-O 64-bit bundle arm64`

4. CLAP-aware installer script syntax validation

```sh
bash -n scripts/build-and-install-mac.sh
```

Result: `PASS`.
- Added opt-in CLAP controls: `LOCUSQ_ENABLE_CLAP`, `LOCUSQ_INSTALL_CLAP`, `LOCUSQ_CLAP_FETCH`, `LOCUSQ_CLAP_JUCE_EXTENSIONS_DIR`.
- Full installer execution was intentionally not run in this slice to avoid unsolicited host/plugin install side effects.

## P2 BL-011 Slice 2 CLAP Lifecycle Telemetry + Self-Test Lane (UTC 2026-02-22)

1. JS + script syntax guards for BL-011 self-test lane

```sh
node --check Source/ui/public/js/index.js
bash -n scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS`.

2. CLAP-enabled configure/build after telemetry integration

```sh
cmake -S . -B build_local -DCMAKE_BUILD_TYPE=Release -DLOCUSQ_ENABLE_CLAP=ON -DLOCUSQ_CLAP_FETCH=ON -DLOCUSQ_CLAP_JUCE_EXTENSIONS_TAG=02f91b7988298f7f1f05c706da16e1d9da852a87 -DBUILD_LOCUSQ_QA=ON
cmake --build build_local --config Release --target LocusQ_CLAP -j 8
ls -la build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap/Contents/MacOS
file build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap/Contents/MacOS/LocusQ
```

Result: `PASS`.
- `LocusQ_CLAP` rebuilt successfully with new scene-state telemetry fields.
- Artifact confirmed at `build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap`.

3. Production self-test with BL-011 lane enabled

```sh
LOCUSQ_UI_SELFTEST_BL011=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh
jq '.result.checks[] | select(.id=="UI-P2-011")' TestEvidence/locusq_production_p0_selftest_20260222T172815Z.json
```

Result: `PASS`.
- `UI-P2-011` detail: `build=true properties=true clap=false stage=non_clap_instance active=false processing=false transport=false mode=disabled wrapper=Standalone version=0.0.0`.
- Artifact: `TestEvidence/locusq_production_p0_selftest_20260222T172815Z.json`.

4. CLAP-disabled regression build path verification

```sh
cmake -S . -B build_no_clap_check -DCMAKE_BUILD_TYPE=Release -DLOCUSQ_ENABLE_CLAP=OFF -DBUILD_LOCUSQ_QA=OFF
cmake --build build_no_clap_check --config Release --target LocusQ_VST3 -j 8
```

Result: `PASS`.
- Confirms no-CLAP compile path remains healthy after `clap_properties` integration.

5. BL-011 tool availability note

```sh
command -v clap-info
command -v clap-validator
```

Result: `NOT AVAILABLE` on this machine (`clap-info`: missing, `clap-validator`: missing).

6. Docs freshness gate after BL-011 Slice 2 sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## Spatial Audio Skill + BL-018 Addendum (UTC 2026-02-22)

1. Validate new `spatial-audio-engineering` skill package

```sh
/tmp/locusq-skillcreator-venv/bin/python /Users/artbox/.codex/skills/.system/skill-creator/scripts/quick_validate.py .codex/skills/spatial-audio-engineering
```

Result: `PASS` (`Skill is valid!`)

2. Run BL-018 ambisonic/layout contract lane (lightweight mode)

```sh
bash scripts/qa-bl018-ambisonic-contract-mac.sh --no-binaural-runtime
```

Result: `PASS_WITH_WARNING`
- warnings are expected for this mode:
  - `integration_probe=warn` (`no_ambisonic_backend_markers_in_source_pending_backend_impl`)
  - `binaural_runtime_contract=warn` (`disabled_by_flag`)
- artifact: `TestEvidence/bl018_ambisonic_contract_20260222T195416Z/`

3. Run skill wrapper lane smoke

```sh
RUN_BINAURAL_RUNTIME=0 ./.codex/skills/spatial-audio-engineering/scripts/run_spatial_lanes.sh --skip-bl009
```

Result: `PASS_WITH_WARNING`
- BL-018 lane executed end-to-end through skill wrapper.
- artifact: `TestEvidence/bl018_ambisonic_contract_20260222T195448Z/`

4. Docs freshness gate after skill/routing/backlog updates

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`)

## Spatial Lane Closeout Addendum (UTC 2026-02-22)

1. Integrated spatial lane run (`BL-009` + `BL-018`) before strict remediation

```sh
./.codex/skills/spatial-audio-engineering/scripts/run_spatial_lanes.sh
```

Result: `PASS_WITH_WARNING`
- `BL-009`: `PASS`.
- `BL-018`: `PASS_WITH_WARNING` due missing ambisonic marker gate prior to remediation.
- artifacts: `TestEvidence/bl009_headphone_contract_20260222T195826Z/`, `TestEvidence/bl018_ambisonic_contract_20260222T195837Z/`.

2. Strict BL-018 integration gate (pre-remediation)

```sh
bash scripts/qa-bl018-ambisonic-contract-mac.sh --strict-integration
```

Result: `FAIL`
- failing step: `integration_probe` (`no_ambisonic_backend_markers_in_source_strict_mode`).
- artifact: `TestEvidence/bl018_ambisonic_contract_20260222T195852Z/`.

3. Ambisonic telemetry marker patch + syntax/marker gate

```sh
node --check Source/ui/public/js/index.js
rg -n -i "(ambisonic|\\bhoa\\b|b-format|\\bfoa\\b|rend_ambi|rendererAmbi|ambi_)" Source/PluginProcessor.cpp Source/SpatialRenderer.h Source/ui/public/js/index.js
```

Result: `PASS`
- Added explicit `rendererAmbi*` scene-state telemetry placeholders to support deterministic BL-018 strict gating.

4. Integrated spatial lane run (`BL-009` + `BL-018`) after remediation

```sh
./.codex/skills/spatial-audio-engineering/scripts/run_spatial_lanes.sh
```

Result: `PASS`
- `BL-009`: `PASS`.
- `BL-018`: `PASS`.
- artifacts: `TestEvidence/bl009_headphone_contract_20260222T200016Z/`, `TestEvidence/bl018_ambisonic_contract_20260222T200026Z/`.

5. Strict BL-018 integration gate rerun

```sh
bash scripts/qa-bl018-ambisonic-contract-mac.sh --strict-integration
```

Result: `PASS`
- key steps: `integration_probe=pass`, `binaural_runtime_contract=pass`.
- artifact: `TestEvidence/bl018_ambisonic_contract_20260222T200040Z/`.

6. Docs freshness gate after BL-018 strict remediations

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

7. Compile sanity for `rendererAmbi*` scene-state telemetry patch

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS` (`LocusQ_Standalone` built successfully; warnings unchanged).

## BL-018 Spatial Profile Matrix Remediation (UTC 2026-02-22)

1. QA spatial adapter mapping fix for profile lane

- root cause: scenario engine rejected `parameter_variations.rend_spatial_profile` because `qa/locusq_adapter` did not expose the parameter name/index.
- fix: added `rend_spatial_profile` mapping to `LocusQSpatialAdapter` (`kNumParameters` 35 -> 36).

2. Rebuild QA binary

```sh
cmake --build build_local --config Release --target locusq_qa -j 8
```

Result: `PASS`

3. Failing scenario repro rerun

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_bl018_profile_virtual3d_stereo.json --sample-rate 48000 --block-size 512 --channels 2
```

Result: `PASS`

4. Full BL-018 ambisonic/layout contract lane rerun (with profile matrix enabled)

```sh
./scripts/qa-bl018-ambisonic-contract-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl018_ambisonic_contract_20260222T202029Z/`
- profile matrix rows now all `pass` in `status.tsv` (`virtual3d_stereo`, `ambisonic_foa`, `ambisonic_hoa`, `surround_5_2_1`, `surround_7_2_1`, `surround_7_4_2`, `codec_iamf`, `codec_adm`).

5. Integrated spatial wrapper rerun (`BL-009` + `BL-018`)

```sh
./.codex/skills/spatial-audio-engineering/scripts/run_spatial_lanes.sh
```

Result: `PASS`
- artifacts: `TestEvidence/bl009_headphone_contract_20260222T202233Z/`, `TestEvidence/bl018_ambisonic_contract_20260222T202244Z/`.

## BL-009 Headphone Profile Lane Addendum (UTC 2026-02-22)

1. Renderer/profile wiring compile guard

```sh
node --check Source/ui/public/js/index.js
bash -n scripts/qa-bl009-headphone-profile-contract-mac.sh
cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8
```

Result: `PASS`

2. Baseline BL-009 regression lane rerun

```sh
./scripts/qa-bl009-headphone-contract-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl009_headphone_contract_20260222T204437Z/`

3. New BL-009 headphone profile contract lane

```sh
./scripts/qa-bl009-headphone-profile-contract-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl009_headphone_profile_contract_20260222T204451Z/`
- checks:
  - deterministic baseline (`generic` repeat hash stable)
  - profile divergence (`generic` vs `airpods_pro_2`; `generic` vs `sony_wh1000xm5`)
  - UI BL-009 diagnostics include profile request/active detail string

4. Integrated spatial lanes rerun

```sh
./.codex/skills/spatial-audio-engineering/scripts/run_spatial_lanes.sh
```

Result: `PASS`
- artifacts: `TestEvidence/bl009_headphone_contract_20260222T204505Z/`, `TestEvidence/bl018_ambisonic_contract_20260222T204516Z/`.

## Spatial Profile Usage Guide Docs Sync (UTC 2026-02-22)

1. Added canonical operator guide for spatial audio profile usage

- new doc: `Documentation/spatial-audio-profiles-usage.md`
- index update: `Documentation/README.md`
- scope: profile matrix (`rend_spatial_profile`), headphone modes/profiles, mono/stereo/multichannel behavior, and validation command references.

2. Docs freshness gate

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## BL-009 Steam Headphone Contract Closeout (UTC 2026-02-23)

1. Rebuild standalone after BL-009 self-test timeout and diagnostics fallback hardening

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`
- Updated BL-009/BL-011 opt-in self-test timeout budget in `Source/PluginEditor.cpp` (30s poll horizon for extended lanes).
- Added Steam diagnostics fallback capture/restore in `Source/ui/public/js/index.js` so BL-009 checks remain deterministic after synthetic viewport injections.

2. BL-009 opt-in production self-test verification

```sh
LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/locusq_production_p0_selftest_20260223T020559Z.json`
- key check: `UI-P1-009` passed with deterministic detail string including `request=steam_binaural`, `active=steam_binaural`, `steamAvailable=true`, `stage=ready`.

3. BL-009 deterministic headphone contract lane rerun

```sh
./scripts/qa-bl009-headphone-contract-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl009_headphone_contract_20260223T020702Z/`
- deterministic hashes:
  - downmix: `0e6c96dd6e0ee2c6ae654b0d768bbb2adc56420103b22dd9513f89f26d2f4ccb`
  - steam request: `0f6f56d418470c92563a598549fa64dc49520e721de190364924a7753c9e9240`
- cross-mode assertion: `cross_mode_divergence=pass` (`steamAvailable=true hashes_diverge`).

4. BL-009 deterministic headphone profile contract lane rerun

```sh
./scripts/qa-bl009-headphone-profile-contract-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl009_headphone_profile_contract_20260223T020906Z/`
- profile determinism/divergence assertions all passed (`generic`, `airpods`, `sony`).
- UI profile diagnostics assertion passed (`ui_selftest_profile_diag=pass`).

5. Required production baseline self-test rerun

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/locusq_production_p0_selftest_20260223T020923Z.json`

6. Docs freshness gate after BL-009 closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## BL-011 CLAP Lifecycle and Host/CI Closeout (UTC 2026-02-23)

1. Deterministic BL-011 closeout bundle

```sh
./scripts/qa-bl011-clap-closeout-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl011_clap_closeout_20260223T032730Z/`
- key lane assertions from `status.tsv`:
  - `clap_info=pass`
  - `clap_validator=pass`
  - `qa_smoke_suite=pass`
  - `qa_phase_2_6_acceptance_suite=pass`
  - `ui_selftest_bl011=pass` (`UI-P2-011`; artifact reused deterministically from `TestEvidence/locusq_production_p0_selftest_20260223T032004Z.json`)
  - `cmake_build_nonclap_vst3_guard=pass`
  - `reaper_clap_discovery=pass` (`TestEvidence/reaper_clap_discovery_probe_20260223T023314Z.json`; `matchedFxName=CLAP: LocusQ (Noizefield)`)
  - `docs_freshness=pass`

2. CLAP documentation consolidation and ADR closure

- canonical closeout doc: `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`
- ADR: `Documentation/adr/ADR-0009-clap-closeout-documentation-consolidation.md`
- archived CLAP references/PDFs: `Documentation/archive/2026-02-23-clap-reference-bundle/`
- backlog/status surfaces moved BL-011 from `In Validation`/`In Progress` to `Done (2026-02-23)`.

3. Final docs freshness gate after BL-011 closeout + CLAP docs consolidation sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## BL-015 All-Emitter Realtime Rendering Closeout (UTC 2026-02-23)

1. Rebuild standalone and QA binaries before closeout rerun

```sh
cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
```

Result: `PASS`

2. Production self-test rerun (BL-015 contract)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS`
- artifact: `TestEvidence/locusq_production_p0_selftest_20260223T034704Z.json`
- BL-015 gate: `UI-P1-015` passed (`selected vs non-selected emitter styling verified`).
- companion downstream checks in same run remained green (`UI-P1-014`, `UI-P1-019`, `UI-P1-022`).

3. Spatial smoke-suite companion rerun

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
```

Result: `PASS_WITH_WARNING` (baseline retained)
- artifact: `TestEvidence/locusq_smoke_suite_spatial_bl015_20260223T034751Z.log`
- summary: `3 PASS / 1 WARN / 0 FAIL / 0 ERROR`.

4. BL-015 closeout synchronization

- backlog/status promoted BL-015 from `In Validation` to `Done (2026-02-23)` with dependency graph preserved for BL-014/BL-019/BL-021.

## HX-01 shared_ptr Atomic Migration Guard Closeout (UTC 2026-02-23)

1. Rebuild with HX-01 migration patch (`SharedPtrAtomicContract`)

```sh
cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
```

Result: `PASS`
- artifact: `TestEvidence/hx01_sharedptr_atomic_build_20260223T034848Z.log`

2. QA adapter smoke verification (spatial adapter)

```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_renderer_spatial_output.json
```

Result: `PASS`
- artifact: `TestEvidence/hx01_sharedptr_atomic_qa_smoke_20260223T034918Z.log`

3. Deprecated call-site scan (excluding contract wrapper implementation fallback)

```sh
rg -n "std::atomic_(store|load)(_explicit)?\\s*\\(" Source --glob '!Source/SharedPtrAtomicContract.h'
```

Result: `PASS`
- artifact: `TestEvidence/hx01_sharedptr_atomic_deprecation_scan_excluding_wrapper_20260223T034931Z.log`
- note: no direct deprecated call sites remain outside the contract wrapper.

4. Docs freshness gate after HX-01 sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_hx01_20260223T035128Z.log`

5. Final docs freshness gate after BL-015 closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_bl015_20260223T035149Z.log`

6. Final docs freshness gate after status timestamp sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

## HX-04 Scenario Coverage Drift Guard Closeout (UTC 2026-02-23)

1. Run standalone HX-04 audit lane

```sh
./scripts/qa-hx04-scenario-audit.sh --qa-bin build_bl010/locusq_qa_artefacts/Release/locusq_qa
```

Result: `PASS_WITH_WARNINGS`
- artifact: `TestEvidence/hx04_scenario_audit_20260223T172312Z/status.tsv`
- matrix: `TestEvidence/hx04_scenario_audit_20260223T172312Z/coverage_matrix.tsv`
- summary: required AirAbsorption/Calibration/directivity scenarios are present, mapped, and executed in the parity suite (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`).

2. Run targeted directivity sample lane

```sh
build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_directivity_aim.json
```

Result: `PASS`
- artifact: `TestEvidence/hx04_sample_directivity_aim_20260223T172316Z.log`

3. Rerun BL-012 tranche-1 lane with HX-04 embedded enforcement

```sh
LQ_BL012_BUILD_DIR=build_bl010 LQ_BL012_RUN_HARNESS_SANITY=0 LQ_BL012_QA_BIN=build_bl010/locusq_qa_artefacts/Release/locusq_qa ./scripts/qa-bl012-harness-backport-tranche1-mac.sh
```

Result: `PASS_WITH_WARNINGS`
- artifact: `TestEvidence/bl012_harness_backport_20260223T172301Z/status.tsv`
- note: `hx04_audit_run=pass` is now a native BL-012 lane assertion.

4. Docs freshness gate after HX-04 closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_hx04_20260223T035938Z.log`

5. Final docs freshness gate after HX-04 backlog/root/status synchronization

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_hx04_20260223T171851Z.log`

6. Final docs freshness gate after README + Documentation index sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_hx04_20260223T172010Z.log`

## BL-019 Physics Interaction Lens Closeout (UTC 2026-02-23)

1. Production self-test lane attempt 1 (`UI-P1-019`)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh build_bl019/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `FAIL` (`ui_selftest_timeout_before_pass_or_fail`)
- artifact: `TestEvidence/locusq_production_p0_selftest_20260223T171504Z.json`
- note: transient boot error (`NotFoundError`).

2. Production self-test lane rerun (`UI-P1-019`)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh build_bl019/LocusQ_artefacts/Release/Standalone/LocusQ.app
```

Result: `PASS`
- artifact: `TestEvidence/locusq_production_p0_selftest_20260223T171542Z.json`
- BL-019 gate: `UI-P1-019` passed (`physics lens overlays verified (force/collision/trajectory)`).

3. Spatial smoke-suite companion rerun

```sh
build_bl010/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
```

Result: `PASS_WITH_WARNING` (baseline retained)
- artifact: `TestEvidence/locusq_smoke_suite_spatial_bl019_20260223T121613.log`
- summary: `3 PASS / 1 WARN / 0 FAIL / 0 ERROR`.

4. BL-019 closeout docs freshness gate

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_bl019_20260223T121618.log`

5. BL-019 final docs freshness gate after backlog/status/root synchronization

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_bl019_20260223T122029_postsync.log`

## BL-013 HostRunner Feasibility Bundle (UTC 2026-02-23)

1. Configure HostRunner-enabled LocusQ QA build (local harness path pinned)

```sh
cmake -S . -B build_bl013_hostrunner -DBUILD_LOCUSQ_QA=ON -DQA_HARNESS_DIR="$HOME/Documents/Repos/audio-dsp-qa-harness" -DBUILD_HOST_RUNNER=ON -DLOCUSQ_ENABLE_STEAM_AUDIO=OFF -DLOCUSQ_ENABLE_CLAP=0 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Result: `PASS`

2. Build HostRunner-enabled QA binary and VST3 artifact

```sh
cmake --build build_bl013_hostrunner --config Release --target locusq_qa LocusQ_VST3 -j 8
```

Result: `PASS`

3. Run BL-013 feasibility lane (fast rerun mode)

```sh
LQ_BL013_SKIP_BUILD=1 ./scripts/qa-bl013-hostrunner-feasibility-mac.sh
```

Result: `PASS_WITH_WARNINGS`
- artifact: `TestEvidence/bl013_hostrunner_feasibility_20260223T172005Z/status.tsv`
- report: `TestEvidence/bl013_hostrunner_feasibility_20260223T172005Z/report.md`
- key signal: backend VST3 probe warns (`exit=139`, deterministic segfault), while skeleton fallback probe passes.

4. BL-013 plan package published

- `Documentation/plans/bl-013-hostrunner-feasibility-2026-02-23.md`

5. BL-013 docs freshness gate after backlog/status/evidence sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).

6. BL-013 crash root-cause patch in HostRunner VST3 backend

- `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/runners/vst3_plugin_host.cpp`
- Fix: preserve/reinitialize `processData_` lifecycle across `loadPlugin()` -> `unloadPlugin()` -> `configure()` so backend prepare no longer dereferences null.

7. BL-013 harness host-test interface parity patch

- `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/tests/host_runner_unit_test.cpp`
- Fix: added `sendMidiEvents(...)` override in `MockPluginHost` to satisfy `PluginHostInterface` and restore host-runner unit-test build.

8. BL-013 clean feasibility rerun with harness host tests enabled

```sh
LQ_BL013_RUN_HARNESS_HOST_TESTS=1 ./scripts/qa-bl013-hostrunner-feasibility-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/bl013_hostrunner_feasibility_20260223T173642Z/status.tsv`
- report: `TestEvidence/bl013_hostrunner_feasibility_20260223T173642Z/report.md`
- key signal: `hostrunner_vst3_probe=pass` (`dry.wav` + `wet.wav`), `hostrunner_vst3_skeleton_probe=pass`, `harness_host_ctest=pass`.

9. BL-013 staged diagnostics emission in `locusq_qa`

- `qa/main.cpp`: added `HOSTRUNNER_STAGE` markers for `init`, `prepare`, `render`, and `release` phases.
- verification log: `TestEvidence/bl013_hostrunner_feasibility_20260223T173642Z/hostrunner_vst3_probe.log`

10. BL-013 promotion final docs freshness gate

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_bl013_20260223T173744Z.log`

## P0 BL-025 + BL-014 Closeout Refresh (UTC 2026-02-24)

1. Rebuild standalone before reruns

```sh
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Result: `PASS`

2. Production self-test rerun (BL-025 + BL-014 gates)

```sh
./scripts/standalone-ui-selftest-production-p0-mac.sh
```

Result: `PASS`
- artifact: `TestEvidence/locusq_production_p0_selftest_20260224T032239Z.json`
- key assertions: `UI-P1-025A`, `UI-P1-025B`, `UI-P1-025C`, `UI-P1-025D`, `UI-P1-025E`, `UI-P1-014` all `pass=true`.

3. REAPER host smoke rerun

```sh
./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap
```

Result: `PASS`
- artifact: `TestEvidence/reaper_headless_render_20260224T032300Z/status.json`

4. BL-014 strict companion suite refresh

```sh
build_bl013_hostrunner/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
build_bl013_hostrunner/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json
```

Result:
- `locusq_smoke_suite`: `PASS_WITH_WARNING` (`3 PASS / 1 WARN / 0 FAIL / 0 ERROR`)
- `locusq_phase_2_6_acceptance_suite`: `PASS` (`3 PASS / 0 WARN / 0 FAIL / 0 ERROR`)
- artifacts:
  - `TestEvidence/locusq_smoke_suite_spatial_bl014_20260224T032355Z.log`
  - `TestEvidence/locusq_phase_2_6_acceptance_suite_spatial_bl014_20260224T032355Z.log`

5. Documentation update: plain-language lane explainer published

- `Documentation/testing/production-selftest-and-reaper-headless-smoke-guide.md`
- linked from `Documentation/README.md` Tier 1 testing references.

6. Canonical backlog/status synchronization for P0 closure

- `Documentation/backlog-post-v1-agentic-sprints.md` updated: BL-025 and BL-014 moved to `Done (2026-02-24)`.
- `status.json` updated with refreshed P0 evidence pointers and done-state flags.

7. Final docs freshness gate after closeout sync

```sh
./scripts/validate-docs-freshness.sh
```

Result: `PASS` (`0 warning(s)`).
- artifact: `TestEvidence/validate_docs_freshness_p0_closeout_20260224T033500Z.log`
