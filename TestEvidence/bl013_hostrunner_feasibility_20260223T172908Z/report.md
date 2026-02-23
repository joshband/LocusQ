Title: BL-013 HostRunner Feasibility Report
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-013 HostRunner Feasibility (20260223T172908Z)

- overall: `pass_with_warnings`
- build_dir: `/Users/artbox/Documents/Repos/LocusQ/build_bl013_hostrunner`
- build_config: `Release`
- qa_bin: `/Users/artbox/Documents/Repos/LocusQ/build_bl013_hostrunner/locusq_qa_artefacts/Release/locusq_qa`
- harness_path: `/Users/artbox/Documents/Repos/audio-dsp-qa-harness`
- run_harness_host_tests: `1`
- enable_clap_probe: `0`

## Prototype Lane

1. Build harness with `BUILD_HOST_RUNNER=ON` and run host-focused ctests (configurable).
2. Configure/build LocusQ with `BUILD_LOCUSQ_QA=ON` + `BUILD_HOST_RUNNER=ON`.
3. Run `locusq_qa --host-runner-smoke` against a real `LocusQ.vst3` artifact.
4. Run `locusq_qa --host-runner-smoke --host-skeleton` as deterministic fallback contract.

## Summary Counts

- pass: `14`
- warn: `2`
- fail: `0`

## Artifacts

- `status.tsv`
- `locusq_configure.log`
- `locusq_build.log`
- `hostrunner_vst3_probe.log`
- `hostrunner_vst3_skeleton_probe.log`
- `hostrunner_vst3_output/dry.wav` and `hostrunner_vst3_output/wet.wav` (when backend probe passes)
- `harness_host_ctest.log` (when harness-host-tests are enabled)
