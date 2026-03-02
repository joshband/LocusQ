Title: BL-077 Capture Harness UI Spec v1
Document Type: Design Iteration
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# v1 UI Spec

## Intent
Baseline a one-command guided capture flow that removes manual note-taking.

## Interaction Model
- Entry: `scripts/capture-headtracking-rotation-mac.sh --cue-profile dense`.
- Terminal shows:
  - preflight summary,
  - countdown,
  - timed cues,
  - completion output paths.
- Artifacts: video + extracted frames + markdown summary.

## Core Panels (Terminal Sections)
1. `Preflight` (device, duration, cue profile, extraction cadence).
2. `Guided Run` (countdown + cue prompts).
3. `Post-Process` (frame extraction success/failure).
4. `Results` (artifact paths).

## Known UX Gaps
- No explicit session state machine labels.
- No checkpoint confidence markers.
- No deterministic manifest hash output.
