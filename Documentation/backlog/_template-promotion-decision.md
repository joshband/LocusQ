Title: BL-XXX Promotion Decision Template
Document Type: Backlog Template
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-28

# BL-XXX Promotion Decision (`Slice Z*` Owner Sync)

Use this template for owner-authoritative promotion packets under:
- `TestEvidence/<bl_or_hx>_owner_sync_<slice>_<timestamp>/promotion_decision.md`

## Decision
- Result: `PASS | FAIL`
- Decision: `Done-candidate | In Validation | Blocked`

## Scope Reviewed
- [List the implementation/validation slices reconciled in this owner sync.]

## Required Gate Matrix

| Gate | Command | Expected | Actual | Status | Evidence |
|---|---|---|---|---|---|
| Build | `cmake --build ...` | PASS | [PASS/FAIL] | [PASS/FAIL] | `build.log` |
| Smoke suite | `locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | PASS | [PASS/FAIL] | [PASS/FAIL] | `qa_smoke.log` |
| Item lane replay | `[qa lane command]` | PASS | [PASS/FAIL] | [PASS/FAIL] | `validation_matrix.tsv` |
| Contract lane(s) | `[qa contract command]` | PASS | [PASS/FAIL] | [PASS/FAIL] | `[lane log]` |
| RT safety | `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` | `non_allowlisted=0` | `[value]` | [PASS/FAIL] | `rt_audit.tsv` |
| Replay cadence compliance | `runbook replay tier + run budget check` | PASS | [PASS/FAIL] | [PASS/FAIL] | `owner_decisions.md` |
| Ownership safety | `SHARED_FILES_TOUCHED marker + ownership delta check` | `no` | `[no/yes]` | [PASS/FAIL] | `handoff_resolution.md` |
| Evidence localization | `promotion evidence path check` | `TestEvidence/...` only | [PASS/FAIL] | [PASS/FAIL] | `handoff_resolution.md` |
| Status schema | `jq empty status.json` | PASS | [PASS/FAIL] | [PASS/FAIL] | `status_json_check.log` |
| Docs freshness | `./scripts/validate-docs-freshness.sh` | PASS | [PASS/FAIL] | [PASS/FAIL] | `docs_freshness.log` |

## Determinism / Reliability Checks

| Check | Expected | Actual | Status | Evidence |
|---|---|---|---|---|
| Replay run count | [N] | [N] | [PASS/FAIL] | `validation_matrix.tsv` |
| Replay outcomes | all PASS | [summary] | [PASS/FAIL] | `validation_matrix.tsv` |
| Hash/parity stability (if applicable) | stable | [stable/drift] | [PASS/FAIL] | `replay_hashes.tsv` |

## Contract Consistency

| Surface | Expected | Status | Notes |
|---|---|---|---|
| `Documentation/backlog/bl-XXX-*.md` | status + acceptance mapping current | [PASS/FAIL] | [notes] |
| `Documentation/backlog/index.md` | row status aligned | [PASS/FAIL] | [notes] |
| `Documentation/implementation-traceability.md` | acceptance/evidence mapping updated | [PASS/FAIL] | [notes] |
| `status.json` | evidence keys + notes aligned | [PASS/FAIL] | [notes] |
| `TestEvidence/build-summary.md` | snapshot updated | [PASS/FAIL] | [notes] |
| `TestEvidence/validation-trend.md` | trend entries appended | [PASS/FAIL] | [notes] |

## Done Transition Readiness (Required if proposing Done)

| Check | Expected | Status | Notes |
|---|---|---|---|
| Closeout template applied | `Documentation/backlog/_template-closeout.md` structure used | [PASS/FAIL] | [notes] |
| Runbook move planned | `Documentation/backlog/done/bl-XXX-*.md` target path explicit | [PASS/FAIL] | [notes] |
| Index row ready | row state/status/path updated for Done | [PASS/FAIL] | [notes] |

## Blockers (if any)
- [blocker 1]
- [blocker 2]

## Recommendation Rule
- `Done-candidate` only if all required gates pass and no blockers remain.
- `In Validation` if implementation is complete but promotion gates/evidence are still converging.
- `Blocked` if any hard gate fails (build/smoke/lane/RT/docs freshness/status schema).

## Evidence Index
- `status.tsv`
- `validation_matrix.tsv`
- `qa_lane.log`
- `build.log`
- `qa_smoke.log`
- `qa_bl009.log` (or equivalent lane log)
- `rt_audit.tsv`
- `docs_freshness.log`
- `owner_decisions.md`
- `handoff_resolution.md`
