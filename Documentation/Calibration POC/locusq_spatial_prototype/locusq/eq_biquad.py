from __future__ import annotations
import numpy as np
from dataclasses import dataclass
from typing import List, Literal

FilterType = Literal["PK", "LSC", "HSC"]

@dataclass(frozen=True)
class BiquadSpec:
    type: FilterType
    fc_hz: float
    gain_db: float
    q: float

def _db_to_a(gain_db: float) -> float:
    return 10.0 ** (gain_db / 40.0)

def design_biquad(spec: BiquadSpec, fs: int):
    """RBJ cookbook biquad design for peak and shelf filters."""
    w0 = 2.0 * np.pi * (spec.fc_hz / fs)
    cosw0 = np.cos(w0)
    sinw0 = np.sin(w0)
    alpha = sinw0 / (2.0 * spec.q)
    A = _db_to_a(spec.gain_db)

    if spec.type == "PK":
        b0 = 1 + alpha * A
        b1 = -2 * cosw0
        b2 = 1 - alpha * A
        a0 = 1 + alpha / A
        a1 = -2 * cosw0
        a2 = 1 - alpha / A

    elif spec.type == "LSC":
        # Low shelf (RBJ)
        beta = np.sqrt(A) / spec.q
        b0 =    A*((A+1) - (A-1)*cosw0 + beta*sinw0)
        b1 =  2*A*((A-1) - (A+1)*cosw0)
        b2 =    A*((A+1) - (A-1)*cosw0 - beta*sinw0)
        a0 =       (A+1) + (A-1)*cosw0 + beta*sinw0
        a1 =   -2*((A-1) + (A+1)*cosw0)
        a2 =       (A+1) + (A-1)*cosw0 - beta*sinw0

    elif spec.type == "HSC":
        # High shelf (RBJ)
        beta = np.sqrt(A) / spec.q
        b0 =    A*((A+1) + (A-1)*cosw0 + beta*sinw0)
        b1 = -2*A*((A-1) + (A+1)*cosw0)
        b2 =    A*((A+1) + (A-1)*cosw0 - beta*sinw0)
        a0 =       (A+1) - (A-1)*cosw0 + beta*sinw0
        a1 =    2*((A-1) - (A+1)*cosw0)
        a2 =       (A+1) - (A-1)*cosw0 - beta*sinw0

    else:
        raise ValueError(f"Unsupported filter type: {spec.type}")

    b = np.array([b0, b1, b2], dtype=np.float64) / a0
    a = np.array([1.0, a1/a0, a2/a0], dtype=np.float64)
    return b.astype(np.float64), a.astype(np.float64)

def process_biquad(x: np.ndarray, b: np.ndarray, a: np.ndarray) -> np.ndarray:
    """Direct Form I biquad processing (prototype)."""
    y = np.zeros_like(x, dtype=np.float64)
    x1 = x2 = 0.0
    y1 = y2 = 0.0
    b0, b1, b2 = b
    _, a1, a2 = a
    for n in range(len(x)):
        xn = float(x[n])
        yn = b0*xn + b1*x1 + b2*x2 - a1*y1 - a2*y2
        y[n] = yn
        x2, x1 = x1, xn
        y2, y1 = y1, yn
    return y.astype(np.float32)

def apply_peq_stereo(y_stereo: np.ndarray, fs: int, preamp_db: float, filters: List[BiquadSpec]) -> np.ndarray:
    y = y_stereo.astype(np.float32)
    y *= (10.0 ** (preamp_db / 20.0))
    for ch in (0, 1):
        x = y[:, ch]
        for spec in filters:
            b, a = design_biquad(spec, fs)
            x = process_biquad(x, b, a)
        y[:, ch] = x
    return y
