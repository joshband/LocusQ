#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

BUILD_DIR="${LQ_BL012_BUILD_DIR:-$ROOT_DIR/build_local}"
BUILD_CONFIG="${LQ_BL012_BUILD_CONFIG:-Release}"
BUILD_JOBS="${LQ_BL012_BUILD_JOBS:-8}"
RUN_HARNESS_SANITY="${LQ_BL012_RUN_HARNESS_SANITY:-1}"
HARNESS_PATH="${LQ_BL012_HARNESS_PATH:-${APC_DSP_QA_HARNESS_PATH:-$HOME/Documents/Repos/audio-dsp-qa-harness}}"
HARNESS_BUILD_DIR="${LQ_BL012_HARNESS_BUILD_DIR:-$HARNESS_PATH/build_bl012_sanity}"

QA_BIN="${LQ_BL012_QA_BIN:-$BUILD_DIR/locusq_qa_artefacts/$BUILD_CONFIG/locusq_qa}"
CONTRACT_SUITE="qa/scenarios/locusq_contract_pack_suite.json"
PERF_SCENARIO="qa/scenarios/locusq_26_full_system_cpu_draft.json"
RUN_HX04_AUDIT="${LQ_BL012_RUN_HX04_AUDIT:-1}"
HX04_AUDIT_SCRIPT="${LQ_BL012_HX04_AUDIT_SCRIPT:-$ROOT_DIR/scripts/qa-hx04-scenario-audit.sh}"

OUT_DIR="$ROOT_DIR/TestEvidence/bl012_harness_backport_${TIMESTAMP}"
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

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "pass" "$(command -v "$cmd")"
  else
    log_status "tool_${cmd}" "fail" "missing_command"
    return 1
  fi
}

fail_and_exit() {
  local msg="$1"
  echo "FAIL: $msg"
  echo "artifact_dir=$OUT_DIR"
  exit 1
}

log_status "init" "pass" "ts=${TIMESTAMP}"
log_status "config" "pass" "build_dir=${BUILD_DIR}; build_config=${BUILD_CONFIG}; run_harness_sanity=${RUN_HARNESS_SANITY}; run_hx04_audit=${RUN_HX04_AUDIT}; harness_path=${HARNESS_PATH}"

require_cmd cmake || fail_and_exit "cmake not found"
require_cmd jq || fail_and_exit "jq not found"

if [[ ! -d "$HARNESS_PATH" ]]; then
  log_status "harness_path" "fail" "missing=${HARNESS_PATH}"
  fail_and_exit "audio-dsp-qa-harness path missing"
fi
log_status "harness_path" "pass" "$HARNESS_PATH"

if [[ "$RUN_HARNESS_SANITY" == "1" ]]; then
  HARNESS_CONFIG_LOG="$OUT_DIR/harness_configure.log"
  HARNESS_BUILD_LOG="$OUT_DIR/harness_build.log"
  HARNESS_CTEST_LOG="$OUT_DIR/harness_ctest.log"

  if cmake -S "$HARNESS_PATH" -B "$HARNESS_BUILD_DIR" -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 >"$HARNESS_CONFIG_LOG" 2>&1; then
    log_status "harness_configure" "pass" "log=${HARNESS_CONFIG_LOG}"
  else
    log_status "harness_configure" "fail" "log=${HARNESS_CONFIG_LOG}"
    fail_and_exit "harness configure failed"
  fi

  if cmake --build "$HARNESS_BUILD_DIR" -j "$BUILD_JOBS" >"$HARNESS_BUILD_LOG" 2>&1; then
    log_status "harness_build" "pass" "log=${HARNESS_BUILD_LOG}"
  else
    log_status "harness_build" "fail" "log=${HARNESS_BUILD_LOG}"
    fail_and_exit "harness build failed"
  fi

  if ctest --test-dir "$HARNESS_BUILD_DIR" --output-on-failure >"$HARNESS_CTEST_LOG" 2>&1; then
    log_status "harness_ctest" "pass" "log=${HARNESS_CTEST_LOG}"
  else
    log_status "harness_ctest" "fail" "log=${HARNESS_CTEST_LOG}"
    fail_and_exit "harness ctest failed"
  fi
else
  log_status "harness_sanity" "warn" "skipped_by_env=LQ_BL012_RUN_HARNESS_SANITY"
fi

QA_BUILD_LOG="$OUT_DIR/locusq_qa_build.log"
if cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" --target locusq_qa -j "$BUILD_JOBS" >"$QA_BUILD_LOG" 2>&1; then
  log_status "locusq_qa_build" "pass" "log=${QA_BUILD_LOG}"
else
  log_status "locusq_qa_build" "fail" "log=${QA_BUILD_LOG}"
  fail_and_exit "locusq_qa build failed"
fi

if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="$BUILD_DIR/locusq_qa_artefacts/locusq_qa"
fi
if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "fail" "missing=${QA_BIN}"
  fail_and_exit "locusq_qa binary missing"
fi
log_status "qa_bin" "pass" "$QA_BIN"

if [[ ! -f "$ROOT_DIR/$CONTRACT_SUITE" ]]; then
  log_status "contract_suite" "fail" "missing=${CONTRACT_SUITE}"
  fail_and_exit "contract suite missing"
fi
if [[ ! -f "$ROOT_DIR/$PERF_SCENARIO" ]]; then
  log_status "perf_scenario" "fail" "missing=${PERF_SCENARIO}"
  fail_and_exit "perf scenario missing"
fi
log_status "contract_suite" "pass" "$CONTRACT_SUITE"
log_status "perf_scenario" "pass" "$PERF_SCENARIO"

SUITE_OUTPUT_DIR="$(jq -r '.runtime_config.output_dir' "$ROOT_DIR/$CONTRACT_SUITE")"
SUITE_SR="$(jq -r '.runtime_config.sample_rate' "$ROOT_DIR/$CONTRACT_SUITE")"
SUITE_BS="$(jq -r '.runtime_config.block_size' "$ROOT_DIR/$CONTRACT_SUITE")"
SUITE_CH="$(jq -r '.runtime_config.channels' "$ROOT_DIR/$CONTRACT_SUITE")"

if [[ -z "$SUITE_OUTPUT_DIR" || "$SUITE_OUTPUT_DIR" == "null" ]]; then
  log_status "suite_runtime_config" "fail" "runtime_config.output_dir_missing"
  fail_and_exit "suite runtime_config output_dir missing"
fi
log_status "suite_runtime_config" "pass" "output_dir=${SUITE_OUTPUT_DIR}; sample_rate=${SUITE_SR}; block_size=${SUITE_BS}; channels=${SUITE_CH}"

CONTRACT_BASE_LOG="$OUT_DIR/contract_pack_baseline.log"
rm -rf "$ROOT_DIR/$SUITE_OUTPUT_DIR"
if "$QA_BIN" --spatial "$CONTRACT_SUITE" >"$CONTRACT_BASE_LOG" 2>&1; then
  log_status "contract_pack_baseline_run" "pass" "log=${CONTRACT_BASE_LOG}"
else
  log_status "contract_pack_baseline_run" "fail" "log=${CONTRACT_BASE_LOG}"
  fail_and_exit "contract pack baseline run failed"
fi

SUITE_RESULT_JSON="$ROOT_DIR/$SUITE_OUTPUT_DIR/suite_result.json"
if [[ ! -f "$SUITE_RESULT_JSON" ]]; then
  FALLBACK_SUITE_OUTPUT_DIR="qa_output/locusq_spatial"
  FALLBACK_SUITE_RESULT_JSON="$ROOT_DIR/$FALLBACK_SUITE_OUTPUT_DIR/suite_result.json"
  if [[ -f "$FALLBACK_SUITE_RESULT_JSON" ]]; then
    log_status "contract_pack_suite_result_dir_fallback" "warn" "expected_missing=${SUITE_RESULT_JSON}; fallback=${FALLBACK_SUITE_RESULT_JSON}"
    SUITE_OUTPUT_DIR="$FALLBACK_SUITE_OUTPUT_DIR"
    SUITE_RESULT_JSON="$FALLBACK_SUITE_RESULT_JSON"
  else
    log_status "contract_pack_suite_result" "fail" "missing=${SUITE_RESULT_JSON}; fallback_missing=${FALLBACK_SUITE_RESULT_JSON}"
    fail_and_exit "contract pack suite_result.json missing"
  fi
fi
cp "$SUITE_RESULT_JSON" "$OUT_DIR/contract_pack_suite_result_baseline.json"

CONTRACT_STATUS="$(jq -r '.status' "$SUITE_RESULT_JSON")"
CONTRACT_LAT="$(jq -r '.contract_coverage.latency // 0' "$SUITE_RESULT_JSON")"
CONTRACT_SMOOTH="$(jq -r '.contract_coverage.smoothing // 0' "$SUITE_RESULT_JSON")"
CONTRACT_STATE="$(jq -r '.contract_coverage.state // 0' "$SUITE_RESULT_JSON")"
CONTRACT_TOTAL="$(jq -r '.contract_coverage.contract_scenarios // 0' "$SUITE_RESULT_JSON")"

if [[ "$CONTRACT_STATUS" != "PASS" || "$CONTRACT_LAT" -lt 1 || "$CONTRACT_SMOOTH" -lt 1 || "$CONTRACT_STATE" -lt 1 || "$CONTRACT_TOTAL" -lt 3 ]]; then
  log_status "contract_pack_coverage" "fail" "status=${CONTRACT_STATUS}; latency=${CONTRACT_LAT}; smoothing=${CONTRACT_SMOOTH}; state=${CONTRACT_STATE}; total=${CONTRACT_TOTAL}"
  fail_and_exit "contract pack coverage assertion failed"
fi
log_status "contract_pack_coverage" "pass" "status=${CONTRACT_STATUS}; latency=${CONTRACT_LAT}; smoothing=${CONTRACT_SMOOTH}; state=${CONTRACT_STATE}; total=${CONTRACT_TOTAL}"

CONTRACT_OVERRIDE_LOG="$OUT_DIR/contract_pack_override_probe.log"
rm -rf "$ROOT_DIR/$SUITE_OUTPUT_DIR"
if "$QA_BIN" --spatial --sample-rate 44100 --block-size 256 --channels 4 "$CONTRACT_SUITE" >"$CONTRACT_OVERRIDE_LOG" 2>&1; then
  log_status "contract_pack_override_probe_run" "pass" "log=${CONTRACT_OVERRIDE_LOG}"
else
  log_status "contract_pack_override_probe_run" "fail" "log=${CONTRACT_OVERRIDE_LOG}"
  fail_and_exit "contract pack override probe run failed"
fi

if [[ ! -f "$SUITE_RESULT_JSON" ]]; then
  log_status "contract_pack_override_suite_result" "fail" "missing=${SUITE_RESULT_JSON}"
  fail_and_exit "contract pack override probe did not produce suite_result.json"
fi
cp "$SUITE_RESULT_JSON" "$OUT_DIR/contract_pack_suite_result_override_probe.json"

jq -r '.scenario_ids[]' "$ROOT_DIR/$CONTRACT_SUITE" >"$OUT_DIR/contract_pack_scenario_ids.txt"
RUNTIME_MISMATCH_COUNT=0
RUNTIME_CONFIG_REPORT="$OUT_DIR/runtime_config_probe.tsv"
printf "scenario_id\tsample_rate\tblock_size\tchannels\n" >"$RUNTIME_CONFIG_REPORT"

while IFS= read -r SCENARIO_ID; do
  RESULT_JSON="$ROOT_DIR/$SUITE_OUTPUT_DIR/$SCENARIO_ID/result.json"
  if [[ ! -f "$RESULT_JSON" ]]; then
    RESULT_JSON="$(find "$ROOT_DIR/$SUITE_OUTPUT_DIR/$SCENARIO_ID" -type f -name result.json 2>/dev/null | sort | tail -n 1 || true)"
  fi

  if [[ -z "$RESULT_JSON" || ! -f "$RESULT_JSON" ]]; then
    log_status "runtime_config_probe_${SCENARIO_ID}" "fail" "missing=${RESULT_JSON}"
    RUNTIME_MISMATCH_COUNT=$((RUNTIME_MISMATCH_COUNT + 1))
    continue
  fi

  SR="$(jq -r '.audio_config.sample_rate' "$RESULT_JSON")"
  BS="$(jq -r '.audio_config.block_size' "$RESULT_JSON")"
  CH="$(jq -r '.audio_config.num_channels' "$RESULT_JSON")"
  printf "%s\t%s\t%s\t%s\n" "$SCENARIO_ID" "$SR" "$BS" "$CH" >>"$RUNTIME_CONFIG_REPORT"

  if [[ "$SR" != "$SUITE_SR" || "$BS" != "$SUITE_BS" || "$CH" != "$SUITE_CH" ]]; then
    log_status "runtime_config_probe_${SCENARIO_ID}" "fail" "sample_rate=${SR}; block_size=${BS}; channels=${CH}; expected=${SUITE_SR}/${SUITE_BS}/${SUITE_CH}"
    RUNTIME_MISMATCH_COUNT=$((RUNTIME_MISMATCH_COUNT + 1))
  else
    log_status "runtime_config_probe_${SCENARIO_ID}" "pass" "sample_rate=${SR}; block_size=${BS}; channels=${CH}"
  fi
done <"$OUT_DIR/contract_pack_scenario_ids.txt"

if [[ "$RUNTIME_MISMATCH_COUNT" -ne 0 ]]; then
  log_status "runtime_config_probe_summary" "fail" "mismatch_count=${RUNTIME_MISMATCH_COUNT}; report=${RUNTIME_CONFIG_REPORT}"
  fail_and_exit "runtime config probe assertions failed"
fi
log_status "runtime_config_probe_summary" "pass" "mismatch_count=0; report=${RUNTIME_CONFIG_REPORT}"

PERF_PROBE_LOG="$OUT_DIR/perf_metric_auto_profile_probe.log"
if "$QA_BIN" --spatial "$PERF_SCENARIO" >"$PERF_PROBE_LOG" 2>&1; then
  log_status "perf_metric_probe_run" "pass" "log=${PERF_PROBE_LOG}"
else
  log_status "perf_metric_probe_run" "fail" "log=${PERF_PROBE_LOG}"
  fail_and_exit "perf metric probe scenario run failed"
fi

PERF_RESULT_JSON="$ROOT_DIR/qa_output/locusq_spatial/locusq_26_full_system_cpu_draft/result.json"
if [[ ! -f "$PERF_RESULT_JSON" ]]; then
  log_status "perf_metric_probe_result" "fail" "missing=${PERF_RESULT_JSON}"
  fail_and_exit "perf metric probe result json missing"
fi
cp "$PERF_RESULT_JSON" "$OUT_DIR/perf_metric_probe_result.json"

PERF_METRIC_COUNT="$(jq '[.metrics[]? | select((.metric // "") | startswith("perf_"))] | length' "$PERF_RESULT_JSON")"
if [[ "$PERF_METRIC_COUNT" -lt 1 ]]; then
  log_status "perf_metric_probe_assert" "fail" "perf_metric_count=${PERF_METRIC_COUNT}"
  fail_and_exit "perf metric probe did not include perf_* metrics"
fi
log_status "perf_metric_probe_assert" "pass" "perf_metric_count=${PERF_METRIC_COUNT}"

if [[ "$RUN_HX04_AUDIT" == "1" ]]; then
  if [[ ! -x "$HX04_AUDIT_SCRIPT" ]]; then
    log_status "hx04_audit_script" "fail" "missing_or_not_executable=${HX04_AUDIT_SCRIPT}"
    fail_and_exit "HX-04 audit script missing"
  fi
  log_status "hx04_audit_script" "pass" "$HX04_AUDIT_SCRIPT"

  HX04_AUDIT_DIR="$OUT_DIR/hx04_scenario_audit"
  HX04_AUDIT_LOG="$OUT_DIR/hx04_scenario_audit.log"
  if "$HX04_AUDIT_SCRIPT" --qa-bin "$QA_BIN" --out-dir "$HX04_AUDIT_DIR" >"$HX04_AUDIT_LOG" 2>&1; then
    log_status "hx04_audit_run" "pass" "log=${HX04_AUDIT_LOG}; artifacts=${HX04_AUDIT_DIR}"
  else
    log_status "hx04_audit_run" "fail" "log=${HX04_AUDIT_LOG}; artifacts=${HX04_AUDIT_DIR}"
    fail_and_exit "HX-04 scenario audit failed"
  fi
else
  log_status "hx04_audit_run" "warn" "skipped_by_env=LQ_BL012_RUN_HX04_AUDIT"
fi

cp "$SUITE_RESULT_JSON" "$ROOT_DIR/qa_output/suite_result.json"
log_status "qa_output_suite_result_sync" "pass" "qa_output/suite_result.json refreshed from ${SUITE_RESULT_JSON}"

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
Title: BL-012 Harness Backport Tranche 1 Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-012 Harness Backport Tranche 1 (${TIMESTAMP})

- overall: \`${OVERALL}\`
- run_harness_sanity: \`${RUN_HARNESS_SANITY}\`
- run_hx04_audit: \`${RUN_HX04_AUDIT}\`
- harness_path: \`${HARNESS_PATH}\`
- qa_bin: \`${QA_BIN}\`
- contract_suite: \`${CONTRACT_SUITE}\`
- perf_probe_scenario: \`${PERF_SCENARIO}\`
- pass_count: \`${PASS_COUNT}\`
- warn_count: \`${WARN_COUNT}\`
- fail_count: \`${FAIL_COUNT}\`

## Assertions

- Harness sanity configure/build/ctest passes (unless skipped).
- Contract pack suite passes with coverage counts:
  - latency >= 1
  - smoothing >= 1
  - state >= 1
  - contract_scenarios >= 3
- Suite runtime_config precedence holds against conflicting CLI runtime flags.
- Perf scenario emits \`perf_*\` metrics without forcing \`--profile\`.
- HX-04 scenario audit lane validates required AirAbsorption/Calibration/directivity coverage and executes dedicated parity suite when enabled.
- Canonical \`qa_output/suite_result.json\` refreshed from contract-pack suite output.

## Artifacts

- \`status.tsv\`
- \`contract_pack_suite_result_baseline.json\`
- \`contract_pack_suite_result_override_probe.json\`
- \`runtime_config_probe.tsv\`
- \`perf_metric_probe_result.json\`
- \`hx04_scenario_audit.log\` + \`hx04_scenario_audit/\` (when HX-04 audit is enabled)
- \`harness_*.log\` (when harness sanity enabled)
- \`locusq_qa_build.log\`
EOF

if [[ "$OVERALL" == "fail" ]]; then
  fail_and_exit "BL-012 harness backport tranche-1 lane failed"
fi

if [[ "$OVERALL" == "pass_with_warnings" ]]; then
  echo "PASS_WITH_WARNINGS: BL-012 harness backport tranche-1 lane completed"
else
  echo "PASS: BL-012 harness backport tranche-1 lane completed"
fi
echo "artifact_dir=$OUT_DIR"
