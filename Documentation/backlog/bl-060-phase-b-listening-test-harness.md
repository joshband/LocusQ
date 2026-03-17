Title: BL-060 Phase B Listening Test Harness + Evaluation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-17

# BL-060 Phase B Listening Test Harness + Evaluation

## Plain-Language Summary

BL-060 in plain terms: Execute Phase B 2×2 blind listening test (generic vs personalized HRTF × no EQ vs WH-1000XM5 EQ) across >=5 participants x >=10 scenes. Current state: In Validation (harness T1+T2 PASS; fixture gate PASS 45.5% ext improvement p<0.0001). Blocked on ≥5 real participant sessions. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA owners, release owners, and engineering maintainers who depend on deterministic evidence. |
| What is changing? | Execute Phase B 2×2 blind listening test (generic vs personalized HrealtimeF × no EQ vs WH-1000XM5 EQ) across >=5 participants x >=10 scenes. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: In Validation (harness T1+T2 PASS; fixture gate 45.5% ext improvement, p<0.0001; reproducibility hash stable). Done when ≥5 real participants complete full session and gate evidence is committed to TestEvidence. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-060-phase-b-listening-test-harness.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


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
| ID | BL-060 |
| Priority | P1 |
| Status | In Validation (harness T1 PASS 2026-03-17: `TestEvidence/bl060_phase_b_listening_20260317T174025Z_90778/`; T2 3/3 PASS 2026-03-17: `TestEvidence/bl060_phase_b_listening_20260317T174918Z_99523/` et al.; fixture gate 45.5% ext improvement, p<0.0001, reproducibility hash stable; blocked on ≥5 real participant sessions) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-059 |
| Blocks | BL-061 (conditional) |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Execute Phase B 2×2 blind listening test (generic vs personalized HRTF × no EQ vs WH-1000XM5 EQ) across >=5 participants x >=10 scenes. Run reproducible statistical analysis and persist machine-readable results. Gate: >=20% mean externalization improvement OR p<0.05 localization gain.

## Acceptance IDs

- ≥5 participants complete full session
- analysis script exits 0
- Phase B gate result recorded in `verification` fields of CalibrationProfile
- result documented in TestEvidence
- blind trial logs contain at minimum: condition, true angle, response angle, absolute error, reaction time
- per-condition metrics are exported: MAE, front/back confusion rate, externalization summary
- run-to-run reproducibility packet exists (same input session => stable stats output)

## Methodology Reference

- Canonical methodology: `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`.
- Reconciliation review: `Documentation/reviews/2026-03-01-headtracking-research-backlog-reconciliation.md`.
- This backlog item must follow the blinded 2x2 protocol and gate criteria defined in that methodology.


## Validation Plan

QA harness script: `scripts/qa-bl060-phase-b-listening-test-mac.sh`.
Analysis script: `scripts/bl060-analyze-results.py`.
Evidence schema: `TestEvidence/bl060_*/status.tsv`.

Required analysis artifacts:
- `trial_log.csv`
- `metrics_summary.tsv`
- `stats_report.md`
- `gate_decision.md`
- `reproducibility_check.tsv`

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

## Governance Alignment (2026-02-28)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

