Title: [SHORT TITLE]
Document Type: Backlog Intake
Author: [AUTHOR]
Created Date: [YYYY-MM-DD]
Last Modified Date: [YYYY-MM-DD]

# Intake: [SHORT TITLE]

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
- [ ] Archive this intake doc
