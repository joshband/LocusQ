Title: BL-051 Ambisonics and ADM Roadmap
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-02

# BL-051 Ambisonics and ADM Roadmap

## Plain-Language Summary

This runbook tracks **BL-051** (BL-051 Ambisonics and ADM Roadmap). Current status: **Done-candidate**. In plain terms: Define v2 roadmap and ADR decisions for Ambisonics intermediate bus adoption and ADM/IAMF delivery/export readiness.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-051 Ambisonics and ADM Roadmap |
| Why is this important? | Define v2 roadmap and ADR decisions for Ambisonics intermediate bus adoption and ADM/IAMF delivery/export readiness. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/bl-051-ambisonics-and-adm-roadmap.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Implementation slices table | Explains step-by-step delivery order and boundaries. | `## Implementation Slices` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| ID | BL-051 |
| Priority | P3 |
| Status | Done-candidate |
| Track | E - R&D Expansion |
| Effort | Very High / XL |
| Depends On | BL-046, BL-050 |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| Latest Slice | A1b/A2/A3/C1 — Decision + Contract + ADR + Decomposition |
| Latest Evidence | `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/` |
| ADR Authority | `Documentation/adr/ADR-0014-bl051-ambisonics-adm-roadmap-governance.md` |

## Objective

Define v2 roadmap and ADR decisions for Ambisonics intermediate bus adoption and ADM/IAMF delivery/export readiness.

## Scope

In scope:
- ADR-backed decision package for ambisonics intermediate representation.
- Migration phases for decode/output targets.
- ADM/IAMF interoperability roadmap and risk register.

Out of scope:
- Immediate production implementation in v1.x.
- End-user export UI completion.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Architecture decision package (ADR) | ADR approved with migration phases |
| B | Prototype and parity lane contract | Prototype lane reports deterministically |
| C | Backlog decomposition for implementation | Follow-on BL items created and linked |

## TODOs

- [x] Produce ADR for ambisonics intermediate bus decision.
- [x] Define phased migration and rollback criteria.
- [x] Define ADM/IAMF interoperability milestones and dependencies.
- [x] Draft prototype validation lane and evidence schema.
- [x] Decompose approved roadmap into implementation backlog items.


## Validation Plan

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `validation_matrix.tsv`
- `adr_decision.md`
- `migration_plan.tsv`
- `risk_register.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## A1b Decision Package Snapshot (2026-02-28)

- Slice: `A1b`
- Scope:
  - Ambisonics intermediate-bus decision framing.
  - Explicit migration phases with rollback/governance criteria.
  - Risk register for ADM/IAMF interoperability and rollout sequencing.
- Evidence packet:
  - `TestEvidence/bl051_slice_a1b_decision_package_20260228_121851/status.tsv`
  - `TestEvidence/bl051_slice_a1b_decision_package_20260228_121851/validation_matrix.tsv`
  - `TestEvidence/bl051_slice_a1b_decision_package_20260228_121851/adr_decision.md`
  - `TestEvidence/bl051_slice_a1b_decision_package_20260228_121851/migration_plan.tsv`
  - `TestEvidence/bl051_slice_a1b_decision_package_20260228_121851/risk_register.tsv`
  - `TestEvidence/bl051_slice_a1b_decision_package_20260228_121851/lane_notes.md`
  - `TestEvidence/bl051_slice_a1b_decision_package_20260228_121851/docs_freshness.log`

## A1b Owner Intake Pass (2026-02-28)

- Decision: `ACCEPTED` for planning-to-validation transition.
- Owner packet:
  - `TestEvidence/bl051_owner_progression_a1b_a2_a3_20260228_124415/status.tsv`
  - `TestEvidence/bl051_owner_progression_a1b_a2_a3_20260228_124415/validation_matrix.tsv`
  - `TestEvidence/bl051_owner_progression_a1b_a2_a3_20260228_124415/lane_notes.md`
- Shared-surface sync:
  - BL-051 row status in `Documentation/backlog/index.md` updated to reflect owner intake acceptance.

## A2 Prototype/Parity Contract Draft (2026-02-28)

- Contract authority captured in:
  - `Documentation/testing/bl-051-ambisonics-and-adm-roadmap-qa.md`
- A2 defines required artifacts, acceptance criteria, and failure taxonomy for:
  - prototype lane determinism,
  - ambisonics-to-ADM/IAMF parity checks,
  - replayable evidence schema.

## A3 ADR Formalization (2026-02-28)

- Formal ADR created:
  - `Documentation/adr/ADR-0014-bl051-ambisonics-adm-roadmap-governance.md`
- BL-051 now anchors roadmap governance decisions to ADR-0014 before execution-lane intake.

## C1 Backlog Decomposition Snapshot (2026-02-28)

- Decomposition packet:
  - `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/status.tsv`
  - `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/validation_matrix.tsv`
  - `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/decomposition_plan.tsv`
  - `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/dependency_graph.tsv`
  - `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/lane_notes.md`
  - `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/docs_freshness.log`

Follow-on implementation items:

| Work ID | Name | Scope | Depends On | Exit Signal |
|---|---|---|---|---|
| BL051-WI-001 | Ambisonics IR Interface Contract | Canonical bus shape, frame/order semantics, invariant contract, adapter boundaries | ADR-0014, BL-046, BL-050 | Contract matrix pass + deterministic schema artifacts |
| BL051-WI-002 | Renderer Compatibility Guardrails | Parity checks for stereo/quad/5.1/7.1/7.4.2 against ambisonics IR pathway | BL051-WI-001 | Replay matrix pass with zero compatibility blockers |
| BL051-WI-003 | ADM Mapping Contract | Deterministic map from ambisonics IR to ADM metadata/audio objects | BL051-WI-001, BL051-WI-002 | ADM contract lane emits stable PASS matrix |
| BL051-WI-004 | IAMF Mapping Contract | Deterministic map from ambisonics IR to IAMF scene/profile outputs | BL051-WI-001, BL051-WI-002 | IAMF contract lane emits stable PASS matrix |
| BL051-WI-005 | Pilot Execution Intake | Controlled implementation pilot gated by rollback and artifact policy | BL051-WI-003, BL051-WI-004 | Owner intake packet accepted for execution tranche |

## C1 Workstream Realization (A1b -> concrete backlog lanes)

| Workstream ID | Concrete Backlog Lane | Contract Focus | Evidence Anchor |
| --- | --- | --- | --- |
| WI-001 | [BL-062 Ambisonics IR Interface Contract](done/bl-062-ambisonics-ir-interface-contract.md) | IR loader/ownership contract, deterministic handoff boundaries | `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/decomposition_plan.tsv` |
| WI-002 | [BL-063 Ambisonics Renderer Compatibility Guardrails](done/bl-063-ambisonics-renderer-compatibility-guardrails.md) | Renderer-side compatibility and rollout gate criteria | `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/dependency_graph.tsv` |
| WI-003 | [BL-064 ADM Mapping Contract](done/bl-064-adm-mapping-contract.md) | ADM schema mapping, validation invariants, downgrade behavior | `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/decomposition_plan.tsv` |
| WI-004 | [BL-065 IAMF Mapping Contract](done/bl-065-iamf-mapping-contract.md) | IAMF lane contract and migration protections | `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/decomposition_plan.tsv` |
| WI-005 | [BL-066 Ambisonics + ADM Pilot Execution Intake](done/bl-066-ambisonics-adm-pilot-execution-intake.md) | Intake bridge for execution lanes with explicit readiness gates | `TestEvidence/bl051_slice_c1_decomposition_20260228_124747/status.tsv` |


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

## BL-051 Parent Closeout Sync (BL-062..BL-066 Done)

Resolution:
- BL-062 through BL-066 have been promoted to Done in TestEvidence/bl062_bl066_done_promotion_20260228_153040.
- BL-051 parent lane now advances to Done-candidate.
- Dependency policy holds BL-051 below Done until BL-050 is closed.

Evidence:
- TestEvidence/bl051_parent_closeout_sync_20260228_153306/status.tsv
- TestEvidence/bl051_parent_closeout_sync_20260228_153306/validation_matrix.tsv
- TestEvidence/bl051_parent_closeout_sync_20260228_153306/lane_notes.md
- TestEvidence/bl062_bl066_done_promotion_20260228_153040/status.tsv
- TestEvidence/bl062_bl066_done_promotion_20260228_153040/validation_matrix.tsv
