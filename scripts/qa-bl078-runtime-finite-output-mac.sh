#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

BUILD_DIR="${BL078_BUILD_DIR:-$ROOT_DIR/build_local}"
DEFAULT_OUT_DIR="$ROOT_DIR/TestEvidence/bl078_c1_${TIMESTAMP}"
OUT_DIR="${BL078_OUT_DIR:-$DEFAULT_OUT_DIR}"
SUITE_PATH="${BL078_SUITE_PATH:-$ROOT_DIR/qa/scenarios/locusq_bl078_runtime_finite_output_suite.json}"
SMOKE_SUITE="${BL078_SMOKE_SUITE:-$ROOT_DIR/qa/scenarios/locusq_smoke_suite.json}"
QA_BIN="${BL078_QA_BIN:-$BUILD_DIR/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL078_QA_BIN_FALLBACK:-$BUILD_DIR/locusq_qa_artefacts/locusq_qa}"
fi

RUNS="${BL078_RUNS:-5}"
SKIP_BUILD=0
SKIP_SMOKE=0
SKIP_UI_SELFTEST=0
SKIP_RT_AUDIT=0
SKIP_DOCS=0

usage() {
  cat <<USAGE
Usage: ./scripts/qa-bl078-runtime-finite-output-mac.sh [options]

Options:
  --out-dir <path>      Output artifact directory (default: TestEvidence/bl078_c1_<timestamp>).
  --build-dir <path>    Build directory containing locusq_qa + LocusQ_Standalone (default: build_local).
  --suite <path>        BL-078 suite path (default: qa/scenarios/locusq_bl078_runtime_finite_output_suite.json).
  --smoke-suite <path>  Smoke suite path (default: qa/scenarios/locusq_smoke_suite.json).
  --qa-bin <path>       QA runner path override.
  --runs <N>            Deterministic execute-suite replay count, integer >= 1 (default: 5).
  --skip-build          Skip the build step.
  --skip-smoke          Skip the post-lane smoke suite replay.
  --skip-ui-selftest    Skip standalone UI selftest.
  --skip-rt-audit       Skip RT safety audit.
  --skip-docs           Skip docs freshness validation.
  --help                Show usage.

Exit codes:
  0  Lane is green.
  1  One or more checks failed.
  2  Usage/configuration error.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -lt 2 ]] && { echo "ERROR: --out-dir requires a path" >&2; usage >&2; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    --build-dir)
      [[ $# -lt 2 ]] && { echo "ERROR: --build-dir requires a path" >&2; usage >&2; exit 2; }
      BUILD_DIR="$2"
      shift 2
      ;;
    --suite)
      [[ $# -lt 2 ]] && { echo "ERROR: --suite requires a path" >&2; usage >&2; exit 2; }
      SUITE_PATH="$2"
      shift 2
      ;;
    --smoke-suite)
      [[ $# -lt 2 ]] && { echo "ERROR: --smoke-suite requires a path" >&2; usage >&2; exit 2; }
      SMOKE_SUITE="$2"
      shift 2
      ;;
    --qa-bin)
      [[ $# -lt 2 ]] && { echo "ERROR: --qa-bin requires a path" >&2; usage >&2; exit 2; }
      QA_BIN="$2"
      shift 2
      ;;
    --runs)
      [[ $# -lt 2 ]] && { echo "ERROR: --runs requires an integer value" >&2; usage >&2; exit 2; }
      RUNS="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --skip-smoke)
      SKIP_SMOKE=1
      shift
      ;;
    --skip-ui-selftest)
      SKIP_UI_SELFTEST=1
      shift
      ;;
    --skip-rt-audit)
      SKIP_RT_AUDIT=1
      shift
      ;;
    --skip-docs)
      SKIP_DOCS=1
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

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
VALIDATION_MATRIX_TSV="$OUT_DIR/validation_matrix.tsv"
REPLAY_HASHES_TSV="$OUT_DIR/replay_hashes.tsv"
FAILURE_TAXONOMY_TSV="$OUT_DIR/failure_taxonomy.tsv"
SOAK_SUMMARY_TSV="$OUT_DIR/soak_summary.tsv"
FINITE_FUZZ_TSV="$OUT_DIR/finite_fuzz.tsv"
DIAGNOSTICS_SCHEMA_TSV="$OUT_DIR/diagnostics_schema.tsv"
QA_LANE_LOG="$OUT_DIR/qa_lane.log"
BUILD_LOG="$OUT_DIR/build.log"
SMOKE_LOG="$OUT_DIR/smoke.log"
SMOKE_SUITE_RESULT_JSON="$OUT_DIR/smoke_suite_result.json"
UI_SELFTEST_RESULT_JSON="$OUT_DIR/ui_selftest.result.json"
UI_SELFTEST_META_JSON="$OUT_DIR/ui_selftest.meta.json"
UI_SELFTEST_ATTEMPTS_TSV="$OUT_DIR/ui_selftest.attempts.tsv"
UI_SELFTEST_FAILURE_TAXONOMY_TSV="$OUT_DIR/ui_selftest.failure_taxonomy.tsv"
UI_SELFTEST_STDOUT_LOG="$OUT_DIR/ui_selftest.stdout.log"
RT_AUDIT_TSV="$OUT_DIR/rt_audit.tsv"
RT_AUDIT_STDOUT_LOG="$OUT_DIR/rt_audit.stdout.log"
RT_AUDIT_SUMMARY_LOG="$OUT_DIR/rt_audit.summary.log"
DOCS_FRESHNESS_LOG="$OUT_DIR/docs_freshness.log"
LANE_NOTES_MD="$OUT_DIR/lane_notes.md"

printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
printf "run_id\trun_dir\texit_code\tsuite_status\tpass_count\twarn_count\tfail_count\terror_count\tnon_finite_leaks\tdeadline_failures\tmissing_artifacts\tcombined_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\trun_log\n" >"$VALIDATION_MATRIX_TSV"
printf "run_id\tsuite_semantic_sha256\tscenario_semantic_sha256\tdiagnostics_schema_sha256\tcombined_signature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" >"$REPLAY_HASHES_TSV"
printf "failure_class\tcount\tdetail\n" >"$FAILURE_TAXONOMY_TSV"
printf "mode\truns\tsignature_divergence\tmax_signature_divergence\trow_drift\tmax_row_drift\truntime_execution_failure\tdeterministic_contract_failure\tmissing_result_artifact\tvalidation_gate_failure\tfinal_failures\tresult\tbaseline_signature\tbaseline_row_signature\n" >"$SOAK_SUMMARY_TSV"
printf "run_id\tscenario_id\tstress_axis\tcoverage\tseed\tstatus\twarning_count\tnon_finite_count\tdeadline_status\tpeak_level_status\trms_energy_dbfs\tresult_json\n" >"$FINITE_FUZZ_TSV"
printf "field\ttype\tcontract_required\theader_declared\tpublish_loop_store\tresult\tnotes\n" >"$DIAGNOSTICS_SCHEMA_TSV"

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
validation_gate_failure=0

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
    validation_gate_failure)
      validation_gate_failure=$((validation_gate_failure + 1))
      ;;
  esac
}

hash_text() {
  printf "%s" "$1" | shasum -a 256 | awk '{print $1}'
}

hash_file() {
  if [[ -f "$1" ]]; then
    shasum -a 256 "$1" | awk '{print $1}'
  else
    printf "missing"
  fi
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

write_lane_notes() {
  local lane_result="$1"
  cat >"$LANE_NOTES_MD" <<EOF
Title: BL-078 Runtime Finite Output C1 Lane Notes
Document Type: Test Evidence
Author: Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-078 Runtime Finite Output C1 Lane Notes

- Timestamp: \`${DOC_TS}\`
- Lane result: \`${lane_result}\`
- Suite: \`${SUITE_PATH}\`
- Smoke suite: \`${SMOKE_SUITE}\`
- Runs: \`${RUNS}\`
- Build dir: \`${BUILD_DIR}\`
- QA binary: \`${QA_BIN}\`
- Artifact dir: \`${OUT_DIR}\`

## Commands

\`\`\`bash
cmake --build ${BUILD_DIR} --config Release --target LocusQ_Standalone locusq_qa -j 8
${QA_BIN} --spatial ${SUITE_PATH}
${QA_BIN} --spatial ${SMOKE_SUITE}
LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh
./scripts/rt-safety-audit.sh --print-summary --output ${RT_AUDIT_TSV}
./scripts/validate-docs-freshness.sh
\`\`\`

## Key Artifacts

- \`status.tsv\`
- \`validation_matrix.tsv\`
- \`replay_hashes.tsv\`
- \`soak_summary.tsv\`
- \`finite_fuzz.tsv\`
- \`diagnostics_schema.tsv\`
- \`smoke_suite_result.json\`
- \`ui_selftest.meta.json\`
- \`rt_audit.tsv\`
EOF
}

echo "BL-078 C1 lane start: $DOC_TS"
echo "build_dir=$BUILD_DIR"
echo "suite=$SUITE_PATH"
echo "smoke_suite=$SMOKE_SUITE"
echo "qa_bin=$QA_BIN"
echo "runs=$RUNS"

require_cmd jq
require_cmd rg
require_cmd shasum
require_cmd cmake

if [[ ! -f "$SUITE_PATH" ]]; then
  log_status "BL078-C1-001_contract_schema" "FAIL" "missing_suite=$SUITE_PATH" "$SUITE_PATH"
  record_failure deterministic_contract_failure
fi

if [[ ! -f "$SMOKE_SUITE" ]]; then
  log_status "smoke_suite_path" "FAIL" "missing_smoke_suite=$SMOKE_SUITE" "$SMOKE_SUITE"
  record_failure runtime_execution_failure
fi

if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "FAIL" "missing_or_not_executable=$QA_BIN" "$QA_BIN"
  record_failure runtime_execution_failure
fi

if [[ "$failure_count" -eq 0 ]]; then
  if jq empty "$SUITE_PATH" >/dev/null 2>&1; then
    log_status "BL078-C1-001_contract_schema" "PASS" "suite_json_parseable" "$SUITE_PATH"
  else
    log_status "BL078-C1-001_contract_schema" "FAIL" "suite_json_not_parseable" "$SUITE_PATH"
    record_failure deterministic_contract_failure
  fi
fi

acceptance_count="$(jq -r '(.bl078_contract_checks.acceptance_ids // []) | length' "$SUITE_PATH" 2>/dev/null || printf "0")"
fuzz_axis_count="$(jq -r '(.bl078_contract_checks.required_fuzz_axes // []) | length' "$SUITE_PATH" 2>/dev/null || printf "0")"
diagnostics_field_count="$(jq -r '(.bl078_contract_checks.required_diagnostics_fields // []) | length' "$SUITE_PATH" 2>/dev/null || printf "0")"
suite_channels="$(jq -r '.runtime_config.channels // "0"' "$SUITE_PATH" 2>/dev/null || printf "0")"
max_warnings_threshold="$(jq -r '.bl078_contract_checks.thresholds.max_warnings // "0"' "$SUITE_PATH" 2>/dev/null || printf "0")"
max_signature_divergence="$(jq -r '.bl078_contract_checks.thresholds.max_signature_divergence // "0"' "$SUITE_PATH" 2>/dev/null || printf "0")"
max_row_drift="$(jq -r '.bl078_contract_checks.thresholds.max_row_drift // "0"' "$SUITE_PATH" 2>/dev/null || printf "0")"

if [[ "$acceptance_count" -ge 7 && "$fuzz_axis_count" -eq 3 && "$diagnostics_field_count" -eq 6 && "$suite_channels" -eq 4 && "$max_warnings_threshold" -eq 0 ]]; then
  log_status "BL078-C1-001_contract_schema" "PASS" "acceptance_ids=$acceptance_count fuzz_axes=$fuzz_axis_count diagnostics_fields=$diagnostics_field_count channels=$suite_channels" "$SUITE_PATH"
else
  log_status "BL078-C1-001_contract_schema" "FAIL" "acceptance_ids=$acceptance_count fuzz_axes=$fuzz_axis_count diagnostics_fields=$diagnostics_field_count channels=$suite_channels max_warnings=$max_warnings_threshold" "$SUITE_PATH"
  record_failure deterministic_contract_failure
fi

SUITE_SCENARIO_IDS=()
while IFS= read -r scenario_id; do
  [[ -n "$scenario_id" ]] || continue
  SUITE_SCENARIO_IDS+=("$scenario_id")
done < <(jq -r '.scenario_ids[]?' "$SUITE_PATH")
missing_fuzz_axes=""
for scenario_id in "${SUITE_SCENARIO_IDS[@]}"; do
  scenario_path="$ROOT_DIR/qa/scenarios/${scenario_id}.json"
  if [[ ! -f "$scenario_path" ]]; then
    missing_fuzz_axes="${missing_fuzz_axes:+$missing_fuzz_axes,}${scenario_id}:missing_scenario"
    continue
  fi
  axis="$(jq -r '.bl078_fuzz_profile.stress_axis // empty' "$scenario_path" 2>/dev/null || printf "")"
  if ! jq -e --arg axis "$axis" '.bl078_contract_checks.required_fuzz_axes[] | select(. == $axis)' "$SUITE_PATH" >/dev/null 2>&1; then
    missing_fuzz_axes="${missing_fuzz_axes:+$missing_fuzz_axes,}${scenario_id}:axis_missing"
  fi
done

if [[ -z "$missing_fuzz_axes" ]]; then
  log_status "BL078-C1-002_fuzz_coverage" "PASS" "scenario_count=${#SUITE_SCENARIO_IDS[@]}" "$SUITE_PATH"
else
  log_status "BL078-C1-002_fuzz_coverage" "FAIL" "coverage_issues=$missing_fuzz_axes" "$SUITE_PATH"
  record_failure deterministic_contract_failure
fi

diagnostics_failures=0
while IFS= read -r field; do
  [[ -n "$field" ]] || continue
  header_line="$(rg -n "std::atomic<[^>]+>\\s+${field}\\s*\\{" "$ROOT_DIR/Source/PluginProcessor.h" || true)"
  publish_line="$(rg -n "publishedFiniteGuardrailDiagnostics\\.${field}\\.store" "$ROOT_DIR/Source/PluginProcessor.cpp" || true)"
  field_type="missing"
  header_declared="false"
  publish_store="false"
  result="FAIL"
  notes=""

  if [[ -n "$header_line" ]]; then
    header_declared="true"
    field_type="$(printf "%s\n" "$header_line" | head -n 1 | sed -E 's/.*std::atomic<([^>]+)>.*/\1/')"
  fi
  if [[ -n "$publish_line" ]]; then
    publish_store="true"
  fi
  if [[ "$header_declared" == "true" && "$publish_store" == "true" ]]; then
    result="PASS"
    notes="stable_contract_field"
  else
    diagnostics_failures=$((diagnostics_failures + 1))
    notes="header_declared=${header_declared};publish_store=${publish_store}"
  fi

  printf "%s\t%s\tyes\t%s\t%s\t%s\t%s\n" \
    "$field" "$field_type" "$header_declared" "$publish_store" "$result" "$notes" >>"$DIAGNOSTICS_SCHEMA_TSV"
done < <(jq -r '.bl078_contract_checks.required_diagnostics_fields[]?' "$SUITE_PATH")

snapshot_header="$(rg -n "snapshotSeq" "$ROOT_DIR/Source/PluginProcessor.h" | head -n 1 || true)"
snapshot_publish="$(rg -n "publishedFiniteGuardrailDiagnostics\\.snapshotSeq\\.fetch_add" "$ROOT_DIR/Source/PluginProcessor.cpp" || true)"
printf "snapshotSeq\tstd::uint64_t\tno\t%s\t%s\t%s\t%s\n" \
  "$([[ -n "$snapshot_header" ]] && echo true || echo false)" \
  "$([[ -n "$snapshot_publish" ]] && echo true || echo false)" \
  "$([[ -n "$snapshot_header" && -n "$snapshot_publish" ]] && echo PASS || echo WARN)" \
  "seqlock_publication_counter" >>"$DIAGNOSTICS_SCHEMA_TSV"

if [[ "$diagnostics_failures" -eq 0 ]]; then
  log_status "BL078-C1-003_diagnostics_schema" "PASS" "required_fields=$diagnostics_field_count" "$DIAGNOSTICS_SCHEMA_TSV"
else
  log_status "BL078-C1-003_diagnostics_schema" "FAIL" "missing_or_unpublished_fields=$diagnostics_failures" "$DIAGNOSTICS_SCHEMA_TSV"
  record_failure deterministic_contract_failure
fi

diagnostics_schema_sha256="$(hash_file "$DIAGNOSTICS_SCHEMA_TSV")"

if [[ "$SKIP_BUILD" -eq 1 ]]; then
  printf "build skipped via --skip-build\n" >"$BUILD_LOG"
  log_status "build_targets" "PASS" "skipped_by_flag" "$BUILD_LOG"
else
  set +e
  cmake --build "$BUILD_DIR" --config Release --target LocusQ_Standalone locusq_qa -j 8 >"$BUILD_LOG" 2>&1
  build_exit=$?
  set -e
  if [[ "$build_exit" -eq 0 ]]; then
    log_status "build_targets" "PASS" "cmake_build_exit=0" "$BUILD_LOG"
  else
    log_status "build_targets" "FAIL" "cmake_build_exit=$build_exit" "$BUILD_LOG"
    record_failure runtime_execution_failure
  fi
fi

baseline_signature=""
baseline_row_signature=""
signature_divergence_count=0
row_drift_count=0

for run_index in $(seq 1 "$RUNS"); do
  run_label="$(printf "run_%02d" "$run_index")"
  run_dir="$OUT_DIR/$run_label"
  run_log="$OUT_DIR/${run_label}.log"
  run_suite_json="$run_dir/suite_result.json"
  run_scenario_dir="$run_dir/scenario_results"
  run_fuzz_tsv="$run_dir/finite_fuzz.tsv"

  mkdir -p "$run_scenario_dir"
  printf "scenario_id\tstress_axis\tcoverage\tseed\tstatus\twarning_count\tnon_finite_count\tdeadline_status\tpeak_level_status\trms_energy_dbfs\tresult_json\n" >"$run_fuzz_tsv"

  suite_output_json="$ROOT_DIR/qa_output/locusq_spatial/suite_result.json"
  rm -f "$suite_output_json"
  for scenario_id in "${SUITE_SCENARIO_IDS[@]}"; do
    rm -f "$ROOT_DIR/qa_output/locusq_spatial/${scenario_id}/result.json"
  done

  set +e
  "$QA_BIN" --spatial "$SUITE_PATH" >"$run_log" 2>&1
  run_exit=$?
  set -e

  missing_artifacts="none"
  suite_status="MISSING"
  pass_count="0"
  warn_count="0"
  fail_count="0"
  error_count="0"
  run_non_finite_leaks=0
  run_deadline_failures=0

  if [[ -f "$suite_output_json" ]]; then
    cp "$suite_output_json" "$run_suite_json"
    suite_status="$(jq -r '.status // "MISSING"' "$run_suite_json")"
    pass_count="$(jq -r '.summary.passed // 0' "$run_suite_json")"
    warn_count="$(jq -r '.summary.warned // 0' "$run_suite_json")"
    fail_count="$(jq -r '.summary.failed // 0' "$run_suite_json")"
    error_count="0"
  else
    missing_artifacts="suite_result.json"
  fi

  for scenario_id in "${SUITE_SCENARIO_IDS[@]}"; do
    source_json="$ROOT_DIR/qa/scenarios/${scenario_id}.json"
    scenario_output_json="$ROOT_DIR/qa_output/locusq_spatial/${scenario_id}/result.json"
    copied_json="$run_scenario_dir/${scenario_id}.json"
    if [[ -f "$scenario_output_json" ]]; then
      cp "$scenario_output_json" "$copied_json"
      stress_axis="$(jq -r '.bl078_fuzz_profile.stress_axis // "unknown"' "$source_json")"
      coverage="$(jq -r '(.bl078_fuzz_profile.coverage // []) | join(",")' "$source_json")"
      seed_value="$(jq -r '.bl078_fuzz_profile.seed // (.stimulus.parameters.seed // 0)' "$source_json")"
      scenario_status="$(jq -r '.status // "MISSING"' "$copied_json")"
      warning_count="$(jq -r '(.warnings // []) | length' "$copied_json")"
      non_finite_count="$(jq -r '.metrics.no_nan_inf.value // .metrics.non_finite.value // 0' "$copied_json")"
      deadline_status="$(jq -r '.metrics.deadline.status // "MISSING"' "$copied_json")"
      peak_level_status="$(jq -r '.metrics.default_peak_level.status // "MISSING"' "$copied_json")"
      rms_energy_dbfs="$(jq -r '.metrics.signal_present.value // .metrics.rms_energy.value // "nan"' "$copied_json")"

      printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
        "$scenario_id" "$stress_axis" "$coverage" "$seed_value" "$scenario_status" "$warning_count" \
        "$non_finite_count" "$deadline_status" "$peak_level_status" "$rms_energy_dbfs" "$copied_json" >>"$run_fuzz_tsv"
      printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
        "$run_label" "$scenario_id" "$stress_axis" "$coverage" "$seed_value" "$scenario_status" "$warning_count" \
        "$non_finite_count" "$deadline_status" "$peak_level_status" "$rms_energy_dbfs" "$copied_json" >>"$FINITE_FUZZ_TSV"

      if [[ "$deadline_status" != "PASS" ]]; then
        run_deadline_failures=$((run_deadline_failures + 1))
      fi

      leak_value="${non_finite_count%%.*}"
      if [[ -z "$leak_value" ]]; then
        leak_value=0
      fi
      run_non_finite_leaks=$((run_non_finite_leaks + leak_value))
    else
      if [[ "$missing_artifacts" == "none" ]]; then
        missing_artifacts="scenario_results/${scenario_id}.json"
      else
        missing_artifacts="${missing_artifacts},scenario_results/${scenario_id}.json"
      fi
    fi
  done

  suite_semantic_sha256="missing"
  scenario_semantic_sha256="missing"
  combined_signature="missing"
  row_signature="missing"
  signature_match="1"
  row_match="1"

  if [[ -f "$run_suite_json" ]]; then
    suite_semantic_sha256="$(
      jq -rcS '{status, summary:{passed:.summary.passed, failed:.summary.failed, warned:.summary.warned, total:.summary.total}, scenarios:[.scenarios[] | {id, status}]}' \
        "$run_suite_json" | shasum -a 256 | awk '{print $1}'
    )"
  fi

  scenario_semantic_stream="$run_dir/.scenario_semantic.jsonl"
  : >"$scenario_semantic_stream"
  for scenario_json in "$run_scenario_dir"/*.json; do
    [[ -f "$scenario_json" ]] || continue
    jq -rcS '{scenario_id, status, warnings:(.warnings | length), metrics:{non_finite:(.metrics.no_nan_inf.value // .metrics.non_finite.value // 0), deadline:(.metrics.deadline.status // "MISSING"), signal_present:(.metrics.signal_present.status // "MISSING"), peak:(.metrics.default_peak_level.status // "MISSING")}}' \
      "$scenario_json" >>"$scenario_semantic_stream"
  done
  if [[ -s "$scenario_semantic_stream" ]]; then
    scenario_semantic_sha256="$(shasum -a 256 "$scenario_semantic_stream" | awk '{print $1}')"
  fi

  combined_signature="$(hash_text "${suite_semantic_sha256}|${scenario_semantic_sha256}|${diagnostics_schema_sha256}")"
  row_signature="$(hash_text "${suite_status}|${pass_count}|${warn_count}|${fail_count}|${error_count}|${run_non_finite_leaks}|${run_deadline_failures}")"

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
    record_failure runtime_execution_failure
  fi

  if [[ "$suite_status" != "PASS" || "$warn_count" -ne 0 || "$fail_count" -ne 0 || "$error_count" -ne 0 || "$run_non_finite_leaks" -ne 0 || "$run_deadline_failures" -ne 0 ]]; then
    record_failure deterministic_contract_failure
  fi

  if [[ "$missing_artifacts" != "none" ]]; then
    record_failure missing_result_artifact
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_label" "$suite_semantic_sha256" "$scenario_semantic_sha256" "$diagnostics_schema_sha256" \
    "$combined_signature" "$baseline_signature" "$signature_match" "$row_signature" \
    "$baseline_row_signature" "$row_match" >>"$REPLAY_HASHES_TSV"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_label" "$run_dir" "$run_exit" "$suite_status" "$pass_count" "$warn_count" "$fail_count" \
    "$error_count" "$run_non_finite_leaks" "$run_deadline_failures" "$missing_artifacts" \
    "$combined_signature" "$signature_match" "$row_signature" "$baseline_row_signature" "$row_match" "$run_log" \
    >>"$VALIDATION_MATRIX_TSV"
done

if [[ "$signature_divergence_count" -le "$max_signature_divergence" && "$row_drift_count" -le "$max_row_drift" ]]; then
  log_status "BL078-C1-004_execute_suite_replay" "PASS" "signature_divergence=$signature_divergence_count row_drift=$row_drift_count runs=$RUNS" "$REPLAY_HASHES_TSV"
else
  log_status "BL078-C1-004_execute_suite_replay" "FAIL" "signature_divergence=$signature_divergence_count row_drift=$row_drift_count runs=$RUNS" "$REPLAY_HASHES_TSV"
  if [[ "$signature_divergence_count" -gt "$max_signature_divergence" ]]; then
    record_failure deterministic_replay_divergence
  fi
  if [[ "$row_drift_count" -gt "$max_row_drift" ]]; then
    record_failure deterministic_replay_row_drift
  fi
fi

run_warning_total="$(awk -F'\t' 'NR>1 {sum += $6} END {print sum+0}' "$VALIDATION_MATRIX_TSV")"
run_fail_total="$(awk -F'\t' 'NR>1 {sum += $7} END {print sum+0}' "$VALIDATION_MATRIX_TSV")"
run_error_total="$(awk -F'\t' 'NR>1 {sum += $8} END {print sum+0}' "$VALIDATION_MATRIX_TSV")"
run_non_finite_total="$(awk -F'\t' 'NR>1 {sum += $9} END {print sum+0}' "$VALIDATION_MATRIX_TSV")"
run_deadline_failure_total="$(awk -F'\t' 'NR>1 {sum += $10} END {print sum+0}' "$VALIDATION_MATRIX_TSV")"

if [[ "$run_warning_total" -eq 0 && "$run_fail_total" -eq 0 && "$run_error_total" -eq 0 && "$run_non_finite_total" -eq 0 && "$run_deadline_failure_total" -eq 0 ]]; then
  log_status "BL078-C1-005_execute_suite_green" "PASS" "warnings=$run_warning_total failures=$run_fail_total errors=$run_error_total non_finite=$run_non_finite_total deadline_failures=$run_deadline_failure_total" "$VALIDATION_MATRIX_TSV"
else
  log_status "BL078-C1-005_execute_suite_green" "FAIL" "warnings=$run_warning_total failures=$run_fail_total errors=$run_error_total non_finite=$run_non_finite_total deadline_failures=$run_deadline_failure_total" "$VALIDATION_MATRIX_TSV"
  record_failure deterministic_contract_failure
fi

if [[ "$SKIP_SMOKE" -eq 1 ]]; then
  printf "smoke suite skipped via --skip-smoke\n" >"$SMOKE_LOG"
  printf "{\n  \"status\": \"SKIP\"\n}\n" >"$SMOKE_SUITE_RESULT_JSON"
  log_status "smoke_suite" "SKIP" "skipped_by_flag" "$SMOKE_LOG"
else
  smoke_source_json="$ROOT_DIR/qa_output/locusq_spatial/suite_result.json"
  rm -f "$smoke_source_json"
  set +e
  "$QA_BIN" --spatial "$SMOKE_SUITE" >"$SMOKE_LOG" 2>&1
  smoke_exit=$?
  set -e
  if [[ -f "$smoke_source_json" ]]; then
    cp "$smoke_source_json" "$SMOKE_SUITE_RESULT_JSON"
  fi
  smoke_status="$(jq -r '.status // "MISSING"' "$SMOKE_SUITE_RESULT_JSON" 2>/dev/null || printf "MISSING")"
  smoke_warn_count="$(jq -r '.summary.warned // 0' "$SMOKE_SUITE_RESULT_JSON" 2>/dev/null || printf "0")"
  if [[ "$smoke_exit" -eq 0 && "$smoke_status" == "PASS" && "$smoke_warn_count" -eq 0 ]]; then
    log_status "smoke_suite" "PASS" "status=$smoke_status warnings=$smoke_warn_count" "$SMOKE_SUITE_RESULT_JSON"
  else
    log_status "smoke_suite" "FAIL" "exit=$smoke_exit status=$smoke_status warnings=$smoke_warn_count" "$SMOKE_LOG"
    record_failure validation_gate_failure
  fi
fi

if [[ "$SKIP_UI_SELFTEST" -eq 1 ]]; then
  printf "{\n  \"status\": \"skip\"\n}\n" >"$UI_SELFTEST_META_JSON"
  log_status "ui_selftest" "SKIP" "skipped_by_flag" "$UI_SELFTEST_META_JSON"
else
  set +e
  LOCUSQ_UI_SELFTEST_SCOPE=bl029 \
  LOCUSQ_UI_SELFTEST_RESULT_PATH="$UI_SELFTEST_RESULT_JSON" \
  LOCUSQ_UI_SELFTEST_RUN_LOG_PATH="$UI_SELFTEST_STDOUT_LOG" \
  LOCUSQ_UI_SELFTEST_ATTEMPT_TABLE_PATH="$UI_SELFTEST_ATTEMPTS_TSV" \
  LOCUSQ_UI_SELFTEST_META_PATH="$UI_SELFTEST_META_JSON" \
  LOCUSQ_UI_SELFTEST_FAILURE_TAXONOMY_PATH="$UI_SELFTEST_FAILURE_TAXONOMY_TSV" \
  "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" >"$OUT_DIR/ui_selftest.command.log" 2>&1
  ui_exit=$?
  set -e
  ui_status="$(jq -r '.status // "missing"' "$UI_SELFTEST_META_JSON" 2>/dev/null || printf "missing")"
  if [[ "$ui_exit" -eq 0 && "$ui_status" == "pass" ]]; then
    log_status "ui_selftest" "PASS" "status=$ui_status" "$UI_SELFTEST_META_JSON"
  else
    log_status "ui_selftest" "FAIL" "exit=$ui_exit status=$ui_status" "$UI_SELFTEST_META_JSON"
    record_failure validation_gate_failure
  fi
fi

if [[ "$SKIP_RT_AUDIT" -eq 1 ]]; then
  printf "file\tline\tpattern\tseverity\tallowlisted\tsnippet\n" >"$RT_AUDIT_TSV"
  log_status "rt_audit" "SKIP" "skipped_by_flag" "$RT_AUDIT_TSV"
else
  set +e
  "$ROOT_DIR/scripts/rt-safety-audit.sh" --print-summary --output "$RT_AUDIT_TSV" >"$RT_AUDIT_STDOUT_LOG" 2>"$RT_AUDIT_SUMMARY_LOG"
  rt_exit=$?
  set -e
  non_allowlisted="$(rg -o 'non_allowlisted=[0-9]+' "$RT_AUDIT_SUMMARY_LOG" 2>/dev/null | tail -n 1 | cut -d= -f2 || true)"
  if [[ -z "${non_allowlisted:-}" ]]; then
    non_allowlisted="unknown"
  fi
  if [[ "$rt_exit" -eq 0 ]]; then
    log_status "rt_audit" "PASS" "non_allowlisted=$non_allowlisted" "$RT_AUDIT_TSV"
  else
    log_status "rt_audit" "FAIL" "exit=$rt_exit non_allowlisted=$non_allowlisted" "$RT_AUDIT_SUMMARY_LOG"
    record_failure validation_gate_failure
  fi
fi

write_lane_notes "in_progress"

if [[ "$SKIP_DOCS" -eq 1 ]]; then
  printf "docs freshness skipped via --skip-docs\n" >"$DOCS_FRESHNESS_LOG"
  log_status "docs_freshness" "SKIP" "skipped_by_flag" "$DOCS_FRESHNESS_LOG"
else
  set +e
  "$ROOT_DIR/scripts/validate-docs-freshness.sh" >"$DOCS_FRESHNESS_LOG" 2>&1
  docs_exit=$?
  set -e
  if [[ "$docs_exit" -eq 0 ]]; then
    log_status "docs_freshness" "PASS" "exit=0" "$DOCS_FRESHNESS_LOG"
  else
    log_status "docs_freshness" "FAIL" "exit=$docs_exit" "$DOCS_FRESHNESS_LOG"
    record_failure validation_gate_failure
  fi
fi

validation_gate_rows=0
for check_name in smoke_suite ui_selftest rt_audit docs_freshness; do
  result_value="$(awk -F'\t' -v check="$check_name" '$1==check {value=$2} END {print value}' "$STATUS_TSV")"
  if [[ "$result_value" == "FAIL" ]]; then
    validation_gate_rows=$((validation_gate_rows + 1))
  fi
done

if [[ "$validation_gate_rows" -eq 0 ]]; then
  log_status "BL078-C1-006_validation_gates" "PASS" "smoke_ui_rt_docs_green" "$STATUS_TSV"
else
  log_status "BL078-C1-006_validation_gates" "FAIL" "failing_validation_gates=$validation_gate_rows" "$STATUS_TSV"
fi

artifact_missing_count=0
while IFS= read -r artifact_name; do
  [[ -n "$artifact_name" ]] || continue
  if [[ ! -f "$OUT_DIR/$artifact_name" ]]; then
    artifact_missing_count=$((artifact_missing_count + 1))
  fi
done < <(jq -r '.bl078_contract_checks.artifact_schema[]?' "$SUITE_PATH")

if [[ "$artifact_missing_count" -eq 0 ]]; then
  log_status "BL078-C1-007_artifact_schema" "PASS" "artifact_bundle_complete" "$OUT_DIR"
else
  log_status "BL078-C1-007_artifact_schema" "FAIL" "missing_artifact_count=$artifact_missing_count" "$OUT_DIR"
  record_failure missing_result_artifact
fi

final_failures=0
if [[ "$signature_divergence_count" -gt "$max_signature_divergence" ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$row_drift_count" -gt "$max_row_drift" ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$runtime_execution_failure" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$deterministic_contract_failure" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$missing_result_artifact" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi
if [[ "$validation_gate_failure" -ne 0 ]]; then
  final_failures=$((final_failures + 1))
fi

lane_result="PASS"
if [[ "$final_failures" -ne 0 ]]; then
  lane_result="FAIL"
fi

printf "deterministic_contract_failure\t%s\tsuite/schema/threshold mismatch or execute suite not green\n" "$deterministic_contract_failure" >>"$FAILURE_TAXONOMY_TSV"
printf "runtime_execution_failure\t%s\tbuild or execute suite command failed\n" "$runtime_execution_failure" >>"$FAILURE_TAXONOMY_TSV"
printf "missing_result_artifact\t%s\trequired machine-readable artifact missing\n" "$missing_result_artifact" >>"$FAILURE_TAXONOMY_TSV"
printf "deterministic_replay_divergence\t%s\treplay hash signature mismatch\n" "$deterministic_replay_divergence" >>"$FAILURE_TAXONOMY_TSV"
printf "deterministic_replay_row_drift\t%s\treplay semantic row mismatch\n" "$deterministic_replay_row_drift" >>"$FAILURE_TAXONOMY_TSV"
printf "validation_gate_failure\t%s\tsmoke/ui/rt/docs validation gate failed\n" "$validation_gate_failure" >>"$FAILURE_TAXONOMY_TSV"

printf "execute_suite\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
  "$RUNS" \
  "$signature_divergence_count" \
  "$max_signature_divergence" \
  "$row_drift_count" \
  "$max_row_drift" \
  "$runtime_execution_failure" \
  "$deterministic_contract_failure" \
  "$missing_result_artifact" \
  "$validation_gate_failure" \
  "$final_failures" \
  "$lane_result" \
  "$baseline_signature" \
  "$baseline_row_signature" \
  >>"$SOAK_SUMMARY_TSV"

write_lane_notes "$lane_result"

if [[ "$lane_result" == "PASS" ]]; then
  log_status "lane_result" "PASS" "bl078_c1_green" "$STATUS_TSV"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 0
fi

log_status "lane_result" "FAIL" "failure_count=$final_failures" "$STATUS_TSV"
printf "artifact_dir=%s\n" "$OUT_DIR" >&3
exit 1
