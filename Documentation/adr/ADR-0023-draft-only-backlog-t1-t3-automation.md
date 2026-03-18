Title: ADR-0023 Draft-Only Backlog T1-T3 Automation
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# ADR-0023: Draft-Only Backlog T1-T3 Automation

## Status

Accepted

## Context

LocusQ already has strong replay cadence rules.
It does not yet have a clean automation contract for T1, T2, and T3.

That gap causes two problems:
- repeated manual packet assembly,
- inconsistent closeout prep for `Done-candidate` items.

The repo also has a hard rule: do not auto-advance phases or status blindly.

## Decision

Adopt a draft-only automation model for backlog T1, T2, and T3 flows.

### Core Rule

Automation may:
- run declared validation commands,
- assemble evidence packets,
- draft promotion or closeout notes,
- draft status/index/doc update summaries.

Automation may not, by default:
- change authoritative backlog status,
- move runbooks into `Documentation/backlog/done/`,
- finalize archive transitions,
- claim `Done` without owner confirmation.

### Eligibility Contract

Each automated backlog item must declare or inherit:
- `automation_mode`
- `automation_stage_cap`
- `owner_required_for`
- `heavy_wrapper`
- `shared_files_risk`
- `closeout_ready`
- declared commands or reusable command class

Allowed modes:
- `manual_only`
- `draft_only`
- `owner_gated_auto`

Repository default is `draft_only`.

### Stage Semantics

`T1`
- allowed for repeatable dev-loop checks,
- may emit draft evidence and draft notes.

`T2`
- allowed for candidate-intake checks,
- may emit blocker taxonomy and owner-ready packet inputs.

`T3`
- allowed for promotion replay and packet assembly,
- may emit `promotion_decision.md` and draft update summaries,
- must stop at `DRAFT_READY` unless the item is explicitly configured for owner-gated automation.

### Packet Shape

Draft automation packets must stay compact.

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `lane_notes.md`
- `draft_update_summary.md`

For `T2` and `T3`:
- `owner_decisions.md`
- `handoff_resolution.md`

For `T3`:
- `promotion_decision.md`

Optional files are allowed only when they improve triage.

### Pilot Scope

The first rollout targets `Done-candidate` items first.

Initial pilot set:
- `BL-080`
- `BL-089`
- `BL-090`
- `BL-091`
- `BL-092`
- `BL-093`
- `BL-094`

## Consequences

### Positive

- less repeated packet-writing work,
- consistent evidence bundle shape,
- easier owner review,
- lower closeout friction for near-finished items.

### Costs

- backlog items need a small automation contract,
- owner review still remains part of the path,
- mixed manual and automated items will coexist for a while.

## Implementation Rules

1. Keep automation eligibility machine-readable.
2. Keep automation packets under `TestEvidence/`.
3. Keep active packet prose short and decision-focused.
4. Keep authoritative status transitions owner-confirmed unless an item is explicitly marked `owner_gated_auto`.
5. Keep root routing docs synchronized when automation posture changes.

## Related

- `Documentation/backlog/index.md`
- `Documentation/backlog/automation-draft-flow.md`
- `Documentation/backlog/automation-contracts.json`
- `Documentation/adr/ADR-0021-smart-brevity-documentation-contract.md`
- `Documentation/adr/ADR-0022-current-only-generated-artifacts-and-decision-grade-evidence.md`
