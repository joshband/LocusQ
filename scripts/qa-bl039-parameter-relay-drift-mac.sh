#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUN_MODE="contract_only"
MODE_SET=0
RUNS=1
OUT_DIR="${ROOT_DIR}/TestEvidence/bl039_parameter_relay_drift_lane_${TIMESTAMP}"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-039-parameter-relay-spec-generation.md"
QA_DOC="${ROOT_DIR}/Documentation/testing/bl-039-parameter-relay-spec-generation-qa.md"

usage() {
  cat <<'USAGE'
Usage: qa-bl039-parameter-relay-drift-mac.sh [options]

Deterministic BL-039 parameter-relay drift lane wrapper.

Options:
  --contract-only     Run deterministic contract checks only (default mode).
  --execute-suite     Alias mode for lane parity (no build/runtime execution in contract slices).
  --runs <N>          Deterministic replay runs (integer >= 1, default: 1).
  --out-dir <path>    Output directory for lane artifacts.
  --help, -h          Show this help.

Artifacts:
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv
  drift_summary.tsv

Sentinel thresholds:
  C3 replay sentinel: runs >= 20
  C4 soak sentinel:   runs >= 50
  C5 mode parity:     runs >= 20 per mode

Exit semantics:
  0 = pass
  1 = lane/contract fail
  2 = usage error
USAGE
}

die_usage() {
  echo "ERROR: $1" >&2
  exit 2
}

is_uint() {
  [[ "$1" =~ ^[0-9]+$ ]]
}

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

hash_text() {
  printf '%s' "$1" | shasum -a 256 | awk '{print $1}'
}

hash_doc_semantic() {
  local path="$1"
  if [[ ! -f "$path" ]]; then
    printf 'missing'
    return 0
  fi
  sed '/^Last Modified Date:/d' "$path" | shasum -a 256 | awk '{print $1}'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only)
      if (( MODE_SET == 1 )) && [[ "$RUN_MODE" != "contract_only" ]]; then
        die_usage "--contract-only cannot be combined with --execute-suite"
      fi
      RUN_MODE="contract_only"
      MODE_SET=1
      shift
      ;;
    --execute-suite)
      if (( MODE_SET == 1 )) && [[ "$RUN_MODE" != "execute_suite" ]]; then
        die_usage "--execute-suite cannot be combined with --contract-only"
      fi
      RUN_MODE="execute_suite"
      MODE_SET=1
      shift
      ;;
    --runs)
      [[ $# -ge 2 ]] || die_usage "--runs requires a value"
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || die_usage "--out-dir requires a value"
      OUT_DIR="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      die_usage "unknown argument: $1"
      ;;
  esac
done

if ! is_uint "$RUNS" || (( RUNS < 1 )); then
  die_usage "--runs must be a positive integer"
fi

if [[ -z "$OUT_DIR" ]]; then
  die_usage "--out-dir must not be empty"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/validation_matrix.tsv"
REPLAY_HASHES_TSV="${OUT_DIR}/replay_hashes.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"
DRIFT_SUMMARY_TSV="${OUT_DIR}/drift_summary.tsv"
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"

printf "check\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_index\tgate_id\tgate\tthreshold\tmeasured_value\tresult\tartifact\n" > "$VALIDATION_TSV"
printf "run_index\thash_name\thash_value\tinput_signature\tresult\tartifact\n" > "$REPLAY_HASHES_TSV"
printf "metric\tvalue\tthreshold\tresult\tdetail\n" > "$DRIFT_SUMMARY_TSV"
: > "$FAILURE_EVENTS_TSV"

append_status() {
  local check="$1"
  local result="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$check")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$STATUS_TSV"
}

append_validation() {
  local run_index="$1"
  local gate_id="$2"
  local gate="$3"
  local threshold="$4"
  local measured_value="$5"
  local result="$6"
  local artifact="$7"
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(sanitize_tsv_field "$gate_id")" \
    "$(sanitize_tsv_field "$gate")" \
    "$(sanitize_tsv_field "$threshold")" \
    "$(sanitize_tsv_field "$measured_value")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$VALIDATION_TSV"
}

append_replay_hash() {
  local run_index="$1"
  local hash_name="$2"
  local hash_value="$3"
  local input_signature="$4"
  local result="$5"
  local artifact="$6"
  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(sanitize_tsv_field "$hash_name")" \
    "$(sanitize_tsv_field "$hash_value")" \
    "$(sanitize_tsv_field "$input_signature")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$REPLAY_HASHES_TSV"
}

record_failure() {
  local failure_class="$1"
  local detail="$2"
  local artifact="$3"
  printf "%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$failure_class")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$FAILURE_EVENTS_TSV"
}

require_command() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    append_status "tool_${cmd}" "PASS" "$(command -v "$cmd")" ""
    return 0
  fi
  append_status "tool_${cmd}" "FAIL" "missing_command" ""
  record_failure "runtime_execution_failure" "missing_required_tool:${cmd}" ""
  return 1
}

extract_unique_matches() {
  local pattern="$1"
  local path="$2"
  local out_path="$3"
  if [[ ! -f "$path" ]]; then
    : > "$out_path"
    return 0
  fi
  rg -o "$pattern" "$path" 2>/dev/null | sort -u > "$out_path" || : > "$out_path"
}

count_lines() {
  local path="$1"
  if [[ ! -f "$path" ]]; then
    printf '0'
    return 0
  fi
  awk 'END {print NR+0}' "$path"
}

runtime_failure=0
if ! require_command rg; then
  runtime_failure=1
fi
if ! require_command shasum; then
  runtime_failure=1
fi

if (( runtime_failure == 1 )); then
  printf "failure_class\tcount\tdetail\n" > "$FAILURE_TAXONOMY_TSV"
  printf "runtime_execution_failure\t1\trequired tools missing\n" >> "$FAILURE_TAXONOMY_TSV"
  printf "deterministic_contract_failure\t0\tcontract gates not evaluated\n" >> "$FAILURE_TAXONOMY_TSV"
  printf "deterministic_replay_divergence\t0\treplay hash mismatch\n" >> "$FAILURE_TAXONOMY_TSV"
  printf "deterministic_replay_row_drift\t0\treplay row signature mismatch\n" >> "$FAILURE_TAXONOMY_TSV"
  printf "missing_result_artifact\t0\trequired artifact missing\n" >> "$FAILURE_TAXONOMY_TSV"
  append_status "lane_result" "FAIL" "preflight_runtime_failure" "$STATUS_TSV"
  exit 1
fi

baseline_combined_signature=""
baseline_row_signature=""
signature_divergence_count=0
row_drift_count=0
run_failure_count=0

for run_index in $(seq 1 "$RUNS"); do
  run_label="run_$(printf '%02d' "$run_index")"
  run_dir="${OUT_DIR}/${run_label}"
  mkdir -p "$run_dir"
  run_status_tsv="${run_dir}/status.tsv"
  printf "check\tresult\tdetail\n" > "$run_status_tsv"

  run_gate_failures=0

  if [[ -f "$BACKLOG_DOC" ]]; then
    printf "backlog_doc_exists\tPASS\tfound\n" >> "$run_status_tsv"
  else
    printf "backlog_doc_exists\tFAIL\tmissing\n" >> "$run_status_tsv"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "missing_result_artifact" "missing_backlog_doc" "$BACKLOG_DOC"
  fi

  if [[ -f "$QA_DOC" ]]; then
    printf "qa_doc_exists\tPASS\tfound\n" >> "$run_status_tsv"
  else
    printf "qa_doc_exists\tFAIL\tmissing\n" >> "$run_status_tsv"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "missing_result_artifact" "missing_qa_doc" "$QA_DOC"
  fi

  schema_clause="missing"
  if rg -n 'schema_version|Canonical Parameter-Relay Schema|apvts_param_id' "$BACKLOG_DOC" >/dev/null 2>&1; then
    schema_clause="present"
  fi
  schema_result="PASS"
  if [[ "$schema_clause" != "present" ]]; then
    schema_result="FAIL"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "deterministic_contract_failure" "BL039-B1-001 schema clause missing" "$BACKLOG_DOC"
  fi
  append_validation "$run_label" "BL039-B1-001" "schema_contract" "required schema clauses present" "$schema_clause" "$schema_result" "$BACKLOG_DOC"

  ordering_clause="missing"
  if rg -n 'Sort precedence|mode_scope_rank|ordinal' "$BACKLOG_DOC" >/dev/null 2>&1; then
    ordering_clause="present"
  fi
  ordering_result="PASS"
  if [[ "$ordering_clause" != "present" ]]; then
    ordering_result="FAIL"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "deterministic_contract_failure" "BL039-B1-002 ordering clause missing" "$BACKLOG_DOC"
  fi
  append_validation "$run_label" "BL039-B1-002" "ordering_contract" "ordering + ordinal clauses present" "$ordering_clause" "$ordering_result" "$BACKLOG_DOC"

  drift_clause="missing"
  if rg -n 'spec_content_sha256|schema_definition_sha256|ordering_fingerprint_sha256' "$BACKLOG_DOC" >/dev/null 2>&1; then
    drift_clause="present"
  fi
  drift_result="PASS"
  if [[ "$drift_clause" != "present" ]]; then
    drift_result="FAIL"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "deterministic_contract_failure" "BL039-B1-003 drift hash clause missing" "$BACKLOG_DOC"
  fi
  append_validation "$run_label" "BL039-B1-003" "drift_hash_contract" "required deterministic hash clauses present" "$drift_clause" "$drift_result" "$BACKLOG_DOC"

  acceptance_backlog="${run_dir}/acceptance_backlog.txt"
  acceptance_qa="${run_dir}/acceptance_qa.txt"
  acceptance_b1_backlog="${run_dir}/acceptance_b1_backlog.txt"
  acceptance_b1_qa="${run_dir}/acceptance_b1_qa.txt"
  extract_unique_matches 'BL039-(A1|B1)-[0-9]{3}' "$BACKLOG_DOC" "$acceptance_backlog"
  extract_unique_matches 'BL039-(A1|B1)-[0-9]{3}' "$QA_DOC" "$acceptance_qa"
  extract_unique_matches 'BL039-B1-[0-9]{3}' "$BACKLOG_DOC" "$acceptance_b1_backlog"
  extract_unique_matches 'BL039-B1-[0-9]{3}' "$QA_DOC" "$acceptance_b1_qa"

  acceptance_count_backlog="$(count_lines "$acceptance_backlog")"
  acceptance_count_qa="$(count_lines "$acceptance_qa")"
  b1_count_backlog="$(count_lines "$acceptance_b1_backlog")"
  b1_count_qa="$(count_lines "$acceptance_b1_qa")"

  acceptance_diff_path="${run_dir}/acceptance_diff.patch"
  acceptance_parity="match"
  if ! diff -u "$acceptance_backlog" "$acceptance_qa" > "$acceptance_diff_path"; then
    acceptance_parity="mismatch"
  fi
  if (( b1_count_backlog < 7 || b1_count_qa < 7 )); then
    acceptance_parity="mismatch"
  fi

  acceptance_result="PASS"
  if [[ "$acceptance_parity" != "match" ]]; then
    acceptance_result="FAIL"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "deterministic_contract_failure" "BL039-B1-004 acceptance parity mismatch" "$acceptance_diff_path"
  fi
  append_validation "$run_label" "BL039-B1-004" "acceptance_parity" "A1/B1 acceptance sets equal and B1 count>=7" "backlog=${acceptance_count_backlog};qa=${acceptance_count_qa};b1_backlog=${b1_count_backlog};b1_qa=${b1_count_qa};parity=${acceptance_parity}" "$acceptance_result" "$acceptance_diff_path"

  fx_backlog="${run_dir}/failure_backlog.txt"
  fx_qa="${run_dir}/failure_qa.txt"
  extract_unique_matches 'BL039-FX-[0-9]{3}' "$BACKLOG_DOC" "$fx_backlog"
  extract_unique_matches 'BL039-FX-[0-9]{3}' "$QA_DOC" "$fx_qa"
  fx_count_backlog="$(count_lines "$fx_backlog")"
  fx_count_qa="$(count_lines "$fx_qa")"
  fx_diff_path="${run_dir}/failure_diff.patch"
  fx_parity="match"
  if ! diff -u "$fx_backlog" "$fx_qa" > "$fx_diff_path"; then
    fx_parity="mismatch"
  fi
  if (( fx_count_backlog < 9 || fx_count_qa < 9 )); then
    fx_parity="mismatch"
  fi

  fx_result="PASS"
  if [[ "$fx_parity" != "match" ]]; then
    fx_result="FAIL"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "deterministic_contract_failure" "BL039-B1-005 failure taxonomy mismatch" "$fx_diff_path"
  fi
  append_validation "$run_label" "BL039-B1-005" "failure_taxonomy_parity" "failure taxonomy sets equal and count>=9" "backlog=${fx_count_backlog};qa=${fx_count_qa};parity=${fx_parity}" "$fx_result" "$fx_diff_path"

  artifact_clause="missing"
  if rg -n 'status\.tsv|validation_matrix\.tsv|replay_hashes\.tsv|failure_taxonomy\.tsv' "$QA_DOC" >/dev/null 2>&1; then
    artifact_clause="present"
  fi
  artifact_result="PASS"
  if [[ "$artifact_clause" != "present" ]]; then
    artifact_result="FAIL"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "deterministic_contract_failure" "BL039-B1-006 artifact schema clause missing" "$QA_DOC"
  fi
  append_validation "$run_label" "BL039-B1-006" "artifact_schema_contract" "required lane artifacts declared" "$artifact_clause" "$artifact_result" "$QA_DOC"

  mode_result="PASS"
  if [[ "$RUN_MODE" != "contract_only" && "$RUN_MODE" != "execute_suite" ]]; then
    mode_result="FAIL"
    run_gate_failures=$((run_gate_failures + 1))
    record_failure "deterministic_contract_failure" "BL039-B1-007 unsupported mode" "$STATUS_TSV"
  fi
  append_validation "$run_label" "BL039-B1-007" "execution_mode_contract" "mode in {contract_only,execute_suite}" "$RUN_MODE" "$mode_result" "$STATUS_TSV"

  backlog_hash="$(hash_doc_semantic "$BACKLOG_DOC")"
  qa_hash="$(hash_doc_semantic "$QA_DOC")"
  acceptance_hash="$(hash_text "$(cat "$acceptance_backlog" 2>/dev/null; printf '|'; cat "$acceptance_qa" 2>/dev/null)")"
  failure_hash="$(hash_text "$(cat "$fx_backlog" 2>/dev/null; printf '|'; cat "$fx_qa" 2>/dev/null)")"

  input_signature="mode=${RUN_MODE}|schema=${schema_clause}|ordering=${ordering_clause}|drift=${drift_clause}|artifact=${artifact_clause}|acceptance_parity=${acceptance_parity}|fx_parity=${fx_parity}|acceptance_b1=${b1_count_backlog}/${b1_count_qa}|fx=${fx_count_backlog}/${fx_count_qa}"
  combined_signature="$(hash_text "${backlog_hash}|${qa_hash}|${acceptance_hash}|${failure_hash}|${input_signature}")"
  row_signature="$(hash_text "${schema_result}|${ordering_result}|${drift_result}|${acceptance_result}|${fx_result}|${artifact_result}|${mode_result}")"

  hash_result="PASS"
  row_result="PASS"
  if [[ -z "$baseline_combined_signature" ]]; then
    baseline_combined_signature="$combined_signature"
    baseline_row_signature="$row_signature"
  else
    if [[ "$combined_signature" != "$baseline_combined_signature" ]]; then
      hash_result="FAIL"
      signature_divergence_count=$((signature_divergence_count + 1))
      record_failure "deterministic_replay_divergence" "combined signature mismatch run=${run_label}" "$REPLAY_HASHES_TSV"
    fi
    if [[ "$row_signature" != "$baseline_row_signature" ]]; then
      row_result="FAIL"
      row_drift_count=$((row_drift_count + 1))
      record_failure "deterministic_replay_row_drift" "row signature mismatch run=${run_label}" "$REPLAY_HASHES_TSV"
    fi
  fi

  append_replay_hash "$run_label" "backlog_semantic_sha256" "$backlog_hash" "$input_signature" "PASS" "$BACKLOG_DOC"
  append_replay_hash "$run_label" "qa_semantic_sha256" "$qa_hash" "$input_signature" "PASS" "$QA_DOC"
  append_replay_hash "$run_label" "acceptance_set_sha256" "$acceptance_hash" "$input_signature" "$acceptance_result" "$acceptance_diff_path"
  append_replay_hash "$run_label" "failure_set_sha256" "$failure_hash" "$input_signature" "$fx_result" "$fx_diff_path"
  append_replay_hash "$run_label" "combined_signature_sha256" "$combined_signature" "$input_signature" "$hash_result" "$REPLAY_HASHES_TSV"
  append_replay_hash "$run_label" "row_signature_sha256" "$row_signature" "$input_signature" "$row_result" "$REPLAY_HASHES_TSV"

  if (( run_gate_failures == 0 )) && [[ "$hash_result" == "PASS" ]] && [[ "$row_result" == "PASS" ]]; then
    printf "lane_result\tPASS\tall_contract_gates_passed\n" >> "$run_status_tsv"
  else
    printf "lane_result\tFAIL\trun_gate_failures=%s hash_result=%s row_result=%s\n" "$run_gate_failures" "$hash_result" "$row_result" >> "$run_status_tsv"
    run_failure_count=$((run_failure_count + 1))
  fi
done

required_outputs=(
  "$STATUS_TSV"
  "$VALIDATION_TSV"
  "$REPLAY_HASHES_TSV"
  "$DRIFT_SUMMARY_TSV"
  "$FAILURE_EVENTS_TSV"
)
missing_artifacts=0
for output_path in "${required_outputs[@]}"; do
  if [[ ! -f "$output_path" ]]; then
    missing_artifacts=$((missing_artifacts + 1))
    record_failure "missing_result_artifact" "missing_output:${output_path}" "$output_path"
  fi
done

if [[ "$signature_divergence_count" -eq 0 ]]; then
  append_status "BL039-B1-004_replay_hash_stability" "PASS" "signature_divergence=0 threshold=0" "$REPLAY_HASHES_TSV"
else
  append_status "BL039-B1-004_replay_hash_stability" "FAIL" "signature_divergence=${signature_divergence_count} threshold=0" "$REPLAY_HASHES_TSV"
fi

if [[ "$row_drift_count" -eq 0 ]]; then
  append_status "BL039-B1-005_replay_row_stability" "PASS" "row_drift=0 threshold=0" "$REPLAY_HASHES_TSV"
else
  append_status "BL039-B1-005_replay_row_stability" "FAIL" "row_drift=${row_drift_count} threshold=0" "$REPLAY_HASHES_TSV"
fi

if [[ "$missing_artifacts" -eq 0 ]]; then
  append_status "BL039-B1-006_artifact_schema_complete" "PASS" "required_outputs_present" "$OUT_DIR"
else
  append_status "BL039-B1-006_artifact_schema_complete" "FAIL" "missing_artifacts=${missing_artifacts}" "$OUT_DIR"
fi

append_status "BL039-B1-007_execution_mode_contract" "PASS" "mode=${RUN_MODE}" "$STATUS_TSV"

printf "failure_class\tcount\tdetail\n" > "$FAILURE_TAXONOMY_TSV"
for class_name in \
  deterministic_contract_failure \
  deterministic_replay_divergence \
  deterministic_replay_row_drift \
  runtime_execution_failure \
  missing_result_artifact; do
  class_count="$(awk -F'\t' -v c="$class_name" '$1==c {n++} END {print n+0}' "$FAILURE_EVENTS_TSV")"
  case "$class_name" in
    deterministic_contract_failure)
      class_detail="contract gate failure"
      ;;
    deterministic_replay_divergence)
      class_detail="combined replay hash mismatch"
      ;;
    deterministic_replay_row_drift)
      class_detail="row-signature drift"
      ;;
    runtime_execution_failure)
      class_detail="tool/runtime execution failure"
      ;;
    missing_result_artifact)
      class_detail="required artifact missing"
      ;;
    *)
      class_detail="n/a"
      ;;
  esac
  printf "%s\t%s\t%s\n" "$class_name" "$class_count" "$class_detail" >> "$FAILURE_TAXONOMY_TSV"
done

deterministic_contract_failure_count="$(awk -F'\t' '$1=="deterministic_contract_failure" {print $2}' "$FAILURE_TAXONOMY_TSV")"
runtime_execution_failure_count="$(awk -F'\t' '$1=="runtime_execution_failure" {print $2}' "$FAILURE_TAXONOMY_TSV")"
missing_result_artifact_count="$(awk -F'\t' '$1=="missing_result_artifact" {print $2}' "$FAILURE_TAXONOMY_TSV")"
replay_divergence_count="$(awk -F'\t' '$1=="deterministic_replay_divergence" {print $2}' "$FAILURE_TAXONOMY_TSV")"
replay_row_drift_count="$(awk -F'\t' '$1=="deterministic_replay_row_drift" {print $2}' "$FAILURE_TAXONOMY_TSV")"

append_drift_summary() {
  local metric="$1"
  local value="$2"
  local threshold="$3"
  local result="$4"
  local detail="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$metric")" \
    "$(sanitize_tsv_field "$value")" \
    "$(sanitize_tsv_field "$threshold")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$detail")" \
    >> "$DRIFT_SUMMARY_TSV"
}

if [[ "$signature_divergence_count" -eq 0 ]]; then
  append_drift_summary "signature_divergence_count" "0" "0" "PASS" "combined signatures match baseline across all runs"
else
  append_drift_summary "signature_divergence_count" "$signature_divergence_count" "0" "FAIL" "combined signatures diverged from baseline"
fi

if [[ "$row_drift_count" -eq 0 ]]; then
  append_drift_summary "row_drift_count" "0" "0" "PASS" "row signatures match baseline across all runs"
else
  append_drift_summary "row_drift_count" "$row_drift_count" "0" "FAIL" "row signatures diverged from baseline"
fi

if (( RUNS >= 20 )); then
  append_drift_summary "c3_sentinel_run_count" "$RUNS" ">=20" "PASS" "replay sentinel run-count threshold satisfied"
else
  append_drift_summary "c3_sentinel_run_count" "$RUNS" ">=20" "SKIP" "replay sentinel threshold not requested for this invocation"
fi

if (( RUNS >= 50 )); then
  append_drift_summary "c4_soak_run_count" "$RUNS" ">=50" "PASS" "soak sentinel run-count threshold satisfied"
else
  append_drift_summary "c4_soak_run_count" "$RUNS" ">=50" "SKIP" "soak sentinel threshold not requested for this invocation"
fi

if (( RUNS >= 20 )); then
  append_drift_summary "c5_mode_parity_min_run_count" "$RUNS" ">=20" "PASS" "mode parity minimum run-count satisfied for this invocation"
else
  append_drift_summary "c5_mode_parity_min_run_count" "$RUNS" ">=20" "SKIP" "mode parity minimum run-count not requested for this invocation"
fi

if [[ "$run_failure_count" -eq 0 ]]; then
  append_drift_summary "run_failure_count" "0" "0" "PASS" "all run-level gates passed"
else
  append_drift_summary "run_failure_count" "$run_failure_count" "0" "FAIL" "one or more run-level gates failed"
fi

if [[ "$deterministic_contract_failure_count" -eq 0 ]]; then
  append_drift_summary "deterministic_contract_failure_count" "0" "0" "PASS" "no contract gate failures"
else
  append_drift_summary "deterministic_contract_failure_count" "$deterministic_contract_failure_count" "0" "FAIL" "contract gate failures recorded"
fi

if [[ "$replay_divergence_count" -eq 0 ]]; then
  append_drift_summary "deterministic_replay_divergence_count" "0" "0" "PASS" "no replay hash divergence"
else
  append_drift_summary "deterministic_replay_divergence_count" "$replay_divergence_count" "0" "FAIL" "replay hash divergence recorded"
fi

if [[ "$replay_row_drift_count" -eq 0 ]]; then
  append_drift_summary "deterministic_replay_row_drift_count" "0" "0" "PASS" "no replay row drift"
else
  append_drift_summary "deterministic_replay_row_drift_count" "$replay_row_drift_count" "0" "FAIL" "replay row drift recorded"
fi

final_failures=0
if [[ "$deterministic_contract_failure_count" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$runtime_execution_failure_count" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$missing_result_artifact_count" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$signature_divergence_count" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$row_drift_count" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$run_failure_count" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi

if [[ "$final_failures" -eq 0 ]]; then
  append_status "BL039-C5-002_execute_mode_alias_contract" "PASS" "requested_mode=${RUN_MODE};effective_behavior=contract_checks_only" "$STATUS_TSV"
  if (( RUNS >= 20 )); then
    append_status "BL039-C3-001_replay_sentinel" "PASS" "runs=${RUNS};signature_divergence=${signature_divergence_count};row_drift=${row_drift_count}" "$DRIFT_SUMMARY_TSV"
    append_status "BL039-C5-001_mode_guard_threshold" "PASS" "mode=${RUN_MODE};runs=${RUNS};parity_min_runs_met" "$DRIFT_SUMMARY_TSV"
  else
    append_status "BL039-C3-001_replay_sentinel" "SKIP" "runs=${RUNS};requires>=20 for C3 sentinel packet" "$DRIFT_SUMMARY_TSV"
    append_status "BL039-C5-001_mode_guard_threshold" "SKIP" "mode=${RUN_MODE};runs=${RUNS};requires>=20 for C5 parity guard" "$DRIFT_SUMMARY_TSV"
  fi
  if (( RUNS >= 50 )); then
    append_status "BL039-C4-001_replay_sentinel_soak" "PASS" "runs=${RUNS};signature_divergence=${signature_divergence_count};row_drift=${row_drift_count}" "$DRIFT_SUMMARY_TSV"
  else
    append_status "BL039-C4-001_replay_sentinel_soak" "SKIP" "runs=${RUNS};requires>=50 for C4 soak packet" "$DRIFT_SUMMARY_TSV"
  fi
  append_status "lane_result" "PASS" "mode=${RUN_MODE};runs=${RUNS}" "$STATUS_TSV"
  exit 0
fi

append_status "lane_result" "FAIL" "mode=${RUN_MODE};runs=${RUNS};final_failures=${final_failures}" "$STATUS_TSV"
exit 1
