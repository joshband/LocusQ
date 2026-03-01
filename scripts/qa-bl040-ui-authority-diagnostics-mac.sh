#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUNS=1
CONTRACT_ONLY=0
OUT_DIR="${ROOT_DIR}/TestEvidence/bl040_ui_authority_diag_${TIMESTAMP}"

usage() {
  cat <<'USAGE'
Usage: qa-bl040-ui-authority-diagnostics-mac.sh [options]

BL-040 authority-status diagnostics contract lane.

Options:
  --runs <N>         Replay run count (integer >= 1)
  --out-dir <path>   Artifact output directory
  --contract-only    Run deterministic contract checks only (default behavior)
  --help, -h         Show usage

Outputs:
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv
  ui_diagnostics_summary.tsv

Exit semantics:
  0  pass
  1  gate fail
  2  usage/config error
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
      CONTRACT_ONLY=1
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
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"
SUMMARY_TSV="${OUT_DIR}/ui_diagnostics_summary.tsv"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$VALIDATION_TSV"
printf "run\tsignature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" > "$REPLAY_TSV"
printf "failure_id\tcount\tclassification\tdetail\n" > "$FAILURE_TSV"
printf "metric\tvalue\tdetail\tartifact\n" > "$SUMMARY_TSV"
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

ROOT_HTML="${ROOT_DIR}/Source/ui/public/index.html"
ROOT_JS="${ROOT_DIR}/Source/ui/public/js/index.js"
ROOT_QA_DOC="${ROOT_DIR}/Documentation/testing/bl-040-ui-modularization-and-authority-status-qa.md"
ROOT_BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-040-ui-modularization-and-authority-status.md"

OVERALL_FAIL=0

for req in "$ROOT_HTML" "$ROOT_JS" "$ROOT_QA_DOC" "$ROOT_BACKLOG_DOC"; do
  if [[ -f "$req" ]]; then
    log_status "file_exists" "PASS" "0" "found" "$req"
  else
    log_status "file_exists" "FAIL" "1" "missing" "$req"
    append_failure_event "BL040-B1-901" "0" "missing_required_file:$(basename "$req")"
    OVERALL_FAIL=1
  fi
done

if [[ "$CONTRACT_ONLY" -eq 1 ]]; then
  log_status "mode" "PASS" "0" "contract_only" "$STATUS_TSV"
else
  log_status "mode" "PASS" "0" "runtime_not_enabled_in_this_lane;running_contract_checks" "$STATUS_TSV"
fi

baseline_signature=""
baseline_row_signature=""
signature_drift_count=0
row_drift_count=0

for run in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run}.log"
  : > "$run_log"

  run_fail_count=0

  run_check() {
    local check_id="$1"
    local pattern="$2"
    local file_path="$3"
    if rg -q "$pattern" "$file_path"; then
      add_validation "$run" "$check_id" "PASS" "pattern_present" "$file_path"
      printf "[%s] %s PASS (%s)\n" "$run" "$check_id" "$pattern" >> "$run_log"
    else
      add_validation "$run" "$check_id" "FAIL" "pattern_missing:$pattern" "$file_path"
      append_failure_event "BL040-B1-901" "$run" "$check_id:pattern_missing"
      run_fail_count=$((run_fail_count + 1))
      OVERALL_FAIL=1
      printf "[%s] %s FAIL (%s)\n" "$run" "$check_id" "$pattern" >> "$run_log"
    fi
  }

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
      append_failure_event "BL040-B1-901" "$run" "$check_id:missing_patterns"
      run_fail_count=$((run_fail_count + 1))
      OVERALL_FAIL=1
      printf "[%s] %s FAIL (missing_patterns:%s)\n" "$run" "$check_id" "${missing[*]}" >> "$run_log"
    fi
  }

  run_check "BL040-B1-001" "rend-auth-card" "$ROOT_HTML"
  run_check "BL040-B1-002" "rend-auth-toggle" "$ROOT_HTML"
  run_check_all_patterns "BL040-B1-003" "$ROOT_HTML" \
    "rend-auth-source" \
    "rend-auth-status-class" \
    "rend-auth-lock-reason" \
    "rend-auth-snapshot-age" \
    "rend-auth-fallback-reason" \
    "rend-auth-replay-seq"
  run_check "BL040-B1-004" "hasRendererAuthorityDiagnosticsPayload" "$ROOT_JS"
  run_check "BL040-B1-005" "setRendererAuthorityDiagnosticsExpanded" "$ROOT_JS"
  run_check_all_patterns "BL040-B1-006" "$ROOT_JS" \
    "authoritySource" \
    "authorityStatusClass" \
    "authorityLockReason" \
    "authoritySnapshotAgeMs" \
    "authorityFallbackReason" \
    "authorityReplaySeq"
  run_check "BL040-B1-007" "BL040-B1-001" "$ROOT_QA_DOC"
  run_check "BL040-B1-008" "BL040-B1-001" "$ROOT_BACKLOG_DOC"

  html_sig="$(rg -n 'rend-auth-(card|toggle|chip|detail|content|source|status-class|lock-reason|snapshot-age|fallback-reason|replay-seq)' "$ROOT_HTML" | shasum -a 256 | awk '{print $1}')"
  js_sig="$(rg -n 'hasRendererAuthorityDiagnosticsPayload|setRendererAuthorityDiagnosticsExpanded|authoritySource|authorityStatusClass|authorityLockReason|authoritySnapshotAgeMs|authorityFallbackReason|authorityReplaySeq' "$ROOT_JS" | shasum -a 256 | awk '{print $1}')"
  qa_sig="$(rg -n 'BL040-B1-' "$ROOT_QA_DOC" | shasum -a 256 | awk '{print $1}')"
  backlog_sig="$(rg -n 'BL040-B1-' "$ROOT_BACKLOG_DOC" | shasum -a 256 | awk '{print $1}')"

  signature="$(hash_text "${html_sig}|${js_sig}|${qa_sig}|${backlog_sig}")"
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
      append_failure_event "BL040-B1-902" "$run" "signature_divergence"
      OVERALL_FAIL=1
    fi
    if [[ "$row_signature" != "$baseline_row_signature" ]]; then
      row_match=0
      row_drift_count=$((row_drift_count + 1))
      append_failure_event "BL040-B1-903" "$run" "row_divergence"
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
        if (id == "BL040-B1-901") {
          detail = "missing_required_pattern_or_file"
        } else if (id == "BL040-B1-902") {
          detail = "replay_signature_divergence"
        } else if (id == "BL040-B1-903") {
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

runs_observed="$(awk 'NR > 1 { count++ } END { print count + 0 }' "$REPLAY_TSV")"
contract_fail_rows="$(awk -F'\t' 'NR > 1 && $3 == "FAIL" { count++ } END { print count + 0 }' "$VALIDATION_TSV")"
taxonomy_nonzero_rows="$(awk -F'\t' 'NR > 1 && $1 != "none" && ($2 + 0) > 0 { count++ } END { print count + 0 }' "$FAILURE_TSV")"

determinism_gate="PASS"
if [[ "$signature_drift_count" -ne 0 || "$row_drift_count" -ne 0 ]]; then
  determinism_gate="FAIL"
fi

taxonomy_gate="PASS"
if [[ "$taxonomy_nonzero_rows" -ne 0 ]]; then
  taxonomy_gate="FAIL"
fi

overall_result="PASS"
if [[ "$OVERALL_FAIL" -ne 0 ]]; then
  overall_result="FAIL"
fi

printf "runs_requested\t%s\trequested via --runs\t%s\n" "$RUNS" "$STATUS_TSV" >> "$SUMMARY_TSV"
printf "runs_observed\t%s\trows in replay_hashes.tsv\t%s\n" "$runs_observed" "$REPLAY_TSV" >> "$SUMMARY_TSV"
printf "signature_drift_count\t%s\tsignature_match==0 rows\t%s\n" "$signature_drift_count" "$REPLAY_TSV" >> "$SUMMARY_TSV"
printf "row_drift_count\t%s\trow_match==0 rows\t%s\n" "$row_drift_count" "$REPLAY_TSV" >> "$SUMMARY_TSV"
printf "contract_fail_rows\t%s\tvalidation_matrix FAIL rows\t%s\n" "$contract_fail_rows" "$VALIDATION_TSV" >> "$SUMMARY_TSV"
printf "taxonomy_nonzero_rows\t%s\tnon-none taxonomy rows with count>0\t%s\n" "$taxonomy_nonzero_rows" "$FAILURE_TSV" >> "$SUMMARY_TSV"
printf "determinism_gate\t%s\tsignature+row stability gate\t%s\n" "$determinism_gate" "$REPLAY_TSV" >> "$SUMMARY_TSV"
printf "taxonomy_gate\t%s\tfailure taxonomy stability gate\t%s\n" "$taxonomy_gate" "$FAILURE_TSV" >> "$SUMMARY_TSV"
printf "overall_result\t%s\trequired validation aggregate\t%s\n" "$overall_result" "$STATUS_TSV" >> "$SUMMARY_TSV"
log_status "diagnostics_summary" "PASS" "0" "emitted" "$SUMMARY_TSV"

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  log_status "lane_result" "PASS" "0" "all_contract_checks_passed" "$STATUS_TSV"
  rm -f "$FAILURE_EVENTS_TSV"
  exit 0
fi

log_status "lane_result" "FAIL" "1" "contract_gate_failed" "$STATUS_TSV"
rm -f "$FAILURE_EVENTS_TSV"
exit 1
