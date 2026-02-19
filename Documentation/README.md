Title: LocusQ Documentation Index
Document Type: Documentation Index
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-19

# Documentation Index

## Purpose
Keep documentation lean, canonical, and current while preserving traceability from specs to code to validation evidence.

## Canonical Documents
- `README.md`: Root implementation status snapshot and next-phase pointer.
- `CHANGELOG.md`: Lean chronological change record for implementation and validation milestones.
- `Documentation/standards.md`: Documentation structure, metadata, naming, and update rules.
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`: Mandatory phase-closeout synchronization gate for status/evidence/readme/changelog surfaces.
- `Documentation/invariants.md`: Non-negotiable behavioral and implementation invariants.
- `Documentation/scene-state-contract.md`: Source-of-truth contract across audio/physics/UI state domains.
- `Documentation/implementation-traceability.md`: Parameter/control wiring to source files.
- `Documentation/lessons-learned.md`: Compact operational lessons and associated corrective actions.
- `Documentation/adr/`: Architecture Decision Records (ADRs), one decision per file.
- `Documentation/research/quadraphonic-audio-spatialization-next-steps.md`: Research-backed prioritized execution matrix for `skill_plan`, `skill_design`, and `skill_impl`.
- `Documentation/Doxyfile`: API documentation generation configuration for Doxygen.
- `TestEvidence/build-summary.md`: Latest validation snapshot.
- `TestEvidence/validation-trend.md`: Time-series validation trend entries.

## Normative Inputs For Implementation
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- Relevant `Documentation/adr/*.md`

## Exclusions
- `qa_output/` markdown reports are generated artifacts and are not manually normalized for metadata.

## Compliance Snapshot (2026-02-19)
- All non-generated markdown files are metadata-compliant.
- Generated markdown under `qa_output/` remains intentionally unmanaged.
- Root `README.md` and `CHANGELOG.md` are now present and maintained as canonical, concise status/change surfaces.
- Phase closeout updates are gated by ADR-0005 and validated via `scripts/validate-docs-freshness.sh`.
