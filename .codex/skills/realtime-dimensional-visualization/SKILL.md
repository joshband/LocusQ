---
name: realtime-dimensional-visualization
description: Design and implement realtime visualization systems for LocusQ across 2D/3D/4D views (time-aware state), with information-visualization clarity, modern UI art direction, and plugin-host performance constraints.
---

Title: Realtime Dimensional Visualization Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Realtime Dimensional Visualization

Use this skill when the task is about high-quality plugin UI/visualization design and implementation, including 2D, 3D, and time-layered 4D representations.

## Scope
- Information visualization for audio/spatial/simulation telemetry.
- Realtime 2D/3D rendering behavior and 4D presentation (state over time).
- UI art direction for beautiful, intentional interfaces (type, color, layout, motion).
- Host-aware performance/latency constraints for plugin and standalone runtimes.

## Workflow
1. Lock visual intent and operator goals first.
   - Define what must be understood at a glance vs explored interactively.
2. Build explicit mapping contracts.
   - Data -> encodings (position, color, size, motion, history trail/time window).
3. Choose rendering architecture by budget.
   - 2D layer, 3D layer, and temporal history strategy with quality tiers.
4. Apply art-direction system.
   - Typography, spacing, color tokens, motion language, and hierarchy rules.
5. Validate clarity and performance.
   - Verify legibility, interaction latency, jitter resistance, and frame budget behavior.

## Design and Runtime Rules
- Avoid generic UI defaults; choose intentional visual direction per feature.
- Keep information density readable under fast-changing realtime conditions.
- Preserve deterministic visual response for deterministic input playback.
- Gate heavy effects behind quality tiers and fallback gracefully.

## Cross-Skill Routing
- Pair with `threejs` for scene architecture and render-loop implementation.
- Pair with `reactive-av` for feature extraction and smoothing contracts.
- Pair with `juce-webview-runtime` for host/backend runtime parity.
- Pair with `simulation-behavior-audio-visual` when complex simulation drives visuals.

## References
- `references/ui-art-direction-trends.md`
- `references/info-viz-mappings.md`
- `references/realtime-2d-3d-4d-contract.md`
- `references/visual-language-tokens.md`
- `references/prompt-examples.md`

## Deliverables
- Visual mapping table and design-token summary.
- Validation status: `tested`, `partially tested`, or `not tested`.
- Residual risk statement for skipped performance/clarity checks.
