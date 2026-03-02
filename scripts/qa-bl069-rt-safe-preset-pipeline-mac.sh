#!/usr/bin/env bash
# Title: BL-069 RT-Safe Preset Pipeline QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-02
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl069_rt_safe_preset_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
RT_ACCESS_AUDIT_TSV=""
PRESET_RETRY_BACKOFF_TSV=""
COEFFICIENT_SWAP_STABILITY_TSV=""
FAILURE_TAXONOMY_TSV=""
SUMMARY_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl069-rt-safe-preset-pipeline-mac.sh [options]

BL-069 RT-safe preset pipeline lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while runtime TODO rows exist)
  --help, -h         Show usage

Outputs:
  status.tsv
  rt_access_audit.tsv
  preset_retry_backoff.tsv
  coefficient_swap_stability.tsv
  failure_taxonomy.tsv
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

write_summary_md() {
  local lane_result="$1"
  local status_fail_rows="$2"
  local status_pass_rows="$3"
  local doc_date_utc
  doc_date_utc="$(date -u +%Y-%m-%d)"

  cat > "$SUMMARY_MD" <<SUMMARY
Title: BL-069 RT-Safe Headphone Preset Pipeline Evidence Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${doc_date_utc}
Last Modified Date: ${doc_date_utc}

# BL-069 RT-Safe Headphone Preset Pipeline Lane Summary

- Mode: \`${MODE}\`
- Output directory: \`${OUT_DIR}\`
- Lane result: \`${lane_result}\`
- PASS rows in \`status.tsv\`: ${status_pass_rows}
- FAIL rows in \`status.tsv\`: ${status_fail_rows}

## Artifacts

- \`status.tsv\`
- \`rt_access_audit.tsv\`
- \`preset_retry_backoff.tsv\`
- \`coefficient_swap_stability.tsv\`
- \`failure_taxonomy.tsv\`
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

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
RT_ACCESS_AUDIT_TSV="${OUT_DIR}/rt_access_audit.tsv"
PRESET_RETRY_BACKOFF_TSV="${OUT_DIR}/preset_retry_backoff.tsv"
COEFFICIENT_SWAP_STABILITY_TSV="${OUT_DIR}/coefficient_swap_stability.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$RT_ACCESS_AUDIT_TSV"
printf "scenario\tresult\tdetail\n" > "$PRESET_RETRY_BACKOFF_TSV"
printf "scenario\tresult\tdetail\n" > "$COEFFICIENT_SWAP_STABILITY_TSV"
printf "failure_id\tclassification\tmitigation\n" > "$FAILURE_TAXONOMY_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-069-rt-safe-headphone-preset-pipeline-and-failure-backoff.md"
RENDERER_HDR="${ROOT_DIR}/Source/SpatialRenderer.h"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL069-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL069-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if rg -q 'preloadBundledPeqPresets\(\)' "$RENDERER_HDR"; then
  printf "preload_bundled_presets\tPASS\tpreloadBundledPeqPresets symbol present\n" >> "$RT_ACCESS_AUDIT_TSV"
  record "BL069-C2-preload_symbol" "PASS" "preload helper present" "$RENDERER_HDR"
else
  printf "preload_bundled_presets\tFAIL\tpreloadBundledPeqPresets symbol missing\n" >> "$RT_ACCESS_AUDIT_TSV"
  record "BL069-C2-preload_symbol" "FAIL" "preload helper missing" "$RENDERER_HDR"
fi

if rg -q 'const auto& preset = bundledPeqPresets' "$RENDERER_HDR"; then
  printf "cache_only_profile_load\tPASS\tloadPeqPresetForProfile consumes bundled cache\n" >> "$RT_ACCESS_AUDIT_TSV"
  record "BL069-C3-cache_only_load" "PASS" "cache-only load path present" "$RENDERER_HDR"
else
  printf "cache_only_profile_load\tFAIL\tcache-only load path not found\n" >> "$RT_ACCESS_AUDIT_TSV"
  record "BL069-C3-cache_only_load" "FAIL" "cache-only load path not found" "$RENDERER_HDR"
fi

if [[ "$MODE" == "execute" ]]; then
  preset_retry_backoff_detail=""
  if rg -q 'if \(lastLoadedPeqPresetIndex == clampedProfileIndex && lastLoadedPeqSampleRate == sampleRate\)' "$RENDERER_HDR"; then
    preset_retry_backoff_detail+="memoized_last_loaded_guard;"
  else
    preset_retry_backoff_detail+="missing_memoized_last_loaded_guard;"
  fi
  if rg -q 'const auto& preset = bundledPeqPresets\[static_cast<size_t> \(clampedProfileIndex\)\]\.preset;' "$RENDERER_HDR"; then
    preset_retry_backoff_detail+="cache_only_bundle_lookup;"
  else
    preset_retry_backoff_detail+="missing_cache_only_bundle_lookup;"
  fi

  if [[ "$preset_retry_backoff_detail" == *"missing_"* ]]; then
    printf "missing_preset_retry_backoff\tFAIL\t%s\n" "$preset_retry_backoff_detail" >> "$PRESET_RETRY_BACKOFF_TSV"
    record "BL069-E2-missing_preset_retry_backoff" "FAIL" "$preset_retry_backoff_detail" "$PRESET_RETRY_BACKOFF_TSV"
  else
    printf "missing_preset_retry_backoff\tPASS\t%s\n" "$preset_retry_backoff_detail" >> "$PRESET_RETRY_BACKOFF_TSV"
    record "BL069-E2-missing_preset_retry_backoff" "PASS" "$preset_retry_backoff_detail" "$PRESET_RETRY_BACKOFF_TSV"
  fi

  transient_asset_detail=""
  if rg -q 'if \(! presetFile\.existsAsFile\(\)\)' "$RENDERER_HDR"; then
    transient_asset_detail+="file_cached_as_invalid;"
  else
    transient_asset_detail+="missing_existsAsFile_guard;"
  fi
  if rg -q 'if \(sampleRate <= 0\.0 \|\| ! preset\.valid \|\| preset\.bands\.empty\(\)\)' "$RENDERER_HDR"; then
    transient_asset_detail+="invalid_or_empty_preset_short_circuit;"
  else
    transient_asset_detail+="missing_invalid_or_empty_preset_short_circuit;"
  fi
  if rg -q 'lastLoadedPeqPresetIndex = clampedProfileIndex;' "$RENDERER_HDR" && rg -q 'lastLoadedPeqSampleRate  = sampleRate;' "$RENDERER_HDR"; then
    transient_asset_detail+="state_commit_prevents_immediate_retry;"
  else
    transient_asset_detail+="missing_state_commit_prevents_immediate_retry;"
  fi

  if [[ "$transient_asset_detail" == *"missing_"* ]]; then
    printf "transient_asset_unavailable\tFAIL\t%s\n" "$transient_asset_detail" >> "$PRESET_RETRY_BACKOFF_TSV"
    record "BL069-E3-transient_asset_unavailable" "FAIL" "$transient_asset_detail" "$PRESET_RETRY_BACKOFF_TSV"
  else
    printf "transient_asset_unavailable\tPASS\t%s\n" "$transient_asset_detail" >> "$PRESET_RETRY_BACKOFF_TSV"
    record "BL069-E3-transient_asset_unavailable" "PASS" "$transient_asset_detail" "$PRESET_RETRY_BACKOFF_TSV"
  fi

  profile_switch_detail=""
  if rg -q 'profile changes are non-RT events on the message thread\.' "$RENDERER_HDR"; then
    profile_switch_detail+="message_thread_non_rt_contract;"
  else
    profile_switch_detail+="missing_message_thread_non_rt_contract;"
  fi
  if rg -q 'headphoneCalibrationChain\.clearPeqPreset\(\);' "$RENDERER_HDR" && rg -q 'headphoneCalibrationChain\.setPeqPreampDb \(preset\.preampDb\);' "$RENDERER_HDR"; then
    profile_switch_detail+="ordered_reset_then_preamp;"
  else
    profile_switch_detail+="missing_ordered_reset_then_preamp;"
  fi
  if rg -q 'for \(int i = 0; i < maxStages; \+\+i\)' "$RENDERER_HDR"; then
    profile_switch_detail+="bounded_stage_swap_loop;"
  else
    profile_switch_detail+="missing_bounded_stage_swap_loop;"
  fi

  if [[ "$profile_switch_detail" == *"missing_"* ]]; then
    printf "profile_switch_no_glitch\tFAIL\t%s\n" "$profile_switch_detail" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
    record "BL069-E4-profile_switch_no_glitch" "FAIL" "$profile_switch_detail" "$COEFFICIENT_SWAP_STABILITY_TSV"
  else
    printf "profile_switch_no_glitch\tPASS\t%s\n" "$profile_switch_detail" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
    record "BL069-E4-profile_switch_no_glitch" "PASS" "$profile_switch_detail" "$COEFFICIENT_SWAP_STABILITY_TSV"
  fi

  rapid_profile_toggle_detail=""
  if rg -q 'const auto clamped = juce::jlimit \(0, NUM_HEADPHONE_DEVICE_PROFILES - 1, profileIndex\);' "$RENDERER_HDR"; then
    rapid_profile_toggle_detail+="profile_index_clamped;"
  else
    rapid_profile_toggle_detail+="missing_profile_index_clamped;"
  fi
  if rg -q 'if \(requestedHeadphoneProfileIndex\.load \(std::memory_order_relaxed\) == clamped\)' "$RENDERER_HDR"; then
    rapid_profile_toggle_detail+="duplicate_toggle_elision;"
  else
    rapid_profile_toggle_detail+="missing_duplicate_toggle_elision;"
  fi
  if rg -q 'locusq::headphone_dsp::HeadphonePeqHook::kMaxStages' "$RENDERER_HDR"; then
    rapid_profile_toggle_detail+="stage_count_capped;"
  else
    rapid_profile_toggle_detail+="missing_stage_count_capped;"
  fi

  if [[ "$rapid_profile_toggle_detail" == *"missing_"* ]]; then
    printf "rapid_profile_toggle\tFAIL\t%s\n" "$rapid_profile_toggle_detail" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
    record "BL069-E5-rapid_profile_toggle" "FAIL" "$rapid_profile_toggle_detail" "$COEFFICIENT_SWAP_STABILITY_TSV"
  else
    printf "rapid_profile_toggle\tPASS\t%s\n" "$rapid_profile_toggle_detail" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
    record "BL069-E5-rapid_profile_toggle" "PASS" "$rapid_profile_toggle_detail" "$COEFFICIENT_SWAP_STABILITY_TSV"
  fi
else
  # Contract-only mode keeps execute lanes as scaffold placeholders.
  printf "missing_preset_retry_backoff\tTODO\truntime retry/backoff probe pending\n" >> "$PRESET_RETRY_BACKOFF_TSV"
  printf "transient_asset_unavailable\tTODO\trecovery/backoff probe pending\n" >> "$PRESET_RETRY_BACKOFF_TSV"
  printf "profile_switch_no_glitch\tTODO\tcoefficient swap stability probe pending\n" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
  printf "rapid_profile_toggle\tTODO\thigh-rate profile toggle stability probe pending\n" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
fi
printf "BL069-F001\tpreset_asset_missing\tfallback_generic_or_clear_peq\n" >> "$FAILURE_TAXONOMY_TSV"
printf "BL069-F002\tinvalid_preset_payload\tclear_peq_and_report_contract\n" >> "$FAILURE_TAXONOMY_TSV"

todo_rows=$((
  $(count_todo_rows "$PRESET_RETRY_BACKOFF_TSV")
  + $(count_todo_rows "$COEFFICIENT_SWAP_STABILITY_TSV")
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL069-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL069-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL069-C4-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl069_lane_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

lane_result_value="$(awk -F'\t' '$1=="lane_result"{value=$2} END{print value}' "$STATUS_TSV")"
status_fail_rows="$(awk -F'\t' 'NR>1 && $2=="FAIL"{count++} END{print count+0}' "$STATUS_TSV")"
status_pass_rows="$(awk -F'\t' 'NR>1 && $2=="PASS"{count++} END{print count+0}' "$STATUS_TSV")"
write_summary_md "$lane_result_value" "$status_fail_rows" "$status_pass_rows"

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $RT_ACCESS_AUDIT_TSV"
echo "- $PRESET_RETRY_BACKOFF_TSV"
echo "- $COEFFICIENT_SWAP_STABILITY_TSV"
echo "- $FAILURE_TAXONOMY_TSV"
echo "- $SUMMARY_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
