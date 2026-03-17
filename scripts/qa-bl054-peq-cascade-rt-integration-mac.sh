#!/usr/bin/env bash
# Title: BL-054 PEQ Cascade RT Integration QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-07
# Last Modified Date: 2026-03-07
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl054_peq_cascade_rt_integration_${TIMESTAMP}_$$"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
RT_SWAP_CONTRACT_TSV=""
BYPASS_IDENTITY_CONTRACT_TSV=""
MONITOR_CHAIN_ORDER_TSV=""
SUMMARY_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl054-peq-cascade-rt-integration-mac.sh [options]

BL-054 PEQ cascade RT integration lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks
  --help, -h         Show usage

Outputs:
  status.tsv
  rt_swap_contract.tsv
  bypass_identity_contract.tsv
  monitor_chain_order.tsv
  summary.md
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

write_summary_md() {
  local lane_result="$1"

  cat > "$SUMMARY_MD" <<SUMMARY
Title: BL-054 PEQ Cascade RT Integration Evidence Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-054 PEQ Cascade RT Integration Lane Summary

- Mode: \`${MODE}\`
- Output directory: \`${OUT_DIR}\`
- Lane result: \`${lane_result}\`
- PASS rows in \`status.tsv\`: ${pass_count}
- FAIL rows in \`status.tsv\`: ${fail_count}

## Artifacts

- \`status.tsv\`
- \`rt_swap_contract.tsv\`
- \`bypass_identity_contract.tsv\`
- \`monitor_chain_order.tsv\`
- \`summary.md\` (this file)
SUMMARY
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
RT_SWAP_CONTRACT_TSV="${OUT_DIR}/rt_swap_contract.tsv"
BYPASS_IDENTITY_CONTRACT_TSV="${OUT_DIR}/bypass_identity_contract.tsv"
MONITOR_CHAIN_ORDER_TSV="${OUT_DIR}/monitor_chain_order.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$RT_SWAP_CONTRACT_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$BYPASS_IDENTITY_CONTRACT_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$MONITOR_CHAIN_ORDER_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-054-peq-cascade-rt-integration.md"
PEQ_HOOK_HDR="${ROOT_DIR}/Source/headphone_dsp/HeadphonePeqHook.h"
CALIBRATION_CHAIN_HDR="${ROOT_DIR}/Source/headphone_dsp/HeadphoneCalibrationChain.h"
SPATIAL_PROFILE_IMPL="${ROOT_DIR}/Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp"
OUTPUT_ROUTING_CPP="${ROOT_DIR}/Source/spatial_renderer/SpatialOutputRoutingStage.cpp"
PROCESSOR_CALIBRATION_CPP="${ROOT_DIR}/Source/processor_core/ProcessorCalibrationBridge.cpp"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL054-C0-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL054-C0-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

rt_swap_detail=""
if rg -Fq 'double-buffered coefficient banks with atomic active-bank swap.' "$PEQ_HOOK_HDR"; then
  rt_swap_detail+="atomic_swap_contract_comment;"
else
  rt_swap_detail+="missing_atomic_swap_contract_comment;"
fi
if rg -Fq 'std::array<Bank, 2> coefficientBanks {};' "$PEQ_HOOK_HDR" \
   && rg -Fq 'std::atomic<int> activeBankIndex { 0 };' "$PEQ_HOOK_HDR"; then
  rt_swap_detail+="double_buffer_bank_storage;"
else
  rt_swap_detail+="missing_double_buffer_bank_storage;"
fi
if rg -Fq 'activeBankIndex.store (inactiveBankIndex, std::memory_order_release);' "$PEQ_HOOK_HDR" \
   && rg -Fq 'activeBankIndex.load (std::memory_order_acquire)' "$PEQ_HOOK_HDR"; then
  rt_swap_detail+="release_acquire_bank_publish;"
else
  rt_swap_detail+="missing_release_acquire_bank_publish;"
fi
if rg -Fq 'void applyPeqPreset (const HeadphonePeqHook::Preset& preset) noexcept' "$CALIBRATION_CHAIN_HDR" \
   && ! rg -Fq 'setPeqPreampDb' "$CALIBRATION_CHAIN_HDR" \
   && ! rg -Fq 'setPeqStage' "$CALIBRATION_CHAIN_HDR"; then
  rt_swap_detail+="single_call_chain_apply_api;"
else
  rt_swap_detail+="missing_single_call_chain_apply_api;"
fi
if rg -Fq 'headphoneCalibrationChain.applyPeqPreset (buildBundledPeqPreset (preset, sampleRate));' "$SPATIAL_PROFILE_IMPL" \
   && rg -Fq 'headphoneCalibrationChain.applyPeqPreset (buildJsonPeqPreset (bandsArray, preampDb, sampleRate));' "$SPATIAL_PROFILE_IMPL"; then
  rt_swap_detail+="message_thread_preset_build_and_publish;"
else
  rt_swap_detail+="missing_message_thread_preset_build_and_publish;"
fi

if [[ "$rt_swap_detail" == *"missing_"* ]]; then
  printf "atomic_peq_swap_contract\tFAIL\t%s\t%s\n" "$rt_swap_detail" "$PEQ_HOOK_HDR" >> "$RT_SWAP_CONTRACT_TSV"
  record "BL054-C1-atomic_peq_swap_contract" "FAIL" "$rt_swap_detail" "$RT_SWAP_CONTRACT_TSV"
else
  printf "atomic_peq_swap_contract\tPASS\t%s\t%s\n" "$rt_swap_detail" "$PEQ_HOOK_HDR" >> "$RT_SWAP_CONTRACT_TSV"
  record "BL054-C1-atomic_peq_swap_contract" "PASS" "$rt_swap_detail" "$RT_SWAP_CONTRACT_TSV"
fi

calibration_bridge_detail=""
if rg -Fq 'spatialRenderer.applyJsonPeqBands (bandsVar, 0.0f, currentSampleRate);' "$PROCESSOR_CALIBRATION_CPP" \
   && rg -Fq 'spatialRenderer.setHeadphoneCalibrationEngine (1);' "$PROCESSOR_CALIBRATION_CPP" \
   && rg -Fq 'spatialRenderer.setHeadphoneCalibrationEnabled (true);' "$PROCESSOR_CALIBRATION_CPP"; then
  calibration_bridge_detail+="calibration_profile_json_to_peq_engine;"
else
  calibration_bridge_detail+="missing_calibration_profile_json_to_peq_engine;"
fi

if [[ "$calibration_bridge_detail" == *"missing_"* ]]; then
  printf "calibration_profile_bridge\tFAIL\t%s\t%s\n" "$calibration_bridge_detail" "$PROCESSOR_CALIBRATION_CPP" >> "$RT_SWAP_CONTRACT_TSV"
  record "BL054-C2-calibration_profile_bridge" "FAIL" "$calibration_bridge_detail" "$RT_SWAP_CONTRACT_TSV"
else
  printf "calibration_profile_bridge\tPASS\t%s\t%s\n" "$calibration_bridge_detail" "$PROCESSOR_CALIBRATION_CPP" >> "$RT_SWAP_CONTRACT_TSV"
  record "BL054-C2-calibration_profile_bridge" "PASS" "$calibration_bridge_detail" "$RT_SWAP_CONTRACT_TSV"
fi

bypass_detail=""
if rg -Fq 'if (! ready || bypassed)' "$PEQ_HOOK_HDR"; then
  bypass_detail+="bypass_short_circuit;"
else
  bypass_detail+="missing_bypass_short_circuit;"
fi
if rg -Fq 'publishPreset (makeIdentityPreset());' "$PEQ_HOOK_HDR"; then
  bypass_detail+="identity_preset_clear_path;"
else
  bypass_detail+="missing_identity_preset_clear_path;"
fi
if ! rg -q '\b(new|malloc|realloc|calloc)\b' "$PEQ_HOOK_HDR" \
   && ! rg -q 'std::mutex|std::lock_guard|std::scoped_lock|SpinLock::ScopedLockType' "$PEQ_HOOK_HDR" \
   && ! rg -q 'juce::File|std::ifstream|std::ofstream|fopen|fread|fwrite' "$PEQ_HOOK_HDR"; then
  bypass_detail+="rt_safe_no_heap_lock_io;"
else
  bypass_detail+="missing_rt_safe_no_heap_lock_io;"
fi

if [[ "$bypass_detail" == *"missing_"* ]]; then
  printf "bypass_identity_contract\tFAIL\t%s\t%s\n" "$bypass_detail" "$PEQ_HOOK_HDR" >> "$BYPASS_IDENTITY_CONTRACT_TSV"
  record "BL054-C3-bypass_identity_contract" "FAIL" "$bypass_detail" "$BYPASS_IDENTITY_CONTRACT_TSV"
else
  printf "bypass_identity_contract\tPASS\t%s\t%s\n" "$bypass_detail" "$PEQ_HOOK_HDR" >> "$BYPASS_IDENTITY_CONTRACT_TSV"
  record "BL054-C3-bypass_identity_contract" "PASS" "$bypass_detail" "$BYPASS_IDENTITY_CONTRACT_TSV"
fi

apply_comp_line="$(rg -n 'applyHeadphoneProfileCompensation \(stereo\.left, stereo\.right\);' "$OUTPUT_ROUTING_CPP" | awk -F: 'NR==1 { print $1 }')"
peq_line="$(rg -n 'headphoneCalibrationChain\.processStereoSample \(stereo\.left, stereo\.right\);' "$OUTPUT_ROUTING_CPP" | awk -F: 'NR==1 { print $1 }')"

order_detail=""
if [[ -n "$apply_comp_line" && -n "$peq_line" ]]; then
  if (( apply_comp_line < peq_line )); then
    order_detail+="post_render_profile_comp_then_peq;"
  else
    order_detail+="missing_post_render_profile_comp_then_peq;"
  fi
else
  order_detail+="missing_post_render_profile_comp_then_peq;"
fi

if [[ "$order_detail" == *"missing_"* ]]; then
  printf "monitor_chain_order\tFAIL\t%s\t%s\n" "$order_detail" "$OUTPUT_ROUTING_CPP" >> "$MONITOR_CHAIN_ORDER_TSV"
  record "BL054-C4-monitor_chain_order" "FAIL" "$order_detail" "$MONITOR_CHAIN_ORDER_TSV"
else
  printf "monitor_chain_order\tPASS\t%s\t%s\n" "$order_detail" "$OUTPUT_ROUTING_CPP" >> "$MONITOR_CHAIN_ORDER_TSV"
  record "BL054-C4-monitor_chain_order" "PASS" "$order_detail" "$MONITOR_CHAIN_ORDER_TSV"
fi

if [[ "$MODE" == "execute" ]]; then
  record "BL054-E1-execute_contract_surface" "PASS" "execute mode uses live structural contract checks with zero TODO rows" "$STATUS_TSV"
else
  record "BL054-C5-contract_mode" "PASS" "contract-only mode complete; execute mode available for identical structural checks" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl054_lane_pass" "$STATUS_TSV"
  write_summary_md "PASS"
  exit 0
fi

record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
write_summary_md "FAIL"
exit 1
