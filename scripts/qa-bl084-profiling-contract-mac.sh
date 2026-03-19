#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HARNESS_DIR="/Users/artbox/Documents/Repos/audio-dsp-qa-harness"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract-only"
RUNS=1
OUT_DIR="${ROOT_DIR}/TestEvidence/bl084_profiling_contract_${TIMESTAMP}"

usage() {
  cat <<EOF
Usage: qa-bl084-profiling-contract-mac.sh [options]

Options:
  --contract-only      Run static contract checks only (default)
  --execute            Run harness + LocusQ validation commands
  --runs <count>       Number of execute runs (default: 1)
  --out <dir>          Output directory (default: ${OUT_DIR})
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only)
      MODE="contract-only"
      shift
      ;;
    --execute)
      MODE="execute"
      shift
      ;;
    --runs)
      RUNS="${2:-}"
      shift 2
      ;;
    --out)
      OUT_DIR="${2:-}"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check\tresult\tdetail\n" > "$STATUS_TSV"

record() {
  local check="$1"
  local result="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$check" "$result" "$detail" >> "$STATUS_TSV"
}

if [[ "$MODE" == "contract-only" ]]; then
  if rg -Fq '#include "runners/performance_profiler.h"' "$HARNESS_DIR/lib/qa_runner_app/BaseQARunner.h" \
    && rg -Fq 'attachProfilingMetricsIfEnabled' "$HARNESS_DIR/lib/qa_runner_app/BaseQARunner.h" \
    && rg -Fq 'qa::profileIntoResult' "$HARNESS_DIR/lib/qa_runner_app/BaseQARunner.h"; then
    record "runner_app_auto_profile" "PASS" "BaseQARunner owns profiling attachment"
  else
    record "runner_app_auto_profile" "FAIL" "BaseQARunner missing shared profiling attachment path"
  fi

  if ! rg -Fq '#include "runners/performance_profiler.h"' "$ROOT_DIR/qa/LocusQQARunner.cpp" \
    && ! rg -Fq 'attachProfilingMetrics(' "$ROOT_DIR/qa/LocusQQARunner.cpp"; then
    record "locusq_workaround_removed" "PASS" "LocusQ runner no longer carries local profiling attachment workaround"
  else
    record "locusq_workaround_removed" "FAIL" "LocusQ runner still carries local profiling attachment workaround"
  fi

  if rg -Fq 'ProfilingPolicy::WARN' "$HARNESS_DIR/scenario_engine/scenario_types.h" \
    && rg -Fq 'perf_precondition' "$HARNESS_DIR/scenario_engine/invariant_evaluator.cpp"; then
    record "profiling_policy_contract" "PASS" "profiling precondition policy is enforced upstream"
  else
    record "profiling_policy_contract" "FAIL" "profiling precondition policy markers missing"
  fi

  if rg -Fq '"enable_profiling": true' "$ROOT_DIR/qa/scenarios/locusq_smoke_suite.json"; then
    record "locusq_suite_profile_flag" "PASS" "LocusQ smoke suite requests profiling through suite runtime config"
  else
    record "locusq_suite_profile_flag" "FAIL" "LocusQ smoke suite does not request profiling"
  fi
else
  fail_count=0
  for run in $(seq 1 "$RUNS"); do
    harness_log="${OUT_DIR}/qa_runner_app_test_${run}.log"
    if cmake --build "$HARNESS_DIR/build_bl100" --target qa_runner_app_test -j8 >"$harness_log" 2>&1 \
      && ctest --test-dir "$HARNESS_DIR/build_bl100" --output-on-failure -R '^qa_runner_app_test$' >>"$harness_log" 2>&1; then
      record "harness_execute_${run}" "PASS" "log=${harness_log}"
    else
      record "harness_execute_${run}" "FAIL" "log=${harness_log}"
      fail_count=$((fail_count + 1))
    fi

    locusq_log="${OUT_DIR}/locusq_smoke_${run}.log"
    if cmake --build "$ROOT_DIR/build_local" --config Release --target locusq_qa -j8 >"$locusq_log" 2>&1 \
      && "$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa" "$ROOT_DIR/qa/scenarios/locusq_smoke_suite.json" >>"$locusq_log" 2>&1; then
      record "locusq_execute_${run}" "PASS" "log=${locusq_log}"
    else
      record "locusq_execute_${run}" "FAIL" "log=${locusq_log}"
      fail_count=$((fail_count + 1))
    fi
  done

  if [[ "$fail_count" -eq 0 ]]; then
    record "lane_result" "PASS" "bl084 execute replay passed runs=${RUNS}"
  else
    record "lane_result" "FAIL" "bl084 execute replay failed runs=${RUNS} fail_count=${fail_count}"
  fi
fi

{
  echo "# BL-084 Profiling Contract QA"
  echo
  echo "- mode: \`${MODE}\`"
  echo "- runs: \`${RUNS}\`"
  echo "- status_tsv: \`${STATUS_TSV}\`"
} > "$SUMMARY_MD"

cat "$STATUS_TSV"
