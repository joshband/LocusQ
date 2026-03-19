Title: BL-077 Capture Harness UI Spec v2
Document Type: Design Iteration
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# v2 UI Spec

## Intent
Improve operator confidence and reduce retakes via explicit run-state diagnostics.

## Additions Over v1
- State machine banners: `preflight_ready`, `capture_active`, `postprocess_active`, `complete`.
- Checkpoint table emitted at completion with expected cue count and observed cue count.
- Optional spoken cue status and readiness checks (`say` availability).
- Structured summary block for future machine parsing.

## UX Contract
- All warnings include remediation text.
- Capture exits non-zero only on hard blockers.
- Soft degradations (for example no speech support) remain visible but non-fatal.
