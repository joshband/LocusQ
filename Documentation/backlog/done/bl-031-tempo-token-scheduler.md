Title: BL-031 Tempo-Locked Visual Token Scheduler
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-25

# BL-031: Tempo-Locked Visual Token Scheduler

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Done (2026-02-25 owner promotion sync complete) |
| Owner Track | Track B — Scene/UI Runtime |
| Depends On | BL-016 (Done), BL-025 (Done) |
| Blocks | BL-029 |
| Annex Spec | `Documentation/plans/bl-031-tempo-locked-visual-token-scheduler-spec-2026-02-24.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | High | M | Audio-thread token scheduler |
| B | Med | M | Lock-free snapshot publication |
| C | Med | M | UI polling + bridge integration |
| D | Med | M | Deterministic tempo ramp tests |

## Objective

Implement a host-tempo-synchronized visual timing contract using sample-stamped tokens (bar/beat/subdivision) published from the audio thread via a fixed-size atomic snapshot, consumed on the message thread for sub-frame UI interpolation. Success: visuals lock to DAW tempo with sample-accurate precision and no RT safety violations.

## Scope & Non-Scope

**In scope:**
- Sample-stamped token scheduling on audio thread using host musical context
- Lock-free fixed-size token snapshot publication (atomic seq/count)
- UI polling + interpolation scheduler for visual consumers
- Deterministic tests for token emission and host tempo ramp behavior

**Out of scope:**
- Allocating/posting async UI callbacks from audio thread
- Any lock/blocking path in processBlock
- Changing DSP graph semantics for audio rendering
- Visual rendering of tempo effects (that's BL-029)

## Architecture Context

- Host musical context: sampleRate, blockStartSample, tempo, PPQ/beat position, time signature, transport state
- Data contract from annex spec:
  ```cpp
  struct VisualToken { uint32_t sampleOffsetInBlock; float ppq; uint8_t type; };
  struct VisualTokenSnapshot { std::atomic<uint32_t> seq{0}; std::atomic<uint32_t> count{0}; VisualToken tokens[32]; };
  ```
- Publication order: fill tokens -> count.store(n, release) -> seq.fetch_add(1, release)
- Consumer: message thread polls seq, reads count, copies tokens
- Transport contract lineage: BL-016 (scene-state contract), `Documentation/scene-state-contract.md`
- Invariants: Audio Thread (RT safety — absolutely critical), Scene Graph (sequence-safe transport)
- Platform wrappers differ (AU/VST3/CLAP) but scheduler API stays format-agnostic

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Audio-thread token scheduler | `Source/PluginProcessor.cpp`, new `Source/VisualTokenScheduler.h` | BL-016, BL-025 done | Tokens emitted on beat boundaries in processBlock |
| B | Lock-free snapshot publication | `Source/VisualTokenScheduler.h` | Slice A done | Atomic seq/count publication correct, no tearing |
| C | UI polling + bridge integration | `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js` | Slice B done | JS receives tokens, interpolation works |
| D | Deterministic tempo ramp tests | `tests/`, `TestEvidence/` | Slice C done | Tempo ramp produces monotonic token sequence |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl BL-031 Slice A: Audio-thread visual token scheduler
Load: $juce-webview-runtime, $reactive-av, $physics-reactive-audio, $skill_impl

Objective: Create VisualTokenScheduler.h — a header-only class called from processBlock
that emits VisualToken events on bar, beat, and subdivision boundaries using host musical
context (tempo, PPQ, time signature, transport state).

Files to create/modify:
- Create: Source/VisualTokenScheduler.h
- Modify: Source/PluginProcessor.cpp — instantiate scheduler, call from processBlock

Data contract (from annex spec):
struct VisualToken {
    uint32_t sampleOffsetInBlock;
    float ppq;
    uint8_t type; // 0=bar, 1=beat, 2=micro, 3=swing
};

struct VisualTokenSnapshot {
    std::atomic<uint32_t> seq { 0 };
    std::atomic<uint32_t> count { 0 };
    VisualToken tokens[32];
};

Constraints:
- CRITICAL: No heap allocation, locks, or blocking I/O — this runs in processBlock
- Use host-provided PPQ position to detect beat/bar boundaries
- Handle missing host time info gracefully (some hosts don't provide PPQ)
- Fixed-size token array (32 slots max per block)
- Token type detection: compare current PPQ to previous, emit on integer crossings
- Must handle tempo changes within a block (linear interpolation between block boundaries)

Validation:
- Build succeeds with no warnings
- Standalone with internal transport: tokens appear at beat boundaries
- No allocations in processBlock (verified by inspection)

Evidence:
- TestEvidence/bl031_tempo_token_<timestamp>/slice_a_build.log
```

### Slice A — Standalone Fallback Prompt

```
You are implementing BL-031 Slice A for LocusQ, a JUCE spatial audio plugin.

PROJECT CONTEXT:
- LocusQ processes audio in Source/PluginProcessor.cpp::processBlock()
- Host musical context is available via juce::AudioPlayHead::CurrentPositionInfo
- Existing lock-free pattern: SceneGraph uses atomic double-buffering (Source/SceneGraph.h)
- RT safety invariant: absolutely no heap allocation, locks, or blocking I/O in processBlock()
- All DSP headers are header-only (template-heavy JUCE patterns)

TASK:
1. Create Source/VisualTokenScheduler.h as a header-only class:
   - Struct VisualToken: { uint32_t sampleOffsetInBlock; float ppq; uint8_t type; }
   - Struct VisualTokenSnapshot: { atomic<uint32_t> seq; atomic<uint32_t> count; VisualToken tokens[32]; }
   - Class VisualTokenScheduler:
     - processBlock(AudioPlayHead*, int numSamples, double sampleRate) method
     - Detects beat/bar boundaries by comparing current PPQ to previous PPQ
     - Emits bar token (type=0) on integer bar crossings
     - Emits beat token (type=1) on integer beat crossings
     - Emits micro token (type=2) on subdivision crossings (configurable subdivision)
     - Handles missing host time info (no-op if PPQ unavailable)
     - Fills snapshot.tokens[], updates count and seq atomically

2. Modify Source/PluginProcessor.cpp:
   - Add VisualTokenScheduler member
   - Call scheduler.processBlock() from processBlock() after DSP processing

CONSTRAINTS:
- Zero heap allocation in processBlock path
- Fixed-size arrays only (32 tokens max)
- Atomic ordering: fill tokens -> count.store(n, release) -> seq.fetch_add(1, release)
- Handle transport not playing (emit no tokens)
- Handle tempo = 0 or invalid (emit no tokens)

VALIDATION:
- cmake --build build --target all (zero warnings)
- Launch standalone, verify no crash
- Add temporary logging (message thread only!) to verify tokens appear

EVIDENCE:
- TestEvidence/bl031_tempo_token_<timestamp>/slice_a_build.log
```

### Slice B — Skill-Aware Prompt

```
/impl BL-031 Slice B: Lock-free snapshot publication
Load: $skill_impl, $juce-webview-runtime

Objective: Verify and harden the atomic publication path — ensure no tearing between
audio thread writes and message thread reads.

Constraints:
- Publication order: fill tokens -> count.store(n, release) -> seq.fetch_add(1, release)
- Consumer reads: read seq -> read count -> copy tokens -> re-read seq (verify unchanged)
- If seq changed during read, discard and retry
- No locks, no mutexes — pure atomic + fixed memory only

Validation:
- Code review of atomic ordering
- Stress test: high tempo (300 BPM) with rapid transport toggle
- Verify no torn reads in consumer output

Evidence:
- TestEvidence/bl031_tempo_token_<timestamp>/slice_b_atomics_review.md
```

### Slice C — Skill-Aware Prompt

```
/impl BL-031 Slice C: UI polling and bridge integration
Load: $juce-webview-runtime, $reactive-av, $threejs

Objective: Add message-thread polling that reads VisualTokenSnapshot and forwards
tokens to WebView JS via the native bridge for visual consumption.

Files to modify:
- Source/PluginEditor.cpp — add timer-driven poll, bridge function for tokens
- Source/ui/public/js/index.js — add token receiver, interpolation utility

Constraints:
- Polling on message thread only (juce::Timer or high-res timer callback)
- Bridge payload: JSON array of {sampleOffset, ppq, type} per token batch
- JS interpolation: use requestAnimationFrame timing + token PPQ for sub-frame sync
- Payload must be bounded (32 tokens max per batch)

Validation:
- Visual indicator in UI shows beat flashes at correct tempo
- Token delivery latency < 2 animation frames
- No bridge payload exceeds budget (check HX-05 thresholds)

Evidence:
- TestEvidence/bl031_tempo_token_<timestamp>/slice_c_bridge.log
```

### Slice D — Skill-Aware Prompt

```
/test BL-031 Slice D: Deterministic tempo ramp tests
Load: $skill_test, $skill_testing

Objective: Write deterministic tests proving token emission is correct under
tempo ramps, transport start/stop, and edge cases.

Test cases:
1. Constant 120 BPM: tokens at expected PPQ intervals
2. Tempo ramp 60->180 BPM over 4 bars: token count increases proportionally
3. Transport stop/start: no tokens during stop, resume on start
4. Very high tempo (300 BPM): tokens within 32-slot capacity
5. Missing host time info: zero tokens emitted (graceful no-op)
6. Sequence monotonicity: seq values are strictly increasing

Validation:
- All tests pass
- Sequence monotonicity assertion never fails

Evidence:
- TestEvidence/bl031_tempo_token_<timestamp>/slice_d_tests.log
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| UI-P2-031A | Automated | Build + basic token emission | Tokens at beat boundaries |
| UI-P2-031B | Automated | Atomic ordering stress test | No torn reads at 300 BPM |
| UI-P2-031C | Automated | Bridge delivery + visual sync | Beat flashes at correct tempo |
| UI-P2-031D | Automated | Deterministic test suite | All 6 test cases pass |
| BL-031-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Host tempo info unavailable (varies AU/VST3/CLAP) | High | Med | Graceful no-op when PPQ missing, test across formats |
| Token overflow at very high tempos | Med | Low | 32-slot cap with oldest-drop policy |
| Bridge payload too large for high subdivision | Med | Med | Limit micro tokens, respect HX-05 budget |
| Atomic ordering bug causes torn reads | High | Low | Seq-count-seq double-check pattern |

## Failure & Rollback Paths

- If token emission incorrect: add PPQ logging (message thread), compare against expected beat positions
- If atomic tearing detected: review memory ordering, add seq verification in consumer
- If bridge latency too high: reduce polling interval, batch fewer tokens
- If host format doesn't provide tempo: verify getPosition() API per format, add format-specific fallbacks

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Build log | `TestEvidence/bl031_tempo_token_<timestamp>/build.log` | warnings, errors |
| Atomics review | `TestEvidence/bl031_tempo_token_<timestamp>/slice_b_atomics_review.md` | ordering analysis |
| Bridge log | `TestEvidence/bl031_tempo_token_<timestamp>/slice_c_bridge.log` | latency, payload sizes |
| Test results | `TestEvidence/bl031_tempo_token_<timestamp>/slice_d_tests.log` | test case, result, notes |
| Validation trend | `TestEvidence/validation-trend.md` | date, lane, result per slice |

## Owner Validation Snapshot (2026-02-25)

| Slice | Status | Evidence |
|---|---|---|
| Slice A | Implemented and owner-validated through Slice B integrated lane | `TestEvidence/bl031_tempo_token_20260224T190152Z/status.tsv`, `TestEvidence/bl031_slice_b_20260224T194102Z/status.tsv` |
| Slice B | Implemented and owner-validated | `TestEvidence/bl031_slice_b_20260224T194102Z/status.tsv` |
| Slice C | Covered under integrated deterministic lane contract checks | `TestEvidence/bl031_slice_d_20260225T160932Z/status.tsv` (lane contract and scenario checks), `TestEvidence/owner_parallel_integration_20260225T162457Z/qa_bl031_lane.log` (owner replay pass) |
| Slice D | Implemented and validated | `TestEvidence/bl031_slice_d_20260225T160932Z/status.tsv` (worker lane pass), `TestEvidence/owner_parallel_integration_20260225T162457Z/qa_bl031_lane.log` (owner replay pass) |

## Slice E Promotion Verifier Packet (2026-02-25)

Worker promotion-verifier reran the required branch-state gates and emitted a promotion packet at:
`TestEvidence/bl031_done_promotion_20260225T163524Z`

| Gate | Result | Evidence |
|---|---|---|
| Build (`locusq_qa` + `LocusQ_Standalone`) | PASS | `TestEvidence/bl031_done_promotion_20260225T163524Z/build.log` |
| Deterministic BL-031 lane | PASS | `TestEvidence/bl031_done_promotion_20260225T163524Z/qa_lane.log`, `TestEvidence/bl031_done_promotion_20260225T163524Z/scenario_result.log`, `TestEvidence/bl031_done_promotion_20260225T163524Z/token_monotonicity.tsv` |
| RT safety audit gate | PASS | `TestEvidence/bl031_done_promotion_20260225T163524Z/rt_audit.tsv`, `TestEvidence/bl031_done_promotion_20260225T163524Z/rt_audit.log` (`non_allowlisted=0`) |
| Docs freshness gate | PASS | `TestEvidence/bl031_done_promotion_20260225T163524Z/docs_freshness.log` |

Promotion recommendation from worker packet: **PASS** (eligible for `In Validation -> Done` promotion by owner-managed Tier 0 sync).

Owner disposition: BL-031 is promoted to **Done** on 2026-02-25 after owner-managed sync of backlog/index/status/evidence surfaces.

## Closeout Checklist

- [x] Slice A: VisualTokenScheduler emits tokens at beat boundaries in processBlock
- [x] Slice B: Atomic publication verified, no tearing under stress
- [x] Slice C: UI receives tokens via bridge, visual sync works
- [x] Slice D: All 6 deterministic test cases pass
- [x] Sequence monotonicity holds under all test conditions
- [x] No RT safety violations (zero allocations in processBlock)
- [x] Evidence captured at designated paths
- [x] Slice E promotion packet gates PASS (lane + RT + docs)
- [x] status.json updated
- [x] Documentation/backlog/index.md row updated
- [x] TestEvidence surfaces updated
- [x] ./scripts/validate-docs-freshness.sh passes
