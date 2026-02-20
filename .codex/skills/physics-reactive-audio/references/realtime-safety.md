Title: Realtime Safety for Physics-Reactive Audio
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Realtime Safety

## Hard Rules
- No locks on audio thread.
- No heap allocation in process callback.
- No unbounded loops tied to entity count without cap.

## Transfer Patterns
- Double-buffered state snapshots.
- Atomically published immutable frame state.
- Fixed-size ring buffers with overwrite policy.

## Guardrails
- Validate finite values before use.
- Clamp and sanitize all external/simulation inputs.
- Track overrun/drop counters for diagnostics.
