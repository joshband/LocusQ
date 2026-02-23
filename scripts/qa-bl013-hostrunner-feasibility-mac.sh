#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

BUILD_DIR="${LQ_BL013_BUILD_DIR:-$ROOT_DIR/build_bl013_hostrunner}"
BUILD_CONFIG="${LQ_BL013_BUILD_CONFIG:-Release}"
BUILD_JOBS="${LQ_BL013_BUILD_JOBS:-8}"
ENABLE_CLAP="${LQ_BL013_ENABLE_CLAP:-0}"
RUN_HARNESS_HOST_TESTS="${LQ_BL013_RUN_HARNESS_HOST_TESTS:-0}"
SKIP_BUILD="${LQ_BL013_SKIP_BUILD:-0}"

HARNESS_PATH="${LQ_BL013_HARNESS_PATH:-${APC_DSP_QA_HARNESS_PATH:-$HOME/Documents/Repos/audio-dsp-qa-harness}}"
HARNESS_BUILD_DIR="${LQ_BL013_HARNESS_BUILD_DIR:-$HARNESS_PATH/build_bl013_hostrunner}"

QA_BIN="${LQ_BL013_QA_BIN:-$BUILD_DIR/locusq_qa_artefacts/$BUILD_CONFIG/locusq_qa}"

OUT_DIR="$ROOT_DIR/TestEvidence/bl013_hostrunner_feasibility_${TIMESTAMP}"
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

find_plugin_artifact() {
  local base_dir="$1"
  local name="$2"
  local type_flag="$3"
  find "$base_dir" -type "$type_flag" -name "$name" 2>/dev/null | sort | head -n 1
}

log_status "init" "pass" "ts=${TIMESTAMP}"
log_status "config" "pass" "build_dir=${BUILD_DIR}; build_config=${BUILD_CONFIG}; build_jobs=${BUILD_JOBS}; enable_clap=${ENABLE_CLAP}; run_harness_host_tests=${RUN_HARNESS_HOST_TESTS}; skip_build=${SKIP_BUILD}; harness_path=${HARNESS_PATH}"

require_cmd cmake || fail_and_exit "cmake not found"
require_cmd ctest || fail_and_exit "ctest not found"
require_cmd jq || fail_and_exit "jq not found"

if [[ ! -d "$HARNESS_PATH" ]]; then
  log_status "harness_path" "fail" "missing=${HARNESS_PATH}"
  fail_and_exit "audio-dsp-qa-harness path missing"
fi
log_status "harness_path" "pass" "$HARNESS_PATH"

if [[ "$RUN_HARNESS_HOST_TESTS" == "1" ]]; then
  HARNESS_CONFIG_LOG="$OUT_DIR/harness_configure.log"
  HARNESS_BUILD_LOG="$OUT_DIR/harness_build.log"
  HARNESS_TEST_LIST_LOG="$OUT_DIR/harness_host_ctest_list.log"
  HARNESS_TEST_LOG="$OUT_DIR/harness_host_ctest.log"

  if cmake -S "$HARNESS_PATH" -B "$HARNESS_BUILD_DIR" -DBUILD_QA_TESTS=ON -DBUILD_HOST_RUNNER=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 >"$HARNESS_CONFIG_LOG" 2>&1; then
    log_status "harness_configure" "pass" "log=${HARNESS_CONFIG_LOG}"
  else
    log_status "harness_configure" "warn" "configure_failed; log=${HARNESS_CONFIG_LOG}"
  fi

  if cmake --build "$HARNESS_BUILD_DIR" -j "$BUILD_JOBS" >"$HARNESS_BUILD_LOG" 2>&1; then
    log_status "harness_build" "pass" "log=${HARNESS_BUILD_LOG}"
  else
    log_status "harness_build" "warn" "build_failed; log=${HARNESS_BUILD_LOG}"
  fi

  if [[ -f "$HARNESS_BUILD_DIR/CTestTestfile.cmake" ]]; then
    ctest --test-dir "$HARNESS_BUILD_DIR" -N >"$HARNESS_TEST_LIST_LOG" 2>&1 || true

    if ctest --test-dir "$HARNESS_BUILD_DIR" --output-on-failure -R "(host_runner|vst3_plugin_host|clap_plugin_host|au_plugin_host)" >"$HARNESS_TEST_LOG" 2>&1; then
      if rg -q "No tests were found!!!" "$HARNESS_TEST_LOG"; then
        log_status "harness_host_ctest" "warn" "no_tests_matched; log=${HARNESS_TEST_LOG}"
      else
        log_status "harness_host_ctest" "pass" "log=${HARNESS_TEST_LOG}"
      fi
    else
      log_status "harness_host_ctest" "warn" "ctest_failed; log=${HARNESS_TEST_LOG}"
    fi
  else
    log_status "harness_host_ctest" "warn" "ctest_unavailable_after_build; build_dir=${HARNESS_BUILD_DIR}"
  fi
else
  log_status "harness_host_ctest" "warn" "skipped_by_env=LQ_BL013_RUN_HARNESS_HOST_TESTS"
fi

CONFIGURE_LOG="$OUT_DIR/locusq_configure.log"
if [[ "$SKIP_BUILD" == "1" ]]; then
  log_status "locusq_configure" "warn" "skipped_by_env=LQ_BL013_SKIP_BUILD"
  log_status "locusq_build" "warn" "skipped_by_env=LQ_BL013_SKIP_BUILD"
else
  if cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DBUILD_LOCUSQ_QA=ON -DQA_HARNESS_DIR="$HARNESS_PATH" -DBUILD_HOST_RUNNER=ON -DLOCUSQ_ENABLE_STEAM_AUDIO=OFF -DLOCUSQ_ENABLE_CLAP="$ENABLE_CLAP" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 >"$CONFIGURE_LOG" 2>&1; then
    log_status "locusq_configure" "pass" "log=${CONFIGURE_LOG}"
  else
    log_status "locusq_configure" "fail" "log=${CONFIGURE_LOG}"
    fail_and_exit "LocusQ configure failed"
  fi

  BUILD_LOG="$OUT_DIR/locusq_build.log"
  if cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" --target locusq_qa LocusQ_VST3 -j "$BUILD_JOBS" >"$BUILD_LOG" 2>&1; then
    log_status "locusq_build" "pass" "targets=locusq_qa,LocusQ_VST3; log=${BUILD_LOG}"
  else
    if cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" --target locusq_qa LocusQ -j "$BUILD_JOBS" >>"$BUILD_LOG" 2>&1; then
      log_status "locusq_build" "warn" "fallback_targets=locusq_qa,LocusQ; log=${BUILD_LOG}"
    else
      log_status "locusq_build" "fail" "log=${BUILD_LOG}"
      fail_and_exit "LocusQ build failed"
    fi
  fi

  if [[ "$ENABLE_CLAP" == "1" ]]; then
    CLAP_BUILD_LOG="$OUT_DIR/locusq_build_clap.log"
    if cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" --target LocusQ_CLAP -j "$BUILD_JOBS" >"$CLAP_BUILD_LOG" 2>&1; then
      log_status "locusq_build_clap" "pass" "target=LocusQ_CLAP; log=${CLAP_BUILD_LOG}"
    else
      log_status "locusq_build_clap" "warn" "target=LocusQ_CLAP unavailable_or_failed; log=${CLAP_BUILD_LOG}"
    fi
  fi
fi

if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="$BUILD_DIR/locusq_qa_artefacts/locusq_qa"
fi
if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "fail" "missing=${QA_BIN}"
  fail_and_exit "locusq_qa binary missing"
fi
log_status "qa_bin" "pass" "$QA_BIN"

VST3_PLUGIN_PATH="${LQ_BL013_VST3_PLUGIN_PATH:-}"
if [[ -z "$VST3_PLUGIN_PATH" ]]; then
  VST3_PLUGIN_PATH="$(find_plugin_artifact "$BUILD_DIR" "LocusQ.vst3" d)"
fi
if [[ -z "$VST3_PLUGIN_PATH" || ! -d "$VST3_PLUGIN_PATH" ]]; then
  log_status "vst3_plugin_path" "fail" "missing=${VST3_PLUGIN_PATH}"
  fail_and_exit "Unable to locate LocusQ.vst3 artifact"
fi
log_status "vst3_plugin_path" "pass" "$VST3_PLUGIN_PATH"

VST3_OUTPUT_DIR="$OUT_DIR/hostrunner_vst3_output"
VST3_LOG="$OUT_DIR/hostrunner_vst3_probe.log"
set +e
"$QA_BIN" --host-runner-smoke --host-format vst3 --host-plugin "$VST3_PLUGIN_PATH" --host-output "$VST3_OUTPUT_DIR" --sample-rate 48000 --block-size 512 --channels 2 >"$VST3_LOG" 2>&1
VST3_EXIT=$?
set -e

if [[ "$VST3_EXIT" -eq 0 ]]; then
  log_status "hostrunner_vst3_probe" "pass" "log=${VST3_LOG}"
  if [[ -f "$VST3_OUTPUT_DIR/dry.wav" && -f "$VST3_OUTPUT_DIR/wet.wav" ]]; then
    log_status "hostrunner_vst3_artifacts" "pass" "dry=${VST3_OUTPUT_DIR}/dry.wav; wet=${VST3_OUTPUT_DIR}/wet.wav"
  else
    log_status "hostrunner_vst3_artifacts" "fail" "missing_dry_or_wet_in=${VST3_OUTPUT_DIR}"
    fail_and_exit "HostRunner VST3 probe reported success without dry/wet files"
  fi
else
  CRASH_IPS="$(ls -1t "$HOME/Library/Logs/DiagnosticReports" 2>/dev/null | rg '^locusq_qa.*\\.ips$' | head -n 1 || true)"
  if [[ -n "$CRASH_IPS" ]]; then
    log_status "hostrunner_vst3_probe" "warn" "exit=${VST3_EXIT}; log=${VST3_LOG}; crash=$HOME/Library/Logs/DiagnosticReports/${CRASH_IPS}"
  else
    log_status "hostrunner_vst3_probe" "warn" "exit=${VST3_EXIT}; log=${VST3_LOG}"
  fi
fi

VST3_SKELETON_LOG="$OUT_DIR/hostrunner_vst3_skeleton_probe.log"
if "$QA_BIN" --host-runner-smoke --host-skeleton --host-format vst3 --host-plugin "$VST3_PLUGIN_PATH" --host-output "$OUT_DIR/hostrunner_vst3_skeleton_output" --sample-rate 48000 --block-size 512 --channels 2 >"$VST3_SKELETON_LOG" 2>&1; then
  log_status "hostrunner_vst3_skeleton_probe" "pass" "log=${VST3_SKELETON_LOG}"
else
  log_status "hostrunner_vst3_skeleton_probe" "fail" "log=${VST3_SKELETON_LOG}"
  fail_and_exit "HostRunner VST3 skeleton probe failed"
fi

if [[ "$ENABLE_CLAP" == "1" ]]; then
  CLAP_PLUGIN_PATH="${LQ_BL013_CLAP_PLUGIN_PATH:-}"
  if [[ -z "$CLAP_PLUGIN_PATH" ]]; then
    CLAP_PLUGIN_PATH="$(find_plugin_artifact "$BUILD_DIR" "LocusQ.clap" f)"
  fi

  if [[ -n "$CLAP_PLUGIN_PATH" && -f "$CLAP_PLUGIN_PATH" ]]; then
    log_status "clap_plugin_path" "pass" "$CLAP_PLUGIN_PATH"

    CLAP_OUTPUT_DIR="$OUT_DIR/hostrunner_clap_output"
    CLAP_LOG="$OUT_DIR/hostrunner_clap_probe.log"
    if "$QA_BIN" --host-runner-smoke --host-format clap --host-plugin "$CLAP_PLUGIN_PATH" --host-output "$CLAP_OUTPUT_DIR" --sample-rate 48000 --block-size 512 --channels 2 >"$CLAP_LOG" 2>&1; then
      log_status "hostrunner_clap_probe" "pass" "log=${CLAP_LOG}"
      if [[ -f "$CLAP_OUTPUT_DIR/dry.wav" && -f "$CLAP_OUTPUT_DIR/wet.wav" ]]; then
        log_status "hostrunner_clap_artifacts" "pass" "dry=${CLAP_OUTPUT_DIR}/dry.wav; wet=${CLAP_OUTPUT_DIR}/wet.wav"
      else
        log_status "hostrunner_clap_artifacts" "fail" "missing_dry_or_wet_in=${CLAP_OUTPUT_DIR}"
        fail_and_exit "HostRunner CLAP probe did not emit dry/wet files"
      fi
    else
      log_status "hostrunner_clap_probe" "warn" "probe_failed; log=${CLAP_LOG}"
    fi
  else
    log_status "clap_plugin_path" "warn" "missing_clap_artifact"
  fi
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

cat >"$REPORT_MD" <<EOF2
Title: BL-013 HostRunner Feasibility Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-013 HostRunner Feasibility (${TIMESTAMP})

- overall: \`${OVERALL}\`
- build_dir: \`${BUILD_DIR}\`
- build_config: \`${BUILD_CONFIG}\`
- qa_bin: \`${QA_BIN}\`
- harness_path: \`${HARNESS_PATH}\`
- run_harness_host_tests: \`${RUN_HARNESS_HOST_TESTS}\`
- enable_clap_probe: \`${ENABLE_CLAP}\`

## Prototype Lane

1. Build harness with \`BUILD_HOST_RUNNER=ON\` and run host-focused ctests (configurable).
2. Configure/build LocusQ with \`BUILD_LOCUSQ_QA=ON\` + \`BUILD_HOST_RUNNER=ON\`.
3. Run \`locusq_qa --host-runner-smoke\` against a real \`LocusQ.vst3\` artifact.
4. Run \`locusq_qa --host-runner-smoke --host-skeleton\` as deterministic fallback contract.

## Summary Counts

- pass: \`${PASS_COUNT}\`
- warn: \`${WARN_COUNT}\`
- fail: \`${FAIL_COUNT}\`

## Artifacts

- \`status.tsv\`
- \`locusq_configure.log\`
- \`locusq_build.log\`
- \`hostrunner_vst3_probe.log\`
- \`hostrunner_vst3_skeleton_probe.log\`
- \`hostrunner_vst3_output/dry.wav\` and \`hostrunner_vst3_output/wet.wav\` (when backend probe passes)
- \`harness_host_ctest.log\` (when harness-host-tests are enabled)
EOF2

if [[ "$OVERALL" == "fail" ]]; then
  fail_and_exit "BL-013 HostRunner feasibility lane failed"
fi

if [[ "$OVERALL" == "pass_with_warnings" ]]; then
  echo "PASS_WITH_WARNINGS: BL-013 HostRunner feasibility lane completed"
else
  echo "PASS: BL-013 HostRunner feasibility lane completed"
fi

echo "artifact_dir=$OUT_DIR"
