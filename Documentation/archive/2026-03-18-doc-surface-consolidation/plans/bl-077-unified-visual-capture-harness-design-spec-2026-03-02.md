Title: BL-077 Unified Visual Capture Harness Design Spec
Document Type: Design Document
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# BL-077 Unified Visual Capture Harness Design Spec

## Purpose

Define the operator experience for a robust, automated capture workflow that produces promotion-grade artifacts with minimal manual overhead.

## Design Bundle

Iteration artifacts live in:
- `Documentation/plans/bl-077-capture-harness-design-2026-03-02/`

Included files:
- `v1-ui-spec.md`, `v1-style-guide.md`, `v1-test.html`
- `v2-ui-spec.md`, `v2-style-guide.md`, `v2-test.html`
- `v3-ui-spec.md`, `v3-style-guide.md`, `v3-test.html`
- `index.html` (redirects to v3 preview)
- `HANDOFF.md`

## Final Recommendation

- Recommended iteration: `v3`.
- Why: best balance of operator speed, deterministic reporting, and troubleshooting clarity.

## UX Invariants

1. One-command entry with profile + mode clarity.
2. Cue stream remains orientation-explicit and timestamped.
3. Artifact checklist rows are deterministic and machine-parseable.
4. Failures always include the next actionable command or setting path.

## Acceptance Mapping

- Supports `BL077-A-001` and `BL077-A-002` via guided cue flow and dense checkpoints.
- Supports `BL077-A-003` and `BL077-A-005` via artifact checklist and manifest-friendly naming.
- Supports `BL077-A-004` and `BL077-A-006` by keeping the design CLI-first and extension-safe.
