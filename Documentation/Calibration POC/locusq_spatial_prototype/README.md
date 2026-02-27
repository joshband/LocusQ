Title: LocusQ Spatial Audio Prototype
Document Type: Research Prototype
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# LocusQ Spatial Audio Prototype

This prototype provides:
- SOFA HRIR loader (SimpleFreeFieldHRIR-style)
- Nearest-neighbor direction selection
- Offline binaural rendering (FFT convolution)
- Optional headphone EQ (RBJ biquad filters)
- WH-1000XM5 preset (AutoEq/oratory1990)

## Install

```bash
pip install numpy scipy soundfile h5py pyyaml
```

## Render example

```bash
python tools/render.py \
  --sofa /path/to/SADIE/H3_HRIR_SOFA/H3.sofa \
  --inwav input.wav \
  --outwav out_binaural.wav \
  --az 30 --el 0 \
  --eq locusq/eq_presets/sony_wh1000xm5_autoeq_oratory.yaml
```

## Notes
- This is an offline reference. For real-time, move to partitioned convolution and crossfaded filter updates.
- Validate coordinate conventions carefully.
