Title: BL-039 Parameter Relay Spec Generation
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-18

# BL-039 Parameter Relay Spec Generation

## Status
Done. Z11 promotion was accepted on 2026-03-17. Deterministic contract/execute evidence remains green, docs freshness is green, and archive/index sync is complete.

## Plain-Language Summary
BL-039 defines one authoritative parameter-relay spec. That spec deterministically drives APVTS IDs, native relay binding, and UI binding contracts so manual drift does not creep back in.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Spec-to-generation contract for parameter relay plumbing. |
| Why | Prevents drift between source spec, generated IDs, and runtime bindings. |
| Who | Spec authors, generator scripts, relay bindings, and UI bindings. |
| When | Done; D2 promotion packet was accepted on 2026-02-27 and the runbook was re-closed on 2026-03-17. |
| Where | [`Documentation/backlog/done/bl-039-parameter-relay-spec-generation.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-039-parameter-relay-spec-generation.md) and `TestEvidence/...`. |
| How | Deterministic generation, replay hashes, drift checks, and promotion evidence. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Snapshot contract and evidence map | This runbook |
| Flow diagram | Spec-to-generation pipeline | Archived legacy copy |

## Core Contract
- The parameter-relay spec is normative.
- Generated artifacts must be deterministic and replay-stable.
- Drift checks compare generated output back to the spec.
- Replay artifacts are required for promotion evidence.
- Failure taxonomy separates schema gaps, ordering drift, and hash divergence.

## Key Gates
- Contract-only and execute-suite parity must match at promotion depth.
- Required evidence files must exist and be machine-readable.
- Docs freshness and archive/index sync must remain green.

## Evidence Pointers
| Signal | Path |
|---|---|
| D2 done-promotion parity packet | `TestEvidence/bl039_slice_d2_done_promotion_20260227T201844Z/` |
| Owner sync acceptance | `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z10_20260227T203004Z/` |
| Done-candidate parity packet | `TestEvidence/bl039_slice_d1_done_candidate_20260227T183730Z/` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| A1 | Done | Normative generation behavior and evidence schema established. |
| C5/C6 | Done | Semantics and long-run parity held. |
| D1 | Done-candidate | Confidence packet accepted. |
| D2 | Done | Promotion parity accepted and archived. |

## Archive Note
Full historical material is preserved at [`bl-039-parameter-relay-spec-generation-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-039-parameter-relay-spec-generation-legacy.md).
