Title: ADR-0001 Documentation Governance Baseline
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

# ADR-0001: Documentation Governance Baseline

## Status
Accepted

## Context
Documentation quality drifted across specs, implementation notes, and evidence logs. Required metadata, traceability, and trend logging were not consistently enforced.

## Decision
Adopt a single documentation baseline:
- Metadata header required for human-authored markdown (`Title`, `Document Type`, `Author`, `Created Date`, `Last Modified Date`).
- Canonical organization by folder (`.ideas/`, `Design/`, `Documentation/`, `TestEvidence/`).
- Cross-reference obligations from code changes to specs, invariants, and ADRs.
- Validation snapshot + trend logs required for meaningful build/test runs.

## Consequences
### Positive
- Faster review of freshness and ownership.
- Better traceability from requirements to code and evidence.
- Reduced duplicate docs through canonical placement.

### Costs
- Small overhead per doc update for metadata maintenance.
- Initial retrofit effort across legacy files.

## Related
- `Documentation/standards.md`
- `Documentation/invariants.md`
- `Documentation/implementation-traceability.md`
