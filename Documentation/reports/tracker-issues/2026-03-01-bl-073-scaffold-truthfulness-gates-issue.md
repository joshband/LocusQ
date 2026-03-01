Title: Tracker Issue Draft - BL-073 QA Scaffold Truthfulness Gates for BL-067 and BL-068
Document Type: Tracker Issue Draft
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-073 Tracker Issue Draft

## Proposed Title

BL-073: QA scaffold truthfulness gates for BL-067 and BL-068

## Summary

Introduce explicit contract-vs-execute mode semantics and fail execute mode when required rows remain scaffold/TODO to prevent false-green promotions.

## Evidence

- `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md` (Finding #7)
- `scripts/qa-bl067-auv3-lifecycle-mac.sh:153`
- `scripts/qa-bl067-auv3-lifecycle-mac.sh:161`
- `scripts/qa-bl068-temporal-effects-mac.sh:146`
- `scripts/qa-bl068-temporal-effects-mac.sh:156`

## Acceptance Checklist

- [ ] `--contract-only` and `--execute` modes are explicit.
- [ ] Execute mode fails when any required row is `TODO`.
- [ ] Promotion checklist rejects scaffold-only execute bundles.
- [ ] BL-067 and BL-068 runbook validation sections reflect new gate contract.
