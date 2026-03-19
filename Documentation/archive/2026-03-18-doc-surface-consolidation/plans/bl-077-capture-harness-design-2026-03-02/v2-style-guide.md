Title: BL-077 Capture Harness Style Guide v2
Document Type: Design Iteration
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# v2 Style Guide

## Messaging Pattern
- `INFO:` facts and progression.
- `WARN:` degraded but usable.
- `ERROR:` stop and remediate.

## Information Density
- Keep cue lines short (<72 chars preferred).
- Print artifact paths once in live flow and once in final summary.
- Use ordered, timestamped checkpoints for human+machine parity.

## Motion/Timing Semantics
- Countdown always decrements in whole seconds.
- Cue emission must not drift relative to configured cue times by more than one print interval.
