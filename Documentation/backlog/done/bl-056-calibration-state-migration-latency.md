Title: BL-056 Calibration State Migration + Latency Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-17 (Z1 Done-candidate promotion)

# BL-056 Calibration State Migration + Latency Contract

## Plain-Language Summary

BL-056 in plain terms: Bump plugin state_version, serialize new headphone calibration parameters in getStateInformation/setStateInformation. Current state: Open. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Headphone users, companion-app operators, QA/release owners, and audio-engine maintainers. |
| What is changing? | Bump plugin state_version, serialize new headphone calibration parameters in getStateInformation/setStateInformation. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: Done-candidate (Z1 owner sync 2026-03-17; T3 10/10 PASS). Formal Done when BL-054 + BL-055 are formally Done. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-056-calibration-state-migration-latency.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


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
| ID | BL-056 |
| Priority | P1 |
| Status | **Done** (Z1 owner sync 2026-03-17: T3 10/10 PASS; BL-054 + BL-055 Done gates met; archive sync complete 2026-03-17) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-054, BL-055 |
| Blocks | BL-059 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Bump plugin `state_version`, serialize new headphone calibration parameters in getStateInformation/setStateInformation. Regenerate golden state snapshots. Ensure reported latency resets to 0 on bypass.

## Acceptance IDs

- state migration is idempotent (old state loads cleanly)
- golden snapshots regenerated and committed
- latency = 0 when calibration is bypassed


## Implementation Snapshot (2026-03-17)

- Added `kSnapshotSchemaValueV3 = "locusq-state-v3"` to `Source/processor_core/ProcessorConstants.h`.
- Updated `getStateInformation` in `Source/processor_core/ProcessorStateSerializer.cpp` to write V3 schema.
- Added V2→V3 migration comments documenting transparent migration contract.
- V2→V3 migration is transparent: no new mandatory state fields; PEQ/FIR/SOFA data re-polled from CalibrationProfile.json on startup.
- Latency-zero-on-disable confirmed: `resolveCalibrationChainState` returns `activeLatencySamples=0` when `!request.enabled`.
- `setLatencySamples(0)` on bypass confirmed in `PluginProcessor.cpp` bypass path.
- QA harness authored: `scripts/qa-bl056-calibration-state-migration-mac.sh`.
- Execute lane PASS (10/10): `TestEvidence/bl056_calibration_state_migration_20260317T045411Z/status.tsv`

## Validation Plan

QA harness script: `scripts/qa-bl056-calibration-state-migration-mac.sh`.
Evidence schema: `TestEvidence/bl056_*/status.tsv`.

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

