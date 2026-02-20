Title: Reactive Runtime Stability and Performance
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Runtime Stability and Performance

## Goal
Maintain smooth visuals while reactive mappings update continuously.

## Rules
- No allocations inside per-frame hot paths.
- Cap update rates for heavy feature pipelines.
- Decouple feature cadence from render cadence using cached state.
- Provide reduced-quality mode for weaker hosts.

## Targets
- 60 FPS path: keep average frame time below 16.67 ms.
- 120 FPS path: keep average frame time below 8.33 ms.
