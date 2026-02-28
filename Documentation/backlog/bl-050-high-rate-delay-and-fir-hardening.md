Title: BL-050 High-Rate Delay and FIR Hardening
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-28

# BL-050 High-Rate Delay and FIR Hardening

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-050 |
| Priority | P2 |
| Status | In Planning |
| Track | F - Hardening |
| Effort | Med / M |
| Depends On | BL-043, BL-046 |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Harden high-sample-rate behavior by expanding delay headroom and defining a path from direct FIR convolution toward partitioned FIR for scalability.

## Scope

In scope:
- Delay max bounds suitable for 192kHz operation.
- FIR path profiling and partitioned FIR migration contract.
- Deterministic high-rate stress validation.

Out of scope:
- Full ambisonics architecture shift.
- New UI features.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Delay-range hardening for high-rate operation | Delay range validation passes at 192kHz |
| B | Partitioned FIR migration contract and prototype lane | FIR scalability metrics captured |
| C | High-rate soak and evidence closeout | High-rate replay lane passes deterministically |

## TODOs

- [ ] Validate and adjust delay max bounds for 192kHz support.
- [ ] Define partitioned FIR migration contract (latency/cpu/quality bounds).
- [ ] Add high-rate soak lane for delay + FIR paths.
- [ ] Add deterministic failure taxonomy for high-rate regressions.
- [ ] Capture promotion evidence and update runbook status.


## Validation Plan

- `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
- `./scripts/qa-bl050-highrate-lane-mac.sh --out-dir TestEvidence/bl050_<slice>_<timestamp>`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `build.log`
- `highrate_matrix.tsv`
- `fir_profile.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

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


## Governance Alignment (2026-02-28)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).
