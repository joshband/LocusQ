Title: BL-029 DSP Visualization and Tooling Spec + Implementation Plan
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# BL-029 DSP Visualization and Tooling Spec + Implementation Plan

## Purpose
Define one cohesive, implementation-ready contract for the next LocusQ visualization/tooling tranche across three runtime modes (`CALIBRATE`, `EMITTER`, `RENDERER`), covering four priorities:
1. Deterministic modulation visualizer.
2. Spectral-spatial hybrid room view.
3. Reflection ghost modeling.
4. Offline ML calibration assistant.

## Backlog Link
- Proposed Backlog ID: `BL-029`
- Canonical backlog file: `Documentation/backlog-post-v1-agentic-sprints.md`

## Normative Inputs
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- `Documentation/scene-state-contract.md`
- `Documentation/implementation-traceability.md`
- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Skill Routing Used
Execution composition for this spec:
1. `skill_docs`
2. `skill_dream`
3. `skill_plan`
4. `skill_design`
5. `skill_impl`
6. `juce-webview-runtime`
7. `physics-reactive-audio`
8. `reactive-av`
9. `spatial-audio-engineering`
10. `steam-audio-capi`
11. `threejs`

## Product Intent (Dream Contract)
LocusQ should move from “position-only spatial UI” to “behavior-first spatial instrumentation.”

Operator value:
1. Show what was requested versus what was actually applied in DSP.
2. Show spectral and transient behavior in the same spatial frame.
3. Show first-order room reflection structure as visible geometry.
4. Turn calibration into a reproducible assist loop (export -> analyze offline -> apply recommendation), without runtime ML risk.

## Problem Statement
Current runtime already exposes strong spatial telemetry, but there is no unified introspection layer for:
1. Base parameter intent vs applied runtime truth.
2. Spectral behavior per emitter inside the room model.
3. Perceptual reflection bias and early-reflection structure.
4. Calibration recommendation workflow with versioned offline assistant contracts.

Resulting gap: users can hear changes but cannot deterministically inspect the cause/effect chain.

## Scope
In scope:
1. New trace transport and targets for deterministic visualization.
2. Emitter spectral features and flux-driven visual mapping.
3. First-order reflection ghosts and reflection-adjusted centroid diagnostics.
4. Offline session export, recommendation generation, and apply path.
5. Host/runtime-safe WebView bridge additions with backend-aware QA.

Out of scope:
1. Audio-thread ML inference.
2. Full acoustic simulation (diffraction, frequency-dependent multi-bounce physics).
3. Breaking APVTS parameter ID renames.
4. New codec/format export workflows (ADM/IAMF implementation).

## Mode-by-Mode Architecture Map

### CALIBRATE
Primary ownership:
1. Capture/measure/feature status from calibration engine.
2. Export deterministic session bundle.
3. Apply recommendation payload into renderer parameters.

Key runtime/code surfaces:
1. `LocusQAudioProcessor::startCalibrationFromUI(...)`
2. `LocusQAudioProcessor::getCalibrationStatus()`
3. New: `exportCalibrationSessionFromUI(...)`
4. New: `applyCalibrationRecommendationFromUI(...)`
5. Web bridge native functions in `PluginEditor`.

### EMITTER
Primary ownership:
1. Publish motion/audio truth state.
2. Trace base vs applied state for emitter and physics-derived signals.
3. Compute spectral/spatial hybrid metadata for UI.
4. Compute reflection ghosts and reflection-adjusted centroid.

Key runtime/code surfaces:
1. `LocusQAudioProcessor::publishEmitterState(...)` (truth-point tap)
2. `LocusQAudioProcessor::getSceneStateJSON()` (message-thread feature and ghost computation)
3. JS `updateSceneState(...)`, emitter mesh styling, constellation mode.

### RENDERER
Primary ownership:
1. Publish renderer-applied parameter truth traces.
2. Publish energy centroid from actual output energy distribution.
3. Preserve profile/stage diagnostics and deterministic fallback visibility.

Key runtime/code surfaces:
1. `LocusQAudioProcessor::updateRendererParameters()` (truth-point tap)
2. `LocusQAudioProcessor::processBlock(...)` (energy centroid computation)
3. `SpatialRenderer` diagnostics fields already exposed in scene snapshot.

## Priority 1: Deterministic Modulation Visualizer

### Goal
Expose “base vs applied” control truth for selected targets with bounded overhead and deterministic ordering.

### Contract
1. Trace capture point must be at DSP truth usage, not UI intent listener.
2. Transport is lock-free SPSC ring buffer (audio producer, UI consumer).
3. Phase 1 cadence is block-accurate truth; per-sample decimation is later optional.
4. Overflow policy is explicit and documented (drop-newest or overwrite-oldest).

### Data Schema
`locusq-mod-trace-v1`:
1. `target` (enum id)
2. `sampleRate`
3. `points[]` where each point has:
   - `t` (sample index)
   - `base`
   - `applied`

### Target Sets
1. Emitter/motion targets: `PosX/Y/Z`, `Azimuth`, `Elevation`, `Distance`, `SizeUniform`, `EmitGain`, `EmitSpread`, `EmitDirectivity`.
2. Physics-derived targets: `PhysicsVelocityMag`, `PhysicsForceMag`, `CollisionEnergy`.
3. Renderer targets: master/distance/doppler/room/speaker trims and delays.
4. Renderer state-resolution targets: requested vs active profile/mode + Steam availability.
5. Output-perception target: `EnergyCentroidX/Y/Z`.

### Native Bridge
1. `locusqSetTraceTarget(target)`
2. `locusqGetModTrace(maxPoints)`
3. Optional push path in `timerCallback`: `updateModTrace(payload)`.

### UI Rendering Baseline
1. Line A: base value.
2. Line B: applied value.
3. Derived-only targets render applied line only.
4. Enum/state targets use step rendering and legend labels.

## Priority 2: Spectral-Spatial Hybrid Room View

### Goal
Map per-emitter timbral behavior into the spatial viewport and a constellation lens without touching audio-thread safety.

### Feature Extraction Contract
Compute on message thread only (inside scene snapshot build):
1. `centroidHz`
2. `rolloffHz` (85%)
3. `hfRatio` (above configurable split, baseline 4kHz)
4. `flux` and `fluxEma` from positive spectral difference against cached previous magnitude bins.

### Cache Contract
Per-emitter cache (`EmitterSpectralCache`):
1. `hasPrev`
2. `prevMag[256]` (FFT-512 bins)
3. `fluxEma`

### JSON Additions (Per Emitter)
1. `centroidHz`
2. `centroidNorm`
3. `rolloffHz`
4. `hfRatio`
5. `flux`
6. `fluxEma`
7. `fluxNorm`

### UI Contracts
Room view:
1. Position remains world-space emitter position.
2. Color ties to `centroidNorm`/`hfRatio`.
3. Halo/trail intensity ties to `fluxNorm`.
4. Preserve quality tiers for constrained hosts.

Constellation view:
1. X: `centroidNorm`
2. Y: `hfRatio`
3. Z: `fluxNorm`
4. Stable emitter identity labels/tooltips.

## Priority 3: Reflection Ghost Modeling

### Goal
Render first-order reflection image sources and expose early-reflection spatial bias.

### Geometry Contract
Rectangular room, first-order image sources for:
1. Left wall
2. Right wall
3. Front wall
4. Back wall
5. Floor
6. Ceiling

### Reflection Weights
Per-ghost gain uses:
1. Surface coefficient (`wall`, `floor`, `ceiling` baselines).
2. Room mix scaling.
3. Damping-to-HF absorption factor weighted by emitter spectral brightness.
4. Inverse-distance-squared weight with epsilon.
5. Early-window gate (`delayMs <= earlyMsMax`, room-size-scaled).

### JSON Additions (Per Emitter)
1. `reflections[]` entries:
   - `x`, `y`, `z`
   - `delayMs`
   - `gain`
   - `hfAbsorb`
   - `brightness`
2. `erCentroid` (`x`,`y`,`z`) including direct path + valid ghosts.
3. `erShift` (distance between direct emitter position and ER centroid).

### UI Contracts
1. Ghosts inherit emitter hue with reduced saturation and delay/gain-based opacity.
2. Damping should visibly dim ghost brightness, especially for bright emitters.
3. Optional vector from emitter -> `erCentroid` to show reflection bias.

## Priority 4: Offline ML Calibration Assistant

### Goal
Create a deterministic offline recommendation loop with no runtime inference in plugin audio paths.

### Session Export Contract
Session bundle directory:
1. `session.json` (`locusq-calibration-session-v1`)
2. `features.json` (`locusq-calibration-features-v1`)
3. Optional `ir_spkN.wav` assets

### Recommendation Contract
`locusq-calibration-recommendation-v1` contains:
1. Per-speaker:
   - `gainTrimDb`
   - `delayMs`
2. Room:
   - `enable`
   - `mix`
   - `size`
   - `damping`
   - `earlyReflectionsOnly`
3. Evidence summary block (arrival/level/rt60) for auditability.

### Apply Contract
1. Parse and schema-validate recommendation payload.
2. Apply via APVTS `setValueNotifyingHost` on supported IDs.
3. Reject invalid schema/paths safely and return structured failure to UI.

### Offline Analyzer Contract
Baseline (non-ML) deterministic script:
1. Input: exported `features.json`.
2. Output: `recommendation.json` with heuristic mapping.

ML upgrade path:
1. Synthetic dataset generator (image-source IR baseline).
2. Lightweight regression model for room params (`mix`, `size`, `damping`).
3. Keep delays/gain trims deterministic from measured features.

## Threading and Realtime Safety Rules
1. No allocation/locks/blocking I/O in `processBlock()`.
2. Scene-feature and ghost computation occurs on message thread snapshot path only.
3. Bridge callbacks must never mutate DSP graph shape directly.
4. All mapping values are clamped finite before render or DSP use.

## JUCE WebView Runtime and Host Matrix Contracts
1. Validate both `WKWebView` and `WebView2` for bridge timing and UI update cadence.
2. Preserve existing native function names unless explicit migration spec is approved.
3. Keep startup ordering deterministic: bridge availability checks before UI-dependent updates.
4. If native calls fail, UI must degrade with explicit status chips, not silent failure.

Minimum host lanes:
1. Standalone (macOS).
2. REAPER VST3.
3. REAPER CLAP (when CLAP artifacts enabled).

## Spatial + Steam Audio Contracts
1. Requested vs active mode/profile/stage diagnostics remain mandatory in snapshots.
2. Steam availability/init stage remains explicit to explain binaural fallback.
3. Reflection/spectral visual layers are descriptive and must not claim changes in spatial DSP contract authority.

## Implementation Plan (Slices)

### Slice A: Trace Core (Emitter)
Files:
1. `Source/ModTrace.h` (new)
2. `Source/PluginProcessor.h`
3. `Source/PluginProcessor.cpp`
4. `Source/PluginEditor.cpp`

Deliverables:
1. Trace target enum and ring transport.
2. Emitter truth-point capture.
3. Bridge APIs + UI polling payload.

Acceptance:
1. `base != applied` visible for animation/physics overrides.

### Slice B: Trace Core (Renderer + State)
Files:
1. `Source/PluginProcessor.h`
2. `Source/PluginProcessor.cpp`
3. `Source/PluginEditor.cpp`

Deliverables:
1. Renderer truth-point targets.
2. Requested vs active state targets.
3. Steam availability state tracing.

Acceptance:
1. Deterministic state-step traces across profile/fallback transitions.

### Slice C: Spectral-Spatial Hybrid
Files:
1. `Source/PluginProcessor.h`
2. `Source/PluginProcessor.cpp`
3. `Source/ui/public/js/index.js`
4. `Source/ui/public/index.html`

Deliverables:
1. Magnitude extraction helper and spectral cache.
2. Flux fields in scene snapshot.
3. Flux-reactive room styling.

Acceptance:
1. Stable room-view styling and no UI jitter from un-smoothed flux.

### Slice D: Constellation View
Files:
1. `Source/ui/public/js/index.js`
2. `Source/ui/public/index.html`

Deliverables:
1. Constellation mode toggle and axis mapping.
2. Tooltips/labels for feature coordinates.

Acceptance:
1. Deterministic emitter placement in feature-space for same input stream.

### Slice E: Reflection Ghosts + ER Centroid
Files:
1. `Source/PluginProcessor.cpp`
2. `Source/ui/public/js/index.js`
3. `Documentation/scene-state-contract.md`

Deliverables:
1. First-order ghost geometry + weighted gains.
2. ER centroid + shift metrics.
3. Damping/spectral-linked ghost appearance.

Acceptance:
1. Room controls visibly and deterministically affect ghost intensity/bias.

### Slice F: Energy Centroid Trace Integration
Files:
1. `Source/PluginProcessor.h`
2. `Source/PluginProcessor.cpp`

Deliverables:
1. Output-energy centroid metrics in renderer path.
2. Trace targets for centroid axes.

Acceptance:
1. Centroid trace follows output-energy movement under known pan trajectories.

### Slice G: Calibration Session Export/Apply
Files:
1. `Source/PluginProcessor.h`
2. `Source/PluginProcessor.cpp`
3. `Source/PluginEditor.cpp`
4. `Source/ui/public/js/index.js`

Deliverables:
1. Session bundle export API.
2. Recommendation apply API.
3. UI actions with explicit success/failure surface.

Acceptance:
1. Exported bundle is schema-valid and replayable by analyzer script.

### Slice H: Offline Analyzer + Training Tooling
Files:
1. `tools/calibration_assistant/analyze_session.py`
2. `tools/calibration_assistant/synth/image_source_ir.py`
3. `tools/calibration_assistant/synth/feature_extract.py`
4. `tools/calibration_assistant/synth/generate_dataset.py`
5. `tools/calibration_assistant/train_model.py`

Deliverables:
1. Deterministic heuristic recommender.
2. Synthetic dataset generator.
3. Optional regression model export contract.

Acceptance:
1. Analyzer emits valid recommendation JSON consumed by Slice G apply path.

## Validation Plan

### Automated
1. `node --check Source/ui/public/js/index.js`
2. `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
3. `./scripts/standalone-ui-selftest-production-p0-mac.sh`
4. `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`
5. `./scripts/qa-bl009-headphone-contract-mac.sh`
6. `./scripts/qa-bl018-ambisonic-contract-mac.sh`

### Planned Self-Test Assertions
1. `UI-P2-029A`: trace target selection + schema validity.
2. `UI-P2-029B`: base vs applied divergence under internal animation/physics.
3. `UI-P2-029C`: spectral payload finite/clamped + flux smoothing behavior.
4. `UI-P2-029D`: reflection ghost payload and ER centroid finite contracts.
5. `UI-P2-029E`: calibration export/apply API roundtrip.

### Manual
1. Standalone visual confirmation for room and constellation modes.
2. REAPER host interaction checks for trace controls and fallback chip behavior.
3. Headphone path sanity check (requested vs active stage visibility).

## Risks and Mitigations
1. Risk: snapshot payload growth can impact UI cadence.
   - Mitigation: cap per-frame spectral work and enforce stale-mode fallback.
2. Risk: trace polling overhead can starve UI thread.
   - Mitigation: bounded pop count and fixed UI ring/history size.
3. Risk: reflection visuals misread as full acoustic simulation.
   - Mitigation: explicit “first-order perceptual model” UI label and docs note.
4. Risk: calibration recommendation drift across versions.
   - Mitigation: schema versioning + evidence block + deterministic script seed control.

## Exit Criteria
1. All four priorities implemented behind deterministic contracts.
2. Realtime invariants remain intact (`no alloc/lock/I/O` in audio thread).
3. Host matrix passes with updated evidence.
4. Scene-state and trace schemas documented and synchronized.
5. Tier 0 surfaces updated when acceptance/status claims are promoted.

## Delivery Status
- Current status: `spec_complete`
- Validation status: `not tested` (planning artifact only)
