Title: BL-059 CalibrationProfile Integration Handoff
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-17 (Z1 Done promotion)

# BL-059 CalibrationProfile Integration Handoff

## Plain-Language Summary

BL-059 in plain terms: Wire CalibrationProfile.json from companion to plugin state end-to-end. Current state: In Validation (fixture-driven contract+execute smoke PASS with BL-053/BL-055 dependency replays green on 2026-03-07). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Headphone users, companion-app operators, QA/release owners, and audio-engine maintainers. |
| What is changing? | Wire CalibrationProfile.json from companion to plugin state end-to-end. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: **Done** (2026-03-17 formal promotion; Z1 owner sync 2026-03-16 PASS; execute smoke 11/11; BL-053/BL-055 deps green; BL-056 Done gate met). |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-059-calibration-profile-integration-handoff.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Evidence visual snapshot | Consolidated replay/evidence view for promotion decisions. | `## Evidence Visual Snapshot` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Evidence Visual Snapshot

| Evidence Artifact | Purpose | Path |
|---|---|---|
| Integration status packet | Single run-level pass/fail and blockers | `TestEvidence/bl059_calibration_integration_smoke_<timestamp>_<pid>/status.tsv` |
| Contract matrix | Verify handoff invariants remain deterministic | `TestEvidence/bl059_calibration_integration_smoke_<timestamp>_<pid>/contract_matrix.tsv` |
| Replay hashes | Verify run-to-run deterministic shape | `TestEvidence/bl059_calibration_integration_smoke_<timestamp>_<pid>/replay_hashes.tsv` |

```mermaid
flowchart LR
    A[Companion emits CalibrationProfile] --> B[Plugin ingest + state sync]
    B --> C[Smoke/contract replay lanes]
    C --> D[Owner intake packet]
```

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-059 |
| Priority | P0 |
| Status | **Done** (Z1 owner sync 2026-03-16: execute smoke 11/11 PASS; BL-053/BL-055 dependency replays PASS; BL-056 Done gate met; formal Done 2026-03-17; promotion packet at `TestEvidence/bl059_owner_sync_z1_20260316T050200Z/`) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-052, BL-053, BL-054, BL-055, BL-056, BL-057, BL-058 |
| Blocks | BL-060 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Wire `CalibrationProfile.json` from companion to plugin state end-to-end. Device-profile fields update APVTS state, and PEQ/FIR/SOFA/tracking fields update renderer/runtime state without glitches. Plugin reloads on file change and clears runtime state cleanly when the profile disappears.

## Acceptance IDs

- profile load/unload cycle is stable (no glitches)
- SOFA swap is atomic
- APVTS params update on profile change
- smoke test `qa-bl059-calibration-integration-smoke-mac.sh` exits 0
- orientation/tracking fields preserve BL-053 invariants (stale fallback, yaw composition, deterministic behavior)
- profile reload path preserves packet-age/sequence diagnostics visibility for companion handoff debugging

## Methodology Reference

- Canonical methodology: `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`.
- Reconciliation review: `Documentation/reviews/2026-03-01-headtracking-research-backlog-reconciliation.md`.
- Integration acceptance should preserve the orientation-path invariants validated by BL-053 (stale fallback, yaw offset composition, deterministic behavior).


## Validation Plan

QA harness script: `scripts/qa-bl059-calibration-integration-smoke-mac.sh`.
Evidence schema: `TestEvidence/bl059_*/status.tsv`.

Primary dev-loop commands:
- `./scripts/qa-bl059-calibration-integration-smoke-mac.sh --contract-only`
- `./scripts/qa-bl059-calibration-integration-smoke-mac.sh --execute`

Required dependency replays in execute mode:
- `./scripts/qa-bl053-head-tracking-orientation-injection-mac.sh`
- `./scripts/qa-bl055-fir-convolution-engine-mac.sh --execute`

## Validation Refresh Snapshot (2026-03-07)

- Contract-only PASS: `TestEvidence/bl059_calibration_integration_smoke_20260307T063151Z_93321/status.tsv`
- Execute PASS: `TestEvidence/bl059_calibration_integration_smoke_20260307T063250Z_94423/status.tsv`
- Runtime smoke scenarios green: AirPods PEQ, Sony PEQ, custom SOFA+FIR, and missing-profile unload recovery
- Dependency replays green inside execute lane: BL-053 orientation injection and BL-055 FIR convolution engine
- Harness/runtime deltas included in this refresh:
  - sandbox-safe profile override seam via `LOCUSQ_COMPANION_PROFILE_FILE` / `LOCUSQ_COMPANION_PROFILE_DIR`
  - contract matrix now checks profile clear path, APVTS sync, SOFA/FIR/tracking ingest, UI status publication, and renderer tracking diagnostics

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
