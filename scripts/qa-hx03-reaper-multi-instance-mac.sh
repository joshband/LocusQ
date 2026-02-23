#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
HEADLESS_SCRIPT="$ROOT_DIR/scripts/reaper-headless-render-smoke-mac.sh"
BUILD_INSTALL_SCRIPT="$ROOT_DIR/scripts/build-and-install-mac.sh"

INSTANCE_COUNT="${LQ_HX03_INSTANCE_COUNT:-3}"
START_STAGGER_SEC="${LQ_HX03_START_STAGGER_SEC:-1}"
SKIP_INSTALL="${LQ_HX03_SKIP_INSTALL:-0}"
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
BOOTSTRAP_TIMEOUT_SEC="${LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC:-45}"
RENDER_TIMEOUT_SEC="${LQ_REAPER_RENDER_TIMEOUT_SEC:-90}"
CHECK_CRASH_REPORTS="${LQ_HX03_CHECK_CRASH_REPORTS:-1}"
DIAGNOSTIC_REPORTS_DIR="${LQ_HX03_DIAGNOSTIC_REPORTS_DIR:-$HOME/Library/Logs/DiagnosticReports}"

OUT_DIR="$ROOT_DIR/TestEvidence/hx03_reaper_multi_instance_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
REPORT_MD="$OUT_DIR/report.md"

printf "step\tstatus\tdetail\n" >"$STATUS_TSV"

log_status() {
  local step="$1"
  local status="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$step" "$status" "$detail" | tee -a "$STATUS_TSV" >/dev/null
}

is_positive_integer() {
  [[ "$1" =~ ^[0-9]+$ ]] && [[ "$1" -ge 1 ]]
}

is_nonnegative_integer() {
  [[ "$1" =~ ^[0-9]+$ ]]
}

collect_new_crash_reports() {
  local ref_file="$1"
  local out_file="$2"

  if [[ ! -d "$DIAGNOSTIC_REPORTS_DIR" ]]; then
    : >"$out_file"
    return 0
  fi

  find "$DIAGNOSTIC_REPORTS_DIR" -maxdepth 1 -type f \
    \( -name 'REAPER-*.ips' -o -name 'REAPER_*.ips' -o -name 'reaper-*.ips' -o -name 'LocusQ-*.ips' \) \
    -newer "$ref_file" \
    | sort >"$out_file" || true
}

run_multi_instance_phase() {
  local phase_name="$1"
  local phase_dir="$OUT_DIR/$phase_name"
  local phase_fail=0
  local phase_ref="$phase_dir/phase_reference.timestamp"
  mkdir -p "$phase_dir"
  : >"$phase_ref"

  if pgrep -ix reaper >/dev/null 2>&1; then
    log_status "${phase_name}_preflight_reaper_running" "fail" "close_reaper_before_running_hx03"
    return 1
  fi
  log_status "${phase_name}_preflight_reaper_running" "pass" "reaper_not_running"

  local -a phase_pids=()
  local -a phase_logs=()

  for ((instance=1; instance<=INSTANCE_COUNT; instance+=1)); do
    local run_log="$phase_dir/instance_${instance}.log"
    phase_logs+=("$run_log")

    LQ_REAPER_REQUIRE_LOCUSQ="$REQUIRE_LOCUSQ" \
      LQ_REAPER_BOOTSTRAP_TIMEOUT_SEC="$BOOTSTRAP_TIMEOUT_SEC" \
      LQ_REAPER_RENDER_TIMEOUT_SEC="$RENDER_TIMEOUT_SEC" \
      "$HEADLESS_SCRIPT" --auto-bootstrap >"$run_log" 2>&1 &
    local launch_pid=$!
    phase_pids+=("$launch_pid")

    log_status "${phase_name}_launch_${instance}" "pass" "pid=${launch_pid}; log=${run_log}"

    if [[ "$instance" -lt "$INSTANCE_COUNT" && "$START_STAGGER_SEC" -gt 0 ]]; then
      sleep "$START_STAGGER_SEC"
    fi
  done

  for ((idx=0; idx<${#phase_pids[@]}; idx+=1)); do
    local instance_id=$((idx + 1))
    local run_log="${phase_logs[$idx]}"
    local artifact_json=""

    set +e
    wait "${phase_pids[$idx]}"
    local run_exit=$?
    set -e

    artifact_json="$(awk -F= '/^artifact=/{print $2}' "$run_log" | tail -n 1)"
    if [[ -z "$artifact_json" || ! -f "$artifact_json" ]]; then
      log_status "${phase_name}_instance_${instance_id}" "fail" "exit=${run_exit}; artifact_missing; log=${run_log}"
      phase_fail=1
      continue
    fi

    cp "$artifact_json" "$phase_dir/instance_${instance_id}_status.json"

    local lane_status
    local locusq_found
    local render_output_detected
    local render_exit
    local render_attempts
    lane_status="$(jq -r '.status' "$artifact_json")"
    locusq_found="$(jq -r '.locusqFxFound' "$artifact_json")"
    render_output_detected="$(jq -r '.renderOutputDetected' "$artifact_json")"
    render_exit="$(jq -r '.renderExitCode' "$artifact_json")"
    render_attempts="$(jq -r '.renderAttempts // 1' "$artifact_json")"

    if [[ "$run_exit" -ne 0 || "$lane_status" != "pass" || "$locusq_found" != "true" || "$render_output_detected" != "true" ]]; then
      log_status "${phase_name}_instance_${instance_id}" "fail" "exit=${run_exit}; status=${lane_status}; locusqFxFound=${locusq_found}; renderOutputDetected=${render_output_detected}; renderExitCode=${render_exit}; renderAttempts=${render_attempts}; log=${run_log}; artifact=${artifact_json}"
      phase_fail=1
    else
      log_status "${phase_name}_instance_${instance_id}" "pass" "renderExitCode=${render_exit}; renderAttempts=${render_attempts}; artifact=${artifact_json}"
    fi
  done

  if pgrep -ix reaper >/dev/null 2>&1; then
    ps -Ao pid,ppid,comm | rg -i "reaper" >"$phase_dir/lingering_reaper_processes.txt" || true
    log_status "${phase_name}_postcheck_reaper_processes" "fail" "lingering_reaper_processes_detected; details=${phase_dir}/lingering_reaper_processes.txt"
    phase_fail=1
  else
    log_status "${phase_name}_postcheck_reaper_processes" "pass" "reaper_processes_clean"
  fi

  if [[ "$CHECK_CRASH_REPORTS" == "1" ]]; then
    local crash_list="$phase_dir/new_crash_reports.txt"
    collect_new_crash_reports "$phase_ref" "$crash_list"
    local crash_count
    crash_count="$(wc -l <"$crash_list" | tr -d ' ')"
    if [[ "$crash_count" -gt 0 ]]; then
      log_status "${phase_name}_crash_reports" "fail" "new_reports=${crash_count}; list=${crash_list}"
      phase_fail=1
    else
      log_status "${phase_name}_crash_reports" "pass" "new_reports=0"
    fi
  else
    log_status "${phase_name}_crash_reports" "warn" "disabled_by_env=LQ_HX03_CHECK_CRASH_REPORTS"
  fi

  if [[ "$phase_fail" -ne 0 ]]; then
    return 1
  fi
  return 0
}

log_status "init" "pass" "ts=${TIMESTAMP}"
log_status "config" "pass" "instance_count=${INSTANCE_COUNT}; start_stagger_sec=${START_STAGGER_SEC}; skip_install=${SKIP_INSTALL}; require_locusq=${REQUIRE_LOCUSQ}; bootstrap_timeout_sec=${BOOTSTRAP_TIMEOUT_SEC}; render_timeout_sec=${RENDER_TIMEOUT_SEC}; check_crash_reports=${CHECK_CRASH_REPORTS}"

if ! is_positive_integer "$INSTANCE_COUNT"; then
  log_status "preflight_instance_count" "fail" "invalid=${INSTANCE_COUNT}"
  echo "FAIL: LQ_HX03_INSTANCE_COUNT must be a positive integer"
  echo "artifact_dir=$OUT_DIR"
  exit 2
fi
if ! is_nonnegative_integer "$START_STAGGER_SEC"; then
  log_status "preflight_start_stagger" "fail" "invalid=${START_STAGGER_SEC}"
  echo "FAIL: LQ_HX03_START_STAGGER_SEC must be a non-negative integer"
  echo "artifact_dir=$OUT_DIR"
  exit 2
fi

if [[ ! -x "$REAPER_BIN" ]]; then
  log_status "preflight_reaper_bin" "fail" "missing_or_not_executable=${REAPER_BIN}"
  echo "FAIL: REAPER executable missing: $REAPER_BIN"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi
log_status "preflight_reaper_bin" "pass" "${REAPER_BIN}"

if [[ ! -x "$HEADLESS_SCRIPT" ]]; then
  log_status "preflight_headless_script" "fail" "missing_or_not_executable=${HEADLESS_SCRIPT}"
  echo "FAIL: missing script: $HEADLESS_SCRIPT"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi
log_status "preflight_headless_script" "pass" "$HEADLESS_SCRIPT"

if ! command -v jq >/dev/null 2>&1; then
  log_status "preflight_jq" "fail" "jq_not_found"
  echo "FAIL: jq is required for HX-03 lane parsing"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi
log_status "preflight_jq" "pass" "$(command -v jq)"

if [[ "$SKIP_INSTALL" != "1" ]]; then
  if [[ ! -x "$BUILD_INSTALL_SCRIPT" ]]; then
    log_status "preflight_build_install_script" "fail" "missing_or_not_executable=${BUILD_INSTALL_SCRIPT}"
    echo "FAIL: missing script: $BUILD_INSTALL_SCRIPT"
    echo "artifact_dir=$OUT_DIR"
    exit 1
  fi

  INSTALL_LOG="$OUT_DIR/clean_cache_build_install.log"
  if LOCUSQ_REAPER_AUTO_QUIT=1 \
     LOCUSQ_REAPER_FORCE_KILL=1 \
     LOCUSQ_REFRESH_REAPER_CACHE=1 \
     LOCUSQ_REFRESH_AU_CACHE=1 \
     "$BUILD_INSTALL_SCRIPT" >"$INSTALL_LOG" 2>&1; then
    log_status "clean_cache_build_install" "pass" "log=$INSTALL_LOG"
  else
    log_status "clean_cache_build_install" "fail" "log=$INSTALL_LOG"
    echo "FAIL: clean-cache build/install step failed"
    echo "artifact_dir=$OUT_DIR"
    exit 1
  fi
else
  log_status "clean_cache_build_install" "warn" "skipped_by_env=LQ_HX03_SKIP_INSTALL"
fi

if ! run_multi_instance_phase "clean_cache"; then
  log_status "clean_cache_phase" "fail" "multi_instance_phase_failed"
  echo "FAIL: HX-03 clean-cache phase failed"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi
log_status "clean_cache_phase" "pass" "instances=${INSTANCE_COUNT}"

if ! run_multi_instance_phase "warm_cache"; then
  log_status "warm_cache_phase" "fail" "multi_instance_phase_failed"
  echo "FAIL: HX-03 warm-cache phase failed"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi
log_status "warm_cache_phase" "pass" "instances=${INSTANCE_COUNT}"

FAIL_COUNT="$(awk -F'\t' 'NR>1 && $2=="fail" { c++ } END { print c+0 }' "$STATUS_TSV")"
WARN_COUNT="$(awk -F'\t' 'NR>1 && $2=="warn" { c++ } END { print c+0 }' "$STATUS_TSV")"
PASS_COUNT="$(awk -F'\t' 'NR>1 && $2=="pass" { c++ } END { print c+0 }' "$STATUS_TSV")"

OVERALL="pass"
if [[ "$FAIL_COUNT" != "0" ]]; then
  OVERALL="fail"
elif [[ "$WARN_COUNT" != "0" ]]; then
  OVERALL="pass_with_warnings"
fi

cat >"$REPORT_MD" <<EOF
Title: HX-03 REAPER Multi-Instance Stability Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# HX-03 REAPER Multi-Instance Stability (${TIMESTAMP})

- overall: \`${OVERALL}\`
- instance_count_per_phase: \`${INSTANCE_COUNT}\`
- phase_count: \`2\` (clean_cache, warm_cache)
- start_stagger_sec: \`${START_STAGGER_SEC}\`
- skip_install: \`${SKIP_INSTALL}\`
- require_locusq: \`${REQUIRE_LOCUSQ}\`
- bootstrap_timeout_sec: \`${BOOTSTRAP_TIMEOUT_SEC}\`
- render_timeout_sec: \`${RENDER_TIMEOUT_SEC}\`
- check_crash_reports: \`${CHECK_CRASH_REPORTS}\`
- pass_count: \`${PASS_COUNT}\`
- warn_count: \`${WARN_COUNT}\`
- fail_count: \`${FAIL_COUNT}\`

## Artifacts

- \`status.tsv\`
- \`clean_cache/instance_*.log\`
- \`clean_cache/instance_*_status.json\`
- \`clean_cache/new_crash_reports.txt\`
- \`warm_cache/instance_*.log\`
- \`warm_cache/instance_*_status.json\`
- \`warm_cache/new_crash_reports.txt\`
- \`clean_cache_build_install.log\` (unless install skipped)
EOF

if [[ "$OVERALL" == "fail" ]]; then
  echo "FAIL: HX-03 REAPER multi-instance stability lane failed"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

if [[ "$OVERALL" == "pass_with_warnings" ]]; then
  echo "PASS_WITH_WARNINGS: HX-03 REAPER multi-instance stability lane completed"
else
  echo "PASS: HX-03 REAPER multi-instance stability lane completed"
fi
echo "artifact_dir=$OUT_DIR"
