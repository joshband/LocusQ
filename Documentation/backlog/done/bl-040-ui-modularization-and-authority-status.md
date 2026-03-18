Title: BL-040 UI Modularization and Authority Status
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-18

# BL-040 UI Modularization and Authority Status

## Status
Done. Owner-verified D2 promotion evidence is complete, strict usage exits are green, docs freshness is green, and the runbook is archived in full.

## Plain-Language Summary
BL-040 defines the modular UI and authority-status contract. Operators need to know who owns state, when state is stale, and why controls are locked or falling back.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Deterministic modular-UI and authority-status contract. |
| Why | Prevents ambiguous control provenance and stale-state confusion. |
| Who | UI diagnostics, authority mapping, and fallback/status logic. |
| When | Done; D2 promotion was accepted on 2026-02-27 and owner-verified done promotion completed on 2026-03-17. |
| Where | [`Documentation/backlog/done/bl-040-ui-modularization-and-authority-status.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-040-ui-modularization-and-authority-status.md) and `TestEvidence/...`. |
| How | Authority precedence rules, replay-stable status rows, and sentinel evidence packets. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Snapshot contract and evidence map | This runbook |
| Authority map | Status precedence and fallback states | Archived legacy copy |

## Core Contract
- Authority provenance must be explicit.
- Status rows and chip mappings must be replay-stable.
- Stale, lock, and fallback states must be distinct.
- `status_hash` and `row_signature` must match baseline replay.
- Required evidence artifacts stay machine-readable and promotion-ready.

## Key Gates
- Contract-only and execute-suite parity must hold at D2 depth.
- The promotion packet must include the required evidence bundle.
- Docs freshness must pass before done promotion is accepted.

## Evidence Pointers
| Signal | Path |
|---|---|
| Done promotion packet | `TestEvidence/bl040_done_promotion_20260317T180000Z/` |
| D2 done-promotion sentinel packet | `TestEvidence/bl040_slice_d2_done_promotion_20260227T201804Z/` |
| Owner sync acceptance | `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z10_20260227T203004Z/` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| A1 | Done | Authority precedence and status classes defined. |
| B1 | Done | UI diagnostics instrumentation added. |
| D1 | Done-candidate | Long-run sentinel passed. |
| D2 | Done | Promotion sentinel and owner-verified closeout completed. |

## Archive Note
Full historical material is preserved at [`bl-040-ui-modularization-and-authority-status-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-040-ui-modularization-and-authority-status-legacy.md).
