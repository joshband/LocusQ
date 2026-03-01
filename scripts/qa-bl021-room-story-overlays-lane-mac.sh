#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

DEFAULT_OUT_DIR="$ROOT_DIR/TestEvidence/bl021_room_story_lane_${TIMESTAMP}"
OUT_DIR="${BL021_OUT_DIR:-$DEFAULT_OUT_DIR}"
SCENARIO_PATH="${BL021_SCENARIO_PATH:-$ROOT_DIR/qa/scenarios/locusq_bl021_room_story_suite.json}"
QA_BIN="${BL021_QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL021_QA_BIN_FALLBACK:-$ROOT_DIR/build_local/locusq_qa_artefacts/locusq_qa}"
fi

SCRIPT_PATH="$ROOT_DIR/scripts/qa-bl021-room-story-overlays-lane-mac.sh"

RUN_MODE="contract_only"
RUNS="${BL021_RUNS:-1}"
SKIP_BUILD="${BL021_SKIP_BUILD:-0}"

usage() {
  cat <<USAGE
Usage: ./scripts/qa-bl021-room-story-overlays-lane-mac.sh [options]

Options:
  --out-dir <path>     Artifact output directory (overrides BL021_OUT_DIR).
  --scenario <path>    Scenario suite path (overrides BL021_SCENARIO_PATH).
  --qa-bin <path>      QA runner path (overrides BL021_QA_BIN).
  --runs <N>           Deterministic run count, integer >= 1 (default: 1).
  --contract-only      Validate BL-021 contract lane only (default).
  --execute-suite      Run build + suite in addition to contract checks.
  --skip-build         Skip build step during --execute-suite mode.
  --help               Show usage.

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
  FAILURE_TAXONOMY_TSV="$OUT_DIR/failure_taxonomy.tsv"
  SOAK_SUMMARY_TSV="$OUT_DIR/soak_summary.tsv"

  printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
  printf "run_id\trun_dir\texit_code\tlane_result\tstatus_fail_count\tcontract_schema\ttransition_contract\tfallback_contract\tscenario_status\tcombined_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\tmissing_artifacts\trun_log\n" >"$VALIDATION_MATRIX_TSV"
  printf "run_id\tstatus_semantic_sha256\tcontract_semantic_sha256\tscenario_semantic_sha256\ttaxonomy_sha256\tcombined_signature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" >"$REPLAY_HASHES_TSV"
  printf "failure_class\tcount\tdetail\n" >"$FAILURE_TAXONOMY_TSV"
  printf "mode\truns\tsignature_divergence\tmax_signature_divergence\trow_drift\tmax_row_drift\truntime_execution_failure\tdeterministic_contract_failure\tmissing_result_artifact\ttotal_artifact_failures\tfinal_failures\tresult\tbaseline_signature\tbaseline_row_signature\n" >"$SOAK_SUMMARY_TSV"

  exec 3>&1
  exec >>"$QA_LANE_LOG" 2>&1

  echo "BL-021 multi-run lane start: $DOC_TS"
  echo "runs=$RUNS"
  echo "mode=$RUN_MODE"
  echo "scenario=$SCENARIO_PATH"

  if ! command -v jq >/dev/null 2>&1; then
    printf "BL021-B1-001_contract_schema\tFAIL\tmissing_jq\t%s\n" "$SCENARIO_PATH" >>"$STATUS_TSV"
    printf "lane_result\tFAIL\tmissing_jq_required_for_multi_run\t%s\n" "$STATUS_TSV" >>"$STATUS_TSV"
    printf "runtime_execution_failure\t1\tjq missing\n" >>"$FAILURE_TAXONOMY_TSV"
    printf "artifact_dir=%s\n" "$OUT_DIR" >&3
    exit 1
  fi
  if ! command -v shasum >/dev/null 2>&1; then
    printf "BL021-B1-001_contract_schema\tFAIL\tmissing_shasum\t%s\n" "$SCENARIO_PATH" >>"$STATUS_TSV"
    printf "lane_result\tFAIL\tmissing_shasum_required_for_multi_run\t%s\n" "$STATUS_TSV" >>"$STATUS_TSV"
    printf "runtime_execution_failure\t1\tshasum missing\n" >>"$FAILURE_TAXONOMY_TSV"
    printf "artifact_dir=%s\n" "$OUT_DIR" >&3
    exit 1
  fi

  hash_file() {
    local path="$1"
    if [[ -f "$path" ]]; then
      shasum -a 256 "$path" | awk '{print $1}'
    else
      printf "missing"
    fi
  }

  hash_semantic_tsv() {
    local path="$1"
    if [[ -f "$path" ]]; then
      awk -F'\t' 'NR>1 {print $1 "\t" $2}' "$path" | shasum -a 256 | awk '{print $1}'
    else
      printf "missing"
    fi
  }

  hash_kv_log() {
    local path="$1"
    if [[ -f "$path" ]]; then
      awk -F'=' '/=/{print $1 "=" $2}' "$path" | shasum -a 256 | awk '{print $1}'
    else
      printf "missing"
    fi
  }

  hash_text() {
    local text="$1"
    printf "%s" "$text" | shasum -a 256 | awk '{print $1}'
  }

  max_signature_divergence="$(jq -r '.bl021_contract_checks.replay_contract.max_signature_divergence // 0' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
  max_row_drift="$(jq -r '.bl021_contract_checks.replay_contract.max_row_drift // 0' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
  if ! [[ "$max_signature_divergence" =~ ^[0-9]+$ ]]; then
    max_signature_divergence=0
  fi
  if ! [[ "$max_row_drift" =~ ^[0-9]+$ ]]; then
    max_row_drift=0
  fi

  base_artifacts_tmp="$OUT_DIR/.base_artifacts.tmp"
  mode_artifacts_tmp="$OUT_DIR/.mode_artifacts.tmp"
  jq -r '.bl021_contract_checks.artifact_schema[]?' "$SCENARIO_PATH" >"$base_artifacts_tmp" 2>/dev/null || : >"$base_artifacts_tmp"
  if [[ "$RUN_MODE" == "execute_suite" ]]; then
    jq -r '.bl021_contract_checks.artifact_schema_execute_additions[]?' "$SCENARIO_PATH" >"$mode_artifacts_tmp" 2>/dev/null || : >"$mode_artifacts_tmp"
  else
    : >"$mode_artifacts_tmp"
  fi

  baseline_signature=""
  baseline_row_signature=""
  signature_divergence_count=0
  row_drift_count=0
  runtime_execution_failure_count=0
  deterministic_contract_failure_count=0
  missing_result_artifact_count=0

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

    run_status_tsv="$run_dir/status.tsv"
    run_contract_log="$run_dir/scenario_contract.log"
    run_result_log="$run_dir/scenario_result.log"
    run_taxonomy_tsv="$run_dir/failure_taxonomy.tsv"

    lane_result="MISSING"
    status_fail_count="1"
    contract_schema_result="MISSING"
    transition_result="MISSING"
    fallback_result="MISSING"
    scenario_status="MISSING"

    if [[ -f "$run_status_tsv" ]]; then
      lane_result="$(awk -F'\t' '$1=="lane_result"{value=$2} END{if(value=="") value="UNKNOWN"; print value}' "$run_status_tsv")"
      status_fail_count="$(awk -F'\t' 'NR>1 && $2=="FAIL"{count++} END{print count+0}' "$run_status_tsv")"
      contract_schema_result="$(awk -F'\t' '$1=="BL021-B1-001_contract_schema"{value=$2} END{if(value=="") value="MISSING"; print value}' "$run_status_tsv")"
      transition_result="$(awk -F'\t' '$1=="BL021-B1-002_transition_contract"{value=$2} END{if(value=="") value="MISSING"; print value}' "$run_status_tsv")"
      fallback_result="$(awk -F'\t' '$1=="BL021-B1-003_fallback_contract"{value=$2} END{if(value=="") value="MISSING"; print value}' "$run_status_tsv")"
      scenario_status="$(awk -F'\t' '$1=="scenario_status"{value=$2} END{if(value=="") value="MISSING"; print value}' "$run_status_tsv")"
    fi

    missing_artifacts="none"
    while IFS= read -r artifact_name; do
      [[ -n "$artifact_name" ]] || continue
      if [[ ! -f "$run_dir/$artifact_name" ]]; then
        if [[ "$missing_artifacts" == "none" ]]; then
          missing_artifacts="$artifact_name"
        else
          missing_artifacts="${missing_artifacts},$artifact_name"
        fi
      fi
    done <"$base_artifacts_tmp"
    while IFS= read -r artifact_name; do
      [[ -n "$artifact_name" ]] || continue
      if [[ ! -f "$run_dir/$artifact_name" ]]; then
        if [[ "$missing_artifacts" == "none" ]]; then
          missing_artifacts="$artifact_name"
        else
          missing_artifacts="${missing_artifacts},$artifact_name"
        fi
      fi
    done <"$mode_artifacts_tmp"

    status_semantic_sha256="$(hash_semantic_tsv "$run_status_tsv")"
    contract_semantic_sha256="$(hash_kv_log "$run_contract_log")"
    scenario_semantic_sha256="$(hash_kv_log "$run_result_log")"
    taxonomy_sha256="$(hash_file "$run_taxonomy_tsv")"
    combined_signature="$(hash_text "${status_semantic_sha256}|${contract_semantic_sha256}|${scenario_semantic_sha256}|${taxonomy_sha256}")"
    row_signature="$(hash_text "${lane_result}|${status_fail_count}|${contract_schema_result}|${transition_result}|${fallback_result}|${scenario_status}")"

    signature_match="1"
    row_match="1"
    if [[ -z "$baseline_signature" ]]; then
      baseline_signature="$combined_signature"
      baseline_row_signature="$row_signature"
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

    if [[ "$run_exit" -ne 0 ]]; then
      runtime_execution_failure_count=$((runtime_execution_failure_count + 1))
    fi
    if [[ "$lane_result" != "PASS" ]]; then
      deterministic_contract_failure_count=$((deterministic_contract_failure_count + 1))
    fi
    if [[ "$missing_artifacts" != "none" ]]; then
      missing_result_artifact_count=$((missing_result_artifact_count + 1))
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_label" \
      "$run_dir" \
      "$run_exit" \
      "$lane_result" \
      "$status_fail_count" \
      "$contract_schema_result" \
      "$transition_result" \
      "$fallback_result" \
      "$scenario_status" \
      "$combined_signature" \
      "$signature_match" \
      "$row_signature" \
      "$baseline_row_signature" \
      "$row_match" \
      "$missing_artifacts" \
      "$run_log" \
      >>"$VALIDATION_MATRIX_TSV"

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_label" \
      "$status_semantic_sha256" \
      "$contract_semantic_sha256" \
      "$scenario_semantic_sha256" \
      "$taxonomy_sha256" \
      "$combined_signature" \
      "$baseline_signature" \
      "$signature_match" \
      "$row_signature" \
      "$baseline_row_signature" \
      "$row_match" \
      >>"$REPLAY_HASHES_TSV"

    run_index=$((run_index + 1))
  done

  aggregate_artifact_missing=0
  while IFS= read -r artifact_name; do
    [[ -n "$artifact_name" ]] || continue
    if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
      aggregate_artifact_missing=$((aggregate_artifact_missing + 1))
    fi
  done < <(jq -r '.bl021_contract_checks.artifact_schema_multi_run[]?' "$SCENARIO_PATH" 2>/dev/null)

  if [[ "$signature_divergence_count" -le "$max_signature_divergence" ]]; then
    printf "BL021-B1-004_replay_hash_stability\tPASS\tsignature_divergence=%s max=%s row_drift=%s max_row_drift=%s\t%s\n" \
      "$signature_divergence_count" "$max_signature_divergence" "$row_drift_count" "$max_row_drift" "$REPLAY_HASHES_TSV" >>"$STATUS_TSV"
  else
    printf "BL021-B1-004_replay_hash_stability\tFAIL\tsignature_divergence=%s max=%s row_drift=%s max_row_drift=%s\t%s\n" \
      "$signature_divergence_count" "$max_signature_divergence" "$row_drift_count" "$max_row_drift" "$REPLAY_HASHES_TSV" >>"$STATUS_TSV"
  fi

  if [[ "$row_drift_count" -le "$max_row_drift" ]]; then
    printf "BL021-B1-006_hash_input_contract\tPASS\trow_drift=%s max=%s\t%s\n" \
      "$row_drift_count" "$max_row_drift" "$REPLAY_HASHES_TSV" >>"$STATUS_TSV"
  else
    printf "BL021-B1-006_hash_input_contract\tFAIL\trow_drift=%s max=%s\t%s\n" \
      "$row_drift_count" "$max_row_drift" "$REPLAY_HASHES_TSV" >>"$STATUS_TSV"
  fi

  total_artifact_failures=$((missing_result_artifact_count + aggregate_artifact_missing))
  if [[ "$total_artifact_failures" -eq 0 ]]; then
    printf "BL021-B1-005_artifact_schema_complete\tPASS\trun_and_aggregate_artifacts_present\t%s\n" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  else
    printf "BL021-B1-005_artifact_schema_complete\tFAIL\tartifact_failures=%s\t%s\n" "$total_artifact_failures" "$VALIDATION_MATRIX_TSV" >>"$STATUS_TSV"
  fi

  if [[ "$RUN_MODE" == "execute_suite" ]]; then
    printf "BL021-B1-007_execution_mode_contract\tPASS\tmode=execute_suite\t%s\n" "$STATUS_TSV" >>"$STATUS_TSV"
  else
    printf "BL021-B1-007_execution_mode_contract\tPASS\tmode=contract_only\t%s\n" "$STATUS_TSV" >>"$STATUS_TSV"
  fi

  printf "deterministic_contract_failure\t%s\tnon-pass lane_result in replay runs\n" "$deterministic_contract_failure_count" >>"$FAILURE_TAXONOMY_TSV"
  printf "runtime_execution_failure\t%s\tnon-zero run exit\n" "$runtime_execution_failure_count" >>"$FAILURE_TAXONOMY_TSV"
  printf "missing_result_artifact\t%s\tmissing required artifact(s)\n" "$total_artifact_failures" >>"$FAILURE_TAXONOMY_TSV"
  printf "deterministic_replay_divergence\t%s\treplay signature mismatch\n" "$signature_divergence_count" >>"$FAILURE_TAXONOMY_TSV"
  printf "deterministic_replay_row_drift\t%s\treplay semantic row mismatch\n" "$row_drift_count" >>"$FAILURE_TAXONOMY_TSV"

  final_failures=0
  if [[ "$signature_divergence_count" -gt "$max_signature_divergence" ]]; then
    final_failures=$((final_failures + 1))
  fi
  if [[ "$row_drift_count" -gt "$max_row_drift" ]]; then
    final_failures=$((final_failures + 1))
  fi
  if [[ "$runtime_execution_failure_count" -ne 0 ]]; then
    final_failures=$((final_failures + 1))
  fi
  if [[ "$deterministic_contract_failure_count" -ne 0 ]]; then
    final_failures=$((final_failures + 1))
  fi
  if [[ "$total_artifact_failures" -ne 0 ]]; then
    final_failures=$((final_failures + 1))
  fi

  summary_result="PASS"
  if [[ "$final_failures" -ne 0 ]]; then
    summary_result="FAIL"
  fi
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$RUN_MODE" \
    "$RUNS" \
    "$signature_divergence_count" \
    "$max_signature_divergence" \
    "$row_drift_count" \
    "$max_row_drift" \
    "$runtime_execution_failure_count" \
    "$deterministic_contract_failure_count" \
    "$missing_result_artifact_count" \
    "$total_artifact_failures" \
    "$final_failures" \
    "$summary_result" \
    "$baseline_signature" \
    "$baseline_row_signature" \
    >>"$SOAK_SUMMARY_TSV"

  if [[ "$summary_result" == "PASS" ]]; then
    printf "BL021-C2-001_soak_summary\tPASS\tresult=%s runs=%s\t%s\n" "$summary_result" "$RUNS" "$SOAK_SUMMARY_TSV" >>"$STATUS_TSV"
  else
    printf "BL021-C2-001_soak_summary\tFAIL\tresult=%s runs=%s\t%s\n" "$summary_result" "$RUNS" "$SOAK_SUMMARY_TSV" >>"$STATUS_TSV"
  fi

  if [[ "$final_failures" -eq 0 ]]; then
    printf "lane_result\tPASS\tmulti_run_replay_pass\t%s\n" "$STATUS_TSV" >>"$STATUS_TSV"
    printf "artifact_dir=%s\n" "$OUT_DIR" >&3
    exit 0
  fi

  printf "lane_result\tFAIL\tfailure_count=%s\t%s\n" "$final_failures" "$STATUS_TSV" >>"$STATUS_TSV"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
QA_LANE_LOG="$OUT_DIR/qa_lane.log"
SCENARIO_CONTRACT_LOG="$OUT_DIR/scenario_contract.log"
SCENARIO_RESULT_LOG="$OUT_DIR/scenario_result.log"
VALIDATION_MATRIX_TSV="$OUT_DIR/validation_matrix.tsv"
REPLAY_HASHES_TSV="$OUT_DIR/replay_hashes.tsv"
FAILURE_TAXONOMY_TSV="$OUT_DIR/failure_taxonomy.tsv"
BUILD_LOG="$OUT_DIR/build.log"
SCENARIO_RUN_LOG="$OUT_DIR/scenario_run.log"
RESULT_COPY_JSON="$OUT_DIR/scenario_result.json"

printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
printf "key=value\n" >"$SCENARIO_CONTRACT_LOG"
: >"$SCENARIO_RESULT_LOG"
printf "run_id\trun_dir\texit_code\tlane_result\tstatus_fail_count\tcontract_schema\ttransition_contract\tfallback_contract\tscenario_status\tcombined_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\tmissing_artifacts\trun_log\n" >"$VALIDATION_MATRIX_TSV"
printf "run_id\tstatus_semantic_sha256\tcontract_semantic_sha256\tscenario_semantic_sha256\ttaxonomy_sha256\tcombined_signature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" >"$REPLAY_HASHES_TSV"
printf "failure_class\tcount\tdetail\n" >"$FAILURE_TAXONOMY_TSV"

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

failure_count=0
deterministic_contract_failure=0
runtime_execution_failure=0
missing_result_artifact=0
deterministic_replay_divergence=0
deterministic_replay_row_drift=0

record_failure() {
  local class_name="$1"
  failure_count=$((failure_count + 1))
  case "$class_name" in
    deterministic_contract_failure)
      deterministic_contract_failure=$((deterministic_contract_failure + 1))
      ;;
    runtime_execution_failure)
      runtime_execution_failure=$((runtime_execution_failure + 1))
      ;;
    missing_result_artifact)
      missing_result_artifact=$((missing_result_artifact + 1))
      ;;
    deterministic_replay_divergence)
      deterministic_replay_divergence=$((deterministic_replay_divergence + 1))
      ;;
    deterministic_replay_row_drift)
      deterministic_replay_row_drift=$((deterministic_replay_row_drift + 1))
      ;;
  esac
}

hash_file() {
  local path="$1"
  if [[ -f "$path" ]]; then
    shasum -a 256 "$path" | awk '{print $1}'
  else
    printf "missing"
  fi
}

hash_semantic_tsv() {
  local path="$1"
  if [[ -f "$path" ]]; then
    awk -F'\t' 'NR>1 {print $1 "\t" $2}' "$path" | shasum -a 256 | awk '{print $1}'
  else
    printf "missing"
  fi
}

hash_kv_log() {
  local path="$1"
  if [[ -f "$path" ]]; then
    awk -F'=' '/=/{print $1 "=" $2}' "$path" | shasum -a 256 | awk '{print $1}'
  else
    printf "missing"
  fi
}

hash_text() {
  local text="$1"
  printf "%s" "$text" | shasum -a 256 | awk '{print $1}'
}

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "PASS" "$(command -v "$cmd")" ""
  else
    log_status "tool_${cmd}" "FAIL" "missing_command" ""
    record_failure runtime_execution_failure
  fi
}

echo "BL-021 single-run lane start: $DOC_TS"
echo "mode=$RUN_MODE"
echo "out_dir=$OUT_DIR"
echo "scenario=$SCENARIO_PATH"
echo "qa_bin=$QA_BIN"

require_cmd jq
require_cmd rg
require_cmd shasum

if [[ ! -f "$SCENARIO_PATH" ]]; then
  log_status "BL021-B1-001_contract_schema" "FAIL" "missing_scenario=$SCENARIO_PATH" "$SCENARIO_PATH"
  record_failure deterministic_contract_failure
fi

if [[ "$RUN_MODE" == "execute_suite" && ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "FAIL" "missing_or_not_executable=$QA_BIN" "$QA_BIN"
  record_failure runtime_execution_failure
fi

if [[ "$failure_count" -eq 0 ]]; then
  if jq empty "$SCENARIO_PATH" >/dev/null 2>&1; then
    log_status "BL021-B1-001_contract_schema" "PASS" "scenario_json_parseable" "$SCENARIO_PATH"
  else
    log_status "BL021-B1-001_contract_schema" "FAIL" "scenario_json_not_parseable" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  fi
fi

ACCEPTANCE_IDS=()
if [[ "$failure_count" -eq 0 ]]; then
  while IFS= read -r acceptance_id; do
    [[ -n "$acceptance_id" ]] || continue
    ACCEPTANCE_IDS+=("$acceptance_id")
  done < <(jq -r '.bl021_contract_checks.acceptance_ids[]?.id // empty' "$SCENARIO_PATH")
  if [[ "${#ACCEPTANCE_IDS[@]}" -ge 7 ]]; then
    log_status "BL021-B1-001_contract_schema" "PASS" "acceptance_ids=${#ACCEPTANCE_IDS[@]}" "$SCENARIO_PATH"
  else
    log_status "BL021-B1-001_contract_schema" "FAIL" "expected_acceptance_ids>=7 actual=${#ACCEPTANCE_IDS[@]}" "$SCENARIO_PATH"
    record_failure deterministic_contract_failure
  fi
fi

transition_states_count="$(jq -r '(.bl021_contract_checks.required_transition_states // []) | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
transition_has_fallback="$(jq -r '[.bl021_contract_checks.required_transition_states[]? | select(. == "state_fallback_safe")] | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
if [[ "$transition_states_count" -ge 6 && "$transition_has_fallback" -ge 1 ]]; then
  log_status "BL021-B1-002_transition_contract" "PASS" "state_count=$transition_states_count has_state_fallback_safe=$transition_has_fallback" "$SCENARIO_PATH"
else
  log_status "BL021-B1-002_transition_contract" "FAIL" "state_count=$transition_states_count has_state_fallback_safe=$transition_has_fallback" "$SCENARIO_PATH"
  record_failure deterministic_contract_failure
fi

fallback_taxonomy_count="$(jq -r '(.bl021_contract_checks.required_fallback_taxonomy // []) | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
fallback_has_nonfinite="$(jq -r '[.bl021_contract_checks.required_fallback_taxonomy[]? | select(. == "non_finite_payload_field")] | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
if [[ "$fallback_taxonomy_count" -ge 6 && "$fallback_has_nonfinite" -ge 1 ]]; then
  log_status "BL021-B1-003_fallback_contract" "PASS" "taxonomy_count=$fallback_taxonomy_count has_non_finite_payload_field=$fallback_has_nonfinite" "$SCENARIO_PATH"
else
  log_status "BL021-B1-003_fallback_contract" "FAIL" "taxonomy_count=$fallback_taxonomy_count has_non_finite_payload_field=$fallback_has_nonfinite" "$SCENARIO_PATH"
  record_failure deterministic_contract_failure
fi

required_taxonomy=(
  "deterministic_contract_failure"
  "runtime_execution_failure"
  "missing_result_artifact"
  "deterministic_replay_divergence"
  "deterministic_replay_row_drift"
)

taxonomy_missing=""
for class_name in "${required_taxonomy[@]}"; do
  count="$(jq -r --arg c "$class_name" '[.bl021_contract_checks.failure_taxonomy[]? | select(.failure_class == $c)] | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
  if [[ "$count" -eq 0 ]]; then
    if [[ -z "$taxonomy_missing" ]]; then
      taxonomy_missing="$class_name"
    else
      taxonomy_missing="${taxonomy_missing},${class_name}"
    fi
  fi
done
if [[ -z "$taxonomy_missing" ]]; then
  log_status "BL021-B1-008_failure_taxonomy_schema" "PASS" "required_classes_present" "$SCENARIO_PATH"
else
  log_status "BL021-B1-008_failure_taxonomy_schema" "FAIL" "missing_classes=${taxonomy_missing}" "$SCENARIO_PATH"
  record_failure deterministic_contract_failure
fi

default_mode="$(jq -r '.bl021_contract_checks.execution_contract.default_mode // empty' "$SCENARIO_PATH" 2>/dev/null || printf "")"
suite_mode="$(jq -r '.bl021_contract_checks.execution_contract.suite_mode // empty' "$SCENARIO_PATH" 2>/dev/null || printf "")"
if [[ "$default_mode" == "contract_only" && "$suite_mode" == "execute_suite" ]]; then
  log_status "BL021-B1-007_execution_mode_contract" "PASS" "default=$default_mode suite=$suite_mode" "$SCENARIO_PATH"
else
  log_status "BL021-B1-007_execution_mode_contract" "FAIL" "default=$default_mode suite=$suite_mode" "$SCENARIO_PATH"
  record_failure deterministic_contract_failure
fi

hash_include_count="$(jq -r '(.bl021_contract_checks.deterministic_hash_inputs.include // []) | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
hash_exclude_count="$(jq -r '(.bl021_contract_checks.deterministic_hash_inputs.exclude // []) | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
exclude_has_timestamp="$(jq -r '[.bl021_contract_checks.deterministic_hash_inputs.exclude[]? | select(. == "timestamp")] | length' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
if [[ "$hash_include_count" -ge 3 && "$hash_exclude_count" -ge 3 && "$exclude_has_timestamp" -ge 1 ]]; then
  log_status "BL021-B1-006_hash_input_contract" "PASS" "include=$hash_include_count exclude=$hash_exclude_count" "$SCENARIO_PATH"
else
  log_status "BL021-B1-006_hash_input_contract" "FAIL" "include=$hash_include_count exclude=$hash_exclude_count has_timestamp_exclusion=$exclude_has_timestamp" "$SCENARIO_PATH"
  record_failure deterministic_contract_failure
fi

THRESHOLD_SUITE_STATUS="$(jq -r '.bl021_contract_checks.thresholds.suite_status // "PASS"' "$SCENARIO_PATH" 2>/dev/null || printf "PASS")"
THRESHOLD_MAX_WARNINGS="$(jq -r '.bl021_contract_checks.thresholds.max_warnings // 0' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
if ! [[ "$THRESHOLD_MAX_WARNINGS" =~ ^[0-9]+$ ]]; then
  THRESHOLD_MAX_WARNINGS=0
fi

{
  printf "scenario_id=%s\n" "$(jq -r '.id // ""' "$SCENARIO_PATH" 2>/dev/null || printf "")"
  printf "run_mode=%s\n" "$RUN_MODE"
  printf "runs=%s\n" "$RUNS"
  printf "suite_status_threshold=%s\n" "$THRESHOLD_SUITE_STATUS"
  printf "max_warnings_threshold=%s\n" "$THRESHOLD_MAX_WARNINGS"
  printf "hash_include_count=%s\n" "$hash_include_count"
  printf "hash_exclude_count=%s\n" "$hash_exclude_count"
  printf "required_transition_states=%s\n" "$transition_states_count"
  printf "required_fallback_taxonomy=%s\n" "$fallback_taxonomy_count"
} >"$SCENARIO_CONTRACT_LOG"

if [[ "$RUN_MODE" == "execute_suite" ]]; then
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

  SCENARIO_ID="$(jq -r '.id // empty' "$SCENARIO_PATH" 2>/dev/null || printf "")"
  RUN_ARTIFACT_DIR="$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID"

  if [[ "$failure_count" -eq 0 ]]; then
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
      passed_count="$(jq -r '.summary.passed // 0' "$RESULT_COPY_JSON")"
      failed_count="$(jq -r '.summary.failed // 0' "$RESULT_COPY_JSON")"
      total_count="$(jq -r '.summary.total // 0' "$RESULT_COPY_JSON")"

      if [[ "$result_status" == "$THRESHOLD_SUITE_STATUS" && "$warnings_count" -le "$THRESHOLD_MAX_WARNINGS" ]]; then
        log_status "scenario_status" "PASS" "status=$result_status warnings=$warnings_count" "$RESULT_COPY_JSON"
      else
        log_status "scenario_status" "FAIL" "status=$result_status warnings=$warnings_count" "$RESULT_COPY_JSON"
        record_failure deterministic_contract_failure
      fi

      {
        printf "scenario_id=%s\n" "$SCENARIO_ID"
        printf "result_status=%s\n" "$result_status"
        printf "warnings=%s\n" "$warnings_count"
        printf "passed=%s\n" "$passed_count"
        printf "failed=%s\n" "$failed_count"
        printf "total=%s\n" "$total_count"
      } >"$SCENARIO_RESULT_LOG"
    else
      log_status "scenario_status" "FAIL" "missing_result_json" "$SCENARIO_RUN_LOG"
      {
        printf "scenario_id=%s\n" "$SCENARIO_ID"
        printf "result_status=MISSING\n"
        printf "warnings=0\n"
        printf "passed=0\n"
        printf "failed=0\n"
        printf "total=0\n"
      } >"$SCENARIO_RESULT_LOG"
      record_failure missing_result_artifact
    fi
  fi
else
  log_status "build_targets" "SKIP" "mode=contract_only" "$BUILD_LOG"
  log_status "scenario_exec" "SKIP" "mode=contract_only" "$SCENARIO_RUN_LOG"
  log_status "scenario_status" "SKIP" "mode=contract_only" "$SCENARIO_RESULT_LOG"
  {
    printf "scenario_id=%s\n" "$(jq -r '.id // ""' "$SCENARIO_PATH" 2>/dev/null || printf "")"
    printf "result_status=SKIP\n"
    printf "warnings=0\n"
    printf "passed=0\n"
    printf "failed=0\n"
    printf "total=0\n"
  } >"$SCENARIO_RESULT_LOG"
fi

artifact_missing_count=0
artifact_missing_list=""
while IFS= read -r artifact_name; do
  [[ -n "$artifact_name" ]] || continue
  if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
    artifact_missing_count=$((artifact_missing_count + 1))
    if [[ -z "$artifact_missing_list" ]]; then
      artifact_missing_list="$artifact_name"
    else
      artifact_missing_list="${artifact_missing_list},${artifact_name}"
    fi
  fi
done < <(jq -r '.bl021_contract_checks.artifact_schema[]?' "$SCENARIO_PATH")
if [[ "$RUN_MODE" == "execute_suite" ]]; then
  while IFS= read -r artifact_name; do
    [[ -n "$artifact_name" ]] || continue
    if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
      artifact_missing_count=$((artifact_missing_count + 1))
      if [[ -z "$artifact_missing_list" ]]; then
        artifact_missing_list="$artifact_name"
      else
        artifact_missing_list="${artifact_missing_list},${artifact_name}"
      fi
    fi
  done < <(jq -r '.bl021_contract_checks.artifact_schema_execute_additions[]?' "$SCENARIO_PATH")
fi
if [[ "$artifact_missing_count" -eq 0 ]]; then
  log_status "BL021-B1-005_artifact_schema_complete" "PASS" "all_required_artifacts_present" "$OUT_DIR"
else
  log_status "BL021-B1-005_artifact_schema_complete" "FAIL" "missing_artifacts=$artifact_missing_list" "$OUT_DIR"
  record_failure missing_result_artifact
fi

printf "deterministic_contract_failure\t%s\tscenario/schema/threshold mismatch\n" "$deterministic_contract_failure" >>"$FAILURE_TAXONOMY_TSV"
printf "runtime_execution_failure\t%s\tnon-zero build/runner/tool failure\n" "$runtime_execution_failure" >>"$FAILURE_TAXONOMY_TSV"
printf "missing_result_artifact\t%s\trequired artifact not produced\n" "$missing_result_artifact" >>"$FAILURE_TAXONOMY_TSV"
printf "deterministic_replay_divergence\t%s\tsingle-run placeholder (use --runs > 1 for enforcement)\n" "$deterministic_replay_divergence" >>"$FAILURE_TAXONOMY_TSV"
printf "deterministic_replay_row_drift\t%s\tsingle-run placeholder (use --runs > 1 for enforcement)\n" "$deterministic_replay_row_drift" >>"$FAILURE_TAXONOMY_TSV"

status_semantic_sha256="$(hash_semantic_tsv "$STATUS_TSV")"
contract_semantic_sha256="$(hash_kv_log "$SCENARIO_CONTRACT_LOG")"
scenario_semantic_sha256="$(hash_kv_log "$SCENARIO_RESULT_LOG")"
taxonomy_sha256="$(hash_file "$FAILURE_TAXONOMY_TSV")"
combined_signature="$(hash_text "${status_semantic_sha256}|${contract_semantic_sha256}|${scenario_semantic_sha256}|${taxonomy_sha256}")"
row_signature="$(hash_text "${failure_count}|${RUN_MODE}")"

lane_result="PASS"
if [[ "$failure_count" -ne 0 ]]; then
  lane_result="FAIL"
fi

if [[ "$lane_result" == "PASS" ]]; then
  log_status "BL021-B1-004_replay_hash_stability" "PASS" "single_run_signature_recorded" "$REPLAY_HASHES_TSV"
else
  log_status "BL021-B1-004_replay_hash_stability" "FAIL" "single_run_failures_present" "$REPLAY_HASHES_TSV"
fi

printf "run_01\t%s\t0\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t1\t%s\t%s\t1\tnone\t%s\n" \
  "$OUT_DIR" \
  "$lane_result" \
  "$(awk -F'\t' 'NR>1 && $2=="FAIL"{count++} END{print count+0}' "$STATUS_TSV")" \
  "$(awk -F'\t' '$1=="BL021-B1-001_contract_schema"{value=$2} END{if(value=="") value="MISSING"; print value}' "$STATUS_TSV")" \
  "$(awk -F'\t' '$1=="BL021-B1-002_transition_contract"{value=$2} END{if(value=="") value="MISSING"; print value}' "$STATUS_TSV")" \
  "$(awk -F'\t' '$1=="BL021-B1-003_fallback_contract"{value=$2} END{if(value=="") value="MISSING"; print value}' "$STATUS_TSV")" \
  "$(awk -F'\t' '$1=="scenario_status"{value=$2} END{if(value=="") value="MISSING"; print value}' "$STATUS_TSV")" \
  "$combined_signature" \
  "$row_signature" \
  "$row_signature" \
  "$QA_LANE_LOG" \
  >>"$VALIDATION_MATRIX_TSV"

printf "run_01\t%s\t%s\t%s\t%s\t%s\t%s\t1\t%s\t%s\t1\n" \
  "$status_semantic_sha256" \
  "$contract_semantic_sha256" \
  "$scenario_semantic_sha256" \
  "$taxonomy_sha256" \
  "$combined_signature" \
  "$combined_signature" \
  "$row_signature" \
  "$row_signature" \
  >>"$REPLAY_HASHES_TSV"

log_status "lane_result" "$lane_result" "failure_count=$failure_count" "$STATUS_TSV"

printf "artifact_dir=%s\n" "$OUT_DIR" >&3
if [[ "$lane_result" == "PASS" ]]; then
  exit 0
fi
exit 1
