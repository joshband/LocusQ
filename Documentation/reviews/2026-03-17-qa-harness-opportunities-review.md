Title: QA Harness Opportunities Review
Document Type: Review
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# 2026-03-17 QA Harness Opportunities Review

## Review Status Legend

- `[HIGH]` blocks trustworthy automation, creates false-green risk, or leaves the most important QA contract unprovable.
- `[MEDIUM]` is a strong leverage opportunity where reusable harness ownership would remove repo-local drift or manual interpretation.
- `[LOW]` is cleanup debt that will otherwise create adoption confusion or recurring friction.

## Findings

### [HIGH] F1 — Head-tracking packet validation is split across incompatible contracts, so end-to-end companion-to-plugin QA is not trustworthy yet

- Evidence:
  - Live companion send path still constructs `PosePacketV1` and asserts a `40` byte payload in `companion/Sources/LocusQHeadTrackingCompanion/main.swift:101-120`, `companion/Sources/LocusQHeadTrackingCompanion/main.swift:4191-4199`, and `companion/Sources/LocusQHeadTrackingCompanion/main.swift:4522-4529`.
  - Companion receiver-side constants describe a different packet contract in `companion/Sources/LocusQHeadTrackingCompanion/main.swift:1218-1220`.
  - Plugin bridge advertises a `48` byte snapshot struct while also documenting `packetSizeV1 = 36` and `packetSizeV2 = 52` in `Source/HeadTrackingBridge.h:20-35` and `Source/HeadTrackingBridge.h:217-220`.
  - Companion tests cover only the V2 serializer shape in `companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift:5-33`.
- Why it matters:
  - The current tests can pass while the live sender, the companion-side receiver, and the plugin-side bridge disagree on what “the packet contract” actually is.
  - This is the clearest example in the repo of a QA gap that should not be solved with more repo-local assertions; it needs a shared contract test kit.
- Classification:
  - `companion` + `downstream-LocusQ` + `missing upstream-harness protocol contract`
- Smallest high-leverage fix direction:
  - Add an upstream `audio-dsp-qa-harness` protocol conformance kit with golden packet fixtures, serializer/deserializer property tests, and a loopback runner that both the companion and plugin bridge must pass unchanged.

### [HIGH] F2 — Headphone “verification” scores are synthetic policy scores, not measured evidence

- Evidence:
  - `Source/PluginProcessor.cpp:1199-1266` derives `frontBackScore`, `elevationScore`, `externalizationScore`, and `confidence` from hard-coded base values, fallback penalties, and a stage-based bias.
- Why it matters:
  - The runtime surface currently presents “verification” outputs that are heuristics rather than measured acoustic outcomes. That is a governance risk because dashboards, QA notes, or future gates can mistake them for objective evidence.
  - This is exactly the kind of false-green problem the harness should help prevent by making measured provenance explicit.
- Classification:
  - `governance` + `downstream-LocusQ runtime`
- Smallest high-leverage fix direction:
  - Treat these values as heuristics in the runtime UI and push any promotion-grade verification contract into harness-owned measured invariants and evidence packets.

### [HIGH] F3 — The companion’s main automated entrypoint is currently red before runtime tests even start

- Evidence:
  - `swift test` failed in this review turn because `companion/Sources/LocusQHeadTrackingCompanion/main.swift:3048-3051` contains regex-style JavaScript escapes inside a Swift literal, which produces `invalid escape sequence in literal`.
- Why it matters:
  - The companion QA stack is not just under-instrumented; the normal package test entrypoint is currently non-runnable. That means runtime and end-to-end assertions are being asked to compensate for a build-surface failure that should be caught much earlier.
  - For a low-human-touch QA program, “package compile smoke” is a mandatory gate, not optional background hygiene.
- Classification:
  - `companion` + `automation missing`
- Smallest high-leverage fix direction:
  - Add a mandatory compile-smoke/package-test gate for the companion, and move embedded JS/HTML generation toward a form that can be compiled and sanity-checked independently of the full app runtime.

### [HIGH] F4 — Install/update/sync truth is still split across multiple scripts with no shared machine-readable install manifest

- Evidence:
  - `scripts/build-and-install-mac.sh:8`, `scripts/build-and-install-mac.sh:16`, and `scripts/build-and-install-mac.sh:237-250` show that unattended success depends on local CMake policy/harness path resolution.
  - `scripts/build-and-install-mac.sh:258-260` and `scripts/build-and-install-mac.sh:314-320` make standalone install opt-in.
  - `scripts/build-and-install-mac.sh:347-362` verifies plugin bundle binaries, but not a unified multi-app install state.
  - `scripts/sync-companion-app-mac.sh:7-15`, `scripts/sync-companion-app-mac.sh:28-31`, `scripts/sync-companion-app-mac.sh:38-49`, and `scripts/sync-companion-app-mac.sh:60-83` mutate an existing companion app bundle in place and emit console output rather than a reusable install manifest.
- Why it matters:
  - The repo can build green while `/Applications` is stale for one app and current for the other.
  - In this session, unattended build/install required local hardening of `scripts/build-and-install-mac.sh` before the one-liner worked reliably. That confirms the user’s “minimal human in the loop” goal is not yet fully supported by the current install automation.
- Classification:
  - `downstream-LocusQ install/update` + `missing upstream harness install contract`
- Smallest high-leverage fix direction:
  - Create an upstream install-manifest + bundle-sync verifier that emits a single machine-readable artifact covering VST3/AU/CLAP/Standalone/Companion build-to-install parity.

### [MEDIUM] F5 — The standalone UI selftest is still a product-specific mini-framework embedded in one Bash script

- Evidence:
  - `scripts/standalone-ui-selftest-production-p0-mac.sh:33-68` defines a large result-path and artifact contract.
  - `scripts/standalone-ui-selftest-production-p0-mac.sh:146-180` layers auto-retry policy and scope-specific behavior.
  - `scripts/standalone-ui-selftest-production-p0-mac.sh:190-260` contains lock orchestration, result settling, and post-exit reconciliation logic.
- Why it matters:
  - This script is already doing the work of a reusable harness subsystem: launch coordination, retries, result stabilization, failure taxonomy capture, and machine-readable output.
  - Leaving it in `LocusQ/scripts/` guarantees the same shape of infrastructure will be rewritten for the next standalone or companion app.
- Classification:
  - `upstream-harness opportunity`
- Smallest high-leverage fix direction:
  - Extract an app/GUI selftest runner into `audio-dsp-qa-harness`, with thin product adapters for app path resolution and product-specific assertions.

### [MEDIUM] F6 — LocusQ’s QA runner still owns reusable responsibilities that should be harness-owned

- Evidence:
  - `qa/main.cpp:9-12` is already thin, which is good.
  - `qa/LocusQQARunner.cpp:64-123` still owns scenario parameter merging and parameter-name/index resolution.
  - `qa/LocusQQARunner.cpp:125-158` still owns profiling metric attachment policy.
  - `qa/LocusQQARunner.cpp:160-240` still owns host-runner parsing/backend selection and smoke construction.
- Why it matters:
  - BL-082/083/084 moved major responsibilities upstream, but the consumer runner is still accumulating cross-repo QA logic.
  - If the goal is a thin adapter model, this file is still carrying more than just DUT factories and product-specific routes.
- Classification:
  - `upstream-harness opportunity`
- Smallest high-leverage fix direction:
  - Continue lifting generic parameter application, profiling attachment, and host-smoke plumbing into `qa_runner_app` or adjacent harness modules so consumer repos only provide adapters and product routes.

### [MEDIUM] F7 — The most useful blocked-validation semantics are still repo-local conventions rather than harness-native result states

- Evidence:
  - BL-067 records honest “inventory only” and `BLOCKED` host states in `scripts/qa-bl067-auv3-lifecycle-mac.sh:163-190`.
  - BL-060 explicitly distinguishes statistical success from promotion readiness in `Documentation/backlog/bl-060-phase-b-listening-test-harness.md:11`, `Documentation/backlog/bl-060-phase-b-listening-test-harness.md:21`, `LocusQ-bl060-integrate/TestEvidence/bl060_blocked_sync_20260317T191327Z/lane_notes.md:29-40`, and `LocusQ-bl060-integrate/TestEvidence/bl060_blocked_sync_20260317T191327Z/execute_fixture/analysis/gate_summary.md:12-20`.
  - The harness already has the beginnings of this model for profiling preconditions in `audio-dsp-qa-harness/scenario_engine/invariant_evaluator.cpp:91-125`, `audio-dsp-qa-harness/lib/qa_runner_app/BaseQARunner.h:251-257`, and `audio-dsp-qa-harness/scenario_engine/test_suite_executor.cpp:131-145`.
- Why it matters:
  - Today, the most valuable truth-preserving statuses in LocusQ are expressed in bespoke scripts and markdown packets instead of in a first-class shared result model.
  - That forces humans to read prose to understand whether a lane is “code green but env blocked,” “analysis green but promotion blocked,” or “manual host execution pending.”
- Classification:
  - `upstream-harness opportunity` + `governance`
- Smallest high-leverage fix direction:
  - Extend harness result schemas to support gate classes such as `analysis_pass_promotion_blocked`, `inventory_only`, `environment_blocked`, and `manual_host_execution_pending`.

### [MEDIUM] F8 — CI harness checkout/auth is still duplicated and inconsistent across repos

- Evidence:
  - LocusQ duplicates “Require QA harness access token” and “Checkout QA harness” blocks in `.github/workflows/qa_harness.yml:227-243` and `.github/workflows/qa_harness.yml:524-536`.
  - The open BL-086 runbook already describes this as a four-repo problem in `Documentation/backlog/bl-086-ci-checkout-composite-action.md:11`, `Documentation/backlog/bl-086-ci-checkout-composite-action.md:19`, and `Documentation/backlog/bl-086-ci-checkout-composite-action.md:49`.
  - Cross-repo comparison shows the inconsistency is real:
    - `echoform/.github/workflows/qa_full.yml:16-20`
    - `memory-echoes/.github/workflows/qa_full.yml:24-28`
    - `monument-reverb/.github/workflows/qa_harness.yml:16-19`
    - `monument-reverb/.github/workflows/qa_harness.yml:70-74`
- Why it matters:
  - This is high-churn YAML with auth failure modes that are subtle and repo-specific. It is exactly the kind of repeated operational logic that a harness-owned composite action should absorb.
- Classification:
  - `upstream-harness / CI`
- Smallest high-leverage fix direction:
  - Execute BL-086 next and move the token validation + checkout block into a shared composite action with a single error contract.

### [MEDIUM] F9 — Production observability still depends on always-on file appends and grep-driven evidence

- Evidence:
  - `Source/editor_webview/EditorWebViewRuntime.h:537-542` and `Source/editor_webview/EditorWebViewRuntime.h:744-746` append to `resource_requests.log` on every request and result.
  - Historical validation evidence directly greps that file in `TestEvidence/validation-trend.md:204`.
- Why it matters:
  - This is not “debug mode”; it is production behavior being used as an ad hoc evidence source.
  - The harness should prefer explicit, structured debug/evidence channels over silent always-on text logs that later need `rg` to become machine-readable.
- Classification:
  - `observability gap` + `missing structured debug contract`
- Smallest high-leverage fix direction:
  - Replace always-on append logging with debug-gated structured traces that are turned on explicitly by selftests and exported in a stable JSONL/TSV contract.

### [LOW] F10 — The harness migration guide is already stale relative to the integration module

- Evidence:
  - `audio-dsp-qa-harness/cmake/MIGRATION.md:56-60` says consumers should still link `qa_runner_app` explicitly.
  - `audio-dsp-qa-harness/cmake/qa_harness_integration.cmake:122-154` already links `qa_runner_app` when it exists.
- Why it matters:
  - This is low severity, but it is exactly the sort of doc drift that slows adoption and causes teams to cargo-cult outdated integration steps.
- Classification:
  - `upstream-harness docs`
- Smallest high-leverage fix direction:
  - Update the migration guide to match the actual behavior of `enable_qa_harness()`.

## Harness Leverage Map

- Already in the right direction:
  - `qa/main.cpp:9-12` is now the thin consumer entrypoint BL-082 was meant to unlock.
  - `audio-dsp-qa-harness/scenario_engine/scenario_executor.cpp:151-163` already emits useful `runtime_config` no-op warnings.
  - `audio-dsp-qa-harness/scenario_engine/invariant_evaluator.cpp:91-125` and `audio-dsp-qa-harness/scenario_engine/test_suite_executor.cpp:131-145` already model profiling preconditions as provenance plus blocking behavior instead of silent false-green.
  - `audio-dsp-qa-harness/cmake/qa_harness_integration.cmake:62-166` proves the harness can now own meaningful consumer-build integration.
- Still misplaced in LocusQ:
  - GUI/standalone selftest orchestration in `scripts/standalone-ui-selftest-production-p0-mac.sh:33-260`
  - install/update/sync verification in `scripts/build-and-install-mac.sh:237-362` and `scripts/sync-companion-app-mac.sh:38-83`
  - host inventory + blocked taxonomy in `scripts/qa-bl067-auv3-lifecycle-mac.sh:163-190`
  - finite-output lane scaffolding patterns in `scripts/qa-bl078-runtime-finite-output-mac.sh:122-260`
  - companion/plugin protocol conformance checking across `main.swift`, `HeadTrackingBridge.h`, and `PosePacketTests.swift`
- Cross-repo leverage signal:
  - Echoform and Memory Echoes already standardize on strict `SUBMODULE_TOKEN` checkout blocks in `echoform/.github/workflows/qa_full.yml:16-20` and `memory-echoes/.github/workflows/qa_full.yml:24-28`.
  - Monument Reverb still carries a different fallback rule and older integration shape in `monument-reverb/.github/workflows/qa_harness.yml:16-19`, `monument-reverb/.github/workflows/qa_harness.yml:70-74`, and `monument-reverb/CMakeLists.txt:1431-1443`.
  - That combination is a strong sign the next harness wins should be CI composite actions, install manifests, and app selftest extraction, not more repo-local glue.

## Observability And Debug-Mode Gaps

- LocusQ already has one good harness-side pattern: `runtime_config` and `profiling_precondition` are explicit provenance objects, not implicit console-only behavior. That should become the model for other domains.
- WebView resource observability is currently always-on and file-based rather than debug-scoped and structured (`Source/editor_webview/EditorWebViewRuntime.h:537-542`, `Source/editor_webview/EditorWebViewRuntime.h:744-746`).
- Head-tracking runtime observability exposes low-level counters (`Source/HeadTrackingBridge.h:99-110`) but not a shared structured trace for seq restarts, stale-window decisions, ack timing, or packet-shape mismatches.
- Install/update observability is fragmented: plugin install verification lives in one script, companion sync prints console stats, and there is no shared end-to-end install state artifact.
- Crash triage still relies on repo-specific evidence packets rather than a harness-owned reproduction/bisect helper, even though BL-079 proved the value of classifying “unrelated current-HEAD defect” separately from the feature under review.

## Automated Test Matrix Gaps

- Companion package tests are currently blocked at compile time by `main.swift:3048-3051`, so the most basic unit/build smoke is not dependable.
- There is no shared packet golden-test suite spanning companion sender, plugin bridge, and any loopback receiver.
- Host inventory is honestly captured in BL-067, but host execution remains manual/inventory-only for Apple hosts; that means the evidence is truthful but not yet autonomous.
- Standalone UI selftest is strong in functionality but remains LocusQ-specific infrastructure rather than a reusable harness lane.
- Install/update regression coverage is missing across the combined product set. There is no one command that proves “plugin bundles, standalone app, and companion app in `/Applications` all match the just-built artifacts.”
- Crash isolation and first-bad-surface tooling are still ad hoc. BL-079’s triage packet is useful evidence, but the harness does not yet own a standard repro/bisect/regression classification flow.

## Benchmark And Metric Truthfulness Risks

- The largest current truthfulness problem is still the synthetic headphone verification snapshot in `Source/PluginProcessor.cpp:1199-1266`.
- BL-060 shows a better decision model: “analysis PASS” and “promotion-ready FAIL” can both be true at once, and the evidence should say that explicitly (`LocusQ-bl060-integrate/TestEvidence/bl060_blocked_sync_20260317T191327Z/lane_notes.md:29-40`).
- BL-084 is a positive counterexample: the harness now records profiling preconditions explicitly and blocks false-green perf results with provenance-backed `skip`/`fail` semantics (`audio-dsp-qa-harness/scenario_engine/invariant_evaluator.cpp:91-125`, `audio-dsp-qa-harness/scenario_engine/test_suite_executor.cpp:131-145`).
- The next benchmark-truthfulness step should be to extend that same policy discipline beyond perf invariants into perceptual evidence, install state, host inventory, and runtime verification surfaces.

## CI / Install / Reproducibility Gaps

- LocusQ’s harness checkout/auth YAML is still duplicated inside a single workflow (`.github/workflows/qa_harness.yml:227-243`, `.github/workflows/qa_harness.yml:524-536`).
- Cross-repo token behavior is already divergent:
  - strict secret in Echoform
  - strict secret in Memory Echoes
  - secret-or-`github.token` fallback in Monument Reverb
- The new `enable_qa_harness()` module is a big reproducibility improvement, but the migration docs lag the implementation (`audio-dsp-qa-harness/cmake/MIGRATION.md:56-60` vs `audio-dsp-qa-harness/cmake/qa_harness_integration.cmake:122-154`).
- In this session’s local environment, unattended build/install did not work cleanly until `scripts/build-and-install-mac.sh` was hardened for canonical sibling harness resolution and `CMAKE_POLICY_VERSION_MINIMUM`. That is a practical sign that install/repro flows still need more shared infrastructure, not just repo-local scripting.

## Companion-Specific QA Opportunities

- Add a required companion compile-smoke/package-test lane so `swift test` failures are treated as first-class validation blockers rather than discovered late.
- Move packet contract tests out of one repo’s unit tests and into shared fixtures consumed by the companion sender and plugin bridge.
- Add a machine-driven loopback E2E lane:
  - launch plugin or standalone with a deterministic synthetic sender
  - verify packet ingest, seq monotonicity, stale-window behavior, and ack cadence
  - export a single evidence packet
- Add default-threshold fallback coverage for ear-photo matching. Current fallback coverage in `companion/Tests/LocusQHeadTrackerTests/EarPhotoMatcherTests.swift:42-63` forces a custom `fallbackSimilarity: 0.95`, which is useful but does not prove default-threshold behavior.
- Add install/sync verification for the companion app bundle, not just the backend binary copied into an already-existing app shell.

## Proposed Backlog Items

- `BL-087 App Selftest Runner Extraction`
  - Owner: `upstream-harness`
  - Why it belongs there: `scripts/standalone-ui-selftest-production-p0-mac.sh:33-260` is reusable orchestration infrastructure, not LocusQ-specific product logic.
  - Expected leverage: shared GUI/standalone smoke runner for LocusQ, companion apps, and future APC standalone tooling.
  - Likely validation path: harness unit tests for lock/retry/result-settle behavior plus one consumer smoke in LocusQ.

- `BL-088 Bundle Install Manifest And Sync Verifier`
  - Owner: `upstream-harness` with thin downstream adapters
  - Why it belongs there: install truth currently spans `scripts/build-and-install-mac.sh` and `scripts/sync-companion-app-mac.sh` with no shared artifact contract.
  - Expected leverage: one-command proof that built artifacts and installed app/plugin bundles match across repos.
  - Likely validation path: build + install + hash-manifest replay on at least one plugin repo and one companion app.

- `BL-089 Head-Tracking Protocol Conformance Kit`
  - Owner: `upstream-harness`
  - Why it belongs there: packet contract drift is a producer/consumer protocol problem that should not be solved separately in each repo.
  - Expected leverage: reusable golden fixtures, serializer/deserializer property tests, and loopback packet QA for every product that speaks APC pose packets.
  - Likely validation path: companion sender tests, plugin bridge tests, and a loopback integration lane that uses shared fixtures unchanged.

- `BL-090 Blocked-Validation Evidence Contract`
  - Owner: `upstream-harness`
  - Why it belongs there: BL-060 and BL-067 are already proving that truthful QA needs more than binary PASS/FAIL.
  - Expected leverage: shared machine-readable statuses for `inventory_only`, `environment_blocked`, `analysis_pass_promotion_blocked`, and related gate states.
  - Likely validation path: harness schema tests plus migration of one perceptual lane and one host-validation lane.

- `BL-086 CI Checkout Composite Action`
  - Owner: `upstream-harness`
  - Why it belongs there: checkout/auth drift is already affecting at least four repos and is explicitly captured in the existing BL-086 runbook.
  - Expected leverage: one source of truth for private harness checkout, token validation, and path output.
  - Likely validation path: adopt the composite action in LocusQ first, then in one sibling repo with a different current token policy.

- `BL-091 Companion Compile-Smoke And Asset Template Hardening`
  - Owner: `downstream-LocusQ companion`
  - Why it belongs there: the current `swift test` failure is specific to the companion’s embedded asset code path in `main.swift:3048-3051`.
  - Expected leverage: restores a reliable unit/build smoke gate for every companion change and reduces late discovery of templating breakage.
  - Likely validation path: `swift test`, release build, and one synthetic-send smoke.

- `BL-092 Measured Verification Contract For Headphone Diagnostics`
  - Owner: `downstream-LocusQ runtime` with harness-backed evidence contract
  - Why it belongs there: the synthetic snapshot fields live in the product runtime today, but the promotion-grade evidence should be harness-owned and measured.
  - Expected leverage: removes a major false-green governance risk and clarifies which values are heuristics vs measured outputs.
  - Likely validation path: measured invariant packet from harness, UI/runtime label split, and promotion evidence that no longer relies on synthetic “verification” scores.

## Quick Wins Vs Deep Bets

- Quick wins:
  - Execute BL-086 and remove the duplicated checkout/auth YAML.
  - Fix the companion compile break at `main.swift:3048-3051`.
  - Update `audio-dsp-qa-harness/cmake/MIGRATION.md:56-60` to match the actual integration module.
  - Debug-gate or restructure `resource_requests.log` usage so validation no longer depends on always-on append logging.
- Deep bets:
  - Extract the standalone/app selftest runtime into the harness.
  - Add the install-manifest + sync-verifier contract.
  - Add the head-tracking protocol conformance kit.
  - Add a first-class blocked-validation evidence model.
  - Replace heuristic runtime verification fields with measured harness evidence where promotion decisions depend on them.

## Command Log

- `cd /Users/artbox/Documents/Repos/LocusQ/companion && swift test`
  - Result: `FAIL`
  - What it proved: the companion’s default package-test entrypoint is currently broken by invalid escape sequences in `main.swift:3048-3051`.
- `cd /Users/artbox/Documents/Repos/LocusQ && ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
  - Result: `PASS`
  - What it proved: the current local QA runner binary and the canonical smoke suite are runnable in this environment.
- `ctest --test-dir /Users/artbox/Documents/Repos/audio-dsp-qa-harness/build_bl084 --output-on-failure -R 'performance_invariant_test|test_suite_test|qa_runner_app_test'`
  - Result: `PASS`
  - What it proved: the reviewed harness-side runner-app, suite, and profiling-precondition regression surfaces are green in the existing upstream build tree.

## Validation Status

- `partially tested`

## Residual Uncertainty

- I did not rerun the heavy AUv3 lifecycle lane, standalone UI selftest lane, or perceptual listening lanes during this review turn.
- Some review conclusions rely on source inspection plus previously captured evidence packets rather than fresh full-lane replays.
- The companion install/sync observations were strengthened by earlier work in this session, but this review itself intentionally limited new execution to a small targeted command set.
