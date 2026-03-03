Title: BL-073 QA Scaffold Truthfulness Gates for BL-067 and BL-068
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-03

# BL-073 QA Scaffold Truthfulness Gates for BL-067 and BL-068

## Plain-Language Summary

BL-073 in plain terms: Prevent false-green promotions by separating contract-only and execute-mode QA semantics for BL-067/BL-068 and enforcing execute-mode failure when runtime matrix rows remain scaffold/TODO. Current state: In Validation (clean verification packet merged via PR #6 on 2026-03-03; contract-only and execute `--runs 3` gates passed with zero execute TODO/SCAFFOLD rows). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA owners, release owners, and engineering maintainers who depend on deterministic evidence. |
| What is changing? | Prevent false-green promotions by separating contract-only and execute-mode QA semantics for BL-067/BL-068 and enforcing execute-mode failure when runtime matrix rows remain scaffold/TODO. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: In Validation (clean verification merged via PR #6 on 2026-03-03). This item is done when owner closeout/archive sync is complete. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-073-qa-scaffold-truthfulness-gates-bl067-bl068.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-073 |
| Priority | P1 |
| Status | In Validation (clean verification merged via PR #6; owner closeout sync pending) |
| Track | G - Release/Governance |
| Effort | Med / M |
| Depends On | — |
| Blocks | BL-067, BL-068 |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Prevent false-green promotions by separating contract-only and execute-mode QA semantics for BL-067/BL-068 and enforcing execute-mode failure when runtime matrix rows remain scaffold/TODO.

## Acceptance IDs

- BL-067 and BL-068 QA lanes expose explicit `--contract-only` and `--execute` modes.
- Execute mode fails whenever required matrix rows contain `TODO` outcomes.
- Promotion checklists reject evidence bundles with scaffold-only execute rows.
- Status/evidence packets clearly distinguish contract scaffolding from runtime execution.

## Validation Plan

QA harness script: `scripts/qa-bl073-scaffold-truthfulness-gates-mac.sh`.
Evidence schema: `TestEvidence/bl073_*/status.tsv`.

Minimum evidence additions:
- `mode_semantics_contract.tsv`
- `todo_row_enforcement.tsv`
- `promotion_gate_policy.md`
- `bl067_bl068_matrix_reconcile.tsv`

Verification evidence (2026-03-03 clean worktree replay):
- `TestEvidence/bl073_truthfulness_20260303T005655Z/status.tsv` (`--contract-only --runs 3`)
- `TestEvidence/bl073_truthfulness_20260303T005659Z/status.tsv` (`--execute --runs 3`)
- PR: `https://github.com/joshband/LocusQ/pull/6`

### Mode Semantics Contract

- `--contract-only`: BL-067 and BL-068 are run in contract mode. TODO/SCAFFOLD rows are allowed, but each lane run must exit `0`.
- `--execute`: BL-067 and BL-068 are run in execute mode. TODO/SCAFFOLD rows are promotion-blocking and lane exit must match that truthfulness contract.
- `--runs <N>`: deterministic replay count. Every run is independently evaluated and logged in the artifact matrices.

### Exact Execute Failure Criteria

1. For each lane run, scan lane TSV outputs and count rows where any cell is `TODO` or `SCAFFOLD`.
2. Execute mode is promotion-blocking and must return non-zero if any run contains TODO/SCAFFOLD rows.
3. Execute mode fails on false-green if `scaffold_rows > 0` and lane exit code is `0`.
4. Execute mode fails on false-red if `scaffold_rows = 0` and lane exit code is non-zero.
5. Execute mode passes only when all runs have zero TODO/SCAFFOLD rows and lane exit equals expected exit (`0`).

### Validation Commands

```bash
./scripts/qa-bl073-scaffold-truthfulness-gates-mac.sh --contract-only --runs 3
./scripts/qa-bl073-scaffold-truthfulness-gates-mac.sh --execute --runs 3
```

Expected evidence path pattern:

- `TestEvidence/bl073_truthfulness_<timestamp>/mode_semantics_contract.tsv`
- `TestEvidence/bl073_truthfulness_<timestamp>/todo_row_enforcement.tsv`
- `TestEvidence/bl073_truthfulness_<timestamp>/promotion_gate_policy.md`
- `TestEvidence/bl073_truthfulness_<timestamp>/bl067_bl068_matrix_reconcile.tsv`

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
