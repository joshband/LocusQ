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
BOOTSTRAP_RETRY_ONCE="${LQ_REAPER_BOOTSTRAP_RETRY_ONCE:-1}"

usage() {
  cat <<USAGE
Usage:
  scripts/reaper-headless-render-smoke-mac.sh [--project /path/to/project.rpp]
  scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap

Optional env:
  REAPER_BIN=/Applications/REAPER.app/Contents/MacOS/REAPER
  LQ_REAPER_REQUIRE_LOCUSQ=1      # default: required; set 0 only for diagnostics
  LQ_REAPER_RENDER_RETRIES=2      # transient render retry attempts after first failure
  LQ_REAPER_RENDER_RETRY_DELAY_SEC=1
  LQ_REAPER_RENDER_RETRY_EXIT_CODES=130,137,143
  LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC=45
  LQ_REAPER_RENDER_TIMEOUT_SEC=90
  LQ_REAPER_BOOTSTRAP_RETRY_ONCE=1  # bounded one retry on bootstrap transport/setup failures
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

STAGE_BOOTSTRAP_RESULT="skipped"
STAGE_INSTALL_RESULT="skipped"
STAGE_RENDER_RESULT="not_run"
STAGE_OUTPUT_RESULT="not_run"
TERMINAL_STAGE="none"
TERMINAL_REASON_CODE="none"
TERMINAL_REASON_DETAIL=""

BOOTSTRAP_EXIT=0
BOOTSTRAP_OK=true
BOOTSTRAP_ERROR=""
BOOTSTRAP_ERROR_CODE="none"
BOOTSTRAP_ATTEMPTS=0
BOOTSTRAP_RETRY_USED=false

LOCUSQ_FOUND=true
RENDER_EXIT=0
RENDER_ATTEMPTS=0

json_escape() {
  printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g'
}

set_terminal_reason() {
  local stage="$1"
  local code="$2"
  local detail="$3"
  if [[ "$TERMINAL_REASON_CODE" == "none" ]]; then
    TERMINAL_STAGE="$stage"
    TERMINAL_REASON_CODE="$code"
    TERMINAL_REASON_DETAIL="$detail"
  fi
}

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

retryable_render_exit() {
  local code="$1"
  local list=",${RENDER_RETRY_EXIT_CODES},"
  [[ "$list" == *",$code,"* ]]
}

bootstrap_retry_cleanup() {
  echo "bootstrap_retry_cleanup=1" | tee -a "$LOG_FILE"
  rm -f "$PROJECT_PATH" "$BOOTSTRAP_STATUS"
  pkill -f "REAPER.*${BOOTSTRAP_SCRIPT}" >/dev/null 2>&1 || true
  sleep 1
}

run_bootstrap_attempt() {
  local attempt="$1"
  BOOTSTRAP_ATTEMPTS="$attempt"
  BOOTSTRAP_OK=true
  BOOTSTRAP_ERROR=""
  BOOTSTRAP_ERROR_CODE="none"
  STAGE_BOOTSTRAP_RESULT="not_run"

  echo "bootstrap_attempt=$attempt" | tee -a "$LOG_FILE"
  {
    echo "==== bootstrap attempt ${attempt} ===="
    echo "cmd=${BOOTSTRAP_CMD[*]}"
  } >> "$BOOTSTRAP_LOG"

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
    BOOTSTRAP_ERROR_CODE="bootstrap_timeout"
    BOOTSTRAP_ERROR="bootstrap timed out after ${BOOTSTRAP_TIMEOUT_SEC}s"
    STAGE_BOOTSTRAP_RESULT="fail"
    return 1
  fi

  if [[ "$BOOTSTRAP_EXIT" -ne 0 ]]; then
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR_CODE="bootstrap_command_failed"
    BOOTSTRAP_ERROR="bootstrap command failed (exitCode=${BOOTSTRAP_EXIT})"
    STAGE_BOOTSTRAP_RESULT="fail"
    return 1
  fi

  if [[ ! -f "$PROJECT_PATH" ]]; then
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR_CODE="bootstrap_project_missing"
    BOOTSTRAP_ERROR="bootstrap did not produce project file"
    STAGE_BOOTSTRAP_RESULT="fail"
    return 1
  fi

  if [[ ! -f "$BOOTSTRAP_STATUS" ]]; then
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR_CODE="bootstrap_status_missing"
    BOOTSTRAP_ERROR="bootstrap did not produce status JSON"
    STAGE_BOOTSTRAP_RESULT="fail"
    return 1
  fi

  STAGE_BOOTSTRAP_RESULT="pass"

  if rg -q '"locusqFxFound": true' "$BOOTSTRAP_STATUS"; then
    LOCUSQ_FOUND=true
    STAGE_INSTALL_RESULT="pass"
    return 0
  fi

  LOCUSQ_FOUND=false
  if [[ "$REQUIRE_LOCUSQ" == "1" ]]; then
    STAGE_INSTALL_RESULT="fail"
    BOOTSTRAP_OK=false
    BOOTSTRAP_ERROR_CODE="locusq_fx_missing"
    BOOTSTRAP_ERROR="LocusQ FX was not found during bootstrap"
    return 1
  fi

  STAGE_INSTALL_RESULT="warn_optional_missing"
  return 0
}

echo "timestamp=$TIMESTAMP" | tee "$LOG_FILE"
echo "reaper_bin=$REAPER_BIN" | tee -a "$LOG_FILE"
echo "auto_bootstrap=$AUTO_BOOTSTRAP" | tee -a "$LOG_FILE"
echo "require_locusq=$REQUIRE_LOCUSQ" | tee -a "$LOG_FILE"
echo "bootstrap_retry_once=$BOOTSTRAP_RETRY_ONCE" | tee -a "$LOG_FILE"
echo "render_retries=$RENDER_RETRIES" | tee -a "$LOG_FILE"
echo "render_retry_delay_sec=$RENDER_RETRY_DELAY_SEC" | tee -a "$LOG_FILE"
echo "render_retry_exit_codes=$RENDER_RETRY_EXIT_CODES" | tee -a "$LOG_FILE"
echo "bootstrap_timeout_sec=$BOOTSTRAP_TIMEOUT_SEC" | tee -a "$LOG_FILE"
echo "render_timeout_sec=$RENDER_TIMEOUT_SEC" | tee -a "$LOG_FILE"

if [[ "$AUTO_BOOTSTRAP" -eq 1 ]]; then
  if [[ ! -f "$BOOTSTRAP_SCRIPT" ]]; then
    STAGE_BOOTSTRAP_RESULT="fail"
    set_terminal_reason "bootstrap" "bootstrap_script_missing" "Bootstrap script not found: $BOOTSTRAP_SCRIPT"
    BOOTSTRAP_OK=false
  else
    PROJECT_PATH="$RUN_DIR/locusq_headless_smoke.rpp"
    BOOTSTRAP_CMD=("$REAPER_BIN" -newinst -noactivate -new "$BOOTSTRAP_SCRIPT" -saveas "$PROJECT_PATH" -closeall:nosave:exit -nosplash)
    echo "bootstrap_cmd=${BOOTSTRAP_CMD[*]}" | tee -a "$LOG_FILE"

    if run_bootstrap_attempt 1; then
      true
    else
      if [[ "$BOOTSTRAP_RETRY_ONCE" == "1" && "$BOOTSTRAP_ERROR_CODE" != "locusq_fx_missing" ]]; then
        BOOTSTRAP_RETRY_USED=true
        bootstrap_retry_cleanup
        run_bootstrap_attempt 2 || true
      fi
    fi

    if [[ "$BOOTSTRAP_OK" != "true" ]]; then
      if [[ "$BOOTSTRAP_ERROR_CODE" == "locusq_fx_missing" ]]; then
        set_terminal_reason "install" "$BOOTSTRAP_ERROR_CODE" "$BOOTSTRAP_ERROR"
      else
        set_terminal_reason "bootstrap" "$BOOTSTRAP_ERROR_CODE" "$BOOTSTRAP_ERROR"
      fi
    fi
  fi
else
  STAGE_BOOTSTRAP_RESULT="skipped"
  STAGE_INSTALL_RESULT="skipped"
  if [[ -z "$PROJECT_PATH" || ! -f "$PROJECT_PATH" ]]; then
    set_terminal_reason "bootstrap" "project_missing" "Project file not found: $PROJECT_PATH"
    STAGE_RENDER_RESULT="skipped"
    STAGE_OUTPUT_RESULT="skipped"
  fi
fi

if [[ "$TERMINAL_REASON_CODE" == "none" ]]; then
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

  if [[ "$RENDER_EXIT" -eq 0 ]]; then
    STAGE_RENDER_RESULT="pass"
  else
    STAGE_RENDER_RESULT="fail"
    if [[ "$RENDER_EXIT" -eq 124 ]]; then
      set_terminal_reason "render" "render_timeout" "Render timed out after ${RENDER_TIMEOUT_SEC}s"
    else
      set_terminal_reason "render" "render_command_failed" "Render command failed (exitCode=${RENDER_EXIT})"
    fi
  fi

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

  if [[ "$AUTO_BOOTSTRAP" -eq 1 ]]; then
    if [[ "$RENDER_OUTPUT_DETECTED" == "true" ]]; then
      STAGE_OUTPUT_RESULT="pass"
    else
      STAGE_OUTPUT_RESULT="fail"
      set_terminal_reason "output" "render_output_missing" "Render completed but no new non-empty audio file detected"
    fi
  else
    STAGE_OUTPUT_RESULT="skipped"
  fi
else
  PROJECT_DIR=""
  PROJECT_BASENAME=""
  NEW_AUDIO_COUNT=0
  AFTER_AUDIO_COUNT=0
  RECENT_AUDIO_COUNT=0
  RENDER_OUTPUT_DETECTED=false
fi

STATUS="pass"
if [[ "$TERMINAL_REASON_CODE" != "none" ]]; then
  STATUS="fail"
elif [[ "$STAGE_BOOTSTRAP_RESULT" == "fail" || "$STAGE_INSTALL_RESULT" == "fail" || "$STAGE_RENDER_RESULT" == "fail" || "$STAGE_OUTPUT_RESULT" == "fail" ]]; then
  STATUS="fail"
  set_terminal_reason "unknown" "stage_result_failed_without_terminal_reason" "One or more stage results failed without explicit reason"
fi

echo "stage_results=bootstrap:${STAGE_BOOTSTRAP_RESULT},install:${STAGE_INSTALL_RESULT},render:${STAGE_RENDER_RESULT},output:${STAGE_OUTPUT_RESULT}" | tee -a "$LOG_FILE"
echo "terminal_stage=${TERMINAL_STAGE}" | tee -a "$LOG_FILE"
echo "terminal_reason_code=${TERMINAL_REASON_CODE}" | tee -a "$LOG_FILE"
echo "terminal_reason_detail=${TERMINAL_REASON_DETAIL}" | tee -a "$LOG_FILE"

cat > "$STATUS_FILE" <<JSON
{
  "timestampUtc": "$TIMESTAMP",
  "status": "$STATUS",
  "autoBootstrap": $( [[ "$AUTO_BOOTSTRAP" -eq 1 ]] && echo "true" || echo "false" ),
  "requireLocusQ": $( [[ "$REQUIRE_LOCUSQ" == "1" ]] && echo "true" || echo "false" ),
  "stageBootstrapResult": "$(json_escape "$STAGE_BOOTSTRAP_RESULT")",
  "stageInstallResult": "$(json_escape "$STAGE_INSTALL_RESULT")",
  "stageRenderResult": "$(json_escape "$STAGE_RENDER_RESULT")",
  "stageOutputResult": "$(json_escape "$STAGE_OUTPUT_RESULT")",
  "terminalStage": "$(json_escape "$TERMINAL_STAGE")",
  "terminalReasonCode": "$(json_escape "$TERMINAL_REASON_CODE")",
  "terminalReasonDetail": "$(json_escape "$TERMINAL_REASON_DETAIL")",
  "bootstrapExitCode": $BOOTSTRAP_EXIT,
  "bootstrapOk": $BOOTSTRAP_OK,
  "bootstrapErrorCode": "$(json_escape "$BOOTSTRAP_ERROR_CODE")",
  "bootstrapError": "$(json_escape "$BOOTSTRAP_ERROR")",
  "bootstrapAttempts": $BOOTSTRAP_ATTEMPTS,
  "bootstrapRetryUsed": $BOOTSTRAP_RETRY_USED,
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
  "reaperBin": "$(json_escape "$REAPER_BIN")",
  "projectPath": "$(json_escape "$PROJECT_PATH")",
  "projectBasename": "$(json_escape "$PROJECT_BASENAME")",
  "runLog": "$(json_escape "$LOG_FILE")",
  "bootstrapLog": "$(json_escape "$BOOTSTRAP_LOG")",
  "bootstrapStatusFile": "$(json_escape "$BOOTSTRAP_STATUS")",
  "renderLog": "$(json_escape "$RENDER_LOG")",
  "recentAudioFilesList": "$(json_escape "$RENDER_RECENT_LIST")",
  "newAudioFilesList": "$(json_escape "$NEW_LIST")"
}
JSON

if [[ "$STATUS" == "pass" ]]; then
  echo "PASS: REAPER headless render smoke completed" | tee -a "$LOG_FILE"
  echo "artifact=$STATUS_FILE"
  exit 0
fi

echo "FAIL: REAPER headless render smoke failed (terminalStage=$TERMINAL_STAGE terminalReasonCode=$TERMINAL_REASON_CODE)" | tee -a "$LOG_FILE"
echo "artifact=$STATUS_FILE"
exit 1
