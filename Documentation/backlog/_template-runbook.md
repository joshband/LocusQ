Title: BL-XXX [TITLE]
Document Type: Backlog Runbook
Author: APC Codex
Created Date: [YYYY-MM-DD]
Last Modified Date: [YYYY-MM-DD]

# BL-XXX: [TITLE]

## Plain-Language Summary

[1-3 short sentences. Say what is changing, why it matters, and the current state.]

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | [Users / operators / QA / release owners / agents] |
| What is changing? | [Simple statement] |
| Why is this important? | [Risk or value reason] |
| How will we deliver it? | [High-level implementation + validation plan] |
| When is it done? | [Plain-language gate] |
| Where is the source of truth? | [Runbook path + evidence path] |

## Visual Aid Index

Use visuals only when they improve clarity.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state scan | `## Status Ledger` |
| Progress snapshot | Shows current, next, and blocked work | `## Progress Snapshot` |
| Slice table | Clarifies sequencing and ownership | `## Implementation Slices` |
| Diagram (optional) | Clarifies flow when tables are not enough | Adjacent to the relevant section |

## Status Ledger

| Field | Value |
|---|---|
| Priority | [P0/P1/P2/P3] |
| Status | [Open / In Planning / In Implementation / In Validation / Done-candidate / Done] |
| Owner Track | [Track X - Name] |
| Depends On | [BL-XXX or —] |
| Blocks | [BL-YYY or —] |
| Annex Spec | `Documentation/plans/...` or `n/a` |
| Default Replay Tier | [T0/T1/T2/T3/T4] |
| Heavy Lane Budget | [Standard / High-cost wrapper] |

## Automation Contract

Draft-only by default.

| Field | Value |
|---|---|
| Automation Mode | `draft_only` unless owner-approved otherwise |
| Stage Cap | `T1` / `T2` / `T3` |
| Owner Approval Required For | `Done`, archive move, status/index transition |
| Runner Output | `DRAFT_READY`, `BLOCKED`, `MANUAL_ONLY` |

## Progress Snapshot

| Item | Status | Updated | Where | Remaining |
|---|---|---|---|---|
| [Current slice] | `[ACTIVE]` | [YYYY-MM-DD] | `Source/...` | [what remains] |
| [Completed slice] | `[DONE]` | [YYYY-MM-DD] | `Source/...` | none |
| [Next slice] | `[NEXT]` | [YYYY-MM-DD] | `Source/...` | [why it is next] |

## Objective

[Short description of the intended outcome and success condition.]

## Scope

### In scope

- [scope item]
- [scope item]

### Out of scope

- [out-of-scope item]
- [out-of-scope item]

## Architecture Context

- Invariants: `Documentation/invariants.md` - [relevant area]
- ADRs: [ADR-XXXX or `n/a`]
- Architecture: `.ideas/architecture.md` - [relevant subsystem]

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | [slice summary] | `Source/...` | [gate] | [exit signal] |
| B | [slice summary] | `Source/...` | [gate] | [exit signal] |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| [LANE-1] | Automated | `[command]` | [pass rule] |
| [LANE-2] | Manual | [steps] | [pass rule] |

## Replay Cadence

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | [T0/T1] | [1/3] | `validation_matrix.tsv` and logs |
| Candidate | [T2] | [5 or approved alternative] | replay summary + taxonomy |
| Promotion | [T3] | [10 or approved alternative] | owner packet evidence |

## Risks

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| [risk] | [High/Med/Low] | [High/Med/Low] | [mitigation] |

## Evidence Bundle

| Artifact | Path | Notes |
|---|---|---|
| `status.tsv` | `TestEvidence/...` | machine-readable packet status |
| `validation_matrix.tsv` | `TestEvidence/...` | per-command results |
| `promotion_decision.md` | `TestEvidence/...` | owner packet when applicable |

## Closeout Checklist

- [ ] Slices complete
- [ ] Validation lanes pass
- [ ] Evidence captured under `TestEvidence/...`
- [ ] Runbook summary and 6W stay current
- [ ] `Documentation/backlog/index.md` updated when state changes
- [ ] `status.json` updated when state changes
- [ ] `TestEvidence/build-summary.md` updated when required
- [ ] `TestEvidence/validation-trend.md` updated when required
- [ ] `./scripts/validate-docs-freshness.sh` passes

## Owner Sync Handoff

Use the canonical owner packet under:
- `TestEvidence/<bl_or_hx>_owner_sync_<slice>_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `promotion_decision.md`
- `owner_decisions.md` when coordination risk is not low
- `handoff_resolution.md` when coordination risk is not low
