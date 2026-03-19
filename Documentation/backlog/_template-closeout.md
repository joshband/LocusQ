Title: BL-XXX [TITLE]
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: [YYYY-MM-DD]
Last Modified Date: [YYYY-MM-DD]

# BL-XXX: [TITLE]

## Plain-Language Summary

[1-3 short sentences. Explain what shipped, who benefits, and why it matters.]

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who benefited? | [Users / operators / QA / release owners] |
| What changed? | [Delivered behavior in plain language] |
| Why did this matter? | [Outcome or risk reduction] |
| How was it delivered safely? | [Implementation + validation summary] |
| When was it complete? | [Date + decision signal] |
| Where is the evidence? | [Done runbook path + `TestEvidence/...`] |

## Visual Aid Index

Use visuals only when they improve clarity.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Completion snapshot | Fast scan of what closed and what did not | `## Completion Snapshot` |
| Gate summary | Quick confidence scan | `## Promotion Gate Summary` |
| Evidence list | Fast traceability | `## Evidence References` |

## Status Ledger

| Field | Value |
|---|---|
| Priority | [P0/P1/P2/P3] |
| Status | Done |
| Completed | [YYYY-MM-DD] |
| Owner Track | [Track X - Name] |
| Promotion Decision Packet | `TestEvidence/<owner_sync_or_promotion_packet>/promotion_decision.md` |
| Final Evidence Root | `TestEvidence/<bl_or_hx>_<slice>_<timestamp>/` |
| Archived Runbook Path | `Documentation/backlog/done/bl-XXX-[slug].md` |

## Completion Snapshot

| Item | Status | Completed | Where | Evidence / Remaining |
|---|---|---|---|---|
| [Closed scope] | `[DONE]` | [YYYY-MM-DD] | `Source/...` | `TestEvidence/...` |
| [Follow-on if any] | `[NEXT]` / `[DEFERRED]` | [YYYY-MM-DD or `n/a`] | `Documentation/backlog/...` | [remaining work] |

## Objective

[Past-tense summary of what was accomplished.]

## What Shipped

- [Change 1]
- [Change 2]
- [Change 3]

## Key Files

- `Source/...`
- `Source/...`
- `Documentation/...`

## Evidence References

- [Primary evidence path]
- [Secondary evidence path]
- [Relevant summary path]

## Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | [PASS/FAIL] | `[path]` |
| Lane replay/parity | [PASS/FAIL] | `[path]` |
| RT safety | [PASS/FAIL] | `[path]` |
| Docs freshness | [PASS/FAIL] | `[path]` |
| Status schema | [PASS/FAIL] | `[path]` |
| Ownership safety | [PASS/FAIL] | `[path]` |

## Closeout Checklist

- [ ] Runbook moved to `Documentation/backlog/done/`
- [ ] `Documentation/backlog/index.md` updated
- [ ] `status.json` updated
- [ ] `TestEvidence/build-summary.md` updated
- [ ] `TestEvidence/validation-trend.md` updated
- [ ] Owner decision and handoff linked
- [ ] `./scripts/validate-docs-freshness.sh` passes
- [ ] `jq empty status.json` passes
