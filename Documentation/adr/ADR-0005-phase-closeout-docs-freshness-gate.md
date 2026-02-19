Title: ADR-0005 Phase Closeout Docs Freshness Gate
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# ADR-0005: Phase Closeout Docs Freshness Gate

## Status
Accepted

## Context

LocusQ phase closeout status has drifted multiple times across planning, acceptance, and shipping cycles. The main failure mode was partial closeout updates: `status.json` changed while root docs or validation surfaces lagged behind, leading to conflicting "done" signals.

This creates planning churn, inaccurate command routing, and weak confidence in acceptance claims.

## Decision

Adopt a mandatory closeout bundle for every phase-state change that impacts implementation or validation status:

1. `plugins/LocusQ/status.json`
2. `plugins/LocusQ/README.md`
3. `plugins/LocusQ/CHANGELOG.md`
4. `plugins/LocusQ/TestEvidence/build-summary.md`
5. `plugins/LocusQ/TestEvidence/validation-trend.md`

Closeout is complete only when all five surfaces are updated in the same change set with consistent dates and status language.

Additional gate rules:

1. `Documentation/standards.md` remains the normative policy for metadata/naming/folder placement.
2. `scripts/validate-docs-freshness.sh` is the enforcement mechanism for freshness/metadata checks.
3. Any invariant-impacting behavior change still requires an ADR update before closeout.

## Rationale

- Reduces stale or conflicting phase claims.
- Makes `/status` and root/docs snapshots dependable for planning and implementation.
- Converts "docs hygiene" from best effort to testable contract.

## Consequences

### Positive
- Stronger continuity across `/plan`, `/impl`, `/test`, and `/ship`.
- Faster review and debugging due to one canonical closeout bundle.
- Better trend fidelity for acceptance and regression analysis.

### Costs
- Small per-phase documentation overhead.
- CI failures when closeout docs are incomplete (intentional friction).

## Related

- `Documentation/adr/ADR-0001-documentation-governance.md`
- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/standards.md`
- `Documentation/README.md`
- `Documentation/lessons-learned.md`
