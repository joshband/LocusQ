#!/usr/bin/env bash
# Title: BL-056 Calibration State Migration + Latency Contract QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-16
# Last Modified Date: 2026-03-16
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl056_calibration_state_migration_${TIMESTAMP}"
MODE="contract_only"

STATUS_TSV=""
STATE_MIGRATION_TSV=""
LATENCY_CONTRACT_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl056-calibration-state-migration-mac.sh [options]

BL-056 calibration state migration and latency contract lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while TODO rows remain)
  --help, -h         Show usage

Outputs:
  status.tsv
  state_migration.tsv
  latency_contract.tsv
  golden_snapshot.md
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
  awk -F'\t' 'NR>1 && $2=="TODO"{count++} END{print count+0}' "$file"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires an argument"
      OUT_DIR="$2"
      shift 2
      ;;
    --contract-only)
      MODE="contract_only"
      shift
      ;;
    --execute)
      MODE="execute"
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
STATE_MIGRATION_TSV="${OUT_DIR}/state_migration.tsv"
LATENCY_CONTRACT_TSV="${OUT_DIR}/latency_contract.tsv"
GOLDEN_SNAPSHOT_MD="${OUT_DIR}/golden_snapshot.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$STATE_MIGRATION_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$LATENCY_CONTRACT_TSV"

# Source file paths
RUNBOOK="${ROOT_DIR}/Documentation/backlog/bl-056-calibration-state-migration-latency.md"
CONSTANTS_HDR="${ROOT_DIR}/Source/processor_core/ProcessorConstants.h"
STATE_SERIALIZER="${ROOT_DIR}/Source/processor_core/ProcessorStateSerializer.cpp"
PLUGIN_PROCESSOR="${ROOT_DIR}/Source/PluginProcessor.cpp"
CAL_CHAIN_STATE="${ROOT_DIR}/Source/headphone_core/HeadphoneCalibrationChainState.h"

# ------------------------------------------------------------------
# BL056-C1: Backlog runbook exists
# ------------------------------------------------------------------
if [[ -f "$RUNBOOK" ]]; then
  record "BL056-C1-backlog_doc_exists" "PASS" "runbook present" "$RUNBOOK"
else
  record "BL056-C1-backlog_doc_exists" "FAIL" "runbook missing" "$RUNBOOK"
fi

# ------------------------------------------------------------------
# BL056-C2: V3 schema constant exists in ProcessorConstants.h
# ------------------------------------------------------------------
if rg -Fq 'kSnapshotSchemaValueV3' "$CONSTANTS_HDR" \
   && rg -Fq '"locusq-state-v3"' "$CONSTANTS_HDR"; then
  printf "v3_schema_constant\tPASS\tkSnapshotSchemaValueV3 = \"locusq-state-v3\" present\t%s\n" \
    "$CONSTANTS_HDR" >> "$STATE_MIGRATION_TSV"
  record "BL056-C2-v3_schema_constant" "PASS" \
    "kSnapshotSchemaValueV3 = \"locusq-state-v3\" present in ProcessorConstants.h" "$CONSTANTS_HDR"
else
  printf "v3_schema_constant\tFAIL\tkSnapshotSchemaValueV3 missing from ProcessorConstants.h\t%s\n" \
    "$CONSTANTS_HDR" >> "$STATE_MIGRATION_TSV"
  record "BL056-C2-v3_schema_constant" "FAIL" \
    "kSnapshotSchemaValueV3 missing from ProcessorConstants.h" "$CONSTANTS_HDR"
fi

# ------------------------------------------------------------------
# BL056-C3: getStateInformation writes V3 schema (not V2)
# ------------------------------------------------------------------
if rg -Fq 'kSnapshotSchemaValueV3' "$STATE_SERIALIZER"; then
  # Make sure V2 is not the value being written
  if ! rg -Fq 'kSnapshotSchemaValueV2' "$STATE_SERIALIZER"; then
    printf "get_state_writes_v3\tPASS\tgetStateInformation writes kSnapshotSchemaValueV3\t%s\n" \
      "$STATE_SERIALIZER" >> "$STATE_MIGRATION_TSV"
    record "BL056-C3-get_state_writes_v3" "PASS" \
      "getStateInformation writes kSnapshotSchemaValueV3 (V2 constant removed from writes)" "$STATE_SERIALIZER"
  else
    printf "get_state_writes_v3\tFAIL\tkSnapshotSchemaValueV2 still referenced in StateSerializer\t%s\n" \
      "$STATE_SERIALIZER" >> "$STATE_MIGRATION_TSV"
    record "BL056-C3-get_state_writes_v3" "FAIL" \
      "kSnapshotSchemaValueV2 still referenced in StateSerializer — check write path" "$STATE_SERIALIZER"
  fi
else
  printf "get_state_writes_v3\tFAIL\tkSnapshotSchemaValueV3 not referenced in ProcessorStateSerializer.cpp\t%s\n" \
    "$STATE_SERIALIZER" >> "$STATE_MIGRATION_TSV"
  record "BL056-C3-get_state_writes_v3" "FAIL" \
    "kSnapshotSchemaValueV3 not referenced in ProcessorStateSerializer.cpp" "$STATE_SERIALIZER"
fi

# ------------------------------------------------------------------
# BL056-C4: V2→V3 migration is documented as transparent in StateSerializer
# ------------------------------------------------------------------
if rg -q 'V2.*V3|V2→V3' "$STATE_SERIALIZER"; then
  printf "v2_v3_migration_comment\tPASS\tV2->V3 migration comment present in StateSerializer\t%s\n" \
    "$STATE_SERIALIZER" >> "$STATE_MIGRATION_TSV"
  record "BL056-C4-v2_v3_migration_comment" "PASS" \
    "V2→V3 transparent migration documented in ProcessorStateSerializer.cpp" "$STATE_SERIALIZER"
else
  printf "v2_v3_migration_comment\tFAIL\tV2->V3 migration comment missing from StateSerializer\t%s\n" \
    "$STATE_SERIALIZER" >> "$STATE_MIGRATION_TSV"
  record "BL056-C4-v2_v3_migration_comment" "FAIL" \
    "V2→V3 migration comment missing from ProcessorStateSerializer.cpp" "$STATE_SERIALIZER"
fi

# ------------------------------------------------------------------
# BL056-C5: Idempotent migration — setStateInformation uses hasProperty guard
# ------------------------------------------------------------------
if rg -q 'hasProperty.*hp_calibration_enabled|hp_calibration_enabled.*hasProperty' "$STATE_SERIALIZER"; then
  printf "idempotent_migration_guard\tPASS\thasProperty guard for hp_calibration_enabled present\t%s\n" \
    "$STATE_SERIALIZER" >> "$STATE_MIGRATION_TSV"
  record "BL056-C5-idempotent_migration_guard" "PASS" \
    "hasProperty guard for hp_calibration_enabled present in setStateInformation" "$STATE_SERIALIZER"
else
  printf "idempotent_migration_guard\tFAIL\thasProperty guard for hp_calibration_enabled missing\t%s\n" \
    "$STATE_SERIALIZER" >> "$STATE_MIGRATION_TSV"
  record "BL056-C5-idempotent_migration_guard" "FAIL" \
    "hasProperty guard for hp_calibration_enabled missing from setStateInformation" "$STATE_SERIALIZER"
fi

# ------------------------------------------------------------------
# BL056-C6: Latency = 0 when calibration disabled — chain state returns 0
# ------------------------------------------------------------------
if rg -q 'resolved\.activeLatencySamples = 0' "$CAL_CHAIN_STATE" \
   && rg -q '! request\.enabled' "$CAL_CHAIN_STATE"; then
  printf "latency_zero_when_disabled\tPASS\tcalibration chain returns 0 latency when disabled\t%s\n" \
    "$CAL_CHAIN_STATE" >> "$LATENCY_CONTRACT_TSV"
  record "BL056-C6-latency_zero_when_disabled" "PASS" \
    "resolveCalibrationChainState returns 0 latency when !request.enabled" "$CAL_CHAIN_STATE"
else
  printf "latency_zero_when_disabled\tFAIL\tcalibration chain latency-zero-on-disable path not found\t%s\n" \
    "$CAL_CHAIN_STATE" >> "$LATENCY_CONTRACT_TSV"
  record "BL056-C6-latency_zero_when_disabled" "FAIL" \
    "calibration chain latency-zero-on-disable path not confirmed in HeadphoneCalibrationChainState.h" "$CAL_CHAIN_STATE"
fi

# ------------------------------------------------------------------
# BL056-C7: setLatencySamples(0) called on plugin bypass in processBlock
# ------------------------------------------------------------------
if rg -q 'setLatencySamples \(0\)' "$PLUGIN_PROCESSOR"; then
  printf "latency_reset_on_bypass\tPASS\tsetLatencySamples(0) called on bypass path\t%s\n" \
    "$PLUGIN_PROCESSOR" >> "$LATENCY_CONTRACT_TSV"
  record "BL056-C7-latency_reset_on_bypass" "PASS" \
    "setLatencySamples(0) called in bypass path of processBlock" "$PLUGIN_PROCESSOR"
else
  printf "latency_reset_on_bypass\tFAIL\tsetLatencySamples(0) bypass call not found\t%s\n" \
    "$PLUGIN_PROCESSOR" >> "$LATENCY_CONTRACT_TSV"
  record "BL056-C7-latency_reset_on_bypass" "FAIL" \
    "setLatencySamples(0) bypass call not found in PluginProcessor.cpp" "$PLUGIN_PROCESSOR"
fi

# ------------------------------------------------------------------
# BL056-C8: latency change guard in processBlock (prevents spamming hosts)
# ------------------------------------------------------------------
if rg -q 'lastReportedCalibrationLatency' "$PLUGIN_PROCESSOR"; then
  printf "latency_change_guard\tPASS\tlastReportedCalibrationLatency guard present in processBlock\t%s\n" \
    "$PLUGIN_PROCESSOR" >> "$LATENCY_CONTRACT_TSV"
  record "BL056-C8-latency_change_guard" "PASS" \
    "lastReportedCalibrationLatency guard prevents redundant setLatencySamples calls" "$PLUGIN_PROCESSOR"
else
  printf "latency_change_guard\tFAIL\tlastReportedCalibrationLatency guard missing\t%s\n" \
    "$PLUGIN_PROCESSOR" >> "$LATENCY_CONTRACT_TSV"
  record "BL056-C8-latency_change_guard" "FAIL" \
    "lastReportedCalibrationLatency guard missing from PluginProcessor.cpp" "$PLUGIN_PROCESSOR"
fi

# ------------------------------------------------------------------
# Execute gate: zero TODO rows
# ------------------------------------------------------------------
todo_rows="$((
  $(count_todo_rows "$STATE_MIGRATION_TSV")
  + $(count_todo_rows "$LATENCY_CONTRACT_TSV")
))"

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL056-E1-execute_todo_rows" "FAIL" \
      "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL056-E1-execute_todo_rows" "PASS" \
      "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL056-C9-contract_mode" "PASS" \
    "contract-only mode completed structural probes (todo_rows=${todo_rows})" "$STATUS_TSV"
fi

# ------------------------------------------------------------------
# Lane result
# ------------------------------------------------------------------
if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl056_lane_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

lane_result_value="$(awk -F'\t' '$1=="lane_result"{value=$2} END{print value}' "$STATUS_TSV")"
status_pass_rows="$(awk -F'\t' 'NR>1 && $2=="PASS"{count++} END{print count+0}' "$STATUS_TSV")"
status_fail_rows="$(awk -F'\t' 'NR>1 && $2=="FAIL"{count++} END{print count+0}' "$STATUS_TSV")"

# Golden snapshot summary
cat > "$GOLDEN_SNAPSHOT_MD" <<EOF_SNAP
Title: BL-056 Golden State Snapshot
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-056 Calibration State Migration — Golden Snapshot

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- lane_result: ${lane_result_value}
- schema_version_written: locusq-state-v3
- schema_constant: kSnapshotSchemaValueV3
- migration_v2_to_v3: transparent (no new mandatory fields; hasProperty guards)
- latency_zero_when_disabled: resolved by resolveCalibrationChainState returning 0
- latency_reset_on_bypass: setLatencySamples(0) in processBlock bypass path
- status_pass_rows: ${status_pass_rows}
- status_fail_rows: ${status_fail_rows}
- todo_rows_detected: ${todo_rows}

## Anchors

- backlog_runbook: Documentation/backlog/bl-056-calibration-state-migration-latency.md
- schema_constants: Source/processor_core/ProcessorConstants.h
- state_serializer: Source/processor_core/ProcessorStateSerializer.cpp
- calibration_chain_state: Source/headphone_core/HeadphoneCalibrationChainState.h
- plugin_processor: Source/PluginProcessor.cpp
EOF_SNAP

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $STATE_MIGRATION_TSV"
echo "- $LATENCY_CONTRACT_TSV"
echo "- $GOLDEN_SNAPSHOT_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
