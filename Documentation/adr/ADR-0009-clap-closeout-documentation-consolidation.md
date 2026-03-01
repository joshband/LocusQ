Title: ADR-0009 CLAP Closeout Documentation Consolidation
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-01

# ADR-0009: CLAP Closeout Documentation Consolidation

## Status
Accepted

## Context

BL-011 CLAP closeout evidence was spread across multiple surfaces:

1. `Documentation/plans/LocusQClapContract.h` (contract header)
2. `Documentation/plans/CLAP_References.md` (research notes)
3. local CLAP PDF references under `Documentation/plans/`
4. backlog/status/test-evidence surfaces

That layout made BL-011 closeout handoff less clear and mixed active planning with reference artifacts.

## Decision

Adopt a consolidated CLAP documentation model:

1. Keep `Documentation/plans/LocusQClapContract.h` as the normative low-level CLAP adapter/runtime contract.
2. Use one active operator-facing CLAP closeout document:
- `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`
3. Archive CLAP research/reference markdown and PDFs under:
- `Documentation/archive/2026-02-23-clap-reference-bundle/`
4. Keep BL-011 acceptance authority in Tier 0 state/evidence surfaces:
- `status.json`
- `Documentation/backlog/index.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

## Rationale

1. One CLAP closeout doc reduces ambiguity for final BL-011 promotion and regression reruns.
2. Preserving `LocusQClapContract.h` keeps the contract close to implementation and QA harness usage.
3. Archiving references/PDFs maintains traceability without crowding active execution surfaces.

## Consequences

### Positive

1. BL-011 ownership and closeout evidence are easier to audit.
2. CLAP docs now follow the same active-vs-archive governance model used elsewhere in `Documentation/`.
3. Future CLAP regressions can start from one canonical closeout entrypoint.

### Costs

1. Historical links to pre-archive CLAP reference locations must be rewired.
2. Contributors now need to update both the CLAP closeout doc and Tier 0 state surfaces when BL-011 claims change.

## Guardrails

1. Do not duplicate CLAP closeout requirements across multiple active planning docs.
2. If `LocusQClapContract.h` contract semantics change, update the canonical CLAP closeout doc and Tier 0 evidence/state surfaces in the same change set.
3. Keep archived CLAP artifacts immutable unless a new dated archive pass is created.

## Related

- `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`
- `Documentation/plans/LocusQClapContract.h`
- `Documentation/archive/2026-02-23-clap-reference-bundle/README.md`
- `Documentation/backlog/index.md`
- `status.json`
