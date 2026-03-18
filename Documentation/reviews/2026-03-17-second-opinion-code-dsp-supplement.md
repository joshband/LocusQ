Title: LocusQ Second-Opinion Code + DSP Review Supplement
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# LocusQ Second-Opinion Code + DSP Review Supplement

## Purpose

Independent second opinion on the LocusQ codebase following a strict anti-anchoring sequence:

1. Repo contract files and core source files read first.
2. Independent findings and severity ordering formed before reading the existing review.
3. Smallest meaningful validation run (companion swift tests).
4. Existing review (`2026-03-17-comprehensive-code-dsp-review.md`) read.
5. This supplement produced as a comparison, challenge, and addendum.

The goal is not to duplicate the existing review, but to identify blind spots, refine severity calls, and add findings the first pass missed.

## Validation Status

- `partially tested`
- Targeted validation run:
  - `cd companion && swift test` → `PASS` (8/8 tests)
- Not rerun: CMake build, pluginval, BL-055 FIR lane, end-to-end companion-to-plugin streaming.

## Anti-Anchoring Sequence Files Read

Before reading the existing review:

- `AGENTS.md`, `.codex/rules/agent.md`
- `Source/PluginProcessor.h` (full, 538 lines)
- `Source/PluginProcessor.cpp` (lines 1–2400+)
- `Source/HeadTrackingBridge.h` (full, 620 lines)
- `Source/headphone_dsp/HeadphoneFirHook.h` (full, 271 lines)
- `Source/headphone_dsp/HeadphoneCalibrationChain.h` (full, 146 lines)
- `Source/spatial_renderer/SpatialHeadphoneCompensation.h` (full, 101 lines)
- `Source/spatial_renderer/SpatialSteamAudioBackend.h` (full, 167 lines)
- `Source/editor_webview/EditorWebViewRuntime.h` (full, 762 lines)
- `companion/Tests/LocusQHeadTrackerTests/*.swift` (all 3 test files)
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift` (lines 1–500)

## Comparison Against Existing Review

### Where This Review Agrees

All seven findings in the existing review were independently reached before reading it. Severity assessments match:

| Existing Finding | Independent Agreement | Notes |
|---|---|---|
| BL-055 false-green | AGREE — high | `runActiveConvolver` is always direct FIR regardless of `firEngineManager.activeEngine`. `PartitionedFftConvolver` struct is only used in `ignoreUnused()`. |
| FIR path claims partitioned latency it does not implement | AGREE — high | `partitionedLatencySamples = nextPow2BlockSize` is computed but never introduced into the signal path. |
| Companion v1 shipping vs v2 tested | AGREE — high | `PosePacketTests` validates 52-byte v2 layout; main.swift still sends 40-byte v1. |
| 30 Hz editor full-scene serialization | AGREE — medium | Independently noted as the primary UI-pressure finding. |
| Companion profile polling → renderer teardown | AGREE — medium | Thread boundary is too direct. |
| `locusq_webui_typecheck` not dep-complete | AGREE — medium | Verified by observation of stamp wiring in CMakeLists.txt. |
| CoreAudio property-string warning | AGREE — low | Low but worth treating as signal. |

**Severity refinement:** Existing findings #1 and #2 (BL-055 governance and false-latency advertisement) are the same underlying defect in two layers. They should be tracked as one combined high-severity truthfulness problem, not as two separate issues. Treating them as separate inflates the "high count" without adding resolution clarity.

### Where This Review Disagrees or Extends

The following are new findings not present in the existing review, ordered by severity.

---

## New Findings

### High

#### N1 — HeadphoneVerificationSnapshot scores are synthesized from static lookup tables and are not real perceptual measurements

- **Severity:** `high`
- **Affected files:**
  - `Source/PluginProcessor.cpp:1195–1278` (`buildHeadphoneVerificationSnapshot`)
  - `Source/PluginProcessor.cpp:1199–1216` (base score table by engine)
  - `Source/PluginProcessor.cpp:1218–1244` (penalty table by fallback reason)
- **Finding:**
  - `buildHeadphoneVerificationSnapshot()` populates `frontBackScore`, `elevationScore`, and `externalizationScore` entirely from hardcoded per-engine base values plus a hardcoded penalty lookup by fallback reason code.
  - For the `FirConvolution` engine: `baseFrontBack=0.84`, `baseElevation=0.79`, `baseExternalization=0.82`.
  - For the `ParametricEq` engine: `baseFrontBack=0.70`, `baseElevation=0.62`, `baseExternalization=0.66`.
  - A `confidence` score is then computed from the aggregate of these synthesized values plus a stage-based bias.
  - None of these values are derived from psychoacoustic measurement, participant data, or objective impulse-response analysis. They are fully fabricated constants presented as perceptual-quality metrics.
  - These fields are then published into `PublishedHeadphoneDiagnosticsSnapshot` which is consumed by the UI bridge, making them user-visible.
- **Why it matters:**
  - This is the same truthfulness problem as BL-055, but on the perceptual-verification side.
  - A user or QA engineer seeing `frontBackScore: 0.84` for the FIR path will conclude that front-back discrimination has been validated at 84%. It has not — that number is a baked-in constant.
  - Governance and release gating that relies on verification scores to assess HRTF calibration quality are relying on fabricated signals.
  - The existing review called out BL-055 and FIR latency dishonesty. This finding is equally dishonest in a different layer.
- **Recommended fix:**
  - Replace static base values with either (a) hard-coded `0.0f` until real measurement data exists, or (b) clear labeling that these are "reference estimates" not measured scores.
  - Connect verification scores to the perceptual-listening harness output (BL-060 evidence) rather than to a table of constants.
  - At minimum, mark the `verificationScoreStatus` as `kUnavailable` when the score source is purely synthetic.
- **Relationship to existing review:**
  - The existing review called out BL-055 and the FIR latency mismatch as the primary truthfulness problems. This is a third, independent truthfulness problem of equal structural concern.

---

### Medium

#### N2 — `getResource()` in EditorWebViewRuntime writes to a log file on every WebView asset request with no debug-only guard

- **Severity:** `medium`
- **Affected files:**
  - `Source/editor_webview/EditorWebViewRuntime.h:541–542` (getResource log write path A)
  - `Source/editor_webview/EditorWebViewRuntime.h:744–746` (getResource log write path B)
- **Finding:**
  - Both branches of `getResource()` call `resourceLogFile.appendText(logLine, true)` unconditionally.
  - This is synchronous file I/O on the message thread for every WebView asset request — every script, stylesheet, and image the WebView loads triggers a log write.
  - There is no `JUCE_DEBUG`, `#ifdef DEBUG`, `LOCUSQ_RESOURCE_LOG_ENABLED`, or similar guard in either call site.
  - The existing review identified the 30 Hz timer-driven serialization as the primary message-thread I/O concern. This is a separate, additive I/O pressure point that is not timer-gated and fires for every asset request during plugin open and any WebView reload.
- **Why it matters:**
  - Combined with the 30 Hz full-scene push, the message thread now has two independent I/O sources that are always active in production builds.
  - Synchronous file writes from `appendText()` block the message thread for disk-latency durations. On slow or busy storage this is a real editor-open hitch source.
  - Log files also grow unboundedly — there is no rotation, truncation, or file-size guard visible in the implementation.
- **Recommended fix:**
  - Guard both call sites behind a compile-time or runtime flag. For production, either disable entirely or write only when an explicit diagnostic mode is active.
  - Add a file-size cap or rotation guard if the log is intentionally always-on for diagnostics.
- **Relationship to existing review:**
  - The existing review flagged message-thread I/O pressure but focused on the serialization path. This log-file path compounds that pressure and needs to be addressed in the same cleanup pass.

#### N3 — HeadphoneCompensation per-profile coefficients are untraced magic numbers with no measurement provenance

- **Severity:** `medium`
- **Affected files:**
  - `Source/spatial_renderer/SpatialHeadphoneCompensation.h:33–58` (`makeHeadphoneCompensationConfig`)
- **Finding:**
  - The function applies a 700 Hz first-order shelf filter + crossfeed to compensate for specific headphone frequency response.
  - Profile-matched coefficients are fully hardcoded:
    - AirPods Pro 2 and 3: `lowGain=0.98`, `highGain=1.03`, `crossfeed=0.015`
    - Sony WH-1000XM5: `lowGain=1.04`, `highGain=0.97`, `crossfeed=0.020`
    - Custom SOFA: `lowGain=1.00`, `highGain=1.00`, `crossfeed=0.010`
  - No reference to measurements, published frequency response data, or calibration captures links these numbers to the named headphones.
  - The shelf cutoff (700 Hz) is a single universal constant regardless of profile.
- **Why it matters:**
  - In isolation, tiny ±2–4% gains are unlikely to cause audible problems. The concern is:
    1. The feature is named "headphone compensation" and references specific commercial products, which implies measured behavior.
    2. The constants are different per profile, creating the appearance of specificity that does not exist.
    3. The existing BL-058 companion work (ear-photo HRTF matching) builds toward personalized HRTF selection; compensation coefficients that are not personalized create an inconsistency in the personalization story.
  - This is a lower-severity instance of the same governance pattern as BL-055 and N1: features that present as data-driven but are actually constant-coded.
- **Recommended fix:**
  - Either derive coefficients from published or measured headphone response data and document the source, or remove per-profile variation and use a single, clearly labeled "generic correction" until real data is available.
  - The 700 Hz cutoff should at minimum be documented with its origin.
- **Severity note:**
  - This is `medium` rather than `high` because the coefficients are small and unlikely to cause audible harm. The risk is governance and user trust, not audio correctness.

---

### Low

#### N4 — Finite-output guardrail diagnostics use a "first wins" fallbackReason that can misclassify root cause

- **Severity:** `low`
- **Affected files:**
  - `Source/PluginProcessor.cpp:1928` (fallbackReason initialization)
  - `Source/PluginProcessor.cpp:1940–1975` (guardrail classification logic)
- **Finding:**
  - The `fallbackReason` variable is only updated when `fallbackReason == 0` for denormal and hard-clamp cases.
  - Non-finite samples unconditionally set `fallbackReason = 5`.
  - Result: if denormals appear before hard-clamp samples in the same block, the published reason stays `3` (denormal) even if hard clamping also occurred in the same block.
  - The non-finite path sets reason `5`, which is the same code used for limiter clamping (lines 1945 and 1975). This means a block with both non-finite and limiter-clamped samples will publish reason `5` regardless of which occurred first.
- **Why it matters:**
  - `PublishedFiniteGuardrailDiagnostics` is observable telemetry used for diagnosing renderer instability (BL-078).
  - If an operator observes `fallbackReason=3` (denormal), they may conclude the renderer has a denormal issue when the actual severity is a hard-clamp event.
  - The non-finite and limiter cases sharing reason code `5` is a collision that reduces diagnostic resolution.
- **Recommended fix:**
  - Use the highest-severity reason rather than the first. Priority order: non-finite (highest) > hard-clamp > limiter-clamp > denormal.
  - Assign distinct reason codes to non-finite and limiter-clamp paths.
- **Severity note:**
  - Audio output is still correctly protected. This is a diagnostics clarity problem, not a safety problem.

#### N5 — `Time::currentTimeMillis()` is called on the audio thread inside `tryBuildFreshInterpolatedHeadPose()`

- **Severity:** `low` (observation)
- **Affected files:**
  - `Source/PluginProcessor.cpp:424` (`tryBuildFreshInterpolatedHeadPose`)
  - `Source/PluginProcessor.cpp:1686–1689` (called from `prepareRendererRealtimeStateForBlock`, which runs in `processBlock`)
- **Finding:**
  - `juce::Time::currentTimeMillis()` at line 424 is a system time call inside a function that is invoked from the audio callback.
  - JUCE's own codebase calls similar functions from the audio thread, and this is widely tolerated in practice. However, `currentTimeMillis()` is a kernel call whose cost can vary under OS scheduling pressure.
  - The existing review called head-pose freshness and timebase handling a strength. That assessment is correct for the freshness logic itself, but the system call mechanism is worth noting as a strictness caveat.
- **Why it matters:**
  - On most modern host configurations this is benign. It becomes a concern under heavy load, OS scheduling pressure, or on hosts with very tight buffer sizes.
  - `juce::Time::getHighResolutionTicks()` (used elsewhere in processBlock for perf timing) has similar characteristics but is explicitly in non-RT-critical bookkeeping code. The freshness check in `tryBuildFreshInterpolatedHeadPose` is on the hot path for pose delivery.
- **Recommended fix:**
  - Cache a "now" snapshot at the top of `processBlock` (already done for `blockStartTicks`) and pass it through to freshness checks rather than re-querying the system inside the helper.
- **Severity note:**
  - No evidence this is causing problems in practice. It is worth correcting the next time the function is touched.

#### N6 — EarPhotoMatcher fallback test uses an artificially high similarity threshold that does not cover default-threshold behavior

- **Severity:** `low` (test coverage gap)
- **Affected files:**
  - `companion/Tests/LocusQHeadTrackerTests/EarPhotoMatcherTests.swift:59` (`fallbackSimilarity: 0.95`)
- **Finding:**
  - `testLowSimilarityFallsBackToDefaultSubject` passes `fallbackSimilarity: 0.95` to force a fallback even for plausibly similar embeddings.
  - The default similarity threshold used in production is not covered by this test. A change to the default threshold that accidentally eliminated the fallback gate would not be caught.
  - The inverse-embedding construction ensures fallback regardless of threshold at 0.95, so the test is validating the mechanism, not the default-threshold contract.
- **Why it matters:**
  - BL-058 relies on the fallback path to protect against low-quality ear-photo matches being treated as high-confidence. If the default threshold is too low, users with poor ear photos could receive incorrect HRTF profiles silently.
- **Recommended fix:**
  - Add a test case that invokes `match(captureImages:)` without a custom `fallbackSimilarity` and verifies fallback behavior at the default threshold value.
  - Export or document the default threshold constant so it is a testable contract.

---

## Where The Existing Review Was Stronger

These items from the existing review were not reached during independent reading (either due to scope limitation or files not fully read):

1. **Calibration profile polling → `getCallbackLock()` → Steam Audio reload chain** (existing finding #5): The existing review traced this through `ProcessorCalibrationBridge.cpp:1285–1313`. That detail is more precise than my independent read.

2. **`locusq_webui_typecheck` clean-tree dependency gap** (existing finding #6): I observed the CMakeLists structure but did not build-verify this independently.

3. **`ctest` finding no registered plugin tests** (existing validation gap #4): Worth noting that the plugin itself has no CMake/CTest-registered tests — not just "no passing tests" but literally no test targets registered. This is a significant gap that the existing review surfaced and this supplement confirms as unresolved.

4. **AUv3 filesystem assumption concern** (existing Plausible section #3): The `EditorWebViewRuntime.h` desktop-style temp directory path is a real risk for sandboxed AUv3 extension contexts. This was not independently flagged by this review at high severity.

---

## Strengthened Priority Ordering

Based on all findings (existing + new), the revised severity stack is:

| Rank | Finding | Source | Severity |
|---|---|---|---|
| 1 | BL-055 false-green + FIR claims partitioned latency it doesn't implement | Existing #1 + #2 (consolidated) | High |
| 2 | Companion v1 shipping / v2 tested protocol divergence | Existing #3 | High |
| 3 | HeadphoneVerificationSnapshot synthetic perceptual scores | **NEW N1** | High |
| 4 | 30 Hz full-scene serialization | Existing #4 | Medium |
| 5 | Companion profile polling → renderer teardown coupling | Existing #5 | Medium |
| 6 | `locusq_webui_typecheck` dep-incomplete | Existing #6 | Medium |
| 7 | `getResource()` unconditional file I/O in production | **NEW N2** | Medium |
| 8 | HeadphoneCompensation coefficient traceability | **NEW N3** | Medium |
| 9 | CoreAudio property-string helper warning | Existing #7 | Low |
| 10 | Finite-output diagnostics first-wins fallbackReason | **NEW N4** | Low |
| 11 | `Time::currentTimeMillis()` on audio thread | **NEW N5** | Low |
| 12 | EarPhotoMatcher default-threshold fallback coverage gap | **NEW N6** | Low |

---

## Amended Quick Wins

### Add Now (from new findings)

1. **Mark HeadphoneVerificationSnapshot scores as estimates, not measurements.** Either zero them until real data exists or label `verificationScoreStatus` as `kUnavailable` when source is static.
2. **Guard `resourceLogFile.appendText()` behind a debug/diagnostic flag.** Unconditional file I/O in production is never free.
3. **Fix finite-output diagnostic reason priority.** Non-finite > hard-clamp > limiter-clamp > denormal. Assign distinct codes to non-finite vs limiter-clamp.

### Retain From Existing Review (confirmed)

1. Correct BL-055 status to reflect structural-marker scaffolding, not partitioned-engine behavior.
2. Stop reporting partitioned-FIR latency until an actual partitioned engine exists.
3. Decide companion v1/v2 packet contract and align tests to the shipping path.
4. Wire `locusq_webui_typecheck` to dependency-install stamp.
5. Fix CoreAudio string helper warning.

---

## Overall Assessment Refinement

The existing review's overall assessment is accurate: the repo is materially better than a prototype, the headtracking bridge is disciplined, and the finite-output guardrails are real and working.

The supplement adds one finding of equal structural weight to the two main existing high-severity items: the headphone verification scores are synthesized constants presented as perceptual quality measurements. This is a governance truthfulness problem equivalent in nature to BL-055, and it shares the same root risk — the instrumentation that QA and release decisions depend on is not actually measuring what it claims to measure.

The secondary new findings (unconditional resource log, compensation coefficient traceability) are additive concerns in the medium tier rather than new critical paths.

The "biggest realtime/DSP contract problem is honesty, not classic lock/allocation violations" framing from the existing review stands. This supplement adds a third dishonesty vector (verification scores) to the two already identified (BL-055 governance and FIR latency reporting).

All companion swift tests pass. No new test regressions introduced.
