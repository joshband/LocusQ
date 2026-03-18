Title: QA Harness Opportunities Review
Document Type: Review
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# QA Harness Opportunities Review

## Status

Findings consolidated. This is the short decision record.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reviews/2026-03-17-qa-harness-opportunities-review-legacy.md`

## Primary Findings

1. Head-tracking packet validation is split across incompatible contracts.
   The live sender, companion tests, and plugin bridge do not yet share one loopback-trustworthy packet truth.

2. Headphone verification scores are synthetic policy scores.
   They are useful heuristics, but they are not measured evidence and should not be treated like promotion-grade results.

3. The companion test entrypoint is currently red before runtime tests start.
   Package compile smoke needs to be a first-class gate.

4. Install/update/sync truth is split across scripts.
   The repo needs one machine-readable install manifest or bundle-sync verifier.

5. The standalone UI selftest is already a reusable harness subsystem.
   It should be extracted rather than copied for the next app or companion.

6. LocusQ’s QA runner still owns reusable responsibilities.
   Parameter merging, profiling attachment policy, and smoke plumbing belong upstream.

7. Blocked-validation semantics still live mostly in repo-local prose and scripts.
   The harness should own explicit blocked states.

8. CI checkout/auth logic still drifts across repos.
   Composite-action reuse would remove duplicated token and path handling.

## Implications

- The best harness work is upstream work.
- The highest leverage comes from shared contracts: packet fixtures, install manifests, blocked states, and app selftest orchestration.
- LocusQ should stop being the place where reusable QA infra grows by accident.

## Follow-Up Links

- `BL-086`
- `BL-087`
- `BL-088`
- `BL-089`
- `BL-090`
- `BL-091`
- `BL-092`

## Archive Note

The original long-form review is preserved in the archive copy above.
Use this file for the short decision record only.
