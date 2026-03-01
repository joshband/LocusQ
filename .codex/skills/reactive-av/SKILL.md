---
name: reactive-av
description: Build and troubleshoot audio-reactive and physics-reactive visualization systems for JUCE WebView plugins.
---

Title: Reactive Audio-Visual Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-03-01

# Reactive Audio-Visual

Use this skill when visualization behavior must react to audio features and/or physics state in real time.

## Workflow
1. Define sources first: which audio features (RMS, peak, band energy, onset) and which physics features (velocity, acceleration, collisions, density).
2. Define a mapping contract before coding: input range, smoothing, hysteresis, clamp, and visual target parameter.
3. Split runtime into layers: feature extraction, normalization, mapping, rendering.
4. Add quality tiers and fallback paths for constrained hosts.
5. Validate with repeatable stimuli and evidence artifacts.

## Reference Map
- `references/mapping-contracts.md`: Mapping schema and normalization patterns.
- `references/runtime-stability-and-performance.md`: Frame-time budgets and memory hygiene.
- `references/qa-troubleshooting-checklist.md`: Reactive visual QA and failure signatures.

## Backend-Aware QA Requirements
- Validate reactive visualization behavior on both:
  - `WKWebView` hosts (Apple platforms)
  - `WebView2` hosts (Windows)
- Record backend-specific differences in:
  - update cadence/jitter behavior,
  - UI interaction responsiveness,
  - fallback behavior when native bridge calls are delayed.

## Execution Rules
- Keep mapping deterministic for the same input stream.
- Avoid direct coupling from transport callbacks into render mutations.
- Apply smoothing and deadbands to prevent visual jitter.
- Version mapping contracts when changing semantics.
- When requirements include audio-thread behavior changes, hand off to `physics-reactive-audio`.

## Head-Tracking Telemetry Mapping Notes
- When rendering orientation-reactive visuals, include packet-age and effective-rate overlays where feasible.
- Apply smoothing/deadband with explicit, logged parameters to avoid hidden behavior drift.
- Preserve deterministic output for repeated telemetry playback inputs.

## Deliverables
- Changed files with rationale.
- Mapping table (feature -> transform -> visual parameter).
- Validation status as `tested`, `partially tested`, or `not tested`.
