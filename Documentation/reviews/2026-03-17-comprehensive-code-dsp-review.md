Title: LocusQ Comprehensive Code + DSP Review
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# LocusQ Comprehensive Code + DSP Review

## Status

Findings consolidated. This is the short decision record.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reviews/2026-03-17-comprehensive-code-dsp-review-legacy.md`

## Primary Findings

1. `BL-055` is false-green.
   The backlog and QA evidence mark the FIR engine as done even though the current code still behaves like a direct loop and the lane mainly checks marker strings.

2. The FIR calibration path advertises partitioned behavior it does not implement.
   Reported latency and engine mode can diverge from actual DSP behavior, which makes host PDC and performance evidence untrustworthy.

3. Companion packet truth is split.
   The shipping executable still emits `PosePacketV1` while the tested core path uses a different v2 contract, so the live sender is not protected by the current tests.

4. Editor bridge work is too heavy on the message thread.
   Full scene/calibration JSON and JS marshalling every tick creates avoidable UI churn and hides cadence problems.

5. Companion profile polling triggers heavyweight runtime reload behavior from the editor thread.
   File observation, parse, and apply are not cleanly separated, so profile changes can stall the UI or blur ownership.

6. `locusq_webui_typecheck` is not dependency-complete in clean builds.
   It can pass only after unrelated dependency installation has already happened.

## Implications

- Release decisions can be made on evidence that overstates DSP readiness.
- Host timing, CPU, and latency reporting are not yet honest enough for promotion-grade review.
- Companion maintenance cost is higher than it should be because the shipped sender and tested core do not share one packet truth.
- UI responsiveness and build reliability both need tighter ownership boundaries.

## Follow-Up Links

- `BL-055`
- `BL-050`
- `BL-053`
- `BL-058`
- `BL-059`
- `BL-060`
- `BL-067`
- `BL-068`

## Archive Note

The original long-form review is preserved in the archive copy above.
Use this file for the short decision record only.
