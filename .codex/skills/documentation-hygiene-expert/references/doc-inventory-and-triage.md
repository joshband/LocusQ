Title: Documentation Inventory And Triage Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Doc Inventory And Triage

## Objective
Create a fast, repeatable inventory that separates authoritative docs from duplication and stale clutter.

## Triage Labels
- `canonical`: current source-of-truth for a topic.
- `supporting`: useful but non-authoritative context.
- `generated`: machine/export output; should not be treated as canonical.
- `archive`: historical and intentionally non-authoritative.
- `delete-candidate`: redundant or obsolete content pending owner review.

## Required Columns
- `path`
- `topic`
- `owner`
- `last_reviewed_date`
- `label`
- `action`
- `notes`

## Action Rules
1. If two docs claim authority on the same topic, keep one canonical and convert the other to a pointer or archive candidate.
2. If no owner can be identified, assign temporary owner and mark as `needs-owner`.
3. If behavior claims are not backed by current code/evidence, mark as stale and queue rewrite or removal.
