#!/usr/bin/env bash
# LocusQ BL-095 PDC Host Gate — headless Reaper PDC truthfulness check.
#
# Launches LocusQ_PDCHostGate.lua inside a headless Reaper instance, polls
# for the gate's status.json, and exits 0 (pass) or 1 (fail / timeout).
#
# Usage:
#   scripts/reaper-pdc-host-gate-mac.sh
#
# Optional env:
#   REAPER_BIN=/Applications/REAPER.app/Contents/MacOS/REAPER
#   LQ_REAPER_REQUIRE_LOCUSQ=1   fail if LocusQ FX not found (default: 1)
#   LQ_GATE_TIMEOUT_SEC=60       overall timeout waiting for status.json (default: 60)
#   LQ_GATE_POLL_INTERVAL_SEC=2  polling interval in seconds (default: 2)
#
# Exit codes:
#   0  PDC gate passed (pdc_samples == 0)
#   1  PDC gate failed, timeout, or Reaper/LocusQ not available
#   2  configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"

REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
GATE_SCRIPT="$ROOT_DIR/qa/reaper/reascripts/LocusQ_PDCHostGate.lua"
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
GATE_TIMEOUT_SEC="${LQ_GATE_TIMEOUT_SEC:-60}"
GATE_POLL_INTERVAL_SEC="${LQ_GATE_POLL_INTERVAL_SEC:-2}"
PROJECT_PATH=""  # optional: path to an existing .rpp with LocusQ pre-loaded

while [[ $# -gt 0 ]]; do
  case "$1" in
    --project)
      [[ $# -ge 2 ]] || { echo "ERROR: --project requires a path" >&2; exit 2; }
      PROJECT_PATH="$2"; shift 2 ;;
    --timeout)
      [[ $# -ge 2 ]] || { echo "ERROR: --timeout requires a value" >&2; exit 2; }
      GATE_TIMEOUT_SEC="$2"; shift 2 ;;
    --help|-h)
      echo "Usage: reaper-pdc-host-gate-mac.sh [--project /path/to/file.rpp] [--timeout <sec>]"
      exit 0 ;;
    *)
      echo "ERROR: unknown argument: $1" >&2; exit 2 ;;
  esac
done

# ── Validate ──────────────────────────────────────────────────────────────────

if [[ ! -x "$REAPER_BIN" ]]; then
  echo "ERROR: REAPER not found or not executable: $REAPER_BIN" >&2
  echo "       Set REAPER_BIN env var to point to the REAPER binary." >&2
  exit 2
fi

if [[ ! -f "$GATE_SCRIPT" ]]; then
  echo "ERROR: Gate script not found: $GATE_SCRIPT" >&2
  exit 2
fi

# ── Evidence directory ────────────────────────────────────────────────────────

RUN_DIR="$ROOT_DIR/TestEvidence/bl095_pdc_host_gate_${TIMESTAMP}"
mkdir -p "$RUN_DIR"

LOG_FILE="$RUN_DIR/run.log"
GATE_STATUS="$RUN_DIR/status.json"
REAPER_LOG="$RUN_DIR/reaper.log"
SUMMARY_MD="$RUN_DIR/summary.md"

{
  echo "timestamp=$TIMESTAMP"
  echo "reaper_bin=$REAPER_BIN"
  echo "gate_script=$GATE_SCRIPT"
  echo "project_path=${PROJECT_PATH:-(auto-create)}"
  echo "require_locusq=$REQUIRE_LOCUSQ"
  echo "gate_timeout_sec=$GATE_TIMEOUT_SEC"
  echo "run_dir=$RUN_DIR"
} | tee "$LOG_FILE"

# ── Launch Reaper headlessly ──────────────────────────────────────────────────

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
  LQ_REAPER_PROJECT="$PROJECT_PATH" \
  "${REAPER_CMD[@]}" >>"$REAPER_LOG" 2>&1 &

REAPER_PID=$!
echo "reaper_pid=$REAPER_PID" | tee -a "$LOG_FILE"

# ── Poll for status.json ──────────────────────────────────────────────────────

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

# ── Kill Reaper if still running ──────────────────────────────────────────────

if kill -0 "$REAPER_PID" >/dev/null 2>&1; then
  echo "killing reaper pid=$REAPER_PID" | tee -a "$LOG_FILE"
  kill "$REAPER_PID" >/dev/null 2>&1 || true
  sleep 1
  kill -9 "$REAPER_PID" >/dev/null 2>&1 || true
  wait "$REAPER_PID" >/dev/null 2>&1 || true
fi

# ── Parse result ──────────────────────────────────────────────────────────────

if [[ "$GATE_STATUS_FOUND" == "false" ]]; then
  echo "FAIL: gate status.json not produced within ${GATE_TIMEOUT_SEC}s" | tee -a "$LOG_FILE"
  cat > "$SUMMARY_MD" <<EOF_TIMEOUT
Title: BL-095 PDC Host Gate Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-095 PDC Host Gate Summary

- timestamp_utc: ${TIMESTAMP}
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
GATE_PDC_ZERO="$(grep '"gate_pdc_zero"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
PDC_SAMPLES="$(grep '"pdc_samples"' "$GATE_STATUS" | sed 's/.*"pdc_samples": *\([^,}]*\).*/\1/' | tr -d ' ')"

echo "" | tee -a "$LOG_FILE"
echo "gate_pdc_zero=${GATE_PDC_ZERO}   (1=pass)" | tee -a "$LOG_FILE"
echo "pdc_samples=${PDC_SAMPLES}"                 | tee -a "$LOG_FILE"
echo "overall_status=$GATE_OVERALL"               | tee -a "$LOG_FILE"

cat > "$SUMMARY_MD" <<EOF_SUMMARY
Title: BL-095 PDC Host Gate Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-095 PDC Host Gate Summary

- timestamp_utc: ${TIMESTAMP}
- lane_result: ${GATE_OVERALL}
- gate_pdc_zero: ${GATE_PDC_ZERO} (1=pass)
- pdc_samples: ${PDC_SAMPLES} (expected: 0)
- reaper_log: ${REAPER_LOG}
- gate_status: ${GATE_STATUS}

## Interpretation

pdc_samples=0 means Reaper sees zero latency from the LocusQ FIR calibration
engine — the host will not over-compensate and audio timing stays aligned.

pdc_samples>0 means the plugin is reporting false latency to the host.
Check getLatencySamples() in HeadphoneFirHook.h and the setLatencySamples()
publication path in PluginProcessor.cpp.
EOF_SUMMARY

echo "artifact=$RUN_DIR"

if [[ "$GATE_OVERALL" == "pass" ]]; then
  echo "PASS: BL-095 PDC Host Gate — pdc_samples=${PDC_SAMPLES}"
  exit 0
fi

echo "FAIL: BL-095 PDC Host Gate — see $GATE_STATUS" >&2
exit 1
