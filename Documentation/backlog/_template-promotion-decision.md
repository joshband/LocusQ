Title: BL-XXX Promotion Decision Template
Document Type: Backlog Template
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-18

# BL-XXX Promotion Decision

Use this file inside:
- `TestEvidence/<bl_or_hx>_owner_sync_<slice>_<timestamp>/promotion_decision.md`

## Plain-Language Summary

- What changed: [short summary]
- Why this decision: [short rationale]
- Decision: [promote / hold / block]

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is impacted? | [Users / operators / QA / release owners] |
| What was reviewed? | [Slices or evidence set] |
| Why this outcome? | [Risk and evidence summary] |
| How was confidence established? | [Replay, gates, review] |
| When can this be revisited? | [Trigger or date] |
| Where is the evidence? | [`TestEvidence/<packet>/...`] |

## Visual Aid Index

Use visuals only when they improve clarity.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Gate matrix | Fast PASS/FAIL scan | `## Gate Matrix` |
| Reliability table | Shows replay confidence | `## Reliability Checks` |
| Diagram (optional) | Clarifies a tricky no-go path | Adjacent to the relevant section |

## Decision

| Field | Value |
|---|---|
| Result | `PASS` / `FAIL` |
| Decision | `Done-candidate` / `In Validation` / `Blocked` |
| Automation Mode | `draft_only` |
| Owner Required For | `Done`, archive move, status/index sync |

## Scope Reviewed

- [Slice or evidence set 1]
- [Slice or evidence set 2]

## Gate Matrix

| Gate | Expected | Actual | Status | Evidence |
|---|---|---|---|---|
| Build | PASS | [PASS/FAIL] | [PASS/FAIL] | `build.log` |
| Smoke | PASS | [PASS/FAIL] | [PASS/FAIL] | `qa_smoke.log` |
| Item lane | PASS | [PASS/FAIL] | [PASS/FAIL] | `validation_matrix.tsv` |
| RT safety | `non_allowlisted=0` | [value] | [PASS/FAIL] | `rt_audit.tsv` |
| Docs freshness | PASS | [PASS/FAIL] | [PASS/FAIL] | `docs_freshness.log` |
| Status schema | PASS | [PASS/FAIL] | [PASS/FAIL] | `status_json_check.log` |
| Ownership safety | `SHARED_FILES_TOUCHED=no` or justified | [value] | [PASS/FAIL] | `handoff_resolution.md` |

## Reliability Checks

| Check | Expected | Actual | Status | Evidence |
|---|---|---|---|---|
| Replay count | [N] | [N] | [PASS/FAIL] | `validation_matrix.tsv` |
| Replay outcome | all PASS | [summary] | [PASS/FAIL] | `validation_matrix.tsv` |
| Hash/parity stability | stable | [stable/drift] | [PASS/FAIL] | `replay_hashes.tsv` |

## Consistency Checks

| Surface | Expected | Status | Notes |
|---|---|---|---|
| Runbook | current | [PASS/FAIL] | [notes] |
| Backlog index | aligned | [PASS/FAIL] | [notes] |
| `status.json` | aligned | [PASS/FAIL] | [notes] |
| `TestEvidence/build-summary.md` | updated | [PASS/FAIL] | [notes] |
| `TestEvidence/validation-trend.md` | updated | [PASS/FAIL] | [notes] |

## Done Readiness

Fill this only if proposing `Done`.

| Check | Expected | Status | Notes |
|---|---|---|---|
| Closeout template applied | yes | [PASS/FAIL] | [notes] |
| Runbook move planned | yes | [PASS/FAIL] | [notes] |
| Index row ready | yes | [PASS/FAIL] | [notes] |

## Blockers

- [Blocker or `none`]

## Recommendation

- `Done-candidate` only if all required gates pass and no blocker remains.
- `In Validation` if implementation is done but promotion confidence is still converging.
- `Blocked` if any hard gate is red.

## Evidence Index

- `status.tsv`
- `validation_matrix.tsv`
- `build.log`
- `qa_smoke.log`
- `rt_audit.tsv`
- `docs_freshness.log`
- `owner_decisions.md`
- `handoff_resolution.md`
