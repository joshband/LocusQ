#!/usr/bin/env bash
# Title: BL-074 WebView Runtime Reliability Diagnostics
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-03
# Last Modified Date: 2026-03-03
#
# Purpose:
# - Validate strict_gesture enforcement in the WebView self-test lane.
# - Validate degraded-mode startup control-lock behavior when critical native bindings fail.
# - Validate centralized native/bridge diagnostics schema surfacing for operators.
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl074_webview_reliability_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=""
RUNS_SET=0
DIAGNOSTICS_SCHEMA="locusq-native-bridge-diagnostics-v1"

STATUS_TSV=""
STRICT_GESTURE_TSV=""
DEGRADED_MODE_TSV=""
NATIVE_ERROR_SURFACE_TSV=""
OPERATOR_DIAGNOSTICS_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl074-webview-reliability-diagnostics-mac.sh [options]

BL-074 WebView reliability diagnostics lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode checks (same checks + strict pass/fail enforcement)
  --runs <N>         Number of deterministic replay runs
  --help, -h         Show usage

Outputs:
  status.tsv
  strict_gesture_matrix.tsv
  degraded_mode_contract.tsv
  native_error_surface.tsv
  operator_diagnostics_snapshot.md
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

sanitize_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf "%s" "$value"
}

record_status() {
  local check_id="$1"
  local result="$2"
  local detail="$3"
  local artifact="${4:-}"

  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$check_id")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$STATUS_TSV"

  if [[ "$result" == "PASS" ]]; then
    ((pass_count++)) || true
  else
    ((fail_count++)) || true
  fi
}

append_matrix_row() {
  local file="$1"
  local run_id="$2"
  local check_id="$3"
  local result="$4"
  local detail="$5"
  local artifact="$6"

  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$run_id")" \
    "$(sanitize_field "$check_id")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$file"
}

run_pattern_check() {
  local matrix_file="$1"
  local run_id="$2"
  local check_id="$3"
  local file_path="$4"
  local pattern="$5"
  local pass_detail="$6"
  local fail_detail="$7"

  if rg -q --pcre2 "$pattern" "$file_path"; then
    append_matrix_row "$matrix_file" "$run_id" "$check_id" "PASS" "$pass_detail" "$file_path"
    record_status "$check_id" "PASS" "$pass_detail" "$file_path"
  else
    append_matrix_row "$matrix_file" "$run_id" "$check_id" "FAIL" "$fail_detail" "$file_path"
    record_status "$check_id" "FAIL" "$fail_detail" "$file_path"
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires a value"
      OUT_DIR="$2"
      shift 2
      ;;
    --contract-only)
      if (( MODE_SET == 1 )) && [[ "$MODE" != "contract_only" ]]; then
        usage_error "--contract-only cannot be combined with --execute"
      fi
      MODE="contract_only"
      MODE_SET=1
      shift
      ;;
    --execute)
      if (( MODE_SET == 1 )) && [[ "$MODE" != "execute" ]]; then
        usage_error "--execute cannot be combined with --contract-only"
      fi
      MODE="execute"
      MODE_SET=1
      shift
      ;;
    --runs)
      [[ $# -ge 2 ]] || usage_error "--runs requires a value"
      RUNS="$2"
      RUNS_SET=1
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      usage_error "unknown argument: $1"
      ;;
  esac
done

if (( RUNS_SET == 0 )); then
  if [[ "$MODE" == "contract_only" ]]; then
    RUNS=3
  else
    RUNS=1
  fi
fi

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  usage_error "--runs must be an integer >= 1"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
STRICT_GESTURE_TSV="${OUT_DIR}/strict_gesture_matrix.tsv"
DEGRADED_MODE_TSV="${OUT_DIR}/degraded_mode_contract.tsv"
NATIVE_ERROR_SURFACE_TSV="${OUT_DIR}/native_error_surface.tsv"
OPERATOR_DIAGNOSTICS_MD="${OUT_DIR}/operator_diagnostics_snapshot.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$STRICT_GESTURE_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$DEGRADED_MODE_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$NATIVE_ERROR_SURFACE_TSV"

JS_FILE="${ROOT_DIR}/Source/ui/public/js/index.js"
SCENE_BRIDGE_FILE="${ROOT_DIR}/Source/processor_bridge/ProcessorSceneStateBridgeOps.h"
RUNBOOK_FILE="${ROOT_DIR}/Documentation/backlog/bl-074-webview-runtime-reliability-diagnostics-strict-gesture-and-degraded-mode.md"

for required in "$JS_FILE" "$SCENE_BRIDGE_FILE" "$RUNBOOK_FILE"; do
  if [[ -f "$required" ]]; then
    record_status "BL074-PRE-file_exists-$(basename "$required")" "PASS" "required file present" "$required"
  else
    record_status "BL074-PRE-file_exists-$(basename "$required")" "FAIL" "required file missing" "$required"
  fi
done

for run_id in $(seq 1 "$RUNS"); do
  # strict_gesture enforcement contract
  run_pattern_check "$STRICT_GESTURE_TSV" "$run_id" "BL074-SG-001" "$JS_FILE" 'strictGestureModeEnabled\s*=\s*parseQueryBooleanFlag\(queryParams,\s*"strict_gesture"\)' \
    "strict_gesture query flag parsed" \
    "strict_gesture query flag parsing missing"

  run_pattern_check "$STRICT_GESTURE_TSV" "$run_id" "BL074-SG-002" "$JS_FILE" 'strictGestureModeEnabled\s*&&\s*gestureFallbacks\.length\s*>\s*0' \
    "strict_gesture fallback gate present" \
    "strict_gesture fallback gate missing"

  run_pattern_check "$STRICT_GESTURE_TSV" "$run_id" "BL074-SG-003" "$JS_FILE" 'strict_gesture enabled; fallback gesture path used' \
    "strict_gesture fail detail emitted" \
    "strict_gesture fail detail missing"

  run_pattern_check "$STRICT_GESTURE_TSV" "$run_id" "BL074-SG-004" "$JS_FILE" 'strict_gesture=\$\{strictGestureModeEnabled \? "on" : "off"\}' \
    "strict_gesture surfaced in operator/self-test detail" \
    "strict_gesture operator/self-test detail missing"

  # degraded startup/control-lock contract
  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-001" "$JS_FILE" 'nativeBridgeDegraded:\s*false' \
    "runtime degraded-state field declared" \
    "runtime degraded-state field missing"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-002" "$JS_FILE" 'function\s+setNativeBridgeDegradedMode\(' \
    "degraded-mode state transition function present" \
    "degraded-mode state transition function missing"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-003" "$JS_FILE" 'function\s+applyNativeBridgeControlLock\(' \
    "degraded-mode control lock function present" \
    "degraded-mode control lock function missing"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-004" "$JS_FILE" 'nativeBridgeDegradedControlIds' \
    "degraded control ownership list declared" \
    "degraded control ownership list missing"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-004A" "$JS_FILE" '"preset-save-btn"' \
    "degraded control ownership includes preset save control" \
    "degraded control ownership missing preset save control"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-004B" "$JS_FILE" '"cal-start-btn"' \
    "degraded control ownership includes calibration start control" \
    "degraded control ownership missing calibration start control"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-004C" "$JS_FILE" '"timeline-play-btn"' \
    "degraded control ownership includes timeline play control" \
    "degraded control ownership missing timeline play control"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-005" "$JS_FILE" 'evaluateNativeBridgeBindingContract\(\);' \
    "startup binding contract evaluation invoked" \
    "startup binding contract evaluation missing"

  run_pattern_check "$DEGRADED_MODE_TSV" "$run_id" "BL074-DM-006" "$JS_FILE" 'controls disabled in degraded mode' \
    "degraded mode operator copy confirms controls disabled" \
    "degraded mode operator copy missing"

  # centralized native/bridge diagnostics surface
  run_pattern_check "$NATIVE_ERROR_SURFACE_TSV" "$run_id" "BL074-NE-001" "$JS_FILE" 'nativeBridgeDiagnosticsState\.callsFailed\s*\+=' \
    "native failure counter increments on call errors" \
    "native failure counter increment missing"

  run_pattern_check "$NATIVE_ERROR_SURFACE_TSV" "$run_id" "BL074-NE-002" "$JS_FILE" 'callsTimeout|callsUnavailable|callsBlocked' \
    "native failure classification counters present" \
    "native failure classification counters missing"

  run_pattern_check "$NATIVE_ERROR_SURFACE_TSV" "$run_id" "BL074-NE-003" "$JS_FILE" 'window\.__LQ_NATIVE_BRIDGE_DIAGNOSTICS__' \
    "native diagnostics channel exported" \
    "native diagnostics channel export missing"

  run_pattern_check "$NATIVE_ERROR_SURFACE_TSV" "$run_id" "BL074-NE-004" "$JS_FILE" 'window\.__LQ_OPERATOR_DIAGNOSTICS__' \
    "operator diagnostics channel exported" \
    "operator diagnostics channel export missing"

  run_pattern_check "$NATIVE_ERROR_SURFACE_TSV" "$run_id" "BL074-NE-005" "$JS_FILE" 'function\s+hasNativeBridgeDiagnosticsPayload\(' \
    "UI parser recognizes native bridge diagnostics payload" \
    "UI parser for native bridge diagnostics payload missing"

  run_pattern_check "$NATIVE_ERROR_SURFACE_TSV" "$run_id" "BL074-NE-006" "$SCENE_BRIDGE_FILE" 'nativeBridgeDiagnosticsSchema' \
    "scene-state payload publishes native diagnostics schema" \
    "scene-state payload missing native diagnostics schema"

  run_pattern_check "$NATIVE_ERROR_SURFACE_TSV" "$run_id" "BL074-NE-007" "$SCENE_BRIDGE_FILE" 'nativeBridgeDiagnostics(?!Schema)' \
    "scene-state payload publishes native diagnostics object" \
    "scene-state payload missing native diagnostics object"

done

strict_fail_count="$(awk -F'\t' 'NR > 1 && $3 == "FAIL" { count++ } END { print count + 0 }' "$STRICT_GESTURE_TSV")"
degraded_fail_count="$(awk -F'\t' 'NR > 1 && $3 == "FAIL" { count++ } END { print count + 0 }' "$DEGRADED_MODE_TSV")"
native_fail_count="$(awk -F'\t' 'NR > 1 && $3 == "FAIL" { count++ } END { print count + 0 }' "$NATIVE_ERROR_SURFACE_TSV")"

cat > "$OPERATOR_DIAGNOSTICS_MD" <<EOF_MD
# BL-074 Operator Diagnostics Snapshot

- generated_at_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)
- mode: ${MODE}
- runs: ${RUNS}
- schema: ${DIAGNOSTICS_SCHEMA}

## Contract Summary

| Surface | Status | Notes |
|---|---|---|
| strict_gesture_matrix.tsv | $([[ "$strict_fail_count" -eq 0 ]] && echo PASS || echo FAIL) | strict gesture fallback gate + operator detail checks |
| degraded_mode_contract.tsv | $([[ "$degraded_fail_count" -eq 0 ]] && echo PASS || echo FAIL) | degraded startup + control lock contract checks |
| native_error_surface.tsv | $([[ "$native_fail_count" -eq 0 ]] && echo PASS || echo FAIL) | call-failure counters + operator/native channel schema checks |

## Diagnostics Channel Schema

- 'window.__LQ_NATIVE_BRIDGE_DIAGNOSTICS__': per-call counters, classification, recent errors, payload state.
- 'window.__LQ_OPERATOR_DIAGNOSTICS__': operator-facing snapshot (runtime state + scene transport + native diagnostics).
- Scene-state payload keys: 'nativeBridgeDiagnosticsSchema', 'nativeBridgeAvailable', 'nativeBridgeBackend', 'nativeBridgeDiagnostics'.

## Degraded Mode Controls Disabled

- calibration controls: 'cal-start-btn', 'cal-redetect-btn', 'cal-profile-*'
- preset/timeline controls: 'preset-*-btn', 'timeline-*-btn', 'motion-transport-*-btn'
- runtime-native commands: 'rend-headtrack-set-forward', 'choreo-apply-btn', 'choreo-save-btn'
EOF_MD

if [[ "$fail_count" -eq 0 ]]; then
  record_status "lane_result" "PASS" "mode=${MODE};runs=${RUNS};strict=${strict_fail_count};degraded=${degraded_fail_count};native=${native_fail_count}" "$STATUS_TSV"
else
  record_status "lane_result" "FAIL" "mode=${MODE};runs=${RUNS};failures=${fail_count}" "$STATUS_TSV"
fi

if [[ "$MODE" == "execute" && "$fail_count" -gt 0 ]]; then
  exit 1
fi

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi

exit 0
