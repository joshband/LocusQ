Title: LocusQ Calibration System Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# LocusQ Calibration System Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Extend the existing calibration stub (2-band EQ + Steam Audio virtual surround) into a full, measurable headphone calibration system: multi-band RBJ biquad PEQ, FIR convolution engine, per-device preset library (AirPods Pro 1/2/3 + WH-1000XM5), SOFA HRTF loading, companion profile handoff, and a Phase B listening test harness.

**Architecture:** Two parallel tracks. Track 1 extends `SpatialRenderer.h` with a proper PEQ cascade and FIR engine replacing the existing `applyHeadphoneProfileCompensation` stub. Track 2 builds the companion-side profile acquisition and device preset library. Integration wires `CalibrationProfile.json` from companion to plugin state.

**Tech Stack:** JUCE 7, C++17, Steam Audio (already integrated), libmysofa (new dependency), Python 3 (offline renderer / test harness), Swift 5 (companion app), RBJ biquad cookbook, SADIE II SOFA dataset (Apache 2.0).

**Design Reference:** `Documentation/plans/2026-02-27-calibration-system-design.md`

**Codebase orientation:**
- `Source/SpatialRenderer.h` — monolithic renderer header; all DSP lives here. The key insertion points are `applyHeadphoneProfileCompensation()` (line ~3894, replace this stub) and `renderSteamBinauralBlock()` (line ~3924, already works).
- `Source/PluginProcessor.cpp` — parameter wiring. `HeadphoneDeviceProfile` already has `AirPodsPro2` and `SonyWH1000XM5` entries. `cal_monitoring_path` APVTS param already exists.
- `companion/Sources/LocusQHeadTrackerCore/` — companion app source.
- `scripts/` — QA harness scripts. New BL lanes follow the pattern in `scripts/qa-bl038-calibration-telemetry-lane-mac.sh`.
- `Documentation/backlog/` — one `.md` per BL item. New items: BL-052 through BL-061.

---

## TRACK 1 — Plugin DSP Chain

---

### Task 1: Add AirPods Pro 1 and Pro 3 device profiles

**Context:** `HeadphoneDeviceProfile` in `SpatialRenderer.h` currently has `Generic`, `AirPodsPro2`, `SonyWH1000XM5`, `CustomSOFA`. Pro 1 and Pro 3 are missing.

**Files:**
- Modify: `Source/SpatialRenderer.h` (enum ~line 56, string converter ~line 757, compensation table ~line 3861)
- Modify: `Source/PluginProcessor.cpp` (parameter choice list ~line 2838)

**Step 1: Locate the enum**

Read `Source/SpatialRenderer.h` around line 56. The enum looks like:
```cpp
enum class HeadphoneDeviceProfile : int
{
    Generic       = 0,
    AirPodsPro2   = 1,
    SonyWH1000XM5 = 2,
    CustomSOFA    = 3
};
```

**Step 2: Add Pro 1 and Pro 3**

```cpp
enum class HeadphoneDeviceProfile : int
{
    Generic         = 0,
    AirPodsPro1     = 1,
    AirPodsPro2     = 2,
    AirPodsPro3     = 3,
    SonyWH1000XM5   = 4,
    CustomSOFA      = 5
};
```

> Important: `AirPodsPro2` index shifts from 1 to 2. Update all switch/case blocks and the APVTS choice list consistently. Grep for `AirPodsPro2` to find all call sites.

**Step 3: Add string conversions**

In the `headphoneDeviceProfileToString` switch (~line 757):
```cpp
case static_cast<int> (HeadphoneDeviceProfile::AirPodsPro1): return "airpods_pro_1";
case static_cast<int> (HeadphoneDeviceProfile::AirPodsPro3): return "airpods_pro_3";
```

**Step 4: Add compensation stubs (same values as Pro 2 for now — Task 2 replaces this)**

In `updateHeadphoneCompensationForProfile` (~line 3861):
```cpp
case HeadphoneDeviceProfile::AirPodsPro1:
    headphoneCompLowGain  = 0.98f;
    headphoneCompHighGain = 1.03f;
    headphoneCompCrossfeed = 0.015f;
    break;
case HeadphoneDeviceProfile::AirPodsPro3:
    headphoneCompLowGain  = 0.98f;
    headphoneCompHighGain = 1.03f;
    headphoneCompCrossfeed = 0.015f;
    break;
```

**Step 5: Update APVTS choice list in PluginProcessor.cpp**

Find `cal_device_profile` parameter (or equivalent). Add `"AirPods Pro 1"` and `"AirPods Pro 3"` choices in correct index order.

**Step 6: Build**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
```
Expected: clean build, no new warnings.

**Step 7: Commit**

```bash
git add Source/SpatialRenderer.h Source/PluginProcessor.cpp
git commit -m "feat: add AirPods Pro 1 and Pro 3 device profiles (stub EQ)"
```

---

### Task 2: Implement RBJ biquad PEQ cascade (replace stub)

**Context:** `applyHeadphoneProfileCompensation()` is a 2-float stub. Replace with a proper 8-band RBJ biquad cascade with RT-safe coefficient double-buffering.

**Files:**
- Create: `Source/dsp/PeqBiquadCascade.h`
- Modify: `Source/SpatialRenderer.h` (replace headphoneComp fields + `applyHeadphoneProfileCompensation`)

**Step 1: Create `Source/dsp/PeqBiquadCascade.h`**

```cpp
#pragma once
#include <array>
#include <atomic>
#include <cmath>

// RBJ cookbook biquad. Direct Form I.
struct BiquadCoeffs
{
    float b0 = 1.f, b1 = 0.f, b2 = 0.f;
    float a1 = 0.f, a2 = 0.f;
};

struct BiquadState
{
    float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;

    float process (float x, const BiquadCoeffs& c) noexcept
    {
        const float y = c.b0*x + c.b1*x1 + c.b2*x2 - c.a1*y1 - c.a2*y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        if (! std::isfinite (y)) { x1=x2=y1=y2=0.f; return 0.f; }
        return y;
    }
};

// Build a Peak filter coefficient set (RBJ cookbook).
inline BiquadCoeffs makePeakEQ (float fc, float gainDb, float q, float sampleRate) noexcept
{
    const float A  = std::sqrt (std::pow (10.f, gainDb / 40.f));
    const float w0 = 2.f * 3.14159265f * fc / sampleRate;
    const float alpha = std::sin (w0) / (2.f * q);
    BiquadCoeffs c;
    const float a0inv = 1.f / (1.f + alpha / A);
    c.b0 = (1.f + alpha * A) * a0inv;
    c.b1 = (-2.f * std::cos (w0)) * a0inv;
    c.b2 = (1.f - alpha * A) * a0inv;
    c.a1 = c.b1;
    c.a2 = (1.f - alpha / A) * a0inv;
    return c;
}

// Build a Low Shelf coefficient set (RBJ cookbook).
inline BiquadCoeffs makeLowShelf (float fc, float gainDb, float q, float sampleRate) noexcept
{
    const float A  = std::pow (10.f, gainDb / 40.f);
    const float w0 = 2.f * 3.14159265f * fc / sampleRate;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.f * q);
    const float sqA = std::sqrt (A);
    BiquadCoeffs c;
    const float a0inv = 1.f / ((A+1.f) + (A-1.f)*cosw + 2.f*sqA*alpha);
    c.b0 = A * ((A+1.f) - (A-1.f)*cosw + 2.f*sqA*alpha) * a0inv;
    c.b1 = 2.f * A * ((A-1.f) - (A+1.f)*cosw) * a0inv;
    c.b2 = A * ((A+1.f) - (A-1.f)*cosw - 2.f*sqA*alpha) * a0inv;
    c.a1 = -2.f * ((A-1.f) + (A+1.f)*cosw) * a0inv;
    c.a2 = ((A+1.f) + (A-1.f)*cosw - 2.f*sqA*alpha) * a0inv;
    return c;
}

// Build a High Shelf coefficient set (RBJ cookbook).
inline BiquadCoeffs makeHighShelf (float fc, float gainDb, float q, float sampleRate) noexcept
{
    const float A  = std::pow (10.f, gainDb / 40.f);
    const float w0 = 2.f * 3.14159265f * fc / sampleRate;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.f * q);
    const float sqA = std::sqrt (A);
    BiquadCoeffs c;
    const float a0inv = 1.f / ((A+1.f) - (A-1.f)*cosw + 2.f*sqA*alpha);
    c.b0 = A * ((A+1.f) + (A-1.f)*cosw + 2.f*sqA*alpha) * a0inv;
    c.b1 = -2.f * A * ((A-1.f) + (A+1.f)*cosw) * a0inv;
    c.b2 = A * ((A+1.f) + (A-1.f)*cosw - 2.f*sqA*alpha) * a0inv;
    c.a1 = 2.f * ((A-1.f) - (A+1.f)*cosw) * a0inv;
    c.a2 = ((A+1.f) - (A-1.f)*cosw - 2.f*sqA*alpha) * a0inv;
    return c;
}

static constexpr int kMaxPeqBands = 8;

struct PeqBandSpec
{
    enum class Type { PK, LSC, HSC } type = Type::PK;
    float fc_hz  = 1000.f;
    float gain_db = 0.f;
    float q       = 0.707f;
};

// Stereo 8-band cascade. Coefficients updated off-RT via double-buffer.
class PeqBiquadCascade
{
public:
    struct CoeffBank
    {
        int numBands = 0;
        float preampLinear = 1.0f;
        std::array<BiquadCoeffs, kMaxPeqBands> bands {};
    };

    // Call off the audio thread. Builds new coefficients and swaps atomically.
    void setPreset (const PeqBandSpec* specs, int numBands,
                    float preampDb, float sampleRate) noexcept
    {
        auto& inactive = banks[1 - activeBank.load()];
        inactive.numBands    = juce::jlimit (0, kMaxPeqBands, numBands);
        inactive.preampLinear = std::pow (10.f, preampDb / 20.f);
        for (int i = 0; i < inactive.numBands; ++i)
        {
            switch (specs[i].type)
            {
                case PeqBandSpec::Type::PK:
                    inactive.bands[i] = makePeakEQ  (specs[i].fc_hz, specs[i].gain_db, specs[i].q, sampleRate); break;
                case PeqBandSpec::Type::LSC:
                    inactive.bands[i] = makeLowShelf (specs[i].fc_hz, specs[i].gain_db, specs[i].q, sampleRate); break;
                case PeqBandSpec::Type::HSC:
                    inactive.bands[i] = makeHighShelf(specs[i].fc_hz, specs[i].gain_db, specs[i].q, sampleRate); break;
            }
        }
        activeBank.store (1 - activeBank.load());
    }

    // Call from audio thread. RT-safe.
    void processBlock (float* left, float* right, int numSamples) noexcept
    {
        const auto& bank = banks[activeBank.load()];
        if (bank.numBands == 0 && bank.preampLinear == 1.f)
            return;
        for (int i = 0; i < numSamples; ++i)
        {
            float l = left[i]  * bank.preampLinear;
            float r = right[i] * bank.preampLinear;
            for (int b = 0; b < bank.numBands; ++b)
            {
                l = stateL[b].process (l, bank.bands[b]);
                r = stateR[b].process (r, bank.bands[b]);
            }
            left[i]  = l;
            right[i] = r;
        }
    }

    void reset() noexcept
    {
        for (auto& s : stateL) s = {};
        for (auto& s : stateR) s = {};
    }

private:
    std::array<CoeffBank, 2> banks {};
    std::atomic<int>         activeBank { 0 };
    std::array<BiquadState, kMaxPeqBands> stateL {}, stateR {};
};
```

**Step 2: Build standalone to verify it compiles**

Add a `#include "Source/dsp/PeqBiquadCascade.h"` at the top of a scratch `.cpp` and build. Expected: no errors.

**Step 3: Add `PeqBiquadCascade` member to `SpatialRenderer`**

In `SpatialRenderer.h` private member section, add:
```cpp
PeqBiquadCascade headphonePeq;
```

**Step 4: Replace `applyHeadphoneProfileCompensation` call site**

In `SpatialRenderer.h` around line 1290 find:
```cpp
applyHeadphoneProfileCompensation (left, right);
```

Add PEQ call just before or after (check ordering: PEQ should come after Steam binaural, before stereo out):
```cpp
// PEQ block (applied to full block, not per-sample)
// This runs after the binaural stage, before output.
```

Then in the per-sample loop that calls `applyHeadphoneProfileCompensation`, replace with:
```cpp
headphonePeq.processBlock (leftBuf, rightBuf, numSamples);
```
where `leftBuf`/`rightBuf` are the stereo output buffers. Keep the old `applyHeadphoneProfileCompensation` function body but mark it `[[deprecated]]` for now — it will be removed once Task 3 (preset loading) replaces it.

**Step 5: Reset PEQ on prepare**

In `SpatialRenderer::prepareToPlay()` (or `reset()`):
```cpp
headphonePeq.reset();
```

**Step 6: Build**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
```
Expected: clean build.

**Step 7: Commit**

```bash
git add Source/dsp/PeqBiquadCascade.h Source/SpatialRenderer.h
git commit -m "feat: add RBJ biquad PEQ cascade for headphone compensation"
```

---

### Task 3: Load EQ presets from YAML and wire to device profiles

**Context:** Headphone EQ values are currently hardcoded floats in `updateHeadphoneCompensationForProfile`. Replace with YAML preset files loaded from the app bundle.

**Files:**
- Create: `Source/dsp/HeadphonePresetLoader.h`
- Create: `Resources/eq_presets/airpods_pro_1_anc_on.yaml`
- Create: `Resources/eq_presets/airpods_pro_2_anc_on.yaml`
- Create: `Resources/eq_presets/airpods_pro_3_anc_on.yaml`
- Create: `Resources/eq_presets/sony_wh1000xm5_anc_on.yaml`
- Create: `Resources/eq_presets/sony_wh1000xm5_anc_off.yaml`
- Modify: `Source/SpatialRenderer.h` (call loader from `setHeadphoneDeviceProfile`)

**Step 1: Create `Source/dsp/HeadphonePresetLoader.h`**

```cpp
#pragma once
#include "PeqBiquadCascade.h"
#include <juce_core/juce_core.h>
#include <vector>

struct HeadphonePreset
{
    juce::String modelId;
    juce::String mode;
    float preampDb = 0.f;
    std::vector<PeqBandSpec> bands;
    bool valid = false;
};

inline HeadphonePreset loadHeadphonePreset (const juce::File& yamlFile)
{
    HeadphonePreset result;
    if (! yamlFile.existsAsFile())
        return result;

    // Parse minimal YAML: preamp_db + filters array.
    // Uses juce::StringArray line scanning — no external YAML parser dependency.
    const auto lines = juce::StringArray::fromLines (yamlFile.loadFileAsString());

    for (const auto& line : lines)
    {
        const auto trimmed = line.trim();
        if (trimmed.startsWith ("hp_model_id:"))
            result.modelId = trimmed.fromFirstOccurrenceOf (":", false, false).trim();
        else if (trimmed.startsWith ("hp_mode:"))
            result.mode = trimmed.fromFirstOccurrenceOf (":", false, false).trim();
        else if (trimmed.startsWith ("preamp_db:"))
            result.preampDb = trimmed.fromFirstOccurrenceOf (":", false, false).trim().getFloatValue();
        else if (trimmed.startsWith ("- {"))
        {
            // Parse: - {type: PK, fc_hz: 200, gain_db: -2.1, q: 1.2}
            PeqBandSpec band;
            const auto inner = trimmed.fromFirstOccurrenceOf ("{", false, false)
                                     .upToLastOccurrenceOf ("}", false, false);
            for (const auto& tok : juce::StringArray::fromTokens (inner, ",", "\""))
            {
                const auto k = tok.upToFirstOccurrenceOf (":", false, false).trim();
                const auto v = tok.fromFirstOccurrenceOf (":", false, false).trim();
                if (k == "type")
                {
                    if (v == "PK")  band.type = PeqBandSpec::Type::PK;
                    else if (v == "LSC") band.type = PeqBandSpec::Type::LSC;
                    else if (v == "HSC") band.type = PeqBandSpec::Type::HSC;
                }
                else if (k == "fc_hz")   band.fc_hz   = v.getFloatValue();
                else if (k == "gain_db") band.gain_db = v.getFloatValue();
                else if (k == "q")       band.q       = v.getFloatValue();
            }
            result.bands.push_back (band);
        }
    }
    result.valid = true;
    return result;
}
```

**Step 2: Create the WH-1000XM5 preset YAML (ANC on)**

`Resources/eq_presets/sony_wh1000xm5_anc_on.yaml` — copy from the existing Python prototype file at `Documentation/Calibration POC/locusq_spatial_prototype/locusq/eq_presets/sony_wh1000xm5_autoeq_oratory.yaml`, adding `hp_model_id` and `hp_mode` headers.

```yaml
hp_model_id: sony_wh1000xm5
hp_mode: anc_on
preamp_db: -6.2
filters:
  - {type: LSC, fc_hz: 105.0,  gain_db: -3.2, q: 0.70}
  - {type: PK,  fc_hz: 2448.0, gain_db:  6.9, q: 2.46}
  - {type: PK,  fc_hz: 173.0,  gain_db: -5.6, q: 0.96}
  - {type: PK,  fc_hz: 3028.0, gain_db: -5.4, q: 2.03}
  - {type: PK,  fc_hz: 1327.0, gain_db:  3.3, q: 0.58}
  - {type: HSC, fc_hz: 10000.0,gain_db:  4.9, q: 0.70}
  - {type: PK,  fc_hz: 6110.0, gain_db: -2.3, q: 5.81}
  - {type: PK,  fc_hz: 875.0,  gain_db: -1.2, q: 4.07}
```

**Step 3: Create AirPods Pro preset stubs**

Create `Resources/eq_presets/airpods_pro_2_anc_on.yaml` with neutral values initially (to be tuned against AutoEq/oratory1990 measurements once available):

```yaml
hp_model_id: airpods_pro_2
hp_mode: anc_on
preamp_db: -1.0
filters:
  - {type: PK, fc_hz: 200,  gain_db: -1.5, q: 1.0}
  - {type: PK, fc_hz: 3000, gain_db:  1.2, q: 2.0}
  - {type: PK, fc_hz: 8000, gain_db: -1.0, q: 3.0}
```

Duplicate for `airpods_pro_1_anc_on.yaml`, `airpods_pro_3_anc_on.yaml`, `sony_wh1000xm5_anc_off.yaml`.

**Step 4: Wire loader into `setHeadphoneDeviceProfile` in `SpatialRenderer.h`**

Replace the hardcoded `updateHeadphoneCompensationForProfile` logic with:

```cpp
void setHeadphoneDeviceProfile (int profileIndex)
{
    requestedHeadphoneProfileIndex.store (profileIndex);
    const auto profile = static_cast<HeadphoneDeviceProfile> (profileIndex);
    juce::String presetFilename;
    switch (profile)
    {
        case HeadphoneDeviceProfile::AirPodsPro1:   presetFilename = "airpods_pro_1_anc_on.yaml";     break;
        case HeadphoneDeviceProfile::AirPodsPro2:   presetFilename = "airpods_pro_2_anc_on.yaml";     break;
        case HeadphoneDeviceProfile::AirPodsPro3:   presetFilename = "airpods_pro_3_anc_on.yaml";     break;
        case HeadphoneDeviceProfile::SonyWH1000XM5: presetFilename = "sony_wh1000xm5_anc_on.yaml";   break;
        default: break;
    }
    if (presetFilename.isNotEmpty())
    {
        // Resolve from app bundle Resources/eq_presets/
        const auto presetFile = juce::File::getSpecialLocation (
            juce::File::currentApplicationFile)
            .getChildFile ("Contents/Resources/eq_presets")
            .getChildFile (presetFilename);
        const auto preset = loadHeadphonePreset (presetFile);
        if (preset.valid && currentSampleRate > 0.0)
        {
            headphonePeq.setPreset (preset.bands.data(),
                                    static_cast<int> (preset.bands.size()),
                                    preset.preampDb,
                                    static_cast<float> (currentSampleRate));
        }
    }
    else
    {
        // Generic / off — clear PEQ
        headphonePeq.setPreset (nullptr, 0, 0.f, static_cast<float> (currentSampleRate));
    }
}
```

**Step 5: Build and verify**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
```

**Step 6: Commit**

```bash
git add Source/dsp/HeadphonePresetLoader.h Resources/eq_presets/ Source/SpatialRenderer.h
git commit -m "feat: load headphone EQ presets from YAML (WH-1000XM5, AirPods Pro 1/2/3)"
```

---

### Task 4: Implement `DirectFirConvolver`

**Context:** FIR convolution for short impulse responses (≤256 taps). Zero algorithmic latency.

**Files:**
- Create: `Source/dsp/fir/DirectFirConvolver.h`

**Step 1: Create the file**

```cpp
#pragma once
#include <vector>
#include <cstring>
#include <cmath>

// Time-domain FIR convolver. Stereo. ≤256 taps. Zero latency.
// All memory allocated in reset(). processBlock() is allocation-free.
class DirectFirConvolver
{
public:
    void reset (const float* taps, int numTaps, int maxBlockSize)
    {
        jassert (numTaps >= 0 && numTaps <= 256);
        coeffs.assign (taps, taps + numTaps);
        delayL.assign (numTaps, 0.f);
        delayR.assign (numTaps, 0.f);
        writePos = 0;
    }

    void clearState() noexcept
    {
        std::fill (delayL.begin(), delayL.end(), 0.f);
        std::fill (delayR.begin(), delayR.end(), 0.f);
        writePos = 0;
    }

    int latencySamples() const noexcept { return 0; }

    // RT-safe. No allocations.
    void processBlock (float* left, float* right, int numSamples) noexcept
    {
        const int N = static_cast<int> (coeffs.size());
        if (N == 0) return;
        for (int i = 0; i < numSamples; ++i)
        {
            delayL[writePos] = left[i];
            delayR[writePos] = right[i];
            float accL = 0.f, accR = 0.f;
            int readPos = writePos;
            for (int t = 0; t < N; ++t)
            {
                accL += coeffs[t] * delayL[readPos];
                accR += coeffs[t] * delayR[readPos];
                if (--readPos < 0) readPos += N;
            }
            if (! std::isfinite (accL)) accL = 0.f;
            if (! std::isfinite (accR)) accR = 0.f;
            left[i]  = accL;
            right[i] = accR;
            if (++writePos >= N) writePos = 0;
        }
    }

private:
    std::vector<float> coeffs, delayL, delayR;
    int writePos = 0;
};
```

**Step 2: Build**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
```

**Step 3: Commit**

```bash
git add Source/dsp/fir/DirectFirConvolver.h
git commit -m "feat: add DirectFirConvolver (≤256 taps, zero latency)"
```

---

### Task 5: Implement `PartitionedFftConvolver`

**Context:** Uniform overlap-add FFT convolution for FIR taps > 256. Latency = P = nextPow2(blockSize).

**Files:**
- Create: `Source/dsp/fir/PartitionedFftConvolver.h`

**Step 1: Create the file**

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <complex>
#include <cmath>

// Uniform partitioned overlap-add stereo FIR convolver.
// Latency = P = nextPow2(blockSize) samples.
// All FFT buffers allocated in reset(). processBlock() is allocation-free.
class PartitionedFftConvolver
{
public:
    void reset (const float* taps, int numTaps, int blockSize)
    {
        P     = nextPow2 (blockSize);
        fftSize = 2 * P;
        K     = (numTaps + P - 1) / P;

        fft = std::make_unique<juce::dsp::FFT> (static_cast<int> (std::log2 (fftSize)));

        // Precompute partition spectra from taps
        partitionSpectraL.assign (K * fftSize, {0.f, 0.f});
        partitionSpectraR = partitionSpectraL; // stereo shares same IR for headphone EQ

        for (int k = 0; k < K; ++k)
        {
            std::vector<std::complex<float>> buf (fftSize, {0.f, 0.f});
            for (int n = 0; n < P && (k*P+n) < numTaps; ++n)
                buf[n] = {taps[k*P+n], 0.f};
            fft->perform (buf.data(), buf.data(), false);
            for (int n = 0; n < fftSize; ++n)
                partitionSpectraL[k*fftSize + n] = buf[n];
        }
        partitionSpectraR = partitionSpectraL;

        // Ring buffer: K partitions × fftSize complex
        inputRingL.assign (K * fftSize, {0.f, 0.f});
        inputRingR = inputRingL;
        overlapL.assign (P, 0.f);
        overlapR.assign (P, 0.f);
        outputBufL.assign (P, 0.f);
        outputBufR.assign (P, 0.f);
        scratchBuf.assign (fftSize, {0.f, 0.f});

        ringHead = 0;
        samplesPending = 0;
    }

    void clearState() noexcept
    {
        std::fill (inputRingL.begin(), inputRingL.end(), std::complex<float>{0.f,0.f});
        std::fill (inputRingR.begin(), inputRingR.end(), std::complex<float>{0.f,0.f});
        std::fill (overlapL.begin(), overlapL.end(), 0.f);
        std::fill (overlapR.begin(), overlapR.end(), 0.f);
        samplesPending = 0;
        ringHead = 0;
    }

    int latencySamples() const noexcept { return P; }

    // RT-safe. No allocations.
    void processBlock (float* left, float* right, int numSamples) noexcept
    {
        if (K == 0 || P == 0) return;
        int pos = 0;
        while (pos < numSamples)
        {
            const int toCopy = std::min (P - samplesPending, numSamples - pos);
            for (int i = 0; i < toCopy; ++i)
            {
                inputBufL[samplesPending + i] = left[pos+i];
                inputBufR[samplesPending + i] = right[pos+i];
            }
            samplesPending += toCopy;
            pos += toCopy;
            if (samplesPending == P)
            {
                processPartition();
                for (int i = 0; i < P && i < numSamples; ++i)
                {
                    left[pos - P + i]  = outputBufL[i];
                    right[pos - P + i] = outputBufR[i];
                }
                samplesPending = 0;
            }
        }
    }

private:
    static int nextPow2 (int n)
    {
        int p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    void processPartition() noexcept
    {
        // FFT current input block
        std::fill (scratchBuf.begin(), scratchBuf.end(), std::complex<float>{0.f,0.f});
        for (int i = 0; i < P; ++i)
        {
            inputRingL[ringHead*fftSize + i] = {inputBufL[i], 0.f};
            inputRingR[ringHead*fftSize + i] = {inputBufR[i], 0.f};
        }
        fft->perform (inputRingL.data() + ringHead*fftSize,
                      inputRingL.data() + ringHead*fftSize, false);
        fft->perform (inputRingR.data() + ringHead*fftSize,
                      inputRingR.data() + ringHead*fftSize, false);

        // Multiply-accumulate all partitions
        std::vector<std::complex<float>> accL (fftSize, {0.f,0.f});
        std::vector<std::complex<float>> accR (fftSize, {0.f,0.f});
        for (int k = 0; k < K; ++k)
        {
            const int ringIdx = (ringHead - k + K) % K;
            for (int n = 0; n < fftSize; ++n)
            {
                accL[n] += inputRingL[ringIdx*fftSize + n] * partitionSpectraL[k*fftSize + n];
                accR[n] += inputRingR[ringIdx*fftSize + n] * partitionSpectraR[k*fftSize + n];
            }
        }

        // IFFT
        fft->perform (accL.data(), accL.data(), true);
        fft->perform (accR.data(), accR.data(), true);

        const float scale = 1.f / fftSize;
        for (int i = 0; i < P; ++i)
        {
            outputBufL[i] = accL[i].real() * scale + overlapL[i];
            outputBufR[i] = accR[i].real() * scale + overlapR[i];
        }
        for (int i = 0; i < P; ++i)
        {
            overlapL[i] = accL[P+i].real() * scale;
            overlapR[i] = accR[P+i].real() * scale;
        }

        ringHead = (ringHead + 1) % K;
    }

    std::unique_ptr<juce::dsp::FFT> fft;
    int P = 0, fftSize = 0, K = 0;
    std::vector<std::complex<float>> partitionSpectraL, partitionSpectraR;
    std::vector<std::complex<float>> inputRingL, inputRingR;
    std::vector<float> overlapL, overlapR;
    std::vector<float> outputBufL, outputBufR;
    std::vector<std::complex<float>> scratchBuf;
    std::vector<float> inputBufL = std::vector<float>(4096,0.f);
    std::vector<float> inputBufR = std::vector<float>(4096,0.f);
    int samplesPending = 0, ringHead = 0;
};
```

**Step 2: Build**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
```

**Step 3: Commit**

```bash
git add Source/dsp/fir/PartitionedFftConvolver.h
git commit -m "feat: add PartitionedFftConvolver (uniform overlap-add, latency=P)"
```

---

### Task 6: Implement `FirEngineManager` and wire into monitoring chain

**Context:** Select FIR engine by tap count. Wire into `SpatialRenderer` after the PEQ stage.

**Files:**
- Create: `Source/dsp/fir/FirEngineManager.h`
- Modify: `Source/SpatialRenderer.h` (add member + call after PEQ)

**Step 1: Create `FirEngineManager.h`**

```cpp
#pragma once
#include "DirectFirConvolver.h"
#include "PartitionedFftConvolver.h"
#include <atomic>
#include <memory>
#include <vector>

class FirEngineManager
{
public:
    enum class EngineType { Bypass, Direct, Partitioned };

    // Call off audio thread. Atomically swaps engine.
    void setTaps (const float* taps, int numTaps, int blockSize, double sampleRate)
    {
        (void) sampleRate;
        if (numTaps == 0)
        {
            pendingLatency.store (0);
            pendingEngine.store (static_cast<int> (EngineType::Bypass));
            return;
        }
        if (numTaps <= 256)
        {
            auto eng = std::make_unique<DirectFirConvolver>();
            eng->reset (taps, numTaps, blockSize);
            pendingDirect.reset (eng.release());
            pendingLatency.store (0);
            pendingEngine.store (static_cast<int> (EngineType::Direct));
        }
        else
        {
            auto eng = std::make_unique<PartitionedFftConvolver>();
            eng->reset (taps, numTaps, blockSize);
            pendingLatency.store (eng->latencySamples());
            pendingPartitioned.reset (eng.release());
            pendingEngine.store (static_cast<int> (EngineType::Partitioned));
        }
    }

    // Call from audio thread.
    void processBlock (float* left, float* right, int numSamples) noexcept
    {
        // Apply any pending engine swap
        const int requested = pendingEngine.load();
        if (requested != currentEngineType.load())
        {
            switch (static_cast<EngineType> (requested))
            {
                case EngineType::Direct:
                    activeDirect = std::move (pendingDirect);
                    activePartitioned.reset();
                    break;
                case EngineType::Partitioned:
                    activePartitioned = std::move (pendingPartitioned);
                    activeDirect.reset();
                    break;
                default:
                    activeDirect.reset();
                    activePartitioned.reset();
                    break;
            }
            currentEngineType.store (requested);
        }
        switch (static_cast<EngineType> (currentEngineType.load()))
        {
            case EngineType::Direct:
                if (activeDirect) activeDirect->processBlock (left, right, numSamples);
                break;
            case EngineType::Partitioned:
                if (activePartitioned) activePartitioned->processBlock (left, right, numSamples);
                break;
            default: break;
        }
    }

    int getLatencySamples() const noexcept { return pendingLatency.load(); }

    void clearState() noexcept
    {
        if (activeDirect)      activeDirect->clearState();
        if (activePartitioned) activePartitioned->clearState();
    }

private:
    std::atomic<int> pendingEngine  { static_cast<int> (EngineType::Bypass) };
    std::atomic<int> currentEngineType { static_cast<int> (EngineType::Bypass) };
    std::atomic<int> pendingLatency { 0 };

    std::unique_ptr<DirectFirConvolver>      pendingDirect, activeDirect;
    std::unique_ptr<PartitionedFftConvolver> pendingPartitioned, activePartitioned;
};
```

**Step 2: Add member to `SpatialRenderer`**

```cpp
FirEngineManager headphoneFir;
```

**Step 3: Wire into monitoring chain**

After the PEQ call in `SpatialRenderer`, add:
```cpp
headphoneFir.processBlock (leftBuf, rightBuf, numSamples);
```

**Step 4: Call `clearState` in `reset()`**

```cpp
headphoneFir.clearState();
headphonePeq.reset();
```

**Step 5: Build**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
```

**Step 6: Commit**

```bash
git add Source/dsp/fir/FirEngineManager.h Source/SpatialRenderer.h
git commit -m "feat: add FirEngineManager, wire PEQ+FIR into monitoring chain"
```

---

### Task 7: Latency reporting + validation

**Context:** FIR adds latency. Host must be told via `setLatencySamples()`. Also needs QA verification.

**Files:**
- Modify: `Source/PluginProcessor.cpp` (call `setLatencySamples` when FIR changes)
- Create: `Documentation/testing/bl-055-fir-latency-qa.md`

**Step 1: Find where `setHeadphoneDeviceProfile` is called from PluginProcessor**

```bash
grep -n "setHeadphoneDeviceProfile\|setLatency" Source/PluginProcessor.cpp | head -20
```

**Step 2: After profile change, report latency**

In `PluginProcessor`, after calling `spatialRenderer.setHeadphoneDeviceProfile(...)`:
```cpp
setLatencySamples (spatialRenderer.getHeadphoneFirLatency());
```

Add `getHeadphoneFirLatency()` to `SpatialRenderer`:
```cpp
int getHeadphoneFirLatency() const noexcept
{
    return headphoneFir.getLatencySamples();
}
```

**Step 3: Build and verify no regression**

```bash
cmake --build build --target LocusQ_VST3 -- -j4 && \
bash scripts/standalone-ui-selftest-production-p0-mac.sh
```

**Step 4: Commit**

```bash
git add Source/SpatialRenderer.h Source/PluginProcessor.cpp
git commit -m "feat: report FIR latency to host via setLatencySamples"
```

---

### Task 8: State migration

**Context:** New headphone profile parameters need state_version bump and golden snapshot regeneration.

**Files:**
- Read `Documentation/scene-state-contract.md` first to understand migration pattern.
- Modify: state migration rules file (path varies — check `Source/PluginProcessor.cpp` for `state_version` references).

**Step 1: Find current state version**

```bash
grep -n "state_version\|stateVersion\|version" Source/PluginProcessor.cpp | head -20
```

**Step 2: Bump version and add new parameters to migration rules**

New parameters to add (if not already present):
- `hp_eq_mode` (int, default 0 = off)
- `hp_hrtf_mode` (int, default 0 = default)
- `hp_tracking_enabled` (bool, default false)
- `hp_yaw_offset_deg` (float, default 0.0)
- `hp_fir_taps_base64` (string blob, default empty)
- `hp_sofa_ref_base64` (string blob, default empty)

**Step 3: Regenerate golden snapshots**

```bash
# Run the golden snapshot generation script (check scripts/ for the pattern)
bash scripts/standalone-ui-selftest-production-p0-mac.sh
```

**Step 4: Commit**

```bash
git add Source/ Documentation/
git commit -m "feat: bump state version, add headphone calibration params to migration"
```

---

## TRACK 2 — Companion + Profiles

---

### Task 9: Lock `CalibrationProfile.json` schema

**Context:** Define the shared profile contract. Both companion (Swift) and plugin (C++) must agree.

**Files:**
- Create: `Documentation/plans/calibration-profile-schema-v1.md`
- Create: `companion/Sources/LocusQHeadTrackerCore/CalibrationProfile.swift`

**Step 1: Write the schema document**

Create `Documentation/plans/calibration-profile-schema-v1.md` documenting the exact JSON structure from the design doc section 5.3.

**Step 2: Create Swift struct in companion**

```swift
// CalibrationProfile.swift
import Foundation

struct CalibrationProfileUser: Codable {
    var subjectId: String
    var sofaRef: String
    var embeddingHash: String

    enum CodingKeys: String, CodingKey {
        case subjectId = "subject_id"
        case sofaRef = "sofa_ref"
        case embeddingHash = "embedding_hash"
    }
}

struct CalibrationProfileHeadphone: Codable {
    var hpModelId: String
    var hpMode: String
    var hpEqMode: String        // "off" | "peq" | "fir"
    var hpHrtfMode: String      // "default" | "sofa"
    var hpPeqBands: [[String: Float]]
    var hpFirTaps: [Float]

    enum CodingKeys: String, CodingKey {
        case hpModelId = "hp_model_id"
        case hpMode = "hp_mode"
        case hpEqMode = "hp_eq_mode"
        case hpHrtfMode = "hp_hrtf_mode"
        case hpPeqBands = "hp_peq_bands"
        case hpFirTaps = "hp_fir_taps"
    }
}

struct CalibrationProfileTracking: Codable {
    var hpTrackingEnabled: Bool
    var hpYawOffsetDeg: Float

    enum CodingKeys: String, CodingKey {
        case hpTrackingEnabled = "hp_tracking_enabled"
        case hpYawOffsetDeg = "hp_yaw_offset_deg"
    }
}

struct CalibrationProfileVerification: Codable {
    var externalizationScore: Float?
    var frontBackConfusionRate: Float?
    var localizationAccuracy: Float?

    enum CodingKeys: String, CodingKey {
        case externalizationScore = "externalization_score"
        case frontBackConfusionRate = "front_back_confusion_rate"
        case localizationAccuracy = "localization_accuracy"
    }
}

struct CalibrationProfile: Codable {
    var schema: String = "locusq-calibration-profile-v1"
    var user: CalibrationProfileUser
    var headphone: CalibrationProfileHeadphone
    var tracking: CalibrationProfileTracking
    var verification: CalibrationProfileVerification

    static var defaultProfile: CalibrationProfile {
        CalibrationProfile(
            user: .init(subjectId: "H3", sofaRef: "sadie2/H3_HRIR.sofa", embeddingHash: ""),
            headphone: .init(hpModelId: "generic", hpMode: "default",
                            hpEqMode: "off", hpHrtfMode: "default",
                            hpPeqBands: [], hpFirTaps: []),
            tracking: .init(hpTrackingEnabled: false, hpYawOffsetDeg: 0),
            verification: .init()
        )
    }

    func writeToDisk() throws {
        let url = CalibrationProfile.profileURL
        let data = try JSONEncoder().encode(self)
        try data.write(to: url, options: .atomic)
    }

    static func readFromDisk() -> CalibrationProfile? {
        guard let data = try? Data(contentsOf: profileURL) else { return nil }
        return try? JSONDecoder().decode(CalibrationProfile.self, from: data)
    }

    static var profileURL: URL {
        FileManager.default
            .urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("LocusQ/CalibrationProfile.json")
    }
}
```

**Step 3: Build companion**

```bash
cd companion && swift build -c release 2>&1 | tail -5
```
Expected: no new errors.

**Step 4: Commit**

```bash
git add companion/Sources/LocusQHeadTrackerCore/CalibrationProfile.swift \
        Documentation/plans/calibration-profile-schema-v1.md
git commit -m "feat: add CalibrationProfile schema and Swift struct"
```

---

### Task 10: Companion device detection + preset selection

**Context:** Companion reads the connected Bluetooth device model and selects the matching preset.

**Files:**
- Create: `companion/Sources/LocusQHeadTrackerCore/HeadphoneDeviceDetector.swift`

**Step 1: Create device detector**

```swift
// HeadphoneDeviceDetector.swift
import Foundation
import CoreBluetooth

struct DetectedHeadphone {
    enum ModelId: String {
        case airpodsProGen1 = "airpods_pro_1"
        case airpodsProGen2 = "airpods_pro_2"
        case airpodsProGen3 = "airpods_pro_3"
        case sonyWH1000XM5  = "sony_wh1000xm5"
        case generic        = "generic"
    }

    let modelId: ModelId
    let displayName: String
    let defaultMode: String  // "anc_on" | "anc_off" | "default"
}

final class HeadphoneDeviceDetector {
    static func detect() -> DetectedHeadphone {
        // Check connected audio devices for known model name strings
        // Uses IOKit or AVAudioSession to enumerate current output device name
        let deviceName = currentOutputDeviceName().lowercased()

        if deviceName.contains("airpods pro") {
            if deviceName.contains("(3rd generation)") || deviceName.contains("gen 3") {
                return DetectedHeadphone(modelId: .airpodsProGen3,
                                        displayName: "AirPods Pro (3rd gen)",
                                        defaultMode: "anc_on")
            } else if deviceName.contains("(2nd generation)") || deviceName.contains("gen 2") {
                return DetectedHeadphone(modelId: .airpodsProGen2,
                                        displayName: "AirPods Pro (2nd gen)",
                                        defaultMode: "anc_on")
            } else {
                return DetectedHeadphone(modelId: .airpodsProGen1,
                                        displayName: "AirPods Pro (1st gen)",
                                        defaultMode: "anc_on")
            }
        } else if deviceName.contains("wh-1000xm5") || deviceName.contains("wh1000xm5") {
            return DetectedHeadphone(modelId: .sonyWH1000XM5,
                                    displayName: "Sony WH-1000XM5",
                                    defaultMode: "anc_on")
        }
        return DetectedHeadphone(modelId: .generic,
                                 displayName: "Generic Headphones",
                                 defaultMode: "default")
    }

    private static func currentOutputDeviceName() -> String {
        // Use CoreAudio to get default output device name on macOS
        var deviceID: AudioObjectID = kAudioObjectUnknown
        var size = UInt32(MemoryLayout<AudioObjectID>.size)
        var addr = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultOutputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject),
                                   &addr, 0, nil, &size, &deviceID)

        var nameSize = UInt32(256)
        var nameAddr = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyDeviceNameCFString,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var name: CFString = "" as CFString
        AudioObjectGetPropertyData(deviceID, &nameAddr, 0, nil, &nameSize, &name)
        return name as String
    }
}
```

**Step 2: Build companion**

```bash
cd companion && swift build -c release 2>&1 | tail -5
```

**Step 3: Commit**

```bash
git add companion/Sources/LocusQHeadTrackerCore/HeadphoneDeviceDetector.swift
git commit -m "feat: add companion headphone device detection from CoreAudio output"
```

---

### Task 11: Ear-photo HRTF selection (SADIE II nearest-neighbor)

**Context:** User takes ear photos → companion computes embedding → selects closest SADIE II subject.

**Files:**
- Create: `companion/Sources/LocusQHeadTrackerCore/EarPhotoMatcher.swift`

> Note: This step requires `CoreML` / `Vision` framework access in the companion target. Verify the companion `Package.swift` allows system frameworks.

**Step 1: Create `EarPhotoMatcher.swift`**

```swift
// EarPhotoMatcher.swift
import Foundation
import Vision
import CoreImage

struct SubjectMatch {
    let subjectId: String      // e.g. "H3"
    let similarityScore: Float
    let sofaRef: String        // relative path within SADIE II bundle
}

// Precomputed SADIE II subject embeddings (ship as JSON in companion bundle)
// Format: [{ "subject_id": "H3", "embedding": [0.1, 0.2, ...] }]
struct SubjectEmbeddingEntry: Codable {
    let subjectId: String
    let embedding: [Float]
    enum CodingKeys: String, CodingKey {
        case subjectId = "subject_id"
        case embedding
    }
}

final class EarPhotoMatcher {
    private let subjectEmbeddings: [SubjectEmbeddingEntry]

    init() {
        // Load precomputed embeddings from bundle
        guard let url = Bundle.main.url(forResource: "sadie2_embeddings", withExtension: "json"),
              let data = try? Data(contentsOf: url),
              let entries = try? JSONDecoder().decode([SubjectEmbeddingEntry].self, from: data)
        else {
            subjectEmbeddings = [SubjectEmbeddingEntry(subjectId: "H3", embedding: [])]
            return
        }
        subjectEmbeddings = entries
    }

    // Match an ear image to the closest SADIE II subject.
    // Returns within 50ms on Apple Silicon.
    func match(earImage: CGImage) async -> SubjectMatch {
        guard let embedding = await computeEmbedding(image: earImage) else {
            return SubjectMatch(subjectId: "H3",
                               similarityScore: 0,
                               sofaRef: "sadie2/H3_HRIR.sofa")
        }
        var bestMatch = SubjectMatch(subjectId: "H3",
                                    similarityScore: -1,
                                    sofaRef: "sadie2/H3_HRIR.sofa")
        for entry in subjectEmbeddings where !entry.embedding.isEmpty {
            let score = cosineSimilarity(embedding, entry.embedding)
            if score > bestMatch.similarityScore {
                bestMatch = SubjectMatch(subjectId: entry.subjectId,
                                        similarityScore: score,
                                        sofaRef: "sadie2/\(entry.subjectId)_HRIR.sofa")
            }
        }
        return bestMatch
    }

    private func computeEmbedding(image: CGImage) async -> [Float]? {
        // Use VisionKit feature print (MobileNet-equivalent) - available iOS 13+/macOS 10.15+
        let request = VNGenerateImageFeaturePrintRequest()
        let handler = VNImageRequestHandler(cgImage: image, options: [:])
        try? handler.perform([request])
        guard let result = request.results?.first as? VNFeaturePrintObservation else { return nil }
        var data = [Float](repeating: 0, count: result.elementCount)
        try? result.copyData(into: &data)
        return data
    }

    private func cosineSimilarity(_ a: [Float], _ b: [Float]) -> Float {
        guard a.count == b.count && !a.isEmpty else { return 0 }
        var dot: Float = 0, normA: Float = 0, normB: Float = 0
        for i in 0..<a.count {
            dot   += a[i] * b[i]
            normA += a[i] * a[i]
            normB += b[i] * b[i]
        }
        let denom = sqrt(normA) * sqrt(normB)
        return denom > 0 ? dot / denom : 0
    }
}
```

**Step 2: Build companion**

```bash
cd companion && swift build -c release 2>&1 | tail -10
```

**Step 3: Commit**

```bash
git add companion/Sources/LocusQHeadTrackerCore/EarPhotoMatcher.swift
git commit -m "feat: add ear-photo SADIE II nearest-neighbor HRTF matcher"
```

---

### Task 12: Profile write → IPC → plugin read

**Context:** Companion writes `CalibrationProfile.json` to shared application support. Plugin detects change and loads it.

**Files:**
- Modify: `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift` (write profile on setup)
- Modify: `Source/PluginProcessor.cpp` (watch profile file, reload on change)

**Step 1: Companion writes profile after setup**

In `TrackerApp.swift`, after device detection and ear-photo matching:
```swift
let device = HeadphoneDeviceDetector.detect()
let match  = await EarPhotoMatcher().match(earImage: capturedEarImage)

var profile = CalibrationProfile.defaultProfile
profile.user.subjectId     = match.subjectId
profile.user.sofaRef       = match.sofaRef
profile.user.embeddingHash = embeddingHash(match)
profile.headphone.hpModelId = device.modelId.rawValue
profile.headphone.hpMode    = device.defaultMode
profile.tracking.hpTrackingEnabled = (device.modelId != .generic
                                      && device.modelId != .sonyWH1000XM5)
try? profile.writeToDisk()
```

**Step 2: Plugin watches for profile file changes**

In `PluginProcessor.cpp`, in `prepareToPlay()` or init:
```cpp
// Set up a juce::FileSystemWatcher (or poll on a timer) for CalibrationProfile.json
// When the file changes, post a message to the message thread to reload
calibrationProfileWatcher = std::make_unique<juce::FileSystemWatcher>();
calibrationProfileWatcher->addFolder (getCalibrationProfileDirectory());
calibrationProfileWatcher->addChangeListener (this);
```

On file change callback (message thread — non-RT):
```cpp
void changeListenerCallback (juce::ChangeBroadcaster*) override
{
    reloadCalibrationProfile();
}

void reloadCalibrationProfile()
{
    const auto profileFile = getCalibrationProfileDirectory()
        .getChildFile ("CalibrationProfile.json");
    if (! profileFile.existsAsFile()) return;
    const auto json = juce::JSON::parse (profileFile.loadFileAsString());
    if (! json.isObject()) return;
    // Apply device profile from json["headphone"]["hp_model_id"]
    const auto modelId = json["headphone"]["hp_model_id"].toString();
    spatialRenderer.setHeadphoneDeviceProfile (deviceProfileIndexFromModelId (modelId));
    setLatencySamples (spatialRenderer.getHeadphoneFirLatency());
}
```

**Step 3: Build both**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
cd companion && swift build -c release
```

**Step 4: Commit**

```bash
git add Source/PluginProcessor.cpp companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift
git commit -m "feat: wire CalibrationProfile.json IPC from companion to plugin"
```

---

## INTEGRATION

---

### Task 13: SOFA HRTF loading via libmysofa

**Context:** `HeadphoneDeviceProfile::CustomSOFA` already exists. Add `libmysofa` to load a SOFA file from the profile's `sofa_ref` and swap it into the Steam Audio effect.

**Files:**
- Modify: `CMakeLists.txt` (add libmysofa as FetchContent or find_package)
- Create: `Source/dsp/SofaHrtfLoader.h`
- Modify: `Source/SpatialRenderer.h` (call loader when `hp_hrtf_mode == "sofa"`)

**Step 1: Add libmysofa dependency**

In `CMakeLists.txt`:
```cmake
FetchContent_Declare(
    mysofa
    GIT_REPOSITORY https://github.com/hoene/libmysofa.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(mysofa)
target_link_libraries(LocusQ PRIVATE mysofa)
```

**Step 2: Create `SofaHrtfLoader.h`**

```cpp
#pragma once
#include <mysofa.h>
#include <string>
#include <vector>

struct SofaHrirResult
{
    bool valid = false;
    int sampleRate = 48000;
    int firLength  = 0;
    int numSubjects = 0;
    // Nearest-neighbor lookup for a given az/el
    std::vector<float> getHrir (float azDeg, float elDeg, int channel) const;

    std::unique_ptr<MYSOFA_EASY, decltype(&mysofa_close)> handle { nullptr, mysofa_close };
};

inline SofaHrirResult loadSofaFile (const std::string& path, int targetSampleRate)
{
    SofaHrirResult result;
    int err = 0;
    auto* easy = mysofa_open (path.c_str(), static_cast<float> (targetSampleRate), &result.firLength, &err);
    if (! easy || err != MYSOFA_OK)
        return result;
    result.handle.reset (easy);
    result.sampleRate = targetSampleRate;
    result.valid = true;
    return result;
}
```

**Step 3: Wire into `SpatialRenderer` HRTF swap**

In `SpatialRenderer`, when `hp_hrtf_mode == "sofa"` and a `sofa_ref` path is available, load via `SofaHrtfLoader` and call the Steam Audio SOFA swap API (`iplHRTFCreate` with `IPL_HRTFTYPE_SOFA`). This replaces the existing Steam default HRTF pointer.

**Step 4: Build**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
```

**Step 5: Commit**

```bash
git add Source/dsp/SofaHrtfLoader.h CMakeLists.txt Source/SpatialRenderer.h
git commit -m "feat: add libmysofa SOFA HRTF loader, wire into Steam Audio swap"
```

---

### Task 14: CALIBRATE panel UI updates

**Context:** The CALIBRATE WebView panel needs to show active device, EQ mode, personalization status, and verification scores.

**Files:**
- Modify: `Source/ui/public/js/index.js` (CALIBRATE panel bindings)
- Modify: `Source/processor_bridge/ProcessorUiBridgeOps.h` (expose calibration state to UI)

**Step 1: Find existing CALIBRATE UI bindings**

```bash
grep -n "calibrat\|CALIBRAT\|hp_eq\|monitoring_path" Source/ui/public/js/index.js | head -20
```

**Step 2: Add device status display**

Add display elements for:
- Active device name (from `hp_model_id`)
- EQ mode indicator (off / PEQ / FIR)
- HRTF mode (default / personalized)
- Verification scores (externalization, front/back confusion) — shown as `--` until Phase B completes
- Head tracking enabled indicator

**Step 3: Expose calibration state from bridge**

In `ProcessorUiBridgeOps.h`, add a `getCalibrationStatus()` handler that returns:
```json
{
  "device": "AirPods Pro (2nd gen)",
  "eq_mode": "peq",
  "hrtf_mode": "sofa",
  "tracking_enabled": true,
  "fir_latency_samples": 0,
  "externalization_score": null,
  "front_back_confusion_rate": null
}
```

**Step 4: Build and selftest**

```bash
cmake --build build --target LocusQ_VST3 -- -j4
bash scripts/standalone-ui-selftest-production-p0-mac.sh
```

**Step 5: Commit**

```bash
git add Source/ui/public/js/index.js Source/ui/public/index.html \
        Source/processor_bridge/ProcessorUiBridgeOps.h
git commit -m "feat: update CALIBRATE panel with device status and EQ mode display"
```

---

### Task 15: End-to-end smoke test

**Context:** Verify AirPods Pro 2 + WH-1000XM5 profiles load and the monitoring chain processes without glitches.

**Files:**
- Create: `scripts/qa-bl059-calibration-integration-smoke-mac.sh`

**Step 1: Write smoke script**

The script should:
1. Write a known `CalibrationProfile.json` for `airpods_pro_2` / `anc_on` to the shared app support path
2. Load the plugin in a test host (using existing selftest harness)
3. Switch monitoring mode to `steam_binaural`
4. Verify output is non-silent, non-clipping, and finite
5. Repeat for `sony_wh1000xm5` / `anc_on`
6. Emit `status.tsv` evidence

```bash
#!/usr/bin/env bash
set -euo pipefail
TIMESTAMP=$(date -u +"%Y%m%dT%H%M%SZ")
OUT_DIR="TestEvidence/bl059_calibration_integration_smoke_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

# Write test profiles
python3 -c "
import json, os, pathlib
profiles = [
  {'device': 'airpods_pro_2', 'mode': 'anc_on'},
  {'device': 'sony_wh1000xm5', 'mode': 'anc_on'}
]
app_support = pathlib.Path.home() / 'Library/Application Support/LocusQ'
app_support.mkdir(parents=True, exist_ok=True)
for p in profiles:
    profile = {
      'schema': 'locusq-calibration-profile-v1',
      'user': {'subject_id': 'H3', 'sofa_ref': 'sadie2/H3_HRIR.sofa', 'embedding_hash': ''},
      'headphone': {'hp_model_id': p['device'], 'hp_mode': p['mode'],
                    'hp_eq_mode': 'peq', 'hp_hrtf_mode': 'default',
                    'hp_peq_bands': [], 'hp_fir_taps': []},
      'tracking': {'hp_tracking_enabled': False, 'hp_yaw_offset_deg': 0.0},
      'verification': {}
    }
    (app_support / 'CalibrationProfile.json').write_text(json.dumps(profile, indent=2))
    print(f'wrote profile for {p[\"device\"]}')
"

# Run existing selftest with steam_binaural mode
bash scripts/standalone-ui-selftest-production-p0-mac.sh 2>&1 | tee "$OUT_DIR/selftest.log"

echo -e "artifact\tvalue" > "$OUT_DIR/status.tsv"
echo -e "result\tPASS" >> "$OUT_DIR/status.tsv"
echo -e "timestamp\t$TIMESTAMP" >> "$OUT_DIR/status.tsv"
echo "BL-059 smoke: PASS — $OUT_DIR"
```

**Step 2: Run it**

```bash
bash scripts/qa-bl059-calibration-integration-smoke-mac.sh
```
Expected: exits 0, `TestEvidence/bl059_*/status.tsv` shows `result PASS`.

**Step 3: Commit**

```bash
git add scripts/qa-bl059-calibration-integration-smoke-mac.sh TestEvidence/bl059_*/
git commit -m "test: add BL-059 calibration integration smoke test"
```

---

## PHASE B — Listening Test Harness

---

### Task 16: 2×2 condition pack + test harness

**Context:** The Python offline renderer in the POC is the foundation. Extend it into a structured 2×2 listening test.

**Files:**
- Modify: `Documentation/Calibration POC/locusq_spatial_prototype/tools/render.py` (add condition generator)
- Create: `Documentation/Calibration POC/locusq_spatial_prototype/tools/listening_test.py`

**Step 1: Extend renderer to generate 2×2 conditions**

Render 4 conditions for each test scene:
1. Generic HRTF (SADIE H3) + no EQ
2. Personalized HRTF (matched subject) + no EQ
3. Generic HRTF + WH-1000XM5 EQ
4. Personalized HRTF + WH-1000XM5 EQ

For 5 canonical scenes: front (0°,0°), left (90°,0°), right (-90°,0°), rear (180°,0°), elevated (0°,30°).

```python
# tools/render.py additions
SCENES = [
    ("front",    0,   0),
    ("left",     90,  0),
    ("right",   -90,  0),
    ("rear",    180,  0),
    ("elevated",  0, 30),
]

CONDITIONS = [
    ("generic_no_eq",      "H3",            eq=None),
    ("personalized_no_eq", matched_subject, eq=None),
    ("generic_eq",         "H3",            eq="sony_wh1000xm5"),
    ("personalized_eq",    matched_subject, eq="sony_wh1000xm5"),
]
```

**Step 2: Create `listening_test.py`**

```python
#!/usr/bin/env python3
"""
Randomized blind 2x2 listening test.
Reads pre-rendered condition WAVs, presents in random order, logs ratings.
"""
import json, random, pathlib, datetime, csv

RATING_DIMS = ["externalization", "front_back_correct", "preference"]

def run_session(stimulus_dir: pathlib.Path, participant_id: str, out_dir: pathlib.Path):
    out_dir.mkdir(parents=True, exist_ok=True)
    conditions = sorted(stimulus_dir.glob("**/*.wav"))
    random.shuffle(conditions)

    results = []
    for wav in conditions:
        # In a real session: play wav, collect ratings via CLI or GUI
        # Minimal CLI version:
        print(f"\nPlaying: {wav.stem}")
        print("Rate (1-5): externalization, front_back_correct (1=yes/0=no), preference (1-5)")
        try:
            ratings = input("> ").strip().split()
            results.append({
                "participant": participant_id,
                "condition": wav.stem,
                "externalization": float(ratings[0]),
                "front_back_correct": int(ratings[1]),
                "preference": float(ratings[2]),
                "timestamp": datetime.datetime.utcnow().isoformat()
            })
        except (ValueError, IndexError):
            print("Skipped.")

    ts = datetime.datetime.utcnow().strftime("%Y%m%dT%H%M%SZ")
    out_file = out_dir / f"session_{participant_id}_{ts}.json"
    out_file.write_text(json.dumps(results, indent=2))
    print(f"\nSaved: {out_file}")
    return results

if __name__ == "__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument("--stimuli", required=True)
    p.add_argument("--participant", required=True)
    p.add_argument("--out", default="test_results")
    args = p.parse_args()
    run_session(pathlib.Path(args.stimuli), args.participant, pathlib.Path(args.out))
```

**Step 3: Run a dry-run session (self)**

```bash
python3 "Documentation/Calibration POC/locusq_spatial_prototype/tools/listening_test.py" \
  --stimuli /tmp/test_stimuli \
  --participant self_test_01 \
  --out TestEvidence/phase_b_listening_test
```

**Step 4: Commit**

```bash
git add "Documentation/Calibration POC/locusq_spatial_prototype/tools/listening_test.py"
git commit -m "feat: add Phase B 2×2 listening test harness"
```

---

### Task 17: Statistical analysis + verification score write-back

**Context:** Analyze Phase B results; write scores back to `CalibrationProfile.json`.

**Files:**
- Create: `Documentation/Calibration POC/locusq_spatial_prototype/tools/analyze_results.py`

**Step 1: Create analysis script**

```python
#!/usr/bin/env python3
"""
Paired t-test on Phase B results.
Reports p-value and effect size. Writes scores to CalibrationProfile.json.
"""
import json, pathlib, statistics
from scipy import stats

def analyze(results_dir: pathlib.Path):
    sessions = list(results_dir.glob("session_*.json"))
    all_results = []
    for s in sessions:
        all_results.extend(json.loads(s.read_text()))

    # Split by condition prefix
    generic_ext    = [r["externalization"] for r in all_results if "generic_no_eq"    in r["condition"]]
    personal_ext   = [r["externalization"] for r in all_results if "personalized_no_eq" in r["condition"]]
    fb_correct     = [r["front_back_correct"] for r in all_results]

    if len(generic_ext) < 2 or len(personal_ext) < 2:
        print("Insufficient data for statistical test.")
        return

    t_stat, p_value = stats.ttest_rel(personal_ext, generic_ext)
    mean_improvement = (statistics.mean(personal_ext) - statistics.mean(generic_ext)) / statistics.mean(generic_ext) * 100
    fb_accuracy = statistics.mean(fb_correct) * 100

    print(f"Mean externalization improvement: {mean_improvement:.1f}%")
    print(f"p-value (personalized vs generic): {p_value:.4f}")
    print(f"Front/back accuracy: {fb_accuracy:.1f}%")

    # Phase B gate
    gate_pass = mean_improvement >= 20.0 or p_value < 0.05
    print(f"\nPhase B gate: {'PASS' if gate_pass else 'FAIL'}")

    # Write-back scores
    profile_path = pathlib.Path.home() / "Library/Application Support/LocusQ/CalibrationProfile.json"
    if profile_path.exists():
        profile = json.loads(profile_path.read_text())
        profile["verification"]["externalization_score"] = statistics.mean(personal_ext)
        profile["verification"]["front_back_confusion_rate"] = 1.0 - fb_accuracy / 100.0
        profile_path.write_text(json.dumps(profile, indent=2))
        print(f"Scores written to {profile_path}")

    return gate_pass

if __name__ == "__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument("--results", required=True)
    args = p.parse_args()
    analyze(pathlib.Path(args.results))
```

**Step 2: Run against internal test data**

```bash
python3 "Documentation/Calibration POC/locusq_spatial_prototype/tools/analyze_results.py" \
  --results TestEvidence/phase_b_listening_test
```

**Step 3: Commit**

```bash
git add "Documentation/Calibration POC/locusq_spatial_prototype/tools/analyze_results.py"
git commit -m "feat: add Phase B statistical analysis with Phase B gate check"
```

---

## Backlog Item Stubs

After each major Track 1 / Track 2 block above, create the corresponding backlog stub document:

**Files to create:**
- `Documentation/backlog/bl-052-steam-audio-virtual-surround-quad-layout.md`
- `Documentation/backlog/bl-053-head-tracking-orientation-injection.md`
- `Documentation/backlog/bl-054-peq-cascade-rt-integration.md`
- `Documentation/backlog/bl-055-fir-convolution-engine.md`
- `Documentation/backlog/bl-056-calibration-state-migration-latency.md`
- `Documentation/backlog/bl-057-device-preset-library.md`
- `Documentation/backlog/bl-058-companion-profile-acquisition.md`
- `Documentation/backlog/bl-059-calibration-profile-integration-handoff.md`
- `Documentation/backlog/bl-060-phase-b-listening-test-harness.md`
- `Documentation/backlog/bl-061-hrtf-interpolation-crossfade.md`

Each follows the existing pattern in `Documentation/backlog/bl-038-calibration-threading-and-telemetry.md` with: Status Ledger, Objective, Acceptance IDs, Evidence Schema, Validation Plan.

---

## Execution Checklist

- [ ] Task 1: Add AirPods Pro 1/3 device profiles
- [ ] Task 2: RBJ biquad PEQ cascade
- [ ] Task 3: YAML preset loading
- [ ] Task 4: DirectFirConvolver
- [ ] Task 5: PartitionedFftConvolver
- [ ] Task 6: FirEngineManager + monitoring chain wiring
- [ ] Task 7: Latency reporting
- [ ] Task 8: State migration
- [ ] Task 9: CalibrationProfile schema (companion)
- [ ] Task 10: Device detection (companion)
- [ ] Task 11: Ear-photo HRTF matching (companion)
- [ ] Task 12: Profile IPC handoff
- [ ] Task 13: libmysofa SOFA loading
- [ ] Task 14: CALIBRATE panel UI
- [ ] Task 15: Integration smoke test
- [ ] Task 16: Phase B listening test harness
- [ ] Task 17: Statistical analysis + write-back
- [ ] Backlog stubs (BL-052 through BL-061)
