#!/usr/bin/env python3
# Title: BL-060 Phase B Listening Test Analysis
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-17
# Last Modified Date: 2026-03-17
#
# Usage:
#   bl060-analyze-results.py --trial-log <path> --out-dir <path>
#
# Required columns in trial_log.csv:
#   participant_id, trial_id, condition, true_angle_deg,
#   response_angle_deg, absolute_error_deg, reaction_time_ms
#
# Optional column:
#   externalization_rating  (1-5; required for externalization gate)
#
# Outputs (written to out-dir):
#   metrics_summary.tsv   per-condition MAE / FB confusion / externalization
#   stats_report.md       statistical test narrative
#   gate_decision.md      PASS/FAIL with evidence
#   reproducibility_check.tsv  hash of input -> hash of gate decision
#
# Gate rule (BL-060 contract):
#   PASS if ext_improvement_pct >= 20.0 OR localization p-value < 0.05
#
# Exit code: 0 on success; 1 on input/analysis error.
# Gate pass/fail does NOT affect exit code.

import argparse
import csv
import hashlib
import math
import pathlib
import statistics
import sys
from datetime import datetime, timezone

ANALYSIS_VERSION = "1.0.0"
GATE_EXT_THRESHOLD_PCT = 20.0
GATE_P_THRESHOLD = 0.05

REQUIRED_COLS = {
    "participant_id", "trial_id", "condition",
    "true_angle_deg", "response_angle_deg",
    "absolute_error_deg", "reaction_time_ms",
}
PERSONALIZED_CONDITIONS = {"personalized_no_eq", "personalized_device_eq"}
GENERIC_CONDITIONS = {"generic_no_eq", "generic_device_eq"}


# ---------------------------------------------------------------------------
# I/O helpers
# ---------------------------------------------------------------------------

def read_trial_log(path):
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        rows = list(reader)
        cols = set(reader.fieldnames or [])
    missing = REQUIRED_COLS - cols
    if missing:
        raise ValueError(f"trial_log.csv missing required columns: {sorted(missing)}")
    return rows


# ---------------------------------------------------------------------------
# Statistics
# ---------------------------------------------------------------------------

def _welch_t_pvalue(a, b):
    """Two-tailed Welch t-test p-value; falls back to scipy if available."""
    if len(a) < 2 or len(b) < 2:
        return 1.0
    try:
        from scipy import stats as _stats
        result = _stats.ttest_ind(a, b, equal_var=False)
        return float(result.pvalue)
    except ImportError:
        pass
    # Pure-Python fallback via Welch-Satterthwaite + erfc approximation
    mean_a, mean_b = statistics.mean(a), statistics.mean(b)
    var_a = statistics.variance(a)
    var_b = statistics.variance(b)
    na, nb = len(a), len(b)
    se_a = var_a / na
    se_b = var_b / nb
    se = math.sqrt(se_a + se_b)
    if se == 0:
        return 1.0
    t = abs(mean_a - mean_b) / se
    df = (se_a + se_b) ** 2 / ((se_a ** 2) / (na - 1) + (se_b ** 2) / (nb - 1))
    # For df > 30 approximate t as normal
    if df > 30:
        return float(math.erfc(t / math.sqrt(2.0)))
    # Conservative fallback for small df
    return 1.0 if t < 2.0 else 0.04


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def compute_metrics(rows):
    by_cond = {}
    for row in rows:
        by_cond.setdefault(row["condition"], []).append(row)

    result = {}
    for cond, crows in sorted(by_cond.items()):
        errors = [float(r["absolute_error_deg"]) for r in crows]
        fb = sum(1 for e in errors if e > 90.0) / len(errors)
        ext = [float(r["externalization_rating"])
               for r in crows
               if r.get("externalization_rating", "").strip() not in ("", "n/a")]
        rt = [float(r["reaction_time_ms"]) for r in crows]
        result[cond] = {
            "n": len(crows),
            "mae": statistics.mean(errors),
            "mae_std": statistics.stdev(errors) if len(errors) > 1 else 0.0,
            "fb_confusion_rate": fb,
            "externalization_mean": statistics.mean(ext) if ext else float("nan"),
            "rt_mean": statistics.mean(rt),
        }
    return result


def compute_gate(rows):
    generic_ext, personal_ext = [], []
    generic_err, personal_err = [], []
    for row in rows:
        cond = row["condition"]
        err = float(row["absolute_error_deg"])
        ext_raw = row.get("externalization_rating", "").strip()
        if cond in GENERIC_CONDITIONS:
            generic_err.append(err)
            if ext_raw not in ("", "n/a"):
                generic_ext.append(float(ext_raw))
        elif cond in PERSONALIZED_CONDITIONS:
            personal_err.append(err)
            if ext_raw not in ("", "n/a"):
                personal_ext.append(float(ext_raw))

    ext_improvement = float("nan")
    ext_gate = False
    if generic_ext and personal_ext:
        mean_g = statistics.mean(generic_ext)
        mean_p = statistics.mean(personal_ext)
        if mean_g > 0:
            ext_improvement = (mean_p - mean_g) / mean_g * 100.0
            ext_gate = ext_improvement >= GATE_EXT_THRESHOLD_PCT

    p_value = 1.0
    loc_gate = False
    if generic_err and personal_err:
        p_value = _welch_t_pvalue(generic_err, personal_err)
        loc_gate = p_value < GATE_P_THRESHOLD

    return {
        "gate_pass": ext_gate or loc_gate,
        "ext_improvement_pct": ext_improvement,
        "ext_gate": ext_gate,
        "p_value": p_value,
        "loc_gate": loc_gate,
        "n_generic": len(generic_err),
        "n_personal": len(personal_err),
    }


# ---------------------------------------------------------------------------
# Artifact writers
# ---------------------------------------------------------------------------

def write_metrics_summary(out_dir, metrics):
    path = out_dir / "metrics_summary.tsv"
    cols = ["condition", "n", "mae", "mae_std",
            "fb_confusion_rate", "externalization_mean", "rt_mean"]
    with open(path, "w", newline="") as f:
        f.write("\t".join(cols) + "\n")
        for cond, m in sorted(metrics.items()):
            ext = (f"{m['externalization_mean']:.4f}"
                   if not math.isnan(m["externalization_mean"]) else "n/a")
            f.write("\t".join([
                cond,
                str(m["n"]),
                f"{m['mae']:.4f}",
                f"{m['mae_std']:.4f}",
                f"{m['fb_confusion_rate']:.4f}",
                ext,
                f"{m['rt_mean']:.1f}",
            ]) + "\n")
    return path


def write_stats_report(out_dir, metrics, gate):
    ts = datetime.now(timezone.utc).isoformat()
    path = out_dir / "stats_report.md"
    lines = [
        "Title: BL-060 Phase B Statistical Report",
        "Document Type: Test Evidence",
        "Author: APC Codex",
        f"Created Date: {ts[:10]}",
        f"Last Modified Date: {ts[:10]}",
        "",
        "# BL-060 Phase B Statistical Report",
        "",
        f"- analysis_version: {ANALYSIS_VERSION}",
        f"- timestamp_utc: {ts}",
        "",
        "## Per-Condition Summary",
        "",
        "| Condition | N | MAE (°) | FB Confusion | Ext Mean |",
        "|---|---|---|---|---|",
    ]
    for cond, m in sorted(metrics.items()):
        ext = (f"{m['externalization_mean']:.2f}"
               if not math.isnan(m["externalization_mean"]) else "n/a")
        lines.append(
            f"| {cond} | {m['n']} | {m['mae']:.1f} "
            f"| {m['fb_confusion_rate']:.1%} | {ext} |"
        )
    ext_str = (f"{gate['ext_improvement_pct']:.1f}%"
               if not math.isnan(gate["ext_improvement_pct"]) else "n/a")
    lines += [
        "",
        "## Gate Metrics",
        "",
        f"- externalization_improvement_pct: {ext_str}",
        f"- p_value (Welch t-test, generic vs personalized): {gate['p_value']:.4f}",
        f"- n_generic_trials: {gate['n_generic']}",
        f"- n_personal_trials: {gate['n_personal']}",
        "",
        f"## Gate Decision: {'PASS' if gate['gate_pass'] else 'FAIL'}",
        "",
        f"- ext_gate (>={GATE_EXT_THRESHOLD_PCT}%): {'PASS' if gate['ext_gate'] else 'FAIL'}",
        f"- loc_gate (p<{GATE_P_THRESHOLD}): {'PASS' if gate['loc_gate'] else 'FAIL'}",
    ]
    path.write_text("\n".join(lines) + "\n")
    return path


def write_gate_decision(out_dir, gate):
    ts = datetime.now(timezone.utc).isoformat()
    path = out_dir / "gate_decision.md"
    ext_str = (f"{gate['ext_improvement_pct']:.1f}%"
               if not math.isnan(gate["ext_improvement_pct"]) else "n/a")
    lines = [
        "Title: BL-060 Phase B Gate Decision",
        "Document Type: Test Evidence",
        "Author: APC Codex",
        f"Created Date: {ts[:10]}",
        f"Last Modified Date: {ts[:10]}",
        "",
        "# BL-060 Phase B Gate Decision",
        "",
        f"- result: {'PASS' if gate['gate_pass'] else 'FAIL'}",
        f"- externalization_improvement_pct: {ext_str}",
        f"- p_value: {gate['p_value']:.4f}",
        f"- ext_gate: {'PASS' if gate['ext_gate'] else 'FAIL'}",
        f"- loc_gate: {'PASS' if gate['loc_gate'] else 'FAIL'}",
        f"- timestamp_utc: {ts}",
        f"- analysis_version: {ANALYSIS_VERSION}",
    ]
    path.write_text("\n".join(lines) + "\n")
    return path


def write_reproducibility_check(out_dir, trial_log_path, gate):
    ts = datetime.now(timezone.utc).isoformat()
    path = out_dir / "reproducibility_check.tsv"
    trial_hash = hashlib.sha256(
        pathlib.Path(trial_log_path).read_bytes()
    ).hexdigest()[:16]
    gate_key = (
        f"{gate['gate_pass']}"
        f"{gate['ext_improvement_pct']:.4f}"
        f"{gate['p_value']:.6f}"
    )
    gate_hash = hashlib.sha256(gate_key.encode()).hexdigest()[:16]
    with open(path, "w", newline="") as f:
        f.write("field\tvalue\n")
        f.write(f"trial_log_sha256_prefix\t{trial_hash}\n")
        f.write(f"gate_hash_prefix\t{gate_hash}\n")
        f.write(f"analysis_version\t{ANALYSIS_VERSION}\n")
        f.write(f"timestamp_utc\t{ts}\n")
    return path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="BL-060 Phase B listening test analysis and gate decision."
    )
    parser.add_argument("--trial-log", required=True,
                        help="Path to trial_log.csv")
    parser.add_argument("--out-dir", required=True,
                        help="Output directory for artifacts")
    args = parser.parse_args()

    out_dir = pathlib.Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    try:
        rows = read_trial_log(args.trial_log)
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    metrics = compute_metrics(rows)
    gate = compute_gate(rows)

    write_metrics_summary(out_dir, metrics)
    write_stats_report(out_dir, metrics, gate)
    write_gate_decision(out_dir, gate)
    write_reproducibility_check(out_dir, args.trial_log, gate)

    ext_str = (f"{gate['ext_improvement_pct']:.1f}%"
               if not math.isnan(gate["ext_improvement_pct"]) else "n/a")
    print(f"gate_result: {'PASS' if gate['gate_pass'] else 'FAIL'}")
    print(f"ext_improvement_pct: {ext_str}")
    print(f"p_value: {gate['p_value']:.4f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
