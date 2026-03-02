Title: BL-071 Calibration Generation Guard and Error-State Enforcement
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-02

# BL-071 Calibration Generation Guard and Error-State Enforcement

## Plain-Language Summary

BL-071 in plain terms: Harden calibration lifecycle correctness by enforcing generation isolation across abort/restart transitions, guaranteeing explicit error-state behavior for invalid analysis, and publishing thread-safe immutable progress/result snapshots. Current state: Done (execute + T2 + T3 packets pass; owner promotion decision recorded; done archive sync complete). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-071 Calibration Generation Guard and Error-State Enforcement |
| Why is this important? | Harden calibration lifecycle correctness by enforcing generation isolation across abort/restart transitions, guaranteeing explicit error-state behavior for invalid analysis, and publishing thread-safe immutable progress/result snapshots. |
| How will we deliver it? | Use the validation plan and evidence bundle contract in this runbook to prove behavior and safety before promotion. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-071-calibration-generation-guard-and-error-state-enforcement.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-071 |
| Priority | P0 |
| Status | Done (execute + T2 + T3 packets pass; owner promotion decision recorded; done archive sync complete) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-056, BL-059 |
| Blocks | BL-060 |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| SHARED_FILES_TOUCHED | no |
| Promotion Decision Packet | `TestEvidence/bl071_promotion_t3_20260301T220915Z/promotion_decision.md` |
| Final Evidence Root | `TestEvidence/bl071_promotion_t3_20260301T220915Z/` |
| Archived Runbook Path | `Documentation/backlog/done/bl-071-calibration-generation-guard-and-error-state-enforcement.md` |

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

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment (2026-03-01)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

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
- Done transition checklist:
  - runbook archived under `Documentation/backlog/done/`;
  - backlog index + `status.json` synchronized to Done;
  - promotion decision packet linked in Status Ledger.
