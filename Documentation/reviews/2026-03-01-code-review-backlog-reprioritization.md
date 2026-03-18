Title: LocusQ Code Review and Backlog Reprioritization
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-18

# LocusQ Code Review and Backlog Reprioritization

## Status

Findings consolidated. This is the short decision record.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reviews/2026-03-01-code-review-backlog-reprioritization-legacy.md`

## Primary Findings

1. RT-unsafe preset loading is still reachable from the audio path.
   Missing or invalid presets can retry on every block and turn a config issue into a sustained dropout risk.

2. Audio snapshot reads are not fully coherent.
   Pointer and sample-count state can be observed from different generations.

3. Telemetry data races remain between audio writes and scene-state reads.
   Cross-thread float/double access needs atomic snapshot handoff.

4. Calibration abort/restart and completion semantics are not strict enough.
   Old analysis can bleed into a new run, and invalid analysis can still be reported as `Complete`.

5. Several QA scaffolds still allow false-green outcomes.
   The BL-067/BL-068 lanes and the UI self-test fallback paths need stricter execute-mode semantics.

6. Companion protocol drift still exists.
   The runtime sender and the tested pose model do not yet share one authoritative packet contract.

7. Host-notifying writes and logging paths are still too close to runtime paths.
   These should move off the audio thread or into explicit non-RT lanes.

8. The most useful blocked-validation semantics are still repo-local.
   The review reinforced the need for shared blocked states rather than prose-only status.

## Implications

- The biggest remaining correctness risk is false confidence on RT-safety and state coherence.
- Backlog reprioritization should favor ownership boundaries before feature expansion.
- QA lanes need stricter pass/fail separation between contract coverage and real execution.

## Follow-Up Links

- `BL-037`
- `BL-038`
- `BL-040`
- `BL-050`
- `BL-053`
- `BL-056`
- `BL-059`
- `BL-060`
- `BL-067`
- `BL-068`

## Archive Note

The original long-form review is preserved in the archive copy above.
Use this file for the short decision record only.
