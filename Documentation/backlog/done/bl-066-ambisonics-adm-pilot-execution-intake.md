Title: BL-066 Ambisonics + ADM Pilot Execution Intake
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-066 Ambisonics + ADM Pilot Execution Intake

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-066 |
| Priority | P1 |
| Status | Done |
| Track | E - R&D Expansion |
| Effort | High / L |
| Depends On | BL-063, BL-064, BL-065 |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Prepare owner-gated pilot execution intake package for BL-051 roadmap implementation once compatibility and mapping contracts are accepted, with explicit rollback and blocker governance.

## Scope

In scope:
- Pilot lane entry checklist and pass/fail gates.
- Rollback policy and hard-stop blocker taxonomy.
- Owner intake evidence packet schema and decision template linkage.

Out of scope:
- Full production rollout and release packaging.
- Runtime feature implementation across all downstream lanes.

## Architecture Definition

### Core Components

| Component | Responsibility | Output |
|---|---|---|
| `IntakeChecklistEngine` | Evaluates prerequisite contract completion | `pilot_intake_contract.tsv` checklist rows |
| `GateDecisionRouter` | Maps failed checks to stop/defer/escalate outcomes | deterministic decision actions |
| `RollbackGovernor` | Defines rollback trigger, scope, and owner acknowledgment fields | `rollback_policy.tsv` |
| `EvidenceBundleAssembler` | Assembles final owner-ready packet structure | stable evidence manifest |

### Processing Chain

`BL-063/064/065 Contract Results -> Intake Checklist -> Gate Decision Router -> Rollback Governor -> Owner Intake Packet`

### Intake Parameter Mapping

| Parameter | Component | Function | Allowed Values |
|---|---|---|---|
| `gate_id` | `IntakeChecklistEngine` | Stable gate identity | immutable token |
| `gate_status` | `IntakeChecklistEngine` | Gate pass/fail state | pass/fail/defer |
| `severity` | `GateDecisionRouter` | Decision priority routing | info/warn/fail |
| `rollback_trigger` | `RollbackGovernor` | Defines rollback condition | deterministic expression |
| `owner_ack_required` | `RollbackGovernor` | Ownership confirmation requirement | yes/no |
| `evidence_path` | `EvidenceBundleAssembler` | Links produced evidence artifact | repo-local path |

## Complexity Assessment

- Score: `3/5`
- Rationale: Primarily governance and decision orchestration work with deterministic artifact and escalation contracts; moderate complexity with multi-lane dependency intake.

## Implementation Plan

### Strategy

Phased implementation (score >=3).

### Phase 2.1.1: Intake Contract Baseline
- [ ] Freeze gate inventory for BL-063/064/065 prerequisites.
- [ ] Define gate semantics (`pass/fail/defer`) and blocking behavior.
- [ ] Define mandatory evidence artifact list and path schema.

### Phase 2.1.2: Rollback and Escalation Governance
- [ ] Define rollback triggers and owner acknowledgment policy.
- [ ] Bind blocker taxonomy severities to gate decisions.
- [ ] Define required owner decision fields for deferred gates.

### Phase 2.1.3: Owner-Ready Packet Contract
- [ ] Publish final intake packet template and acceptance criteria.
- [ ] Link decision packet to BL-051 runbook ledger/evidence references.
- [ ] Mark readiness criteria for transition from planning to implementation intake.

## Dependencies and Entry/Exit Gates

Entry gates:
- BL-063 compatibility guardrails accepted.
- BL-064 ADM mapping contract accepted.
- BL-065 IAMF mapping contract accepted.

Exit gates:
- Pilot intake contract and rollback policy are finalized and owner-readable.
- Decision packet template supports deterministic promotion call.

## Risk Assessment

High risk:
- Incomplete dependency intake leading to unsafe pilot start.

Medium risk:
- Rollback criteria too coarse for partial contract failures.
- Ambiguous owner decision fields causing repeat escalations.

Low risk:
- Documentation linkage and artifact naming drift.


## Validation Plan

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `validation_matrix.tsv`
- `pilot_intake_contract.tsv`
- `rollback_policy.tsv`
- `docs_freshness.log`

## BL-062..BL-066 Planning Packet (latest)

- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl066/status.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl066/validation_matrix.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl066/pilot_intake_contract.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl066/rollback_policy.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl066/risk_register.tsv`

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


## BL-066 I1 Implementation Kickoff

- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h` now emits pilot intake gate telemetry fields:
- `rendererPilotIntakeStatus`
- `rendererPilotIntakeBlocker`
- `rendererPilotIntakeReason`
- `rendererPilotIntakeGate` object

Evidence:
- `TestEvidence/bl066_slice_i1_pilot_intake_gate_scaffold_20260228_132139/status.tsv`
- `TestEvidence/bl066_slice_i1_pilot_intake_gate_scaffold_20260228_132139/validation_matrix.tsv`
- `TestEvidence/bl066_slice_i1_pilot_intake_gate_scaffold_20260228_132139/pilot_intake_contract.tsv`
- `TestEvidence/bl066_slice_i1_pilot_intake_gate_scaffold_20260228_132139/rollback_policy.tsv`
- `TestEvidence/bl066_slice_i1_pilot_intake_gate_scaffold_20260228_132139/lane_notes.md`

## BL-066 I2 Runtime Intake Execution

- Pilot intake gate now includes runtime codec execution dimensions sourced from `Source/SpatialRenderer.h` and consumed in `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`:
- `executionMode`
- `executionFinite`
- `executionFallbackActive`

Evidence:
- `TestEvidence/bl066_slice_i2_runtime_intake_execution_20260228_132744/status.tsv`
- `TestEvidence/bl066_slice_i2_runtime_intake_execution_20260228_132744/validation_matrix.tsv`
- `TestEvidence/bl066_slice_i2_runtime_intake_execution_20260228_132744/pilot_intake_contract.tsv`
- `TestEvidence/bl066_slice_i2_runtime_intake_execution_20260228_132744/rollback_policy.tsv`
- `TestEvidence/bl066_slice_i2_runtime_intake_execution_20260228_132744/lane_notes.md`

## BL-066 I3 Payload Intake Contract

- Pilot intake now composes concrete runtime payload objects from BL-064/065:
- `rendererAdmRuntimePayload`
- `rendererIamfRuntimePayload`
- plus runtime execution gate dimensions (`executionMode`, `executionFinite`, `executionFallbackActive`).

Evidence:
- `TestEvidence/bl066_slice_i3_payload_materialization_20260228_133104/status.tsv`
- `TestEvidence/bl066_slice_i3_payload_materialization_20260228_133104/validation_matrix.tsv`
- `TestEvidence/bl066_slice_i3_payload_materialization_20260228_133104/pilot_intake_contract.tsv`
- `TestEvidence/bl066_slice_i3_payload_materialization_20260228_133104/rollback_policy.tsv`
- `TestEvidence/bl066_slice_i3_payload_materialization_20260228_133104/lane_notes.md`

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
