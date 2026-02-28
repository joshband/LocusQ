Title: BL-XXX [TITLE]
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: [YYYY-MM-DD]
Last Modified Date: [YYYY-MM-DD]

# BL-XXX: [TITLE]

## Status Ledger

| Field | Value |
|---|---|
| Priority | [P0/P1/P2] |
| Status | Done |
| Completed | [YYYY-MM-DD] |
| Owner Track | [Track X — Name] |
| Promotion Decision Packet | `TestEvidence/<owner_sync_or_promotion_packet>/promotion_decision.md` |
| Final Evidence Root | `TestEvidence/<bl_or_hx>_<slice>_<timestamp>/` |
| Archived Runbook Path | `Documentation/backlog/done/bl-XXX-[slug].md` |

## Objective

[Past tense description of what was accomplished and why it mattered.]

## What Was Built

- [Key change 1]
- [Key change 2]
- [Key change 3]

## Key Files

- `[Source/file1.h]`
- `[Source/file2.cpp]`
- `[Source/ui/public/js/index.js]`

## Evidence References

- [Link or path to validation artifacts]
- [TestEvidence/ entries]
- [Self-test lane results]

## Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | [PASS/FAIL] | `[path]` |
| Lane replay/parity | [PASS/FAIL] | `[path]` |
| RT safety | [PASS/FAIL] | `[path]` |
| Docs freshness | [PASS/FAIL] | `[path]` |
| Status schema | [PASS/FAIL] | `[path]` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | [PASS/FAIL] | `[path]` |

## Backlog/Status Sync Checklist

- [ ] Runbook moved from `Documentation/backlog/` to `Documentation/backlog/done/`
- [ ] `Documentation/backlog/index.md` row updated to Done with done-path link
- [ ] `status.json` updated
- [ ] `TestEvidence/build-summary.md` updated
- [ ] `TestEvidence/validation-trend.md` updated
- [ ] Owner decision + handoff resolution linked
- [ ] `./scripts/validate-docs-freshness.sh` passes
- [ ] `jq empty status.json` passes

## Completion Date

[YYYY-MM-DD]
