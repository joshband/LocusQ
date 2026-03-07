Title: BL-076 SpatialRenderer Decomposition and Boundary Guardrails
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-06

# BL-076 SpatialRenderer Decomposition and Boundary Guardrails

## Plain-Language Summary

BL-076 in plain terms: Decompose `Source/SpatialRenderer.h` into cohesive renderer modules with explicit ownership boundaries so the runtime can evolve without a single giant multipurpose header becoming a merge-risk and defect hotspot. Current state: Done (owner T2 `5/5` + T3 `10/10` execute replay PASS; closeout/archive sync complete). For technical detail, see `## Objective`, `## Validation Plan`, and the historical execution snapshots below.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA owners, release owners, and engineering maintainers who depend on deterministic evidence. |
| What changed? | `SpatialRenderer` was decomposed into focused implementation units so the runtime is no longer carried by one giant multipurpose header/body pair. |
| Why is this important? | It reduces merge risk, bounds module size, and keeps related spatial/headphone backlog lanes from depending on a monolithic renderer file. |
| How was it delivered safely? | The work landed in bounded extraction waves, then passed BL-076 structure/dependency/RT/bridge guardrails at T1, T2, and T3 execute cadence. |
| When was it considered complete? | 2026-03-06 local date, after T2 `5/5` and T3 `10/10` execute replay PASS plus closeout/archive sync. |
| Where is the source of truth? | Runbook `Documentation/backlog/done/bl-076-spatial-renderer-decomposition-and-boundary-guardrails.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/bl076_*`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status legend + completion snapshot | Fast scan of closed scope, dates, and follow-on work without color dependencies. | `## Status Legend`, `## Completion Snapshot` |
| Evidence visual snapshot | Quick PASS scan across T1/T2/T3 replay evidence. | `## Evidence Visual Snapshot` |
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Promotion gate table | Shows final closeout confidence and evidence linkage. | `## Promotion Gate Summary` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Status Legend

- `[DONE]` completed slice or milestone. Use `~~strikethrough~~` on the item name when the named slice is fully complete.
- `[ACTIVE]` current implementation focus with meaningful remaining scope.
- `[NEXT]` the next recommended slice after the active one finishes.
- `[QUEUED]` planned but not the current focus.
- `[DEFERRED]` intentionally moved to another lane or later milestone.
- `[BLOCKED]` waiting on a dependency, owner decision, or failing validation lane.
- Portable markdown only: do not use inline HTML/CSS color for status because it is not reliable across renderers.
- `Actual / Time` should use exact dates or focused-session wording when available. If time was not logged, say `not logged`.
- `Tokens` should be `n/a` unless a session explicitly recorded them; token counts are not auto-captured in repo docs today.

## Evidence Visual Snapshot

| Replay Stage | Result | Evidence |
|---|---|---|
| T1 execute replay | PASS | `TestEvidence/bl076_spatial_renderer_20260307T002358Z/status.tsv` |
| T2 candidate replay | PASS (`5/5`) | `TestEvidence/bl076_candidate_t2_closeout/t2_summary.tsv` |
| T3 promotion replay | PASS (`10/10`) | `TestEvidence/bl076_promotion_t3_closeout/t3_summary.tsv` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-076 |
| Priority | P1 |
| Status | Done (owner T2 `5/5` + T3 `10/10` execute replay PASS; closeout/archive sync complete) |
| Track | F - Hardening |
| Effort | High / L |
| Depends On | BL-050, BL-069, BL-070 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-076-spatial-renderer-decomposition-planning-packet-2026-03-02.md` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| SHARED_FILES_TOUCHED | no |
| Promotion Decision Packet | `TestEvidence/bl076_promotion_t3_closeout/promotion_decision.md` |
| Final Evidence Root | `TestEvidence/bl076_promotion_t3_closeout/` |
| Archived Runbook Path | `Documentation/backlog/done/bl-076-spatial-renderer-decomposition-and-boundary-guardrails.md` |

## Completion Snapshot

| Item | Status | Priority | Estimate | Actual / Time | Tokens | Completed | Where | Evidence / Remaining |
|---|---|---|---|---|---|---|---|---|
| `~~W0-B~~` header/body split | `[DONE]` | P1 | Medium | done; time not logged | `n/a` | 2026-03-06 | `Source/SpatialRenderer.h`, `Source/SpatialRenderer.cpp` | Tier 0 objective complete |
| `~~Wave 4~~` Steam backend extraction | `[DONE]` | P1 | Medium | done; time not logged | `n/a` | 2026-03-06 | `Source/spatial_renderer/SpatialSteamAudioBackend.cpp` | none |
| `~~Wave 5~~` audition implementation-unit extraction | `[DONE]` | P1 | Large | done; time not logged | `n/a` | 2026-03-06 | `SpatialAuditionControl.cpp`, `SpatialAuditionSupport.cpp`, `SpatialAuditionSignalGenerator.cpp`, `SpatialAuditionRender.cpp` | none |
| `~~Wave 6~~` output-stage module extraction | `[DONE]` | P1 | Medium | done; same-day focused continuation | `n/a` | 2026-03-06 | `Source/spatial_renderer/SpatialOutputRoutingStage.cpp` | none |
| `~~Wave 6~~` headphone/profile control + support extraction | `[DONE]` | P1 | Medium | done; same-day focused continuation | `n/a` | 2026-03-06 | `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp`, `Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp` | moved `739` LOC total; `Source/SpatialRenderer.cpp` now `662` LOC |
| `~~BL-076~~` promotion closeout | `[DONE]` | P1 | Medium | owner T2/T3 cadence completed on 2026-03-06; wall-clock not logged | `n/a` | 2026-03-06 | `Documentation/backlog/done/bl-076-spatial-renderer-decomposition-and-boundary-guardrails.md`, `TestEvidence/bl076_promotion_t3_closeout/` | T2 `5/5` + T3 `10/10` execute replay PASS |
| W1-B thread-safety fixes | `[NEXT]` | P1 | Medium | not started | `n/a` | 2026-03-06 | `Documentation/architecture-code-review-2026-03-06.md` | next formal architecture item after BL-076 closeout |

## Objective

Decompose `Source/SpatialRenderer.h` into cohesive renderer modules with explicit ownership boundaries so the runtime can evolve without a single giant multipurpose header becoming a merge-risk and defect hotspot.

## What Was Built

- Moved the renderer from a monolithic header/body pair into focused `Source/spatial_renderer/*.cpp` units for Steam runtime, audition, output routing, and headphone/profile control/support.
- Reduced `Source/SpatialRenderer.cpp` to `662` LOC while keeping each extracted `.cpp` under the BL-076 planning-packet `<=700` LOC target.
- Closed the item with T1/T2/T3 execute replay evidence that kept structure guardrails, RT audit, smoke parity, and bridge payload parity green.

## Key Files

- `Source/SpatialRenderer.h`
- `Source/SpatialRenderer.cpp`
- `Source/spatial_renderer/SpatialSteamAudioBackend.cpp`
- `Source/spatial_renderer/SpatialAuditionControl.cpp`
- `Source/spatial_renderer/SpatialAuditionSupport.cpp`
- `Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp`
- `Source/spatial_renderer/SpatialAuditionRender.cpp`
- `Source/spatial_renderer/SpatialOutputRoutingStage.cpp`
- `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp`
- `Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp`

## Acceptance IDs

- `BL076-A-001`: `SpatialRenderer` responsibilities are split into named modules (for example: routing/mode orchestration, binaural/HRTF path, delay/FIR path, diagnostics snapshot publication, and format/profile contracts).
- `BL076-A-002`: `Source/SpatialRenderer.h` becomes a bounded orchestration/public-contract surface rather than a multipurpose implementation container.
- `BL076-A-003`: Structure guardrail lane enforces line-count and forbidden dependency rules for SpatialRenderer module boundaries.
- `BL076-A-004`: Existing smoke and RT-safety lanes remain green (`non_allowlisted=0`) after decomposition.
- `BL076-A-005`: Scene-state/bridge payload contracts remain parity-stable with deterministic replay evidence.

## Scope

In scope:
- `Source/SpatialRenderer.h` decomposition into focused `Source/spatial_renderer/*` units.
- Deterministic module-boundary and dependency guardrails for new SpatialRenderer units.
- Behavior-parity validation for existing rendering modes and bridge payloads.

Out of scope:
- New DSP features unrelated to decomposition.
- UI feature redesign work (handled in separate UI track runbooks).
- Runtime policy changes already owned by BL-069/BL-070 except where needed for structural extraction wiring.

## Validation Plan

QA harness script: `scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh`.
Evidence schema: `TestEvidence/bl076_*/status.tsv`.

Minimum evidence additions:
- `spatial_renderer_structure_guardrails.tsv`
- `spatial_renderer_module_dependency_matrix.tsv`
- `rt_audit.tsv`
- `smoke_parity_matrix.tsv`
- `bridge_payload_parity.tsv`

Primary lane commands:
- `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3`
- `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1`
- `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 5 --out-dir TestEvidence/bl076_candidate_t2_closeout`
- `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 10 --out-dir TestEvidence/bl076_promotion_t3_closeout`

## Owner Intake Blocker Snapshot (2026-03-02)

- Handoff replay attempt for decomposition planning packet stopped before execution.
- Blocker: global-lock guard detected unrelated workspace edits outside task ownership:
  - `TestEvidence/locusq_production_p0_selftest_20260302T035100Z.attempts.tsv`
  - `TestEvidence/locusq_production_p0_selftest_20260302T035100Z.failure_taxonomy.tsv`
  - `TestEvidence/locusq_production_p0_selftest_20260302T035100Z.meta.json`
- No scoped BL-076 files were changed and no validation artifacts were produced for that attempt.

## Owner Planning Packet Snapshot (2026-03-02)

- Planning packet authored:
  - `Documentation/plans/bl-076-spatial-renderer-decomposition-planning-packet-2026-03-02.md`
- Baseline captured:
  - `Source/SpatialRenderer.h` currently spans `4837` LOC.
  - extraction boundaries defined across 7 modules with a 6-wave migration plan.
- Guardrails defined:
  - dependency boundaries per module,
  - size caps (`<=700` LOC per `.cpp`, `<=250` LOC per `.h`),
  - RT-safety + validation-lane replay contract.
- Previous global-lock blocker is no longer active in owner workspace.

## Owner Execution Snapshot (2026-03-03)

- Guardrail lane authored:
  - `scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh`
- Wave 3 slice landed:
  - `Source/spatial_renderer/SpatialHeadphonePose.h`
  - `Source/spatial_renderer/SpatialHeadphoneCompensation.h`
  - `Source/spatial_renderer/SpatialHeadphonePoseAndCompensation.h` (module wrapper)
  - `Source/SpatialRenderer.h` rewired to extracted pose/compensation helpers.
- Wave 4 kickoff slice landed:
  - `Source/spatial_renderer/SpatialSteamAudioBackend.h`
  - `Source/SpatialRenderer.h` rewired to extracted Steam backend helpers
    (init-stage mapping, diagnostics strings, runtime candidate/open helpers, symbol-resolution helper).
- Wave 5 kickoff slice landed:
  - `Source/spatial_renderer/SpatialAuditionPrimitives.h`
  - `Source/SpatialRenderer.h` rewired to extracted audition primitives
    (audition level lookup, oscillator/noise/random helpers, azimuth wrapping, and audition voice layout helpers).
- Wave 5 completion slice landed:
  - `Source/spatial_renderer/SpatialAuditionEngine.h`
  - `Source/SpatialRenderer.h` rewired to extracted audition engine contracts
    (voice-excitation and physics-reactive timbre paths using explicit state/input structs).
- Wave 5 implementation-unit slice landed:
  - `Source/spatial_renderer/SpatialAuditionControl.cpp`
  - `Source/spatial_renderer/SpatialAuditionSupport.cpp`
  - `Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp`
  - `Source/spatial_renderer/SpatialAuditionRender.cpp`
  - `Source/SpatialRenderer.cpp` rewired to those out-of-line audition units
    (control state, telemetry/support helpers, signal generation, and audition render path).
- Wave 6 kickoff slice landed:
  - `Source/SpatialRenderer.h` now uses explicit staged orchestrator helpers:
    `runEmitterAccumulationStage` and `applyRoomAndSpeakerPostFx`.
  - Emitter selection/render accumulation and room/delay/trim post-FX were split from `process` into dedicated stage helpers with no behavior change.
- Wave 6 continuation slice landed:
  - `Source/SpatialRenderer.h` now routes profile/headphone/output-write orchestration through
    `runOutputRoutingAndHeadphoneStage`.
  - Output profile resolution, codec telemetry publication, stereo/quad/surround writers, and audition headphone parity updates were split from `process` into a dedicated stage helper with no behavior change.
- Wave 6 continuation slice landed (telemetry split):
  - `Source/SpatialRenderer.h` now routes ambisonic + codec contract publication through
    `publishAmbisonicAndCodecTelemetryContracts`.
  - Ambisonic IR contract updates and codec mapping/payload telemetry publication were split from output routing into a dedicated stage helper with no behavior change.
- Wave 6 continuation slice landed (audition parity split):
  - `Source/SpatialRenderer.h` now routes audition headphone parity accounting through dedicated helpers:
    `determineAuditionHeadphoneFallbackReason`, `accumulateAuditionHeadphoneParitySample`,
    and `finalizeAuditionHeadphoneParity`.
  - Audition headphone energy/reference/peak accumulation and parity publication were split from output routing into dedicated helpers with no behavior change.
- Wave 6 continuation slice landed (output writer split):
  - `Source/SpatialRenderer.h` now routes per-format output sample writing through dedicated helpers:
    `writeDiscreteOrAmbisonicOutputSample` and `renderStereoOutputSample`.
  - Surround/ambisonic/quad writers and stereo sample render selection were split from the main output loop into dedicated helpers with no behavior change.
- Wave 6 continuation slice landed (headphone runtime split):
  - `Source/SpatialRenderer.h` now routes headphone runtime configuration through dedicated helper
    `configureHeadphoneRuntime`.
  - Requested/active headphone mode selection, profile gating, Steam-availability fallbacks, and
    runtime calibration-chain activation were split from output routing into a dedicated helper with
    no behavior change.
- Wave 6 continuation slice landed (calibration runtime sync split):
  - `Source/SpatialRenderer.h` now routes calibration request/state synchronization through dedicated
    helpers `applyRequestedHeadphoneCalibrationSettings` and
    `publishHeadphoneCalibrationRuntimeState`.
  - Repeated prepare/reset/runtime calibration chain request+state publication logic was split into
    dedicated helpers with no behavior change.
- Wave 6 continuation slice landed (output-stage context split):
  - `Source/SpatialRenderer.h` now routes output-stage setup through dedicated helper
    `prepareOutputRoutingStageContext`.
  - Profile resolution + stage publication + headphone runtime setup were split from
    `runOutputRoutingAndHeadphoneStage` into a dedicated context helper with no behavior change.
- Wave 6 continuation slice landed (mono output writer split):
  - `Source/SpatialRenderer.h` now routes mono output writes through dedicated helper
    `writeMonoOutputSample`.
  - Mono summation/write behavior was split from `runOutputRoutingAndHeadphoneStage` into a
    dedicated helper with no behavior change.
- Wave 6 continuation slice landed (stereo output writer split):
  - `Source/SpatialRenderer.h` now routes stereo output writes through dedicated helper
    `writeStereoOutputSample`.
  - Stereo render/parity accumulation/compensation/write behavior was split from
    `runOutputRoutingAndHeadphoneStage` into a dedicated helper with no behavior change.
- Wave 6 continuation slice landed (output-sample dispatch split):
  - `Source/SpatialRenderer.h` now routes per-sample channel-layout dispatch through dedicated helper
    `writeOutputSampleForChannelLayout`.
  - Discrete/ambisonic, stereo, and mono branch dispatch was split from
    `runOutputRoutingAndHeadphoneStage` into a dedicated helper with no behavior change.
- Wave 6 continuation slice landed (audition parity lifecycle split):
  - `Source/SpatialRenderer.h` now routes audition parity setup + publish through dedicated helpers
    `prepareAuditionHeadphoneParityAccumulator` and `publishAuditionHeadphoneParityForBlock`.
  - Parity fallback-reason initialization and end-of-block parity publication were split from
    `runOutputRoutingAndHeadphoneStage` into dedicated helpers with no behavior change.
- Wave 6 continuation slice landed (emitter render-pass helper module split):
  - Added `Source/spatial_renderer/SpatialEmitterRenderPass.h` with shared emitters-budget helper
    primitives `insertCandidateWithBudget` and `sortSelectedBySlotIndex`.
  - `runEmitterAccumulationStage` now routes candidate budget insertion + deterministic slot ordering
    through that module with no behavior change.
- Wave 6 continuation slice landed (post-FX chain helper module split):
  - Added `Source/spatial_renderer/SpatialPostFxChain.h` with room-FX and speaker
    delay/trim helper primitives.
  - `applyRoomAndSpeakerPostFx` now routes room processing + speaker delay/trim steps through that
    module with no behavior change.
- Wave 6 continuation slice landed (emitter second-pass helper split):
  - `Source/SpatialRenderer.h` now routes selected-emitter DSP/render accumulation via dedicated helper
    `processSelectedEmitterCandidate`.
  - Per-emitter audio snapshot, activity gate, doppler/absorption, panner/directivity, smoothing,
    and accumulation steps were split from `runEmitterAccumulationStage` into that helper with no
    behavior change.
- Wave 6 continuation slice landed (emitter first-pass helper split):
  - `Source/SpatialRenderer.h` now routes first-pass emitter candidate collection + budget selection
    via dedicated helper `collectEmitterCandidatesForBlock`.
  - Per-slot active/mute/gain/distance/priority eligibility and budgeted candidate selection were
    split from `runEmitterAccumulationStage` into that helper with no behavior change.
- Wave 6 continuation slice landed (emitter audition-fallback helper split):
  - `Source/SpatialRenderer.h` now routes emitters-empty audition fallback finalization via dedicated
    helper `finalizeEmitterStageWithAuditionFallback`.
  - End-of-stage audition fallback render path and no-fallback telemetry reset were split from
    `runEmitterAccumulationStage` into that helper with no behavior change.
- Wave 6 continuation slice landed (selected-emitter loop helper split):
  - `Source/SpatialRenderer.h` now routes second-pass selected-emitter loop execution via dedicated
    helper `processSelectedEmittersForBlock`.
  - Selected emitter iteration + per-candidate processing invocation were split from
    `runEmitterAccumulationStage` into that helper with no behavior change.
- Wave 6 continuation slice landed (codec payload publication helper split):
  - `Source/SpatialRenderer.h` now routes codec ADM/IAMF payload publication through dedicated helpers
    `publishCodecAdmPayloadContract` and `publishCodecIamfPayloadContract`.
  - Codec payload atomics publication loops were split from
    `publishAmbisonicAndCodecTelemetryContracts` into those helpers with no behavior change.
- Wave 6 continuation slice landed (codec mapping contract helper split):
  - `Source/SpatialRenderer.h` now routes codec mapping contract details through dedicated helpers
    `determineCodecMappedChannelCount`, `isCodecMappingFiniteForBlock`, and
    `publishCodecMappingContractState`.
  - Codec mapped-channel selection, finite-sample guard scan, and mapping-state atomics publication
    were split from `publishAmbisonicAndCodecTelemetryContracts` into those helpers with no behavior
    change.
- Wave 6 continuation slice landed (codec mode/signature helper split):
  - `Source/SpatialRenderer.h` now routes codec mode + count derivation and signature build via
    dedicated helpers `determineCodecModeForProfile`, `determineCodecObjectCount`,
    `determineCodecElementCount`, and `buildCodecMappingSignature`.
  - Codec mode routing, object/element count derivation, and signature composition were split from
    `publishAmbisonicAndCodecTelemetryContracts` into those helpers with no behavior change.
- Wave 6 continuation slice landed (ambisonic IR contract helper split):
  - `Source/SpatialRenderer.h` now routes ambisonic IR contract publication through dedicated helper
    `publishAmbisonicIrContractState`.
  - Requested/active ambisonic order resolution, fallback derivation, timestamp cursor advance, and
    ambisonic contract atomics publication were split from
    `publishAmbisonicAndCodecTelemetryContracts` into that helper with no behavior change.
- Validation replay:
  - `cmake --build build --config Release --target LocusQ -- -j8` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1` -> PASS
- Evidence roots:
  - `TestEvidence/bl076_spatial_renderer_20260305T013056Z/` (contract-only)
  - `TestEvidence/bl076_spatial_renderer_20260305T013106Z/` (execute)
- Required evidence emitted:
  - `spatial_renderer_structure_guardrails.tsv`
  - `spatial_renderer_module_dependency_matrix.tsv`
  - `rt_audit.tsv`
  - `smoke_parity_matrix.tsv`
  - `bridge_payload_parity.tsv`
- Execute lane semantics:
  - `BL076-EXEC-scaffold_rows` PASS with zero `TODO`/`SCAFFOLD` rows.

## W0-B Closeout Snapshot (2026-03-06)

- W0-B landed as a real header/body split:
  - added `Source/SpatialRenderer.cpp`
  - reduced `Source/SpatialRenderer.h` from `4366` LOC to `982` LOC
  - preserved the previously extracted `Source/spatial_renderer/*` helper modules
- Affected non-plugin targets were aligned to the post-W0-A source layout:
  - `locusq_qa` and `locusq_bl018_profile_probe` now include `Source/SpatialRenderer.cpp`
  - those same console targets now also include the extracted `Source/processor_core/*.cpp` units so `LocusQAudioProcessor` links cleanly after the earlier modularization work
- Validation replay:
  - `cmake --build build_local --config Release --target LocusQ -- -j8` -> PASS
  - `cmake --build build_local --config Release --target locusq_qa locusq_bl018_profile_probe -- -j8` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1` -> PASS
- Evidence roots:
  - `TestEvidence/bl076_spatial_renderer_20260306T211113Z/` (contract-only)
  - `TestEvidence/bl076_spatial_renderer_20260306T211126Z/` (execute)
- Follow-up note:
  - W0-B is complete and the header is now bounded, but BL-076 remains `In Implementation` because `Source/SpatialRenderer.cpp` is still a large implementation unit relative to the planning-packet size goals. Further decomposition can build on this safer out-of-line baseline.

## Wave 4 Steam Backend Implementation-Unit Snapshot (2026-03-06)

- Wave 4 made concrete progress on the next BL-076 slice:
  - added `Source/spatial_renderer/SpatialSteamAudioBackend.cpp`
  - moved Steam runtime, diagnostics, monitoring, and binaural render method bodies out of `Source/SpatialRenderer.cpp`
  - fixed the Steam monitoring path to call `locusq::spatial_headphone_pose::buildSpeakerMixFromOrientation(...)` explicitly, which keeps Steam-enabled builds honest instead of relying on an unqualified lookup
- Current file-size posture after the slice:
  - `Source/SpatialRenderer.cpp` reduced from `3998` LOC to `3530` LOC
  - `Source/spatial_renderer/SpatialSteamAudioBackend.cpp` now owns `453` LOC of Steam-specific implementation
- Validation replay:
  - `cmake --build build_local --config Release --target LocusQ locusq_qa locusq_bl018_profile_probe -- -j8` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1` -> PASS
- Evidence roots:
  - `TestEvidence/bl076_spatial_renderer_20260306T214410Z/` (contract-only)
  - `TestEvidence/bl076_spatial_renderer_20260306T214421Z/` (execute)
- Follow-up note:
  - BL-076 remains `In Implementation`; the next highest-yield slice is still the audition engine extraction because `Source/SpatialRenderer.cpp` remains materially above the planning-packet `<=700` LOC target.

## Wave 5 Audition Implementation-Unit Snapshot (2026-03-06)

- Wave 5 made concrete progress on the next BL-076 slice:
  - added `Source/spatial_renderer/SpatialAuditionControl.cpp`
  - added `Source/spatial_renderer/SpatialAuditionSupport.cpp`
  - added `Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp`
  - added `Source/spatial_renderer/SpatialAuditionRender.cpp`
  - moved audition control, telemetry/support, signal-generation, and render method bodies out of `Source/SpatialRenderer.cpp`
  - kept every new audition `.cpp` under the planning-packet `<=700` LOC goal
- Current file-size posture after the slice:
  - `Source/SpatialRenderer.cpp` reduced from `3530` LOC to `1981` LOC
  - `Source/spatial_renderer/SpatialAuditionControl.cpp` now owns `270` LOC
  - `Source/spatial_renderer/SpatialAuditionSupport.cpp` now owns `293` LOC
  - `Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp` now owns `424` LOC
  - `Source/spatial_renderer/SpatialAuditionRender.cpp` now owns `531` LOC
- Validation replay:
  - `cmake --build build_local --config Release --target LocusQ locusq_qa locusq_bl018_profile_probe -- -j8` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1` -> PASS
- Evidence roots:
  - `TestEvidence/bl076_spatial_renderer_20260306T221704Z/` (contract-only)
  - `TestEvidence/bl076_spatial_renderer_20260306T221823Z/` (execute)
- Follow-up note:
  - BL-076 remains `In Implementation`; the next highest-yield slice is the remaining Wave 6 staged-orchestrator cleanup because `Source/SpatialRenderer.cpp` still exceeds the planning-packet `<=700` LOC target even after the audition split.

## Wave 6 Output-Stage Module Snapshot (2026-03-06)

- Wave 6 made concrete progress on the next BL-076 slice:
  - added `Source/spatial_renderer/SpatialOutputRoutingStage.cpp`
  - moved codec payload publication, codec mapping/signature, ambisonic IR contract publication, output routing, headphone runtime/calibration sync, and mono/stereo/discrete output writer method bodies out of `Source/SpatialRenderer.cpp`
  - updated `CMakeLists.txt` so `LocusQ`, `locusq_qa`, and `locusq_bl018_profile_probe` all build the new output-stage module
- Current file-size posture after the slice:
  - moved `577` LOC from `Source/SpatialRenderer.cpp` into `Source/spatial_renderer/SpatialOutputRoutingStage.cpp`
  - `Source/SpatialRenderer.cpp` reduced from `1981` LOC to `1404` LOC
  - `Source/spatial_renderer/SpatialOutputRoutingStage.cpp` now owns `577` LOC
- Validation replay:
  - `cmake --build build_local --config Release --target LocusQ locusq_qa locusq_bl018_profile_probe -- -j8` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1` -> PASS
- Evidence roots:
  - `TestEvidence/bl076_spatial_renderer_20260306T235606Z/` (shared contract-only + execute status ledger)
- Follow-up note:
  - BL-076 remains `In Implementation`; `Source/SpatialRenderer.cpp` is still above the planning-packet `<=700` LOC target, so the next highest-yield slice remains the remaining staged-orchestrator cleanup in the lifecycle/setup and head-pose/preset helper clusters.

## Wave 6 Headphone/Profile Support Snapshot (2026-03-06)

- Wave 6 completed the remaining staged-orchestrator cleanup slice:
  - added `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp`
  - added `Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp`
  - moved `426` LOC of headphone/profile control, telemetry getter, and `*ToString` method bodies out of `Source/SpatialRenderer.cpp` into `SpatialHeadphoneProfileControl.cpp`
  - moved `313` LOC of bundled-preset, head-pose, profile-routing, output-support, headphone-compensation, and geometry helper bodies out of `Source/SpatialRenderer.cpp` into `SpatialHeadphoneProfileSupport.cpp`
  - updated `CMakeLists.txt` so `LocusQ`, `locusq_qa`, and `locusq_bl018_profile_probe` all build the new support modules
- Current file-size posture after the slice:
  - moved `739` LOC total from `Source/SpatialRenderer.cpp`
  - `Source/SpatialRenderer.cpp` reduced from `1404` LOC to `662` LOC
  - `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp` now owns `428` LOC
  - `Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp` now owns `315` LOC
  - the planning-packet `<=700` LOC target is now satisfied for `Source/SpatialRenderer.cpp`
- Validation replay:
  - `cmake --build build_local --config Release --target LocusQ locusq_qa locusq_bl018_profile_probe -- -j8` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1` -> PASS
- Evidence roots:
  - `TestEvidence/bl076_spatial_renderer_20260307T002345Z/` (contract-only)
  - `TestEvidence/bl076_spatial_renderer_20260307T002358Z/` (execute)
- Follow-up note:
  - The decomposition target was met here; owner candidate/promotion cadence and closeout sync followed, so the next architecture-roadmap implementation item is `W1-B`.

## Owner T2/T3 Replay Snapshot (2026-03-06)

- T2 candidate replay (5 execute runs): `PASS`
  - `TestEvidence/bl076_candidate_t2_closeout/`
  - `t2_summary.tsv`: `5/5` runs with `lane_result=PASS`, execute TODO gate `PASS`, and RT audit `non_allowlisted=0` on every run.
- T3 promotion replay (10 execute runs): `PASS`
  - `TestEvidence/bl076_promotion_t3_closeout/`
  - `t3_summary.tsv`: `10/10` runs with `lane_result=PASS`, execute TODO gate `PASS`, and RT audit `non_allowlisted=0` on every run.

## Evidence References

- `TestEvidence/bl076_spatial_renderer_20260307T002345Z/status.tsv`
- `TestEvidence/bl076_spatial_renderer_20260307T002358Z/status.tsv`
- `TestEvidence/bl076_candidate_t2_closeout/t2_summary.tsv`
- `TestEvidence/bl076_promotion_t3_closeout/t3_summary.tsv`
- `TestEvidence/bl076_promotion_t3_closeout/promotion_decision.md`

## Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + target wiring | PASS | `TestEvidence/build-summary.md` |
| T1 execute replay | PASS | `TestEvidence/bl076_spatial_renderer_20260307T002358Z/status.tsv` |
| T2 candidate replay | PASS | `TestEvidence/bl076_candidate_t2_closeout/t2_summary.tsv` |
| T3 promotion replay | PASS | `TestEvidence/bl076_promotion_t3_closeout/t3_summary.tsv` |
| RT safety | PASS | `TestEvidence/bl076_promotion_t3_closeout/rt_audit.tsv` |
| Bridge payload parity | PASS | `TestEvidence/bl076_promotion_t3_closeout/bridge_payload_parity.tsv` |
| Docs freshness | PASS | `TestEvidence/bl076_promotion_t3_closeout/docs_freshness.log` |
| Status schema | PASS | `TestEvidence/bl076_promotion_t3_closeout/status_json_check.log` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | PASS | `TestEvidence/bl076_promotion_t3_closeout/handoff_resolution.md` |

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

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment (2026-03-01)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

## Closeout Checklist (Done Transition)

- [x] T2 candidate (`5/5`) and T3 promotion (`10/10`) execute replays are PASS.
- [x] Execute-mode TODO gate is PASS (`BL076-EXEC-scaffold_rows`).
- [x] RT audit remains PASS (`non_allowlisted=0`) across promotion packet runs.
- [x] Promotion decision packet is recorded under `TestEvidence/bl076_promotion_t3_closeout/`.
- [x] Runbook moved to `Documentation/backlog/done/bl-076-spatial-renderer-decomposition-and-boundary-guardrails.md`.
- [x] `Documentation/backlog/index.md` row updated to Done and switched to `done/` path.
- [x] `status.json`, `TestEvidence/build-summary.md`, and `TestEvidence/validation-trend.md` updated in the same change set.
- [x] `./scripts/validate-docs-freshness.sh` passes after done/archive sync.
- [x] `jq empty status.json` passes after done/archive sync.

## Completion Date

2026-03-06
