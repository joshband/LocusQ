Title: BL-062 Ambisonics IR Interface Contract
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-18

# BL-062 Ambisonics IR Interface Contract

## Status
Done.

## Plain-Language Summary
BL-062 defined the canonical ambisonics intermediate-representation interface contract used by downstream lanes. The result is a stable IR schema, ownership model, channel-order policy, and deterministic validation surface for BL-063, BL-064, and BL-065.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Canonical ambisonics IR interface contract. |
| Why | Gives downstream ambisonics work one explicit interface instead of divergent local assumptions. |
| Who | Ambisonics roadmap work, renderer compatibility, ADM/IAMF mapping, and QA. |
| When | Done. |
| Where | [`Documentation/backlog/done/bl-062-ambisonics-ir-interface-contract.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-062-ambisonics-ir-interface-contract.md) and `TestEvidence/...`. |
| How | Schema contract, ownership boundaries, channel-order rules, and deterministic contract artifacts. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Interface detail | Full schema and ownership tables | Archived legacy copy |

## Core Outcome
- IR schema and field semantics became explicit.
- Channel-order policy and ownership boundaries were defined.
- Downstream contracts now share one canonical interface base.

## Key Gates
- IR schema and ownership model were defined.
- Deterministic contract artifacts were established.
- BL-063, BL-064, and BL-065 could depend on this interface directly.

## Evidence Pointers
| Signal | Path |
|---|---|
| Evidence family | `TestEvidence/bl062_*` |
| Downstream dependencies | `Documentation/backlog/done/bl-063-ambisonics-renderer-compatibility-guardrails.md`, `Documentation/backlog/done/bl-064-adm-mapping-contract.md`, `Documentation/backlog/done/bl-065-iamf-mapping-contract.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| IR schema | Done | Canonical frame contract defined. |
| Ownership model | Done | Mutation and lifetime boundaries defined. |
| Downstream dependency | Done | Contract became the base for later ambisonics lanes. |

## Archive Note
Full historical material is preserved at [`bl-062-ambisonics-ir-interface-contract-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-062-ambisonics-ir-interface-contract-legacy.md).
