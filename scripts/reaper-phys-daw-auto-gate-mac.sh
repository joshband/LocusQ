#!/usr/bin/env bash
# LocusQ Physics DAW Automation — headless Reaper acceptance gate wrapper.
#
# Runs LocusQ_PhysicsDAWAuto_Gate.lua inside a headless Reaper instance backed
# by BlackHole 16ch (virtual audio device), polls for the gate's status.json,
# and exits 0 (all pass) or 1 (any fail / timeout).
#
# BlackHole gives REAPER a real CoreAudio device so prepareToPlay + processBlock
# run — meaning gates B/D/E/F prove actual physics-engine → APVTS-param writes, not
# just sentinel values.
#
# The script temporarily patches coreaudiooutdevnew/coreaudioindevnew in the
# user's reaper.ini and restores the original on exit (EXIT/INT/TERM trap).
# Do not run while your regular REAPER session has unsaved state — it will
# restore the audio device back to its original value when it exits.
#
# Usage:
#   scripts/reaper-phys-daw-auto-gate-mac.sh
#
# Optional env:
#   REAPER_BIN=/Applications/REAPER.app/Contents/MacOS/REAPER
#   LQ_REAPER_REQUIRE_LOCUSQ=1     fail if LocusQ FX not found (default: 1)
#   LQ_GATE_TIMEOUT_SEC=90         overall timeout waiting for status.json (default: 90)
#   LQ_GATE_POLL_INTERVAL_SEC=2    how often to poll status.json (default: 2)
#   LQ_BLACKHOLE_DEVICE=           CoreAudio device name (default: "BlackHole 16ch")
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%d_%H%M%SZ)"

REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
GATE_SCRIPT="$ROOT_DIR/qa/reaper/reascripts/LocusQ_PhysicsDAWAuto_Gate.lua"
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
GATE_TIMEOUT_SEC="${LQ_GATE_TIMEOUT_SEC:-120}"
GATE_POLL_INTERVAL_SEC="${LQ_GATE_POLL_INTERVAL_SEC:-2}"
# Pre-built RPP with LocusQ on Track 1 — avoids TrackFX_AddByName + plugin-scan dependency.
# Override with LQ_REAPER_PROJECT_FILE="" to fall back to the blank-project path.
PROJECT_FILE="${LQ_REAPER_PROJECT_FILE:-$HOME/Documents/REAPER Media/LocusQ-Loaded-Track1.RPP}"
# BlackHole virtual audio device — gives REAPER a real CoreAudio device so
# the JUCE audio callback fires and physics → APVTS writes are genuine.
BLACKHOLE_DEVICE="${LQ_BLACKHOLE_DEVICE:-BlackHole 16ch}"
REAPER_INI="$HOME/Library/Application Support/REAPER/reaper.ini"

# ── Validate ────────────────────────────────────────────────────────────────

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
  echo "       Set LQ_REAPER_PROJECT_FILE=\"\" to use the blank-project fallback." >&2
  exit 2
fi

if [[ ! -f "$REAPER_INI" ]]; then
  echo "ERROR: REAPER ini not found: $REAPER_INI" >&2
  exit 2
fi

# ── Evidence directory ───────────────────────────────────────────────────────

RUN_DIR="$ROOT_DIR/TestEvidence/reaper_phys_daw_auto_gate_${TIMESTAMP}"
mkdir -p "$RUN_DIR"

LOG_FILE="$RUN_DIR/run.log"
GATE_STATUS="$RUN_DIR/status.json"
REAPER_LOG="$RUN_DIR/reaper.log"
INI_BAK="$RUN_DIR/reaper.ini.gate_bak"

# ── Patch reaper.ini to use BlackHole, restore on exit ───────────────────────
# We back up the entire ini then do a targeted key swap with Python.  The
# EXIT trap restores the backup so the user's audio device is never permanently
# changed even if the gate crashes mid-run.

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
        # Insert after [REAPER] section header
        content = re.sub(r'(\[REAPER\])', rf'\1\n{key}={device}', content, count=1)
with open(path, 'w') as f:
    f.write(content)
PYSCRIPT

echo "timestamp=$TIMESTAMP"               | tee "$LOG_FILE"
echo "reaper_bin=$REAPER_BIN"             | tee -a "$LOG_FILE"
echo "gate_script=$GATE_SCRIPT"           | tee -a "$LOG_FILE"
echo "require_locusq=$REQUIRE_LOCUSQ"     | tee -a "$LOG_FILE"
echo "gate_timeout_sec=$GATE_TIMEOUT_SEC" | tee -a "$LOG_FILE"
echo "project_file=$PROJECT_FILE"         | tee -a "$LOG_FILE"
echo "blackhole_device=$BLACKHOLE_DEVICE" | tee -a "$LOG_FILE"
echo "run_dir=$RUN_DIR"                   | tee -a "$LOG_FILE"

# ── Launch Reaper headlessly ─────────────────────────────────────────────────

# Pass the RPP as the first file argument so REAPER opens it (and calls
# prepareToPlay on LocusQ) before the Lua script starts running.  The Lua
# detects a pre-loaded project by checking CountTracks() > 0 at entry and
# skips Main_openProject so it doesn't re-initialize the audio graph.
if [[ -n "$PROJECT_FILE" ]]; then
  REAPER_CMD=(
    "$REAPER_BIN"
    -newinst
    -noactivate
    -nosplash
    "$PROJECT_FILE"
    "$GATE_SCRIPT"
  )
else
  REAPER_CMD=(
    "$REAPER_BIN"
    -newinst
    -noactivate
    -nosplash
    "$GATE_SCRIPT"
  )
fi

echo "launching reaper: ${REAPER_CMD[*]}" | tee -a "$LOG_FILE"

env \
  LQ_REAPER_NONINTERACTIVE=1 \
  LQ_REAPER_STATUS_JSON="$GATE_STATUS" \
  LQ_REAPER_REQUIRE_LOCUSQ="$REQUIRE_LOCUSQ" \
  LQ_REAPER_PROJECT_FILE="$PROJECT_FILE" \
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

GATE_A="$(grep '"gate_a_param_reg"'        "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_B="$(grep '"gate_b_live_output"'      "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_C="$(grep '"gate_c_no_jump"'          "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_D="$(grep '"gate_d_live_resume"'      "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_E="$(grep '"gate_e_transient_frozen"' "$GATE_STATUS" | grep -c 'true' || echo 0)"
GATE_F="$(grep '"gate_f_frozen_stable"'    "$GATE_STATUS" | grep -c 'true' || echo 0)"

echo "" | tee -a "$LOG_FILE"
echo "gate_a_param_reg=${GATE_A}        (1=pass)" | tee -a "$LOG_FILE"
echo "gate_b_live_output=${GATE_B}      (1=pass)" | tee -a "$LOG_FILE"
echo "gate_c_no_jump=${GATE_C}          (1=pass)" | tee -a "$LOG_FILE"
echo "gate_d_live_resume=${GATE_D}      (1=pass)" | tee -a "$LOG_FILE"
echo "gate_e_transient_frozen=${GATE_E} (1=pass)" | tee -a "$LOG_FILE"
echo "gate_f_frozen_stable=${GATE_F}    (1=pass)" | tee -a "$LOG_FILE"
echo "overall_status=$GATE_OVERALL" | tee -a "$LOG_FILE"
echo "artifact=$RUN_DIR"

if [[ "$GATE_OVERALL" == "pass" ]]; then
  echo "PASS: Physics DAW Auto Gate — all gates pass"
  exit 0
fi

echo "FAIL: Physics DAW Auto Gate — one or more gates failed (see $GATE_STATUS)" >&2
exit 1
