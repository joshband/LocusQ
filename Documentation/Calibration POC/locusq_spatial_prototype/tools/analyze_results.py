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
    generic_ext  = [r["externalization"] for r in all_results if "generic_no_eq"      in r["condition"]]
    personal_ext = [r["externalization"] for r in all_results if "personalized_no_eq" in r["condition"]]
    fb_correct   = [r["front_back_correct"] for r in all_results]

    if len(generic_ext) < 2 or len(personal_ext) < 2:
        print("Insufficient data for statistical test.")
        return None

    _, p_value = stats.ttest_rel(personal_ext, generic_ext)
    mean_improvement = (statistics.mean(personal_ext) - statistics.mean(generic_ext)) / statistics.mean(generic_ext) * 100
    fb_accuracy = statistics.mean(fb_correct) * 100

    print(f"Mean externalization improvement: {mean_improvement:.1f}%")
    print(f"p-value (personalized vs generic): {p_value:.4f}")
    print(f"Front/back accuracy: {fb_accuracy:.1f}%")

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
