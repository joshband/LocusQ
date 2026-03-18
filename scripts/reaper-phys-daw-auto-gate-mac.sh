#!/usr/bin/env bash
# LocusQ Physics DAW Automation — headless Reaper acceptance gate wrapper.
#
# Runs LocusQ_PhysicsDAWAuto_Gate.lua inside a headless Reaper instance, polls
# for the gate's status.json, and exits 0 (all pass) or 1 (any fail / timeout).
#
# Usage:
#   scripts/reaper-phys-daw-auto-gate-mac.sh
#
# Optional env:
#   REAPER_BIN=/Applications/REAPER.app/Contents/MacOS/REAPER
#   LQ_REAPER_REQUIRE_LOCUSQ=1   fail if LocusQ FX not found (default: 1)
#   LQ_GATE_TIMEOUT_SEC=60       overall timeout waiting for status.json (default: 60)
#   LQ_GATE_POLL_INTERVAL_SEC=2  how often to poll status.json (default: 2)
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%d_%H%M%SZ)"

REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
GATE_SCRIPT="$ROOT_DIR/qa/reaper/reascripts/LocusQ_PhysicsDAWAuto_Gate.lua"
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
GATE_TIMEOUT_SEC="${LQ_GATE_TIMEOUT_SEC:-60}"
GATE_POLL_INTERVAL_SEC="${LQ_GATE_POLL_INTERVAL_SEC:-2}"

# ── Validate ────────────────────────────────────────────────────────────────

if [[ ! -x "$REAPER_BIN" ]]; then
  echo "ERROR: REAPER not found or not executable: $REAPER_BIN" >&2
  exit 2
fi

if [[ ! -f "$GATE_SCRIPT" ]]; then
  echo "ERROR: Gate script not found: $GATE_SCRIPT" >&2
  exit 2
fi

# ── Evidence directory ───────────────────────────────────────────────────────

RUN_DIR="$ROOT_DIR/TestEvidence/reaper_phys_daw_auto_gate_${TIMESTAMP}"
mkdir -p "$RUN_DIR"

LOG_FILE="$RUN_DIR/run.log"
GATE_STATUS="$RUN_DIR/status.json"
REAPER_LOG="$RUN_DIR/reaper.log"

echo "timestamp=$TIMESTAMP"         | tee "$LOG_FILE"
echo "reaper_bin=$REAPER_BIN"       | tee -a "$LOG_FILE"
echo "gate_script=$GATE_SCRIPT"     | tee -a "$LOG_FILE"
echo "require_locusq=$REQUIRE_LOCUSQ" | tee -a "$LOG_FILE"
echo "gate_timeout_sec=$GATE_TIMEOUT_SEC" | tee -a "$LOG_FILE"
echo "run_dir=$RUN_DIR"             | tee -a "$LOG_FILE"

# ── Launch Reaper headlessly ─────────────────────────────────────────────────

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
  "${REAPER_CMD[@]}" >>"$REAPER_LOG" 2>&1 &

REAPER_PID=$!
echo "reaper_pid=$REAPER_PID" | tee -a "$LOG_FILE"

# ── Poll for status.json ─────────────────────────────────────────────────────

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

# ── Kill Reaper if still running ─────────────────────────────────────────────

if kill -0 "$REAPER_PID" >/dev/null 2>&1; then
  echo "killing reaper pid=$REAPER_PID" | tee -a "$LOG_FILE"
  kill "$REAPER_PID" >/dev/null 2>&1 || true
  sleep 1
  kill -9 "$REAPER_PID" >/dev/null 2>&1 || true
  wait "$REAPER_PID" >/dev/null 2>&1 || true
fi

# ── Parse result ─────────────────────────────────────────────────────────────

if [[ "$GATE_STATUS_FOUND" == "false" ]]; then
  echo "FAIL: gate status.json not produced within ${GATE_TIMEOUT_SEC}s" | tee -a "$LOG_FILE"
  echo "artifact=$RUN_DIR"
  exit 1
fi

echo "gate status.json found:" | tee -a "$LOG_FILE"
cat "$GATE_STATUS" | tee -a "$LOG_FILE"

GATE_OVERALL="$(grep '"status"' "$GATE_STATUS" | sed 's/.*"status": *"\([^"]*\)".*/\1/')"

GATE_A="$(grep '"gate_a_param_reg"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_B="$(grep '"gate_b_live_output"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_C="$(grep '"gate_c_no_jump"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_D="$(grep '"gate_d_live_resume"' "$GATE_STATUS" | grep -c 'true' || echo 0)"

echo "" | tee -a "$LOG_FILE"
echo "gate_a_param_reg=${GATE_A}   (1=pass)" | tee -a "$LOG_FILE"
echo "gate_b_live_output=${GATE_B}  (1=pass)" | tee -a "$LOG_FILE"
echo "gate_c_no_jump=${GATE_C}     (1=pass)" | tee -a "$LOG_FILE"
echo "gate_d_live_resume=${GATE_D} (1=pass)" | tee -a "$LOG_FILE"
echo "overall_status=$GATE_OVERALL" | tee -a "$LOG_FILE"
echo "artifact=$RUN_DIR"

if [[ "$GATE_OVERALL" == "pass" ]]; then
  echo "PASS: Physics DAW Auto Gate — all gates pass"
  exit 0
fi

echo "FAIL: Physics DAW Auto Gate — one or more gates failed (see $GATE_STATUS)" >&2
exit 1
