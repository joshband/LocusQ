#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BL085_BUILD_DIR:-$ROOT_DIR/build_bl085}"
OUT_DIR="${BL085_OUT_DIR:-$ROOT_DIR/TestEvidence/bl085_$(date -u +%Y%m%dT%H%M%SZ)}"
HARNESS_DIR="${BL085_HARNESS_DIR:-$ROOT_DIR/../audio-dsp-qa-harness}"
BUILD_CONFIG="${BL085_BUILD_CONFIG:-Release}"
SMOKE_SUITE="${BL085_SMOKE_SUITE:-$ROOT_DIR/qa/scenarios/locusq_smoke_suite.json}"
BUILD_JOBS="${BL085_BUILD_JOBS:-8}"

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --build-dir <path>     Build directory (default: build_bl085)
  --out-dir <path>       Output evidence directory
  --harness-dir <path>   audio-dsp-qa-harness root (default: ../audio-dsp-qa-harness)
  --config <name>        Build config (default: Release)
  --smoke-suite <path>   Smoke suite JSON (default: qa/scenarios/locusq_smoke_suite.json)
  --jobs <count>         Parallel build jobs (default: 8)
  -h, --help             Show help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="$2"
      shift 2
      ;;
    --harness-dir)
      HARNESS_DIR="$2"
      shift 2
      ;;
    --config)
      BUILD_CONFIG="$2"
      shift 2
      ;;
    --smoke-suite)
      SMOKE_SUITE="$2"
      shift 2
      ;;
    --jobs)
      BUILD_JOBS="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

mkdir -p "$OUT_DIR"
STATUS_TSV="$OUT_DIR/status.tsv"
VALIDATION_MATRIX="$OUT_DIR/validation_matrix.tsv"
CONFIGURE_LOG="$OUT_DIR/configure.log"
BUILD_LOG="$OUT_DIR/build.log"
SMOKE_LOG="$OUT_DIR/smoke.log"

printf 'step\tstatus\texit_code\tartifact\n' >"$STATUS_TSV"
printf 'step\tcommand\tstatus\texit_code\tartifact\n' >"$VALIDATION_MATRIX"

log_step() {
  local step="$1"
  local status="$2"
  local exit_code="$3"
  local artifact="$4"
  printf '%s\t%s\t%s\t%s\n' "$step" "$status" "$exit_code" "$artifact" >>"$STATUS_TSV"
}

log_matrix() {
  local step="$1"
  local command="$2"
  local status="$3"
  local exit_code="$4"
  local artifact="$5"
  printf '%s\t%s\t%s\t%s\t%s\n' "$step" "$command" "$status" "$exit_code" "$artifact" >>"$VALIDATION_MATRIX"
}

run_logged() {
  local step="$1"
  local log_path="$2"
  shift 2
  local cmd=("$@")
  local status="PASS"
  local exit_code=0
  if "${cmd[@]}" >"$log_path" 2>&1; then
    :
  else
    status="FAIL"
    exit_code=$?
  fi
  log_step "$step" "$status" "$exit_code" "$log_path"
  log_matrix "$step" "${cmd[*]}" "$status" "$exit_code" "$log_path"
  if [[ "$status" != "PASS" ]]; then
    return "$exit_code"
  fi
}

run_logged "configure" "$CONFIGURE_LOG" \
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DBUILD_LOCUSQ_QA=ON \
    -DQA_HARNESS_DIR="$HARNESS_DIR" \
    -DLOCUSQ_ENABLE_STEAM_AUDIO=OFF \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5

run_logged "build" "$BUILD_LOG" \
  cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" \
    --target LocusQ_Standalone locusq_qa -j "$BUILD_JOBS"

QA_BIN="$BUILD_DIR/locusq_qa_artefacts/$BUILD_CONFIG/locusq_qa"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="$BUILD_DIR/locusq_qa_artefacts/locusq_qa"
fi

if [[ ! -x "$QA_BIN" ]]; then
  log_step "qa_bin" "FAIL" "1" "$QA_BIN"
  log_matrix "qa_bin" "locate locusq_qa binary" "FAIL" "1" "$QA_BIN"
  echo "locusq_qa binary not found under $BUILD_DIR" >&2
  exit 1
fi

run_logged "smoke" "$SMOKE_LOG" \
  "$QA_BIN" --spatial "$SMOKE_SUITE"

echo "BL-085 validation complete"
echo "  out_dir: $OUT_DIR"
echo "  build_dir: $BUILD_DIR"
echo "  qa_bin: $QA_BIN"
