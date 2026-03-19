Title: HX-05 Payload Budget and Throttle Contract
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# HX-05 Payload Budget and Throughput Contract

## Status
Done. Owner sync packet was finalized from Slice D promotion evidence.

## Plain-Language Summary
HX-05 defined the scene-state payload budget and bridge cadence limits that keep UI transport deterministic under load. The practical result is explicit byte, cadence, burst, and degrade rules instead of implicit payload growth.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Payload budget and throughput contract. |
| Why | Prevents bridge transport from drifting into oversized or unstable update behavior. |
| Who | UI transport/runtime maintainers, QA, and throughput hardening follow-on work. |
| When | Done; promotion evidence closed the item. |
| Where | [`Documentation/backlog/done/hx-05-payload-budget.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/hx-05-payload-budget.md) and `TestEvidence/...`. |
| How | Explicit payload limits, degrade tiers, cadence caps, and promotion evidence. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Contract detail | Full budget and degradation tables | Archived legacy copy |

## Core Outcome
- Payload bytes, cadence, burst windows, and degrade tiers became explicit.
- Bridge transport now has deterministic fallback behavior under pressure.
- Contract surfaces became machine-readable and suitable for promotion gating.

## Key Gates
- Authoritative budget contract published.
- Degrade policy and cadence caps defined.
- Slice D promotion evidence completed.
- Owner sync packet finalized the closeout.

## Evidence Pointers
| Signal | Path |
|---|---|
| Evidence family | `TestEvidence/hx05_*` |
| Historical closeout detail | archived legacy copy |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Slice A-C | Done | Budget, cadence, and artifact contract established. |
| Slice D | Done | Promotion replay and governance decision completed. |

## Archive Note
Full historical material is preserved at [`hx-05-payload-budget-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/hx-05-payload-budget-legacy.md).
