#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"
TIMESTAMP_UTC="$(date -u +%Y%m%dT%H%M%SZ)"

SCOPE="bl029"
RUNS=1
BL009=0
OUT_DIR=""
SELFTEST_TIMEOUT_SECONDS=""

usage() {
  cat <<'USAGE'
Usage: diagnose-selftest-abrt-mac.sh [options]

Options:
  --scope <value>      Selftest scope value (default: bl029)
  --runs <int>         Number of probe runs (default: 1)
  --bl009 <0|1>        Set LOCUSQ_UI_SELFTEST_BL009 flag (default: 0)
  --timeout <seconds>  Override selftest timeout seconds
  --out <path>         Output directory (default: TestEvidence/bl029_abrt_probe_s2_<ts>)
  --help               Show this message
USAGE
}

is_uint() {
  [[ "$1" =~ ^[0-9]+$ ]]
}

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

lowercase() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

find_newest_crash_since() {
  local marker_path="$1"
  local crash_dir="$HOME/Library/Logs/DiagnosticReports"

  if [[ ! -d "$crash_dir" ]]; then
    return 0
  fi

  local newest
  newest="$({
    find "$crash_dir" -maxdepth 1 -type f \( -name 'LocusQ*.ips' -o -name 'LocusQ*.crash' \) -newer "$marker_path" -print0 2>/dev/null \
      | while IFS= read -r -d '' file; do
          local mtime
          mtime="$(stat -f '%m' "$file" 2>/dev/null || echo 0)"
          printf '%s\t%s\n' "$mtime" "$file"
        done \
      | sort -nr \
      | head -n 1 \
      | cut -f2-
  } || true)"

  printf '%s' "$newest"
}

is_path_newer_than_marker() {
  local path="$1"
  local marker="$2"
  if [[ -z "$path" || -z "$marker" || ! -f "$path" || ! -f "$marker" ]]; then
    return 1
  fi

  [[ "$path" -nt "$marker" ]]
}

extract_main_thread_frames() {
  local crash_path="$1"
  if [[ -z "$crash_path" || ! -f "$crash_path" ]]; then
    return 0
  fi

  if command -v jq >/dev/null 2>&1; then
    local ips_frames
    ips_frames="$(jq -r -s '
      (.[1] // .[0]) as $r
      | ($r.threads // [])
      | if length == 0 then [] else . end
      | (map(select((.triggered // false) == true))[0] // map(select((.id // -1) == ($r.faultingThread // -2)))[0] // .[0]) as $t
      | ($t.frames // [])[:12][]
      | if (.symbol // "") != ""
          then (.symbol + (if (.symbolLocation | type) == "number" then "+" + (.symbolLocation | tostring) else "" end))
          else ("imageOffset:" + ((.imageOffset // 0) | tostring))
        end
    ' "$crash_path" 2>/dev/null || true)"

    if [[ -n "$ips_frames" ]]; then
      printf '%s\n' "$ips_frames"
      return 0
    fi
  fi

  local crash_line
  crash_line="$(rg -n -m1 '^Thread 0 (Crashed|crashed|triggered)' "$crash_path" 2>/dev/null | cut -d: -f1 || true)"
  if [[ -z "$crash_line" ]]; then
    crash_line="$(rg -n -m1 '^Thread 0' "$crash_path" 2>/dev/null | cut -d: -f1 || true)"
  fi

  if [[ -n "$crash_line" ]]; then
    tail -n "+$((crash_line + 1))" "$crash_path" | awk '
      BEGIN {count = 0}
      /^[[:space:]]*$/ { if (count > 0) exit; next }
      /^[[:space:]]*[0-9]+[[:space:]]/ {
        print
        count++
        if (count >= 12) exit
        next
      }
      { if (count > 0) exit }
    '
  fi
}

extract_crash_signal_number() {
  local crash_path="$1"
  if [[ -z "$crash_path" || ! -f "$crash_path" ]]; then
    return 0
  fi

  if command -v jq >/dev/null 2>&1; then
    jq -r -s '((.[1] // .[0]).termination.code // "")' "$crash_path" 2>/dev/null || true
    return 0
  fi

  rg -n -m1 'Termination Signal:.*([0-9]+)' "$crash_path" 2>/dev/null | sed -E 's/.*([0-9]+).*/\1/' || true
}

extract_crash_signal_name() {
  local crash_path="$1"
  if [[ -z "$crash_path" || ! -f "$crash_path" ]]; then
    return 0
  fi

  if command -v jq >/dev/null 2>&1; then
    jq -r -s '((.[1] // .[0]).exception.signal // "")' "$crash_path" 2>/dev/null || true
    return 0
  fi

  rg -n -m1 'Exception Type:|Termination Signal:' "$crash_path" 2>/dev/null | sed -E 's/.*(SIG[A-Z]+).*/\1/' || true
}

classify_phase() {
  local reason="$1"
  local frames="$2"
  local reason_lc
  local frames_lc
  reason_lc="$(lowercase "$reason")"
  frames_lc="$(lowercase "$frames")"

  if rg -q 'registerapplication|_registerapplication|getcurrentprocess|nsapplication|appkit|_nsinitializeappcontext|audiocomponentregistrar' <<<"$frames_lc"; then
    printf 'appkit_registration'
    return
  fi

  if rg -q 'wkwebview|webkit|webbrowser|javascriptcore|juce::webbrowser|juce webbrowser|webview2|juce::webview' <<<"$frames_lc"; then
    printf 'post_ui_bootstrap'
    return
  fi

  if [[ "$reason_lc" == "app_exited_before_result" || "$reason_lc" == result_json_missing_after_* || "$reason_lc" == "selftest_payload_not_ok" ]]; then
    printf 'pre_result_emit'
    return
  fi

  printf 'unknown'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --scope)
      SCOPE="${2:-}"
      shift 2
      ;;
    --runs)
      RUNS="${2:-}"
      shift 2
      ;;
    --bl009)
      BL009="${2:-}"
      shift 2
      ;;
    --timeout)
      SELFTEST_TIMEOUT_SECONDS="${2:-}"
      shift 2
      ;;
    --out)
      OUT_DIR="${2:-}"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: Unknown argument: $1"
      usage
      exit 2
      ;;
  esac
done

if ! is_uint "$RUNS" || (( RUNS < 1 )); then
  echo "ERROR: --runs must be a positive integer"
  exit 2
fi

if [[ "$BL009" != "0" && "$BL009" != "1" ]]; then
  echo "ERROR: --bl009 must be 0 or 1"
  exit 2
fi

if [[ -n "$SELFTEST_TIMEOUT_SECONDS" ]] && ! is_uint "$SELFTEST_TIMEOUT_SECONDS"; then
  echo "ERROR: --timeout must be an integer"
  exit 2
fi

if [[ -z "$OUT_DIR" ]]; then
  OUT_DIR="$ROOT_DIR/TestEvidence/bl029_abrt_probe_s2_${TIMESTAMP_UTC}"
fi

mkdir -p "$OUT_DIR"

TAXONOMY_TSV="$OUT_DIR/crash_taxonomy.tsv"
TRIAGE_SUMMARY_MD="$OUT_DIR/triage_summary.md"
CRASH_STACK_SAMPLES_MD="$OUT_DIR/crash_stack_samples.md"
REPRO_COMMANDS_MD="$OUT_DIR/repro_commands.md"
PROBE_LOG="$OUT_DIR/probe.log"

{
  echo "probe_ts=${TIMESTAMP_UTC}"
  echo "scope=${SCOPE}"
  echo "runs=${RUNS}"
  echo "bl009=${BL009}"
  if [[ -n "$SELFTEST_TIMEOUT_SECONDS" ]]; then
    echo "timeout_seconds=${SELFTEST_TIMEOUT_SECONDS}"
  fi
  echo "out_dir=${OUT_DIR}"
} | tee "$PROBE_LOG"

printf "run_id\tscope\tbl009\tselftest_exit_code\tselftest_status\tterminal_failure_reason\tapp_exit_code\tapp_signal\tapp_signal_name\tphase_classification\tcrash_report_path\tmain_thread_frames_summary\tselftest_result_json\tselftest_meta_json\tselftest_run_log\tselftest_invocation_log\n" > "$TAXONOMY_TSV"

cat > "$CRASH_STACK_SAMPLES_MD" <<__STACKS__
Title: BL-029 ABRT Crash Stack Samples
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# Crash Stack Samples

- scope: \`${SCOPE}\`
- runs: \`${RUNS}\`
- bl009: \`${BL009}\`

__STACKS__

for run in $(seq 1 "$RUNS"); do
  RUN_ID="$(printf '%02d' "$run")"
  RUN_DIR="$OUT_DIR/run_${RUN_ID}"
  mkdir -p "$RUN_DIR"

  INVOCATION_LOG="$RUN_DIR/selftest_invocation.log"
  SELFTEST_RESULT_JSON="$RUN_DIR/selftest_result.json"
  SELFTEST_RUN_LOG="$RUN_DIR/selftest_run.log"
  SELFTEST_ATTEMPT_TSV="$RUN_DIR/selftest_attempts.tsv"
  SELFTEST_META_JSON="$RUN_DIR/selftest_meta.json"

  MARKER_FILE="$(mktemp /tmp/locusq_abrt_probe_marker.XXXXXX)"

  {
    echo "run_id=${RUN_ID}"
    echo "selftest_invocation_log=${INVOCATION_LOG}"
    echo "selftest_result_json=${SELFTEST_RESULT_JSON}"
    echo "selftest_run_log=${SELFTEST_RUN_LOG}"
    echo "selftest_attempt_tsv=${SELFTEST_ATTEMPT_TSV}"
    echo "selftest_meta_json=${SELFTEST_META_JSON}"
  } | tee -a "$PROBE_LOG"

  set +e
  if [[ -n "$SELFTEST_TIMEOUT_SECONDS" ]]; then
    LOCUSQ_UI_SELFTEST_SCOPE="$SCOPE" \
    LOCUSQ_UI_SELFTEST_BL009="$BL009" \
    LOCUSQ_UI_SELFTEST_MAX_ATTEMPTS=1 \
    LOCUSQ_UI_SELFTEST_TIMEOUT_SECONDS="$SELFTEST_TIMEOUT_SECONDS" \
    LOCUSQ_UI_SELFTEST_RESULT_PATH="$SELFTEST_RESULT_JSON" \
    LOCUSQ_UI_SELFTEST_RUN_LOG_PATH="$SELFTEST_RUN_LOG" \
    LOCUSQ_UI_SELFTEST_ATTEMPT_TABLE_PATH="$SELFTEST_ATTEMPT_TSV" \
    LOCUSQ_UI_SELFTEST_META_PATH="$SELFTEST_META_JSON" \
    "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" > "$INVOCATION_LOG" 2>&1
  else
    LOCUSQ_UI_SELFTEST_SCOPE="$SCOPE" \
    LOCUSQ_UI_SELFTEST_BL009="$BL009" \
    LOCUSQ_UI_SELFTEST_MAX_ATTEMPTS=1 \
    LOCUSQ_UI_SELFTEST_RESULT_PATH="$SELFTEST_RESULT_JSON" \
    LOCUSQ_UI_SELFTEST_RUN_LOG_PATH="$SELFTEST_RUN_LOG" \
    LOCUSQ_UI_SELFTEST_ATTEMPT_TABLE_PATH="$SELFTEST_ATTEMPT_TSV" \
    LOCUSQ_UI_SELFTEST_META_PATH="$SELFTEST_META_JSON" \
    "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" > "$INVOCATION_LOG" 2>&1
  fi
  SELFTEST_EXIT_CODE=$?
  set -e

  SELFTEST_STATUS="fail"
  TERMINAL_REASON=""
  APP_EXIT_CODE=""
  APP_SIGNAL=""
  APP_SIGNAL_NAME=""
  CRASH_REPORT_PATH=""

  if [[ "$SELFTEST_EXIT_CODE" -eq 0 ]]; then
    SELFTEST_STATUS="pass"
  fi

  if [[ -f "$SELFTEST_META_JSON" ]] && command -v jq >/dev/null 2>&1; then
    SELFTEST_STATUS="$(jq -r '.status // "unknown"' "$SELFTEST_META_JSON" 2>/dev/null || echo "$SELFTEST_STATUS")"
    TERMINAL_REASON="$(jq -r '.terminalFailureReason // .terminal_failure_reason // ""' "$SELFTEST_META_JSON" 2>/dev/null || true)"
    APP_EXIT_CODE="$(jq -r '.appExitCode // .app_exit_code // ""' "$SELFTEST_META_JSON" 2>/dev/null || true)"
    APP_SIGNAL="$(jq -r '.appSignal // .app_signal // ""' "$SELFTEST_META_JSON" 2>/dev/null || true)"
    APP_SIGNAL_NAME="$(jq -r '.appSignalName // .app_signal_name // ""' "$SELFTEST_META_JSON" 2>/dev/null || true)"
    CRASH_REPORT_PATH="$(jq -r '.crashReportPath // .crash_report_path // ""' "$SELFTEST_META_JSON" 2>/dev/null || true)"
  else
    TERMINAL_REASON="$(awk -F= '/^terminal_failure_reason=/{v=$2} END{print v}' "$INVOCATION_LOG")"
    APP_EXIT_CODE="$(awk -F= '/^app_exit_code=/{v=$2} END{print v}' "$INVOCATION_LOG")"
    APP_SIGNAL="$(awk -F= '/^app_signal=/{v=$2} END{print v}' "$INVOCATION_LOG")"
    APP_SIGNAL_NAME="$(awk -F= '/^app_signal_name=/{v=$2} END{print v}' "$INVOCATION_LOG")"
    CRASH_REPORT_PATH="$(awk -F= '/^crash_report_path=/{v=$2} END{print v}' "$INVOCATION_LOG")"
  fi

  if ! is_path_newer_than_marker "$CRASH_REPORT_PATH" "$MARKER_FILE"; then
    CRASH_REPORT_PATH=""
  fi

  if [[ -z "$CRASH_REPORT_PATH" || ! -f "$CRASH_REPORT_PATH" ]]; then
    CRASH_REPORT_PATH="$(find_newest_crash_since "$MARKER_FILE")"
  fi

  if [[ -n "$CRASH_REPORT_PATH" && -f "$CRASH_REPORT_PATH" ]]; then
    if [[ -z "$APP_SIGNAL" ]]; then
      APP_SIGNAL="$(extract_crash_signal_number "$CRASH_REPORT_PATH")"
    fi
    if [[ -z "$APP_SIGNAL_NAME" ]]; then
      APP_SIGNAL_NAME="$(extract_crash_signal_name "$CRASH_REPORT_PATH")"
    fi
  fi

  MAIN_THREAD_FRAMES=""
  if [[ -n "$CRASH_REPORT_PATH" && -f "$CRASH_REPORT_PATH" ]]; then
    MAIN_THREAD_FRAMES="$(extract_main_thread_frames "$CRASH_REPORT_PATH")"
  fi

  PHASE_CLASSIFICATION="$(classify_phase "$TERMINAL_REASON" "$MAIN_THREAD_FRAMES")"
  FRAMES_SUMMARY="$(printf '%s\n' "$MAIN_THREAD_FRAMES" | head -n 5 | paste -sd ' | ' -)"

  if [[ -z "$TERMINAL_REASON" ]]; then
    if [[ "$SELFTEST_EXIT_CODE" -eq 0 ]]; then
      TERMINAL_REASON="none"
    else
      TERMINAL_REASON="unknown"
    fi
  fi

  if [[ -z "$FRAMES_SUMMARY" ]]; then
    FRAMES_SUMMARY="none"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$RUN_ID" \
    "$SCOPE" \
    "$BL009" \
    "$SELFTEST_EXIT_CODE" \
    "$SELFTEST_STATUS" \
    "$(sanitize_tsv_field "$TERMINAL_REASON")" \
    "$(sanitize_tsv_field "$APP_EXIT_CODE")" \
    "$(sanitize_tsv_field "$APP_SIGNAL")" \
    "$(sanitize_tsv_field "$APP_SIGNAL_NAME")" \
    "$PHASE_CLASSIFICATION" \
    "$(sanitize_tsv_field "$CRASH_REPORT_PATH")" \
    "$(sanitize_tsv_field "$FRAMES_SUMMARY")" \
    "$(sanitize_tsv_field "$SELFTEST_RESULT_JSON")" \
    "$(sanitize_tsv_field "$SELFTEST_META_JSON")" \
    "$(sanitize_tsv_field "$SELFTEST_RUN_LOG")" \
    "$(sanitize_tsv_field "$INVOCATION_LOG")" >> "$TAXONOMY_TSV"

  {
    echo "## Run ${RUN_ID}"
    echo
    echo "- selftest_exit_code: \`${SELFTEST_EXIT_CODE}\`"
    echo "- selftest_status: \`${SELFTEST_STATUS}\`"
    echo "- terminal_failure_reason: \`${TERMINAL_REASON}\`"
    echo "- app_exit_code: \`${APP_EXIT_CODE:-n/a}\`"
    echo "- app_signal: \`${APP_SIGNAL:-n/a}\`"
    echo "- app_signal_name: \`${APP_SIGNAL_NAME:-n/a}\`"
    echo "- phase_classification: \`${PHASE_CLASSIFICATION}\`"
    echo "- crash_report_path: \`${CRASH_REPORT_PATH:-none}\`"
    echo
    echo "Main-thread frames:"
    echo
    echo '```text'
    if [[ -n "$MAIN_THREAD_FRAMES" ]]; then
      printf '%s\n' "$MAIN_THREAD_FRAMES"
    else
      echo "(none)"
    fi
    echo '```'
    echo
  } >> "$CRASH_STACK_SAMPLES_MD"

  rm -f "$MARKER_FILE"
done

TOTAL_RUNS="$(awk 'NR>1{c++} END{print c+0}' "$TAXONOMY_TSV")"
PASS_RUNS="$(awk -F'\t' 'NR>1 && $4==0 {c++} END{print c+0}' "$TAXONOMY_TSV")"
FAIL_RUNS="$(awk -F'\t' 'NR>1 && $4!=0 {c++} END{print c+0}' "$TAXONOMY_TSV")"

PHASE_COUNTS="$(awk -F'\t' 'NR>1 {k=$10; if (k=="") k="unknown"; c[k]++} END {for (k in c) printf "%s\t%d\n", k, c[k]}' "$TAXONOMY_TSV" | sort)"
REASON_COUNTS="$(awk -F'\t' 'NR>1 {k=$6; if (k=="") k="unknown"; c[k]++} END {for (k in c) printf "%s\t%d\n", k, c[k]}' "$TAXONOMY_TSV" | sort)"

TOP_PHASE="$(awk -F'\t' 'NR>1 {k=$10; if (k=="") k="unknown"; c[k]++} END {max=0; top="unknown"; for (k in c) { if (c[k] > max) {max=c[k]; top=k} } print top}' "$TAXONOMY_TSV")"
TOP_REASON="$(awk -F'\t' 'NR>1 {k=$6; if (k=="") k="unknown"; c[k]++} END {max=0; top="unknown"; for (k in c) { if (c[k] > max) {max=c[k]; top=k} } print top}' "$TAXONOMY_TSV")"
FIRST_FAIL_REPRO_RESULT="$(awk -F'\t' 'NR>1 && $4!=0 {print $13; exit}' "$TAXONOMY_TSV")"

cat > "$TRIAGE_SUMMARY_MD" <<__SUMMARY__
Title: BL-029 ABRT Probe Triage Summary
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# ABRT Probe Triage Summary

- scope: \`${SCOPE}\`
- runs: \`${RUNS}\`
- bl009: \`${BL009}\`
- total_runs: \`${TOTAL_RUNS}\`
- pass_runs: \`${PASS_RUNS}\`
- fail_runs: \`${FAIL_RUNS}\`
- top_phase_classification: \`${TOP_PHASE}\`
- top_terminal_reason: \`${TOP_REASON}\`

## Phase Classification Counts

\`\`\`text
${PHASE_COUNTS}
\`\`\`

## Terminal Failure Reason Counts

\`\`\`text
${REASON_COUNTS}
\`\`\`

## Notes

- \`appkit_registration\` indicates top main-thread crash frames include AppKit registration/bootstrap symbols.
- \`pre_result_emit\` indicates the app terminated before selftest JSON was emitted.
- \`post_ui_bootstrap\` indicates WebView/UI bootstrap symbols appear before abort.
__SUMMARY__

cat > "$REPRO_COMMANDS_MD" <<__REPRO__
Title: BL-029 ABRT Probe Repro Commands
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# Repro Commands

## Minimal Direct Repro

\`\`\`bash
LOCUSQ_UI_SELFTEST_SCOPE=${SCOPE} \\
LOCUSQ_UI_SELFTEST_BL009=${BL009} \\
LOCUSQ_UI_SELFTEST_MAX_ATTEMPTS=1 \\
./scripts/standalone-ui-selftest-production-p0-mac.sh
\`\`\`

## Probe Repro

\`\`\`bash
./scripts/diagnose-selftest-abrt-mac.sh --scope ${SCOPE} --runs ${RUNS} --bl009 ${BL009} --out ${OUT_DIR}
\`\`\`

## First Failing Result Path

- \`${FIRST_FAIL_REPRO_RESULT:-none}\`
__REPRO__

echo "artifact_dir=${OUT_DIR}" | tee -a "$PROBE_LOG"
echo "crash_taxonomy=${TAXONOMY_TSV}" | tee -a "$PROBE_LOG"
echo "triage_summary=${TRIAGE_SUMMARY_MD}" | tee -a "$PROBE_LOG"
echo "crash_stack_samples=${CRASH_STACK_SAMPLES_MD}" | tee -a "$PROBE_LOG"
echo "repro_commands=${REPRO_COMMANDS_MD}" | tee -a "$PROBE_LOG"
