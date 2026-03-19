Title: BL-041 Doppler v2 and VBAP Geometry Validation
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-18

# BL-041 Doppler v2 and VBAP Geometry Validation

## Status
Done. Owner Z10 accepted D2 done-promotion mode-parity intake. Deterministic 100/100 contract/execute parity is green, strict usage exits are green, and docs freshness is green.

## Plain-Language Summary
BL-041 locked the deterministic contract for Doppler v2 and VBAP geometry validity before runtime implementation. The key result is simple: bounds, continuity, fallback, and replay identity are now explicit and machine-auditable.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Deterministic Doppler and VBAP validation contract. |
| Why | Prevents interpolation drift, geometry ambiguity, and unstable fallback behavior. |
| Who | Runtime, QA, release owners, and future implementation slices. |
| When | Done; D2 promotion intake accepted on 2026-02-27 and owner sync stayed green. |
| Where | [`Documentation/backlog/done/bl-041-doppler-v2-and-vbap-geometry-validation.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-041-doppler-v2-and-vbap-geometry-validation.md) and `TestEvidence/...`. |
| How | Explicit bounds, deterministic tie-breaks, replay-stable traces, and taxonomy-backed evidence. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Contract summary and evidence map | This runbook |
| Flow diagram | Doppler and VBAP validation pipeline | Archived legacy copy |

## Core Contract
- Doppler inputs are finite-only, bounded, and fallback-safe.
- Smoothing and continuity thresholds are explicit and replay-stable.
- VBAP triplet selection uses deterministic tie-breaks and boundary rules.
- Invalid geometry degrades through fixed fallback tokens, not ad-hoc behavior.
- Identical replay identity inputs must produce identical trace outputs.

## Key Gates
- Contract-only and execute-suite parity must match.
- Required evidence files and TSV schemas are present.
- Replay trace identity and taxonomy outputs agree.
- Docs freshness must pass before closeout is accepted.

## Evidence Pointers
| Signal | Path |
|---|---|
| D2 done-promotion packet | `TestEvidence/bl041_slice_d2_done_promotion_20260227T201844Z/` |
| Owner sync acceptance | `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z10_20260227T203004Z/` |
| Done-candidate parity packet | `TestEvidence/bl041_slice_d1_done_candidate_20260227T183602Z/` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| A1 | Done | Docs-only contract and acceptance IDs established. |
| C7 | Done | Long-run mode parity held. |
| D1 | Done-candidate | Confidence packet accepted. |
| D2 | Done | Promotion parity and owner sync completed. |

## Archive Note
Full historical material is preserved at [`bl-041-doppler-v2-and-vbap-geometry-validation-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-041-doppler-v2-and-vbap-geometry-validation-legacy.md).
