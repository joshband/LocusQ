#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract-only"
RUNS=1
APP_PATH="${ROOT_DIR}/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl097_editor_bridge_cadence_${TIMESTAMP}"

usage() {
  cat <<EOF
Usage: qa-bl097-editor-bridge-cadence-mac.sh [options]

Options:
  --contract-only      Run static contract checks only (default)
  --execute            Run standalone execute smoke using the production selftest lane
  --runs <count>       Number of execute runs (default: 1)
  --app <path>         Standalone app path (default: ${APP_PATH})
  --out <dir>          Output directory (default: ${OUT_DIR})
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only)
      MODE="contract-only"
      shift
      ;;
    --execute)
      MODE="execute"
      shift
      ;;
    --runs)
      RUNS="${2:-}"
      shift 2
      ;;
    --app)
      APP_PATH="${2:-}"
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
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check\tresult\tdetail\n" > "$STATUS_TSV"

record() {
  local check="$1"
  local result="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$check" "$result" "$detail" >> "$STATUS_TSV"
}

if [[ "$MODE" == "contract-only" ]]; then
  if rg -q 'kStructuralScenePublishIntervalTicks = 3|kCalibrationStatusPublishIntervalTicks = 6|kCalibrationProfilePollIntervalTicks = 15' "$ROOT_DIR/Source/PluginEditor.h"; then
    record "cadence_constants" "PASS" "editor cadence tiers declared in Source/PluginEditor.h"
  else
    record "cadence_constants" "FAIL" "missing editor cadence tier constants in Source/PluginEditor.h"
  fi

  if rg -q 'pushBridgePayloadsIfDue' "$ROOT_DIR/Source/PluginEditor.cpp" \
    && rg -q 'bridgeTickCount % kCalibrationProfilePollIntervalTicks' "$ROOT_DIR/Source/PluginEditor.cpp"; then
    record "tiered_timer" "PASS" "timerCallback uses tiered publish path and debounced calibration polling"
  else
    record "tiered_timer" "FAIL" "timerCallback still appears to perform untiered bridge work"
  fi

  if rg -q 'pushSceneUpdate' "$ROOT_DIR/Source/editor_shell/EditorShellHelpers.h" \
    && rg -q 'pushCalibrationUpdate' "$ROOT_DIR/Source/editor_shell/EditorShellHelpers.h"; then
    record "split_push_helpers" "PASS" "scene and calibration bridge updates can publish independently"
  else
    record "split_push_helpers" "FAIL" "missing split push helpers for scene/calibration updates"
  fi

  if rg -q 'applyPendingCompanionCalibrationProfileReload' "$ROOT_DIR/Source/PluginProcessor.h" \
    && rg -q 'pendingCompanionCalibrationRuntimeReload = true' "$ROOT_DIR/Source/processor_core/ProcessorCalibrationBridge.cpp" \
    && rg -q 'applyPendingCompanionCalibrationProfileReload' "$ROOT_DIR/Source/PluginEditor.cpp"; then
    if rg -q 'void LocusQAudioProcessor::applyPendingCompanionCalibrationProfileReload' "$ROOT_DIR/Source/processor_core/ProcessorCalibrationBridge.cpp" \
      && rg -q 'spatialRenderer.reloadSteamAudioRuntime' "$ROOT_DIR/Source/processor_core/ProcessorCalibrationBridge.cpp"; then
      record "staged_reload_apply" "PASS" "profile polling marks pending reload and editor timer applies it separately"
    else
      record "staged_reload_apply" "FAIL" "missing dedicated staged reload apply method"
    fi
  else
    record "staged_reload_apply" "FAIL" "missing pending reload staging contract"
  fi

  if rg -q 'shouldLogResourceRequests' "$ROOT_DIR/Source/editor_webview/EditorWebViewRuntime.h" \
    && rg -q 'LOCUSQ_WEBVIEW_RESOURCE_LOG' "$ROOT_DIR/Source/editor_webview/EditorWebViewRuntime.h"; then
    record "resource_log_gating" "PASS" "resource-request logging is debug or env gated"
  else
    record "resource_log_gating" "FAIL" "resource-request logging still appears unbounded"
  fi
else
  if [[ ! -e "$APP_PATH" ]]; then
    record "execute_preflight" "FAIL" "standalone app missing at $APP_PATH"
  else
    record "execute_preflight" "PASS" "standalone app found at $APP_PATH"
  fi

  fail_count=0
  for run in $(seq 1 "$RUNS"); do
    log_path="${OUT_DIR}/run_${run}.log"
    if LOCUSQ_UI_SELFTEST_SCOPE=bl101 "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" "$APP_PATH" >"$log_path" 2>&1; then
      artifact_path="$(awk -F= '/^artifact=|^result_json=/{print $2}' "$log_path" | tail -n 1)"
      record "execute_run_${run}" "PASS" "artifact=${artifact_path:-missing}"
    else
      record "execute_run_${run}" "FAIL" "log=${log_path}"
      fail_count=$((fail_count + 1))
    fi
  done

  if [[ "$fail_count" -eq 0 ]]; then
    record "lane_result" "PASS" "bl097 execute smoke passed runs=${RUNS}"
  else
    record "lane_result" "FAIL" "bl097 execute smoke failed runs=${RUNS} fail_count=${fail_count}"
  fi
fi

{
  echo "# BL-097 Editor Bridge Cadence QA"
  echo
  echo "- mode: \`${MODE}\`"
  echo "- runs: \`${RUNS}\`"
  echo "- status_tsv: \`${STATUS_TSV}\`"
  if [[ "$MODE" == "execute" ]]; then
    echo "- app: \`${APP_PATH}\`"
  fi
} > "$SUMMARY_MD"

cat "$STATUS_TSV"
