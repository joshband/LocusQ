Title: BL-068 Temporal Effects Core (Delay/Echo/Looper/Frippertronics)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-068 Temporal Effects Core (Delay/Echo/Looper/Frippertronics)

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-068 |
| Priority | P2 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-050, BL-055 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-068-temporal-effects-core-spec-2026-03-01.md` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Define and integrate a deterministic temporal-effects core spanning delay/echo, controlled feedback behavior, and looper/frippertronics-style layering that remains realtime-safe and host-automation reliable.

## Acceptance IDs

- Delay/echo timing and feedback behavior are stable from 44.1kHz through 192kHz.
- Feedback-network safety ceiling prevents runaway/non-finite output in stress lanes.
- Looper overdub/clear/transport interactions are deterministic on session recall.
- Parameter automation and mode transitions are click-safe and zipper-safe.
- Temporal-effect lanes remain compatible with existing spatial and FIR paths.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Delay/echo and bounded feedback architecture | finite-output and runaway-guard lanes pass |
| B | Looper + frippertronics-style layering behavior | transport/recall lanes pass without drift or clicks |
| C | Evidence and visualization handshake contracts | deterministic replay + telemetry evidence packet captured |

## Validation Plan

QA harness script: `scripts/qa-bl068-temporal-effects-mac.sh`.
Evidence schema: `TestEvidence/bl068_*/status.tsv`.

Minimum evidence additions:
- `temporal_matrix.tsv` (delay/echo/looper scenario results)
- `runaway_guard.tsv` (feedback safety + finite-output checks)
- `transport_recall.tsv` (timeline/recall determinism checks)
- `cpu_latency_budget.tsv` (sample-rate and topology budget snapshots)

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
