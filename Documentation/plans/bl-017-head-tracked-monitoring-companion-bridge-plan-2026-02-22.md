Title: BL-017 Head-Tracked Headphone Monitoring Companion Bridge Plan
Document Type: Planning
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-25

# BL-017 Head-Tracked Headphone Monitoring Companion Bridge Plan

## Objective

Investigate and define an implementation-ready path for head-tracked headphone monitoring using a companion app bridge, without coupling plugin `processBlock()` to app-level APIs or IPC.

## Decision Summary

1. Keep PHASE out of plugin DSP/runtime (`R5` guardrail remains valid).
2. Use a companion app as the head-tracking source of truth.
3. Bridge head pose to plugin via local IPC with sequence and timestamp contracts.
4. Consume bridge data in plugin via lock-free snapshots only; no network or blocking work on audio thread.

## Why This Architecture

1. PHASE is an app-level rendering framework and not a plugin-hosted real-time callback surface for VST3/AU `processBlock()` usage.
2. `CMHeadphoneMotionManager` can provide headphone motion updates and is the correct source for Apple head-tracking data in a companion app.
3. VST3 real-time processing guidance prohibits filesystem/network/UI operations in process callbacks; IPC and app integration must stay off the audio thread.

## Scope

In scope:
- Companion app bridge architecture.
- Plugin-side data contract and runtime integration plan.
- QA contract (automated synthetic lane + manual AirPods listening lane).

Out of scope:
- Full PHASE render pipeline inside plugin.
- Personalized HRTF/sofa personalization engine.
- Cross-platform head-tracking beyond Apple stack for this slice.

## Source Inputs

- `Documentation/backlog-post-v1-agentic-sprints.md` (`BL-017`)
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-2026-02-20.md` (Section 0b)
- `Documentation/invariants.md` (canonical PHASE runtime exclusion invariant)
- `Documentation/archive/2026-02-25-research-legacy/section0-integration-recommendations-2026-02-20.md` (`R5`, historical source)
- `Documentation/scene-state-contract.md`
- `Documentation/invariants.md`

External references:
- Apple PHASE: https://developer.apple.com/documentation/phase
- Apple CMHeadphoneMotionManager: https://developer.apple.com/documentation/coremotion/cmheadphonemotionmanager
- WWDC24 "Create custom audio experiences with AirPods" (head tracking/session behavior): https://developer.apple.com/videos/play/wwdc2024/10111/
- Steinberg VST3 real-time processing FAQ: https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Processing.html
- JUCE `AudioProcessor::processBlock` reference: https://docs.juce.com/master/classAudioProcessor.html

## Proposed System Architecture

```text
AirPods Motion Sensors
  -> CMHeadphoneMotionManager (Companion App, non-plugin process)
  -> Head Pose Packetizer (seq, timestamp, quaternion, yaw/pitch/roll)
  -> Local IPC (localhost UDP primary; optional loopback WebSocket debug path)
  -> Plugin Bridge Ingress Thread (message/background thread only)
  -> Lock-free Pose Snapshot (atomic seq + struct)
  -> SpatialRenderer pose application in processBlock (read-only snapshot)
```

## Runtime Contracts

### Companion -> Plugin pose packet

Minimum payload fields:
- `seq` (uint64, monotonic)
- `timestampMs` (uint64, monotonic clock)
- `quatW`, `quatX`, `quatY`, `quatZ` (float)
- `yawDeg`, `pitchDeg`, `rollDeg` (float, optional convenience)
- `trackingState` (`ok`, `stale`, `unavailable`)

Safety rules:
1. Plugin discards packets with `seq <= lastSeq`.
2. Plugin clamps non-finite values and normalizes quaternion.
3. Plugin marks pose stale if no packet arrives within timeout budget (default 250 ms).
4. Audio thread reads latest valid pose snapshot only (no locks).

### Plugin diagnostics (scene-state additions)

Planned scene-state fields:
- `rendererHeadTrackingEnabled` (bool)
- `rendererHeadTrackingSource` (`off`, `companion_udp`, `companion_ws`)
- `rendererHeadTrackingSeq` (int)
- `rendererHeadTrackingAgeMs` (int)
- `rendererHeadTrackingState` (`ok`, `stale`, `unavailable`, `invalid_packet`)
- `rendererHeadTrackingYawDeg`, `rendererHeadTrackingPitchDeg`, `rendererHeadTrackingRollDeg` (float)

## DSP/Application Integration Plan

### Slice A: bridge skeleton

Files (planned):
- `Source/HeadTrackingBridge.h` (non-RT ingress + lock-free snapshot)
- `Source/PluginProcessor.h/.cpp` (lifecycle and scene diagnostics wiring)
- `Source/SpatialRenderer.h` (optional listener orientation apply hook)

Rules:
1. Bridge socket/thread starts/stops in `prepareToPlay` and `releaseResources` or editor lifecycle helpers, never in audio callback.
2. No dynamic memory or string parsing in `processBlock`.
3. Pose application is deterministic for identical packet sequences.

### Slice B: parameter/UI wiring

Planned parameter additions:
- `rend_headtrack_enable` (bool)
- `rend_headtrack_mix` (0..1)
- `rend_headtrack_yaw_offset` (-180..180)
- `rend_headtrack_source` (choice: `companion_udp`, `companion_ws`)

WebView updates:
- Renderer rail controls for enable/source/mix/offset.
- Status text showing source/state/age.

### Slice C: spatial application strategy

1. Interpret head pose as inverse listener rotation for binaural/headphone monitoring modes only.
2. Apply orientation after scene accumulation but before final stereo/binaural output selection.
3. Disable pose influence for mono or non-headphone multichannel beds unless explicitly enabled in future scope.

## QA and Validation Plan

### Automated lane (deterministic)

New script (planned):
- `scripts/qa-bl017-headtracking-bridge-contract-mac.sh`

Checks:
1. Synthetic packet replay (`seq` monotonic) updates diagnostics deterministically.
2. Stale timeout transition (`ok` -> `stale`) occurs at configured threshold.
3. Invalid packet handling is safe (`invalid_packet`, no crash/non-finite audio).
4. Audio output determinism holds for fixed input + fixed synthetic pose stream.

### Manual lane (hardware)

1. AirPods Pro 2 connected.
2. Companion app streaming pose.
3. Reaper session with LocusQ in renderer/headphone mode.
4. Verify audible head-locked scene response and fallback on tracking loss.
5. Capture checklist row in `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` or new BL-017 evidence sheet.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Companion app not running | Head tracking unavailable | Deterministic fallback to static orientation and explicit scene diagnostics |
| Packet jitter/drop | Audible instability | Pose smoothing + stale timeout + sequence guard |
| Host/plugin lifecycle races | UI or bridge instability | Start/stop bridge on clear lifecycle boundaries and expose state telemetry |
| RT regression | Audio glitches | Zero blocking/no IPC/no allocation on audio thread; dedicated QA lane |

## Backlog Exit Criteria (BL-017)

1. Architecture doc accepted and linked from backlog.
2. Bridge contract fields added to `Documentation/scene-state-contract.md`.
3. Minimal bridge skeleton implemented with deterministic synthetic test lane.
4. Manual AirPods validation run captured with pass/fail evidence.
5. Backlog row moved `In Planning` -> `In Validation` -> `Done`.

## Implementation Recommendation

Start BL-017 with Slice A only, behind a feature flag:
- `LOCUSQ_ENABLE_HEADTRACK_BRIDGE=ON/OFF`

This de-risks host stability while enabling deterministic contract testing before full UI and perceptual tuning.
