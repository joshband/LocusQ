Title: BL-065 IAMF Mapping Contract
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-18

# BL-065 IAMF Mapping Contract

## Status
Done.

## Plain-Language Summary
BL-065 defined the deterministic contract for mapping the ambisonics intermediate representation into IAMF scene and profile outputs. The result is a stable mapping/parity surface and blocker taxonomy for later intake and pilot work.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | IAMF mapping contract from ambisonics IR. |
| Why | Prevents IAMF-facing work from starting without explicit transform and parity rules. |
| Who | Ambisonics roadmap work, pilot intake, and QA/governance surfaces. |
| When | Done. |
| Where | [`Documentation/backlog/done/bl-065-iamf-mapping-contract.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-065-iamf-mapping-contract.md) and `TestEvidence/...`. |
| How | Deterministic profile mapping, transform rules, constraint validation, and parity reporting. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Mapping detail | Full rule and component tables | Archived legacy copy |

## Core Outcome
- IAMF mapping rules became explicit.
- Profile constraints and parity reporting were defined.
- Intake-facing blocker taxonomy became machine-readable.

## Key Gates
- Mapping matrix and transform rules were defined.
- Constraint validation and parity schema were completed.
- BL-066 could rely on this contract as a prerequisite.

## Evidence Pointers
| Signal | Path |
|---|---|
| Evidence family | `TestEvidence/bl065_*` |
| Downstream intake | `Documentation/backlog/done/bl-066-ambisonics-adm-pilot-execution-intake.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Mapping contract | Done | IAMF semantic mapping defined. |
| Parity schema | Done | Deterministic parity reporting established. |
| Intake dependency | Done | BL-066 prerequisite closed. |

## Archive Note
Full historical material is preserved at [`bl-065-iamf-mapping-contract-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-065-iamf-mapping-contract-legacy.md).
