#!/usr/bin/env bash
# Title: BL-059 Calibration Integration Smoke Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-02-28
# Last Modified Date: 2026-03-01
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl059_calibration_integration_smoke_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
INTEGRATION_MATRIX_TSV=""
PROFILE_ROUNDTRIP_TSV=""
ORIENTATION_INVARIANTS_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl059-calibration-integration-smoke-mac.sh [options]

BL-059 calibration profile integration smoke lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while runtime TODO rows exist)
  --help, -h         Show usage

Outputs:
  status.tsv
  integration_matrix.tsv
  profile_roundtrip.tsv
  orientation_invariants.tsv
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
INTEGRATION_MATRIX_TSV="${OUT_DIR}/integration_matrix.tsv"
PROFILE_ROUNDTRIP_TSV="${OUT_DIR}/profile_roundtrip.tsv"
ORIENTATION_INVARIANTS_TSV="${OUT_DIR}/orientation_invariants.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$INTEGRATION_MATRIX_TSV"
printf "scenario\tresult\tdetail\n" > "$PROFILE_ROUNDTRIP_TSV"
printf "scenario\tresult\tdetail\n" > "$ORIENTATION_INVARIANTS_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-059-calibration-profile-integration-handoff.md"
PROCESSOR_CPP="${ROOT_DIR}/Source/PluginProcessor.cpp"
CALIBRATION_ENGINE_HDR="${ROOT_DIR}/Source/CalibrationEngine.h"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL059-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL059-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if [[ -f "$CALIBRATION_ENGINE_HDR" ]]; then
  record "BL059-C2-calibration_engine_exists" "PASS" "calibration engine source present" "$CALIBRATION_ENGINE_HDR"
else
  record "BL059-C2-calibration_engine_exists" "FAIL" "calibration engine source missing" "$CALIBRATION_ENGINE_HDR"
fi

if rg -q 'pollCompanionCalibrationProfileFromDisk|resolveCompanionCalibrationProfileFile|CalibrationProfile\.json' "$PROCESSOR_CPP"; then
  printf "companion_state_bridge_presence\tPASS\tprocessor calibration-profile bridge paths found\n" >> "$INTEGRATION_MATRIX_TSV"
  record "BL059-C3-companion_bridge_presence" "PASS" "processor calibration-profile bridge paths found" "$PROCESSOR_CPP"
else
  printf "companion_state_bridge_presence\tFAIL\tprocessor calibration-profile bridge paths missing\n" >> "$INTEGRATION_MATRIX_TSV"
  record "BL059-C3-companion_bridge_presence" "FAIL" "processor calibration-profile bridge paths missing" "$PROCESSOR_CPP"
fi

printf "profile_load_roundtrip\tTODO\truntime profile load/reload probe pending\n" >> "$PROFILE_ROUNDTRIP_TSV"
printf "profile_unload_recovery\tTODO\truntime profile unload recovery probe pending\n" >> "$PROFILE_ROUNDTRIP_TSV"
printf "stale_packet_fallback\tTODO\torientation stale fallback probe pending\n" >> "$ORIENTATION_INVARIANTS_TSV"
printf "yaw_composition_invariant\tTODO\tyaw composition invariant probe pending\n" >> "$ORIENTATION_INVARIANTS_TSV"

todo_rows=$((
  $(count_todo_rows "$PROFILE_ROUNDTRIP_TSV")
  + $(count_todo_rows "$ORIENTATION_INVARIANTS_TSV")
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL059-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL059-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL059-C4-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl059_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $INTEGRATION_MATRIX_TSV"
echo "- $PROFILE_ROUNDTRIP_TSV"
echo "- $ORIENTATION_INVARIANTS_TSV"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
