Title: LocusQ Comprehensive Code + DSP Review
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# LocusQ Comprehensive Code + DSP Review

## Purpose

Provide a repo-aware, findings-first assessment of LocusQ's current codebase with emphasis on realtime DSP behavior, WebView/editor-runtime behavior, state/thread boundaries, resource/lifetime management, and code-consolidation opportunities.

The LocusQ Headtrack Companion app is included in this review as a secondary scope. It does not require a separate effort for this pass. A separate dedicated review would only be justified if the goal shifts toward Apple-platform capture/privacy productization, companion UI architecture, or a deliberate packet-protocol migration plan.

## Skills Used (Applied In Order)

| Stage | Skills | Why |
|---|---|---|
| Validation posture | `skill_testing` | Confirm current build/test posture before turning architectural concerns into claims. |
| Failure-mode audit | `skill_troubleshooting` | Separate real defects from environment noise and build-order artifacts. |
| DSP + renderer review | `spatial-audio-engineering`, `hrtf-rendering-validation-lab`, `steam-audio-capi`, `temporal-effects-engineering` | Audit low-latency spatial/headphone/render-path behavior, HRTF/FIR truthfulness, Steam Audio fallback behavior, and time-domain transition safety. |
| Companion runtime review | `headtracking-companion-runtime`, `apple-spatial-companion-platform` | Audit companion readiness, packet/runtime integrity, and Apple-platform-specific implementation edges. |
| UI/runtime bridge review | `juce-webview-runtime`, `juce-webview-windows`, `threejs`, `reactive-av`, `realtime-dimensional-visualization` | Audit WebView/editor bridge cadence, Windows host/runtime assumptions, scene-update pressure, and Three.js-facing runtime delivery choices. |
| Format lifecycle review | `auv3-plugin-lifecycle`, `clap-plugin-lifecycle` | Distinguish "missing format work" from "format work exists but is still blocked on ship-readiness details." |
| Validation/governance review | `perceptual-listening-harness`, `documentation-hygiene-expert`, `skill_docs` | Separate evidence maturity from code maturity and keep the report repo-compliant. |

Named-skill routing was also checked against `skill_dream`, `skill_plan`, `skill_design`, `audio-ui-visual-dna-designer`, `imagegen`, `screenshot`, `playwright`, `physics-reactive-audio`, and `simulation-behavior-audio-visual`. They were not materially applied in the final assessment because this pass remained a static technical/governance review rather than ideation, design production, image generation, browser automation, or simulation-feature implementation work.

## Scope Reviewed

- Plugin/runtime:
  - `Source/PluginProcessor.*`
  - `Source/PluginEditor.*`
  - `Source/SpatialRenderer.*`
  - `Source/spatial_renderer/*`
  - `Source/headphone_dsp/*`
  - `Source/HeadTrackingBridge.h`
  - selected processor bridge/core files
- Companion:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift`
  - `companion/Sources/LocusQHeadTrackerCore/*`
  - `companion/Tests/LocusQHeadTrackerTests/*`
  - `companion/README.md`
- Build/config:
  - `CMakeLists.txt`

## Validation Status

- `partially tested`
- Targeted validation run during this review:
  - `cd companion && swift test` -> `PASS` (8 tests)
  - `cmake -S . -B /tmp/locusq_review_build -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR="$PWD/../audio-plugin-coder/_tools/JUCE"` -> `PASS`
  - `cmake --build /tmp/locusq_review_build --config Release --target LocusQ_Standalone -j 4` -> `PASS` with warnings
  - `cmake --build /tmp/locusq_review_build --config Release --target locusq_webui_typecheck -j 4` -> `PASS` after dependencies had been installed by another target; the target was not self-sufficient on a clean tree
  - `ctest --test-dir /tmp/locusq_review_build --output-on-failure` -> `PASS`, but no registered tests were found
- Existing repo evidence reviewed during this refinement:
  - `Documentation/backlog/bl-060-phase-b-listening-test-harness.md` and `TestEvidence/bl060_phase_b_listening_20260317T174025Z_90778/analysis/gate_decision.md` show the perceptual-listening harness is real and currently `PASS` on fixture evidence (`45.5%` externalization improvement, `p=0.0000`), with the remaining blocker being human participant count
  - `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md` and `TestEvidence/validation-trend.md` show AUv3 lifecycle intake is already `PASS_WITH_BLOCKERS`, not missing
  - `CMakeLists.txt`, `.github/workflows/qa_harness.yml`, `.github/workflows/release-governance.yml`, and `TestEvidence/validation-trend.md` show CLAP build/validation scaffolding is present and exercised
- Documentation closeout note:
  - `./scripts/validate-docs-freshness.sh` still fails in this checkout because of unrelated pre-existing metadata omissions under `Documentation/reports/ui-ux-refinement-2026-03-17/`; those files were left untouched to keep this review scoped
- Not rerun in this review:
  - pluginval/DAW host smoke
  - BL-055 objective FIR/latency parity lanes
  - end-to-end companion-to-plugin live streaming with hardware

## Findings (Confirmed, Ordered By Severity)

### High

1. **BL-055 is currently false-green: backlog and QA evidence mark a partitioned FIR engine as done based on markers, not on implemented behavior**

- Severity: `high`
- Affected files:
  - `Documentation/backlog/done/bl-055-fir-convolution-engine.md:11-21`
  - `Documentation/backlog/done/bl-055-fir-convolution-engine.md:55-67`
  - `Documentation/backlog/done/bl-055-fir-convolution-engine.md:102-123`
  - `scripts/qa-bl055-fir-convolution-engine-mac.sh:199-237`
  - `Source/headphone_dsp/HeadphoneFirHook.h:18-23`
  - `Source/headphone_dsp/HeadphoneFirHook.h:52-55`
  - `Source/headphone_dsp/HeadphoneFirHook.h:199-220`
- Why it matters:
  - BL-055 is documented as "already implemented" and `Done`, with acceptance language that implies real direct-vs-partitioned behavior, latency publication, crossfade, and offline parity.
  - The associated QA script passes those gates by grepping for marker strings such as `DirectFirConvolver`, `PartitionedFftConvolver`, `nextPow2`, `crossfade`, and `blend`.
  - The actual DSP path still runs one direct time-domain tap loop in `runActiveConvolver()`.
- Likely runtime/DSP/user impact:
  - Release/governance decisions can be made on evidence that overstates true DSP readiness
  - Real CPU/PDC regressions can hide behind green structural lanes
  - Team trust in backlog/evidence surfaces erodes because "done" no longer means "behavior exists"
- Recommended fix:
  - Immediately downgrade BL-055 status or split it into "contract scaffolding landed" versus "partitioned engine implemented."
  - Rewrite the BL-055 execute lane to measure actual impulse offset, engine behavior across the tap threshold, and transition artifacts instead of marker presence.
  - Keep structural-marker checks only as a lightweight preflight, not as promotion evidence.
- Expected payoff:
  - More truthful release governance
  - QA evidence that actually protects users and hosts
  - Lower risk of shipping a performance/latency mismatch under a green backlog state

2. **The FIR calibration path advertises a partitioned engine and nonzero latency that the DSP never actually implements**

- Severity: `high`
- Affected files:
  - `Source/headphone_dsp/HeadphoneFirHook.h:18-23`
  - `Source/headphone_dsp/HeadphoneFirHook.h:33-38`
  - `Source/headphone_dsp/HeadphoneFirHook.h:52-55`
  - `Source/headphone_dsp/HeadphoneFirHook.h:90-99`
  - `Source/headphone_dsp/HeadphoneFirHook.h:199-220`
  - `Source/headphone_dsp/HeadphoneCalibrationChain.h:123-135`
  - `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp:204-206`
  - `Source/PluginProcessor.cpp:2204-2210`
- Why it matters:
  - Tap counts above `kDirectFirTapThreshold` switch the reported engine to `PartitionedFftConvolver` and expose `partitionedLatencySamples`, but `runActiveConvolver()` always executes the same direct time-domain tap loop.
  - That means the code is simultaneously making two incorrect promises:
    - performance: long FIRs are still O(taps) per sample even when the engine says "partitioned"
    - latency: the plugin can report nonzero host latency for an algorithm that is still running with direct-form behavior
- Likely runtime/DSP/user impact:
  - Incorrect host PDC for FIR calibration paths above the threshold
  - Higher-than-advertised CPU cost for long impulse responses
  - Misleading telemetry/diagnostics when validating BL-055-style partitioned-FIR readiness
- Recommended fix:
  - Either implement a real partitioned FFT convolver and dual-path swap logic, or stop reporting a partitioned engine/latency until that implementation exists.
  - Add an objective test that compares reported latency, actual impulse peak offset, and CPU behavior across tap-count thresholds.
- Expected payoff:
  - Correct host timing
  - Honest DSP/runtime reporting
  - Clearer path to low-latency long-IR calibration without accidental regressions

3. **The shipping companion executable and the tested companion core have diverged into different packet protocols and runtime paths**

- Severity: `high`
- Affected files:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:101-121`
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:3626-3634`
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:3957-3964`
  - `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift:3-18`
  - `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift:37-63`
  - `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift:21-43`
  - `companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift:5-33`
  - `companion/README.md:23-39`
- Why it matters:
  - `main.swift` imports `LocusQHeadTrackerCore` but still defines and transmits its own `PosePacketV1` path.
  - The core library and its tests validate a different `PosePacket` schema (`version = 2`, `encodedSize = 52`), while the executable still sends the v1 40-byte payload.
  - This creates an especially risky form of code bloat: duplicated transport logic where the tested path is not the shipping path.
- Likely runtime/DSP/user impact:
  - Packet evolution can silently break the live executable even while tests remain green
  - State/diagnostic fields such as angular velocity and sensor-location flags can drift between "core" and "app" expectations
  - Companion maintenance cost rises because transport changes have to be made in at least two places
- Recommended fix:
  - Choose one source of truth for pose packets and runtime flow.
  - If v2 is the intended contract, move the executable onto `TrackerApp`/core transport immediately and add an integration test for plugin decode.
  - If v1 is still intentional, remove or clearly quarantine the unused v2 transport path so tests reflect the shipping contract.
- Expected payoff:
  - Better protocol integrity
  - Lower maintenance drag
  - Stronger confidence that headtracking behavior in the field matches what CI/test evidence claims

### Medium

4. **The editor message thread is doing heavyweight scene serialization and JS marshalling every 33 ms**

- Severity: `medium`
- Affected files:
  - `Source/PluginEditor.cpp:75-97`
  - `Source/editor_shell/EditorShellHelpers.h:7-19`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:4-19`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1062-1064`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1607-1609`
- Why it matters:
  - `timerCallback()` runs at 30 Hz and performs:
    - companion profile polling
    - full scene JSON generation
    - full calibration JSON generation
    - JS string construction and `evaluateJavascript()`
  - The serializer lives in a 1625-line header and hand-builds a large JSON string. The outbound bridge then wraps that payload into another JavaScript string before evaluating it in the WebView.
  - For a Three.js/WebView surface this is especially expensive because geometry-ish scene state, calibration state, and diagnostics all share one generic push path instead of having cadence tiers.
- Likely runtime/DSP/user impact:
  - UI hitch risk on the message thread
  - avoidable CPU churn when scene/calibration state has not materially changed
  - bridge back-pressure risk if the WebView cannot consume updates as quickly as they are generated
- Recommended fix:
  - Split scene-state publication into dirty regions or cadence tiers instead of pushing the full snapshot every tick.
  - Move the serializer out of the giant header into smaller translation units with explicit ownership boundaries.
  - Prefer structured/native bridge messages over large ad-hoc JS string concatenation where possible.
- Expected payoff:
  - Lower UI latency
  - less serialization churn
  - easier profiling and maintenance of the editor/runtime bridge

5. **Companion profile polling is coupled to renderer teardown/reload work from the editor thread**

- Severity: `medium`
- Affected files:
  - `Source/PluginEditor.cpp:79-86`
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1196-1210`
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1285-1313`
- Why it matters:
  - The editor timer polls the companion calibration file every tick.
  - On removal or on SOFA request changes, the code clears FIR state, toggles calibration enablement, takes `getCallbackLock()`, and reloads the Steam Audio runtime.
  - None of that work is in the audio callback, which is good, but the thread boundary is still too blunt: message-thread file detection is directly triggering heavyweight renderer state transitions.
- Likely runtime/DSP/user impact:
  - host/editor hitches during profile changes
  - callback-lock contention windows during profile-driven reloads
  - harder-to-reason-about ownership between UI polling, persistent profile state, and renderer reconfiguration
- Recommended fix:
  - Separate file observation, file parsing, and renderer application into distinct stages.
  - Use a background parse/debounce step plus an atomic or lock-minimized handoff to the processor.
  - Reserve heavyweight runtime reloads for explicit change application rather than raw timer polling.
- Expected payoff:
  - smoother editor behavior
  - cleaner state ownership
  - lower risk of UI-driven reconfiguration stalls

6. **`locusq_webui_typecheck` is not dependency-complete in clean builds**

- Severity: `medium`
- Affected files:
  - `CMakeLists.txt:167-205`
- Why it matters:
  - The deps stamp created by `npm ci` is correctly wired into the bundle target, but the typecheck target has no dependency on that stamp.
  - In a clean build flow during this review, the standalone build path installed dependencies and then `locusq_webui_typecheck` passed; before that install step, the typecheck target was not self-sufficient.
- Likely runtime/DSP/user impact:
  - false-negative build failures in CI or local automation
  - hidden order dependencies between unrelated build targets
  - reduced trust in UI validation lanes
- Recommended fix:
  - Make `locusq_webui_typecheck` depend on `${LOCUSQ_WEBUI_DEPS_STAMP}` or a common `locusq_webui_deps` target.
  - Add a clean-tree UI build/typecheck lane so this cannot regress quietly.
- Expected payoff:
  - more reliable CI/local validation
  - cleaner build graph semantics
  - fewer "works only after some other target ran" failures

### Low

7. **The companion still carries a confirmed CoreAudio interop warning in a property-string helper**

- Severity: `low`
- Affected files:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:1518-1529`
- Why it matters:
  - `swift test` passed, but the build emitted a warning for forming an `UnsafeMutableRawPointer` to an `Optional<AnyObject>` while calling `AudioObjectGetPropertyData`.
  - Warnings in this kind of CoreAudio boundary code are worth treating as signal, not noise.
- Likely runtime/DSP/user impact:
  - no confirmed functional break in this review
  - lingering interop fragility and noisier builds
- Recommended fix:
  - Rework the helper to use an explicitly typed temporary object for CoreAudio string retrieval instead of passing the optional CF object slot directly.
- Expected payoff:
  - cleaner Swift builds
  - safer native interop
  - less chance of this helper turning into a brittle runtime issue later

## Plausible But Unverified Concerns

These did not rise to confirmed defects in this pass, but they are credible enough to merit follow-up.

1. **The current FIR engine-swap crossfade is not future-proof for a real dual-engine implementation**
- Affected files:
  - `Source/headphone_dsp/HeadphoneFirHook.h:181-182`
  - `Source/headphone_dsp/HeadphoneFirHook.h:241-252`
- Concern:
  - The swap logic crossfades wet against dry, not previous-engine output against next-engine output.
  - That is harmless while both paths are effectively the same direct FIR, but it will likely produce level dips or zipper artifacts once a true partitioned path lands.

2. **The output-routing/headphone stage is still heavily sample-by-sample and will likely become a CPU bottleneck as profile complexity grows**
- Affected files:
  - `Source/SpatialRenderer.cpp:514-530`
  - `Source/spatial_renderer/SpatialOutputRoutingStage.cpp:503-505`
  - `Source/spatial_renderer/SpatialOutputRoutingStage.cpp:563-574`
  - `Source/headphone_dsp/HeadphoneCalibrationChain.h:96-120`
- Concern:
  - Current structure is acceptable for modest quad/stereo work, but it is not especially SIMD-friendly and will not scale gracefully to longer FIRs, richer binaural paths, or larger output layouts without a deliberate block-processing pass.

3. **AUv3 extension-safe runtime behavior still appears to depend on standalone-style filesystem assumptions**
- Affected files:
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:130-161`
  - `Source/editor_webview/EditorWebViewRuntime.h:206-217`
  - `Source/editor_webview/EditorWebViewRuntime.h:293-347`
  - `CMakeLists.txt:40-49`
  - `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md:63-68`
  - `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md:95-107`
- Concern:
  - Recent BL-067 evidence shows AUv3 lifecycle/build scaffolding is real, but the runtime still resolves profiles/SOFA assets from user-home/app-data-style locations and uses desktop-oriented file-dialog assumptions.
  - That may be fine for standalone/AU/VST3 and still incomplete for hardened AUv3 container/app-group flows.

## Strengths Worth Preserving

1. **Head-pose freshness and timebase handling are materially better than in earlier review cycles**
- Evidence:
  - `Source/PluginProcessor.cpp:357-363`
  - `Source/PluginProcessor.cpp:416-430`
- Why it is good:
  - Pose freshness now keys off epoch milliseconds consistently before interpolation is used.

2. **The head-tracking bridge decode path is disciplined**
- Evidence:
  - `Source/HeadTrackingBridge.h:272-336`
  - `Source/HeadTrackingBridge.h:339-347`
- Why it is good:
  - Packets are validated for magic/version, quaternion finiteness is checked, quaternions are normalized, and publication uses a simple double-buffered snapshot handoff.

3. **The renderer already has meaningful guardrails instead of wishful comments**
- Evidence:
  - `Source/SpatialRenderer.cpp:536-629`
  - `Source/PluginProcessor.cpp:1919-1990`
- Why it is good:
  - Emitter budget culling is deterministic, and the finite-output pass actively zeros/clamps invalid samples while publishing telemetry.

4. **The Steam Audio integration is a real runtime with explicit fallback behavior, not just a compile-time placeholder**
- Evidence:
  - `Source/spatial_renderer/SpatialSteamAudioBackend.cpp:197-359`
- Why it is good:
  - The runtime prefers bundle-local library paths, resolves required symbols explicitly, attempts SOFA loading, and falls back to the default HRTF if SOFA creation fails.

5. **The companion UI asset path is now bundle/local-first rather than network-coupled**
- Evidence:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:1650-1655`
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:1944-2012`
- Why it is good:
  - Three.js is injected or resolved from bundled/local assets, which is the right direction for deterministic startup, privacy, and offline behavior.

## Format / Platform Posture

1. **AUv3 work is materially underway, but not yet ship-proven**
- Evidence:
  - `CMakeLists.txt:30-45`
  - `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md:11-21`
  - `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md:95-107`
  - `TestEvidence/validation-trend.md:16`
- Assessment:
  - AUv3 is no longer a missing-format story. It has real build/lifecycle validation, and the remaining risk is signing, host execution, and extension-safe runtime behavior.

2. **CLAP support looks structurally mature in build/CI terms**
- Evidence:
  - `CMakeLists.txt:92-120`
  - `CMakeLists.txt:347-365`
  - `.github/workflows/release-governance.yml:134-162`
  - `TestEvidence/validation-trend.md:24-25`
- Assessment:
  - No new CLAP-specific high-severity defects surfaced in this pass. The main CLAP risk is parity regression management, not missing lifecycle wiring.

3. **Windows WebView2 host wiring is explicit, but desktop-convenience paths should stay separated from sandboxed-format assumptions**
- Evidence:
  - `Source/editor_webview/EditorWebViewRuntime.h:206-217`
  - `Source/editor_webview/EditorWebViewRuntime.h:293-347`
- Assessment:
  - The Windows path is correctly intentional for desktop hosts. It just should not become the silent template for more constrained runtime environments.

## Performance Optimization Opportunities

1. **Replace full scene pushes with dirty-region or cadence-tiered updates**
- Current pressure points:
  - `Source/PluginEditor.cpp:75-97`
  - `Source/editor_shell/EditorShellHelpers.h:7-19`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:4-19`
- Opportunity:
  - Send a slower-changing structural snapshot at low cadence and small high-frequency deltas for meters, drift, and active pose diagnostics.

2. **Implement a real block/partitioned FIR path and SIMD-friendly routing path**
- Current pressure points:
  - `Source/headphone_dsp/HeadphoneFirHook.h:199-220`
  - `Source/SpatialRenderer.cpp:514-530`
  - `Source/spatial_renderer/SpatialOutputRoutingStage.cpp:563-574`
- Opportunity:
  - Move long-IR work to actual partitioned convolution and reduce per-sample function-call overhead in the output stage.

3. **Separate visualization cadence from structural scene cadence**
- Current pressure points:
  - `Source/PluginEditor.cpp:75-97`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1062-1064`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1607-1609`
- Opportunity:
  - Publish geometry/topology at a slower rate, while meters, pose freshness, and short-lived reactive diagnostics travel on smaller high-frequency deltas.

4. **Stop rebuilding large JavaScript payload strings every timer tick**
- Current pressure points:
  - `Source/editor_shell/EditorShellHelpers.h:11-19`
- Opportunity:
  - Prefer structured bridge payloads, smaller diffs, or reusable JS handlers with compact arguments.

5. **Move calibration profile observation off the editor timer**
- Current pressure points:
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1196-1210`
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1306-1313`
- Opportunity:
  - A file watcher or debounced background refresh would cut repeated stat/load work and isolate heavy runtime changes from UI cadence.

## Code Bloat / Consolidation / Reduction Candidates

1. **Oversized core files are carrying too many responsibilities**
- Evidence:
  - `Source/PluginProcessor.cpp` -> 2941 lines
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h` -> 1625 lines
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift` -> 4440 lines
- Assessment:
  - The current file concentration increases compile/review cost, obscures ownership boundaries, and makes small behavioral changes riskier than they need to be.

2. **The companion transport/runtime logic is duplicated rather than layered**
- Evidence:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:101-121`
  - `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift:3-18`
  - `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift:21-43`
- Assessment:
  - The executable should either wrap the core cleanly or own transport fully. The current hybrid shape pays the maintenance cost of both.

3. **The scene-state serializer is a strong candidate for functional decomposition**
- Evidence:
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:4-19`
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h:1607-1609`
- Assessment:
  - The file is doing serialization, authoring/timeline state publication, renderer diagnostics, and UI-facing formatting in one giant header surface.

## Resource And Lifetime Management

1. **Renderer reload lifetime is too directly coupled to UI-side file detection**
- Evidence:
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1204-1208`
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1306-1310`
- Assessment:
  - This is not an audio-thread correctness failure, but it is still a resource/lifetime smell because runtime teardown/reload is being triggered from a UI polling loop.

2. **WebView update strings are short-lived but unnecessarily large and frequent**
- Evidence:
  - `Source/editor_shell/EditorShellHelpers.h:11-19`
- Assessment:
  - Repeated allocation/copy pressure on the UI side is avoidable and grows with scene-state size.

3. **Companion native interop should be kept warning-clean**
- Evidence:
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:1518-1529`
- Assessment:
  - Native boundary warnings are often early indicators of lifetime/ownership ambiguity.

4. **Steam Audio runtime lifetime handling is relatively disciplined and worth preserving during refactors**
- Evidence:
  - `Source/spatial_renderer/SpatialSteamAudioBackend.cpp:222-241`
  - `Source/spatial_renderer/SpatialSteamAudioBackend.cpp:318-359`
- Assessment:
  - The current init/teardown sequence is more structured than many ad hoc SDK integrations; the main problem is when reloads are triggered, not that the runtime has no lifetime discipline.

## State-Management And Threading Risks

1. **Calibration profile state is crossing UI, filesystem, renderer, and callback-lock boundaries in one path**
- Evidence:
  - `Source/PluginEditor.cpp:79-86`
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1196-1210`
  - `Source/processor_core/ProcessorCalibrationBridge.cpp:1306-1313`
- Risk:
  - The path works, but it is harder than necessary to reason about ordering and responsiveness.

2. **Companion protocol/state truth is split across docs, executable code, and tested core code**
- Evidence:
  - `companion/README.md:23-39`
  - `companion/Sources/LocusQHeadTrackingCompanion/main.swift:101-121`
  - `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift:3-18`
- Risk:
  - Protocol drift becomes a state-management problem, not just a code-style problem.

3. **The head-tracking bridge itself is a relative bright spot**
- Evidence:
  - `Source/HeadTrackingBridge.h:272-347`
- Assessment:
  - The low-level handoff path is comparatively clean and should remain the model for other cross-thread state publication surfaces.

## Realtime DSP Safety And Low-Latency Notes

1. **The biggest realtime/DSP contract problem is honesty, not a classic lock/allocation violation**
- Evidence:
  - `Source/headphone_dsp/HeadphoneFirHook.h:90-99`
  - `Source/headphone_dsp/HeadphoneFirHook.h:199-220`
  - `Source/PluginProcessor.cpp:2204-2210`
- Assessment:
  - The audio thread is not obviously blocking here, but the latency/engine contract is still wrong. That matters for hosts and for user trust.

2. **The renderer already includes useful denormal and non-finite protections**
- Evidence:
  - `Source/PluginProcessor.cpp:1790`
  - `Source/PluginProcessor.cpp:1919-1990`
- Assessment:
  - This is the right direction for realtime safety and should be preserved.

3. **Sample-by-sample calibration/routing is acceptable for now, but it is the clearest future low-latency scaling ceiling**
- Evidence:
  - `Source/spatial_renderer/SpatialOutputRoutingStage.cpp:503-505`
  - `Source/spatial_renderer/SpatialOutputRoutingStage.cpp:563-574`
  - `Source/headphone_dsp/HeadphoneCalibrationChain.h:96-120`

4. **The perceptual-validation story is no longer "missing," but it is still not broad enough to close the product loop alone**
- Evidence:
  - `Documentation/backlog/bl-060-phase-b-listening-test-harness.md:11-21`
  - `Documentation/backlog/bl-060-phase-b-listening-test-harness.md:47-57`
  - `TestEvidence/bl060_phase_b_listening_20260317T174025Z_90778/analysis/gate_decision.md:9-15`
- Assessment:
  - The harness and fixture gate are real strengths, but they do not substitute for objective FIR truth-render checks or real-user participant breadth.

## Validation Gaps And Recommended Next Tests

1. **Replace BL-055 marker-only validation with an objective FIR latency/parity lane**
- Goal:
  - Verify that reported latency, actual impulse response peak location, CPU profile, and crossfade behavior agree across short and long FIR tap counts.
- Evidence gap:
  - `scripts/qa-bl055-fir-convolution-engine-mac.sh:199-237` currently checks text markers rather than runtime behavior.

2. **Add a clean-tree UI validation lane**
- Goal:
  - Run `locusq_webui_typecheck` and bundle generation in a fresh environment so dependency-order bugs are caught automatically.

3. **Add a companion-to-plugin integration test for packet versions**
- Goal:
  - Exercise the shipping executable path, not just the core library serializer.

4. **Add at least one plugin-owned automated test target**
- Evidence:
  - `ctest --test-dir /tmp/locusq_review_build --output-on-failure` found no registered tests
- Goal:
  - Ensure CMake/CTest evidence exists for plugin-side behavior instead of relying on ad hoc build success alone.

5. **Treat BL-060 as a participant-maturity problem now, not as a missing-harness problem**
- Goal:
  - Complete the `>=5` real-participant requirement and connect the outcome to profile verification fields and release gating.

6. **Complete host-executed AUv3 validation after the current contract/build blockers are cleared**
- Goal:
  - Validate signing, host execution, and sandbox-safe profile/SOFA/runtime behavior in real AUv3 hosts.

7. **Rerun host-facing validation after the FIR contract is corrected**
- Goal:
  - Verify PDC, calibration mode timing, and no-click/no-zipper behavior under automation.

## Quick Wins Vs Deeper Refactors

### Do Now

1. Correct BL-055 status/evidence so it reflects actual implemented behavior, not marker presence.
2. Stop reporting partitioned-FIR latency until an actual partitioned engine exists.
3. Decide whether v1 or v2 is the real companion packet contract and make tests follow the shipping path.
4. Wire `locusq_webui_typecheck` to the dependency-install step.
5. Fix the CoreAudio string helper warning in the companion.

### Next

1. Split the editor scene publication path into smaller, lower-cadence or diff-based updates.
2. Move companion calibration file observation and parsing out of the editor timer.
3. Break up `ProcessorSceneStateBridgeOps.h` and `PluginProcessor.cpp` by ownership domain.
4. Audit AUv3 runtime paths for container/app-group-safe profile and SOFA access before promotion.

### Later

1. Implement a real partitioned FFT FIR path with correct latency accounting and swap behavior.
2. Refactor the companion executable to compose the tested core runtime instead of shadowing it.
3. Revisit output-stage vectorization/block processing once the FIR contract is corrected and profiled.

## Overall Assessment

The repo is in materially better shape than a "wild west" prototype. The headtracking bridge is disciplined, the renderer has real guardrails, and several earlier freshness/timebase risks are now handled more cleanly.

This refinement pass also shows more maturity than the first pass alone suggested:
- Steam Audio fallback/runtime wiring is more concrete
- the perceptual-listening harness is real and already generating meaningful fixture evidence
- AUv3/CLAP lifecycle scaffolding is present, with AUv3 blocked on signing/host execution rather than on missing build wiring

The main remaining issues are still truthfulness and boundary problems rather than catastrophic audio-thread mistakes:
- BL-055 governance/evidence currently overstates a partitioned FIR implementation that is not really there yet
- the FIR calibration path itself claims a partitioned, latency-bearing engine that is still direct-form DSP
- the companion's shipping runtime has drifted away from the tested core transport path
- the editor/WebView path is functional but too chatty and too centralized

If the immediate goal is strengthening the product without a broad rewrite, the best near-term return is:
1. fix the BL-055 truthfulness gap in both code and validation evidence
2. unify the companion packet/runtime contract
3. reduce message-thread serialization and file-polling pressure
