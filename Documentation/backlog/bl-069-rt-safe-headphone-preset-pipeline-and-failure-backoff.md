Title: BL-069 RT-Safe Headphone Preset Pipeline and Failure Backoff
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-069 RT-Safe Headphone Preset Pipeline and Failure Backoff

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-069 |
| Priority | P0 |
| Status | In Implementation (Wave 1 kickoff: cache-only preset load path landed in runtime code) |
| Track | F - Hardening |
| Effort | Med / M |
| Depends On | BL-050 |
| Blocks | — |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Remove RT-unsafe file/config loading from the headphone preset path by moving preset hydration and parse work out of `processBlock()`, introducing atomic runtime handoff for prepared coefficients, and enforcing retry backoff semantics when preset assets are missing or invalid.

## Acceptance IDs

- No filesystem access, parse work, or blocking I/O is executed from `processBlock()` during profile changes.
- Missing/invalid preset assets do not retrigger load attempts every callback block.
- Prepared preset coefficients are atomically swapped into audio path without discontinuities.
- Failure/backoff diagnostics are visible in scene/runtime status payloads.

## Validation Plan

QA harness script: `scripts/qa-bl069-rt-safe-preset-pipeline-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl069_*/status.tsv`.

Minimum evidence additions:
- `rt_access_audit.tsv`
- `preset_retry_backoff.tsv`
- `coefficient_swap_stability.tsv`
- `failure_taxonomy.tsv`

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

- Initial remediation landed in runtime code:
  - `Source/SpatialRenderer.h` now preloads bundled PEQ presets during `prepare()`.
  - `loadPeqPresetForProfile()` now uses cache-only preset data (no filesystem access on callback path).
  - Failed/missing preset states are now cached through invalid preset entries and no longer trigger per-block file retries.
- Remaining BL-069 scope:
  - Add explicit QA harness lane and backoff evidence packet (`preset_retry_backoff.tsv`).
  - Add diagnostics surfacing for cache hit/miss/backoff reason in bridge payloads.
