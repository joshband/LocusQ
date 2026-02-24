Title: ADR-0010 Repository Artifact Tracking and Retention Policy
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-24

# ADR-0010: Repository Artifact Tracking and Retention Policy

## Status
Accepted

## Context

LocusQ has accumulated generated build outputs and high-volume test artifacts in Git history and active index state, which increases repository noise, review friction, and clone/checkout cost.

Recent cleanup removed tracked build directories and heavy test media from the index while preserving local working copies. That cleanup needs a durable policy so future contributions follow one deterministic rule-set.

## Decision

Adopt a class-first artifact governance model:

1. Determine handling by artifact class first (source, canonical docs, generated outputs, evidence, media), not by extension alone.
2. Track only artifacts required for reproducibility, closeout claims, and traceability.
3. Keep generated/heavy artifacts local-only by default.
4. Require explicit exception documentation before tracking unusual or large generated assets.
5. This ADR refines `ADR-0001` by defining artifact tracking/retention mechanics; it does not replace metadata, traceability, or doc-baseline governance.

## Scope Boundary
`ADR-0001` remains the baseline for documentation metadata, canonical placement, and traceability obligations.
This ADR is limited to artifact lifecycle policy: classification, retention, promotion exceptions, and archive hygiene.

## Artifact Handling Matrix

| Artifact Class | Representative Paths / Types | Repo Policy | Retention Rule |
|---|---|---|---|
| Source and configuration | `Source/**`, `qa/**`, `scripts/**`, `CMakeLists.txt`, `.json`, `.md` (human-authored) | Tracked | Long-lived |
| Canonical governance/status docs | `README.md`, `CHANGELOG.md`, `status.json`, `Documentation/backlog-post-v1-agentic-sprints.md`, `Documentation/adr/**`, `Documentation/invariants.md`, `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md` | Tracked | Long-lived |
| Active plans/specs | `Documentation/plans/**`, `Documentation/testing/**` | Tracked | Current cycle; archive when superseded |
| Generated build outputs | `build/**`, `build_*/**`, `build_bl*/**`, `build_no_clap_check/**`, `cmake-build-*/**`, `out/**`, `dist/**` | Local-only, ignored | Ephemeral |
| Generated scratch docs/exports | `Documentation/exports/**` | Local-only, ignored | Ephemeral; archive only when explicitly preserved |
| Raw test media and bulky binaries | `TestEvidence/**/*.wav`, `TestEvidence/**/*.gz`, other large generated media dumps | Local-only, ignored | Ephemeral; keep local or external storage |
| Run-scoped generated evidence bundles | Timestamped run outputs under `TestEvidence/**` (for example logs, PNG captures, temporary status dumps) | Local-only by default | Keep only while actively debugging/validating |
| Promoted evidence summaries | Stable summary rows/files referenced by Tier 0 docs and closeout lanes | Tracked | Retain latest decision-grade evidence; archive stale bundles |
| Archived historical bundles | `Documentation/archive/**` | Tracked | Immutable historical record |

## File-Type Guidance

1. Extension checks are secondary to class policy.
2. If class is unknown, treat artifact as local-only until explicitly classified.
3. New generated artifact types must be added to `.gitignore` and/or this ADR in the same change set that introduces them.

## Exception Process

A local-only artifact may be promoted to tracked only when all of the following are true:

1. It is required to substantiate a backlog/ADR closeout claim.
2. No smaller summary form provides equivalent decision evidence.
3. The owning task references the artifact from a canonical Tier 0/1 surface.
4. The promotion rationale is documented in:
   - `Documentation/backlog-post-v1-agentic-sprints.md` (task note), and
   - `TestEvidence/build-summary.md` or `TestEvidence/validation-trend.md` (evidence row).

## Consequences

### Positive

1. Lower repository churn and smaller review surface.
2. Clear, repeatable handling for generated artifacts.
3. Stronger separation between canonical evidence and debug exhaust.

### Costs

1. Contributors must classify new artifacts before committing.
2. Some deep raw evidence remains local unless explicitly promoted.

## Implementation Notes (2026-02-23)

Policy-aligned repository changes completed:

1. Build directories tracked in error were untracked from Git index and kept local.
2. Heavy `TestEvidence` media (`.wav`, `.gz`) were untracked from Git index and kept local.
3. `.gitignore` was updated to prevent reintroduction of these generated artifacts.

## Related

- `Documentation/standards.md`
- `Documentation/README.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/backlog-post-v1-agentic-sprints.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`
