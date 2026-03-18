Title: QA Platform Leverage Review
Document Type: Review
Author: Claude Code
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# QA Platform Leverage Review

## Status

Findings consolidated. This is the short decision record.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reviews/2026-03-17-qa-platform-leverage-review-legacy.md`

## Primary Findings

1. LocusQ can inject profiling metrics outside harness precondition control.
   `afterScenarioExecution` re-attaches metrics before the harness has a chance to enforce profiling policy, which can create false-green performance evidence.

2. Runtime config application is optional on some execution paths.
   A scenario can run in isolation without suite overrides, so the same JSON can behave differently depending on invocation mode.

3. There are still no CTest-registered plugin-side unit targets.
   Processor and bridge regressions still depend on full-harness or manual host execution.

4. Companion wire protocol truth is split.
   The executable and its tests do not exercise the same packet version, which leaves end-to-end packet confidence incomplete.

5. Companion sync/install verification does not prove source and destination binary identity.
   Install flows can appear successful while leaving a stale app bundle in place.

6. Standalone selftest is still a product-specific mini-framework.
   The script works, but the reusable orchestration belongs in harness code.

7. The most useful blocked-validation states are still repo-local conventions.
   LocusQ needs a first-class result model for inventory-only, environment-blocked, and promotion-blocked states.

8. CI checkout/auth logic is duplicated across repos.
   That creates drift and keeps harness path changes expensive.

## Implications

- The harness is strong, but LocusQ still owns too many reusable concerns.
- False-green performance and install evidence remain the biggest trust risks.
- The next gains come from moving reusable orchestration upstream, not from adding more repo-local scripts.

## Follow-Up Links

- `BL-083`
- `BL-084`
- `BL-086`
- `BL-087`
- `BL-088`
- `BL-089`
- `BL-090`

## Archive Note

The original long-form review is preserved in the archive copy above.
Use this file for the short decision record only.
