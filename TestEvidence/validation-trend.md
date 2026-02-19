Title: LocusQ Validation Trend
Document Type: Validation Trend Log
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-19

# Validation Trend

## Purpose
Track a concise run history for regression visibility across implementation phases.

## Trend Table

| Timestamp (UTC) | Phase Context | Command | Result |
|---|---|---|---|
| 2026-02-18T17:55:45Z | Phase 2.4 acceptance closeout configure | `cmake -S plugins/LocusQ -B plugins/LocusQ/build -DBUILD_LOCUSQ_QA=ON -DJUCE_DIR=.../_tools/JUCE` | PASS |
| 2026-02-18T17:56:00Z | Phase 2.4 acceptance closeout build | `cmake --build plugins/LocusQ/build --target locusq_physics_probe locusq_qa -j 8` | PASS |
| 2026-02-18T17:56:23Z | Phase 2.4 deterministic physics probe | `./plugins/LocusQ/build/locusq_physics_probe_artefacts/locusq_physics_probe` | PASS (`5/5` checks) |
| 2026-02-18T17:56:43Z | Phase 2.4 physics spatial motion scenario | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_24_physics_spatial_motion.json` | PASS |
| 2026-02-18T18:02:01Z | Phase 2.4 zero-g drift scenario | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_24_physics_zero_g_drift.json` | PASS |
| 2026-02-18T18:02:01Z | Phase 2.4 acceptance suite rollup (phase-pure) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_4_acceptance_suite.json` | PASS (`2 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-18T22:41:30Z | Phase 2.1-2.3 validation | `locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | PASS (4/4) |
| 2026-02-18T23:07:35Z | Phase 2.5 start validation | `locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | PASS (4/4) |
| 2026-02-18T23:19:39Z | Documentation standardization | `doxygen Documentation/Doxyfile` | PASS (HTML generated in `build/docs_api/html`) |
| 2026-02-18T23:24:55Z | Phase 2.5 build verification | `cmake --build build --target LocusQ_VST3 LocusQ_Standalone -j 8` | PASS |
| 2026-02-18T23:26:28Z | Phase 2.5 host-load verification | `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`exit code 0`) |
| 2026-02-18T23:27:12Z | Phase 2.5 standalone smoke | `open build/LocusQ_artefacts/Standalone/LocusQ.app` | PASS (process observed/terminated) |
| 2026-02-18T23:43:12Z | Phase 2.5 acceptance suite | `locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json` | PASS (`9` scenarios: `8 PASS`, `1 WARN`, `0 FAIL`) |
| 2026-02-19T00:01:22Z | Phase 2.6 QA build | `cmake --build plugins/LocusQ/build --target locusq_qa -j 8` | PASS (warnings only) |
| 2026-02-19T00:01:28Z | Phase 2.6 smoke suite | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json` | PASS (`4` scenarios: `4 PASS`, `0 WARN`, `0 FAIL`) |
| 2026-02-19T00:01:48Z | Phase 2.6 animation smoke | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_animation_internal_smoke.json` | PASS |
| 2026-02-19T00:01:53Z | Phase 2.5 regression (from Phase 2.6 branch) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_5_acceptance_suite.json` | WARN (`9` scenarios: `8 PASS`, `1 WARN`, `0 FAIL`) |
| 2026-02-19T00:02:43Z | Phase 2.6 plugin build | `cmake --build plugins/LocusQ/build --target LocusQ_VST3 LocusQ_Standalone -j 8` | PASS |
| 2026-02-19T00:02:54Z | Phase 2.6 host-load verification | `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`exit code 0`) |
| 2026-02-19T00:02:59Z | Phase 2.6 standalone smoke | `open plugins/LocusQ/build/LocusQ_artefacts/Standalone/LocusQ.app` | PASS (process observed/terminated) |
| 2026-02-19T00:03:04Z | Phase 2.6 API docs snapshot | `doxygen plugins/LocusQ/Documentation/Doxyfile` | PASS |
| 2026-02-19T00:26:12Z | Phase 2.6 acceptance/tuning QA build | `cmake --build build --target locusq_qa -j 8` | PASS |
| 2026-02-19T00:26:14Z | Phase 2.6 acceptance/tuning smoke suite | `./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json` | PASS (`4` scenarios: `4 PASS`, `0 WARN`, `0 FAIL`) |
| 2026-02-19T00:26:20Z | Phase 2.6 acceptance/tuning animation smoke | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_26_animation_internal_smoke.json` | PASS (`perf_avg_block_time=0.0659 ms`) |
| 2026-02-19T00:26:25Z | Phase 2.6 acceptance/tuning Phase 2.5 regression | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_5_acceptance_suite.json` | WARN (`9` scenarios: `8 PASS`, `1 WARN`, `0 FAIL`) |
| 2026-02-19T00:26:31Z | Phase 2.6 acceptance/tuning plugin build | `cmake --build build --target LocusQ_VST3 LocusQ_Standalone -j 8` | PASS |
| 2026-02-19T00:26:37Z | Phase 2.6 acceptance/tuning host-load verification | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`exit code 0`) |
| 2026-02-19T00:26:49Z | Phase 2.6 acceptance/tuning standalone smoke | `open build/LocusQ_artefacts/Standalone/LocusQ.app` | PASS (process observed/terminated) |
| 2026-02-19T00:26:52Z | Phase 2.6 acceptance/tuning API docs snapshot | `doxygen Documentation/Doxyfile` | PASS |
| 2026-02-19T01:31:55Z | Phase 2.6 acceptance closeout QA build | `cmake --build plugins/LocusQ/build --target locusq_qa -j 8` | PASS |
| 2026-02-19T01:31:58Z | Phase 2.6 full-system CPU gate | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512` | WARN (hard gates PASS; allocation soft warning) |
| 2026-02-19T01:32:00Z | Phase 2.6 host edge matrix (`44.1k/256`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 44100 --block-size 256` | PASS |
| 2026-02-19T01:32:01Z | Phase 2.6 host edge matrix (`48k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 512` | PASS |
| 2026-02-19T01:32:02Z | Phase 2.6 host edge matrix (`48k/1024`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 1024` | PASS |
| 2026-02-19T01:32:03Z | Phase 2.6 host edge matrix (`96k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 96000 --block-size 512` | PASS |
| 2026-02-19T01:32:05Z | Phase 2.6 acceptance suite rollup | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json` | PASS (`2 PASS / 1 WARN / 0 FAIL`) |
| 2026-02-19T01:32:09Z | Phase 2.6 closeout plugin build | `cmake --build plugins/LocusQ/build --target LocusQ_VST3 LocusQ_Standalone -j 8` | PASS |
| 2026-02-19T01:32:15Z | Phase 2.6 closeout host-load verification | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`exit code 0`) |
| 2026-02-19T01:32:24Z | Phase 2.6 closeout standalone smoke | `open plugins/LocusQ/build/LocusQ_artefacts/Standalone/LocusQ.app` | PASS |
| 2026-02-19T01:32:30Z | Phase 2.6 closeout WebView checklist (manual fallback) | Source/CMake verification of checklist items (PowerShell validator unavailable) | PASS |
| 2026-02-19T01:36:45Z | `/test` harness configure/build/ctest | `cmake -S $HARNESS -B $HARNESS/build_test ...; cmake --build $HARNESS/build_test; ctest --test-dir $HARNESS/build_test --output-on-failure` | PASS (`45/45`) |
| 2026-02-19T01:40:11Z | `/test` focused acceptance matrix suite (`2.6`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json` | WARN (`3` scenarios: `2 PASS`, `1 WARN`, `0 FAIL`) |
| 2026-02-19T01:40:15Z | `/test` focused acceptance matrix regression suite (`2.5`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_5_acceptance_suite.json` | WARN (`9` scenarios: `8 PASS`, `1 WARN`, `0 FAIL`) |
| 2026-02-19T01:40:20Z | `/test` focused full-system CPU gate | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512` | WARN (hard gates PASS; `perf_avg_block_time_ms=0.434559`, delta `+0.006446`) |
| 2026-02-19T01:40:24Z | `/test` focused host-edge matrix (`44.1k/256, 48k/512, 48k/1024, 96k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate ... --block-size ...` | PASS (`4/4`) |
| 2026-02-19T01:41:28Z | `/test` pluginval host validation | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`exit code 0`) |
| 2026-02-19T01:41:33Z | `/test` standalone smoke | `open plugins/LocusQ/build/LocusQ_artefacts/Standalone/LocusQ.app` | PASS (process observed/terminated) |
| 2026-02-19T01:42:38Z | `/test` trend delta publication | `jq ... plugins/LocusQ/qa_output/suite_result.json` | PASS (`overall=PASS_WITH_WARNING`; suite deltas unchanged, CPU delta published) |
| 2026-02-19T02:08:14Z | `/ship` universal configure | `cmake -S plugins/LocusQ -B plugins/LocusQ/build_ship_universal -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\" -DJUCE_DIR=.../_tools/JUCE` | PASS |
| 2026-02-19T02:10:36Z | `/ship` universal build | `cmake --build plugins/LocusQ/build_ship_universal --target LocusQ_VST3 LocusQ_AU LocusQ_Standalone -j 8` | PASS |
| 2026-02-19T02:27:33Z | `/ship` package + archive | `ditto -c -k --sequesterRsrc --keepParent dist/LocusQ-v0.1.0-macOS dist/LocusQ-v0.1.0-macOS.zip` | PASS |
| 2026-02-19T02:30:05Z | `/ship` pluginval on universal VST3 | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build_ship_universal/LocusQ_artefacts/Release/VST3/LocusQ.vst3` | PASS (`exit code 0`) |
| 2026-02-19T02:30:28Z | `/ship` standalone smoke on universal app | `open plugins/LocusQ/build_ship_universal/LocusQ_artefacts/Release/Standalone/LocusQ.app` | PASS (process observed/terminated) |
| 2026-02-19T03:07:13Z | `2.7a` UI runtime resilience syntax gate | `node --input-type=module --check < plugins/LocusQ/Source/ui/public/js/index.js` | PASS |
| 2026-02-19T03:07:13Z | `2.7a` UI runtime resilience rebuild | `cmake --build plugins/LocusQ/build --target LocusQ_VST3 -j 8` | PASS (warnings unchanged) |
| 2026-02-19T03:16:55Z | `/test` harness configure | `cmake -S $HARNESS_PATH -B $HARNESS_PATH/build_test -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5` | PASS |
| 2026-02-19T03:16:55Z | `/test` harness build | `cmake --build $HARNESS_PATH/build_test` | PASS |
| 2026-02-19T03:16:55Z | `/test` harness ctest | `ctest --test-dir $HARNESS_PATH/build_test --output-on-failure` | PASS (`45/45`) |
| 2026-02-19T03:16:55Z | `/test` LocusQ QA build (`2.7 UI matrix`) | `cmake --build plugins/LocusQ/build --target locusq_qa -j 8` | PASS |
| 2026-02-19T03:16:55Z | `/test` smoke suite (Emitter adapter) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json` | PASS (`4 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T03:16:55Z | `/test` animation smoke (`48k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_animation_internal_smoke.json` | PASS |
| 2026-02-19T03:16:55Z | `/test` phase 2.6 acceptance suite (`UI proxy`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json` | WARN (`2 PASS / 1 WARN / 0 FAIL`) |
| 2026-02-19T03:16:55Z | `/test` full-system CPU gate (`48k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512` | WARN (hard gates pass; allocation warning trend retained) |
| 2026-02-19T03:16:55Z | `/test` host-edge matrix (`44.1k/256,48k/512,48k/1024,96k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate ... --block-size ...` | PASS (`4/4`) |
| 2026-02-19T03:16:55Z | `/test` pluginval host validation (`2.7 UI matrix`) | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`exit code 0`) |
| 2026-02-19T03:16:55Z | `/test` standalone smoke (`2.7 UI matrix`) | `open plugins/LocusQ/build/LocusQ_artefacts/Standalone/LocusQ.app` | PASS |
| 2026-02-19T03:16:55Z | `/test` trend delta publication (`2.7 UI matrix`) | `updated plugins/LocusQ/qa_output/suite_result.json + plugins/LocusQ/TestEvidence/test-summary.md` | PASS (`overall=PASS_WITH_WARNING`) |
| 2026-02-19T03:30:15Z | `2.7a` manual host UI checklist handoff | `created plugins/LocusQ/TestEvidence/phase-2-7a-manual-host-ui-acceptance.md + synced status/plan/readme` | PASS (pending user-run DAW validation) |
| 2026-02-19T04:38:37Z | `2.7b` viewport/calibration JS syntax gate | `node --input-type=module --check < plugins/LocusQ/Source/ui/public/js/index.js` | PASS |
| 2026-02-19T04:38:37Z | `2.7b` VST3 rebuild | `cmake --build plugins/LocusQ/build --target LocusQ_VST3 -j 8` | PASS |
| 2026-02-19T04:41:31Z | `2.7b` smoke regression suite | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json` | PASS (`4 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T04:45:25Z | `/test` full acceptance rerun harness sanity | `cmake -S $HARNESS_PATH -B $HARNESS_PATH/build_test ...; cmake --build $HARNESS_PATH/build_test; ctest --test-dir $HARNESS_PATH/build_test --output-on-failure` | PASS (`45/45`) |
| 2026-02-19T04:45:25Z | `/test` full acceptance rerun suite matrix | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json; ./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_5_acceptance_suite.json; ./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json` | PASS_WITH_WARNING (`2 suites WARN-only; no FAIL`) |
| 2026-02-19T04:45:25Z | `/test` full acceptance rerun perf/host matrix | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512; host edge multipass @ 44.1k/256, 48k/512, 48k/1024, 96k/512` | PASS_WITH_WARNING (CPU allocation warning retained; host-edge `4/4` pass) |
| 2026-02-19T04:45:25Z | `/test` full acceptance rerun host validation | `cmake --build plugins/LocusQ/build --target LocusQ_VST3 LocusQ_Standalone -j 8; /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3; open plugins/LocusQ/build/LocusQ_artefacts/Standalone/LocusQ.app` | PASS (`pluginval exit 0`) |
| 2026-02-19T04:49:13Z | `/test` full acceptance rerun evidence publication | `updated plugins/LocusQ/qa_output/suite_result.json + plugins/LocusQ/TestEvidence/test-summary.md + plugins/LocusQ/status.json` | PASS (`overall=PASS_WITH_WARNING`) |
| 2026-02-19T04:49:13Z | `/test` manual host UI checklist scheduling | `manual checklist deferred by user` | DEFERRED (`run on 2026-02-20`) |
| 2026-02-19T05:19:40Z | `2.6c` QA build for allocation-free closeout | `cmake --build plugins/LocusQ/build --target locusq_qa -j 8` | PASS |
| 2026-02-19T05:19:42Z | `2.6c` full-system CPU gate (`48k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512` | PASS (`perf_avg_block_time_ms=0.304457`, `perf_p95_block_time_ms=0.318466`, `perf_allocation_free=true`) |
| 2026-02-19T05:19:43Z | `2.6c` Phase 2.6 acceptance suite refresh | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json` | PASS (`3 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T05:56:00Z | `skill_plan` ADR/docs freshness gate closeout sync | `updated ADR-0005 + standards/plan/readme/changelog/status/build-summary/validation-trend` | PASS |
| 2026-02-19T05:33:02Z | `2.7d` acceptance rebaseline harness + suites + host-edge | `harness ctest (45/45) + phase_2_5 suite + phase_2_6 suite + full-system CPU + host-edge 4-run matrix` | PASS |
| 2026-02-19T05:33:23Z | `2.7d` acceptance rebaseline pluginval run | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests plugins/LocusQ/build/LocusQ_artefacts/VST3/LocusQ.vst3` | FAIL (`exit 9`, segfault in automation test) |
| 2026-02-19T05:34:59Z | `2.7d` acceptance rebaseline pluginval retry | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`exit 0`) |
| 2026-02-19T05:58:40Z | `2.7d` JS syntax gate | `node --input-type=module --check < plugins/LocusQ/Source/ui/public/js/index.js` | PASS |
| 2026-02-19T05:58:45Z | `2.7d` VST3 + QA build | `cmake --build plugins/LocusQ/build --target LocusQ_VST3 locusq_qa -j 8` | PASS (warnings unchanged) |
| 2026-02-19T05:58:48Z | `2.7d` smoke suite | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa plugins/LocusQ/qa/scenarios/locusq_smoke_suite.json` | PASS (`4 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T05:58:50Z | `2.7d` animation smoke | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_animation_internal_smoke.json` | PASS (`perf_avg_block_time_ms=0.0534585`, deadline pass) |
| 2026-02-19T05:58:52Z | `2.7d` Phase 2.6 acceptance suite | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_phase_2_6_acceptance_suite.json` | PASS (`3 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T05:58:54Z | `2.7d` host-edge roundtrip check (`48k/512`) | `./plugins/LocusQ/build/locusq_qa_artefacts/locusq_qa --spatial plugins/LocusQ/qa/scenarios/locusq_26_host_edge_roundtrip_multipass.json --sample-rate 48000 --block-size 512` | PASS |
| 2026-02-19T19:12:25Z | `pluginval` flake probe (`5` repeats) | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`5/5`) |
| 2026-02-19T19:12:42Z | `pluginval` long probe (random seeds) | repeated `pluginval` runs (`20` cap) | FAIL on run `9` (`exit 9`, segfault) |
| 2026-02-19T19:13:53Z | `pluginval` deterministic repro seed | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --random-seed 0x2a331c6 build/LocusQ_artefacts/VST3/LocusQ.vst3` | FAIL (`exit 9`) |
| 2026-02-19T19:14:00Z | `pluginval` crash backtrace capture | `lldb --batch ... --random-seed 0x2a331c6 ...` | FAIL (top frame `SpatialRenderer::process`) |
| 2026-02-19T19:14:20Z | pluginval mitigation build | `cmake --build build --target LocusQ_VST3 locusq_qa -j 8` | PASS |
| 2026-02-19T19:15:05Z | `pluginval` deterministic seed after fix | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --random-seed 0x2a331c6 build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS |
| 2026-02-19T19:15:44Z | `pluginval` post-fix stability probe | `10x pluginval --strictness-level 5 --validate-in-process --skip-gui-tests build/LocusQ_artefacts/VST3/LocusQ.vst3` | PASS (`10/10`) |
| 2026-02-19T19:16:06Z | post-fix QA regression refresh | `locusq_smoke_suite + locusq_phase_2_6_acceptance_suite + locusq_phase_2_5_acceptance_suite + host_edge(48k/512) + full_system(48k/512)` | PASS |
| 2026-02-19T19:21:22Z | continue checkpoint build | `cmake --build build --target LocusQ_VST3 locusq_qa -j 8` | PASS |
| 2026-02-19T19:21:36Z | continue checkpoint smoke suite | `./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json` | PASS (`4 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T19:22:04Z | continue checkpoint docs freshness gate | `./scripts/validate-docs-freshness.sh` | PASS |
| 2026-02-19T19:28:49Z | phase 2.8 bus-layout expansion build | `cmake --build build --target LocusQ_VST3 locusq_qa -j 8` | PASS |
| 2026-02-19T19:28:49Z | phase 2.8 renderer 4-channel scenario | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_renderer_spatial_output.json --channels 4` | PASS |
| 2026-02-19T19:28:49Z | phase 2.8 smoke suite 4-channel mode | `./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json --channels 4` | PASS (`4 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T19:37:25Z | phase 2.8 output telemetry JS check | `node --input-type=module --check < Source/ui/public/js/index.js` | PASS |
| 2026-02-19T19:37:25Z | phase 2.8 output mapping build | `cmake --build build --target LocusQ_VST3 locusq_qa -j 8` | PASS |
| 2026-02-19T19:37:25Z | phase 2.8 stereo layout suite | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_stereo_suite.json` | PASS (`3 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T19:37:25Z | phase 2.8 quad layout suite | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_quad_suite.json` | PASS (`3 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T19:43:56Z | phase 2.8 mono layout suite | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_mono_suite.json` | PASS (`3 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T19:44:06Z | phase 2.11 snapshot migration QA build | `cmake --build build --target locusq_qa -j 1` | PASS |
| 2026-02-19T19:44:07Z | phase 2.11 snapshot migration suite (stereo) | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_11_snapshot_migration_suite.json` | PASS (`2 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T19:44:08Z | phase 2.11 snapshot migration legacy-layout scenario (quad) | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_211_snapshot_migration_legacy_layout.json --channels 4` | PASS |
| 2026-02-19T19:44:13Z | phase 2.9 CI quad/pluginval lane definition check | `rg -n "qa-pluginval-seeded-stress\|--channels 4\|ci_phase_2_6_host_edge_.*_ch4\|pluginval_seeded_stress/status.tsv" .github/workflows/qa_harness.yml` | PASS |
| 2026-02-19T19:44:13Z | phase 2.9 docs freshness gate check | `./scripts/validate-docs-freshness.sh` | PASS (`0 warning(s)`) |
| 2026-02-19T19:45:52Z | phase 2.10 renderer guardrail build | `cmake --build build --target LocusQ_VST3 locusq_qa -j 8` | PASS |
| 2026-02-19T19:45:58Z | phase 2.10 full-system CPU baseline (`8` emitters, `48k/512`) | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_26_full_system_cpu_draft.json --sample-rate 48000 --block-size 512` | PASS (`perf_avg_block_time_ms=0.304505`, `perf_p95_block_time_ms=0.323633`, `perf_allocation_free=true`) |
| 2026-02-19T19:45:59Z | phase 2.10 high-emitter guardrail stress (`16` emitters, `48k/512`) | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_29_renderer_guardrail_high_emitters.json --sample-rate 48000 --block-size 512` | PASS (`perf_avg_block_time_ms=0.412833`, `perf_p95_block_time_ms=0.433221`, `perf_allocation_free=true`) |
| 2026-02-19T19:46:00Z | phase 2.10 renderer CPU guardrail suite | `./build/locusq_qa_artefacts/locusq_qa --spatial qa/scenarios/locusq_phase_2_9_renderer_cpu_suite.json --sample-rate 48000 --block-size 512` | PASS (`2 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T19:46:01Z | phase 2.10 smoke suite regression | `./build/locusq_qa_artefacts/locusq_qa qa/scenarios/locusq_smoke_suite.json` | PASS (`4 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T20:26:03Z | phase 2.10b renderer trend build (`build_local`) | `cmake --build build_local --target LocusQ_VST3 locusq_qa -j 8` | PASS |
| 2026-02-19T20:25:24Z | phase 2.10b draft high-emitter matrix (`48k/512,96k/512` x `2ch/4ch`) | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial .../locusq_29_renderer_guardrail_high_emitters.json --sample-rate {48000,96000} --block-size 512 [--channels 4]` | PASS (`4/4`, allocation-free) |
| 2026-02-19T20:25:24Z | phase 2.10b final-quality high-emitter matrix (`48k/512,96k/512` x `2ch/4ch`) | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial .../locusq_210b_renderer_guardrail_high_emitters_final_quality.json --sample-rate {48000,96000} --block-size 512 [--channels 4]` | PASS (`4/4`, allocation-free, `perf_total_allocations=0`) |
| 2026-02-19T20:25:24Z | phase 2.10b trend suite matrix (`48k/512,96k/512` x `2ch/4ch`) | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial .../locusq_phase_2_10b_renderer_cpu_trend_suite.json --sample-rate {48000,96000} --block-size 512 [--channels 4]` | PASS (`3 PASS / 0 WARN / 0 FAIL` in each run) |
| 2026-02-19T20:25:51Z | phase 2.11b snapshot migration matrix configure | `cmake -S . -B build -DBUILD_LOCUSQ_QA=ON -DJUCE_DIR=/Users/artbox/Documents/Repos/audio-plugin-coder/_tools/JUCE -DQA_HARNESS_DIR=/Users/artbox/Documents/Repos/audio-dsp-qa-harness -DCMAKE_POLICY_VERSION_MINIMUM=3.5` | PASS |
| 2026-02-19T20:25:51Z | phase 2.11b snapshot migration matrix QA build | `cmake --build build --target locusq_qa -j 1` | PASS |
| 2026-02-19T20:27:42Z | phase 2.11b snapshot migration mono suite | `./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11b_snapshot_migration_mono_suite.json` | PASS (`2 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T20:27:42Z | phase 2.11b snapshot migration stereo suite | `./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11b_snapshot_migration_stereo_suite.json` | PASS (`2 PASS / 0 WARN / 0 FAIL`) |
| 2026-02-19T20:27:42Z | phase 2.11b snapshot migration quad suite | `./build/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_11b_snapshot_migration_quad_suite.json` | PASS (`2 PASS / 0 WARN / 0 FAIL`) |

## Notes
- Use `TestEvidence/build-summary.md` for latest snapshot details.
- Append one line per meaningful validation run; do not duplicate identical reruns unless outcome changes.
- Phase 2.4 acceptance moved from partial to complete based on the probe + scenario + suite closeout set.
- Historical Phase 2.6 allocation warning rows are superseded by the `2.6c` allocation-free closeout entries above.
