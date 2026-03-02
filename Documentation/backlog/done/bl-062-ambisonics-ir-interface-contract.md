Title: BL-062 Ambisonics IR Interface Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-02

# BL-062 Ambisonics IR Interface Contract

## Plain-Language Summary

This runbook tracks **BL-062** (BL-062 Ambisonics IR Interface Contract). Current status: **Done**. In plain terms: Define the canonical ambisonics intermediate-representation (IR) interface contract, including frame semantics, channel-order policy, ownership boundaries, and deterministic validation artifacts used by downstream lanes.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-062 Ambisonics IR Interface Contract |
| Why is this important? | Define the canonical ambisonics intermediate-representation (IR) interface contract, including frame semantics, channel-order policy, ownership boundaries, and deterministic validation artifacts used by downstream lanes. |
| How will we deliver it? | Use the validation plan and evidence bundle contract in this runbook to prove behavior and safety before promotion. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-062-ambisonics-ir-interface-contract.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| ID | BL-062 |
| Priority | P2 |
| Status | Done |
| Track | E - R&D Expansion |
| Effort | High / L |
| Depends On | BL-051 |
| Blocks | BL-063 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Define the canonical ambisonics intermediate-representation (IR) interface contract, including frame semantics, channel-order policy, ownership boundaries, and deterministic validation artifacts used by downstream lanes.

## Scope

In scope:
- IR schema (fields, types, required invariants).
- Ownership/lifetime contract across producer and consumer boundaries.
- Deterministic contract evidence format for downstream reuse.

Out of scope:
- Runtime implementation in production render paths.
- Export UI and host workflow authoring.

## Architecture Definition

### Core Components

| Component | Responsibility | Output |
|---|---|---|
| `IRFrameSchema` | Defines canonical frame fields and type constraints | `ir_contract.tsv` schema rows |
| `ChannelOrderContract` | Formalizes HOA order/normalization/channel sequence rules | `interface_map.tsv` order map |
| `FrameOwnershipModel` | Declares allocation/ownership transfer and mutation boundaries | ownership invariants |
| `IRInvariantValidator` | Encodes hard-fail checks for invalid frames/contracts | deterministic pass/fail rows |

### Processing Chain

`Emitter Snapshot -> IR Frame Builder -> Invariant Validator -> Contract Artifact Writer -> Downstream Consumers (BL-063/064/065)`

### Contract Parameter Mapping

| Parameter | Component | Function | Allowed Values |
|---|---|---|---|
| `order` | `ChannelOrderContract` | HOA order declaration | FOA/HOA integer order set |
| `normalization` | `ChannelOrderContract` | Normalization policy | SN3D or N3D (explicit) |
| `channel_count` | `IRFrameSchema` | Layout consistency guard | positive integer, order-matched |
| `frame_id` | `IRFrameSchema` | Replay/determinism identity | monotonic unsigned integer |
| `timestamp_samples` | `IRFrameSchema` | Sample-accurate sequencing | non-decreasing integer |
| `ownership_token` | `FrameOwnershipModel` | Mutation boundary enforcement | immutable token/id pair |

## Complexity Assessment

- Score: `4/5`
- Rationale: Multi-lane contract authority with strict determinism requirements and cross-lane schema coupling; error in this lane propagates to BL-063/064/065.

## Implementation Plan

### Strategy

Phased implementation (score >=3).

### Phase 2.1.1: Contract Baseline
- [ ] Freeze `IRFrameSchema` required fields and nullability policy.
- [ ] Define canonical channel-order and normalization matrix.
- [ ] Encode ownership/lifetime contract boundaries.

### Phase 2.1.2: Determinism Instrumentation
- [ ] Define row-stable `ir_contract.tsv` serialization contract.
- [ ] Define invariant validator rule IDs and failure taxonomy.
- [ ] Add replay signature fields for downstream diffing.

### Phase 2.1.3: Intake Readiness
- [ ] Produce owner-readable lane notes and unresolved assumptions.
- [ ] Confirm BL-063 dependency contract references are complete.
- [ ] Mark promotion conditions for BL-062 -> BL-063 handoff.

## Dependencies and Entry/Exit Gates

Entry gates:
- BL-051 A1b/C1 planning outputs accepted.
- Canonical dependency row present in `Documentation/backlog/index.md`.

Exit gates:
- IR schema and ownership contract marked stable for BL-063 consumption.
- Deterministic evidence schema published in this runbook.

## Risk Assessment

High risk:
- Schema churn after downstream lanes begin implementation.
- Ambiguous ownership semantics causing runtime regressions.

Medium risk:
- Incomplete normalization/order constraints for edge layouts.
- Inconsistent artifact serialization affecting parity checks.

Low risk:
- Documentation linkage and evidence path maintenance.


## Validation Plan

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `validation_matrix.tsv`
- `ir_contract.tsv`
- `interface_map.tsv`
- `docs_freshness.log`

## BL-062..BL-066 Planning Packet (latest)

- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl062/status.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl062/validation_matrix.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl062/ir_contract.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl062/interface_map.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl062/risk_register.tsv`

## BL-062 I1 Implementation Kickoff

-  adds  with deterministic frame/timestamp/order/normalization/channel-count contract fields.
-  publishes the contract as  and  JSON fields for downstream lanes.

Evidence:
- 
- 
- 
- 
- 
- 

### Evidence Paths (canonical)

- TestEvidence/bl062_slice_i1_ir_contract_scaffold_20260228_131636/status.tsv
- TestEvidence/bl062_slice_i1_ir_contract_scaffold_20260228_131636/validation_matrix.tsv
- TestEvidence/bl062_slice_i1_ir_contract_scaffold_20260228_131636/ir_contract.tsv
- TestEvidence/bl062_slice_i1_ir_contract_scaffold_20260228_131636/interface_map.tsv
- TestEvidence/bl062_slice_i1_ir_contract_scaffold_20260228_131636/blocker_taxonomy.tsv
- TestEvidence/bl062_slice_i1_ir_contract_scaffold_20260228_131636/lane_notes.md

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

## Owner Progression Sync (I3 PASS)

Owner packet:
- TestEvidence/bl062_bl066_owner_readiness_20260228_152312/status.tsv
- TestEvidence/bl062_bl066_owner_readiness_20260228_152312/validation_matrix.tsv
- TestEvidence/bl062_bl066_owner_readiness_20260228_152312/blocker_taxonomy.tsv
- TestEvidence/bl062_bl066_owner_readiness_20260228_152312/lane_notes.md
- TestEvidence/bl062_bl066_owner_readiness_20260228_152312/owner_decision.md

Latest validation run:
- TestEvidence/bl064_bl066_i3_validation_run_20260228_151230/build.log
- TestEvidence/bl064_bl066_i3_validation_run_20260228_151230/qa_smoke.log
- TestEvidence/bl064_bl066_i3_validation_run_20260228_151230/selftest.log
- TestEvidence/bl064_bl066_i3_validation_run_20260228_151230/docs_freshness.log

## Sequential Owner Closeout

Owner decision: Done-candidate accepted.

Owner bundle:
- TestEvidence/bl062_bl066_owner_sequential_closeout_20260228_152703/status.tsv
- TestEvidence/bl062_bl066_owner_sequential_closeout_20260228_152703/validation_matrix.tsv
- TestEvidence/bl062_bl066_owner_sequential_closeout_20260228_152703/lane_decisions.tsv
- TestEvidence/bl062_bl066_owner_sequential_closeout_20260228_152703/blocker_taxonomy.tsv
- TestEvidence/bl062_bl066_owner_sequential_closeout_20260228_152703/owner_notes.md
