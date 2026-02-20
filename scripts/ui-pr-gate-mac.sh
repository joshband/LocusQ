#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="$ROOT_DIR/TestEvidence/ui_pr_gate_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
echo -e "step\tresult\texit_code\tlog" > "$STATUS_TSV"

APP_PATH="${1:-}"
if [[ -n "$APP_PATH" && ! -d "$APP_PATH" ]]; then
  echo "ERROR: app path does not exist: $APP_PATH" >&2
  exit 2
fi

run_step() {
  local step="$1"
  local log="$2"
  shift 2

  set +o pipefail
  "$@" 2>&1 | tee "$log"
  local exit_code=${PIPESTATUS[0]}
  set -o pipefail
  set -e

  local result="FAIL"
  if [[ "$exit_code" -eq 0 ]]; then
    result="PASS"
  fi
  echo -e "${step}\t${result}\t${exit_code}\t${log}" | tee -a "$STATUS_TSV"
  return "$exit_code"
}

echo "=== LocusQ UI PR Gate (macOS) ==="
echo "out_dir: $OUT_DIR"
echo "selftest_enabled: ${UI_PR_GATE_WITH_SELFTEST:-1}"
echo "smoke_enabled: ${UI_PR_GATE_WITH_SMOKE:-0}"
echo "primary_gate: standalone-ui-selftest-stage11-mac.sh"
echo "appium_enabled: ${UI_PR_GATE_WITH_APPIUM:-0}"
echo

if [[ "${UI_PR_GATE_WITH_SELFTEST:-1}" == "1" ]]; then
  SELFTEST_CMD=("$ROOT_DIR/scripts/standalone-ui-selftest-stage11-mac.sh")
  if [[ -n "$APP_PATH" ]]; then
    SELFTEST_CMD+=("$APP_PATH")
  fi

  if ! run_step "ui_stage11_selftest" "$OUT_DIR/ui_stage11_selftest.log" "${SELFTEST_CMD[@]}"; then
    echo
    echo "UI PR GATE RESULT: FAIL (stage11 self-test gate failed)"
    echo "status: $STATUS_TSV"
    exit 1
  fi
else
  echo -e "ui_stage11_selftest\tSKIP\t0\t-" | tee -a "$STATUS_TSV"
fi

if [[ "${UI_PR_GATE_WITH_SMOKE:-0}" == "1" ]]; then
  SMOKE_CMD=("$ROOT_DIR/scripts/standalone-ui-smoke-mac.sh")
  if [[ -n "$APP_PATH" ]]; then
    SMOKE_CMD+=("$APP_PATH")
  fi

  if ! run_step "ui_smoke_fast_gate" "$OUT_DIR/ui_smoke_fast_gate.log" "${SMOKE_CMD[@]}"; then
    echo
    echo "UI PR GATE RESULT: FAIL (smoke gate failed)"
    echo "status: $STATUS_TSV"
    exit 1
  fi
else
  echo -e "ui_smoke_fast_gate\tSKIP\t0\t-" | tee -a "$STATUS_TSV"
fi

if [[ "${UI_PR_GATE_WITH_APPIUM:-0}" == "1" ]]; then
  export APPIUM_UI_STEP_TIMEOUT_SECONDS="${APPIUM_UI_STEP_TIMEOUT_SECONDS:-8}"
  export APPIUM_UI_MAX_RUN_SECONDS="${APPIUM_UI_MAX_RUN_SECONDS:-120}"

  APPIUM_CMD=("$ROOT_DIR/scripts/appium-mac2-ui-regression.sh")
  if [[ -n "$APP_PATH" ]]; then
    APPIUM_CMD+=("$APP_PATH")
  fi

  if ! run_step "ui_regression_appium" "$OUT_DIR/ui_regression_appium.log" "${APPIUM_CMD[@]}"; then
    echo
    echo "UI PR GATE RESULT: FAIL (Appium lane enabled and failed)"
    echo "status: $STATUS_TSV"
    exit 1
  fi
else
  echo -e "ui_regression_appium\tSKIP\t0\t-" | tee -a "$STATUS_TSV"
fi

echo
echo "UI PR GATE RESULT: PASS"
echo "status: $STATUS_TSV"
