Title: BL-072 Companion Runtime Protocol Parity and BL-058 QA Harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-072 Companion Runtime Protocol Parity and BL-058 QA Harness

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-072 |
| Priority | P0 |
| Status | In Validation (execute lane pass + T2 candidate packet pass: 5/5 runs, zero TODO rows) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-058, BL-059 |
| Blocks | BL-060 |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Align companion runtime packet contract behavior with validated model semantics and establish a real BL-058 QA lane that verifies readiness/sync gating, stale-packet behavior, sequence continuity, and axis-sweep correctness.

## Acceptance IDs

- Companion executable packet contract is parity-verified against the chosen runtime schema (single-version or explicitly dual-version).
- `--require-sync` semantics are consistent between synthetic and live runtime paths.
- BL-058 QA harness exists and emits required deterministic evidence bundle.
- Stale-packet fallback and readiness-state transitions are asserted by automated or hybrid repeatable checks.

## Validation Plan

QA harness script: `scripts/qa-bl072-companion-protocol-parity-mac.sh`.
Evidence schema: `TestEvidence/bl072_*/status.tsv`.

Minimum evidence additions:
- `protocol_parity.tsv`
- `readiness_gate.tsv`
- `axis_sweeps.md`
- `sequence_age_contract.tsv`
- `bl058_lane_packet.md`

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

- Initial QA scaffold authored:
  - `scripts/qa-bl072-companion-protocol-parity-mac.sh` with `--contract-only` and `--execute` modes.
- Contract checks currently verify:
  - companion CLI sync-gate controls (`--require-sync` / `--auto-sync`) and live readiness/send-gate wiring;
  - plugin packet decode/stale-sequence guards in `Source/HeadTrackingBridge.h`;
  - BL-058 lane linkage (readiness + axis evidence surfaces available from BL-058 harness).
- Execute evidence:
  - `TestEvidence/bl072_companion_protocol_execute_20260301T220311Z/status.tsv` (execute mode pass, zero TODO rows).
- T2 candidate evidence:
  - `TestEvidence/bl072_candidate_t2_20260301T220718Z/run_summary.tsv` (5/5 PASS, zero TODO rows).
  - `TestEvidence/bl072_candidate_t2_20260301T220718Z/candidate_decision.md`.
- Remaining BL-072 scope:
  - advance to T3 promotion packet per replay policy and owner intake timing.
