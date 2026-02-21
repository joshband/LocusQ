Title: LocusQ Comprehensive Architecture Review and Forward-Looking Roadmap
Document Type: Architecture Review
Author: APC Codex
Created Date: 2026-02-21
Last Modified Date: 2026-02-21

# LocusQ — Comprehensive Architecture Review and Forward-Looking Roadmap

**Version under review:** v1.0.0-ga (tagged 2026-02-20)
**Scope:** Full codebase architecture, DSP chain, UI layer, build/QA infrastructure, documentation governance, and forward roadmap.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Source Code Review](#3-source-code-review)
4. [DSP Chain Assessment](#4-dsp-chain-assessment)
5. [UI and WebView Layer](#5-ui-and-webview-layer)
6. [Build System and Infrastructure](#6-build-system-and-infrastructure)
7. [Quality Assurance and Testing](#7-quality-assurance-and-testing)
8. [Documentation and Governance](#8-documentation-and-governance)
9. [Identified Issues and Technical Debt](#9-identified-issues-and-technical-debt)
10. [Forward-Looking Roadmap](#10-forward-looking-roadmap)
11. [Risk Assessment](#11-risk-assessment)
12. [Recommendations Summary](#12-recommendations-summary)

---

## 1. Executive Summary

LocusQ is a research-grade (complexity 5/5) JUCE 8 spatial audio plugin implementing a novel multi-instance architecture where Emitter plugin instances publish spatial state through a lock-free SceneGraph singleton, and a single Renderer instance spatializes all active emitters to quad/stereo/mono output. The project has reached v1.0.0-ga with a fully functional DSP chain, physics engine, calibration system, keyframe animation, and WebView-based 3D UI.

### Strengths

- **Novel architecture**: The lock-free SceneGraph singleton enabling zero-allocation inter-instance communication on the audio thread is well-designed and correctly implemented with double-buffered atomic swaps.
- **Complete DSP chain**: VBAP panning, three distance models, air absorption, doppler, directivity, spread, early reflections, and FDN reverb — all allocation-free in the audio path.
- **Rigorous QA methodology**: Harness-first scenario testing with machine-readable evidence, pluginval stability probes, host-edge matrix coverage, and documented validation trends.
- **Thorough documentation**: 8 ADRs, formal invariants, scene-state contract, implementation traceability, and a docs freshness gate enforced by CI.
- **Phase discipline**: 35+ tracked phases in status.json with explicit acceptance criteria, validation evidence, and no auto-advancement.

### Concerns

- **Monolithic source files**: `PluginProcessor.cpp` (~88KB) and `PluginEditor.cpp` (~44KB) concentrate too much logic in two files, creating maintenance and merge risk.
- **Header-only DSP**: All DSP components (SpatialRenderer, VBAPPanner, etc.) are implemented entirely in headers, which increases compile times and couples implementation to declaration.
- **Incremental UI stages**: 12 HTML/JS stage files create a confusing artifact trail; production should converge to a single canonical UI entry point.
- **Missing automated UI tests**: Manual DAW host verification remains an open gating item. The self-test automation is headless-proxy only.
- **Limited platform coverage**: macOS-only build/ship pipeline; Windows and Linux are untested despite JUCE cross-platform capability.

---

## 2. Architecture Overview

### 2.1 System Design

LocusQ uses a **single binary, three-mode** architecture:

| Mode | Input | Output | Purpose |
|------|-------|--------|---------|
| Calibrate | 1ch mic | 4ch speakers | Room measurement and profile generation |
| Emitter | 1-2ch track audio | 1-2ch passthrough | Publishes spatial state + audio to SceneGraph |
| Renderer | SceneGraph (N emitters) | 1/2/4ch host layout | Spatializes all emitters to speaker output |

This is an unusual and effective design for a DAW plugin. The separation of spatial metadata publishing (Emitter) from rendering (Renderer) allows N-to-1 spatial mixing within a single DAW session using standard routing.

### 2.2 SceneGraph Singleton

The SceneGraph (`Source/SceneGraph.h`) is the central data exchange:

- **Meyer's singleton** — process-local, created on first access
- **256 EmitterSlots** — each double-buffered with atomic swap for lock-free write/read
- **Registration** — uses SpinLock (non-audio-thread operation)
- **Room profile** — atomic pointer swap, written by calibration background thread
- **Audio fast path** — emitter publishes raw audio buffer pointer valid only within current `processBlock`

**Assessment**: The double-buffer design is correct for the use case. The 256-slot limit is generous for v1 but the hard per-block rendering budget of 8 emitters provides a practical ceiling. The ephemeral audio buffer pointer sharing relies on DAW processing order (emitters before renderer), which holds for standard track→master routing but is not guaranteed by the VST3 spec. This is documented as a critical risk and the implementation degrades gracefully when the assumption breaks.

### 2.3 Component Hierarchy

```
LocusQAudioProcessor (juce::AudioProcessor)
├── SceneGraph& (singleton reference)
├── SpatialRenderer
│   ├── VBAPPanner
│   ├── DistanceAttenuator
│   ├── SpreadProcessor
│   ├── DirectivityFilter
│   ├── AirAbsorption[64]
│   ├── DopplerProcessor[64]
│   ├── EarlyReflections
│   └── FDNReverb
├── CalibrationEngine
│   ├── TestSignalGenerator
│   ├── IRCapture
│   └── RoomAnalyzer
├── PhysicsEngine (worker thread)
└── KeyframeTimeline

LocusQAudioProcessorEditor (juce::AudioProcessorEditor, juce::Timer)
├── ~100 Parameter Relays
├── WebBrowserComponent (Three.js UI)
└── ~100 Parameter Attachments
```

### 2.4 Thread Model

| Thread | Components | Safety Mechanism |
|--------|-----------|-----------------|
| Audio thread | processBlock, SpatialRenderer, emitter publish | Lock-free atomics, no allocation |
| UI timer thread | Editor timer callback, WebView JSON push | SpinLock for UI state |
| Physics thread | PhysicsEngine worker | Double-buffered atomic state |
| Calibration thread | CalibrationEngine analysis | Atomic state machine + background worker |
| WebView thread | JavaScript UI, bridge callbacks | JUCE message thread marshalling |

**Assessment**: Thread safety is well-considered. The audio thread path is confirmed allocation-free through QA instrumentation (`perf_allocation_free=true`). The physics engine uses proper double-buffering. The calibration engine's state machine uses atomics correctly.

---

## 3. Source Code Review

### 3.1 PluginProcessor (`Source/PluginProcessor.h/cpp`)

**Size**: ~88KB implementation, ~8KB header
**Responsibilities**: Mode switching, parameter tree (~140 parameters), SceneGraph registration, delegation to CalibrationEngine/SpatialRenderer/PhysicsEngine, performance telemetry, preset/snapshot serialization with layout migration.

**Observations**:
- The `processBlock()` method correctly delegates by mode with no allocation, locks, or blocking I/O.
- Mode-transition registration sync (`syncSceneGraphRegistrationForMode`) properly prevents stale emitter-slot reads during pluginval-style rapid automation.
- Snapshot migration (`setStateInformation`) handles legacy/mismatched layout metadata correctly with deterministic remap.
- Performance telemetry (EMA of block time, emitter publish time, renderer time) provides operational visibility.

**Concern**: At ~88KB, this file handles too many responsibilities. Parameter layout creation, mode delegation, snapshot serialization, scene-state JSON generation, and calibration routing auto-detection are all in one file. Extracting parameter layout and state serialization into separate translation units would improve maintainability.

### 3.2 PluginEditor (`Source/PluginEditor.h/cpp`)

**Size**: ~44KB implementation, ~13KB header
**Responsibilities**: WebView lifecycle, ~100 relay/attachment pairs, native bridge function bindings, resource serving, timer-driven scene state push.

**Observations**:
- **Critical member ordering** is correctly documented and implemented (relays → WebBrowserComponent → attachments) to prevent DAW crashes during teardown.
- The resource-serving pattern (`getResource`) serves embedded HTML/JS/CSS from the plugin binary.
- Native bridge functions handle calibration, timeline, preset, and UI state operations.
- Timer callback serializes scene state to JSON and pushes to WebView at a fixed rate.

**Concern**: The 100+ relay/attachment declarations create significant boilerplate. A table-driven or macro-based approach could reduce the risk of relay/attachment mismatch bugs while keeping the same runtime behavior.

### 3.3 DSP Components (Header-Only)

All DSP processors are implemented as header-only classes:

| File | Lines | Purpose |
|------|-------|---------|
| `SpatialRenderer.h` | ~625 | Main renderer orchestration |
| `VBAPPanner.h` | ~180 | 2D/3D VBAP for quad layout |
| `DistanceAttenuator.h` | ~70 | Three distance models |
| `AirAbsorption.h` | ~65 | Distance-driven 1-pole LPF |
| `DopplerProcessor.h` | ~80 | Variable-delay pitch shift |
| `DirectivityFilter.h` | ~60 | Cardioid shaping |
| `SpreadProcessor.h` | ~25 | Focused-to-diffuse blend |
| `EarlyReflections.h` | ~110 | Multi-tap delay network |
| `FDNReverb.h` | ~140 | 4×4 Hadamard FDN |
| `CalibrationEngine.h` | ~370 | Room measurement state machine |
| `TestSignalGenerator.h` | ~210 | Sweep/noise/impulse generation |
| `IRCapture.h` | ~170 | FFT-based IR deconvolution |
| `RoomAnalyzer.h` | ~230 | IR analysis (delay, level, freq, RT60) |
| `SceneGraph.h` | ~260 | Process-wide singleton |
| `PhysicsEngine.h` | ~370 | Double-buffered physics simulation |
| `KeyframeTimeline.h` | ~75 | Animation data model |
| `KeyframeTimeline.cpp` | ~200 | Animation interpolation |

**Assessment**: The header-only approach works for small, self-contained DSP units (VBAPPanner, DistanceAttenuator) but becomes problematic for larger classes (SpatialRenderer, CalibrationEngine, PhysicsEngine). Any change to these headers triggers recompilation of every translation unit that includes them. Moving implementations to `.cpp` files for the larger classes would significantly improve build times.

### 3.4 Parameter Management

Parameters are defined via `juce::AudioProcessorValueTreeState::ParameterLayout` with ~140 entries across mode-prefixed groups (cal_, pos_, emit_, phys_, anim_, rend_). The WebView bridge uses JUCE's WebSliderRelay/WebToggleButtonRelay/WebComboBoxRelay pattern for bidirectional parameter synchronization.

**Assessment**: Parameter IDs are stable and spec-aligned per invariants. The relay/attachment pattern is the correct JUCE 8 approach for WebView parameter binding. The parameter count is high but well-organized by mode prefix.

---

## 4. DSP Chain Assessment

### 4.1 Renderer Processing Pipeline

The renderer processes up to 8 emitters per block (hard budget) through this chain:

```
Per-emitter:
  1. Priority selection (gain × distance attenuation)
  2. Activity gate (peak > -120 dB)
  3. VBAP panning (2D quad, optional 3D elevation)
  4. Spread processing (focused ↔ diffuse blend)
  5. Directivity filter (cardioid shaping)
  6. Doppler processing (variable delay)
  7. Air absorption (1-pole LPF)
  8. 20ms gain ramp (click-free)

Post-accumulation:
  9. Early reflections (8/16 taps by quality)
  10. FDN reverb (4×4 Hadamard)
  11. Per-speaker delay compensation (≤50ms)
  12. Per-speaker gain trim
  13. Master gain with smoothing
  14. Output mapping (quad→stereo/mono downmix)
```

**Assessment**: The chain order is correct and follows established spatial audio practice. The per-block emitter budget (8) with priority gating is a practical guardrail verified by QA evidence (p95 block time 0.32ms at 48kHz/512, allocation-free).

### 4.2 DSP Quality Notes

**VBAP**: Standard 2D implementation with correct enclosing-pair detection and inverse matrix solve. 3D elevation uses a cosine-blend attenuation model rather than true 3D VBAP triangulation — adequate for quad but will need revision for higher-order speaker arrays.

**Distance Models**: All three models (inverse square, linear, logarithmic) correctly clamp to [0,1] with reference/max distance boundaries. The logarithmic model matches Web Audio API conventions.

**Air Absorption**: Simple but effective 1-pole design. The coefficient `cutoff = maxCutoff / (1 + distance * absorptionFactor)` provides a reasonable perceptual approximation. A more physically accurate model would use frequency-dependent absorption bands, but the current approach is appropriate for v1.

**Doppler**: Linear interpolation on a circular delay buffer. This introduces slight artifacts at extreme velocities due to first-order interpolation. Cubic or Lagrange interpolation would improve quality at minimal cost.

**FDN Reverb**: The 4×4 Hadamard matrix is orthogonal and energy-preserving. Delay lengths are coprime, which is correct for avoiding modal resonance. The RT60-derived feedback gains are properly clamped to [0.2, 0.93] to prevent runaway. The one-pole damping filter provides basic high-frequency rolloff. A more sophisticated approach would use per-channel damping with different cutoffs.

**Early Reflections**: Tap times are well-distributed (prime-like spacing). The exponential decay model `0.72^(i+1)` provides reasonable room character. The draft/high-quality tier split (8/16 taps) is a sensible cost/quality tradeoff.

### 4.3 Calibration System

The calibration pipeline (TestSignalGenerator → IRCapture → RoomAnalyzer → RoomProfileSerializer) is architecturally sound:

- **Sweep design**: Farina exponential sweep with pre-computed inverse filter is the industry standard approach.
- **Deconvolution**: Overlap-save FFT convolution is correct. The frequency-dependent amplitude correction on the inverse filter prevents spectral coloring.
- **Analysis**: Delay detection (>10% peak threshold), gain trim (RMS in 10ms window), and Schroeder RT60 estimation are standard methods.
- **Serialization**: JSON v1 schema with room dimensions, listener position, and per-speaker profiles is adequate and extensible.

**Concern**: The calibration system runs its analysis on a dedicated background thread, which is correct, but error recovery from failed measurements (e.g., ambient noise contamination, speaker not producing sound) relies on the UI displaying status messages. More robust automatic detection of measurement quality (SNR threshold, correlation check) would improve reliability.

### 4.4 Physics Engine

The physics engine runs on a dedicated timer thread at configurable rates (30/60/120/240 Hz) with Euler integration. The double-buffered state model with atomic swap is correct for cross-thread position sharing.

**Assessment**: Euler integration is adequate for the simple force models in v1 (gravity, drag, friction, wall collision). The AABB collision with coefficient of restitution (elasticity) and tangential friction is correctly implemented. The "physics as offset from animation rest position" model (per ADR-0003) is a good design choice that prevents animation/physics conflicts.

**Concern**: At higher physics tick rates (240 Hz), the Euler integrator may introduce visible energy drift in long-running simulations. A Verlet or semi-implicit Euler scheme would provide better energy conservation with negligible additional cost.

---

## 5. UI and WebView Layer

### 5.1 Architecture

The UI uses JUCE's `WebBrowserComponent` hosting a Three.js-based 3D viewport with HTML/CSS control panels. Data flow is bidirectional:

- **C++ → WebView**: Timer-driven JSON scene snapshots (30-60fps) via `evaluateJavascript`
- **WebView → C++**: Parameter changes via relay system, commands via native bridge functions

### 5.2 Incremental Stage System

The UI was developed through 12 incremental stages, each adding control surface coverage:

| Stage | Coverage |
|-------|----------|
| Stage 2 | Three.js viewport, mode tabs, adaptive rail widths |
| Stage 3-4 | Emitter audio controls (gain, mute, solo, spread, directivity) |
| Stage 5 | Renderer core controls |
| Stage 6-8 | Calibrate controls and capture progress |
| Stage 9 | Emitter identity/position/size/physics/animation/preset |
| Stage 10 | Renderer rail parity |
| Stage 11 | Calibrate parity |
| Stage 12 | Visual polish, debug gate, primary route |

**Assessment**: The incremental approach was effective for development but has left 12 HTML/JS stage files in the codebase. Stage 12 is the production route, but earlier stages remain as fallback resource paths. This creates confusion about which file is canonical. A post-GA cleanup to remove or archive pre-production stages would reduce cognitive load.

### 5.3 Native Bridge Functions

11 native bridge functions provide structured command/ack paths:

| Function | Purpose |
|----------|---------|
| `locusqStartCalibration` | Begin room measurement |
| `locusqAbortCalibration` | Cancel measurement |
| `locusqRedetectCalibrationRouting` | Auto-detect speaker channels |
| `locusqGetKeyframeTimeline` | Retrieve animation data |
| `locusqSetKeyframeTimeline` | Update animation data |
| `locusqSetTimelineTime` | Set playhead position |
| `locusqListEmitterPresets` | List saved presets |
| `locusqSaveEmitterPreset` | Save emitter state |
| `locusqLoadEmitterPreset` | Load emitter preset |
| `locusqGetUiState` | Retrieve persisted UI state |
| `locusqSetUiState` | Store UI state |

**Assessment**: The bridge surface is well-defined with clear command/ack semantics. Input validation (finite/clamped timeline time, schema checks on presets) is present. The UI-state persistence bridge (`locusqGetUiState`/`locusqSetUiState`) correctly separates UI-only state from APVTS parameters.

### 5.4 WebView Resilience

The UI follows a deterministic bootstrap contract:

```
BOOT_START → DOM_READY → BRIDGE_READY → CONTROL_BINDINGS_READY → VIEWPORT_READY (optional) → RUNNING
```

Controls remain functional even when the 3D viewport fails to initialize (degraded mode). This is a correct design for in-host reliability where WebGL may not always be available.

### 5.5 Remaining UI Gaps

- **Motion trails** and **velocity vectors** are deferred per ADR-0008.
- **Manual DAW host verification** remains an open gating item per Phase 2.7 acceptance.
- The self-test automation covers headless proxy scenarios but not real in-host click interactions.

---

## 6. Build System and Infrastructure

### 6.1 CMake Configuration

The project uses CMake 3.22+ with JUCE integration:

- **Plugin targets**: `LocusQ_VST3`, `LocusQ_AU`, `LocusQ_Standalone`
- **QA target**: `locusq_qa` (links against external `audio-dsp-qa-harness`)
- **JUCE discovery**: `-DJUCE_DIR`, env var, or sibling repo fallback
- **Build script**: `scripts/build-and-install-mac.sh` for macOS build/install with optional AU cache refresh

### 6.2 CI/CD Pipeline

GitHub Actions workflows:
- **qa_harness.yml**: Harness sanity, critical QA (2ch/4ch matrix), renderer CPU trend (48k/96k × 2ch/4ch), seeded pluginval stress (5 seeds × strictness 5)
- **docs-freshness.yml**: Validates documentation synchronization

### 6.3 QA Harness

The QA harness is a scenario-driven testing framework:
- **Scenario files**: JSON specs in `qa/scenarios/` defining stimulus parameters, DUT configuration, and acceptance gates
- **Adapter**: `qa/locusq_adapter.cpp/h` bridges the harness to LocusQ's processor
- **Evidence**: Machine-readable `qa_output/suite_result.json` plus per-run logs in `TestEvidence/`
- **Coverage**: Smoke (4 scenarios), phase acceptance suites (2.4, 2.5, 2.6), host-edge matrix (4 SR/BS combos), CPU guardrail stress (8/16 emitters), snapshot migration (mono/stereo/quad), output layout regression (mono/stereo/quad)

**Assessment**: The QA infrastructure is unusually thorough for an audio plugin project. The machine-readable evidence trail, validation trend logging, and docs freshness gate provide strong regression detection. The seeded pluginval stress testing (specific crash-reproducing seeds) is a valuable stability tool.

**Gap**: The QA harness operates at the DSP/host-proxy level. There is no automated test coverage for the WebView UI layer that exercises actual browser-based interactions. The self-test system runs within the WebView context but is driven by JavaScript, not external browser automation.

### 6.4 Platform Coverage

- **macOS**: Primary and only tested platform. Universal (x86_64 + arm64) build verified.
- **Windows**: CMake configuration includes Windows-specific flags (`webview2` backend, `NEEDS_WEB_BROWSER=TRUE`) but no build/test evidence exists.
- **Linux**: No explicit support or testing.

---

## 7. Quality Assurance and Testing

### 7.1 Test Evidence Summary (v1.0.0-ga)

| Gate | Result | Evidence |
|------|--------|----------|
| Harness ctest | 45/45 PASS | `TestEvidence/harness_ctest.log` |
| Smoke suite | 4/4 PASS | Multiple runs documented |
| Phase 2.4 physics | 2 PASS, 0 FAIL | Probe + spatial + zero-g |
| Phase 2.5 room acoustics | 9 PASS, 0 FAIL | Signal depth + CPU gates |
| Phase 2.6 full system | PASS (alloc-free) | avg 0.30ms, p95 0.32ms @ 48k/512 |
| Host-edge matrix | PASS | 44.1k/256, 48k/512, 48k/1024, 96k/512 |
| Pluginval strictness 5 | PASS | 10/10 stability probe |
| Output layout (mono/stereo/quad) | PASS | Regression suites |
| Snapshot migration | PASS | Legacy + mismatch suites |
| CPU guardrail (16 emitters) | PASS | avg 0.41ms, p95 0.43ms |
| Stage 12 self-test | PASS | UI PR gate |
| Docs freshness | 0 warnings | `validate-docs-freshness.sh` |

### 7.2 Open Test Items

1. **Manual DAW host verification** — The Phase 2.7 manual host UI acceptance checklist has not been fully executed. This covers tab focus, viewport drag, and calibration lifecycle in real DAW hosts.
2. **First CI run of seeded/quad lanes** — CI pipeline configured but first remote execution not captured.
3. **Windows/Linux validation** — No platform evidence outside macOS.

---

## 8. Documentation and Governance

### 8.1 ADR Registry

| ADR | Decision | Status |
|-----|----------|--------|
| ADR-0001 | Documentation governance | Accepted |
| ADR-0002 | V1 routing model (SceneGraph) | Accepted |
| ADR-0003 | Automation authority precedence | Accepted |
| ADR-0004 | V1 AI deferral | Accepted |
| ADR-0005 | Phase closeout docs freshness gate | Accepted |
| ADR-0006 | Device compatibility profiles | Accepted |
| ADR-0007 | Emitter directivity/velocity UI exposure | Accepted |
| ADR-0008 | Viewport scope v1 vs post-v1 | Accepted |

**Assessment**: The ADR practice is well-maintained. Each record has clear context, decision, rationale, consequences, and related documents. The ADR registry provides effective institutional memory for architectural decisions.

### 8.2 Documentation Assets

- **Architecture spec** (`.ideas/architecture.md`): Comprehensive, kept in sync with implementation
- **Parameter spec** (`.ideas/parameter-spec.md`): Maps parameters to components
- **Implementation plan** (`.ideas/plan.md`): Detailed phase breakdown with acceptance criteria
- **Invariants** (`Documentation/invariants.md`): Audio thread, scene graph, DSP chain, device compatibility, and traceability constraints
- **Scene-state contract** (`Documentation/scene-state-contract.md`): SceneGraph data exchange specification
- **Implementation traceability** (`Documentation/implementation-traceability.md`): Parameter-to-code mapping
- **Validation trend** (`TestEvidence/validation-trend.md`): Longitudinal QA evidence

### 8.3 Docs Freshness Gate

The `scripts/validate-docs-freshness.sh` script enforces synchronization across status.json, README, CHANGELOG, build-summary, and validation-trend. This is integrated into CI. The gate passed with 0 warnings at GA.

---

## 9. Identified Issues and Technical Debt

### 9.1 Critical (should address before next feature work)

| # | Issue | Impact | Location |
|---|-------|--------|----------|
| C1 | **Monolithic PluginProcessor.cpp** (~88KB) | Merge conflicts, slow navigation, high cognitive load | `Source/PluginProcessor.cpp` |
| C2 | **Manual DAW host verification incomplete** | Unproven in-host UI interaction for shipped v1 | Phase 2.7 acceptance |
| C3 | **No Windows build/test evidence** | Half the target market untested | Build infrastructure |

### 9.2 High (address in next development cycle)

| # | Issue | Impact | Location |
|---|-------|--------|----------|
| H1 | **Header-only DSP** for large classes | Long rebuild times, tight coupling | `SpatialRenderer.h`, `CalibrationEngine.h`, `PhysicsEngine.h` |
| H2 | **12 incremental UI stage files** in production | Confusing artifact trail, resource path fragility | `Source/ui/public/` |
| H3 | **Doppler linear interpolation** | Audible artifacts at extreme velocities | `DopplerProcessor.h` |
| H4 | **Euler integrator** for physics | Energy drift in long simulations | `PhysicsEngine.h` |
| H5 | **No automated WebView UI testing** | UI regressions detectable only manually | QA infrastructure |

### 9.3 Medium (address opportunistically)

| # | Issue | Impact | Location |
|---|-------|--------|----------|
| M1 | **Boilerplate relay/attachment declarations** (~100 pairs) | Error-prone, hard to maintain | `PluginEditor.h/cpp` |
| M2 | **Fixed quad speaker layout** in VBAP | Cannot support other arrangements | `VBAPPanner.h` |
| M3 | **Single-pole air absorption** | Limited physical accuracy | `AirAbsorption.h` |
| M4 | **4×4 FDN only** | No higher-order option for quality tier | `FDNReverb.h` |
| M5 | **No HRTF/binaural** headphone rendering | Stereo output is simple downmix | `SpatialRenderer.h` |
| M6 | **No inter-emitter physics** beyond basic force | ADR-0004 defers advanced simulation | `PhysicsEngine.h` |
| M7 | **No Linux support** | Smaller but growing market segment | Build infrastructure |

### 9.4 Low (future enhancement)

| # | Issue | Impact | Location |
|---|-------|--------|----------|
| L1 | Missing motion trails in viewport | ADR-0008 deferred | UI layer |
| L2 | Missing velocity vectors in viewport | ADR-0008 deferred | UI layer |
| L3 | No AI orchestration | ADR-0004 deferred | Architecture |
| L4 | No OSC/network inter-process routing | Single-DAW-process only | SceneGraph |

---

## 10. Forward-Looking Roadmap

### Phase 3.0: Post-GA Stabilization and Cleanup

**Goal**: Reduce technical debt, verify cross-platform, close open gating items.

#### 3.0.1 — Source Refactoring
- Extract parameter layout creation from `PluginProcessor.cpp` into `ParameterLayout.h/cpp`
- Extract state serialization/migration into `StateSerializer.h/cpp`
- Extract scene-state JSON generation into `SceneStateSerializer.h/cpp`
- Move large header-only implementations to `.cpp` files: `SpatialRenderer`, `CalibrationEngine`, `PhysicsEngine`, `SceneGraph`
- Introduce a table-driven relay/attachment registration in `PluginEditor` to replace manual boilerplate

#### 3.0.2 — UI Consolidation
- Archive incremental stage files (Stage 2-11) to a `Source/ui/archive/` directory
- Verify Stage 12 is the sole production entry point with no fallback resource paths
- Add build-time validation that the resource list matches the canonical UI file set

#### 3.0.3 — Platform Expansion
- Establish Windows build and test pipeline (WebView2 backend)
- Run full QA harness + pluginval on Windows
- Evaluate Linux support scope (LV2 target, webkit2gtk backend)

#### 3.0.4 — Manual DAW Verification Closure
- Execute full Phase 2.7 manual host UI acceptance checklist in:
  - REAPER (macOS)
  - Logic Pro (macOS)
  - Ableton Live (macOS)
  - At least one Windows host (post Windows build)
- Document results in TestEvidence with explicit pass/fail per check

### Phase 3.1: DSP Quality Improvements

**Goal**: Address DSP quality gaps identified in review.

#### 3.1.1 — Doppler Processor Upgrade
- Replace linear interpolation with cubic (Hermite or Lagrange) on the delay line
- Add configurable doppler smoothing to prevent zipper noise
- Validate with focused QA scenario at extreme velocities

#### 3.1.2 — Physics Integrator Upgrade
- Replace Euler with semi-implicit Euler (Symplectic Euler) for better energy conservation
- Add optional Verlet integration mode for advanced physics presets
- Validate energy conservation in long-running zero-gravity drift scenario

#### 3.1.3 — Air Absorption Enhancement
- Add multi-band absorption model (low/mid/high) for more physically accurate distance filtering
- Keep single-pole as "draft" quality, multi-band as "final" quality tier

#### 3.1.4 — FDN Reverb Enhancement
- Add 8×8 FDN option for "final" quality tier
- Add per-channel damping with different cutoff frequencies
- Add modulated delay lines for reduced metallic coloring

### Phase 3.2: HRTF / Binaural Rendering

**Goal**: Deliver proper headphone monitoring (currently deferred per ADR-0006).

#### 3.2.1 — HRTF Integration
- Integrate a public HRTF dataset (MIT KEMAR or SADIE II)
- Implement per-emitter HRTF convolution as alternative to VBAP for headphone output
- Add headphone mode selector in renderer parameters
- Maintain canonical scene model (no separate parameter schema per ADR-0006)

#### 3.2.2 — Binaural Quality Tiers
- Draft: minimum-phase HRTF (lower latency, reduced spatial accuracy)
- Final: linear-phase HRTF with ITD modeling

### Phase 3.3: Flexible Speaker Layouts

**Goal**: Extend beyond fixed quad to support arbitrary speaker configurations.

#### 3.3.1 — Configurable Speaker Array
- Replace hardcoded 4-speaker VBAP with configurable N-speaker support
- Support common layouts: stereo, LCR, quad, 5.1, 7.1, 7.1.4, custom
- Use 3D VBAP triangulation (Delaunay) for layouts with elevation speakers
- Update calibration system to support variable speaker counts

#### 3.3.2 — Dolby Atmos / Apple Spatial Audio Compatibility
- Add Atmos-compatible bed/object metadata output
- Investigate Apple Spatial Audio renderer integration
- Add 7.1.4 layout preset per Three.js skill references

### Phase 3.4: Advanced Physics and Simulation

**Goal**: Implement v2 physics features (currently deferred per ADR-0004).

#### 3.4.1 — Inter-Object Forces
- Attractors and repulsors between emitters
- Configurable force field (per-emitter or global)
- Spring connections between emitter pairs

#### 3.4.2 — Flocking / Boids
- Separation, alignment, cohesion forces
- Configurable group behavior presets
- Per-emitter flocking membership

#### 3.4.3 — Flow Fields
- 3D vector field definition (procedural or imported)
- Emitters follow flow streamlines
- Turbulence and vortex generation

### Phase 3.5: AI Orchestration (Post-Deterministic Core)

**Goal**: Introduce AI-assisted spatial scene control (deferred per ADR-0004).

#### 3.5.1 — Scene Suggestion Engine
- LLM-assisted spatial arrangement from text descriptions
- Proposals rendered as preview overlays (not auto-applied)
- Must not bypass deterministic scene-state contracts

#### 3.5.2 — Motion Generation
- AI-driven keyframe generation from musical analysis
- Beat-sync and phrase-sync motion patterns
- User approval workflow before timeline mutation

#### 3.5.3 — Acoustic Analysis Integration
- AI-assisted room profile interpretation
- Suggested speaker placement optimization
- Calibration quality assessment and recommendations

### Phase 3.6: Network and Multi-Process Routing

**Goal**: Extend SceneGraph beyond single-process limitation.

#### 3.6.1 — OSC Bridge
- Expose SceneGraph read/write via OSC protocol
- Enable multi-machine spatial audio control
- Maintain canonical state authority in the primary process

#### 3.6.2 — IPC/Shared Memory
- Optional shared-memory SceneGraph for multi-process DAW hosts
- Backward-compatible with single-process singleton model

### Phase 3.7: UI Enhancement and Visualization

**Goal**: Complete deferred UI features and expand visualization.

#### 3.7.1 — Motion Trails (ADR-0008 follow-up)
- Render emitter position history as fading trails
- Configurable trail length and fade rate
- Performance-gated (disable when frame budget exceeded)

#### 3.7.2 — Velocity Vectors (ADR-0008 follow-up)
- Render emitter velocity as directional arrows
- Scale with speed magnitude
- Color-coded by physics state

#### 3.7.3 — Advanced Viewport
- Room acoustic visualization (reflection paths, decay overlay)
- Speaker directivity patterns
- Frequency-domain visualization (per-emitter spectrum in 3D)

---

## 11. Risk Assessment

### Roadmap Risks

| Phase | Risk | Mitigation |
|-------|------|------------|
| 3.0 Refactoring | Regressions from large-scale restructuring | Comprehensive QA harness provides safety net; incremental file-by-file extraction |
| 3.1 DSP Quality | Audible changes break user expectations | A/B testing with reference renders; togglable improvements |
| 3.2 HRTF | Large dataset integration, licensing | Use MIT/Creative Commons HRTF sets; lazy-load to manage memory |
| 3.3 Speaker Layouts | Breaking change to VBAP/calibration assumptions | Feature-flag new layouts; keep quad as default; migration path for presets |
| 3.4 Physics | CPU budget for N² inter-emitter forces | Spatial partitioning (octree); culling beyond interaction radius |
| 3.5 AI | Non-deterministic behavior violating invariants | Preview-only model; all mutations require user confirmation |
| 3.6 Network | Latency and state consistency across processes | Clock synchronization; conflict resolution protocol |

### Architecture Limits to Watch

1. **256 EmitterSlot hard ceiling**: Sufficient for foreseeable use but would need redesign for very large installations.
2. **Single Renderer assumption**: The architecture enforces one renderer per process. Multi-renderer scenarios (e.g., multiple speaker arrays in one session) would need SceneGraph redesign.
3. **Audio buffer pointer sharing**: The ephemeral same-block fast path relies on DAW processing order. If a host processes renderer before emitters, the fast path silently degrades. This is acceptable for v1 but should be monitored across hosts.

---

## 12. Recommendations Summary

### Immediate (before next feature work)

1. Execute the manual DAW host verification checklist (C2).
2. Begin `PluginProcessor.cpp` decomposition into focused translation units (C1).
3. Establish Windows build pipeline and run the QA harness (C3).

### Near-Term (next development cycle)

4. Move large header-only implementations to `.cpp` files (H1).
5. Archive incremental UI stages and consolidate to a single entry point (H2).
6. Upgrade Doppler interpolation to cubic (H3).
7. Upgrade physics integrator to semi-implicit Euler (H4).
8. Investigate browser-based UI test automation (Playwright/Puppeteer against standalone) (H5).

### Medium-Term (next 2-3 cycles)

9. Implement HRTF binaural rendering for headphone output (Phase 3.2).
10. Extend VBAP to configurable N-speaker layouts (Phase 3.3).
11. Implement inter-emitter physics forces (Phase 3.4.1).
12. Add Linux LV2 target (M7).

### Long-Term (strategic)

13. AI scene orchestration (Phase 3.5) — after deterministic core is battle-tested.
14. OSC/network routing (Phase 3.6) — when multi-machine use cases emerge.
15. Advanced viewport visualization (Phase 3.7) — progressive enhancement.

---

## Appendix A: File Inventory

### Core Source Files (Source/)

| File | Size | Role |
|------|------|------|
| `PluginProcessor.h` | ~8KB | Processor declarations |
| `PluginProcessor.cpp` | ~88KB | Processor implementation |
| `PluginEditor.h` | ~13KB | Editor declarations |
| `PluginEditor.cpp` | ~44KB | Editor implementation |
| `SpatialRenderer.h` | ~25KB | Renderer DSP engine |
| `VBAPPanner.h` | ~7KB | VBAP panning |
| `DistanceAttenuator.h` | ~3KB | Distance models |
| `AirAbsorption.h` | ~3KB | Air absorption filter |
| `DopplerProcessor.h` | ~3KB | Doppler processing |
| `DirectivityFilter.h` | ~2.5KB | Directivity shaping |
| `SpreadProcessor.h` | ~1KB | Spread blend |
| `EarlyReflections.h` | ~4.5KB | Early reflections |
| `FDNReverb.h` | ~5.5KB | FDN reverb |
| `CalibrationEngine.h` | ~15KB | Room measurement |
| `TestSignalGenerator.h` | ~8KB | Test signals |
| `IRCapture.h` | ~7KB | IR capture/deconvolution |
| `RoomAnalyzer.h` | ~9KB | Room analysis |
| `RoomProfileSerializer.h` | ~6KB | Room profile JSON |
| `SceneGraph.h` | ~11KB | Singleton scene graph |
| `PhysicsEngine.h` | ~15KB | Physics simulation |
| `KeyframeTimeline.h` | ~3KB | Timeline data model |
| `KeyframeTimeline.cpp` | ~8KB | Timeline interpolation |

### Documentation Artifacts

| Category | Count |
|----------|-------|
| ADRs | 8 |
| Spec documents (.ideas/) | 4 |
| Documentation/ | 10+ docs |
| TestEvidence/ | 60+ log files |
| QA scenarios | 25+ JSON specs |

### ADR Decision Summary

| ADR | Key Decision |
|-----|-------------|
| 0001 | Doc governance with metadata requirements |
| 0002 | SceneGraph canonical metadata + ephemeral audio fast path |
| 0003 | DAW/APVTS → timeline rest → physics offset precedence |
| 0004 | No AI in v1 critical path |
| 0005 | Docs freshness gate on phase closeout |
| 0006 | Device profiles (quad/laptop/headphone) |
| 0007 | Emit directivity/velocity UI exposure |
| 0008 | Viewport trails/vectors deferred to post-v1 |

---

## Appendix B: Validation Evidence Cross-Reference

| Metric | Value | Source |
|--------|-------|--------|
| Harness tests | 45/45 pass | `TestEvidence/harness_ctest.log` |
| Smoke suite | 4/4 pass | Multiple log files |
| Full-system CPU (8E, 48k/512, draft) | avg 0.30ms, p95 0.32ms | Phase 2.6c evidence |
| Full-system CPU (16E, 48k/512, draft) | avg 0.41ms, p95 0.43ms | Phase 2.10 evidence |
| Pluginval strictness 5 | 10/10 stability | Post-fix probe |
| Host-edge matrix | 4/4 SR/BS pass | Phase 2.6 evidence |
| Output layout regression | mono/stereo/quad pass | Phase 2.8 evidence |
| Snapshot migration | mono/stereo/quad pass | Phase 2.11b evidence |
| Allocation-free audio path | confirmed | `perf_allocation_free=true` |
| Stage 12 self-test | PASS | UI PR gate evidence |
| Docs freshness gate | 0 warnings | `validate-docs-freshness.sh` |

---

*End of review.*
