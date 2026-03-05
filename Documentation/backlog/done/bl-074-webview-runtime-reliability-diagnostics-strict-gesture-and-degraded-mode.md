Title: BL-074 WebView Runtime Reliability Diagnostics (Strict Gesture and Degraded Mode)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-05

# BL-074 WebView Runtime Reliability Diagnostics (Strict Gesture and Degraded Mode)

## Plain-Language Summary

BL-074 hardens WebView startup trust by enforcing strict gesture behavior, forcing degraded mode when critical native bindings are unavailable, and surfacing centralized native/bridge diagnostics to operators. Current state: Done (deterministic contract + execute evidence PASS; shared-control closeout sync complete and runbook archived).

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Operators, QA/release owners, and UI/runtime maintainers. |
| What is changing? | Strict gesture enforcement, degraded-mode startup control locking, and centralized native/bridge diagnostics surfacing. |
| Why is this important? | It turns ambiguous startup/runtime failures into deterministic states and machine-checked evidence. |
| How is confidence established? | Replay lane checks for strict gesture, degraded controls, and native diagnostics in `contract-only` and `execute` modes. |
| When is this done? | Done as of 2026-03-05 after owner/orchestrator shared-control closeout sync and archive transition. |
| Where is evidence? | `TestEvidence/bl074_webview_reliability_*` packets plus BL-074 owner-sync packet/report artifacts. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Evidence + acceptance reconciliation table | One place to verify each acceptance line against deterministic evidence. | `## Evidence And Acceptance Reconciliation` |
| Operator diagnostics notes | Non-technical interpretation of diagnostics PASS/FAIL meaning. | `## Operator Diagnostics Interpretation` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-074 |
| Priority | P1 |
| Status | **Done** (deterministic execute evidence PASS; shared-control closeout sync complete) |
| Track | B - Scene/UI Runtime |
| Effort | Med / M |
| Depends On | BL-040, BL-067 |
| Blocks | — |
| Annex Spec | `(no annex spec — self-contained runbook + diagnostics contract tables)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |

## Objective

Improve WebView runtime trust by:
- failing self-test when fallback gesture mutation paths are used under `strict_gesture`,
- entering deterministic degraded mode and disabling native-dependent controls on critical startup binding failures,
- exposing centralized native/bridge diagnostics for operator and owner decisions.

## Acceptance IDs

- `BL074-A-001`: strict gesture enforcement fails fallback mutation paths.
- `BL074-A-002`: degraded mode is deterministic and disables impacted controls.
- `BL074-A-003`: native/bridge failures are surfaced through centralized diagnostics channels.
- `BL074-A-004`: deterministic counters and schemas are present in operator + scene payload diagnostics.

## Validation Commands

```bash
./scripts/qa-bl074-webview-reliability-diagnostics-mac.sh --contract-only --runs 3
./scripts/qa-bl074-webview-reliability-diagnostics-mac.sh --execute --runs 3
./scripts/validate-docs-freshness.sh
```

## Evidence And Acceptance Reconciliation

| Acceptance | Required Surface | Result | Evidence |
|---|---|---|---|
| `BL074-A-001` strict gesture enforcement | `BL074-SG-001..004` in `strict_gesture_matrix.tsv` | PASS (`contract_only` and `execute`) | `TestEvidence/bl074_webview_reliability_20260305T004455Z/strict_gesture_matrix.tsv`; `TestEvidence/bl074_webview_reliability_20260305T010550Z/strict_gesture_matrix.tsv` |
| `BL074-A-002` degraded mode control lock | `BL074-DM-001..006` in `degraded_mode_contract.tsv` | PASS (`contract_only` and `execute`) | `TestEvidence/bl074_webview_reliability_20260305T004455Z/degraded_mode_contract.tsv`; `TestEvidence/bl074_webview_reliability_20260305T010550Z/degraded_mode_contract.tsv` |
| `BL074-A-003` native error surface | `BL074-NE-001..007` in `native_error_surface.tsv` | PASS (`contract_only` and `execute`) | `TestEvidence/bl074_webview_reliability_20260305T004455Z/native_error_surface.tsv`; `TestEvidence/bl074_webview_reliability_20260305T010550Z/native_error_surface.tsv` |
| `BL074-A-004` deterministic lane counters/schema | `lane_result` counters (`strict=0;degraded=0;native=0`) and diagnostics schema summary | PASS | `...20260305T004455Z/status.tsv`; `...20260305T010550Z/status.tsv`; `...20260305T010550Z/operator_diagnostics_snapshot.md` |
| Owner packet readiness | promotion + owner decisions + handoff artifacts | PASS (prepared + final closeout decision) | `TestEvidence/bl074_owner_sync_z1_20260303T225640/promotion_decision.md`; `.../owner_decisions.md`; `.../handoff_resolution.md`; `Documentation/reports/owner-sync/bl-074-owner-sync-20260305T010558Z.md`; `Documentation/reports/owner-sync/bl-074-owner-sync-20260305T011420Z.md` |

## Operator Diagnostics Interpretation

- If all three matrices show `PASS`, the WebView runtime contract is healthy: strict gesture checks are active, degraded-mode guards are wired, and native diagnostics channels are available.
- If degraded-mode checks fail, operators should assume native-dependent controls may remain incorrectly interactive during startup faults; this is a release blocker.
- If native-error surface checks fail, operators lose reliable error attribution (timeout/unavailable/blocked), which weakens incident triage and promotion confidence.
- The `operator_diagnostics_snapshot.md` file is the fastest non-technical summary: it reports schema, mode, run count, and whether strict/degraded/native surfaces passed.

## Owner Sync Artifacts

- Existing owner packet: `TestEvidence/bl074_owner_sync_z1_20260303T225640/`
- Pre-closeout readiness report: `Documentation/reports/owner-sync/bl-074-owner-sync-20260305T010558Z.md`
- Final closeout decision report: `Documentation/reports/owner-sync/bl-074-owner-sync-20260305T011420Z.md`

## Handoff Return Contract

Use the canonical owner sync packet contract from `Documentation/backlog/index.md` and include `SHARED_FILES_TOUCHED: no|yes` in handoff output.
