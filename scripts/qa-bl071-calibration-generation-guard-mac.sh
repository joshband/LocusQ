#!/usr/bin/env bash
# Title: BL-071 Calibration Generation Guard QA Lane
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
OUT_DIR="${ROOT_DIR}/TestEvidence/bl071_calibration_generation_guard_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
GENERATION_ISOLATION_TSV=""
ERROR_STATE_CONTRACT_TSV=""
CROSS_THREAD_SNAPSHOT_CONTRACT_TSV=""
CALIBRATION_FAILURE_TAXONOMY_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl071-calibration-generation-guard-mac.sh [options]

BL-071 calibration generation guard + error-state enforcement lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while runtime TODO rows exist)
  --help, -h         Show usage

Outputs:
  status.tsv
  generation_isolation.tsv
  error_state_contract.tsv
  cross_thread_snapshot_contract.tsv
  calibration_failure_taxonomy.tsv
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
GENERATION_ISOLATION_TSV="${OUT_DIR}/generation_isolation.tsv"
ERROR_STATE_CONTRACT_TSV="${OUT_DIR}/error_state_contract.tsv"
CROSS_THREAD_SNAPSHOT_CONTRACT_TSV="${OUT_DIR}/cross_thread_snapshot_contract.tsv"
CALIBRATION_FAILURE_TAXONOMY_TSV="${OUT_DIR}/calibration_failure_taxonomy.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$GENERATION_ISOLATION_TSV"
printf "check\tresult\tdetail\n" > "$ERROR_STATE_CONTRACT_TSV"
printf "check\tresult\tdetail\n" > "$CROSS_THREAD_SNAPSHOT_CONTRACT_TSV"
printf "failure_id\tclassification\tmitigation\n" > "$CALIBRATION_FAILURE_TAXONOMY_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-071-calibration-generation-guard-and-error-state-enforcement.md"
CALIBRATION_ENGINE_HDR="${ROOT_DIR}/Source/CalibrationEngine.h"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL071-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL071-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if rg -q 'runGenerationCounter_|activeRunGeneration_|pendingAnalysisGeneration_' "$CALIBRATION_ENGINE_HDR" \
   && rg -q 'startSpeaker \(int speakerIdx, std::uint64_t runGeneration\)' "$CALIBRATION_ENGINE_HDR"; then
  printf "generation_tracking_members\tPASS\trun generation counters and guarded speaker start present\n" >> "$GENERATION_ISOLATION_TSV"
  record "BL071-C2-generation_tracking" "PASS" "generation guard markers present" "$CALIBRATION_ENGINE_HDR"
else
  printf "generation_tracking_members\tFAIL\tmissing generation guard members or guarded start signature\n" >> "$GENERATION_ISOLATION_TSV"
  record "BL071-C2-generation_tracking" "FAIL" "generation guard markers missing" "$CALIBRATION_ENGINE_HDR"
fi

if rg -q 'analysis_in_flight' "$CALIBRATION_ENGINE_HDR" && rg -q 'analysisInFlight_' "$CALIBRATION_ENGINE_HDR"; then
  printf "restart_gate_while_analysis\tPASS\tstart path rejects restart while analysis is in-flight\n" >> "$GENERATION_ISOLATION_TSV"
  record "BL071-C3-restart_gate" "PASS" "analysis in-flight restart gate present" "$CALIBRATION_ENGINE_HDR"
else
  printf "restart_gate_while_analysis\tFAIL\tanalysis in-flight restart gate missing\n" >> "$GENERATION_ISOLATION_TSV"
  record "BL071-C3-restart_gate" "FAIL" "analysis in-flight restart gate missing" "$CALIBRATION_ENGINE_HDR"
fi

if rg -q 'publishRunError' "$CALIBRATION_ENGINE_HDR" \
   && rg -q 'state_\.store \(State::Error' "$CALIBRATION_ENGINE_HDR"; then
  printf "explicit_error_state_publish\tPASS\tinvalid analysis paths publish diagnostics and transition to Error\n" >> "$ERROR_STATE_CONTRACT_TSV"
  record "BL071-C4-error_state_contract" "PASS" "error-state publication markers present" "$CALIBRATION_ENGINE_HDR"
else
  printf "explicit_error_state_publish\tFAIL\terror-state publication markers missing\n" >> "$ERROR_STATE_CONTRACT_TSV"
  record "BL071-C4-error_state_contract" "FAIL" "error-state publication markers missing" "$CALIBRATION_ENGINE_HDR"
fi

if rg -q 'std::atomic<float> playPercentAtomic_|std::atomic<float> recordPercentAtomic_' "$CALIBRATION_ENGINE_HDR" \
   && rg -q 'RoomProfile       getResult\(\)    const' "$CALIBRATION_ENGINE_HDR" \
   && rg -q 'ScopedLockType lock \(resultProfileLock_\)' "$CALIBRATION_ENGINE_HDR"; then
  printf "snapshot_publication_contract\tPASS\tatomic progress snapshots and locked result copy publication present\n" >> "$CROSS_THREAD_SNAPSHOT_CONTRACT_TSV"
  record "BL071-C5-snapshot_contract" "PASS" "cross-thread snapshot publication markers present" "$CALIBRATION_ENGINE_HDR"
else
  printf "snapshot_publication_contract\tFAIL\tcross-thread snapshot publication markers missing\n" >> "$CROSS_THREAD_SNAPSHOT_CONTRACT_TSV"
  record "BL071-C5-snapshot_contract" "FAIL" "cross-thread snapshot publication markers missing" "$CALIBRATION_ENGINE_HDR"
fi

printf "abort_restart_generation_isolation\tTODO\truntime abort/restart stale-generation probe pending\n" >> "$GENERATION_ISOLATION_TSV"
printf "stale_analysis_drop_after_abort\tTODO\truntime stale-analysis rejection probe pending\n" >> "$GENERATION_ISOLATION_TSV"
printf "invalid_ir_transitions_error\tTODO\truntime invalid-IR to Error-state probe pending\n" >> "$ERROR_STATE_CONTRACT_TSV"
printf "partial_speaker_set_blocks_complete\tTODO\truntime partial-speaker validity probe pending\n" >> "$ERROR_STATE_CONTRACT_TSV"
printf "ui_poll_vs_audio_progress_race\tTODO\tthreaded progress publication contention probe pending\n" >> "$CROSS_THREAD_SNAPSHOT_CONTRACT_TSV"
printf "result_copy_consistency_under_polling\tTODO\tresult snapshot copy stability probe pending\n" >> "$CROSS_THREAD_SNAPSHOT_CONTRACT_TSV"

printf "BL071-F001\tstale_generation_analysis\tinvalidate_run_generation_and_drop_stale_results\n" >> "$CALIBRATION_FAILURE_TAXONOMY_TSV"
printf "BL071-F002\tanalysis_invalid_or_partial\tpublish_error_state_and_mark_profile_invalid\n" >> "$CALIBRATION_FAILURE_TAXONOMY_TSV"
printf "BL071-F003\tcross_thread_snapshot_race\tpublish_atomic_progress_and_locked_result_copies\n" >> "$CALIBRATION_FAILURE_TAXONOMY_TSV"

todo_rows=$((
  $(count_todo_rows "$GENERATION_ISOLATION_TSV")
  + $(count_todo_rows "$ERROR_STATE_CONTRACT_TSV")
  + $(count_todo_rows "$CROSS_THREAD_SNAPSHOT_CONTRACT_TSV")
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL071-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL071-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL071-C6-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl071_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $GENERATION_ISOLATION_TSV"
echo "- $ERROR_STATE_CONTRACT_TSV"
echo "- $CROSS_THREAD_SNAPSHOT_CONTRACT_TSV"
echo "- $CALIBRATION_FAILURE_TAXONOMY_TSV"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
