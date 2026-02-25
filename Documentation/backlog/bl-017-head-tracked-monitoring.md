---
Title: BL-017 Head-Tracked Monitoring Companion Bridge
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-25
---

# BL-017 Head-Tracked Monitoring Companion Bridge

## 1. Identity

| Field | Value |
|---|---|
| ID | BL-017 |
| Title | Head-Tracked Monitoring Companion Bridge |
| Status | In Implementation (Slices A-B owner-validated; Slice C pending) |
| Priority | P2 |
| Track | E — R&D Expansion |
| Effort | High / L |
| Depends On | BL-009 (Done), BL-018 (stable gate) |
| Blocks | BL-028 |
| Annex | `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md` |
| Slice C MVP design note | `Documentation/plans/bl-017-slice-c-companion-app-mvp-structure-2026-02-25.md` |
| ADRs | ADR-0006, ADR-0012 |
| Skills | `$skill_plan`, `$spatial-audio-engineering`, `$steam-audio-capi` |

---

## 2. Objective

Deliver a companion bridge for head-tracked headphone monitoring. The full signal path is:

```
CMHeadphoneMotionManager (iOS/macOS companion app)
        |
        v  UDP pose packet (quat + timestamp + seq)
Companion UDP sender (macOS companion app)
        |
        v  loopback / LAN UDP
HeadTrackingBridge (message thread, UDP recv)
        |
        v  atomic store — lock-free PoseSnapshot
PluginProcessor::processBlock (audio thread)
        |
        v  atomic load — reads PoseSnapshot
SpatialRenderer::applyHeadPose (Steam Audio HRTF orientation)
```

All RT constraints must be preserved: no allocation, no lock, no blocking inside `processBlock()`. Head-tracking integration is restricted to `InternalBinaural` renderer mode per ADR-0012.

---

## 3. Entry Criteria

| Gate | Condition |
|---|---|
| BL-009 | Merged and stable — SpatialRenderer binaural path exercised |
| BL-018 | Stable (bridge may land before BL-018 completes but must not regress it) |
| Feature flag | `LOCUS_HEAD_TRACKING` compile-time flag defined as `0` by default |
| No RT debt | processBlock() passes existing RT audit before slice work begins |

---

## 4. Slice Plan

### Slice A — Bridge Receiver + Lock-Free Pose Snapshot

**Scope:** New `Source/HeadTrackingBridge.h`, integration hook in `Source/PluginProcessor.cpp`. Entire slice is behind `LOCUS_HEAD_TRACKING` compile flag.

**Deliverables:**
- `Source/HeadTrackingBridge.h` — UDP listener on message thread, atomic `PoseSnapshot` storage
- `PluginProcessor.cpp` — instantiate bridge, wire audio-thread read

**Entry:** BL-009 done, BL-018 stable.

**Exit:** Unit test confirms atomic snapshot round-trip. processBlock() reads pose without any alloc/lock.

---

### Slice B — Pose Application in SpatialRenderer

**Scope:** `Source/SpatialRenderer.h` — consume the pose snapshot from the audio thread and apply to Steam Audio HRTF listener orientation.

**Deliverables:**
- `SpatialRenderer::applyHeadPose(const PoseSnapshot&)` method
- Steam Audio `IPLCoordinateSpace3` update from quaternion

**Entry:** Slice A merged and unit-tested.

**Exit:** HRTF orientation updates correctly in RENDERER + InternalBinaural mode under test tone.

---

### Slice C — Companion App MVP

**Scope:** Separate macOS-only project/directory (outside `Source/`). Uses `CMHeadphoneMotionManager` to read device orientation and transmit UDP pose packets to the plugin bridge.

**Deliverables:**
- Companion app skeleton directory (Swift/SwiftUI or bare Foundation CLI)
- UDP sender transmitting `{ magic, version, quat[x,y,z,w], timestamp_ms:uint64, seq:uint32 }` packets
- README documenting pairing flow

**Entry:** Slice B merged and validated.

**Exit:** Companion app running on macOS; plugin receives and applies pose; audible HRTF movement on head rotation.

---

## 5. Architecture Notes

### PoseSnapshot Struct

```cpp
// Source/HeadTrackingBridge.h
struct alignas(16) PoseSnapshot {
    float qx, qy, qz, qw;   // quaternion (device orientation)
    uint64_t timestamp_ms;   // sender wall-clock ms
    uint32_t seq;            // monotonic packet counter
    uint32_t _pad;           // alignment
};

static_assert(sizeof(PoseSnapshot) == 32, "PoseSnapshot size contract");
```

### Lock-Free Exchange Pattern

```cpp
// HeadTrackingBridge owns:
std::atomic<PoseSnapshot*> activePose_{ nullptr };
PoseSnapshot slots_[2];   // double-buffer, no alloc after init
std::atomic<int> writeSlot_{ 0 };

// Message thread (UDP recv) writes:
void onUdpPacket(const PoseSnapshot& p) {
    int w = writeSlot_.load(std::memory_order_relaxed);
    slots_[w] = p;
    activePose_.store(&slots_[w], std::memory_order_release);
    writeSlot_.store(w ^ 1, std::memory_order_relaxed);
}

// Audio thread reads (processBlock):
const PoseSnapshot* pose = bridge_.activePose_.load(std::memory_order_acquire);
if (pose) renderer_.applyHeadPose(*pose);
```

### UDP Packet Format (v1)

| Byte offset | Type | Field |
|---|---|---|
| 0 | uint32_t | magic = 0x4C515054 ("LQPT") |
| 4 | uint32_t | version = 1 |
| 8 | float[4] | qx, qy, qz, qw |
| 24 | uint64_t | timestamp_ms |
| 32 | uint32_t | seq |
| 36 | uint32_t | reserved |

Total: 40 bytes. Port default: 19765 (configurable via plugin init param).

### System Diagram

```
+-------------------------+     UDP / loopback      +-----------------------------+
|  Companion App (macOS)  | ----------------------> |  HeadTrackingBridge         |
|                         |    port 19765            |  (message thread, UDP recv) |
|  CMHeadphoneMotionMgr   |                          |                             |
|  -> quat + timestamp    |                          |  atomic PoseSnapshot store  |
+-------------------------+                          +-------------|---------------+
                                                                   | atomic load (acquire)
                                                     +-------------|---------------+
                                                     |  PluginProcessor            |
                                                     |  processBlock()             |
                                                     |  -> SpatialRenderer         |
                                                     |     applyHeadPose()         |
                                                     |  -> Steam Audio HRTF        |
                                                     +-----------------------------+
```

---

## 6. RT Invariant Checklist

| Check | Rule |
|---|---|
| No `new`/`delete` in processBlock | PoseSnapshot slots pre-allocated in HeadTrackingBridge ctor |
| No mutex/lock in processBlock | atomic load only (acquire) |
| No socket/IO in processBlock | UDP recv is message-thread only |
| No blocking call in processBlock | confirmed — bridge only writes atomically |
| Feature flag zero-cost when disabled | `#if LOCUS_HEAD_TRACKING` wraps all bridge code |

---

## 7. Risks

| Risk | Severity | Mitigation |
|---|---|---|
| UDP latency variation (> 20 ms jitter) | Med | Timestamp-based staleness check; reject packets older than threshold |
| Pose jitter causing audible HRTF artifacts | Med | Low-pass filter applied to quaternion on message thread before atomic store |
| Feature flag complexity increasing build matrix | Low | Single flag, CI matrix adds one row only |
| ADR-0012 violation if enabled in non-binaural modes | High | Guard in SpatialRenderer::applyHeadPose — no-op unless mode == InternalBinaural |
| Companion app pairing UX undefined | Low | Out-of-scope for Slice C MVP; document manual port config |

---

## 8. Validation Plan

| Step | Method | Pass Criteria |
|---|---|---|
| Slice A unit test | GoogleTest / Catch2 mock UDP packet -> atomic read | Pose round-trip correct, zero alloc in read path |
| Slice B integration | REAPER + test tone + head rotation simulation | Audible HRTF pan on pose change |
| RT audit | AddressSanitizer + custom alloc interceptor | Zero alloc/lock in processBlock during test |
| Slice C system test | Companion app on macOS + plugin in standalone | HRTF updates at >= 30 Hz, < 20 ms average latency |
| Regression | Existing BL-009 binaural test suite | All tests pass with flag disabled |

---

## 9. Files Touched

| File | Action |
|---|---|
| `Source/HeadTrackingBridge.h` | CREATE — Slice A |
| `Source/PluginProcessor.cpp` | MODIFY — Slice A bridge instantiation + audio-thread read |
| `Source/PluginProcessor.h` | MODIFY — Slice A member declaration |
| `Source/SpatialRenderer.h` | MODIFY — Slice B applyHeadPose() |
| `companion/` (new directory) | CREATE — Slice C |
| `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md` | REFERENCE (annex) |
| `status.json` | UPDATE per slice completion |

---

## 10. ADR References

| ADR | Relevance |
|---|---|
| ADR-0006 | RT invariant contract — no alloc/lock/blocking in processBlock |
| ADR-0012 | Head tracking is restricted to InternalBinaural renderer mode |

If any slice implementation must deviate from these ADRs, record a new ADR before closing the slice.

---

## 11. Agent Mega-Prompts

### Slice A — Skill-Aware Prompt

```
SKILLS: $skill_plan $spatial-audio-engineering $steam-audio-capi
BACKLOG: BL-017 Slice A
TASK: Implement HeadTrackingBridge.h and PluginProcessor.cpp integration

CONTEXT:
- LocusQ is a JUCE VST3/AU/CLAP spatial audio plugin.
- RT invariant (ADR-0006): no alloc, no lock, no blocking in processBlock().
- Head tracking is InternalBinaural-only (ADR-0012).
- BL-009 is done: SpatialRenderer binaural path is stable.
- Entire Slice A is behind compile flag LOCUS_HEAD_TRACKING (default 0).

OBJECTIVE:
Design and implement Source/HeadTrackingBridge.h with:
1. PoseSnapshot struct (alignas(16), fields: qx/qy/qz/qw float, timestamp_ms uint64,
   seq uint32, _pad uint32). Static assert size == 32.
2. HeadTrackingBridge class:
   - Constructor: opens UDP socket on configurable port (default 19765), binds to loopback.
   - Runs recv loop on juce::MessageManager thread (or a dedicated background thread
     using juce::Thread — prefer background thread to avoid blocking message thread).
   - On packet receipt: validates magic (0x4C515054) and version (1), deserializes PoseSnapshot,
     writes into double-buffer slots_[2] via writeSlot_ atomic index, stores pointer to
     activePose_ with memory_order_release.
   - Audio-thread API: const PoseSnapshot* currentPose() const noexcept — atomic load
     with memory_order_acquire.
3. Feature flag pattern: all HeadTrackingBridge code wrapped in #if LOCUS_HEAD_TRACKING.

INTEGRATION in PluginProcessor.cpp:
- In PluginProcessor constructor (or prepareToPlay): instantiate bridge under flag.
- In processBlock(): under flag, call bridge_.currentPose() (atomic load only),
  pass to renderer if non-null.

CONSTRAINTS:
- currentPose() must be wait-free and allocation-free.
- UDP recv must never be called from processBlock().
- No std::mutex or juce::CriticalSection in the audio-thread read path.

DELIVERABLES:
- Source/HeadTrackingBridge.h (complete, compiling)
- Diff of Source/PluginProcessor.cpp showing bridge member + processBlock wiring
- Unit test outline: mock UDP packet -> currentPose() reads correct quaternion

OUTPUT FORMAT:
1. HeadTrackingBridge.h full file content
2. PluginProcessor.cpp diff (relevant sections only)
3. Unit test pseudocode or GoogleTest sketch
4. RT invariant self-check table (column: check | satisfied | notes)
```

---

### Slice A — Standalone Fallback Prompt

```
CONTEXT (no skill files available):
You are implementing a head-tracking companion bridge for a JUCE audio plugin (LocusQ).
The plugin processes audio in processBlock() which is a real-time audio thread.

HARD CONSTRAINTS:
- processBlock() must NEVER allocate memory, take a mutex, or block.
- Head tracking is only active in InternalBinaural mode (guard this in the renderer).
- All bridge code is behind #if LOCUS_HEAD_TRACKING (default disabled).

SYSTEM ARCHITECTURE (from annex spec):
The companion bridge connects a macOS device running CMHeadphoneMotionManager
to the plugin via UDP. The full path is:

  [Companion App]
      CMHeadphoneMotionManager (device orientation as quaternion)
      -> UDP sender: packet {magic, version, qx, qy, qz, qw, timestamp_ms, seq}
      -> loopback UDP port 19765

  [Plugin — HeadTrackingBridge]
      Background thread: UDP recv loop
      -> validate magic + version
      -> deserialize PoseSnapshot
      -> atomic double-buffer write (store pointer with memory_order_release)

  [Plugin — processBlock (audio thread)]
      -> atomic load activePose_ (memory_order_acquire) — ZERO alloc/lock
      -> if pose non-null: pass to SpatialRenderer::applyHeadPose()

  [SpatialRenderer]
      -> convert quat to IPLCoordinateSpace3
      -> set Steam Audio HRTF listener orientation

WHAT TO IMPLEMENT NOW (Slice A):
1. PoseSnapshot struct — 32 bytes, alignas(16), static_assert enforced.
2. HeadTrackingBridge class — UDP recv on background thread, atomic double-buffer,
   wait-free currentPose() for audio thread.
3. PluginProcessor.cpp — bridge member (under flag), processBlock() read wiring.

WHAT NOT TO IMPLEMENT YET:
- SpatialRenderer changes (Slice B)
- Companion app (Slice C)

Please produce:
1. Full Source/HeadTrackingBridge.h
2. Key PluginProcessor.cpp additions
3. Self-check: confirm no mutex/alloc in audio-thread read path
```

---

### Slice B — Skill-Aware Prompt

```
SKILLS: $spatial-audio-engineering $steam-audio-capi
BACKLOG: BL-017 Slice B
TASK: Implement SpatialRenderer::applyHeadPose()

CONTEXT:
- Slice A is merged: HeadTrackingBridge.h provides const PoseSnapshot* currentPose().
- SpatialRenderer.h wraps Steam Audio (IPLContext, IPLHRTF, IPLBinauralEffect).
- Head tracking is InternalBinaural-only (ADR-0012).
- No alloc/lock in audio path.

OBJECTIVE:
Add applyHeadPose(const PoseSnapshot& pose) to SpatialRenderer:
1. Convert quaternion (qx,qy,qz,qw) to IPLCoordinateSpace3 (right/up/ahead vectors).
2. Call iplBinauralEffectSetListenerCoordinates or equivalent Steam Audio 4.x API to
   update listener HRTF orientation.
3. Guard: if currentRendererMode != InternalBinaural, return immediately.

DELIVERABLES:
- Diff of Source/SpatialRenderer.h showing applyHeadPose() implementation
- Quaternion-to-IPLCoordinateSpace3 conversion helper (show math)
- Mode guard implementation

OUTPUT FORMAT:
1. SpatialRenderer.h diff
2. Conversion math explanation (2-3 sentences + code)
3. Integration test description: test tone + simulated pose rotation -> audible HRTF movement
```

---

### Slice C — Skill-Aware Prompt

```
SKILLS: $skill_plan $spatial-audio-engineering
BACKLOG: BL-017 Slice C
TASK: Design companion app MVP structure

CONTEXT:
- Slices A and B are merged and validated.
- Companion app is macOS-only, separate from the plugin project.
- Uses CMHeadphoneMotionManager (requires AirPods or compatible device).
- Transmits UDP pose packets to HeadTrackingBridge on port 19765.

OBJECTIVE:
1. Propose companion app directory layout (Swift CLI or SwiftUI minimal app).
2. Show CMHeadphoneMotionManager setup code snippet (request motion access,
   start updates, receive attitude quaternion).
3. Show UDP sender snippet (Network framework or POSIX socket, loopback to 127.0.0.1:19765).
4. Define packet serialization matching the plugin's expected format:
   magic=0x4C515054, version=1, qx/qy/qz/qw float32, timestamp_ms uint64, seq uint32.
5. Outline README content for pairing flow.

DELIVERABLES:
- Proposed directory tree
- CMHeadphoneMotionManager snippet
- UDP sender snippet
- Packet serialization code
- Pairing flow README outline
```

---

## 12. Owner Validation Snapshot (2026-02-25)

| Slice | Status | Evidence |
|---|---|---|
| Slice A | Implemented and owner-validated | `TestEvidence/bl017_head_tracking_slice_a_20260224T190129Z/status.tsv` (implementation), `TestEvidence/bl017_slice_b_owner_replay_20260224T195259Z/rt_audit.tsv` (current-tree RT audit pass) |
| Slice B | Implemented and owner-validated | `TestEvidence/bl017_slice_b_20260224T194727Z/status.tsv`, `TestEvidence/bl017_slice_b_owner_replay_20260224T195259Z/status.tsv` |
| Slice C | Implemented (companion MVP) and partially validated | `companion/` Swift package + `swift test` packet contract check on current branch; hardware AirPods manual lane pending |

## 13. Closeout Checklist

- [x] Slice A: HeadTrackingBridge.h implemented, unit tested, RT audit clean
- [x] Slice A: PluginProcessor.cpp integration merged under feature flag
- [x] Slice B: SpatialRenderer::applyHeadPose() implemented, integration tested
- [x] Slice C: Companion app MVP in repository (system test documentation pending hardware lane)
- [ ] ADR-0006 and ADR-0012 compliance confirmed in code review
- [ ] `status.json` updated with BL-017 completion and BL-028 gate unblocked
- [ ] `TestEvidence/validation-trend.md` updated with slice validation results
- [ ] `TestEvidence/build-summary.md` updated with build and RT audit outcome
- [ ] Annex plan document reviewed and archived if superseded
