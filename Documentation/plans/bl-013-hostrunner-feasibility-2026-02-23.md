Title: BL-013 HostRunner Feasibility Package
Document Type: Planning
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-013 HostRunner Feasibility Package

## Scope

Establish a reproducible HostRunner feasibility lane after BL-012 tranche-1:

1. Add a minimal HostRunner probe path in `locusq_qa`.
2. Validate runtime behavior against a real `LocusQ.vst3` artifact.
3. Capture deterministic fallback behavior when no backend host is provided.
4. Record risks that block BL-013 promotion to `In Validation` or `Done`.

## Prototype Implementation

- QA probe entrypoint:
  - `qa/main.cpp`
  - Added `--host-runner-smoke` mode (VST3/AU format parser, plugin-path validation, HostRunner run path).
  - Added `--host-skeleton` mode (HostRunner without backend, expected `SKIPPED` contract).
- Feasibility lane script:
  - `scripts/qa-bl013-hostrunner-feasibility-mac.sh`
  - Supports optional build skip (`LQ_BL013_SKIP_BUILD=1`) for fast reruns against already-built artifacts.

## Latest Evidence

- Feasibility artifact (latest):
  - `TestEvidence/bl013_hostrunner_feasibility_20260223T173642Z/status.tsv`
  - `TestEvidence/bl013_hostrunner_feasibility_20260223T173642Z/report.md`
- Summary from `status.tsv`:
  - `hostrunner_vst3_probe`: `pass` (`dry.wav` + `wet.wav` emitted)
  - `hostrunner_vst3_skeleton_probe`: `pass`
  - `harness_host_ctest`: `pass`
  - overall: `pass`

## Feasibility Decision

BL-013 is **feasible and now in validation-ready state** for VST3 + skeleton contracts.

- Positive signal:
  - LocusQ executes HostRunner control flow deterministically for both backend VST3 and skeleton fallback.
  - Feasibility lane is scripted/reproducible with structured status/report artifacts.
  - Stage diagnostics are now emitted in probe logs (`HOSTRUNNER_STAGE init/prepare/render/release`).

## Risk Map

1. VST3 backend runtime stability
- Root cause (closed): `VST3PluginHost::loadPlugin()` called `unloadPlugin()`, which reset `processData_`; `configure()` then dereferenced null `processData_`.
- Closure patch:
  - `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/runners/vst3_plugin_host.cpp`
  - Added re-allocation/initialization guard for `processData_` in `configure()`.
  - Kept `processData_` lifecycle valid across unload/reconfigure cycles.
- Validation: backend probe now passes in BL-013 lane (`20260223T173642Z`).

2. Upstream harness host-test drift (optional lane)
- Root cause (closed): `MockPluginHost` in `host_runner_unit_test.cpp` did not implement `sendMidiEvents(...)`.
- Closure patch:
  - `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/tests/host_runner_unit_test.cpp`
  - Added `sendMidiEvents` mock implementation.
- Validation: harness host-focused ctests pass in BL-013 lane (`20260223T173642Z`).

3. CLAP host-path parity not yet in this lane
- Symptom: BL-013 lane currently validates VST3 path + skeleton fallback; CLAP host backend not promoted in this feasibility tranche.
- Impact: incomplete multi-format HostRunner coverage.
- Required closure: add CLAP backend probe once VST3 host path is stable.

## Recommended Next Slice

1. Add optional CLAP backend probe to `scripts/qa-bl013-hostrunner-feasibility-mac.sh` once host backend is available in harness path.
2. Decide BL-013 promotion target after one additional rerun on updated harness snapshot (pin/record harness revision used for green run).
3. Keep stage diagnostics enabled in `qa/main.cpp` to preserve deterministic failure-stage evidence.
4. Feed BL-013 lane into recurring host-regression cadence with BL-024/HX-03 when host runtime code changes.

## Repro Commands

```sh
# Full lane (build + probe)
./scripts/qa-bl013-hostrunner-feasibility-mac.sh

# Fast rerun against existing build artifacts
LQ_BL013_SKIP_BUILD=1 ./scripts/qa-bl013-hostrunner-feasibility-mac.sh
```
