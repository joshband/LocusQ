Title: LocusQ Documentation Archive Index
Document Type: Archive Index
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# Documentation Archive Index

## Purpose
Provide a stable archive location for non-canonical documentation artifacts while keeping top-level `Documentation/` focused on active source-of-truth docs.

## Archive Policy
1. Archive generated exports, one-off reports, and superseded historical bundles under date-stamped folders.
2. Keep archived files intact for traceability; do not treat archived content as source-of-truth unless explicitly re-promoted.
3. Update `Documentation/README.md` and `Documentation/standards.md` whenever archival scope changes.

## Current Archive Sets
1. `Documentation/archive/2026-02-23-ops-artifacts/`
- Contents:
  - top-level exports moved from prior `Documentation/exports/`
  - consolidated report snapshot at `reports-final/` (authoritative archive for 2026-02-23 report outputs)
- Reason:
  - generated/operational artifacts were crowding active docs surface
  - these artifacts are reference-only and non-authoritative

2. `Documentation/archive/2026-02-23-historical-review-bundles/`
- Contents:
  - full-review and design-review bundles
  - stage14 review/checklist bundle
  - v3 parity and stage 9+ checklists
- Reason:
  - reduce active documentation surface while retaining historical traceability

3. `Documentation/archive/2026-02-23-clap-reference-bundle/`
- Contents:
  - archived CLAP reference markdown and PDF sources previously under `Documentation/plans/`
- Reason:
  - BL-011 CLAP closeout now uses one canonical active plan (`Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`) plus ADR governance, while references remain preserved for traceability

## Next-Pass Candidates (review required before move)
1. Historical planning drafts that are now reference-only:
- `Documentation/plans/2026-02-20-full-project-review.md`
2. Optional runtime/process notes if no longer active:
- `Documentation/multi-agent-thread-watchdog.md`
