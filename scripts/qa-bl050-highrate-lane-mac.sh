#!/usr/bin/env bash
# Title: BL-050 High-Rate Delay and FIR Hardening Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-01
#
# Exit codes:
#   0 all hard gates passed
#   1 one or more hard gates failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

OUT_DIR="${ROOT_DIR}/TestEvidence/bl050_highrate_lane_${TIMESTAMP}"
QA_BIN="${ROOT_DIR}/build_local/locusq_qa_artefacts/Release/locusq_qa"
FIR_SCENARIO="${ROOT_DIR}/qa/scenarios/locusq_210c_fdn_modulated_deterministic.json"
TARGET_DELAY_MS="50.0"
SAMPLE_RATES="44100 48000 88200 96000 192000"
BLOCK_SIZE="512"
CHANNELS="2"
PROFILE_ITERATIONS="200"
PROFILE_WARMUP="10"
SKIP_BUILD=0
SKIP_FIR=0

usage() {
  cat <<'USAGE'
Usage: qa-bl050-highrate-lane-mac.sh [options]

BL-050 high-rate delay/FIR lane.

Options:
  --out-dir <path>             Artifact output directory
  --qa-bin <path>              QA binary path
  --fir-scenario <path>        FIR scenario JSON path
  --target-delay-ms <float>    Delay target in milliseconds (default: 50.0)
  --sample-rates "list"        Space-delimited sample-rate list
  --block-size <int>           QA block size (default: 512)
  --channels <int>             QA channel count (default: 2)
  --profile-iterations <int>   QA profile iterations (default: 200)
  --profile-warmup <int>       QA profile warmup (default: 10)
  --skip-build                 Skip cmake build gate
  --skip-fir                   Skip FIR runtime profiling runs
  --help, -h                   Show usage

Outputs:
  status.tsv
  build.log
  highrate_matrix.tsv
  fir_profile.tsv
  failure_taxonomy.tsv
  docs_freshness.log
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires a value"
      OUT_DIR="$2"
      shift 2
      ;;
    --qa-bin)
      [[ $# -ge 2 ]] || usage_error "--qa-bin requires a value"
      QA_BIN="$2"
      shift 2
      ;;
    --fir-scenario)
      [[ $# -ge 2 ]] || usage_error "--fir-scenario requires a value"
      FIR_SCENARIO="$2"
      shift 2
      ;;
    --target-delay-ms)
      [[ $# -ge 2 ]] || usage_error "--target-delay-ms requires a value"
      TARGET_DELAY_MS="$2"
      shift 2
      ;;
    --sample-rates)
      [[ $# -ge 2 ]] || usage_error "--sample-rates requires a value"
      SAMPLE_RATES="$2"
      shift 2
      ;;
    --block-size)
      [[ $# -ge 2 ]] || usage_error "--block-size requires a value"
      BLOCK_SIZE="$2"
      shift 2
      ;;
    --channels)
      [[ $# -ge 2 ]] || usage_error "--channels requires a value"
      CHANNELS="$2"
      shift 2
      ;;
    --profile-iterations)
      [[ $# -ge 2 ]] || usage_error "--profile-iterations requires a value"
      PROFILE_ITERATIONS="$2"
      shift 2
      ;;
    --profile-warmup)
      [[ $# -ge 2 ]] || usage_error "--profile-warmup requires a value"
      PROFILE_WARMUP="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --skip-fir)
      SKIP_FIR=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      usage_error "unknown argument: $1"
      ;;
  esac
done

if ! [[ "$BLOCK_SIZE" =~ ^[0-9]+$ ]] || [[ "$BLOCK_SIZE" -lt 1 ]]; then
  usage_error "--block-size must be an integer >= 1"
fi
if ! [[ "$CHANNELS" =~ ^[0-9]+$ ]] || [[ "$CHANNELS" -lt 1 ]]; then
  usage_error "--channels must be an integer >= 1"
fi
if ! [[ "$PROFILE_ITERATIONS" =~ ^[0-9]+$ ]] || [[ "$PROFILE_ITERATIONS" -lt 1 ]]; then
  usage_error "--profile-iterations must be an integer >= 1"
fi
if ! [[ "$PROFILE_WARMUP" =~ ^[0-9]+$ ]] || [[ "$PROFILE_WARMUP" -lt 0 ]]; then
  usage_error "--profile-warmup must be an integer >= 0"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
BUILD_LOG="${OUT_DIR}/build.log"
HIGHRATE_MATRIX_TSV="${OUT_DIR}/highrate_matrix.tsv"
FIR_PROFILE_TSV="${OUT_DIR}/fir_profile.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"
DOCS_FRESHNESS_LOG="${OUT_DIR}/docs_freshness.log"
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"

printf "step\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "sample_rate_hz\ttarget_delay_ms\trequired_samples\tmax_buffer_samples\tmax_supported_samples\tmax_supported_delay_ms\theadroom_samples\tresult\n" > "$HIGHRATE_MATRIX_TSV"
printf "sample_rate_hz\texit_code\tscenario_status\tdeadline_status\tdeadline_value\tavg_block_status\tavg_block_ms\tp95_status\tp95_block_ms\tallocation_free_status\tallocation_free_value\ttotal_alloc_status\ttotal_alloc_value\tnon_finite_status\tnon_finite_value\trt60_status\trt60_value\trms_energy_status\trms_energy_value\tlog\n" > "$FIR_PROFILE_TSV"
printf "failure_id\tclassification\tcount\tdetail\tartifact\n" > "$FAILURE_TAXONOMY_TSV"
: > "$FAILURE_EVENTS_TSV"

FAIL_COUNT=0
WARN_COUNT=0

record_status() {
  local step="$1"
  local result="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "${step//$'\t'/ }" \
    "${result//$'\t'/ }" \
    "${detail//$'\t'/ }" \
    "${artifact//$'\t'/ }" \
    >> "$STATUS_TSV"
}

record_failure() {
  local failure_id="$1"
  local classification="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "${failure_id//$'\t'/ }" \
    "${classification//$'\t'/ }" \
    "${detail//$'\t'/ }" \
    "${artifact//$'\t'/ }" \
    >> "$FAILURE_EVENTS_TSV"
  ((FAIL_COUNT++)) || true
}

record_warn() {
  local step="$1"
  local detail="$2"
  local artifact="$3"
  record_status "$step" "WARN" "$detail" "$artifact"
  ((WARN_COUNT++)) || true
}

extract_metric_pair() {
  local metric="$1"
  local log_path="$2"
  awk -v metric="${metric}:" '
    $1 == metric {
      status = $2;
      value = $3;
      gsub(/^\(value=/, "", value);
      gsub(/\)$/, "", value);
      print status "\t" value;
      found = 1;
      exit;
    }
    END {
      if (!found)
        print "MISSING\tNA";
    }
  ' "$log_path"
}

record_status "init" "PASS" "ts=${TIMESTAMP};out_dir=${OUT_DIR}" "$OUT_DIR"

if [[ "$SKIP_BUILD" -eq 1 ]]; then
  record_warn "build" "skipped_by_flag" "$BUILD_LOG"
else
  if cmake --build "$ROOT_DIR/build_local" \
      --config Release \
      --target locusq_qa LocusQ_Standalone \
      -j 8 > "$BUILD_LOG" 2>&1; then
    record_status "build" "PASS" "cmake_release_targets_built" "$BUILD_LOG"
  else
    record_status "build" "FAIL" "cmake_build_failed" "$BUILD_LOG"
    record_failure "BL050-FX-001" "build_failure" "cmake_release_targets_failed" "$BUILD_LOG"
  fi
fi

SPATIAL_RENDERER_HDR="${ROOT_DIR}/Source/SpatialRenderer.h"
MAX_SPEAKER_DELAY_MS_HDR="$(awk '/static constexpr int MAX_SPEAKER_DELAY_MS/ {
  for (i = 1; i <= NF; ++i) {
    token = $i;
    gsub(/[^0-9]/, "", token);
    if (token ~ /^[0-9]+$/) {
      print token;
      exit;
    }
  }
}' "$SPATIAL_RENDERER_HDR")"
MAX_DELAY_RATE_HZ_HDR="$(awk '/static constexpr int MAX_DELAY_SAMPLE_RATE_HZ/ {
  for (i = 1; i <= NF; ++i) {
    token = $i;
    gsub(/[^0-9]/, "", token);
    if (token ~ /^[0-9]+$/) {
      print token;
      exit;
    }
  }
}' "$SPATIAL_RENDERER_HDR")"

if [[ -z "$MAX_SPEAKER_DELAY_MS_HDR" || -z "$MAX_DELAY_RATE_HZ_HDR" ]]; then
  record_status "delay_headroom_parse" "FAIL" "delay_constants_not_found" "$SPATIAL_RENDERER_HDR"
  record_failure "BL050-FX-002" "delay_headroom_parse_failure" "delay constants not parseable" "$SPATIAL_RENDERER_HDR"
  MAX_DELAY_SAMPLES=0
  MAX_SUPPORTED_SAMPLES=0
else
  MAX_DELAY_SAMPLES=$((((MAX_SPEAKER_DELAY_MS_HDR * MAX_DELAY_RATE_HZ_HDR) / 1000) + 1))
  MAX_SUPPORTED_SAMPLES=$((MAX_DELAY_SAMPLES - 1))
  record_status "delay_headroom_parse" "PASS" "max_delay_samples=${MAX_DELAY_SAMPLES};max_supported_samples=${MAX_SUPPORTED_SAMPLES};max_delay_ms=${MAX_SPEAKER_DELAY_MS_HDR};max_rate_hz=${MAX_DELAY_RATE_HZ_HDR}" "$SPATIAL_RENDERER_HDR"
fi

for rate in $SAMPLE_RATES; do
  if ! [[ "$rate" =~ ^[0-9]+$ ]] || [[ "$rate" -lt 1 ]]; then
    record_status "delay_headroom_${rate}" "FAIL" "invalid_sample_rate_token" "$HIGHRATE_MATRIX_TSV"
    record_failure "BL050-FX-003" "invalid_sample_rate" "sample_rate_token=${rate}" "$HIGHRATE_MATRIX_TSV"
    continue
  fi

  required_samples="$(awk -v sr="$rate" -v ms="$TARGET_DELAY_MS" 'BEGIN { printf "%d", int((sr * ms / 1000.0) + 1.0e-9) }')"
  max_supported_delay_ms="$(awk -v n="$MAX_SUPPORTED_SAMPLES" -v sr="$rate" 'BEGIN { printf "%.6f", (n * 1000.0) / sr }')"
  headroom_samples=$((MAX_SUPPORTED_SAMPLES - required_samples))

  row_result="PASS"
  if [[ "$headroom_samples" -lt 0 ]]; then
    row_result="FAIL"
    record_failure "BL050-FX-004" "delay_headroom_insufficient" "rate=${rate};required=${required_samples};available=${MAX_SUPPORTED_SAMPLES}" "$HIGHRATE_MATRIX_TSV"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$rate" \
    "$TARGET_DELAY_MS" \
    "$required_samples" \
    "$MAX_DELAY_SAMPLES" \
    "$MAX_SUPPORTED_SAMPLES" \
    "$max_supported_delay_ms" \
    "$headroom_samples" \
    "$row_result" \
    >> "$HIGHRATE_MATRIX_TSV"
done

if awk -F'\t' 'NR>1 && $8=="FAIL" { found=1; exit 0 } END { if (!found) exit 1 }' "$HIGHRATE_MATRIX_TSV"; then
  record_status "highrate_matrix" "FAIL" "one_or_more_sample_rates_exceeded_delay_headroom" "$HIGHRATE_MATRIX_TSV"
else
  record_status "highrate_matrix" "PASS" "all_sample_rates_within_delay_headroom" "$HIGHRATE_MATRIX_TSV"
fi

if [[ "$SKIP_FIR" -eq 1 ]]; then
  record_warn "fir_profile" "skipped_by_flag" "$FIR_PROFILE_TSV"
else
  if [[ ! -x "$QA_BIN" ]]; then
    record_status "fir_profile_prereq" "FAIL" "qa_bin_missing_or_not_executable" "$QA_BIN"
    record_failure "BL050-FX-005" "fir_profile_prereq" "qa_bin_missing" "$QA_BIN"
  elif [[ ! -f "$FIR_SCENARIO" ]]; then
    record_status "fir_profile_prereq" "FAIL" "fir_scenario_missing" "$FIR_SCENARIO"
    record_failure "BL050-FX-006" "fir_profile_prereq" "fir_scenario_missing" "$FIR_SCENARIO"
  else
    fir_run_failures=0
    fir_run_warnings=0

    for rate in $SAMPLE_RATES; do
      if ! [[ "$rate" =~ ^[0-9]+$ ]] || [[ "$rate" -lt 1 ]]; then
        continue
      fi

      run_log="${OUT_DIR}/fir_profile_${rate}.log"
      run_cmd=(
        "$QA_BIN"
        --spatial "$FIR_SCENARIO"
        --sample-rate "$rate"
        --block-size "$BLOCK_SIZE"
        --channels "$CHANNELS"
        --profile
        --profile-iterations "$PROFILE_ITERATIONS"
        --profile-warmup "$PROFILE_WARMUP"
      )

      exit_code=0
      if "${run_cmd[@]}" > "$run_log" 2>&1; then
        exit_code=0
      else
        exit_code=$?
      fi

      scenario_status="$(awk '/^Status:/ { print $2; exit }' "$run_log")"
      [[ -n "$scenario_status" ]] || scenario_status="UNKNOWN"

      IFS=$'\t' read -r deadline_status deadline_value < <(extract_metric_pair "perf_meets_deadline" "$run_log")
      IFS=$'\t' read -r avg_status avg_value < <(extract_metric_pair "perf_avg_block_time_ms" "$run_log")
      IFS=$'\t' read -r p95_status p95_value < <(extract_metric_pair "perf_p95_block_time_ms" "$run_log")
      IFS=$'\t' read -r alloc_free_status alloc_free_value < <(extract_metric_pair "perf_allocation_free" "$run_log")
      IFS=$'\t' read -r total_alloc_status total_alloc_value < <(extract_metric_pair "perf_total_allocations" "$run_log")
      IFS=$'\t' read -r non_finite_status non_finite_value < <(extract_metric_pair "non_finite" "$run_log")
      IFS=$'\t' read -r rt60_status rt60_value < <(extract_metric_pair "rt60" "$run_log")
      IFS=$'\t' read -r rms_energy_status rms_energy_value < <(extract_metric_pair "rms_energy" "$run_log")

      printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
        "$rate" \
        "$exit_code" \
        "$scenario_status" \
        "$deadline_status" \
        "$deadline_value" \
        "$avg_status" \
        "$avg_value" \
        "$p95_status" \
        "$p95_value" \
        "$alloc_free_status" \
        "$alloc_free_value" \
        "$total_alloc_status" \
        "$total_alloc_value" \
        "$non_finite_status" \
        "$non_finite_value" \
        "$rt60_status" \
        "$rt60_value" \
        "$rms_energy_status" \
        "$rms_energy_value" \
        "$run_log" \
        >> "$FIR_PROFILE_TSV"

      if [[ "$exit_code" -ne 0 ]]; then
        ((fir_run_failures++)) || true
        record_failure "BL050-FX-007" "fir_profile_run_failure" "rate=${rate};exit_code=${exit_code}" "$run_log"
      elif [[ "$scenario_status" == "WARN" ]]; then
        ((fir_run_warnings++)) || true
      fi
    done

    if [[ "$fir_run_failures" -gt 0 ]]; then
      record_status "fir_profile" "FAIL" "run_failures=${fir_run_failures};warnings=${fir_run_warnings}" "$FIR_PROFILE_TSV"
    elif [[ "$fir_run_warnings" -gt 0 ]]; then
      record_warn "fir_profile" "run_failures=0;warnings=${fir_run_warnings}" "$FIR_PROFILE_TSV"
    else
      record_status "fir_profile" "PASS" "run_failures=0;warnings=0" "$FIR_PROFILE_TSV"
    fi
  fi
fi

if [[ -x "${ROOT_DIR}/scripts/validate-docs-freshness.sh" ]]; then
  if "${ROOT_DIR}/scripts/validate-docs-freshness.sh" > "$DOCS_FRESHNESS_LOG" 2>&1; then
    record_status "docs_freshness" "PASS" "docs_freshness_gate_passed" "$DOCS_FRESHNESS_LOG"
  else
    record_status "docs_freshness" "FAIL" "docs_freshness_gate_failed" "$DOCS_FRESHNESS_LOG"
    record_failure "BL050-FX-008" "docs_freshness_failure" "validate-docs-freshness.sh failed" "$DOCS_FRESHNESS_LOG"
  fi
else
  record_status "docs_freshness" "FAIL" "validator_missing_or_not_executable" "${ROOT_DIR}/scripts/validate-docs-freshness.sh"
  record_failure "BL050-FX-009" "docs_freshness_prereq" "validate-docs-freshness.sh missing" "${ROOT_DIR}/scripts/validate-docs-freshness.sh"
fi

if [[ -s "$FAILURE_EVENTS_TSV" ]]; then
  awk -F'\t' '
    {
      key = $1 FS $2 FS $4;
      count[key]++;
      detail[key] = $3;
    }
    END {
      for (k in count) {
        split(k, parts, FS);
        printf "%s\t%s\t%d\t%s\t%s\n", parts[1], parts[2], count[k], detail[k], parts[3];
      }
    }
  ' "$FAILURE_EVENTS_TSV" | sort -t$'\t' -k1,1 -k2,2 >> "$FAILURE_TAXONOMY_TSV"
else
  printf "none\tnone\t0\tno_failures\t-\n" >> "$FAILURE_TAXONOMY_TSV"
fi

if [[ "$FAIL_COUNT" -gt 0 ]]; then
  record_status "lane_result" "FAIL" "failures=${FAIL_COUNT};warnings=${WARN_COUNT}" "$STATUS_TSV"
  echo "Artifacts:"
  echo "- $STATUS_TSV"
  echo "- $BUILD_LOG"
  echo "- $HIGHRATE_MATRIX_TSV"
  echo "- $FIR_PROFILE_TSV"
  echo "- $FAILURE_TAXONOMY_TSV"
  echo "- $DOCS_FRESHNESS_LOG"
  rm -f "$FAILURE_EVENTS_TSV"
  exit 1
fi

record_status "lane_result" "PASS" "failures=0;warnings=${WARN_COUNT}" "$STATUS_TSV"
echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $BUILD_LOG"
echo "- $HIGHRATE_MATRIX_TSV"
echo "- $FIR_PROFILE_TSV"
echo "- $FAILURE_TAXONOMY_TSV"
echo "- $DOCS_FRESHNESS_LOG"
rm -f "$FAILURE_EVENTS_TSV"
exit 0
