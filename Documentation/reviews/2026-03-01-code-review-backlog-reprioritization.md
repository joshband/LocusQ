Title: LocusQ Code Review and Backlog Reprioritization
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# LocusQ Code Review and Backlog Reprioritization

## Purpose

Provide a code-first, backlog-aligned review that can be used immediately to:

1. Create new backlog issues.
2. Reprioritize existing BL lanes.
3. Tighten promotion gates to prevent false-green outcomes.

## Scope Reviewed

- Core runtime and DSP surfaces:
  - `Source/PluginProcessor.cpp`
  - `Source/SpatialRenderer.h`
  - `Source/SceneGraph.h`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`
  - `Source/processor_core/ProcessorParameterReaders.h`
- Headtracking and calibration surfaces:
  - `Source/HeadTrackingBridge.h`
  - `Source/HeadPoseInterpolator.h`
  - `Source/CalibrationEngine.h`
  - `Source/PluginEditor.cpp`
  - `Source/processor_bridge/ProcessorUiBridgeOps.h`
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift`
  - `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift`
- WebView runtime, UI logic, and QA lanes:
  - `Source/editor_webview/EditorWebViewRuntime.h`
  - `Source/ui/public/js/index.js`
  - `Source/ui/public/index.html`
  - `scripts/qa-bl067-auv3-lifecycle-mac.sh`
  - `scripts/qa-bl068-temporal-effects-mac.sh`
  - `qa/scenarios/*.json`
- Backlog authority for alignment:
  - `Documentation/backlog/index.md`
  - Open runbooks: `BL-050`, `BL-053`..`BL-061`, `BL-067`, `BL-068`

## Validation Status

- `partially tested`
  - Static code audit across the above modules.
  - Script execution in audit context for BL-067/BL-068 scaffold QA lanes.
- Not all findings are runtime-reproduced; many are deterministic code-contract risks.

## Findings (Ordered by Severity)

### Critical

1. **RT-unsafe preset loading in audio callback with repeated retry behavior on failure**
   - Evidence:
     - `Source/PluginProcessor.cpp:2049`
     - `Source/PluginProcessor.cpp:2318`
     - `Source/SpatialRenderer.h:644`
     - `Source/SpatialRenderer.h:679`
     - `Source/SpatialRenderer.h:682`
     - `Source/SpatialRenderer.h:690`
   - Risk:
     - Filesystem/config parsing can run inside `processBlock` path.
     - Missing/invalid presets can retrigger load attempts every block, risking sustained dropouts.
   - Backlog alignment:
     - Directly impacts `BL-050`, `BL-055`, `BL-068`.
   - Recommendation:
     - Move preset load/parse to non-RT lane, atomically swap prepared coeffs, and add failure backoff cache.

### High

2. **Torn audio snapshot reads (`ptr` and `numSamples` not captured atomically as one snapshot)**
   - Evidence:
     - `Source/SceneGraph.h:135`
     - `Source/SceneGraph.h:141`
     - `Source/SpatialRenderer.h:1418`
     - `Source/SpatialRenderer.h:1419`
     - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1082`
     - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1083`
   - Risk:
     - Consumer can read pointer from one buffer and sample count from another during concurrent publish.
   - Backlog alignment:
     - `BL-037`, `BL-050`.
   - Recommendation:
     - Add single coherent snapshot API (`readAudioSnapshot`) or seqlock-style read.

3. **Telemetry data races between audio thread writes and scene-state reads**
   - Evidence:
     - `Source/PluginProcessor.cpp:2006`
     - `Source/PluginProcessor.cpp:2027`
     - `Source/PluginProcessor.cpp:2095`
     - `Source/PluginProcessor.cpp:2132`
     - `Source/PluginProcessor.cpp:2287`
     - `Source/PluginProcessor.cpp:3086`
     - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1128`
     - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1607`
   - Risk:
     - Unsynchronized float/double cross-thread reads are undefined behavior and can corrupt diagnostics.
   - Backlog alignment:
     - `BL-050`, `BL-068`.
   - Recommendation:
     - Publish telemetry snapshots atomically with sequence-consistent handoff.

4. **Calibration abort/restart race can let previous analysis complete into a new run**
   - Evidence:
     - `Source/CalibrationEngine.h:161`
     - `Source/CalibrationEngine.h:186`
     - `Source/CalibrationEngine.h:366`
     - `Source/CalibrationEngine.h:379`
     - `Source/CalibrationEngine.h:428`
   - Risk:
     - Old generation results can bleed into new calibration run when abort is reset early.
   - Backlog alignment:
     - `BL-056`, `BL-059`, `BL-060`.
   - Recommendation:
     - Introduce run-generation token + explicit `analysisInFlight` gate before accepting restart.

5. **Calibration can report `Complete` despite invalid/failed analysis**
   - Evidence:
     - `Source/CalibrationEngine.h:62`
     - `Source/CalibrationEngine.h:386`
     - `Source/CalibrationEngine.h:390`
     - `Source/CalibrationEngine.h:433`
     - `Source/CalibrationEngine.h:445`
   - Risk:
     - Bad calibration profiles can be treated as successful and propagate to later phases.
   - Backlog alignment:
     - `BL-059`, `BL-060`.
   - Recommendation:
     - Enforce per-speaker validity gates, use `State::Error`, and publish failure diagnostics contractually.

6. **Cross-thread UB on calibration progress/profile state fields**
   - Evidence:
     - `Source/CalibrationEngine.h:261`
     - `Source/CalibrationEngine.h:273`
     - `Source/CalibrationEngine.h:297`
     - `Source/CalibrationEngine.h:399`
     - `Source/TestSignalGenerator.h:94`
     - `Source/IRCapture.h:87`
   - Risk:
     - Non-atomic mutable state read/written across UI/audio/analysis threads.
   - Backlog alignment:
     - `BL-059`, `BL-060`.
   - Recommendation:
     - Publish immutable snapshots atomically and avoid exposing mutable shared references.

7. **BL-067 and BL-068 QA scaffolds can appear PASS while execute rows remain TODO (false-green governance risk)**
   - Evidence:
     - `scripts/qa-bl067-auv3-lifecycle-mac.sh:153`
     - `scripts/qa-bl067-auv3-lifecycle-mac.sh:161`
     - `scripts/qa-bl068-temporal-effects-mac.sh:146`
     - `scripts/qa-bl068-temporal-effects-mac.sh:156`
     - `Documentation/backlog/index.md:126`
     - `Documentation/backlog/index.md:127`
   - Risk:
     - Promotion decisions can be made on scaffold evidence that has no runtime execution coverage.
   - Backlog alignment:
     - `BL-067`, `BL-068`.
   - Recommendation:
     - Split `--contract-only` vs `--execute` modes and fail execute mode if any row is TODO.

8. **UI self-test fallback mutation paths can mask real gesture-path failures**
   - Evidence:
     - `Source/ui/public/js/index.js:6689`
     - `Source/ui/public/js/index.js:6700`
     - `Source/ui/public/js/index.js:6743`
     - `Source/ui/public/js/index.js:6770`
     - `Source/ui/public/js/index.js:6793`
     - `Source/ui/public/js/index.js:6804`
   - Risk:
     - Interaction regressions can pass CI due to internal state mutation fallback.
   - Backlog alignment:
     - `BL-040` follow-through, `BL-067`, `BL-068` QA quality gates.
   - Recommendation:
     - Add strict-gesture CI mode that hard-fails when fallback path is used.

### Medium

9. **Host-notifying parameter writes may run in process callback path**
   - Evidence:
     - `Source/PluginProcessor.cpp:1757`
     - `Source/PluginProcessor.cpp:1965`
     - `Source/processor_core/ProcessorParameterReaders.h:47`
     - `Source/processor_core/ProcessorParameterReaders.h:52`
   - Risk:
     - `setValueNotifyingHost` on RT path can lock/reenter host.
   - Backlog alignment:
     - `BL-041`, `BL-068`.
   - Recommendation:
     - Defer host notifications to non-RT lane.

10. **Head pose interpolation API/doc clock-domain mismatch can collapse prediction effectiveness**
   - Evidence:
     - `Source/HeadPoseInterpolator.h:14`
     - `Source/HeadPoseInterpolator.h:67`
     - `Source/HeadPoseInterpolator.h:75`
     - `Source/HeadPoseInterpolator.h:83`
   - Risk:
     - Comparing packet epoch timestamps with monotonic clock can force degenerate interpolation.
   - Backlog alignment:
     - `BL-053`, `BL-058`.
   - Recommendation:
     - Unify clock domain and add impossible-delta assertions.

11. **HeadTrackingBridge pose handoff uses raw pointer with limited slot versioning protection**
   - Evidence:
     - `Source/HeadTrackingBridge.h:187`
     - `Source/HeadTrackingBridge.h:339`
     - `Source/HeadTrackingBridge.h:344`
   - Risk:
     - Bursty updates can race reader visibility semantics for pointed slot contents.
   - Backlog alignment:
     - `BL-053`, `BL-059`.
   - Recommendation:
     - Return value-copy snapshot with sequence/version verification.

12. **Companion executable protocol drift: runtime emits v1 while core model/tests include v2 fields**
   - Evidence:
     - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:217`
     - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:3020`
     - `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift:5`
   - Risk:
     - Runtime loses ang-velocity/sensor-location semantics and can diverge from test confidence.
   - Backlog alignment:
     - `BL-053`, `BL-058`, `BL-059`.
   - Recommendation:
     - Align executable with v2 packet model or explicitly dual-contract test both versions.

13. **Synthetic companion mode advertises `--require-sync` but can still transmit immediately**
   - Evidence:
     - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:334`
     - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:2706`
     - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:2718`
   - Risk:
     - QA can miss readiness/send-gate regressions.
   - Backlog alignment:
     - `BL-058`.
   - Recommendation:
     - Mirror live gating behavior in synthetic path.

14. **Always-on resource-request logging does synchronous append in runtime path**
   - Evidence:
     - `Source/editor_webview/EditorWebViewRuntime.h:386`
     - `Source/editor_webview/EditorWebViewRuntime.h:390`
     - `Source/editor_webview/EditorWebViewRuntime.h:597`
   - Risk:
     - UI-thread jitter and unbounded log growth.
   - Backlog alignment:
     - `BL-067`/`BL-068` infra quality, docs hygiene follow-up.
   - Recommendation:
     - Gate logging behind debug flag and add rotation/size cap.

15. **Timeline native-call failures are swallowed in multiple paths**
   - Evidence:
     - `Source/ui/public/js/index.js:873`
     - `Source/ui/public/js/index.js:888`
     - `Source/ui/public/js/index.js:2200`
     - `Source/ui/public/js/index.js:2518`
     - `Source/ui/public/js/index.js:6960`
   - Risk:
     - UI/native drift can go invisible to operator and CI.
   - Backlog alignment:
     - `BL-040` residual UX reliability, `BL-067`/`BL-068` gate confidence.
   - Recommendation:
     - Centralize reporting and expose operator-visible diagnostics counter.

16. **Critical startup binding failures only log and continue into partial runtime**
   - Evidence:
     - `Source/ui/public/js/index.js:5670`
     - `Source/ui/public/js/index.js:5677`
     - `Source/ui/public/js/index.js:5681`
   - Risk:
     - Controls appear available but are not correctly bound.
   - Backlog alignment:
     - `BL-040`, `BL-067` UX/runtime hardening.
   - Recommendation:
     - Introduce explicit degraded mode and disable dependent controls.

17. **BL-058 runbook requires evidence lanes that are not yet implemented**
   - Evidence:
     - `Documentation/backlog/bl-058-companion-profile-acquisition.md:60`
   - Risk:
     - Readiness/sync/axis/stale contracts are currently ungated.
   - Backlog alignment:
     - `BL-058`, `BL-059`.
   - Recommendation:
     - Add dedicated `qa-bl058-...` harness and required artifact bundle.

18. **BL-053/BL-059 automation is mostly structural/smoke and under-covers runtime freshness/race contracts**
   - Evidence:
     - `scripts/qa-bl053-head-tracking-orientation-injection-mac.sh:45`
     - `Documentation/testing/bl-053-head-tracking-orientation-injection-qa.md:28`
     - `Documentation/testing/bl-053-head-tracking-orientation-injection-qa.md:42`
     - `scripts/qa-bl059-calibration-integration-smoke-mac.sh:37`
   - Risk:
     - Acceptance can pass while race/freshness regressions remain latent.
   - Backlog alignment:
     - `BL-053`, `BL-059`, `BL-060`, `BL-061`.
   - Recommendation:
     - Add explicit runtime assertions for stale fallback, seq continuity, abort/restart race, profile handoff integrity.

### Low

19. **Out-of-range diagnostics path is neutralized by pre-validation clamping**
   - Evidence:
     - `Source/processor_bridge/ProcessorUiBridgeOps.h:477`
     - `Source/processor_bridge/ProcessorUiBridgeOps.h:478`
   - Risk:
     - Invalid input visibility is reduced in diagnostics.
   - Backlog alignment:
     - `BL-059`.
   - Recommendation:
     - Validate raw input before clamp and publish out-of-range flag.

20. **Scaffold script fail-counter composition can become inconsistent**
   - Evidence:
     - `scripts/qa-bl067-auv3-lifecycle-mac.sh:98`
     - `scripts/qa-bl067-auv3-lifecycle-mac.sh:164`
     - `scripts/qa-bl068-temporal-effects-mac.sh:98`
     - `scripts/qa-bl068-temporal-effects-mac.sh:159`
   - Risk:
     - Reported fail count can diverge from lane detail payloads.
   - Backlog alignment:
     - `BL-067`, `BL-068`.
   - Recommendation:
     - Compute lane summary counters separately from final lane_result status.

21. **Scenario metadata freshness drift (done lanes still tagged scaffold)**
   - Evidence:
     - `qa/scenarios/locusq_bl044_quality_tier_switch_suite.json:6`
     - `Documentation/backlog/index.md:103`
     - `qa/scenarios/locusq_bl036_finite_output_suite.json:6`
     - `qa/scenarios/locusq_bl037_snapshot_budget_suite.json:6`
     - `Documentation/backlog/index.md:96`
     - `Documentation/backlog/index.md:97`
   - Risk:
     - Governance dashboards and lane readiness interpretation become noisy.
   - Backlog alignment:
     - docs hygiene follow-up (non-functional but high leverage).
   - Recommendation:
     - Run metadata normalization pass for scenario descriptions.

## Backlog Reprioritization Recommendations

### Recommended Priority Changes (Current -> Proposed)

1. `BL-059`: `P1` -> `P0`
   - Reason: calibration correctness and thread-safety faults can produce invalid profiles with apparently successful status.
2. `BL-058`: `P1` -> `P0`
   - Reason: protocol/version drift, missing QA lane, and readiness-gate blind spots directly impact headtracking reliability claims.
3. `BL-050`: `P2` -> `P0`
   - Reason: RT path hazards (file I/O/data races/torn snapshots) are release-class reliability risks.
4. `BL-068`: `P2` -> `P1`
   - Reason: temporal-effects lane depends on same RT-safety foundations and currently has scaffold-only QA semantics.
5. `BL-067`: keep `P1`, but set explicit promotion blocker: no TODO rows in execute evidence.

### Recommended Status/Gate Adjustments

1. Keep `BL-053` in validation, but block promotion until:
   - clock-domain consistency,
   - pose snapshot handoff hardening,
   - stale/sequence runtime assertions.
2. Keep `BL-060` blocked behind hardened `BL-059` (calibration validity + race closure).
3. Keep `BL-061` conditional; require objective gain evidence from hardened `BL-060`.

## New Intake-Ready Backlog Issues (Proposed)

1. **BL-069: RT-Safe Headphone Preset Pipeline and Failure Backoff**
   - Priority: `P0`
   - Depends on: `BL-050`
   - Acceptance highlights:
     - No filesystem/parsing in `processBlock`.
     - Missing preset does not retry every callback.
     - Atomic coeff swap contract + regression coverage.

2. **BL-070: Coherent Audio Snapshot + Telemetry Seqlock Contract**
   - Priority: `P0`
   - Depends on: `BL-050`
   - Acceptance highlights:
     - Single coherent `{ptr,numSamples}` snapshot API.
     - Atomic telemetry publish/read with sequence consistency.
     - TSAN/polling stress lane in CI or scheduled gate.

3. **BL-071: Calibration Generation Guard + Error-State Enforcement**
   - Priority: `P0`
   - Depends on: `BL-056`, `BL-059`
   - Acceptance highlights:
     - Abort/restart generation isolation.
     - `State::Error` exercised on invalid analysis.
     - No data races in progress/profile publication.

4. **BL-072: Companion Runtime Protocol Parity + BL-058 QA Harness**
   - Priority: `P0`
   - Depends on: `BL-058`, `BL-059`
   - Acceptance highlights:
     - Runtime packet contract parity (v2 or explicit dual-version gates).
     - `--require-sync` semantics parity in synthetic/live modes.
     - New `qa-bl058-*` evidence bundle (`status.tsv`, `results.tsv`, axis/readiness artifacts).

5. **BL-073: QA Scaffold Truthfulness Gates (BL-067/BL-068)**
   - Priority: `P1`
   - Depends on: `BL-067`, `BL-068`
   - Acceptance highlights:
     - `--contract-only` and `--execute` mode split.
     - Execute mode fails on TODO rows.
     - Promotion checklist requires non-TODO runtime matrix rows.

6. **BL-074: WebView Runtime Reliability Diagnostics (Strict Gesture + Degraded Mode)**
   - Priority: `P1`
   - Depends on: `BL-040` follow-up, `BL-067`
   - Acceptance highlights:
     - Strict gesture CI mode with fallback-as-failure.
     - Startup binding failure enters explicit degraded mode.
     - Native-call failures surfaced in diagnostics chip/counter.

## Feature and Product Hardening Opportunities

1. **Deterministic diagnostics surface in UI**
   - Add unified runtime diagnostics panel for binding failures, native call errors, sync gate state, and stale packet counters.
2. **Headtracking quality profile modes**
   - Add explicit runtime profile toggle for prediction/smoothing strategy (baseline vs adaptive), with artifact-backed promotion.
3. **Calibration confidence envelope**
   - Publish objective confidence metadata (per-speaker validity, fit residual metrics) with profile artifacts to support BL-060 decisions.

## Notable Strengths

1. Strong finite-check and sanitization posture in core rendering math.
2. Lock-free slot-state mechanics and registration diagnostics are conceptually solid and instrumented.
3. Backlog authority is mature and provides a clean path for deterministic promotion governance.

## Suggested Execution Sequence

1. Reprioritize `BL-050`, `BL-058`, `BL-059` immediately.
2. Open and schedule `BL-069`..`BL-072` as first-wave defect containment.
3. Gate `BL-067`/`BL-068` with `BL-073` so future evidence is execution-truthful.
4. Fold `BL-074` into UI quality gate hardening before next major UX promotion cycle.

## Closeout Notes

- This report is intended as intake-ready material for issue creation and backlog resequencing.
- Findings are prioritized for risk containment first, then feature enablement.
