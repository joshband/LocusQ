from __future__ import annotations
import numpy as np
import soundfile as sf
from scipy.signal import fftconvolve, resample_poly
import yaml

from .sofa_loader import load_sofa_hrir
from .hrir_select import select_hrir_nearest
from .eq_biquad import BiquadSpec, apply_peq_stereo

def _to_mono(x: np.ndarray) -> np.ndarray:
    if x.ndim == 1:
        return x
    return x.mean(axis=1)

def render_binaural_offline(
    sofa_path: str,
    input_wav: str,
    output_wav: str,
    az_deg: float,
    el_deg: float,
    headphone_preset_yaml: str | None = None,
    normalize: bool = True,
):
    sofa = load_sofa_hrir(sofa_path)
    x, fs_in = sf.read(input_wav, dtype="float32")
    x = _to_mono(np.asarray(x))
    if fs_in != sofa.sample_rate:
        x = resample_poly(x, sofa.sample_rate, fs_in).astype(np.float32)

    hL, hR, idx = select_hrir_nearest(sofa, az_deg, el_deg)

    yL = fftconvolve(x, hL, mode="full").astype(np.float32)
    yR = fftconvolve(x, hR, mode="full").astype(np.float32)
    y = np.stack([yL, yR], axis=1)

    if headphone_preset_yaml:
        preset = yaml.safe_load(open(headphone_preset_yaml, "r"))
        preamp_db = float(preset["preamp_db"])
        filters = [BiquadSpec(**f) for f in preset["filters"]]
        y = apply_peq_stereo(y, sofa.sample_rate, preamp_db=preamp_db, filters=filters)

    if normalize:
        peak = float(np.max(np.abs(y)) + 1e-12)
        if peak > 0.99:
            y *= (0.99 / peak)

    sf.write(output_wav, y, sofa.sample_rate, subtype="PCM_24")
    return idx
