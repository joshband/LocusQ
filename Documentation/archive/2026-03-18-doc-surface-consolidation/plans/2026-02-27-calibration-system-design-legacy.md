Title: LocusQ Calibration System Design
Document Type: Design Document
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# LocusQ Calibration System Design

## Status

**Approved** — supersedes BL-033 and BL-034 as the canonical calibration architecture reference.
Backlog items BL-052 through BL-061 derive from this document.

---

## 1. Objective

Integrate robust headphone calibration and binaural monitoring into LocusQ for:

- Quadraphonic spatialization inside a DAW
- Stereo headphone playback with personalized binaural rendering
- Head-tracked binaural monitoring (AirPods Pro 1/2/3)
- Model-based headphone EQ (AirPods Pro 1/2/3, Sony WH-1000XM5)
- Speaker layout virtualization (quad bed → accurate binaural)

Primary constraints:

- No allocations or locks in `processBlock()`
- Deterministic state and QA harness compatibility
- Measurable perceptual improvement: ≥20% mean externalization gain or p<0.05 localization improvement (Phase B gate)

---

## 2. The Two-Profile Model (Non-Negotiable)

Every device maps to exactly two orthogonal profiles. They must never be combined into one blob.

```
User Profile (HRTF)              Headphone Profile (EQ / HpTF)
─────────────────────            ──────────────────────────────
Derived from ear/head geometry   Derived from headphone model measurements
SADIE II subject match (v1)      RBJ biquad PEQ preset per model/mode
SOFA file reference              WH-1000XM5: ANC-on / ANC-off
One per person, any headphones   One per headphone model, any person
```

Separation is enforced at the `CalibrationProfile.json` schema level.

---

## 3. System Architecture

### 3.1 Boundary Diagram

```
┌─────────────────────────────────────────────┐
│  LocusQ HeadTracking Companion (macOS)       │
│                                             │
│  ┌─────────────┐   ┌───────────────────┐   │
│  │ Ear Photo   │   │ Headphone Model   │   │
│  │ Capture     │   │ Selector          │   │
│  │ (SADIE II   │   │ (AirPods Pro 1/2/3│   │
│  │  nearest-   │   │  WH-1000XM5       │   │
│  │  neighbor)  │   │  + others)        │   │
│  └──────┬──────┘   └────────┬──────────┘   │
│         │                   │               │
│  ┌──────▼───────────────────▼──────────┐   │
│  │       CalibrationProfile.json        │   │
└──┴──────────────────┬──────────────────┴───┘
                       │ write on setup / update
                       ▼
┌─────────────────────────────────────────────┐
│  LocusQ Plugin (JUCE / APVTS)               │
│                                             │
│  processBlock()                             │
│  ┌──────────────────────────────────────┐  │
│  │ Spatial Renderer → quad bed output   │  │
│  │        ↓                             │  │
│  │ SteamAudioVirtualSurround            │  │
│  │  (quad → stereo binaural)            │  │
│  │  [speaker layout virtualization]     │  │
│  │        ↓                             │  │
│  │ PEQ Cascade (RBJ biquads, ≤8 bands)  │  │
│  │        ↓                             │  │
│  │ FIR Engine (direct ≤256 / FFT >256)  │  │
│  │        ↓                             │  │
│  │ Stereo output                        │  │
│  └──────────────────────────────────────┘  │
│                                             │
│  head_pose (quaternion) ──► yaw offset      │
│  [from companion via IPC]                   │
└─────────────────────────────────────────────┘
```

### 3.2 Monitoring Mode Switch

| Mode | Path |
|---|---|
| `speakers` | Pass-through; no binaural processing |
| `steam_binaural` | Quad bed → SteamAudioVirtualSurround → PEQ → FIR → stereo |
| `virtual_binaural` | Bypass Steam Audio; multichannel to system; optional PEQ |

Mode persists in `CalibrationProfile.json` and the CALIBRATE panel WebView UI.

### 3.3 RT Safety Invariants

Enforced across all DSP backlog items. Checked in every QA acceptance matrix.

1. No heap allocation in `processBlock()`
2. No mutex lock/unlock in `processBlock()`
3. No file I/O in `processBlock()`
4. All coefficient and engine updates via atomic pointer swap on non-RT thread
5. Latency reported to host via `setLatencySamples()` on every FIR engine change
6. `NaN`/`Inf` output classified as error class `non_finite_metric` (BL-038 taxonomy)

---

## 4. Device Matrix

### 4.1 v1 Scope

| Device | Head Tracking | HRTF Personalization | Headphone EQ | Modes |
|---|---|---|---|---|
| AirPods Pro 1 | ✓ CMHeadphoneMotionManager | ✓ ear-photo nearest-neighbor | Model EQ (Pro 1 preset) | ANC on/off |
| AirPods Pro 2 | ✓ | ✓ | Model EQ (Pro 2 preset) | ANC on/off/transparency |
| AirPods Pro 3 | ✓ | ✓ | Model EQ (Pro 3 preset) | ANC on/off/transparency |
| Sony WH-1000XM5 | ✗ | ✓ SOFA baseline | Model EQ (preset in POC) | ANC on/off |
| Generic / custom | ✗ | ✓ SOFA baseline | Off or user-defined PEQ | — |

### 4.2 Future (v2+, documented, not in v1 scope)

AirPods 1/2/3/4 — model EQ only, no spatial audio, no HRTF. Stubbed as planned expansions.

### 4.3 AirPods Pro Version Detection

All three Pro variants use `CMHeadphoneMotionManager` with identical API. Companion detects variant from Bluetooth device name / Core Bluetooth `modelNumber` characteristic and auto-selects preset.

- **Pro 1**: ANC on/off only; older driver tuning.
- **Pro 2**: Adds H2 chip, transparency mode; ANC/transparency preset split required.
- **Pro 3**: Extends Pro 2; requires separate measurement once hardware is broadly available.

---

## 5. Profile Schemas

### 5.1 Headphone Preset (per model/mode)

YAML format, same as existing `sony_wh1000xm5_autoeq_oratory.yaml`:

```yaml
hp_model_id: airpods_pro_2
hp_mode: anc_on
preamp_db: -4.5
filters:
  - {type: PK, fc_hz: 200, gain_db: -2.1, q: 1.2}
  # up to 8 bands
```

One file per `(model, mode)` pair. Stored in companion app bundle and validated against a frequency sweep check.

### 5.2 User Profile

Written by companion after ear-photo HRTF matching:

```json
{
  "schema": "locusq-user-profile-v1",
  "subject_id": "H3",
  "similarity_score": 0.87,
  "embedding_hash": "sha256:abc...",
  "sofa_ref": "sadie2/H3_HRIR.sofa",
  "created_at": "2026-02-27T00:00:00Z"
}
```

`subject_id` maps to a SADIE II subject. Plugin holds a SOFA cache keyed by `sofa_ref` hash — no reloads on repeated opens.

### 5.3 CalibrationProfile.json (plugin handoff contract)

Schema: `locusq-calibration-profile-v1`

```json
{
  "schema": "locusq-calibration-profile-v1",
  "user": {
    "subject_id": "H3",
    "sofa_ref": "sadie2/H3_HRIR.sofa",
    "embedding_hash": "sha256:abc..."
  },
  "headphone": {
    "hp_model_id": "airpods_pro_2",
    "hp_mode": "anc_on",
    "hp_eq_mode": "peq",
    "hp_peq_bands": [],
    "hp_fir_taps": [],
    "hp_hrtf_mode": "sofa"
  },
  "tracking": {
    "hp_tracking_enabled": true,
    "hp_yaw_offset_deg": 0.0
  },
  "verification": {
    "externalization_score": null,
    "front_back_confusion_rate": null,
    "localization_accuracy": null,
    "preference_score": null
  }
}
```

`verification` fields are `null` until Phase B listening tests complete. Plugin surfaces non-null scores in the CALIBRATE panel.

Plugin state mapping:
- Primitive params → APVTS parameters
- `sofa_ref`, `hp_fir_taps` → base64 blobs in state

---

## 6. Implementation Tracks

### 6.1 Track 1 — Plugin DSP Chain (JUCE)

| Stage | Deliverable | Depends On |
|---|---|---|
| T1-S1 | `QuadSpeakerLayout.h` — `QuadOrder` enum, `fillQuadSpeakerLayout()`, default L/R/Ls/Rs | — |
| T1-S2 | `SteamAudioVirtualSurround` — init/shutdown/reset, quad→stereo processBlock, atomic HRTF swap | T1-S1 |
| T1-S3 | Monitoring path switch — `speakers` / `steam_binaural` / `virtual_binaural` in PluginProcessor | T1-S2 |
| T1-S4 | Head tracking injection — `IPLCoordinateSpace3` from bridge, yaw offset, null fallback | T1-S3 |
| T1-S5 | PEQ cascade integration — `PeqBiquadCascade` bound to `hp_eq_mode`, off-thread coefficient update | T1-S3 |
| T1-S6 | `DirectFirConvolver` — time-domain, ≤256 taps, zero latency | — |
| T1-S7 | `PartitionedFftConvolver` — uniform overlap-add, P=nextPow2(blockSize), precomputed partitions, latency=P | T1-S6 |
| T1-S8 | `FirEngineManager` — tap-count selection, atomic swap, `setLatencySamples()` | T1-S6, T1-S7 |
| T1-S9 | State migration — bump `state_version`, add hp params, regenerate goldens | T1-S5, T1-S8 |
| T1-S10 | Latency validation QA — reported latency matches engine; reset on bypass | T1-S8 |

### 6.2 Track 2 — Companion + Profiles

| Stage | Deliverable | Depends On |
|---|---|---|
| T2-S1 | SADIE II baseline renderer (offline) — SOFA loader, nearest-direction HRIR, FFT convolution, WAV export | — |
| T2-S2 | `CalibrationProfile.json` schema lock — shared type definitions (Swift + C++) | — |
| T2-S3 | WH-1000XM5 preset validation — add ANC-on/off split, frequency sweep check | T2-S1 |
| T2-S4 | AirPods Pro 1/2/3 EQ presets — one YAML per (model, mode), AutoEq/oratory1990 source | T2-S1 |
| T2-S5 | Companion device detection — Bluetooth model string → preset selection, ANC mode toggle | T2-S4 |
| T2-S6 | Ear-photo capture UI — guided left/right ear + frontal, quality gates | T2-S2 |
| T2-S7 | SADIE II nearest-neighbor matching — MobileNetV3 embedding, cosine similarity, <50ms | T2-S6 |
| T2-S8 | Profile write → companion IPC → plugin read — companion writes profile, plugin loads on change | T2-S2, T2-S5, T2-S7 |

### 6.3 Integration Handoff (sequential, after both tracks green)

| Stage | Deliverable |
|---|---|
| I-1 | Wire `CalibrationProfile.json` into APVTS — primitives via params, blobs via state |
| I-2 | SOFA HRTF loading in plugin — `libmysofa` load from profile ref, atomic swap into SteamAudioVirtualSurround |
| I-3 | CALIBRATE panel WebView UI — active device, EQ mode, personalization status, verification scores |
| I-4 | End-to-end smoke test — AirPods Pro 2 + WH-1000XM5 profiles load; monitoring chain processes without glitches |

---

## 7. Phase Gates

### Phase B Gate (listening tests — sequential, after I-4)

| Stage | Deliverable |
|---|---|
| B-1 | 2×2 condition pack — generic HRTF vs personalized, with/without EQ; randomized blind playback |
| B-2 | ≥5 participants, ≥10 scenes each — externalization (1–5), front/back confusion, localization accuracy, preference |
| B-3 | Statistical analysis — paired t-test; write scores to `verification` fields in profile |

**Hard gate:** ≥20% mean externalization improvement OR p<0.05 localization gain.
If not met: improve ear feature extraction (embedding model, capture quality, SADIE subject pool expansion) before Phase C. Do not proceed to interpolation/ML.

### Phase C (conditional on Phase B gate pass)

| Stage | Deliverable |
|---|---|
| C-1 | HRIR interpolation — replace nearest-neighbor with `libmysofa` continuous az/el interpolation |
| C-2 | Crossfaded filter updates — no zipper artifacts on HRIR switch under source movement |
| C-3 | AirPods 1/2/3/4 EQ presets — model EQ only, no spatial; expands device library |
| C-4 | Expanded device library — community-sourced presets, validation pipeline |

---

## 8. Backlog Mapping

| BL ID | Title | Track | Depends On |
|---|---|---|---|
| BL-038 | Calibration threading and telemetry *(contract done)* | — | — |
| BL-046 | SOFA HRTF binaural expansion *(existing)* | T2 | BL-038 |
| BL-052 | Steam Audio virtual surround + quad layout | T1: S1–S3 | BL-038 |
| BL-053 | Head tracking orientation injection | T1: S4 | BL-052, BL-045 |
| BL-054 | PEQ cascade RT integration | T1: S5 | BL-052 |
| BL-055 | FIR convolution engine | T1: S6–S8 | — |
| BL-056 | Calibration state migration + latency contract | T1: S9–S10 | BL-054, BL-055 |
| BL-057 | Device preset library (AirPods Pro 1/2/3 + WH-1000XM5) | T2: S3–S5 | BL-046 |
| BL-058 | Companion profile acquisition UI + HRTF matching | T2: S6–S8 | BL-057 |
| BL-059 | CalibrationProfile integration handoff | Integration | BL-052–058 |
| BL-060 | Phase B listening test harness + evaluation | Phase B gate | BL-059 |
| BL-061 | HRTF interpolation + crossfade *(Phase C, conditional)* | Phase C | BL-060 gate pass |

### Dependency Graph

```
BL-038 (done) ──────────────────► BL-052 ──► BL-053
                                        │
BL-046 (existing) ──► BL-057 ──► BL-058 ──► BL-059 ◄── BL-054
                                             │         │
BL-045 (done) ──► BL-053 ──────────────────┤      BL-055 ──► BL-056
                                             │
                                        BL-060 (Phase B gate)
                                             │
                                        BL-061 (conditional)
```

---

## 9. Key Acceptance IDs

| BL | Acceptance ID | Threshold |
|---|---|---|
| BL-052 | quad→binaural renders correctly | no RT allocation in processBlock |
| BL-052 | monitoring mode switch deterministic | speakers path unchanged |
| BL-055 | FIR latency reported = engine latency | direct/partitioned switch atomic |
| BL-056 | state migration idempotent | golden snapshots regenerated |
| BL-059 | profile load/unload cycle stable | SOFA swap without glitches |
| BL-060 | **Phase B gate** | ≥20% externalization gain OR p<0.05 localization |

---

## 10. HRTF Dataset and Tooling

- **SADIE II** (Apache 2.0) — SOFA HRTFs + hi-res ear images + 3D scans. Primary dataset.
- **libmysofa** — C library for SOFA HRIR loading and interpolation (Phase C+).
- **AutoEq / oratory1990** — headphone frequency response corrections for model-based EQ presets.
- **HUTUBS / SONICOM** — Phase C+ expansion datasets for HRTF morphing and ML personalization.

Coordinate convention (SADIE II, must be preserved end-to-end):
- Azimuth: 0° front; positive = anti-clockwise (listener's left)
- Elevation: positive above horizontal plane

---

## 11. Privacy Constraint

Ear/face imagery is biometric-adjacent data.

- All ear-photo processing: on-device in companion app only.
- Images are not stored after embedding computation.
- Embedding hash stored in user profile for reproducibility; original image discarded.
- User profile sync (if added): end-to-end encrypted, explicit user consent.

---

## 12. References

- `Documentation/Calibration POC/LocusQ Headphone Calibration Research Outline.md`
- `Documentation/Calibration POC/HRTF and Personalized Headphone Calibration.md`
- `Documentation/Calibration POC/LocusQ Spatial Personalization Phase-Gated Execution Plan.md`
- `Documentation/Calibration POC/locusq_spatial_audio_spec.md`
- `Documentation/Calibration POC/locusq_spatial_prototype/`
- `Documentation/backlog/bl-038-calibration-threading-and-telemetry.md`
- `Documentation/plans/bl-033-headphone-calibration-core-spec-2026-02-25.md`
- `Documentation/plans/bl-034-headphone-calibration-verification-spec-2026-02-25.md`
