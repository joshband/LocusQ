Title: Backlog Summary Schema Contract
Document Type: Data Contract
Author: APC Codex
Created Date: 2026-03-03
Last Modified Date: 2026-03-18

# Backlog Summary Schema Contract

## Purpose

Define the stable machine-readable shape for backlog summary exports.

## Canonical Artifacts

- `Documentation/reports/data/backlog-summary.json`
- `Documentation/reports/data/backlog-summary.csv`
- generator: `./scripts/export-backlog-summaries.py`

## Refresh Rules

1. Backlog changes under `Documentation/backlog/**` should refresh the summaries.
2. Manual refresh:
   - `./scripts/export-backlog-summaries.py`
3. Freshness check:
   - `./scripts/export-backlog-summaries.py --check`

## JSON Top-Level Contract

Schema version: `locusq-backlog-summary-v1`

| Field | Type | Rule |
|---|---|---|
| `schema_version` | string | must equal `locusq-backlog-summary-v1` |
| `source` | string | must equal `Documentation/backlog` |
| `counts` | object | includes `total`, `open`, `done` |
| `items` | array | ordered backlog item objects |

`counts` fields:
- `total`
- `open`
- `done`

## JSON Item Contract

| Field | Type | Rule |
|---|---|---|
| `id` | string | backlog ID, for example `BL-038` |
| `title` | string | first `#` heading |
| `state_bucket` | string | `open` or `done` |
| `status` | string | value from `## Status Ledger` |
| `priority` | string | value from `## Status Ledger` |
| `track` | string | value from `## Status Ledger` |
| `depends_on` | string | value from `## Status Ledger` |
| `blocks` | string | value from `## Status Ledger` |
| `who` | string | `6W` who answer |
| `what` | string | `6W` what answer |
| `why` | string | `6W` why answer |
| `how` | string | `6W` how answer |
| `when` | string | `6W` when answer |
| `where` | string | `6W` where answer |
| `plain_language_summary` | string | first paragraph from `## Plain-Language Summary` |
| `objective` | string | first paragraph from `## Objective` |
| `runbook_path` | string | repo-relative runbook path |

## CSV Contract

Header order:

`id,title,state_bucket,status,priority,track,depends_on,blocks,who,what,why,how,when,where,plain_language_summary,objective,runbook_path`

## Compatibility Rules

1. Breaking changes require a new `schema_version`.
2. Additive fields are allowed if current fields stay stable.
3. Consumers should gate on `schema_version`.

## Consumer Guidance

1. Prefer `id` as the primary key.
2. Use `runbook_path` as fallback if `id` is temporarily missing.
3. Treat empty strings as `not specified yet`, not parser failure.
