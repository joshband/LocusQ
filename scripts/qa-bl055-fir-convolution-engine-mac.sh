#!/usr/bin/env bash
# Title: BL-055 FIR Convolution Engine QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-02
# Last Modified Date: 2026-03-02
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl055_fir_convolution_engine_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
LATENCY_CONTRACT_TSV=""
SWAP_CROSSFADE_CHECK_TSV=""
OFFLINE_PARITY_SUMMARY_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl055-fir-convolution-engine-mac.sh [options]

BL-055 FIR convolution engine lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while TODO rows remain)
  --help, -h         Show usage

Outputs:
  status.tsv
  latency_contract.tsv
  swap_crossfade_check.tsv
  offline_parity_summary.md
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

record() {
  local check_id="$1"
  local result="$2"
  local detail="$3"
  local artifact="${4:-}"

  printf "%s\t%s\t%s\t%s\n" \
    "$check_id" \
    "$result" \
    "${detail//$'\t'/ }" \
    "${artifact//$'\t'/ }" \
    >> "$STATUS_TSV"

  if [[ "$result" == "PASS" ]]; then
    ((pass_count++)) || true
  else
    ((fail_count++)) || true
  fi
}

count_todo_rows() {
  local file="$1"
  [[ -f "$file" ]] || {
    echo 0
    return
  }

  awk -F'\t' '
    NR == 1 { next }
    {
      for (i = 1; i <= NF; ++i)
      {
        if ($i == "TODO")
        {
          count++
          break
        }
      }
    }
    END { print count + 0 }
  ' "$file"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires a value"
      OUT_DIR="$2"
      shift 2
      ;;
    --contract-only)
      if (( MODE_SET == 1 )) && [[ "$MODE" != "contract_only" ]]; then
        usage_error "--contract-only cannot be combined with --execute"
      fi
      MODE="contract_only"
      MODE_SET=1
      shift
      ;;
    --execute)
      if (( MODE_SET == 1 )) && [[ "$MODE" != "execute" ]]; then
        usage_error "--execute cannot be combined with --contract-only"
      fi
      MODE="execute"
      MODE_SET=1
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

command -v rg >/dev/null 2>&1 || usage_error "ripgrep (rg) is required"

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
LATENCY_CONTRACT_TSV="${OUT_DIR}/latency_contract.tsv"
SWAP_CROSSFADE_CHECK_TSV="${OUT_DIR}/swap_crossfade_check.tsv"
OFFLINE_PARITY_SUMMARY_MD="${OUT_DIR}/offline_parity_summary.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$LATENCY_CONTRACT_TSV"
printf "scenario\tresult\tdetail\tartifact\n" > "$SWAP_CROSSFADE_CHECK_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-055-fir-convolution-engine.md"
PROCESSOR_CPP="${ROOT_DIR}/Source/PluginProcessor.cpp"
SPATIAL_RENDERER_HDR="${ROOT_DIR}/Source/SpatialRenderer.h"
CALIBRATION_CHAIN_HDR="${ROOT_DIR}/Source/headphone_dsp/HeadphoneCalibrationChain.h"
FIR_HOOK_HDR="${ROOT_DIR}/Source/headphone_dsp/HeadphoneFirHook.h"
HEADPHONE_VERIFICATION_CONTRACT_HDR="${ROOT_DIR}/Source/shared_contracts/HeadphoneVerificationContract.h"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL055-C0-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL055-C0-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if rg -Fq 'return headphoneCalibrationChain.getActiveLatencySamples();' "$SPATIAL_RENDERER_HDR"; then
  printf "calibration_latency_surface\tPASS\tSpatialRenderer reports active calibration latency samples\t%s\n" "$SPATIAL_RENDERER_HDR" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C1-calibration_latency_surface" "PASS" \
    "SpatialRenderer latency surface present" "$SPATIAL_RENDERER_HDR"
else
  printf "calibration_latency_surface\tFAIL\tmissing active calibration latency getter path\t%s\n" "$SPATIAL_RENDERER_HDR" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C1-calibration_latency_surface" "FAIL" \
    "SpatialRenderer latency surface missing" "$SPATIAL_RENDERER_HDR"
fi

if rg -Fq 'const int calLatency = spatialRenderer.getCalibrationLatencySamples();' "$PROCESSOR_CPP" \
   && rg -Fq 'setLatencySamples (calLatency);' "$PROCESSOR_CPP"; then
  printf "host_latency_publication\tPASS\tPluginProcessor publishes calibration latency to host via setLatencySamples\t%s\n" "$PROCESSOR_CPP" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C2-host_latency_publication" "PASS" \
    "setLatencySamples(calLatency) publication path present" "$PROCESSOR_CPP"
else
  printf "host_latency_publication\tFAIL\tmissing setLatencySamples(calLatency) publication markers\t%s\n" "$PROCESSOR_CPP" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C2-host_latency_publication" "FAIL" \
    "setLatencySamples(calLatency) publication markers missing" "$PROCESSOR_CPP"
fi

if rg -Fq 'configuredTapCount = 1;' "$FIR_HOOK_HDR" \
   && rg -Fq 'return juce::jmax (0, clampedTapCount - 1);' "$FIR_HOOK_HDR"; then
  printf "direct_path_identity_latency_zero\tPASS\tidentity FIR path resolves to zero latency samples\t%s\n" "$FIR_HOOK_HDR" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C3-direct_identity_latency_zero" "PASS" \
    "identity FIR latency contract markers present" "$FIR_HOOK_HDR"
else
  printf "direct_path_identity_latency_zero\tFAIL\tidentity FIR zero-latency markers missing\t%s\n" "$FIR_HOOK_HDR" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C3-direct_identity_latency_zero" "FAIL" \
    "identity FIR zero-latency markers missing" "$FIR_HOOK_HDR"
fi

if rg -q 'DirectFirConvolver|PartitionedFftConvolver|FirEngineManager' \
      "$CALIBRATION_CHAIN_HDR" "$FIR_HOOK_HDR" "$SPATIAL_RENDERER_HDR" "$PROCESSOR_CPP" \
   && rg -q 'nextPow2|nextPowerOfTwo|juce::nextPowerOfTwo' \
      "$CALIBRATION_CHAIN_HDR" "$FIR_HOOK_HDR" "$SPATIAL_RENDERER_HDR" "$PROCESSOR_CPP"; then
  printf "partitioned_latency_next_pow2_contract\tPASS\tdirect/partitioned engine markers with nextPow2 latency contract detected\t%s\n" "$FIR_HOOK_HDR" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C4-partitioned_latency_next_pow2_contract" "PASS" \
    "direct/partitioned nextPow2 latency markers present" "$FIR_HOOK_HDR"
else
  printf "partitioned_latency_next_pow2_contract\tFAIL\tdirect/partitioned engine markers or nextPow2 latency contract missing\t%s\n" "$FIR_HOOK_HDR" \
    >> "$LATENCY_CONTRACT_TSV"
  record "BL055-C4-partitioned_latency_next_pow2_contract" "FAIL" \
    "direct/partitioned nextPow2 latency markers missing" "$FIR_HOOK_HDR"
fi

if rg -Fq 'headphoneCalibrationChain.setRequestedEngineIndex (' "$SPATIAL_RENDERER_HDR" \
   && rg -Fq 'activeHeadphoneCalibrationEngineIndex.store (' "$SPATIAL_RENDERER_HDR" \
   && rg -Fq 'activeHeadphoneCalibrationFallbackReasonIndex.store (' "$SPATIAL_RENDERER_HDR"; then
  printf "engine_state_swap_publication\tPASS\trequested/active/fallback engine state publication markers present\t%s\n" "$SPATIAL_RENDERER_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C5-engine_state_swap_publication" "PASS" \
    "engine swap publication markers present" "$SPATIAL_RENDERER_HDR"
else
  printf "engine_state_swap_publication\tFAIL\trequested/active/fallback engine state publication markers missing\t%s\n" "$SPATIAL_RENDERER_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C5-engine_state_swap_publication" "FAIL" \
    "engine swap publication markers missing" "$SPATIAL_RENDERER_HDR"
fi

if rg -qi 'crossfade|blend' "$CALIBRATION_CHAIN_HDR" "$FIR_HOOK_HDR"; then
  printf "swap_crossfade_structure\tPASS\tcalibration FIR/engine swap crossfade markers present\t%s\n" "$CALIBRATION_CHAIN_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C6-swap_crossfade_structure" "PASS" \
    "crossfade/blend markers present in calibration chain path" "$CALIBRATION_CHAIN_HDR"
else
  printf "swap_crossfade_structure\tFAIL\tno crossfade/blend markers in calibration FIR/engine swap path\t%s\n" "$CALIBRATION_CHAIN_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C6-swap_crossfade_structure" "FAIL" \
    "crossfade/blend markers missing in calibration chain path" "$CALIBRATION_CHAIN_HDR"
fi

if ! rg -q '\b(new|malloc|realloc|calloc)\b' "$CALIBRATION_CHAIN_HDR" "$FIR_HOOK_HDR" \
   && ! rg -q 'std::mutex|std::lock_guard|std::scoped_lock|SpinLock::ScopedLockType' "$CALIBRATION_CHAIN_HDR" "$FIR_HOOK_HDR" \
   && ! rg -q 'juce::File|std::ifstream|std::ofstream|fopen|fread|fwrite' "$CALIBRATION_CHAIN_HDR" "$FIR_HOOK_HDR"; then
  printf "rt_safety_no_alloc_lock_io\tPASS\tno heap/lock/blocking-I/O markers in calibration FIR apply path headers\t%s\n" "$FIR_HOOK_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C7-rt_safety_no_alloc_lock_io" "PASS" \
    "no heap/lock/blocking-I/O markers in calibration FIR apply path headers" "$FIR_HOOK_HDR"
else
  printf "rt_safety_no_alloc_lock_io\tFAIL\theap/lock/blocking-I/O markers found in calibration FIR apply path headers\t%s\n" "$FIR_HOOK_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C7-rt_safety_no_alloc_lock_io" "FAIL" \
    "heap/lock/blocking-I/O markers found in calibration FIR apply path headers" "$FIR_HOOK_HDR"
fi

if rg -Fq 'kRequestedEngineId' "$HEADPHONE_VERIFICATION_CONTRACT_HDR" \
   && rg -Fq 'kActiveEngineId' "$HEADPHONE_VERIFICATION_CONTRACT_HDR" \
   && rg -Fq 'kLatencySamples' "$HEADPHONE_VERIFICATION_CONTRACT_HDR"; then
  printf "offline_parity_contract_fields\tPASS\toffline parity reporting fields for requested/active/latency are defined\t%s\n" "$HEADPHONE_VERIFICATION_CONTRACT_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C8-offline_parity_contract_fields" "PASS" \
    "verification contract fields for requested/active engine + latency present" \
    "$HEADPHONE_VERIFICATION_CONTRACT_HDR"
else
  printf "offline_parity_contract_fields\tFAIL\tverification contract fields for requested/active engine + latency missing\t%s\n" "$HEADPHONE_VERIFICATION_CONTRACT_HDR" \
    >> "$SWAP_CROSSFADE_CHECK_TSV"
  record "BL055-C8-offline_parity_contract_fields" "FAIL" \
    "verification contract fields for requested/active engine + latency missing" \
    "$HEADPHONE_VERIFICATION_CONTRACT_HDR"
fi

todo_rows="$((
  $(count_todo_rows "$LATENCY_CONTRACT_TSV")
  + $(count_todo_rows "$SWAP_CROSSFADE_CHECK_TSV")
))"

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL055-E1-execute_todo_rows" "FAIL" \
      "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL055-E1-execute_todo_rows" "PASS" \
      "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL055-C9-contract_mode" "PASS" \
    "contract-only mode completed structural probes (todo_rows=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl055_lane_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

lane_result_value="$(awk -F'\t' '$1=="lane_result"{value=$2} END{print value}' "$STATUS_TSV")"
status_pass_rows="$(awk -F'\t' 'NR>1 && $2=="PASS"{count++} END{print count+0}' "$STATUS_TSV")"
status_fail_rows="$(awk -F'\t' 'NR>1 && $2=="FAIL"{count++} END{print count+0}' "$STATUS_TSV")"

cat > "$OFFLINE_PARITY_SUMMARY_MD" <<EOF_SUMMARY
Title: BL-055 Offline Parity Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-055 FIR Convolution Engine Offline Parity Summary

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- lane_result: ${lane_result_value}
- status_pass_rows: ${status_pass_rows}
- status_fail_rows: ${status_fail_rows}
- todo_rows_detected: ${todo_rows}

## Evidence Fields

- status.tsv: check_id, result, detail, artifact
- latency_contract.tsv: check, result, detail, artifact
- swap_crossfade_check.tsv: scenario, result, detail, artifact

## Offline Parity Anchors

- backlog_runbook: Documentation/backlog/bl-055-fir-convolution-engine.md
- verification_contract: Source/shared_contracts/HeadphoneVerificationContract.h
- host_latency_publication: Source/PluginProcessor.cpp
- calibration_chain: Source/headphone_dsp/HeadphoneCalibrationChain.h
- fir_hook: Source/headphone_dsp/HeadphoneFirHook.h
EOF_SUMMARY

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $LATENCY_CONTRACT_TSV"
echo "- $SWAP_CROSSFADE_CHECK_TSV"
echo "- $OFFLINE_PARITY_SUMMARY_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
