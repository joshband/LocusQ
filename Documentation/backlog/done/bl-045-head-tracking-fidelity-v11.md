Title: BL-045 Head Tracking Fidelity v1.1
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-28

# BL-045 Head Tracking Fidelity v1.1

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-045 |
| Priority | P1 |
| Status | Done |
| Track | E - R&D Expansion |
| Effort | High / L |
| Depends On | BL-017 (Done), BL-034 (Done) |
| Blocks | BL-046, BL-047 |
| Annex Spec | `Documentation/plans/bl-045-head-tracking-fidelity-v11-spec-2026-02-26.md` |

## Objective

Improve head-tracking perceptual quality by adding interpolation, prediction, re-center workflow,
and sensor-switch handling for stable binaural motion cues.

## Scope

In scope:
- Pose interpolation (slerp) between snapshots.
- Optional short-horizon pose prediction using angular velocity.
- Yaw re-center UX (`Set Forward`) and drift indicator.
- Earbud sensor-location switch smoothing.
- Fix pre-existing v1 packet size constant discrepancy in bridge (`packetSizeBytes = 40` vs
  companion's actual 36-byte v1 packet; resolved as part of v2 version bump in Slice A).

Out of scope:
- Multi-listener architecture.
- ML personalization.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Companion + bridge payload extension: v2 packet (52 bytes), angV + sensorLocationFlags fields, v1 size fix | Companion builds; bridge accepts v2 + graceful v1 fallback; size assert 48B passes |
| B | `HeadPoseInterpolator` — slerp, bounded prediction, sensor-switch crossfade | Jitter < 1.5ms mean; discontinuity < 2° RMS at sensor switch; no NaN/Inf on extreme input |
| C | UI re-center (`Set Forward`) + drift telemetry + QA lane | Re-center deterministic in selftest; drift reported at 500ms ± 20ms; state NOT persisted |

## Architecture Summary

Full design in annex spec. Key decisions:

**v2 Packet (52 bytes):** `magic(4) + version=2(4) + qxyzw(16) + timestampMs(8) + seq(4) +
angVxyz(12) + sensorLocationFlags(4)`. Bridge performs versioned dispatch: v1 accepted at ≥36
bytes (fixes existing off-by-4), v2 at ≥52 bytes.

**`HeadTrackingPoseSnapshot`:** Extended from 32→48 bytes. `SpatialRenderer::PoseSnapshot`
extended in the same changeset.

**`HeadPoseInterpolator`:** Header-only, allocation-free, audio-thread safe. Holds a 2-slot
snapshot ring. Exposes `ingest(snapshot, nowMs)` and `interpolatedAt(nowMs)`.

**Re-center:** Yaw reference stored as atomic float (session-transient). Not persisted to XML.

## TODOs

- [x] Architecture and annex spec complete (`Documentation/plans/bl-045-head-tracking-fidelity-v11-spec-2026-02-26.md`).
- [x] Slice A: Extend `MotionSample` and `PosePacket` in companion; bump to v2 (52 bytes).
- [x] Slice A: Extend `HeadTrackingPoseSnapshot` to 48B; update `decodePacket` for v1/v2 dispatch; fix v1 size constant.
- [x] Slice A: Extend `SpatialRenderer::PoseSnapshot` to 48B in same changeset.
- [x] Slice B: Implement `Source/HeadPoseInterpolator.h` (slerp + bounded prediction + sensor-switch smoothing).
- [x] Slice B: Wire `HeadPoseInterpolator` into `PluginProcessor` processBlock head-tracking path.
- [x] Slice C: Add `Set Forward` command path and `yawReferenceDeg` atomic to `PluginProcessor`.
- [x] Slice C: Add drift telemetry timer (500ms, non-audio-thread) publishing `headTrackDrift` JSON.
- [x] Slice C: Add `Set Forward` button + drift display to `Source/ui/public/js/index.js` and `index.html`.
- [x] Create `Documentation/testing/bl-045-headtracking-fidelity-qa.md`.
- [x] Implement `scripts/qa-bl045-headtracking-fidelity-lane-mac.sh` (Slice C checks PASS 2026-02-27).

## Agent Mega-Prompt (Slice A)

```
Load skill_impl. Work on BL-045 Slice A (companion + bridge payload extension).

Annex spec: Documentation/plans/bl-045-head-tracking-fidelity-v11-spec-2026-02-26.md

Files to change:
1. companion/Sources/LocusQHeadTrackerCore/MotionService.swift
   — Extend MotionSample with angVx, angVy, angVz (Float), sensorLocation (UInt8), hasRotationRate (Bool).
   — HeadphoneMotionService: read motion.rotationRate.x/y/z. For sensorLocation, use
     #available(macOS 12, iOS 15, *) guard around motion.sensorLocation; emit 0 when unavailable.
   — Set hasRotationRate = true unconditionally when CoreMotion motion object is available.

2. companion/Sources/LocusQHeadTrackerCore/PosePacket.swift
   — Bump static let version: UInt32 = 2 and static let encodedSize = 52.
   — Add fields: angVx, angVy, angVz: Float; sensorLocationFlags: UInt32.
   — sensorLocationFlags = (UInt32(sensorLocation) & 0x3) | (hasRotationRate ? 0x4 : 0x0).
   — Serialize new fields LE after seq.

3. companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift
   — Pass sample.angVx/angVy/angVz/sensorLocation/hasRotationRate to PosePacket init.

4. Source/HeadTrackingBridge.h
   — Extend HeadTrackingPoseSnapshot from 32→48 bytes (add angVx, angVy, angVz: float, sensorLocationFlags: uint32_t).
   — Update static_assert to == 48.
   — In SharedCore: fix packetSizeBytes to 36 (v1); add packetSizeV2Bytes = 52; grow receive buffer to 64B.
   — decodePacket: if version==1 && numBytes>=36: decode v1 fields, zero angV/sensorLocationFlags.
     if version==2 && numBytes>=52: decode all fields.
     else: return false.

5. Source/SpatialRenderer.h — extend PoseSnapshot from 32→48 bytes to match. Update static_assert.

Validate:
  cd companion && swift build -c release
  cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j8
  ./scripts/standalone-ui-selftest-production-p0-mac.sh --runs 3
  ./scripts/rt-safety-allowlist.txt (verify no new non-allowlisted symbols)
```

## Agent Mega-Prompt (Slice B)

```
Load skill_impl. Work on BL-045 Slice B (HeadPoseInterpolator).
Prerequisite: Slice A complete.

Annex spec: Documentation/plans/bl-045-head-tracking-fidelity-v11-spec-2026-02-26.md

Create Source/HeadPoseInterpolator.h:
  - Header-only. No dynamic allocation.
  - Holds prevSnapshot, currSnapshot (HeadTrackingPoseSnapshot), hasPrev (bool),
    prevSensorLocation (uint8_t), sensorSwitchBlendRemaining (float),
    blendOutSnapshot (HeadTrackingPoseSnapshot).
  - ingest(const HeadTrackingPoseSnapshot& snap, float nowMs):
      if hasPrev && snap.sensorLocationFlags & 0x3 != prevSensorLocation:
          blendOutSnapshot = cached interpolated at nowMs
          sensorSwitchBlendRemaining = 50.0f
      rotate ring: prev = curr; curr = snap; hasPrev = true
      prevSensorLocation = snap.sensorLocationFlags & 0x3
  - interpolatedAt(float nowMs) const noexcept:
      t = clamp((nowMs - prev.timestampMs) / (curr.timestampMs - prev.timestampMs), 0, 1)
      raw = slerp(prev quaternion, curr quaternion, t)
      if curr.sensorLocationFlags & 0x4 (hasRotationRate) && nowMs > curr.timestampMs + 1.0f:
          dt = min((nowMs - curr.timestampMs) / 1000.0f, maxPredHorizon)
          maxPredHorizon = min(0.05f, (π/4) / max(||angV||, 1e-6f))
          apply small-angle extrapolation: q_pred = normalize(raw * [1, angVx*dt/2, angVy*dt/2, angVz*dt/2])
      if sensorSwitchBlendRemaining > 0:
          alpha = 1 - sensorSwitchBlendRemaining / 50.0f
          result = slerp(blendOut, result, alpha)
          sensorSwitchBlendRemaining = max(0, sensorSwitchBlendRemaining - blockDt)

Integrate in PluginProcessor.h: add HeadPoseInterpolator headPoseInterpolator member.
Integrate in PluginProcessor.cpp processBlock: call headPoseInterpolator.ingest(*pose, nowMs)
  then use headPoseInterpolator.interpolatedAt(nowMs) instead of raw bridge pose.

Validate:
  cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j8
  ./scripts/qa-bl045-headtracking-fidelity-lane-mac.sh --slice B --runs 5
  ./scripts/rt-safety-allowlist.txt
```

## Agent Mega-Prompt (Slice C)

```
Load skill_impl. Work on BL-045 Slice C (re-center UX + drift telemetry + QA lane).
Prerequisites: Slices A and B complete.

Annex spec: Documentation/plans/bl-045-head-tracking-fidelity-v11-spec-2026-02-26.md

PluginProcessor.h:
  - Add: std::atomic<float> yawReferenceDeg { 0.0f };
  - Add: bool yawReferenceSet = false;
  - Add: juce::Timer-derived inner class or DriftTelemetryTimer member (500ms, calls back on message thread).

PluginProcessor.cpp:
  - handleWebUIMessage: add case "setForwardYaw":
      yawReferenceDeg.store(currentListenerYawDeg, std::memory_order_relaxed);
      yawReferenceSet = true;
  - effectiveYaw calculation: subtract yawReferenceDeg before feeding to renderer.
  - Drift timer tick: compute abs(currentYaw - yawReferenceDeg.load()); send JSON
    {"type":"headTrackDrift","driftDeg":<v>,"referenceSet":<bool>} to WebView.

Source/ui/public/js/index.js:
  - Add "Set Forward" button in renderer panel (enable only when bridgeEnabled && !poseStale).
  - On click: sendMessage({cmd:"setForwardYaw"}).
  - On headTrackDrift message: update drift display "Drift: X.X°".

Source/ui/public/index.html:
  - Add re-center control strip layout: [ Set Forward ] Drift: 0.0°

Create Documentation/testing/bl-045-headtracking-fidelity-qa.md (use BL-034 QA doc as template).

Finalize scripts/qa-bl045-headtracking-fidelity-lane-mac.sh (fill in actual test logic).

Validate:
  cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j8
  ./scripts/qa-bl045-headtracking-fidelity-lane-mac.sh --runs 5
  ./scripts/validate-docs-freshness.sh
```

## Validation Plan

```bash
# Companion build
cd companion && swift build -c release

# Plugin build
cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j8

# BL-045 QA lane (Slice A)
./scripts/qa-bl045-headtracking-fidelity-lane-mac.sh --slice A --runs 5 \
  --out-dir TestEvidence/bl045_sliceA_<timestamp>

# BL-045 QA lane (Slice B)
./scripts/qa-bl045-headtracking-fidelity-lane-mac.sh --slice B --runs 5 \
  --out-dir TestEvidence/bl045_sliceB_<timestamp>

# BL-045 QA lane (all slices)
./scripts/qa-bl045-headtracking-fidelity-lane-mac.sh --runs 5 \
  --out-dir TestEvidence/bl045_<timestamp>

# Docs freshness
./scripts/validate-docs-freshness.sh
```

## Evidence Contract

- `status.tsv`
- `companion_build.log`
- `build.log`
- `headtracking_latency.tsv`
- `recenter_drift_metrics.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-045-head-tracking-fidelity-v11.md` |

### Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | Legacy closeout documented | `Evidence References` |
| Lane replay/parity | Legacy closeout documented | `Evidence References` |
| RT safety | Legacy closeout documented | `Evidence References` |
| Docs freshness | Legacy closeout documented | `Evidence References` |
| Status schema | Legacy closeout documented | `Evidence References` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | Required for modern promotions; legacy packets may predate marker | `Evidence References` |

### Backlog/Status Sync Checklist

- [x] Runbook archived under `Documentation/backlog/done/`
- [x] Backlog index links the done runbook
- [x] Historical evidence references retained
- [ ] Legacy packet retrofitted to modern owner packet template (`_template-promotion-decision.md`) where needed
- [ ] Legacy closeout fully normalized to modern checklist fields where needed
