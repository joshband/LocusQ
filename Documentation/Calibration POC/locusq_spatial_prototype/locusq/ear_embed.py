"""Ear embedding utilities (optional dependency: torch/torchvision).

This is intentionally minimal; production should add:
- cropping/quality gating
- consistent orientation normalization
- ear-specific fine-tuning (later)
"""
from __future__ import annotations
import numpy as np

def cosine(a: np.ndarray, b: np.ndarray) -> float:
    a = a / (np.linalg.norm(a) + 1e-12)
    b = b / (np.linalg.norm(b) + 1e-12)
    return float(np.dot(a, b))

def _require_torch():
    try:
        import torch  # noqa
        import torchvision  # noqa
        from PIL import Image  # noqa
    except Exception as e:
        raise RuntimeError("Install optional deps: pip install torch torchvision pillow") from e

def build_encoder(device: str = "cpu"):
    _require_torch()
    import torch
    import torchvision.transforms as T
    from torchvision.models import mobilenet_v3_large

    model = mobilenet_v3_large(weights="DEFAULT")
    model.classifier = torch.nn.Identity()
    model.eval().to(device)
    tfm = T.Compose([
        T.Resize((224, 224)),
        T.ToTensor(),
        T.Normalize(mean=[0.485,0.456,0.406], std=[0.229,0.224,0.225]),
    ])
    return model, tfm

def embed_image(path: str, model, tfm, device: str = "cpu") -> np.ndarray:
    _require_torch()
    import torch
    from PIL import Image

    img = Image.open(path).convert("RGB")
    x = tfm(img).unsqueeze(0).to(device)
    with torch.no_grad():
        e = model(x).cpu().numpy().reshape(-1).astype(np.float32)
    e /= (np.linalg.norm(e) + 1e-12)
    return e
