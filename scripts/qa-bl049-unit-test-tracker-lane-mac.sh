#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUNS=1
CONTRACT_ONLY=0
EXECUTE_SUITE=0
OUT_DIR="${ROOT_DIR}/TestEvidence/bl049_unit_test_tracker_lane_${TIMESTAMP}"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-049-unit-test-framework-and-tracker-automation.md"
QA_DOC="${ROOT_DIR}/Documentation/testing/bl-049-unit-test-framework-and-tracker-automation-qa.md"
SCRIPT_PATH="${ROOT_DIR}/scripts/qa-bl049-unit-test-tracker-lane-mac.sh"

usage() {
  cat <<'USAGE'
Usage: qa-bl049-unit-test-tracker-lane-mac.sh [options]

BL-049 unit-test framework and tracker automation deterministic contract lane (B1 bootstrap).

Options:
  --runs <N>         Replay run count (integer >= 1)
  --out-dir <path>   Artifact output directory
  --contract-only    Run deterministic contract checks only (default behavior)
  --execute-suite    Reserved for future runtime execution; currently runs contract checks
  --help, -h         Show usage

Outputs:
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv

Exit semantics:
  0  pass
  1  gate fail
  2  usage/config error
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      [[ $# -ge 2 ]] || { echo "ERROR: --runs requires a value" >&2; usage >&2; exit 2; }
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --out-dir requires a value" >&2; usage >&2; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    --contract-only)
      CONTRACT_ONLY=1
      EXECUTE_SUITE=0
      shift
      ;;
    --execute-suite)
      EXECUTE_SUITE=1
      CONTRACT_ONLY=0
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  echo "ERROR: --runs must be an integer >= 1 (received: $RUNS)" >&2
  usage >&2
  exit 2
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/validation_matrix.tsv"
REPLAY_TSV="${OUT_DIR}/replay_hashes.tsv"
FAILURE_TSV="${OUT_DIR}/failure_taxonomy.tsv"
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$VALIDATION_TSV"
printf "run\tsignature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" > "$REPLAY_TSV"
printf "failure_id\tcount\tclassification\tdetail\n" > "$FAILURE_TSV"
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
  local failure_id="$1"
  local run="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$failure_id")" \
    "$(sanitize_tsv_field "$run")" \
    "$(sanitize_tsv_field "$detail")" \
    >> "$FAILURE_EVENTS_TSV"
}

hash_text() {
  local text="$1"
  printf "%s" "$text" | shasum -a 256 | awk '{print $1}'
}

OVERALL_FAIL=0

for req in "$BACKLOG_DOC" "$QA_DOC" "$SCRIPT_PATH"; do
  if [[ -f "$req" ]]; then
    log_status "file_exists" "PASS" "0" "found" "$req"
  else
    log_status "file_exists" "FAIL" "1" "missing" "$req"
    append_failure_event "BL049-B1-901" "0" "missing_required_file:$(basename "$req")"
    OVERALL_FAIL=1
  fi
done

if [[ "$CONTRACT_ONLY" -eq 1 || "$EXECUTE_SUITE" -eq 0 ]]; then
  log_status "mode" "PASS" "0" "contract_only" "$STATUS_TSV"
else
  log_status "mode" "PASS" "0" "execute_suite_requested_runtime_reserved_running_contract_checks_only" "$STATUS_TSV"
fi

run_check_pattern() {
  local run="$1"
  local check_id="$2"
  local pattern="$3"
  local file_path="$4"
  local run_fail_count="$5"
  if rg -q -- "$pattern" "$file_path"; then
    add_validation "$run" "$check_id" "PASS" "pattern_present" "$file_path"
  else
    add_validation "$run" "$check_id" "FAIL" "pattern_missing:${pattern}" "$file_path"
    append_failure_event "BL049-B1-901" "$run" "${check_id}:pattern_missing"
    run_fail_count=$((run_fail_count + 1))
    OVERALL_FAIL=1
  fi
  printf "%s" "$run_fail_count"
}

baseline_signature=""
baseline_row_signature=""
signature_drift_count=0
row_drift_count=0

for run in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run}.log"
  : > "$run_log"

  run_fail_count=0

  run_fail_count="$(run_check_pattern "$run" "BL049-B1-001" 'BL049-B1-001|Validation Plan \(B1\)|Evidence Contract \(B1\)|qa-bl049-unit-test-tracker-lane-mac.sh' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL049-B1-002" 'BL049-B1-001|B1 Validation|B1 Evidence Contract|qa-bl049-unit-test-tracker-lane-mac.sh' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL049-B1-003" 'status.tsv|validation_matrix.tsv|contract_runs/replay_hashes.tsv|contract_runs/failure_taxonomy.tsv|lane_notes.md|docs_freshness.log' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL049-B1-004" 'status.tsv|validation_matrix.tsv|contract_runs/replay_hashes.tsv|contract_runs/failure_taxonomy.tsv|lane_notes.md|docs_freshness.log' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL049-B1-005" 'BL049-FX-101|BL049-FX-102|BL049-FX-103|BL049-FX-104' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL049-B1-006" 'BL049-FX-101|BL049-FX-102|BL049-FX-103|BL049-FX-104' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL049-B1-007" 'bl049_slice_a1_contract_20260227T204255Z|Input handoffs resolved|Slice B1 Execution Snapshot' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL049-B1-008" '--contract-only|--execute-suite|Exit semantics|0  pass|1  gate fail|2  usage/config error' "$SCRIPT_PATH" "$run_fail_count")"

  backlog_sig="$(rg -n 'BL049-B1-|Validation Plan \(B1\)|Evidence Contract \(B1\)|Slice B1 Execution Snapshot|bl049_slice_a1_contract_20260227T204255Z|qa-bl049-unit-test-tracker-lane-mac.sh|contract_runs/replay_hashes.tsv|contract_runs/failure_taxonomy.tsv|lane_notes.md' "$BACKLOG_DOC" | shasum -a 256 | awk '{print $1}')"
  qa_sig="$(rg -n 'BL049-B1-|B1 Validation|B1 Evidence Contract|B1 Execution Snapshot|qa-bl049-unit-test-tracker-lane-mac.sh|contract_runs/replay_hashes.tsv|contract_runs/failure_taxonomy.tsv|lane_notes.md' "$QA_DOC" | shasum -a 256 | awk '{print $1}')"
  script_sig="$(rg -n -- '--contract-only|--execute-suite|Exit semantics|--runs must be an integer >= 1|0  pass|1  gate fail|2  usage/config error|replay_hashes.tsv|failure_taxonomy.tsv' "$SCRIPT_PATH" | shasum -a 256 | awk '{print $1}')"

  signature="$(hash_text "${backlog_sig}|${qa_sig}|${script_sig}")"
  row_signature="$(hash_text "${run_fail_count}")"

  signature_match=1
  row_match=1
  if [[ -z "$baseline_signature" ]]; then
    baseline_signature="$signature"
    baseline_row_signature="$row_signature"
  else
    if [[ "$signature" != "$baseline_signature" ]]; then
      signature_match=0
      signature_drift_count=$((signature_drift_count + 1))
      append_failure_event "BL049-B1-902" "$run" "signature_divergence"
      OVERALL_FAIL=1
    fi
    if [[ "$row_signature" != "$baseline_row_signature" ]]; then
      row_match=0
      row_drift_count=$((row_drift_count + 1))
      append_failure_event "BL049-B1-903" "$run" "row_divergence"
      OVERALL_FAIL=1
    fi
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run" \
    "$signature" \
    "$baseline_signature" \
    "$signature_match" \
    "$row_signature" \
    "$baseline_row_signature" \
    "$row_match" \
    >> "$REPLAY_TSV"

  {
    printf "[run=%s] signature=%s baseline=%s signature_match=%s\n" "$run" "$signature" "$baseline_signature" "$signature_match"
    printf "[run=%s] row_signature=%s baseline_row=%s row_match=%s run_fail_count=%s\n" "$run" "$row_signature" "$baseline_row_signature" "$row_match" "$run_fail_count"
  } >> "$run_log"
done

if [[ -s "$FAILURE_EVENTS_TSV" ]]; then
  awk -F'\t' '
    {
      count[$1]++
    }
    END {
      for (id in count) {
        classification = "deterministic_contract_failure"
        detail = "contract_or_replay_failure"
        if (id == "BL049-B1-901") {
          detail = "missing_required_pattern_or_schema"
        } else if (id == "BL049-B1-902") {
          classification = "deterministic_replay_failure"
          detail = "replay_signature_divergence"
        } else if (id == "BL049-B1-903") {
          classification = "deterministic_replay_failure"
          detail = "replay_row_divergence"
        }
        printf "%s\t%d\t%s\t%s\n", id, count[id], classification, detail
      }
    }
  ' "$FAILURE_EVENTS_TSV" | sort >> "$FAILURE_TSV"
else
  printf "none\t0\tnone\tno_failures\n" >> "$FAILURE_TSV"
fi

if [[ "$signature_drift_count" -eq 0 ]]; then
  log_status "replay_signature" "PASS" "0" "stable" "$REPLAY_TSV"
else
  log_status "replay_signature" "FAIL" "1" "drift_count=${signature_drift_count}" "$REPLAY_TSV"
fi

if [[ "$row_drift_count" -eq 0 ]]; then
  log_status "replay_rows" "PASS" "0" "stable" "$REPLAY_TSV"
else
  log_status "replay_rows" "FAIL" "1" "drift_count=${row_drift_count}" "$REPLAY_TSV"
fi

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  log_status "lane_result" "PASS" "0" "all_contract_checks_passed" "$STATUS_TSV"
  rm -f "$FAILURE_EVENTS_TSV"
  exit 0
fi

log_status "lane_result" "FAIL" "1" "contract_gate_failed" "$STATUS_TSV"
rm -f "$FAILURE_EVENTS_TSV"
exit 1
