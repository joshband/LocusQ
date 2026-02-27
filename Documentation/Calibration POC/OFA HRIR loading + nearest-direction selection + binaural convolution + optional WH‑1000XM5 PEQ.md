Title: OFA HRIR loading + nearest-direction selection + binaural convolution + optional WH‑1000XM5 PEQ
Document Type: Calibration POC Note
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

Here you go — I produced (1) a comprehensive Markdown spec and (2) a working Python prototype skeleton that does **SOFA HRIR loading + nearest-direction selection + binaural convolution + optional WH‑1000XM5 PEQ**.

SADIE II is explicitly **Apache 2.0** licensed and includes SOFA HRIRs plus ear imagery/anthropometrics, and its azimuth/elevation conventions are documented — those constraints are baked into the spec. ([Zenodo][1])

## Deliverables

* [Download the comprehensive spec (Markdown)](sandbox:/mnt/data/locusq_spatial_audio_spec.md)
* [Download the Python prototype (zip)](sandbox:/mnt/data/locusq_spatial_prototype.zip)

The prototype is intentionally “v0 pragmatic”: **offline rendering** is the fastest way to validate correctness before you invest in real-time partitioned convolution + crossfading filter updates.

## What’s implemented (v0)

### 1) SOFA-based renderer (SADIE II compatible)

* Loads HRIRs from SOFA (SimpleFreeFieldHRIR-style) via `h5py`
* Uses SADIE II’s coordinate convention (0° front, +left; elevation positive up) ([University of York][2])
* Selects HRIR by nearest-neighbor on the sphere (dot-product)
* Convolves mono audio → stereo binaural (FFT convolution)

For production, the spec calls out upgrading interpolation using **libmysofa** (it can lookup/interpolate HRIRs and normalize), but v0 keeps it simple and testable. ([GitHub][3])

### 2) First-pass user profile: ear-photo nearest-neighbor selection

* Spec defines the pipeline + profile JSON contract
* Prototype includes optional PyTorch embedding utilities (MobileNetV3) and nearest-neighbor scoring
* v0 capture requirement is deliberately pragmatic: guided framing + acceptable manual crop (avoid blocking on ear segmentation)

SADIE II includes hi-res ear pictures explicitly, which makes this approach viable as a first pass. ([University of York][2])

### 3) Separate headphone EQ module (WH‑1000XM5 baseline preset)

* Implemented RBJ biquad parametric EQ (PK/LSC/HSC)
* Includes a **WH‑1000XM5 preset** derived from AutoEq/oratory1990 parameters (preamp + 10 filters) ([GitHub][4])

Important caveat in spec: Bluetooth/ANC headphones can vary response by **mode, DSP state, and seal**; for WH‑1000XM5, seal disruption (e.g., glasses) can reduce bass. Your testing needs to standardize conditions. ([RTINGS.com][5])

### 4) Controlled listening test plan

* 2×2 within-subject design:

  * generic vs personalized
  * with vs without headphone EQ
* Defines stimuli, tasks (ABX, ratings, localization), randomization/blinding, and logging schema
* Defines “pass/fail” criteria so you don’t end up with infinite subjective iteration

## Quick start (prototype)

Once you unzip the prototype:

```bash
pip install numpy scipy soundfile h5py pyyaml
python tools/render.py \
  --sofa /path/to/SADIE/H3_HRIR_SOFA/H3.sofa \
  --inwav input.wav \
  --outwav out_binaural.wav \
  --az 30 --el 0 \
  --eq locusq/eq_presets/sony_wh1000xm5_autoeq_oratory.yaml
```

## What I recommend you do next (high-impact)

1. **Pick one SADIE II subject** as your generic baseline and ship the offline renderer + test harness first.
2. Add ear-photo selection and run the 2×2 tests. If you can’t measure improvement there, morphing/ML is premature.
3. Only after the loop is proven:

   * switch HRIR selection from nearest-neighbor → interpolated (libmysofa)
   * move offline → real-time partitioned convolution with crossfading
   * expand headphone EQ to multiple modes/presets (ANC on/off)

If you want, I can also generate a second Markdown doc that’s purely an **engineering task breakdown** (tickets, acceptance criteria, and sequencing) aligned to the spec you now have, but the two files above already contain everything needed to start building and validating.

[1]: https://zenodo.org/records/10886409 "SADIE II Database"
[2]: https://www.york.ac.uk/sadie-project/database.html?utm_source=chatgpt.com "The SADIE II Database"
[3]: https://github.com/hoene/libmysofa?utm_source=chatgpt.com "hoene/libmysofa: Reader for AES SOFA files to get better ..."
[4]: https://raw.githubusercontent.com/jaakkopasanen/AutoEq/master/results/oratory1990/over-ear/Sony%20WH-1000XM5/Sony%20WH-1000XM5%20ParametricEQ.txt "raw.githubusercontent.com"
[5]: https://www.rtings.com/headphones/reviews/sony/wh-1000xm5-wireless?utm_source=chatgpt.com "Sony WH-1000XM5 Wireless Headphones Review"
