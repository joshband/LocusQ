#!/usr/bin/env bash
# LocusQ collision transient host gate wrapper for headless REAPER.
#
# Opens a preloaded REAPER project with LocusQ on Track 1, runs the collision
# transient gate script, and exits 0 on pass / 1 on fail. Uses BlackHole 16ch
# so REAPER drives the real JUCE audio callback during the host acceptance run.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%d_%H%M%SZ)"

REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
GATE_SCRIPT="$ROOT_DIR/qa/reaper/reascripts/LocusQ_PhysicsCollisionTransient_Gate.lua"
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
GATE_TIMEOUT_SEC="${LQ_GATE_TIMEOUT_SEC:-120}"
GATE_POLL_INTERVAL_SEC="${LQ_GATE_POLL_INTERVAL_SEC:-2}"
PROJECT_FILE="${LQ_REAPER_PROJECT_FILE:-$HOME/Documents/REAPER Media/LocusQ-Loaded-Track1.RPP}"
BLACKHOLE_DEVICE="${LQ_BLACKHOLE_DEVICE:-BlackHole 16ch}"
REAPER_INI="$HOME/Library/Application Support/REAPER/reaper.ini"

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

if [[ ! -f "$REAPER_INI" ]]; then
  echo "ERROR: REAPER ini not found: $REAPER_INI" >&2
  exit 2
fi

RUN_DIR="$ROOT_DIR/TestEvidence/reaper_phys_collision_transient_gate_${TIMESTAMP}"
mkdir -p "$RUN_DIR"

LOG_FILE="$RUN_DIR/run.log"
GATE_STATUS="$RUN_DIR/status.json"
REAPER_LOG="$RUN_DIR/reaper.log"
INI_BAK="$RUN_DIR/reaper.ini.gate_bak"

cp "$REAPER_INI" "$INI_BAK"

restore_reaper_ini() {
  if [[ -f "$INI_BAK" ]]; then
    cp "$INI_BAK" "$REAPER_INI"
    echo "reaper.ini restored to original audio device" | tee -a "$LOG_FILE" 2>/dev/null || true
  fi
}
trap restore_reaper_ini EXIT INT TERM

python3 - "$REAPER_INI" "$BLACKHOLE_DEVICE" << 'PYSCRIPT'
import sys, re
path, device = sys.argv[1], sys.argv[2]
with open(path) as f:
    content = f.read()
for key in ('coreaudiooutdevnew', 'coreaudioindevnew'):
    if re.search(rf'^{key}=', content, re.MULTILINE):
        content = re.sub(rf'^{key}=.*$', f'{key}={device}', content, flags=re.MULTILINE)
    else:
        content = re.sub(r'(\[REAPER\])', rf'\1\n{key}={device}', content, count=1)
with open(path, 'w') as f:
    f.write(content)
PYSCRIPT

echo "timestamp=$TIMESTAMP" | tee "$LOG_FILE"
echo "reaper_bin=$REAPER_BIN" | tee -a "$LOG_FILE"
echo "gate_script=$GATE_SCRIPT" | tee -a "$LOG_FILE"
echo "project_file=$PROJECT_FILE" | tee -a "$LOG_FILE"
echo "blackhole_device=$BLACKHOLE_DEVICE" | tee -a "$LOG_FILE"
echo "run_dir=$RUN_DIR" | tee -a "$LOG_FILE"

REAPER_CMD=(
  "$REAPER_BIN"
  -newinst
  -noactivate
  -nosplash
  "$PROJECT_FILE"
  "$GATE_SCRIPT"
)

echo "launching reaper: ${REAPER_CMD[*]}" | tee -a "$LOG_FILE"

env \
  LQ_REAPER_NONINTERACTIVE=1 \
  LQ_REAPER_STATUS_JSON="$GATE_STATUS" \
  LQ_REAPER_REQUIRE_LOCUSQ="$REQUIRE_LOCUSQ" \
  LQ_REAPER_PROJECT_FILE="$PROJECT_FILE" \
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
  echo "artifact=$RUN_DIR"
  exit 1
fi

echo "gate status.json found:" | tee -a "$LOG_FILE"
cat "$GATE_STATUS" | tee -a "$LOG_FILE"

GATE_OVERALL="$(grep '"status"' "$GATE_STATUS" | sed 's/.*"status": *"\([^"]*\)".*/\1/')"
echo "overall_status=$GATE_OVERALL" | tee -a "$LOG_FILE"
echo "artifact=$RUN_DIR"

if [[ "$GATE_OVERALL" == "pass" ]]; then
  echo "PASS: Physics collision transient host gate"
  exit 0
fi

echo "FAIL: Physics collision transient host gate — see $GATE_STATUS" >&2
exit 1
