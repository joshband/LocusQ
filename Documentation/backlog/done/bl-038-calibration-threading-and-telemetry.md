Title: BL-038 Calibration Threading and Telemetry
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-18

# BL-038 Calibration Threading and Telemetry

## Status
Done. Owner Z10 accepted D2 done-promotion parity intake. Contract/execute parity is green, strict usage exits are green, and docs freshness is green.

## Plain-Language Summary
BL-038 defines deterministic calibration threading boundaries and RT-safe telemetry publication. The goal is simple: audio-thread reads stay atomic, worker writes stay complete, and evidence stays replay-stable.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Deterministic calibration threading and telemetry publication rules. |
| Why | Prevents state drift, timeout ambiguity, and weak evidence. |
| Who | `audio_rt`, `calibration_worker`, `message_ui`, optional `io_aux`. |
| When | Done; D2 promotion intake accepted on 2026-02-27 and owner sync stayed green. |
| Where | [`Documentation/backlog/done/bl-038-calibration-threading-and-telemetry.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-038-calibration-threading-and-telemetry.md) and `TestEvidence/...`. |
| How | Atomic snapshot handoff, monotonic sequence IDs, and replay-checked evidence packets. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Snapshot contract and evidence map | This runbook |
| Sequence diagram | Thread handoff and telemetry flow | Archived legacy copy |

## Core Contract
- `audio_rt` reads only atomically published snapshot payloads.
- `calibration_worker` publishes complete generations only.
- `message_ui` sends intent tokens and never mutates worker-owned internals directly.
- `io_aux` is async-only and never feeds back into the real-time path.
- `snapshot_seq` is monotonic and must not regress.
- Telemetry fields are additive, bounded, and snapshot-backed.

## Key Gates
- Contract-only and execute-suite parity must match.
- Required evidence files are present and machine-readable.
- Replay hashes, failure taxonomy, and status summaries agree.
- Docs freshness must pass before closeout is accepted.

## Evidence Pointers
| Signal | Path |
|---|---|
| D2 done-promotion parity packet | `TestEvidence/bl038_slice_d2_done_promotion_20260227T201829Z/` |
| Owner sync acceptance | `TestEvidence/owner_sync_bl036_bl037_bl038_bl039_bl040_bl041_z10_20260227T203004Z/` |
| Done-candidate parity packet | `TestEvidence/bl038_slice_d1_done_candidate_20260227T183540Z/` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| A1 | Done | Docs-only contract and acceptance IDs established. |
| C7 | Done | Long-run parity held across contract and execute lanes. |
| D1 | Done-candidate | Confidence packet accepted. |
| D2 | Done | Promotion parity and owner sync completed. |

## Archive Note
Full historical material is preserved at [`bl-038-calibration-threading-and-telemetry-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-038-calibration-threading-and-telemetry-legacy.md).
