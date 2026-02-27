Title: BL-045 Head Tracking Fidelity v1.1 — Architecture Spec
Document Type: Annex Spec
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-045 Head Tracking Fidelity v1.1 — Architecture Spec

## Purpose

Concrete architecture for the three BL-045 implementation slices:
- **Slice A** — Companion + bridge payload extension (rotation rate, sensor location, v1 size fix)
- **Slice B** — Quaternion slerp interpolation, bounded angular-velocity prediction, sensor-switch smoothing
- **Slice C** — `Set Forward` re-center UX + drift telemetry + QA lane

Depends on: BL-017 (Done), BL-034 (Done).

---

## Baseline Audit

### v1 Packet Wire Format (Companion → Plugin)

Current companion (`PosePacket.swift`, `encodedSize = 36`):

```
Offset | Size | Field
0      | 4    | magic: 0x4C515054 ("LQPT")
4      | 4    | version: 1
8      | 4    | qx (float32 LE)
12     | 4    | qy (float32 LE)
16     | 4    | qz (float32 LE)
20     | 4    | qw (float32 LE)
24     | 8    | timestampMs (uint64 LE)
32     | 4    | seq (uint32 LE)
Total: 36 bytes
```

Current bridge (`HeadTrackingBridge.h`, `packetSizeBytes = 40`) checks `numBytes < 40`.
**Discrepancy:** companion sends 36 bytes; bridge rejects anything below 40. This is a pre-existing
v1 wire-format defect (bridge minimum size constant is 4 bytes high relative to the actual
v1 byte count). Slice A must repair this as part of the version bump.

### HeadTrackingPoseSnapshot (Plugin-Side Receive Buffer)

```cpp
// Current (32 bytes, assert enforced)
struct alignas(16) HeadTrackingPoseSnapshot {
    float qx = 0.0f;          // +0
    float qy = 0.0f;          // +4
    float qz = 0.0f;          // +8
    float qw = 1.0f;          // +12
    uint64_t timestampMs = 0; // +16
    uint32_t seq = 0;         // +24
    uint32_t pad = 0;         // +28
};                            // = 32 bytes
```

### SpatialRenderer PoseSnapshot (Internal Render Buffer)

```cpp
// Current (32 bytes, mirrors HeadTrackingPoseSnapshot)
struct alignas(16) PoseSnapshot {
    float qx = 0.0f; float qy = 0.0f; float qz = 0.0f; float qw = 1.0f;
    uint64_t timestampMs = 0;
    uint32_t seq = 0; uint32_t pad = 0;
};
```

---

## Slice A — Companion + Bridge Extension

### v2 Packet Wire Format

```
Offset | Size | Field
0      | 4    | magic: 0x4C515054 ("LQPT")
4      | 4    | version: 2
8      | 4    | qx (float32 LE)
12     | 4    | qy (float32 LE)
16     | 4    | qz (float32 LE)
20     | 4    | qw (float32 LE)
24     | 8    | timestampMs (uint64 LE, epoch ms)
32     | 4    | seq (uint32 LE)
36     | 4    | angVx (float32 LE, rad/s, CMDeviceMotion body frame)
40     | 4    | angVy (float32 LE, rad/s)
44     | 4    | angVz (float32 LE, rad/s)
48     | 4    | sensorLocationFlags (uint32 LE):
               bits [1:0] = sensorLocation (0=unknown, 1=left, 2=right)
               bit  [2]   = hasRotationRate (1 if angV fields are valid)
               bits [31:3] = reserved, must be zero
Total: 52 bytes
```

**Note on sensorLocation source**: `CMHeadphoneDeviceMotion.sensorLocation` is available in
CoreMotion on macOS 12+ / iOS 15+. Companion must guard with `#available(macOS 12, iOS 15, *)`.
When unavailable, emit `sensorLocation = 0` (unknown) and set `hasRotationRate` from
`motion.rotationRate` availability.

### Companion Changes (Swift)

**`MotionService.swift`** — extend `MotionSample`:
```swift
public struct MotionSample: Sendable, Equatable {
    public let qx, qy, qz, qw: Float
    public let timestampMs: UInt64
    public let angVx, angVy, angVz: Float       // rad/s; 0 if unavailable
    public let sensorLocation: UInt8            // 0/1/2
    public let hasRotationRate: Bool
}
```

`HeadphoneMotionService` reads `motion.rotationRate` (always available on supported motion
objects) and `motion.sensorLocation` (availability-guarded). Maps to `MotionSample` fields.

**`PosePacket.swift`** — v2:
```swift
public struct PosePacket: Sendable, Equatable {
    public static let magic: UInt32 = 0x4C515054
    public static let version: UInt32 = 2       // bumped from 1
    public static let encodedSize = 52           // bumped from 36
    // ... existing fields ...
    public let angVx, angVy, angVz: Float
    public let sensorLocationFlags: UInt32       // derived from sensorLocation + hasRotationRate
}
```

**`TrackerApp.swift`** — passes new `MotionSample` fields to `PosePacket`.

### Bridge Changes (C++)

**`HeadTrackingBridge.h`** — `HeadTrackingPoseSnapshot` extended to 48 bytes:
```cpp
struct alignas(16) HeadTrackingPoseSnapshot {
    float qx = 0.0f;           // +0
    float qy = 0.0f;           // +4
    float qz = 0.0f;           // +8
    float qw = 1.0f;           // +12
    uint64_t timestampMs = 0;  // +16
    uint32_t seq = 0;          // +24
    uint32_t pad = 0;          // +28  (keep for layout compat)
    float angVx = 0.0f;        // +32
    float angVy = 0.0f;        // +36
    float angVz = 0.0f;        // +40
    uint32_t sensorLocationFlags = 0; // +44
};                             // = 48 bytes
static_assert(sizeof(HeadTrackingPoseSnapshot) == 48, "HeadTrackingPoseSnapshot size contract");
```

**`decodePacket`** — versioned dispatch:
- `packetSizeV1 = 36` (fixing pre-existing off-by-4)
- `packetSizeV2 = 52`
- Accept `version == 1` if `numBytes >= 36`; set angV = 0, sensorLocationFlags = 0.
- Accept `version == 2` if `numBytes >= 52`; decode new fields.
- Reject all other versions.

**Receive buffer** updated to `std::array<uint8_t, 64>` (future-proof headroom).

### `SpatialRenderer::PoseSnapshot`

Extended in parallel to match `HeadTrackingPoseSnapshot` field layout.

### Slice A Exit Criteria

- `swift build -c release` in `companion/` passes with no warnings.
- `cmake --build build_local --target LocusQ_Standalone locusq_qa -j8` passes.
- QA lane reports `packet_v2_compat = PASS` (bridge accepts v2, bridges v1 gracefully).
- `static_assert(sizeof(HeadTrackingPoseSnapshot) == 48)` compiles.

---

## Slice B — Slerp, Prediction, Sensor-Switch Smoothing

### Design Constraints

- All interpolation logic runs on the audio thread (inside `processBlock`) or on the bridge's
  receive thread updating a lock-free snapshot. Must be allocation-free and branch-minimal.
- Prediction horizon is bounded to `kMaxPredictionMs = 50.0f` to prevent divergence.
- Sensor-switch smoothing blends over `kSensorSwitchBlendMs = 50.0f`.

### New File: `Source/HeadPoseInterpolator.h`

Header-only, no dynamic allocation, callable from audio thread.

```
State held:
  prevSnapshot: HeadTrackingPoseSnapshot   // t-1 received snapshot
  currSnapshot: HeadTrackingPoseSnapshot   // most recent received snapshot
  prevSensorLocation: uint8               // for switch detection
  sensorSwitchBlendRemaining: float        // ms remaining in crossfade
  hasPrev: bool

API:
  void ingest(const HeadTrackingPoseSnapshot& snap, float nowMs)
    -> Update prev/curr ring; detect sensor switch; reset/decrement blend.

  HeadTrackingPoseSnapshot interpolatedAt(float nowMs) const noexcept
    -> Returns slerp'd + predicted + sensor-smoothed snapshot.
```

### Quaternion Slerp

Standard shortest-path slerp:

```
dot = qPrev · qCurr
if (dot < 0) { qCurr = -qCurr; dot = -dot; }
if (dot > 0.9995f) { use nlerp (linear + normalize) }
else {
    theta0 = acos(dot)
    t = clamp((nowMs - prevTs) / (currTs - prevTs), 0, 1)
    result = sin((1-t)*theta0)/sin(theta0) * qPrev
           + sin(t*theta0)/sin(theta0) * qCurr
}
```

Fallback when `currTs == prevTs`: return `currSnapshot` as-is.

### Bounded Angular Velocity Prediction

Applies only when `hasRotationRate` flag is set and `nowMs > currTs + 1.0f`:

```
dt = clamp(nowMs - currTs, 0, kMaxPredictionMs) / 1000.0f  // seconds
// Apply small-angle rotation: q_pred = q_curr * delta_q
// where delta_q ≈ (1, angVx*dt/2, angVy*dt/2, angVz*dt/2) normalized
```

Cap condition: if `||angV|| * dt > π/4` (45°), clamp dt to π/4 / ||angV||.

### Sensor-Switch Smoothing

On sensor location change:
- `sensorSwitchBlendRemaining = kSensorSwitchBlendMs` (50 ms)
- Store `blendOutSnapshot = prevInterpolated`
During active blend:
- `alpha = 1.0f - (sensorSwitchBlendRemaining / kSensorSwitchBlendMs)`
- Result = slerp(blendOutSnapshot, rawInterpolated, alpha)

### Integration Point

`PluginProcessor.cpp` processBlock site (currently around line 1908):
```cpp
// Replace direct bridge access:
//   headTrackingBridge.currentPose()
// With interpolator:
//   headPoseInterpolator.ingest(*headTrackingBridge.currentPose(), nowMs);
//   auto interpolated = headPoseInterpolator.interpolatedAt(nowMs);
```

`HeadPoseInterpolator` is a value member of `LocusQAudioProcessor`.

### Slice B Exit Criteria

- Latency metric `headtracking_latency.tsv` shows mean-interpolation jitter < 1.5ms at 48kHz/512.
- Sensor-switch regression test: position discontinuity at switch < 2° RMS.
- Prediction clamp: bogus high-velocity input does not cause quaternion explosion.

---

## Slice C — Re-center UX + Drift Telemetry + QA Lane

### `Set Forward` Command Flow

```
UI button click
  → JS: sendMessage({cmd: "setForwardYaw"})
  → PluginProcessor::handleWebUIMessage (existing message router)
  → setYawReference(currentListenerYawRad)
    stores: float yawReferenceDeg (atomic, MPMAC safe)
```

All rendered yaw: `effectiveYaw = rawYaw - yawReferenceDeg` (wrap to [-180, 180]).

Yaw reference is **not** persisted to state XML (transient, session-scoped). Rationale: re-center
is a live performance action; restoring a stale reference on plugin reload would misalign to
current physical orientation.

### Drift Telemetry

A `juce::Timer`-derived helper (non-audio-thread, 500ms tick) publishes to WebView:
```json
{"type": "headTrackDrift", "driftDeg": <float>, "referenceSet": <bool>}
```
`driftDeg = abs(currentYawDeg - yawReferenceDeg)` at the tick instant.

### UI Layout (Renderer Panel)

```
[ Set Forward ]  Drift: 0.0°
```

Button is enabled only when `bridgeEnabled && !poseStale`. Disabled state shown with 50% opacity.

### New Files

- `scripts/qa-bl045-headtracking-fidelity-lane-mac.sh` — QA lane for all three slices.
- `Documentation/testing/bl-045-headtracking-fidelity-qa.md` — QA test doc.

### QA Lane Checks

| Check ID | Description | Threshold |
|---|---|---|
| BL045-A-001 | Companion v2 packet builds, sends 52-byte datagrams | build PASS |
| BL045-A-002 | Bridge accepts v2 packet, rejects v0/v3, gracefully handles v1 | 0 regressions |
| BL045-B-001 | Slerp interpolation jitter < 1.5ms mean at 512-sample block | mean < 1.5ms |
| BL045-B-002 | Sensor-switch crossfade: discontinuity < 2° RMS over 50ms window | < 2° |
| BL045-B-003 | Prediction clamp: extreme angular velocity does not diverge | no NaN/Inf |
| BL045-C-001 | Re-center command received → yaw snaps within 1 render frame | ≤ 1 frame |
| BL045-C-002 | Drift telemetry reported at 500ms interval ± 20ms | ≤ 20ms jitter |
| BL045-C-003 | Re-center state is NOT persisted across plugin reload | verified |

### Slice C Exit Criteria

- `recenter_drift_metrics.tsv` with all BL045-C-* checks PASS.
- No new RT-unsafe allocations in `HeadPoseInterpolator` under `rt-safety-allowlist.txt`.

---

## File Change Map

| Slice | File | Change |
|---|---|---|
| A | `companion/Sources/LocusQHeadTrackerCore/MotionService.swift` | Extend `MotionSample` with angV + sensorLocation fields |
| A | `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift` | Bump version=2, encodedSize=52, serialize new fields |
| A | `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift` | Pass new `MotionSample` fields to `PosePacket` |
| A | `Source/HeadTrackingBridge.h` | Extend `HeadTrackingPoseSnapshot` to 48B; versioned decode; fix v1 size constant |
| A | `Source/SpatialRenderer.h` | Extend `PoseSnapshot` to match |
| B | `Source/HeadPoseInterpolator.h` | New header-only slerp/prediction/smoothing class |
| B | `Source/PluginProcessor.h` | Add `HeadPoseInterpolator headPoseInterpolator` member |
| B | `Source/PluginProcessor.cpp` | Wire interpolator into processBlock head-tracking ingestion |
| C | `Source/PluginProcessor.h` | Add `yawReferenceDeg` atomic float; drift timer |
| C | `Source/PluginProcessor.cpp` | `setYawReference`, `handleWebUIMessage` extension, drift timer tick |
| C | `Source/ui/public/js/index.js` | `Set Forward` button, drift display |
| C | `Source/ui/public/index.html` | Layout for re-center control strip |
| A–C | `scripts/qa-bl045-headtracking-fidelity-lane-mac.sh` | New QA lane script |
| A–C | `Documentation/testing/bl-045-headtracking-fidelity-qa.md` | QA test doc |

---

## Risk Register

| Risk | Severity | Mitigation |
|---|---|---|
| `sensorLocation` API not available pre-macOS 12 | Medium | `#available` guard; emit unknown (0) gracefully |
| Bridge size-assert breakage if `SpatialRenderer::PoseSnapshot` not updated in sync | High | Update both in same changeset; CI build verifies both asserts |
| Slerp `acos` domain error when `dot > 1.0` due to float rounding | Low | Clamp dot to [-1, 1] before `acos`; fall back to nlerp for dot > 0.9995 |
| Prediction instability for very high angular rates (fast head motion) | Medium | Clamp prediction dt so rotation angle ≤ π/4 per frame |
| Re-center command arriving on wrong thread | Medium | Use JUCE `MessageManager::callAsync` or async queue; do not access atomic from audio thread without memory order |

---

## Complexity Assessment

**Score: 4 / 5** (Expert)

Rationale: Multi-layer change spanning Swift companion, C++ bridge protocol (versioned binary
packet), lock-free audio-thread data paths, quaternion mathematics, and WebView UX command
routing. The slerp/prediction path must be provably allocation-free and numerically stable.
