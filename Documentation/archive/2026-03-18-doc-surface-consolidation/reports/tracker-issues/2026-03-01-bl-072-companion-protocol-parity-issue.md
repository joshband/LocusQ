Title: Tracker Issue Draft - BL-072 Companion Runtime Protocol Parity and BL-058 QA Harness
Document Type: Tracker Issue Draft
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-072 Tracker Issue Draft

## Proposed Title

BL-072: companion runtime protocol parity + BL-058 QA harness

## Summary

Close protocol-version and readiness-gate blind spots by aligning companion runtime packet behavior and adding first-class BL-058 QA harness coverage.

## Evidence

- `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md` (Findings #10, #11, #12, #13, #17, #18)
- `Source/HeadPoseInterpolator.h:14`
- `Source/HeadPoseInterpolator.h:67`
- `Source/HeadTrackingBridge.h:187`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift:217`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift:334`
- `Documentation/backlog/bl-058-companion-profile-acquisition.md:60`

## Acceptance Checklist

- [ ] Companion runtime packet contract is parity-verified with chosen schema.
- [ ] Synthetic and live `--require-sync` behavior is equivalent.
- [ ] BL-058 QA harness exists with required evidence bundle.
- [ ] Stale/sequence/age contracts are explicitly asserted in lane output.
