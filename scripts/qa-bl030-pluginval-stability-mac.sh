#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE="$(date -u +%Y-%m-%d)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

PLUGINVAL_BIN_DEFAULT="/Applications/pluginval.app/Contents/MacOS/pluginval"
PLUGIN_PATH_DEFAULT="build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3"
RUNS_DEFAULT=5
TIMEOUT_MS_DEFAULT=30000
MAX_RETRIES_PER_RUN_DEFAULT=0
RETRY_BACKOFF_MS_DEFAULT=250

usage() {
  cat <<'USAGE'
Usage: qa-bl030-pluginval-stability-mac.sh [options]

Deterministic RL-06 pluginval replay harness with machine-readable taxonomy.

Options:
  --runs <N>            Number of pluginval runs (default: 5)
  --out-dir <path>      Output artifact directory
  --plugin-path <path>  Plugin path to validate (default: build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3)
  --pluginval-bin <p>   pluginval executable path (default: /Applications/pluginval.app/Contents/MacOS/pluginval)
  --timeout-ms <N>      pluginval timeout in ms (default: 30000)
  --max-retries-per-run <N>
                        Retry budget per run (default: 0; bounded deterministic retries only)
  --retry-backoff-ms <N>
                        Fixed retry backoff in ms (default: 250)
  --help, -h            Show this help

Outputs:
  status.tsv
  validation_matrix.tsv
  pluginval_runs.tsv
  failure_taxonomy.tsv
  rl06_hardening_notes.md
  command_transcript.log

Compatibility outputs:
  replay_runs.tsv
  harness_contract.md

Exit semantics:
  0 = all required runs passed
  1 = one or more runs failed, or preflight invalid
  2 = invocation/schema error (invalid args)
USAGE
}

die_invocation_error() {
  echo "ERROR: $1" >&2
  exit 2
}

RUNS="$RUNS_DEFAULT"
TIMEOUT_MS="$TIMEOUT_MS_DEFAULT"
MAX_RETRIES_PER_RUN="$MAX_RETRIES_PER_RUN_DEFAULT"
RETRY_BACKOFF_MS="$RETRY_BACKOFF_MS_DEFAULT"
PLUGINVAL_BIN="$PLUGINVAL_BIN_DEFAULT"
PLUGIN_PATH="$PLUGIN_PATH_DEFAULT"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl030_rl06_stability_l3_${TIMESTAMP}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      if [[ $# -lt 2 ]]; then
        die_invocation_error "--runs requires a value"
      fi
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      if [[ $# -lt 2 ]]; then
        die_invocation_error "--out-dir requires a value"
      fi
      OUT_DIR="$2"
      shift 2
      ;;
    --plugin-path)
      if [[ $# -lt 2 ]]; then
        die_invocation_error "--plugin-path requires a value"
      fi
      PLUGIN_PATH="$2"
      shift 2
      ;;
    --pluginval-bin)
      if [[ $# -lt 2 ]]; then
        die_invocation_error "--pluginval-bin requires a value"
      fi
      PLUGINVAL_BIN="$2"
      shift 2
      ;;
    --timeout-ms)
      if [[ $# -lt 2 ]]; then
        die_invocation_error "--timeout-ms requires a value"
      fi
      TIMEOUT_MS="$2"
      shift 2
      ;;
    --max-retries-per-run)
      if [[ $# -lt 2 ]]; then
        die_invocation_error "--max-retries-per-run requires a value"
      fi
      MAX_RETRIES_PER_RUN="$2"
      shift 2
      ;;
    --retry-backoff-ms)
      if [[ $# -lt 2 ]]; then
        die_invocation_error "--retry-backoff-ms requires a value"
      fi
      RETRY_BACKOFF_MS="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      die_invocation_error "unknown argument: $1"
      ;;
  esac
done

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || (( RUNS <= 0 )); then
  die_invocation_error "--runs must be a positive integer"
fi

if ! [[ "$TIMEOUT_MS" =~ ^[0-9]+$ ]] || (( TIMEOUT_MS <= 0 )); then
  die_invocation_error "--timeout-ms must be a positive integer"
fi

if ! [[ "$MAX_RETRIES_PER_RUN" =~ ^[0-9]+$ ]]; then
  die_invocation_error "--max-retries-per-run must be a non-negative integer"
fi

if ! [[ "$RETRY_BACKOFF_MS" =~ ^[0-9]+$ ]]; then
  die_invocation_error "--retry-backoff-ms must be a non-negative integer"
fi

mkdir -p "$OUT_DIR"

MAX_ATTEMPTS_PER_RUN=$((MAX_RETRIES_PER_RUN + 1))
RETRY_BACKOFF_SECONDS="$(awk -v ms="$RETRY_BACKOFF_MS" 'BEGIN { printf "%.3f", ms / 1000.0 }')"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_MATRIX_TSV="${OUT_DIR}/validation_matrix.tsv"
PLUGINVAL_RUNS_TSV="${OUT_DIR}/pluginval_runs.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"
RL06_HARDENING_NOTES_MD="${OUT_DIR}/rl06_hardening_notes.md"
TRANSCRIPT_LOG="${OUT_DIR}/command_transcript.log"

# Backward compatibility artifacts kept for existing consumers.
REPLAY_RUNS_TSV="${OUT_DIR}/replay_runs.tsv"
HARNESS_CONTRACT_MD="${OUT_DIR}/harness_contract.md"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "gate\tresult\texit_code\tcriteria\tartifact\tnotes\n" > "$VALIDATION_MATRIX_TSV"
printf "run_index\ttimestamp_utc\texit_code\tterminal_reason\tterminal_reason_class\tterminal_signal_number\tclassification\tcrash_report_present\tcrash_report_path\tplugin_path\tlog_path\trun_attempts\tmax_attempts\tpluginval_command\n" > "$PLUGINVAL_RUNS_TSV"
printf "run_index\ttimestamp_utc\texit_code\tterminal_reason\tclassification\tcrash_report_present\tcrash_report_path\tplugin_path\tlog_path\n" > "$REPLAY_RUNS_TSV"
printf "dimension\tkey\tcount\n" > "$FAILURE_TAXONOMY_TSV"
: > "$TRANSCRIPT_LOG"

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

log_status() {
  local step="$1"
  local result="$2"
  local exit_code="$3"
  local detail="$4"
  local artifact="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$step")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$STATUS_TSV"
}

log_matrix() {
  local gate="$1"
  local result="$2"
  local exit_code="$3"
  local criteria="$4"
  local artifact="$5"
  local notes="$6"
  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$gate")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$criteria")" \
    "$(sanitize_tsv_field "$artifact")" \
    "$(sanitize_tsv_field "$notes")" \
    >> "$VALIDATION_MATRIX_TSV"
}

log_transcript() {
  local msg="$1"
  printf "[%s] %s\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$msg" >> "$TRANSCRIPT_LOG"
}

signal_number_from_exit_code() {
  local exit_code="$1"
  if [[ "$exit_code" =~ ^[0-9]+$ ]] && (( exit_code >= 129 && exit_code <= 255 )); then
    echo $((exit_code - 128))
  else
    echo 0
  fi
}

classify_reason() {
  local reason_class="$1"
  local exit_code="$2"

  if [[ "$exit_code" == "0" ]]; then
    printf "pass"
    return
  fi

  case "$reason_class" in
    abrt)
      printf "transient_runtime_abort"
      ;;
    signal)
      printf "transient_runtime_signal"
      ;;
    timeout)
      printf "deterministic_timeout"
      ;;
    *)
      printf "deterministic_failure"
      ;;
  esac
}

detect_crash_report_after() {
  local marker_file="$1"
  local newest_path=""
  local newest_mtime=0
  local candidate=""
  local mtime=0

  for crash_dir in "$HOME/Library/Logs/DiagnosticReports" "/Library/Logs/DiagnosticReports"; do
    if [[ ! -d "$crash_dir" ]]; then
      continue
    fi
    while IFS= read -r candidate; do
      mtime="$(stat -f '%m' "$candidate" 2>/dev/null || echo 0)"
      if [[ "$mtime" =~ ^[0-9]+$ ]] && (( mtime > newest_mtime )); then
        newest_mtime="$mtime"
        newest_path="$candidate"
      fi
    done < <(find "$crash_dir" -maxdepth 1 -type f \( -name '*pluginval*.crash' -o -name '*pluginval*.ips' \) -newer "$marker_file" 2>/dev/null | sort)
  done

  printf '%s' "$newest_path"
}

append_run() {
  local run_index="$1"
  local exit_code="$2"
  local reason="$3"
  local reason_class="$4"
  local signal_number="$5"
  local classification="$6"
  local crash_present="$7"
  local crash_path="$8"
  local plugin_path="$9"
  local log_path="${10}"
  local run_attempts="${11}"
  local max_attempts="${12}"
  local pluginval_command="${13}"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$reason")" \
    "$(sanitize_tsv_field "$reason_class")" \
    "$(sanitize_tsv_field "$signal_number")" \
    "$(sanitize_tsv_field "$classification")" \
    "$(sanitize_tsv_field "$crash_present")" \
    "$(sanitize_tsv_field "$crash_path")" \
    "$(sanitize_tsv_field "$plugin_path")" \
    "$(sanitize_tsv_field "$log_path")" \
    "$(sanitize_tsv_field "$run_attempts")" \
    "$(sanitize_tsv_field "$max_attempts")" \
    "$(sanitize_tsv_field "$pluginval_command")" \
    >> "$PLUGINVAL_RUNS_TSV"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$reason")" \
    "$(sanitize_tsv_field "$classification")" \
    "$(sanitize_tsv_field "$crash_present")" \
    "$(sanitize_tsv_field "$crash_path")" \
    "$(sanitize_tsv_field "$plugin_path")" \
    "$(sanitize_tsv_field "$log_path")" \
    >> "$REPLAY_RUNS_TSV"
}

PLUGIN_PATH_RESOLVED="$PLUGIN_PATH"
if [[ "$PLUGIN_PATH_RESOLVED" != /* ]]; then
  PLUGIN_PATH_RESOLVED="${ROOT_DIR}/${PLUGIN_PATH_RESOLVED}"
fi

PRECHECK_FAIL=0
if [[ ! -x "$PLUGINVAL_BIN" ]]; then
  log_status "preflight_pluginval_bin" "FAIL" "1" "pluginval_executable_not_found_or_not_executable" "$PLUGINVAL_BIN"
  log_matrix "preflight_pluginval_bin" "FAIL" "1" "pluginval executable exists and is executable" "$PLUGINVAL_BIN" "missing_or_not_executable"
  PRECHECK_FAIL=1
else
  log_status "preflight_pluginval_bin" "PASS" "0" "pluginval_executable_found" "$PLUGINVAL_BIN"
  log_matrix "preflight_pluginval_bin" "PASS" "0" "pluginval executable exists and is executable" "$PLUGINVAL_BIN" "ok"
fi

if [[ ! -e "$PLUGIN_PATH_RESOLVED" ]]; then
  log_status "preflight_plugin_path" "FAIL" "1" "plugin_path_not_found" "$PLUGIN_PATH_RESOLVED"
  log_matrix "preflight_plugin_path" "FAIL" "1" "plugin path exists" "$PLUGIN_PATH_RESOLVED" "missing"
  PRECHECK_FAIL=1
else
  log_status "preflight_plugin_path" "PASS" "0" "plugin_path_found" "$PLUGIN_PATH_RESOLVED"
  log_matrix "preflight_plugin_path" "PASS" "0" "plugin path exists" "$PLUGIN_PATH_RESOLVED" "ok"
fi

if (( PRECHECK_FAIL == 1 )); then
  log_status "pluginval_replay" "FAIL" "1" "skipped_due_to_preflight_failure" "$PLUGINVAL_RUNS_TSV"
  log_matrix "rl06_stability_threshold" "FAIL" "1" "all_runs_pass (passes==runs)" "$PLUGINVAL_RUNS_TSV" "preflight_failure"
  printf "preflight\tfailed\t1\n" >> "$FAILURE_TAXONOMY_TSV"

  {
    echo "Title: BL-030 RL-06 Pluginval Stability Hardening Notes"
    echo "Document Type: Testing Runbook"
    echo "Author: APC Codex"
    echo "Created Date: ${DOC_DATE}"
    echo "Last Modified Date: ${DOC_DATE}"
    echo
    echo "# BL-030 RL-06 Pluginval Stability Hardening"
    echo
    echo "- overall_result: FAIL"
    echo "- reason: preflight_failure"
    echo "- runs_requested: ${RUNS}"
    echo "- runs_passed: 0"
    echo "- runs_failed: 0"
    echo "- threshold: all_runs_pass"
    echo "- evaluated_at: ${DOC_TS}"
  } > "$RL06_HARDENING_NOTES_MD"

  cp "$RL06_HARDENING_NOTES_MD" "$HARNESS_CONTRACT_MD"

  echo "artifact_dir=$OUT_DIR"
  echo "status_tsv=$STATUS_TSV"
  echo "validation_matrix_tsv=$VALIDATION_MATRIX_TSV"
  echo "pluginval_runs_tsv=$PLUGINVAL_RUNS_TSV"
  echo "failure_taxonomy_tsv=$FAILURE_TAXONOMY_TSV"
  echo "rl06_hardening_notes_md=$RL06_HARDENING_NOTES_MD"
  echo "replay_runs_tsv=$REPLAY_RUNS_TSV"
  echo "harness_contract_md=$HARNESS_CONTRACT_MD"
  exit 1
fi

RUN_FAILS=0
RUN_PASSES=0

log_status "retry_policy" "PASS" "0" "max_retries_per_run=${MAX_RETRIES_PER_RUN};max_attempts_per_run=${MAX_ATTEMPTS_PER_RUN};retry_backoff_ms=${RETRY_BACKOFF_MS}" "$PLUGINVAL_RUNS_TSV"
log_matrix "retry_policy" "PASS" "0" "deterministic_bounded_retry_policy" "$PLUGINVAL_RUNS_TSV" "max_retries_per_run=${MAX_RETRIES_PER_RUN};retry_backoff_ms=${RETRY_BACKOFF_MS}"

for run_idx in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run_idx}.log"
  marker_file="${OUT_DIR}/run_${run_idx}.marker"
  touch "$marker_file"
  printf -v pluginval_command '%q ' \
    "$PLUGINVAL_BIN" \
    --strictness-level 5 \
    --validate-in-process \
    --skip-gui-tests \
    --timeout-ms "$TIMEOUT_MS" \
    "$PLUGIN_PATH_RESOLVED"
  pluginval_command="${pluginval_command% }"

  run_ec=0
  run_reason="pass"
  run_reason_class="pass"
  run_signal_number=0
  run_attempts=0
  crash_path=""
  crash_present="no"
  while (( run_attempts < MAX_ATTEMPTS_PER_RUN )); do
    run_attempts=$((run_attempts + 1))
    attempt_log="${OUT_DIR}/run_${run_idx}.attempt_${run_attempts}.log"

    log_transcript "run_${run_idx}.attempt_${run_attempts}/${MAX_ATTEMPTS_PER_RUN}: ${pluginval_command}"

    set +e
    "$PLUGINVAL_BIN" \
      --strictness-level 5 \
      --validate-in-process \
      --skip-gui-tests \
      --timeout-ms "$TIMEOUT_MS" \
      "$PLUGIN_PATH_RESOLVED" \
      > "$attempt_log" 2>&1
    run_ec=$?
    set -e

    cp "$attempt_log" "$run_log"

    run_reason="pass"
    run_reason_class="pass"
    run_signal_number=0

    if (( run_ec != 0 )); then
      run_signal_number="$(signal_number_from_exit_code "$run_ec")"

      if [[ "$run_signal_number" == "6" ]] || grep -Eiq 'Abort trap|ABRT|SIGABRT|signal 6|exit 134' "$attempt_log"; then
        run_reason="pluginval_failed_abrt"
        run_reason_class="abrt"
        if [[ "$run_signal_number" == "0" ]]; then
          run_signal_number=6
        fi
      elif grep -Eiq 'timeout|timed out' "$attempt_log"; then
        run_reason="pluginval_failed_timeout"
        run_reason_class="timeout"
      elif [[ "$run_signal_number" != "0" ]]; then
        run_reason="pluginval_failed_signal"
        run_reason_class="signal"
      else
        run_reason="pluginval_failed_exit"
        run_reason_class="exit"
      fi
    fi

    attempt_crash_path="$(detect_crash_report_after "$marker_file")"
    if [[ -n "$attempt_crash_path" ]]; then
      crash_path="$attempt_crash_path"
      crash_present="yes"
    fi

    log_transcript "run_${run_idx}.attempt_${run_attempts}: exit_code=${run_ec};terminal_reason=${run_reason};terminal_reason_class=${run_reason_class};terminal_signal_number=${run_signal_number};crash_report_present=${crash_present}"

    if (( run_ec == 0 )); then
      break
    fi
    if (( run_attempts < MAX_ATTEMPTS_PER_RUN && RETRY_BACKOFF_MS > 0 )); then
      log_transcript "run_${run_idx}.attempt_${run_attempts}: retry_backoff_ms=${RETRY_BACKOFF_MS}"
      sleep "$RETRY_BACKOFF_SECONDS"
    fi
  done

  run_classification="$(classify_reason "$run_reason_class" "$run_ec")"

  append_run \
    "$run_idx" \
    "$run_ec" \
    "$run_reason" \
    "$run_reason_class" \
    "$run_signal_number" \
    "$run_classification" \
    "$crash_present" \
    "$crash_path" \
    "$PLUGIN_PATH_RESOLVED" \
    "$run_log" \
    "$run_attempts" \
    "$MAX_ATTEMPTS_PER_RUN" \
    "$pluginval_command"

  if (( run_ec == 0 )); then
    RUN_PASSES=$((RUN_PASSES + 1))
  else
    RUN_FAILS=$((RUN_FAILS + 1))
  fi
done

if (( RUN_FAILS == 0 )); then
  OVERALL_RESULT="PASS"
  EXIT_CODE=0
  log_status "pluginval_replay" "PASS" "0" "runs=${RUNS};passes=${RUN_PASSES};fails=${RUN_FAILS}" "$PLUGINVAL_RUNS_TSV"
  log_matrix "rl06_stability_threshold" "PASS" "0" "all_runs_pass (passes==runs)" "$PLUGINVAL_RUNS_TSV" "passes=${RUN_PASSES};fails=${RUN_FAILS}"
else
  OVERALL_RESULT="FAIL"
  EXIT_CODE=1
  log_status "pluginval_replay" "FAIL" "1" "runs=${RUNS};passes=${RUN_PASSES};fails=${RUN_FAILS}" "$PLUGINVAL_RUNS_TSV"
  log_matrix "rl06_stability_threshold" "FAIL" "1" "all_runs_pass (passes==runs)" "$PLUGINVAL_RUNS_TSV" "passes=${RUN_PASSES};fails=${RUN_FAILS}"
fi

awk -F'\t' '
  NR == 1 { next }
  {
    reason[$4]++
    reasonClass[$5]++
    signal[$6]++
    class[$7]++
    crash[$8]++
    exitc[$3]++
    attempts[$12]++
  }
  END {
    for (k in reason) printf "terminal_reason\t%s\t%d\n", k, reason[k]
    for (k in reasonClass) printf "terminal_reason_class\t%s\t%d\n", k, reasonClass[k]
    for (k in signal) printf "terminal_signal_number\t%s\t%d\n", k, signal[k]
    for (k in class) printf "classification\t%s\t%d\n", k, class[k]
    for (k in crash) printf "crash_report_present\t%s\t%d\n", k, crash[k]
    for (k in exitc) printf "exit_code\t%s\t%d\n", k, exitc[k]
    for (k in attempts) printf "run_attempts\t%s\t%d\n", k, attempts[k]
  }
' "$PLUGINVAL_RUNS_TSV" | sort >> "$FAILURE_TAXONOMY_TSV"

log_status "failure_taxonomy" "PASS" "0" "taxonomy_rows=$(($(wc -l < "$FAILURE_TAXONOMY_TSV") - 1))" "$FAILURE_TAXONOMY_TSV"
log_matrix "failure_taxonomy" "PASS" "0" "failure taxonomy emitted" "$FAILURE_TAXONOMY_TSV" "ok"

{
  echo "Title: BL-030 RL-06 Pluginval Stability Hardening Notes"
  echo "Document Type: Testing Runbook"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE}"
  echo "Last Modified Date: ${DOC_DATE}"
  echo
  echo "# BL-030 RL-06 Pluginval Stability Hardening"
  echo
  echo "- overall_result: ${OVERALL_RESULT}"
  echo "- runs_requested: ${RUNS}"
  echo "- runs_passed: ${RUN_PASSES}"
  echo "- runs_failed: ${RUN_FAILS}"
  echo "- pass_threshold: all_runs_pass (passes==runs)"
  echo "- pluginval_bin: ${PLUGINVAL_BIN}"
  echo "- plugin_path: ${PLUGIN_PATH_RESOLVED}"
  echo "- timeout_ms: ${TIMEOUT_MS}"
  echo "- max_retries_per_run: ${MAX_RETRIES_PER_RUN}"
  echo "- max_attempts_per_run: ${MAX_ATTEMPTS_PER_RUN}"
  echo "- retry_backoff_ms: ${RETRY_BACKOFF_MS}"
  echo "- evaluated_at: ${DOC_TS}"
  echo
  echo "## Machine-Readable Artifacts"
  echo "- status.tsv"
  echo "- validation_matrix.tsv"
  echo "- pluginval_runs.tsv"
  echo "- failure_taxonomy.tsv"
  echo
  echo "## Compatibility Artifacts"
  echo "- replay_runs.tsv"
  echo "- harness_contract.md"
} > "$RL06_HARDENING_NOTES_MD"

cp "$RL06_HARDENING_NOTES_MD" "$HARNESS_CONTRACT_MD"

log_status "rl06_hardening_notes" "PASS" "0" "notes_written" "$RL06_HARDENING_NOTES_MD"
log_matrix "rl06_hardening_notes" "PASS" "0" "hardening notes emitted" "$RL06_HARDENING_NOTES_MD" "ok"

echo "artifact_dir=$OUT_DIR"
echo "status_tsv=$STATUS_TSV"
echo "validation_matrix_tsv=$VALIDATION_MATRIX_TSV"
echo "pluginval_runs_tsv=$PLUGINVAL_RUNS_TSV"
echo "failure_taxonomy_tsv=$FAILURE_TAXONOMY_TSV"
echo "rl06_hardening_notes_md=$RL06_HARDENING_NOTES_MD"
echo "replay_runs_tsv=$REPLAY_RUNS_TSV"
echo "harness_contract_md=$HARNESS_CONTRACT_MD"

exit "$EXIT_CODE"
