#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

DEFAULT_OUT_DIR="$ROOT_DIR/TestEvidence/bl028_slice_b1_${TIMESTAMP}"
OUT_DIR="${BL028_OUT_DIR:-$DEFAULT_OUT_DIR}"
RUNS="${BL028_RUNS:-}"
SCENARIO_PATH="${BL028_SCENARIO_PATH:-$ROOT_DIR/qa/scenarios/locusq_bl028_output_matrix_suite.json}"
QA_BIN="${BL028_QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL028_QA_BIN_FALLBACK:-$ROOT_DIR/build_local/locusq_qa_artefacts/locusq_qa}"
fi

RUNBOOK_PATH="$ROOT_DIR/Documentation/backlog/done/bl-028-spatial-output-matrix.md"
SPEC_PATH="$ROOT_DIR/Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md"
QA_DOC_PATH="$ROOT_DIR/Documentation/testing/bl-028-spatial-output-matrix-qa.md"
SCRIPT_PATH="$ROOT_DIR/scripts/qa-bl028-output-matrix-lane-mac.sh"

usage() {
  cat <<USAGE
Usage: ./scripts/qa-bl028-output-matrix-lane-mac.sh [--out-dir <path>] [--runs <N>]

Options:
  --out-dir <path>  Explicit artifact output directory (overrides BL028_OUT_DIR).
  --runs <N>        Replay runs count, integer >= 1 (overrides BL028_RUNS/scenario default).
  --help            Show usage.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --out-dir requires a path" >&2
        usage >&2
        exit 2
      fi
      OUT_DIR="$2"
      shift 2
      ;;
    --runs)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --runs requires an integer value" >&2
        usage >&2
        exit 2
      fi
      RUNS="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
QA_LANE_LOG="$OUT_DIR/qa_lane.log"
SCENARIO_RESULT_LOG="$OUT_DIR/scenario_result.log"
MATRIX_REPORT_TSV="$OUT_DIR/matrix_report.tsv"
ACCEPTANCE_PARITY_TSV="$OUT_DIR/acceptance_parity.tsv"
BUILD_LOG="$OUT_DIR/build.log"
SCENARIO_RUN_LOG="$OUT_DIR/scenario_run.log"
RESULT_COPY_JSON="$OUT_DIR/scenario_result.json"
MATRIX_SUMMARY_JSON="$OUT_DIR/matrix_summary.json"
REPLAY_RUNS_TSV="$OUT_DIR/replay_runs.tsv"
REPLAY_HASHES_TSV="$OUT_DIR/replay_hashes.tsv"
RELIABILITY_DECISION_MD="$OUT_DIR/reliability_decision.md"

printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
: >"$SCENARIO_RESULT_LOG"
printf "run\tqa_exit\tresult_status\twarnings\tpassed\tfailed\ttotal\tresult_json\tresult_sha256\tmatrix_sha256\tcombined_signature\tbaseline_match\trun_result\tfailure_class\n" >"$REPLAY_RUNS_TSV"
printf "run\tresult_json_sha256\tmatrix_report_sha256\tcombined_signature\tbaseline_signature\tsignature_match\n" >"$REPLAY_HASHES_TSV"

exec 3>&1
exec >>"$QA_LANE_LOG" 2>&1

log_status() {
  local check="$1"
  local result="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" "$check" "$result" "$detail" "$artifact" >>"$STATUS_TSV"
  printf "%s: %s - %s\n" "$check" "$result" "$detail"
}

mark_fail_count=0
mark_fail() {
  mark_fail_count=$((mark_fail_count + 1))
}

append_failure_class() {
  local current="$1"
  local new_class="$2"
  if [[ "$current" == "none" || -z "$current" ]]; then
    printf "%s" "$new_class"
  else
    printf "%s,%s" "$current" "$new_class"
  fi
}

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "PASS" "$(command -v "$cmd")" ""
  else
    log_status "tool_${cmd}" "FAIL" "missing_command" ""
    mark_fail
  fi
}

echo "BL-028 deterministic output-matrix lane start: $DOC_TS"
echo "out_dir=$OUT_DIR"
echo "scenario=$SCENARIO_PATH"
echo "qa_bin=$QA_BIN"

require_cmd jq
require_cmd python3
require_cmd rg
require_cmd shasum

if [[ ! -f "$SCENARIO_PATH" ]]; then
  log_status "scenario_file" "FAIL" "missing=$SCENARIO_PATH" "$SCENARIO_PATH"
  mark_fail
fi
if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "FAIL" "missing_or_not_executable=$QA_BIN" "$QA_BIN"
  mark_fail
fi

if [[ "$mark_fail_count" -ne 0 ]]; then
  log_status "lane_prereq" "FAIL" "prerequisite_failure_count=$mark_fail_count" "$STATUS_TSV"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

SCENARIO_ID="$(jq -r '.id // empty' "$SCENARIO_PATH")"
if [[ -z "$SCENARIO_ID" ]]; then
  log_status "scenario_id" "FAIL" "missing_id_in_scenario" "$SCENARIO_PATH"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

if [[ -z "$RUNS" ]]; then
  RUNS="$(jq -r '.bl028_contract_checks.replay_contract.default_runs // 1' "$SCENARIO_PATH")"
fi
if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  log_status "replay_runs_requested" "FAIL" "invalid_runs_value=$RUNS" "$SCENARIO_PATH"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

max_runs_contract="$(jq -r '.bl028_contract_checks.replay_contract.max_runs // 10' "$SCENARIO_PATH")"
max_signature_divergence="$(jq -r '.bl028_contract_checks.replay_contract.max_signature_divergence // 0' "$SCENARIO_PATH")"
max_transient_failures="$(jq -r '.bl028_contract_checks.replay_contract.max_transient_failures // 0' "$SCENARIO_PATH")"

if ! [[ "$max_runs_contract" =~ ^[0-9]+$ ]]; then
  max_runs_contract=10
fi
if ! [[ "$max_signature_divergence" =~ ^[0-9]+$ ]]; then
  max_signature_divergence=0
fi
if ! [[ "$max_transient_failures" =~ ^[0-9]+$ ]]; then
  max_transient_failures=0
fi

if [[ "$RUNS" -le "$max_runs_contract" ]]; then
  log_status "replay_runs_requested" "PASS" "runs=$RUNS max_runs_contract=$max_runs_contract" "$SCENARIO_PATH"
else
  log_status "replay_runs_requested" "FAIL" "runs=$RUNS exceeds max_runs_contract=$max_runs_contract" "$SCENARIO_PATH"
  mark_fail
fi

set +e
cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8 >"$BUILD_LOG" 2>&1
build_exit=$?
set -e
if [[ "$build_exit" -eq 0 ]]; then
  log_status "build_targets" "PASS" "cmake_build_exit=0" "$BUILD_LOG"
else
  log_status "build_targets" "FAIL" "cmake_build_exit=$build_exit" "$BUILD_LOG"
  mark_fail
fi

suite_status_threshold="$(jq -r '.bl028_contract_checks.thresholds.suite_status // "PASS"' "$SCENARIO_PATH")"
warnings_max_threshold="$(jq -r '.bl028_contract_checks.thresholds.max_warnings // 0' "$SCENARIO_PATH")"

set +e
python3 - "$SCENARIO_PATH" "$MATRIX_REPORT_TSV" "$MATRIX_SUMMARY_JSON" >"$OUT_DIR/matrix_contract_eval.log" 2>&1 <<'PY'
import json
import sys

scenario_path, report_path, summary_path = sys.argv[1:4]

with open(scenario_path, "r", encoding="utf-8") as fh:
    scenario = json.load(fh)

checks = scenario.get("bl028_contract_checks") or {}
cases = checks.get("matrix_cases") or []
acceptance_entries = checks.get("acceptance_ids") or []
allowed_enums = checks.get("allowed_enums") or {}
status_text_map_entries = checks.get("status_text_map") or []
diag_fields = checks.get("required_diagnostics_fields") or []

acceptance_ids = [entry.get("id", "") for entry in acceptance_entries if entry.get("id")]
allowed_domains = set(allowed_enums.get("domain") or [])
allowed_layouts = set(allowed_enums.get("layout") or [])
allowed_decisions = set(allowed_enums.get("decision") or [])
allowed_fallbacks = set(allowed_enums.get("fallback_mode") or [])
allowed_routes = set(allowed_enums.get("fail_safe_route") or [])
reason_to_text = {
    entry.get("reason_code", ""): entry.get("status_text", "")
    for entry in status_text_map_entries
    if entry.get("reason_code")
}

rows = []
acceptance_coverage = {aid: 0 for aid in acceptance_ids}
passed_cases = 0
blocked_total = 0
blocked_passed = 0
legal_total = 0
legal_passed = 0

for case in cases:
    case_id = str(case.get("case_id", ""))
    acceptance_id = str(case.get("acceptance_id", ""))
    requested_domain = str(case.get("requested_domain", ""))
    host_layout = str(case.get("host_layout", ""))
    head_tracking = str(case.get("head_tracking", ""))
    expected_decision = str(case.get("expected_decision", ""))
    expected_rule_id = str(case.get("expected_rule_id", ""))
    expected_fallback_mode = str(case.get("expected_fallback_mode", ""))
    expected_fail_safe_route = str(case.get("expected_fail_safe_route", ""))
    expected_reason_code = str(case.get("expected_reason_code", ""))
    expected_status_text = str(case.get("expected_status_text", ""))

    if acceptance_id in acceptance_coverage:
        acceptance_coverage[acceptance_id] += 1

    row_ok = True
    details = []

    required_values = {
        "case_id": case_id,
        "acceptance_id": acceptance_id,
        "requested_domain": requested_domain,
        "host_layout": host_layout,
        "head_tracking": head_tracking,
        "expected_decision": expected_decision,
        "expected_rule_id": expected_rule_id,
        "expected_fallback_mode": expected_fallback_mode,
        "expected_fail_safe_route": expected_fail_safe_route,
        "expected_reason_code": expected_reason_code,
        "expected_status_text": expected_status_text,
    }
    for key, value in required_values.items():
        if value == "":
            row_ok = False
            details.append(f"missing_{key}")

    if acceptance_id not in acceptance_ids:
        row_ok = False
        details.append("unknown_acceptance_id")
    if requested_domain not in allowed_domains:
        row_ok = False
        details.append("invalid_domain")
    if host_layout not in allowed_layouts:
        row_ok = False
        details.append("invalid_layout")
    if expected_decision not in allowed_decisions:
        row_ok = False
        details.append("invalid_decision")
    if expected_fallback_mode not in allowed_fallbacks:
        row_ok = False
        details.append("invalid_fallback_mode")
    if expected_fail_safe_route not in allowed_routes:
        row_ok = False
        details.append("invalid_fail_safe_route")

    mapped_status_text = reason_to_text.get(expected_reason_code, "")
    if mapped_status_text == "":
        row_ok = False
        details.append("reason_code_missing_from_map")
    elif mapped_status_text != expected_status_text:
        row_ok = False
        details.append("status_text_map_mismatch")

    if expected_decision == "ALLOW":
        legal_total += 1
        if expected_fallback_mode != "none":
            row_ok = False
            details.append("allow_requires_no_fallback")
        if expected_fail_safe_route != "none":
            row_ok = False
            details.append("allow_requires_no_fail_safe_route")
        if expected_reason_code != "ok":
            row_ok = False
            details.append("allow_requires_ok_reason")
    elif expected_decision == "BLOCK":
        blocked_total += 1
        if expected_fallback_mode == "none":
            row_ok = False
            details.append("block_requires_fallback_mode")
        if expected_fail_safe_route == "none":
            row_ok = False
            details.append("block_requires_fail_safe_route")
        if expected_fallback_mode == "safe_stereo_passthrough" and expected_reason_code != "fallback_safe_stereo_passthrough":
            row_ok = False
            details.append("safe_stereo_reason_mismatch")

    if row_ok:
        passed_cases += 1
        if expected_decision == "BLOCK":
            blocked_passed += 1
        if expected_decision == "ALLOW":
            legal_passed += 1

    rows.append(
        [
            case_id,
            acceptance_id,
            requested_domain,
            host_layout,
            head_tracking,
            expected_decision,
            expected_rule_id,
            expected_fallback_mode,
            expected_fail_safe_route,
            expected_reason_code,
            expected_status_text,
            "PASS" if row_ok else "FAIL",
            ";".join(details) if details else "ok",
        ]
    )

total_cases = len(cases)
failed_cases = total_cases - passed_cases
matrix_accuracy = 1.0 if total_cases == 0 else float(passed_cases) / float(total_cases)
fallback_accuracy = 1.0 if blocked_total == 0 else float(blocked_passed) / float(blocked_total)

with open(report_path, "w", encoding="utf-8") as out:
    out.write(
        "case_id\tacceptance_id\trequested_domain\thost_layout\thead_tracking\texpected_decision\texpected_rule_id\t"
        "expected_fallback_mode\texpected_fail_safe_route\texpected_reason_code\texpected_status_text\trow_result\trow_detail\n"
    )
    for row in rows:
        out.write("\t".join(row))
        out.write("\n")

summary = {
    "total_cases": total_cases,
    "passed_cases": passed_cases,
    "failed_cases": failed_cases,
    "matrix_accuracy": matrix_accuracy,
    "legal_total": legal_total,
    "legal_passed": legal_passed,
    "blocked_total": blocked_total,
    "blocked_passed": blocked_passed,
    "fallback_accuracy": fallback_accuracy,
    "diagnostics_field_count": len(diag_fields),
    "status_text_map_count": len(status_text_map_entries),
    "acceptance_coverage": acceptance_coverage,
}

with open(summary_path, "w", encoding="utf-8") as out:
    json.dump(summary, out, indent=2, sort_keys=True)
PY
matrix_eval_exit=$?
set -e

if [[ "$matrix_eval_exit" -eq 0 ]]; then
  log_status "matrix_contract_eval" "PASS" "matrix_report_generated" "$MATRIX_REPORT_TSV"
else
  log_status "matrix_contract_eval" "FAIL" "matrix_eval_exit=$matrix_eval_exit" "$OUT_DIR/matrix_contract_eval.log"
  mark_fail
fi

matrix_accuracy="$(jq -r '.matrix_accuracy // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"
fallback_accuracy="$(jq -r '.fallback_accuracy // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"
legal_total="$(jq -r '.legal_total // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"
legal_passed="$(jq -r '.legal_passed // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"
blocked_total="$(jq -r '.blocked_total // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"
blocked_passed="$(jq -r '.blocked_passed // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"
diag_field_count="$(jq -r '.diagnostics_field_count // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"
status_text_map_count="$(jq -r '.status_text_map_count // 0' "$MATRIX_SUMMARY_JSON" 2>/dev/null || echo "0")"

matrix_accuracy_threshold="$(jq -r '.bl028_contract_checks.thresholds.matrix_accuracy // 1.0' "$SCENARIO_PATH")"
fallback_accuracy_threshold="$(jq -r '.bl028_contract_checks.thresholds.fallback_accuracy // 1.0' "$SCENARIO_PATH")"
diag_min_threshold="$(jq -r '.bl028_contract_checks.thresholds.diagnostics_schema_min_fields // 11' "$SCENARIO_PATH")"
status_map_min_threshold="$(jq -r '.bl028_contract_checks.thresholds.status_text_map_min_entries // 7' "$SCENARIO_PATH")"

if [[ "$legal_total" -gt 0 && "$legal_passed" -eq "$legal_total" ]]; then
  log_status "BL028-A1-001_matrix_legality" "PASS" "legal_passed=$legal_passed legal_total=$legal_total" "$MATRIX_REPORT_TSV"
else
  log_status "BL028-A1-001_matrix_legality" "FAIL" "legal_passed=$legal_passed legal_total=$legal_total" "$MATRIX_REPORT_TSV"
  mark_fail
fi

if python3 - "$fallback_accuracy" "$fallback_accuracy_threshold" <<'PY'
import sys
actual = float(sys.argv[1])
threshold = float(sys.argv[2])
sys.exit(0 if actual >= threshold else 1)
PY
then
  if [[ "$blocked_total" -gt 0 && "$blocked_passed" -eq "$blocked_total" ]]; then
    log_status "BL028-A1-002_fallback_contract" "PASS" "blocked_passed=$blocked_passed blocked_total=$blocked_total fallback_accuracy=$fallback_accuracy" "$MATRIX_REPORT_TSV"
  else
    log_status "BL028-A1-002_fallback_contract" "FAIL" "blocked_passed=$blocked_passed blocked_total=$blocked_total fallback_accuracy=$fallback_accuracy" "$MATRIX_REPORT_TSV"
    mark_fail
  fi
else
  log_status "BL028-A1-002_fallback_contract" "FAIL" "fallback_accuracy=$fallback_accuracy threshold=$fallback_accuracy_threshold" "$MATRIX_REPORT_TSV"
  mark_fail
fi

if [[ "$diag_field_count" -ge "$diag_min_threshold" ]]; then
  log_status "BL028-A1-003_diagnostics_schema" "PASS" "field_count=$diag_field_count threshold=$diag_min_threshold" "$SCENARIO_PATH"
else
  log_status "BL028-A1-003_diagnostics_schema" "FAIL" "field_count=$diag_field_count threshold=$diag_min_threshold" "$SCENARIO_PATH"
  mark_fail
fi

if [[ "$status_text_map_count" -ge "$status_map_min_threshold" ]]; then
  log_status "BL028-A1-004_status_text_map" "PASS" "entry_count=$status_text_map_count threshold=$status_map_min_threshold" "$SCENARIO_PATH"
else
  log_status "BL028-A1-004_status_text_map" "FAIL" "entry_count=$status_text_map_count threshold=$status_map_min_threshold" "$SCENARIO_PATH"
  mark_fail
fi

python3 - "$SCENARIO_PATH" "$RUNBOOK_PATH" "$SPEC_PATH" "$QA_DOC_PATH" "$SCRIPT_PATH" "$MATRIX_SUMMARY_JSON" "$ACCEPTANCE_PARITY_TSV" <<'PY'
import json
import pathlib
import sys

(
    scenario_path,
    runbook_path,
    spec_path,
    qa_doc_path,
    script_path,
    matrix_summary_path,
    out_tsv_path,
) = sys.argv[1:8]

with open(scenario_path, "r", encoding="utf-8") as fh:
    scenario = json.load(fh)
with open(matrix_summary_path, "r", encoding="utf-8") as fh:
    matrix_summary = json.load(fh)

acceptance_ids = [
    entry.get("id", "")
    for entry in (scenario.get("bl028_contract_checks", {}).get("acceptance_ids") or [])
    if entry.get("id")
]
acceptance_coverage = matrix_summary.get("acceptance_coverage", {})

mapping = {
    "BL028-A1-001": "BL028-A1-001_matrix_legality",
    "BL028-A1-002": "BL028-A1-002_fallback_contract",
    "BL028-A1-003": "BL028-A1-003_diagnostics_schema",
    "BL028-A1-004": "BL028-A1-004_status_text_map",
    "BL028-A1-005": "BL028-A1-005_lane_thresholds",
    "BL028-A1-006": "BL028-A1-006_acceptance_parity",
}

runbook_text = pathlib.Path(runbook_path).read_text(encoding="utf-8")
spec_text = pathlib.Path(spec_path).read_text(encoding="utf-8")
qa_doc_text = pathlib.Path(qa_doc_path).read_text(encoding="utf-8")
scenario_text = pathlib.Path(scenario_path).read_text(encoding="utf-8")
script_text = pathlib.Path(script_path).read_text(encoding="utf-8")

with open(out_tsv_path, "w", encoding="utf-8") as out:
    out.write(
        "acceptance_id\trunbook_count\tspec_count\tqa_doc_count\tscenario_count\tlane_script_count\tmatrix_case_count\tmapped_check\tresult\n"
    )
    for aid in acceptance_ids:
        runbook_count = runbook_text.count(aid)
        spec_count = spec_text.count(aid)
        qa_doc_count = qa_doc_text.count(aid)
        scenario_count = scenario_text.count(aid)
        lane_script_count = script_text.count(aid)
        matrix_case_count = int(acceptance_coverage.get(aid, 0))
        mapped_check = mapping.get(aid, "")

        result = "PASS"
        if not (runbook_count > 0 and spec_count > 0 and qa_doc_count > 0 and scenario_count > 0 and lane_script_count > 0):
            result = "FAIL"
        if mapped_check == "":
            result = "FAIL"

        out.write(
            f"{aid}\t{runbook_count}\t{spec_count}\t{qa_doc_count}\t{scenario_count}\t{lane_script_count}\t{matrix_case_count}\t{mapped_check}\t{result}\n"
        )
PY

if awk -F'\t' 'NR > 1 && $9 != "PASS" { exit 0 } END { exit 1 }' "$ACCEPTANCE_PARITY_TSV"; then
  log_status "BL028-A1-006_acceptance_parity" "FAIL" "parity_rows_with_failures_present" "$ACCEPTANCE_PARITY_TSV"
  mark_fail
else
  log_status "BL028-A1-006_acceptance_parity" "PASS" "all_acceptance_ids_present_and_mapped" "$ACCEPTANCE_PARITY_TSV"
fi

matrix_sha256="missing"
if [[ -f "$MATRIX_REPORT_TSV" ]]; then
  matrix_sha256="$(shasum -a 256 "$MATRIX_REPORT_TSV" | awk '{print $1}')"
fi

RUN_ARTIFACT_DIR="$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID"
baseline_signature=""
replay_divergence_count=0
transient_failure_count=0
run_failure_count=0

for run in $(seq 1 "$RUNS"); do
  run_log="$OUT_DIR/scenario_run_${run}.log"
  run_result_copy="$OUT_DIR/scenario_result_run_${run}.json"

  run_result_status="MISSING"
  run_warnings="NA"
  run_passed="NA"
  run_failed="NA"
  run_total="NA"
  run_result_json=""
  run_result_sha="missing"
  combined_signature="missing"
  baseline_match="NA"
  run_result="PASS"
  failure_class="none"

  rm -rf "$RUN_ARTIFACT_DIR"
  set +e
  "$QA_BIN" --spatial "$SCENARIO_PATH" >"$run_log" 2>&1
  run_qa_exit=$?
  set -e

  if [[ "$run_qa_exit" -ne 0 ]]; then
    run_result="FAIL"
    run_failure_count=$((run_failure_count + 1))
    transient_failure_count=$((transient_failure_count + 1))
    failure_class="$(append_failure_class "$failure_class" "transient_runtime_failure")"
    log_status "scenario_exec_run_${run}" "FAIL" "qa_runner_exit=$run_qa_exit" "$run_log"
    mark_fail
  else
    log_status "scenario_exec_run_${run}" "PASS" "qa_runner_exit=0" "$run_log"

    result_candidates=(
      "$RUN_ARTIFACT_DIR/result.json"
      "$RUN_ARTIFACT_DIR/suite_result.json"
      "$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID/suite_result.json"
      "$ROOT_DIR/qa_output/locusq_spatial/suite_result.json"
      "$ROOT_DIR/qa_output/suite_result.json"
    )

    for candidate in "${result_candidates[@]}"; do
      [[ -f "$candidate" ]] || continue
      suite_id="$(jq -r '.suite_id // empty' "$candidate" 2>/dev/null || true)"
      if [[ -n "$suite_id" && "$suite_id" != "$SCENARIO_ID" ]]; then
        continue
      fi
      run_result_json="$candidate"
      break
    done

    if [[ -z "$run_result_json" ]]; then
      run_result="FAIL"
      run_failure_count=$((run_failure_count + 1))
      transient_failure_count=$((transient_failure_count + 1))
      failure_class="$(append_failure_class "$failure_class" "transient_result_missing")"
      log_status "scenario_status_run_${run}" "FAIL" "missing_result_json_candidates_checked" "$run_log"
      mark_fail
    else
      cp "$run_result_json" "$run_result_copy"
      if [[ "$run" -eq 1 ]]; then
        cp "$run_result_copy" "$RESULT_COPY_JSON"
        cp "$run_log" "$SCENARIO_RUN_LOG"
      fi

      run_result_status="$(jq -r '.status // "UNKNOWN"' "$run_result_copy")"
      run_warnings="$(jq -r '.summary.warned // ((.warnings // []) | length) // 0' "$run_result_copy")"
      run_passed="$(jq -r '.summary.passed // "NA"' "$run_result_copy")"
      run_failed="$(jq -r '.summary.failed // "NA"' "$run_result_copy")"
      run_total="$(jq -r '.summary.total // "NA"' "$run_result_copy")"

      run_result_sha="$(shasum -a 256 "$run_result_copy" | awk '{print $1}')"
      combined_signature="$(printf '%s|%s|%s|%s|%s|%s' "$run_result_status" "$run_warnings" "$run_passed" "$run_failed" "$run_total" "$matrix_sha256" | shasum -a 256 | awk '{print $1}')"

      if [[ "$run_result_status" != "$suite_status_threshold" ]]; then
        run_result="FAIL"
        run_failure_count=$((run_failure_count + 1))
        failure_class="$(append_failure_class "$failure_class" "deterministic_contract_failure")"
        mark_fail
      fi

      if [[ "$run_warnings" =~ ^[0-9]+$ ]] && [[ "$run_warnings" -gt "$warnings_max_threshold" ]]; then
        run_result="FAIL"
        if [[ "$failure_class" != *"deterministic_contract_failure"* ]]; then
          run_failure_count=$((run_failure_count + 1))
        fi
        failure_class="$(append_failure_class "$failure_class" "deterministic_contract_failure")"
        mark_fail
      fi

      if [[ "$run" -eq 1 ]]; then
        baseline_signature="$combined_signature"
        baseline_match="BASELINE"
      else
        if [[ "$combined_signature" == "$baseline_signature" ]]; then
          baseline_match="true"
        else
          baseline_match="false"
          replay_divergence_count=$((replay_divergence_count + 1))
          run_result="FAIL"
          if [[ "$failure_class" != *"deterministic_contract_failure"* ]]; then
            run_failure_count=$((run_failure_count + 1))
          fi
          failure_class="$(append_failure_class "$failure_class" "deterministic_replay_divergence")"
          mark_fail
        fi
      fi

      if [[ "$run_result" == "PASS" ]]; then
        log_status "scenario_status_run_${run}" "PASS" "status=$run_result_status warnings=$run_warnings signature=$combined_signature" "$run_result_copy"
      else
        log_status "scenario_status_run_${run}" "FAIL" "status=$run_result_status warnings=$run_warnings failure_class=$failure_class baseline_match=$baseline_match" "$run_result_copy"
      fi
    fi
  fi

  printf "run=%s qa_exit=%s result_status=%s warnings=%s passed=%s failed=%s total=%s result_json=%s\n" \
    "$run" "$run_qa_exit" "$run_result_status" "$run_warnings" "$run_passed" "$run_failed" "$run_total" "${run_result_json:-missing}" \
    >>"$SCENARIO_RESULT_LOG"
  printf "run=%s result_sha256=%s matrix_sha256=%s combined_signature=%s baseline_match=%s run_result=%s failure_class=%s\n" \
    "$run" "$run_result_sha" "$matrix_sha256" "$combined_signature" "$baseline_match" "$run_result" "$failure_class" \
    >>"$SCENARIO_RESULT_LOG"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run" "$run_qa_exit" "$run_result_status" "$run_warnings" "$run_passed" "$run_failed" "$run_total" "${run_result_json:-missing}" \
    "$run_result_sha" "$matrix_sha256" "$combined_signature" "$baseline_match" "$run_result" "$failure_class" \
    >>"$REPLAY_RUNS_TSV"

  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run" "$run_result_sha" "$matrix_sha256" "$combined_signature" "${baseline_signature:-missing}" "$baseline_match" \
    >>"$REPLAY_HASHES_TSV"
done

if [[ "$replay_divergence_count" -le "$max_signature_divergence" ]]; then
  log_status "replay_signature_consistency" "PASS" "divergence_count=$replay_divergence_count threshold=$max_signature_divergence" "$REPLAY_HASHES_TSV"
else
  log_status "replay_signature_consistency" "FAIL" "divergence_count=$replay_divergence_count threshold=$max_signature_divergence" "$REPLAY_HASHES_TSV"
  mark_fail
fi

if [[ "$transient_failure_count" -le "$max_transient_failures" ]]; then
  log_status "replay_transient_budget" "PASS" "transient_failures=$transient_failure_count threshold=$max_transient_failures" "$REPLAY_RUNS_TSV"
else
  log_status "replay_transient_budget" "FAIL" "transient_failures=$transient_failure_count threshold=$max_transient_failures" "$REPLAY_RUNS_TSV"
  mark_fail
fi

if [[ "$run_failure_count" -eq 0 ]]; then
  log_status "replay_run_outcomes" "PASS" "all_runs_passed runs=$RUNS" "$REPLAY_RUNS_TSV"
else
  log_status "replay_run_outcomes" "FAIL" "failed_runs=$run_failure_count runs=$RUNS" "$REPLAY_RUNS_TSV"
  mark_fail
fi

# Ensure reliability artifact exists before schema verification.
cat >"$RELIABILITY_DECISION_MD" <<EOF_MD
---
Title: BL-028 Slice B2 Reliability Decision
Document Type: Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}
---

# BL-028 Slice B2 Reliability Decision

Decision pending final artifact verification.
EOF_MD

required_artifacts_ok=1
artifact_schema_file="$OUT_DIR/artifact_schema.txt"
jq -r '.bl028_contract_checks.artifact_schema[]?' "$SCENARIO_PATH" >"$artifact_schema_file"
while IFS= read -r artifact_name; do
  [[ -z "$artifact_name" ]] && continue
  if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
    required_artifacts_ok=0
    printf "missing_artifact=%s\n" "$OUT_DIR/$artifact_name" >>"$SCENARIO_RESULT_LOG"
  fi
done <"$artifact_schema_file"

if python3 - "$matrix_accuracy" "$matrix_accuracy_threshold" "$fallback_accuracy" "$fallback_accuracy_threshold" "$required_artifacts_ok" "$replay_divergence_count" "$max_signature_divergence" "$transient_failure_count" "$max_transient_failures" "$run_failure_count" <<'PY'
import sys
matrix_actual = float(sys.argv[1])
matrix_threshold = float(sys.argv[2])
fallback_actual = float(sys.argv[3])
fallback_threshold = float(sys.argv[4])
artifacts_ok = int(sys.argv[5]) == 1
replay_div = int(sys.argv[6])
replay_div_threshold = int(sys.argv[7])
transient_failures = int(sys.argv[8])
transient_threshold = int(sys.argv[9])
run_failures = int(sys.argv[10])
ok = (
    matrix_actual >= matrix_threshold
    and fallback_actual >= fallback_threshold
    and artifacts_ok
    and replay_div <= replay_div_threshold
    and transient_failures <= transient_threshold
    and run_failures == 0
)
sys.exit(0 if ok else 1)
PY
then
  log_status "BL028-A1-005_lane_thresholds" "PASS" "matrix_accuracy=$matrix_accuracy fallback_accuracy=$fallback_accuracy artifacts_ok=$required_artifacts_ok replay_divergence=$replay_divergence_count transient_failures=$transient_failure_count" "$STATUS_TSV"
else
  log_status "BL028-A1-005_lane_thresholds" "FAIL" "matrix_accuracy=$matrix_accuracy fallback_accuracy=$fallback_accuracy artifacts_ok=$required_artifacts_ok replay_divergence=$replay_divergence_count transient_failures=$transient_failure_count" "$STATUS_TSV"
  mark_fail
fi

final_result="PASS"
if [[ "$mark_fail_count" -ne 0 ]]; then
  final_result="FAIL"
fi

cat >"$RELIABILITY_DECISION_MD" <<EOF_MD
---
Title: BL-028 Slice B2 Reliability Decision
Document Type: Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}
---

# BL-028 Slice B2 Reliability Decision

- Result: ${final_result}
- Runs requested: ${RUNS}
- Replay divergence count: ${replay_divergence_count} (threshold: ${max_signature_divergence})
- Transient failure count: ${transient_failure_count} (threshold: ${max_transient_failures})
- Run failure count: ${run_failure_count}
- Matrix accuracy: ${matrix_accuracy} (threshold: ${matrix_accuracy_threshold})
- Fallback accuracy: ${fallback_accuracy} (threshold: ${fallback_accuracy_threshold})
- Required artifacts present: ${required_artifacts_ok}

## Failure Taxonomy Counts

| Taxonomy | Count |
|---|---:|
| deterministic_replay_divergence | ${replay_divergence_count} |
| deterministic_contract_failure | $(awk -F'\t' 'NR > 1 && $14 ~ /deterministic_contract_failure/ { c++ } END { print c + 0 }' "$REPLAY_RUNS_TSV") |
| transient_runtime_failure | $(awk -F'\t' 'NR > 1 && $14 ~ /transient_runtime_failure/ { c++ } END { print c + 0 }' "$REPLAY_RUNS_TSV") |
| transient_result_missing | $(awk -F'\t' 'NR > 1 && $14 ~ /transient_result_missing/ { c++ } END { print c + 0 }' "$REPLAY_RUNS_TSV") |

## Decision Rule

PASS only when all replay runs satisfy suite thresholds and replay signature divergence remains within contract.
EOF_MD

if [[ "$mark_fail_count" -eq 0 ]]; then
  log_status "lane_final" "PASS" "all_bl028_lane_checks_passed" "$STATUS_TSV"
else
  log_status "lane_final" "FAIL" "failure_count=$mark_fail_count" "$STATUS_TSV"
fi

printf "artifact_dir=%s\n" "$OUT_DIR" >&3
if [[ "$mark_fail_count" -eq 0 ]]; then
  exit 0
fi
exit 1
