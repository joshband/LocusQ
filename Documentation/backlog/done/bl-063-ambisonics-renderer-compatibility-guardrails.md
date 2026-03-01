Title: BL-063 Ambisonics Renderer Compatibility Guardrails
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-063 Ambisonics Renderer Compatibility Guardrails

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-063 |
| Priority | P2 |
| Status | Done |
| Track | E - R&D Expansion |
| Effort | High / L |
| Depends On | BL-062 |
| Blocks | BL-066 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Define deterministic guardrails and parity checks that ensure ambisonics IR integration preserves renderer compatibility across supported output layouts before pilot intake.

## Scope

In scope:
- Compatibility profile definitions by layout family.
- Guardrail thresholds and blocker taxonomy.
- Deterministic replay evidence model for compatibility validation.

Out of scope:
- Decoder algorithm redesign.
- Production rollout/promotion decision authority.

## Architecture Definition

### Core Components

| Component | Responsibility | Output |
|---|---|---|
| `LayoutCompatibilityProfiles` | Declares required behaviors per output layout | profile table in `compatibility_matrix.tsv` |
| `GuardrailEvaluator` | Evaluates tolerance windows and hard-stop conditions | pass/fail guardrail rows |
| `RegressionClassifier` | Categorizes compatibility regressions by severity | `blocker_taxonomy.tsv` entries |
| `ReplaySignatureContract` | Guarantees deterministic replay identity fields | stable signature columns |

### Processing Chain

`IR Contract Fixtures -> Layout Adapter Replay -> Guardrail Evaluator -> Regression Classifier -> Compatibility Evidence`

### Guard Parameter Mapping

| Parameter | Component | Function | Allowed Values |
|---|---|---|---|
| `profile_id` | `LayoutCompatibilityProfiles` | Selects compatibility envelope | stable profile token |
| `layout` | `LayoutCompatibilityProfiles` | Identifies output layout | stereo/quad/5.1/7.1/7.4.2 |
| `energy_delta_db` | `GuardrailEvaluator` | Energy drift tolerance | bounded decimal threshold |
| `phase_error_deg` | `GuardrailEvaluator` | Phase parity tolerance | bounded angular threshold |
| `cpu_budget_pct` | `GuardrailEvaluator` | Runtime headroom ceiling | bounded percentage threshold |
| `severity` | `RegressionClassifier` | Blocker routing | info/warn/fail |

## Complexity Assessment

- Score: `3/5`
- Rationale: Contract-heavy planning with deterministic replay requirements and multi-layout coverage, but limited algorithm invention.

## Implementation Plan

### Strategy

Phased implementation (score >=3).

### Phase 2.1.1: Compatibility Contract Baseline
- [ ] Define required profiles and layout coverage matrix.
- [ ] Establish hard-fail guardrails and warning thresholds.
- [ ] Bind guardrail identifiers to deterministic row keys.

### Phase 2.1.2: Regression Governance
- [ ] Formalize blocker taxonomy and escalation rules.
- [ ] Define replay signature expectations for parity evidence.
- [ ] Document allowed deviations and rationale format.

### Phase 2.1.3: Intake Handoff
- [ ] Publish compatibility acceptance checklist for BL-066.
- [ ] Confirm downstream intake dependencies are complete.
- [ ] Mark unresolved assumptions requiring owner call.

## Dependencies and Entry/Exit Gates

Entry gates:
- BL-062 IR contract accepted with stable schema fields.

Exit gates:
- Compatibility matrix and blocker taxonomy are contract-complete.
- BL-066 intake checklist references all mandatory guardrails.

## Risk Assessment

High risk:
- Missing layout edge cases in compatibility profiles.

Medium risk:
- Threshold definitions too loose or too strict for pilot intake.
- Non-deterministic replay signatures causing false diffs.

Low risk:
- Documentation/evidence path drift.


## Validation Plan

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `validation_matrix.tsv`
- `compatibility_matrix.tsv`
- `blocker_taxonomy.tsv`
- `docs_freshness.log`

## BL-062..BL-066 Planning Packet (latest)

- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl063/status.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl063/validation_matrix.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl063/compatibility_matrix.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl063/blocker_taxonomy.tsv`
- `TestEvidence/bl062_bl066_plan_packets_20260228_130835/bl063/risk_register.tsv`

## BL-063 I1 Implementation Kickoff

- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h` now emits deterministic compatibility guardrail fields for downstream contract lanes:
- `rendererCompatGuardStatus`
- `rendererCompatGuardBlocker`
- `rendererCompatGuardReason`
- `rendererCompatGuardrails` object

Evidence:
- `TestEvidence/bl063_slice_i1_compat_guardrails_20260228_131918/status.tsv`
- `TestEvidence/bl063_slice_i1_compat_guardrails_20260228_131918/validation_matrix.tsv`
- `TestEvidence/bl063_slice_i1_compat_guardrails_20260228_131918/compatibility_matrix.tsv`
- `TestEvidence/bl063_slice_i1_compat_guardrails_20260228_131918/blocker_taxonomy.tsv`
- `TestEvidence/bl063_slice_i1_compat_guardrails_20260228_131918/lane_notes.md`

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
