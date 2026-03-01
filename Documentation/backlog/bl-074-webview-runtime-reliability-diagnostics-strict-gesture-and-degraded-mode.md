Title: BL-074 WebView Runtime Reliability Diagnostics (Strict Gesture and Degraded Mode)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-074 WebView Runtime Reliability Diagnostics (Strict Gesture and Degraded Mode)

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-074 |
| Priority | P1 |
| Status | Open |
| Track | B - Scene/UI Runtime |
| Effort | Med / M |
| Depends On | BL-040, BL-067 |
| Blocks | — |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Improve WebView runtime trust by making gesture-path failures explicit in self-test CI (`strict_gesture` mode), surfacing native-call binding failures in an operator-visible diagnostics channel, and introducing a deterministic degraded mode when critical startup bindings fail.

## Acceptance IDs

- Self-test supports strict gesture mode that fails when fallback mutation paths are used.
- Critical startup binding failures force explicit degraded mode and disable impacted controls.
- Timeline/native bridge failures are surfaced in a centralized diagnostics channel.
- Runtime diagnostics include deterministic counters for binding and native-call failures.

## Validation Plan

QA harness script: `scripts/qa-bl074-webview-reliability-diagnostics-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl074_*/status.tsv`.

Minimum evidence additions:
- `strict_gesture_matrix.tsv`
- `degraded_mode_contract.tsv`
- `native_error_surface.tsv`
- `operator_diagnostics_snapshot.md`

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
