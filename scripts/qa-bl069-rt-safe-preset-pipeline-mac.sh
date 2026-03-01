#!/usr/bin/env bash
# Title: BL-069 RT-Safe Preset Pipeline QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-01
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

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
RT_ACCESS_AUDIT_TSV="${OUT_DIR}/rt_access_audit.tsv"
PRESET_RETRY_BACKOFF_TSV="${OUT_DIR}/preset_retry_backoff.tsv"
COEFFICIENT_SWAP_STABILITY_TSV="${OUT_DIR}/coefficient_swap_stability.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"

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

# Runtime execute lanes remain TODO until dedicated probes are implemented.
printf "missing_preset_retry_backoff\tTODO\truntime retry/backoff probe pending\n" >> "$PRESET_RETRY_BACKOFF_TSV"
printf "transient_asset_unavailable\tTODO\trecovery/backoff probe pending\n" >> "$PRESET_RETRY_BACKOFF_TSV"
printf "profile_switch_no_glitch\tTODO\tcoefficient swap stability probe pending\n" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
printf "rapid_profile_toggle\tTODO\thigh-rate profile toggle stability probe pending\n" >> "$COEFFICIENT_SWAP_STABILITY_TSV"
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
  record "lane_result" "PASS" "mode=${MODE};bl069_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $RT_ACCESS_AUDIT_TSV"
echo "- $PRESET_RETRY_BACKOFF_TSV"
echo "- $COEFFICIENT_SWAP_STABILITY_TSV"
echo "- $FAILURE_TAXONOMY_TSV"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
