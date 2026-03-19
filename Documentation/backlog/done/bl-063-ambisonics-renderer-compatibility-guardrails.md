Title: BL-063 Ambisonics Renderer Compatibility Guardrails
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-18

# BL-063 Ambisonics Renderer Compatibility Guardrails

## Status
Done.

## Plain-Language Summary
BL-063 defined the deterministic compatibility guardrails that ambisonics IR integration must satisfy across supported output layouts before pilot intake. The result is an explicit compatibility and blocker surface instead of a vague “should still work” assumption.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Ambisonics renderer compatibility guardrails. |
| Why | Protects output-layout compatibility before pilot execution starts. |
| Who | Ambisonics roadmap work, pilot intake, renderer validation, and QA. |
| When | Done. |
| Where | [`Documentation/backlog/done/bl-063-ambisonics-renderer-compatibility-guardrails.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-063-ambisonics-renderer-compatibility-guardrails.md) and `TestEvidence/...`. |
| How | Compatibility profiles, guardrail thresholds, replay signatures, and blocker taxonomy. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Guardrail detail | Full compatibility profile definitions | Archived legacy copy |

## Core Outcome
- Layout-family compatibility profiles became explicit.
- Guardrail thresholds and blocker classification were defined.
- Replay evidence model became stable enough for BL-066 intake.

## Key Gates
- Compatibility profiles were defined by layout family.
- Guardrail evaluator and blocker taxonomy were documented.
- Deterministic replay identity became part of the contract.
- BL-066 could use the result as a prerequisite.

## Evidence Pointers
| Signal | Path |
|---|---|
| Evidence family | `TestEvidence/bl063_*` |
| Downstream intake | `Documentation/backlog/done/bl-066-ambisonics-adm-pilot-execution-intake.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Compatibility profiles | Done | Layout guardrails defined. |
| Replay contract | Done | Deterministic compatibility evidence defined. |
| Intake dependency | Done | BL-066 prerequisite closed. |

## Archive Note
Full historical material is preserved at [`bl-063-ambisonics-renderer-compatibility-guardrails-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-063-ambisonics-renderer-compatibility-guardrails-legacy.md).
