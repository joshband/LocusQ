Title: ADR-0015 Skill Runtime Doc Standards Boundary
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# ADR-0015: Skill Runtime Doc Standards Boundary

## Status
Accepted

## Context

The repository docs-freshness gate currently enforces metadata headers across nearly all markdown files. This conflicts with how Codex/Claude skill runtime files are authored and maintained (`.codex/skills/**`, `.claude/skills/**`), where portable skill conventions are the governing standard rather than repository-specific metadata fields.

## Decision

Define an explicit standards boundary:

1. Repository documentation standards (`Documentation/standards.md`) apply to repository governance/architecture/backlog/evidence markdown.
2. Skill runtime markdown under `.codex/skills/**` and `.claude/skills/**` follows Codex/Claude skill standards and is exempt from repository metadata-header enforcement.
3. `scripts/validate-docs-freshness.sh` must skip metadata-header validation for these skill-runtime paths.

## Rationale

- Prevents false-positive gate failures on valid skill-runtime files.
- Keeps repo governance strict where it matters (status/backlog/evidence), without forcing local metadata conventions into externalized skill formats.
- Reduces churn during skill creation and refinement.

## Consequences

### Positive
- Docs-freshness gate reflects intended ownership boundaries.
- Skill files can stay concise and portable across Codex/Claude contexts.

### Costs
- Two markdown standards now coexist; contributors must understand the boundary.

## Related

- `Documentation/standards.md`
- `scripts/validate-docs-freshness.sh`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
