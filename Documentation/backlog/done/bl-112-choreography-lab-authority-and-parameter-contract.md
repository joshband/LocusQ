Title: BL-112 Choreography Lab Authority and Parameter Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-20
Last Modified Date: 2026-03-20

# BL-112: Choreography Lab Authority and Parameter Contract

## Plain-Language Summary

BL-112 turns the choreography idea pair into canonical execution authority.
It freezes the namespace, acceptance gates, and backlog ownership for the new Choreography Lab program before runtime work begins.

Current state: **Done** (2026-03-20).

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Maintainers, implementation agents, QA owners, and operators who need one truthful source for the new choreography program. |
| What is changing? | The new choreography idea docs become a canonical backlog-backed authority contract instead of standalone concept notes. |
| Why is this important? | Runtime work is risky if the `choro_*` namespace, authority rules, and promotion gates are still ambiguous. |
| How will we deliver it? | Freeze the backlog bundle, map all planned phases to owned lanes, and define the parameter plus acceptance contract that later phases must obey. |
| When is it done? | Done means the namespace, dependencies, and acceptance IDs are explicit enough that runtime and UI work can proceed without guesswork. |
| Where is the source of truth? | This runbook, [2026-03-20-choreography-lab-execution-packet.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/plans/2026-03-20-choreography-lab-execution-packet.md), and the two choreography idea docs. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state scan | `## Status Ledger` |
| Progress snapshot | Shows what is frozen now and what remains | `## Progress Snapshot` |
| Slice table | Separates authority work from downstream implementation | `## Implementation Slices` |

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | **Done** (2026-03-20) |
| Owner Track | E - R&D Expansion |
| Depends On | BL-094 (Done) |
| Blocks | BL-113, BL-114, BL-115, BL-116 |
| Annex Spec | `Documentation/plans/2026-03-20-choreography-lab-execution-packet.md` |
| Default Replay Tier | T0 |
| Heavy Lane Budget | None |

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
| Choreography concept/spec pair exists | `[DONE]` | 2026-03-20 | `.ideas/choreography-lab-spec.md`, `.ideas/choreography-lab-impl-plan.md` | none |
| Canonical execution packet | `[DONE]` | 2026-03-20 | `Documentation/plans/2026-03-20-choreography-lab-execution-packet.md` | none |
| Namespace + acceptance freeze | `[DONE]` | 2026-03-20 | this runbook: `choro_*` and `bake_*` params must enter `.ideas/parameter-spec.md` and `Documentation/implementation-traceability.md` before any implementation lane advances | none |
| Promotion boundary freeze | `[DONE]` | 2026-03-20 | BL-113/BL-114 = production-candidate; BL-115 = lab-only unless later promoted via BL-116 evidence | none |
| Runtime/UI/testing lanes | `[NEXT]` | 2026-03-20 | BL-113..BL-116 | unblocked now that this contract is accepted |

## Objective

Freeze the authority contract for the new Choreography Lab program.
That includes the `choro_*` and `bake_*` namespace posture, the ADR-0020 placement rule, the BL-094 containment boundary, and the split between production-candidate and lab-only phases.

## Scope

### In scope

- Canonicalize the choreography program as BL-112..BL-116
- Map each planned phase to a backlog owner
- Freeze acceptance IDs and validation ownership for later lanes
- Define the requirement that new parameters be added to `.ideas/parameter-spec.md` and `Documentation/implementation-traceability.md` before implementation claims advance

### Out of scope

- Runtime code changes
- WebView control implementation
- Host validation execution
- Promotion or archive transitions

## Architecture Context

- Invariants: `Documentation/invariants.md` - finite payloads, truthful operator surfaces, and read-only visualization behavior
- ADRs: `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md`
- Architecture: `.ideas/choreography-lab-spec.md`, `.ideas/choreography-lab-impl-plan.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Freeze backlog structure and execution packet | `Documentation/plans/2026-03-20-choreography-lab-execution-packet.md`, `Documentation/backlog/index.md`, this runbook | idea docs exist | backlog authority is explicit |
| B | Freeze namespace and traceability expectations | this runbook + references to `.ideas/parameter-spec.md` and `Documentation/implementation-traceability.md` | Slice A complete | later lanes have a hard parameter gate |
| C | Freeze promotion boundary between production-candidate and lab-only work | this runbook + BL-113..BL-116 | Slice B complete | rollout order is unambiguous |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL112-DOCS | Automated | `./scripts/export-backlog-summaries.py --check`, `./scripts/validate-backlog-plain-language.sh`, `./scripts/validate-backlog-redundancy.py`, `./scripts/validate-docs-freshness.sh`, `jq empty status.json` | exit 0 |
| BL112-REFERENCE | Manual | confirm BL-113..BL-116 all cite this contract and the execution packet | citations present and non-conflicting |

## Replay Cadence

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | T0 | 1 | docs validation logs |
| Candidate | T0/T1 | 1 | backlog/check outputs |
| Promotion | T1 | owner-approved equivalent | owner packet if promoted |

## Risks

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Namespace drift starts before spec updates land | High | High | keep this lane as the hard gate for `choro_*` and `bake_*` planning |
| Older choreography docs are mistaken for current authority | Med | Med | explicitly label BL-022 historical and keep this new bundle canonical |
| Runtime work starts without a promotion boundary | Med | High | split production-candidate and lab-only work into separate follow-on lanes |

## Evidence Bundle

| Artifact | Path | Notes |
|---|---|---|
| `summary.md` | `Documentation/plans/2026-03-20-choreography-lab-execution-packet.md` | canonical planning summary |
| `status.tsv` | `TestEvidence/bl112_<slice>_<timestamp>/status.tsv` | owner packet if later promoted |
| `validation_matrix.tsv` | `TestEvidence/bl112_<slice>_<timestamp>/validation_matrix.tsv` | docs validation proof if packeted later |

## Closeout Checklist

- [x] Authority bundle is accepted
- [x] BL-113..BL-116 remain aligned with this contract
- [x] `Documentation/backlog/index.md` updated when state changes
- [x] `status.json` updated when state changes
- [x] `./scripts/validate-docs-freshness.sh` passes

## Owner Sync Handoff

Use the canonical owner packet under:
- `TestEvidence/bl112_owner_sync_<slice>_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `promotion_decision.md`
