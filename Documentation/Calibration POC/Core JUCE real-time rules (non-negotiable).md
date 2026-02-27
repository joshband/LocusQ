Title: Core JUCE real-time rules (non-negotiable)
Document Type: Calibration POC Note
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

Below is a direct mapping of your phased plan into **JUCE real-time plugin architecture constraints**—the stuff that will keep the audio thread clean, the UI responsive, and your future “real-time spatial engine” from turning into a glitch museum.

## Core JUCE real-time rules (non-negotiable)

### Audio thread invariants

* **No heap allocation** in `processBlock` (including `std::vector` growth, `AudioBuffer` resizing, FFT plan creation, `std::string`, `new`, etc.).
* **No locks** in `processBlock` (no mutex, no `MessageManagerLock`, no `CriticalSection`).
* **No file I/O** in `processBlock`.
* **No dynamic library loading** or SOFA parsing on the audio thread.
* **No blocking waits** (condition variables, futures, joins).
* **Deterministic CPU**: per-block cost should not depend on parameter values (avoid “if changed then rebuild huge stuff” inside the callback).

### Parameter + state invariants

* Parameters must be **atomic/lock-free readable** in the audio thread.
* Large state changes (loading SOFA, changing subject, changing HRTF set) must be **two-phase commit**:

  1. Build new engine state off-thread
  2. Atomically swap a pointer on the audio thread at a block boundary

### Latency invariants

* Partitioned convolution implies latency. You must:

  * Choose a fixed partition size (e.g., 128/256 samples)
  * Call `setLatencySamples(latency)` and keep it accurate
  * Provide a “low-latency / high-quality” mode if needed

---

## Plugin architecture: the clean separation you want

### High-level module split

**UI / Message Thread**

* File selection, profile selection, ear-photo pipeline triggers
* Displays current subject, match score, HRTF set, etc.

**Background Worker Thread(s)**

* SOFA file reading + parsing
* HRIR extraction + preprocessing
* FFT partition precomputation (heavy)
* libmysofa interpolation precomputation / coordinate mapping
* Headphone EQ preset loading, biquad coefficient updates

**Audio Thread (processBlock)**

* Read atomics (az/el, enable flags, gains)
* Pull the active engine pointer (already built)
* Run: input → headphone EQ (optional) → binaural convolution → output
* If engine swap is pending, crossfade old/new engines for N blocks

This keeps the audio thread “dumb and fast.”

---

## Data flow in the real-time renderer

### Stage order (recommended)

1. **Input mono/stereo handling**

   * Decide: mono input only (simpler) or downmix if stereo
2. **Headphone EQ** (optional, WH-1000XM5 preset)

   * Apply EQ **pre-HRTF** to avoid mangling interaural cues after binauralization
3. **Binaural HRTF**

   * HRIR selection/interpolation (control-rate)
   * Partitioned convolution (audio-rate)
4. **Output safety**

   * Gain trim / limiter if you must (prefer fixed headroom + normalization offline)
   * Output stereo

---

## Engine state design (what gets swapped atomically)

### `SpatialEngineState` (immutable once built)

Contains everything needed to run without allocations:

* `sampleRate`, `blockSize`, `maxBlockSize`
* `hrtfSetId`, `subjectId`
* `directionTable` (unit vectors for each measurement point)
* Convolver kernels:

  * partitioned FFT kernels for left/right for each direction (if nearest-neighbor)
  * OR a smaller set + interpolation strategy (if libmysofa-based)
* Convolver working buffers (preallocated):

  * overlap-add buffers
  * FFT scratch buffers
* Crossfade parameters (if doing seamless engine swap)

**Build-time only:**

* SOFA parsing objects
* Temporary arrays
* FFT plan creation

**Runtime only:**

* Plain arrays, fixed buffers, FFT plans already created

### Atomic pointer swap

* Audio thread reads: `std::atomic<SpatialEngineState*> active;`
* Background builds: `std::unique_ptr<SpatialEngineState> next;`
* Commit via:

  * store `pending = next.release()` atomically
  * audio thread picks it up at block boundary and begins crossfade swap

---

## Parameter transport: what must be lock-free

Use atomics for high-rate controls:

* `azimuthRadians` (atomic float)
* `elevationRadians` (atomic float)
* `enableEQ` (atomic bool)
* `enableSpatial` (atomic bool)
* `outputGain` (atomic float)
* `headphonePresetId` (atomic int) — switching triggers off-thread rebuild or coefficient swap

For JUCE, `AudioProcessorValueTreeState` is fine for UI binding, but in `processBlock`:

* do **not** call anything that allocates or locks
* read cached atomics that are updated in `parameterChanged(...)`

---

## Real-time HRIR updates: how to avoid zipper noise

You have two separate “switching” problems:

### A) Direction changes (source moves)

* If nearest-neighbor: HRIR can jump → audible discontinuity
* Solution: **crossfade between convolution outputs** or **crossfade kernels**

Practical approach:

* Maintain two convolution paths (A and B)
* When direction index changes:

  * load B kernels (already precomputed)
  * ramp crossfade over `N` samples (e.g., 256–2048 depending on motion speed)
  * swap roles

This is CPU-expensive but stable.

### B) Engine changes (subject/profile changes)

Same mechanism, but longer crossfade is acceptable (e.g., 50–200 ms).

---

## Partitioned convolution specifics (JUCE constraints)

### Must-haves

* Fixed partition size (power of two): 128 or 256 typically
* Precompute FFT of each partition of HRIR left/right
* Use overlap-add buffers sized to max block + FFT size
* No per-block kernel FFT work

### Latency

* Latency is at least one partition (often more depending on implementation).
* Implement `getLatencySamples()` properly and call `setLatencySamples()` when engine changes.

### Tail

* HRIR length implies a tail. Make `getTailLengthSeconds()` conservative.

---

## Headphone EQ integration (WH-1000XM5) in JUCE

### Two modes of EQ switching

1. **Coefficient swap (fast)**

   * Precompute biquad coefficients for each preset
   * On preset change: atomically swap a small coefficient struct (no allocations)
2. **Rebuild (slow, but acceptable if rare)**

   * If you’re changing an entire EQ graph, build off-thread and swap

Recommendation: do (1). EQ is tiny compared to convolution.

### ANC on/off preset strategy

* Treat ANC state as part of the headphone profile:

  * `xm5_anc_on`
  * `xm5_anc_off`
* Don’t auto-detect at first. Make it explicit UI control. (You can get clever later.)

---

## JUCE class layout (practical)

### `LocusQAudioProcessor`

* Owns atomics and `AudioProcessorValueTreeState`
* Owns `EngineManager` (background thread + atomic state pointers)
* In `prepareToPlay`: request initial engine build
* In `processBlock`: run `activeEngine->process(...)`

### `EngineManager`

* Thread(s) for building
* Methods:

  * `requestBuild(BuildRequest req)` (non-blocking)
  * `tryCommit()` (called by audio thread at safe point)
* Holds:

  * `std::atomic<SpatialEngineState*> active`
  * `std::atomic<SpatialEngineState*> pending`

### `SpatialEngineState`

* `processBlock(AudioBuffer<float>& in, AudioBuffer<float>& out, Params p)`
* Contains:

  * `EQProcessor` (biquads)
  * `BinauralConvolver` (partitioned)

### `BinauralConvolver`

* Preallocated buffers
* Kernel sets
* Crossfade logic
* Optional: two-path A/B for direction changes

---

## What changes when you introduce libmysofa interpolation

### Key shift

You stop treating HRTF as a discrete set of points you snap to; you treat it as a continuous function over direction.

Real-time implications:

* Kernel update frequency becomes a control-rate process (e.g., every block or every few blocks)
* You still must avoid expensive work per block:

  * Best approach: use libmysofa to compute interpolated HRIR **in a worker thread**, then swap kernels + crossfade

Pragmatic architecture:

* Audio thread publishes “desired az/el”
* Worker thread periodically:

  * queries libmysofa
  * generates interpolated HRIR
  * partitions + FFTs HRIR into a “pending kernel set”
* Audio thread atomically picks up kernel set and crossfades

This avoids libmysofa calls and FFT work on the audio thread.

---

## Integration with LocusQ “profiles” concept

Keep these separate in state:

### `UserSpatialProfile`

* subject ID (SADIE subject or later morph params)
* ear embedding hash + confidence
* any ML/morph coefficients (future)

### `HeadphoneProfile`

* model id (WH-1000XM5)
* mode (ANC on/off)
* parametric EQ preset id + coefficients
* optional future: per-user corrections (seal compensation)

### `RenderConfig`

* current az/el source direction
* enable flags
* output gain trim

These map cleanly to your earlier separation rule: **HRTF profile ≠ headphone EQ profile**.

---

## Real-time acceptance criteria (what “done” means)

### Stability

* No allocations in audio callback (verify with allocator tracking in debug)
* No locks in audio callback (audit + thread sanitizer)
* CPU stable across automation sweeps of az/el

### Audio quality

* No zipper noise when moving source (direction updates)
* No clicks when changing subject/profile (engine swap crossfade)
* No clipping on standard test pack

### Host integration

* Correct latency reporting
* Correct state save/restore (profiles + presets)
* Sample rate changes rebuild engine safely

---

## Minimal “Phase C” path in JUCE (aligned to your plan)

When your listening loop is proven, implement in this order:

1. **Real-time partitioned convolution with crossfade** (still nearest-neighbor, precomputed kernels)
2. **Interpolated HRIR via libmysofa** (worker thread kernel generation + audio thread crossfade)
3. **Multi-mode headphone presets** (ANC on/off) with coefficient swap

That sequencing keeps risk contained: each step improves one dimension without multiplying unknowns.

---

If you want this turned into a JUCE-oriented Markdown document you can drop into the repo (e.g., `docs/DSP_REALTIME_SPATIAL_CONSTRAINTS.md`) with a strict “DSP invariants” section (suitable for your QA harness rules), I can generate that next and include a checklist you can run during code review.
