Title: DSP Invariant Contract — Spatial Integrity (LocusQ QA Harness)
Document Type: Engineering Contract
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# DSP Invariant Contract — Spatial Integrity (LocusQ QA Harness)

**Document type:** Engineering Contract (DSP invariants + automated validation plan)  
**Target:** JUCE plugin spatial renderer (SOFA/libmysofa HRTF, headphone EQ module, partitioned convolution + crossfades)  
**Audience:** DSP + QA harness implementers  
**Principle:** If the harness can’t detect regressions automatically, the renderer will drift into “sounds fine on my headphones” territory.

---

## 1) Scope and Terms

### 1.1 Pipeline Under Test (PUT)

The PUT is the **binaural rendering chain**:

1. Input conditioning (mono or stereo downmix)
2. **Headphone EQ** (optional; model-based preset)
3. **Binaural HRTF** (HRIR selection or interpolation)
4. Partitioned convolution and overlap-add
5. Crossfades for direction changes and engine swaps
6. Output safety gain/limiting (if any)

### 1.2 What “Spatial Integrity” Means Here

Spatial integrity is preserved when the renderer:

- Produces **consistent binaural cues** (ITD/ILD + spectral cues) for a given direction
- Changes **smoothly** when direction or profile changes (no zipper/click artifacts)
- Maintains **symmetry** properties (left/right mirror behavior)
- Remains **deterministic** and **bounded** (no clipping, stable latency, no denormals)
- Keeps EQ and HRTF modules **separable** (EQ doesn’t silently alter spatial mapping)

This contract defines invariants and how the QA harness validates them.

---

## 2) Contract Interface: What the Harness Requires from the Plugin

The harness needs a controllable “test mode” interface. Provide one of:

- A headless CLI renderer binary (preferred early)
- A VST3/AU test host wrapper (later)
- A JUCE unit-test target that calls the internal DSP engine directly (best long-term)

### 2.1 Required Controls (Setters)

The harness must be able to set:

- `sampleRate`, `blockSize` (prepareToPlay / restart)
- `HRTFProfile`:
  - `subjectId` (or `sofaAssetId`)
  - optional `enableInterpolation` (nearest vs interpolated)
- `HeadphoneProfile`:
  - `presetId` (e.g., xm5_anc_on, xm5_anc_off)
  - `enableEQ` boolean
- `RenderParams`:
  - `azimuthDeg` in [-180, 180]
  - `elevationDeg` in [-90, 90] (or your dataset limits)
  - `distance` (optional; if unsupported, fix at 1.0)
  - `outputGainDb`
- `CrossfadeParams`:
  - `dirChangeXfadeMs`
  - `engineSwapXfadeMs`

### 2.2 Required Observability (Getters / Metadata)

The harness must be able to read:

- `latencySamples`
- `activeSubjectId`, `activePresetId`
- `hrtfMode` (nearest / interpolated)
- `engineVersionHash` (commit hash or build ID)
- `profileHash` (hash of profile JSON + sofa asset hash)
- Optional debug counters:
  - `numEngineSwaps`
  - `numDirChanges`
  - `numXRunsDetected` (if you implement)

### 2.3 Test Signals Contract

The PUT must accept:

- Mono WAV input (preferred canonical tests)
- Optional: stereo input (for downmix invariant tests)

The PUT must return:

- Stereo output (L/R) with exact length and documented latency behavior.

---

## 3) Canonical Test Pack (Deterministic Stimuli)

All tests operate on a shared stimulus set. Store it in-repo and version it.

### 3.1 Stimuli (minimum)

1. **Impulse**: 1-sample impulse at t=0 (for HRIR extraction)
2. **Log sweep**: 20 Hz → 20 kHz, 2–5 s (for FR)
3. **Pink noise**: 10 s (for stable stats)
4. **Band-limited noise**: 500–4k, 10 s (for ITD/ILD stability)
5. **Speech**: 10 s spoken phrase (for subjective correlation; optional in automated metrics)
6. **Step azimuth pan**: sequence of static blocks (for discontinuity detection)
7. **Continuous pan**: slowly varying azimuth (for zipper detection)

### 3.2 Canonical Directions (minimum grid)

- Front: (0°, 0°)
- Left: (90°, 0°)
- Right: (-90°, 0°)
- Rear: (180°, 0°)
- Up: (0°, +30°)
- Down: (0°, -30°)

Optionally add 45° diagonals for richer coverage.

---

## 4) Invariants and Automated Validations

Each invariant includes:

- **Invariant statement**
- **Validation method**
- **Threshold** (tunable; start strict, relax only with evidence)
- **Failure output** (what to report)

> Important: thresholds below are designed to catch regressions, not to “prove perceptual correctness.” They are guardrails.

---

### INV-001 — Determinism (Bitwise/Approx Repeatability)

**Statement:** With the same inputs, profiles, SR/BS, the output is repeatable within numerical tolerance.

**Validation:**
- Render the same test twice in the same process.
- Compare outputs.

**Threshold:**
- If using float32 DSP: max abs diff ≤ 1e-6 (offline) or ≤ 1e-5 (real-time)  
- RMS diff ≤ -120 dBFS (offline) or ≤ -100 dBFS (real-time)

**Failure report:**
- maxDiff, rmsDiff, firstMismatchSample

---

### INV-002 — No Clipping / Bounded Output

**Statement:** Output never exceeds [-1, 1] unless explicitly allowed (it shouldn’t be).

**Validation:**
- Scan peaks for all stimuli/directions.

**Threshold:**
- Peak ≤ -0.1 dBFS (recommended headroom)  
- Hard fail if peak > 0 dBFS (|x| > 1.0)

**Failure report:**
- peakValue, sampleIndex, condition metadata

---

### INV-003 — Latency Reporting Correctness

**Statement:** Reported latency equals measured latency.

**Validation:**
- Feed impulse, measure peak index in output relative to input.
- Compare to `latencySamples`.

**Threshold:**
- |measured - reported| ≤ 1 sample

**Failure report:**
- reportedLatency, measuredLatency

---

### INV-004 — HRIR Extraction Consistency (Impulse Response Sanity)

**Statement:** With an impulse input and EQ disabled, output is the HRIR pair for that direction (up to known latency and normalization).

**Validation:**
- Render impulse at direction D.
- Window output around expected latency and compare to stored “golden HRIR” for baseline subject OR compare properties:
  - energy, length, early peak timing.

**Threshold:**
- Energy error ≤ 0.5 dB
- Early peak (first significant peak) within ±2 samples
- Optional direct waveform match: normalized correlation ≥ 0.98

**Failure report:**
- corr, energyDbErr, peakShift

---

### INV-005 — Left/Right Mirror Symmetry (Azimuth Reflection)

**Statement:** For symmetric HRTF sets, mirroring azimuth produces swapped channels.

**Validation:**
- Render at (+θ, 0°) and (-θ, 0°)
- Compare: L(+θ) ≈ R(-θ) and R(+θ) ≈ L(-θ)

**Threshold:**
- Channel-swap correlation ≥ 0.98 (impulse-derived HRIR or sweep-derived response)
- ILD band differences ≤ 1 dB (see INV-007)

**Failure report:**
- corrLR, corrRL, worstBandIldDiff

> Note: Some datasets or ear asymmetry can break perfect symmetry. For “generic baseline” tests, enforce symmetry. For personalized (ear-matched) subjects, downgrade to a warning unless the dataset guarantees symmetry.

---

### INV-006 — ITD Monotonicity vs Azimuth (Horizontal Plane)

**Statement:** Estimated interaural time difference (ITD) changes sign appropriately and grows in magnitude as |azimuth| increases (on 0° elevation).

**Validation (robust):**
- Use band-limited noise (500–4k).
- Compute ITD via GCC-PHAT (generalized cross-correlation with phase transform).
- Test azimuth set: 0°, 30°, 60°, 90°.

**Threshold:**
- ITD(0°) within ±50 µs of 0
- ITD sign: positive for left (90°), negative for right (-90°) (or vice versa—pick convention and freeze it)
- |ITD| increases monotonically with |azimuth| (allow 1 small violation due to dataset quirks)

**Failure report:**
- itdByAzimuth, signErrors, monotonicityViolations

---

### INV-007 — ILD Directionality (Bandwise Interaural Level Difference)

**Statement:** Interaural level difference (ILD) behaves consistently:
- Near 0 at front
- Large magnitude at hard left/right
- Band-dependent (more ILD at high frequencies), but directionality must hold.

**Validation:**
- Use log sweep or pink noise.
- Compute short-time RMS or spectral magnitude per band (e.g., 1/3 octave bands).
- ILD = 20 log10( RMS_L / RMS_R )

**Threshold (example bands):**
- At (0°,0°): median |ILD| across bands ≤ 1 dB
- At (90°,0°): median ILD across bands ≥ +3 dB (left louder than right) in HF bands (≥2 kHz)
- At (-90°,0°): median ILD ≤ -3 dB in HF bands

**Failure report:**
- ildBandsFront, ildBandsLeft, ildBandsRight

---

### INV-008 — Spectral Cue Stability (DTF-ish Guardrail)

**Statement:** For a fixed direction, the spectral coloration is stable across blocks and independent of block size.

**Validation:**
- Render pink noise at direction D for 10s under two block sizes (e.g., 128 and 512).
- Compute averaged magnitude spectra for L/R.
- Compare.

**Threshold:**
- Mean absolute spectral deviation ≤ 0.75 dB from 200 Hz–16 kHz
- Max deviation ≤ 2.0 dB (excluding <200 Hz where room/headphone variance dominates)

**Failure report:**
- meanDevDb, maxDevDb, worstFreq

---

### INV-009 — Direction-Update Continuity (No Zipper Noise)

**Statement:** When azimuth changes gradually, output changes smoothly (no clicks/steps).

**Validation:**
- Continuous pan stimulus: az(t) ramps from -90° to +90° over 10s.
- Compute frame-to-frame spectral flux OR click detector on derivative (|Δx| spikes).

**Threshold:**
- No impulses/clicks above -40 dBFS in derivative detector
- Spectral flux ≤ baselineFlux × 1.5

**Failure report:**
- clickCount, maxClickDb, fluxRatio

---

### INV-010 — Engine Swap Crossfade Integrity

**Statement:** Swapping HRTF subject/profile produces no discontinuity (click) and completes within configured crossfade time.

**Validation:**
- Render steady pink noise.
- At t=2s, trigger engine swap from Subject A → B (off-thread swap path).
- Detect discontinuities and verify crossfade window.

**Threshold:**
- No discontinuity spikes above -40 dBFS
- Swap completes within `engineSwapXfadeMs ± 20%`
- Post-swap steady-state within 500 ms

**Failure report:**
- spikeDb, swapDurationMs, settleTimeMs

---

### INV-011 — EQ/HRTF Separation (EQ Shouldn’t Move the Source)

**Statement:** Enabling headphone EQ must not significantly change ITD; it may change ILD slightly but should not invert cues.

**Validation:**
- Render band-limited noise for several directions with EQ off/on.
- Measure ITD (GCC-PHAT) and ILD bands.

**Threshold:**
- ITD difference EQ on/off ≤ 50 µs for all tested directions
- ILD band sign must remain consistent (left stays left, etc.)
- Allow ILD magnitude changes ≤ 2 dB median

**Failure report:**
- itdDiffs, ildSignFlips, ildMedianDiff

---

### INV-012 — Numerical Stability (Denormals / NaNs / Infs)

**Statement:** DSP must not output NaN/Inf and must not enter denormal slow-path.

**Validation:**
- Scan outputs for isnan/isinf
- Optional: run with very low-level input (-120 dBFS noise) and measure CPU or insert denormal guards

**Threshold:**
- Zero NaN/Inf occurrences
- Denormal guard enabled (e.g., JUCE ScopedNoDenormals)

**Failure report:**
- nanCount, infCount, firstIndex

---

## 5) Golden References vs Property-Based Testing

Use **goldens** only where stable and intended:

- Baseline subject HRIR (impulse outputs) for a small direction set
- Known frequency response (sweep output) for baseline subject

Prefer **property-based invariants** elsewhere:

- ITD/ILD monotonicity and sign
- Mirror symmetry
- Continuity and swap crossfade checks

This avoids brittle tests when you change implementation details but preserve perceptual behavior.

---

## 6) Test Matrix (Minimum Required)

### 6.1 Conditions

Run every invariant under:

- Sample rates: 44.1k, 48k (minimum)
- Block sizes: 128, 256, 512 (minimum)
- Modes:
  - HRTF nearest
  - HRTF interpolated (when available)
- EQ:
  - off
  - WH-1000XM5 preset on

### 6.2 Directions

At minimum: {0, ±30, ±60, ±90, 180} × {0, ±30}

---

## 7) Harness Output Schema (JSON)

The harness should emit machine-readable results to make regressions actionable.

```json
{
  "run_id": "uuid",
  "engine_version": "gitsha-or-buildid",
  "profile_hash": "sha256",
  "sample_rate": 48000,
  "block_size": 256,
  "hrtf_mode": "nearest|interpolated",
  "eq_preset": "none|xm5_anc_on|xm5_anc_off",
  "tests": [
    {
      "id": "INV-006",
      "pass": true,
      "metrics": {
        "itd_us": {"az_0": 12, "az_30": 95, "az_60": 180, "az_90": 240},
        "sign_errors": 0,
        "monotonicity_violations": 0
      },
      "artifacts": {
        "plot_paths": [".../itd_curve.png"],
        "wav_paths": []
      }
    }
  ]
}
```

---

## 8) JUCE Integration Notes (How to Make This Testable)

### 8.1 Provide a “Renderer Core” That’s Host-Agnostic

Put the spatial engine behind an interface callable by:
- unit tests (C++)
- a minimal command-line app
- the plugin AudioProcessor

The harness should prefer the command-line app early; it removes host variability.

### 8.2 Expose a Test Mode for Engine Swaps

Engine swap tests (INV-010) require a deterministic trigger. Provide:
- A test API to request a subject swap at a specific sample time
- Or a parameter that the harness automates precisely

### 8.3 Avoid Flaky Tests

- Freeze random seeds
- Log all parameter changes with sample index
- Use fixed CPU flags if possible

---

## 9) “Red Flag” Failures (Immediate Blockers)

Any of the following is an automatic fail:

- INV-001 determinism failure beyond tolerance
- INV-002 clipping
- INV-003 latency mismatch
- INV-009 zipper/click artifacts
- INV-010 engine swap click
- INV-012 NaN/Inf

These indicate broken fundamentals, not “tuning.”

---

## 10) Roadmap: How Invariants Evolve with Features

### When you add libmysofa interpolation
- Tighten INV-009 thresholds (continuity should improve)
- Add a new invariant: **INV-013 Interpolation Smoothness**:
  - HRIR kernel changes should be smooth in L2 norm between adjacent directions

### When you go real-time partitioned convolution
- Tighten INV-003 latency correctness
- Add **INV-014 CPU Bound** (optional if you can measure):
  - rendering time per block under target budget

### When you add per-headphone ANC modes
- Expand INV-011 separation checks per preset
- Add **INV-015 Preset Switching Click-Free**:
  - toggling EQ preset must not click

---

## 11) Recommended Default Thresholds (Starting Point)

These values are “strict but survivable.” Adjust only after you have baseline data.

- Determinism max diff: 1e-5 (RT)
- Peak headroom: -0.1 dBFS
- ITD EQ sensitivity: 50 µs
- Mirror correlation: 0.98
- Continuity click threshold: -40 dBFS
- Spectral mean deviation: 0.75 dB

---

## 12) What This Contract Buys You

- Direction mapping regressions get caught (ITD/ILD invariants)
- Crossfade mistakes get caught (continuity/swap invariants)
- EQ “oops we changed spatial cues” gets caught (separation invariant)
- Latency mistakes get caught (latency invariant)
- You can iterate implementation details without breaking the spatial “truth” constraints

That’s what industrial QA looks like for spatial audio: not perfection—**bounded correctness with fast regression detection**.
