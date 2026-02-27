from __future__ import annotations
import numpy as np
from typing import Tuple
from .sofa_loader import SofaHrir

def _sph_to_unit(az_deg: np.ndarray, el_deg: np.ndarray) -> np.ndarray:
    az = np.deg2rad(az_deg)
    el = np.deg2rad(el_deg)
    # x: front, y: left, z: up
    x = np.cos(el) * np.cos(az)
    y = np.cos(el) * np.sin(az)
    z = np.sin(el)
    v = np.stack([x, y, z], axis=-1)
    return v / (np.linalg.norm(v, axis=-1, keepdims=True) + 1e-12)

def select_hrir_nearest(sofa: SofaHrir, az_deg: float, el_deg: float) -> Tuple[np.ndarray, np.ndarray, int]:
    src = sofa.src_pos_deg_m
    v_all = _sph_to_unit(src[:, 0], src[:, 1])  # (M,3)
    v_q = _sph_to_unit(np.array([az_deg]), np.array([el_deg]))[0]  # (3,)
    idx = int(np.argmax(v_all @ v_q))
    hL = sofa.ir[idx, 0, :].copy()
    hR = sofa.ir[idx, 1, :].copy()
    return hL, hR, idx
