import argparse
from locusq.renderer_offline import render_binaural_offline

# ---------------------------------------------------------------------------
# Phase B condition definitions
# ---------------------------------------------------------------------------

# (label, azimuth_deg, elevation_deg)
SCENES = [
    ("front",    0,   0),
    ("left",     90,  0),
    ("right",   -90,  0),
    ("rear",    180,  0),
    ("elevated",  0, 30),
]

# (label, hrtf_key, headphone_eq_preset or None)
CONDITIONS = [
    ("generic_no_eq",       "H3",             None),
    ("personalized_no_eq",  "matched_subject", None),
    ("generic_eq",          "H3",             "sony_wh1000xm5"),
    ("personalized_eq",     "matched_subject", "sony_wh1000xm5"),
]

# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Offline SOFA binaural renderer (prototype).")
    ap.add_argument("--sofa", required=True, help="Path to SOFA HRIR file")
    ap.add_argument("--inwav", required=True, help="Input WAV (mono or stereo; will be mixed to mono)")
    ap.add_argument("--outwav", required=True, help="Output binaural WAV (stereo)")
    ap.add_argument("--az", type=float, default=0.0, help="Azimuth deg (0 front, +left)")
    ap.add_argument("--el", type=float, default=0.0, help="Elevation deg")
    ap.add_argument("--eq", default=None, help="Path to headphone EQ preset YAML")
    ap.add_argument("--no-normalize", action="store_true", help="Disable safety normalization")
    args = ap.parse_args()

    idx = render_binaural_offline(
        sofa_path=args.sofa,
        input_wav=args.inwav,
        output_wav=args.outwav,
        az_deg=args.az,
        el_deg=args.el,
        headphone_preset_yaml=args.eq,
        normalize=not args.no_normalize,
    )
    print(f"Rendered using measurement index {idx}")

if __name__ == "__main__":
    main()
