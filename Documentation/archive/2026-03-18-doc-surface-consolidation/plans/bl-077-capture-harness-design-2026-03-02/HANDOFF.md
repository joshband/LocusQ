Title: BL-077 Capture Harness Design Handoff
Document Type: Design Handoff
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# BL-077 Design Handoff

## Iteration Decision
- Completed iterations: `v1`, `v2`, `v3`.
- Recommended final: `v3`.
- Rationale: v3 is the first iteration that fully combines operator cue clarity, deterministic evidence checklisting, and actionable recovery messaging in one flow.

## Final Control Placement (Conceptual)
1. Session Header: run id/profile/mode/duration.
2. Live Cue Rail: current cue + next cue + drift.
3. Artifact Checklist: deterministic PASS/FAIL rows.
4. Closeout Block: copy-ready handoff summary.

## Color System
- Background: deep blue gradient.
- Accent: cyan (`#7de8ff`).
- Success: green (`#79e3a0`).
- Warning: amber (`#efc26a`).
- Error: red (`#ff6f72`).

## Typography
- Telemetry and cue streams: monospace.
- Section labels/headlines: clean sans.

## Implementation Notes
- This design is for capture harness UX and docs preview; it is intentionally framework-neutral and does not generate plugin runtime code.
- Mirror these labels and status blocks in terminal output for parity with preview artifacts.
- Keep naming aligned with `status.tsv` and manifest fields for machine+human traceability.
