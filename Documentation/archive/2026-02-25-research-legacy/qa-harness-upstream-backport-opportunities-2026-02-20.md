Title: QA Harness Upstream and Backport Opportunities
Document Type: Comparative Analysis
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# QA Harness Upstream and Backport Opportunities

## Purpose
Compare LocusQ against migrated harness adopters (`echoform`, `memory-echoes`, `monument-reverb`) and define upstream enhancements that can be backported across plugin repos.

## Inputs Reviewed
- `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/docs/GENERALIZATION_QUICK_REFERENCE.md`
- `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/docs/ENHANCEMENT_GENERALIZATION_POLICY.md`
- `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/docs/guides/QA_AUTHORITY_CUTOVER_GUIDE.md`
- `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/docs/guides/QA_AUTHORITY_OWNERSHIP_MATRIX_TEMPLATE.md`
- `/Users/artbox/Documents/Repos/audio-dsp-qa-harness/docs/guides/CONTRACT_PACK_ADAPTATION_GUIDE.md`
- `/Users/artbox/Documents/Repos/echoform/qa/main.cpp`
- `/Users/artbox/Documents/Repos/memory-echoes/qa/main.cpp`
- `/Users/artbox/Documents/Repos/monument-reverb/qa/main.cpp`
- `qa/main.cpp`
- `/Users/artbox/Documents/Repos/echoform/.github/workflows/qa_critical.yml`
- `/Users/artbox/Documents/Repos/memory-echoes/.github/workflows/qa_critical.yml`
- `/Users/artbox/Documents/Repos/monument-reverb/.github/workflows/qa_harness.yml`
- `.github/workflows/qa_harness.yml`

## Comparison Snapshot

| Area | echoform | memory-echoes | monument-reverb | LocusQ | Observation |
|---|---|---|---|---|---|
| QA runner entrypoint size | 451 LOC | 527 LOC | 406 LOC | 597 LOC | Four repos maintain large custom `qa/main.cpp` apps with overlapping logic. |
| Profiling behavior | sets `ExecutionConfig.enableProfiling` | no dedicated profile flag in runner | no dedicated profile flag in runner | explicit `profileDspPerformance()` path in runner | Harness `ScenarioExecutor` does not currently consume `enableProfiling`; profiling behavior is inconsistent across repos. |
| Suite runtime override handling | no explicit `applySuiteRuntimeConfig()` call | no explicit `applySuiteRuntimeConfig()` call | no explicit `applySuiteRuntimeConfig()` call | custom shared-config parser (`sample_rate`, `block_size`, `num_channels`) | Runtime override behavior is fragmented and easy to drift. |
| Harness CMake integration | submodule-only path `external/qa_harness` | submodule-only path `external/audio-dsp-qa-harness` | submodule + `find_package` fallback + namespaced target fallback | submodule/sibling + `find_package` fallback + namespaced target fallback | Integration contracts differ by repo; maintainability cost is high. |
| CI harness checkout/auth | explicit `secrets.SUBMODULE_TOKEN` | explicit `secrets.SUBMODULE_TOKEN` | token fallback expression to `github.token` | token fallback expression to `github.token` | Private-repo harness checkout failure mode is not standardized. |
| Contract pack adoption | latency+smoothing+state contracts in critical/full suites | latency+smoothing+state contracts in critical/full suites | latency+smoothing+state contracts in critical/full suites | state roundtrip used in scenario set; no explicit latency/smoothing contract pack adoption | LocusQ can backport proven contract-pack pattern from migrated repos. |

## Prioritized Opportunities

| Priority | Opportunity | Evidence | Upstream Harness Change | Backport Scope |
|---|---|---|---|---|
| P0 | Shared QA runner app library (`qa_runner_app`) | 4 large custom runner mains across repos | Provide reusable CLI/execution module: option parsing, scenario/suite/discovery routing, suite summary, result export, runtime-config application, baseline/profile hooks | LocusQ, echoform, memory-echoes, monument-reverb reduce custom runner code to adapter registration + repo policy flags |
| P0 | Perf-metric contract hardening | `ExecutionConfig.enableProfiling` exists, but executor path currently does not apply profiling automatically | Add executor-native profiling policy: run profiling when `enableProfiling=true` or when any `perf_*` invariant exists; emit explicit warning/error mode when perf invariants are evaluated without profiling data | Prevent false-green perf suites in migrated repos and remove custom profiling glue in LocusQ |
| P0 | Runtime-config contract enforcement | Some repos never call `applySuiteRuntimeConfig()`, LocusQ uses custom override parsing | Move suite runtime-config application into a single harness-owned execution path (or expose mandatory helper used by runner app library) | Consistent behavior for `sample_rate`, `block_size`, `channels`, `seed`, and `output_dir` across repos |
| P1 | Standardized CMake integration module | CMake integration blocks differ significantly between repos | Add harness-provided CMake include/module (for example, `qa_harness_integration.cmake`) to standardize source fallback, package fallback, target resolution, and default build flags | Simplifies new plugin migrations and normalizes existing migrated repos |
| P1 | Reusable CI harness actions/templates | Checkout/publish/verification logic duplicated across workflows | Add harness-owned composite action(s) for checkout auth policy, suite result verification, and CI summary publishing with strict/private fallback rules | Reduces YAML drift and recurring token misconfiguration issues |
| P1 | Recursive discovery support option | Harness `discoverSuite()` is intentionally top-level only; echoform carries a custom recursive wrapper | Add optional recursive mode (`discoverSuite(..., recursive=true)` or separate API) with deterministic ordering and suite-file filtering | Removes custom recursion wrappers where nested scenario layout is standard |
| P1 | LocusQ contract-pack backport | Migrated repos use latency+smoothing+state contracts in critical/full suites | Reuse harness contract templates and adaptation guide for LocusQ scenario set | Faster parity with proven migration pattern and clearer cross-plugin comparability |
| P2 | HostRunner productionization | `HostRunner` exists as skeleton; no concrete host backend and MIDI path still marked TODO | Ship at least one concrete backend implementation (JUCE host path), with real plugin audio+MIDI execution and deterministic contract tests | Enables harness-native plugin-format validation and reduces ad hoc host/pluginval glue |

## Immediate Backport Plan For LocusQ

1. Add explicit contract-pack scenarios (`latency`, `parameter_smoothing`, `state_roundtrip`) from harness templates and wire into critical/full suites.
2. Align `qa/main.cpp` runtime overrides to harness-native `applySuiteRuntimeConfig()` semantics as an interim step until shared runner-app utility lands upstream.
3. Keep current Stage 14 manual DAW/device-profile checklist as release gate while upstream harness changes are developed.

## Progress Update (UTC 2026-02-20)

- Completed: `qa/main.cpp` now applies suite runtime overrides via `qa::scenario::applySuiteRuntimeConfig(...)` (custom local override parser removed).
- Completed: Added LocusQ contract-pack scenarios and suite:
  - `qa/scenarios/locusq_latency_processing_contract.json`
  - `qa/scenarios/locusq_parameter_smoothing_contract.json`
  - `qa/scenarios/locusq_state_roundtrip_contract.json`
  - `qa/scenarios/locusq_contract_pack_suite.json`
- Completed: CI critical lane now executes `locusq_contract_pack_suite` in `.github/workflows/qa_harness.yml`.
- Pending: upstream harness runner-app/perf/runtime-config unification and cross-repo (`echoform`, `memory-echoes`, `monument-reverb`) rollouts remain open.

## Additional Cross-Repo Verification (UTC 2026-02-20)

- `monument-reverb/scripts/install_macos.sh` and `monument-reverb/scripts/rebuild_and_install.sh` were reviewed for host-cache refresh patterns (`AudioComponentRegistrar` refresh and explicit old-bundle replacement semantics).
- LocusQ backported equivalent operational hardening into `scripts/build-and-install-mac.sh`:
  - optional REAPER pre-install shutdown flow,
  - AU registrar refresh,
  - deterministic REAPER cache prune for `LocusQ` entries,
  - post-install binary hash equality checks.
- `echoform` and `memory-echoes` continue to show strong harness-suite CI/runner patterns; no contradictory contract-pack findings were observed during this verification pass.

## Proposed Upstream Sequence

1. P0-1: shared runner app library.
2. P0-2: perf profiling execution hardening.
3. P0-3: runtime-config single-source execution.
4. P1: CMake + CI integration template modules.
5. P1: recursive discovery option.
6. P2: HostRunner backend completion.

## Gate For Declaring Backport Complete

- At least three migrated repos (including LocusQ) run on the same upstream runner-app/execution contract.
- Perf scenarios fail or warn explicitly when profiling data is absent.
- Runtime-config fields are applied identically across repos without plugin-local parsers.
- CI checkout/auth and summary publication are template-driven with no repo-specific token-expression drift.
