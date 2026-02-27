Title: LocusQ Spatial Audio Personalization
Document Type: Calibration POC Note
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

You’re thinking correctly: specs are theory; tickets are gravity. Let’s turn this into something that an engineering team (or your AI agent swarm) can execute without ambiguity.

Below is a comprehensive Markdown engineering breakdown aligned to the existing SOFA renderer + personalization spec. It is structured as:

* Milestones (sequenced)
* Epics
* Tickets (with acceptance criteria)
* Definition of Done
* Risk gates
* Measurable exit criteria per phase

You can paste this directly into GitHub Projects, Linear, Jira, or feed it to your agent system.

---

# LocusQ Spatial Audio Personalization

## Engineering Task Breakdown (Tickets, Acceptance Criteria, Sequencing)

**Status:** Planning
**Owner:** LocusQ Core
**Scope:** SOFA-based binaural rendering, ear-photo nearest-neighbor personalization, WH-1000XM5 headphone EQ, controlled listening validation loop

---

# 0. System Overview (Execution Order)

Phase 1 — Core Renderer (Generic HRTF, Offline)
Phase 2 — Headphone EQ Module (Model-Based)
Phase 3 — Ear-Photo Personalization (Nearest Neighbor)
Phase 4 — Controlled Listening Framework
Phase 5 — Quantitative Evaluation + Iteration Gate
Phase 6 — Morphing / ML Personalization (Conditional)

No morphing work begins until Phase 5 exit criteria are met.

---

# Phase 1 — SOFA Renderer (Offline, Deterministic)

## Epic 1.1 — SOFA Loader + HRIR Access

### Ticket 1.1.1 — SOFA File Parser

**Goal:** Load SADIE II SOFA file and expose HRIR + source positions.

**Tasks**

* Parse Data.IR
* Parse SourcePosition
* Validate dimensions
* Extract sampling rate
* Validate azimuth/elevation conventions

**Acceptance Criteria**

* Loads SADIE II SOFA without error
* Returns HRIR array shape: [N, 2, L]
* Returns az/el array shape: [N, 3] or [N, 2]
* Sampling rate matches expected metadata
* Fails loudly on malformed SOFA

**Definition of Done**

* Unit tests pass on at least 1 SADIE subject
* Logs coordinate convention

---

### Ticket 1.1.2 — Nearest-Direction Selector

**Goal:** Map requested (az, el) to nearest HRIR index.

**Tasks**

* Convert az/el to unit vector
* Compute dot-product distance
* Return index of closest source

**Acceptance Criteria**

* For known directions (0°, 90°, etc.), returns expected index
* Unit test verifies spherical symmetry behavior
* Handles edge case (wraparound at ±180°)

---

## Epic 1.2 — Offline Binaural Renderer

### Ticket 1.2.1 — Mono → Stereo Convolution

**Goal:** Convolve mono input with selected HRIR.

**Tasks**

* FFT convolution
* Left/right channel separation
* Gain normalization

**Acceptance Criteria**

* Output WAV is stereo
* Impulse input produces HRIR waveform
* No clipping at nominal level

---

### Ticket 1.2.2 — Renderer CLI Tool

**Goal:** Deterministic offline render tool.

**Inputs**

* SOFA path
* Input WAV
* azimuth
* elevation
* optional EQ preset

**Acceptance Criteria**

* Command-line invocation produces output WAV
* Logs selected HRIR index
* Reproducible across runs

---

### Phase 1 Exit Criteria

* Can render 5 test scenes at arbitrary az/el
* Verified via headphone listening that localization changes with azimuth
* No crashes across 3 SADIE subjects

---

# Phase 2 — Headphone EQ Module (WH-1000XM5 Baseline)

## Epic 2.1 — Parametric EQ Engine

### Ticket 2.1.1 — Biquad Implementation (RBJ)

**Acceptance Criteria**

* Implements PK, LS, HS filters
* Frequency response matches reference curve ±0.5 dB
* Stable under 48kHz + 44.1kHz

---

### Ticket 2.1.2 — Preset Loader

**Goal:** Load YAML/JSON EQ preset.

**Acceptance Criteria**

* Loads AutoEq-derived preset
* Applies preamp
* Order of filters deterministic

---

### Ticket 2.1.3 — EQ Toggle Integration

**Acceptance Criteria**

* Renderer runs with and without EQ
* Difference measurable in frequency response sweep
* No distortion introduced at nominal levels

---

### Phase 2 Exit Criteria

* Pink noise sweep shows correction curve applied
* Localization perceptually unaffected by EQ stage
* EQ can be toggled via CLI

---

# Phase 3 — Ear-Photo Personalization (Nearest Neighbor)

## Epic 3.1 — Capture + Preprocessing

### Ticket 3.1.1 — Capture Protocol Spec

**Goal:** Define capture rules.

Must include:

* Left ear
* Right ear
* Frontal face
* Lighting requirements
* Distance constraints

**Acceptance Criteria**

* Documented protocol
* Example good/bad captures
* Stored as markdown

---

### Ticket 3.1.2 — Ear Cropping Utility

**Goal:** Manual or semi-automatic crop.

**Acceptance Criteria**

* Produces normalized ear image (fixed resolution)
* Rejects images below quality threshold

---

## Epic 3.2 — Embedding + Nearest Neighbor

### Ticket 3.2.1 — Embedding Extraction

**Goal:** Extract feature vector from ear image.

**Acceptance Criteria**

* Deterministic embedding size
* Same image produces same embedding
* Embedding normalized (L2)

---

### Ticket 3.2.2 — SADIE Subject Embedding Index

**Goal:** Precompute embeddings for SADIE ear images.

**Acceptance Criteria**

* Index includes subject ID
* Stored in serialized format
* Lookup < 10ms

---

### Ticket 3.2.3 — Nearest Neighbor Match

**Goal:** Match user ear → SADIE subject.

**Acceptance Criteria**

* Returns subject ID
* Returns similarity score
* Score logged for evaluation

---

### Ticket 3.2.4 — Personalized Render Mode

**Acceptance Criteria**

* Renderer accepts user profile
* Loads matched subject SOFA
* Renders binaural output

---

### Phase 3 Exit Criteria

* User can:

  * Capture ear image
  * Generate profile
  * Render personalized binaural file
* Subjectively noticeable difference from generic profile

---

# Phase 4 — Controlled Listening Framework

## Epic 4.1 — Test Harness

### Ticket 4.1.1 — Stimulus Pack Creation

Include:

* Static azimuth sweep
* Rotating source
* Elevation cues
* Spoken word

**Acceptance Criteria**

* All stimuli normalized
* No clipping

---

### Ticket 4.1.2 — 2×2 Test Matrix Generator

Conditions:

1. Generic HRTF
2. Personalized HRTF
3. Generic + EQ
4. Personalized + EQ

**Acceptance Criteria**

* Randomized presentation order
* Double-blind file naming
* Metadata log generated

---

### Ticket 4.1.3 — Rating Interface (Minimal CLI)

Collect:

* Externalization (1–5)
* Localization accuracy
* Front/back confusion
* Preference

**Acceptance Criteria**

* Data saved to CSV
* Unique participant ID
* Timestamped

---

### Phase 4 Exit Criteria

* At least 5 participants complete full matrix
* Data stored in structured format
* No condition bias detected

---

# Phase 5 — Evaluation Gate

## Epic 5.1 — Quantitative Analysis

### Ticket 5.1.1 — Statistical Comparison

Compare:

* Generic vs Personalized
* EQ vs No EQ

Metrics:

* Mean externalization
* Localization error
* Preference %

**Acceptance Criteria**

* Generates summary table
* Performs paired t-test
* Flags significance (p < 0.05)

---

### Phase 5 Exit Criteria (Hard Gate)

Personalization continues ONLY if:

* ≥ 20% improvement in externalization mean
  OR
* Statistically significant localization improvement

If not met:

* Improve ear embedding or subject selection
* Do NOT proceed to morphing

---

# Phase 6 — Morphing / ML (Conditional)

## Epic 6.1 — PCA HRTF Basis

### Ticket 6.1.1 — HRIR Dimensionality Reduction

**Acceptance Criteria**

* Extract principal components
* Explain ≥ 90% variance
* Stable reconstruction

---

### Ticket 6.1.2 — Geometry → Coefficient Model

**Acceptance Criteria**

* Train regression model
* Cross-validated error below baseline nearest-neighbor

---

### Exit Criteria

* Objective spectral error < nearest neighbor baseline
* Listening test shows additional improvement

---

# Global Definition of Done

System is considered MVP-complete when:

* Offline SOFA rendering stable
* WH-1000XM5 EQ separable and toggleable
* Ear-photo personalization produces measurable improvement
* Controlled listening confirms benefit
* No DSP instability
* All profiles versioned and reproducible

---

# Risks & Mitigation

Risk: Ear image embedding not predictive
Mitigation: Add anthropometric metadata or multi-view images

Risk: Headphone EQ affects localization
Mitigation: Apply EQ pre-HRTF stage only

Risk: Dataset bias (SADIE population skew)
Mitigation: Expand dataset (HUTUBS / SONICOM)

---

# Architectural Integrity Rules

* Headphone profile and HRTF profile must remain separate modules
* All profiles stored in versioned JSON
* Renderer deterministic given same inputs
* No DSP allocations in real-time path (future RT upgrade)

---

# Sequencing Summary

1. SOFA loader
2. Offline renderer
3. EQ module
4. Ear embedding + NN selection
5. 2×2 listening tests
6. Evaluate
7. Only then consider ML morphing

---

This structure prevents you from drifting into ML experimentation before you’ve proven that nearest-neighbor personalization actually improves perception. Most teams skip that step and waste months optimizing noise.

If you want, I can now generate a third document that maps this directly into:

* GitHub Issues with labels
* Milestone mapping
* AI-agent mega-prompts per epic
* A cost-aware execution plan aligned to your typical compute budget

That’s where things get industrial.
