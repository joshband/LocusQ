#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract-only"
RUNS=1
APP_PATH="${ROOT_DIR}/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl102_discovery_graph_${TIMESTAMP}"

usage() {
  cat <<EOF
Usage: qa-bl102-discovery-graph-mac.sh [options]

Options:
  --contract-only      Run static contract checks only (default)
  --execute            Run standalone execute lane(s) using selftest_scope=bl102
  --runs <count>       Number of execute runs (default: 1)
  --app <path>         Standalone app path (default: ${APP_PATH})
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
    --app)
      APP_PATH="${2:-}"
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
  if rg -q 'const runBl102ScopeOnly = productionP0SelfTestScope === "bl102";' "$ROOT_DIR/Source/ui/src/index.ts"; then
    record "selftest_scope" "PASS" "bl102 selftest scope wired in Source/ui/src/index.ts"
  else
    record "selftest_scope" "FAIL" "bl102 selftest scope missing from Source/ui/src/index.ts"
  fi

  if rg -q 'UI-P1-102A' "$ROOT_DIR/Source/ui/src/index.ts" \
    && rg -q 'UI-P1-102B' "$ROOT_DIR/Source/ui/src/index.ts"; then
    record "ui_checks" "PASS" "BL-102 discovery selftest checks present in production selftest"
  else
    record "ui_checks" "FAIL" "missing BL-102 selftest check IDs in Source/ui/src/index.ts"
  fi

  if rg -q 'scripts/qa-bl102-discovery-graph-mac.sh' "$ROOT_DIR/Documentation/plans/2026-03-18-calibrate-discoverygraph-task-packet.md"; then
    record "plan_trace" "PASS" "task packet references qa-bl102-discovery-graph-mac.sh"
  else
    record "plan_trace" "FAIL" "task packet missing qa-bl102-discovery-graph-mac.sh reference"
  fi

  if rg -q '"bl102_discoverygraph_waveA_evidence"' "$ROOT_DIR/status.json"; then
    record "status_trace" "PASS" "status.json tracks BL-102 evidence"
  else
    record "status_trace" "FAIL" "status.json missing BL-102 evidence pointer"
  fi
else
  if [[ ! -e "$APP_PATH" ]]; then
    record "execute_preflight" "FAIL" "standalone app missing at $APP_PATH"
  else
    record "execute_preflight" "PASS" "standalone app found at $APP_PATH"
  fi

  pass_count=0
  fail_count=0
  for run in $(seq 1 "$RUNS"); do
    log_path="${OUT_DIR}/run_${run}.log"
    if LOCUSQ_UI_SELFTEST_SCOPE=bl102 "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" "$APP_PATH" >"$log_path" 2>&1; then
      artifact_path="$(awk -F= '/^artifact=/{print $2}' "$log_path" | tail -n 1)"
      record "execute_run_${run}" "PASS" "artifact=${artifact_path:-missing}"
      pass_count=$((pass_count + 1))
    else
      artifact_path="$(awk -F= '/^artifact=/{print $2}' "$log_path" | tail -n 1)"
      detail="log=${log_path}"
      if [[ -n "${artifact_path}" ]]; then
        detail="${detail};artifact=${artifact_path}"
      fi
      record "execute_run_${run}" "FAIL" "$detail"
      fail_count=$((fail_count + 1))
    fi
  done

  if [[ "$fail_count" -eq 0 ]]; then
    record "lane_result" "PASS" "bl102 execute lane passed runs=${RUNS}"
  else
    record "lane_result" "FAIL" "bl102 execute lane failed runs=${RUNS} fail_count=${fail_count}"
  fi
fi

{
  echo "# BL-102 DiscoveryGraph QA"
  echo
  echo "- mode: \`${MODE}\`"
  echo "- runs: \`${RUNS}\`"
  echo "- status_tsv: \`${STATUS_TSV}\`"
  if [[ "$MODE" == "execute" ]]; then
    echo "- app: \`${APP_PATH}\`"
  fi
} > "$SUMMARY_MD"

cat "$STATUS_TSV"
