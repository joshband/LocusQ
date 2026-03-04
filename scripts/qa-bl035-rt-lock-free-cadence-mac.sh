#!/usr/bin/env bash
# Title: BL-035 RT Lock-Free Registration Cadence Runner
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-04
# Last Modified Date: 2026-03-04
#
# Exit codes:
#   0 all runs passed
#   1 one or more runs failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
RUNS=1
SELFTEST_SCOPE="bl029"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl035_cadence_${TIMESTAMP}"

usage() {
  cat <<'USAGE'
Usage: qa-bl035-rt-lock-free-cadence-mac.sh [options]

Run BL-035 lock-free registration validation cadence.

Options:
  --runs <N>            Number of replay runs (default: 1)
  --out-dir <path>      Output directory (default: TestEvidence/bl035_cadence_<timestamp>)
  --selftest-scope <s>  Scope for standalone selftest (default: bl029)
  --help, -h            Show this help

Outputs:
  status.tsv
  run_summary.tsv
  run_XX/build.log
  run_XX/qa_smoke.log
  run_XX/selftest.log
  run_XX/rt_audit.tsv
  run_XX/rt_audit.log
  run_XX/docs_freshness.log
USAGE
}

usage_error() {
  local msg="$1"
  echo "ERROR: ${msg}" >&2
  usage >&2
  exit 2
}

run_cmd() {
  local command="$1"
  local logfile="$2"
  set +e
  (cd "$ROOT_DIR" && eval "$command") >"$logfile" 2>&1
  local ec=$?
  set -e
  echo "$ec"
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
    --selftest-scope)
      [[ $# -ge 2 ]] || usage_error "--selftest-scope requires a value"
      SELFTEST_SCOPE="$2"
      shift 2
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

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || (( RUNS < 1 )); then
  usage_error "--runs must be a positive integer"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
SUMMARY_TSV="${OUT_DIR}/run_summary.tsv"

printf "command\texit_code\tresult\n" >"$STATUS_TSV"
printf "run\tbuild_exit\tsmoke_exit\tselftest_exit\trt_exit\tdocs_exit\trt_non_allowlisted\tlane_result\n" >"$SUMMARY_TSV"

pass_count=0
fail_count=0

for ((i=1; i<=RUNS; i++)); do
  run_id="$(printf "%02d" "$i")"
  run_dir="${OUT_DIR}/run_${run_id}"
  mkdir -p "$run_dir"

  build_log="${run_dir}/build.log"
  smoke_log="${run_dir}/qa_smoke.log"
  selftest_log="${run_dir}/selftest.log"
  rt_audit_tsv="${run_dir}/rt_audit.tsv"
  rt_audit_log="${run_dir}/rt_audit.log"
  docs_log="${run_dir}/docs_freshness.log"

  build_cmd='cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8'
  smoke_cmd='./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json'
  selftest_cmd="LOCUSQ_UI_SELFTEST_SCOPE=${SELFTEST_SCOPE} ./scripts/standalone-ui-selftest-production-p0-mac.sh"
  rt_cmd="./scripts/rt-safety-audit.sh --print-summary --output ${rt_audit_tsv}"
  docs_cmd='./scripts/validate-docs-freshness.sh'

  build_exit="$(run_cmd "$build_cmd" "$build_log")"
  smoke_exit="$(run_cmd "$smoke_cmd" "$smoke_log")"
  selftest_exit="$(run_cmd "$selftest_cmd" "$selftest_log")"
  rt_exit="$(run_cmd "$rt_cmd" "$rt_audit_log")"
  docs_exit="$(run_cmd "$docs_cmd" "$docs_log")"

  rt_non_allowlisted=0
  if [[ -f "$rt_audit_tsv" ]]; then
    rt_non_allowlisted="$(awk -F'\t' 'NR > 1 && $5 != "true" {c++} END {print c + 0}' "$rt_audit_tsv")"
  fi

  lane_result="PASS"
  if (( build_exit != 0 || smoke_exit != 0 || selftest_exit != 0 || rt_exit != 0 || docs_exit != 0 || rt_non_allowlisted != 0 )); then
    lane_result="FAIL"
    ((fail_count++)) || true
  else
    ((pass_count++)) || true
  fi

  printf "run_%s: %s\t%s\t%s\n" "$run_id" "$build_cmd" "$build_exit" "$([[ "$build_exit" == "0" ]] && echo PASS || echo FAIL)" >>"$STATUS_TSV"
  printf "run_%s: %s\t%s\t%s\n" "$run_id" "$smoke_cmd" "$smoke_exit" "$([[ "$smoke_exit" == "0" ]] && echo PASS || echo FAIL)" >>"$STATUS_TSV"
  printf "run_%s: %s\t%s\t%s\n" "$run_id" "$selftest_cmd" "$selftest_exit" "$([[ "$selftest_exit" == "0" ]] && echo PASS || echo FAIL)" >>"$STATUS_TSV"
  printf "run_%s: %s\t%s\t%s\n" "$run_id" "$rt_cmd" "$rt_exit" "$([[ "$rt_exit" == "0" ]] && echo PASS || echo FAIL)" >>"$STATUS_TSV"
  printf "run_%s: %s\t%s\t%s\n" "$run_id" "$docs_cmd" "$docs_exit" "$([[ "$docs_exit" == "0" ]] && echo PASS || echo FAIL)" >>"$STATUS_TSV"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "run_${run_id}" \
    "$build_exit" \
    "$smoke_exit" \
    "$selftest_exit" \
    "$rt_exit" \
    "$docs_exit" \
    "$rt_non_allowlisted" \
    "$lane_result" \
    >>"$SUMMARY_TSV"
done

overall="PASS"
if (( fail_count > 0 )); then
  overall="FAIL"
fi

printf "overall\t%s\t%s\n" "$([[ "$overall" == "PASS" ]] && echo 0 || echo 1)" "$overall" >>"$STATUS_TSV"
printf "pass_count\t%s\t%s\n" "$pass_count" "$overall" >>"$STATUS_TSV"
printf "fail_count\t%s\t%s\n" "$fail_count" "$overall" >>"$STATUS_TSV"

if [[ "$overall" == "PASS" ]]; then
  exit 0
fi

exit 1
