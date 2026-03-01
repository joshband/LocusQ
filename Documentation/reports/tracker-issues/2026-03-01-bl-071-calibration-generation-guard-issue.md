Title: Tracker Issue Draft - BL-071 Calibration Generation Guard and Error-State Enforcement
Document Type: Tracker Issue Draft
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-071 Tracker Issue Draft

## Proposed Title

BL-071: calibration generation guard and error-state enforcement

## Summary

Prevent calibration lifecycle corruption across abort/restart and enforce explicit error-state outcomes for invalid analysis.

## Evidence

- `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md` (Findings #4, #5, #6)
- `Source/CalibrationEngine.h:161`
- `Source/CalibrationEngine.h:186`
- `Source/CalibrationEngine.h:366`
- `Source/CalibrationEngine.h:379`
- `Source/CalibrationEngine.h:428`
- `Source/CalibrationEngine.h:386`
- `Source/CalibrationEngine.h:390`
- `Source/CalibrationEngine.h:433`
- `Source/CalibrationEngine.h:445`

## Acceptance Checklist

- [ ] Abort/restart cannot leak prior generation analysis into active run.
- [ ] Invalid analysis transitions to explicit error state.
- [ ] Completion path requires per-speaker validity success.
- [ ] Cross-thread progress/result publication is race-free.
