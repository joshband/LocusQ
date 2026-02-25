Title: LocusQ Headphone Calibration Research Outline
Document Type: Research Outline
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

Good instinct. Codex 5.3 will behave much better if you hand it a structured problem statement instead of a giant transcript.

Below is a **clean technical outline** of what we designed and why — trimmed to the parts that matter for implementation inside `joshband/LocusQ`.

---

# LocusQ Headphone Calibration & Binaural Monitoring – Research Outline

## 1. Objective

Integrate robust **headphone calibration + binaural monitoring** into LocusQ for:

* Quadraphonic spatialization inside a DAW
* Stereo headphone playback
* Optional head tracking
* Support for:

  * Default HRTF
  * Custom SOFA HRTF
  * Headphone EQ (PEQ + FIR)
  * Internal Steam Audio binauralization

Primary constraint:

* No allocations or locks in `processBlock`
* Deterministic state
* QA harness compatible

---

# 2. Architectural Decision: Monitoring Strategy

## 2.1 Current Internal Format

Likely:

* Per-source spatial engine
* Quadraphonic bed output (L, R, Ls, Rs)
* Not currently Ambisonics

## 2.2 Wrapper Variant Selection (Opinionated)

Chosen for smallest surgery:

### ✅ Steam Audio Virtual Surround (quad bed → binaural)

Why:

* No renderer rewrite required
* Works with existing quad output
* Compatible with SOFA HRTFs
* Supports head orientation
* Clean monitoring adapter layer

Deferred:

* Ambisonics decode path
* Per-source binaural path

---

# 3. Quad Speaker Layout Mapping

## 3.1 Assumed Channel Order

Default:

* L, R, Ls, Rs

Alternate orders supported via enum:

* L_R_Rs_Ls
* Ls_Rs_L_R
* Custom

## 3.2 Speaker Geometry (Coordinate System)

Right-handed:

* +X = right
* +Y = up
* -Z = forward

Vectors:

* L:  (-0.5, 0, -0.866)
* R:  ( 0.5, 0, -0.866)
* Ls: (-0.94, 0,  0.34)
* Rs: ( 0.94, 0,  0.34)

Purpose:

* Accurate binauralization of quad bed
* Prevent channel order mismatch bugs

---

# 4. SteamAudioVirtualSurround Wrapper

## 4.1 Responsibilities

* Create Steam Audio context bindings
* Create quad `IPLSpeakerLayout`
* Create `IPLVirtualSurroundEffect`
* Manage HRTF lifecycle (default + SOFA)
* Accept listener orientation
* Process quad → stereo
* Be fully RT-safe

## 4.2 Lifecycle

Non-RT:

* `init()`
* `setHrtfFromSofaFile()`
* `setDefaultHrtf()`
* `reset()`
* `shutdown()`

RT:

* `processBlock()`

  * No allocations
  * Atomic pointer swap for HRTF/effect

## 4.3 Head Tracking Integration

Orientation fed via:

* `IPLCoordinateSpace3`
* Apply yaw offset
* External smoothing performed outside audio thread

---

# 5. Headphone Calibration System

## 5.1 Components

HeadphoneCalibrationProfile contains:

* hp_enabled
* hp_tracking_enabled
* hp_yaw_offset_deg
* hp_eq_mode (off | peq | fir)
* hp_peq_band_count
* hp_peq band params (up to 8)
* hp_fir_taps + latency
* hp_hrtf_mode (default | sofa)
* verification scores
* blobs:

  * hp_sofa_ref
  * hp_fir_coeffs_f32

## 5.2 State Strategy

* Primitive parameters via APVTS
* Large data (FIR taps, SOFA ref) as base64 blobs
* Deterministic migration
* QA harness compatible

---

# 6. Parametric EQ (PEQ)

## 6.1 Implementation

* Direct Form I biquad
* RBJ cookbook coefficients
* Up to 8 bands
* Per-channel cascades

## 6.2 RT Safety

* Coefficients built non-RT
* Double-bank atomic swap
* No per-sample allocations
* Denormal protection

---

# 7. FIR Convolution Engine Strategy

## 7.1 Engine Selection (Opinionated Threshold)

* 0 taps → bypass
* ≤ 256 taps → direct time-domain
* > 256 taps → partitioned FFT

## 7.2 Direct Convolver

* O(N × taps)
* Zero algorithmic latency
* Suitable for short IRs

## 7.3 Partitioned FFT Convolver

Uniform partitioned overlap-add.

Parameters:

* P = nextPow2(blockSize)
* FFT size = 2P
* K = ceil(taps / P)

Runtime:

1. FFT input block
2. Multiply-accumulate partitions
3. IFFT
4. Overlap-add
5. Output

Latency:

* ≈ P samples
* Report via `setLatencySamples()`

## 7.4 RT Safety

* All FFT plans + buffers preallocated
* Partition spectra precomputed
* Engine swap via atomic pointer

---

# 8. Final Monitoring Chain (Headphone Mode)

When:
`cal_monitoring_path == steam_binaural`

Pipeline:

1. Internal spatial renderer
2. Quad bed output
3. SteamAudioVirtualSurround
4. PEQ (if enabled)
5. FIR (if enabled)
6. Stereo output

When:
`virtual_binaural`

* Skip Steam Audio
* Pass multichannel to system
* Optional PEQ

---

# 9. Latency Management

Must explicitly track:

* FIR latency
* Report to host
* No hidden delay
* No surprise drift between monitoring modes

---

# 10. Future Extensions (Not Required for v1)

* Ambisonics internal format
* Per-source Steam Audio path
* Crossfaded EQ changes
* Zero-latency non-uniform partitioned convolution
* Personalized HRTF generation
* Device profile auto-matching

---

# 11. Implementation Priorities (Recommended Order)

1. SteamAudioVirtualSurround (quad → binaural)
2. PEQ cascade (done)
3. FIR engine selection + convolver
4. Head tracking orientation injection
5. Verification test harness
6. QA migration + golden state update

---

Below is a **condensed Codex task brief** suitable for GPT-5.3 execution inside `joshband/LocusQ`.

It is implementation-oriented, dependency-aware, and ordered.

---

# LocusQ – Headphone Calibration & Binaural Monitoring

## Codex Task Brief (Execution Plan)

---

### 1. Add Quad Speaker Layout Mapping

* Implement `QuadOrder` enum.
* Implement `fillQuadSpeakerLayout(IPLSpeakerLayout&, QuadOrder)`.
* Default to `L, R, Ls, Rs`.
* Add unit test validating channel index → direction mapping.

---

### 2. Implement SteamAudioVirtualSurround Wrapper

* Create `SteamAudioVirtualSurround` class.
* Responsibilities:

  * init / shutdown
  * reset
  * SOFA HRTF swap (non-RT)
  * quad → stereo processBlock (RT safe)
* Ensure atomic pointer swap for effect + HRTF.
* No allocations inside `processBlock`.

---

### 3. Integrate Monitoring Path Switch

* Modify monitoring stage:

  * `speakers` → pass-through
  * `steam_binaural` → route quad bed into wrapper
  * `virtual_binaural` → bypass wrapper
* Publish requested/active diagnostics for CALIBRATE UI.

---

### 4. Add Head Tracking Injection Point

* Accept `IPLCoordinateSpace3` from bridge layer.
* Apply yaw offset (`hp_yaw_offset_deg`) before passing to Steam Audio.
* Do not perform smoothing inside audio thread.
* Add defensive null/orientation fallback.

---

### 5. Finalize PEQ Cascade Integration

* Use existing `PeqBiquadCascade`.
* Bind to `hp_eq_mode` + band parameters.
* Ensure coefficient updates happen off audio thread.
* Insert PEQ after binaural stage.

---

### 6. Implement Direct FIR Convolver

* Time-domain convolution.
* No heap allocations in processBlock.
* Suitable for ≤ 256 taps.
* Latency = 0.

---

### 7. Implement Partitioned FFT FIR Convolver

* Uniform partitioned overlap-add.
* Partition size `P = nextPow2(blockSize)`.
* Precompute partition spectra off-thread.
* Maintain ring buffer of input spectra.
* Latency = P samples.
* Report latency via host API.

---

### 8. Add FIR Engine Selection Logic

* 0 taps → bypass
* ≤ 256 taps → direct convolver
* > 256 taps → partitioned FFT
* Swap engine instances atomically.
* Store/report `hp_fir_latency_samples`.

---

### 9. Integrate FIR Stage into Monitoring Chain

Final chain when `steam_binaural`:

1. Quad bed
2. SteamAudioVirtualSurround
3. PEQ (if enabled)
4. FIR (if enabled)
5. Stereo out

Ensure deterministic stage order.

---

### 10. Add State Version Migration

* Bump `state_version` to next minor.
* Add new headphone params to migration rules.
* Regenerate golden snapshots.
* Add idempotence migration test.

---

### 11. Add Basic Verification Test Mode

* Implement internal test tone routing:

  * front impulse
  * rear impulse
* Validate channel routing and orientation behavior.
* Save verification scores to state.

---

### 12. Add Latency Reporting & Validation

* Call `setLatencySamples()` when FIR changes.
* Add QA test verifying reported latency matches engine.
* Ensure latency resets properly on bypass.

---

### 13. Add Runtime Safety Guards

* Assert no allocations in processBlock (debug instrumentation).
* Add null-effect fallback (silence output if misconfigured).
* Clamp extreme yaw offsets.
* Protect against invalid SOFA load.

---

### 14. Add Quad Channel Order Diagnostic

* Temporary debug toggle to solo each quad channel.
* Verify expected spatial position in binaural output.
* Prevent silent channel order bugs.

---

### 15. Document Monitoring Architecture

* Add `MONITORING_ARCHITECTURE.md`:

  * Quad → Binaural path
  * Head tracking flow
  * FIR strategy
  * Latency rules
* Keep aligned with QA harness documentation.

---

# Execution Order (Recommended)

1 → 2 → 3 → 5 → 6 → 7 → 8 → 9 → 4 → 10 → 12 → 11 → 13 → 14 → 15

---

Below is a **structured JSON project spec** designed for Codex (GPT-5.3) staged execution inside `joshband/LocusQ`.

It is:

* Stage-driven
* Dependency-aware
* Build-safe (incremental)
* Explicit about RT constraints
* Explicit about QA + state migration

You can paste this directly into Codex as the project execution plan.

---

```json
{
  "project": {
    "name": "LocusQ Headphone Calibration & Binaural Monitoring",
    "repo": "joshband/LocusQ",
    "primary_goal": "Implement quad-bed to binaural monitoring with Steam Audio, PEQ, FIR calibration, and RT-safe architecture.",
    "constraints": {
      "real_time_safe": true,
      "no_allocations_in_processBlock": true,
      "no_locks_in_processBlock": true,
      "state_migration_required": true,
      "qa_harness_compatible": true
    }
  },

  "architecture": {
    "monitoring_chain": [
      "Spatial Renderer (quad bed)",
      "SteamAudioVirtualSurround (optional)",
      "PEQ (optional)",
      "FIR Convolver (optional)",
      "Stereo Output"
    ],
    "monitoring_modes": [
      "speakers",
      "steam_binaural",
      "virtual_binaural"
    ],
    "quad_channel_order_default": "L_R_Ls_Rs"
  },

  "stages": [

    {
      "id": "S1",
      "name": "Quad Speaker Layout Mapping",
      "objective": "Implement IPLSpeakerLayout mapping for quad bed.",
      "tasks": [
        "Create QuadOrder enum.",
        "Implement fillQuadSpeakerLayout().",
        "Default to L_R_Ls_Rs.",
        "Add debug validation for channel order."
      ],
      "deliverables": [
        "QuadSpeakerLayout.h",
        "Unit test for channel mapping"
      ],
      "build_should_pass": true
    },

    {
      "id": "S2",
      "name": "SteamAudioVirtualSurround Wrapper",
      "objective": "Implement quad → stereo binaural wrapper.",
      "dependencies": ["S1"],
      "tasks": [
        "Create SteamAudioVirtualSurround class.",
        "Implement init(), shutdown(), reset().",
        "Implement processBlock() (RT-safe).",
        "Add atomic pointer swap for effect + HRTF."
      ],
      "constraints": [
        "No allocations in processBlock.",
        "No locks in processBlock."
      ],
      "deliverables": [
        "SteamAudioVirtualSurround.h/.cpp"
      ],
      "build_should_pass": true
    },

    {
      "id": "S3",
      "name": "Monitoring Path Integration",
      "objective": "Integrate wrapper into monitoring switch.",
      "dependencies": ["S2"],
      "tasks": [
        "Add steam_binaural path.",
        "Route quad bed into wrapper.",
        "Ensure speakers path remains unchanged.",
        "Expose diagnostics (requested vs active mode)."
      ],
      "deliverables": [
        "Updated monitoring stage implementation"
      ],
      "build_should_pass": true
    },

    {
      "id": "S4",
      "name": "Head Tracking Injection",
      "objective": "Inject orientation into Steam Audio.",
      "dependencies": ["S3"],
      "tasks": [
        "Accept IPLCoordinateSpace3 input.",
        "Apply yaw offset parameter.",
        "Ensure orientation fallback if unavailable.",
        "Do not smooth inside audio thread."
      ],
      "deliverables": [
        "Orientation injection in monitoring stage"
      ],
      "build_should_pass": true
    },

    {
      "id": "S5",
      "name": "PEQ Integration",
      "objective": "Insert PEQ cascade after binaural stage.",
      "tasks": [
        "Integrate PeqBiquadCascade.",
        "Bind parameters from state.",
        "Ensure coefficient updates off-thread.",
        "Confirm no RT allocations."
      ],
      "deliverables": [
        "PEQ stage in monitoring chain"
      ],
      "build_should_pass": true
    },

    {
      "id": "S6",
      "name": "Direct FIR Convolver",
      "objective": "Implement time-domain FIR engine.",
      "tasks": [
        "Implement DirectFirConvolver.",
        "Support multi-channel stereo.",
        "Zero allocations in processBlock.",
        "Latency = 0."
      ],
      "deliverables": [
        "DirectFirConvolver.h/.cpp"
      ],
      "build_should_pass": true
    },

    {
      "id": "S7",
      "name": "Partitioned FFT FIR Convolver",
      "objective": "Implement uniform partitioned overlap-add engine.",
      "dependencies": ["S6"],
      "tasks": [
        "Choose partition size P = nextPow2(blockSize).",
        "Precompute partition spectra.",
        "Implement ring buffer for input spectra.",
        "Implement overlap-add.",
        "Expose latencySamples()."
      ],
      "constraints": [
        "All FFT buffers allocated off-thread.",
        "No dynamic memory in processBlock."
      ],
      "deliverables": [
        "PartitionedFftConvolver.h/.cpp"
      ],
      "build_should_pass": true
    },

    {
      "id": "S8",
      "name": "FIR Engine Selection",
      "objective": "Switch engine based on tap count.",
      "dependencies": ["S6", "S7"],
      "tasks": [
        "Implement engine factory.",
        "≤256 taps → Direct.",
        ">256 taps → Partitioned.",
        "Swap engines atomically.",
        "Report latency to host."
      ],
      "deliverables": [
        "FIR engine manager class"
      ],
      "build_should_pass": true
    },

    {
      "id": "S9",
      "name": "State Migration Update",
      "objective": "Add headphone parameters to state version.",
      "tasks": [
        "Bump state_version.",
        "Update migration_rules.json.",
        "Add golden snapshots.",
        "Add idempotence migration test."
      ],
      "deliverables": [
        "Updated migration files",
        "New golden state files"
      ],
      "build_should_pass": true
    },

    {
      "id": "S10",
      "name": "Latency Validation",
      "objective": "Ensure correct latency reporting.",
      "dependencies": ["S8"],
      "tasks": [
        "Call setLatencySamples() on FIR change.",
        "Add QA test verifying latency.",
        "Ensure bypass resets latency."
      ],
      "deliverables": [
        "Latency test cases"
      ],
      "build_should_pass": true
    },

    {
      "id": "S11",
      "name": "Verification Test Mode",
      "objective": "Add basic internal spatial verification.",
      "tasks": [
        "Implement front impulse test.",
        "Implement rear impulse test.",
        "Store verification score in state."
      ],
      "deliverables": [
        "Verification utility code"
      ],
      "build_should_pass": true
    },

    {
      "id": "S12",
      "name": "Documentation",
      "objective": "Document monitoring architecture.",
      "tasks": [
        "Create MONITORING_ARCHITECTURE.md.",
        "Document quad mapping.",
        "Document FIR latency strategy.",
        "Document RT constraints."
      ],
      "deliverables": [
        "Documentation file"
      ],
      "build_should_pass": true
    }
  ],

  "success_criteria": [
    "Quad bed renders correctly to binaural.",
    "SOFA HRTF loads without RT allocation.",
    "PEQ and FIR operate without audio glitches.",
    "Latency correctly reported and tested.",
    "QA harness passes migration + golden tests."
  ]
}
```

---
Excellent. Now we shift from “feature add” to “surgical refactor planning.”
This is about not destabilizing LocusQ while introducing a new monitoring spine.

Below are two deliverables:

1. **Phased Refactor Roadmap aligned to a typical LocusQ JUCE + QA harness repo layout**
2. **Parallel multi-agent Codex execution plan (optimized for GPT-5.3 sessions)**

---

# PART I — Phased Refactor Roadmap

*(Aligned to LocusQ structure: dsp/, monitoring/, state/, qa/, ui/)*

I’ll assume a conventional structure similar to what you’ve described in earlier LocusQ work:

```
/Source
  /dsp
  /monitoring
  /state
  /calibrate
  /ui
  PluginProcessor.*
  PluginEditor.*

/state
  migration_rules.json
  goldens/

/qa
  audio-dsp-qa-harness submodule
```

If your exact folder names differ, the conceptual boundaries still apply.

---

# PHASE 0 — Architectural Freeze (No Behavior Change)

### Goal:

Prepare repo for monitoring refactor without changing audio output.

### Actions:

* Introduce `/monitoring` folder.
* Extract current monitoring logic from `PluginProcessor` into:

  * `MonitoringStage.h/.cpp`
* Introduce enum:

  * `MonitoringMode { Speakers, SteamBinaural, VirtualBinaural }`
* Wire pass-through only.

### Why:

We isolate monitoring before injecting Steam Audio.

**Build must behave identically to current main branch.**

---

# PHASE 1 — Quad Layout Canonicalization

### Goal:

Define quad bed once and forever.

### Add:

```
/monitoring/QuadSpeakerLayout.h
```

Includes:

* `QuadOrder enum`
* `fillQuadSpeakerLayout()`
* Debug assertion utility

### Also:

* Add temporary debug test to confirm channel ordering.

### Risk mitigated:

Silent channel-order mismatch.

---

# PHASE 2 — Steam Audio Virtual Surround Integration

### Add:

```
/monitoring/SteamAudioVirtualSurround.h/.cpp
```

Responsibilities:

* init / shutdown
* default HRTF
* SOFA HRTF swap
* processBlock(quad → stereo)
* atomic pointer swap

### Modify:

* `MonitoringStage`

  * If `mode == SteamBinaural`, route quad bed here.

### RT constraints:

* No allocations in processBlock
* No locks
* No file I/O

### Build checkpoint:

Steam Audio path compiles but may not yet be enabled by UI.

---

# PHASE 3 — Head Tracking Injection

### Modify:

* `/monitoring/MonitoringStage`
* Add `OrientationProvider` interface

Inject:

* Quaternion → `IPLCoordinateSpace3`
* Yaw offset application

Keep smoothing in bridge layer (not audio thread).

---

# PHASE 4 — PEQ Refactor Integration

### Add:

```
/dsp/PeqBiquadCascade.h
```

Insert stage in MonitoringStage:

```
if (hp_eq_mode == PEQ)
    peq.processBlock()
```

Coefficient updates:

* Off-thread only
* Double-buffer swap

Checkpoint:
Audio identical when disabled.

---

# PHASE 5 — FIR Infrastructure

Split cleanly:

```
/dsp/fir/IFirConvolver.h
/dsp/fir/DirectFirConvolver.h
/dsp/fir/PartitionedFftConvolver.h
/dsp/fir/FirEngineManager.h
```

### FirEngineManager:

* Chooses engine by tap count
* Owns atomic pointer
* Reports latency

### Modify:

* `MonitoringStage`

  * Insert FIR after PEQ

Checkpoint:
Direct FIR only first.
Then partitioned.

---

# PHASE 6 — State & Migration Update

### Modify:

```
/state/migration_rules.json
/state/goldens/
```

Add:

* hp_enabled
* hp_eq_mode
* hp_hrtf_mode
* hp_fir_taps
* etc.

Run:

* Golden snapshot regeneration
* Idempotence migration test

Checkpoint:
QA harness passes.

---

# PHASE 7 — Latency Contract Enforcement

### Add:

* Host latency reporting hook
* QA test verifying latency changes
* Reset latency on bypass

Critical:
No hidden delay drift between monitoring modes.

---

# PHASE 8 — Verification Utilities

Add:

```
/monitoring/SpatialVerification.h
```

Implements:

* Front impulse
* Rear impulse
* Mono collapse test

Store scores in state.

---

# PHASE 9 — Documentation & Stability Sweep

Add:

```
/docs/MONITORING_ARCHITECTURE.md
```

Include:

* Quad mapping diagram
* Monitoring chain diagram
* FIR decision tree
* Latency rules
* RT safety guarantees

---

# PART II — Parallel Multi-Agent Codex Execution Plan

This is how to avoid stepping on your own feet.

We divide into four parallel tracks:

---

# Track A — Monitoring Core (Agent A)

**Scope**

* Quad mapping
* SteamAudioVirtualSurround
* MonitoringStage refactor

**Constraints**

* No state changes
* No FIR
* No PEQ

**Deliverables**

* Fully functional quad → binaural wrapper
* Clean monitoring stage

Agent A must not touch:

* state/
* dsp/fir/
* qa/

---

# Track B — FIR System (Agent B)

**Scope**

* IFirConvolver
* DirectFirConvolver
* PartitionedFftConvolver
* FirEngineManager

**Constraints**

* Standalone unit test harness
* No dependency on monitoring yet

Deliverable:

* FIR module compiles independently.

Agent B must not modify:

* MonitoringStage
* SteamAudioVirtualSurround

---

# Track C — PEQ & Calibration (Agent C)

**Scope**

* PeqBiquadCascade
* HeadphoneCalibrationProfile codec
* Parameter wiring

**Constraints**

* No Steam Audio changes
* No FIR engine integration yet

Deliverable:

* PEQ stage compiles and processes stereo buffers.

---

# Track D — State & QA (Agent D)

**Scope**

* Migration update
* Golden regeneration
* Latency QA tests

**Constraints**

* No DSP changes
* No Steam Audio edits

Deliverable:

* State tests pass on CI.

---

# Integration Phase (Sequential)

After A/B/C/D are green:

1. Integrate PEQ into MonitoringStage.
2. Integrate FIR engine.
3. Connect state params to monitoring.
4. Enable steam_binaural path.
5. Final latency validation.

---

# Why This Parallel Strategy Works

* Monitoring and FIR are isolated.
* State changes do not block DSP implementation.
* QA harness work does not interfere with audio thread work.
* Each agent works in a bounded domain.

---

# Critical Coordination Rules for Codex

* Each stage must:

  * Compile cleanly
  * Not introduce allocations in processBlock
  * Not introduce new warnings
* No agent edits files outside its declared scope.
* Integration occurs only after all tracks are green.

---
