Title: LocusQ Scene State Contract
Document Type: Interface Contract
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-26

# Scene State Contract

## Purpose

Define the single source-of-truth contract between DSP runtime, physics, and UI so `skill_plan`, `skill_design`, and `skill_impl` execute against the same state model.

## Normative Decisions

- Routing model: `Documentation/adr/ADR-0002-routing-model-v1.md`
- Automation precedence: `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- AI gating for v1: `Documentation/adr/ADR-0004-v1-ai-deferral.md`

## Scope

Applies to:

1. Emitter/Renderer state exchange through `SceneGraph`
2. Physics-to-audio position/velocity handoff
3. UI snapshot publication and command ingestion

## State Domains

### Audio Domain (Real-Time)

Owned by audio thread in `processBlock`:

- per-emitter metadata state (active, position, size, gain, spread, directivity, velocity, labels, flags)
- ephemeral emitter audio block pointer fast path (v1)
- renderer accumulation/output state

Constraints:

- no locks
- no heap allocation
- deterministic per-block behavior for identical input/state

### Physics Domain (Worker Thread)

Owned by physics worker:

- body state (position, velocity)
- force integration state

Handoff:

- lock-free double-buffer/atomic publication to audio domain
- additive offset semantics against rest pose (per ADR-0003)

### Message/UI Domain

Owned by message thread/WebView bridge:

- JSON snapshots for UI rendering
- incoming control commands (parameter edits, timeline edits, preset actions)

Constraints:

- never call WebView from audio thread
- snapshot publication rate bounded (for example 30-60 Hz)
- bridge commands apply as parameter/state updates, not direct DSP mutation

## Source Of Truth Model

1. APVTS/host parameter state is base authority.
2. Internal timeline can define rest pose for animated tracks when enabled.
3. Physics applies additive offset.
4. SceneGraph publishes the resulting emitter state for renderer consumption.

Any conflict resolution beyond this contract requires ADR update.

## Routing Contract (V1)

1. Emitter publishes metadata each block.
2. Emitter also publishes an ephemeral audio pointer fast path for same-block renderer consumption.
3. Renderer consumes scene state within the same callback cycle.
4. If fast-path assumptions fail in a host/runtime context, runtime must degrade safely (no crash/non-finite output), and host-edge acceptance evidence must capture behavior.

## Serialization Contract

UI snapshot payloads are derived from stable copies of runtime state and include:

- emitter transforms and labels
- velocity vectors and animation state indicators
- room/speaker/listener snapshots
- coarse performance telemetry needed by UI

Serialization is message-thread work; audio thread remains non-blocking.

## Visualization Transport Contract (BL-016)

Scene snapshot payloads used by `window.updateSceneState(...)` must include:

- `snapshotSchema` (string): schema marker, currently `locusq-scene-snapshot-v1`.
- `snapshotSeq` (integer): monotonically increasing publication sequence per editor session.
- `snapshotPublishedAtUtcMs` (integer): UTC milliseconds when snapshot was published.
- `snapshotCadenceHz` (integer): intended publication cadence (for smoothing calibration).
- `snapshotStaleAfterMs` (integer): stale timeout budget for UI fallback.

Rules:

1. UI must reject out-of-order snapshots (`snapshotSeq <= lastAcceptedSeq`).
2. UI rendering may smooth toward snapshot targets, but canonical values remain native snapshot values.
3. If no accepted snapshot arrives within `snapshotStaleAfterMs`, UI must enter a safe stale mode (for example warning status + reduced visual confidence) without blocking control-path interaction.
4. On next accepted snapshot, stale mode must clear deterministically.
5. Contract changes to payload semantics require an ADR update.

## Payload Budget Contract (HX-05 Slice A)

The scene-state transport path is additionally bounded by the following payload/throughput policy.

### Normative Limits

| Dimension | Normal Target | Soft Limit | Hard Limit |
|---|---:|---:|---:|
| Serialized payload bytes per snapshot | <= 24,576 B | <= 32,768 B | <= 65,536 B |
| Publication cadence | 30 Hz nominal | <= 45 Hz burst cap | 60 Hz absolute cap |
| Soft-overage burst window | n/a | max 8 consecutive snapshots | must recover <= 500 ms |

Rules:

1. Transport must never exceed the hard payload limit (`65,536` bytes) or absolute cadence cap (`60 Hz`).
2. Soft-limit overages are temporary and burst-governed; no soft-overage burst may exceed 8 snapshots or 500 ms.
3. Hard-overage snapshots require immediate degrade behavior in the publisher path.

### Degradation Tiers

| Tier | Entry Condition | Required Behavior | Exit Condition |
|---|---|---|---|
| `normal` | Within soft limit and cadence target | Full payload at nominal cadence | n/a |
| `degrade_t1` | Hard overage once OR soft burst overrun | Clamp cadence to <= 20 Hz and prioritize core fields (`emitters`, `listener`, `speakers`, diagnostics) | 120 consecutive compliant snapshots |
| `degrade_t2_safe` | Hard overage in 3 of any 10 snapshots | Clamp cadence to <= 10 Hz and publish minimal deterministic transport subset | 240 consecutive compliant snapshots |

### Additive Schema Guidance (Non-Breaking)

Future HX-05 runtime telemetry fields must be additive/optional:

- `snapshotPayloadBytes` (integer)
- `snapshotBudgetTier` (string enum: `normal`, `degrade_t1`, `degrade_t2_safe`)
- `snapshotBurstCount` (integer)
- `snapshotBudgetPolicyVersion` (string; initial value `hx05-v1`)

Compatibility rule: consumers must ignore unknown fields, and if HX-05 fields are absent, existing behavior remains valid.

### Acceptance Hooks

| Acceptance ID | Required Evidence |
|---|---|
| `HX05-AC-001` | This section contains explicit bytes/cadence/burst/degrade limits |
| `HX05-AC-002` | Additive schema guidance and compatibility rule are present |
| `HX05-AC-004` | `./scripts/validate-docs-freshness.sh` passes for the change set |

## Viewport Visualization Payload Contract (BL-015/BL-014/BL-008/BL-006/BL-007)

For production viewport rendering, scene snapshots must include:

- Per-emitter fields:
  - `id`, `x`, `y`, `z`
  - `vx`, `vy`, `vz`
  - `fx`, `fy`, `fz`
  - `collisionMask`, `collisionEnergy`
  - `selected`
  - `directivity`
  - `aimX`, `aimY`, `aimZ`
  - `rms`, `rmsDb`
- Top-level fields:
  - `roomProfileValid`
  - `roomDimensions` (`width`, `depth`, `height`)
  - `listener` (`x`, `y`, `z`)
  - `speakerRms` (array)
  - `speakers` array (`id`, `label`, `x`, `y`, `z`, `gainTrimDb`, `delayCompMs`, `rms`)
  - Headphone render diagnostics:
    - `rendererHeadphoneModeRequested` (string enum)
    - `rendererHeadphoneModeActive` (string enum)
    - `rendererHeadphoneProfileRequested` (string enum: `generic`, `airpods_pro_2`, `sony_wh1000xm5`, `custom_sofa`)
    - `rendererHeadphoneProfileActive` (string enum)
    - `rendererHeadphoneProfileCatalogVersion` (string enum; current value `bl034-profile-catalog-v1`)
    - `rendererHeadphoneProfileFallbackReason` (string enum: `none`, `requested_profile_unavailable`, `requested_profile_invalid`, `custom_sofa_ref_missing`, `custom_sofa_ref_invalid`, `steam_unavailable`, `output_incompatible`, `monitoring_path_bypassed`, `catalog_version_mismatch`)
    - `rendererHeadphoneProfileFallbackTarget` (string enum: `none`, `generic`, `airpods_pro_2`, `sony_wh1000xm5`, `custom_sofa`)
    - `rendererHeadphoneProfileCustomSofaRef` (string; bounded token, empty when inactive)
    - `rendererHeadphoneProfileGovernance` (object mirror with `catalogVersion`, `requested`, `active`, `fallbackReason`, `fallbackTarget`, `customSofaRef`)
    - `rendererHeadphoneCalibrationSchema` (string; current value `locusq-headphone-calibration-contract-v1`)
    - `rendererHeadphoneCalibrationRequested` (string enum: `speakers`, `stereo_downmix`, `steam_binaural`, `virtual_binaural`)
    - `rendererHeadphoneCalibrationActive` (string enum; same domain as requested)
    - `rendererHeadphoneCalibrationStage` (string enum: `direct`, `ready`, `initializing`, `fallback`, `unavailable`)
    - `rendererHeadphoneCalibrationFallbackReady` (bool)
    - `rendererHeadphoneCalibrationFallbackReason` (string enum: `none`, `steam_unavailable`, `output_incompatible`, `monitoring_path_bypassed`)
    - `rendererHeadphoneCalibration` (object mirror with `schema`, `requested`, `active`, `stage`, `fallbackReady`, `fallbackReason`)
    - Head-tracking bridge diagnostics (additive):
      - `rendererHeadTrackingEnabled` (bool; true only when build enables bridge receiver integration)
      - `rendererHeadTrackingSource` (string; current values: `disabled`, `udp_loopback:19765`)
      - `rendererHeadTrackingPoseAvailable` (bool; true when at least one valid pose packet has been published)
      - `rendererHeadTrackingPoseStale` (bool; true when latest pose age exceeds stale threshold)
      - `rendererHeadTrackingOrientationValid` (bool; true when derived yaw/pitch/roll are finite)
      - `rendererHeadTrackingInvalidPackets` (uint32; cumulative decode/validation rejects since bridge start)
      - `rendererHeadTrackingSeq` (uint32; latest accepted pose sequence number)
      - `rendererHeadTrackingTimestampMs` (uint64; latest accepted pose source timestamp, ms)
      - `rendererHeadTrackingAgeMs` (float; latest pose age in milliseconds)
      - `rendererHeadTrackingQx` / `rendererHeadTrackingQy` / `rendererHeadTrackingQz` / `rendererHeadTrackingQw` (float; latest normalized quaternion telemetry)
      - `rendererHeadTrackingYawDeg` / `rendererHeadTrackingPitchDeg` / `rendererHeadTrackingRollDeg` (float; derived orientation telemetry)
      - `rendererHeadTracking` (object mirror with `enabled`, `source`, `poseAvailable`, `poseStale`, `orientationValid`, `invalidPackets`, `seq`, `timestampMs`, `ageMs`, `qx`, `qy`, `qz`, `qw`, `yawDeg`, `pitchDeg`, `rollDeg`)
    - `rendererPhysicsLensEnabled` (bool)
    - `rendererPhysicsLensMix` (float 0..1)
    - `rendererSteamAudioCompiled` (bool)
    - `rendererSteamAudioAvailable` (bool)
    - `rendererSteamAudioInitStage` (string enum)
    - `rendererSteamAudioInitErrorCode` (integer)
    - `rendererSteamAudioRuntimeLib` (string)
    - `rendererSteamAudioMissingSymbol` (string)
  - Renderer audition cloud metadata (BL-029 Slice C):
    - `rendererAuditionCloud.enabled` (bool)
    - `rendererAuditionCloud.pattern` (string enum; current: `tone_core`, `dual_orbit`, `noise_halo`, `rain_sheet`, `snow_cloud`, `bounce_cluster`, `chime_constellation`)
    - `rendererAuditionCloud.mode` (string enum; current: `single_core`, `dual_pair`, `noise_cluster`, `precipitation_rain`, `precipitation_snow`, `impact_swarm`, `chime_cluster`)
    - `rendererAuditionCloud.emitterCount` (integer; bounded 0..8, 0 when cloud disabled)
    - `rendererAuditionCloud.pointCount` (integer)
    - `rendererAuditionCloud.spreadMeters` (float)
    - `rendererAuditionCloud.seed` (unsigned integer; deterministic for current signal/motion/level state)
    - `rendererAuditionCloud.pulseHz` (float)
    - `rendererAuditionCloud.coherence` (float 0..1)
    - `rendererAuditionCloud.emitters[]` (array of deterministic local cluster sources):
      - `id` (integer)
      - `weight` (float 0..1)
      - `localOffsetX` / `localOffsetY` / `localOffsetZ` (float meters, local to `rendererAuditionVisual` centroid)
      - `phase` (float 0..1)
      - `activity` (float 0..1)
  - Renderer audition authority metadata (BL-029 Slice B1; additive/non-breaking):
    - `rendererAuditionSourceMode` (string enum; legacy standalone family, current values: `single`, `cloud`)
    - `rendererAuditionRequestedMode` (string enum; current values: `single`, `cloud`, `bound_emitter`, `bound_choreography`, `bound_physics`)
    - `rendererAuditionResolvedMode` (string enum; same domain as `rendererAuditionRequestedMode`, after native fallback resolution)
    - `rendererAuditionBindingTarget` (string; current values: `none`, `emitter:<id>`, `timeline:global`)
    - `rendererAuditionBindingAvailable` (bool; true only when resolved mode has a valid binding target)
    - `rendererAuditionSeed` (unsigned integer; deterministic replay seed for renderer audition family)
    - `rendererAuditionTransportSync` (bool; current default `false`)
    - `rendererAuditionDensity` (float 0..1; normalized point-density envelope)
    - `rendererAuditionReactivity` (float 0..1; normalized level/coherence response envelope)
    - `rendererAuditionFallbackReason` (string enum; current values: `none`, `audition_disabled`, `renderer_mode_inactive`, `bound_emitter_unavailable`, `bound_choreography_unavailable`, `bound_physics_unavailable`, `visual_centroid_unavailable`, `visual_centroid_invalid`, `cloud_geometry_invalid`, `cloud_emitters_unavailable`, `cloud_bounds_clamped`, `reactive_payload_missing`, `reactive_payload_invalid`, `reactive_source_count_invalid`)
  - Renderer audition reactive telemetry (BL-029 Slice G1; additive/non-breaking):
    - `rendererAuditionReactive.rms` (float 0..1; mixed audition-source RMS envelope after native guard-rail sanitize)
    - `rendererAuditionReactive.peak` (float 0..1; mixed audition-source peak envelope after native guard-rail sanitize)
    - `rendererAuditionReactive.envFast` (float 0..1; fast-smoothed audition envelope after native guard-rail sanitize)
    - `rendererAuditionReactive.envSlow` (float 0..1; slow-smoothed audition envelope after native guard-rail sanitize)
    - `rendererAuditionReactive.onset` (float 0..1; positive fast-vs-slow onset detector)
    - `rendererAuditionReactive.brightness` (float 0..1; high-frequency energy ratio proxy)
    - `rendererAuditionReactive.rainFadeRate` (float 0..1; deterministic fade driver tuned for rain-family visuals)
    - `rendererAuditionReactive.snowFadeRate` (float 0..1; deterministic fade driver tuned for snow-family visuals)
    - `rendererAuditionReactive.physicsVelocity` (float 0..1; normalized physics-speed drive used by audition timbre/envelope coupling)
    - `rendererAuditionReactive.physicsCollision` (float 0..1; normalized collision-energy drive used by audition transient coupling)
    - `rendererAuditionReactive.physicsDensity` (float 0..1; normalized active-physics-emitter density used by audition cloud/timbre coupling)
    - `rendererAuditionReactive.physicsCoupling` (float 0..1; composite deterministic coupling intensity from velocity/collision/density)
    - `rendererAuditionReactive.geometryScale` (float 0..1; additive geometry morph scalar derived from reactive envelope + physics coupling + source density)
    - `rendererAuditionReactive.geometryWidth` (float 0..1; additive width morph scalar derived from density/velocity/brightness coupling)
    - `rendererAuditionReactive.geometryDepth` (float 0..1; additive depth morph scalar derived from slow envelope + coupling + inverse brightness)
    - `rendererAuditionReactive.geometryHeight` (float 0..1; additive height morph scalar derived from onset/collision/fast envelope/peak)
    - `rendererAuditionReactive.precipitationFade` (float 0..1; additive unified precipitation fade drive derived from rain/snow reactive rates)
    - `rendererAuditionReactive.collisionBurst` (float 0..1; additive transient burst scalar derived from collision energy shaped by onset)
    - `rendererAuditionReactive.densitySpread` (float 0..1; additive spread scalar derived from physics density + source-density + velocity)
    - `rendererAuditionReactive.headphoneOutputRms` (float 0..1; headphone render-output RMS envelope for BL-009 parity diagnostics)
    - `rendererAuditionReactive.headphoneOutputPeak` (float 0..1; headphone render-output peak envelope for BL-009 parity diagnostics)
    - `rendererAuditionReactive.headphoneParity` (float 0..1; deterministic parity confidence scalar after native guard-rail sanitize)
    - `rendererAuditionReactive.headphoneFallback` (bool; true when headphone render path fell back for this snapshot)
    - `rendererAuditionReactive.headphoneFallbackReason` (string enum; current values: `none`, `steam_unavailable`, `steam_render_failed`, `output_incompatible`)
    - `rendererAuditionReactive.sourceEnergy[]` (array<float> 0..1, length 0..8; per-source normalized energy for active audition voices)
    - `rendererAuditionReactive.reactiveActive` (bool; true only when renderer audition is active in renderer mode and a valid visual centroid exists)
    - `rendererAuditionReactive.rmsNorm` / `peakNorm` / `envFastNorm` / `envSlowNorm` (float 0..1; explicit unit mirrors of the base reactive envelopes)
    - `rendererAuditionReactive.onsetNorm` / `brightnessNorm` / `rainFadeRateNorm` / `snowFadeRateNorm` (float 0..1; explicit unit-range aliases for existing reactive drivers)
    - `rendererAuditionReactive.physicsVelocityNorm` / `physicsCollisionNorm` / `physicsDensityNorm` / `physicsCouplingNorm` (float 0..1; explicit unit-range aliases for physics-coupled drivers)
    - `rendererAuditionReactive.headphoneOutputRmsNorm` / `headphoneOutputPeakNorm` (float 0..1; explicit unit mirrors of headphone output envelopes)
    - `rendererAuditionReactive.headphoneParityNorm` (float 0..1; explicit unit mirror of `headphoneParity`)
    - `rendererAuditionReactive.sourceEnergyNorm[]` (array<float> 0..1, length 0..8; explicit unit-range alias of `sourceEnergy[]`)
    - Coupling source contract: physics-coupled fields are derived from renderer-thread physics summaries (active physics emitters only) and are clamped before audition DSP mapping; no lock/heap/IO is permitted on this path.
    - Fallback behavior: when internal audition is not the active renderer source for the block, scalar drive fields and normalized fields must publish neutral-safe values (`0` for envelopes/drivers/parity scalars), and `sourceEnergy`/`sourceEnergyNorm` must publish as empty arrays.
  - Spatial profile diagnostics:
    - `rendererSpatialProfileRequested` (string enum; APVTS `rend_spatial_profile`)
    - `rendererSpatialProfileActive` (string enum)
    - `rendererSpatialProfileStage` (string enum: `direct`, `fallback_stereo`, `fallback_quad`, `ambi_decode_stereo`, `codec_layout_placeholder`)
  - Spatial output matrix diagnostics (BL-028 Slice C1, additive):
    - `rendererMatrixRequestedDomain` (string enum: `InternalBinaural`, `Multichannel`, `ExternalSpatial`)
    - `rendererMatrixActiveDomain` (string enum; same domain set as requested)
    - `rendererMatrixRequestedLayout` (string enum: `stereo_2_0`, `quad_4_0`, `surround_5_1`, `surround_7_1`, `immersive_7_4_2`)
    - `rendererMatrixActiveLayout` (string enum; same layout set as requested)
    - `rendererMatrixRuleId` (string enum; current values include `SOM-028-01`..`SOM-028-11`)
    - `rendererMatrixRuleState` (string enum: `allowed`, `blocked`)
    - `rendererMatrixReasonCode` (string enum: `ok`, `binaural_requires_stereo`, `multichannel_requires_min_4ch`, `headtracking_not_supported_in_multichannel`, `external_spatial_requires_multichannel_bed`, `fallback_derived_from_layout`, `fallback_safe_stereo_passthrough`)
    - `rendererMatrixFallbackMode` (string enum: `none`, `retain_last_legal`, `derive_from_host_layout`, `safe_stereo_passthrough`)
    - `rendererMatrixFailSafeRoute` (string enum: `none`, `last_legal`, `layout_derived`, `stereo_passthrough`)
    - `rendererMatrixStatusText` (string; deterministic reason-code mapped user-visible text)
    - `rendererMatrixEventSeq` (uint64; monotonic matrix event sequence, currently aligned to snapshot sequence)
    - `rendererMatrix` (object; additive mirror for matrix surfaces that consume grouped payloads)
      - `requestedDomain`, `activeDomain`
      - `requestedLayout`, `activeLayout`
      - `ruleId`, `ruleState`
      - `fallbackMode`, `reasonCode`, `statusText`
      - Compatibility rule: `rendererMatrix.*` values must mirror the corresponding `rendererMatrix*` top-level fields in the same snapshot.
  - Ambisonic diagnostics (BL-018 planning gate):
    - `rendererAmbiCompiled` (bool)
    - `rendererAmbiActive` (bool)
    - `rendererAmbiMaxOrder` (integer)
    - `rendererAmbiNormalization` (string enum; current placeholder `sn3d`)
    - `rendererAmbiChannelOrder` (string enum; current placeholder `acn`)
    - `rendererAmbiDecodeLayout` (string enum; current placeholder `quad_baseline`)
    - `rendererAmbiStage` (string enum; current placeholder `not_implemented`)
  - CLAP runtime diagnostics (BL-011):
    - `clapBuildEnabled` (bool)
    - `clapPropertiesAvailable` (bool)
    - `clapIsPluginFormat` (bool)
    - `clapIsActive` (bool)
    - `clapIsProcessing` (bool)
    - `clapHasTransport` (bool)
    - `clapWrapperType` (string)
    - `clapLifecycleStage` (string enum: `not_compiled`, `compiled_no_properties`, `non_clap_instance`, `instantiated`, `active_idle`, `processing`)
    - `clapRuntimeMode` (string enum)
    - `clapVersion` (`major`, `minor`, `revision`)

Rules:

1. UI must render all active emitters from snapshot data each accepted tick; selection affects style, not inclusion.
2. Trails/vectors are visualization overlays controlled by `rend_viz_trails`, `rend_viz_vectors`, and `rend_viz_trail_len`; these controls must not mutate canonical snapshot state.
3. Listener/speaker overlays and energy response must consume snapshot telemetry; fallback defaults are allowed only for missing/invalid fields and must not break control-path interactivity.
4. RMS/energy fields must be finite and clamped to safe visual ranges before rendering.
5. Physics lens fields (`fx/fy/fz`, `collisionMask`, `collisionEnergy`) must be finite and clamped before rendering; UI must degrade safely if absent.
6. Contract changes to these payload fields require synchronized updates to `Documentation/implementation-traceability.md` and validation evidence logs.
7. When `rendererHeadphoneModeRequested == "steam_binaural"` and `rendererSteamAudioAvailable == false`, UI and tests must report `rendererSteamAudioInitStage` for deterministic failure triage.
8. Headphone profile diagnostics (`rendererHeadphoneProfileRequested`, `rendererHeadphoneProfileActive`) must be present in every renderer snapshot so BL-009 profile-lane checks can verify profile request/activation deterministically.
9. Spatial profile diagnostics must be present in every snapshot so QA can distinguish direct rendering from host-layout fallback behavior deterministically.
10. Ambisonic diagnostics must be present in every snapshot so BL-018 strict integration checks can distinguish placeholder telemetry (`not_implemented`) from active ambisonic paths.
11. CLAP diagnostics must be present in every snapshot (including non-CLAP runtime contexts) so self-tests can deterministically distinguish `not_compiled`, `non_clap_instance`, and active CLAP lifecycle states.
12. `rendererAuditionCloud` is additive and backward-compatible; UI consumers must safely ignore unknown fields (including `mode`, `emitterCount`, and `emitters`) and fall back to existing `rendererAuditionVisual` behavior if cloud metadata is absent.
13. BL-029 audition authority fields are additive and backward-compatible; when absent, UI consumers must preserve legacy `rendererAuditionVisual` + `rendererAuditionCloud` behavior with no hard dependency on these fields.
14. Resolver semantics are deterministic for identical snapshot inputs: requested mode precedence is `bound_physics` -> `bound_choreography` -> `bound_emitter` -> standalone (`single`/`cloud`).
15. `rendererAuditionReactive` is additive and backward-compatible; UI consumers that do not implement reactive fading must ignore the block and keep existing single/cloud visual behavior.
16. `rendererMatrix*` diagnostics and the additive `rendererMatrix` object are backward-compatible; consumers that do not implement BL-028 matrix surfaces must ignore these fields without altering existing renderer profile/headphone diagnostics behavior.
17. BL-033 calibration diagnostics are additive and backward-compatible; `rendererHeadphoneCalibration*` fields in scene snapshots and `headphoneCalibration*` fields in calibration status must resolve from the same published native snapshot cycle (`profileSyncSeq`) when both payloads are emitted in the same UI tick.
18. BL-034 profile governance diagnostics are additive and backward-compatible; when `rendererHeadphoneProfileCatalogVersion`, `rendererHeadphoneProfileFallbackReason`, `rendererHeadphoneProfileFallbackTarget`, `rendererHeadphoneProfileCustomSofaRef`, or `rendererHeadphoneProfileGovernance` are absent, consumers must keep legacy BL-009/BL-033 behavior with no hard dependency on these fields.
19. `rendererHeadphoneProfileRequested` and `rendererHeadphoneProfileActive` are bounded to the profile catalog domain `{generic, airpods_pro_2, sony_wh1000xm5, custom_sofa}`; unknown values must be treated as `generic` by deterministic fallback logic.
20. `rendererHeadphoneProfileCustomSofaRef` is bounded to length `0..256` and pattern `[A-Za-z0-9._:/-]*`; it must be non-empty only when requested or active profile is `custom_sofa`, otherwise it must publish as an empty string.
21. `rendererHeadphoneProfileFallbackReason` and `rendererHeadphoneProfileFallbackTarget` must publish deterministically as an ordered pair in every snapshot that includes BL-034 profile governance fields; `fallbackReason == "none"` requires `fallbackTarget == "none"`.

### BL-034 Profile Governance Contract (Slice A1)

#### Canonical Profile Catalog (Normative)

| Profile ID | Class | Source | `requiresCustomSofaRef` |
|---|---|---|---|
| `generic` | `built_in_reference` | bundled | `false` |
| `airpods_pro_2` | `built_in_reference` | bundled | `false` |
| `sony_wh1000xm5` | `built_in_reference` | bundled | `false` |
| `custom_sofa` | `external_reference` | user reference | `true` |

#### Deterministic Fallback Taxonomy (Normative)

| Reason Code | Fallback Target | Class | Deterministic Trigger |
|---|---|---|---|
| `none` | `none` | `no_fallback` | Requested profile resolved without downgrade. |
| `requested_profile_unavailable` | `generic` | `profile_resolution` | Requested profile ID absent from catalog domain. |
| `requested_profile_invalid` | `generic` | `profile_validation` | Requested profile token malformed/outside enum domain. |
| `custom_sofa_ref_missing` | `generic` | `external_reference` | `custom_sofa` requested/active with empty ref token. |
| `custom_sofa_ref_invalid` | `generic` | `external_reference` | `custom_sofa` ref token fails bounded validation. |
| `steam_unavailable` | `generic` | `runtime_capability` | Runtime cannot host requested Steam-dependent route. |
| `output_incompatible` | `generic` | `routing_capability` | Output topology incompatible with requested profile route. |
| `monitoring_path_bypassed` | `generic` | `runtime_resolution` | Monitoring path downgraded by resolver policy. |
| `catalog_version_mismatch` | `generic` | `contract_version` | Published catalog version incompatible with expected contract version. |

#### Acceptance Hooks (BL-034 Slice A1)

| Acceptance ID | Required Evidence |
|---|---|
| `BL034-A1-AC-001` | Catalog identities table present in this section and BL-034 runbook |
| `BL034-A1-AC-002` | Fallback taxonomy table present in this section and `fallback_taxonomy.tsv` |
| `BL034-A1-AC-003` | Additive publication + bounded-domain rules (`18..21`) present |
| `BL034-A1-AC-004` | Downstream machine-checkable artifact schema documented in BL-034 runbook |
| `BL034-A1-AC-005` | `./scripts/validate-docs-freshness.sh` passes for this slice |

### Audition Resolver Examples

Bound emitter resolved:

```json
{
  "rendererAuditionSourceMode": "single",
  "rendererAuditionRequestedMode": "bound_emitter",
  "rendererAuditionResolvedMode": "bound_emitter",
  "rendererAuditionBindingTarget": "emitter:3",
  "rendererAuditionBindingAvailable": true,
  "rendererAuditionFallbackReason": "none"
}
```

Bound physics fallback to standalone:

```json
{
  "rendererAuditionSourceMode": "cloud",
  "rendererAuditionRequestedMode": "bound_physics",
  "rendererAuditionResolvedMode": "cloud",
  "rendererAuditionBindingTarget": "none",
  "rendererAuditionBindingAvailable": false,
  "rendererAuditionFallbackReason": "bound_physics_unavailable"
}
```

## Determinism Contract

For identical:

- input audio
- parameter timeline
- transport/time source
- physics configuration and seed state

output behavior must be reproducible within expected floating-point tolerance.

## Design Integration Contract (`skill_design`)

1. Persistent viewport is required across Calibrate/Emitter/Renderer.
2. Mode switching changes overlays and controls, not scene continuity.
3. Draft/Final visual cues must communicate quality tier without implying different semantic scene state.

## Implementation Integration Contract (`skill_impl`)

1. Maintain one-to-one parameter coverage and traceability updates.
2. Preserve thread-domain boundaries and RT-safety constraints.
3. Treat acceptance evidence logging as mandatory before marking phase completion.

## Validation Hooks

Required evidence classes:

1. smoke/runtime stability
2. full-system CPU/deadline behavior
3. host edge-case lifecycle behavior
4. state/traceability doc synchronization

## Related

- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- `Documentation/implementation-traceability.md`
- `Documentation/lessons-learned.md`
