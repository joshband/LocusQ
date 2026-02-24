Title: ADR-0011 Standalone Renderer Audition Source
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# ADR-0011: Standalone Renderer Audition Source

## Status
Accepted

## Context

Standalone validation needed a deterministic way to audition spatial/headphone rendering without a DAW session or timeline emitters.

Before this change, Renderer mode with zero active emitters produced no direct built-in signal source, which made manual headphone checks and fallback verification slower.

## Decision

Add a built-in Renderer audition source with explicit parameter control and deterministic behavior:

1. Expose four APVTS parameters and WebView controls:
   - `rend_audition_enable`
   - `rend_audition_signal` (`Sine 440`, `Dual Tone`, `Pink Noise`, `Rain`, `Snow`, `Bouncing Balls`, `Wind Chimes`)
   - `rend_audition_motion` (`Center`, `Orbit Slow`, `Orbit Fast`)
   - `rend_audition_level` (`-36`, `-30`, `-24`, `-18`, `-12 dBFS`)
2. Render the internal audition source only when:
   - audition is enabled, and
   - `processedEmitterCount == 0` for the current audio block.
3. Publish audition state in scene telemetry:
   - `rendererAuditionEnabled`
   - `rendererAuditionSignal`
   - `rendererAuditionMotion`
   - `rendererAuditionLevelDb`
   - `rendererAuditionVisualActive`
   - `rendererAuditionVisual.{x,y,z}`
4. Surface audition state in Renderer footer text for runtime visibility.
5. Smooth audition panning gain per speaker to reduce block-stepped artifacts.

## Rationale

1. Keeps normal emitter-driven rendering authoritative when emitters exist.
2. Provides immediate standalone signal generation for manual Steam/stereo checks.
3. Expands standalone test content to include tonal, stochastic, and impact-like content classes.
4. Preserves deterministic behavior through fixed presets and block-stable motion stepping.

## Consequences

### Positive

1. Standalone QA no longer requires external signal injection for quick render checks.
2. Headphone path verification can be repeated with identical signal/motion presets.
3. Operator-facing controls are available in the existing Renderer rail.
4. UI can render an explicit audition source marker in the scene for audio/visual consistency checks.

### Costs

1. Additional renderer control surface and parameter plumbing.
2. One more runtime path to keep covered by self-test/manual validation.

## Guardrails

1. Internal audition source must never run when real emitters are processed.
2. Audition defaults stay conservative (`disabled`, moderate level preset) to avoid unintended output.
3. Any future audition signal additions must update APVTS, WebView controls, and telemetry together.

## Validation Notes (2026-02-24)

1. `cmake --build build_local --config Release --target LocusQ_Standalone -j 8` -> PASS
2. `./scripts/standalone-ui-selftest-production-p0-mac.sh` -> FAIL on existing lane `UI-P1-025B` (not audition-related)
3. `./scripts/standalone-ui-smoke-mac.sh /Users/artbox/Documents/Repos/LocusQ/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app` -> FAIL on existing `UI-03` and `UI-05` checks (not audition-related)

## Related

- `Source/SpatialRenderer.h`
- `Source/PluginProcessor.cpp`
- `Source/PluginEditor.cpp`
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
