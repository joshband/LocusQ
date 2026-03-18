---
Title: QA Platform Leverage Review
Document Type: Review
Author: Claude Code
Created Date: 2026-03-17
Last Modified Date: 2026-03-17
---

# QA Platform Leverage Review — 2026-03-17

## Executive Summary

LocusQ's QA platform is structurally more advanced than most audio plugin repos — the harness abstraction, the scenario engine, and the `BaseQARunner` template are real, working infrastructure — but LocusQ still retains responsibility ownership for several concerns that belong in the harness, and the harness itself has a false-green performance test hole that already affects multiple repos. The biggest single risk is that profiling precondition enforcement, while recently landed in the harness invariant evaluator, is still bypassed in LocusQ's `afterScenarioExecution` hook which re-attaches profiling metrics independently of the harness contract. The biggest opportunity is that BL-082 through BL-084 address precisely the right P0 items, but they are not yet implemented, meaning every CI run for every affected repo still has the potential for false-green performance assertions. The highest-leverage next move is BL-083+BL-084 together: move `applySuiteRuntimeConfig()` and the `ProfilingPolicy` enforcement into `ScenarioExecutor` as automatic steps, then delete the per-repo workarounds.

---

## Findings

### F-1: LocusQ `afterScenarioExecution` Re-attaches Profiling Metrics Outside Harness Precondition Control (Severity: Critical)

- **What is wrong:** `LocusQQARunner.cpp:463–469` — `afterScenarioExecution()` calls `attachProfilingMetrics()`, which independently constructs a new DUT instance, runs `profileDspPerformance()`, and writes `result.performanceMetrics` regardless of whether the harness `ProfilingPolicy` precondition check has been or will be satisfied. The harness invariant evaluator (`invariant_evaluator.cpp:91–125`) checks `config.enableProfiling` and `profilingMetricsAttached` before evaluating `perf_*` invariants — but that check happens *after* `afterScenarioExecution` has already run (see `BaseQARunner.h:344`). The profiling metrics are therefore attached by the plugin-side hook before the harness gets a chance to enforce the precondition.
- **Why it matters:** A scenario that has `perf_*` invariants and `enableProfiling=false` in its suite config will have profiling metrics unconditionally injected by LocusQ's hook, making the harness precondition check see `profilingMetricsAttached=true` and potentially pass the precondition gate it was designed to block. The backlog docs for BL-084 identify this pattern as the cause of silent false-green performance tests in echoform and monument-reverb; LocusQ's own implementation has the same structural issue from the plugin side rather than from the suite side.
- **Owner:** downstream-LocusQ (remove `attachProfilingMetrics` and replace with harness-native `enableProfiling=true` in suite config, per BL-084 S3)
- **Smallest high-leverage fix:** Delete `attachProfilingMetrics()` from `LocusQQARunner.cpp` and migrate LocusQ performance suites to set `"enableProfiling": true` in their JSON suite configs. The harness `ScenarioExecutor` already supports this path via `resolveExecutionConfig(suite)`.

---

### F-2: `applySuiteRuntimeConfig()` Is Not Automatic — LocusQ and Other Repos Can Skip It Silently (Severity: High)

- **What is wrong:** `scenario_executor.cpp:129–131` exposes `resolveExecutionConfig()` as a public call on `ScenarioExecutor`, but calling it is optional. In `BaseQARunner.h:402–406`, `runResolvedSuite()` creates the executor and immediately calls `executor.resolveExecutionConfig(suite)` — so the runner app path does apply runtime-config overrides. However, the `runSingleScenario()` path at `BaseQARunner.h:322–398` calls `executor.execute(loadResult.scenario)` with the *non-suite* overload, which uses `config_` directly and never applies suite-level overrides. More critically, the BL-083 backlog explicitly documents that echoform and memory-echoes never call `applySuiteRuntimeConfig()`, and confirms a LocusQ 27-LOC manual workaround existed. The suite-path is correct today; the single-scenario path is not; and the contract is invisible to plugin authors.
- **Why it matters:** A scenario run in isolation (via `--scenario` flag) against a suite JSON that declares `"runtimeConfig": {"sampleRate": 48000}` will silently run at the runner's default sample rate. This is a hidden contract gap: the same scenario file produces different behavior depending on invocation mode.
- **Owner:** upstream-harness (BL-083: make runtime-config application automatic inside `ScenarioExecutor::execute()` for all execution paths)
- **Smallest high-leverage fix:** In `ScenarioExecutor::executeInternal()`, always apply a no-op `applySuiteRuntimeConfig` when `suite == nullptr` and emit a warning when a suite-level override would have changed the config but is not being applied.

---

### F-3: LocusQ QA Has No CMake/CTest-Registered Test Targets (Severity: High)

- **What is wrong:** The plugin's DSP logic, processor state machine, headphone calibration chain, and registration lock-free contract have no `ctest`-registered test targets. Running `ctest` from the LocusQ build tree produces zero test results for plugin-side code. The QA harness integration builds the scenario-runner binary, but scenario execution is driven entirely by JSON files and requires a pre-installed plugin binary — it is not a unit-level test of processor behavior. The companion Swift tests (`swift test`) are isolated in their own target and cover only the Swift surface.
- **Why it matters:** Any regression in `PluginProcessor.cpp`, `HeadTrackingBridge.h`, or the DSP chain is detectable only by running the full scenario harness or by manual host-load testing. There is no automated gate for the unit-level processor contract.
- **Owner:** downstream-LocusQ
- **Smallest high-leverage fix:** Add a `LocusQ_UnitTests` CMake target with `CTest` registration that covers at minimum: registration state-machine transitions, finite-output guardrail behavior (already exercised by BL-078 scenarios but not as unit tests), and the headphone calibration chain's parameter application. This does not require the full harness and should be buildable without it.

---

### F-4: Companion Wire Protocol Divergence — v1 Sent, v2 Tested (Severity: High)

- **What is wrong:** `companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift:5–53` validates a 52-byte v2 wire format (`PosePacket` with angular velocity and sensor location fields). `companion/Sources/LocusQHeadTrackingCompanion/main.swift:109–122` sends a 40-byte `PosePacketV1` struct. The test covers a format that the companion binary does not transmit.
- **Why it matters:** If the plugin bridge receiver is upgraded to parse v2 (e.g., to read `angularVx/y/z`), it will receive v1 packets and either fail silently or mis-parse fields. The test suite does not protect against this regression because it never exercises the sending path.
- **Owner:** downstream-LocusQ (align either test or sender; this is already noted in the second-opinion supplement but is relevant to QA platform leverage specifically because the companion has no integration test exercising the send→receive loop)
- **Smallest high-leverage fix:** Add a round-trip test that constructs a `PosePacketV1`, encodes it, then decodes the wire bytes against the expected v1 layout — separately from the v2 schema test.

---

### F-5: `sync-companion-app-mac.sh` Performs No Post-Sync Verification of Binary Identity (Severity: High)

- **What is wrong:** `scripts/sync-companion-app-mac.sh:61` copies the built backend binary into the installed app bundle with `cp -f`. The script then prints `stat` and `shasum` output at lines 82–83, but only for the *destination* binary — it does not compare destination hash against the source build artifact hash. There is no assertion that verifies the copy succeeded correctly. By contrast, `build-and-install-mac.sh` has `verify_binary_match()` at lines 192–213 which does exactly this comparison and exits with code 4 on mismatch. The companion sync script does not use this pattern.
- **Why it matters:** A stale, interrupted, or permission-failed copy can leave an older binary installed without the script detecting it. Developers may diagnose companion behavior against the wrong binary version.
- **Owner:** downstream-LocusQ
- **Smallest high-leverage fix:** Add a `verify_binary_match`-style assertion after the `cp -f` in `sync-companion-app-mac.sh` that compares source and destination SHA-256. The function already exists in `build-and-install-mac.sh` and could be extracted to a shared shell utility.

---

### F-6: Standalone UI Selftest Has No Machine-Readable Structured-Log Contract for Agent Consumers (Severity: Medium)

- **What is wrong:** `scripts/standalone-ui-selftest-production-p0-mac.sh` writes `RESULT_JSON`, `ATTEMPT_TABLE`, `META_JSON`, and `FAILURE_TAXONOMY` to `TestEvidence/` with timestamped filenames. The JSON result format is written by the standalone binary itself (not the script), and the failure taxonomy TSV is parsed by the script. However, there is no published schema contract for any of these artifacts — no JSON Schema, no documented field list, no harness-level assertion over the result structure. An agent reviewing this evidence has no machine-checkable way to confirm completeness.
- **Why it matters:** The selftest is the primary automated gate for validating the standalone binary before distribution. If the binary emits a partial or schema-violating JSON and the selftest script doesn't detect the malformation (it only checks for the presence of key fields, not structural validity), a broken selftest can produce a file that passes the script's exit-code check while containing incomplete evidence.
- **Owner:** downstream-LocusQ
- **Smallest high-leverage fix:** Author a JSON Schema for the selftest result artifact and add a `jq` validation step after the binary exits to confirm schema conformance before the script exits 0.

---

### F-7: `qa_harness_integration.cmake` Does Not Emit Which Detection Strategy Was Used at Configure Time in a Machine-Parseable Way (Severity: Medium)

- **What is wrong:** `cmake/qa_harness_integration.cmake:90–106` emits `message(STATUS ...)` messages for the detection strategy, but these go to the CMake configure log only. The detection strategy is stored as a target property `QA_HARNESS_DETECTION_STRATEGY` at line 162, but this property is not exported to any generated file that CI or downstream consumers can read without running CMake. BL-086 specifically notes that monument-reverb falls back to `github.token` (insufficient for private harness) as an example of silent environment-side failure.
- **Why it matters:** In a multi-repo CI environment, a runner that silently uses the `find_package` fallback (because the submodule was not checked out) may build against a stale or version-mismatched harness installation without any explicit indication in the build log that this happened.
- **Owner:** upstream-harness
- **Smallest high-leverage fix:** Have `enable_qa_harness()` write a small detection-result file (e.g., `${CMAKE_CURRENT_BINARY_DIR}/qa_harness_detection.json`) capturing strategy, resolved path, and harness version, which CI can artifact and inspect without parsing CMake log output.

---

### F-8: `EarPhotoMatcherTests` Performance Gate Is a Wall-Clock Bound Without Profiling Context (Severity: Medium)

- **What is wrong:** `EarPhotoMatcherTests.swift:83–123` — `testMatchEmbeddingsP90StaysUnderFiftyMilliseconds` runs 21 iterations, computes a p90, and asserts `< 50ms`. This runs in the XCTest process on whatever hardware is executing the test. No CPU frequency, workload concurrency, or machine-class information is captured alongside the assertion result.
- **Why it matters:** The 50 ms bound is likely generous on development hardware but could become load-dependent on CI runners with constrained CPU budgets. More importantly: the test result in TestEvidence records pass/fail but not the actual measured p90, making it impossible to track latency trends over time. If the p90 degrades from 1 ms to 40 ms over several builds, the gate is still passing and the regression is invisible.
- **Owner:** downstream-LocusQ
- **Smallest high-leverage fix:** Print the measured p90 value to test output even on pass, and capture it as a named metric in the evidence TSV. This converts the binary pass/fail gate into a trend-trackable measurement.

---

### F-9: CI Checkout Composite Action Does Not Exist — Four Repos Maintain Divergent YAML Blocks (Severity: Medium)

- **What is wrong:** BL-086 explicitly documents that monument-reverb uses `github.token` (insufficient for private repo access), echoform and memory-echoes have inconsistent error messaging, and any harness URL change requires four-repo YAML updates. The composite action described in BL-086 does not yet exist. This is a live risk: any harness breaking change currently requires manual coordination across all four consuming repos.
- **Why it matters:** The CI checkout block is not just boilerplate duplication — it is the critical authentication path for the private harness repo. A divergent token strategy in one repo means that repo's CI may silently fall back to a cached or system harness version rather than failing loudly with an actionable error.
- **Owner:** upstream-harness (BL-086)
- **Smallest high-leverage fix:** Author `.github/actions/checkout-qa-harness/action.yml` in `audio-dsp-qa-harness` with token validation step (non-empty check with actionable error message), then update LocusQ's `qa_harness.yml` as the reference consumer.

---

### F-10: Perceptual Harness Analysis Script Lives Locally in LocusQ — Schema Drift Is Already Possible (Severity: Medium)

- **What is wrong:** BL-081 describes the extraction of `bl060-analyze-results.py` into `audio-dsp-qa-harness/tools/perceptual/`. At the time of this review, the upstream package does not yet exist. LocusQ's BL-060 evidence (`TestEvidence/bl060_phase_b_listening_*/`) is therefore produced by a local script with no cross-repo schema contract. If echoform or memory-echoes run similar listening studies with independently authored analysis scripts, their gate evidence will have different field names, gate hash formats, and reproducibility contract.
- **Why it matters:** The BL-081 gate criterion requires that the upstream script reproduce the existing BL-060 evidence byte-for-byte. The longer the extraction is deferred, the more likely the local script diverges from whatever upstream will implement, making byte-for-byte reproduction harder.
- **Owner:** upstream-harness (BL-081)
- **Smallest high-leverage fix:** Freeze the BL-060 `bl060-analyze-results.py` CLI surface and gate-hash now (already done: `1849befd4fda3f44`) and prevent local modifications until the upstream package exists.

---

### F-11: Host-Runner Smoke Path Has No Structured Exit-Code Contract Visible to CI (Severity: Low)

- **What is wrong:** `LocusQQARunner.cpp:226–342` — `runHostRunnerSmoke()` writes `HOSTRUNNER_SMOKE_PASS` or `HOSTRUNNER_SKELETON_PASS` to stdout and returns 0 or 1. The stage telemetry (`HOSTRUNNER_STAGE ...`) goes to stderr. The `HOSTRUNNER_SMOKE_PASS` line is parseable by a human but not structured (no JSON, no TSV). CI jobs that run the host-runner smoke path and want to assert on specific output fields (format, plugin path, dry/wet path) must regex-parse unstructured stdout.
- **Why it matters:** If the plugin path or output directory changes, the PASS line changes silently without breaking the exit code, and a CI job that regex-matches the old path will either fail or miss the regression depending on implementation.
- **Owner:** downstream-LocusQ
- **Smallest high-leverage fix:** Write a JSON result file to `outputDir/hostrunner_smoke_result.json` from `runHostRunnerSmoke()` with structured fields, mirroring the harness `ResultExporter` pattern already used for scenario results.

---

## Harness Leverage Opportunities

**BL-083: `applySuiteRuntimeConfig()` as automatic harness step.** The harness already has `resolveExecutionConfig()` and uses it on the suite path. Making it the universal default — with a no-op passthrough when the suite has no overrides — eliminates the single-scenario silent-skip risk (F-2) and removes the last manual workaround in LocusQ's runner.

**BL-084: `ProfilingPolicy` enforcement in `InvariantEvaluator`.** The evaluator already checks `enableProfiling` before evaluating `perf_*` invariants (confirmed in `invariant_evaluator.cpp:91–125`). The remaining gap is that `afterScenarioExecution` in LocusQ (and presumably echoform/monument-reverb equivalents) can inject profiling metrics before the evaluator runs, undermining the precondition check. The fix direction in BL-084 S3 is correct.

**BL-082: `BaseQARunner` template.** Already implemented and in use. LocusQ's `LocusQQARunner.cpp` is already a well-structured subclass. The remaining value is ensuring echoform, memory-echoes, and monument-reverb consume the same base rather than maintaining their own full implementations.

**`ResultExporter` as a reuse pattern.** LocusQ's host-runner smoke path (F-11) and the standalone selftest script (F-6) both produce ad hoc output artifacts. The harness `ResultExporter` already provides TSV/JSON export with consistent field naming. Extending it to cover host-runner and selftest output would bring those two surfaces under the same evidence contract.

**`qa_harness_integration.cmake` detection-result file.** The cmake module (already at commit `17f2992`) stores the detection strategy as a target property. Writing a machine-readable detection file (F-7) would be a small addition that benefits all four plugin repos with no API change.

---

## Observability / Debug / Telemetry Gaps

**Companion send-side to plugin receive-side correlation.** The companion writes a snapshot TSV via `SnapshotLogWriter` that records `packet_count`, `readiness_state`, `send_gate_open`, and pose fields. The plugin side has `PluginIngestSnapshot` fields (`ackAgeMs`, `poseAgeMs`, `sequence`) visible in the companion's UI. However, there is no automated cross-correlation of send sequence numbers against received sequence numbers in any test or script. A send gap (lost UDP packet) is observable in the UI monitor but not captured in any machine-readable QA artifact.

**Companion mode-switch telemetry.** When the companion transitions from `synthetic` to `live` mode or changes `schedulingProfile`, no event is logged. The snapshot TSV records the current mode per frame but not mode-change events as discrete records. A diagnostic run that switches modes mid-session produces a continuous log where the transition is only inferrable from column changes.

**Plugin processor diagnostics bridge latency.** `PluginProcessor.cpp` maintains `PublishedFiniteGuardrailDiagnostics`, `PublishedHeadphoneDiagnosticsSnapshot`, and registration transition telemetry, all published via `juce::AbstractFifo`-style lock-free paths. None of these surfaces are queryable from the QA scenario harness. A failing scenario with unexpected NaN output cannot be correlated with a concurrent guardrail event.

**Standalone selftest p90/trend.** As noted in F-6, the selftest writes a JSON result but does not include timing trends. There is no TestEvidence analog of `validation-trend.md` for selftest runs — only timestamped snapshot files that require manual comparison.

**`EarPhotoMatcher` p90 trend.** The companion XCTest performance gate records pass/fail per run but does not emit the measured p90 value into a persistent evidence file. This is a missing observability surface analogous to F-8.

---

## Automated Test Matrix Gaps

| Tier | Surface | Coverage | Gap |
|---|---|---|---|
| Unit | LocusQ processor state machine | None — no CMake/CTest targets | Complete gap (F-3) |
| Unit | Registration lock-free contract | None | Complete gap |
| Unit | HeadphoneCalibrationChain | None | Complete gap |
| Unit | HeadphoneVerificationSnapshot | None | Complete gap (synthetic scores not testable as unit) |
| Unit | Companion `PosePacketV1` send path | Partial — v2 tested, v1 not (F-4) | Send-path not exercised |
| Integration | Companion → plugin UDP round-trip | None | No automated test exercises this path end-to-end |
| Integration | Suite runtimeConfig single-scenario path | None | F-2: silent no-op on `--scenario` invocation |
| End-to-end | Host-runner smoke (AU/VST3 format load) | Manual + `--host-runner-smoke` flag | No CI gate for host-runner path |
| End-to-end | Standalone UI selftest | Script-driven, single run | No replay cadence; single attempt by default |
| Performance | `EarPhotoMatcher` p90 trend | Per-run only, no trend capture | F-8 |
| Performance | LocusQ DSP `perf_*` invariants with profiling precondition | At risk of false-green via F-1 | |
| Regression | Harness perceptual analysis | Local only, no upstream schema | F-10 |

---

## Benchmark / Metric Truthfulness Risks

**F-1 compounds here.** If `attachProfilingMetrics()` in `LocusQQARunner.cpp` injects profiling results unconditionally, then any suite that sets `enableProfiling: false` (or omits it) but has `perf_*` invariants will have those invariants evaluated against real profiling data anyway. This means the `ProfilingPolicy::WARN` default in the harness evaluator is bypassed entirely — the scenario records a PASS against profiling data that was collected outside the harness contract.

**Synthetic verification scores (documented in second-opinion supplement N1).** `buildHeadphoneVerificationSnapshot()` populates `frontBackScore`, `elevationScore`, `externalizationScore`, and `confidenceScore` from hardcoded per-engine tables. These values are published to the UI and, potentially, to QA evidence if a scenario exercises calibration verification. Any gate that relies on verification scores as evidence of perceptual quality is relying on fabricated constants.

**EarPhotoMatcher p90 without machine class context.** The 50 ms XCTest wall-clock bound (F-8) may pass comfortably on Apple Silicon development hardware and fail intermittently on CI runners with different scheduling behavior. The measured p90 is not captured, so trend degradation is invisible.

**Selftest JSON without schema validation.** The standalone selftest can write a structurally valid-but-incomplete JSON and the script will exit 0 if the top-level `pass` field is true. Fields that assertions depend on could be missing or null without triggering a gate failure.

---

## Companion QA And Install/Sync Risks

**Post-sync binary identity.** `sync-companion-app-mac.sh` (F-5) prints the destination binary hash but does not compare it to the source. A failed copy leaves the script exiting 0 with a stale binary installed.

**Three.js module sync is silent on miss.** `sync-companion-app-mac.sh:65–73` copies Three.js modules with `cp -f` if the source exists, but does nothing if the source is missing — no warning, no error. An operator who runs the sync after a clean `node_modules` (before npm install) will have a companion app bundle with stale or missing Three.js resources without knowing.

**Companion build does not verify protocol version alignment.** There is no build-time or CI assertion that `main.swift`'s `PosePacketV1` magic/version constants match the `LocusQHeadTrackerCore` `PosePacket` constants. A change to the core constants that is not reflected in `main.swift` (or vice versa) would produce silent protocol divergence.

**Companion selftest coverage of `--bl058-profile-selftest` path.** The `--bl058-profile-selftest <dir>` flag runs a headless BL-058 profile acquisition selftest and writes artifacts to a directory. The `status.json` notes confirm this path was exercised manually (`7/7 PASS`, `matching_latency=0.1050ms`), but there is no automated CI lane that runs this flag on each build. Evidence from this path exists only as point-in-time TestEvidence snapshots.

**Snapshot log TSV completeness.** `SnapshotLogWriter` at `main.swift:697` writes a fixed 17-column TSV header. If the schema evolves (new columns added), consumers parsing this TSV by column index will silently read wrong fields. There is no schema version field in the header.

**`live` mode companion has no automated test.** All companion unit tests and the BL-058 selftest run in controlled/synthetic contexts. The `live` mode code path (CMHeadphoneMotionManager, `recenterOnStart`, `requireSyncToStart`, axis flip state) has no automated test coverage. Bugs in the live mode pose pipeline are detectable only through manual device testing.

---

## Proposed Upstream Backlog

### U-1: `ScenarioExecutor` Automatic `applySuiteRuntimeConfig()` for All Execution Paths
- **Upstream owner:** audio-dsp-qa-harness
- **Rationale:** The suite-path applies runtime-config overrides; the single-scenario path does not. Any plugin that runs scenarios in isolation against a suite-defining JSON will silently use wrong audio config. Four repos are affected.
- **Likely evidence contract:** Scenario executed with `--scenario` flag against a suite that specifies `sampleRate: 48000` uses 48000 Hz. Hash-comparable output parity with suite-path execution for the same scenario.
- **Expected leverage:** All four plugin repos (LocusQ, echoform, memory-echoes, monument-reverb); BL-083.

### U-2: `ProfilingPolicy` Enforcement Before `afterScenarioExecution` Hook
- **Upstream owner:** audio-dsp-qa-harness
- **Rationale:** The current harness hook ordering (execute → afterScenarioExecution → evaluateInvariants) allows a plugin's `afterScenarioExecution` to inject profiling metrics that the `InvariantEvaluator`'s `ProfilingPolicy` check will then see as satisfied. The harness should enforce the profiling precondition check before invoking `afterScenarioExecution`, or document that profiling metrics injected by the plugin hook are treated as harness-contract profiling data only when `enableProfiling=true`.
- **Likely evidence contract:** A scenario with `perf_*` invariants and `enableProfiling=false`, where the plugin's `afterScenarioExecution` would inject metrics, produces SKIP/WARN (not PASS) under the `WARN` policy.
- **Expected leverage:** LocusQ (F-1), and any plugin that has custom `afterScenarioExecution` profiling injection; BL-084.

### U-3: CI Checkout Composite Action for Private Harness
- **Upstream owner:** audio-dsp-qa-harness
- **Rationale:** Four repos maintain divergent checkout blocks. monument-reverb uses `github.token` (insufficient). Any harness URL change requires four-repo coordination.
- **Likely evidence contract:** LocusQ CI `qa_harness.yml` uses the composite action. Missing `SUBMODULE_TOKEN` produces actionable error, not generic 401.
- **Expected leverage:** All four plugin repos; BL-086.

### U-4: Perceptual Analysis Shared Package (`tools/perceptual/`)
- **Upstream owner:** audio-dsp-qa-harness
- **Rationale:** `bl060-analyze-results.py` schema, gate constants, and TSV artifact layout are only defined in LocusQ. Any adopting repo must copy or independently reimplement.
- **Likely evidence contract:** BL-060 gate-hash `1849befd4fda3f44` reproducible via upstream path against existing fixture.
- **Expected leverage:** LocusQ (shim), plus echoform, memory-echoes, monument-reverb for future perceptual study lanes; BL-081.

### U-5: `enable_qa_harness()` Detection-Result File for Machine-Readable Strategy Audit
- **Upstream owner:** audio-dsp-qa-harness
- **Rationale:** The cmake module stores detection strategy as a target property but emits no inspectable artifact. CI jobs cannot assert which detection path was used without parsing configure logs.
- **Likely evidence contract:** `${CMAKE_BINARY_DIR}/qa_harness_detection.json` present after configure, with `strategy`, `harness_path`, and `harness_version` fields.
- **Expected leverage:** All four plugin repos, especially in shared CI environments where multiple detection strategies may be silently active.

---

## Proposed Downstream Backlog

### D-1: Remove `attachProfilingMetrics()` from `LocusQQARunner.cpp`; Migrate to Suite-Level `enableProfiling`
- **Downstream owner:** LocusQ
- **Rationale:** F-1 — the plugin-side profiling injection bypasses the harness `ProfilingPolicy` precondition contract.
- **Likely evidence contract:** LocusQ performance scenarios PASS with `"enableProfiling": true` in suite JSON; the `afterScenarioExecution` hook body returns without injecting metrics.
- **Expected leverage:** LocusQ only; prerequisite for clean BL-084 adoption.

### D-2: Add `verify_binary_match` to `sync-companion-app-mac.sh`
- **Downstream owner:** LocusQ
- **Rationale:** F-5 — post-sync hash verification is present in `build-and-install-mac.sh` but missing from the companion sync script.
- **Likely evidence contract:** Script exits 4 on hash mismatch; exits 0 only when source and destination binaries are byte-identical.

### D-3: Add `warn` or `error` for Missing Three.js Modules in `sync-companion-app-mac.sh`
- **Downstream owner:** LocusQ
- **Rationale:** Silent skip on missing `node_modules` Three.js source leaves companion app with stale or absent resources.
- **Likely evidence contract:** Script emits `WARN: Three.js source missing — companion UI resources not updated` when source path is absent.

### D-4: JSON Schema and Validation Step for Standalone Selftest Result Artifact
- **Downstream owner:** LocusQ
- **Rationale:** F-6 — the selftest JSON has no schema contract; structural incompleteness can pass the exit-code check.
- **Likely evidence contract:** `jq` validation step in the script checks that all required fields are present and non-null before exit.

### D-5: Add CMake/CTest-Registered Unit Tests for PluginProcessor
- **Downstream owner:** LocusQ
- **Rationale:** F-3 — no unit-level test targets exist for the plugin side.
- **Likely evidence contract:** `ctest` from LocusQ build tree runs and passes at least: registration state-machine transitions, finite-output guardrail isolation, headphone calibration chain parameter application.

### D-6: Add Round-Trip Test for `PosePacketV1` Wire Format
- **Downstream owner:** LocusQ
- **Rationale:** F-4 — companion tests cover v2 packet format; the v1 sending path is untested.
- **Likely evidence contract:** XCTest case that constructs `PosePacketV1`, encodes via `encodedData()`, and asserts all field offsets match the v1 wire spec (40 bytes, magic `0x4C515054`, version 1).

### D-7: CI Lane for `--bl058-profile-selftest` Companion Path
- **Downstream owner:** LocusQ
- **Rationale:** BL-058 profile acquisition selftest is only validated manually. The path exercises `EarPhotoMatcher`, `CalibrationProfile` write, and the BL-058 telemetry contract.
- **Likely evidence contract:** CI step runs `locusq-headtrack-companion --bl058-profile-selftest <dir>` and asserts presence and non-empty content of expected artifact files.

### D-8: Capture and Export `EarPhotoMatcher` p90 Value to Evidence TSV
- **Downstream owner:** LocusQ
- **Rationale:** F-8 — the p90 is currently a pass/fail binary in XCTest output with no trend-trackable artifact.
- **Likely evidence contract:** Test case writes `matching_latency_p90_ms` to a TSV in a location analogous to `TestEvidence/`, or at minimum prints a structured key=value line that CI can artifact.

---

## Suggested Review Sequence For Future Agents

For maximum anti-anchoring signal on the LocusQ + harness QA platform:

1. **`audio-dsp-qa-harness/lib/qa_runner_app/BaseQARunner.h`** — understand what the harness provides before reading any plugin-side runner code.
2. **`audio-dsp-qa-harness/scenario_engine/scenario_executor.cpp`** — understand the execution model, runtime-config application, and profiling precondition hook ordering.
3. **`audio-dsp-qa-harness/scenario_engine/invariant_evaluator.cpp`** — understand what the harness enforces on invariants vs. what it expects the plugin to have done beforehand.
4. **`qa/LocusQQARunner.cpp`** — read the full plugin-side runner implementation with harness behavior fresh in mind; F-1 is only visible after reading steps 1–3.
5. **`qa/main.cpp`** — brief; confirms the JUCE initializer wrapper and the thin entrypoint.
6. **`scripts/build-and-install-mac.sh`** — understand the install contract and binary verification pattern before reading the companion sync script.
7. **`scripts/sync-companion-app-mac.sh`** — F-5 is only visible when contrasted with the `verify_binary_match` pattern in the build script.
8. **`companion/Sources/LocusQHeadTrackingCompanion/main.swift` (lines 1–250)** — wire protocol, `PosePacketV1`, argument parsing.
9. **`companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift`** — F-4 is only visible when contrasted with step 8.
10. **`companion/Tests/LocusQHeadTrackerTests/EarPhotoMatcherTests.swift`** — performance gate and fallback threshold coverage; F-8 visible here.
11. **Backlog docs BL-082 through BL-086** — read after forming independent findings from the source; these docs identify what the team already knows is wrong.
12. **`audio-dsp-qa-harness/cmake/qa_harness_integration.cmake`** — confirms BL-085 state; F-7 visible here.
13. **`scripts/standalone-ui-selftest-production-p0-mac.sh` (lines 1–120)** — F-6 visible from the parameter declarations and absence of schema validation.
14. **Existing reviews** (`2026-03-17-second-opinion-code-dsp-supplement.md`, `2026-03-17-comprehensive-code-dsp-review.md`) — read last; confirm or extend independent findings.

---

## Validation Status

- **Status:** not tested
- **Commands run:** none — this is a read-only review; no build or test commands were executed.
- **Residual uncertainty:**
  - Whether `attachProfilingMetrics()` in `LocusQQARunner.cpp` is actually called before `InvariantEvaluator::evaluateInto()` in the full suite execution path. The `BaseQARunner.h` source confirms the ordering (`afterScenarioExecution` at line 344, then `evaluateInto` at line 346), supporting F-1, but this was not verified by running a scenario with `enableProfiling=false` and a `perf_*` invariant.
  - Whether the single-scenario path (`--scenario`) truly skips runtime-config application in the current harness. The executor source at `scenario_executor.cpp:137–139` confirms `execute(scenario)` (no-suite overload) does not call `applySuiteRuntimeConfig`, supporting F-2, but this was not verified with an actual test run.
  - The current state of echoform, memory-echoes, and monument-reverb runner implementations — only LocusQ was read; cross-repo false-green claims in BL-083/BL-084 backlog are taken at face value.

---

## Reference Integration Cross-Reference (echoform / monument-reverb / memory-echoes)

*Added: 2026-03-17. These three repos are treated as integrated audio-dsp-qa-harness successes and used to classify findings as "already solved," "partially solved," or "net new." All code references below were directly read during this analysis.*

### Cross-Reference Table

| ID | Finding / Item | Status | Reference Repo | File Reference | Notes |
|----|---------------|--------|----------------|----------------|-------|
| F-1 | `afterScenarioExecution` profiling injection | Net new | — | — | None of the three repos subclass the runner or override `afterScenarioExecution`; the pattern is LocusQ-specific |
| F-2 | `applySuiteRuntimeConfig()` not automatic | Partially solved | echoform | `scenarios/echoform_performance_suite.json:9–14` | echoform uses `sharedConfig` with `enable_profiling: true` as the workaround; single-scenario gap is unaddressed in all three repos |
| F-3 | No CMake/CTest unit test targets for processor | Partially solved | memory-echoes | `CMakeLists.txt:539–567` | memory-echoes registers three labelled QA CTest targets (smoke/critical/full); echoform registers one unlabelled harness target; monument-reverb registers none |
| F-4 | Companion wire protocol v1 sent, v2 tested | Net new | — | — | No reference repo has a companion protocol or wire-format test |
| F-5 | `sync-companion-app-mac.sh` no binary identity check | Net new | — | — | No reference repo has a companion sync script or binary hash verification pattern |
| F-6 | Selftest JSON schema contract absent | Net new | — | — | No reference repo produces or validates a selftest result schema |
| F-7 | cmake detection strategy not machine-readable | Net new | — | — | None of the three repos use `qa_harness_integration.cmake`; each inlines its own harness integration; detection-result file pattern is not implemented anywhere |
| F-8 | `EarPhotoMatcher` p90 without machine context | Net new | — | — | No reference repo has XCTest performance gates or p90 trend capture |
| F-9 | CI checkout composite action absent | Net new | — | — | None of the three repos have any `.github` YAML at all |

## Codex Second Opinion

Only net-new signal from a source-first pass is included below. I read the requested LocusQ, harness, and reference-repo source before opening this review. Where I agree with the existing review, I only call out the part that materially changes prioritization or framing.

### [HIGH] C-1 — The real harness blocker is not a profiling-policy bypass; it is that the standardized BL-085 integration path and the autonomous host-validation path still diverge

- Classification: `architecture misplacement`
- Evidence:
  - `qa/LocusQQARunner.cpp:130-137` only attaches profiling metrics when `config.enableProfiling` is already true.
  - `audio-dsp-qa-harness/lib/qa_runner_app/README.md:75-78` explicitly documents `afterScenarioExecution()` as the intended place for per-scenario profiling attachment.
  - `audio-dsp-qa-harness/cmake/qa_harness_integration.cmake:90-95` force-disables `BUILD_HOST_RUNNER` for source-tree consumer builds.
  - `audio-dsp-qa-harness/CMakeLists.txt:1246-1257` disables package export/install when `BUILD_HOST_RUNNER=ON`.
  - `qa/LocusQQARunner.cpp:443-447` makes LocusQ’s `--host-runner-smoke` path unavailable unless the consumer is rebuilt with `BUILD_HOST_RUNNER=ON`.
  - `TestEvidence/validation-trend.md:18` shows BL-067 is still blocked at inventory/manual-host level, and `TestEvidence/validation-trend.md:487` shows a separate HostRunner-enabled build had to be created explicitly.
- Why it matters:
  - The current mainstream harness-consumer path is optimized for reusable integration, while the host-validation path still requires an alternate build posture. That is the stronger architectural reason fully autonomous host smoke is still not routine.
- Relationship to existing review:
  - `Contradicts` F-1’s core claim that LocusQ’s profiling hook bypasses the profiling precondition. The source does not support that.
  - `Extends` F-7/F-9 by identifying the deeper blocker: standard harness integration and host automation are still split into different build modes.

### [HIGH] C-2 — The companion’s default QA entrypoint is compile-red right now, so companion automation is missing a stable baseline before higher-order harness work

- Classification: `missing automation`
- Evidence:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:3048-3051` contains regex-style escape sequences inside a Swift string literal.
  - In this review turn, `cd companion && swift test` failed on those exact lines with `invalid escape sequence in literal`.
- Why it matters:
  - This shifts the immediate automation priority. Before richer protocol, install, or observability work can become trustworthy, the companion needs a dependable compile-smoke/package-test lane again.
- Relationship to existing review:
  - `Net new signal`. The existing review was explicitly not tested; this changes the current state from “coverage discussion” to “baseline lane broken.”

### [MEDIUM] C-3 — The suite/single-scenario isolation issue is better solved as a harness-native “run one scenario under suite context” mode, not by making single-scenario execution silently inherit suite config

- Classification: `architecture misplacement`
- Evidence:
  - `audio-dsp-qa-harness/lib/qa_runner_app/BaseQARunner.h:85-89` presents `<scenario.json>` and `<suite.json>` as intentionally distinct CLI modes.
  - `audio-dsp-qa-harness/lib/qa_runner_app/BaseQARunner.h:322-347` runs a single scenario against the base execution config.
  - `audio-dsp-qa-harness/lib/qa_runner_app/BaseQARunner.h:400-421` resolves suite config only on the suite path.
  - `audio-dsp-qa-harness/scenario_engine/scenario_executor.cpp:145-163` applies `runtime_config` only when a `TestSuite*` is present.
- Why it matters:
  - The risk is real, but the cleaner abstraction is not “make scenario mode inherit invisible suite state.” It is “add a filtered-suite execution mode that keeps suite provenance, suite runtime_config, and scenario-level focus together.”
- Relationship to existing review:
  - `Agrees` with F-2 that the isolation matters.
  - `Contradicts` F-2’s framing of this as a plain bug in the current single-scenario path; the current CLI contract is explicit, but incomplete for the use case agents actually need.

### [MEDIUM] C-4 — The harness already has a structured exporter, but LocusQ’s non-scenario QA surfaces still invent separate evidence contracts instead of reusing it

- Classification: `architecture misplacement`
- Evidence:
  - `audio-dsp-qa-harness/scenario_engine/result_exporter.h:13-32` and `audio-dsp-qa-harness/scenario_engine/result_exporter.cpp:1193-1225` / `1365-1368` already define a structured suite export model (`suite_result.json`, `report/summary.md`, `report/ci_summary.md`).
  - `qa/LocusQQARunner.cpp:263-267` and `qa/LocusQQARunner.cpp:324-327` still report host smoke as stderr/stdout strings (`HOSTRUNNER_STAGE`, `HOSTRUNNER_SMOKE_PASS`).
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:697-718` writes a fixed snapshot TSV header, while richer plugin-ingest fields only exist in JSON and console output at `companion/Sources/LocusQHeadTrackingCompanion/main.swift:1814-1824` and `companion/Sources/LocusQHeadTrackingCompanion/main.swift:3963-4023`.
- Why it matters:
  - Scenario/suite QA has a reusable machine-readable contract already. Host smoke, companion streaming telemetry, and similar “adjacent QA” surfaces are still living in one-off schemas that agents have to special-case.
- Relationship to existing review:
  - `Extends` F-11 and the companion observability discussion. The missing abstraction is broader than one JSON file: it is a shared auxiliary-evidence contract for non-scenario QA lanes.

### [MEDIUM] C-5 — The companion sync gap is bigger than missing hash comparison: the current flow patches an existing app bundle instead of installing a canonical built bundle

- Classification: `missing automation`
- Evidence:
  - `scripts/sync-companion-app-mac.sh:28-31` requires a pre-existing app bundle and aborts otherwise.
  - `scripts/sync-companion-app-mac.sh:51-77` only patches selected bundle contents (backend binary, two Three.js files, icon resource).
  - `scripts/sync-companion-app-mac.sh:56-58` only warns when the monitor launcher is missing.
- Why it matters:
  - Even if destination/source binary verification is added, the script is still not proving that the installed companion bundle as a whole matches a canonical build artifact. It is proving only that one binary was copied into an existing shell.
- Relationship to existing review:
  - `Extends` F-5. The binary-identity check is necessary, but not sufficient for end-to-end install truth.

### [LOW] C-6 — The “reference repos as harness integration successes” framing is only partly true; the migration surface is still active and uneven

- Classification: `architecture misplacement`
- Evidence:
  - `monument-reverb/CMakeLists.txt:1431-1462` still inlines its harness detection and target wiring.
  - `memory-echoes/CMakeLists.txt:490-540` still inlines `add_subdirectory(...)` and direct `qa_core` / `qa_runners` / `qa_scenario_engine` linkage.
  - `echoform/.github/workflows/qa_full.yml:16-20`, `memory-echoes/.github/workflows/qa_full.yml:24-28`, and `monument-reverb/.github/workflows/qa_harness.yml:16-19` / `70-74` show three live but non-identical CI consumption patterns.
- Why it matters:
  - The reference repos are successful harness consumers, but they are not yet converged on the same integration story. That means future leverage work needs to budget for migration/adoption, not just new harness features.
- Relationship to existing review:
  - `Contradicts` the cross-reference table’s final row, which says none of the three repos have `.github` YAML.
  - `Extends` the CI ownership discussion by showing the divergence is not theoretical; it is present in current source.

### Validation Notes For This Second Opinion

- Validation status: `partially tested`
- Commands run:
  - `cd /Users/artbox/Documents/Repos/LocusQ/companion && swift test` -> `FAIL`
  - `cd /Users/artbox/Documents/Repos/LocusQ && ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> `PASS`
  - `ctest --test-dir /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_bl084 --output-on-failure -R 'performance_invariant_test|test_suite_test|qa_runner_app_test'` -> `PASS`
| F-10 | Perceptual analysis script lives locally in LocusQ | Net new | — | — | No reference repo has a perceptual listening study or analysis script |
| F-11 | Host-runner smoke has no structured exit-code contract | Net new | — | — | LocusQ-specific architecture; no reference repo has an equivalent host-runner code path |
| U-1 | `ScenarioExecutor` automatic `applySuiteRuntimeConfig()` | Partially solved | echoform | `scenarios/echoform_performance_suite.json:9–14`, `qa/main.cpp:214–262` | echoform's suite JSON `sharedConfig.enable_profiling` exercises the correct suite path; single-scenario gap confirmed unaddressed in all three repos |
| U-2 | `ProfilingPolicy` enforcement before `afterScenarioExecution` | Net new | — | — | No reference repo has a plugin-side `afterScenarioExecution` override that injects profiling; the hook-ordering fix is purely harness-internal and has no reference implementation |
| U-3 | CI checkout composite action for private harness | Net new | — | — | No CI YAML exists in echoform, monument-reverb, or memory-echoes |
| U-4 | Perceptual analysis shared package | Net new | — | — | No reference repo has perceptual analysis infrastructure |
| U-5 | `enable_qa_harness()` detection-result file | Net new | — | — | None of the three repos use `qa_harness_integration.cmake`; the module's `QA_HARNESS_DETECTION_STRATEGY` target property is not exported by any consumer |
| D-1 | Remove `attachProfilingMetrics()` from LocusQ runner | Net new | — | — | None of the three repos have a plugin-side profiling injection hook to remove |
| D-2 | `verify_binary_match` in companion sync script | Net new | — | — | No reference repo has a companion sync script or binary hash assertion |
| D-3 | Warn on missing Three.js modules in sync script | Net new | — | — | No reference repo has a Three.js module sync path |
| D-4 | JSON schema for standalone selftest result artifact | Net new | — | — | No reference repo validates structured QA output with a JSON Schema |
| D-5 | CMake/CTest unit tests for `PluginProcessor` | Already solved | memory-echoes | `CMakeLists.txt:539–567` | memory-echoes registers three QA CTest targets with labels and timeouts; pattern is directly adaptable |
| D-6 | Round-trip test for `PosePacketV1` wire format | Net new | — | — | LocusQ-specific; no reference repo has a companion wire-protocol test |
| D-7 | CI lane for `--bl058-profile-selftest` companion path | Net new | — | — | No reference repo has an automated companion selftest CI lane |
| D-8 | Capture and export `EarPhotoMatcher` p90 to evidence TSV | Net new | — | — | No reference repo has a trend-trackable performance measurement exported from XCTest |

---

### Pattern Annotations

#### F-2 / U-1: echoform `sharedConfig` suite-level profiling (Partially solved)

echoform's `scenarios/echoform_performance_suite.json` (lines 8–14) sets `"sharedConfig": { ..., "enable_profiling": true }` in the suite JSON. The runner's `runTestSuite()` function in `qa/main.cpp` (lines 214–262) passes `&config` as the baseline config pointer to `suiteExecutor.execute()`. This exercises the suite-path execution correctly and is the intended workaround until the harness automatically applies `applySuiteRuntimeConfig()` on the single-scenario path. What LocusQ should adapt: move all performance scenario invocations to suite JSON invocation rather than `--scenario`-flag invocation, and set `enable_profiling: true` in the suite `sharedConfig`. This is a complete workaround for the current harness gap. What remains open: the single-scenario path gap (F-2) still exists in echoform as in all other repos — running `echoform_qa <performance_scenario.json>` without a suite file will use the default `config.enableProfiling = false` from `runScenario()` at `qa/main.cpp:133–138`.

#### D-5: memory-echoes CTest-registered QA targets (Already solved)

memory-echoes `CMakeLists.txt` (lines 539–567) registers three CTest targets tied to the QA harness binary: `memory_echoes_qa_smoke` (timeout 60s, labels `qa;smoke`), `memory_echoes_qa_critical` (timeout 120s, labels `qa;critical`), and `memory_echoes_qa_full` (timeout 180s, labels `qa;full`). Each uses `WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}` to ensure scenario JSON paths resolve correctly. The script `scripts/run_qa.sh` wraps these with `ctest --test-dir build -R memory_echoes_qa -V`. LocusQ should adapt this exact pattern for `D-5`: register `memory_echoes_qa_smoke`, `_critical`, and `_full` equivalents against LocusQ's scenario JSON paths, with matching labels and timeouts. The working-directory argument is the critical detail — without it, scenario paths in the JSON will fail to resolve when `ctest` changes directories. Note that memory-echoes still does not have CTest-registered unit tests for its `PluginProcessor` (it has deprecated JUCE console app tests under `ENABLE_TESTS`), so the unit-test coverage gap from F-3 is only addressed at the harness-scenario tier, not the processor-unit tier.

#### F-3: monument-reverb QA harness CMake gap (partially solved reference — negative example)

monument-reverb's `CMakeLists.txt` (lines 1426–1530) creates a `monument_qa` executable under `BUILD_QA_HARNESS` but does **not** call `add_test()` to register it with CTest. Running `ctest` from the monument-reverb build tree produces zero harness test results even when the binary is built. This is the exact gap LocusQ has (F-3) and confirms the pattern is easy to miss. The correct fix is present in memory-echoes, not monument-reverb.

---

### Net New Items

The following items have no reference implementation in any of the three repos and require original design for LocusQ:

- **F-1 / D-1:** Plugin-side `afterScenarioExecution` profiling injection and its removal. All three reference repos avoid the problem by not overriding the hook at all. LocusQ must delete the override and migrate to suite-level `enableProfiling`.
- **F-4 / D-6:** Companion wire protocol version alignment test. No reference repo has a companion binary or protocol test infrastructure.
- **F-5 / D-2:** Post-sync binary identity verification in the companion sync script. No reference repo has a companion sync script.
- **D-3:** Three.js module missing-source warning. No reference repo has a webview resource sync path.
- **F-6 / D-4:** JSON schema validation for selftest result artifacts. No reference repo validates QA output structure beyond exit-code checks.
- **F-7 / U-5:** Machine-readable cmake detection-result file. No reference repo uses `qa_harness_integration.cmake`; the cmake module exists in the harness but has no consumers who have exercised or extended the detection-result export.
- **F-8 / D-8:** XCTest p90 trend export for `EarPhotoMatcher`. No reference repo has Apple-platform XCTest performance gates.
- **F-9 / U-3:** CI checkout composite action. No reference repo has GitHub Actions YAML at all.
- **F-10 / U-4:** Perceptual listening study shared analysis package. No reference repo has perceptual QA infrastructure.
- **F-11:** Structured exit-code contract for host-runner smoke output. No reference repo has a host-runner smoke path.
- **U-2:** `ProfilingPolicy` enforcement before `afterScenarioExecution` hook. This is a harness-internal change with no reference implementation; all three reference repos avoid the issue by not overriding the hook.
- **D-7:** CI lane for `--bl058-profile-selftest` companion path. No reference repo has this.

---

### Summary

- **Already solved (can adapt existing pattern):** 1 item — D-5 (CTest-registered QA targets), directly adaptable from memory-echoes `CMakeLists.txt:539–567`.
- **Partially solved (need gap-filling):** 2 items — F-2/U-1 (suite-level `sharedConfig` workaround exists in echoform; single-scenario path gap confirmed across all three repos), F-3 (memory-echoes has CTest-registered harness targets; processor-unit-level coverage gap and monument-reverb `add_test` omission are confirmed as anti-examples).
- **Net new (need original design):** 22 items — F-1, F-4, F-5, F-6, F-7, F-8, F-9, F-10, F-11, U-2, U-3, U-4, U-5, D-1, D-2, D-3, D-4, D-6, D-7, D-8, and all items that are LocusQ-specific companion/selftest/host-runner concerns.
- **Highest-leverage "already solved" pattern to adopt first:** memory-echoes `CMakeLists.txt:539–567` — the three-target CTest registration pattern (smoke/critical/full with labels and timeouts). This directly unblocks D-5 and requires approximately 30 lines of CMake. The working-directory argument is the only non-obvious detail. Once this pattern is in place, `ctest -L smoke` from the LocusQ build tree will gate the smoke harness without any harness architecture changes.
