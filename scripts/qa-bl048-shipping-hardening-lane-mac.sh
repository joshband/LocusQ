#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUNS=1
MODE="contract_only"
MODE_SET=0
OUT_DIR="${ROOT_DIR}/TestEvidence/bl048_shipping_hardening_${TIMESTAMP}"

usage() {
  cat <<'USAGE'
Usage: qa-bl048-shipping-hardening-lane-mac.sh [options]

BL-048 shipping-hardening deterministic contract lane.

Options:
  --runs <N>         Replay run count (integer >= 1)
  --out-dir <path>   Artifact output directory
  --contract-only    Contract replay mode
  --execute-suite    Execute-suite parity mode (same deterministic contract checks)
  --help, -h         Show usage

Outputs:
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv
  drift_summary.tsv

Exit semantics:
  0  pass
  1  lane or contract failure
  2  usage or configuration error
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      [[ $# -ge 2 ]] || usage_error "--runs requires a value"
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires a value"
      OUT_DIR="$2"
      shift 2
      ;;
    --contract-only)
      if [[ "$MODE_SET" -eq 1 && "$MODE" != "contract_only" ]]; then
        usage_error "--contract-only conflicts with --execute-suite"
      fi
      MODE="contract_only"
      MODE_SET=1
      shift
      ;;
    --execute-suite)
      if [[ "$MODE_SET" -eq 1 && "$MODE" != "execute_suite" ]]; then
        usage_error "--execute-suite conflicts with --contract-only"
      fi
      MODE="execute_suite"
      MODE_SET=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      usage_error "unknown argument: $1"
      ;;
  esac
done

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  usage_error "--runs must be an integer >= 1 (received: $RUNS)"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/validation_matrix.tsv"
REPLAY_TSV="${OUT_DIR}/replay_hashes.tsv"
FAILURE_TSV="${OUT_DIR}/failure_taxonomy.tsv"
DRIFT_SUMMARY_TSV="${OUT_DIR}/drift_summary.tsv"
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$VALIDATION_TSV"
printf "run\tsignature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" > "$REPLAY_TSV"
printf "failure_id\tcount\tclassification\tdetail\n" > "$FAILURE_TSV"
printf "metric\tvalue\tthreshold\tresult\tdetail\tartifact\n" > "$DRIFT_SUMMARY_TSV"
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

ROOT_BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-048-cross-platform-shipping-hardening.md"
ROOT_QA_DOC="${ROOT_DIR}/Documentation/testing/bl-048-cross-platform-shipping-hardening-qa.md"

OVERALL_FAIL=0

for tool in rg shasum; do
  if command -v "$tool" >/dev/null 2>&1; then
    log_status "tool_${tool}" "PASS" "0" "$(command -v "$tool")" ""
  else
    log_status "tool_${tool}" "FAIL" "1" "missing_required_tool" ""
    OVERALL_FAIL=1
  fi
done

for req in "$ROOT_BACKLOG_DOC" "$ROOT_QA_DOC"; do
  if [[ -f "$req" ]]; then
    log_status "file_exists" "PASS" "0" "found" "$req"
  else
    log_status "file_exists" "FAIL" "1" "missing" "$req"
    append_failure_event "BL048-B1-FX-001" "0" "missing_required_file:$(basename "$req")"
    OVERALL_FAIL=1
  fi
done

log_status "mode" "PASS" "0" "$MODE" "$STATUS_TSV"

baseline_signature=""
baseline_row_signature=""
signature_drift_count=0
row_drift_count=0

for run in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run}.log"
  : > "$run_log"
  run_fail_count=0

  run_check_all_patterns() {
    local check_id="$1"
    local file_path="$2"
    shift 2
    local missing=()
    local pattern
    for pattern in "$@"; do
      if ! rg -q "$pattern" "$file_path"; then
        missing+=("$pattern")
      fi
    done

    if [[ "${#missing[@]}" -eq 0 ]]; then
      add_validation "$run" "$check_id" "PASS" "all_patterns_present" "$file_path"
      printf "[%s] %s PASS (all_patterns_present)\n" "$run" "$check_id" >> "$run_log"
    else
      add_validation "$run" "$check_id" "FAIL" "missing_patterns:${missing[*]}" "$file_path"
      append_failure_event "BL048-B1-FX-001" "$run" "$check_id:missing_patterns"
      run_fail_count=$((run_fail_count + 1))
      OVERALL_FAIL=1
      printf "[%s] %s FAIL (missing_patterns:%s)\n" "$run" "$check_id" "${missing[*]}" >> "$run_log"
    fi
  }

  run_check_all_patterns "BL048-B1-001" "$ROOT_BACKLOG_DOC" \
    "shipping_platform_matrix" \
    "platform_id" \
    "architectures" \
    "plugin_formats" \
    "verification_lanes"

  run_check_all_patterns "BL048-B1-002" "$ROOT_BACKLOG_DOC" \
    "BL048-A1-003" \
    "BL048-A1-004" \
    "BL048-A1-005" \
    "BL048-A1-006"

  run_check_all_patterns "BL048-B1-003" "$ROOT_BACKLOG_DOC" \
    "release_gate_matrix.tsv" \
    "packaging_manifest.tsv" \
    "checksums.tsv"

  run_check_all_patterns "BL048-B1-004" "$ROOT_QA_DOC" \
    "BL048-A1-001" \
    "BL048-A1-008" \
    "acceptance_matrix.tsv"

  run_check_all_patterns "BL048-B1-005" "$ROOT_QA_DOC" \
    "BL048-FX-001" \
    "BL048-FX-010" \
    "failure_taxonomy.tsv"

  backlog_sig="$(rg -n 'BL048-A1-|BL048-FX-|BL048-B1-|shipping_platform_matrix|release_gate_matrix.tsv|packaging_manifest.tsv|checksums.tsv' "$ROOT_BACKLOG_DOC" | shasum -a 256 | awk '{print $1}')"
  qa_sig="$(rg -n 'BL048-A1-|BL048-FX-|BL048-B1-|acceptance_matrix.tsv|failure_taxonomy.tsv' "$ROOT_QA_DOC" | shasum -a 256 | awk '{print $1}')"

  signature="$(hash_text "${backlog_sig}|${qa_sig}")"
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
      append_failure_event "BL048-B1-FX-002" "$run" "signature_divergence"
      OVERALL_FAIL=1
    fi
    if [[ "$row_signature" != "$baseline_row_signature" ]]; then
      row_match=0
      row_drift_count=$((row_drift_count + 1))
      append_failure_event "BL048-B1-FX-003" "$run" "row_divergence"
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
        if (id == "BL048-B1-FX-001") {
          detail = "missing_required_pattern_or_file"
        } else if (id == "BL048-B1-FX-002") {
          detail = "replay_signature_divergence"
        } else if (id == "BL048-B1-FX-003") {
          detail = "replay_row_divergence"
        } else if (id == "BL048-B1-FX-005") {
          classification = "usage_error"
          detail = "usage_or_configuration_error"
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
  signature_result="PASS"
else
  log_status "replay_signature" "FAIL" "1" "drift_count=${signature_drift_count}" "$REPLAY_TSV"
  signature_result="FAIL"
fi

if [[ "$row_drift_count" -eq 0 ]]; then
  log_status "replay_rows" "PASS" "0" "stable" "$REPLAY_TSV"
  row_result="PASS"
else
  log_status "replay_rows" "FAIL" "1" "drift_count=${row_drift_count}" "$REPLAY_TSV"
  row_result="FAIL"
fi

taxonomy_nonzero_rows="$(awk -F'\t' 'NR>1 && $1!="none" && $2+0>0 {c++} END {print c+0}' "$FAILURE_TSV")"

printf "runs_observed\t%s\t>=1\tPASS\tobserved_runs\t%s\n" "$RUNS" "$STATUS_TSV" >> "$DRIFT_SUMMARY_TSV"
printf "signature_drift_count\t%s\t0\t%s\treplay_signature_drift\t%s\n" "$signature_drift_count" "$signature_result" "$REPLAY_TSV" >> "$DRIFT_SUMMARY_TSV"
printf "row_drift_count\t%s\t0\t%s\treplay_row_drift\t%s\n" "$row_drift_count" "$row_result" "$REPLAY_TSV" >> "$DRIFT_SUMMARY_TSV"
printf "taxonomy_nonzero_rows\t%s\t0\t%s\tfailure_taxonomy_nonzero\t%s\n" "$taxonomy_nonzero_rows" "$( [[ "$taxonomy_nonzero_rows" -eq 0 ]] && printf PASS || printf FAIL )" "$FAILURE_TSV" >> "$DRIFT_SUMMARY_TSV"
printf "requested_mode\t%s\tn/a\tPASS\tmode_recorded\t%s\n" "$MODE" "$STATUS_TSV" >> "$DRIFT_SUMMARY_TSV"

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  log_status "lane_result" "PASS" "0" "all_contract_checks_passed" "$STATUS_TSV"
  rm -f "$FAILURE_EVENTS_TSV"
  exit 0
fi

log_status "lane_result" "FAIL" "1" "contract_gate_failed" "$STATUS_TSV"
rm -f "$FAILURE_EVENTS_TSV"
exit 1
