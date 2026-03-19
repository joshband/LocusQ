Title: Remaining Open Backlog Implementation Plan
Document Type: Execution Plan
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-19

# Remaining Open Backlog Implementation Plan

## Status
Active planning packet.
This plan covers the open `BL-*` set still living under `Documentation/backlog/`.

## Bottom Line

No remaining open BL runbook from `BL-020` through `BL-101` should move to `Documentation/backlog/done/` yet.

Closest-to-done items:
- `BL-020`: green evidence, owner promotion still pending
- `BL-085`: local integration lane green and now `Done-candidate`; final closeout sync still pending
- `BL-095`: Slice B done, Slice C still open

Not done:
- `BL-032`: explicit hold blocker still active
- `BL-060`: blocked on real participant sessions
- `BL-067`: blocked on Apple signing, host execution, runtime-access proof
- `BL-079`: blocked on clean-checkout replay and host verification
- `BL-081`..`BL-088`: mostly true open or deferred upstream extraction work
- `BL-096`..`BL-101`: true open follow-on work
- `BL-100`: still a draft, not a done candidate

## Open Set

### Promotion-Ready but Not Closed

- `BL-020` `In Validation`
- `BL-085` `Done-candidate`

### Validation Holds

- `BL-032` `Done-candidate on hold`
- `BL-060` `In Validation`
- `BL-067` `In Validation`
- `BL-079` `In Validation`
- `BL-095` `In Validation`

### True Open Implementation Lanes

- `BL-021`
- `BL-061`
- `BL-081`
- `BL-082`
- `BL-083`
- `BL-084`
- `BL-086`
- `BL-096`
- `BL-097`
- `BL-098`
- `BL-099`
- `BL-101`

### Deferred

- `BL-087`
- `BL-088`

### Draft, Not Yet in the Canonical Queue

- `BL-100`

## Workstreams

### W1. Closeout Reconciliation

Goal:
- finish lanes that look healthy but are still missing formal promotion/closeout

Items:
- `BL-020`
- `BL-085`

Exit:
- owner promotion packet recorded
- `Documentation/backlog/index.md` and runbook status agree
- move to `Documentation/backlog/done/` only after the owner-confirmed closeout set is complete

### W2. Validation Blocker Burn-Down

Goal:
- clear the concrete technical or evidence blockers on near-finished lanes

Items:
- `BL-032`
- `BL-060`
- `BL-067`
- `BL-079`
- `BL-095`

Exit:
- blocker-specific evidence is green
- lane status can advance to owner promotion or done-candidate cleanly

### W3. Truthfulness and Provenance Recovery

Goal:
- fix the trust-language and evidence-truth gaps across FIR, verification, and CALIBRATE

Items:
- `BL-095`
- `BL-099`
- `BL-101`

Exit:
- measured vs estimated vs generic vs unavailable states are explicit
- UI/runtime wording matches evidence class
- QA lanes prove the truthfulness contract

### W4. Companion and Bridge Runtime Consolidation

Goal:
- remove duplicate runtime paths and reduce cadence/reload drift

Items:
- `BL-096`
- `BL-097`

Exit:
- one canonical executable/core protocol path
- bridge cadence is tiered
- calibration reload work is isolated from noisy editor cadence

### W5. Local and Upstream Validation Infrastructure

Goal:
- restore trustworthy local validation and finish the upstream harness extraction set

Items:
- `BL-081`
- `BL-082`
- `BL-083`
- `BL-084`
- `BL-086`
- `BL-098`

Exit:
- local repo validation floor is honest and repeatable
- shared harness components are extracted with clear adoption contracts

### W6. UI/Scene Expansion

Goal:
- complete the remaining product-facing feature work not blocked on the truthfulness tracks

Items:
- `BL-021`
- `BL-061`

Exit:
- implementation lands
- replay and owner evidence are defined and run

## Item Task List

| Item | Next Tasks |
|---|---|
| `BL-020` | record owner promotion decision; sync index/runbook/evidence; move to `done/` only in the same change set |
| `BL-021` | implement the room-story overlay runtime path; emit transition/fallback replay artifacts; capture owner intake/promotion evidence |
| `BL-032` | reduce `Source/PluginProcessor.cpp` below the line-count guardrail; rerun guardrail/build/smoke/RT lanes; refresh owner promotion packet |
| `BL-060` | run real participant sessions; capture gate decision packet; refresh promotion review with human-study evidence |
| `BL-061` | freeze interpolation/crossfade contract; implement runtime lane; add parity and safety evidence once BL-060 gate is satisfied |
| `BL-067` | harden extension-safe runtime profile/SOFA access first; then resolve Apple signing and execute real host inventory coverage |
| `BL-079` | run clean-checkout replay; capture representative AU/VST3 host parameter-view checks; record promotion decision |
| `BL-081` | extract perceptual harness tooling upstream; keep LocusQ shim thin; document adoption path |
| `BL-082` | build the reusable QA runner app library; port LocusQ runner entrypoints; prove parity with current runner behavior |
| `BL-083` | wire runtime-config contract into `ScenarioExecutor`; remove local workaround paths; add no-op override warnings and proof |
| `BL-084` | add profiling policy enforcement; gate `perf_*` fields centrally; update configs and audit downstream consumers |
| `BL-085` | keep `Done-candidate` truth surfaces synced; decide whether upstream follow-up stays here or becomes a separate tracked lane; apply final closeout sync only after that decision |
| `BL-086` | author CI checkout composite action; switch LocusQ to the shared action; document token and path expectations |
| `BL-087` | hold until second real consumer exists; then add recursive discovery and deterministic traversal proof |
| `BL-088` | hold until `BL-082`..`BL-084` are done; then prototype VST3 HostRunner path before AU |
| `BL-095` | finish Slice C objective validation; record latency/parity/CPU evidence honestly; close remaining truth-render gaps |
| `BL-096` | unify companion executable and core protocol paths; add parity tests; remove duplicate runtime behavior |
| `BL-097` | split structural scene publication from diagnostics; isolate calibration reload work; bound noisy logging |
| `BL-098` | fix clean-tree typecheck dependency order; restore one meaningful local automated lane; document the minimum local baseline |
| `BL-099` | freeze compensation provenance contract; align UI/runtime labels to evidence class; add truthfulness QA |
| `BL-100` | decide whether to intake; if accepted, index it canonically before implementation; then build the desktop operator runner and evidence contract |
| `BL-101` | freeze CALIBRATE discovery/provenance semantics; align payloads and persistence; add CALIBRATE truthfulness QA |

## Parallel Agent Plan

### Batch A

Worker 1:
- `BL-020`
- `BL-085`

Worker 2:
- `BL-032`
- `BL-079`

Worker 3:
- `BL-067`
- `BL-098`

Reason:
- these are the highest-leverage closeout and validation lanes

### Batch B

Worker 1:
- `BL-095`
- `BL-099`

Worker 2:
- `BL-096`
- `BL-097`

Worker 3:
- `BL-101`

Reason:
- these lanes are tightly related around trustfulness, provenance, runtime truth, and CALIBRATE semantics

### Batch C

Worker 1:
- `BL-081`
- `BL-082`

Worker 2:
- `BL-083`
- `BL-084`

Worker 3:
- `BL-086`

Reason:
- these are the shared-harness extraction and local-validation infrastructure lanes

### Batch D

Worker 1:
- `BL-021`

Worker 2:
- `BL-061`

Worker 3:
- optional intake on `BL-100`

Reason:
- these are feature-forward lanes that should start after the higher-risk trust and infrastructure work is stable

## Recommended Order

1. `BL-098`
2. `BL-032`
3. `BL-079`
4. `BL-067`
5. `BL-020`
6. `BL-085`

## Batch A Immediate Tasks (2026-03-19)

### BL-079

- Technical blocker is cleared.
- Remaining work is manual host-view verification only.
- Use:
  - `TestEvidence/bl079_validation_20260319T030000Z/`
  - `TestEvidence/bl079_host_view_validation_20260319T030729Z/manual_host_checklist.md`
  - `TestEvidence/bl079_host_view_validation_20260319T030729Z/host_matrix.tsv`
- Fastest path: REAPER VST3 + AU parameter-view inspection, then same-tree backlog/status sync.

### BL-085

- Local replay is green again.
- Item is now `Done-candidate`.
- Owner packet baseline is at:
  - `TestEvidence/bl085_owner_sync_z1_20260319T030729Z/promotion_decision.md`
- Remaining question is governance, not code:
  - whether the upstream adoption follow-up stays here or moves into a separate follow-on lane before final closeout

### BL-067

- Smallest local runtime-access slice is now complete.
- Local proof:
  - `TestEvidence/bl067_runtime_access_20260319T034500Z/sandbox_runtime_access.tsv`
- What changed:
  - calibration-profile fallback now uses the LocusQ user-data directory only
  - custom SOFA fallback now uses the LocusQ user-data directory only
- Next blockers are external or host-facing:
  - Apple signing
  - real AUv3 host execution
  - optional follow-on: tighten native file-dialog defaults in `Source/editor_webview/EditorWebViewRuntime.h`

### Docs Gate

- Scratch build fallout is fixed:
  - `_bl095_probe_build/` removed
  - `.gitignore` now ignores `_bl*_build/`
  - `scripts/validate-docs-freshness.sh` now prunes `_bl*_build`
7. `BL-095`
8. `BL-099`
9. `BL-101`
10. `BL-096`
11. `BL-097`
12. `BL-081`
13. `BL-082`
14. `BL-083`
15. `BL-084`
16. `BL-086`
17. `BL-021`
18. `BL-060`
19. `BL-061`
20. `BL-087`
21. `BL-088`
22. `BL-100`

## Implementation Rules

- Do not move any item to `Documentation/backlog/done/` until the runbook status, index row, and evidence packet all agree.
- Keep `BL-100` out of implementation until it is either indexed or explicitly rejected.
- Treat `BL-060`, `BL-067`, and `BL-095` as truth-critical lanes. Do not compress weak evidence into stronger claims.
- For WebView-facing lanes, `skill_impl` must treat WebView checklist failures as hard blockers.
- For truthfulness/provenance lanes, update ADRs or supporting specs before closing implementation if the runtime contract changes.

## Closeout Target

This plan is complete when:
- every remaining open BL item is either promoted, held with a concrete blocker, deferred intentionally, or rejected explicitly
- no stale “done but still open-folder” lanes remain
- the next implementation wave can be assigned as parallel worker batches without overlap ambiguity
