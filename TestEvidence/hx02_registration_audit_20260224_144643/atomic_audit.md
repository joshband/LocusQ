Title: HX-02 Slice A Atomic Ordering Audit
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# HX-02 Slice A - Atomic Ordering Audit

## Scope
Audited all `std::atomic` usage and atomic operations under `Source/`, including:
- `Source/SceneGraph.h`
- `Source/SharedPtrAtomicContract.h`
- `Source/PluginProcessor.cpp` (APVTS atomic parameter loads + CLAP diagnostics atomics)
- `Source/SpatialRenderer.h`
- `Source/CalibrationEngine.h`
- `Source/PhysicsEngine.h`
- `Source/VisualTokenScheduler.h`
- `Source/HeadTrackingBridge.h`

## Summary Verdict
- Overall status: **FAIL**
- High severity findings: 1
- Medium severity findings: 1
- Low severity findings: 1
- Compare-exchange operations found: 0
- Registration ABA risk in slot-claim path: **Not observed** (path is lock-serialized)

## Findings

### High
1. **Non-atomic slot occupancy read/write race in registration path**
   - `slotOccupied[]` writes are lock-protected, but reads happen without lock/atomic in `isSlotActive()`.
   - Concurrent reads in renderer/editor paths can race with registration updates.

### Medium
2. **Non-atomic renderer registration flag read/write race**
   - `rendererRegistered` writes are lock-protected, but `isRendererRegistered()` reads without lock/atomic.

### Low
3. **Relaxed multi-field diagnostics publication can expose mixed snapshots**
   - `SpatialRenderer` publishes correlated diagnostics fields (stage/error/availability and XYZ visual coordinates) via separate relaxed atomics.
   - Safe for independent scalars, but readers can transiently observe mixed-generation values.

## Detailed Atomic Ordering Table

| File | Line(s) | Variable / Operation | Ordering | Assessment | Recommendation |
|---|---:|---|---|---|---|
| Source/SceneGraph.h | 76,78,84 | `readIndex` load/store/load (emitter data double buffer) | acquire + release + acquire | Correct publish/consume pattern | None |
| Source/SceneGraph.h | 105,123,128,132,137,143 | `audioReadIndex` load/store/load (audio snapshot double buffer) | acquire + release | Correct for cross-thread buffer handoff | None |
| Source/SceneGraph.h | 193 | `++activeEmitterCount` | default seq_cst (RMW) | Correct (stronger than needed) | Optional: explicit `fetch_add(..., memory_order_relaxed)` for intent |
| Source/SceneGraph.h | 212 | `activeEmitterCount.load()/store()` decrement | default seq_cst | Correct under `registrationLock` | Optional: `fetch_sub` for clarity |
| Source/SceneGraph.h | 238 | `activeEmitterCount.load()` | default seq_cst | Correct atomic read | None |
| Source/SceneGraph.h | 245,250 | `currentRoomProfile.store/load` via contract | release / acquire | Correct shared_ptr publication | None |
| Source/SceneGraph.h | 257,262 | `globalSampleCounter.fetch_add/load` | relaxed | Correct for independent monotonic counter | None |
| Source/SceneGraph.h | 269,274 | `physicsRateIndex.store/load` | release / acquire | Correct paired ordering | None |
| Source/SceneGraph.h | 279,284 | `physicsPaused.store/load` | release / acquire | Correct paired ordering | None |
| Source/SceneGraph.h | 289,294 | `physicsWallCollisionEnabled.store/load` | release / acquire | Correct paired ordering | None |
| Source/SceneGraph.h | 299,304 | `physicsInteractionEnabled.store/load` | release / acquire | Correct paired ordering | None |
| Source/SceneGraph.h | 185,187,204,211,237 | `slotOccupied[]` registration claim/release + unlocked read | non-atomic bool | **Incorrect**: data race risk between lock-protected writes and unlocked reads | Convert to `std::atomic<bool>` (acquire/release) or lock reads |
| Source/SceneGraph.h | 220,221,228,231 | `rendererRegistered` set/clear + unlocked read | non-atomic bool | **Incorrect**: data race risk between lock-protected writes and unlocked reads | Convert to `std::atomic<bool>` or lock reads |
| Source/SceneGraph.h | 180-213 | Slot registration / release protocol (ABA check) | lock-serialized (SpinLock) | No ABA issue observed in current algorithm | Optional generation counter only if stale external slot-id reuse becomes a requirement |
| Source/SharedPtrAtomicContract.h | 23 or 29 | `store(nextValue)` | release | Correct pointer publication ordering | None |
| Source/SharedPtrAtomicContract.h | 39 or 45 | `load()` | acquire | Correct pointer consumption ordering | None |
| Source/VisualTokenScheduler.h | 39,40 | `snapshot.count/seq.store` in reset | release | Correct reset publication | None |
| Source/VisualTokenScheduler.h | 167,171,177,187 | Seqlock read path `seq/count/seq` | acquire | Correct seqlock reader ordering | None |
| Source/VisualTokenScheduler.h | 252,257,260 | Seqlock write path `seq++ / count.store / seq++` | acq_rel + release + release | Correct writer protocol for stable snapshots | None |
| Source/CalibrationEngine.h | 78,95,268 | `analysisRunning_` lifecycle signal | release / acquire | Correct worker lifecycle coordination | None |
| Source/CalibrationEngine.h | 117,134 and 143,252,272,283,296,323 | `abortRequested_` store/load | release / acquire | Correct cancellation flag ordering | None |
| Source/CalibrationEngine.h | 135,183,270 | `analysisRequested_` store/exchange | release / acq_rel | Correct producer-consumer handoff | None |
| Source/CalibrationEngine.h | 136,167,182,254,260,286,299,326,349 and 149,196,273,284,297,324 | `state_` store/load | release / acquire | Correct phase-state publication/consumption | None |
| Source/CalibrationEngine.h | 116,242,244,158 | `state_`/`currentSpeaker_` default `load()` sites | default seq_cst | Correct (conservative) | Optional: explicit acquire for consistency with surrounding code style |
| Source/PhysicsEngine.h | 61,123,126,132 | `running` exchange/load/store/load | acq_rel + acquire + release + acquire | Correct thread start/stop synchronization | None |
| Source/PhysicsEngine.h | 115,144,394,396 | `readIndex` state-buffer publish path | acquire / release | Correct double-buffer publication | None |
| Source/PhysicsEngine.h | 69-97 and 134,148-150,182,217,226-229,234-236,262,270-273,357-358 | Config atomics (paused, wall, body, mass/drag/etc., rest, room, gravity) | release stores + acquire loads | Correct paired ordering for worker visibility | None |
| Source/PhysicsEngine.h | 102-105 and 208-214 | Throw command payload + sequence gate | release + acq_rel + acquire | Correct command publication protocol | None |
| Source/PhysicsEngine.h | 110,170 | Reset sequence gate | acq_rel + acquire | Correct command publication protocol | None |
| Source/PhysicsEngine.h | 55 | `currentSampleRate.store` | relaxed | Correct but currently write-only in this unit | Optional: remove if unused or add documented read path |
| Source/HeadTrackingBridge.h | 105,181 | `activePose.load/store` | acquire / release | Correct pointer publication from network thread to readers | None |
| Source/HeadTrackingBridge.h | 179,182 | `writeSlot.load/store` | relaxed | Correct for local producer indexing | None |
| Source/HeadTrackingBridge.h | 183,208 | `hasPose.store/load` | release / acquire | Correct publication of first-valid-pose state | None |
| Source/HeadTrackingBridge.h | 184,210 | `lastSeq.store/load` | relaxed | Correct for current in-thread duplicate filtering usage | If cross-thread consumers are added, upgrade or couple with acquire/release |
| Source/SpatialRenderer.h | 385-406,431,441,451,1736 | Requested profile/mode atomics (`requestedHeadphoneMode/Profile`, `requestedSpatialProfile`) | relaxed load/store | Correct for independent scalar config exchange | None |
| Source/SpatialRenderer.h | 850-851,884-885 and 436,446,456,461 | Active profile/mode atomics | relaxed store/load | Correct for telemetry/state indicators | None |
| Source/SpatialRenderer.h | 807,809-813,1000-1040,1427-1429 | Guardrail counters + audition visual atomics | relaxed store/load | Correct for low-latency diagnostics; coherence not guaranteed across fields | If coherent snapshots are required, publish under one seq counter or packed atomic struct |
| Source/SpatialRenderer.h | 110,112,466,476,481,886,1467-1468,1494,1544,1671,1675,1708 | Steam runtime status atomics (available/stage/error) | relaxed store/load | Functionally safe per-field; mixed-generation tuples possible | Use release/acquire tuple publication if UI requires stage/error consistency |
| Source/PluginProcessor.cpp | 485,603-614,654-660,731-790,880-1067,1143,1182-1199,1227,1291-1295,1550-1551,1585-1599,2299-2335 | APVTS parameter atomics via `getRawParameterValue(...)->load()` | default seq_cst | Correct and conservative for host/RT parameter reads | None |
| Source/PluginProcessor.cpp | 1117-1118 | CLAP lifecycle atomics `is_clap_active/is_clap_processing` | relaxed load | Correct for diagnostics-only status sampling | None |

## Pairing Check
- Release/acquire pairings are present and correct for:
  - SceneGraph emitter and audio double-buffer swaps
  - SharedPtrAtomicContract room profile publication
  - CalibrationEngine cross-thread state transitions
  - PhysicsEngine command publication and state buffer swapping
  - VisualTokenScheduler seqlock publication
  - HeadTrackingBridge pose pointer publication

## Slice A Exit Notes
- No source code modified in this slice.
- Identified violations are confined to registration flag storage/read strategy in SceneGraph and should be addressed in Slice B.
