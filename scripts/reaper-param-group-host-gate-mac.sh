#!/usr/bin/env bash
# LocusQ BL-079 Parameter Group Host Gate — headless Reaper wrapper.
#
# Runs the BL-079 parameter-group ReaScript gate inside a headless REAPER
# instance, writes repo-local evidence, and exits 0 on pass.
#
# Usage:
#   scripts/reaper-param-group-host-gate-mac.sh --format VST3
#   scripts/reaper-param-group-host-gate-mac.sh --format AU
#
# Optional env:
#   REAPER_BIN=/Applications/REAPER.app/Contents/MacOS/REAPER
#   LQ_REAPER_REQUIRE_LOCUSQ=1
#   LQ_GATE_TIMEOUT_SEC=60
#   LQ_GATE_POLL_INTERVAL_SEC=2
#   LQ_REAPER_PROJECT_FILE=/path/to/project.rpp
#
# Exit codes:
#   0  gate passed
#   1  gate failed / timed out
#   2  configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"

REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
GATE_SCRIPT="$ROOT_DIR/qa/reaper/reascripts/LocusQ_ParamGroupHostGate.lua"
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
GATE_TIMEOUT_SEC="${LQ_GATE_TIMEOUT_SEC:-60}"
GATE_POLL_INTERVAL_SEC="${LQ_GATE_POLL_INTERVAL_SEC:-2}"
PROJECT_FILE="${LQ_REAPER_PROJECT_FILE:-}"
FORMAT=""
PREFERRED_FX=""

usage() {
  cat <<'EOF_USAGE'
Usage: reaper-param-group-host-gate-mac.sh --format VST3|AU [--project /path/to/file.rpp] [--timeout <sec>]
EOF_USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --format)
      [[ $# -ge 2 ]] || { echo "ERROR: --format requires a value" >&2; exit 2; }
      FORMAT="$(echo "$2" | tr '[:lower:]' '[:upper:]')"
      shift 2 ;;
    --project)
      [[ $# -ge 2 ]] || { echo "ERROR: --project requires a path" >&2; exit 2; }
      PROJECT_FILE="$2"; shift 2 ;;
    --timeout)
      [[ $# -ge 2 ]] || { echo "ERROR: --timeout requires a value" >&2; exit 2; }
      GATE_TIMEOUT_SEC="$2"; shift 2 ;;
    --help|-h)
      usage
      exit 0 ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage >&2
      exit 2 ;;
  esac
done

case "$FORMAT" in
  VST3) PREFERRED_FX="VST3: LocusQ" ;;
  AU) PREFERRED_FX="AU: LocusQ" ;;
  *)
    echo "ERROR: --format must be VST3 or AU" >&2
    usage >&2
    exit 2 ;;
esac

if [[ ! -x "$REAPER_BIN" ]]; then
  echo "ERROR: REAPER not found or not executable: $REAPER_BIN" >&2
  exit 2
fi

if [[ ! -f "$GATE_SCRIPT" ]]; then
  echo "ERROR: Gate script not found: $GATE_SCRIPT" >&2
  exit 2
fi

if [[ -n "$PROJECT_FILE" && ! -f "$PROJECT_FILE" ]]; then
  echo "ERROR: Project file not found: $PROJECT_FILE" >&2
  exit 2
fi

run_slug="$(echo "$FORMAT" | tr '[:upper:]' '[:lower:]')"
RUN_DIR="$ROOT_DIR/TestEvidence/bl079_param_group_host_gate_${run_slug}_${TIMESTAMP}"
mkdir -p "$RUN_DIR"

LOG_FILE="$RUN_DIR/run.log"
GATE_STATUS="$RUN_DIR/status.json"
REAPER_LOG="$RUN_DIR/reaper.log"
SUMMARY_MD="$RUN_DIR/summary.md"

{
  echo "timestamp=$TIMESTAMP"
  echo "format=$FORMAT"
  echo "preferred_fx=$PREFERRED_FX"
  echo "reaper_bin=$REAPER_BIN"
  echo "gate_script=$GATE_SCRIPT"
  echo "project_path=${PROJECT_FILE:-(auto-create)}"
  echo "require_locusq=$REQUIRE_LOCUSQ"
  echo "gate_timeout_sec=$GATE_TIMEOUT_SEC"
  echo "run_dir=$RUN_DIR"
} | tee "$LOG_FILE"

REAPER_CMD=(
  "$REAPER_BIN"
  -newinst
  -noactivate
  -nosplash
  "$GATE_SCRIPT"
)

echo "launching reaper: ${REAPER_CMD[*]}" | tee -a "$LOG_FILE"

env \
  LQ_REAPER_NONINTERACTIVE=1 \
  LQ_REAPER_STATUS_JSON="$GATE_STATUS" \
  LQ_REAPER_REQUIRE_LOCUSQ="$REQUIRE_LOCUSQ" \
  LQ_REAPER_PROJECT_FILE="$PROJECT_FILE" \
  LQ_REAPER_PREFERRED_FX="$PREFERRED_FX" \
  "${REAPER_CMD[@]}" >>"$REAPER_LOG" 2>&1 &

REAPER_PID=$!
echo "reaper_pid=$REAPER_PID" | tee -a "$LOG_FILE"

elapsed=0
GATE_STATUS_FOUND=false

while (( elapsed < GATE_TIMEOUT_SEC )); do
  if [[ -f "$GATE_STATUS" ]]; then
    GATE_STATUS_FOUND=true
    break
  fi
  sleep "$GATE_POLL_INTERVAL_SEC"
  elapsed=$((elapsed + GATE_POLL_INTERVAL_SEC))
  echo "waiting for gate status... ${elapsed}s / ${GATE_TIMEOUT_SEC}s" | tee -a "$LOG_FILE"
done

if kill -0 "$REAPER_PID" >/dev/null 2>&1; then
  echo "killing reaper pid=$REAPER_PID" | tee -a "$LOG_FILE"
  kill "$REAPER_PID" >/dev/null 2>&1 || true
  sleep 1
  kill -9 "$REAPER_PID" >/dev/null 2>&1 || true
  wait "$REAPER_PID" >/dev/null 2>&1 || true
fi

if [[ "$GATE_STATUS_FOUND" == "false" ]]; then
  echo "FAIL: gate status.json not produced within ${GATE_TIMEOUT_SEC}s" | tee -a "$LOG_FILE"
  cat > "$SUMMARY_MD" <<EOF_TIMEOUT
Title: BL-079 Param Group Host Gate Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-079 Param Group Host Gate Summary

- timestamp_utc: ${TIMESTAMP}
- format: ${FORMAT}
- lane_result: FAIL
- reason: timeout — status.json not produced within ${GATE_TIMEOUT_SEC}s
- reaper_log: ${REAPER_LOG}
EOF_TIMEOUT
  echo "artifact=$RUN_DIR"
  exit 1
fi

echo "gate status.json found:" | tee -a "$LOG_FILE"
cat "$GATE_STATUS" | tee -a "$LOG_FILE"

GATE_OVERALL="$(grep '"status"' "$GATE_STATUS" | sed 's/.*"status": *"\([^"]*\)".*/\1/')"
GATE_A="$(grep '"gate_a_count"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_B="$(grep '"gate_b_names"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_C="$(grep '"gate_c_order_first"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_D="$(grep '"gate_d_order_sect"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
PARAM_COUNT="$(grep '"param_count"' "$GATE_STATUS" | sed 's/.*"param_count": *\([^,}]*\).*/\1/' | tr -d ' ')"

cat > "$SUMMARY_MD" <<EOF_SUMMARY
Title: BL-079 Param Group Host Gate Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-079 Param Group Host Gate Summary

- timestamp_utc: ${TIMESTAMP}
- format: ${FORMAT}
- lane_result: ${GATE_OVERALL}
- param_count: ${PARAM_COUNT}
- gate_a_count: ${GATE_A} (1=pass)
- gate_b_names: ${GATE_B} (1=pass)
- gate_c_order_first: ${GATE_C} (1=pass)
- gate_d_order_sect: ${GATE_D} (1=pass)
- gate_status: ${GATE_STATUS}
- reaper_log: ${REAPER_LOG}
EOF_SUMMARY

echo "artifact=$RUN_DIR"

if [[ "$GATE_OVERALL" == "pass" ]]; then
  echo "PASS: BL-079 Param Group Host Gate (${FORMAT})"
  exit 0
fi

echo "FAIL: BL-079 Param Group Host Gate (${FORMAT}) — see $GATE_STATUS" >&2
exit 1
