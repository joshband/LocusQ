#!/usr/bin/env bash
# Title: BL-060 Phase B Listening Test QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-17
# Last Modified Date: 2026-03-17
#
# Exit codes:
#   0  all checks passed
#   1  one or more checks failed
#   2  usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl060_phase_b_listening_${TIMESTAMP}_$$"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
RESULTS_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl060-phase-b-listening-test-mac.sh [options]

BL-060 Phase B listening test harness.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks with fixture trial log + analysis
  --help, -h         Show usage

Outputs (in out-dir):
  status.tsv
  results.tsv
  trial_log.csv          (fixture; execute mode only)
  metrics_summary.tsv    (execute mode only)
  stats_report.md        (execute mode only)
  gate_decision.md       (execute mode only)
  reproducibility_check.tsv (execute mode only)
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

record() {
  local check_id="$1"
  local result="$2"
  local detail="$3"
  local artifact="${4:-}"
  printf "%s\t%s\t%s\t%s\n" \
    "$check_id" "$result" \
    "${detail//$'\t'/ }" \
    "${artifact//$'\t'/ }" \
    >> "$STATUS_TSV"
  if [[ "$result" == "PASS" ]]; then
    ((pass_count++)) || true
  else
    ((fail_count++)) || true
  fi
}

append_result() {
  local check="$1"
  local result="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$check" "$result" "${detail//$'\t'/ }" >> "$RESULTS_TSV"
}

count_todo_rows() {
  local file="$1"
  [[ -f "$file" ]] || { echo 0; return; }
  awk -F'\t' 'NR==1{next} { for(i=1;i<=NF;i++) if($i=="TODO"){count++;break} } END{print count+0}' "$file"
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires a value"
      OUT_DIR="$2"; shift 2 ;;
    --contract-only)
      (( MODE_SET == 1 )) && [[ "$MODE" != "contract_only" ]] && \
        usage_error "--contract-only cannot be combined with --execute"
      MODE="contract_only"; MODE_SET=1; shift ;;
    --execute)
      (( MODE_SET == 1 )) && [[ "$MODE" != "execute" ]] && \
        usage_error "--execute cannot be combined with --contract-only"
      MODE="execute"; MODE_SET=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) usage_error "unknown argument: $1" ;;
  esac
done

# ---------------------------------------------------------------------------
# Setup
# ---------------------------------------------------------------------------

command -v python3 >/dev/null 2>&1 || usage_error "python3 is required"
command -v awk     >/dev/null 2>&1 || usage_error "awk is required"

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
RESULTS_TSV="${OUT_DIR}/results.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$RESULTS_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-060-phase-b-listening-test-harness.md"
ANALYZE_SCRIPT="${ROOT_DIR}/scripts/bl060-analyze-results.py"
POC_DIR="${ROOT_DIR}/Documentation/Calibration POC/locusq_spatial_prototype/tools"
LISTENING_POC="${POC_DIR}/listening_test.py"
ANALYZE_POC="${POC_DIR}/analyze_results.py"

# ---------------------------------------------------------------------------
# C1: Runbook present
# ---------------------------------------------------------------------------

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL060-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL060-C1-backlog_doc_exists" "FAIL" "runbook missing at ${BACKLOG_DOC}" "$BACKLOG_DOC"
fi

# ---------------------------------------------------------------------------
# C2: Analysis script present
# ---------------------------------------------------------------------------

if [[ -f "$ANALYZE_SCRIPT" ]]; then
  record "BL060-C2-analysis_script_exists" "PASS" "bl060-analyze-results.py present" "$ANALYZE_SCRIPT"
else
  record "BL060-C2-analysis_script_exists" "FAIL" "bl060-analyze-results.py missing" "$ANALYZE_SCRIPT"
fi

# ---------------------------------------------------------------------------
# C3: POC research tools present
# ---------------------------------------------------------------------------

if [[ -f "$LISTENING_POC" && -f "$ANALYZE_POC" ]]; then
  record "BL060-C3-poc_tools_exist" "PASS" "listening_test.py and analyze_results.py present" "$POC_DIR"
else
  record "BL060-C3-poc_tools_exist" "FAIL" "one or more POC tools missing in ${POC_DIR}" "$POC_DIR"
fi

# ---------------------------------------------------------------------------
# C4: python3 can import csv + statistics (stdlib)
# ---------------------------------------------------------------------------

py_stdlib_ok=0
set +e
python3 -c "import csv, statistics, hashlib, pathlib, math" 2>/dev/null
py_stdlib_ok=$?
set -e

if [[ "$py_stdlib_ok" -eq 0 ]]; then
  record "BL060-C4-python3_stdlib" "PASS" "csv/statistics/hashlib available" ""
else
  record "BL060-C4-python3_stdlib" "FAIL" "python3 stdlib import failed" ""
fi

# ---------------------------------------------------------------------------
# C5: Trial schema contract — generate fixture and validate columns
# ---------------------------------------------------------------------------

FIXTURE_TRIAL_LOG="${OUT_DIR}/trial_log.csv"

python3 - <<PYEOF
import csv, math, random, pathlib

def generate_fixture(out_path, seed=42):
    rng = random.Random(seed)
    angles = [0, 45, 90, 135, 180, 225, 270, 315]
    conditions_generic    = ["generic_no_eq", "generic_device_eq"]
    conditions_personal   = ["personalized_no_eq", "personalized_device_eq"]
    participants = [f"P{i:03d}" for i in range(1, 6)]

    rows = []
    trial_id = 1
    for p in participants:
        for angle in angles:
            for cond in conditions_generic + conditions_personal:
                if cond in conditions_generic:
                    err_mean, err_std = 25.0, 8.0
                    ext_mean, ext_std = 2.5, 0.3
                else:
                    err_mean, err_std = 15.0, 6.0
                    ext_mean, ext_std = 3.7, 0.3
                err = max(0.0, rng.gauss(err_mean, err_std))
                direction = rng.choice([-1, 1])
                response = (angle + direction * err) % 360.0
                abs_err = abs(response - angle)
                if abs_err > 180.0:
                    abs_err = 360.0 - abs_err
                ext = round(max(1.0, min(5.0, rng.gauss(ext_mean, ext_std))), 2)
                rt  = round(max(300.0, rng.gauss(1000.0, 200.0)), 0)
                rows.append({
                    "participant_id":       p,
                    "trial_id":             f"T{trial_id:04d}",
                    "condition":            cond,
                    "true_angle_deg":       f"{angle:.1f}",
                    "response_angle_deg":   f"{response:.2f}",
                    "absolute_error_deg":   f"{abs_err:.2f}",
                    "reaction_time_ms":     f"{rt:.0f}",
                    "externalization_rating": f"{ext:.2f}",
                })
                trial_id += 1

    p = pathlib.Path(out_path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with open(p, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

generate_fixture("${FIXTURE_TRIAL_LOG}")
PYEOF

if [[ -f "$FIXTURE_TRIAL_LOG" ]]; then
  # Validate required columns
  fixture_header="$(head -1 "$FIXTURE_TRIAL_LOG")"
  schema_ok=1
  for col in participant_id trial_id condition true_angle_deg response_angle_deg absolute_error_deg reaction_time_ms; do
    if ! echo "$fixture_header" | grep -q "$col"; then
      schema_ok=0
      break
    fi
  done
  if [[ "$schema_ok" -eq 1 ]]; then
    fixture_rows="$(awk -F',' 'NR>1{count++} END{print count+0}' "$FIXTURE_TRIAL_LOG")"
    record "BL060-C5-fixture_schema" "PASS" "fixture generated; rows=${fixture_rows}" "$FIXTURE_TRIAL_LOG"
    append_result "trial_schema_contract" "PASS" "all required columns present; rows=${fixture_rows}"
  else
    record "BL060-C5-fixture_schema" "FAIL" "fixture missing required columns" "$FIXTURE_TRIAL_LOG"
    append_result "trial_schema_contract" "FAIL" "fixture missing required columns"
  fi
else
  record "BL060-C5-fixture_schema" "FAIL" "fixture generation failed" "$FIXTURE_TRIAL_LOG"
  append_result "trial_schema_contract" "FAIL" "fixture generation failed"
fi

# ---------------------------------------------------------------------------
# Execute-mode checks
# ---------------------------------------------------------------------------

if [[ "$MODE" == "execute" ]]; then

  ANALYSIS_OUT="${OUT_DIR}/analysis"

  # E1: Run analysis on fixture
  set +e
  python3 "$ANALYZE_SCRIPT" \
    --trial-log "$FIXTURE_TRIAL_LOG" \
    --out-dir "$ANALYSIS_OUT" \
    > "${OUT_DIR}/analysis_stdout.log" 2> "${OUT_DIR}/analysis_stderr.log"
  analysis_ec=$?
  set -e

  if [[ "$analysis_ec" -eq 0 ]]; then
    record "BL060-E1-analysis_runs" "PASS" "bl060-analyze-results.py exit=0" "$ANALYSIS_OUT"
  else
    record "BL060-E1-analysis_runs" "FAIL" "bl060-analyze-results.py exit=${analysis_ec}" "$ANALYSIS_OUT"
  fi

  # E2: Verify all 5 required artifacts
  required_artifacts=(
    "${ANALYSIS_OUT}/metrics_summary.tsv"
    "${ANALYSIS_OUT}/stats_report.md"
    "${ANALYSIS_OUT}/gate_decision.md"
    "${ANALYSIS_OUT}/reproducibility_check.tsv"
  )
  # trial_log.csv is written by the harness directly (fixture)
  required_artifacts+=("$FIXTURE_TRIAL_LOG")

  artifacts_ok=1
  for artifact in "${required_artifacts[@]}"; do
    if [[ ! -f "$artifact" ]]; then
      artifacts_ok=0
      record "BL060-E2-artifact_missing" "FAIL" "missing: ${artifact}" "$artifact"
    fi
  done
  if [[ "$artifacts_ok" -eq 1 ]]; then
    record "BL060-E2-required_artifacts" "PASS" "all 5 required artifacts present" "$ANALYSIS_OUT"
  fi

  # E3: Extract and record gate decision
  gate_result="UNKNOWN"
  if [[ -f "${ANALYSIS_OUT}/gate_decision.md" ]]; then
    gate_result="$(grep '^- result:' "${ANALYSIS_OUT}/gate_decision.md" | awk '{print $NF}' || echo UNKNOWN)"
  fi
  ext_pct="$(grep 'ext_improvement_pct' "${ANALYSIS_OUT}/gate_decision.md" 2>/dev/null | head -1 | awk '{print $NF}' || echo n/a)"
  p_val="$(grep '^- p_value:' "${ANALYSIS_OUT}/gate_decision.md" 2>/dev/null | awk '{print $NF}' || echo n/a)"

  append_result "gate_decision" "$gate_result" "ext_improvement=${ext_pct} p_value=${p_val}"
  if [[ "$gate_result" == "PASS" ]]; then
    record "BL060-E3-gate_decision" "PASS" "gate=${gate_result} ext=${ext_pct} p=${p_val}" "${ANALYSIS_OUT}/gate_decision.md"
  else
    record "BL060-E3-gate_decision" "FAIL" "gate=${gate_result} ext=${ext_pct} p=${p_val}" "${ANALYSIS_OUT}/gate_decision.md"
  fi

  # E4: Reproducibility check — run analysis a second time, compare gate_hash
  ANALYSIS_OUT2="${OUT_DIR}/analysis_repro"
  set +e
  python3 "$ANALYZE_SCRIPT" \
    --trial-log "$FIXTURE_TRIAL_LOG" \
    --out-dir "$ANALYSIS_OUT2" \
    >/dev/null 2>&1
  repro_ec=$?
  set -e

  repro_result="FAIL"
  if [[ "$repro_ec" -eq 0 &&
        -f "${ANALYSIS_OUT}/reproducibility_check.tsv" &&
        -f "${ANALYSIS_OUT2}/reproducibility_check.tsv" ]]; then
    hash1="$(awk -F'\t' '$1=="gate_hash_prefix"{print $2}' "${ANALYSIS_OUT}/reproducibility_check.tsv")"
    hash2="$(awk -F'\t' '$1=="gate_hash_prefix"{print $2}' "${ANALYSIS_OUT2}/reproducibility_check.tsv")"
    if [[ "$hash1" == "$hash2" && -n "$hash1" ]]; then
      repro_result="PASS"
    fi
  fi

  append_result "reproducibility" "$repro_result" "run1_gate_hash=${hash1:-?} run2_gate_hash=${hash2:-?}"
  record "BL060-E4-reproducibility" "$repro_result" \
    "run1_gate_hash=${hash1:-?} run2_gate_hash=${hash2:-?}" \
    "${ANALYSIS_OUT}/reproducibility_check.tsv"

  # E5: No TODO rows in execute mode
  todo_rows="$(count_todo_rows "$RESULTS_TSV")"
  if [[ "$todo_rows" -eq 0 ]]; then
    record "BL060-E5-execute_no_todo" "PASS" "execute mode: zero TODO rows" "$RESULTS_TSV"
  else
    record "BL060-E5-execute_no_todo" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$RESULTS_TSV"
  fi

else
  # Contract-only: emit TODO rows for execute-mode checks
  append_result "gate_decision"    "TODO" "analysis on real participant data pending"
  append_result "reproducibility"  "TODO" "reproducibility check pending"

  todo_rows="$(count_todo_rows "$RESULTS_TSV")"
  record "BL060-C6-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

# ---------------------------------------------------------------------------
# Lane result
# ---------------------------------------------------------------------------

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl060_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $RESULTS_TSV"
[[ "$MODE" == "execute" ]] && echo "- ${OUT_DIR}/analysis/"
[[ "$MODE" == "execute" ]] && echo "- $FIXTURE_TRIAL_LOG"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
