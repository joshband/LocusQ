Title: BL-076 SpatialRenderer Decomposition Planning Packet
Document Type: Planning Packet
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# BL-076 SpatialRenderer Decomposition Planning Packet

## Scope Baseline

- Target file: `Source/SpatialRenderer.h`
- Total lines: `4837`
- Problem statement: the file is a multipurpose monolith spanning renderer contracts, DSP orchestration, audition synthesis, runtime backends, and output routing.

## Section Inventory (Line Ranges)

- `64-262`: public enums/contracts/snapshots (`HeadphoneRenderMode`, `SpatialOutputProfile`, pose + telemetry snapshots)
- `264-472`: lifecycle (`prepare`, `reset`, `shutdown`)
- `478-837`: runtime setter/config surface
- `838-1137`: diagnostics getters + `*ToString` mappers
- `1151-1271`: monitoring binaural path (`renderVirtualSurroundForMonitoring`)
- `1276-1921`: main `process` render path
- `1923-2039`: guardrail/audition telemetry getters
- `2042-2396`: private core state + DSP members
- `2397-2635`: pose/math/preset helpers
- `2641-4111`: audition synthesis + reactive telemetry engine
- `4113-4430`: Steam runtime init/teardown
- `4432-4836`: profile resolution + output writers + helper kernels

## Recommended Module Extractions

1. `SpatialRendererTypes`
- Responsibilities: enums, snapshots, string mappers, lightweight sanitizers.
- Key symbols: `HeadphoneRenderMode`, `SpatialOutputProfile`, `SpatialProfileStage`, `AmbisonicNormalization`, `CodecMappingMode`, `SteamInitStage`, `PoseSnapshot`, `AuditionReactiveSnapshot`, `*ToString`.

2. `SpatialEmitterRenderPass`
- Responsibilities: emitter selection and quad accumulation before routing.
- Key symbols: `process` emitter block, `calculateDistance`, `calculateAzimuth`, `calculateElevation`.

3. `SpatialPostFxChain`
- Responsibilities: room FX, per-speaker delay/trim, master gain smoothing.
- Key symbols: `setRoomEnabled`, `setRoomMix`, `setRoomSize`, `setRoomDamping`, `setEarlyReflectionsOnly`, `setSpeakerDelay`, `setSpeakerTrim`.

4. `HeadphonePoseAndCompensation`
- Responsibilities: pose transform + headphone virtualization + profile compensation.
- Key symbols: `applyHeadPose`, `updateHeadPoseOrientationFromSnapshot`, `rebuildHeadPoseSpeakerMix`, `renderStereoDownmixSample`, `renderVirtual3dStereoSample`, `updateHeadphoneCompensationForProfile`, `applyHeadphoneProfileCompensation`.

5. `SpatialAuditionEngine`
- Responsibilities: internal audition signal generation, physics-reactive timbre, telemetry publication.
- Key symbols: `generateAuditionSignalSample`, `renderAuditionVoiceExcitation`, `renderInternalAuditionEmitter`, `publishAuditionReactiveTelemetry`.

6. `SteamAudioRuntimeBackend`
- Responsibilities: runtime loading, lifecycle state machine, binaural backend rendering.
- Key symbols: `initialiseSteamAudioRuntimeIfEnabled`, `teardownSteamAudioRuntime`, `renderSteamBinauralBlock`, `setSteamInitStage`.

7. `SpatialProfileRouter`
- Responsibilities: output profile resolution, ambisonic proxy encode/decode, surround writers.
- Key symbols: `resolveSpatialProfileForHost`, `ambisonicOrderForProfile`, `encodeAmbisonicFoaProxyFromQuad`, `decodeAmbisonicFoaProxyToStereo`, `writeSurround521Sample`, `writeSurround721Sample`, `writeSurround742Sample`.

## Wave Plan

1. Wave 1 (low risk): extract `SpatialRendererTypes` without behavior changes.
2. Wave 2 (low risk): extract `SpatialProfileRouter` pure routing/writer helpers.
3. Wave 3 (low-medium): extract pose + compensation helpers (`HeadphonePoseAndCompensation`).
4. Wave 4 (medium): extract `SteamAudioRuntimeBackend` behind stable facade wiring.
5. Wave 5 (medium-high): extract `SpatialAuditionEngine` with explicit state/input structs.
6. Wave 6 (high): split monolithic `process` into staged orchestrator calls.

## Guardrails

- Dependency boundaries:
  - `SpatialAuditionEngine` must not include Steam runtime headers.
  - `SteamAudioRuntimeBackend` must not depend on `SceneGraph.h`.
  - `SpatialProfileRouter` must not depend on calibration chain or Steam runtime.
  - `SpatialRendererTypes` must avoid heavy DSP includes.
- Size goals:
  - `<=700` LOC per `.cpp`.
  - `<=250` LOC per `.h`.
  - no function over `150` LOC without a split plan.
- Stability:
  - preserve existing public symbol names and enum integer values used by `PluginProcessor` and bridge code.
- RT safety:
  - no new allocations/locks in audio-thread paths.

## Required Validation Lanes Per Wave

- `./scripts/qa-bl009-headphone-contract-mac.sh`
- `./scripts/qa-bl009-headphone-profile-contract-mac.sh`
- `./scripts/qa-bl018-ambisonic-contract-mac.sh`
- `./scripts/qa-bl018-profile-matrix-strict-mac.sh`
- `./scripts/qa-bl052-steam-audio-virtual-surround-mac.sh`
- `./scripts/qa-bl053-head-tracking-orientation-injection-mac.sh`
- `./scripts/qa-bl069-rt-safe-preset-pipeline-mac.sh`
- `./scripts/rt-safety-audit.sh`
