Title: [SHORT TITLE]
Document Type: Backlog Intake
Author: [AUTHOR]
Created Date: [YYYY-MM-DD]
Last Modified Date: [YYYY-MM-DD]

# Intake: [SHORT TITLE]

## Plain-Language Summary

[1-3 sentences in non-technical language explaining what changes for people and why this matters now.]

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | [End users / operators / QA / release owners / coding agents] |
| What is changing? | [Simple statement of the change] |
| Why is this important? | [Risk reduction, user value, quality, or delivery reason] |
| How will we approach it? | [High-level implementation + validation approach] |
| When is it considered complete? | [Done signal in plain language] |
| Where is the source of truth? | [`Documentation/backlog/...` + `TestEvidence/...`] |

## Visual Aid Plan

Use visuals only when they improve understanding.

| Visual Aid | Why it helps | Planned location |
|---|---|---|
| Table | Fast status/dependency scan for humans and agents | This intake doc |
| Mermaid diagram (optional) | Explain sequence/ownership when text is hard to follow | `## Visual Diagram` |
| Screenshot/chart (optional) | Clarify UI or metric behavior | `TestEvidence/...` with linked path |

## Origin

| Field | Value |
|---|---|
| Source | [User request / Research / Regression / Audit finding] |
| Discovered | [YYYY-MM-DD] |
| Reporter | [Name or agent ID] |

## Description

[2-3 sentences describing the idea, problem, or opportunity.]

## Proposed Priority

[P1 / P2 / P3] — [One sentence justification.]

## Dependency Guesses

- Likely depends on: [BL-XXX, BL-YYY, or "none known"]
- Likely blocks: [BL-ZZZ, or "none known"]

## Proposed Track

[Track A-G from master index, or "new track needed"]

## Replay / Cost Plan (Required)

| Field | Value |
|---|---|
| Proposed Default Replay Tier | [T0/T1/T2/T3/T4 per `Documentation/backlog/index.md`] |
| Heavy Wrapper | [yes/no] |
| Estimated Binary Launches Per Wrapper Run | [integer or N/A] |
| Dev Loop Run Budget | [1/3 with rationale] |
| Candidate Gate Run Budget | [5 or owner-approved alternative] |
| Promotion Gate Run Budget | [10 or owner-approved alternative] |

## Ownership / Evidence Boundaries (Required)

- Owned files/patterns:
  - [path/glob]
- Do-not-edit files/patterns:
  - [path/glob]
- Planned evidence root:
  - `TestEvidence/[item]_[slice]_<timestamp>/`

## Next Step

- [ ] Triage: assign BL/HX ID and validate dependencies
- [ ] Promote: convert to full runbook in `Documentation/backlog/`
- [ ] Add row to `Documentation/backlog/index.md`
- [ ] Record replay tier + run budget in promoted runbook
- [ ] Define owner packet template usage and done closeout path
- [ ] Confirm plain-language summary + 6W + visual aid plan are complete and understandable by non-technical readers
- [ ] Archive this intake doc
