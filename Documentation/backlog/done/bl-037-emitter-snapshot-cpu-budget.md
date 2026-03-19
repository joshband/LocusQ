Title: BL-037 Emitter Snapshot CPU Budget
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-18

# BL-037 Emitter Snapshot CPU Budget

## Status
Done. Owner Z10 accepted D2 done-promotion readiness intake. Deterministic 100-run replay is green, strict usage semantics are green, and docs freshness is green.

## Plain-Language Summary
BL-037 set the deterministic CPU-budget contract for emitter snapshot publication. The result is clear: no-demand, under-budget, and degraded paths are bounded, replay-stable, and auditable.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Deterministic CPU-budget and publish-decision contract for emitter snapshots. |
| Why | Prevents unbounded publication cost and nondeterministic skip behavior. |
| Who | QA owners, release owners, and runtime maintainers. |
| When | Done; D2 promotion intake accepted on 2026-02-27 and owner sync stayed green. |
| Where | [`Documentation/backlog/done/bl-037-emitter-snapshot-cpu-budget.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-037-emitter-snapshot-cpu-budget.md) and `TestEvidence/...`. |
| How | Fixed cadence rules, explicit budget thresholds, deterministic guard windows, and replay identity checks. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Budget contract and evidence map | This runbook |
| Flow detail | Full publication and guard logic | Archived legacy copy |

## Core Contract
- Publish decisions use a fixed per-block order.
- Late-join first publish must occur within one block.
- CPU envelope thresholds are explicit for p95, max, and overrun ratio.
- Budget guard entry and exit use fixed windows, not adaptive heuristics.
- Identical replay inputs must produce identical publish-decision traces.

## Key Gates
- Contract-only and execute-suite parity must match.
- Publish-decision tokens and thresholds remain stable across reruns.
- Required evidence files and TSV schemas are present.
- Docs freshness must pass before closeout is accepted.

## Evidence Pointers
| Signal | Path |
|---|---|
| D2 done-promotion packet | `TestEvidence/bl037_slice_d2_done_promotion_20260227T201819Z/` |
| Owner sync acceptance | `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z10_20260227T203004Z/` |
| Done-candidate confidence packet | `TestEvidence/bl037_slice_d1_done_candidate_20260227T183530Z/` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| A1 | Done | Docs-only contract and acceptance IDs established. |
| C7 | Done | Long-run replay parity held. |
| D1 | Done-candidate | Confidence packet accepted. |
| D2 | Done | Promotion readiness and owner sync completed. |

## Archive Note
Full historical material is preserved at [`bl-037-emitter-snapshot-cpu-budget-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-037-emitter-snapshot-cpu-budget-legacy.md).
