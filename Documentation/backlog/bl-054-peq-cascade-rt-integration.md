Title: BL-054 PEQ Cascade RT Integration
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-07

# BL-054 PEQ Cascade RT Integration

## Plain-Language Summary

BL-054 in plain terms: Integrate PeqBiquadCascade (8-band RBJ, already implemented) into the monitoring chain after Steam Audio binaural output. Current state: In Validation (atomic preset publish path landed; contract/execute lane + native build PASS). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Headphone users, companion-app operators, QA/release owners, and audio-engine maintainers. |
| What is changing? | Integrate PeqBiquadCascade (8-band RBJ, already implemented) into the monitoring chain after Steam Audio binaural output. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: In Validation (atomic preset publish path landed; contract/execute lane + native build PASS). This item is done when required acceptance checks pass and promotion evidence is complete. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-054-peq-cascade-rt-integration.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


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
| ID | BL-054 |
| Priority | P1 |
| Status | In Validation (atomic preset publish path landed; contract/execute lane + native build PASS) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-052 |
| Blocks | BL-056 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Integrate `PeqBiquadCascade` (8-band RBJ, already implemented) into the monitoring chain after Steam Audio binaural output. Coefficient updates via off-thread atomic swap. Load preset from `CalibrationProfile.json` on profile change.

## Acceptance IDs

- PEQ applies in processBlock with no allocation
- coefficients swap atomically on non-RT thread
- bypass path produces identical output to no-PEQ path


## Validation Plan

QA harness script: `scripts/qa-bl054-peq-cascade-rt-integration-mac.sh`.
Evidence schema: `TestEvidence/bl054_*/status.tsv`.

Minimum evidence additions:
- `rt_swap_contract.tsv`
- `bypass_identity_contract.tsv`
- `monitor_chain_order.tsv`

Script modes and gates:
- `--contract-only` (default): structural contract checks with evidence capture.
- `--execute`: execute gate checks with zero-`TODO`-row semantics.
- Exit semantics: `0` pass, `1` gate fail, `2` usage/config error.

## Implementation Snapshot (2026-03-07)

- Remediation landed in the PEQ runtime path:
  - `Source/headphone_dsp/HeadphonePeqHook.h` now uses double-buffered coefficient banks plus release/acquire active-bank publication instead of live stage mutation.
  - `Source/headphone_dsp/HeadphoneCalibrationChain.h` now exposes single-call `applyPeqPreset(...)` rather than piecemeal stage writes.
  - `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp` now builds bundled/JSON presets off the audio thread and publishes them atomically.
- QA lane authored: `scripts/qa-bl054-peq-cascade-rt-integration-mac.sh`.
- Fresh evidence:
  - Contract: `TestEvidence/bl054_peq_cascade_rt_integration_20260307T061821Z_73138/status.tsv` -> `lane_result=PASS`
  - Execute: `TestEvidence/bl054_peq_cascade_rt_integration_20260307T061821Z_73139/status.tsv` -> `lane_result=PASS`
  - Compile safety: `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8` -> `PASS` (warnings only)

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
