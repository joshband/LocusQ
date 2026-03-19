Title: DSP Real-Time Spatial Constraints (JUCE Integration)
Document Type: Calibration POC Note
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# DSP Real-Time Spatial Constraints (JUCE Integration)

**Document:** `DSP_REALTIME_SPATIAL_CONSTRAINTS.md`  
**Applies to:** LocusQ Spatial Audio engine (SOFA HRTF + headphone EQ + real-time binaural rendering)  
**Goal:** Ensure the spatial renderer is *real-time safe*, *deterministic*, and *host-correct* when integrated into a JUCE plugin.

---

## 1. Hard Real-Time Invariants (Audio Thread)

These are **non-negotiable**. Violations are bugs.

### 1.1 Forbidden in `processBlock()`
- Heap allocation (direct or indirect): `new`, `delete`, `malloc`, `free`, `std::vector::push_back` growth, `juce::String` creation, `AudioBuffer` resize, FFT plan creation, etc.
- Locks / blocking: `std::mutex`, `juce::CriticalSection`, `MessageManagerLock`, `std::condition_variable`, waiting on futures, thread joins.
- File I/O / disk: reading SOFA, JSON, presets, logging to disk.
- Network / IPC.
- Any API with unknown blocking behavior.

### 1.2 Allowed in `processBlock()`
- Reading **atomics** (float/bool/int) for parameters and control-rate state.
- Pure DSP on preallocated memory.
- Atomic pointer loads/stores for state swaps (see §4).
- Branching that does not trigger heavy rebuilds (constant-time).

### 1.3 Determinism
Given the same inputs (audio + parameters + engine state), output must be identical across runs.
- No random numbers unless seeded and *captured in state*.
- No time-based decisions in the callback.

---

## 2. Architectural Separation (Threads & Responsibilities)

### 2.1 Threads
**Message/UI thread**
- User actions: load profile, choose headphone model/state, trigger ear-photo flow.
- Display: subject id, similarity score, current HRTF set, latency.

**Worker thread(s)**
- SOFA parsing and validation.
- HRIR extraction and preprocessing.
- Kernel partitioning + FFT precomputation.
- libmysofa lookup / interpolation and kernel generation (Phase C).
- EQ preset parsing and coefficient derivation.

**Audio thread**
- Run DSP using prebuilt state only.
- Handle pointer swaps and crossfades.

### 2.2 Core Rule
**Anything that touches the filesystem or performs heavy math setup happens off-thread.**
Audio thread only consumes ready-to-run objects.

---

## 3. Module Boundaries (Do Not Merge These)

### 3.1 User Spatial Profile (HRTF-side)
Contains user-specific selection/morph parameters.
- `hrtf_set_id` (e.g., SADIE II)
- `subject_id` (nearest-neighbor phase)
- `match_confidence` / similarity score
- (future) morph coefficients / model version

### 3.2 Headphone Profile (EQ-side)
Contains headphone response compensation choices.
- `headphone_model_id` (e.g., `sony_wh1000xm5`)
- `mode` (e.g., `anc_on`, `anc_off`)
- `eq_preset_id`
- biquad coefficients (or reference to coefficient bank)

### 3.3 Render Config (Runtime Controls)
- `azimuth`, `elevation`
- enable flags: spatial on/off, EQ on/off
- output trim

**Invariant:** Headphone EQ and HRTF selection are separate modules and must remain independently switchable.

---

## 4. Engine State & Atomic Swap (Two-Phase Commit)

### 4.1 Immutable `SpatialEngineState`
Once built, it is **read-only** from the audio thread perspective.

**Contains:**
- sample rate, max block size, partition size
- precomputed convolution kernels (FFT domain)
- overlap-add buffers and scratch buffers (preallocated)
- EQ coefficient bank (or pointer to current preset coefficients)
- crossfade state for safe transitions

**Does not contain:**
- file handles
- SOFA parsing objects
- temporary build buffers
- anything that allocates during processing

### 4.2 Swap Protocol
1. Worker builds a complete `SpatialEngineState` (including all FFT kernels).
2. Worker publishes it as **pending**.
3. Audio thread checks at a block boundary and performs:
   - pointer swap `active <- pending`
   - crossfade (old → new) for N samples/blocks
4. Old state is retired **off-thread** once crossfade completes.

### 4.3 Lock-Free Requirements
- `active` and `pending` stored as atomic pointers.
- No locks during swap.
- No deletion of old state on the audio thread.

---

## 5. HRIR Direction Updates (Avoid Zipper Noise)

There are two kinds of change:

### 5.1 Direction Changes (source movement)
If using nearest-neighbor HRIR selection, direction jumps can click.

**Requirement:** seamless transition by one of:
- **Dual-path crossfade**: two convolvers A/B, fade outputs.
- **Kernel crossfade**: fade between two kernel sets.

**Acceptance:** a continuous azimuth sweep produces no audible jumps.

### 5.2 Profile / Engine Changes (subject/preset changes)
Same mechanism, typically longer crossfade acceptable (50–200 ms).
- Crossfade length is part of the engine state.
- Must not block on a build finishing.

---

## 6. Partitioned Convolution (Real-Time Implementation Constraints)

### 6.1 Requirements
- Fixed partition size (power of two): typically 128 or 256 samples.
- FFT plans are created at build-time (worker thread) and reused.
- Overlap-add buffers preallocated to maxBlockSize + FFTSize.
- Kernel FFTs precomputed.

### 6.2 Latency Reporting
Partitioned convolution introduces latency.
- Must call `setLatencySamples(latencySamples)` when engine becomes active.
- Latency must update if partition size changes.

### 6.3 Tail Reporting
HRTF IR length implies a tail.
- `getTailLengthSeconds()` should be conservative and stable per engine state.

---

## 7. Headphone EQ (WH-1000XM5) Integration Constraints

### 7.1 Where EQ Lives
Apply headphone EQ **pre-HRTF** in the signal chain:
`Input -> HeadphoneEQ -> BinauralConvolution -> Output`

This avoids changing interaural cues after binauralization.

### 7.2 Preset Switching
Preferred: coefficient swap (tiny, fast, deterministic)
- Maintain a coefficient bank for: `anc_on`, `anc_off`.
- Swap active coefficient pointer/struct atomically.

Avoid rebuilding the engine for EQ-only changes unless absolutely necessary.

---

## 8. JUCE Integration Checklist (Code Review Gate)

### 8.1 `prepareToPlay()`
- [ ] Allocates all audio buffers to `maxBlockSize`.
- [ ] Requests initial engine build (async) and starts with a safe bypass state.
- [ ] Sets latency only when engine becomes active (or initial known latency).

### 8.2 `processBlock()`
- [ ] No allocations (validate in debug with allocation tracker).
- [ ] No locks.
- [ ] No file I/O.
- [ ] Atomics used for parameters (cached from APVTS callbacks).
- [ ] Active engine pointer loaded atomically once per block.
- [ ] Swap/crossfade path is constant-time.

### 8.3 `releaseResources()`
- [ ] Stops workers cleanly (not from audio thread).
- [ ] Retires engine state off-thread.

### 8.4 State Save/Restore
- [ ] UserSpatialProfile and HeadphoneProfile serialized separately.
- [ ] Engine rebuild triggered on restore, but processing remains safe during rebuild.

---

## 9. Suggested Class Layout (Reference)

### 9.1 `LocusQAudioProcessor`
Responsibilities:
- APVTS + atomic parameter cache
- Owns `EngineManager`
- Calls `engine->process(...)` in `processBlock()`

### 9.2 `EngineManager`
Responsibilities:
- Worker thread(s) and build queue
- Publishes `pending` engine pointer
- Tracks crossfade completion + retirement

### 9.3 `SpatialEngineState`
Responsibilities:
- `process()` method (real-time safe)
- Holds `HeadphoneEQ` + `BinauralConvolver` and all buffers

### 9.4 `BinauralConvolver`
Responsibilities:
- Partitioned convolution
- A/B crossfade for direction or engine changes
- Kernel set binding without allocations

---

## 10. Phase Mapping (Aligned to the Project Plan)

### Phase A (Offline)
- JUCE plugin may expose offline render utility or separate tool.
- Real-time constraints still apply if run inside plugin.

### Phase B (2×2 Tests)
- Keep rendering offline where possible for determinism.
- Use the same renderer core as real-time, but with offline I/O outside audio thread.

### Phase C (After Proof)
**C1: libmysofa interpolation**
- libmysofa calls occur on worker thread
- resulting interpolated HRIR is partitioned + FFT’d off-thread
- audio thread crossfades to new kernels

**C2: Real-time partitioned convolution**
- engine state becomes mandatory
- host latency reported

**C3: Multiple headphone modes**
- coefficient bank expands; switching remains lock-free

---

## 11. Instrumentation & Validation (Recommended)

### 11.1 Debug Allocation Guard
Add a debug-only allocator guard to assert no allocations in `processBlock()`.

### 11.2 Thread Sanitizer / Lock Audit
Enforce a policy: any function callable from the audio thread must be annotated/commented as RT-safe.

### 11.3 Golden Tests
- Impulse response test: output must match HRIR when input is impulse.
- Sweep tests: ensure EQ curve is applied correctly.
- Motion test: azimuth sweep must be click-free.

---

## 12. Acceptance Criteria (Real-Time “Done”)

A real-time spatial build is acceptable when:
- Stable at 44.1k and 48k sample rates
- Stable at 128-sample host buffer size
- No clicks when:
  - changing direction continuously
  - swapping subject/profile (crossfade)
  - toggling EQ preset
- CPU remains within a defined budget (recorded and tracked)
- Latency reporting matches actual partition latency

---

## Appendix A — Minimal DSP Signal Chain

1. Input downmix (if needed)  
2. Headphone EQ (optional)  
3. Binaural convolution (HRTF)  
4. Output gain trim / safety limiter (optional)

---

## Appendix B — “RT-Safe” Annotation Convention (Suggested)

For any method reachable from `processBlock()`:
- Use comment header: `// RT_SAFE: no alloc, no locks, no IO`
- Add unit tests/CI checks where possible.
