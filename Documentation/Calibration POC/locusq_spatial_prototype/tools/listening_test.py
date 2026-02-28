#!/usr/bin/env python3
"""
Randomized blind 2x2 listening test.
Reads pre-rendered condition WAVs, presents in random order, logs ratings.
"""
import json, random, pathlib, datetime

RATING_DIMS = ["externalization", "front_back_correct", "preference"]

def run_session(stimulus_dir: pathlib.Path, participant_id: str, out_dir: pathlib.Path):
    out_dir.mkdir(parents=True, exist_ok=True)
    conditions = sorted(stimulus_dir.glob("**/*.wav"))
    random.shuffle(conditions)

    results = []
    for wav in conditions:
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
