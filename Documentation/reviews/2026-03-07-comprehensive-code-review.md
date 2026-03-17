Title: LocusQ Comprehensive Code Review
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-07
Last Modified Date: 2026-03-07

# LocusQ Comprehensive Code Review

## Purpose

Provide an up-to-date, findings-first review across code, config, architecture, backlog/runbook/plans/reviews surfaces, and release/QA automation so the repo can prioritize real correctness risks over already-green but incomplete gates.

## Skills Used (Applied as Review Lenses)

| Stage | Skills | Why |
|---|---|---|
| Scope + governance | `documentation-hygiene-expert`, `skill_docs` | Verify doc authority, tier discipline, backlog/runbook consistency, and review artifact quality. |
| WebView/UI runtime | `juce-webview-runtime`, `threejs`, `reactive-av`, `realtime-dimensional-visualization` | Audit WebView bridge contracts, JS packaging/runtime behavior, companion visualization/runtime assumptions, and UI gate coverage. |
| Spatial/headtracking/DSP | `spatial-audio-engineering`, `steam-audio-capi`, `hrtf-rendering-validation-lab`, `temporal-effects-engineering`, `simulation-behavior-audio-visual`, `physics-reactive-audio`, `headtracking-companion-runtime`, `apple-spatial-companion-platform` | Audit head-pose transport, binaural/rendering correctness, FIR/SOFA wiring, companion runtime contracts, and realtime behavior. |
| Formats/automation | `clap-plugin-lifecycle`, `auv3-plugin-lifecycle`, `skill_testing`, `skill_troubleshooting` | Audit CI/release gates, format-lane truthfulness, and false-green risks in closeout scripts. |

## Scope Reviewed

- Core code and config across `Source/`, `companion/`, `scripts/`, `.github/workflows/`, and `CMakeLists.txt`
- Architecture surfaces:
  - `ARCHITECTURE.md`
  - `Documentation/architecture-code-review-2026-03-06.md`
- Governance and backlog surfaces:
  - `Documentation/backlog-post-v1-agentic-sprints.md`
  - `Documentation/reviews/*`
  - `Documentation/plans/*`
  - `Documentation/runbooks/*`
  - `Documentation/backlog/***`

## Validation Status

- `partially tested`
- Static audit plus targeted local validation:
  - `./scripts/validate-docs-freshness.sh` -> `PASS`
  - `./scripts/validate-backlog-plain-language.sh` -> `PASS`
  - `python3 scripts/validate-backlog-redundancy.py` -> `PASS`
  - `python3 scripts/export-backlog-summaries.py --check` -> `PASS`
  - `cd companion && swift test` -> `PASS` (3 tests)
- No full plugin rebuild, WebView self-test, REAPER probe, or end-to-end format release lane was rerun during this review.

## Findings (Ordered by Severity)

### High

1. **Head-pose interpolation mixes epoch time with monotonic uptime, so interpolation and prediction are effectively broken**
- Evidence:
  - `companion/Sources/LocusQHeadTrackerCore/MotionService.swift:85-105` stamps outgoing packets with epoch milliseconds.
  - `Source/PluginProcessor.cpp:1639-1643` and `Source/PluginProcessor.cpp:2220-2222` feed `HeadPoseInterpolator` with `juce::Time::getMillisecondCounterHiRes()`.
  - `Source/HeadPoseInterpolator.h:67-96` compares `nowMs` against packet `timestampMs` and only predicts when `nowMs > currTs + 1.0f`.
- Risk:
  - The interpolator is comparing incompatible clock domains.
  - That makes the blend factor degenerate and leaves the prediction path effectively dead.
  - Head-tracked rendering is therefore driven by the wrong temporal assumptions.
- Recommendation:
  - Unify the time base for both packet timestamps and interpolation time.
  - Add assertions/telemetry for impossible deltas so this cannot silently regress again.

2. **The main render path never stale-gates or clears head tracking, so disconnects can leave the last pose latched indefinitely**
- Evidence:
  - `Source/PluginProcessor.cpp:1639-1662` applies any `currentPose()` directly into `SpatialRenderer`.
  - `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp:143-171` only normalizes/stores the pose and sets `headPoseValid = true`.
  - `Source/PluginProcessor.cpp:483-491` already computes pose age/staleness for telemetry.
  - `Source/PluginProcessor.cpp:2172-2176` documents stale fallback in the calibration monitoring path, and `Source/PluginProcessor.cpp:2212-2238` actually enforces freshness there.
- Risk:
  - Normal head-tracked rendering and calibration monitoring have different freshness behavior.
  - A stopped/disconnected companion can freeze the renderer at the last valid orientation.
- Recommendation:
  - Apply the same freshness gate and explicit identity/reset behavior on the main render path.
  - Keep staleness handling in one canonical helper so telemetry and renderer behavior cannot drift.

3. **`CalibrationProfile.json` FIR/SOFA selections are not actually wired into the DSP/render path**
- Evidence:
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1245-1248` enables FIR mode without loading any impulse taps.
  - `Source/headphone_dsp/HeadphoneCalibrationChain.h:21-26` initializes the FIR hook to identity.
  - `Source/headphone_dsp/HeadphoneFirHook.h:101-143` exposes `loadImpulseResponse()`, but no repo call site loads profile taps into it.
  - `companion/Sources/LocusQHeadTrackerCore/CalibrationProfile.swift:30-45` stores `sofa_ref` under `user`.
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1256-1258` looks for `sofa_ref` under `headphone`.
  - `Source/spatial_renderer/SpatialSteamAudioBackend.cpp:273-300` still marks SOFA HRTF swap wiring as TODO.
- Risk:
  - FIR mode can be selected/enabled while still running the identity response.
  - Custom SOFA profiles can be written/exported without ever affecting the renderer.
  - UI/profile state can therefore claim personalization that the DSP path never applies.
- Recommendation:
  - Wire profile FIR taps into `HeadphoneFirHook::loadImpulseResponse()`.
  - Read `sofa_ref` from the actual schema location.
  - Close the Steam Audio SOFA handoff before treating custom SOFA as a supported surface.

4. **Companion packaging still points at a deleted local Three.js asset and silently falls back to a CDN**
- Evidence:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:1204-1229` searches for `Source/ui/public/js/three.min.js` and falls back to `https://unpkg.com/three@0.161.0/build/three.min.js`.
  - `scripts/sync-companion-app-mac.sh:64-67` still tries to copy that deleted `three.min.js` into the bundle.
  - `Documentation/architecture-code-review-2026-03-06.md:412-414` states the tracked blob was replaced by npm-managed `three@0.183.2`.
  - `Source/ui/package.json:5-10` pins `three` to `0.183.2`.
  - `Documentation/backlog/bl-058-companion-profile-acquisition.md:82-86` lists `privacy: no network calls` as acceptance.
- Risk:
  - The companion is no longer hermetic/offline-safe.
  - It can run a different Three.js version than the repo declares for the product UI.
  - It violates the documented privacy/runtime contract for BL-058-style flows.
- Recommendation:
  - Bundle the same pinned Three.js artifact used by the current UI toolchain, or remove the runtime dependency entirely.
  - Make offline/no-network behavior a hard startup invariant for the companion.

5. **`qa-bl011-clap-closeout-mac.sh` can report false-green closeout against stale local artifacts**
- Evidence:
  - `scripts/qa-bl011-clap-closeout-mac.sh:79-105` builds CLAP/QA/Standalone targets and then requires an already-installed `~/Library/Audio/Plug-Ins/CLAP/LocusQ.clap`.
  - `scripts/qa-bl011-clap-closeout-mac.sh:162-181` reuses the newest passing `UI-P2-011` self-test JSON from `TestEvidence/`.
  - `scripts/qa-bl011-clap-closeout-mac.sh:258-276` can also reuse the newest passing REAPER CLAP discovery artifact from `TestEvidence/`.
- Risk:
  - The script can pass without validating the build it just produced.
  - BL-011 closeout can therefore be green while install, self-test, or REAPER discovery for the current artifact is broken.
- Recommendation:
  - Install/probe the freshly built CLAP bundle in the same run.
  - Treat artifact reuse as advisory-only metadata, not closeout evidence.

6. **Backlog authority is still split across deprecated docs and active plan surfaces**
- Evidence:
  - `Documentation/backlog-post-v1-agentic-sprints.md:9` deprecates the file, but `Documentation/backlog-post-v1-agentic-sprints.md:17-29` still calls it the “single authority,” and `Documentation/backlog-post-v1-agentic-sprints.md:146-153` still requires synchronized updates there.
  - `Documentation/runbooks/backlog-execution-runbooks.md:9` deprecates the runbook bundle, but `Documentation/runbooks/backlog-execution-runbooks.md:20-31` still tells operators to execute from `Documentation/backlog-post-v1-agentic-sprints.md`.
  - `Documentation/README.md:12` and `Documentation/standards.md:48-53` make `Documentation/backlog/index.md` the sole backlog authority.
  - Tier 1 plan docs still point at the deprecated backlog, for example:
    - `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md:12-15`
    - `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md:12-15`
    - `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md:12-15`
    - `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md:12-14`
    - `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md:16-18`
    - `Documentation/plans/bl-031-tempo-locked-visual-token-scheduler-spec-2026-02-24.md:12-14`
- Risk:
  - A contributor following still-active docs can update the wrong ledger.
  - This is governance drift that current freshness gates do not catch.
- Recommendation:
  - Remove all “single authority” language from deprecated files.
  - Rewrite Tier 1 plan headers to point only at `Documentation/backlog/index.md` plus the relevant per-item runbook.

7. **Several active plans/reviews are no longer executable or auditable because they still anchor to a deleted UI source file**
- Evidence:
  - The working tree no longer contains `Source/ui/public/js/index.js`; current UI sources are `Source/ui/src/index.ts` (`Source/ui/src/index.ts:1-2`) plus bridge/runtime helpers such as `Source/ui/public/js/juce/index.js:35-62`.
  - `Documentation/architecture-code-review-2026-03-06.md:412-414` explicitly describes the migration to `src/index.ts` and generated bundle output.
  - Despite that, active docs still cite `Source/ui/public/js/index.js`, for example:
    - `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md:51-61`
    - `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md:178-240`
    - `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md:327-410`
    - `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md:33-36`
    - `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md:165-170`
    - `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md:256-272`
- Risk:
  - Readers cannot replay cited commands or inspect the referenced source evidence as written.
  - Tier 1/Tier 2 docs have drifted out of sync with the actual UI toolchain.
- Recommendation:
  - Update active plan/review references to the Vite/TypeScript layout.
  - Archive or annotate old source-path references where historical context is still useful.

### Medium

8. **Linux format parity is documented, but CI does not provision the required Linux WebView stack or build Linux plugin formats**
- Evidence:
  - `CMakeLists.txt:50-55` declares Linux formats `VST3 LV2 Standalone` with `NEEDS_WEB_BROWSER TRUE` and `WebKitGTK`.
  - `.github/workflows/qa_harness.yml:245-275` and `.github/workflows/qa_harness.yml:542-571` install X11/JACK/font dependencies but not WebKitGTK development packages.
  - Those same workflow sections only build `locusq_qa`, not `LocusQ_VST3`, `LocusQ_LV2`, or `LocusQ_Standalone`.
  - `ARCHITECTURE.md:403-407` still presents Linux `VST3, LV2, Standalone` as a format-matrix surface.
- Risk:
  - Linux parity is currently a documentation claim, not an enforced CI truth.
  - Fresh Linux configure/builds can still fail for missing WebKitGTK headers or target gaps.
- Recommendation:
  - Either add a real Linux format lane with WebKitGTK provisioning, or downgrade the architecture claim until such a lane exists.

9. **Release governance does not protect AUv3, and the optional CLAP release gate is not self-contained**
- Evidence:
  - `.github/workflows/release-governance.yml:43-58` builds only `LocusQ_Standalone`, `LocusQ_VST3`, and `LocusQ_AU`.
  - `.github/workflows/qa_harness.yml:132-193` already treats `LocusQ_AUv3` as a first-class QA lane.
  - `.github/workflows/release-governance.yml:136-162` requires `clap-info` and `clap-validator` to already exist on the runner when the CLAP gate is enabled.
- Risk:
  - Tag/release validation can miss AUv3 regressions that are otherwise considered supported.
  - The CLAP release gate will fail on a stock runner even when the repo itself is healthy.
- Recommendation:
  - Add AUv3 to release-governance coverage or explicitly scope it out of release claims.
  - Provision CLAP tooling inside the workflow instead of assuming ambient runner state.

10. **The primary UI PR gate validates a non-shipping Stage 12 surface and can fail for config reasons unrelated to the production UI**
- Evidence:
  - `scripts/ui-pr-gate-mac.sh:41-56` hard-codes `standalone-ui-selftest-stage12-mac.sh` as the primary gate.
  - `scripts/standalone-ui-selftest-stage12-mac.sh:40-44` only enables `LOCUSQ_UI_SELFTEST=1`.
  - `Source/editor_webview/EditorWebViewRuntime.h:66-77` maps plain self-test mode to `useIncrementalUi = true` unless a variant is forced.
  - `CMakeLists.txt:67-72` and `CMakeLists.txt:216-245` only embed incremental stage assets when `LOCUSQ_UI_POC=ON`.
- Risk:
  - The PR gate can miss regressions in the production UI.
  - It can also red-bar normal builds simply because incremental assets are not embedded.
- Recommendation:
  - Make the production UI self-test the primary PR gate.
  - Keep Stage 12 coverage as an explicit secondary/pre-merge lane if it still has value.

11. **Native bridge timeouts leak unresolved promises indefinitely**
- Evidence:
  - `Source/ui/public/js/juce/index.js:37-58` only removes promise entries when `__juce__complete` arrives.
  - `Source/ui/src/index.ts:690-713` adds app-layer timeouts.
  - `Source/ui/src/index.ts:759-794` records timeout/runtime failures but never cancels or purges the underlying bridge promise entry.
- Risk:
  - Any stalled native bridge call leaves retained closures/state in `promiseHandler.promises` for the lifetime of the page.
  - Repeated failures can accumulate memory and stale diagnostics state over long sessions.
- Recommendation:
  - Add explicit cancellation/cleanup on timeout and expose a bridge-level abort or tombstone path.

12. **The editor reparses malformed companion calibration JSON forever at 30 Hz**
- Evidence:
  - `Source/PluginEditor.cpp:75-86` calls `pollCompanionCalibrationProfileFromDisk()` every timer tick.
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1203-1218` returns early on parse/schema failure.
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1321` only updates `companionCalibrationProfileLastModifiedMs` on the success path.
- Risk:
  - A malformed or partially written file triggers repeated `exists/stat/load/parse` work on the message thread every ~33 ms.
  - The failure mode is therefore more expensive than the healthy case.
- Recommendation:
  - Cache failed `mtime` values with a bounded retry backoff, or switch to atomic write/rename semantics for the companion profile.

13. **The docs freshness gate advertises root-doc sync coverage that the script does not actually enforce**
- Evidence:
  - `.github/workflows/docs-freshness.yml:6-18` and `.github/workflows/docs-freshness.yml:24-36` trigger on `AGENTS.md`, `CODEX.md`, `CLAUDE.md`, `SKILLS.md`, and `AGENT_RULE.md`.
  - `scripts/validate-docs-freshness.sh:127-152` only checks `README.md`, `CHANGELOG.md`, `TestEvidence/build-summary.md`, and `TestEvidence/validation-trend.md` against `status.json`.
- Risk:
  - Routing/governance docs can drift while the official freshness workflow still reports green.
- Recommendation:
  - Either enforce those root-doc contracts in the script or narrow the workflow claim so the gate matches what it really validates.

14. **`Documentation/architecture-code-review-2026-03-06.md` is acting as a parallel live roadmap outside the documented tier model**
- Evidence:
  - `Documentation/architecture-code-review-2026-03-06.md:27-45` marks multiple work packages `[DONE]`.
  - `Documentation/architecture-code-review-2026-03-06.md:61-109` still presents pre-refactor critical findings as if current.
  - `Documentation/architecture-code-review-2026-03-06.md:215-289` continues as a live execution plan.
  - `Documentation/standards.md:44-53` says architecture authority belongs in `ARCHITECTURE.md` and duplicate architecture review docs should be archived once consolidated.
- Risk:
  - The repo now has two competing architecture narratives: `ARCHITECTURE.md` and this review/roadmap hybrid.
  - Readers cannot tell whether the doc is historical context or current execution authority.
- Recommendation:
  - Fold durable architecture state into `ARCHITECTURE.md`.
  - Archive or explicitly demote this review once its surviving findings are reflected in canonical docs/backlog surfaces.

## Additional Observation

1. **Companion device detection will classify many generic “AirPods Pro” names as first generation**
- Evidence:
  - `companion/Sources/LocusQHeadTrackerCore/HeadphoneDeviceDetector.swift:40-61` only identifies Gen 2/3 when the device name literally contains `2nd generation`, `gen 2`, `pro 2`, `3rd generation`, `gen 3`, or `pro 3`.
  - `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift:55-79` persists the detected model into the default calibration profile.
- Risk:
  - Real user-customized or shortened macOS device names can map to the wrong default profile family.
- Recommendation:
  - Prefer hardware identifiers or a user-confirmable selection step over name-fragment heuristics alone.

## Recommended Priority Order

1. Fix headtracking timebase + stale-pose gating together.
2. Decide whether calibration profile FIR/SOFA is truly shipping; if yes, wire it completely before any promotion claim.
3. Remove the companion CDN fallback and stale `three.min.js` packaging assumptions.
4. Make BL-011/format/release gates validate fresh artifacts rather than reused evidence.
5. Collapse backlog/doc authority drift so execution always starts from `Documentation/backlog/index.md`.

## Overall Assessment

The repo has made real structural progress, but the highest-risk problems are now in the seams between “supported” claims and enforced behavior: headtracking timing, calibration profile application, companion packaging, and QA/release/doc governance truthfulness. Most of the current gates passing is consistent with the codebase; it is not yet sufficient evidence that the most user-visible advanced surfaces are correct.
