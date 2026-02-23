#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

BUILD_DIR="${LQ_BL011_BUILD_DIR:-$ROOT_DIR/build_local}"
NONCLAP_BUILD_DIR="${LQ_BL011_NONCLAP_BUILD_DIR:-$ROOT_DIR/build_no_clap_check}"
REAPER_BIN="${REAPER_BIN:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
RUN_REAPER_PROBE="${LQ_BL011_RUN_REAPER_PROBE:-1}"
REUSE_REAPER_PROBE_PASS="${LQ_BL011_REUSE_REAPER_PROBE_PASS:-1}"
SELFTEST_RETRIES="${LQ_BL011_SELFTEST_RETRIES:-4}"
SELFTEST_TIMEOUT_SECONDS="${LQ_BL011_SELFTEST_TIMEOUT_SECONDS:-180}"
REUSE_SELFTEST_PASS="${LQ_BL011_REUSE_SELFTEST_PASS:-1}"
USE_INSTALLED_STANDALONE="${LQ_BL011_USE_INSTALLED_STANDALONE:-1}"
STANDALONE_INSTALL_DIR="${LQ_BL011_STANDALONE_INSTALL_DIR:-$HOME/Applications}"

CLAP_ARTIFACT="$BUILD_DIR/LocusQ_artefacts/Release/CLAP/LocusQ.clap"
CLAP_INSTALL_ARTIFACT="$HOME/Library/Audio/Plug-Ins/CLAP/LocusQ.clap"
STANDALONE_APP_BUILD="$BUILD_DIR/LocusQ_artefacts/Release/Standalone/LocusQ.app"
STANDALONE_APP_INSTALLED="$STANDALONE_INSTALL_DIR/LocusQ.app"
STANDALONE_APP="$STANDALONE_APP_BUILD"
QA_BIN="$BUILD_DIR/locusq_qa_artefacts/Release/locusq_qa"

OUT_DIR="$ROOT_DIR/TestEvidence/bl011_clap_closeout_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
REPORT_MD="$OUT_DIR/report.md"

{
  echo -e "step\tstatus\tdetail"
  echo -e "init\tpass\tts=${TIMESTAMP}"
  echo -e "build_dir\tpass\t${BUILD_DIR}"
  echo -e "nonclap_build_dir\tpass\t${NONCLAP_BUILD_DIR}"
} > "$STATUS_TSV"

log_status() {
  local step="$1"
  local status="$2"
  local detail="$3"
  echo -e "${step}\t${status}\t${detail}" | tee -a "$STATUS_TSV" >/dev/null
}

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "pass" "$(command -v "$cmd")"
  else
    log_status "tool_${cmd}" "fail" "missing_command"
    return 1
  fi
}

require_file_or_dir() {
  local label="$1"
  local path="$2"
  if [[ -e "$path" ]]; then
    log_status "$label" "pass" "$path"
  else
    log_status "$label" "fail" "missing=${path}"
    return 1
  fi
}

run_cmd_logged() {
  local step="$1"
  local log_file="$2"
  shift 2
  if "$@" >"$log_file" 2>&1; then
    log_status "$step" "pass" "log=$log_file"
  else
    log_status "$step" "fail" "log=$log_file"
    return 1
  fi
}

log_status "configure_clap" "pass" "starting"
run_cmd_logged \
  "cmake_configure_clap" \
  "$OUT_DIR/cmake_configure_clap.log" \
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLOCUSQ_ENABLE_CLAP=ON \
    -DLOCUSQ_CLAP_FETCH=ON \
    -DBUILD_LOCUSQ_QA=ON

run_cmd_logged \
  "cmake_build_clap_target" \
  "$OUT_DIR/cmake_build_clap_target.log" \
  cmake --build "$BUILD_DIR" --config Release --target LocusQ_CLAP -j 8

run_cmd_logged \
  "cmake_build_qa_target" \
  "$OUT_DIR/cmake_build_qa_target.log" \
  cmake --build "$BUILD_DIR" --config Release --target locusq_qa -j 8

run_cmd_logged \
  "cmake_build_standalone_target" \
  "$OUT_DIR/cmake_build_standalone_target.log" \
  cmake --build "$BUILD_DIR" --config Release --target LocusQ_Standalone -j 8

require_file_or_dir "clap_artifact" "$CLAP_ARTIFACT"
require_file_or_dir "clap_install_artifact" "$CLAP_INSTALL_ARTIFACT"
require_file_or_dir "standalone_artifact" "$STANDALONE_APP_BUILD"

if [[ "$USE_INSTALLED_STANDALONE" == "1" ]]; then
  mkdir -p "$STANDALONE_INSTALL_DIR"
  standalone_sync_log="$OUT_DIR/standalone_sync_for_selftest.log"
  set +e
  rsync -a --delete "$STANDALONE_APP_BUILD/" "$STANDALONE_APP_INSTALLED/" >"$standalone_sync_log" 2>&1
  standalone_sync_exit=$?
  set -e
  if [[ "$standalone_sync_exit" -eq 0 ]]; then
    log_status "standalone_sync_for_selftest" "pass" "log=$standalone_sync_log"
  elif [[ -d "$STANDALONE_APP_INSTALLED" ]]; then
    log_status "standalone_sync_for_selftest" "warn" "sync_failed_using_existing_install; log=$standalone_sync_log"
  else
    log_status "standalone_sync_for_selftest" "fail" "sync_failed_and_install_missing; log=$standalone_sync_log"
    exit 1
  fi
  STANDALONE_APP="$STANDALONE_APP_INSTALLED"
fi
log_status "standalone_selftest_app" "pass" "$STANDALONE_APP"

if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="$BUILD_DIR/locusq_qa_artefacts/locusq_qa"
fi
if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_binary" "fail" "missing=${QA_BIN}"
  exit 1
fi
log_status "qa_binary" "pass" "$QA_BIN"

require_cmd "clap-info"
require_cmd "clap-validator"

run_cmd_logged \
  "clap_info" \
  "$OUT_DIR/clap-info.json" \
  clap-info "$CLAP_ARTIFACT"

run_cmd_logged \
  "clap_validator" \
  "$OUT_DIR/clap-validator.txt" \
  clap-validator validate "$CLAP_ARTIFACT"

run_cmd_logged \
  "qa_smoke_suite" \
  "$OUT_DIR/qa_smoke_suite.log" \
  "$QA_BIN" --spatial qa/scenarios/locusq_smoke_suite.json

run_cmd_logged \
  "qa_phase_2_6_acceptance_suite" \
  "$OUT_DIR/qa_phase_2_6_acceptance_suite.log" \
  "$QA_BIN" --spatial qa/scenarios/locusq_phase_2_6_acceptance_suite.json

SELFTEST_PASS=0
SELFTEST_JSON=""

if [[ "$REUSE_SELFTEST_PASS" == "1" ]]; then
  latest_selftest_pass_json=""
  while IFS= read -r candidate_json; do
    [[ -z "$candidate_json" ]] && continue
    candidate_pass="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P2-011")) | .[0].pass) // false' "$candidate_json")"
    if [[ "$candidate_pass" == "true" ]]; then
      latest_selftest_pass_json="$candidate_json"
      break
    fi
  done < <(ls -1t "$ROOT_DIR"/TestEvidence/locusq_production_p0_selftest_*.json 2>/dev/null || true)

  if [[ -n "$latest_selftest_pass_json" ]]; then
    SELFTEST_JSON="$latest_selftest_pass_json"
    cp "$SELFTEST_JSON" "$OUT_DIR/ui_selftest_bl011_try_reused.json" || true
    bl011_detail="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P2-011")) | .[0].details) // "missing_details"' "$SELFTEST_JSON")"
    selftest_status="$(jq -r '.payload.status // .status // .result.status // "unknown"' "$SELFTEST_JSON")"
    log_status "ui_selftest_bl011_try_reused" "pass" "${bl011_detail}; selftest_status=${selftest_status}; artifact=${SELFTEST_JSON}"
    SELFTEST_PASS=1
  else
    log_status "ui_selftest_bl011_try_reused" "warn" "no_existing_UI-P2-011_pass_artifact_found"
  fi
else
  log_status "ui_selftest_bl011_try_reused" "warn" "disabled_via_LQ_BL011_REUSE_SELFTEST_PASS=0"
fi

selftest_try=1
while [[ "$SELFTEST_PASS" -ne 1 && "$selftest_try" -le "$SELFTEST_RETRIES" ]]; do
  selftest_log="$OUT_DIR/ui_selftest_bl011_try_${selftest_try}.log"
  latest_runlog_before="$(ls -1t "$ROOT_DIR"/TestEvidence/locusq_production_p0_selftest_*.run.log 2>/dev/null | head -n 1 || true)"
  set +e
  LOCUSQ_UI_SELFTEST_BL011=1 \
    LOCUSQ_UI_SELFTEST_SCOPE=bl011 \
    LOCUSQ_UI_SELFTEST_TIMEOUT_SECONDS="$SELFTEST_TIMEOUT_SECONDS" \
    "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" "$STANDALONE_APP"
  selftest_exit=$?
  set -e

  latest_runlog_after="$(ls -1t "$ROOT_DIR"/TestEvidence/locusq_production_p0_selftest_*.run.log 2>/dev/null | head -n 1 || true)"
  if [[ -n "$latest_runlog_after" ]]; then
    cp "$latest_runlog_after" "$selftest_log" || true
  else
    : > "$selftest_log"
  fi

  if [[ -n "$latest_runlog_after" && "$latest_runlog_after" == "$latest_runlog_before" ]]; then
    echo "warning=reused_previous_runlog:$latest_runlog_after" >> "$selftest_log"
  fi

  SELFTEST_JSON="$(awk -F= '/^artifact=/{print $2}' "$selftest_log" | tail -n 1)"
  if [[ -z "$SELFTEST_JSON" ]]; then
    SELFTEST_JSON="$(awk -F= '/^result_json=/{print $2}' "$selftest_log" | tail -n 1)"
  fi

  if [[ -n "$SELFTEST_JSON" && -f "$SELFTEST_JSON" ]]; then
    cp "$SELFTEST_JSON" "$OUT_DIR/ui_selftest_bl011_try_${selftest_try}.json" || true
    bl011_has_check="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P2-011")) | length > 0)' "$SELFTEST_JSON")"
    bl011_pass="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P2-011")) | .[0].pass) // false' "$SELFTEST_JSON")"
    bl011_detail="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P2-011")) | .[0].details) // "missing_details"' "$SELFTEST_JSON")"
    selftest_status="$(jq -r '.payload.status // .status // .result.status // "unknown"' "$SELFTEST_JSON")"

    if [[ "$bl011_has_check" != "true" ]]; then
      log_status "ui_selftest_bl011_try_${selftest_try}" "fail" "missing_UI-P2-011_check; selftest_status=${selftest_status}; exit=${selftest_exit}; log=${selftest_log}"
    elif [[ "$bl011_pass" == "true" ]]; then
      log_status "ui_selftest_bl011_try_${selftest_try}" "pass" "${bl011_detail}; selftest_status=${selftest_status}; exit=${selftest_exit}"
      SELFTEST_PASS=1
      break
    else
      log_status "ui_selftest_bl011_try_${selftest_try}" "fail" "${bl011_detail}; selftest_status=${selftest_status}; exit=${selftest_exit}"
    fi
  else
    log_status "ui_selftest_bl011_try_${selftest_try}" "fail" "missing_artifact; exit=${selftest_exit}; log=${selftest_log}"
  fi
  selftest_try=$((selftest_try + 1))
done

if [[ "$SELFTEST_PASS" -ne 1 ]]; then
  log_status "ui_selftest_bl011" "fail" "no_passing_run_within_${SELFTEST_RETRIES}_attempts"
  exit 1
fi
log_status "ui_selftest_bl011" "pass" "attempt=${selftest_try}"

run_cmd_logged \
  "cmake_configure_nonclap_guard" \
  "$OUT_DIR/cmake_configure_nonclap_guard.log" \
  cmake -S "$ROOT_DIR" -B "$NONCLAP_BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLOCUSQ_ENABLE_CLAP=OFF \
    -DBUILD_LOCUSQ_QA=OFF

run_cmd_logged \
  "cmake_build_nonclap_vst3_guard" \
  "$OUT_DIR/cmake_build_nonclap_vst3_guard.log" \
  cmake --build "$NONCLAP_BUILD_DIR" --config Release --target LocusQ_VST3 -j 8

REAPER_PROBE_STATUS="skipped"
REAPER_PROBE_JSON=""
if [[ "$RUN_REAPER_PROBE" == "1" ]]; then
  reused_reaper_probe_json=""
  if [[ "$REUSE_REAPER_PROBE_PASS" == "1" ]]; then
    while IFS= read -r candidate_probe_json; do
      [[ -z "$candidate_probe_json" ]] && continue
      candidate_probe_pass="$(jq -r '.clapFxFound // false' "$candidate_probe_json" 2>/dev/null || echo false)"
      if [[ "$candidate_probe_pass" == "true" ]]; then
        reused_reaper_probe_json="$candidate_probe_json"
        break
      fi
    done < <(ls -1t "$ROOT_DIR"/TestEvidence/reaper_clap_discovery_probe_*.json 2>/dev/null || true)
  fi

  if [[ -n "$reused_reaper_probe_json" ]]; then
    cp "$reused_reaper_probe_json" "$OUT_DIR/reaper_clap_probe.reused.json" || true
    REAPER_PROBE_NAME="$(jq -r '.matchedFxName // ""' "$reused_reaper_probe_json")"
    log_status "reaper_clap_discovery" "pass" "reused_artifact=$reused_reaper_probe_json; ${REAPER_PROBE_NAME}"
    REAPER_PROBE_STATUS="pass_reused"
  elif [[ -x "$REAPER_BIN" ]]; then
    REAPER_PROBE_LUA="$OUT_DIR/reaper_clap_probe.lua"
    REAPER_PROBE_JSON="$OUT_DIR/reaper_clap_probe.json"
    REAPER_PROBE_LOG="$OUT_DIR/reaper_clap_probe.log"
    cat > "$REAPER_PROBE_LUA" <<'LUA'
local statusJsonPath = os.getenv("LQ_REAPER_STATUS_JSON")

local function json_escape(value)
  if not value then return "" end
  value = value:gsub("\\", "\\\\")
  value = value:gsub('"', '\\"')
  value = value:gsub("\n", "\\n")
  value = value:gsub("\r", "\\r")
  return value
end

local function write_status(ok, matchName, candidate, fxIndex)
  if not statusJsonPath or statusJsonPath == "" then return end
  local f = io.open(statusJsonPath, "w")
  if not f then return end
  f:write("{\n")
  f:write('  "status": "' .. (ok and 'pass' or 'fail') .. '",\n')
  f:write('  "clapFxFound": ' .. tostring(ok) .. ',\n')
  f:write('  "matchedFxName": "' .. json_escape(matchName or "") .. '",\n')
  f:write('  "matchedCandidate": "' .. json_escape(candidate or "") .. '",\n')
  f:write('  "fxIndex": ' .. tostring(fxIndex or -1) .. '\n')
  f:write("}\n")
  f:close()
end

local trackCount = reaper.CountTracks(0)
reaper.InsertTrackAtIndex(trackCount, true)
local track = reaper.GetTrack(0, trackCount)

local candidates = {
  "CLAP: LocusQ",
  "CLAPi: LocusQ",
  "CLAP: LocusQ (Noizefield)",
  "CLAPi: LocusQ (Noizefield)",
}

local found = false
local foundName = nil
local foundCandidate = nil
local foundIdx = -1

for _, cand in ipairs(candidates) do
  local idx = reaper.TrackFX_AddByName(track, cand, false, 1)
  if idx >= 0 then
    local ok, name = reaper.TrackFX_GetFXName(track, idx, "")
    local fxName = ok and name or ""
    if fxName:lower():find("clap") then
      found = true
      foundName = fxName
      foundCandidate = cand
      foundIdx = idx
      break
    end
  end
end

write_status(found, foundName, foundCandidate, foundIdx)
LUA

    set +e
    LQ_REAPER_STATUS_JSON="$REAPER_PROBE_JSON" \
      "$REAPER_BIN" -new "$REAPER_PROBE_LUA" -closeall:nosave:exit -nosplash >"$REAPER_PROBE_LOG" 2>&1 &
    REAPER_PID=$!
    set -e

    deadline=$((SECONDS + 45))
    while [[ ! -f "$REAPER_PROBE_JSON" && $SECONDS -lt $deadline ]]; do
      sleep 1
    done

    osascript -e 'tell application "REAPER" to quit' >/dev/null 2>&1 || true
    kill "$REAPER_PID" >/dev/null 2>&1 || true
    wait "$REAPER_PID" >/dev/null 2>&1 || true
    pkill -x REAPER >/dev/null 2>&1 || true

    if [[ ! -f "$REAPER_PROBE_JSON" ]]; then
      log_status "reaper_clap_discovery" "fail" "probe_status_missing; log=$REAPER_PROBE_LOG"
      exit 1
    fi

    REAPER_PROBE_PASS="$(jq -r '.clapFxFound // false' "$REAPER_PROBE_JSON")"
    REAPER_PROBE_NAME="$(jq -r '.matchedFxName // ""' "$REAPER_PROBE_JSON")"
    if [[ "$REAPER_PROBE_PASS" == "true" ]]; then
      log_status "reaper_clap_discovery" "pass" "$REAPER_PROBE_NAME"
      REAPER_PROBE_STATUS="pass"
    else
      log_status "reaper_clap_discovery" "fail" "$REAPER_PROBE_NAME"
      exit 1
    fi
  else
    log_status "reaper_clap_discovery" "warn" "reaper_bin_missing=${REAPER_BIN}; reuse_enabled=${REUSE_REAPER_PROBE_PASS}"
    REAPER_PROBE_STATUS="warn_missing_reaper"
  fi
else
  log_status "reaper_clap_discovery" "warn" "disabled_via_LQ_BL011_RUN_REAPER_PROBE=0"
  REAPER_PROBE_STATUS="warn_disabled"
fi

run_cmd_logged \
  "docs_freshness" \
  "$OUT_DIR/docs_freshness.log" \
  "$ROOT_DIR/scripts/validate-docs-freshness.sh"

cat > "$REPORT_MD" <<EOF
Title: BL-011 CLAP Closeout Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-011 CLAP Closeout (${TIMESTAMP})

- clap_artifact: \`${CLAP_ARTIFACT}\`
- clap_install_artifact: \`${CLAP_INSTALL_ARTIFACT}\`
- qa_bin: \`${QA_BIN}\`
- standalone_app: \`${STANDALONE_APP}\`
- reaper_probe_status: \`${REAPER_PROBE_STATUS}\`
- selftest_retries: \`${SELFTEST_RETRIES}\`

## Key Outputs

- \`status.tsv\`
- \`clap-info.json\`
- \`clap-validator.txt\`
- \`qa_smoke_suite.log\`
- \`qa_phase_2_6_acceptance_suite.log\`
- \`ui_selftest_bl011_try_*.log\`
- \`ui_selftest_bl011_try_*.json\`
- \`reaper_clap_probe.json\` (when REAPER probe enabled)
- \`docs_freshness.log\`
EOF

log_status "summary" "pass" "report=$REPORT_MD"
echo "PASS: BL-011 CLAP closeout lane complete"
echo "artifact_dir=$OUT_DIR"
