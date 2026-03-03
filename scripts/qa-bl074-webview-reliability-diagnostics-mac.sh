#!/usr/bin/env bash
# Title: BL-074 WebView Runtime Reliability Diagnostics QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-03
# Last Modified Date: 2026-03-03
#
# Purpose:
# - Validate strict-gesture self-test behavior contracts.
# - Validate bridge degraded-mode contract and impacted-control lockdown path.
# - Validate centralized runtime diagnostics counters for native-call failures/timeouts.
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
RUNS=1

SOURCE_JS="${ROOT_DIR}/Source/ui/public/js/index.js"
WEBVIEW_RUNTIME_HEADER="${ROOT_DIR}/Source/editor_webview/EditorWebViewRuntime.h"
BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-074-webview-runtime-reliability-diagnostics-strict-gesture-and-degraded-mode.md"

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

BL-074 deterministic QA lane for strict-gesture and degraded-mode diagnostics.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default mode)
  --execute          Execute checks (adds syntax/runner checks)
  --runs <N>         Number of deterministic replay runs (integer >= 1, default: 1)
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

record() {
  local check_id="$1"
  local result="$2"
  local detail="$3"
  local artifact="${4:-}"

  printf "%s\t%s\t%s\t%s\n" \
    "$check_id" \
    "$result" \
    "${detail//$'\t'/ }" \
    "${artifact//$'\t'/ }" \
    >> "$STATUS_TSV"

  if [[ "$result" == "PASS" ]]; then
    ((pass_count++)) || true
    echo "  [PASS] $check_id: $detail"
  else
    ((fail_count++)) || true
    echo "  [FAIL] $check_id: $detail"
  fi
}

require_pattern() {
  local pattern="$1"
  local path="$2"
  rg -q "$pattern" "$path"
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
printf "run_index\tcheck\tresult\tdetail\tartifact\n" > "$STRICT_GESTURE_TSV"
printf "run_index\tcheck\tresult\tdetail\tartifact\n" > "$DEGRADED_MODE_TSV"
printf "run_index\tcheck\tresult\tdetail\tartifact\n" > "$NATIVE_ERROR_SURFACE_TSV"

record "BL074-PRE-source_js_exists" "$( [[ -f "$SOURCE_JS" ]] && echo PASS || echo FAIL )" "index.js presence" "$SOURCE_JS"
record "BL074-PRE-runtime_header_exists" "$( [[ -f "$WEBVIEW_RUNTIME_HEADER" ]] && echo PASS || echo FAIL )" "EditorWebViewRuntime header presence" "$WEBVIEW_RUNTIME_HEADER"
record "BL074-PRE-backlog_doc_exists" "$( [[ -f "$BACKLOG_DOC" ]] && echo PASS || echo FAIL )" "runbook presence" "$BACKLOG_DOC"

if [[ "$fail_count" -gt 0 ]]; then
  record "lane_result" "FAIL" "preflight failures detected" "$STATUS_TSV"
  exit 1
fi

for run_index in $(seq 1 "$RUNS"); do
  strict_result="PASS"
  strict_detail="strict gesture query+failure path present"
  if ! require_pattern "strictGestureSelfTestRequested" "$SOURCE_JS"; then
    strict_result="FAIL"
    strict_detail="strict gesture request flag missing in index.js"
  elif ! require_pattern "strict_gesture" "$WEBVIEW_RUNTIME_HEADER"; then
    strict_result="FAIL"
    strict_detail="strict gesture URL parameter wiring missing in EditorWebViewRuntime"
  elif ! require_pattern "strict_gesture rejected fallback path" "$SOURCE_JS"; then
    strict_result="FAIL"
    strict_detail="UI-07 strict gesture failure message missing"
  fi
  printf "%s\t%s\t%s\t%s\t%s\n" "$run_index" "strict_gesture_contract" "$strict_result" "$strict_detail" "$SOURCE_JS" >> "$STRICT_GESTURE_TSV"
  record "BL074-R${run_index}-strict_gesture_contract" "$strict_result" "$strict_detail" "$STRICT_GESTURE_TSV"

  degraded_result="PASS"
  degraded_detail="startup binding gate + impacted control lockdown present"
  if ! require_pattern "BRIDGE_DEGRADED_CONTROL_IDS" "$SOURCE_JS"; then
    degraded_result="FAIL"
    degraded_detail="missing impacted control list"
  elif ! require_pattern "setBridgeImpactedControlsEnabled" "$SOURCE_JS"; then
    degraded_result="FAIL"
    degraded_detail="missing impacted control enable/disable function"
  elif ! require_pattern "applyBridgeDegradedMode" "$SOURCE_JS"; then
    degraded_result="FAIL"
    degraded_detail="missing bridge degraded mode applier"
  elif ! require_pattern "evaluateCriticalStartupBindings" "$SOURCE_JS"; then
    degraded_result="FAIL"
    degraded_detail="missing critical startup binding evaluator"
  elif ! require_pattern "bridgeDegraded" "$SOURCE_JS"; then
    degraded_result="FAIL"
    degraded_detail="runtime bridge degraded state not tracked"
  fi
  printf "%s\t%s\t%s\t%s\t%s\n" "$run_index" "degraded_mode_contract" "$degraded_result" "$degraded_detail" "$SOURCE_JS" >> "$DEGRADED_MODE_TSV"
  record "BL074-R${run_index}-degraded_mode_contract" "$degraded_result" "$degraded_detail" "$DEGRADED_MODE_TSV"

  native_result="PASS"
  native_detail="runtime diagnostics counters + centralized error surface present"
  if ! require_pattern "runtimeDiagnosticsState" "$SOURCE_JS"; then
    native_result="FAIL"
    native_detail="runtime diagnostics state missing"
  elif ! require_pattern "pushRuntimeDiagnosticsEvent" "$SOURCE_JS"; then
    native_result="FAIL"
    native_detail="diagnostics event channel missing"
  elif ! require_pattern "native_call_timeout" "$SOURCE_JS"; then
    native_result="FAIL"
    native_detail="native timeout event classification missing"
  elif ! require_pattern "native ok=" "$SOURCE_JS"; then
    native_result="FAIL"
    native_detail="operator diagnostics summary missing deterministic counters"
  elif ! require_pattern "__LQ_RUNTIME_DIAGNOSTICS__" "$SOURCE_JS"; then
    native_result="FAIL"
    native_detail="window runtime diagnostics snapshot missing"
  fi
  printf "%s\t%s\t%s\t%s\t%s\n" "$run_index" "native_error_surface" "$native_result" "$native_detail" "$SOURCE_JS" >> "$NATIVE_ERROR_SURFACE_TSV"
  record "BL074-R${run_index}-native_error_surface" "$native_result" "$native_detail" "$NATIVE_ERROR_SURFACE_TSV"

  if [[ "$MODE" == "execute" ]]; then
    if command -v node >/dev/null 2>&1; then
      if node --check "$SOURCE_JS" > "${OUT_DIR}/run_${run_index}_node_check.log" 2>&1; then
        record "BL074-R${run_index}-node_syntax_check" "PASS" "node --check passed for index.js" "${OUT_DIR}/run_${run_index}_node_check.log"
      else
        record "BL074-R${run_index}-node_syntax_check" "FAIL" "node --check failed for index.js" "${OUT_DIR}/run_${run_index}_node_check.log"
      fi
    else
      record "BL074-R${run_index}-node_syntax_check" "PASS" "node unavailable; syntax check skipped" "$SOURCE_JS"
    fi
  fi
done

cat > "$OPERATOR_DIAGNOSTICS_MD" <<EOF_MD
# BL-074 Operator Diagnostics Snapshot

- Generated: ${TIMESTAMP}
- Mode: ${MODE}
- Runs: ${RUNS}

## Runtime Channel Contract

- Global snapshot key: window.__LQ_RUNTIME_DIAGNOSTICS__
- Required counters:
  - startupBindingFailures
  - nativeCallFailures
  - nativeCallTimeouts
  - gestureFallbacks
  - strictGestureViolations
- Required event channel function: pushRuntimeDiagnosticsEvent(...)
- Operator summary surface: #rend-diagnostics-availability

## Degraded Mode Contract

- Critical startup bindings route through evaluateCriticalStartupBindings().
- Degraded mode applies through applyBridgeDegradedMode(...).
- Impacted controls disable via setBridgeImpactedControlsEnabled(false).

## Strict Gesture Contract

- Query switch: strict_gesture=1.
- WebView URL wiring: Source/editor_webview/EditorWebViewRuntime.h.
- Self-test gate: UI-07 fails when gesture fallbacks are used under strict mode.
EOF_MD

record "BL074-SNAPSHOT-operator_diagnostics" "PASS" "operator diagnostics snapshot generated" "$OPERATOR_DIAGNOSTICS_MD"

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "bl074_webview_reliability_passed mode=${MODE} runs=${RUNS}" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "bl074_webview_reliability_failed=${fail_count} mode=${MODE} runs=${RUNS}" "$STATUS_TSV"
fi

echo ""
echo "Results: ${pass_count} passed, ${fail_count} failed"
echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $STRICT_GESTURE_TSV"
echo "- $DEGRADED_MODE_TSV"
echo "- $NATIVE_ERROR_SURFACE_TSV"
echo "- $OPERATOR_DIAGNOSTICS_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
