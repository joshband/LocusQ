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

APP_BUNDLE=""
if [[ "$APP_EXEC" == *.app/Contents/MacOS/LocusQ ]]; then
  APP_BUNDLE="${APP_EXEC%/Contents/MacOS/LocusQ}"
elif [[ -d "$APP_INPUT" && "$APP_INPUT" == *.app ]]; then
  APP_BUNDLE="$APP_INPUT"
fi

if [[ ! -x "$APP_EXEC" ]]; then
  echo "ERROR: Standalone executable not found: $APP_EXEC"
  echo "Build first:"
  echo "  cmake --build build_local --config Release --target LocusQ_Standalone -j 8"
  echo "Or pass app/exec explicitly:"
  echo "  scripts/standalone-ui-selftest-production-p0-mac.sh /path/to/LocusQ.app"
  echo "  scripts/standalone-ui-selftest-production-p0-mac.sh /path/to/LocusQ.app/Contents/MacOS/LocusQ"
  exit 2
fi

OUT_DIR="$ROOT_DIR/TestEvidence"
mkdir -p "$OUT_DIR"

RESULT_JSON_DEFAULT="$OUT_DIR/locusq_production_p0_selftest_${TIMESTAMP}.json"
RUN_LOG_DEFAULT="$OUT_DIR/locusq_production_p0_selftest_${TIMESTAMP}.run.log"
ATTEMPT_TABLE_DEFAULT="$OUT_DIR/locusq_production_p0_selftest_${TIMESTAMP}.attempts.tsv"
META_JSON_DEFAULT="$OUT_DIR/locusq_production_p0_selftest_${TIMESTAMP}.meta.json"

RESULT_JSON="${LOCUSQ_UI_SELFTEST_RESULT_PATH:-$RESULT_JSON_DEFAULT}"
RUN_LOG="${LOCUSQ_UI_SELFTEST_RUN_LOG_PATH:-$RUN_LOG_DEFAULT}"
ATTEMPT_TABLE="${LOCUSQ_UI_SELFTEST_ATTEMPT_TABLE_PATH:-$ATTEMPT_TABLE_DEFAULT}"
META_JSON="${LOCUSQ_UI_SELFTEST_META_PATH:-$META_JSON_DEFAULT}"

SELFTEST_TIMEOUT_SECONDS="${LOCUSQ_UI_SELFTEST_TIMEOUT_SECONDS:-75}"
MAX_ATTEMPTS="${LOCUSQ_UI_SELFTEST_MAX_ATTEMPTS:-1}"
RETRY_DELAY_SECONDS="${LOCUSQ_UI_SELFTEST_RETRY_DELAY_SECONDS:-1}"
LAUNCH_MODE_REQUESTED="${LOCUSQ_UI_SELFTEST_LAUNCH_MODE:-direct}"
LOCK_PATH="${LOCUSQ_UI_SELFTEST_LOCK_PATH:-${TMPDIR:-/tmp}/locusq_ui_selftest.lock}"
LOCK_WAIT_TIMEOUT_SECONDS="${LOCUSQ_UI_SELFTEST_LOCK_WAIT_TIMEOUT_SECONDS:-180}"
LOCK_STALE_SECONDS="${LOCUSQ_UI_SELFTEST_LOCK_STALE_SECONDS:-300}"
LOCK_POLL_SECONDS="${LOCUSQ_UI_SELFTEST_LOCK_POLL_SECONDS:-1}"
PROCESS_DRAIN_TIMEOUT_SECONDS="${LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS:-12}"
LAUNCHED_APP_PID=""
LAUNCHED_APP_WAITABLE=0
LAUNCHED_APP_WAITABILITY_REASON="not_launched"

is_uint() {
  [[ "$1" =~ ^[0-9]+$ ]]
}

if ! is_uint "$SELFTEST_TIMEOUT_SECONDS"; then
  SELFTEST_TIMEOUT_SECONDS=75
fi
if ! is_uint "$MAX_ATTEMPTS" || (( MAX_ATTEMPTS < 1 )); then
  MAX_ATTEMPTS=1
fi
if ! is_uint "$RETRY_DELAY_SECONDS"; then
  RETRY_DELAY_SECONDS=1
fi
if ! is_uint "$LOCK_WAIT_TIMEOUT_SECONDS"; then
  LOCK_WAIT_TIMEOUT_SECONDS=180
fi
if ! is_uint "$LOCK_STALE_SECONDS"; then
  LOCK_STALE_SECONDS=300
fi
if ! is_uint "$LOCK_POLL_SECONDS" || (( LOCK_POLL_SECONDS < 1 )); then
  LOCK_POLL_SECONDS=1
fi
if ! is_uint "$PROCESS_DRAIN_TIMEOUT_SECONDS" || (( PROCESS_DRAIN_TIMEOUT_SECONDS < 1 )); then
  PROCESS_DRAIN_TIMEOUT_SECONDS=12
fi

case "$LAUNCH_MODE_REQUESTED" in
  direct|open)
    ;;
  *)
    LAUNCH_MODE_REQUESTED="direct"
    ;;
esac

LOCK_PATH_REAL="$LOCK_PATH"
LOCK_META_PATH="$LOCK_PATH_REAL/owner.meta"
LOCK_ACQUIRED=0
LOCK_WAIT_SECONDS=0
LOCK_STALE_RECOVERED=0
LOCK_STALE_RECOVERY_REASON="none"
LOCK_OWNER_PID=""
LOCK_OWNER_AGE_SECONDS=0
LOCK_WAIT_RESULT="not_attempted"
LOCK_WAIT_POLLS=0
PRELAUNCH_DRAIN_SECONDS=0
PRELAUNCH_DRAIN_FORCED_KILL=0
PRELAUNCH_DRAIN_RESULT="not_run"
PRELAUNCH_DRAIN_REMAINING_PIDS=""
LAUNCH_MODE_USED="$LAUNCH_MODE_REQUESTED"
LAUNCH_MODE_FALLBACK_REASON="none"

ensure_parent_dir() {
  local path="$1"
  local parent
  parent="$(dirname "$path")"
  mkdir -p "$parent"
}

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

lock_meta_read() {
  local key="$1"
  if [[ ! -f "$LOCK_META_PATH" ]]; then
    return 0
  fi
  awk -F= -v key="$key" '$1 == key { print $2 }' "$LOCK_META_PATH" | tail -n 1
}

release_single_instance_lock() {
  if (( LOCK_ACQUIRED != 1 )); then
    return 0
  fi

  local owner_pid
  owner_pid="$(lock_meta_read pid)"
  if [[ -n "$owner_pid" && "$owner_pid" != "$$" ]]; then
    return 0
  fi

  rm -rf "$LOCK_PATH_REAL" >/dev/null 2>&1 || true
  LOCK_ACQUIRED=0
}

acquire_single_instance_lock() {
  local lock_start_epoch lock_now_epoch lock_age_seconds
  lock_start_epoch="$(date +%s)"
  lock_now_epoch="$lock_start_epoch"

  while true; do
    LOCK_WAIT_POLLS=$((LOCK_WAIT_POLLS + 1))
    if mkdir "$LOCK_PATH_REAL" >/dev/null 2>&1; then
      {
        echo "pid=$$"
        echo "started_epoch=$lock_now_epoch"
        echo "app_exec=$APP_EXEC"
      } > "$LOCK_META_PATH"
      LOCK_ACQUIRED=1
      LOCK_WAIT_SECONDS=$((lock_now_epoch - lock_start_epoch))
      LOCK_WAIT_RESULT="acquired"
      return 0
    fi

    LOCK_OWNER_PID="$(lock_meta_read pid)"
    local owner_started_epoch
    owner_started_epoch="$(lock_meta_read started_epoch)"
    lock_now_epoch="$(date +%s)"

    lock_age_seconds=0
    if [[ -n "$owner_started_epoch" ]] && is_uint "$owner_started_epoch"; then
      lock_age_seconds=$((lock_now_epoch - owner_started_epoch))
      if (( lock_age_seconds < 0 )); then
        lock_age_seconds=0
      fi
    fi
    LOCK_OWNER_AGE_SECONDS="$lock_age_seconds"

    local owner_alive=0
    if [[ -n "$LOCK_OWNER_PID" ]] && is_uint "$LOCK_OWNER_PID" && kill -0 "$LOCK_OWNER_PID" >/dev/null 2>&1; then
      owner_alive=1
    fi

    local recover_stale=0
    local stale_reason=""
    if (( owner_alive == 0 )); then
      recover_stale=1
      stale_reason="owner_missing"
    elif (( lock_age_seconds > LOCK_STALE_SECONDS )); then
      recover_stale=1
      stale_reason="stale_age_exceeded"
    fi

    if (( recover_stale == 1 )); then
      rm -rf "$LOCK_PATH_REAL" >/dev/null 2>&1 || true
      LOCK_STALE_RECOVERED=1
      LOCK_STALE_RECOVERY_REASON="$stale_reason"
      sleep "$LOCK_POLL_SECONDS"
      continue
    fi

    if (( lock_now_epoch - lock_start_epoch >= LOCK_WAIT_TIMEOUT_SECONDS )); then
      LOCK_WAIT_SECONDS=$((lock_now_epoch - lock_start_epoch))
      LOCK_WAIT_RESULT="timeout"
      return 1
    fi

    sleep "$LOCK_POLL_SECONDS"
    lock_now_epoch="$(date +%s)"
  done
}

collect_locusq_pids() {
  pgrep -x LocusQ 2>/dev/null || true
}

drain_locusq_processes() {
  local timeout_seconds="$1"
  local emit_telemetry="${2:-0}"
  local forced_kill=0
  local remaining_pids=""
  local start_seconds="$SECONDS"
  local deadline=$((SECONDS + timeout_seconds))

  while true; do
    remaining_pids="$(collect_locusq_pids | tr '\n' ' ' | xargs echo -n)"
    if [[ -z "$remaining_pids" ]]; then
      break
    fi

    if (( SECONDS >= deadline )); then
      pkill -9 -x LocusQ >/dev/null 2>&1 || true
      forced_kill=1
      sleep 1
      remaining_pids="$(collect_locusq_pids | tr '\n' ' ' | xargs echo -n)"
      break
    fi

    sleep 1
  done

  local elapsed_seconds=$((SECONDS - start_seconds))
  if (( emit_telemetry == 1 )); then
    PRELAUNCH_DRAIN_SECONDS="$elapsed_seconds"
    PRELAUNCH_DRAIN_FORCED_KILL="$forced_kill"
    PRELAUNCH_DRAIN_REMAINING_PIDS="$remaining_pids"
    if [[ -z "$remaining_pids" ]]; then
      PRELAUNCH_DRAIN_RESULT="drained"
    else
      PRELAUNCH_DRAIN_RESULT="residual_processes"
    fi
  fi

  if [[ -z "$remaining_pids" ]]; then
    return 0
  fi

  return 1
}

shutdown_locusq() {
  osascript -e 'tell application "LocusQ" to quit' >/dev/null 2>&1 || true
  pkill -x LocusQ >/dev/null 2>&1 || true
  drain_locusq_processes "$PROCESS_DRAIN_TIMEOUT_SECONDS" 0 || true
}

launch_selftest_app() {
  local attempt_result_json="$1"
  local attempt_app_log="$2"
  local pid=""
  local mode="$LAUNCH_MODE_REQUESTED"

  LAUNCH_MODE_USED="$mode"
  LAUNCH_MODE_FALLBACK_REASON="none"
  LAUNCHED_APP_PID=""
  LAUNCHED_APP_WAITABLE=0
  LAUNCHED_APP_WAITABILITY_REASON="not_launched"

  if [[ "$mode" == "open" ]]; then
    if [[ -z "$APP_BUNDLE" || ! -d "$APP_BUNDLE" ]]; then
      LAUNCH_MODE_USED="direct"
      LAUNCH_MODE_FALLBACK_REASON="open_bundle_missing"
      mode="direct"
    fi
  fi

  if [[ "$mode" == "open" ]]; then
    local before_pids after_pids candidate_pid
    before_pids="$(collect_locusq_pids | tr '\n' ' ')"

    (
      LOCUSQ_UI_SELFTEST=1 \
      LOCUSQ_UI_VARIANT=production \
      LOCUSQ_UI_SELFTEST_BL009="${LOCUSQ_UI_SELFTEST_BL009:-0}" \
      LOCUSQ_UI_SELFTEST_BL011="${LOCUSQ_UI_SELFTEST_BL011:-0}" \
      LOCUSQ_UI_SELFTEST_RESULT_PATH="$attempt_result_json" \
      open -n "$APP_BUNDLE"
    ) >>"$attempt_app_log" 2>&1 &
    local open_launcher_pid=$!
    wait "$open_launcher_pid" >/dev/null 2>&1 || true

    for _ in $(seq 1 12); do
      after_pids="$(collect_locusq_pids | tr '\n' ' ')"
      candidate_pid=""
      for pid_candidate in $after_pids; do
        if [[ " $before_pids " != *" $pid_candidate "* ]]; then
          candidate_pid="$pid_candidate"
          break
        fi
      done
      if [[ -n "$candidate_pid" ]]; then
        pid="$candidate_pid"
        break
      fi
      sleep 1
    done

    if [[ -z "$pid" ]]; then
      return 1
    fi
    LAUNCHED_APP_WAITABLE=0
    LAUNCHED_APP_WAITABILITY_REASON="external_launcher"
  else
    (
      LOCUSQ_UI_SELFTEST=1 \
      LOCUSQ_UI_VARIANT=production \
      LOCUSQ_UI_SELFTEST_BL009="${LOCUSQ_UI_SELFTEST_BL009:-0}" \
      LOCUSQ_UI_SELFTEST_BL011="${LOCUSQ_UI_SELFTEST_BL011:-0}" \
      LOCUSQ_UI_SELFTEST_RESULT_PATH="$attempt_result_json" \
      "$APP_EXEC"
    ) >>"$attempt_app_log" 2>&1 &
    pid=$!
    LAUNCHED_APP_WAITABLE=1
    LAUNCHED_APP_WAITABILITY_REASON="direct_child"
  fi

  LAUNCHED_APP_PID="$pid"
  return 0
}

find_crash_report_since() {
  local marker_path="$1"
  local crash_dir="$HOME/Library/Logs/DiagnosticReports"
  if [[ ! -d "$crash_dir" ]]; then
    return 0
  fi
  find "$crash_dir" -maxdepth 1 -type f \( -name 'LocusQ*.crash' -o -name 'LocusQ*.ips' \) -newer "$marker_path" -print 2>/dev/null | sort | tail -n 1
}

signal_number_for_exit_code() {
  local exit_code="$1"
  if is_uint "$exit_code" && (( exit_code > 128 )); then
    printf '%s' "$((exit_code - 128))"
  fi
}

signal_name_for_number() {
  local signal_number="$1"
  if [[ -z "$signal_number" ]]; then
    return 0
  fi
  kill -l "$signal_number" 2>/dev/null || true
}

observe_app_exit_status() {
  local pid="$1"
  local waitable="${2:-0}"
  local __exit_var="$3"
  local __signal_var="$4"
  local __signal_name_var="$5"
  local __source_var="$6"

  local exit_code=""
  local signal=""
  local signal_name=""
  local source="not_observed"

  set +e
  if [[ -n "$pid" ]] && kill -0 "$pid" >/dev/null 2>&1; then
    kill "$pid" >/dev/null 2>&1 || true
  fi

  if [[ "$waitable" == "1" && -n "$pid" ]]; then
    wait "$pid" >/dev/null 2>&1
    exit_code=$?
    source="child_wait"
  elif [[ "$waitable" == "1" ]]; then
    source="child_pid_missing"
  else
    source="external_non_child"
  fi
  set -e

  if [[ -n "$exit_code" ]]; then
    signal="$(signal_number_for_exit_code "$exit_code")"
    signal_name="$(signal_name_for_number "$signal")"
  fi

  printf -v "$__exit_var" '%s' "$exit_code"
  printf -v "$__signal_var" '%s' "$signal"
  printf -v "$__signal_name_var" '%s' "$signal_name"
  printf -v "$__source_var" '%s' "$source"
}

cleanup_launched_app() {
  local pid="$1"
  local waitable="${2:-0}"

  set +e
  if [[ -n "$pid" ]] && kill -0 "$pid" >/dev/null 2>&1; then
    osascript -e 'tell application "LocusQ" to quit' >/dev/null 2>&1 || true
    kill "$pid" >/dev/null 2>&1 || true
  fi
  if [[ "$waitable" == "1" && -n "$pid" ]]; then
    wait "$pid" >/dev/null 2>&1 || true
  fi
  set -e
}

write_metadata_json() {
  local status="$1"
  local reason="$2"
  local app_pid="$3"
  local app_exit_code="$4"
  local app_signal="$5"
  local app_signal_name="$6"
  local crash_report_path="$7"
  local attempts_run="$8"
  local app_exit_status_source="${9:-not_recorded}"

  if command -v jq >/dev/null 2>&1; then
    jq -n \
      --arg selftestTs "$TIMESTAMP" \
      --arg appExec "$APP_EXEC" \
      --arg resultJson "$RESULT_JSON" \
      --arg runLog "$RUN_LOG" \
      --arg attemptStatusTable "$ATTEMPT_TABLE" \
      --arg status "$status" \
      --arg terminalFailureReason "$reason" \
      --arg appPid "$app_pid" \
      --arg appExitCode "$app_exit_code" \
      --arg appSignal "$app_signal" \
      --arg appSignalName "$app_signal_name" \
      --arg appExitStatusSource "$app_exit_status_source" \
      --arg crashReportPath "$crash_report_path" \
      --arg selftestScope "${LOCUSQ_UI_SELFTEST_SCOPE:-}" \
      --arg selftestBl009 "${LOCUSQ_UI_SELFTEST_BL009:-0}" \
      --arg selftestBl011 "${LOCUSQ_UI_SELFTEST_BL011:-0}" \
      --arg timeoutSeconds "$SELFTEST_TIMEOUT_SECONDS" \
      --arg maxAttempts "$MAX_ATTEMPTS" \
      --arg attemptsRun "$attempts_run" \
      --arg launchModeRequested "$LAUNCH_MODE_REQUESTED" \
      --arg launchModeUsed "$LAUNCH_MODE_USED" \
      --arg launchModeFallbackReason "$LAUNCH_MODE_FALLBACK_REASON" \
      --arg lockPath "$LOCK_PATH_REAL" \
      --arg lockWaitSeconds "$LOCK_WAIT_SECONDS" \
      --arg lockWaitResult "$LOCK_WAIT_RESULT" \
      --arg lockWaitPolls "$LOCK_WAIT_POLLS" \
      --arg lockStaleRecovered "$LOCK_STALE_RECOVERED" \
      --arg lockStaleRecoveryReason "$LOCK_STALE_RECOVERY_REASON" \
      --arg prelaunchDrainSeconds "$PRELAUNCH_DRAIN_SECONDS" \
      --arg prelaunchDrainForcedKill "$PRELAUNCH_DRAIN_FORCED_KILL" \
      --arg prelaunchDrainResult "$PRELAUNCH_DRAIN_RESULT" \
      --arg prelaunchDrainRemainingPids "$PRELAUNCH_DRAIN_REMAINING_PIDS" \
      '{
        selftestTs: $selftestTs,
        appExec: $appExec,
        resultJson: $resultJson,
        runLog: $runLog,
        attemptStatusTable: $attemptStatusTable,
        status: $status,
        terminalFailureReason: $terminalFailureReason,
        appPid: $appPid,
        appExitCode: $appExitCode,
        appSignal: $appSignal,
        appSignalName: $appSignalName,
        appExitStatusSource: $appExitStatusSource,
        crashReportPath: $crashReportPath,
        selftestScope: $selftestScope,
        selftestBl009: $selftestBl009,
        selftestBl011: $selftestBl011,
        timeoutSeconds: $timeoutSeconds,
        maxAttempts: $maxAttempts,
        attemptsRun: $attemptsRun,
        launchModeRequested: $launchModeRequested,
        launchModeUsed: $launchModeUsed,
        launchModeFallbackReason: $launchModeFallbackReason,
        lockPath: $lockPath,
        lockWaitSeconds: $lockWaitSeconds,
        lockWaitResult: $lockWaitResult,
        lockWaitPolls: $lockWaitPolls,
        lockStaleRecovered: ($lockStaleRecovered == "1"),
        lockStaleRecoveryReason: $lockStaleRecoveryReason,
        prelaunchDrainSeconds: $prelaunchDrainSeconds,
        prelaunchDrainForcedKill: ($prelaunchDrainForcedKill == "1"),
        prelaunchDrainResult: $prelaunchDrainResult,
        prelaunchDrainRemainingPids: $prelaunchDrainRemainingPids
      }' > "$META_JSON"
  else
    {
      echo "selftest_ts=${TIMESTAMP}"
      echo "app_exec=${APP_EXEC}"
      echo "result_json=${RESULT_JSON}"
      echo "run_log=${RUN_LOG}"
      echo "attempt_status_table=${ATTEMPT_TABLE}"
      echo "status=${status}"
      echo "terminal_failure_reason=${reason}"
      echo "app_pid=${app_pid}"
      echo "app_exit_code=${app_exit_code}"
      echo "app_signal=${app_signal}"
      echo "app_signal_name=${app_signal_name}"
      echo "app_exit_status_source=${app_exit_status_source}"
      echo "crash_report_path=${crash_report_path}"
      echo "selftest_scope=${LOCUSQ_UI_SELFTEST_SCOPE:-}"
      echo "selftest_bl009=${LOCUSQ_UI_SELFTEST_BL009:-0}"
      echo "selftest_bl011=${LOCUSQ_UI_SELFTEST_BL011:-0}"
      echo "timeout_seconds=${SELFTEST_TIMEOUT_SECONDS}"
      echo "max_attempts=${MAX_ATTEMPTS}"
      echo "attempts_run=${attempts_run}"
      echo "launch_mode_requested=${LAUNCH_MODE_REQUESTED}"
      echo "launch_mode_used=${LAUNCH_MODE_USED}"
      echo "launch_mode_fallback_reason=${LAUNCH_MODE_FALLBACK_REASON}"
      echo "lock_path=${LOCK_PATH_REAL}"
      echo "lock_wait_seconds=${LOCK_WAIT_SECONDS}"
      echo "lock_wait_result=${LOCK_WAIT_RESULT}"
      echo "lock_wait_polls=${LOCK_WAIT_POLLS}"
      echo "lock_stale_recovered=${LOCK_STALE_RECOVERED}"
      echo "lock_stale_recovery_reason=${LOCK_STALE_RECOVERY_REASON}"
      echo "prelaunch_drain_seconds=${PRELAUNCH_DRAIN_SECONDS}"
      echo "prelaunch_drain_forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL}"
      echo "prelaunch_drain_result=${PRELAUNCH_DRAIN_RESULT}"
      echo "prelaunch_drain_remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS}"
    } > "$META_JSON"
  fi
}

ensure_parent_dir "$RESULT_JSON"
ensure_parent_dir "$RUN_LOG"
ensure_parent_dir "$ATTEMPT_TABLE"
ensure_parent_dir "$META_JSON"

trap release_single_instance_lock EXIT

if ! acquire_single_instance_lock; then
  FINAL_STATUS="fail"
  FINAL_REASON="single_instance_lock_timeout"
  FINAL_PID=""
  FINAL_EXIT_CODE=""
  FINAL_SIGNAL=""
  FINAL_SIGNAL_NAME=""
  FINAL_CRASH_REPORT=""
  FINAL_EXIT_STATUS_SOURCE="not_observed"
  ATTEMPTS_RUN=0
  write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
  {
    echo "selftest_ts=${TIMESTAMP}"
    echo "status=fail"
    echo "terminal_failure_reason=${FINAL_REASON}"
    echo "lock_path=${LOCK_PATH_REAL}"
    echo "lock_wait_seconds=${LOCK_WAIT_SECONDS}"
    echo "lock_wait_result=${LOCK_WAIT_RESULT}"
    echo "lock_wait_polls=${LOCK_WAIT_POLLS}"
    echo "metadata_json=${META_JSON}"
  } | tee "$RUN_LOG"
  exit 1
fi

# Strict pre-run cleanup for explicit/overridden output paths.
rm -f "$RESULT_JSON" "$RUN_LOG" "$ATTEMPT_TABLE" "$META_JSON"
rm -f "${RESULT_JSON%.json}.attempt"*.json >/dev/null 2>&1 || true
rm -f "${RUN_LOG%.log}.attempt"*.app.log >/dev/null 2>&1 || true

{
  echo "selftest_ts=${TIMESTAMP}"
  echo "app_exec=${APP_EXEC}"
  echo "result_json=${RESULT_JSON}"
  echo "timeout_seconds=${SELFTEST_TIMEOUT_SECONDS}"
  echo "selftest_scope=${LOCUSQ_UI_SELFTEST_SCOPE:-}"
  echo "max_attempts=${MAX_ATTEMPTS}"
  echo "retry_delay_seconds=${RETRY_DELAY_SECONDS}"
  echo "launch_mode_requested=${LAUNCH_MODE_REQUESTED}"
  echo "launch_mode_used=${LAUNCH_MODE_USED}"
  echo "launch_mode_fallback_reason=${LAUNCH_MODE_FALLBACK_REASON}"
  echo "lock_path=${LOCK_PATH_REAL}"
  echo "lock_wait_seconds=${LOCK_WAIT_SECONDS}"
  echo "lock_wait_result=${LOCK_WAIT_RESULT}"
  echo "lock_wait_polls=${LOCK_WAIT_POLLS}"
  echo "lock_stale_recovered=${LOCK_STALE_RECOVERED}"
  echo "lock_stale_recovery_reason=${LOCK_STALE_RECOVERY_REASON}"
  echo "lock_owner_pid=${LOCK_OWNER_PID}"
  echo "lock_owner_age_seconds=${LOCK_OWNER_AGE_SECONDS}"
  echo "attempt_status_table=${ATTEMPT_TABLE}"
  echo "metadata_json=${META_JSON}"
} | tee "$RUN_LOG"

printf "attempt\tstatus\tterminal_failure_reason\tapp_pid\tapp_exit_code\tapp_signal\tapp_signal_name\tresult_wait_seconds\tresult_json\tcrash_report_path\terror_reason\n" > "$ATTEMPT_TABLE"

FINAL_STATUS="fail"
FINAL_REASON="unknown"
FINAL_PID=""
FINAL_EXIT_CODE=""
FINAL_SIGNAL=""
FINAL_SIGNAL_NAME=""
FINAL_CRASH_REPORT=""
FINAL_EXIT_STATUS_SOURCE="not_recorded"
ATTEMPTS_RUN=0

shutdown_locusq
if ! drain_locusq_processes "$PROCESS_DRAIN_TIMEOUT_SECONDS" 1; then
  FINAL_STATUS="fail"
  FINAL_REASON="prelaunch_process_drain_timeout"
  FINAL_EXIT_STATUS_SOURCE="not_observed"
  write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
  {
    echo "prelaunch_drain_result=${PRELAUNCH_DRAIN_RESULT}"
    echo "prelaunch_drain_seconds=${PRELAUNCH_DRAIN_SECONDS}"
    echo "prelaunch_drain_forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL}"
    echo "prelaunch_drain_remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS}"
    echo "terminal_failure_reason=${FINAL_REASON}"
    echo "metadata_json=${META_JSON}"
  } | tee -a "$RUN_LOG"
  exit 1
fi
{
  echo "prelaunch_drain_result=${PRELAUNCH_DRAIN_RESULT}"
  echo "prelaunch_drain_seconds=${PRELAUNCH_DRAIN_SECONDS}"
  echo "prelaunch_drain_forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL}"
} | tee -a "$RUN_LOG"

for (( attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt )); do
  ATTEMPTS_RUN="$attempt"

  if [[ "$RESULT_JSON" == *.json ]]; then
    ATTEMPT_RESULT_JSON="${RESULT_JSON%.json}.attempt${attempt}.json"
  else
    ATTEMPT_RESULT_JSON="${RESULT_JSON}.attempt${attempt}.json"
  fi
  if (( MAX_ATTEMPTS == 1 )); then
    ATTEMPT_RESULT_JSON="$RESULT_JSON"
  fi

  if [[ "$RUN_LOG" == *.log ]]; then
    ATTEMPT_APP_LOG="${RUN_LOG%.log}.attempt${attempt}.app.log"
  else
    ATTEMPT_APP_LOG="${RUN_LOG}.attempt${attempt}.app.log"
  fi

  rm -f "$ATTEMPT_RESULT_JSON" "$ATTEMPT_APP_LOG"

  CRASH_MARKER="$(mktemp /tmp/locusq_selftest_crash_marker.XXXXXX)"

  echo "attempt=${attempt}" | tee -a "$RUN_LOG"
  echo "attempt_result_json=${ATTEMPT_RESULT_JSON}" | tee -a "$RUN_LOG"
  echo "attempt_app_log=${ATTEMPT_APP_LOG}" | tee -a "$RUN_LOG"
  if ! drain_locusq_processes "$PROCESS_DRAIN_TIMEOUT_SECONDS" 1; then
    ATTEMPT_REASON="prelaunch_process_drain_timeout"
    ATTEMPT_STATUS="fail"
    APP_PID=""
    ATTEMPT_EXIT_CODE=""
    ATTEMPT_SIGNAL=""
    ATTEMPT_SIGNAL_NAME=""
    ATTEMPT_CRASH_REPORT=""
    ATTEMPT_ERROR_REASON=""

    echo "prelaunch_drain_result=${PRELAUNCH_DRAIN_RESULT}" | tee -a "$RUN_LOG"
    echo "prelaunch_drain_seconds=${PRELAUNCH_DRAIN_SECONDS}" | tee -a "$RUN_LOG"
    echo "prelaunch_drain_forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL}" | tee -a "$RUN_LOG"
    echo "prelaunch_drain_remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS}" | tee -a "$RUN_LOG"
    echo "terminal_failure_reason=${ATTEMPT_REASON}" | tee -a "$RUN_LOG"

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$attempt" \
      "fail" \
      "$(sanitize_tsv_field "$ATTEMPT_REASON")" \
      "" \
      "" \
      "" \
      "" \
      "0" \
      "$(sanitize_tsv_field "$ATTEMPT_RESULT_JSON")" \
      "" \
      "" >> "$ATTEMPT_TABLE"

    FINAL_STATUS="fail"
    FINAL_REASON="$ATTEMPT_REASON"
    FINAL_PID=""
    FINAL_EXIT_CODE=""
    FINAL_SIGNAL=""
    FINAL_SIGNAL_NAME=""
    FINAL_CRASH_REPORT=""
    FINAL_EXIT_STATUS_SOURCE="not_observed"

    if (( attempt < MAX_ATTEMPTS )); then
      echo "retrying_attempt=$((attempt + 1))" | tee -a "$RUN_LOG"
      sleep "$RETRY_DELAY_SECONDS"
      continue
    fi

    write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
    echo "attempt_status_table=${ATTEMPT_TABLE}" | tee -a "$RUN_LOG"
    echo "metadata_json=${META_JSON}" | tee -a "$RUN_LOG"
    exit 1
  fi

  echo "prelaunch_drain_result=${PRELAUNCH_DRAIN_RESULT}" | tee -a "$RUN_LOG"
  echo "prelaunch_drain_seconds=${PRELAUNCH_DRAIN_SECONDS}" | tee -a "$RUN_LOG"
  echo "prelaunch_drain_forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL}" | tee -a "$RUN_LOG"

  APP_PID=""
  APP_PID_WAITABLE=0
  APP_PID_WAITABILITY_REASON="not_launched"
  if launch_selftest_app "$ATTEMPT_RESULT_JSON" "$ATTEMPT_APP_LOG"; then
    APP_PID="$LAUNCHED_APP_PID"
    APP_PID_WAITABLE="$LAUNCHED_APP_WAITABLE"
    APP_PID_WAITABILITY_REASON="$LAUNCHED_APP_WAITABILITY_REASON"
  fi
  if [[ -z "$APP_PID" ]]; then
    ATTEMPT_REASON="launch_mode_failed_${LAUNCH_MODE_REQUESTED}"
    ATTEMPT_STATUS="fail"
    ATTEMPT_EXIT_CODE=""
    ATTEMPT_SIGNAL=""
    ATTEMPT_SIGNAL_NAME=""
    ATTEMPT_CRASH_REPORT=""
    ATTEMPT_ERROR_REASON="launch_mode_used=${LAUNCH_MODE_USED};fallback_reason=${LAUNCH_MODE_FALLBACK_REASON};waitability=${APP_PID_WAITABILITY_REASON}"

    echo "launch_mode_used=${LAUNCH_MODE_USED}" | tee -a "$RUN_LOG"
    echo "launch_mode_fallback_reason=${LAUNCH_MODE_FALLBACK_REASON}" | tee -a "$RUN_LOG"
    echo "terminal_failure_reason=${ATTEMPT_REASON}" | tee -a "$RUN_LOG"

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$attempt" \
      "fail" \
      "$(sanitize_tsv_field "$ATTEMPT_REASON")" \
      "" \
      "" \
      "" \
      "" \
      "0" \
      "$(sanitize_tsv_field "$ATTEMPT_RESULT_JSON")" \
      "" \
      "$(sanitize_tsv_field "$ATTEMPT_ERROR_REASON")" >> "$ATTEMPT_TABLE"

    FINAL_STATUS="fail"
    FINAL_REASON="$ATTEMPT_REASON"
    FINAL_PID=""
    FINAL_EXIT_CODE=""
    FINAL_SIGNAL=""
    FINAL_SIGNAL_NAME=""
    FINAL_CRASH_REPORT=""
    FINAL_EXIT_STATUS_SOURCE="not_observed"

    if (( attempt < MAX_ATTEMPTS )); then
      echo "retrying_attempt=$((attempt + 1))" | tee -a "$RUN_LOG"
      sleep "$RETRY_DELAY_SECONDS"
      continue
    fi

    write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
    echo "attempt_status_table=${ATTEMPT_TABLE}" | tee -a "$RUN_LOG"
    echo "metadata_json=${META_JSON}" | tee -a "$RUN_LOG"
    exit 1
  fi

  echo "app_pid=${APP_PID}" | tee -a "$RUN_LOG"
  echo "app_waitable=${APP_PID_WAITABLE}" | tee -a "$RUN_LOG"
  echo "app_waitability_reason=${APP_PID_WAITABILITY_REASON}" | tee -a "$RUN_LOG"
  echo "launch_mode_used=${LAUNCH_MODE_USED}" | tee -a "$RUN_LOG"
  echo "launch_mode_fallback_reason=${LAUNCH_MODE_FALLBACK_REASON}" | tee -a "$RUN_LOG"

  ATTEMPT_REASON="none"
  ATTEMPT_EXIT_CODE=""
  ATTEMPT_SIGNAL=""
  ATTEMPT_SIGNAL_NAME=""
  ATTEMPT_CRASH_REPORT=""
  ATTEMPT_EXIT_STATUS_SOURCE="not_observed"
  ATTEMPT_ERROR_REASON=""
  ATTEMPT_STATUS="fail"

  wait_start_seconds=$SECONDS
  deadline=$((SECONDS + SELFTEST_TIMEOUT_SECONDS))
  while [[ ! -f "$ATTEMPT_RESULT_JSON" && $SECONDS -lt $deadline ]]; do
    if ! kill -0 "$APP_PID" >/dev/null 2>&1; then
      break
    fi
    sleep 1
  done
  WAIT_SECONDS=$((SECONDS - wait_start_seconds))

  if [[ ! -f "$ATTEMPT_RESULT_JSON" ]]; then
    if kill -0 "$APP_PID" >/dev/null 2>&1; then
      ATTEMPT_REASON="result_json_missing_after_${SELFTEST_TIMEOUT_SECONDS}s"
    else
      ATTEMPT_REASON="app_exited_before_result"
    fi

    observe_app_exit_status \
      "$APP_PID" \
      "$APP_PID_WAITABLE" \
      ATTEMPT_EXIT_CODE \
      ATTEMPT_SIGNAL \
      ATTEMPT_SIGNAL_NAME \
      ATTEMPT_EXIT_STATUS_SOURCE
    ATTEMPT_CRASH_REPORT="$(find_crash_report_since "$CRASH_MARKER")"

    echo "result_ready=0" | tee -a "$RUN_LOG"
    echo "terminal_failure_reason=${ATTEMPT_REASON}" | tee -a "$RUN_LOG"
    echo "app_exit_status_source=${ATTEMPT_EXIT_STATUS_SOURCE}" | tee -a "$RUN_LOG"
    echo "app_exit_code=${ATTEMPT_EXIT_CODE}" | tee -a "$RUN_LOG"
    if [[ -n "$ATTEMPT_SIGNAL" ]]; then
      echo "app_signal=${ATTEMPT_SIGNAL}" | tee -a "$RUN_LOG"
      echo "app_signal_name=${ATTEMPT_SIGNAL_NAME}" | tee -a "$RUN_LOG"
    fi
    if [[ -n "$ATTEMPT_CRASH_REPORT" ]]; then
      echo "crash_report_path=${ATTEMPT_CRASH_REPORT}" | tee -a "$RUN_LOG"
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$attempt" \
      "fail" \
      "$(sanitize_tsv_field "$ATTEMPT_REASON")" \
      "$(sanitize_tsv_field "$APP_PID")" \
      "$(sanitize_tsv_field "$ATTEMPT_EXIT_CODE")" \
      "$(sanitize_tsv_field "$ATTEMPT_SIGNAL")" \
      "$(sanitize_tsv_field "$ATTEMPT_SIGNAL_NAME")" \
      "$WAIT_SECONDS" \
      "$(sanitize_tsv_field "$ATTEMPT_RESULT_JSON")" \
      "$(sanitize_tsv_field "$ATTEMPT_CRASH_REPORT")" \
      "" >> "$ATTEMPT_TABLE"

    rm -f "$CRASH_MARKER"

    FINAL_STATUS="fail"
    FINAL_REASON="$ATTEMPT_REASON"
    FINAL_PID="$APP_PID"
    FINAL_EXIT_CODE="$ATTEMPT_EXIT_CODE"
    FINAL_SIGNAL="$ATTEMPT_SIGNAL"
    FINAL_SIGNAL_NAME="$ATTEMPT_SIGNAL_NAME"
    FINAL_CRASH_REPORT="$ATTEMPT_CRASH_REPORT"
    FINAL_EXIT_STATUS_SOURCE="$ATTEMPT_EXIT_STATUS_SOURCE"

    shutdown_locusq

    if (( attempt < MAX_ATTEMPTS )); then
      echo "retrying_attempt=$((attempt + 1))" | tee -a "$RUN_LOG"
      sleep "$RETRY_DELAY_SECONDS"
      continue
    fi

    write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
    echo "attempt_status_table=${ATTEMPT_TABLE}" | tee -a "$RUN_LOG"
    echo "metadata_json=${META_JSON}" | tee -a "$RUN_LOG"
    exit 1
  fi

  echo "result_ready=1" | tee -a "$RUN_LOG"
  echo "result_wait_seconds=${WAIT_SECONDS}" | tee -a "$RUN_LOG"

  STATUS="unknown"
  OK="false"
  ERROR_REASON=""
  TIMING_COUNT="0"

  if command -v jq >/dev/null 2>&1; then
    STATUS="$(jq -r '.payload.status // .status // .result.status // "unknown"' "$ATTEMPT_RESULT_JSON" 2>/dev/null || echo unknown)"
    OK="$(jq -r '.payload.ok // .ok // .result.ok // false' "$ATTEMPT_RESULT_JSON" 2>/dev/null || echo false)"
    ERROR_REASON="$(jq -r '.payload.error // .error // .result.error // ""' "$ATTEMPT_RESULT_JSON" 2>/dev/null || true)"
    TIMING_COUNT="$(jq -r '((.payload.timing // .timing // .result.timing // []) | length)' "$ATTEMPT_RESULT_JSON" 2>/dev/null || echo 0)"
    echo "status=${STATUS}" | tee -a "$RUN_LOG"
    echo "ok=${OK}" | tee -a "$RUN_LOG"
    if [[ -n "$ERROR_REASON" ]]; then
      echo "error_reason=${ERROR_REASON}" | tee -a "$RUN_LOG"
    fi
    echo "timing_count=${TIMING_COUNT}" | tee -a "$RUN_LOG"
    if [[ "$TIMING_COUNT" != "0" ]]; then
      echo "timing_steps_begin" | tee -a "$RUN_LOG"
      jq -r '(.payload.timing // .timing // .result.timing // [])[] | "timing_step label=\(.label // "unknown") result=\(.result // "unknown") elapsed_ms=\(.elapsedMs // -1) timeout_ms=\(.timeoutMs // -1) polls=\(.polls // -1)"' "$ATTEMPT_RESULT_JSON" | tee -a "$RUN_LOG" || true
      echo "timing_steps_end" | tee -a "$RUN_LOG"
    fi
  else
    if rg -q '"ok"[[:space:]]*:[[:space:]]*true' "$ATTEMPT_RESULT_JSON"; then
      OK="true"
      STATUS="ok"
    fi
  fi

  if [[ "$OK" == "true" ]]; then
    ATTEMPT_STATUS="pass"
  else
    ATTEMPT_STATUS="fail"
    ATTEMPT_REASON="selftest_payload_not_ok"
    ATTEMPT_ERROR_REASON="$ERROR_REASON"
  fi

  if [[ "$ATTEMPT_STATUS" == "pass" ]]; then
    cleanup_launched_app "$APP_PID" "$APP_PID_WAITABLE"
    ATTEMPT_EXIT_CODE=""
    ATTEMPT_SIGNAL=""
    ATTEMPT_SIGNAL_NAME=""
    ATTEMPT_EXIT_STATUS_SOURCE="not_applicable_pass"
  else
    observe_app_exit_status \
      "$APP_PID" \
      "$APP_PID_WAITABLE" \
      ATTEMPT_EXIT_CODE \
      ATTEMPT_SIGNAL \
      ATTEMPT_SIGNAL_NAME \
      ATTEMPT_EXIT_STATUS_SOURCE
  fi
  ATTEMPT_CRASH_REPORT="$(find_crash_report_since "$CRASH_MARKER")"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$attempt" \
    "$ATTEMPT_STATUS" \
    "$(sanitize_tsv_field "$ATTEMPT_REASON")" \
    "$(sanitize_tsv_field "$APP_PID")" \
    "$(sanitize_tsv_field "$ATTEMPT_EXIT_CODE")" \
    "$(sanitize_tsv_field "$ATTEMPT_SIGNAL")" \
    "$(sanitize_tsv_field "$ATTEMPT_SIGNAL_NAME")" \
    "$WAIT_SECONDS" \
    "$(sanitize_tsv_field "$ATTEMPT_RESULT_JSON")" \
    "$(sanitize_tsv_field "$ATTEMPT_CRASH_REPORT")" \
    "$(sanitize_tsv_field "$ATTEMPT_ERROR_REASON")" >> "$ATTEMPT_TABLE"

  rm -f "$CRASH_MARKER"
  shutdown_locusq

  if [[ "$ATTEMPT_STATUS" == "pass" ]]; then
    if [[ "$ATTEMPT_RESULT_JSON" != "$RESULT_JSON" ]]; then
      cp "$ATTEMPT_RESULT_JSON" "$RESULT_JSON"
    fi
    FINAL_STATUS="pass"
    FINAL_REASON="none"
    FINAL_PID="$APP_PID"
    FINAL_EXIT_CODE="$ATTEMPT_EXIT_CODE"
    FINAL_SIGNAL="$ATTEMPT_SIGNAL"
    FINAL_SIGNAL_NAME="$ATTEMPT_SIGNAL_NAME"
    FINAL_CRASH_REPORT="$ATTEMPT_CRASH_REPORT"
    FINAL_EXIT_STATUS_SOURCE="$ATTEMPT_EXIT_STATUS_SOURCE"
    write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
    echo "app_exit_status_source=${FINAL_EXIT_STATUS_SOURCE}" | tee -a "$RUN_LOG"
    echo "app_exit_code=${FINAL_EXIT_CODE}" | tee -a "$RUN_LOG"
    echo "attempt_status_table=${ATTEMPT_TABLE}" | tee -a "$RUN_LOG"
    echo "metadata_json=${META_JSON}" | tee -a "$RUN_LOG"
    echo "PASS: Production P0 self-test completed." | tee -a "$RUN_LOG"
    echo "artifact=${RESULT_JSON}" | tee -a "$RUN_LOG"
    exit 0
  fi

  echo "terminal_failure_reason=${ATTEMPT_REASON}" | tee -a "$RUN_LOG"
  if [[ -n "$ATTEMPT_ERROR_REASON" ]]; then
    echo "error_reason=${ATTEMPT_ERROR_REASON}" | tee -a "$RUN_LOG"
  fi
  echo "app_exit_status_source=${ATTEMPT_EXIT_STATUS_SOURCE}" | tee -a "$RUN_LOG"
  echo "app_exit_code=${ATTEMPT_EXIT_CODE}" | tee -a "$RUN_LOG"
  if [[ -n "$ATTEMPT_SIGNAL" ]]; then
    echo "app_signal=${ATTEMPT_SIGNAL}" | tee -a "$RUN_LOG"
    echo "app_signal_name=${ATTEMPT_SIGNAL_NAME}" | tee -a "$RUN_LOG"
  fi
  if [[ -n "$ATTEMPT_CRASH_REPORT" ]]; then
    echo "crash_report_path=${ATTEMPT_CRASH_REPORT}" | tee -a "$RUN_LOG"
  fi

  FINAL_STATUS="fail"
  FINAL_REASON="$ATTEMPT_REASON"
  FINAL_PID="$APP_PID"
  FINAL_EXIT_CODE="$ATTEMPT_EXIT_CODE"
  FINAL_SIGNAL="$ATTEMPT_SIGNAL"
  FINAL_SIGNAL_NAME="$ATTEMPT_SIGNAL_NAME"
  FINAL_CRASH_REPORT="$ATTEMPT_CRASH_REPORT"
  FINAL_EXIT_STATUS_SOURCE="$ATTEMPT_EXIT_STATUS_SOURCE"

  if (( attempt < MAX_ATTEMPTS )); then
    echo "retrying_attempt=$((attempt + 1))" | tee -a "$RUN_LOG"
    sleep "$RETRY_DELAY_SECONDS"
    continue
  fi

  write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
  echo "attempt_status_table=${ATTEMPT_TABLE}" | tee -a "$RUN_LOG"
  echo "metadata_json=${META_JSON}" | tee -a "$RUN_LOG"
  exit 1
done

write_metadata_json "$FINAL_STATUS" "$FINAL_REASON" "$FINAL_PID" "$FINAL_EXIT_CODE" "$FINAL_SIGNAL" "$FINAL_SIGNAL_NAME" "$FINAL_CRASH_REPORT" "$ATTEMPTS_RUN" "$FINAL_EXIT_STATUS_SOURCE"
echo "attempt_status_table=${ATTEMPT_TABLE}" | tee -a "$RUN_LOG"
echo "metadata_json=${META_JSON}" | tee -a "$RUN_LOG"
exit 1
