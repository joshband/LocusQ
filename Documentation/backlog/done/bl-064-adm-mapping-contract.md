Title: BL-064 ADM Mapping Contract
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-18

# BL-064 ADM Mapping Contract

## Status
Done.

## Plain-Language Summary
BL-064 defined the deterministic contract for mapping the ambisonics intermediate representation into ADM-targeted metadata and audio representation. The result is a stable transform and parity surface for later intake and pilot work.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | ADM mapping contract from ambisonics IR. |
| Why | Prevents ADM-facing work from starting without explicit transform invariants and fallback rules. |
| Who | Ambisonics roadmap work, pilot intake, and QA/governance surfaces. |
| When | Done. |
| Where | [`Documentation/backlog/done/bl-064-adm-mapping-contract.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-064-adm-mapping-contract.md) and `TestEvidence/...`. |
| How | Field mapping, transform rules, constraint validation, and parity reporting. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Mapping detail | Full rule and component tables | Archived legacy copy |

## Core Outcome
- ADM field mapping rules became explicit.
- Transform invariants and fallback behavior were defined.
- Parity schema became machine-readable.

## Key Gates
- Field map and transform rules were defined.
- Constraint validation and parity schema were completed.
- BL-066 could rely on this contract as a prerequisite.

## Evidence Pointers
| Signal | Path |
|---|---|
| Evidence family | `TestEvidence/bl064_*` |
| Downstream intake | `Documentation/backlog/done/bl-066-ambisonics-adm-pilot-execution-intake.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Mapping contract | Done | ADM field mapping defined. |
| Parity schema | Done | Deterministic parity reporting established. |
| Intake dependency | Done | BL-066 prerequisite closed. |

## Archive Note
Full historical material is preserved at [`bl-064-adm-mapping-contract-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-064-adm-mapping-contract-legacy.md).
