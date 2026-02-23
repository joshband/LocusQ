#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
BOOTSTRAP_SCRIPT="$ROOT_DIR/qa/reaper/reascripts/LocusQ_Create_Manual_QA_Session.lua"
PROJECT_PATH=""
AUTO_BOOTSTRAP=0
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
RENDER_RETRIES="${LQ_REAPER_RENDER_RETRIES:-2}"
RENDER_RETRY_DELAY_SEC="${LQ_REAPER_RENDER_RETRY_DELAY_SEC:-1}"
RENDER_RETRY_EXIT_CODES="${LQ_REAPER_RENDER_RETRY_EXIT_CODES:-130,137,143}"
BOOTSTRAP_TIMEOUT_SEC="${LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC:-45}"
RENDER_TIMEOUT_SEC="${LQ_REAPER_RENDER_TIMEOUT_SEC:-90}"

usage() {
  cat <<USAGE
Usage:
  scripts/reaper-headless-render-smoke-mac.sh [--project /path/to/project.rpp]
  scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap

Optional env:
  REAPER_BIN=/Applications/REAPER.app/Contents/MacOS/REAPER
  LQ_REAPER_REQUIRE_LOCUSQ=1   # default: required; set 0 only for diagnostics
  LQ_REAPER_RENDER_RETRIES=2   # transient render retry attempts after first failure
  LQ_REAPER_RENDER_RETRY_DELAY_SEC=1
  LQ_REAPER_RENDER_RETRY_EXIT_CODES=130,137,143
  LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC=45
  LQ_REAPER_RENDER_TIMEOUT_SEC=90
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --project)
      shift
      PROJECT_PATH="${1:-}"
      ;;
    --auto-bootstrap)
      AUTO_BOOTSTRAP=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      if [[ -z "$PROJECT_PATH" ]]; then
        PROJECT_PATH="$1"
      else
        echo "ERROR: Unexpected argument '$1'"
        usage
        exit 2
      fi
      ;;
  esac
  shift
done

if [[ -z "$PROJECT_PATH" && "$AUTO_BOOTSTRAP" -eq 0 ]]; then
  AUTO_BOOTSTRAP=1
fi

if [[ ! -x "$REAPER_BIN" ]]; then
  echo "ERROR: REAPER executable not found or not executable: $REAPER_BIN"
  exit 2
fi

RUN_DIR="$ROOT_DIR/TestEvidence/reaper_headless_render_${TIMESTAMP}"
mkdir -p "$RUN_DIR"

LOG_FILE="$RUN_DIR/run.log"
STATUS_FILE="$RUN_DIR/status.json"
BOOTSTRAP_LOG="$RUN_DIR/bootstrap.log"
BOOTSTRAP_STATUS="$RUN_DIR/bootstrap_status.json"
RENDER_LOG="$RUN_DIR/render.log"
RENDER_RECENT_LIST="$RUN_DIR/render_recent_audio_files.txt"
BEFORE_LIST="$RUN_DIR/before_audio_files.txt"
AFTER_LIST="$RUN_DIR/after_audio_files.txt"
NEW_LIST="$RUN_DIR/new_audio_files.txt"
RENDER_REF="$RUN_DIR/render_reference.timestamp"

echo "timestamp=$TIMESTAMP" | tee "$LOG_FILE"
echo "reaper_bin=$REAPER_BIN" | tee -a "$LOG_FILE"
echo "auto_bootstrap=$AUTO_BOOTSTRAP" | tee -a "$LOG_FILE"
echo "require_locusq=$REQUIRE_LOCUSQ" | tee -a "$LOG_FILE"
echo "render_retries=$RENDER_RETRIES" | tee -a "$LOG_FILE"
echo "render_retry_delay_sec=$RENDER_RETRY_DELAY_SEC" | tee -a "$LOG_FILE"
echo "render_retry_exit_codes=$RENDER_RETRY_EXIT_CODES" | tee -a "$LOG_FILE"
echo "bootstrap_timeout_sec=$BOOTSTRAP_TIMEOUT_SEC" | tee -a "$LOG_FILE"
echo "render_timeout_sec=$RENDER_TIMEOUT_SEC" | tee -a "$LOG_FILE"

BOOTSTRAP_EXIT=0
BOOTSTRAP_OK=true
LOCUSQ_FOUND=true
BOOTSTRAP_ERROR=""
RENDER_EXIT=0
RENDER_ATTEMPTS=0

run_with_timeout() {
  local timeout_sec="$1"
  local log_file="$2"
  shift 2

  "$@" >>"$log_file" 2>&1 &
  local cmd_pid=$!
  local elapsed=0

  while kill -0 "$cmd_pid" >/dev/null 2>&1; do
    if [[ "$elapsed" -ge "$timeout_sec" ]]; then
      kill "$cmd_pid" >/dev/null 2>&1 || true
      sleep 1
      kill -9 "$cmd_pid" >/dev/null 2>&1 || true
      wait "$cmd_pid" >/dev/null 2>&1 || true
      return 124
    fi
    sleep 1
    elapsed=$((elapsed + 1))
  done

  wait "$cmd_pid"
}

if [[ "$AUTO_BOOTSTRAP" -eq 1 ]]; then
  if [[ ! -f "$BOOTSTRAP_SCRIPT" ]]; then
    echo "ERROR: Bootstrap script not found: $BOOTSTRAP_SCRIPT" | tee -a "$LOG_FILE"
    exit 2
  fi

  PROJECT_PATH="$RUN_DIR/locusq_headless_smoke.rpp"
  BOOTSTRAP_CMD=("$REAPER_BIN" -newinst -noactivate -new "$BOOTSTRAP_SCRIPT" -saveas "$PROJECT_PATH" -closeall:nosave:exit -nosplash)
  echo "bootstrap_cmd=${BOOTSTRAP_CMD[*]}" | tee -a "$LOG_FILE"

  set +e
  run_with_timeout "$BOOTSTRAP_TIMEOUT_SEC" "$BOOTSTRAP_LOG" \
    env \
    LQ_REAPER_NONINTERACTIVE=1 \
    LQ_REAPER_STATUS_JSON="$BOOTSTRAP_STATUS" \
    LQ_REAPER_REQUIRE_LOCUSQ="$REQUIRE_LOCUSQ" \
    "${BOOTSTRAP_CMD[@]}"
  BOOTSTRAP_EXIT=$?
  set -e

  if [[ "$BOOTSTRAP_EXIT" -eq 124 ]]; then
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR="bootstrap timed out after ${BOOTSTRAP_TIMEOUT_SEC}s"
  elif [[ "$BOOTSTRAP_EXIT" -ne 0 ]]; then
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR="bootstrap command failed (exitCode=$BOOTSTRAP_EXIT)"
  elif [[ ! -f "$PROJECT_PATH" ]]; then
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR="bootstrap did not produce project file"
  elif [[ ! -f "$BOOTSTRAP_STATUS" ]]; then
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR="bootstrap did not produce status JSON"
  else
    if rg -q '"locusqFxFound": true' "$BOOTSTRAP_STATUS"; then
      LOCUSQ_FOUND=true
    else
      LOCUSQ_FOUND=false
      if [[ "$REQUIRE_LOCUSQ" == "1" ]]; then
        BOOTSTRAP_OK=false
        BOOTSTRAP_ERROR="LocusQ FX was not found during bootstrap"
      fi
    fi
  fi
else
  if [[ -z "$PROJECT_PATH" || ! -f "$PROJECT_PATH" ]]; then
    echo "ERROR: Project file not found: $PROJECT_PATH" | tee -a "$LOG_FILE"
    exit 2
  fi
fi

PROJECT_DIR="$(cd "$(dirname "$PROJECT_PATH")" && pwd)"
PROJECT_BASENAME="$(basename "$PROJECT_PATH")"
echo "project_path=$PROJECT_PATH" | tee -a "$LOG_FILE"

if [[ "$AUTO_BOOTSTRAP" -eq 1 ]]; then
  PROJECT_RENDER_PATTERN="${PROJECT_BASENAME%.*}"
  perl -0pi -e "s{^  RENDER_FILE \".*\"\\r?\$}{  RENDER_FILE \"$PROJECT_DIR\"}m; s{^  RENDER_PATTERN \".*\"\\r?\$}{  RENDER_PATTERN \"$PROJECT_RENDER_PATTERN\"}m;" "$PROJECT_PATH"
  echo "render_target_dir=$PROJECT_DIR" | tee -a "$LOG_FILE"
  echo "render_target_pattern=$PROJECT_RENDER_PATTERN" | tee -a "$LOG_FILE"
fi

find "$PROJECT_DIR" -maxdepth 1 -type f \( -name '*.wav' -o -name '*.aiff' -o -name '*.aif' -o -name '*.flac' \) | sort >"$BEFORE_LIST"
: >"$RENDER_REF"

RENDER_CMD=("$REAPER_BIN" -newinst -noactivate -renderproject "$PROJECT_PATH" -nosplash)
echo "render_cmd=${RENDER_CMD[*]}" | tee -a "$LOG_FILE"

retryable_render_exit() {
  local code="$1"
  local list=",${RENDER_RETRY_EXIT_CODES},"
  [[ "$list" == *",$code,"* ]]
}

max_attempts=$((RENDER_RETRIES + 1))
for ((attempt=1; attempt<=max_attempts; attempt+=1)); do
  RENDER_ATTEMPTS="$attempt"
  echo "render_attempt=${attempt}" | tee -a "$LOG_FILE"
  set +e
  run_with_timeout "$RENDER_TIMEOUT_SEC" "$RENDER_LOG" "${RENDER_CMD[@]}"
  RENDER_EXIT=$?
  set -e

  if [[ "$RENDER_EXIT" -eq 0 ]]; then
    break
  fi

  if [[ "$RENDER_EXIT" -eq 124 ]]; then
    echo "render_timeout=1 timeout_sec=${RENDER_TIMEOUT_SEC}" | tee -a "$LOG_FILE"
    break
  fi

  if [[ "$attempt" -lt "$max_attempts" ]] && retryable_render_exit "$RENDER_EXIT"; then
    echo "render_retry=1 exit_code=$RENDER_EXIT sleep=${RENDER_RETRY_DELAY_SEC}s" | tee -a "$LOG_FILE"
    sleep "$RENDER_RETRY_DELAY_SEC"
    continue
  fi

  break
done

find "$PROJECT_DIR" -maxdepth 1 -type f \( -name '*.wav' -o -name '*.aiff' -o -name '*.aif' -o -name '*.flac' \) | sort >"$AFTER_LIST"
comm -13 "$BEFORE_LIST" "$AFTER_LIST" >"$NEW_LIST" || true
NEW_AUDIO_COUNT="$(wc -l <"$NEW_LIST" | tr -d ' ')"
AFTER_AUDIO_COUNT="$(wc -l <"$AFTER_LIST" | tr -d ' ')"
find "$PROJECT_DIR" -maxdepth 1 -type f \( -name '*.wav' -o -name '*.aiff' -o -name '*.aif' -o -name '*.flac' \) -newer "$RENDER_REF" -size +0c | sort >"$RENDER_RECENT_LIST" || true
RECENT_AUDIO_COUNT="$(wc -l <"$RENDER_RECENT_LIST" | tr -d ' ')"

RENDER_OUTPUT_DETECTED=true
if [[ "$RECENT_AUDIO_COUNT" == "0" ]]; then
  RENDER_OUTPUT_DETECTED=false
  if [[ "$AUTO_BOOTSTRAP" -eq 1 && "$AFTER_AUDIO_COUNT" -gt 0 && "$RENDER_EXIT" -eq 0 ]]; then
    # Fallback for same-second mtime resolution where -newer may miss freshly rendered files.
    RENDER_OUTPUT_DETECTED=true
  fi
fi

STATUS="pass"
if [[ "$BOOTSTRAP_OK" != "true" || "$RENDER_EXIT" -ne 0 ]]; then
  STATUS="fail"
elif [[ "$AUTO_BOOTSTRAP" -eq 1 && "$RENDER_OUTPUT_DETECTED" != "true" ]]; then
  STATUS="fail"
fi

cat > "$STATUS_FILE" <<JSON
{
  "timestampUtc": "$TIMESTAMP",
  "status": "$STATUS",
  "autoBootstrap": $( [[ "$AUTO_BOOTSTRAP" -eq 1 ]] && echo "true" || echo "false" ),
  "requireLocusQ": $( [[ "$REQUIRE_LOCUSQ" == "1" ]] && echo "true" || echo "false" ),
  "bootstrapExitCode": $BOOTSTRAP_EXIT,
  "bootstrapOk": $BOOTSTRAP_OK,
  "bootstrapError": "$(printf '%s' "$BOOTSTRAP_ERROR")",
  "locusqFxFound": $LOCUSQ_FOUND,
  "renderExitCode": $RENDER_EXIT,
  "renderAttempts": $RENDER_ATTEMPTS,
  "renderRetryBudget": $RENDER_RETRIES,
  "renderTimeoutSec": $RENDER_TIMEOUT_SEC,
  "bootstrapTimeoutSec": $BOOTSTRAP_TIMEOUT_SEC,
  "renderOutputDetected": $RENDER_OUTPUT_DETECTED,
  "afterAudioFileCount": $AFTER_AUDIO_COUNT,
  "newAudioFileCount": $NEW_AUDIO_COUNT,
  "recentAudioFileCount": $RECENT_AUDIO_COUNT,
  "reaperBin": "$REAPER_BIN",
  "projectPath": "$PROJECT_PATH",
  "projectBasename": "$PROJECT_BASENAME",
  "runLog": "$LOG_FILE",
  "bootstrapLog": "$BOOTSTRAP_LOG",
  "bootstrapStatusFile": "$BOOTSTRAP_STATUS",
  "renderLog": "$RENDER_LOG",
  "recentAudioFilesList": "$RENDER_RECENT_LIST",
  "newAudioFilesList": "$NEW_LIST"
}
JSON

if [[ "$STATUS" == "pass" ]]; then
  echo "PASS: REAPER headless render smoke completed" | tee -a "$LOG_FILE"
  echo "artifact=$STATUS_FILE"
  exit 0
fi

echo "FAIL: REAPER headless render smoke failed (bootstrapOk=$BOOTSTRAP_OK renderExitCode=$RENDER_EXIT renderOutputDetected=$RENDER_OUTPUT_DETECTED)" | tee -a "$LOG_FILE"
echo "artifact=$STATUS_FILE"
exit 1
