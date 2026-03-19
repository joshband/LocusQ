Title: LocusQ Spatial Personalization
Document Type: Calibration POC Note
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

Excellent. This is the correct sequencing discipline. Most spatial audio projects fail because they jump to interpolation and ML before proving the loop.

Below is a **Phase-Gated Engineering Plan** aligned exactly to your 3-step directive. This is not a wish list. This is an execution contract.

---

# LocusQ Spatial Personalization

## Phase-Gated Execution Plan (Hard Technical Milestones)

---

# PHASE A — Ship Baseline Renderer + Test Harness First

## A1. Select Generic Baseline Subject

**Decision:**
Pick a single SADIE II subject (e.g., median anthropometric subject or arbitrary stable ID like `H3`).

**Rationale:**
You need a deterministic reference. Changing baseline subjects during experimentation corrupts perceptual comparisons.

### Acceptance Criteria

* Baseline subject ID hardcoded in config.
* All test renders reproducible from:

  * SOFA hash
  * subject ID
  * azimuth/elevation
  * seed
* Baseline frequency response measured and archived.

---

## A2. Lock Offline Rendering Architecture

**Scope**

* SOFA loader
* Nearest-direction HRIR selection
* FFT convolution
* Deterministic WAV export
* Optional WH-1000XM5 EQ toggle

### Engineering Constraints

* No real-time path yet.
* No interpolation.
* No dynamic HRIR switching.
* No ML.

### Acceptance Criteria

* Render 5 canonical scenes:

  * Front center (0°, 0°)
  * Hard left (90°, 0°)
  * Hard right (-90°, 0°)
  * Elevated (0°, 30°)
  * Rear (180°, 0°)
* Output verified audibly different across directions.
* No clipping at -3 dBFS input.

---

## A3. Ship Test Harness (Before Personalization)

You cannot evaluate improvement without a measurement harness.

### Required Components

* Stimulus pack (normalized)
* 2-condition generator:

  * Generic HRTF
  * Generic + EQ
* Randomized playback order
* Data logger

### Acceptance Criteria

* Participant session generates:

  * CSV log
  * condition label
  * rating values
* At least 3 internal test runs completed.
* Logs archived with commit hash.

---

## PHASE A EXIT GATE

You proceed only if:

* Renderer is stable.
* Baseline listening experience is coherent (front ≠ rear, left ≠ right).
* EQ toggle does not break localization.

No personalization work begins before this gate.

---

# PHASE B — Add Ear-Photo Nearest Neighbor + Run 2×2

This is where most teams lose discipline. Don’t.

---

## B1. Implement Ear-Photo → SADIE Subject Match

**Important:**
This is subject selection only. No HRIR morphing.

### Engineering Steps

1. Define capture protocol (left ear, right ear, frontal).
2. Normalize image size.
3. Extract embedding (pretrained CNN is fine).
4. Precompute SADIE subject embeddings.
5. Compute cosine similarity.
6. Select best match.

### Acceptance Criteria

* Profile JSON contains:

  * subject_id
  * similarity_score
  * embedding_hash
* Same image always yields same subject.
* Matching takes < 50ms.

---

## B2. Implement 2×2 Condition Generator

Now the real experiment begins.

Conditions:

1. Generic HRTF
2. Personalized HRTF
3. Generic + EQ
4. Personalized + EQ

### Requirements

* Randomized playback order.
* Blind labeling (A/B/X).
* No UI bias.
* Logged condition mapping.

---

## B3. Run Controlled Listening Tests

Minimum viable evaluation:

* ≥ 5 participants
* ≥ 10 scenes each
* Collect:

  * Externalization (1–5)
  * Front/back confusion
  * Localization accuracy
  * Preference

### Statistical Test

* Paired t-test:

  * Generic vs Personalized
  * With EQ vs Without EQ
* Report p-value.
* Report effect size.

---

# PHASE B EXIT GATE (CRITICAL)

Personalization pipeline continues ONLY if:

At least one of:

* ≥ 20% mean improvement in externalization
* Statistically significant localization improvement (p < 0.05)

If not:

* Improve ear feature extraction.
* Increase SADIE subject pool.
* Improve matching metric.
* DO NOT PROCEED TO ML.

This gate prevents months of wasted morphing work.

---

# PHASE C — Upgrade Only After Proof

Now, and only now, you harden the system.

---

## C1. Switch Nearest Neighbor → Interpolated HRIR (libmysofa)

### Why

Nearest neighbor causes spatial discontinuities when source moves.

### Requirements

* Continuous azimuth interpolation.
* Elevation interpolation.
* No zipper artifacts.

### Acceptance Criteria

* Smooth panning test produces no audible jumps.
* Crossfade between filters < 10ms.
* CPU cost measured and logged.

---

## C2. Move Offline → Real-Time Partitioned Convolution

### Architecture Requirements

* Partitioned FFT convolution
* Overlap-add
* Crossfaded HRIR switching
* No memory allocations in audio callback
* Precomputed FFT blocks

### Acceptance Criteria

* Stable at 128-sample buffer
* CPU < 15% (target machine)
* No glitching under HRIR changes

---

## C3. Expand Headphone EQ Modes (WH-1000XM5)

Sony has multiple states:

* ANC on
* ANC off
* Transparency
* Seal variation

### Engineering Plan

* Create preset bank:

  * xm5_anc_on
  * xm5_anc_off
* Add runtime toggle.
* Add profile metadata for headphone state.

### Acceptance Criteria

* Switching EQ modes does not destabilize renderer.
* Frequency sweep verifies correct filter activation.
* Listening tests confirm no localization regression.

---

# System Architecture Integrity Rules (Non-Negotiable)

1. HRTF profile and Headphone profile are separate modules.
2. No DSP allocations in real-time path.
3. All personalization reproducible from profile JSON.
4. Baseline generic profile never modified.
5. Every listening test tied to commit hash.

---

# Risk Map

### Risk: Personalization effect too small

Mitigation:

* Multi-view ear capture
* Use both ears independently
* Add frontal face geometry weighting

### Risk: EQ alters localization cues

Mitigation:

* Apply EQ before HRIR convolution
* Validate ILD/ITD preservation

### Risk: Overfitting to SADIE dataset bias

Mitigation:

* Add HUTUBS or SONICOM expansion in later phase

---

# Timeline Reality Check

Phase A: 1–2 weeks
Phase B: 2–3 weeks
Evaluation: 1 week
Phase C: 3–6 weeks

If Phase B fails, you saved yourself 6–8 weeks of ML thrashing.

---

# The Core Discipline

You are not building “AI spatial audio.”
You are building:

Measurement → Improvement → Verification → Upgrade.

Most audio teams skip the verification step and ship placebo personalization.

You’re not doing that.

If you’d like, I can now:

* Convert this into GitHub Milestones + Issue Templates for LocusQ
* Or map this directly into JUCE real-time architecture constraints for plugin integration
* Or design the DSP invariant contract so your QA harness can validate spatial integrity automatically

That’s where this becomes industrial-grade.
