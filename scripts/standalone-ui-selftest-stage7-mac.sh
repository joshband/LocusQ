#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

APP_INPUT="${1:-}"
if [[ -z "$APP_INPUT" ]]; then
  APP_EXEC="$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app/Contents/MacOS/LocusQ"
elif [[ -d "$APP_INPUT" ]]; then
  APP_EXEC="$APP_INPUT/Contents/MacOS/LocusQ"
else
  APP_EXEC="$APP_INPUT"
fi

if [[ ! -x "$APP_EXEC" ]]; then
  echo "ERROR: Standalone executable not found: $APP_EXEC"
  echo "Build first:"
  echo "  cmake --build build_local --config Release --target LocusQ_Standalone -j 8"
  echo "Or pass app/exec explicitly:"
  echo "  scripts/standalone-ui-selftest-stage7-mac.sh /path/to/LocusQ.app"
  echo "  scripts/standalone-ui-selftest-stage7-mac.sh /path/to/LocusQ.app/Contents/MacOS/LocusQ"
  exit 2
fi

OUT_DIR="$ROOT_DIR/TestEvidence"
mkdir -p "$OUT_DIR"

RESULT_JSON="$OUT_DIR/locusq_incremental_stage7_selftest_${TIMESTAMP}.json"
RUN_LOG="$OUT_DIR/locusq_incremental_stage7_selftest_${TIMESTAMP}.run.log"

echo "selftest_ts=${TIMESTAMP}" | tee "$RUN_LOG"
echo "app_exec=${APP_EXEC}" | tee -a "$RUN_LOG"
echo "result_json=${RESULT_JSON}" | tee -a "$RUN_LOG"

osascript -e 'tell application "LocusQ" to quit' >/dev/null 2>&1 || true
pkill -x LocusQ >/dev/null 2>&1 || true
sleep 1

(
  LOCUSQ_UI_SELFTEST=1 \
  LOCUSQ_UI_SELFTEST_RESULT_PATH="$RESULT_JSON" \
  "$APP_EXEC"
) >>"$RUN_LOG" 2>&1 &
APP_PID=$!

echo "app_pid=${APP_PID}" | tee -a "$RUN_LOG"

deadline=$((SECONDS + 60))
while [[ ! -f "$RESULT_JSON" && $SECONDS -lt $deadline ]]; do
  sleep 1
done

if [[ ! -f "$RESULT_JSON" ]]; then
  echo "result_ready=0" | tee -a "$RUN_LOG"
  kill "$APP_PID" >/dev/null 2>&1 || true
  wait "$APP_PID" >/dev/null 2>&1 || true
  exit 1
fi

echo "result_ready=1" | tee -a "$RUN_LOG"

if command -v jq >/dev/null 2>&1; then
  STATUS="$(jq -r '.payload.status // .status // "unknown"' "$RESULT_JSON" 2>/dev/null || echo unknown)"
  OK="$(jq -r '.payload.ok // .ok // false' "$RESULT_JSON" 2>/dev/null || echo false)"
  echo "status=${STATUS}" | tee -a "$RUN_LOG"
  echo "ok=${OK}" | tee -a "$RUN_LOG"

  if [[ "$OK" != "true" ]]; then
    osascript -e 'tell application "LocusQ" to quit' >/dev/null 2>&1 || true
    kill "$APP_PID" >/dev/null 2>&1 || true
    wait "$APP_PID" >/dev/null 2>&1 || true
    exit 1
  fi
else
  if ! rg -q '"ok"[[:space:]]*:[[:space:]]*true' "$RESULT_JSON"; then
    osascript -e 'tell application "LocusQ" to quit' >/dev/null 2>&1 || true
    kill "$APP_PID" >/dev/null 2>&1 || true
    wait "$APP_PID" >/dev/null 2>&1 || true
    exit 1
  fi
fi

osascript -e 'tell application "LocusQ" to quit' >/dev/null 2>&1 || true
kill "$APP_PID" >/dev/null 2>&1 || true
wait "$APP_PID" >/dev/null 2>&1 || true
pkill -x LocusQ >/dev/null 2>&1 || true

echo "PASS: Stage 7 self-test completed." | tee -a "$RUN_LOG"
echo "artifact=${RESULT_JSON}" | tee -a "$RUN_LOG"
