#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
DOC_DATE="$(date -u +%Y-%m-%d)"

usage() {
  cat <<'USAGE'
Usage: qa-bl030-device-matrix-capture-mac.sh [options]

Deterministic RL-05 device matrix capture harness for DEV-01..DEV-06.

Options:
  --out-dir <path>              Output artifact directory.
                                Default: TestEvidence/bl030_rl05_harness_g3_<timestamp>
  --dev01-manual-notes <path>   Manual evidence path for DEV-01.
  --dev02-manual-notes <path>   Manual evidence path for DEV-02.
  --dev03-manual-notes <path>   Manual evidence path for DEV-03.
  --dev04-manual-notes <path>   Manual evidence path for DEV-04.
  --dev05-manual-notes <path>   Manual evidence path for DEV-05.
  --dev06-manual-notes <path>   Manual evidence path for DEV-06 when not waived.
  --dev06-waiver <path>         DEV-06 waiver path. If present, DEV-06 records N/A.
  --skip-build                  Skip preflight build.
  --help, -h                    Print this help.

Artifacts:
  status.tsv
  dev_matrix_results.tsv
  blocker_taxonomy.tsv
  replay_transcript.log
  command_transcript.log (compat)
  harness_contract.md

Blocker categories:
  deterministic_missing_manual_evidence
  runtime_flake_abrt
  not_applicable_with_waiver

Deterministic preflight contract:
- Bounded stale-process drain executes before lane steps:
  - TERM window: 4 seconds
  - KILL window: 4 seconds
  - Poll interval: 1 second
- Warmup pause executes after drain:
  - warmup sleep: 2 seconds
- Deterministic selftest warmup bootstrap executes after successful build:
  - command: standalone selftest with BL-009 profile enabled
  - max attempts: 3
  - retry barrier between attempts: bounded drain + 2 second warmup
- Drain timeout is a hard RL-05 fail (no lane replay execution).

DEV-06 waiver preflight reason codes:
  dev06_waiver_path_valid
  dev06_waiver_path_missing
  dev06_waiver_path_not_file
  dev06_waiver_path_unreadable
  dev06_waiver_path_empty

Exit semantics:
  0  RL-05 PASS (DEV-01..DEV-05 PASS and DEV-06 PASS or N/A with waiver)
  1  RL-05 FAIL
  2  Usage/invocation error
USAGE
}

OUT_DIR="${BL030_RL05_OUT_DIR:-$ROOT_DIR/TestEvidence/bl030_rl05_device_matrix_capture_${TIMESTAMP}}"
DEV01_MANUAL_NOTES=""
DEV02_MANUAL_NOTES=""
DEV03_MANUAL_NOTES=""
DEV04_MANUAL_NOTES=""
DEV05_MANUAL_NOTES=""
DEV06_MANUAL_NOTES=""
DEV06_WAIVER=""
SKIP_BUILD=0
OVERALL="FAIL"
EXIT_CODE=1
PRELAUNCH_DRAIN_TIMEOUT_SECONDS=8
PRELAUNCH_DRAIN_TERM_WINDOW_SECONDS=4
PRELAUNCH_DRAIN_KILL_WINDOW_SECONDS=4
PRELAUNCH_DRAIN_POLL_SECONDS=1
PRELAUNCH_WARMUP_SECONDS=2
COMMAND_RETRY_MAX_ATTEMPTS=2
COMMAND_RETRY_SLEEP_SECONDS=2
SELFTEST_WARMUP_MAX_ATTEMPTS=3
SELFTEST_WARMUP_RETRY_SLEEP_SECONDS=2
SELFTEST_WARMUP_TIMEOUT_SECONDS=90
SELFTEST_LAUNCH_READY_DELAY_SECONDS=2
SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS=18
SELFTEST_RESULT_JSON_SETTLE_TIMEOUT_SECONDS=4
SELFTEST_AUTO_ASSERTION_RETRY_MAX_ATTEMPTS=3
SELFTEST_AUTO_ASSERTION_RETRY_DELAY_SECONDS=2
SELFTEST_TARGETED_CHECK_MAX_ATTEMPTS=6
PRELAUNCH_DRAIN_RESULT="not_started"
PRELAUNCH_DRAIN_FORCED_KILL=0
PRELAUNCH_DRAIN_INITIAL_PIDS="none"
PRELAUNCH_DRAIN_REMAINING_PIDS="none"
PRELAUNCH_DRAIN_ELAPSED_SECONDS=0
CURRENT_STEP="initialization"
SIGNAL_TERMINATION_HANDLED=0
RL05_GATE_EMITTED=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --out-dir requires a value" >&2; usage; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    --dev01-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev01-manual-notes requires a value" >&2; usage; exit 2; }
      DEV01_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev02-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev02-manual-notes requires a value" >&2; usage; exit 2; }
      DEV02_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev03-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev03-manual-notes requires a value" >&2; usage; exit 2; }
      DEV03_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev04-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev04-manual-notes requires a value" >&2; usage; exit 2; }
      DEV04_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev05-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev05-manual-notes requires a value" >&2; usage; exit 2; }
      DEV05_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev06-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev06-manual-notes requires a value" >&2; usage; exit 2; }
      DEV06_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev06-waiver)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev06-waiver requires a value" >&2; usage; exit 2; }
      DEV06_WAIVER="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

mkdir -p "$OUT_DIR"

if [[ -z "$DEV01_MANUAL_NOTES" ]]; then DEV01_MANUAL_NOTES="$OUT_DIR/dev01_quad_manual_notes.md"; fi
if [[ -z "$DEV02_MANUAL_NOTES" ]]; then DEV02_MANUAL_NOTES="$OUT_DIR/dev02_laptop_manual_notes.md"; fi
if [[ -z "$DEV03_MANUAL_NOTES" ]]; then DEV03_MANUAL_NOTES="$OUT_DIR/dev03_headphone_generic_manual_notes.md"; fi
if [[ -z "$DEV04_MANUAL_NOTES" ]]; then DEV04_MANUAL_NOTES="$OUT_DIR/dev04_steam_manual_notes.md"; fi
if [[ -z "$DEV05_MANUAL_NOTES" ]]; then DEV05_MANUAL_NOTES="$OUT_DIR/dev05_builtin_mic_manual_notes.md"; fi
if [[ -z "$DEV06_MANUAL_NOTES" ]]; then DEV06_MANUAL_NOTES="$OUT_DIR/dev06_external_mic_manual_notes.md"; fi

STATUS_TSV="$OUT_DIR/status.tsv"
DEV_MATRIX_TSV="$OUT_DIR/dev_matrix_results.tsv"
BLOCKER_TSV="$OUT_DIR/blocker_taxonomy.tsv"
HARNESS_CONTRACT_MD="$OUT_DIR/harness_contract.md"
TRANSCRIPT_LOG="$OUT_DIR/replay_transcript.log"
LEGACY_TRANSCRIPT_LOG="$OUT_DIR/command_transcript.log"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "dev_id\tresult\ttimestamp_utc\tclassification\tevidence_path\tnotes\n" > "$DEV_MATRIX_TSV"
printf "blocker_id\tdev_id\tcategory\tdetail\tevidence_path\n" > "$BLOCKER_TSV"
: > "$TRANSCRIPT_LOG"
: > "$LEGACY_TRANSCRIPT_LOG"

SELFTEST_ENV_OVERRIDES=(
  "LOCUSQ_UI_SELFTEST_LAUNCH_READY_DELAY_SECONDS=${SELFTEST_LAUNCH_READY_DELAY_SECONDS}"
  "LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS=${SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS}"
  "LOCUSQ_UI_SELFTEST_RESULT_JSON_SETTLE_TIMEOUT_SECONDS=${SELFTEST_RESULT_JSON_SETTLE_TIMEOUT_SECONDS}"
  "LOCUSQ_UI_SELFTEST_AUTO_ASSERTION_RETRY_MAX_ATTEMPTS=${SELFTEST_AUTO_ASSERTION_RETRY_MAX_ATTEMPTS}"
  "LOCUSQ_UI_SELFTEST_AUTO_ASSERTION_RETRY_DELAY_SECONDS=${SELFTEST_AUTO_ASSERTION_RETRY_DELAY_SECONDS}"
  "LOCUSQ_UI_SELFTEST_TARGETED_CHECK_MAX_ATTEMPTS=${SELFTEST_TARGETED_CHECK_MAX_ATTEMPTS}"
)

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

record_gate_decision() {
  local result="$1"
  local exit_code="$2"
  local detail="$3"
  local artifact="$4"
  RL05_GATE_EMITTED=1
  log_status "rl05_gate_decision" "$result" "$exit_code" "$detail" "$artifact"
}

log_transcript() {
  local message="$1"
  local line
  line="[$(date -u +%Y-%m-%dT%H:%M:%SZ)] $message"
  printf "%s\n" "$line" >> "$TRANSCRIPT_LOG"
  printf "%s\n" "$line" >> "$LEGACY_TRANSCRIPT_LOG"
}

BLOCKER_SEQ=0
add_blocker() {
  local dev_id="$1"
  local category="$2"
  local detail="$3"
  local artifact="$4"
  BLOCKER_SEQ=$((BLOCKER_SEQ + 1))
  printf "BL030-G3-%03d\t%s\t%s\t%s\t%s\n" \
    "$BLOCKER_SEQ" \
    "$(sanitize_tsv_field "$dev_id")" \
    "$(sanitize_tsv_field "$category")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$BLOCKER_TSV"
}

append_dev_result() {
  local dev_id="$1"
  local result="$2"
  local classification="$3"
  local evidence_path="$4"
  local notes="$5"
  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$dev_id" \
    "$result" \
    "$DOC_TS" \
    "$(sanitize_tsv_field "$classification")" \
    "$(sanitize_tsv_field "$evidence_path")" \
    "$(sanitize_tsv_field "$notes")" \
    >> "$DEV_MATRIX_TSV"
}

join_csv() {
  if (( $# == 0 )); then
    printf '%s' "none"
    return
  fi
  local first=1
  local output=""
  local item
  for item in "$@"; do
    if (( first == 1 )); then
      output="$item"
      first=0
    else
      output="${output},${item}"
    fi
  done
  printf '%s' "$output"
}

collect_capture_preflight_pids() {
  local -a pids=()
  local pid=""
  local current_pid parent_pid
  current_pid="${BASHPID:-$$}"
  parent_pid="${PPID:-0}"
  while IFS= read -r pid; do
    [[ "$pid" =~ ^[0-9]+$ ]] || continue
    if [[ "$pid" -ne "$$" && "$pid" -ne "$current_pid" && "$pid" -ne "$parent_pid" ]]; then
      pids+=("$pid")
    fi
  done < <(pgrep -f "LocusQ_artefacts/.*/Standalone/LocusQ|qa-bl018-profile-matrix-strict-mac.sh|qa-bl009-headphone-contract-mac.sh|qa-bl009-headphone-profile-contract-mac.sh|standalone-ui-selftest-production-p0-mac.sh|reaper-headless-render-smoke-mac.sh|qa-bl030-device-matrix-capture-mac.sh" || true)

  if (( ${#pids[@]} == 0 )); then
    return 1
  fi

  printf '%s\n' "${pids[@]}" | awk '!seen[$0]++'
}

alive_pids_from_list() {
  local -a input_pids=("$@")
  local pid=""
  for pid in "${input_pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      printf '%s\n' "$pid"
    fi
  done
}

drain_capture_preflight_processes() {
  local start_epoch now_epoch term_deadline kill_deadline
  local -a tracked_pids=()
  local -a alive_pids=()

  start_epoch="$(date +%s)"
  while IFS= read -r now_epoch; do
    [[ -n "$now_epoch" ]] || continue
    tracked_pids+=("$now_epoch")
  done < <(collect_capture_preflight_pids || true)

  if (( ${#tracked_pids[@]} == 0 )); then
    PRELAUNCH_DRAIN_RESULT="no_stale_processes"
    PRELAUNCH_DRAIN_INITIAL_PIDS="none"
    PRELAUNCH_DRAIN_REMAINING_PIDS="none"
    PRELAUNCH_DRAIN_ELAPSED_SECONDS=0
    return 0
  fi

  PRELAUNCH_DRAIN_INITIAL_PIDS="$(join_csv "${tracked_pids[@]}")"
  PRELAUNCH_DRAIN_RESULT="term_sent"
  PRELAUNCH_DRAIN_FORCED_KILL=0
  kill -TERM "${tracked_pids[@]}" 2>/dev/null || true

  term_deadline=$((start_epoch + PRELAUNCH_DRAIN_TERM_WINDOW_SECONDS))
  while :; do
    alive_pids=()
    while IFS= read -r now_epoch; do
      [[ -n "$now_epoch" ]] || continue
      alive_pids+=("$now_epoch")
    done < <(alive_pids_from_list "${tracked_pids[@]}")
    if (( ${#alive_pids[@]} == 0 )); then
      now_epoch="$(date +%s)"
      PRELAUNCH_DRAIN_RESULT="drained"
      PRELAUNCH_DRAIN_REMAINING_PIDS="none"
      PRELAUNCH_DRAIN_ELAPSED_SECONDS=$((now_epoch - start_epoch))
      return 0
    fi
    now_epoch="$(date +%s)"
    if (( now_epoch >= term_deadline )); then
      break
    fi
    sleep "$PRELAUNCH_DRAIN_POLL_SECONDS"
  done

  PRELAUNCH_DRAIN_RESULT="kill_sent"
  PRELAUNCH_DRAIN_FORCED_KILL=1
  kill -KILL "${alive_pids[@]}" 2>/dev/null || true

  now_epoch="$(date +%s)"
  kill_deadline=$((now_epoch + PRELAUNCH_DRAIN_KILL_WINDOW_SECONDS))
  while :; do
    alive_pids=()
    while IFS= read -r now_epoch; do
      [[ -n "$now_epoch" ]] || continue
      alive_pids+=("$now_epoch")
    done < <(alive_pids_from_list "${tracked_pids[@]}")
    if (( ${#alive_pids[@]} == 0 )); then
      now_epoch="$(date +%s)"
      PRELAUNCH_DRAIN_RESULT="drained"
      PRELAUNCH_DRAIN_REMAINING_PIDS="none"
      PRELAUNCH_DRAIN_ELAPSED_SECONDS=$((now_epoch - start_epoch))
      return 0
    fi
    now_epoch="$(date +%s)"
    if (( now_epoch >= kill_deadline )); then
      break
    fi
    sleep "$PRELAUNCH_DRAIN_POLL_SECONDS"
  done

  now_epoch="$(date +%s)"
  PRELAUNCH_DRAIN_RESULT="timeout"
  PRELAUNCH_DRAIN_REMAINING_PIDS="$(join_csv "${alive_pids[@]}")"
  PRELAUNCH_DRAIN_ELAPSED_SECONDS=$((now_epoch - start_epoch))
  return 1
}

run_preflight_drain_and_warmup() {
  CURRENT_STEP="preflight_process_drain"
  if drain_capture_preflight_processes; then
    log_status "preflight_process_drain" "PASS" "0" \
      "result=${PRELAUNCH_DRAIN_RESULT};initial_pids=${PRELAUNCH_DRAIN_INITIAL_PIDS};remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS};forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL};elapsed_seconds=${PRELAUNCH_DRAIN_ELAPSED_SECONDS};timeout_seconds=${PRELAUNCH_DRAIN_TIMEOUT_SECONDS}" \
      "$OUT_DIR"
  else
    log_status "preflight_process_drain" "FAIL" "1" \
      "result=${PRELAUNCH_DRAIN_RESULT};initial_pids=${PRELAUNCH_DRAIN_INITIAL_PIDS};remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS};forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL};elapsed_seconds=${PRELAUNCH_DRAIN_ELAPSED_SECONDS};timeout_seconds=${PRELAUNCH_DRAIN_TIMEOUT_SECONDS}" \
      "$OUT_DIR"
    return 1
  fi

  CURRENT_STEP="preflight_warmup"
  sleep "$PRELAUNCH_WARMUP_SECONDS"
  log_status "preflight_warmup" "PASS" "0" "warmup_sleep_seconds=${PRELAUNCH_WARMUP_SECONDS}" "$OUT_DIR"
  return 0
}

run_retry_recovery_barrier() {
  local context="$1"
  local warmup_sleep_seconds="${2:-$COMMAND_RETRY_SLEEP_SECONDS}"
  local barrier_detail
  CURRENT_STEP="retry_recovery_barrier:${context}"
  if ! drain_capture_preflight_processes; then
    barrier_detail="result=${PRELAUNCH_DRAIN_RESULT};remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS};forced_kill=${PRELAUNCH_DRAIN_FORCED_KILL};elapsed_seconds=${PRELAUNCH_DRAIN_ELAPSED_SECONDS}"
    log_transcript "retry_barrier[$context]=FAIL;$barrier_detail"
    return 1
  fi
  sleep "$warmup_sleep_seconds"
  barrier_detail="result=${PRELAUNCH_DRAIN_RESULT};warmup_sleep_seconds=${warmup_sleep_seconds};remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS}"
  log_transcript "retry_barrier[$context]=PASS;$barrier_detail"
  return 0
}

run_preflight_selftest_warmup() {
  local warmup_log="$OUT_DIR/preflight_selftest_warmup.log"
  local attempt=1
  local ec=1
  local attempt_log=""
  local terminal_failure_reason="none"
  local payload_reason_code="none"
  local payload_check="none"
  : > "$warmup_log"
  CURRENT_STEP="preflight_selftest_warmup"

  while (( attempt <= SELFTEST_WARMUP_MAX_ATTEMPTS )); do
    CURRENT_STEP="preflight_selftest_warmup:attempt_${attempt}"
    attempt_log="${warmup_log}.attempt_${attempt}"
    log_transcript "preflight_selftest_warmup[attempt=$attempt]=start"

    set +e
    env \
      "${SELFTEST_ENV_OVERRIDES[@]}" \
      LOCUSQ_UI_SELFTEST_BL009=1 \
      LOCUSQ_UI_SELFTEST_SCOPE=bl009 \
      LOCUSQ_UI_SELFTEST_TIMEOUT_SECONDS="${SELFTEST_WARMUP_TIMEOUT_SECONDS}" \
      "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" \
      "$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app" \
      > "$attempt_log" 2>&1
    ec=$?
    set -e

    cat "$attempt_log" >> "$warmup_log"
    terminal_failure_reason="$(awk -F= '/^terminal_failure_reason=/{print $2}' "$attempt_log" | tail -n 1)"
    payload_reason_code="$(awk -F= '/^payload_failure_reason_code=/{print $2}' "$attempt_log" | tail -n 1)"
    payload_check="$(awk -F= '/^payload_failure_check=/{print $2}' "$attempt_log" | tail -n 1)"
    rm -f "$attempt_log"

    if [[ -z "$terminal_failure_reason" ]]; then terminal_failure_reason="none"; fi
    if [[ -z "$payload_reason_code" ]]; then payload_reason_code="none"; fi
    if [[ -z "$payload_check" ]]; then payload_check="none"; fi

    if (( ec == 0 )); then
      log_status "preflight_selftest_warmup" "PASS" "0" \
        "attempts=${attempt};max_attempts=${SELFTEST_WARMUP_MAX_ATTEMPTS};timeout_seconds=${SELFTEST_WARMUP_TIMEOUT_SECONDS};retry_sleep_seconds=${SELFTEST_WARMUP_RETRY_SLEEP_SECONDS};launch_ready_delay_seconds=${SELFTEST_LAUNCH_READY_DELAY_SECONDS};targeted_check_max_attempts=${SELFTEST_TARGETED_CHECK_MAX_ATTEMPTS}" \
        "$warmup_log"
      return 0
    fi

    if (( attempt < SELFTEST_WARMUP_MAX_ATTEMPTS )); then
      log_transcript "preflight_selftest_warmup[attempt=$attempt]=exit=$ec;terminal_failure_reason=${terminal_failure_reason};payload_reason_code=${payload_reason_code};payload_check=${payload_check}"
      if ! run_retry_recovery_barrier "preflight_selftest_warmup" "$SELFTEST_WARMUP_RETRY_SLEEP_SECONDS"; then
        log_status "preflight_selftest_warmup" "FAIL" "1" \
          "attempts=${attempt};retry_barrier=failed;terminal_failure_reason=${terminal_failure_reason};payload_reason_code=${payload_reason_code};payload_check=${payload_check}" \
          "$warmup_log"
        return 1
      fi
    fi

    attempt=$((attempt + 1))
  done

  log_status "preflight_selftest_warmup" "FAIL" "1" \
    "attempts=${SELFTEST_WARMUP_MAX_ATTEMPTS};max_attempts=${SELFTEST_WARMUP_MAX_ATTEMPTS};timeout_seconds=${SELFTEST_WARMUP_TIMEOUT_SECONDS};retry_sleep_seconds=${SELFTEST_WARMUP_RETRY_SLEEP_SECONDS};terminal_failure_reason=${terminal_failure_reason};payload_reason_code=${payload_reason_code};payload_check=${payload_check}" \
    "$warmup_log"
  return 1
}

latest_artifact_dir() {
  local pattern="$1"
  local found=""
  found="$(ls -dt $pattern 2>/dev/null | head -n 1 || true)"
  printf '%s' "$found"
}

file_has_abrt_signature() {
  local file_path="$1"
  if [[ ! -f "$file_path" ]]; then
    return 1
  fi
  rg -n "(Abort trap|app_exited_before_result|signal_name=ABRT|launch_mode_failed_open|renderExitCode=134)" "$file_path" >/dev/null 2>&1
}

manual_evidence_present() {
  local file_path="$1"
  [[ -f "$file_path" && -s "$file_path" ]]
}

run_command_capture() {
  local step="$1"
  local cmd="$2"
  local log_path="$3"
  local previous_step="$CURRENT_STEP"
  local ec=1
  local attempt=1
  local attempts_used=0
  local attempt_log=""
  CURRENT_STEP="$step"
  : > "$log_path"

  while (( attempt <= COMMAND_RETRY_MAX_ATTEMPTS )); do
    attempt_log="${log_path}.attempt_${attempt}"
    log_transcript "cmd[$step][attempt=$attempt]=$cmd"
    set +e
    env "${SELFTEST_ENV_OVERRIDES[@]}" bash -lc "$cmd" > "$attempt_log" 2>&1
    ec=$?
    set -e
    cat "$attempt_log" >> "$log_path"
    rm -f "$attempt_log"
    attempts_used="$attempt"

    if (( ec == 0 )); then
      break
    fi

    if (( attempt < COMMAND_RETRY_MAX_ATTEMPTS )); then
      log_transcript "cmd[$step][attempt=$attempt]=exit=$ec;retrying_with_recovery_barrier=1"
      if ! run_retry_recovery_barrier "$step"; then
        log_transcript "cmd[$step]=retry_barrier_failed;final_exit=$ec"
        break
      fi
    fi

    attempt=$((attempt + 1))
  done

  if (( ec == 0 )); then
    log_status "$step" "PASS" "$ec" "command_succeeded;attempts=$attempts_used" "$log_path"
  else
    log_status "$step" "FAIL" "$ec" "command_failed;attempts=$attempts_used" "$log_path"
  fi
  CURRENT_STEP="$previous_step"
  printf '%s' "$ec"
}

resolve_dev06_waiver_reason_code() {
  local waiver_path="$1"
  if [[ ! -e "$waiver_path" ]]; then
    printf '%s' "dev06_waiver_path_missing"
    return
  fi
  if [[ ! -f "$waiver_path" ]]; then
    printf '%s' "dev06_waiver_path_not_file"
    return
  fi
  if [[ ! -r "$waiver_path" ]]; then
    printf '%s' "dev06_waiver_path_unreadable"
    return
  fi
  if [[ ! -s "$waiver_path" ]]; then
    printf '%s' "dev06_waiver_path_empty"
    return
  fi
  printf '%s' "dev06_waiver_path_valid"
}

write_harness_contract() {
  {
    echo "Title: BL-030 RL-05 Device Matrix Capture Harness Contract"
    echo "Document Type: Test Evidence"
    echo "Author: APC Codex"
    echo "Created Date: ${DOC_DATE}"
    echo "Last Modified Date: ${DOC_DATE}"
    echo
    echo "# BL-030 RL-05 Device Matrix Capture Harness"
    echo
    echo "## Command"
    echo "- \`./scripts/qa-bl030-device-matrix-capture-mac.sh\`"
    echo
    echo "## Blocker Categories"
    echo "- deterministic_missing_manual_evidence"
    echo "- runtime_flake_abrt"
    echo "- not_applicable_with_waiver"
    echo
    echo "## DEV-06 Waiver Preflight Reason Codes"
    echo "- dev06_waiver_path_valid"
    echo "- dev06_waiver_path_missing"
    echo "- dev06_waiver_path_not_file"
    echo "- dev06_waiver_path_unreadable"
    echo "- dev06_waiver_path_empty"
    echo
    echo "## Exit Semantics"
    echo "- exit 0: DEV-01..DEV-05 are PASS and DEV-06 is PASS or N/A with waiver"
    echo "- exit 1: RL-05 fail criteria"
    echo "- exit 2: usage/invocation error"
    echo
    echo "## Artifacts"
    echo "- \`status.tsv\`"
    echo "- \`dev_matrix_results.tsv\`"
    echo "- \`blocker_taxonomy.tsv\`"
    echo "- \`replay_transcript.log\`"
    echo "- \`command_transcript.log\`"
    echo
    echo "## Result"
    echo "- overall: ${OVERALL}"
    echo "- artifact_dir: \`${OUT_DIR#"$ROOT_DIR/"}\`"
  } > "$HARNESS_CONTRACT_MD"
}

emit_artifact_paths() {
  echo "artifact_dir=$OUT_DIR"
  echo "status_tsv=$STATUS_TSV"
  echo "dev_matrix_tsv=$DEV_MATRIX_TSV"
  echo "blocker_taxonomy_tsv=$BLOCKER_TSV"
  echo "harness_contract_md=$HARNESS_CONTRACT_MD"
}

signal_to_exit_code() {
  local signal="$1"
  case "$signal" in
    HUP) printf '129' ;;
    INT) printf '130' ;;
    QUIT) printf '131' ;;
    TERM) printf '143' ;;
    *) printf '1' ;;
  esac
}

handle_signal_termination() {
  local signal="$1"
  local signal_exit_code
  local detail
  signal_exit_code="$(signal_to_exit_code "$signal")"

  if (( SIGNAL_TERMINATION_HANDLED == 1 )); then
    exit "$signal_exit_code"
  fi
  SIGNAL_TERMINATION_HANDLED=1

  set +e
  OVERALL="FAIL"
  EXIT_CODE=1
  detail="capture_aborted_without_gate;signal=${signal};signal_exit_code=${signal_exit_code};current_step=${CURRENT_STEP}"
  log_transcript "signal_termination;${detail}"
  add_blocker "DEV-ALL" "runtime_flake_abrt" "$detail" "$STATUS_TSV"
  log_status "signal_termination" "FAIL" "$signal_exit_code" "$detail" "$STATUS_TSV"
  if (( RL05_GATE_EMITTED == 0 )); then
    record_gate_decision "FAIL" "1" "$detail" "$DEV_MATRIX_TSV"
  fi
  write_harness_contract
  emit_artifact_paths
  exit "$signal_exit_code"
}

trap 'handle_signal_termination HUP' HUP
trap 'handle_signal_termination INT' INT
trap 'handle_signal_termination QUIT' QUIT
trap 'handle_signal_termination TERM' TERM

if [[ -n "$DEV06_WAIVER" ]]; then
  DEV06_WAIVER_REASON_CODE="$(resolve_dev06_waiver_reason_code "$DEV06_WAIVER")"
  if [[ "$DEV06_WAIVER_REASON_CODE" != "dev06_waiver_path_valid" ]]; then
    add_blocker "DEV-06" "deterministic_missing_manual_evidence" "reason_code=$DEV06_WAIVER_REASON_CODE;invalid_dev06_waiver_path=$DEV06_WAIVER" "$DEV06_WAIVER"
    append_dev_result "DEV-06" "FAIL" "deterministic_missing_manual_evidence" "$DEV06_WAIVER" "reason_code=$DEV06_WAIVER_REASON_CODE;waiver_preflight_failed=1"
    log_status "dev06_waiver_preflight" "FAIL" "1" "reason_code=$DEV06_WAIVER_REASON_CODE" "$DEV06_WAIVER"
    record_gate_decision "FAIL" "1" "rl05_criteria_not_met_dev06_waiver_preflight" "$DEV_MATRIX_TSV"
    OVERALL="FAIL"
    EXIT_CODE=1
    write_harness_contract
    emit_artifact_paths
    exit "$EXIT_CODE"
  fi
  log_status "dev06_waiver_preflight" "PASS" "0" "reason_code=$DEV06_WAIVER_REASON_CODE" "$DEV06_WAIVER"
fi

if ! run_preflight_drain_and_warmup; then
  add_blocker "DEV-ALL" "runtime_flake_abrt" \
    "preflight_process_drain_timeout;remaining_pids=${PRELAUNCH_DRAIN_REMAINING_PIDS}" \
    "$OUT_DIR"
  record_gate_decision "FAIL" "1" "rl05_criteria_not_met_preflight_process_drain_timeout" "$DEV_MATRIX_TSV"
  OVERALL="FAIL"
  EXIT_CODE=1
  write_harness_contract
  emit_artifact_paths
  exit "$EXIT_CODE"
fi

if (( SKIP_BUILD == 0 )); then
  BUILD_LOG="$OUT_DIR/dev00_build.log"
  BUILD_EC="$(run_command_capture "preflight_build" "cd '$ROOT_DIR' && cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8" "$BUILD_LOG")"
else
  BUILD_EC=0
  log_status "preflight_build" "PASS" "0" "skipped_by_flag" "$OUT_DIR"
fi

if (( BUILD_EC != 0 )); then
  add_blocker "DEV-ALL" "runtime_flake_abrt" "preflight_build_failed" "$OUT_DIR/dev00_build.log"
fi

if (( BUILD_EC == 0 )); then
  if ! run_preflight_selftest_warmup; then
    add_blocker "DEV-ALL" "runtime_flake_abrt" "preflight_selftest_warmup_failed" "$OUT_DIR/preflight_selftest_warmup.log"
    record_gate_decision "FAIL" "1" "rl05_criteria_not_met_preflight_selftest_warmup_failed" "$DEV_MATRIX_TSV"
    OVERALL="FAIL"
    EXIT_CODE=1
    write_harness_contract
    emit_artifact_paths
    exit "$EXIT_CODE"
  fi
else
  : > "$OUT_DIR/preflight_selftest_warmup.log"
  log_status "preflight_selftest_warmup" "FAIL" "1" "skipped_due_to_preflight_build_failed" "$OUT_DIR/preflight_selftest_warmup.log"
fi

DEV01_RESULT="FAIL"
DEV01_CLASS="deterministic_missing_manual_evidence"
DEV01_NOTES=""
DEV01_EVIDENCE=""
{
  DEV01_BL018_LOG="$OUT_DIR/dev01_bl018_profile_matrix.log"
  DEV01_REAPER_LOG="$OUT_DIR/dev01_reaper_headless.log"
  BEFORE_BL018="$(latest_artifact_dir "$ROOT_DIR/TestEvidence/bl018_profile_matrix_*")"
  DEV01_BL018_EC="$(run_command_capture "dev01_bl018_profile_matrix" "cd '$ROOT_DIR' && ./scripts/qa-bl018-profile-matrix-strict-mac.sh" "$DEV01_BL018_LOG")"
  AFTER_BL018="$(latest_artifact_dir "$ROOT_DIR/TestEvidence/bl018_profile_matrix_*")"
  BL018_ARTIFACT="$AFTER_BL018"
  if [[ -z "$BL018_ARTIFACT" ]]; then BL018_ARTIFACT="$BEFORE_BL018"; fi

  DEV01_REAPER_EC="$(run_command_capture "dev01_reaper_headless" "cd '$ROOT_DIR' && ./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap" "$DEV01_REAPER_LOG")"

  runtime_flake=0
  if (( DEV01_BL018_EC != 0 )) && (file_has_abrt_signature "$DEV01_BL018_LOG" || [[ -n "$BL018_ARTIFACT" && -f "$BL018_ARTIFACT/production_selftest.log" ]] && file_has_abrt_signature "$BL018_ARTIFACT/production_selftest.log"); then
    runtime_flake=1
  fi
  if (( DEV01_REAPER_EC != 0 )) && file_has_abrt_signature "$DEV01_REAPER_LOG"; then
    runtime_flake=1
  fi
  if (( runtime_flake == 1 )); then
    add_blocker "DEV-01" "runtime_flake_abrt" "automation_lane_abrt_signature" "$DEV01_BL018_LOG;$DEV01_REAPER_LOG"
  fi

  manual_missing=0
  if ! manual_evidence_present "$DEV01_MANUAL_NOTES"; then
    manual_missing=1
    add_blocker "DEV-01" "deterministic_missing_manual_evidence" "missing_manual_notes=$DEV01_MANUAL_NOTES" "$DEV01_MANUAL_NOTES"
  fi

  DEV01_EVIDENCE="$DEV01_BL018_LOG;$DEV01_REAPER_LOG;$DEV01_MANUAL_NOTES"
  if (( DEV01_BL018_EC == 0 && DEV01_REAPER_EC == 0 && manual_missing == 0 )); then
    DEV01_RESULT="PASS"
    DEV01_CLASS="none"
    DEV01_NOTES="automation_and_manual_checks_passed"
  else
    if (( manual_missing == 1 )); then
      DEV01_CLASS="deterministic_missing_manual_evidence"
    elif (( runtime_flake == 1 )); then
      DEV01_CLASS="runtime_flake_abrt"
    else
      DEV01_CLASS="deterministic_missing_manual_evidence"
    fi
    DEV01_NOTES="bl018_exit=$DEV01_BL018_EC;reaper_exit=$DEV01_REAPER_EC;manual_missing=$manual_missing"
  fi
  append_dev_result "DEV-01" "$DEV01_RESULT" "$DEV01_CLASS" "$DEV01_EVIDENCE" "$DEV01_NOTES"
}

run_bl009_contract_dev() {
  local dev_id="$1"
  local manual_notes="$2"
  local log_path="$3"
  local step_label="$4"
  local artifact_glob="$5"
  local script_cmd="$6"

  local result="FAIL"
  local class="deterministic_missing_manual_evidence"
  local notes=""
  local evidence=""
  local runtime_flake=0
  local manual_missing=0

  local before_artifact
  before_artifact="$(latest_artifact_dir "$artifact_glob")"
  local ec
  ec="$(run_command_capture "$step_label" "cd '$ROOT_DIR' && LOCUSQ_UI_SELFTEST_SCOPE=bl009 $script_cmd" "$log_path")"
  local after_artifact
  after_artifact="$(latest_artifact_dir "$artifact_glob")"
  local artifact_dir="$after_artifact"
  if [[ -z "$artifact_dir" ]]; then artifact_dir="$before_artifact"; fi

  if (( ec != 0 )); then
    if file_has_abrt_signature "$log_path" || [[ -n "$artifact_dir" && -f "$artifact_dir/status.tsv" ]] && file_has_abrt_signature "$artifact_dir/status.tsv"; then
      runtime_flake=1
      add_blocker "$dev_id" "runtime_flake_abrt" "lane_failed_with_abrt_signature" "$log_path;${artifact_dir:-none}"
    fi
  fi

  if ! manual_evidence_present "$manual_notes"; then
    manual_missing=1
    add_blocker "$dev_id" "deterministic_missing_manual_evidence" "missing_manual_notes=$manual_notes" "$manual_notes"
  fi

  evidence="$log_path;${artifact_dir:-none};$manual_notes"
  if (( ec == 0 && manual_missing == 0 )); then
    result="PASS"
    class="none"
    notes="automation_and_manual_checks_passed"
  else
    if (( manual_missing == 1 )); then
      class="deterministic_missing_manual_evidence"
    elif (( runtime_flake == 1 )); then
      class="runtime_flake_abrt"
    else
      class="deterministic_missing_manual_evidence"
    fi
    notes="lane_exit=$ec;manual_missing=$manual_missing"
  fi

  append_dev_result "$dev_id" "$result" "$class" "$evidence" "$notes"
}

run_bl009_contract_dev "DEV-02" "$DEV02_MANUAL_NOTES" "$OUT_DIR/dev02_bl009_headphone_contract.log" "dev02_bl009_contract" "$ROOT_DIR/TestEvidence/bl009_headphone_contract_*" "./scripts/qa-bl009-headphone-contract-mac.sh"
run_bl009_contract_dev "DEV-03" "$DEV03_MANUAL_NOTES" "$OUT_DIR/dev03_bl009_headphone_profile.log" "dev03_bl009_profile_contract" "$ROOT_DIR/TestEvidence/bl009_headphone_profile_contract_*" "./scripts/qa-bl009-headphone-profile-contract-mac.sh"
run_bl009_contract_dev "DEV-04" "$DEV04_MANUAL_NOTES" "$OUT_DIR/dev04_bl009_headphone_contract.log" "dev04_bl009_contract" "$ROOT_DIR/TestEvidence/bl009_headphone_contract_*" "./scripts/qa-bl009-headphone-contract-mac.sh"

run_bl026_selftest_dev() {
  local dev_id="$1"
  local manual_notes="$2"
  local log_path="$3"
  local step_label="$4"

  local ec
  ec="$(run_command_capture "$step_label" "cd '$ROOT_DIR' && LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh" "$log_path")"

  local runtime_flake=0
  local manual_missing=0
  if (( ec != 0 )) && file_has_abrt_signature "$log_path"; then
    runtime_flake=1
    add_blocker "$dev_id" "runtime_flake_abrt" "bl026_selftest_abrt_signature" "$log_path"
  fi
  if ! manual_evidence_present "$manual_notes"; then
    manual_missing=1
    add_blocker "$dev_id" "deterministic_missing_manual_evidence" "missing_manual_notes=$manual_notes" "$manual_notes"
  fi

  local result="FAIL"
  local class="deterministic_missing_manual_evidence"
  local notes="selftest_exit=$ec;manual_missing=$manual_missing"
  if (( ec == 0 && manual_missing == 0 )); then
    result="PASS"
    class="none"
    notes="automation_and_manual_checks_passed"
  else
    if (( manual_missing == 1 )); then
      class="deterministic_missing_manual_evidence"
    elif (( runtime_flake == 1 )); then
      class="runtime_flake_abrt"
    else
      class="deterministic_missing_manual_evidence"
    fi
  fi

  append_dev_result "$dev_id" "$result" "$class" "$log_path;$manual_notes" "$notes"
}

run_bl026_selftest_dev "DEV-05" "$DEV05_MANUAL_NOTES" "$OUT_DIR/dev05_bl026_selftest.log" "dev05_bl026_selftest"

if [[ -n "$DEV06_WAIVER" ]]; then
  add_blocker "DEV-06" "not_applicable_with_waiver" "dev06_marked_na_with_waiver" "$DEV06_WAIVER"
  append_dev_result "DEV-06" "N/A" "not_applicable_with_waiver" "$DEV06_WAIVER" "external_mic_hardware_waived;waiver_reason_code=${DEV06_WAIVER_REASON_CODE:-dev06_waiver_path_valid}"
  log_status "dev06_waiver" "PASS" "0" "waiver_applied" "$DEV06_WAIVER"
else
  run_bl026_selftest_dev "DEV-06" "$DEV06_MANUAL_NOTES" "$OUT_DIR/dev06_bl026_selftest.log" "dev06_bl026_selftest"
fi

RL05_PASS=1
for dev in DEV-01 DEV-02 DEV-03 DEV-04 DEV-05; do
  if ! awk -F'\t' -v d="$dev" 'NR>1 && $1==d && $2=="PASS" { found=1 } END { exit(found ? 0 : 1) }' "$DEV_MATRIX_TSV"; then
    RL05_PASS=0
  fi
done
if ! awk -F'\t' 'NR>1 && $1=="DEV-06" && ($2=="PASS" || $2=="N/A") { found=1 } END { exit(found ? 0 : 1) }' "$DEV_MATRIX_TSV"; then
  RL05_PASS=0
fi

if (( RL05_PASS == 1 )); then
  record_gate_decision "PASS" "0" "dev01_05_pass_and_dev06_pass_or_na" "$DEV_MATRIX_TSV"
  OVERALL="PASS"
  EXIT_CODE=0
else
  record_gate_decision "FAIL" "1" "rl05_criteria_not_met" "$DEV_MATRIX_TSV"
  OVERALL="FAIL"
  EXIT_CODE=1
fi

write_harness_contract
emit_artifact_paths

exit "$EXIT_CODE"
