Title: BL-031 Tempo-Locked Visual Token Scheduler Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# BL-031 Tempo-Locked Visual Token Scheduler Spec

## Purpose
Define a realtime-safe, host-tempo-synchronized visual timing contract so LocusQ visuals lock to DAW transport with sample-stamped token precision and sub-frame UI interpolation.

## Backlog Link
- Backlog ID: `BL-031`
- Canonical backlog file: `Documentation/backlog-post-v1-agentic-sprints.md`

## Companion Specs
- `Documentation/plans/bl-016` transport contract lineage is represented in `Documentation/scene-state-contract.md`.
- `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md`
- `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md`

## Problem Statement
Current visual updates are cadence-safe but not explicitly musical-time authoritative. For tempo-critical visual behavior (downbeat flashes, sequencer cursors, rhythmic particles), wall-clock scheduling can drift from host PPQ/tempo.

## Goal
Emit musical timing tokens (`bar`, `beat`, `microbeat`, optional `swing phase`) from the audio thread as sample-stamped events, then consume them on the message thread through lock-free polling for deterministic, sub-frame visual synchronization.

## Scope
In scope:
1. Sample-stamped token scheduling on audio thread using host musical context.
2. Lock-free fixed-size token snapshot publication.
3. UI polling + interpolation scheduler for visual consumers.
4. Deterministic tests for token emission and host tempo ramp behavior.

Out of scope:
1. Allocating/posting async UI callbacks from audio thread.
2. Any lock/blocking path in `processBlock()`.
3. Changing DSP graph semantics for audio rendering.

## Realtime Safety Contract
1. Audio-thread code must perform no heap allocation, no locks, no blocking I/O.
2. Audio thread must not call `AsyncUpdater::triggerAsyncUpdate()` or `MessageManager::callAsync()`.
3. Token publication uses fixed-size data and atomic sequence/count fields.
4. UI consumes tokens by timer/high-resolution polling on message thread.

## Host-Time Contract
Audio-thread scheduler inputs per block:
1. `sampleRate`
2. `blockStartSample`
3. `tempo`
4. `PPQ / beat position`
5. `time signature`
6. transport state (`playing`/`recording`)

Platform wrappers may differ (AU/VST3/CLAP), but scheduler API stays format-agnostic.

## Data Contract

```cpp
struct VisualToken
{
    uint32_t sampleOffsetInBlock;
    float ppq;
    uint8_t type; // bar, beat, micro, swing
};

struct VisualTokenSnapshot
{
    std::atomic<uint32_t> seq { 0 };
    std::atomic<uint32_t> count { 0 };
    VisualToken tokens[32];
};
```

Publication order:
1. Fill `tokens[0..n-1]`
2. `count.store(n, release)`
3. `seq.fetch_add(1, release)`

Consumption order:
1. Read `seq` (acquire)
2. Read `count` (acquire)
3. Copy bounded token array into local UI buffer
4. Schedule visual events against local render clock

## Token Semantics
Minimum token set:
1. `Downbeat`
2. `Beat`
3. `Subdivision` (configurable)

Optional token semantics:
1. `SwingMicrobeat` (phase-shifted subdivisions)
2. `BarBoundary`

## Implementation Slices

### Slice A: Core Scheduler + Snapshot
Files:
1. `Source/PluginProcessor.h`
2. `Source/PluginProcessor.cpp`
3. `Source/` new helper header/source for scheduler utilities

Deliverables:
1. `TokenScheduler` with deterministic token prediction per block
2. `VisualTokenSnapshot` publication path in audio thread

### Slice B: Format-Adapter Layer
Files:
1. `Source/PluginProcessor.cpp`
2. Optional adapter helper for host musical context reads

Deliverables:
1. Unified extraction of tempo/PPQ/time-sig/transport into scheduler input struct
2. Explicit fallback when host context unavailable

### Slice C: UI Poll + Sub-frame Interpolation
Files:
1. `Source/PluginEditor.cpp`
2. `Source/ui/public/js/index.js`

Deliverables:
1. Token polling bridge function(s)
2. UI-side event scheduler for downbeat/beat/subdivision visual hooks
3. Stable behavior when editor closed/reopened

### Slice D: QA + Determinism Lanes
Files:
1. `qa/` harness scenarios
2. `scripts/` lane wrappers
3. `Documentation/scene-state-contract.md`
4. `Documentation/implementation-traceability.md`

Deliverables:
1. Fixed-seed token sequence tests under static tempo and tempo ramps
2. Self-test assertions for sequence monotonicity, finite timing, stale fallback behavior

## Validation Plan
Automated:
1. `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8`
2. Production self-test lane extensions (`UI-P2-031A..D`):
   - `UI-P2-031A`: token sequence monotonicity and bounded count
   - `UI-P2-031B`: downbeat/beat alignment under fixed tempo
   - `UI-P2-031C`: stable behavior under tempo ramps and transport restarts
   - `UI-P2-031D`: stale/empty-token fallback without UI lockup
3. Existing host lanes:
   - `./scripts/standalone-ui-selftest-production-p0-mac.sh`
   - `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`

Manual:
1. Verify rhythm-sensitive visuals against DAW metronome at multiple tempos.
2. Verify transport stop/start and loop boundaries remain visually phase-locked.

## Dependencies
1. BL-016 transport/stale safety contracts remain authoritative.
2. BL-025 UI responsiveness baseline should remain stable.
3. BL-029 introspection roadmap consumes this timing layer for rhythm-locked trace and reactive overlays.

## Risks and Mitigations
1. Risk: token overproduction on extreme subdivisions.
   - Mitigation: strict `K` cap with deterministic truncation policy and QA assertions.
2. Risk: host-time discontinuities at loop jumps.
   - Mitigation: detect transport discontinuity and reset phase accumulator deterministically.
3. Risk: UI jitter despite accurate tokens.
   - Mitigation: hybrid token-trigger + interpolation path tied to render frame cadence.

## Exit Criteria
1. Visual tokens are emitted sample-stamped and sequence-safe under host transport.
2. No realtime invariant violations in audio thread.
3. Host matrix lanes remain green with new token path active.
4. Traceability docs are updated for new timing contract surfaces.

## Delivery Status
- Current status: `spec_complete`
- Validation status: `not tested` (planning artifact only)
