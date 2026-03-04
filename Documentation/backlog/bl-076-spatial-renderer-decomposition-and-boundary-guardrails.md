Title: BL-076 SpatialRenderer Decomposition and Boundary Guardrails
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-04

# BL-076 SpatialRenderer Decomposition and Boundary Guardrails

## Plain-Language Summary

BL-076 in plain terms: Decompose Source/SpatialRenderer.h into cohesive renderer modules with explicit ownership boundaries so the runtime can evolve without a single giant multipurpose header becoming a merge-risk and defect hotspot. Current state: In Implementation (Wave 6 emitter render-pass helper module extraction landed; contract+execute replay PASS on 2026-03-04). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA owners, release owners, and engineering maintainers who depend on deterministic evidence. |
| What is changing? | Decompose Source/SpatialRenderer.h into cohesive renderer modules with explicit ownership boundaries so the runtime can evolve without a single giant multipurpose header becoming a merge-risk and defect hotspot. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: In Implementation (Wave 6 emitter render-pass helper module extraction landed; contract+execute replay PASS on 2026-03-04). This item is done when required acceptance checks pass and promotion evidence is complete. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-076-spatial-renderer-decomposition-and-boundary-guardrails.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-076 |
| Priority | P1 |
| Status | In Implementation (Wave 6 emitter render-pass helper module extraction landed; contract+execute replay PASS on 2026-03-04) |
| Track | F - Hardening |
| Effort | High / L |
| Depends On | BL-050, BL-069, BL-070 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-076-spatial-renderer-decomposition-planning-packet-2026-03-02.md` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Decompose `Source/SpatialRenderer.h` into cohesive renderer modules with explicit ownership boundaries so the runtime can evolve without a single giant multipurpose header becoming a merge-risk and defect hotspot.

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
- Validation replay:
  - `cmake --build build --config Release --target LocusQ -- -j8` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --contract-only --runs 3` -> PASS
  - `./scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh --execute --runs 1` -> PASS
- Evidence roots:
  - `TestEvidence/bl076_spatial_renderer_20260304T234340Z/` (contract-only)
  - `TestEvidence/bl076_spatial_renderer_20260304T234355Z/` (execute)
- Required evidence emitted:
  - `spatial_renderer_structure_guardrails.tsv`
  - `spatial_renderer_module_dependency_matrix.tsv`
  - `rt_audit.tsv`
  - `smoke_parity_matrix.tsv`
  - `bridge_payload_parity.tsv`
- Execute lane semantics:
  - `BL076-EXEC-scaffold_rows` PASS with zero `TODO`/`SCAFFOLD` rows.

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
