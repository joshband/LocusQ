#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

DEFAULT_OUT_DIR="$ROOT_DIR/TestEvidence/bl033_headphone_core_${TIMESTAMP}"
OUT_DIR="${BL033_OUT_DIR:-$DEFAULT_OUT_DIR}"
SCENARIO_PATH="${BL033_SCENARIO_PATH:-$ROOT_DIR/qa/scenarios/locusq_bl033_headphone_core_suite.json}"
QA_BIN="${BL033_QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL033_QA_BIN_FALLBACK:-$ROOT_DIR/build_local/locusq_qa_artefacts/locusq_qa}"
fi

RUNBOOK_PATH="$ROOT_DIR/Documentation/backlog/bl-033-headphone-calibration-core.md"
QA_DOC_PATH="$ROOT_DIR/Documentation/testing/bl-033-headphone-core-qa.md"
TRACE_DOC_PATH="$ROOT_DIR/Documentation/implementation-traceability.md"
SCRIPT_PATH="$ROOT_DIR/scripts/qa-bl033-headphone-core-lane-mac.sh"

RUN_MODE="contract_only"
SKIP_BUILD="${BL033_SKIP_BUILD:-0}"
RUNS="${BL033_RUNS:-1}"

usage() {
  cat <<USAGE
Usage: ./scripts/qa-bl033-headphone-core-lane-mac.sh [options]

Options:
  --out-dir <path>     Artifact output directory (overrides BL033_OUT_DIR).
  --scenario <path>    Scenario suite path (overrides BL033_SCENARIO_PATH).
  --qa-bin <path>      QA runner path (overrides BL033_QA_BIN).
  --runs <N>           Number of deterministic lane runs (default: 1).
  --contract-only      Validate contract scaffolding only (default).
  --execute-suite      Run build + suite execution in addition to contract checks.
  --skip-build         Skip build step during --execute-suite mode.
  --help               Show this help.

Exit codes:
  0  All enabled checks pass.
  1  One or more checks fail.
  2  Usage/configuration error.
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
    --scenario)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --scenario requires a path" >&2
        usage >&2
        exit 2
      fi
      SCENARIO_PATH="$2"
      shift 2
      ;;
    --qa-bin)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --qa-bin requires a path" >&2
        usage >&2
        exit 2
      fi
      QA_BIN="$2"
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
    --contract-only)
      RUN_MODE="contract_only"
      shift
      ;;
    --execute-suite)
      RUN_MODE="execute_suite"
      shift
      ;;
    --skip-build)
      SKIP_BUILD="1"
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
  exit 2
fi

if [[ "$RUNS" -gt 1 ]]; then
  mkdir -p "$OUT_DIR"

  STATUS_TSV="$OUT_DIR/status.tsv"
  QA_LANE_LOG="$OUT_DIR/qa_lane.log"
  VALIDATION_MATRIX_TSV="$OUT_DIR/validation_matrix.tsv"
  REPLAY_HASHES_TSV="$OUT_DIR/replay_hashes.tsv"
  ACCEPTANCE_PARITY_TSV="$OUT_DIR/acceptance_parity.tsv"
  TAXONOMY_TABLE_TSV="$OUT_DIR/taxonomy_table.tsv"

  printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
  printf "run_id\trun_dir\texit_code\tlane_result\tstatus_fail_count\tscenario_status\tdiagnostics_check\tartifact_schema_check\tcombined_signature\tsignature_match\trow_match\tmissing_artifacts\trun_log\n" >"$VALIDATION_MATRIX_TSV"
  printf "run_id\tstatus_semantic_sha256\tscenario_result_sha256\tacceptance_parity_sha256\ttaxonomy_sha256\tcombined_signature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" >"$REPLAY_HASHES_TSV"
  printf "failure_class\tcount\tdetail\n" >"$TAXONOMY_TABLE_TSV"

  exec 3>&1
  exec >>"$QA_LANE_LOG" 2>&1

  echo "BL-033 multi-run lane start: $DOC_TS"
  echo "runs=$RUNS"
  echo "out_dir=$OUT_DIR"
  echo "scenario=$SCENARIO_PATH"
  echo "qa_bin=$QA_BIN"
  echo "run_mode=$RUN_MODE"

  if ! command -v jq >/dev/null 2>&1; then
    printf "tool_jq\tFAIL\tmissing_command\t\n" >>"$STATUS_TSV"
    printf "lane_result\tFAIL\tmissing_jq_required_for_multi_run\t%s\n" "$STATUS_TSV" >>"$STATUS_TSV"
    printf "deterministic_contract_failure\t1\tjq missing for multi-run aggregation\n" >>"$TAXONOMY_TABLE_TSV"
    printf "artifact_dir=%s\n" "$OUT_DIR" >&3
    exit 1
  fi
  if ! command -v shasum >/dev/null 2>&1; then
    printf "tool_shasum\tFAIL\tmissing_command\t\n" >>"$STATUS_TSV"
    printf "lane_result\tFAIL\tmissing_shasum_required_for_multi_run\t%s\n" "$STATUS_TSV" >>"$STATUS_TSV"
    printf "deterministic_contract_failure\t1\tshasum missing for multi-run aggregation\n" >>"$TAXONOMY_TABLE_TSV"
    printf "artifact_dir=%s\n" "$OUT_DIR" >&3
    exit 1
  fi

  max_signature_divergence=0
  max_row_drift=0
  if [[ -f "$SCENARIO_PATH" ]]; then
    max_signature_divergence="$(jq -r '.bl033_contract_checks.replay_contract.max_signature_divergence // 0' "$SCENARIO_PATH")"
    max_row_drift="$(jq -r '.bl033_contract_checks.replay_contract.max_row_drift // 0' "$SCENARIO_PATH")"
  fi
  if ! [[ "$max_signature_divergence" =~ ^[0-9]+$ ]]; then
    max_signature_divergence=0
  fi
  if ! [[ "$max_row_drift" =~ ^[0-9]+$ ]]; then
    max_row_drift=0
  fi

  hash_file() {
    local path="$1"
    if [[ -f "$path" ]]; then
      shasum -a 256 "$path" | awk '{print $1}'
    else
      printf "missing"
    fi
  }

  hash_text() {
    local text="$1"
    printf "%s" "$text" | shasum -a 256 | awk '{print $1}'
  }

  base_artifacts_tmp="$OUT_DIR/.base_artifacts.tmp"
  mode_artifacts_tmp="$OUT_DIR/.mode_artifacts.tmp"
  jq -r '.bl033_contract_checks.artifact_schema[]?' "$SCENARIO_PATH" >"$base_artifacts_tmp" 2>/dev/null || : >"$base_artifacts_tmp"
  if [[ "$RUN_MODE" == "execute_suite" ]]; then
    jq -r '.bl033_contract_checks.artifact_schema_execute_additions[]?' "$SCENARIO_PATH" >"$mode_artifacts_tmp" 2>/dev/null || : >"$mode_artifacts_tmp"
  else
    : >"$mode_artifacts_tmp"
  fi

  multi_run_failed=0
  signature_divergence_count=0
  row_drift_count=0
  diagnostics_consistency_failures=0
  artifact_schema_failures=0
  runtime_execution_failures=0
  aggregate_artifact_failures=0

  baseline_signature=""
  baseline_row_signature=""
  baseline_acceptance_parity=""
  baseline_taxonomy=""

  run_index=1
  while [[ "$run_index" -le "$RUNS" ]]; do
    run_label="$(printf "run_%02d" "$run_index")"
    run_dir="$OUT_DIR/$run_label"
    run_log="$OUT_DIR/${run_label}.log"

    cmd=("$SCRIPT_PATH" "--out-dir" "$run_dir" "--runs" "1" "--scenario" "$SCENARIO_PATH" "--qa-bin" "$QA_BIN")
    if [[ "$RUN_MODE" == "execute_suite" ]]; then
      cmd+=("--execute-suite")
    else
      cmd+=("--contract-only")
    fi
    if [[ "$SKIP_BUILD" == "1" ]]; then
      cmd+=("--skip-build")
    fi

    set +e
    "${cmd[@]}" >"$run_log" 2>&1
    run_exit=$?
    set -e

    lane_result="MISSING_STATUS"
    status_fail_count="1"
    scenario_status="MISSING_STATUS"
    diagnostics_check="MISSING_STATUS"
    artifact_schema_check="MISSING_STATUS"

    run_status_tsv="$run_dir/status.tsv"
    run_result_json="$run_dir/scenario_result.json"
    run_acceptance_tsv="$run_dir/acceptance_parity.tsv"
    run_taxonomy_tsv="$run_dir/taxonomy_table.tsv"

    if [[ -f "$run_status_tsv" ]]; then
      lane_result="$(awk -F'\t' '$1=="lane_result"{value=$2} END{if(value=="") value="UNKNOWN"; print value}' "$run_status_tsv")"
      status_fail_count="$(awk -F'\t' 'NR>1 && $2=="FAIL"{count++} END{print count+0}' "$run_status_tsv")"
      scenario_status="$(awk -F'\t' '$1=="scenario_status"{value=$2} END{if(value=="") value="NA"; print value}' "$run_status_tsv")"
      diagnostics_check="$(awk -F'\t' '$1=="BL033-D2-001_diagnostics_consistency"{value=$2} END{if(value=="") value="MISSING"; print value}' "$run_status_tsv")"
      artifact_schema_check="$(awk -F'\t' '$1=="BL033-D2-003_artifact_schema_complete"{value=$2} END{if(value=="") value="MISSING"; print value}' "$run_status_tsv")"
    fi

    status_semantic_sha256="missing"
    if [[ -f "$run_status_tsv" ]]; then
      status_semantic_sha256="$(awk -F'\t' 'NR>1 {print $1 "\t" $2}' "$run_status_tsv" | shasum -a 256 | awk '{print $1}')"
    fi
    scenario_result_sha256="$(hash_file "$run_result_json")"
    acceptance_parity_sha256="$(hash_file "$run_acceptance_tsv")"
    taxonomy_sha256="$(hash_file "$run_taxonomy_tsv")"
    combined_signature="$(hash_text "${status_semantic_sha256}|${scenario_result_sha256}|${acceptance_parity_sha256}|${taxonomy_sha256}")"
    row_signature="$(hash_text "${lane_result}|${status_fail_count}|${scenario_status}|${diagnostics_check}|${artifact_schema_check}")"

    signature_match="1"
    row_match="1"
    if [[ -z "$baseline_signature" ]]; then
      baseline_signature="$combined_signature"
      baseline_row_signature="$row_signature"
      baseline_acceptance_parity="$run_acceptance_tsv"
      baseline_taxonomy="$run_taxonomy_tsv"
      if [[ -f "$baseline_acceptance_parity" ]]; then
        cp "$baseline_acceptance_parity" "$ACCEPTANCE_PARITY_TSV"
      else
        aggregate_artifact_failures=$((aggregate_artifact_failures + 1))
        multi_run_failed=1
      fi
    else
      if [[ "$combined_signature" != "$baseline_signature" ]]; then
        signature_match="0"
        signature_divergence_count=$((signature_divergence_count + 1))
      fi
      if [[ "$row_signature" != "$baseline_row_signature" ]]; then
        row_match="0"
        row_drift_count=$((row_drift_count + 1))
      fi
    fi

    missing_artifacts=""
    run_artifact_missing_count=0
    while IFS= read -r artifact_name; do
      [[ -n "$artifact_name" ]] || continue
      if [[ ! -f "$run_dir/$artifact_name" ]]; then
        run_artifact_missing_count=$((run_artifact_missing_count + 1))
        if [[ -z "$missing_artifacts" ]]; then
          missing_artifacts="$artifact_name"
        else
          missing_artifacts="${missing_artifacts},$artifact_name"
        fi
      fi
    done <"$base_artifacts_tmp"
    while IFS= read -r artifact_name; do
      [[ -n "$artifact_name" ]] || continue
      if [[ ! -f "$run_dir/$artifact_name" ]]; then
        run_artifact_missing_count=$((run_artifact_missing_count + 1))
        if [[ -z "$missing_artifacts" ]]; then
          missing_artifacts="$artifact_name"
        else
          missing_artifacts="${missing_artifacts},$artifact_name"
        fi
      fi
    done <"$mode_artifacts_tmp"
    if [[ "$run_artifact_missing_count" -eq 0 ]]; then
      missing_artifacts="none"
    else
      artifact_schema_failures=$((artifact_schema_failures + 1))
      multi_run_failed=1
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_label" \
      "$status_semantic_sha256" \
      "$scenario_result_sha256" \
      "$acceptance_parity_sha256" \
      "$taxonomy_sha256" \
      "$combined_signature" \
      "$baseline_signature" \
      "$signature_match" \
      "$row_signature" \
      "$baseline_row_signature" \
      "$row_match" \
      >>"$REPLAY_HASHES_TSV"

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_label" \
      "$run_dir" \
      "$run_exit" \
      "$lane_result" \
      "$status_fail_count" \
      "$scenario_status" \
      "$diagnostics_check" \
      "$artifact_schema_check" \
      "$combined_signature" \
      "$signature_match" \
      "$row_match" \
      "$missing_artifacts" \
      "$run_log" \
      >>"$VALIDATION_MATRIX_TSV"

    if [[ "$run_exit" -ne 0 || "$lane_result" != "PASS" ]]; then
      multi_run_failed=1
      runtime_execution_failures=$((runtime_execution_failures + 1))
    fi
    if [[ "$diagnostics_check" != "PASS" ]]; then
      multi_run_failed=1
      diagnostics_consistency_failures=$((diagnostics_consistency_failures + 1))
    fi
    if [[ "$artifact_schema_check" != "PASS" ]]; then
      multi_run_failed=1
      artifact_schema_failures=$((artifact_schema_failures + 1))
    fi

    run_index=$((run_index + 1))
  done

  if [[ "$signature_divergence_count" -gt "$max_signature_divergence" ]]; then
    multi_run_failed=1
  fi
  if [[ "$row_drift_count" -gt "$max_row_drift" ]]; then
    multi_run_failed=1
  fi

  multi_run_artifacts_tmp="$OUT_DIR/.multi_run_artifacts.tmp"
  jq -r '.bl033_contract_checks.artifact_schema_multi_run[]?' "$SCENARIO_PATH" >"$multi_run_artifacts_tmp" 2>/dev/null || : >"$multi_run_artifacts_tmp"
  while IFS= read -r artifact_name; do
    [[ -n "$artifact_name" ]] || continue
    if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
      aggregate_artifact_failures=$((aggregate_artifact_failures + 1))
      multi_run_failed=1
    fi
  done <"$multi_run_artifacts_tmp"

  if [[ "$signature_divergence_count" -le "$max_signature_divergence" ]]; then
    printf "BL033-D2-002_replay_hash_stability\tPASS\tsignature_divergence=%s max=%s row_drift=%s max_row_drift=%s\t%s\n" \
      "$signature_divergence_count" "$max_signature_divergence" "$row_drift_count" "$max_row_drift" "$REPLAY_HASHES_TSV" >>"$STATUS_TSV"
  else
    printf "BL033-D2-002_replay_hash_stability\tFAIL\tsignature_divergence=%s max=%s row_drift=%s max_row_drift=%s\t%s\n" \
      "$signature_divergence_count" "$max_signature_divergence" "$row_drift_count" "$max_row_drift" "$REPLAY_HASHES_TSV" >>"$STATUS_TSV"
  fi

  if [[ "$diagnostics_consistency_failures" -eq 0 ]]; then
    printf "BL033-D2-001_diagnostics_consistency\tPASS\tall_runs_reported_pass\t%s\n" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  else
    printf "BL033-D2-001_diagnostics_consistency\tFAIL\truns_with_nonpass=%s\t%s\n" "$diagnostics_consistency_failures" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  fi

  total_artifact_failures=$((artifact_schema_failures + aggregate_artifact_failures))
  if [[ "$total_artifact_failures" -eq 0 ]]; then
    printf "BL033-D2-003_artifact_schema_complete\tPASS\trun_level_and_aggregate_complete\t%s\n" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  else
    printf "BL033-D2-003_artifact_schema_complete\tFAIL\tartifact_failures=%s\t%s\n" "$total_artifact_failures" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  fi

  printf "multi_run_requested\tPASS\truns=%s\t%s\n" "$RUNS" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  printf "multi_run_validation_matrix\tPASS\taggregated_runs=%s\t%s\n" "$RUNS" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  printf "multi_run_replay_hashes\tPASS\trows=%s\t%s\n" "$RUNS" "$REPLAY_HASHES_TSV" >>"$STATUS_TSV"

  printf "failure_class\tcount\tdetail\n" >"$TAXONOMY_TABLE_TSV"
  printf "runtime_execution_failure\t%s\truns with non-zero exit or non-pass lane_result\n" "$runtime_execution_failures" >>"$TAXONOMY_TABLE_TSV"
  printf "diagnostics_consistency_failure\t%s\truns with BL033-D2-001 non-pass\n" "$diagnostics_consistency_failures" >>"$TAXONOMY_TABLE_TSV"
  printf "artifact_schema_failure\t%s\tmissing required run/aggregate artifacts\n" "$total_artifact_failures" >>"$TAXONOMY_TABLE_TSV"
  printf "replay_signature_divergence\t%s\texceeds threshold=%s when greater\n" "$signature_divergence_count" "$max_signature_divergence" >>"$TAXONOMY_TABLE_TSV"
  printf "replay_row_drift\t%s\texceeds threshold=%s when greater\n" "$row_drift_count" "$max_row_drift" >>"$TAXONOMY_TABLE_TSV"
  if [[ "$multi_run_failed" -eq 0 ]]; then
    printf "none\t0\tno failures\n" >>"$TAXONOMY_TABLE_TSV"
  fi

  if [[ "$multi_run_failed" -eq 0 ]]; then
    printf "lane_result\tPASS\tall_runs_passed_with_d2_closeout_checks\t%s\n" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
    printf "artifact_dir=%s\n" "$OUT_DIR" >&3
    exit 0
  fi

  printf "lane_result\tFAIL\tone_or_more_runs_failed_or_d2_criteria_unmet\t%s\n" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
QA_LANE_LOG="$OUT_DIR/qa_lane.log"
SCENARIO_CONTRACT_LOG="$OUT_DIR/scenario_contract.log"
SCENARIO_RUN_LOG="$OUT_DIR/scenario_run.log"
SCENARIO_RESULT_LOG="$OUT_DIR/scenario_result.log"
RESULT_COPY_JSON="$OUT_DIR/scenario_result.json"
ACCEPTANCE_PARITY_TSV="$OUT_DIR/acceptance_parity.tsv"
TAXONOMY_TABLE_TSV="$OUT_DIR/taxonomy_table.tsv"
LANE_CONTRACT_MD="$OUT_DIR/lane_contract.md"
BUILD_LOG="$OUT_DIR/build.log"

printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
printf "acceptance_id\trunbook_count\tqa_doc_count\ttrace_doc_count\tscenario_count\tlane_script_count\trequired_surfaces\tmapped_check\tresult\n" >"$ACCEPTANCE_PARITY_TSV"
printf "failure_class\tcount\tdetail\n" >"$TAXONOMY_TABLE_TSV"
: >"$SCENARIO_CONTRACT_LOG"
: >"$SCENARIO_RESULT_LOG"

exec 3>&1
exec >>"$QA_LANE_LOG" 2>&1

sanitize_field() {
  local value="${1:-}"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  value="${value//$'\r'/ }"
  printf "%s" "$value"
}

log_status() {
  local check="$1"
  local result="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$check")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >>"$STATUS_TSV"
  printf "%s: %s - %s\n" "$check" "$result" "$detail"
}

failure_count=0
deterministic_contract_failure=0
runtime_execution_failure=0
missing_result_artifact=0
acceptance_parity_failure=0
diagnostics_consistency_failure=0
artifact_schema_failure=0

record_failure() {
  local failure_class="$1"
  failure_count=$((failure_count + 1))
  case "$failure_class" in
    deterministic_contract_failure)
      deterministic_contract_failure=$((deterministic_contract_failure + 1))
      ;;
    runtime_execution_failure)
      runtime_execution_failure=$((runtime_execution_failure + 1))
      ;;
    missing_result_artifact)
      missing_result_artifact=$((missing_result_artifact + 1))
      ;;
    acceptance_parity_failure)
      acceptance_parity_failure=$((acceptance_parity_failure + 1))
      ;;
  esac
}

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "PASS" "$(command -v "$cmd")" ""
  else
    log_status "tool_${cmd}" "FAIL" "missing_command" ""
    record_failure deterministic_contract_failure
  fi
}

acceptance_to_check() {
  local acceptance_id="$1"
  case "$acceptance_id" in
    BL033-D1-001)
      printf "BL033-D1-001_contract_schema"
      ;;
    BL033-D1-002)
      printf "BL033-D1-002_diagnostics_fields"
      ;;
    BL033-D1-003)
      printf "BL033-D1-003_artifact_schema"
      ;;
    BL033-D1-004)
      printf "BL033-D1-004_acceptance_parity"
      ;;
    BL033-D1-005)
      printf "BL033-D1-005_lane_thresholds"
      ;;
    BL033-D1-006)
      printf "BL033-D1-006_execution_mode"
      ;;
    BL033-D2-001)
      printf "BL033-D2-001_diagnostics_consistency"
      ;;
    BL033-D2-002)
      printf "BL033-D2-002_replay_hash_stability"
      ;;
    BL033-D2-003)
      printf "BL033-D2-003_artifact_schema_complete"
      ;;
    *)
      printf "UNMAPPED"
      ;;
  esac
}

echo "BL-033 headphone core lane start: $DOC_TS"
echo "out_dir=$OUT_DIR"
echo "scenario=$SCENARIO_PATH"
echo "qa_bin=$QA_BIN"
echo "run_mode=$RUN_MODE"

require_cmd jq
require_cmd python3
require_cmd rg

if [[ ! -f "$SCENARIO_PATH" ]]; then
  log_status "scenario_file" "FAIL" "missing=$SCENARIO_PATH" "$SCENARIO_PATH"
  record_failure deterministic_contract_failure
fi
if [[ ! -f "$RUNBOOK_PATH" ]]; then
  log_status "runbook_file" "FAIL" "missing=$RUNBOOK_PATH" "$RUNBOOK_PATH"
  record_failure deterministic_contract_failure
fi
if [[ ! -f "$QA_DOC_PATH" ]]; then
  log_status "qa_doc_file" "FAIL" "missing=$QA_DOC_PATH" "$QA_DOC_PATH"
  record_failure deterministic_contract_failure
fi
if [[ ! -f "$TRACE_DOC_PATH" ]]; then
  log_status "trace_doc_file" "FAIL" "missing=$TRACE_DOC_PATH" "$TRACE_DOC_PATH"
  record_failure deterministic_contract_failure
fi

SCENARIO_ID=""
THRESHOLD_SUITE_STATUS="PASS"
THRESHOLD_MAX_WARNINGS="0"
ACCEPTANCE_IDS=()
D2_ACCEPTANCE_IDS=()

if [[ "$failure_count" -eq 0 ]]; then
  SCENARIO_ID="$(jq -r '.id // empty' "$SCENARIO_PATH")"
  if [[ -z "$SCENARIO_ID" ]]; then
    log_status "BL033-D1-001_contract_schema" "FAIL" "missing_scenario_id" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  else
    log_status "BL033-D1-001_contract_schema" "PASS" "scenario_id=$SCENARIO_ID" "$SCENARIO_PATH"
  fi

  while IFS= read -r acceptance_id; do
    [[ -n "$acceptance_id" ]] || continue
    ACCEPTANCE_IDS+=("$acceptance_id")
  done < <(jq -r '.bl033_contract_checks.acceptance_ids[]?.id // empty' "$SCENARIO_PATH")
  if [[ "${#ACCEPTANCE_IDS[@]}" -ge 6 ]]; then
    log_status "BL033-D1-001_acceptance_ids" "PASS" "count=${#ACCEPTANCE_IDS[@]}" "$SCENARIO_PATH"
  else
    log_status "BL033-D1-001_acceptance_ids" "FAIL" "expected>=6 actual=${#ACCEPTANCE_IDS[@]}" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  fi

  required_diagnostics_count="$(jq -r '(.bl033_contract_checks.required_diagnostics_fields // []) | length' "$SCENARIO_PATH")"
  if [[ "$required_diagnostics_count" -ge 6 ]]; then
    log_status "BL033-D1-002_diagnostics_fields" "PASS" "fields=$required_diagnostics_count" "$SCENARIO_PATH"
  else
    log_status "BL033-D1-002_diagnostics_fields" "FAIL" "required>=6 actual=$required_diagnostics_count" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  fi

  artifact_schema_count="$(jq -r '(.bl033_contract_checks.artifact_schema // []) | length' "$SCENARIO_PATH")"
  if [[ "$artifact_schema_count" -ge 6 ]]; then
    log_status "BL033-D1-003_artifact_schema" "PASS" "artifacts=$artifact_schema_count" "$SCENARIO_PATH"
  else
    log_status "BL033-D1-003_artifact_schema" "FAIL" "required>=6 actual=$artifact_schema_count" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  fi

  THRESHOLD_SUITE_STATUS="$(jq -r '.bl033_contract_checks.thresholds.suite_status // "PASS"' "$SCENARIO_PATH")"
  THRESHOLD_MAX_WARNINGS="$(jq -r '.bl033_contract_checks.thresholds.max_warnings // 0' "$SCENARIO_PATH")"
  if [[ -n "$THRESHOLD_SUITE_STATUS" ]] && [[ "$THRESHOLD_MAX_WARNINGS" =~ ^[0-9]+$ ]]; then
    log_status "BL033-D1-005_lane_thresholds" "PASS" "suite_status=$THRESHOLD_SUITE_STATUS max_warnings=$THRESHOLD_MAX_WARNINGS" "$SCENARIO_PATH"
  else
    log_status "BL033-D1-005_lane_thresholds" "FAIL" "invalid_thresholds" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  fi

  while IFS= read -r acceptance_id; do
    [[ -n "$acceptance_id" ]] || continue
    D2_ACCEPTANCE_IDS+=("$acceptance_id")
    ACCEPTANCE_IDS+=("$acceptance_id")
  done < <(jq -r '.bl033_contract_checks.d2_acceptance_ids[]?.id // empty' "$SCENARIO_PATH")
  if [[ "${#D2_ACCEPTANCE_IDS[@]}" -ge 3 ]]; then
    log_status "BL033-D2-000_acceptance_ids" "PASS" "count=${#D2_ACCEPTANCE_IDS[@]}" "$SCENARIO_PATH"
  else
    log_status "BL033-D2-000_acceptance_ids" "FAIL" "expected>=3 actual=${#D2_ACCEPTANCE_IDS[@]}" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  fi

  diagnostics_contract_ok=1
  diagnostics_contract_details=""

  diag_pair_count=0
  while IFS=$'\t' read -r requested_field active_field; do
    [[ -n "$requested_field" && -n "$active_field" ]] || continue
    diag_pair_count=$((diag_pair_count + 1))
    if ! jq -e --arg field "$requested_field" '(.bl033_contract_checks.required_diagnostics_fields // []) | index($field) != null' "$SCENARIO_PATH" >/dev/null; then
      diagnostics_contract_ok=0
      diagnostics_contract_details="${diagnostics_contract_details}missing_requested:${requested_field};"
    fi
    if ! jq -e --arg field "$active_field" '(.bl033_contract_checks.required_diagnostics_fields // []) | index($field) != null' "$SCENARIO_PATH" >/dev/null; then
      diagnostics_contract_ok=0
      diagnostics_contract_details="${diagnostics_contract_details}missing_active:${active_field};"
    fi
    requested_root="${requested_field%Requested}"
    active_root="${active_field%Active}"
    if [[ "$requested_root" != "$active_root" ]]; then
      diagnostics_contract_ok=0
      diagnostics_contract_details="${diagnostics_contract_details}pair_mismatch:${requested_field}/${active_field};"
    fi
  done < <(jq -r '.bl033_contract_checks.diagnostics_consistency_contract.requested_active_pairs[]? | [.requested, .active] | @tsv' "$SCENARIO_PATH")

  if [[ "$diag_pair_count" -lt 1 ]]; then
    diagnostics_contract_ok=0
    diagnostics_contract_details="${diagnostics_contract_details}missing_requested_active_pairs;"
  fi

  stage_field="$(jq -r '.bl033_contract_checks.diagnostics_consistency_contract.stage_field // empty' "$SCENARIO_PATH")"
  fallback_field="$(jq -r '.bl033_contract_checks.diagnostics_consistency_contract.fallback_field // empty' "$SCENARIO_PATH")"
  if [[ -z "$stage_field" ]]; then
    diagnostics_contract_ok=0
    diagnostics_contract_details="${diagnostics_contract_details}missing_stage_field;"
  elif ! jq -e --arg field "$stage_field" '(.bl033_contract_checks.required_diagnostics_fields // []) | index($field) != null' "$SCENARIO_PATH" >/dev/null; then
    diagnostics_contract_ok=0
    diagnostics_contract_details="${diagnostics_contract_details}missing_stage_in_required_fields:${stage_field};"
  fi
  if [[ -z "$fallback_field" ]]; then
    diagnostics_contract_ok=0
    diagnostics_contract_details="${diagnostics_contract_details}missing_fallback_field;"
  elif ! jq -e --arg field "$fallback_field" '(.bl033_contract_checks.required_diagnostics_fields // []) | index($field) != null' "$SCENARIO_PATH" >/dev/null; then
    diagnostics_contract_ok=0
    diagnostics_contract_details="${diagnostics_contract_details}missing_fallback_in_required_fields:${fallback_field};"
  fi

  if [[ "$diagnostics_contract_ok" -eq 1 ]]; then
    log_status "BL033-D2-001_diagnostics_consistency" "PASS" "requested_active_pairs=${diag_pair_count};stage=${stage_field};fallback=${fallback_field}" "$SCENARIO_PATH"
  else
    log_status "BL033-D2-001_diagnostics_consistency" "FAIL" "$diagnostics_contract_details" "$SCENARIO_PATH"
    diagnostics_consistency_failure=$((diagnostics_consistency_failure + 1))
    record_failure deterministic_contract_failure
  fi

  {
    printf "scenario_id=%s\n" "$SCENARIO_ID"
    printf "acceptance_ids=%s\n" "$(IFS=, ; echo "${ACCEPTANCE_IDS[*]}")"
    printf "d2_acceptance_ids=%s\n" "$(IFS=, ; echo "${D2_ACCEPTANCE_IDS[*]}")"
    printf "required_diagnostics_fields=%s\n" "$(jq -cr '.bl033_contract_checks.required_diagnostics_fields // []' "$SCENARIO_PATH")"
    printf "diagnostics_consistency_contract=%s\n" "$(jq -cr '.bl033_contract_checks.diagnostics_consistency_contract // {}' "$SCENARIO_PATH")"
    printf "artifact_schema=%s\n" "$(jq -cr '.bl033_contract_checks.artifact_schema // []' "$SCENARIO_PATH")"
    printf "threshold_suite_status=%s\n" "$THRESHOLD_SUITE_STATUS"
    printf "threshold_max_warnings=%s\n" "$THRESHOLD_MAX_WARNINGS"
  } >>"$SCENARIO_CONTRACT_LOG"
fi

if [[ "$failure_count" -eq 0 ]]; then
  for acceptance_id in "${ACCEPTANCE_IDS[@]}"; do
    [[ -n "$acceptance_id" ]] || continue
    mapped_check="$(acceptance_to_check "$acceptance_id")"
    runbook_count="$( { rg -o --fixed-strings "$acceptance_id" "$RUNBOOK_PATH" 2>/dev/null || true; } | wc -l | tr -d ' ' )"
    qa_doc_count="$( { rg -o --fixed-strings "$acceptance_id" "$QA_DOC_PATH" 2>/dev/null || true; } | wc -l | tr -d ' ' )"
    trace_doc_count="$( { rg -o --fixed-strings "$acceptance_id" "$TRACE_DOC_PATH" 2>/dev/null || true; } | wc -l | tr -d ' ' )"
    scenario_count="$( { rg -o --fixed-strings "$acceptance_id" "$SCENARIO_PATH" 2>/dev/null || true; } | wc -l | tr -d ' ' )"
    lane_script_count="$( { rg -o --fixed-strings "$acceptance_id" "$SCRIPT_PATH" 2>/dev/null || true; } | wc -l | tr -d ' ' )"

    parity_result="PASS"
    required_surfaces="runbook,qa_doc,trace_doc,scenario,lane_script"
    require_runbook=1
    if [[ "$acceptance_id" == BL033-D2-* ]]; then
      required_surfaces="qa_doc,trace_doc,scenario,lane_script"
      require_runbook=0
    fi

    if [[ "$qa_doc_count" -eq 0 || "$trace_doc_count" -eq 0 || "$scenario_count" -eq 0 || "$lane_script_count" -eq 0 || "$mapped_check" == "UNMAPPED" ]]; then
      parity_result="FAIL"
      record_failure acceptance_parity_failure
    fi
    if [[ "$require_runbook" -eq 1 && "$runbook_count" -eq 0 ]]; then
      parity_result="FAIL"
      record_failure acceptance_parity_failure
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$(sanitize_field "$acceptance_id")" \
      "$runbook_count" \
      "$qa_doc_count" \
      "$trace_doc_count" \
      "$scenario_count" \
      "$lane_script_count" \
      "$required_surfaces" \
      "$(sanitize_field "$mapped_check")" \
      "$parity_result" \
      >>"$ACCEPTANCE_PARITY_TSV"
  done
fi

parity_fail_count="$(awk -F'\t' 'NR>1 && $9=="FAIL" {count++} END {print count+0}' "$ACCEPTANCE_PARITY_TSV")"
if [[ "$parity_fail_count" -eq 0 ]]; then
  log_status "BL033-D1-004_acceptance_parity" "PASS" "all_acceptance_ids_mapped" "$ACCEPTANCE_PARITY_TSV"
else
  log_status "BL033-D1-004_acceptance_parity" "FAIL" "parity_failures=$parity_fail_count" "$ACCEPTANCE_PARITY_TSV"
fi

if [[ "$RUN_MODE" == "execute_suite" ]]; then
  if [[ ! -x "$QA_BIN" ]]; then
    log_status "BL033-D1-006_execution_mode" "FAIL" "qa_bin_missing_or_not_executable" "$QA_BIN"
    record_failure runtime_execution_failure
  fi

  if [[ "$failure_count" -eq 0 ]]; then
    if [[ "$SKIP_BUILD" == "1" ]]; then
      log_status "build_targets" "PASS" "skipped_by_flag" "$BUILD_LOG"
    else
      set +e
      cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8 >"$BUILD_LOG" 2>&1
      build_exit=$?
      set -e
      if [[ "$build_exit" -eq 0 ]]; then
        log_status "build_targets" "PASS" "cmake_build_exit=0" "$BUILD_LOG"
      else
        log_status "build_targets" "FAIL" "cmake_build_exit=$build_exit" "$BUILD_LOG"
        record_failure runtime_execution_failure
      fi
    fi
  fi

  RUN_ARTIFACT_DIR="$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID"
  if [[ "$failure_count" -eq 0 ]]; then
    rm -rf "$RUN_ARTIFACT_DIR"
    set +e
    "$QA_BIN" --spatial "$SCENARIO_PATH" >"$SCENARIO_RUN_LOG" 2>&1
    qa_exit=$?
    set -e
    if [[ "$qa_exit" -eq 0 ]]; then
      log_status "scenario_exec" "PASS" "qa_runner_exit=0" "$SCENARIO_RUN_LOG"
    else
      log_status "scenario_exec" "FAIL" "qa_runner_exit=$qa_exit" "$SCENARIO_RUN_LOG"
      record_failure runtime_execution_failure
    fi
  fi

  RESULT_JSON=""
  if [[ "$failure_count" -eq 0 ]]; then
    result_candidates=(
      "$RUN_ARTIFACT_DIR/suite_result.json"
      "$RUN_ARTIFACT_DIR/result.json"
      "$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID/suite_result.json"
      "$ROOT_DIR/qa_output/locusq_spatial/suite_result.json"
      "$ROOT_DIR/qa_output/suite_result.json"
    )
    for candidate in "${result_candidates[@]}"; do
      if [[ -f "$candidate" ]]; then
        RESULT_JSON="$candidate"
        break
      fi
    done

    if [[ -n "$RESULT_JSON" ]]; then
      cp "$RESULT_JSON" "$RESULT_COPY_JSON"
      result_status="$(jq -r '.status // "UNKNOWN"' "$RESULT_COPY_JSON")"
      warnings_count="$(jq -r '.summary.warned // ((.warnings // []) | length) // 0' "$RESULT_COPY_JSON")"
      if [[ "$result_status" == "$THRESHOLD_SUITE_STATUS" ]] && [[ "$warnings_count" -le "$THRESHOLD_MAX_WARNINGS" ]]; then
        log_status "scenario_status" "PASS" "status=$result_status warnings=$warnings_count" "$RESULT_COPY_JSON"
      else
        log_status "scenario_status" "FAIL" "status=$result_status warnings=$warnings_count" "$RESULT_COPY_JSON"
        record_failure deterministic_contract_failure
      fi

      {
        printf "scenario_id=%s\n" "$SCENARIO_ID"
        printf "result_status=%s\n" "$result_status"
        printf "warnings=%s\n" "$warnings_count"
        printf "result_json=%s\n" "$RESULT_COPY_JSON"
      } >>"$SCENARIO_RESULT_LOG"
    else
      log_status "scenario_status" "FAIL" "missing_result_json" "$SCENARIO_RUN_LOG"
      record_failure missing_result_artifact
    fi
  fi

  if [[ "$failure_count" -eq 0 ]]; then
    log_status "BL033-D1-006_execution_mode" "PASS" "mode=execute_suite" "$SCENARIO_RESULT_LOG"
  else
    log_status "BL033-D1-006_execution_mode" "FAIL" "mode=execute_suite failures_present" "$SCENARIO_RESULT_LOG"
  fi
else
  log_status "build_targets" "SKIP" "mode=contract_only" "$BUILD_LOG"
  log_status "scenario_exec" "SKIP" "mode=contract_only" "$SCENARIO_RUN_LOG"
  log_status "scenario_status" "SKIP" "mode=contract_only" "$SCENARIO_RESULT_LOG"
  log_status "BL033-D1-006_execution_mode" "PASS" "mode=contract_only" "$SCENARIO_RESULT_LOG"
fi

log_status "BL033-D2-002_replay_hash_stability" "SKIP" "single_run_mode_use_--runs_for_replay_determinism" "$STATUS_TSV"

artifact_missing_count=0
artifact_missing_list=""
while IFS= read -r artifact_name; do
  [[ -n "$artifact_name" ]] || continue
  if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
    artifact_missing_count=$((artifact_missing_count + 1))
    if [[ -z "$artifact_missing_list" ]]; then
      artifact_missing_list="$artifact_name"
    else
      artifact_missing_list="${artifact_missing_list},$artifact_name"
    fi
  fi
done < <(jq -r '.bl033_contract_checks.artifact_schema[]?' "$SCENARIO_PATH")
if [[ "$RUN_MODE" == "execute_suite" ]]; then
  while IFS= read -r artifact_name; do
    [[ -n "$artifact_name" ]] || continue
    if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
      artifact_missing_count=$((artifact_missing_count + 1))
      if [[ -z "$artifact_missing_list" ]]; then
        artifact_missing_list="$artifact_name"
      else
        artifact_missing_list="${artifact_missing_list},$artifact_name"
      fi
    fi
  done < <(jq -r '.bl033_contract_checks.artifact_schema_execute_additions[]?' "$SCENARIO_PATH")
fi
if [[ "$artifact_missing_count" -eq 0 ]]; then
  log_status "BL033-D2-003_artifact_schema_complete" "PASS" "all_required_artifacts_present" "$OUT_DIR"
else
  log_status "BL033-D2-003_artifact_schema_complete" "FAIL" "missing_artifacts=${artifact_missing_list}" "$OUT_DIR"
  artifact_schema_failure=$((artifact_schema_failure + 1))
  record_failure deterministic_contract_failure
fi

printf "deterministic_contract_failure\t%s\tcontract/schema/threshold mismatches\n" "$deterministic_contract_failure" >>"$TAXONOMY_TABLE_TSV"
printf "runtime_execution_failure\t%s\tbuild or qa runner non-zero exit\n" "$runtime_execution_failure" >>"$TAXONOMY_TABLE_TSV"
printf "missing_result_artifact\t%s\tqa execution succeeded but expected result artifact missing\n" "$missing_result_artifact" >>"$TAXONOMY_TABLE_TSV"
printf "acceptance_parity_failure\t%s\tacceptance IDs missing in one or more contract surfaces\n" "$acceptance_parity_failure" >>"$TAXONOMY_TABLE_TSV"
printf "diagnostics_consistency_failure\t%s\tdiagnostics requested/active/stage/fallback consistency mismatch\n" "$diagnostics_consistency_failure" >>"$TAXONOMY_TABLE_TSV"
printf "artifact_schema_failure\t%s\trequired artifact missing from lane output\n" "$artifact_schema_failure" >>"$TAXONOMY_TABLE_TSV"
if [[ "$failure_count" -eq 0 ]]; then
  printf "none\t0\tno failures\n" >>"$TAXONOMY_TABLE_TSV"
fi

cat >"$LANE_CONTRACT_MD" <<EOF_DOC
Title: BL-033 Headphone Core Lane Contract Snapshot
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-033 Headphone Core Lane Contract Snapshot

## Lane Inputs
- Scenario: ${SCENARIO_PATH}
- QA doc: ${QA_DOC_PATH}
- Runbook: ${RUNBOOK_PATH}
- Traceability: ${TRACE_DOC_PATH}

## Lane Mode
- run_mode: ${RUN_MODE}
- skip_build: ${SKIP_BUILD}

## Artifact Set
1. status.tsv
2. qa_lane.log
3. scenario_contract.log
4. scenario_result.log
5. acceptance_parity.tsv
6. taxonomy_table.tsv

## Verdict
- failure_count: ${failure_count}
- status.tsv path: ${STATUS_TSV}
EOF_DOC

if [[ "$failure_count" -eq 0 ]]; then
  log_status "lane_result" "PASS" "all_enabled_checks_passed" "$STATUS_TSV"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 0
fi

log_status "lane_result" "FAIL" "failure_count=$failure_count" "$STATUS_TSV"
printf "artifact_dir=%s\n" "$OUT_DIR" >&3
exit 1
