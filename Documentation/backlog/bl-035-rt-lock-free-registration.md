Title: BL-035 RT Lock-Free Registration
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-01

# BL-035 RT Lock-Free Registration

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-035 |
| Priority | P0 |
| Status | In Validation (Owner D8 recheck on current branch: build/smoke/selftest/RT/docs all pass; `non_allowlisted=0`) |
| Track | F - Hardening |
| Effort | High / L |
| Depends On | HX-02 (Done), BL-032 (Done-candidate) |
| Blocks | BL-030 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Resume Checkpoint (2026-03-01)

- Current posture: **In Validation** (D7 blockers cleared by D8 owner replay).
- Latest owner-ready evidence: `TestEvidence/bl035_slice_d8_owner_ready_20260228T203301Z/status.tsv` (`overall=PASS`, `non_allowlisted=0`).
- Last blocker state (historical): D7 failed on selftest startup abort + RT allowlist drift; both remediated in D8.
- Next work to resume:
  - Run candidate/promotion cadence replay for BL-035 (per Global Replay Cadence Policy) and capture owner packet artifacts.
  - If cadence gates stay green, advance BL-035 to done-candidate through owner promotion decision packet.

## Objective

Remove lock acquisition from all audio-thread registration paths so `processBlock()` remains lock-free and invariant-compliant under multi-instance stress.

## Scope and Non-Scope

In scope:
- Lock-free SceneGraph emitter/renderer registration and deregistration paths.
- Processor mode-switch registration flow that is RT-safe and deterministic.
- Stress validation for mode toggling with multiple instances.
- Deterministic failure taxonomy and evidence packet contract for owner intake.

Out of scope:
- UI redesign.
- New DSP features unrelated to registration safety.
- Changes to release-governance policy outside BL-035-linked blocker handling.

## Architecture Context

Registration flow currently traverses processor mode transitions and SceneGraph slot ownership paths. BL-035 focuses on eliminating lock-bearing or ambiguous thread-context behavior at those boundaries while preserving existing behavior contracts.

Current hotspot surfaces (for implementation slices):
- `Source/PluginProcessor.cpp` mode registration transitions (`registerEmitter`, `registerRenderer`, and unregister counterparts).
- `Source/SceneGraph.h` emitter/renderer slot claim and release implementation.
- Any registration-path calls that can be reached from `processBlock()` or block-adjacent mode transitions.

## Deterministic Contract (Authoritative for BL-035)

1. Audio-thread registration path must be lock-free:
   - no `SpinLock`, `CriticalSection`, `std::mutex`, waiting primitives, or blocking calls.
2. Registration ownership transitions are one-shot and deterministic:
   - no duplicate claim of same logical owner in one transition window.
   - no stale ownership after unregister/teardown.
3. Mode-switch handoff is bounded:
   - transition from previous registration state to next registration state completes within one control cycle and never stalls audio processing.
4. Deregistration is safe under multi-instance replay:
   - no use-after-release behavior in renderer/emitter references.
5. Contract must be evidence-backed by replayable stress runs and RT audit output.

## Acceptance IDs

| ID | Requirement | Pass Signal | Evidence |
|---|---|---|---|
| `BL035-A-001` | No lock-bearing primitives in registration path reachable from audio thread | static audit + RT audit green | `registration_audit.tsv`, `rt_audit.tsv` |
| `BL035-A-002` | Lock-free slot claim/release path is deterministic under contention | repeated replay produces stable claim/release outcomes | `claim_release_replay.tsv` |
| `BL035-B-001` | Processor mode-switch registration handoff is deterministic | no transition ambiguity across stress matrix | `mode_switch_matrix.tsv` |
| `BL035-B-002` | No stale renderer/emitter ownership after unregister | replay shows zero stale-owner detections | `failure_taxonomy.tsv` |
| `BL035-C-001` | Multi-instance rapid toggle replay is stable | all required runs PASS with stable status shape | `stress_results.tsv` |
| `BL035-C-002` | RT safety remains green after changes | `non_allowlisted=0` | `rt_audit.tsv` |

## Failure Taxonomy Contract

| Code | Class | Definition |
|---|---|---|
| `BL035-RT-001` | lock_detected_audio_thread | Lock-bearing primitive observed in audio-thread registration path |
| `BL035-RT-002` | registration_transition_ambiguity | Multiple incompatible registration states observed in one transition window |
| `BL035-RT-003` | stale_owner_after_unreg | Released emitter/renderer ownership still observable post-unregister |
| `BL035-RT-004` | contention_claim_failure | Slot claim/release replay diverges under contention |
| `BL035-RT-900` | harness_or_environment_blocker | Validation could not complete due external harness/environment condition |

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Registration path baseline audit + contract map | `BL035-A-001`/`A-002` evidence captured and reviewed |
| B | SceneGraph lock-free claim/release implementation | Lock-free ownership transitions verified under replay |
| C | Processor mode-switch handshake hardening | Deterministic transition matrix passes with zero stale-owner taxonomy hits |
| D | Stress lane + owner intake packet | Multi-instance replay stable and RT gate green (`non_allowlisted=0`) |

## TODOs

- [x] Slice A: Produce `registration_audit.tsv` mapping every registration/deregistration call path and thread context.
- [x] Slice A: Produce `claim_release_replay.tsv` baseline from current branch behavior.
- [x] Slice B: Replace lock-bearing registration claim/release with atomic CAS ownership semantics.
- [x] Slice B: Prove no lock acquisition on audio-thread registration path (`BL035-A-001`).
- [x] Slice C: Harden processor mode-switch handshake to deterministic registration transitions.
- [x] Slice C: Emit taxonomy rows for stale ownership / transition ambiguity.
- [x] Slice D: Run multi-instance stress replay and publish owner-ready evidence packet.
- [x] Slice D: Re-run RT audit and keep `non_allowlisted=0`.
- [x] Update BL-035 status from `In Planning` only after evidence-backed slice completion.
- [x] Owner reconcile current branch RT/docs blockers observed in Slice B/C worker rerun (`20260226T235335Z`/`20260226T235348Z`) via D6 fresh owner replay.

## Owner D7 Recheck (2026-02-28)

Task: `BL-035 RT Lock-Free Registration D7 Owner-Readiness Recheck`

- Validation package: `TestEvidence/bl035_slice_d7_owner_ready_20260228_115509/`
- Validation result: `FAIL`
- Hard blockers recorded:
  - `BL035-D7-BLK-001` — selftest app exits before result capture (`app_exited_before_result`, abort trap 6).
  - `BL035-D7-BLK-002` — RT audit non-allowlisted hits (`non_allowlisted=3`).
- Required command recheck:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` ✅
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` ✅
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` ❌
  - `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl035_slice_d7_owner_ready_20260228_115509/rt_audit.tsv` ❌
  - `./scripts/validate-docs-freshness.sh` ✅
- Decision: remain **In Implementation** until both selftest and RT audit blockers are cleared.

## Owner D8 Recheck (2026-02-28)

Task: `BL-035 RT Lock-Free Registration D8 Owner-Readiness Recheck`

- Validation package: `TestEvidence/bl035_slice_d8_owner_ready_20260228T203301Z/`
- Validation result: `PASS`
- Remediation applied before replay:
  - Fixed retry fallback routing in `scripts/standalone-ui-selftest-production-p0-mac.sh` so direct-launch `ABRT` on `app_exited_before_result` correctly retries with `open`.
  - Reconciled RT allowlist line-map drift in `scripts/rt-safety-allowlist.txt` for `Source/SpatialRenderer.h` dynamic container mutation entries.
- Required command recheck:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` ✅
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` ✅
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` ✅ (`status=pass`)
  - `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl035_slice_d8_owner_ready_20260228T203301Z/rt_audit.tsv` ✅ (`non_allowlisted=0`)
  - `./scripts/validate-docs-freshness.sh` ✅
- Decision: D7 blockers are cleared; BL-035 remains **In Validation** pending next promotion packet cadence step.

## Skill Routing (Per Slice)

- Slice A: `$skill_plan -> $skill_troubleshooting -> $skill_docs`
- Slice B: `$skill_impl -> $skill_troubleshooting`
- Slice C: `$skill_impl -> $skill_test -> $skill_docs`
- Slice D: `$skill_test -> $skill_testing -> $skill_docs`

## Agent Mega-Prompts

### Slice A Prompt (Contract Baseline)

```
TASK: BL-035 Slice A — Registration Path Contract Baseline
ROLE: Worker
SKILLS: $skill_plan -> $skill_troubleshooting -> $skill_docs

GOAL:
- Map all registration/deregistration call paths and thread context boundaries.
- Produce deterministic baseline evidence for BL035-A-001/A-002.

VALIDATION:
- ./scripts/validate-docs-freshness.sh

EVIDENCE:
- status.tsv
- registration_audit.tsv
- claim_release_replay.tsv
- contract_notes.md
```

### Slice B Prompt (Lock-Free Claim/Release)

```
TASK: BL-035 Slice B — Lock-Free Claim/Release Implementation
ROLE: Worker
SKILLS: $skill_impl -> $skill_troubleshooting

GOAL:
- Implement lock-free emitter/renderer claim and release flow.
- Preserve existing behavior while removing lock-bearing registration primitives from RT-reachable paths.

VALIDATION:
- cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
- ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
- ./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl035_slice_b_<timestamp>/rt_audit.tsv

EVIDENCE:
- status.tsv
- build.log
- qa_smoke.log
- rt_audit.tsv
- contract_diff.md
```

### Slice C Prompt (Mode-Switch Determinism)

```
TASK: BL-035 Slice C — Mode-Switch Registration Determinism
ROLE: Worker
SKILLS: $skill_impl -> $skill_test -> $skill_docs

GOAL:
- Ensure mode-switch registration transitions are deterministic and stale-owner-safe.

VALIDATION:
- cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
- ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
- LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh

EVIDENCE:
- status.tsv
- mode_switch_matrix.tsv
- failure_taxonomy.tsv
- selftest.log
```

### Slice D Prompt (Stress + Owner Intake)

```
TASK: BL-035 Slice D — Stress and Owner Intake Packet
ROLE: Worker
SKILLS: $skill_test -> $skill_testing -> $skill_docs

GOAL:
- Produce deterministic multi-instance stress evidence and final RT gate packet for owner sync.

VALIDATION:
- cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
- ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
- LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh
- ./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl035_slice_d_<timestamp>/rt_audit.tsv
- ./scripts/validate-docs-freshness.sh

EVIDENCE:
- status.tsv
- validation_matrix.tsv
- stress_results.tsv
- failure_taxonomy.tsv
- rt_audit.tsv
- docs_freshness.log
```


## Validation Plan

- `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
- `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
- `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh`
- `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl035_<slice>_<timestamp>/rt_audit.tsv`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `validation_matrix.tsv`
- `build.log`
- `qa_smoke.log`
- `selftest.log`
- `stress_results.tsv`
- `registration_audit.tsv`
- `claim_release_replay.tsv`
- `mode_switch_matrix.tsv`
- `failure_taxonomy.tsv`
- `rt_audit.tsv`
- `contract_diff.md`
- `docs_freshness.log`

## Promotion Rule

BL-035 may move out of `In Planning` only when at least one implementation slice (B or C) has completed with:
- deterministic replay evidence present,
- RT audit at `non_allowlisted=0`,
- and no unresolved `BL035-RT-*` blocker taxonomy entries.

## Slice A Baseline Execution Snapshot (2026-02-26)

- Evidence packet:
  - `TestEvidence/bl035_slice_a_20260226T222747Z/status.tsv`
  - `registration_audit.tsv`
  - `lock_points.tsv`
  - `claim_release_replay.tsv`
  - `validation_matrix.tsv`
  - `contract_notes.md`
- Validation:
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Baseline classification:
  - `BL035-A-001` => `FAIL` (registration path remains RT-reachable with lock-bearing primitives)
  - `BL035-A-002` => `FAIL` (slot claim/release remains spin-lock serialized; lock-free contention contract not yet met)
- Owner-facing implication:
  - Slice A baseline is complete and supports immediate start of Slice B/C implementation hardening.

## Slice B Implementation Execution Snapshot (2026-02-26)

- Evidence packet:
  - `TestEvidence/bl035_slice_b_20260226T224548Z/status.tsv`
  - `build.log`
  - `qa_smoke.log`
  - `rt_audit.tsv`
  - `validation_matrix.tsv`
  - `contract_diff.md`
- Validation:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Slice B classification:
  - `BL035-A-001` => `PASS`
  - `BL035-A-002` => `PASS`
- Notes:
  - SceneGraph registration lock removed from claim/release paths.
  - Emitter label restore on registration path migrated to atomic snapshot read.

## Slice C Mode-Switch Determinism Execution Snapshot (2026-02-26)

- Evidence packet:
  - `TestEvidence/bl035_slice_c_20260226T225616Z/status.tsv`
  - `validation_matrix.tsv`
  - `mode_switch_matrix.tsv`
  - `failure_taxonomy.tsv`
  - `build.log`
  - `qa_smoke.log`
  - `selftest.log`
  - `docs_freshness.log`
- Validation:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Slice C classification:
  - `BL035-B-001` => `PASS`
  - `BL035-B-002` => `PASS`
- Notes:
  - Processor mode-switch sync now publishes additive registration diagnostics:
    `registrationTransitionSeq`, `registrationRequestedMode`, `registrationStage`,
    `registrationFallbackReason`, `registrationEmitterSlot`, `registrationEmitterActive`,
    `registrationRendererOwned`, `registrationAmbiguityCount`, `registrationStaleOwnerCount`,
    and nested `registrationTransition{...}`.
  - Mode-sync handshake now guards release completeness, stale-owner recovery, and deterministic
    claim-conflict taxonomy (`stale_emitter_owner`, `renderer_already_claimed`, `release_incomplete`).

## Slice D Stress + Owner Intake Execution Snapshot (2026-02-26)

- Evidence packet:
  - `TestEvidence/bl035_slice_d_20260226T231143Z/status.tsv`
  - `validation_matrix.tsv`
  - `stress_results.tsv`
  - `failure_taxonomy.tsv`
  - `build.log`
  - `qa_smoke.log`
  - `selftest.log`
  - `rt_audit.tsv`
  - `rt_audit.log`
  - `docs_freshness.log`
- Validation:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` (x5) => `PASS`
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `FAIL` (`non_allowlisted=87`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Slice D classification:
  - `BL035-C-001` => `PASS`
  - `BL035-C-002` => `FAIL`
- Interim blocker (resolved by D2):
  - Slice D RT gate was red on branch line map due allowlist drift (`non_allowlisted=87`).

## Slice D2 RT Gate Reconciliation Snapshot (2026-02-26)

- Evidence packet:
  - `TestEvidence/bl035_rt_gate_d2_20260226T233641Z/status.tsv`
  - `rt_before.tsv`
  - `rt_after.tsv`
  - `allowlist_delta.md`
  - `blocker_resolution.md`
  - `docs_freshness.log`
- Validation:
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_before.tsv` => `PASS` (command exit `0`, pre-state non-allowlisted findings present)
  - allowlist reconciliation => `PASS` (explicit line-map drift entries added; no wildcard suppression)
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_after.tsv` => `PASS` (`non_allowlisted=0`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- D2 classification:
  - `BL035-C-002` => `PASS` (RT gate green for this branch snapshot)

## Owner Sync D3 Snapshot (2026-02-26)

- Evidence packet:
  - `TestEvidence/owner_sync_bl035_d3_20260226T234145Z/status.tsv`
  - `validation_matrix.tsv`
  - `owner_decisions.md`
  - `handoff_resolution.md`
  - `rt_audit.tsv`
  - `docs_freshness.log`
  - `status_json.log`
- Validation:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
  - `jq empty status.json` => `PASS`
- Owner disposition:
  - RT contradiction A vs D is resolved by D2 plus fresh D3 RT-green replay evidence (`non_allowlisted=0`).
  - Acceptance status remains explicit: Slice A baseline recorded `BL035-A-001`/`BL035-A-002` as `FAIL`, and Slice B closed both to `PASS`.
  - BL-035 advances to `In Validation` (not `Done-candidate`).

## Slice B/C Worker Rerun Snapshot (2026-02-26)

- Evidence packets:
  - `TestEvidence/bl035_slice_b_20260226T235335Z/status.tsv`
  - `TestEvidence/bl035_slice_b_20260226T235335Z/cas_contention_matrix.tsv`
  - `TestEvidence/bl035_slice_b_20260226T235335Z/contract_delta.md`
  - `TestEvidence/bl035_slice_c_20260226T235348Z/status.tsv`
  - `TestEvidence/bl035_slice_c_20260226T235348Z/registration_path_audit.tsv`
  - `TestEvidence/bl035_slice_c_20260226T235348Z/contract_delta.md`
- Validation:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS` (Slice B/C packet builds)
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS` (Slice B/C packet smoke)
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS` (Slice C packet)
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `FAIL` (`non_allowlisted=74`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (external stale evidence metadata blocker)
- Worker rerun classification:
  - `BL035-A-002` (Slice B gate verdict) => `FAIL` at packet level due shared RT/docs gates.
  - `BL035-A-001` (Slice C gate verdict) => `FAIL` at packet level due shared RT/docs gates.
  - Registration-path lock scan in Slice C packet => `PASS` (`registration_lock_fail_count=0`).
- Blockers:
  - `BL035-RT-900` `rt_gate_allowlist_drift`: RT audit reports `non_allowlisted=74`.
  - `BL035-RT-900` `docs_freshness_external_metadata`: `TestEvidence/bl035_slice_c_20260226T235242Z/contract_delta.md` missing required metadata fields.

## Owner Sync D6 Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/owner_sync_bl035_d6_20260227T001734Z/status.tsv`
  - `validation_matrix.tsv`
  - `owner_decisions.md`
  - `handoff_resolution.md`
  - `rt_audit.tsv`
  - `docs_freshness.log`
  - `status_json.log`
- Inputs reviewed:
  - `TestEvidence/bl035_metadata_hygiene_d4a_20260227T001706Z/status.tsv` (`PASS`)
  - `TestEvidence/bl035_slice_d5_integrated_20260227T002051Z/status.tsv` (`result=FAIL` at slice time due docs freshness metadata mismatch; RT and `BL035-A-001`/`BL035-A-002` remained green)
  - `TestEvidence/bl035_rt_gate_d5b_20260227T001744Z/rt_before.tsv` (pre-reconcile RT snapshot)
  - `TestEvidence/owner_sync_bl035_d3_20260226T234145Z/status.tsv`
- Validation:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (`./TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/harness_contract.md` missing metadata field `Document Type`)
  - `jq empty status.json` => `PASS`
- Owner disposition:
  - Decision is `In Implementation` by owner decision rule because docs gate is not green.
  - `BL035-A-001` and `BL035-A-002` stay explicitly closed by Slice B evidence lineage.
  - Fresh owner replay is authoritative for current branch state and is blocked by out-of-slice docs freshness metadata debt.

## Slice D5 Integrated Snapshot (2026-02-27, post-D6 branch state)

- Evidence packet:
  - `TestEvidence/bl035_slice_d5_integrated_20260227T002051Z/status.tsv`
  - `validation_matrix.tsv`
  - `build.log`
  - `qa_smoke.log`
  - `selftest.log`
  - `rt_audit.tsv`
  - `registration_path_audit.tsv`
  - `cas_contention_matrix.tsv`
  - `contract_delta.md`
  - `docs_freshness.log`
- Validation:
  - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  - `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  - `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (repo-level docs sync blockers outside D5 ownership)
- D5 integrated classification:
  - `BL035-A-001` => `PASS` (`registration_lock_fail_count=0`, `rt_non_allowlisted=0`)
  - `BL035-A-002` => `PASS` (`rt_non_allowlisted=0`, deterministic CAS matrix emitted)
  - D5 packet verdict => `FAIL` (docs freshness gate only)
- External blockers observed in `docs_freshness.log`:
  - `README.md` Last Modified Date mismatch vs `status.json` date (`2026-02-27`)
  - `CHANGELOG.md` Last Modified Date mismatch vs `status.json` date (`2026-02-27`)
  - `TestEvidence/build-summary.md` Last Modified Date mismatch vs `status.json` date (`2026-02-27`)
  - `TestEvidence/validation-trend.md` Last Modified Date mismatch vs `status.json` date (`2026-02-27`)
  - `TestEvidence/validation-trend.md` missing trend row for `2026-02-27`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | runbook primary lane command at dev-loop depth | validation matrix + replay summary |
| Candidate intake | T2 | 5 (or heavy-wrapper 2-run cap) | runbook candidate replay command set | contract/execute artifacts + taxonomy |
| Promotion | T3 | 10 (or owner-approved heavy-wrapper 3-run equivalent) | owner-selected promotion replay command set | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run sentinel drill when explicitly requested | parity/sentinel artifacts |

### Cost/Flake Policy

- Diagnose failing run index before repeating full multi-run sweeps.
- Heavy wrappers (`>=20` binary launches per wrapper run) use targeted reruns, candidate at 2 runs, and promotion at 3 runs unless owner requests broader coverage.
- Document cadence overrides with rationale in `lane_notes.md` or `owner_decisions.md`.


## Handoff Return Contract

All worker and owner handoffs for this runbook must include:
- `SHARED_FILES_TOUCHED: no|yes`

Required return block:
```
HANDOFF_READY
TASK: <BL ID + Title>
RESULT: PASS|FAIL
FILES_TOUCHED: ...
VALIDATION: ...
ARTIFACTS: ...
SHARED_FILES_TOUCHED: no|yes
BLOCKERS: ...
```


## Governance Alignment (2026-02-28)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).
