from __future__ import annotations
import h5py
import numpy as np
from dataclasses import dataclass

@dataclass(frozen=True)
class SofaHrir:
    sample_rate: int
    ir: np.ndarray            # shape (M, R, N) where R=2 (L,R)
    src_pos_deg_m: np.ndarray # shape (M, 3) [az_deg, el_deg, r_m]

def load_sofa_hrir(path: str) -> SofaHrir:
    """Load a SOFA file that follows SimpleFreeFieldHRIR conventions.

    Notes:
      - This loader assumes SourcePosition is spherical in degrees/deg/m.
      - For robust production use, validate attrs (Type/Units) and handle radians.
    """
    with h5py.File(path, "r") as f:
        ir = f["Data.IR"][:]  # (M, R, N)
        fs = int(np.array(f["Data.SamplingRate"][:]).squeeze())
        src_pos = f["SourcePosition"][:]  # (M, 3)
        return SofaHrir(
            sample_rate=fs,
            ir=ir.astype(np.float32),
            src_pos_deg_m=src_pos.astype(np.float32),
        )
