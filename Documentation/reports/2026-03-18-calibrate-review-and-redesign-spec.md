Title: LocusQ CALIBRATE Review and Redesign Spec
Document Type: Design Review Report
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# LocusQ CALIBRATE Review and Redesign Spec

## Status

Decision record. Validation status: `not tested`.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reports/2026-03-18-calibrate-review-and-redesign-spec-legacy.md`

## Executive Call

`CALIBRATE` has a solid foundation, but it overstates how complete its current workflow is.

The main correction is:
- split speaker calibration from headphone personalization,
- make auto-detected state visible and honest,
- demote compatibility-only controls,
- stop presenting wide-layout support like a complete operator flow.

## Primary Decisions

| Area | Decision |
|---|---|
| Calibration target | Keep one `CALIBRATE` mode, but split it into `Speaker Room` and `Headphones` tracks. |
| Speaker workflow | Keep topology, host output layout, writable outputs, mic/input, auto-map, and manual routing together. |
| Headphone workflow | Present profile handoff, device profile, active personalization, and listening validation as a first-class guided flow. |
| Routing truth | Rename redetect behavior toward `Auto-map Outputs` or `Detect Host Output Layout`. |
| Legacy config | Hide compatibility aliases behind `Advanced`; do not present them as the main mental model. |
| Wide layouts | Keep explicit warnings when topology exceeds current writable routing support. |
| Automation | Always show what was detected, where it came from, and what is still manual. |

## Follow-Up Links

- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/2026-02-27-calibration-system-design.md`
- `Documentation/plans/2026-02-27-calibration-implementation-plan.md`
- `BL-026`
- `BL-057`
- `BL-058`
- `BL-059`

## Archive Note

The original long-form redesign review is preserved in the archive copy above.
Use this file for current design direction and the archive file for full review detail.
