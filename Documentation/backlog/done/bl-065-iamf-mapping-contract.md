Title: BL-065 IAMF Mapping Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-02

# BL-065 IAMF Mapping Contract

## Plain-Language Summary

This runbook tracks **BL-065** (BL-065 IAMF Mapping Contract). Current status: **Done**. In plain terms: Define deterministic mapping contract from ambisonics IR into IAMF scene/profile outputs, including transform rules, profile constraints, parity evidence schema, and blocker routing.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-065 IAMF Mapping Contract |
| Why is this important? | Define deterministic mapping contract from ambisonics IR into IAMF scene/profile outputs, including transform rules, profile constraints, parity evidence schema, and blocker routing. |
| How will we deliver it? | Use the validation plan and evidence bundle contract in this runbook to prove behavior and safety before promotion. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-065-iamf-mapping-contract.md` plus repo-local evidence under `TestEvidence/...`. |

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
| ID | BL-065 |
| Priority | P2 |
| Status | Done |
| Track | E - R&D Expansion |
| Effort | Medium / M |
| Depends On | BL-051 |
| Blocks | BL-066 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Define deterministic mapping contract from ambisonics IR into IAMF scene/profile outputs, including transform rules, profile constraints, parity evidence schema, and blocker routing.

## Scope

In scope:
- IAMF profile/scene mapping rules from IR.
- Deterministic parity and failure classification contract.
- Intake-facing blocker taxonomy and mitigation annotation format.

Out of scope:
- IAMF production exporter/runtime implementation.
- Host-specific decode/playback optimizations.

## Architecture Definition

### Core Components

| Component | Responsibility | Output |
|---|---|---|
| `IAMFProfileMap` | Maps IR semantics to IAMF profile fields | `iamf_mapping_matrix.tsv` |
| `SceneTransformRules` | Encodes coordinate/gain/metadata transforms | transform columns |
| `ProfileConstraintValidator` | Validates profile-level invariants and required fields | constraint pass/fail rows |
| `ParityReporter` | Produces deterministic parity summaries and deltas | `parity_report.tsv` |

### Processing Chain

`IR Frames -> IAMF Semantic Mapper -> Scene Transform Engine -> Profile Constraint Validator -> Parity Reporter`

### Contract Parameter Mapping

| Parameter | Component | Function | Allowed Values |
|---|---|---|---|
| `profile_id` | `IAMFProfileMap` | Targets IAMF profile template | stable profile token |
| `scene_id` | `IAMFProfileMap` | Declares scene grouping identity | immutable identifier |
| `gain_db` | `SceneTransformRules` | Level mapping | bounded decimal range |
| `position_vector` | `SceneTransformRules` | Spatial placement mapping | normalized vector contract |
| `lang_tag` | `IAMFProfileMap` | Localization metadata mapping | BCP-47 compliant tag |
| `constraint_code` | `ProfileConstraintValidator` | Deterministic failure category | stable enumerated code |

## Complexity Assessment

- Score: `4/5`
- Rationale: Standards coupling plus metadata richness increases ambiguity risk; deterministic evidence and strict constraint governance are required for safe pilot intake.

## Implementation Plan

### Strategy

Phased implementation (score >=3).

### Phase 2.1.1: Mapping Baseline
- [ ] Freeze IR->IAMF mapping matrix and required profile fields.
- [ ] Define transform rules for spatial and metadata fields.
- [ ] Define unsupported-field fallback behaviors.

### Phase 2.1.2: Constraint Governance
- [ ] Formalize profile constraint checks and severity mapping.
- [ ] Define deterministic parity schema and row identity fields.
- [ ] Align blocker taxonomy format with BL-066 intake needs.

### Phase 2.1.3: Pilot Handoff Readiness
- [ ] Publish IAMF gate checklist for BL-066 intake.
- [ ] Record unresolved spec interpretation questions.
- [ ] Mark owner escalation criteria for non-standard mappings.

## Dependencies and Entry/Exit Gates

Entry gates:
- BL-051 roadmap contract accepted.
- BL-062/BL-063 contracts available for schema and compatibility alignment (soft dependency).

Exit gates:
- IAMF mapping matrix and parity schema are finalized.
- BL-066 intake runbook references IAMF gate IDs and blocker routes.

## Risk Assessment

High risk:
- IAMF profile interpretation drift between lanes.

Medium risk:
- Metadata completeness gaps affecting deterministic parity.
- Constraint codes not stable across replay cycles.

Low risk:
- Evidence file naming and path governance.


## Validation Plan

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `validation_matrix.tsv`
- `iamf_mapping_matrix.tsv`
- `parity_report.tsv`
- `docs_freshness.log`

## BL-062..BL-066 Planning Packet (latest)

- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl065/status.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl065/validation_matrix.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl065/iamf_mapping_matrix.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl065/parity_report.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl065/risk_register.tsv`

## BL-065 I1 Implementation Kickoff

- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h` now emits `rendererIamfMappingStatus` and `rendererCodecMappingContract` telemetry for deterministic IAMF mapping readiness checks.

Evidence:
- `TestEvidence/bl065_slice_i1_mapping_contract_scaffold_20260228_132051/status.tsv`
- `TestEvidence/bl065_slice_i1_mapping_contract_scaffold_20260228_132051/validation_matrix.tsv`
- `TestEvidence/bl065_slice_i1_mapping_contract_scaffold_20260228_132051/iamf_mapping_matrix.tsv`
- `TestEvidence/bl065_slice_i1_mapping_contract_scaffold_20260228_132051/parity_report.tsv`
- `TestEvidence/bl065_slice_i1_mapping_contract_scaffold_20260228_132051/lane_notes.md`

## BL-065 I2 Runtime Mapping Execution

- Runtime mapping execution state is now sourced from `Source/SpatialRenderer.h` codec mapping snapshot and surfaced through `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`.
- BL-065 now consumes runtime mode/applied/finite/element-count/signature fields (not contract-coverage-only signals).

Evidence:
- `TestEvidence/bl065_slice_i2_runtime_mapping_execution_20260228_132744/status.tsv`
- `TestEvidence/bl065_slice_i2_runtime_mapping_execution_20260228_132744/validation_matrix.tsv`
- `TestEvidence/bl065_slice_i2_runtime_mapping_execution_20260228_132744/iamf_mapping_matrix.tsv`
- `TestEvidence/bl065_slice_i2_runtime_mapping_execution_20260228_132744/parity_report.tsv`
- `TestEvidence/bl065_slice_i2_runtime_mapping_execution_20260228_132744/lane_notes.md`

## BL-065 I3 Payload Materialization

- `Source/SpatialRenderer.h` now materializes runtime IAMF payload elements (id/gain + scene gain metadata).
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h` now publishes `rendererIamfRuntimePayload` for downstream intake and verification lanes.

Evidence:
- `TestEvidence/bl065_slice_i3_payload_materialization_20260228_133104/status.tsv`
- `TestEvidence/bl065_slice_i3_payload_materialization_20260228_133104/validation_matrix.tsv`
- `TestEvidence/bl065_slice_i3_payload_materialization_20260228_133104/iamf_mapping_matrix.tsv`
- `TestEvidence/bl065_slice_i3_payload_materialization_20260228_133104/parity_report.tsv`
- `TestEvidence/bl065_slice_i3_payload_materialization_20260228_133104/lane_notes.md`

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
