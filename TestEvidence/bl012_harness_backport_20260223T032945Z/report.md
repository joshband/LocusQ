Title: BL-012 Harness Backport Tranche 1 Report
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-012 Harness Backport Tranche 1 (20260223T032945Z)

- overall: `pass`
- run_harness_sanity: `1`
- harness_path: `/Users/artbox/Documents/Repos/audio-dsp-qa-harness`
- qa_bin: `/Users/artbox/Documents/Repos/LocusQ/build_local/locusq_qa_artefacts/Release/locusq_qa`
- contract_suite: `qa/scenarios/locusq_contract_pack_suite.json`
- perf_probe_scenario: `qa/scenarios/locusq_26_full_system_cpu_draft.json`
- pass_count: `23`
- warn_count: `0`
- fail_count: `0`

## Assertions

- Harness sanity configure/build/ctest passes (unless skipped).
- Contract pack suite passes with coverage counts:
  - latency >= 1
  - smoothing >= 1
  - state >= 1
  - contract_scenarios >= 3
- Suite runtime_config precedence holds against conflicting CLI runtime flags.
- Perf scenario emits `perf_*` metrics without forcing `--profile`.
- Canonical `qa_output/suite_result.json` refreshed from contract-pack suite output.

## Artifacts

- `status.tsv`
- `contract_pack_suite_result_baseline.json`
- `contract_pack_suite_result_override_probe.json`
- `runtime_config_probe.tsv`
- `perf_metric_probe_result.json`
- `harness_*.log` (when harness sanity enabled)
- `locusq_qa_build.log`
