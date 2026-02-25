#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

APP_INPUT=""
QA_BIN_INPUT=""
RUN_CALIBRATION=1
RUN_BINAURAL=1
RUN_MULTICHANNEL=1
BL018_STRICT=0

usage() {
  cat <<'EOF'
Usage: scripts/qa-standalone-calibration-binaural-multichannel-mac.sh [options]

Run a standalone-first validation bundle for:
  - Calibration contracts (BL-026 self-test scope)
  - Binaural headphone contracts (BL-009)
  - Multichannel profile matrix (BL-018)

Options:
  --app <path>              Standalone app or executable path
  --qa-bin <path>           QA runner binary path
  --skip-calibration        Skip BL-026 standalone calibration lane
  --skip-binaural           Skip BL-009 binaural lanes
  --skip-multichannel       Skip BL-018 multichannel lane
  --strict-multichannel     Enable strict integration probe for BL-018
  -h, --help                Show this help

Defaults:
  app:    build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app
  qa-bin: build_local/locusq_qa_artefacts/Release/locusq_qa
EOF
}

while (($# > 0)); do
  case "$1" in
    --app)
      APP_INPUT="${2:-}"
      shift 2
      ;;
    --qa-bin)
      QA_BIN_INPUT="${2:-}"
      shift 2
      ;;
    --skip-calibration)
      RUN_CALIBRATION=0
      shift
      ;;
    --skip-binaural)
      RUN_BINAURAL=0
      shift
      ;;
    --skip-multichannel)
      RUN_MULTICHANNEL=0
      shift
      ;;
    --strict-multichannel)
      BL018_STRICT=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

resolve_app_exec() {
  local app_input="$1"
  local app_exec
  if [[ -z "$app_input" ]]; then
    app_exec="$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app/Contents/MacOS/LocusQ"
  elif [[ -d "$app_input" ]]; then
    app_exec="$app_input/Contents/MacOS/LocusQ"
  else
    app_exec="$app_input"
  fi
  printf '%s\n' "$app_exec"
}

resolve_qa_bin() {
  local qa_bin_input="$1"
  if [[ -n "$qa_bin_input" ]]; then
    printf '%s\n' "$qa_bin_input"
    return
  fi

  local default_release="$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa"
  local default_plain="$ROOT_DIR/build_local/locusq_qa_artefacts/locusq_qa"
  if [[ -x "$default_release" ]]; then
    printf '%s\n' "$default_release"
  else
    printf '%s\n' "$default_plain"
  fi
}

APP_EXEC="$(resolve_app_exec "$APP_INPUT")"
QA_BIN="$(resolve_qa_bin "$QA_BIN_INPUT")"

OUT_DIR="$ROOT_DIR/TestEvidence/standalone_calibration_binaural_multichannel_${TIMESTAMP}"
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

log_status "init" "pass" "ts=${TIMESTAMP}"
log_status "config" "pass" "run_calibration=${RUN_CALIBRATION}; run_binaural=${RUN_BINAURAL}; run_multichannel=${RUN_MULTICHANNEL}; strict_multichannel=${BL018_STRICT}"
log_status "paths" "pass" "app=${APP_EXEC}; qa_bin=${QA_BIN}"

if [[ "$RUN_CALIBRATION" == "1" || "$RUN_BINAURAL" == "1" ]]; then
  if [[ ! -x "$APP_EXEC" ]]; then
    log_status "app_exec" "fail" "missing=${APP_EXEC}"
  else
    log_status "app_exec" "pass" "$APP_EXEC"
  fi
fi

if [[ "$RUN_BINAURAL" == "1" || "$RUN_MULTICHANNEL" == "1" ]]; then
  if [[ ! -x "$QA_BIN" ]]; then
    log_status "qa_bin" "fail" "missing=${QA_BIN}"
  else
    log_status "qa_bin" "pass" "$QA_BIN"
  fi
fi

FAIL_COUNT="$(awk -F'\t' 'NR>1 && $2=="fail" { c++ } END { print c+0 }' "$STATUS_TSV")"
if [[ "$FAIL_COUNT" != "0" ]]; then
  echo "FAIL: missing required runtime dependencies" >&2
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

CALIBRATION_ARTIFACT=""
BL009_CONTRACT_ARTIFACT=""
BL009_PROFILE_ARTIFACT=""
BL018_ARTIFACT=""

if [[ "$RUN_CALIBRATION" == "1" ]]; then
  calibration_log="$OUT_DIR/bl026_calibration_selftest.log"
  if LOCUSQ_UI_SELFTEST_SCOPE=bl026 "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" "$APP_EXEC" >"$calibration_log" 2>&1; then
    CALIBRATION_ARTIFACT="$(awk -F= '/^artifact=/{print $2}' "$calibration_log" | tail -n 1)"
    if [[ -n "$CALIBRATION_ARTIFACT" && -f "$CALIBRATION_ARTIFACT" ]]; then
      cp "$CALIBRATION_ARTIFACT" "$OUT_DIR/bl026_calibration_selftest.json"
      if command -v jq >/dev/null 2>&1; then
        bl026_total="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id | test("^UI-P1-026[A-E]$"))) | length)' "$OUT_DIR/bl026_calibration_selftest.json")"
        bl026_fail="$(jq -r '((.payload.checks // .result.checks // []) | map(select((.id | test("^UI-P1-026[A-E]$")) and (.pass != true))) | length)' "$OUT_DIR/bl026_calibration_selftest.json")"
        if [[ "$bl026_total" -lt 5 || "$bl026_fail" != "0" ]]; then
          log_status "bl026_calibration_standalone" "fail" "unexpected_check_state total=${bl026_total} fail=${bl026_fail}; log=${calibration_log}"
        else
          log_status "bl026_calibration_standalone" "pass" "checks=UI-P1-026A..E; artifact=$CALIBRATION_ARTIFACT"
        fi
      else
        log_status "bl026_calibration_standalone" "warn" "jq_missing; run_passed_without_check_introspection; artifact=$CALIBRATION_ARTIFACT"
      fi
    else
      log_status "bl026_calibration_standalone" "fail" "artifact_missing; log=${calibration_log}"
    fi
  else
    log_status "bl026_calibration_standalone" "fail" "script_failed; log=${calibration_log}"
  fi
else
  log_status "bl026_calibration_standalone" "warn" "skipped_by_flag"
fi

if [[ "$RUN_BINAURAL" == "1" ]]; then
  bl009_contract_log="$OUT_DIR/bl009_headphone_contract.log"
  if "$ROOT_DIR/scripts/qa-bl009-headphone-contract-mac.sh" "$QA_BIN" "$APP_EXEC" >"$bl009_contract_log" 2>&1; then
    BL009_CONTRACT_ARTIFACT="$(awk -F= '/^artifact_dir=/{print $2}' "$bl009_contract_log" | tail -n 1)"
    log_status "bl009_headphone_contract" "pass" "${BL009_CONTRACT_ARTIFACT:-log_only}; log=${bl009_contract_log}"
  else
    log_status "bl009_headphone_contract" "fail" "script_failed; log=${bl009_contract_log}"
  fi

  bl009_profile_log="$OUT_DIR/bl009_headphone_profile_contract.log"
  if "$ROOT_DIR/scripts/qa-bl009-headphone-profile-contract-mac.sh" "$QA_BIN" "$APP_EXEC" >"$bl009_profile_log" 2>&1; then
    BL009_PROFILE_ARTIFACT="$(awk -F= '/^artifact_dir=/{print $2}' "$bl009_profile_log" | tail -n 1)"
    log_status "bl009_headphone_profile_contract" "pass" "${BL009_PROFILE_ARTIFACT:-log_only}; log=${bl009_profile_log}"
  else
    log_status "bl009_headphone_profile_contract" "fail" "script_failed; log=${bl009_profile_log}"
  fi
else
  log_status "bl009_headphone_contract" "warn" "skipped_by_flag"
  log_status "bl009_headphone_profile_contract" "warn" "skipped_by_flag"
fi

if [[ "$RUN_MULTICHANNEL" == "1" ]]; then
  bl018_log="$OUT_DIR/bl018_multichannel_contract.log"
  bl018_args=(--no-binaural-runtime)
  if [[ "$BL018_STRICT" == "1" ]]; then
    bl018_args+=(--strict-integration)
  fi

  if QA_BIN="$QA_BIN" "$ROOT_DIR/scripts/qa-bl018-ambisonic-contract-mac.sh" "${bl018_args[@]}" >"$bl018_log" 2>&1; then
    BL018_ARTIFACT="$(awk -F= '/^artifact_dir=/{print $2}' "$bl018_log" | tail -n 1)"
    if rg -q '^PASS_WITH_WARNINGS:' "$bl018_log"; then
      log_status "bl018_multichannel_contract" "warn" "${BL018_ARTIFACT:-log_only}; log=${bl018_log}"
    else
      log_status "bl018_multichannel_contract" "pass" "${BL018_ARTIFACT:-log_only}; log=${bl018_log}"
    fi
  else
    log_status "bl018_multichannel_contract" "fail" "script_failed; log=${bl018_log}"
  fi
else
  log_status "bl018_multichannel_contract" "warn" "skipped_by_flag"
fi

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
Title: Standalone Calibration, Binaural, and Multichannel Validation Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# Standalone Validation Bundle (${TIMESTAMP})

- overall: \`${OVERALL}\`
- run_calibration: \`${RUN_CALIBRATION}\`
- run_binaural: \`${RUN_BINAURAL}\`
- run_multichannel: \`${RUN_MULTICHANNEL}\`
- strict_multichannel: \`${BL018_STRICT}\`
- app_exec: \`${APP_EXEC}\`
- qa_bin: \`${QA_BIN}\`
- pass_count: \`${PASS_COUNT}\`
- warn_count: \`${WARN_COUNT}\`
- fail_count: \`${FAIL_COUNT}\`

## Child Artifacts

- bl026_calibration_artifact: \`${CALIBRATION_ARTIFACT:-n/a}\`
- bl009_headphone_contract_artifact: \`${BL009_CONTRACT_ARTIFACT:-n/a}\`
- bl009_headphone_profile_artifact: \`${BL009_PROFILE_ARTIFACT:-n/a}\`
- bl018_multichannel_artifact: \`${BL018_ARTIFACT:-n/a}\`

## Bundle Files

- \`status.tsv\`
- \`bl026_calibration_selftest.log\`
- \`bl009_headphone_contract.log\`
- \`bl009_headphone_profile_contract.log\`
- \`bl018_multichannel_contract.log\`
EOF

if [[ "$OVERALL" == "fail" ]]; then
  echo "FAIL: standalone calibration+binaural+multichannel bundle failed" >&2
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

if [[ "$OVERALL" == "pass_with_warnings" ]]; then
  echo "PASS_WITH_WARNINGS: standalone calibration+binaural+multichannel bundle completed"
else
  echo "PASS: standalone calibration+binaural+multichannel bundle completed"
fi
echo "artifact_dir=$OUT_DIR"
