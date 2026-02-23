Title: LocusQ Scene State Contract
Document Type: Interface Contract
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-22

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
    - `rendererPhysicsLensEnabled` (bool)
    - `rendererPhysicsLensMix` (float 0..1)
    - `rendererSteamAudioCompiled` (bool)
    - `rendererSteamAudioAvailable` (bool)
    - `rendererSteamAudioInitStage` (string enum)
    - `rendererSteamAudioInitErrorCode` (integer)
    - `rendererSteamAudioRuntimeLib` (string)
    - `rendererSteamAudioMissingSymbol` (string)
  - Spatial profile diagnostics:
    - `rendererSpatialProfileRequested` (string enum; APVTS `rend_spatial_profile`)
    - `rendererSpatialProfileActive` (string enum)
    - `rendererSpatialProfileStage` (string enum: `direct`, `fallback_stereo`, `fallback_quad`, `ambi_decode_stereo`, `codec_layout_placeholder`)
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
