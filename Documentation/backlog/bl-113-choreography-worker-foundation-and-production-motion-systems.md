Title: BL-113 Choreography Worker Foundation and Production Motion Systems
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-20
Last Modified Date: 2026-03-20

# BL-113: Choreography Worker Foundation and Production Motion Systems

## Plain-Language Summary

BL-113 is the runtime foundation lane for the new Choreography Lab program.
It owns the worker integration, production-candidate motion systems, and bake-to-Timeline bridge needed before UI or promotion claims can become real.

Current state: Open.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Runtime maintainers, DSP/worker implementers, and QA owners proving choreography behavior. |
| What is changing? | LocusQ gains a colocated `ChoreographyWorker`, bounded audio-ring input, production-candidate motion systems, and bake export. |
| Why is this important? | The feature cannot stay design-only if it is meant to drive real emitter state and graduate into Timeline. |
| How will we deliver it? | Implement CL-P1, CL-P2, CL-P3, CL-P4, and CL-P7 in sequence with deterministic probes and finite-safety checks. |
| When is it done? | Done means the runtime path exists, bake export works, and the planned production-candidate acceptance gates have evidence owners. |
| Where is the source of truth? | This runbook, [2026-03-20-choreography-lab-execution-packet.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/plans/2026-03-20-choreography-lab-execution-packet.md), and `.ideas/choreography-lab-impl-plan.md`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick runtime-scope scan | `## Status Ledger` |
| Slice table | Separates infrastructure from production systems | `## Implementation Slices` |
| Validation plan | Keeps runtime proof obligations explicit | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Open |
| Owner Track | E - R&D Expansion |
| Depends On | BL-112 |
| Blocks | BL-114, BL-115, BL-116 |
| Annex Spec | `Documentation/plans/2026-03-20-choreography-lab-execution-packet.md` |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Automation Contract

Draft-only by default.

| Field | Value |
|---|---|
| Automation Mode | `draft_only` unless owner-approved otherwise |
| Stage Cap | `T1` / `T2` / `T3` |
| Owner Approval Required For | `Done`, archive move, status/index transition |
| Runner Output | `DRAFT_READY`, `BLOCKED`, `MANUAL_ONLY` |

## Progress Snapshot

| Item | Status | Updated | Where | Remaining |
|---|---|---|---|---|
| Authority and sequencing packet | `[DONE]` | 2026-03-20 | BL-112 + execution packet | none |
| Worker/runtime foundation | `[IN PROGRESS]` | 2026-03-20 | `Source/ChoreographyWorker.h/.cpp`, `Source/AudioRingBuffer.h`, `Source/PhysicsWorker.h` — CL-P1 infrastructure: worker + ring buffer + tick integration + `choro_enable` APVTS param + traceability | CL-P2..P4 subsystems remaining |
| Production-candidate systems | `[QUEUED]` | 2026-03-20 | formation/path/beat-sync/bake runtime files | blocked on runtime foundation |
| Deterministic proof | `[QUEUED]` | 2026-03-20 | targeted probes and tests | owned later with BL-116 |

## Objective

Implement the runtime core for Choreography Lab without violating ADR-0020.
That means one colocated choreography compute path, no second `EmitterSlot` writer, and a clean bake bridge into Timeline.

## Scope

### In scope

- CL-P1 infrastructure: `ChoreographyWorker`, `AudioRingBuffer`, `ChoreographyOffset`, and tick integration
- CL-P2 formation patterns
- CL-P3 procedural paths
- CL-P4 beat-sync choreography
- CL-P7 bake to Timeline

### Feature ownership in this lane

- Formation Patterns runtime ownership lives here.
- Procedural Paths runtime ownership lives here.
- Beat-Sync Choreography runtime ownership lives here.
- Graduation Mechanism - Bake to Timeline runtime ownership lives here.
- This lane does not own lab-only audio-reactive or coordination modes; that work lives in BL-115.

### Out of scope

- WebView controls and overlays
- Lab-only audio-reactive choreography
- Lab-only coordination systems
- Promotion evidence finalization

## Architecture Context

- Invariants: `Documentation/invariants.md` - finite-safe runtime behavior, bounded payloads, no read-only visualization mutation
- ADRs: `Documentation/adr/ADR-0020-four-layer-authority-chain-and-choreography-worker-arbitration.md`
- Architecture: `.ideas/choreography-lab-spec.md`, `.ideas/choreography-lab-impl-plan.md`, `.ideas/timeline-spec.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Worker foundation and single-writer composition | `Source/ChoreographyWorker.h`, `Source/ChoreographyWorker.cpp`, `Source/AudioRingBuffer.h`, `Source/PhysicsWorker.h`, `Source/PhysicsWorker.cpp` | BL-112 accepted | choreography compute runs in the physics tick with zero-offset bypass support |
| B | Formation and path systems | `Source/FormationSystem*`, `Source/PathSystem*`, choreography worker integration | Slice A complete | formation/path outputs are finite and traceable |
| C | Beat-sync and bake export | `Source/BeatSyncSystem*`, `Source/BakeRecorder*`, timeline integration files | Slice B complete | beat-trigger and bake pipeline exist with bounded timing/error contracts |
| D | Runtime-proof hooks | targeted tests, probes, and evidence files | Slice C complete | runtime acceptance gates have executable proof lanes |

## Acceptance Coverage

| Feature | Runtime acceptance focus in this lane |
|---|---|
| Formation Patterns | worker integration, geometry generation, morph behavior, and spread-delta publication |
| Procedural Paths | analytical path evaluation, velocity publication, and finite-safe runtime behavior |
| Beat-Sync Choreography | quantized runtime behavior, glide/teleport execution path, and timing-path integration |
| Graduation Mechanism - Bake to Timeline | capture pipeline, curve-fit/export path, and handoff into editable Timeline assets |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL113-CONFIGURE | Automated | `cmake -S . -B build_local` | exit 0 |
| BL113-BUILD | Automated | `cmake --build build_local --config Release --target LocusQ_Standalone -j 8` | exit 0 |
| BL113-CTEST | Automated | `ctest --test-dir build_local --output-on-failure -R "test_suite|qa_runner_app|physics|timeline|choreography"` | targeted choreography and integration lanes pass once authored |
| BL113-RT-AUDIT | Automated | `./scripts/rt-safety-audit.sh` | no new non-allowlisted RT violations |
| BL113-DOCS | Automated | `./scripts/validate-docs-freshness.sh`, `jq empty status.json` | exit 0 |

## Replay Cadence

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | T1 | 3 | build logs, targeted test logs, RT audit notes |
| Candidate | T2 | 5 | stable replay summary + blocker taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner packet + deterministic runtime proof |

## Risks

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Second writer path sneaks into `EmitterSlot` updates | High | Med | keep slice A anchored to ADR-0020 single-writer composition |
| Beat-sync timing is correct in theory but not host-visible | High | Med | require BL-116 host timing evidence before promotion claims |
| Bake export creates Timeline assets that are not editable | Med | Med | make timeline editability an explicit slice C exit criterion |
| Random or bounded-path systems become non-deterministic under replay | Med | High | keep seed, bounds, and replay checks explicit in tests |

## Evidence Bundle

| Artifact | Path | Notes |
|---|---|---|
| `status.tsv` | `TestEvidence/bl113_<slice>_<timestamp>/status.tsv` | machine-readable packet status |
| `validation_matrix.tsv` | `TestEvidence/bl113_<slice>_<timestamp>/validation_matrix.tsv` | per-command results |
| `summary.md` | `TestEvidence/bl113_<slice>_<timestamp>/summary.md` | runtime lane summary |

## Closeout Checklist

- [ ] Slices complete
- [ ] Validation lanes pass
- [ ] Evidence captured under `TestEvidence/...`
- [ ] `Documentation/backlog/index.md` updated when state changes
- [ ] `status.json` updated when state changes
- [ ] `TestEvidence/build-summary.md` updated when required
- [ ] `TestEvidence/validation-trend.md` updated when required
- [ ] `./scripts/validate-docs-freshness.sh` passes

## Owner Sync Handoff

Use the canonical owner packet under:
- `TestEvidence/bl113_owner_sync_<slice>_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `promotion_decision.md`
