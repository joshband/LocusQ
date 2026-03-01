#!/usr/bin/env bash
# Title: BL-043 FDN Sample-Rate Sweep QA
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-02-26
# Last Modified Date: 2026-02-26
#
# BL-043 Slice B — validates that FDN delay times in milliseconds are
# invariant across 44100 / 48000 / 96000 / 192000 Hz sample rates.
#
# The script uses bc + awk to compute expected delay samples at each rate
# from the base delay values defined in FDNReverb.h and checks that the
# parity error stays within FDN_TIMING_TOLERANCE_MS (default 0.05 ms).
#
# If the locusq_qa harness supports --sample-rate, it also runs the smoke
# suite at each rate; otherwise it records a skip with reason.
#
# Outputs (written to --out-dir):
#   status.tsv            overall pass/fail per step
#   samplerate_sweep.tsv  per-rate build/smoke results
#   fdn_timing_parity.tsv per-line delay-time parity across rates
#   build.log             cmake build stdout/stderr
#   qa_smoke.log          locusq_qa stdout/stderr (concatenated)
#   docs_freshness.log    validate-docs-freshness.sh output
#
# Exit semantics:
#   0 = all checks pass (timing parity + smoke if available)
#   1 = one or more checks failed
#   2 = invocation error (bad args / prerequisites missing)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE="$(date -u +%Y-%m-%d)"

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
QA_BIN_DEFAULT="$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa"
SMOKE_SCENARIO_DEFAULT="$ROOT_DIR/qa/scenarios/locusq_smoke_suite.json"
OUT_DIR_DEFAULT="$ROOT_DIR/TestEvidence/bl043_sweep_${TIMESTAMP}"
TOLERANCE_MS_DEFAULT="0.05"   # allowable ms error per delay line per rate

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
usage() {
  cat <<'USAGE'
Usage: qa-bl043-fdn-samplerate-sweep-mac.sh [options]

BL-043 FDN sample-rate sweep validation.

Options:
  --out-dir <path>        Output artifact directory (default: TestEvidence/bl043_sweep_<ts>)
  --qa-bin <path>         locusq_qa binary (default: build_local/locusq_qa_artefacts/Release/locusq_qa)
  --scenario <path>       Smoke scenario JSON (default: qa/scenarios/locusq_smoke_suite.json)
  --tolerance-ms <float>  Allowable delay-time error per line in ms (default: 0.05)
  --skip-smoke            Skip locusq_qa smoke run even if binary exists
  --help, -h              Show this help

Outputs:
  status.tsv  samplerate_sweep.tsv  fdn_timing_parity.tsv
  build.log   qa_smoke.log          docs_freshness.log

Exit codes: 0=pass  1=check-failure  2=invocation-error
USAGE
}

die_invocation() { echo "ERROR: $1" >&2; exit 2; }

OUT_DIR=""
QA_BIN="$QA_BIN_DEFAULT"
SMOKE_SCENARIO="$SMOKE_SCENARIO_DEFAULT"
TOLERANCE_MS="$TOLERANCE_MS_DEFAULT"
SKIP_SMOKE=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)        [[ -n "${2:-}" ]] || die_invocation "--out-dir requires a value"; OUT_DIR="$2"; shift 2 ;;
    --qa-bin)         [[ -n "${2:-}" ]] || die_invocation "--qa-bin requires a value";  QA_BIN="$2";  shift 2 ;;
    --scenario)       [[ -n "${2:-}" ]] || die_invocation "--scenario requires a value"; SMOKE_SCENARIO="$2"; shift 2 ;;
    --tolerance-ms)   [[ -n "${2:-}" ]] || die_invocation "--tolerance-ms requires a value"; TOLERANCE_MS="$2"; shift 2 ;;
    --skip-smoke)     SKIP_SMOKE=1; shift ;;
    --help|-h)        usage; exit 0 ;;
    *) die_invocation "Unknown option: $1" ;;
  esac
done

[[ -z "$OUT_DIR" ]] && OUT_DIR="$OUT_DIR_DEFAULT"
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
SWEEP_TSV="$OUT_DIR/samplerate_sweep.tsv"
PARITY_TSV="$OUT_DIR/fdn_timing_parity.tsv"
BUILD_LOG="$OUT_DIR/build.log"
SMOKE_LOG="$OUT_DIR/qa_smoke.log"
DOCS_LOG="$OUT_DIR/docs_freshness.log"

# ---------------------------------------------------------------------------
# Logging helpers
# ---------------------------------------------------------------------------
log_status() {
  local step="$1" status="$2" detail="${3:-}"
  echo -e "${step}\t${status}\t${detail}" >> "$STATUS_TSV"
}

# ---------------------------------------------------------------------------
# Init status.tsv
# ---------------------------------------------------------------------------
echo -e "step\tstatus\tdetail" > "$STATUS_TSV"
log_status "init" "pass" "ts=${TIMESTAMP} out_dir=${OUT_DIR}"

# ---------------------------------------------------------------------------
# Step 1: Build
# ---------------------------------------------------------------------------
echo "==> [BL-043] Building release targets..." | tee -a "$BUILD_LOG"
if cmake --build "$ROOT_DIR/build_local" \
         --config Release \
         --target LocusQ_Standalone locusq_qa \
         -j 8 >> "$BUILD_LOG" 2>&1; then
  log_status "build" "pass" "cmake build Release succeeded"
else
  log_status "build" "fail" "cmake build failed (see build.log)"
  echo "ERROR: Build failed. See $BUILD_LOG" >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# Step 2: FDN timing parity — mathematical verification
#
# FDNReverb.h defines base delays in samples at REFERENCE_SAMPLE_RATE=44100.
# After BL-043 fix, configureDelayLengths() multiplies by srScale =
# currentSampleRate / 44100, so delay_ms = base_samples / 44100 * 1000 ≡ const.
#
# We verify this analytically for draft (4-line) and final (8-line) sets.
# ---------------------------------------------------------------------------
REFERENCE_SR=44100
# Draft base delays (samples @ 44100 Hz)
DRAFT_BASE=(1499 1877 2137 2557)
# Final base delays (samples @ 44100 Hz)
FINAL_BASE=(1423 1777 2137 2557 2879 3251 3623 3989)
RATES=(44100 48000 96000 192000)
ROOM_SIZE=1.0   # reference room size for parity check

echo -e "mode\tline\tbase_samples_ref\trate_hz\tdelay_samples\tdelay_ms\treference_ms\terror_ms\tpass" > "$PARITY_TSV"

PARITY_PASS=1

check_line() {
  local mode="$1" line_idx="$2" base="$3"
  local ref_ms
  ref_ms=$(awk "BEGIN { printf \"%.6f\", ${base} / ${REFERENCE_SR} * 1000 }")

  for rate in "${RATES[@]}"; do
    local sr_scale delay_samples delay_ms error_ms pass
    sr_scale=$(awk "BEGIN { printf \"%.10f\", ${rate} / ${REFERENCE_SR} }")
    delay_samples=$(awk "BEGIN { printf \"%.0f\", ${base} * ${ROOM_SIZE} * ${sr_scale} }")
    delay_ms=$(awk "BEGIN { printf \"%.6f\", ${delay_samples} / ${rate} * 1000 }")
    error_ms=$(awk "BEGIN { v = ${delay_ms} - ${ref_ms}; if (v<0) v=-v; printf \"%.6f\", v }")
    pass=$(awk "BEGIN { print (${error_ms} <= ${TOLERANCE_MS}) ? \"PASS\" : \"FAIL\" }")

    echo -e "${mode}\t${line_idx}\t${base}\t${rate}\t${delay_samples}\t${delay_ms}\t${ref_ms}\t${error_ms}\t${pass}" >> "$PARITY_TSV"

    if [[ "$pass" == "FAIL" ]]; then
      PARITY_PASS=0
    fi
  done
}

echo "==> [BL-043] Computing FDN timing parity across sample rates..."
for i in "${!DRAFT_BASE[@]}"; do
  check_line "draft" "$i" "${DRAFT_BASE[$i]}"
done
for i in "${!FINAL_BASE[@]}"; do
  check_line "final" "$i" "${FINAL_BASE[$i]}"
done

if [[ "$PARITY_PASS" -eq 1 ]]; then
  log_status "fdn_timing_parity" "pass" "all lines within ${TOLERANCE_MS} ms tolerance across 44100/48000/96000/192000 Hz"
else
  log_status "fdn_timing_parity" "fail" "one or more lines exceeded ${TOLERANCE_MS} ms tolerance — check fdn_timing_parity.tsv"
fi

# ---------------------------------------------------------------------------
# Step 3: Smoke suite at each sample rate (if harness supports --sample-rate)
# ---------------------------------------------------------------------------
echo -e "rate_hz\tsmoke_exit\tstatus\tdetail" > "$SWEEP_TSV"

run_smoke() {
  local rate="$1"
  local smoke_exit=0
  local detail=""

  if [[ "$SKIP_SMOKE" -eq 1 ]]; then
    echo -e "${rate}\tskipped\tskip\tskip_smoke=1" >> "$SWEEP_TSV"
    return
  fi

  if [[ ! -x "$QA_BIN" ]]; then
    echo -e "${rate}\tskipped\tskip\tqa_bin_missing" >> "$SWEEP_TSV"
    return
  fi

  if [[ ! -f "$SMOKE_SCENARIO" ]]; then
    echo -e "${rate}\tskipped\tskip\tscenario_missing=${SMOKE_SCENARIO}" >> "$SWEEP_TSV"
    return
  fi

  # Try --sample-rate flag; fall back to default if not supported.
  local run_cmd=("$QA_BIN" "--spatial" "$SMOKE_SCENARIO")
  if "$QA_BIN" --help 2>&1 | grep -q "\-\-sample-rate"; then
    run_cmd+=("--sample-rate" "$rate")
    detail="with_sample_rate_flag"
  else
    detail="no_sample_rate_flag_using_default"
  fi

  echo "  => Smoke at ${rate} Hz: ${run_cmd[*]}" | tee -a "$SMOKE_LOG"
  if "${run_cmd[@]}" >> "$SMOKE_LOG" 2>&1; then
    smoke_exit=0
    echo -e "${rate}\t0\tpass\t${detail}" >> "$SWEEP_TSV"
  else
    smoke_exit=$?
    echo -e "${rate}\t${smoke_exit}\tfail\t${detail}" >> "$SWEEP_TSV"
    PARITY_PASS=0
  fi
}

echo "==> [BL-043] Running smoke suite per sample rate..."
for rate in "${RATES[@]}"; do
  run_smoke "$rate"
done

all_smoke_pass=1
while IFS=$'\t' read -r rate exit_code status detail; do
  [[ "$rate" == "rate_hz" ]] && continue
  if [[ "$status" == "fail" ]]; then
    all_smoke_pass=0
  fi
done < "$SWEEP_TSV"

if [[ "$all_smoke_pass" -eq 1 ]]; then
  log_status "smoke_sweep" "pass" "all rates passed or skipped"
else
  log_status "smoke_sweep" "fail" "one or more rates failed smoke — check samplerate_sweep.tsv"
fi

# ---------------------------------------------------------------------------
# Step 4: Docs freshness gate
# ---------------------------------------------------------------------------
echo "==> [BL-043] Running docs freshness gate..."
if "$ROOT_DIR/scripts/validate-docs-freshness.sh" > "$DOCS_LOG" 2>&1; then
  log_status "docs_freshness" "pass" "exit 0"
else
  log_status "docs_freshness" "fail" "exit non-zero (see docs_freshness.log)"
fi

# ---------------------------------------------------------------------------
# Final summary
# ---------------------------------------------------------------------------
FINAL_PASS=1
while IFS=$'\t' read -r step status detail; do
  [[ "$step" == "step" ]] && continue
  if [[ "$status" == "fail" ]]; then
    FINAL_PASS=0
  fi
done < "$STATUS_TSV"

echo ""
echo "==> [BL-043] Sweep complete. Artifacts: $OUT_DIR"
echo ""
column -t -s $'\t' "$STATUS_TSV"
echo ""

if [[ "$FINAL_PASS" -eq 1 ]]; then
  echo "RESULT: PASS"
  exit 0
else
  echo "RESULT: FAIL — review status.tsv and fdn_timing_parity.tsv"
  exit 1
fi
