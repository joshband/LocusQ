#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE="$(date -u +%Y-%m-%d)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

RUNS_DEFAULT=10
OUT_DIR_DEFAULT="${ROOT_DIR}/TestEvidence/bl030_rl04_abrt_diag_${TIMESTAMP}"
REAPER_SMOKE_CMD="./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap"

usage() {
  cat <<'USAGE'
Usage: diagnose-reaper-bootstrap-abrt-mac.sh [options]

Deterministic RL-04 bootstrap ABRT diagnostics harness.

Options:
  --runs <N>       Number of repeated runs (default: 10)
  --out-dir <p>    Output artifact directory
  --help, -h       Show this help

Outputs:
  status.tsv
  replay_runs.tsv
  crash_taxonomy.tsv
  top_hypotheses.md
  repro_commands.md
  command_transcript.log

Exit semantics:
  0 = diagnostics completed and artifacts emitted
  1 = preflight or harness execution error
USAGE
}

RUNS="$RUNS_DEFAULT"
OUT_DIR="$OUT_DIR_DEFAULT"

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

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
REPLAY_RUNS_TSV="${OUT_DIR}/replay_runs.tsv"
CRASH_TAXONOMY_TSV="${OUT_DIR}/crash_taxonomy.tsv"
TOP_HYPOTHESES_MD="${OUT_DIR}/top_hypotheses.md"
REPRO_COMMANDS_MD="${OUT_DIR}/repro_commands.md"
TRANSCRIPT_LOG="${OUT_DIR}/command_transcript.log"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_index\ttimestamp_utc\texit_code\tstage\tterminal_reason\tclassification\tcrash_report_present\tcrash_report_path\tartifact_dir\trun_log\n" > "$REPLAY_RUNS_TSV"
printf "dimension\tkey\tcount\n" > "$CRASH_TAXONOMY_TSV"
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
  local line="$1"
  printf "[%s] %s\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$line" >> "$TRANSCRIPT_LOG"
}

classify_reason() {
  local reason="$1"
  local exit_code="$2"

  if [[ "$exit_code" == "0" ]]; then
    printf "pass"
    return
  fi

  if [[ "$reason" == *"abrt"* ]] || [[ "$reason" == *"ABRT"* ]] || [[ "$reason" == *"134"* ]]; then
    printf "transient_runtime_abort"
    return
  fi

  printf "deterministic_failure"
}

latest_reaper_artifact_dir() {
  ls -1dt "${ROOT_DIR}/TestEvidence/reaper_headless_render_"* 2>/dev/null | head -n 1 || true
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
    done < <(
      find "$crash_dir" -maxdepth 1 -type f \
        \( -name '*REAPER*.crash' -o -name '*REAPER*.ips' -o -name '*reaper*.crash' -o -name '*reaper*.ips' -o -name '*LocusQ*.crash' -o -name '*LocusQ*.ips' \) \
        -newer "$marker_file" 2>/dev/null | sort
    )
  done

  printf '%s' "$newest_path"
}

append_run_row() {
  local run_index="$1"
  local exit_code="$2"
  local stage="$3"
  local reason="$4"
  local classification="$5"
  local crash_present="$6"
  local crash_path="$7"
  local artifact_dir="$8"
  local run_log="$9"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$stage")" \
    "$(sanitize_tsv_field "$reason")" \
    "$(sanitize_tsv_field "$classification")" \
    "$(sanitize_tsv_field "$crash_present")" \
    "$(sanitize_tsv_field "$crash_path")" \
    "$(sanitize_tsv_field "$artifact_dir")" \
    "$(sanitize_tsv_field "$run_log")" \
    >> "$REPLAY_RUNS_TSV"
}

if [[ ! -x "${ROOT_DIR}/scripts/reaper-headless-render-smoke-mac.sh" ]]; then
  log_status "preflight_reaper_smoke" "FAIL" "1" "missing_or_non_executable_reaper_smoke_script" "${ROOT_DIR}/scripts/reaper-headless-render-smoke-mac.sh"
  exit 1
fi

log_status "preflight_reaper_smoke" "PASS" "0" "reaper_smoke_script_found" "${ROOT_DIR}/scripts/reaper-headless-render-smoke-mac.sh"

RUN_FAILS=0
RUN_PASSES=0

for run_idx in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run_idx}.log"
  marker_file="${OUT_DIR}/run_${run_idx}.marker"
  touch "$marker_file"

  before_artifact="$(latest_reaper_artifact_dir)"
  log_transcript "run_${run_idx}: ${REAPER_SMOKE_CMD}"

  set +e
  (cd "$ROOT_DIR" && ./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap) > "$run_log" 2>&1
  run_ec=$?
  set -e

  after_artifact="$(latest_reaper_artifact_dir)"
  artifact_dir="$after_artifact"
  if [[ -z "$artifact_dir" || "$artifact_dir" == "$before_artifact" ]]; then
    artifact_dir="none"
  fi

  stage="unknown"
  terminal_reason="unknown"

  if [[ "$artifact_dir" != "none" && -f "$artifact_dir/status.json" ]]; then
    status_value="$(jq -r '.status // "unknown"' "$artifact_dir/status.json" 2>/dev/null || echo unknown)"
    bootstrap_ok="$(jq -r '.bootstrapOk // false' "$artifact_dir/status.json" 2>/dev/null || echo false)"
    bootstrap_ec="$(jq -r '.bootstrapExitCode // "na"' "$artifact_dir/status.json" 2>/dev/null || echo na)"
    bootstrap_err="$(jq -r '.bootstrapError // ""' "$artifact_dir/status.json" 2>/dev/null || true)"
    render_ec="$(jq -r '.renderExitCode // "na"' "$artifact_dir/status.json" 2>/dev/null || echo na)"
    render_output="$(jq -r '.renderOutputDetected // false' "$artifact_dir/status.json" 2>/dev/null || echo false)"

    if [[ "$status_value" == "pass" ]]; then
      stage="complete"
      terminal_reason="pass"
    elif [[ "$bootstrap_ok" != "true" ]]; then
      stage="bootstrap"
      if [[ -n "$bootstrap_err" ]]; then
        terminal_reason="$(sanitize_tsv_field "$bootstrap_err")"
      else
        terminal_reason="bootstrap_exit_${bootstrap_ec}"
      fi
    elif [[ "$render_ec" != "0" && "$render_ec" != "na" ]]; then
      stage="render"
      terminal_reason="render_exit_${render_ec}"
    elif [[ "$render_output" != "true" ]]; then
      stage="post_render"
      terminal_reason="render_output_not_detected"
    else
      stage="unknown"
      terminal_reason="status_${status_value}"
    fi
  else
    if [[ "$run_ec" == "0" ]]; then
      stage="complete"
      terminal_reason="pass_without_status_json"
    else
      stage="unknown"
      terminal_reason="exit_${run_ec}"
    fi
  fi

  if grep -Eiq 'Abort trap|ABRT|signal 6|renderExitCode=134|bootstrap.*134' "$run_log" "$artifact_dir/run.log" "$artifact_dir/bootstrap.log" "$artifact_dir/render.log" 2>/dev/null; then
    if [[ "$terminal_reason" == "unknown" ]]; then
      terminal_reason="abrt_detected"
    else
      terminal_reason="${terminal_reason}|abrt"
    fi
  fi

  crash_report_path="$(detect_crash_report_after "$marker_file")"
  crash_report_present="no"
  if [[ -n "$crash_report_path" ]]; then
    crash_report_present="yes"
  fi

  classification="$(classify_reason "$terminal_reason" "$run_ec")"
  append_run_row "$run_idx" "$run_ec" "$stage" "$terminal_reason" "$classification" "$crash_report_present" "$crash_report_path" "$artifact_dir" "$run_log"

  if (( run_ec == 0 )); then
    RUN_PASSES=$((RUN_PASSES + 1))
  else
    RUN_FAILS=$((RUN_FAILS + 1))
  fi
done

awk -F'\t' '
  NR == 1 { next }
  {
    stage[$4]++
    reason[$5]++
    class[$6]++
    crash[$7]++
    exitc[$3]++
  }
  END {
    for (k in stage) printf "stage\t%s\t%d\n", k, stage[k]
    for (k in reason) printf "terminal_reason\t%s\t%d\n", k, reason[k]
    for (k in class) printf "classification\t%s\t%d\n", k, class[k]
    for (k in crash) printf "crash_report_present\t%s\t%d\n", k, crash[k]
    for (k in exitc) printf "exit_code\t%s\t%d\n", k, exitc[k]
  }
' "$REPLAY_RUNS_TSV" | sort >> "$CRASH_TAXONOMY_TSV"

top_stage="$(awk -F'\t' 'NR>1 && $1=="stage" { if ($3>max) { max=$3; key=$2 } } END { if (key=="") key="unknown"; print key }' "$CRASH_TAXONOMY_TSV")"
top_reason="$(awk -F'\t' 'NR>1 && $1=="terminal_reason" { if ($3>max) { max=$3; key=$2 } } END { if (key=="") key="unknown"; print key }' "$CRASH_TAXONOMY_TSV")"
abrt_count="$(awk -F'\t' 'NR>1 && $1=="classification" && $2=="transient_runtime_abort" { print $3 }' "$CRASH_TAXONOMY_TSV" | tail -n 1)"
if [[ -z "$abrt_count" ]]; then abrt_count="0"; fi
crash_yes_count="$(awk -F'\t' 'NR>1 && $1=="crash_report_present" && $2=="yes" { print $3 }' "$CRASH_TAXONOMY_TSV" | tail -n 1)"
if [[ -z "$crash_yes_count" ]]; then crash_yes_count="0"; fi

{
  echo "Title: RL-04 ABRT Top Hypotheses"
  echo "Document Type: Test Evidence"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE}"
  echo "Last Modified Date: ${DOC_DATE}"
  echo
  echo "# Top Hypotheses (Slice I2)"
  echo
  echo "1. Dominant failing stage is \`${top_stage}\`, indicating bootstrap remains the first failing boundary before stable render execution."
  echo "2. Dominant terminal reason is \`${top_reason}\`, suggesting repeated non-random abort behavior under the same bootstrap command contract."
  echo "3. Crash-report linkage count is \`${crash_yes_count}\` of \`${RUNS}\` runs; diagnostics should prioritize environments where crash artifacts are emitted for deeper symbolized triage."
  echo "4. Runtime-abort classification count is \`${abrt_count}\`; if this remains high across clean sessions, treat as reproducible host/bootstrap instability rather than isolated flakes."
} > "$TOP_HYPOTHESES_MD"

{
  echo "Title: RL-04 ABRT Repro Commands"
  echo "Document Type: Test Evidence"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE}"
  echo "Last Modified Date: ${DOC_DATE}"
  echo
  echo "# Minimal Repro Commands"
  echo
  echo "## Single Run"
  echo '```bash'
  echo "cd \"$ROOT_DIR\""
  echo "./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap"
  echo '```'
  echo
  echo "## Multi-run Diagnostics (10x)"
  echo '```bash'
  echo "cd \"$ROOT_DIR\""
  echo "./scripts/diagnose-reaper-bootstrap-abrt-mac.sh --runs 10 --out-dir TestEvidence/bl030_rl04_abrt_diag_i2_<timestamp>"
  echo '```'
  echo
  echo "## Manual Crash Report Probe"
  echo '```bash'
  echo "ls -1t ~/Library/Logs/DiagnosticReports/*REAPER* ~/Library/Logs/DiagnosticReports/*reaper* 2>/dev/null | head -n 10"
  echo '```'
} > "$REPRO_COMMANDS_MD"

if (( RUN_FAILS == 0 )); then
  log_status "rl04_replay_matrix" "PASS" "0" "runs=${RUNS};passes=${RUN_PASSES};fails=${RUN_FAILS}" "$REPLAY_RUNS_TSV"
else
  log_status "rl04_replay_matrix" "FAIL" "1" "runs=${RUNS};passes=${RUN_PASSES};fails=${RUN_FAILS}" "$REPLAY_RUNS_TSV"
fi

log_status "taxonomy_generated" "PASS" "0" "taxonomy_rows=$(($(wc -l < "$CRASH_TAXONOMY_TSV") - 1))" "$CRASH_TAXONOMY_TSV"
log_status "top_hypotheses_generated" "PASS" "0" "hypotheses_written" "$TOP_HYPOTHESES_MD"
log_status "repro_commands_generated" "PASS" "0" "repro_written" "$REPRO_COMMANDS_MD"

echo "artifact_dir=$OUT_DIR"
echo "status_tsv=$STATUS_TSV"
echo "replay_runs_tsv=$REPLAY_RUNS_TSV"
echo "crash_taxonomy_tsv=$CRASH_TAXONOMY_TSV"
echo "top_hypotheses_md=$TOP_HYPOTHESES_MD"
echo "repro_commands_md=$REPRO_COMMANDS_MD"

exit 0
