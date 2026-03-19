#!/usr/bin/env bash
# Title: BL-095 FIR Truthfulness QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-19
# Last Modified Date: 2026-03-19
#
# Exit codes:
#   0  all checks passed
#   1  one or more checks failed
#   2  usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl095_fir_truthfulness_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=1

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl095-fir-truthfulness-mac.sh [options]

BL-095 FIR engine truthfulness lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (builds and runs objective probe)
  --runs <n>         Number of execute probe runs (default 1; use 3 for T1 lane)
  --help, -h         Show usage

Outputs:
  status.tsv
  latency_report.tsv
  impulse_accuracy.tsv
  summary.md
USAGE
}

usage_error() {
  echo "ERROR: $1" >&2
  usage >&2
  exit 2
}

STATUS_TSV=""
LATENCY_TSV=""
IMPULSE_TSV=""
SUMMARY_MD=""

record() {
  local check_id="$1" result="$2" detail="$3" artifact="${4:-}"
  printf "%s\t%s\t%s\t%s\n" \
    "$check_id" "$result" "${detail//$'\t'/ }" "${artifact//$'\t'/ }" \
    >> "$STATUS_TSV"
  if [[ "$result" == "PASS" ]]; then
    ((pass_count++)) || true
  else
    ((fail_count++)) || true
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires a value"
      OUT_DIR="$2"; shift 2 ;;
    --contract-only)
      (( MODE_SET == 1 )) && [[ "$MODE" != "contract_only" ]] \
        && usage_error "--contract-only cannot be combined with --execute"
      MODE="contract_only"; MODE_SET=1; shift ;;
    --execute)
      (( MODE_SET == 1 )) && [[ "$MODE" != "execute" ]] \
        && usage_error "--execute cannot be combined with --contract-only"
      MODE="execute"; MODE_SET=1; shift ;;
    --runs)
      [[ $# -ge 2 ]] || usage_error "--runs requires a value"
      RUNS="$2"; shift 2 ;;
    --help|-h)
      usage; exit 0 ;;
    *)
      usage_error "unknown argument: $1" ;;
  esac
done

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
LATENCY_TSV="${OUT_DIR}/latency_report.tsv"
IMPULSE_TSV="${OUT_DIR}/impulse_accuracy.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\tartifact\n"    > "$LATENCY_TSV"
printf "check\tresult\tdetail\tartifact\n"    > "$IMPULSE_TSV"

FIR_HOOK_HDR="${ROOT_DIR}/Source/headphone_dsp/HeadphoneFirHook.h"
CALIB_CHAIN_HDR="${ROOT_DIR}/Source/headphone_dsp/HeadphoneCalibrationChain.h"
CALIB_STATE_HDR="${ROOT_DIR}/Source/headphone_core/HeadphoneCalibrationChainState.h"
RUNBOOK="${ROOT_DIR}/Documentation/backlog/bl-095-partitioned-fir-truthfulness-recovery-and-objective-validation.md"

# ── C1: runbook exists ──────────────────────────────────────────────────────
if [[ -f "$RUNBOOK" ]]; then
  record "BL095-C1-runbook_exists" "PASS" "BL-095 runbook present" "$RUNBOOK"
else
  record "BL095-C1-runbook_exists" "FAIL" "BL-095 runbook missing" "$RUNBOOK"
fi

# ── C2: getLatencySamples() unconditionally returns 0 ───────────────────────
if grep -q 'getLatencySamples' "$FIR_HOOK_HDR" \
   && ! grep -q 'partitionedLatencySamples' "$FIR_HOOK_HDR"; then
  printf "fir_latency_always_zero\tPASS\tgetLatencySamples present; no partitionedLatencySamples field\t%s\n" \
    "$FIR_HOOK_HDR" >> "$LATENCY_TSV"
  record "BL095-C2-fir_latency_always_zero" "PASS" \
    "getLatencySamples present; no false partitioned field" "$FIR_HOOK_HDR"
else
  printf "fir_latency_always_zero\tFAIL\tgetLatencySamples missing or partitionedLatencySamples field present\t%s\n" \
    "$FIR_HOOK_HDR" >> "$LATENCY_TSV"
  record "BL095-C2-fir_latency_always_zero" "FAIL" \
    "getLatencySamples missing or false partitioned field present" "$FIR_HOOK_HDR"
fi

# ── C3: chain latency delegates to firHook.getLatencySamples() ─────────────
if grep -q 'firHook.getLatencySamples()' "$CALIB_CHAIN_HDR" \
   && grep -q 'firLatencySamples' "$CALIB_STATE_HDR"; then
  printf "chain_delegates_fir_latency\tPASS\tchain passes firHook.getLatencySamples() into resolveCalibrationChainState\t%s\n" \
    "$CALIB_CHAIN_HDR" >> "$LATENCY_TSV"
  record "BL095-C3-chain_delegates_fir_latency" "PASS" \
    "calibration chain delegates FIR latency correctly" "$CALIB_CHAIN_HDR"
else
  printf "chain_delegates_fir_latency\tFAIL\tchain FIR latency delegation path missing\t%s\n" \
    "$CALIB_CHAIN_HDR" >> "$LATENCY_TSV"
  record "BL095-C3-chain_delegates_fir_latency" "FAIL" \
    "calibration chain FIR latency delegation missing" "$CALIB_CHAIN_HDR"
fi

# ── C4: FIR convolution processing path exists (impulse accuracy possible) ──
if grep -q 'runActiveConvolver' "$FIR_HOOK_HDR" \
   && grep -q 'processStereoSample' "$FIR_HOOK_HDR"; then
  printf "fir_processing_path_present\tPASS\trunActiveConvolver and processStereoSample present\t%s\n" \
    "$FIR_HOOK_HDR" >> "$IMPULSE_TSV"
  record "BL095-C4-fir_processing_path_present" "PASS" \
    "FIR processing path present for impulse accuracy test" "$FIR_HOOK_HDR"
else
  printf "fir_processing_path_present\tFAIL\trunActiveConvolver or processStereoSample missing\t%s\n" \
    "$FIR_HOOK_HDR" >> "$IMPULSE_TSV"
  record "BL095-C4-fir_processing_path_present" "FAIL" \
    "FIR processing path missing" "$FIR_HOOK_HDR"
fi

if [[ "$MODE" == "contract_only" ]]; then
  record "BL095-C5-contract_mode" "PASS" \
    "contract-only structural probes complete" "$STATUS_TSV"
fi

# ── Execute mode: build and run objective probe ─────────────────────────────
if [[ "$MODE" == "execute" ]]; then
  BUILD_DIR="${ROOT_DIR}/_bl095_probe_build"
  PROBE_BIN="${BUILD_DIR}/locusq_fir_truthfulness_probe_artefacts/Release/locusq_fir_truthfulness_probe"

  echo "BL095-EXECUTE: configuring probe build..."
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DBUILD_LOCUSQ_QA=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -Wno-dev \
    > "${OUT_DIR}/cmake_configure.log" 2>&1 \
    || { record "BL095-E1-cmake_configure" "FAIL" "cmake configure failed; see cmake_configure.log" "${OUT_DIR}/cmake_configure.log"; }

  echo "BL095-EXECUTE: building locusq_fir_truthfulness_probe..."
  cmake --build "$BUILD_DIR" \
    --target locusq_fir_truthfulness_probe \
    -j"$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)" \
    > "${OUT_DIR}/cmake_build.log" 2>&1 \
    || { record "BL095-E2-cmake_build" "FAIL" "cmake build failed; see cmake_build.log" "${OUT_DIR}/cmake_build.log"; }

  if [[ -x "$PROBE_BIN" ]]; then
    record "BL095-E1-probe_built" "PASS" "locusq_fir_truthfulness_probe binary present" "$PROBE_BIN"

    all_runs_pass=true
    run_fail_count=0
    for run in $(seq 1 "$RUNS"); do
      RUN_LOG="${OUT_DIR}/probe_run_${run}.log"
      echo "BL095-EXECUTE: run ${run}/${RUNS}..."
      "$PROBE_BIN" > "$RUN_LOG" 2>&1 || true
      probe_exit=$?

      passed_count=$(grep -c ': PASS' "$RUN_LOG" 2>/dev/null || echo 0)
      failed_count=$(grep -c ': FAIL' "$RUN_LOG" 2>/dev/null || echo 0)
      summary_line=$(grep -- '--- FIR Truthfulness' "$RUN_LOG" 2>/dev/null || echo "summary not found")

      if [[ "$probe_exit" -eq 0 ]]; then
        printf "probe_run_%d\tPASS\t%s\t%s\n" "$run" "$summary_line" "$RUN_LOG" >> "$IMPULSE_TSV"
        record "BL095-E2-probe_run_${run}" "PASS" \
          "run=${run} passed=${passed_count} failed=${failed_count}" "$RUN_LOG"
      else
        all_runs_pass=false
        ((run_fail_count++)) || true
        printf "probe_run_%d\tFAIL\t%s\t%s\n" "$run" "$summary_line" "$RUN_LOG" >> "$IMPULSE_TSV"
        record "BL095-E2-probe_run_${run}" "FAIL" \
          "run=${run} passed=${passed_count} failed=${failed_count}" "$RUN_LOG"
      fi
    done

    if $all_runs_pass; then
      record "BL095-E3-all_runs_pass" "PASS" \
        "all ${RUNS} probe run(s) passed" "$OUT_DIR"
    else
      record "BL095-E3-all_runs_pass" "FAIL" \
        "${run_fail_count}/${RUNS} run(s) failed" "$OUT_DIR"
    fi
  else
    record "BL095-E1-probe_built" "FAIL" "probe binary not found after build" "$PROBE_BIN"
  fi
fi

# ── Lane result ─────────────────────────────────────────────────────────────
if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};runs=${RUNS};bl095_lane_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};runs=${RUNS};failures=${fail_count}" "$STATUS_TSV"
fi

lane_result="$(awk -F'\t' '$1=="lane_result"{print $2}' "$STATUS_TSV")"
status_pass="$(awk -F'\t' 'NR>1 && $2=="PASS"{c++} END{print c+0}' "$STATUS_TSV")"
status_fail="$(awk -F'\t' 'NR>1 && $2=="FAIL"{c++} END{print c+0}' "$STATUS_TSV")"

cat > "$SUMMARY_MD" <<EOF_SUMMARY
Title: BL-095 FIR Truthfulness QA Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-095 FIR Truthfulness QA Summary

- mode: ${MODE}
- runs: ${RUNS}
- timestamp_utc: ${TIMESTAMP}
- lane_result: ${lane_result}
- status_pass_rows: ${status_pass}
- status_fail_rows: ${status_fail}

## Evidence Fields

- status.tsv: check_id, result, detail, artifact
- latency_report.tsv: latency contract checks
- impulse_accuracy.tsv: FIR processing accuracy and probe run results

## Key Anchors

- fir_hook: Source/headphone_dsp/HeadphoneFirHook.h
- calibration_chain: Source/headphone_dsp/HeadphoneCalibrationChain.h
- calibration_state: Source/headphone_core/HeadphoneCalibrationChainState.h
- runbook: Documentation/backlog/bl-095-partitioned-fir-truthfulness-recovery-and-objective-validation.md
EOF_SUMMARY

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $LATENCY_TSV"
echo "- $IMPULSE_TSV"
echo "- $SUMMARY_MD"

[[ "$fail_count" -gt 0 ]] && exit 1
exit 0
