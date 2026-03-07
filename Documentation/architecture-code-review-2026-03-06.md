Title: Architecture & Code Review — LocusQ v1.0.0-ga
Document Type: Review
Author: Claude Code (Opus 4.6)
Created Date: 2026-03-06
Last Modified Date: 2026-03-07 (Rev 11 — W1-B thread-safety hardening)

# LocusQ Architecture & Code Review

## Executive Summary

LocusQ is a sophisticated quadraphonic 3D spatial audio plugin (VST3/AU/LV2/CLAP) built on JUCE 8 with a WebView-based Three.js UI. The codebase demonstrates strong architectural foundations — lock-free inter-instance communication, phase-gated development workflow, and comprehensive QA infrastructure. However, the v1.0.0-ga codebase has accumulated complexity debt in several critical areas that impact maintainability, extensibility, and runtime performance.

This review covers **architecture**, **code quality**, **functionality**, and **usability**, and concludes with a **prioritized, dependency-aware execution plan** for parallel agent sessions.

## Review Status Legend

- `[DONE]` completed and no longer the active focus for this review cycle. Use `~~strikethrough~~` on the item name when the task itself is complete.
- `[ACTIVE]` in progress right now with meaningful remaining work.
- `[NEXT]` the next recommended item after the current active slice closes.
- `[QUEUED]` dependency-cleared or roadmap-approved, but not the current focus.
- `[DEFERRED]` intentionally postponed into another lane.
- `[BLOCKED]` waiting on a dependency, owner decision, or failing validation lane.
- `Actual / Time` should use exact dates or focused-session wording when available. If wall-clock effort was not logged, say `not logged` instead of guessing.
- `Tokens` should be `n/a` unless a session explicitly recorded token usage. This repo does not auto-log LLM token counts.
- Portable markdown only: do not rely on HTML/CSS color for state. Use status tags, tables, and `~~strikethrough~~` instead.

## Priority Snapshot

| Item | Status | Priority | Estimate | Actual / Time | Tokens | Updated | Where | Evidence / Remaining |
|---|---|---|---|---|---|---|---|---|
| `~~W0-A~~` PluginProcessor decomposition | `[DONE]` | P0 | Large | done; time not logged | `n/a` | 2026-03-06 | `PluginProcessor.cpp/h`, `Source/processor_core/*.cpp` | Tier 0 complete; `W1-B` now unlocked |
| `~~W0-B~~` SpatialRenderer body split + BL-076 continuation | `[DONE]` | P0 | Medium | multiple same-day continuation slices on 2026-03-06 | `n/a` | 2026-03-06 | `SpatialRenderer.h/.cpp`, `Source/spatial_renderer/*.cpp` | Tier 0 complete; BL-076 closeout complete |
| `~~W0-C~~` UI POC/incremental gating | `[DONE]` | P0 | Small | done; time not logged | `n/a` | 2026-03-06 | `CMakeLists.txt`, `Source/ui/public/` | Tier 0 complete |
| `~~W0-D~~` DSP correctness fixes | `[DONE]` | P0 | Medium | done; time not logged | `n/a` | 2026-03-06 | `DistanceAttenuator.h`, `VBAPPanner.h`, `PhysicsEngine.h` | Tier 0 complete |
| `~~BL-076~~` staged-orchestrator cleanup | `[DONE]` | P1 | Medium | latest cleanup slice landed 2026-03-06 | `n/a` | 2026-03-07 | `Source/SpatialRenderer.cpp`, `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp`, `Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp` | `Source/SpatialRenderer.cpp` now `662` LOC; W1-B is complete locally and W1-A is the next recommended cleanup slice |
| `~~W1-B~~` thread-safety fixes | `[DONE]` | P1 | Medium | focused hardening slice completed 2026-03-07 | `n/a` | 2026-03-07 | timeline, physics worker, diagnostics ownership | Triple-buffered RT timeline snapshots, `juce::Thread` physics cadence, and fixed-size sequence-safe diagnostics landed; validated by `locusq_qa` spatial scenarios plus `locusq_physics_probe` |
| W1-A ParameterBridge | `[NEXT]` | P2 | Medium | not started | `n/a` | 2026-03-07 | `PluginEditor.*`, parameter relay/attachment surfaces | Next architecture-roadmap slice after W1-B hardening |
| W1-C JS/Vite toolchain | `[QUEUED]` | P2 | Medium | not started | `n/a` | 2026-03-06 | WebView build pipeline | Depends on W0-C, but not the current priority |
| W1-D APVTS parameter grouping | `[QUEUED]` | P2 | Small | not started | `n/a` | 2026-03-06 | parameter layout/grouping surfaces | Useful cleanup, but lower urgency than W1-B |

---

## 1. Architecture Review

### 1.1 Strengths

- **Three-Mode Architecture** (Calibrate/Emitter/Renderer): Clean separation of concerns. Each mode has clear responsibilities and well-defined SceneGraph interaction patterns.
- **Lock-Free SceneGraph Singleton**: The double-buffered EmitterSlot design with atomic state transitions (`Free → Initializing → Active → Retiring → Free`) is textbook correct for cross-instance audio-thread communication.
- **Spatial Output Profile Router**: The profile/stage/domain matrix (SOM-028-*) provides a systematic way to handle output layout negotiation across stereo, quad, surround, and immersive configurations.
- **Feature Gate System**: CMake options cleanly gate optional features (Steam Audio, SOFA, CLAP, AUv3, head tracking), ensuring the core binary stays lean.
- **QA Harness**: 50+ JSON test scenarios with CI-integrated RT-safety auditing is production-grade infrastructure.

### 1.2 Critical Findings

#### CF-1: PluginProcessor God Object (~2400+ lines .cpp, 400+ lines .h)

`PluginProcessor.cpp` has grown into a monolithic file mixing:
- Parameter layout creation
- Scene graph registration logic
- Calibration profile I/O (JSON serialization, file discovery)
- Emitter preset management (save/load/rename/delete)
- Timeline serialization
- UI state persistence
- Rendering matrix computation
- Head tracking pose transformation
- Performance telemetry
- Confidence masking diagnostics

**Impact**: Very difficult to reason about, test in isolation, or modify safely. Any change to preset logic risks breaking calibration or rendering.

**Recommendation**: Extract into focused compilation units:
- `ProcessorParameterLayout.cpp` — parameter tree creation
- `ProcessorPresetManager.cpp` — preset/profile file I/O
- `ProcessorCalibrationBridge.cpp` — calibration profile management
- `ProcessorStateSerializer.cpp` — getStateInformation/setStateInformation
- `ProcessorSceneRegistration.cpp` — mode transition + SceneGraph claim/release
- Keep `PluginProcessor.cpp` as the thin orchestrator (~300 lines)

#### CF-2: SpatialRenderer Header-Only Mega-Class

`SpatialRenderer.h` is 64K+ tokens — an enormous header-only implementation. The class manages:
- VBAP panning per emitter
- Distance attenuation, air absorption, Doppler
- Speaker delay compensation
- FDN reverb + early reflections
- Head pose rotation
- Headphone calibration chain
- Steam Audio binaural backend
- Audition engine (13 signal types, 6 motion patterns)
- Ambisonics IR encoding
- Codec mapping (ADM/IAMF)

**Impact**: Compile times are inflated because every translation unit that includes `SpatialRenderer.h` recompiles 64K tokens. The class is untestable in isolation.

**Recommendation**: Split into `.cpp` implementation + focused sub-components (already started with `spatial_renderer/` subdir). Move the 800+ lines of `prepare()`, `process()`, and `shutdown()` into `SpatialRenderer.cpp`.

#### CF-3: PluginEditor Relay/Attachment Boilerplate Explosion

`PluginEditor.h` declares **~75 relay objects** and **~75 attachment objects** — 150 member variables of pure boilerplate. `PluginEditor.cpp` then manually creates each attachment one-by-one across 200+ lines.

**Impact**: Adding a new parameter requires edits in 4+ locations (parameter layout, relay, attachment, WebView JS). High risk of wiring errors.

**Recommendation**: Introduce a data-driven `ParameterBridge` that takes a parameter ID list and auto-generates relays + attachments. This is a common JUCE WebView pattern. Could reduce PluginEditor to ~50 lines of member declarations.

#### CF-4: Anonymous Namespace Scope in PluginProcessor.cpp

The file has an enormous anonymous namespace (lines 20–999+) containing helper structs, conversion functions, constants, and snapshot builders. Some of these (like `RendererMatrixSnapshot`, `RendererHeadTrackingSnapshot`) are substantial types that should be in headers for testability.

#### CF-5: Incremental UI Stages Shipped in Production Binary

`Source/ui/public/incremental/` contains 12 HTML stage files and their JS counterparts, plus a `poc/` directory. These are embedded into the binary via `juce_add_binary_data`. The production binary ships ~12 unused HTML files.

**Impact**: Wasted binary size, expanded attack surface for the resource provider.

**Recommendation**: Gate incremental/POC assets behind `LOCUSQ_UI_POC` CMake flag (partially exists but not enforced on incremental stages). Strip from release builds.

---

## 2. Code Quality Review

### 2.1 Thread Safety

**Good patterns observed:**
- `ScopedNoDenormals` in processBlock
- Atomic state transitions in SceneGraph (compare-exchange for slot lifecycle)
- Double-buffered PhysicsEngine state
- HeadTrackingBridge shared-core reference counting with CriticalSection

**Issues:**

- **TQ-1: SpinLock for keyframeTimeline** (`PluginProcessor.h:302`): `juce::SpinLock` is used across both the audio thread and the message thread for timeline serialization. If the message thread holds the lock during a complex JSON operation, the audio thread will spin-wait. Consider a triple-buffer or lock-free approach for the timeline data consumed on the audio thread.

- **TQ-2: PhysicsEngine uses `std::thread` + `sleep_for`**: The physics worker uses `std::this_thread::sleep_for` for timing, which provides no accuracy guarantees and wastes CPU in sleep granularity. On Windows, sleep granularity is ~15ms — at 240Hz target rate (4.17ms period), the effective rate could drop to ~60Hz. Consider using a high-resolution timer or JUCE's `Thread` with `wait()`.

- **TQ-3: `juce::String` in `PublishedHeadphoneCalibrationDiagnostics`**: These diagnostics structs contain `juce::String` members that are read/written under a `SpinLock`. String operations can allocate, and the lock is taken from the audio thread context. Move to fixed-size char arrays or enum indices.

### 2.2 DSP Correctness

**Good patterns:**
- VBAP with pre-computed inverse matrices — correct and efficient
- Gain smoothing at 20ms ramp time — appropriate for avoiding zipper noise
- Distance attenuation with 3 models is well-implemented
- Doppler processor and air absorption are per-emitter (correct)

**Issues:**

- **DQ-1: Distance model naming**: `InverseSquare` applies `ratio * ratio` (1/r²), but the comment says `gain = refDist / max(distance, refDist)` which is 1/r. The code is correct (physically realistic), but the comment is misleading.

- **DQ-2: VBAPPanner elevation handling**: The elevation projection uses a simple cosine blend toward equal gain. For immersive formats (7.1.4, 7.4.2), this will produce incorrect imaging. The height speakers should receive gain from a proper 3D VBAP or VBIP implementation, not a 2D projection with blending.

- **DQ-3: Physics collision energy accumulation**: In `PhysicsEngine::resolveCollisions`, `collisionEnergy` accumulates across all three axes without normalization. A simultaneous corner collision could report 3x the energy of a wall collision, creating inconsistent collision-reactive audio behavior.

### 2.3 Memory & Performance

- **MQ-1: `std::vector` in SpatialRenderer**: Several `std::vector<float>` members (`tempMonoBuffer`, `steamBinauralLeft/Right`, `speakerDelayLines`) are resized in `prepare()`. These should be fixed-size arrays or `juce::HeapBlock` to avoid potential reallocation during realtime operation.

- **MQ-2: MAX_EMITTERS = 256 with full EmitterSlot array**: Each `EmitterSlot` contains two `AudioBufferSnapshot` structs of 8192 floats each. Total: `256 * 2 * 8192 * 4 bytes = 16MB` of audio buffer space in the SceneGraph singleton, regardless of how many emitters are active. Consider lazy allocation or a smaller default.

- **MQ-3: Scene snapshot JSON generation**: `getSceneStateJSON()` likely allocates on every 30Hz timer call. For the renderer, this means string allocation 30x/sec. Consider a pre-allocated JSON builder or binary snapshot protocol.

### 2.4 WebView UI

**Good patterns:**
- Fallback JUCE bridge implementation for non-WebView contexts
- Self-test infrastructure for automated UI validation
- Clean separation of native bindings from UI logic

**Issues:**

- **UQ-1: No build toolchain for JS**: `index.js` is a single 200+ line IIFE with no module system, no TypeScript, no bundler. As the UI grows, this will become unmaintainable.

- **UQ-2: Three.js vendored as minified blob**: `three.min.js` is checked in without version tracking. No way to determine version or apply security patches.

- **UQ-3: `index.html` and `incremental/` stages are pure HTML with inline script dependencies**: No CSS framework, no component system. The 12-stage incremental approach suggests the UI was built iteratively without consolidation.

---

## 3. Functionality Review

### 3.1 Working Well
- Three-mode paradigm (Calibrate/Emitter/Renderer) with automatic host layout negotiation
- 76 automatable parameters with full DAW state save/restore
- Physics simulation with throw/reset/gravity/collision
- Keyframe animation timeline with transport sync
- Room calibration with sweep/noise/impulse measurements
- Head tracking via UDP companion bridge with sequence validation
- Emitter preset and calibration profile persistence

### 3.2 Gaps & Enhancements

- **FG-1: No undo/redo for parameter changes** from the WebView UI. DAW undo typically covers APVTS parameters, but manual keyframe edits and preset operations have no undo path.

- **FG-2: Calibration profile portability**: Profiles are stored in platform-specific app data directories with 4 fallback paths. No export/import mechanism for users to share calibration data across machines.

- **FG-3: No parameter grouping/hierarchy in APVTS**: All 76 parameters are flat. DAWs that support parameter groups (AU, VST3) would benefit from organized categories (Position, Physics, Renderer, Calibration).

- **FG-4: Audition signal generative engines (13 types)**: Impressive scope, but no user-facing documentation or tooltips explaining what each signal sounds like or its intended use case.

- **FG-5: CLAP and AUv3 are feature-gated OFF by default**: These format gates exist but are not exercised in CI. Risk of bitrot.

---

## 4. Usability Review

- **US-1: WebView cold start latency**: WebView2/WKWebView initialization adds noticeable delay on plugin first open. Consider a loading indicator or skeleton UI.

- **US-2: Mode switching UX**: Switching from Emitter to Renderer re-registers with SceneGraph, potentially causing audio interruption. The transition should be sample-accurate or at least cross-faded.

- **US-3: Error messaging**: Calibration and profile operations return `juce::var` with error codes. The UI needs to surface these clearly to users, not just log them.

- **US-4: No keyboard shortcuts or accessibility**: WebView UI has no documented keyboard navigation, screen reader support, or high-contrast mode.

---

## 5. Prioritized Execution Plan

### Dependency Graph

```
Tier 0 (Foundation — No Dependencies)
  ├─ [DONE] ~~W0-A: Extract PluginProcessor into focused compilation units~~
  ├─ [DONE] ~~W0-B: Move SpatialRenderer implementation to .cpp~~
  ├─ [DONE] ~~W0-C: Strip incremental/POC UI from release binary~~
  └─ [DONE] ~~W0-D: Fix DSP correctness issues (DQ-1, DQ-2, DQ-3)~~

Tier 1 (Depends on Tier 0)
  ├─ [NEXT] W1-A: Data-driven ParameterBridge for PluginEditor (depends W0-A)
  ├─ [DONE] ~~W1-B~~: Fix thread-safety issues TQ-1, TQ-2, TQ-3 (depends W0-A, W0-B)
  ├─ [QUEUED] W1-C: Introduce JS build toolchain (TypeScript/Vite) (depends W0-C)
  └─ [QUEUED] W1-D: APVTS parameter grouping (depends W0-A)

Tier 2 (Depends on Tier 1)
  ├─ [QUEUED] W2-A: Optimize memory (MQ-1, MQ-2, MQ-3) (depends W1-B)
  ├─ [QUEUED] W2-B: UI loading indicator + error surfaces (depends W1-C)
  ├─ [QUEUED] W2-C: Enable CLAP/AUv3 in CI (depends W0-A)
  └─ [QUEUED] W2-D: Calibration profile export/import (depends W0-A)

Tier 3 (Polish — Depends on Tier 2)
  ├─ [QUEUED] W3-A: Undo/redo for keyframe + preset operations (depends W2-A)
  ├─ [QUEUED] W3-B: Mode transition crossfade (depends W2-A)
  ├─ [QUEUED] W3-C: Keyboard/accessibility for WebView (depends W2-B)
  └─ [QUEUED] W3-D: Audition signal documentation/tooltips (depends W2-B)
```

### Work Package Details

#### Tier 0 — Foundation (All Independent, All Parallelizable)

| ID | Status | Task | Agent Type | Est. Scope | Files Affected |
|----|--------|------|-----------|-----------|----------------|
| **W0-A** | `[DONE]` | `~~Extract PluginProcessor into focused compilation units~~` | Claude Opus | Large | `PluginProcessor.cpp/h`, 6 new `.cpp` files, `CMakeLists.txt` |
| **W0-B** | `[DONE]` | `~~Move SpatialRenderer body to .cpp, keep header lean~~` | Claude Sonnet | Medium | `SpatialRenderer.h` → `SpatialRenderer.h` + `SpatialRenderer.cpp`, `CMakeLists.txt` |
| **W0-C** | `[DONE]` | `~~Gate incremental/POC UI assets behind build flag; strip from release~~` | Claude Haiku | Small | `CMakeLists.txt`, possibly `Source/ui/` restructure |
| **W0-D** | `[DONE]` | `~~Fix DQ-1 (comment), DQ-2 (3D VBAP for height speakers), DQ-3 (collision energy normalization)~~` | Claude Sonnet | Medium | `DistanceAttenuator.h`, `VBAPPanner.h`, `PhysicsEngine.h` |

**Parallel execution**: All 4 work packages can run simultaneously in separate worktrees.

#### Tier 1 — Structural Improvement (After Tier 0 Merges)

| ID | Status | Task | Agent Type | Est. Scope | Depends On |
|----|--------|------|-----------|-----------|-----------|
| **W1-A** | `[NEXT]` | Data-driven ParameterBridge: auto-generate relay+attachment from ID list | Claude Opus | Medium | W0-A |
| **W1-B** | `[DONE]` | Thread safety fixes: triple-buffer timeline, JUCE Thread for physics, fixed-size diagnostics | Claude Opus | Medium | W0-A, W0-B |
| **W1-C** | `[QUEUED]` | Add Vite/TypeScript build for WebView UI, vendor Three.js via npm | Claude Sonnet | Medium | W0-C |
| **W1-D** | `[QUEUED]` | APVTS parameter groups (Position, Physics, Renderer, Calibration, Animation, Visualization) | Claude Sonnet | Small | W0-A |

**Parallel execution**: W1-A and W1-B can run in parallel. W1-C is independent. W1-D is independent.

#### Tier 2 — Optimization & Feature Completion

| ID | Status | Task | Agent Type | Est. Scope | Depends On |
|----|--------|------|-----------|-----------|-----------|
| **W2-A** | `[QUEUED]` | Memory optimization: fixed buffers, lazy EmitterSlot allocation, binary snapshot protocol | Claude Opus | Medium | W1-B |
| **W2-B** | `[QUEUED]` | WebView loading skeleton, error toast system, calibration status panel | Claude Sonnet | Medium | W1-C |
| **W2-C** | `[QUEUED]` | Enable CLAP + AUv3 format builds in CI, add validation lanes | Claude Sonnet | Small | W0-A |
| **W2-D** | `[QUEUED]` | Calibration profile export/import (JSON file picker) | Claude Haiku | Small | W0-A |

**Parallel execution**: All 4 can run simultaneously.

#### Tier 3 — Polish

| ID | Status | Task | Agent Type | Est. Scope | Depends On |
|----|--------|------|-----------|-----------|-----------|
| **W3-A** | `[QUEUED]` | Undo/redo for keyframe timeline and preset operations | Claude Opus | Large | W2-A |
| **W3-B** | `[QUEUED]` | Sample-accurate mode transition with gain crossfade | Claude Opus | Medium | W2-A |
| **W3-C** | `[QUEUED]` | Keyboard navigation + ARIA attributes for WebView UI | Claude Sonnet | Medium | W2-B |
| **W3-D** | `[QUEUED]` | Audition signal tooltips and documentation in UI | Claude Haiku | Small | W2-B |

**Parallel execution**: W3-A + W3-B can pair. W3-C + W3-D can pair.

---

## 6. Recommended Execution Order for Maximum Parallelism

```
Phase 1 (4 agents):  W0-A  |  W0-B  |  W0-C  |  W0-D
Phase 2 (4 agents):  W1-A  |  W1-B  |  W1-C  |  W1-D
Phase 3 (4 agents):  W2-A  |  W2-B  |  W2-C  |  W2-D
Phase 4 (4 agents):  W3-A  |  W3-B  |  W3-C  |  W3-D
```

Each phase requires the previous phase to be merged before starting. Within each phase, all work packages are independent and can run in parallel across separate worktrees.

---

## 7. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| W0-A refactor breaks state serialization | Medium | High | Preserve exact `getStateInformation`/`setStateInformation` byte layout; round-trip test with saved sessions |
| W0-B breaks SpatialRenderer initialization order | Low | High | Maintain member declaration order; test with pluginval stress suite |
| W1-B triple-buffer introduces latency | Low | Medium | Timeline is already 30Hz; one frame of latency is imperceptible |
| W1-C TypeScript migration breaks self-test harness | Medium | Medium | Keep self-test JS as a separate entry point; migrate incrementally |
| W2-A lazy EmitterSlot changes SceneGraph memory model | Low | High | Gate behind feature flag; test with max emitter count scenario |

---

## 8. Backlog Alignment

This review was cross-referenced against the full backlog (84 items: BL-001–BL-077, HX-01–HX-06).

### Findings That Overlap Existing Backlog Items

| Review Finding | Backlog Item | BL Status | Alignment |
|---------------|-------------|-----------|-----------|
| CF-1: PluginProcessor God Object | **BL-032** Source modularization | Done-candidate | W0-A extends BL-032's extraction with additional units (PresetManager, CalibrationBridge, StateSerializer) |
| CF-2: SpatialRenderer Mega-Header | **BL-076** SpatialRenderer decomposition | Done | W0-B landed the out-of-line `SpatialRenderer.cpp` split locally, and the BL-076 follow-on moved the Steam runtime/monitoring code, audition control/render path, output/headphone/codec stage, and headphone/profile control+support surfaces into dedicated `Source/spatial_renderer/*.cpp` units; `Source/SpatialRenderer.cpp` is now `662` LOC and the closeout cadence is complete |
| CF-3: PluginEditor Boilerplate | **BL-040** UI modularization + authority UX | Done-candidate | W1-A extends BL-040 with data-driven ParameterBridge |
| CF-3: PluginEditor Boilerplate | **BL-039** Parameter relay spec generation | Done-candidate | W1-A builds on BL-039's relay spec contract |
| TQ-1: SpinLock timeline | **BL-035** RT lock-free registration | Done-candidate | W1-B extends lock-free patterns to timeline |
| TQ-3: String in diagnostics | **BL-036** DSP finite output guardrails | Done-candidate | W1-B aligns with BL-036 RT-safety scope |
| DQ-2: VBAP elevation | **BL-041** Doppler v2 + VBAP geometry | Done-candidate | W0-D directly implements BL-041's VBAP geometry validation |
| FG-5: CLAP/AUv3 CI | **BL-011** CLAP lifecycle (Done), **BL-067** AUv3 lifecycle (Open) | Mixed | W2-C activates CI lanes for both |
| MQ-3: JSON snapshot | **BL-070** Coherent audio snapshot seqlock | Done | W2-A builds on BL-070's seqlock contract |

### New Items Not Yet in Backlog

| Review Finding | Proposed BL | Priority |
|---------------|------------|----------|
| CF-4: Anonymous namespace scope | Fold into BL-032 promotion | P1 |
| CF-5: Incremental UI in release binary | New BL (P2) | P2 |
| TQ-2: PhysicsEngine sleep_for timing | New BL (P1) | P1 |
| UQ-1/UQ-2: JS toolchain + Three.js vendoring | New BL (P2) | P2 |
| FG-1: Undo/redo for keyframes | New BL (P3) | P3 |
| FG-2: Calibration profile export | New BL (P2) | P2 |
| FG-3: APVTS parameter grouping | New BL (P2) | P2 |
| US-1/US-4: WebView UX polish | New BL (P3) | P3 |

### Backlog Gate Dependencies for Tier 0

- **W0-A** can proceed: BL-032 is done-candidate; this extends and completes it.
- **W0-B** coordinated with BL-076 and the closeout is now complete.
- **W0-C** is net-new: no backlog conflict.
- **W0-D** extends BL-041 (done-candidate): safe to proceed, validates BL-041 promotion.

### Open Backlog Items That May Be Impacted by Tier 0 Changes

| BL ID | Title | Risk from Tier 0 |
|-------|-------|------------------|
| BL-020 | Confidence/masking overlay | Low — CF-1 extraction preserves diagnostics API |
| BL-021 | Room-story overlays | Low — rendering path unchanged |
| BL-053 | Head tracking orientation injection | Medium — W0-A moves head-tracking helpers; coordinate |
| BL-058 | Companion profile acquisition | Low — calibration bridge extraction is additive |
| BL-076 | SpatialRenderer decomposition | High — W0-B directly overlaps; must merge or coordinate |

---

## 9. Execution Log

### Tier 0 Execution Status

| ID | Status | Branch | Agent | Notes |
|----|--------|--------|-------|-------|
| W0-A | **DONE (local continuation)** | `review/claude-architecture-code-review-CmsoV` | Codex + Opus 4.6 handoff | Completed the remaining three extractions: SceneRegistration, PresetManager, and CalibrationBridge. `resolveCalibrationWritableChannels` is now a shared processor helper instead of a stranded local function. |
| W0-B | **DONE (local continuation)** | `main` | Codex | Added `Source/SpatialRenderer.cpp`, reduced `Source/SpatialRenderer.h` to `982` LOC, refreshed BL-076 contract+execute guardrails after wiring affected QA/probe targets to the modularized processor sources, then continued BL-076 with `Source/spatial_renderer/SpatialSteamAudioBackend.cpp`, the Wave 5 audition implementation units (`SpatialAuditionControl.cpp`, `SpatialAuditionSupport.cpp`, `SpatialAuditionSignalGenerator.cpp`, `SpatialAuditionRender.cpp`), the Wave 6 output-stage module `SpatialOutputRoutingStage.cpp`, and the Wave 6 headphone/profile support modules (`SpatialHeadphoneProfileControl.cpp`, `SpatialHeadphoneProfileSupport.cpp`), reducing `Source/SpatialRenderer.cpp` to `662` LOC. |
| W0-C | **DONE** | `claude/architecture-code-review-CmsoV` | Opus 4.6 | Gated ~24 incremental/POC UI assets behind `LOCUSQ_UI_POC` CMake flag |
| W0-D | **DONE** | `claude/architecture-code-review-CmsoV` | Opus 4.6 | Fixed DQ-1 (DistanceAttenuator comment), DQ-2 (VBAPPanner elevation note), DQ-3 (PhysicsEngine collision energy normalization) |

### BL-076 Closeout After W0-B

- The architecture review’s Tier 0 recommendation for W0-B is complete, and BL-076 is now closed as the broader follow-on for CF-2.
- Latest local continuation after the W0-B merge:
  - added `Source/spatial_renderer/SpatialSteamAudioBackend.cpp`
  - moved Steam runtime, diagnostics, monitoring, and binaural render method bodies out of `Source/SpatialRenderer.cpp`
  - added `Source/spatial_renderer/SpatialAuditionControl.cpp`, `SpatialAuditionSupport.cpp`, `SpatialAuditionSignalGenerator.cpp`, and `SpatialAuditionRender.cpp`
  - moved the audition control/support/signal/render method bodies out of `Source/SpatialRenderer.cpp`
  - added `Source/spatial_renderer/SpatialOutputRoutingStage.cpp`
  - moved `577` LOC of output routing, codec telemetry/publication, and headphone runtime/calibration helpers out of `Source/SpatialRenderer.cpp`
  - added `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp` and `Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp`
  - moved `426` LOC of headphone/profile control, snapshot getter, and `*ToString` methods plus `313` LOC of preset/head-pose/profile-routing/output-support helpers out of `Source/SpatialRenderer.cpp`
  - reduced `Source/SpatialRenderer.cpp` from `3998` LOC to `662` LOC across the Wave 4 through Wave 6 follow-on slices, which brings the file below the planning-packet `<=700` LOC target
  - replayed `build_local` (`LocusQ`, `locusq_qa`, `locusq_bl018_profile_probe`) plus BL-076 contract/execute guardrails on 2026-03-06 (UTC evidence roots `2026-03-07T00:23:45Z` and `2026-03-07T00:23:58Z`)
  - completed owner T2/T3 execute cadence with `TestEvidence/bl076_candidate_t2_closeout/` (`5/5`) and `TestEvidence/bl076_promotion_t3_closeout/` (`10/10`)
- Recommended next architecture-roadmap item: **W1-A** ParameterBridge cleanup.

### W1-B Detail: Thread-Safety Hardening

- Replaced the shared `keyframeTimeline` spin-lock path with non-RT timeline state plus triple-buffered RT playback snapshots, so the audio thread no longer competes with UI/state serialization for timeline ownership.
- Swapped `PhysicsEngine` from `std::thread` + `sleep_for` to a `juce::Thread` wait cadence with wakeups on rate/throw/reset changes, which removes the coarse sleep granularity risk called out in TQ-2.
- Replaced published headphone calibration/verification diagnostics `juce::String` payloads and their shared `SpinLock` with fixed-size sequence-safe snapshots, aligning the telemetry handoff with the repo’s existing seqlock-style contracts.
- Validation replay for the slice:
  - `cmake --build build_local --target LocusQ -j4` -> `PASS`
  - `cmake --build build_local --target locusq_qa locusq_physics_probe -j4` -> `PASS`
  - `build_local/locusq_physics_probe_artefacts/Release/locusq_physics_probe` -> `PASS` (`5/5`)
  - `build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_26_animation_internal_smoke.json` -> `PASS`
  - `build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_24_physics_spatial_motion.json` -> `PASS`
  - `build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_keyframe_loop_playback.json` -> `PASS`
  - `build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_state_roundtrip_contract.json` -> `PASS`

### W0-A Detail: PluginProcessor Decomposition

**Extracted compilation units:**

1. `Source/processor_core/ProcessorParameterLayout.cpp` — all 76 APVTS parameter definitions (~400 lines)
2. `Source/processor_core/ProcessorStateSerializer.cpp` — `getStateInformation`/`setStateInformation` (~90 lines after helper extraction)
3. `Source/processor_core/ProcessorSceneRegistration.cpp` — mode transition + SceneGraph claim/release orchestration (~270 lines)
4. `Source/processor_core/ProcessorPresetManager.cpp` — preset persistence, emitter UI state persistence, and preset JSON I/O (~620 lines)
5. `Source/processor_core/ProcessorCalibrationBridge.cpp` — calibration profile routing/state helpers, calibration profile JSON I/O, and companion profile polling (~1020 lines)
6. `Source/processor_core/ProcessorConstants.h` — shared snapshot schema/layout constants extracted from anonymous namespace

**PluginProcessor.cpp reduction:** ~3653 → ~2621 lines (−1032 lines, ~28% reduction)

**Follow-up note:** the W0-A extraction exposed one broken cross-file dependency in the branch version (`resolveCalibrationWritableChannels`) and one W0-C resource-provider regression (optional POC assets were still referenced unconditionally). Both are fixed in the local continuation.

### W0-C Detail: Binary Bloat Gate

Wrapped ~24 incremental/POC UI assets (`poc_*.html`, `visualizer_*.html`, stage JS files) in `if(LOCUSQ_UI_POC)` CMake guard. Production builds include only the 5 core UI files, and the runtime resource provider now gates those optional BinaryData lookups behind `LOCUSQ_UI_POC_DEFAULT` so default builds still compile.

### W0-D Detail: DSP Correctness Fixes

1. **DQ-1** (`DistanceAttenuator.h:11`): Corrected misleading comment — code implements `(refDist/distance)²` (1/r²), not clamped linear.
2. **DQ-2** (`VBAPPanner.h:114-115`): Added documentation note that `calculateGains(az, el)` is a 2D projection only suitable for horizontal quad; proper 3D VBAP needed for height speakers (BL-041).
3. **DQ-3** (`PhysicsEngine.h` `resolveCollisions`): Fixed corner collision energy accumulation — was per-axis additive (corner = 3× wall energy); now computes Euclidean magnitude of full velocity change vector.
