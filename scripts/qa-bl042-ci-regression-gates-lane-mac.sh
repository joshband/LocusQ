#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUNS=1
MODE="contract_only"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl042_ci_regression_gates_lane_${TIMESTAMP}"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-042-qa-ci-regression-gates.md"
QA_DOC="${ROOT_DIR}/Documentation/testing/bl-042-qa-ci-regression-gates-qa.md"
SCRIPT_PATH="${ROOT_DIR}/scripts/qa-bl042-ci-regression-gates-lane-mac.sh"
SELFTEST_SCRIPT="${ROOT_DIR}/scripts/standalone-ui-selftest-production-p0-mac.sh"
RT_AUDIT_SCRIPT="${ROOT_DIR}/scripts/rt-safety-audit.sh"
DOCS_SCRIPT="${ROOT_DIR}/scripts/validate-docs-freshness.sh"
SMOKE_SUITE="${ROOT_DIR}/qa/scenarios/locusq_smoke_suite.json"
SMOKE_BIN="${ROOT_DIR}/build_local/locusq_qa_artefacts/Release/locusq_qa"
STATUS_JSON="${ROOT_DIR}/status.json"

usage() {
  cat <<'USAGE'
Usage: qa-bl042-ci-regression-gates-lane-mac.sh [options]

BL-042 deterministic CI regression gates lane.

Options:
  --runs <N>         Replay run count (integer >= 1)
  --out-dir <path>   Artifact output directory
  --contract-only    Run contract checks only (default)
  --execute-suite    Run deterministic ordered CI gate execution suite
  --help, -h         Show usage

Outputs (written under --out-dir):
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv
  gate_results.tsv

Exit semantics:
  0  pass
  1  lane fail
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
      MODE="contract_only"
      shift
      ;;
    --execute-suite)
      MODE="execute_suite"
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
GATE_TSV="${OUT_DIR}/gate_results.tsv"
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"

printf "check\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_id\tcheck_id\tresult\tdetail\tartifact\n" > "$VALIDATION_TSV"
printf "run_id\tsignature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" > "$REPLAY_TSV"
printf "failure_class\tcount\tdetail\n" > "$FAILURE_TSV"
printf "run_id\tgate_id\tordinal\tcommand\texit_code\tresult\tartifact\n" > "$GATE_TSV"
: > "$FAILURE_EVENTS_TSV"

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf "%s" "$value"
}

add_status() {
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

add_validation() {
  local run_id="$1"
  local check_id="$2"
  local result="$3"
  local detail="$4"
  local artifact="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_id")" \
    "$(sanitize_tsv_field "$check_id")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$VALIDATION_TSV"
}

add_gate() {
  local run_id="$1"
  local gate_id="$2"
  local ordinal="$3"
  local command="$4"
  local exit_code="$5"
  local result="$6"
  local artifact="$7"
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_id")" \
    "$(sanitize_tsv_field "$gate_id")" \
    "$(sanitize_tsv_field "$ordinal")" \
    "$(sanitize_tsv_field "$command")" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$GATE_TSV"
}

append_failure_event() {
  local failure_class="$1"
  local run_id="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$failure_class")" \
    "$(sanitize_tsv_field "$run_id")" \
    "$(sanitize_tsv_field "$detail")" \
    >> "$FAILURE_EVENTS_TSV"
}

hash_text() {
  local text="$1"
  printf "%s" "$text" | shasum -a 256 | awk '{print $1}'
}

run_check_pattern() {
  local run_id="$1"
  local check_id="$2"
  local pattern="$3"
  local file_path="$4"
  local run_fail_count="$5"
  if rg -q -- "$pattern" "$file_path"; then
    add_validation "$run_id" "$check_id" "PASS" "pattern_present" "$file_path"
  else
    add_validation "$run_id" "$check_id" "FAIL" "pattern_missing:${pattern}" "$file_path"
    append_failure_event "deterministic_contract_failure" "$run_id" "${check_id}:pattern_missing"
    run_fail_count=$((run_fail_count + 1))
    OVERALL_FAIL=1
  fi
  printf "%s" "$run_fail_count"
}

run_gate_command() {
  local run_id="$1"
  local gate_id="$2"
  local ordinal="$3"
  local command="$4"
  local log_path="$5"

  local exit_code=0
  if bash -lc "$command" >"$log_path" 2>&1; then
    exit_code=0
    add_gate "$run_id" "$gate_id" "$ordinal" "$command" "$exit_code" "PASS" "$log_path"
    add_validation "$run_id" "$gate_id" "PASS" "exit_code=0" "$log_path"
  else
    exit_code=$?
    add_gate "$run_id" "$gate_id" "$ordinal" "$command" "$exit_code" "FAIL" "$log_path"
    add_validation "$run_id" "$gate_id" "FAIL" "exit_code=${exit_code}" "$log_path"
    append_failure_event "gate_execution_failure" "$run_id" "${gate_id}:exit_${exit_code}"
  fi

  printf "%s" "$exit_code"
}

count_failure() {
  local failure_class="$1"
  awk -F'\t' -v klass="$failure_class" '$1==klass{count++} END{print count+0}' "$FAILURE_EVENTS_TSV"
}

OVERALL_FAIL=0

if ! command -v rg >/dev/null 2>&1; then
  add_status "tooling" "FAIL" "missing_rg" "$STATUS_TSV"
  printf "runtime_execution_failure\t1\trg missing\n" >> "$FAILURE_TSV"
  add_status "lane_result" "FAIL" "missing_required_tool_rg" "$STATUS_TSV"
  exit 1
fi

if ! command -v shasum >/dev/null 2>&1; then
  add_status "tooling" "FAIL" "missing_shasum" "$STATUS_TSV"
  printf "runtime_execution_failure\t1\tshasum missing\n" >> "$FAILURE_TSV"
  add_status "lane_result" "FAIL" "missing_required_tool_shasum" "$STATUS_TSV"
  exit 1
fi

if [[ "$MODE" == "execute_suite" ]] && ! command -v jq >/dev/null 2>&1; then
  add_status "tooling" "FAIL" "missing_jq_for_execute_suite" "$STATUS_TSV"
  printf "runtime_execution_failure\t1\tjq missing\n" >> "$FAILURE_TSV"
  add_status "lane_result" "FAIL" "missing_required_tool_jq" "$STATUS_TSV"
  exit 1
fi

if [[ "$MODE" == "contract_only" ]]; then
  add_status "mode" "PASS" "mode=contract_only" "$STATUS_TSV"
else
  add_status "mode" "PASS" "mode=execute_suite" "$STATUS_TSV"
fi

for req in "$BACKLOG_DOC" "$QA_DOC" "$SCRIPT_PATH"; do
  if [[ -f "$req" ]]; then
    add_status "file_exists" "PASS" "found" "$req"
  else
    add_status "file_exists" "FAIL" "missing" "$req"
    append_failure_event "missing_result_artifact" "0" "missing_required_file:$(basename "$req")"
    OVERALL_FAIL=1
  fi
done

if [[ "$MODE" == "execute_suite" ]]; then
  for req in "$SELFTEST_SCRIPT" "$RT_AUDIT_SCRIPT" "$DOCS_SCRIPT" "$STATUS_JSON" "$SMOKE_SUITE"; do
    if [[ -f "$req" ]]; then
      add_status "execute_suite_input" "PASS" "found" "$req"
    else
      add_status "execute_suite_input" "FAIL" "missing" "$req"
      append_failure_event "missing_result_artifact" "0" "missing_execute_suite_input:$(basename "$req")"
      OVERALL_FAIL=1
    fi
  done
fi

baseline_signature=""
baseline_row_signature=""
signature_drift_count=0
row_drift_count=0

for run in $(seq 1 "$RUNS"); do
  run_id="$(printf "run_%02d" "$run")"
  run_fail_count=0

  if [[ "$MODE" == "contract_only" ]]; then
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-001" "ci_gate_build|ci_gate_smoke|ci_gate_selftest|ci_gate_rt|ci_gate_docs|ci_gate_schema" "$BACKLOG_DOC" "$run_fail_count")"
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-002" "Required deterministic gate order" "$BACKLOG_DOC" "$run_fail_count")"
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-003" "BL042-A1-001|BL042-A1-011" "$BACKLOG_DOC" "$run_fail_count")"
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-004" "BL042-FX-001|BL042-FX-010" "$BACKLOG_DOC" "$run_fail_count")"
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-005" "status.tsv|validation_matrix.tsv|replay_hashes.tsv|failure_taxonomy.tsv" "$BACKLOG_DOC" "$run_fail_count")"
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-006" "BL042-A1-001|BL042-A1-011" "$QA_DOC" "$run_fail_count")"
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-007" "BL042-FX-001|BL042-FX-010" "$QA_DOC" "$run_fail_count")"
    run_fail_count="$(run_check_pattern "$run_id" "BL042-B1-008" "Exit semantics:" "$SCRIPT_PATH" "$run_fail_count")"

    semantic_payload="$(
      {
        printf "mode=%s\n" "$MODE"
        rg -No -- "ci_gate_[a-z_]+" "$BACKLOG_DOC" | sort -u
        rg -No -- "BL042-A1-[0-9]{3}" "$BACKLOG_DOC" "$QA_DOC" | sort -u
        rg -No -- "BL042-FX-[0-9]{3}" "$BACKLOG_DOC" "$QA_DOC" | sort -u
        rg -No -- "status.tsv|validation_matrix.tsv|replay_hashes.tsv|failure_taxonomy.tsv" "$BACKLOG_DOC" "$QA_DOC" | sort -u
      } | sed '/^$/d'
    )"
  else
    run_dir="${OUT_DIR}/${run_id}"
    mkdir -p "$run_dir"

    build_cmd='cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8'
    smoke_cmd='./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json'
    selftest_cmd='LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh'
    rt_tsv="${run_dir}/rt_audit.tsv"
    rt_cmd="./scripts/rt-safety-audit.sh --print-summary --output ${rt_tsv}"
    docs_cmd='./scripts/validate-docs-freshness.sh'
    schema_cmd='jq empty status.json'

    ec="$(run_gate_command "$run_id" "ci_gate_build" "1" "$build_cmd" "${run_dir}/gate_01_build.log")"
    [[ "$ec" -eq 0 ]] || run_fail_count=$((run_fail_count + 1))

    ec="$(run_gate_command "$run_id" "ci_gate_smoke" "2" "$smoke_cmd" "${run_dir}/gate_02_smoke.log")"
    [[ "$ec" -eq 0 ]] || run_fail_count=$((run_fail_count + 1))

    ec="$(run_gate_command "$run_id" "ci_gate_selftest" "3" "$selftest_cmd" "${run_dir}/gate_03_selftest.log")"
    [[ "$ec" -eq 0 ]] || run_fail_count=$((run_fail_count + 1))

    ec="$(run_gate_command "$run_id" "ci_gate_rt" "4" "$rt_cmd" "${run_dir}/gate_04_rt.log")"
    if [[ "$ec" -eq 0 ]]; then
      non_allowlisted=""
      if [[ -f "$rt_tsv" ]]; then
        non_allowlisted="$(awk -F'\t' 'tolower($1)=="summary" && tolower($2)=="non_allowlisted" {print $3}' "$rt_tsv" | tail -n 1)"
      fi
      if [[ -z "$non_allowlisted" ]]; then
        non_allowlisted="$(rg -No 'non_allowlisted=([0-9]+)' "${run_dir}/gate_04_rt.log" | sed -E 's/.*=([0-9]+)/\1/' | tail -n 1 || true)"
      fi
      if [[ -z "$non_allowlisted" ]]; then
        add_validation "$run_id" "ci_gate_rt_threshold" "FAIL" "non_allowlisted_missing" "$rt_tsv"
        add_gate "$run_id" "ci_gate_rt_threshold" "4b" "non_allowlisted==0" "1" "FAIL" "$rt_tsv"
        append_failure_event "gate_execution_failure" "$run_id" "ci_gate_rt_threshold:missing"
        run_fail_count=$((run_fail_count + 1))
      elif [[ "$non_allowlisted" != "0" ]]; then
        add_validation "$run_id" "ci_gate_rt_threshold" "FAIL" "non_allowlisted=${non_allowlisted}" "$rt_tsv"
        add_gate "$run_id" "ci_gate_rt_threshold" "4b" "non_allowlisted==0" "1" "FAIL" "$rt_tsv"
        append_failure_event "gate_execution_failure" "$run_id" "ci_gate_rt_threshold:non_allowlisted_${non_allowlisted}"
        run_fail_count=$((run_fail_count + 1))
      else
        add_validation "$run_id" "ci_gate_rt_threshold" "PASS" "non_allowlisted=0" "$rt_tsv"
        add_gate "$run_id" "ci_gate_rt_threshold" "4b" "non_allowlisted==0" "0" "PASS" "$rt_tsv"
      fi
    else
      run_fail_count=$((run_fail_count + 1))
    fi

    ec="$(run_gate_command "$run_id" "ci_gate_docs" "5" "$docs_cmd" "${run_dir}/gate_05_docs.log")"
    [[ "$ec" -eq 0 ]] || run_fail_count=$((run_fail_count + 1))

    ec="$(run_gate_command "$run_id" "ci_gate_schema" "6" "$schema_cmd" "${run_dir}/gate_06_schema.log")"
    [[ "$ec" -eq 0 ]] || run_fail_count=$((run_fail_count + 1))

    if [[ "$run_fail_count" -gt 0 ]]; then
      OVERALL_FAIL=1
    fi

    semantic_payload="$(
      {
        printf "mode=%s\n" "$MODE"
        printf "gate_order=ci_gate_build>ci_gate_smoke>ci_gate_selftest>ci_gate_rt>ci_gate_docs>ci_gate_schema\n"
        awk -F'\t' -v run="$run_id" 'NR>1 && $1==run {print $2"=" $6 ":" $5}' "$GATE_TSV"
      } | sed '/^$/d'
    )"
  fi

  signature="$(hash_text "$semantic_payload")"
  row_payload="$(awk -F'\t' -v run="$run_id" 'NR>1 && $1==run {print $2"=" $3}' "$VALIDATION_TSV")"
  row_signature="$(hash_text "$row_payload")"

  if [[ -z "$baseline_signature" ]]; then
    baseline_signature="$signature"
  fi
  if [[ -z "$baseline_row_signature" ]]; then
    baseline_row_signature="$row_signature"
  fi

  signature_match=1
  row_match=1
  if [[ "$signature" != "$baseline_signature" ]]; then
    signature_match=0
    signature_drift_count=$((signature_drift_count + 1))
    append_failure_event "deterministic_replay_divergence" "$run_id" "signature_mismatch"
    OVERALL_FAIL=1
  fi
  if [[ "$row_signature" != "$baseline_row_signature" ]]; then
    row_match=0
    row_drift_count=$((row_drift_count + 1))
    append_failure_event "deterministic_replay_row_drift" "$run_id" "row_signature_mismatch"
    OVERALL_FAIL=1
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_id")" \
    "$(sanitize_tsv_field "$signature")" \
    "$(sanitize_tsv_field "$baseline_signature")" \
    "$signature_match" \
    "$(sanitize_tsv_field "$row_signature")" \
    "$(sanitize_tsv_field "$baseline_row_signature")" \
    "$row_match" \
    >> "$REPLAY_TSV"
done

contract_failures="$(count_failure deterministic_contract_failure)"
runtime_failures="$(count_failure runtime_execution_failure)"
missing_artifacts="$(count_failure missing_result_artifact)"
gate_failures="$(count_failure gate_execution_failure)"
replay_divergence="$(count_failure deterministic_replay_divergence)"
replay_row_drift="$(count_failure deterministic_replay_row_drift)"

printf "deterministic_contract_failure\t%s\tnon-pass contract checks\n" "$contract_failures" >> "$FAILURE_TSV"
printf "runtime_execution_failure\t%s\ttooling/runtime command failure\n" "$runtime_failures" >> "$FAILURE_TSV"
printf "missing_result_artifact\t%s\tmissing required lane inputs/artifacts\n" "$missing_artifacts" >> "$FAILURE_TSV"
printf "gate_execution_failure\t%s\texecute-suite gate failure\n" "$gate_failures" >> "$FAILURE_TSV"
printf "deterministic_replay_divergence\t%s\treplay signature mismatch\n" "$replay_divergence" >> "$FAILURE_TSV"
printf "deterministic_replay_row_drift\t%s\treplay semantic row mismatch\n" "$replay_row_drift" >> "$FAILURE_TSV"

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  if [[ "$MODE" == "execute_suite" ]]; then
    add_status "BL042-C3-001_gate_order_execution" "PASS" "build>smoke>selftest>rt>docs>schema" "$GATE_TSV"
    add_status "BL042-C3-002_gate_result_schema" "PASS" "machine_readable_gate_results_emitted" "$GATE_TSV"
  else
    add_status "BL042-B1-001_contract_surface" "PASS" "contract_patterns_verified" "$VALIDATION_TSV"
  fi
  add_status "BL042-B1-004_replay_hash_stability" "PASS" "signature_divergence=${replay_divergence} row_drift=${replay_row_drift}" "$REPLAY_TSV"
  add_status "BL042-B1-007_execution_mode_contract" "PASS" "mode=${MODE}" "$STATUS_TSV"
  add_status "BL042-B1-009_soak_summary" "PASS" "result=PASS runs=${RUNS}" "$STATUS_TSV"
  add_status "lane_result" "PASS" "multi_run_replay_pass" "$STATUS_TSV"
  exit 0
fi

if [[ "$MODE" == "execute_suite" ]]; then
  add_status "BL042-C3-001_gate_order_execution" "FAIL" "one_or_more_execute_suite_gates_failed" "$GATE_TSV"
else
  add_status "BL042-B1-001_contract_surface" "FAIL" "contract_checks_failed" "$VALIDATION_TSV"
fi
add_status "BL042-B1-004_replay_hash_stability" "FAIL" "signature_divergence=${replay_divergence} row_drift=${replay_row_drift}" "$REPLAY_TSV"
add_status "BL042-B1-007_execution_mode_contract" "PASS" "mode=${MODE}" "$STATUS_TSV"
add_status "BL042-B1-009_soak_summary" "FAIL" "result=FAIL runs=${RUNS}" "$STATUS_TSV"
add_status "lane_result" "FAIL" "multi_run_replay_fail" "$STATUS_TSV"
exit 1
