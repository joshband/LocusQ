#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

RUN_COUNT="${LQ_BL024_HEADLESS_RUNS:-3}"
SKIP_INSTALL="${LQ_BL024_SKIP_INSTALL:-0}"
REQUIRE_LOCUSQ="${LQ_REAPER_REQUIRE_LOCUSQ:-1}"
REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"

if [[ "$RUN_COUNT" -lt 1 ]]; then
  echo "ERROR: LQ_BL024_HEADLESS_RUNS must be >= 1"
  exit 2
fi

OUT_DIR="$ROOT_DIR/TestEvidence/bl024_reaper_automation_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
REPORT_MD="$OUT_DIR/report.md"

{
  echo -e "step\tstatus\tdetail"
  echo -e "init\tpass\tts=${TIMESTAMP}"
  echo -e "run_count\tpass\t${RUN_COUNT}"
  echo -e "require_locusq\tpass\t${REQUIRE_LOCUSQ}"
  echo -e "reaper_bin\tpass\t${REAPER_BIN}"
} > "$STATUS_TSV"

log_status() {
  local step="$1"
  local status="$2"
  local detail="$3"
  echo -e "${step}\t${status}\t${detail}" | tee -a "$STATUS_TSV" >/dev/null
}

if [[ ! -x "$REAPER_BIN" ]]; then
  log_status "preflight_reaper_bin" "fail" "missing_or_not_executable=${REAPER_BIN}"
  echo "FAIL: REAPER executable missing: $REAPER_BIN"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
  log_status "preflight_jq" "fail" "jq_not_found"
  echo "FAIL: jq is required for BL-024 lane parsing"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi
log_status "preflight_jq" "pass" "jq_available"

if [[ "$SKIP_INSTALL" != "1" ]]; then
  INSTALL_LOG="$OUT_DIR/build_install.log"
  if LOCUSQ_REAPER_AUTO_QUIT=1 \
     LOCUSQ_REAPER_FORCE_KILL=1 \
     LOCUSQ_REFRESH_REAPER_CACHE=1 \
     LOCUSQ_REFRESH_AU_CACHE=1 \
     "$ROOT_DIR/scripts/build-and-install-mac.sh" >"$INSTALL_LOG" 2>&1; then
    log_status "build_install" "pass" "log=$INSTALL_LOG"
  else
    log_status "build_install" "fail" "log=$INSTALL_LOG"
    echo "FAIL: build/install step failed"
    echo "artifact_dir=$OUT_DIR"
    exit 1
  fi

  if pgrep -ix reaper >/dev/null 2>&1; then
    log_status "post_install_reaper_running" "fail" "reaper_still_running"
    echo "FAIL: REAPER is still running after build/install step"
    echo "artifact_dir=$OUT_DIR"
    exit 1
  fi
  log_status "post_install_reaper_running" "pass" "reaper_not_running"
else
  log_status "build_install" "warn" "skipped_by_env=LQ_BL024_SKIP_INSTALL"
fi

for ((i=1; i<=RUN_COUNT; i+=1)); do
  RUN_LOG="$OUT_DIR/headless_run_${i}.log"
  set +e
  LQ_REAPER_REQUIRE_LOCUSQ="$REQUIRE_LOCUSQ" \
  "$ROOT_DIR/scripts/reaper-headless-render-smoke-mac.sh" --auto-bootstrap >"$RUN_LOG" 2>&1
  RUN_EXIT=$?
  set -e

  ARTIFACT_JSON="$(awk -F= '/^artifact=/{print $2}' "$RUN_LOG" | tail -n 1)"
  if [[ -z "$ARTIFACT_JSON" || ! -f "$ARTIFACT_JSON" ]]; then
    log_status "headless_run_${i}" "fail" "artifact_missing; log=$RUN_LOG"
    echo "FAIL: headless run ${i} did not return a valid artifact JSON"
    echo "artifact_dir=$OUT_DIR"
    exit 1
  fi

  cp "$ARTIFACT_JSON" "$OUT_DIR/headless_run_${i}_status.json"

  STATUS="$(jq -r '.status' "$ARTIFACT_JSON")"
  LOCUSQ_FOUND="$(jq -r '.locusqFxFound' "$ARTIFACT_JSON")"
  RENDER_OUTPUT_DETECTED="$(jq -r '.renderOutputDetected' "$ARTIFACT_JSON")"
  RENDER_EXIT="$(jq -r '.renderExitCode' "$ARTIFACT_JSON")"
  RENDER_ATTEMPTS="$(jq -r '.renderAttempts // 1' "$ARTIFACT_JSON")"

  if [[ "$RUN_EXIT" -ne 0 || "$STATUS" != "pass" || "$LOCUSQ_FOUND" != "true" || "$RENDER_OUTPUT_DETECTED" != "true" ]]; then
    log_status "headless_run_${i}" "fail" "exit=${RUN_EXIT}; status=${STATUS}; locusqFxFound=${LOCUSQ_FOUND}; renderOutputDetected=${RENDER_OUTPUT_DETECTED}; renderExitCode=${RENDER_EXIT}; log=${RUN_LOG}; artifact=${ARTIFACT_JSON}"
    echo "FAIL: BL-024 headless run ${i} failed"
    echo "artifact_dir=$OUT_DIR"
    exit 1
  fi

  log_status "headless_run_${i}" "pass" "renderAttempts=${RENDER_ATTEMPTS}; renderExitCode=${RENDER_EXIT}; artifact=${ARTIFACT_JSON}"
done

cat >"$REPORT_MD" <<EOF
Title: BL-024 REAPER Automation Lane Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-024 REAPER Automation Lane (${TIMESTAMP})

- run_count: \`${RUN_COUNT}\`
- require_locusq: \`${REQUIRE_LOCUSQ}\`
- skip_install: \`${SKIP_INSTALL}\`
- reaper_bin: \`${REAPER_BIN}\`

## Result

- Automated host lane: \`PASS\`
- Headless passes required: \`${RUN_COUNT}/${RUN_COUNT}\`

## Artifacts

- \`status.tsv\`
- \`headless_run_*_status.json\`
- \`headless_run_*.log\`
- \`build_install.log\` (unless install was skipped)

## Remaining BL-024 Gate

- Manual runbook evidence row is still required by:
  - \`Documentation/testing/reaper-manual-qa-session.md\`
  - \`Documentation/plans/reaper-host-automation-plan-2026-02-22.md\`
EOF

log_status "summary" "pass" "report=$REPORT_MD"
echo "PASS: BL-024 REAPER automation lane complete"
echo "artifact_dir=$OUT_DIR"
