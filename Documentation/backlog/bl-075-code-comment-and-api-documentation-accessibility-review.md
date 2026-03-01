Title: BL-075 Code Comment and API Documentation Accessibility Review
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-075 Code Comment and API Documentation Accessibility Review

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-075 |
| Priority | P2 |
| Status | In Implementation (kickoff: docs/comment accessibility harness scaffold authored) |
| Track | G - Release/Governance |
| Effort | Med / M |
| Depends On | — |
| Blocks | — |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T0 (docs/governance lane; escalate to T1 when code touch verification is needed) |
| Heavy Lane Budget | Standard |

## Objective

Run a structured review of in-code comments and LocusQ API documentation so non-obvious runtime logic, threading/RT decisions, and integration contracts are discoverable and understandable for contributors (human and AI).

## Acceptance IDs

- Non-obvious logic paths have concise rationale comments (why, not restating what).
- Public/shared API contracts used by plugin/UI/companion boundaries are documented and current.
- Stale/misleading comments are removed or corrected in touched files.
- Contributor-facing docs identify authoritative API entry points and contract boundaries.
- Comment/documentation updates preserve RT-safety and architecture invariants language.

## Scope

In scope:
- `Source/` headers and implementation files where decisions are non-obvious.
- Shared contracts and bridge layers (`Source/shared_contracts/`, `Source/processor_bridge/`).
- Contributor-facing API references and generated API-doc workflow guidance.

Out of scope:
- Functional feature changes unrelated to documentation/comment clarity.
- Broad prose rewrites outside API/comment/accessibility context.

## Validation Plan

QA harness script: `scripts/qa-bl075-doc-accessibility-and-api-contract-review.sh`.
Evidence schema: `TestEvidence/bl075_*/status.tsv`.

Minimum evidence additions:
- `comment_review_matrix.tsv`
- `api_doc_coverage_map.md`
- `stale_comment_remediation.tsv`
- `contributor_entrypoints.md`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T0/T1 | 1/3 (as needed) | docs lint + targeted build/contract checks if code comments touched near critical paths | matrix + notes |
| Candidate intake | T2 | 5 (or owner-approved equivalent) | runbook candidate replay command set | coverage/remediation artifacts |
| Promotion | T3 | 10 (or owner-approved equivalent) | owner-selected promotion replay command set | owner packet + deterministic evidence |
| Sentinel | T4 | explicit only | long-run sentinel only when explicitly requested | sentinel artifacts |

### Cost/Flake Policy

- Keep this lane docs-first; only invoke heavy runtime replays when comment changes are adjacent to sensitive RT or bridge code.
- Document any replay tier escalation rationale in `lane_notes.md` or `owner_decisions.md`.

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

## Governance Alignment (2026-03-01)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).

## Execution Notes (2026-03-01)

- Initial QA scaffold authored:
  - `scripts/qa-bl075-doc-accessibility-and-api-contract-review.sh` with `--contract-only` and `--execute` modes.
- Scaffold output now captures:
  - comment review matrix for critical runtime/bridge surfaces;
  - API doc coverage map anchored to `Documentation/Doxyfile` and standards authority;
  - stale-comment remediation TODO matrix;
  - contributor entrypoint map for human and AI onboarding.
- Remaining BL-075 scope:
  - replace TODO review rows with executed remediation outcomes per file/surface;
  - land comment/API doc updates in touched high-value files and re-run execute-mode checks.
