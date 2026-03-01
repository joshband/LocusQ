Title: BL-070 Coherent Audio Snapshot and Telemetry Seqlock Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-070 Coherent Audio Snapshot and Telemetry Seqlock Contract

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-070 |
| Priority | P0 |
| Status | In Implementation (Wave 1 kickoff: coherent snapshot + atomic telemetry publication landed) |
| Track | F - Hardening |
| Effort | Med / M |
| Depends On | BL-050 |
| Blocks | — |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Eliminate torn snapshot and telemetry race risks by introducing coherent audio snapshot reads (`ptr + sample_count` from one publication point) and sequence-safe telemetry publication/consumption between audio and bridge/UI threads.

## Acceptance IDs

- Audio snapshot consumers read coherent tuples (`buffer pointer`, `num samples`, `valid`) from one publication epoch.
- Cross-thread telemetry reads are sequence-consistent and race-free under concurrent polling.
- Scene-state bridge contracts preserve deterministic output under high-frequency UI polling.
- Concurrency validation lane reports zero contract drift under stress replay.

## Validation Plan

QA harness script: `scripts/qa-bl070-snapshot-telemetry-contract-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl070_*/status.tsv`.

Minimum evidence additions:
- `snapshot_coherency.tsv`
- `telemetry_seqlock_contract.tsv`
- `scene_bridge_stress.tsv`
- `tsan_or_equivalent_report.md`

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
  - `Source/SceneGraph.h` adds `readAudioSnapshot()` for coherent tuple reads (`mono`, `numSamples`, `valid`).
  - `Source/SpatialRenderer.h` and `Source/processor_bridge/ProcessorSceneStateBridgeOps.h` now consume coherent snapshots instead of split reads.
  - `Source/PluginProcessor.h/.cpp` and bridge ops now use atomic telemetry fields for speaker RMS and perf EMA publication/reads.
- Remaining BL-070 scope:
  - Add sequence-stamped telemetry packet semantics (if seqlock contract is kept as strict requirement).
  - Add dedicated stress validation lane (`scene_bridge_stress.tsv` / thread-safety diagnostics).
