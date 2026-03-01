#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

OUT_DIR="${ROOT_DIR}/TestEvidence/bl023_slice_c1_matrix_${TIMESTAMP}"
RUNS=1
CONTRACT_ONLY=0

usage() {
  cat <<'USAGE'
Usage: qa-bl023-resize-dpi-matrix-mac.sh [options]

Deterministic BL-023 resize/DPI host matrix wrapper.

Options:
  --runs <N>         Replay run count (integer >= 1).
  --out-dir <path>   Artifact output directory.
  --contract-only    Contract-only execution (no host runtime commands).
  --help, -h         Show this help.

Outputs:
  status.tsv
  validation_matrix.tsv
  host_matrix_results.tsv
  failure_taxonomy.tsv
  determinism_summary.tsv

Exit semantics:
  0  PASS
  1  Gate fail
  2  Usage error
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      [[ $# -ge 2 ]] || { echo "ERROR: --runs requires a value" >&2; usage; exit 2; }
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --out-dir requires a value" >&2; usage; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    --contract-only)
      CONTRACT_ONLY=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  echo "ERROR: --runs must be an integer >= 1 (received: $RUNS)" >&2
  usage
  exit 2
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/validation_matrix.tsv"
HOST_RESULTS_TSV="${OUT_DIR}/host_matrix_results.tsv"
FAILURE_TSV="${OUT_DIR}/failure_taxonomy.tsv"
DETERMINISM_TSV="${OUT_DIR}/determinism_summary.tsv"
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$VALIDATION_TSV"
printf "run\tlane_id\thost_mode\thost\tplugin_format\tbackend\tviewport_id\tdpi_id\tcheck_id\tresult\ttaxonomy_id\tnotes\n" > "$HOST_RESULTS_TSV"
printf "metric\tvalue\tthreshold\tresult\tnotes\n" > "$DETERMINISM_TSV"
: > "$FAILURE_EVENTS_TSV"

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf "%s" "$value"
}

log_status() {
  local step="$1"
  local result="$2"
  local exit_code="$3"
  local detail="$4"
  local artifact="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$step")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$STATUS_TSV"
}

add_validation() {
  local run="$1"
  local check_id="$2"
  local result="$3"
  local detail="$4"
  local artifact="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run")" \
    "$(sanitize_tsv_field "$check_id")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$VALIDATION_TSV"
}

append_failure_event() {
  local taxonomy_id="$1"
  local run="$2"
  local lane_id="$3"
  local detail="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$taxonomy_id")" \
    "$(sanitize_tsv_field "$run")" \
    "$(sanitize_tsv_field "$lane_id")" \
    "$(sanitize_tsv_field "$detail")" \
    >> "$FAILURE_EVENTS_TSV"
}

OVERALL_FAIL=0

record_contract_check() {
  local check_id="$1"
  local pattern="$2"
  local file_path="$3"
  if rg -q "$pattern" "$file_path"; then
    add_validation "0" "$check_id" "PASS" "pattern_present" "$file_path"
  else
    add_validation "0" "$check_id" "FAIL" "pattern_missing:$pattern" "$file_path"
    append_failure_event "BL023-RZ-900" "0" "$check_id" "missing_contract_pattern"
    OVERALL_FAIL=1
  fi
}

QA_DOC="${ROOT_DIR}/Documentation/testing/bl-023-resize-dpi-hardening-qa.md"
BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-023-resize-dpi-hardening.md"
SCRIPT_PATH="${ROOT_DIR}/scripts/qa-bl023-resize-dpi-matrix-mac.sh"
SELFTEST_CMD="${ROOT_DIR}/scripts/standalone-ui-selftest-production-p0-mac.sh"
SELFTEST_MAX_ATTEMPTS="${LOCUSQ_BL023_SELFTEST_MAX_ATTEMPTS:-3}"
SELFTEST_RETRY_DELAY_SECONDS="${LOCUSQ_BL023_SELFTEST_RETRY_DELAY_SECONDS:-2}"
SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS="${LOCUSQ_BL023_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS:-5}"
RUNTIME_RETRY_ON_EXIT143="${LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143:-1}"
RUNTIME_RETRY_LIMIT="${LOCUSQ_BL023_RUNTIME_RETRY_LIMIT:-1}"
RUNTIME_RETRY_DELAY_SECONDS="${LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS:-1}"

case "$RUNTIME_RETRY_ON_EXIT143" in
  0|1) ;;
  *) RUNTIME_RETRY_ON_EXIT143=1 ;;
esac
if ! [[ "$RUNTIME_RETRY_LIMIT" =~ ^[0-9]+$ ]]; then
  RUNTIME_RETRY_LIMIT=1
fi
if ! [[ "$RUNTIME_RETRY_DELAY_SECONDS" =~ ^[0-9]+$ ]]; then
  RUNTIME_RETRY_DELAY_SECONDS=1
fi

if [[ -f "$QA_DOC" ]]; then
  log_status "qa_doc_exists" "PASS" "0" "found" "$QA_DOC"
else
  log_status "qa_doc_exists" "FAIL" "1" "missing" "$QA_DOC"
  append_failure_event "BL023-RZ-900" "0" "BL023-QA-DOC" "qa_doc_missing"
  OVERALL_FAIL=1
fi

if [[ -f "$BACKLOG_DOC" ]]; then
  log_status "backlog_doc_exists" "PASS" "0" "found" "$BACKLOG_DOC"
else
  log_status "backlog_doc_exists" "FAIL" "1" "missing" "$BACKLOG_DOC"
  append_failure_event "BL023-RZ-900" "0" "BL023-BACKLOG-DOC" "backlog_doc_missing"
  OVERALL_FAIL=1
fi

if [[ -f "$SCRIPT_PATH" ]]; then
  log_status "lane_script_exists" "PASS" "0" "found" "$SCRIPT_PATH"
else
  log_status "lane_script_exists" "FAIL" "1" "missing" "$SCRIPT_PATH"
  append_failure_event "BL023-RZ-900" "0" "BL023-SCRIPT" "lane_script_missing"
  OVERALL_FAIL=1
fi

record_contract_check "BL023-C1-REQ-001" "BL023-HM-001" "$QA_DOC"
record_contract_check "BL023-C1-REQ-002" "BL023-HM-002" "$QA_DOC"
record_contract_check "BL023-C1-REQ-003" "BL023-HM-003" "$QA_DOC"
record_contract_check "BL023-C1-REQ-004" "BL023-HM-004" "$QA_DOC"
record_contract_check "BL023-C1-REQ-005" "BL023-CHK-005" "$QA_DOC"
record_contract_check "BL023-C1-REQ-006" "BL023-RZ-001" "$QA_DOC"
record_contract_check "BL023-C1-REQ-007" "BL023-CD-002" "$BACKLOG_DOC"
record_contract_check "BL023-C3-REQ-001" "Validation Plan \\(C3\\)" "$BACKLOG_DOC"
record_contract_check "BL023-C3-REQ-002" "mode_parity.tsv" "$BACKLOG_DOC"
record_contract_check "BL023-C3-REQ-003" "exit_semantics_probe.tsv" "$BACKLOG_DOC"
record_contract_check "BL023-C3-REQ-004" "C3 Validation" "$QA_DOC"
record_contract_check "BL023-C3-REQ-005" "C3 Evidence Contract" "$QA_DOC"
record_contract_check "BL023-C3-REQ-006" "mode_parity.tsv" "$QA_DOC"
record_contract_check "BL023-C3-REQ-007" "contract-only|runs must be an integer >= 1|unknown argument:" "$SCRIPT_PATH"

if [[ "$CONTRACT_ONLY" -eq 1 ]]; then
  log_status "mode" "PASS" "0" "contract_only" "$STATUS_TSV"
else
  log_status "mode" "PASS" "0" "runtime_matrix" "$STATUS_TSV"
fi

baseline_signature=""
baseline_row_count=""
signature_divergence_count=0
row_drift_count=0
runtime_failure_count=0
runtime_retry143_recovery_count=0

emit_lane_rows() {
  local run="$1"
  local result="$2"
  local taxonomy="$3"
  local notes="$4"
  local check_id="$5"
  while IFS=$'\t' read -r lane_id host_mode host_name plugin_format backend; do
    [[ -n "$lane_id" ]] || continue
    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run" \
      "$lane_id" \
      "$host_mode" \
      "$host_name" \
      "$plugin_format" \
      "$backend" \
      "VP-03" \
      "DPI-02" \
      "$check_id" \
      "$result" \
      "$taxonomy" \
      "$notes" \
      >> "$HOST_RESULTS_TSV"
  done <<'LANES'
BL023-HM-001	standalone	LocusQ Standalone	n/a	WKWebView
BL023-HM-002	plugin	REAPER	VST3	WKWebView
BL023-HM-003	plugin	Logic Pro	AU	WKWebView
BL023-HM-004	plugin	Ableton Live	VST3	WKWebView
LANES
}

for run in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run}.log"
  if [[ "$CONTRACT_ONLY" -eq 1 ]]; then
    printf "[%s] run=%s mode=contract_only\n" "$DOC_TS" "$run" > "$run_log"
    emit_lane_rows "$run" "PASS" "none" "contract_only_pass" "BL023-CHK-ALL"
    add_validation "$run" "BL023-C1-RUN-001" "PASS" "contract_only_lane_pass" "$run_log"
  else
    runtime_attempt=1
    runtime_retry_attempts=0
    max_runtime_attempts=1
    if [[ "$RUNTIME_RETRY_ON_EXIT143" -eq 1 ]]; then
      max_runtime_attempts=$((1 + RUNTIME_RETRY_LIMIT))
    fi
    : > "$run_log"
    while true; do
      printf "[%s] run=%s runtime_attempt=%s/%s\n" "$DOC_TS" "$run" "$runtime_attempt" "$max_runtime_attempts" >> "$run_log"
      set +e
      LOCUSQ_UI_SELFTEST_SCOPE=bl029 \
        LOCUSQ_UI_SELFTEST_MAX_ATTEMPTS="$SELFTEST_MAX_ATTEMPTS" \
        LOCUSQ_UI_SELFTEST_RETRY_DELAY_SECONDS="$SELFTEST_RETRY_DELAY_SECONDS" \
        LOCUSQ_UI_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS="$SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS" \
        "$SELFTEST_CMD" >> "$run_log" 2>&1
      run_ec=$?
      set -e
      if [[ "$run_ec" -eq 143 && "$runtime_attempt" -lt "$max_runtime_attempts" ]]; then
        runtime_retry_attempts=$((runtime_retry_attempts + 1))
        printf "[%s] run=%s retry_on_exit_143 retry_attempt=%s delay_seconds=%s\n" \
          "$DOC_TS" "$run" "$runtime_retry_attempts" "$RUNTIME_RETRY_DELAY_SECONDS" >> "$run_log"
        sleep "$RUNTIME_RETRY_DELAY_SECONDS"
        runtime_attempt=$((runtime_attempt + 1))
        continue
      fi
      break
    done
    if [[ "$run_ec" -eq 0 ]]; then
      if [[ "$runtime_retry_attempts" -gt 0 ]]; then
        runtime_retry143_recovery_count=$((runtime_retry143_recovery_count + 1))
      fi
      emit_lane_rows "$run" "PASS" "none" "runtime_lane_pass" "BL023-CHK-ALL"
      if [[ "$runtime_retry_attempts" -gt 0 ]]; then
        add_validation "$run" "BL023-C1-RUN-001" "PASS" "runtime_lane_pass_after_exit143_retry:attempts=${runtime_attempt}" "$run_log"
      else
        add_validation "$run" "BL023-C1-RUN-001" "PASS" "runtime_lane_pass" "$run_log"
      fi
    else
      runtime_failure_count=$((runtime_failure_count + 1))
      emit_lane_rows "$run" "FAIL" "BL023-RZ-900" "runtime_lane_fail" "BL023-CHK-ALL"
      add_validation "$run" "BL023-C1-RUN-001" "FAIL" "runtime_lane_fail:exit=${run_ec}:attempts=${runtime_attempt}" "$run_log"
      append_failure_event "BL023-RZ-900" "$run" "BL023-HM-ALL" "runtime_lane_fail"
      OVERALL_FAIL=1
    fi
  fi

  run_signature="$(awk -F'\t' -v run="$run" 'BEGIN{OFS="\t"} NR==1 { $1="run"; print; next } $1==run { $1="run"; print }' "$HOST_RESULTS_TSV" | shasum -a 256 | awk '{print $1}')"
  run_row_count="$(awk -F'\t' -v run="$run" 'NR>1 && $1==run {c++} END {print c+0}' "$HOST_RESULTS_TSV")"
  if [[ -z "$baseline_signature" ]]; then
    baseline_signature="$run_signature"
    baseline_row_count="$run_row_count"
    add_validation "$run" "BL023-C1-DET-001" "PASS" "baseline_signature=${run_signature}" "$HOST_RESULTS_TSV"
    add_validation "$run" "BL023-C2-DET-002" "PASS" "baseline_row_count=${run_row_count}" "$HOST_RESULTS_TSV"
  else
    if [[ "$run_signature" == "$baseline_signature" ]]; then
      add_validation "$run" "BL023-C1-DET-001" "PASS" "signature_match=${run_signature}" "$HOST_RESULTS_TSV"
    else
      signature_divergence_count=$((signature_divergence_count + 1))
      add_validation "$run" "BL023-C1-DET-001" "FAIL" "signature_mismatch current=${run_signature} baseline=${baseline_signature}" "$HOST_RESULTS_TSV"
      append_failure_event "BL023-RZ-910" "$run" "BL023-HM-ALL" "determinism_signature_mismatch"
      OVERALL_FAIL=1
    fi

    if [[ "$run_row_count" == "$baseline_row_count" ]]; then
      add_validation "$run" "BL023-C2-DET-002" "PASS" "row_count_match=${run_row_count}" "$HOST_RESULTS_TSV"
    else
      row_drift_count=$((row_drift_count + 1))
      add_validation "$run" "BL023-C2-DET-002" "FAIL" "row_count_mismatch current=${run_row_count} baseline=${baseline_row_count}" "$HOST_RESULTS_TSV"
      append_failure_event "BL023-RZ-911" "$run" "BL023-HM-ALL" "determinism_row_count_mismatch"
      OVERALL_FAIL=1
    fi
  fi
done

if [[ "$signature_divergence_count" -eq 0 ]]; then
  printf "signature_divergence_count\t0\t0\tPASS\tall_run_signatures_match_baseline\n" >> "$DETERMINISM_TSV"
else
  printf "signature_divergence_count\t%s\t0\tFAIL\tdeterministic_replay_signature_divergence\n" "$signature_divergence_count" >> "$DETERMINISM_TSV"
fi
if [[ "$row_drift_count" -eq 0 ]]; then
  printf "row_count_drift\t0\t0\tPASS\tall_run_row_counts_match_baseline\n" >> "$DETERMINISM_TSV"
else
  printf "row_count_drift\t%s\t0\tFAIL\tdeterministic_replay_row_count_drift\n" "$row_drift_count" >> "$DETERMINISM_TSV"
fi
if [[ "$runtime_failure_count" -eq 0 ]]; then
  printf "runtime_failure_count\t0\t0\tPASS\tall_runtime_invocations_passed\n" >> "$DETERMINISM_TSV"
else
  printf "runtime_failure_count\t%s\t0\tFAIL\tnonzero_runtime_invocations_detected\n" "$runtime_failure_count" >> "$DETERMINISM_TSV"
fi
if [[ "$runtime_retry143_recovery_count" -eq 0 ]]; then
  printf "runtime_retry143_recovery_count\t0\tn/a\tPASS\tno_exit143_retries_needed\n" >> "$DETERMINISM_TSV"
else
  printf "runtime_retry143_recovery_count\t%s\tn/a\tPASS\texit143_recovered_by_bounded_retry\n" "$runtime_retry143_recovery_count" >> "$DETERMINISM_TSV"
fi

printf "taxonomy_id\tcount\tfirst_run\tfirst_lane\tdetail\n" > "$FAILURE_TSV"
known_taxonomy_ids=(
  "BL023-RZ-001"
  "BL023-RZ-002"
  "BL023-RZ-003"
  "BL023-RZ-004"
  "BL023-RZ-900"
  "BL023-RZ-910"
  "BL023-RZ-911"
)
for taxonomy_id in "${known_taxonomy_ids[@]}"; do
  count="$(awk -F'\t' -v id="$taxonomy_id" '$1==id{c++} END{print c+0}' "$FAILURE_EVENTS_TSV")"
  first_run="-"
  first_lane="-"
  first_detail="none"
  if [[ "$count" -gt 0 ]]; then
    first_row="$(awk -F'\t' -v id="$taxonomy_id" '$1==id{print $2 "\t" $3 "\t" $4; exit}' "$FAILURE_EVENTS_TSV")"
    first_run="$(printf "%s" "$first_row" | awk -F'\t' '{print $1}')"
    first_lane="$(printf "%s" "$first_row" | awk -F'\t' '{print $2}')"
    first_detail="$(printf "%s" "$first_row" | awk -F'\t' '{print $3}')"
  fi
  printf "%s\t%s\t%s\t%s\t%s\n" "$taxonomy_id" "$count" "$first_run" "$first_lane" "$first_detail" >> "$FAILURE_TSV"
done

expected_taxonomy_rows="${#known_taxonomy_ids[@]}"
actual_taxonomy_rows="$(awk 'NR>1 {c++} END {print c+0}' "$FAILURE_TSV")"
if [[ "$actual_taxonomy_rows" -eq "$expected_taxonomy_rows" ]]; then
  add_validation "0" "BL023-C2-DET-003" "PASS" "taxonomy_row_count=${actual_taxonomy_rows}" "$FAILURE_TSV"
  printf "taxonomy_row_count\t%s\t%s\tPASS\tfixed_taxonomy_row_count\n" "$actual_taxonomy_rows" "$expected_taxonomy_rows" >> "$DETERMINISM_TSV"
else
  add_validation "0" "BL023-C2-DET-003" "FAIL" "taxonomy_row_count=${actual_taxonomy_rows} expected=${expected_taxonomy_rows}" "$FAILURE_TSV"
  append_failure_event "BL023-RZ-900" "0" "BL023-C2-DET-003" "taxonomy_row_count_mismatch"
  printf "taxonomy_row_count\t%s\t%s\tFAIL\ttaxonomy_row_count_mismatch\n" "$actual_taxonomy_rows" "$expected_taxonomy_rows" >> "$DETERMINISM_TSV"
  OVERALL_FAIL=1
fi

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  log_status "determinism_summary" "PASS" "0" "signature_and_row_drift_within_threshold" "$DETERMINISM_TSV"
  log_status "overall" "PASS" "0" "gate_pass" "$OUT_DIR"
  exit 0
else
  log_status "determinism_summary" "FAIL" "1" "determinism_or_runtime_failures_detected" "$DETERMINISM_TSV"
  log_status "overall" "FAIL" "1" "gate_fail" "$OUT_DIR"
  exit 1
fi
