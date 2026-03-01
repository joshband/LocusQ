#!/usr/bin/env bash
# Title: BL-070 Snapshot and Telemetry Contract QA Lane
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
OUT_DIR="${ROOT_DIR}/TestEvidence/bl070_snapshot_telemetry_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
SNAPSHOT_COHERENCY_TSV=""
TELEMETRY_SEQLOCK_TSV=""
SCENE_BRIDGE_STRESS_TSV=""
TSAN_REPORT_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl070-snapshot-telemetry-contract-mac.sh [options]

BL-070 coherent snapshot + telemetry publication lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while runtime TODO rows exist)
  --help, -h         Show usage

Outputs:
  status.tsv
  snapshot_coherency.tsv
  telemetry_seqlock_contract.tsv
  scene_bridge_stress.tsv
  tsan_or_equivalent_report.md
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
SNAPSHOT_COHERENCY_TSV="${OUT_DIR}/snapshot_coherency.tsv"
TELEMETRY_SEQLOCK_TSV="${OUT_DIR}/telemetry_seqlock_contract.tsv"
SCENE_BRIDGE_STRESS_TSV="${OUT_DIR}/scene_bridge_stress.tsv"
TSAN_REPORT_MD="${OUT_DIR}/tsan_or_equivalent_report.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$SNAPSHOT_COHERENCY_TSV"
printf "check\tresult\tdetail\n" > "$TELEMETRY_SEQLOCK_TSV"
printf "scenario\tresult\tdetail\n" > "$SCENE_BRIDGE_STRESS_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-070-coherent-audio-snapshot-and-telemetry-seqlock-contract.md"
SCENEGRAPH_HDR="${ROOT_DIR}/Source/SceneGraph.h"
BRIDGE_HDR="${ROOT_DIR}/Source/processor_bridge/ProcessorSceneStateBridgeOps.h"
PROCESSOR_HDR="${ROOT_DIR}/Source/PluginProcessor.h"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL070-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL070-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if rg -q 'struct AudioReadSnapshot' "$SCENEGRAPH_HDR" && rg -q 'readAudioSnapshot\(\) const' "$SCENEGRAPH_HDR"; then
  printf "audio_snapshot_api\tPASS\tAudioReadSnapshot + readAudioSnapshot present\n" >> "$SNAPSHOT_COHERENCY_TSV"
  record "BL070-C2-snapshot_api" "PASS" "coherent snapshot API present" "$SCENEGRAPH_HDR"
else
  printf "audio_snapshot_api\tFAIL\tcoherent snapshot API missing\n" >> "$SNAPSHOT_COHERENCY_TSV"
  record "BL070-C2-snapshot_api" "FAIL" "coherent snapshot API missing" "$SCENEGRAPH_HDR"
fi

if rg -q 'readAudioSnapshot\(\)' "$BRIDGE_HDR"; then
  printf "bridge_snapshot_consumer\tPASS\tbridge consumes coherent snapshots\n" >> "$SNAPSHOT_COHERENCY_TSV"
  record "BL070-C3-bridge_snapshot_consumer" "PASS" "bridge consumes coherent snapshots" "$BRIDGE_HDR"
else
  printf "bridge_snapshot_consumer\tFAIL\tbridge does not consume coherent snapshots\n" >> "$SNAPSHOT_COHERENCY_TSV"
  record "BL070-C3-bridge_snapshot_consumer" "FAIL" "bridge not updated to coherent snapshots" "$BRIDGE_HDR"
fi

if rg -q 'std::atomic<float> perfProcessBlockMs' "$PROCESSOR_HDR" && rg -q 'sceneSpeakerRms\[' "$BRIDGE_HDR"; then
  printf "telemetry_atomic_publication\tPASS\tperf/speaker telemetry atomics present and consumed\n" >> "$TELEMETRY_SEQLOCK_TSV"
  record "BL070-C4-telemetry_atomic_publication" "PASS" "atomic telemetry publication/reads present" "$PROCESSOR_HDR"
else
  printf "telemetry_atomic_publication\tFAIL\tatomic telemetry publication contract missing\n" >> "$TELEMETRY_SEQLOCK_TSV"
  record "BL070-C4-telemetry_atomic_publication" "FAIL" "atomic telemetry contract missing" "$PROCESSOR_HDR"
fi

printf "high_frequency_ui_polling\tTODO\tstress replay lane pending implementation\n" >> "$SCENE_BRIDGE_STRESS_TSV"
printf "mixed_audio_ui_thread_contention\tTODO\tcontention stress probe pending\n" >> "$SCENE_BRIDGE_STRESS_TSV"

cat > "$TSAN_REPORT_MD" <<EOF_MD
Title: BL-070 TSAN/Equivalent Report (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-070 TSAN/Equivalent Report (Stub)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- note: dedicated thread-sanitizer or equivalent stress report integration is pending.
EOF_MD

todo_rows="$(count_todo_rows "$SCENE_BRIDGE_STRESS_TSV")"
if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL070-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL070-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL070-C5-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl070_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $SNAPSHOT_COHERENCY_TSV"
echo "- $TELEMETRY_SEQLOCK_TSV"
echo "- $SCENE_BRIDGE_STRESS_TSV"
echo "- $TSAN_REPORT_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
