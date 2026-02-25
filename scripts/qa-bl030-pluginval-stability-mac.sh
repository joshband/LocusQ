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
  --help, -h            Show this help

Outputs:
  status.tsv
  replay_runs.tsv
  failure_taxonomy.tsv
  harness_contract.md
  command_transcript.log

Exit semantics:
  0 = all required runs passed
  1 = one or more runs failed, or preflight invalid
USAGE
}

RUNS="$RUNS_DEFAULT"
TIMEOUT_MS="$TIMEOUT_MS_DEFAULT"
PLUGINVAL_BIN="$PLUGINVAL_BIN_DEFAULT"
PLUGIN_PATH="$PLUGIN_PATH_DEFAULT"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl030_rl06_pluginval_stability_${TIMESTAMP}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --runs requires a value" >&2
        exit 1
      fi
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --out-dir requires a value" >&2
        exit 1
      fi
      OUT_DIR="$2"
      shift 2
      ;;
    --plugin-path)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --plugin-path requires a value" >&2
        exit 1
      fi
      PLUGIN_PATH="$2"
      shift 2
      ;;
    --pluginval-bin)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --pluginval-bin requires a value" >&2
        exit 1
      fi
      PLUGINVAL_BIN="$2"
      shift 2
      ;;
    --timeout-ms)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --timeout-ms requires a value" >&2
        exit 1
      fi
      TIMEOUT_MS="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || (( RUNS <= 0 )); then
  echo "ERROR: --runs must be a positive integer" >&2
  exit 1
fi

if ! [[ "$TIMEOUT_MS" =~ ^[0-9]+$ ]] || (( TIMEOUT_MS <= 0 )); then
  echo "ERROR: --timeout-ms must be a positive integer" >&2
  exit 1
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
REPLAY_RUNS_TSV="${OUT_DIR}/replay_runs.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"
HARNESS_CONTRACT_MD="${OUT_DIR}/harness_contract.md"
TRANSCRIPT_LOG="${OUT_DIR}/command_transcript.log"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
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

log_transcript() {
  local msg="$1"
  printf "[%s] %s\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$msg" >> "$TRANSCRIPT_LOG"
}

classify_reason() {
  local reason="$1"
  local exit_code="$2"
  if [[ "$exit_code" == "0" ]]; then
    printf "pass"
    return
  fi

  if [[ "$reason" == *"abrt"* ]] || [[ "$reason" == *"ABRT"* ]] || [[ "$reason" == *"exit_134"* ]]; then
    printf "transient_runtime_abort"
    return
  fi

  if [[ "$reason" == *"timeout"* ]]; then
    printf "deterministic_timeout"
    return
  fi

  printf "deterministic_failure"
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
  local classification="$4"
  local crash_present="$5"
  local crash_path="$6"
  local plugin_path="$7"
  local log_path="$8"

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
  PRECHECK_FAIL=1
else
  log_status "preflight_pluginval_bin" "PASS" "0" "pluginval_executable_found" "$PLUGINVAL_BIN"
fi

if [[ ! -e "$PLUGIN_PATH_RESOLVED" ]]; then
  log_status "preflight_plugin_path" "FAIL" "1" "plugin_path_not_found" "$PLUGIN_PATH_RESOLVED"
  PRECHECK_FAIL=1
else
  log_status "preflight_plugin_path" "PASS" "0" "plugin_path_found" "$PLUGIN_PATH_RESOLVED"
fi

if (( PRECHECK_FAIL == 1 )); then
  log_status "pluginval_replay" "FAIL" "1" "skipped_due_to_preflight_failure" "$REPLAY_RUNS_TSV"
  {
    echo "Title: BL-030 RL-06 Pluginval Stability Harness Contract"
    echo "Document Type: Testing Runbook"
    echo "Author: APC Codex"
    echo "Created Date: ${DOC_DATE}"
    echo "Last Modified Date: ${DOC_DATE}"
    echo
    echo "# BL-030 RL-06 Pluginval Stability Harness"
    echo
    echo "## Result"
    echo "- overall: FAIL"
    echo "- reason: preflight_failure"
    echo "- evaluated_at: ${DOC_TS}"
  } > "$HARNESS_CONTRACT_MD"
  exit 1
fi

RUN_FAILS=0
RUN_PASSES=0

for run_idx in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run_idx}.log"
  marker_file="${OUT_DIR}/run_${run_idx}.marker"
  touch "$marker_file"
  log_transcript "run_${run_idx}: ${PLUGINVAL_BIN} --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms ${TIMEOUT_MS} ${PLUGIN_PATH_RESOLVED}"

  set +e
  "$PLUGINVAL_BIN" \
    --strictness-level 5 \
    --validate-in-process \
    --skip-gui-tests \
    --timeout-ms "$TIMEOUT_MS" \
    "$PLUGIN_PATH_RESOLVED" \
    > "$run_log" 2>&1
  run_ec=$?
  set -e

  run_reason="unknown"
  if (( run_ec == 0 )); then
    run_reason="pass"
  elif grep -Eiq 'Abort trap|ABRT|SIGABRT|signal 6|EXC_BAD_ACCESS' "$run_log"; then
    run_reason="pluginval_exit_${run_ec}|abrt"
  elif grep -Eiq 'timeout|timed out' "$run_log"; then
    run_reason="pluginval_timeout"
  else
    run_reason="pluginval_exit_${run_ec}"
  fi

  crash_path="$(detect_crash_report_after "$marker_file")"
  crash_present="no"
  if [[ -n "$crash_path" ]]; then
    crash_present="yes"
  fi

  run_classification="$(classify_reason "$run_reason" "$run_ec")"
  append_run "$run_idx" "$run_ec" "$run_reason" "$run_classification" "$crash_present" "$crash_path" "$PLUGIN_PATH_RESOLVED" "$run_log"

  if (( run_ec == 0 )); then
    RUN_PASSES=$((RUN_PASSES + 1))
  else
    RUN_FAILS=$((RUN_FAILS + 1))
  fi
done

if (( RUN_FAILS == 0 )); then
  log_status "pluginval_replay" "PASS" "0" "runs=${RUNS};passes=${RUN_PASSES};fails=${RUN_FAILS}" "$REPLAY_RUNS_TSV"
  OVERALL_RESULT="PASS"
  EXIT_CODE=0
else
  log_status "pluginval_replay" "FAIL" "1" "runs=${RUNS};passes=${RUN_PASSES};fails=${RUN_FAILS}" "$REPLAY_RUNS_TSV"
  OVERALL_RESULT="FAIL"
  EXIT_CODE=1
fi

awk -F'\t' '
  NR == 1 { next }
  {
    reason[$4]++
    class[$5]++
    crash[$6]++
    exitc[$3]++
  }
  END {
    for (k in reason) printf "terminal_reason\t%s\t%d\n", k, reason[k]
    for (k in class) printf "classification\t%s\t%d\n", k, class[k]
    for (k in crash) printf "crash_report_present\t%s\t%d\n", k, crash[k]
    for (k in exitc) printf "exit_code\t%s\t%d\n", k, exitc[k]
  }
' "$REPLAY_RUNS_TSV" | sort > "${OUT_DIR}/.failure_taxonomy.body"
cat "${OUT_DIR}/.failure_taxonomy.body" >> "$FAILURE_TAXONOMY_TSV"
rm -f "${OUT_DIR}/.failure_taxonomy.body"

{
  echo "Title: BL-030 RL-06 Pluginval Stability Harness Contract"
  echo "Document Type: Testing Runbook"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE}"
  echo "Last Modified Date: ${DOC_DATE}"
  echo
  echo "# BL-030 RL-06 Pluginval Stability Harness"
  echo
  echo "## Command"
  echo "- \`./scripts/qa-bl030-pluginval-stability-mac.sh --runs ${RUNS} --out-dir <artifact_dir>\`"
  echo
  echo "## Exit Semantics"
  echo "- exit 0: all required runs passed"
  echo "- exit 1: preflight invalid or one/more runs failed"
  echo
  echo "## Outputs"
  echo "- \`status.tsv\`"
  echo "- \`replay_runs.tsv\`"
  echo "- \`failure_taxonomy.tsv\`"
  echo "- \`harness_contract.md\`"
  echo "- \`command_transcript.log\`"
  echo
  echo "## Result"
  echo "- overall: ${OVERALL_RESULT}"
  echo "- runs: ${RUNS}"
  echo "- passes: ${RUN_PASSES}"
  echo "- fails: ${RUN_FAILS}"
  echo "- evaluated_at: ${DOC_TS}"
  echo "- artifact_dir: \`${OUT_DIR#"$ROOT_DIR/"}\`"
} > "$HARNESS_CONTRACT_MD"

echo "artifact_dir=$OUT_DIR"
echo "status_tsv=$STATUS_TSV"
echo "replay_runs_tsv=$REPLAY_RUNS_TSV"
echo "failure_taxonomy_tsv=$FAILURE_TAXONOMY_TSV"
echo "harness_contract_md=$HARNESS_CONTRACT_MD"

exit "$EXIT_CODE"
