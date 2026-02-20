Title: LocusQ Documentation Index
Document Type: Documentation Index
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-20

# Documentation Index

## Purpose
Keep documentation lean, canonical, and current while preserving traceability from specs to code to validation evidence.

## Canonical Documents
- `README.md`: Root implementation status snapshot and next-phase pointer.
- `CHANGELOG.md`: Lean chronological change record for implementation and validation milestones.
- `Documentation/standards.md`: Documentation structure, metadata, naming, and update rules.
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`: Mandatory phase-closeout synchronization gate for status/evidence/readme/changelog surfaces.
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`: Device-profile contract for quad studio + laptop stereo + headphone workflows and release gating.
- `Documentation/invariants.md`: Non-negotiable behavioral and implementation invariants.
- `Documentation/scene-state-contract.md`: Source-of-truth contract across audio/physics/UI state domains.
- `Documentation/implementation-traceability.md`: Parameter/control wiring to source files.
- `Documentation/v3-ui-parity-checklist.md`: Live checked/unchecked parity tracker against `Design/v3-ui-spec.md` and `Design/v3-style-guide.md`.
- `Documentation/v3-stage-9-plus-detailed-checklists.md`: Detailed Stage 9-13 execution checklists with task-level acceptance gates and Codex mega-prompts.
- `Documentation/stage14-review-release-checklist.md`: Stage 14 comprehensive architecture/code/design/QA review and release decision checklist.
- `Documentation/lessons-learned.md`: Compact operational lessons and associated corrective actions.
- `Documentation/multi-agent-thread-watchdog.md`: Optional guide for thread contract + heartbeat watchdog flows in parallel Codex sessions (disabled by default).
- `Documentation/adr/`: Architecture Decision Records (ADRs), one decision per file.
- `Documentation/research/quadraphonic-audio-spatialization-next-steps.md`: Research-backed prioritized execution matrix for `skill_plan`, `skill_design`, and `skill_impl`.
- `Documentation/research/qa-harness-upstream-backport-opportunities-2026-02-20.md`: Comparison across LocusQ, echoform, memory-echoes, and monument-reverb with prioritized upstream harness and backport opportunities.
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
