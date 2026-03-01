Title: BL-071 Calibration Generation Guard and Error-State Enforcement
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-071 Calibration Generation Guard and Error-State Enforcement

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-071 |
| Priority | P0 |
| Status | Done-candidate (execute + T2 + T3 packets pass; owner promotion decision pending) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-056, BL-059 |
| Blocks | BL-060 |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Harden calibration lifecycle correctness by enforcing generation isolation across abort/restart transitions, guaranteeing explicit error-state behavior for invalid analysis, and publishing thread-safe immutable progress/result snapshots.

## Acceptance IDs

- Abort/restart cannot leak prior-generation analysis into active calibration run.
- Invalid or partial analysis transitions to explicit error state and cannot be promoted as complete.
- Calibration progress/result publication is race-free across audio/UI/analysis threads.
- Handoff diagnostics include generation ID, state transition reason, and failure category.

## Validation Plan

QA harness script: `scripts/qa-bl071-calibration-generation-guard-mac.sh`.
Evidence schema: `TestEvidence/bl071_*/status.tsv`.

Minimum evidence additions:
- `generation_isolation.tsv`
- `error_state_contract.tsv`
- `cross_thread_snapshot_contract.tsv`
- `calibration_failure_taxonomy.tsv`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | runbook primary lane command at dev-loop depth | validation matrix + replay summary |
| Candidate intake | T2 | 5 (or heavy-wrapper 2-run cap) | runbook candidate replay command set | contract/execute artifacts + taxonomy |
| Promotion | T3 | 10 (or owner-approved heavy-wrapper 3-run equivalent) | owner-selected promotion replay command set | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run sentinel drill when explicitly requested | parity/sentinel artifacts |

### Cost/Flake Policy

- Diagnose failing run index before repeating full multi-run sweeps.
- Heavy wrappers (`>=20` binary launches per wrapper run) use targeted reruns, candidate at 2 runs, and promotion at 3 runs unless owner requests broader coverage.
- Document cadence overrides with rationale in `lane_notes.md` or `owner_decisions.md`.

## Handoff Return Contract

All worker and owner handoffs for this runbook must include:
- `SHARED_FILES_TOUCHED: no|yes`

Required return block:
```
HANDOFF_READY
TASK: <BL ID + Title>
RESULT: PASS|FAIL
FILES_TOUCHED: ...
VALIDATION: ...
ARTIFACTS: ...
SHARED_FILES_TOUCHED: no|yes
BLOCKERS: ...
```

## Governance Alignment (2026-03-01)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).

## Execution Notes (2026-03-01)

- Initial runtime hardening landed in `Source/CalibrationEngine.h`:
  - generation counters now gate speaker start and reject stale/aborted analysis publications;
  - restart is explicitly rejected while prior analysis is still in flight;
  - invalid/partial analysis now transitions to explicit `State::Error` with failure diagnostics;
  - progress/result publication now uses atomic snapshots plus locked result-copy reads.
- Initial QA scaffold authored:
  - `scripts/qa-bl071-calibration-generation-guard-mac.sh` with `--contract-only` and `--execute` modes.
- Execute evidence:
  - `TestEvidence/bl071_calibration_generation_guard_execute_20260301T220310Z/status.tsv` (execute mode pass, zero TODO rows).
- T2 candidate evidence:
  - `TestEvidence/bl071_candidate_t2_20260301T220718Z/run_summary.tsv` (5/5 PASS, zero TODO rows).
  - `TestEvidence/bl071_candidate_t2_20260301T220718Z/candidate_decision.md`.
- T3 promotion evidence:
  - `TestEvidence/bl071_promotion_t3_20260301T220915Z/run_summary.tsv` (10/10 PASS, zero TODO rows).
  - `TestEvidence/bl071_promotion_t3_20260301T220915Z/promotion_decision.md`.
- Remaining BL-071 scope:
  - owner promotion decision and done-transition archive sync.
