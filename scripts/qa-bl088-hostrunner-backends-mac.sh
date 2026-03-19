#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

MODE="contract"
RUNS=1
OUT_DIR=""
LOCUSQ_BUILD_ROOT="${ROOT_DIR}/build_bl088_live"
HARNESS_BUILD_DIR="${ROOT_DIR}/../audio-dsp-qa-harness/build_bl088_live"
LOCUSQ_VST3_PATH_OVERRIDE="${LOCUSQ_VST3_PATH:-}"
LOCUSQ_QA_BIN_OVERRIDE="${LOCUSQ_QA_BIN:-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only) MODE="contract"; shift ;;
    --execute) MODE="execute"; shift ;;
    --live) MODE="live"; shift ;;
    --runs) RUNS="${2:-1}"; shift 2 ;;
    --out) OUT_DIR="${2:?missing output dir}"; shift 2 ;;
    --locusq-build-root) LOCUSQ_BUILD_ROOT="${2:?missing build root}"; shift 2 ;;
    --harness-build-dir) HARNESS_BUILD_DIR="${2:?missing harness build dir}"; shift 2 ;;
    --locusq-vst3-path) LOCUSQ_VST3_PATH_OVERRIDE="${2:?missing vst3 path}"; shift 2 ;;
    --locusq-qa-bin) LOCUSQ_QA_BIN_OVERRIDE="${2:?missing qa binary path}"; shift 2 ;;
    *) echo "Unknown argument: $1" >&2; exit 1 ;;
  esac
done

timestamp() { date -u +"%Y%m%dT%H%M%SZ"; }

if [[ -z "$OUT_DIR" ]]; then
  OUT_DIR="${ROOT_DIR}/TestEvidence/bl088_hostrunner_backends_$(timestamp)"
fi
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
printf "check\tresult\tdetail\n" > "$STATUS_TSV"

record() { printf "%s\t%s\t%s\n" "$1" "$2" "$3" | tee -a "$STATUS_TSV"; }

HARNESS_DIR="${ROOT_DIR}/../audio-dsp-qa-harness"
INTEGRATION_TEST="${HARNESS_DIR}/tests/host_runner_integration_test.cpp"
HARNESS_CMAKE="${HARNESS_DIR}/CMakeLists.txt"

# ---- contract checks ----

for runner_file in \
    "runners/vst3_plugin_host.h" \
    "runners/au_plugin_host.h" \
    "runners/host_runner.h" \
    "runners/host_runner.cpp"; do
  full_path="${HARNESS_DIR}/${runner_file}"
  key="runner_${runner_file//[\/.]/_}"
  if [[ -f "$full_path" ]]; then
    record "$key" "PASS" "found $full_path"
  else
    record "$key" "FAIL" "missing $full_path"
    exit 1
  fi
done

if grep -q "LOCUSQ_HOST_RUNNER_LIVE" "$INTEGRATION_TEST"; then
  record "live_test_guard" "PASS" "LOCUSQ_HOST_RUNNER_LIVE guard present in integration test"
else
  record "live_test_guard" "FAIL" "LOCUSQ_HOST_RUNNER_LIVE guard missing from integration test"
  exit 1
fi

if grep -q "LOCUSQ_HOST_RUNNER_LIVE" "$HARNESS_CMAKE"; then
  record "cmake_option" "PASS" "LOCUSQ_HOST_RUNNER_LIVE CMake option present"
else
  record "cmake_option" "FAIL" "LOCUSQ_HOST_RUNNER_LIVE CMake option missing from CMakeLists.txt"
  exit 1
fi

echo "# BL-088 Contract Check — PASS" > "$OUT_DIR/summary.md"

if [[ "$MODE" == "contract" ]]; then
  exit 0
fi

if [[ "$MODE" == "live" ]]; then
  LOCUSQ_VST3_PATH_DEFAULT="${LOCUSQ_BUILD_ROOT}/LocusQ_artefacts/Release/VST3/LocusQ.vst3"
  LOCUSQ_QA_BIN_DEFAULT="${LOCUSQ_BUILD_ROOT}/locusq_qa_artefacts/Release/locusq_qa"
  LOCUSQ_VST3_PATH_FINAL="${LOCUSQ_VST3_PATH_OVERRIDE:-$LOCUSQ_VST3_PATH_DEFAULT}"
  LOCUSQ_QA_BIN_FINAL="${LOCUSQ_QA_BIN_OVERRIDE:-$LOCUSQ_QA_BIN_DEFAULT}"

  if [[ ! -e "$LOCUSQ_VST3_PATH_FINAL" ]]; then
    record "live_plugin_path" "FAIL" "missing LocusQ VST3 bundle at $LOCUSQ_VST3_PATH_FINAL"
    exit 1
  fi
  record "live_plugin_path" "PASS" "found $LOCUSQ_VST3_PATH_FINAL"

  if [[ ! -x "$LOCUSQ_QA_BIN_FINAL" ]]; then
    record "live_repo_smoke_binary" "FAIL" "missing locusq_qa binary at $LOCUSQ_QA_BIN_FINAL"
    exit 1
  fi
  record "live_repo_smoke_binary" "PASS" "found $LOCUSQ_QA_BIN_FINAL"

  LIVE_BUILD_LOG="$OUT_DIR/harness_live_build.log"
  LIVE_CTEST_LOG="$OUT_DIR/harness_live_ctest.log"
  LIVE_REPO_SMOKE_LOG="$OUT_DIR/repo_hostrunner_smoke.log"
  LIVE_REPO_OUTPUT_DIR="$OUT_DIR/repo_hostrunner_output"

  cmake \
    -S "$HARNESS_DIR" \
    -B "$HARNESS_BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_HOST_RUNNER=ON \
    -DBUILD_QA_TESTS=ON \
    -DLOCUSQ_HOST_RUNNER_LIVE=ON \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    > "$LIVE_BUILD_LOG" 2>&1

  cmake --build "$HARNESS_BUILD_DIR" \
    --target host_runner_integration_test \
    -j4 \
    >> "$LIVE_BUILD_LOG" 2>&1
  record "live_harness_build" "PASS" "host_runner_integration_test built in $HARNESS_BUILD_DIR"

  "$LOCUSQ_QA_BIN_FINAL" \
    --host-runner-smoke \
    --host-format vst3 \
    --host-plugin "$LOCUSQ_VST3_PATH_FINAL" \
    --host-output "$LIVE_REPO_OUTPUT_DIR" \
    --sample-rate 48000 \
    --block-size 512 \
    --channels 2 \
    > "$LIVE_REPO_SMOKE_LOG" 2>&1
  record "live_repo_smoke" "PASS" "locusq_qa host-runner smoke PASS against $LOCUSQ_VST3_PATH_FINAL"

  if [[ -f "$LIVE_REPO_OUTPUT_DIR/dry.wav" && -f "$LIVE_REPO_OUTPUT_DIR/wet.wav" ]]; then
    record "live_repo_artifacts" "PASS" "dry.wav and wet.wav captured under $LIVE_REPO_OUTPUT_DIR"
  else
    record "live_repo_artifacts" "FAIL" "missing dry/wet output under $LIVE_REPO_OUTPUT_DIR"
    exit 1
  fi

  LOCUSQ_VST3_PATH="$LOCUSQ_VST3_PATH_FINAL" \
  ctest \
    --test-dir "$HARNESS_BUILD_DIR" \
    -R "host_runner_integration_test" \
    --output-on-failure \
    > "$LIVE_CTEST_LOG" 2>&1
  record "live_harness_ctest" "PASS" "host_runner_integration_test PASS under LOCUSQ_HOST_RUNNER_LIVE"

  record "lane_result" "PASS" "bl088 live lane passed repo smoke + harness live ctest"
  cat > "$OUT_DIR/summary.md" <<EOF
# BL-088 Live Check — PASS

- plugin: \`$LOCUSQ_VST3_PATH_FINAL\`
- repo smoke: \`$LOCUSQ_QA_BIN_FINAL --host-runner-smoke\`
- harness live ctest: \`ctest --test-dir $HARNESS_BUILD_DIR -R host_runner_integration_test\`
EOF
  exit 0
fi

# ---- execute: build + ctest mock paths; expect live test SKIPPED ----

BUILD_DIR="${HARNESS_DIR}/build_bl088"
fail_count=0

for ((run=1; run<=RUNS; run++)); do
  BUILD_LOG="$OUT_DIR/build_run_${run}.log"
  CTEST_LOG="$OUT_DIR/ctest_run_${run}.log"

  cmake \
    -S "$HARNESS_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_HOST_RUNNER=ON \
    -DBUILD_QA_TESTS=ON \
    -DLOCUSQ_HOST_RUNNER_LIVE=OFF \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    > "$BUILD_LOG" 2>&1

  cmake --build "$BUILD_DIR" \
    --target host_runner_unit_test host_runner_integration_test \
    -j4 \
    >> "$BUILD_LOG" 2>&1

  if ctest \
    --test-dir "$BUILD_DIR" \
    -R "host_runner" \
    --output-on-failure \
    > "$CTEST_LOG" 2>&1; then
    record "ctest_run_${run}" "PASS" "host_runner ctest PASS (mock paths)"
  else
    record "ctest_run_${run}" "FAIL" "host_runner ctest FAIL — see $CTEST_LOG"
    fail_count=$((fail_count + 1))
  fi
done

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "bl088 execute lane passed runs=${RUNS}"
  echo "# BL-088 Execute Check — PASS" >> "$OUT_DIR/summary.md"
  exit 0
fi

record "lane_result" "FAIL" "bl088 execute lane failed fail_count=${fail_count}"
echo "# BL-088 Execute Check — FAIL" >> "$OUT_DIR/summary.md"
exit 1
