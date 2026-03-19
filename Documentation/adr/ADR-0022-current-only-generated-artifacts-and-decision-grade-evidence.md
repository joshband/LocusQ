Title: ADR-0022 Current-Only Generated Artifacts and Decision-Grade Evidence
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# ADR-0022: Current-Only Generated Artifacts and Decision-Grade Evidence

## Status

Accepted

## Context

`ADR-0010` established class-first artifact retention, but active folders still accumulated too many timestamped outputs, repeated debug snippets, stale generated summaries, and long evidence narratives.

That made active folders noisy and made it harder for both humans and agents to identify the current truth.

## Decision

Adopt two stricter rules for tracked artifacts.

### 1. Current-only in active folders

Generated support artifacts stay active only while they are the current version referenced by current docs.

This applies especially to:
- `Documentation/reports/data/**`
- `Documentation/reports/visuals/**`
- timestamped report support packets
- active visual or prototype bundles

Older generated outputs move to `Documentation/archive/<YYYY-MM-DD>-<slug>/`.

### 2. Decision-grade tracked evidence

Tracked artifacts in `TestEvidence/` must be decision-grade by default.

Tracked evidence should support one of these jobs:
- closeout summary,
- validation trend,
- promotion packet,
- stable contract schema or replay summary,
- active owner-facing decision packet.

Debug exhaust, repeated payload-failure snippets, and bulky raw run output are not active documentation surfaces.
Keep them local-only when possible.
If they must be preserved, treat them as archive/debug artifacts rather than active authority.

### 3. Short markdown plus compact structured companion

When tracked evidence must remain active:
- keep markdown short,
- keep one compact machine-readable companion,
- avoid repeating the same truth across prose, tables, and raw output.

Preferred active bundle shape:
- `status.tsv`
- `report.md`
- one primary machine-readable result file
- optional logs only when they materially improve triage

### 4. Testing surface boundary

`Documentation/testing/` is a support surface.
It may hold:
- reusable testing guides,
- stable harness contracts,
- compact BL-specific QA contracts.

It must not become a replay log pile or a second status system.
One-off testing notes and superseded QA narratives move to archive.

### 5. Archive manifest discipline

Archive bundles for generated or superseded evidence must record:
- what moved,
- why it moved,
- what active surface replaces it,
- whether re-promotion is allowed.

## Consequences

### Positive

- active folders stay shorter and easier to trust,
- machine-readable consumers see current outputs instead of mixed vintages,
- onboarding and review cost drops,
- history is preserved without cluttering active surfaces.

### Costs

- contributors must archive or replace stale outputs faster,
- some debug history no longer stays in active folders.

## Implementation Rules

1. Keep `Documentation/reports/data/backlog-summary.json` and `Documentation/reports/data/backlog-summary.csv` as the only active backlog-summary exports.
2. Keep `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md` compact and summary-first.
3. Keep only current visuals or prototype sets that are directly referenced by active docs.
4. Archive stale timestamped packets once their findings are absorbed into backlog docs, ADRs, or canonical evidence.
5. Update `Documentation/README.md`, `Documentation/standards.md`, and archive manifests in the same change set when artifact-handling rules change.

## Related

- `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`
- `Documentation/adr/ADR-0021-smart-brevity-documentation-contract.md`
- `Documentation/README.md`
- `Documentation/standards.md`
