Title: [SHORT TITLE]
Document Type: Backlog Intake
Author: [AUTHOR]
Created Date: [YYYY-MM-DD]
Last Modified Date: [YYYY-MM-DD]

# Intake: [SHORT TITLE]

## Plain-Language Summary

[1-3 short sentences. Explain the change, why it matters, and who cares.]

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | [Users / operators / QA / release owners / agents] |
| What is changing? | [Simple statement] |
| Why is this important? | [Risk, value, or quality reason] |
| How will we approach it? | [High-level implementation + validation plan] |
| When is it complete? | [Plain-language done signal] |
| Where is the source of truth? | [`Documentation/backlog/...` + `TestEvidence/...`] |

## Visual Aid Index

Use visuals only when they improve clarity.

| Visual Aid | Why it helps | Planned location |
|---|---|---|
| Status table | Fast scan for humans and agents | This intake doc |
| Diagram (optional) | Clarifies flow or ownership | Adjacent to the relevant section |
| Screenshot/chart (optional) | Clarifies UI or evidence | `TestEvidence/...` |

## Intake Snapshot

| Field | Value |
|---|---|
| Source | [User request / audit / regression / research] |
| Discovered | [YYYY-MM-DD] |
| Reporter | [Name or agent] |
| Proposed Priority | [P0/P1/P2/P3] |
| Proposed Track | [Track A-G or new track] |
| Likely Depends On | [BL-XXX or none known] |
| Likely Blocks | [BL-YYY or none known] |

## Problem

[2-4 short sentences. State the problem or opportunity. Keep it concrete.]

## Proposed Approach

- [Approach point 1]
- [Approach point 2]
- [Approach point 3]

## Replay / Cost Plan

| Field | Value |
|---|---|
| Default Replay Tier | [T0/T1/T2/T3/T4] |
| Heavy Wrapper | [yes/no] |
| Binary Launches Per Wrapper Run | [integer or `n/a`] |
| Dev Loop Budget | [1/3 with brief reason] |
| Candidate Budget | [5 or approved alternative] |
| Promotion Budget | [10 or approved alternative] |

## Automation Contract

Draft-only by default.

| Field | Value |
|---|---|
| Automation Mode | `draft_only` unless owner-approved otherwise |
| Stage Cap | `T1` / `T2` / `T3` |
| Owner Approval Required For | promotion, archive move, status/index transition |
| Runner Output | `DRAFT_READY`, `BLOCKED`, `MANUAL_ONLY` |

## Ownership / Evidence Boundaries

- Owned files or patterns: [path/glob]
- Do-not-edit files or patterns: [path/glob]
- Planned evidence root: `TestEvidence/[item]_[slice]_<timestamp>/`

## Next Step

- [ ] Assign BL/HX ID
- [ ] Promote to full runbook
- [ ] Add row to `Documentation/backlog/index.md`
- [ ] Confirm replay tier and ownership boundaries
- [ ] Archive this intake doc after promotion
