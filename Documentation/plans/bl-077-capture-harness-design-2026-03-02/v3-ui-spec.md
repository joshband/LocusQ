Title: BL-077 Capture Harness UI Spec v3
Document Type: Design Iteration
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# v3 UI Spec (Recommended)

## Intent
Provide a production-ready operator experience for deterministic QA evidence collection.

## Final Interaction Model
- `qa wrapper` command drives profile + execute/contract modes.
- Terminal runbook mode prints concise cues and immediate remediation hints.
- Post-process emits deterministic artifact tree plus replay-hash manifest.

## Final Surface Areas
1. `Session Header`
- run id, profile, mode, duration, output root.

2. `Live Cue Rail`
- current cue, next cue, elapsed, drift indicator.

3. `Artifact Checklist`
- row-by-row PASS/FAIL for video, frames, contact sheet, cue clips, manifests.

4. `Closeout Block`
- copy-ready handoff block with run id and artifact pointers.

## Accessibility Contract
- All cue labels are plain-language and orientation explicit.
- Non-obvious failures include direct next command to retry.
- Reduced cognitive load: no multiline stack traces in normal path.
