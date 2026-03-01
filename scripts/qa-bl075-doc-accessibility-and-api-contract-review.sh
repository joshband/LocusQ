#!/usr/bin/env bash
# Title: BL-075 Doc Accessibility and API Contract Review QA Lane
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
OUT_DIR="${ROOT_DIR}/TestEvidence/bl075_doc_accessibility_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
COMMENT_REVIEW_MATRIX_TSV=""
API_DOC_COVERAGE_MAP_MD=""
STALE_COMMENT_REMEDIATION_TSV=""
CONTRIBUTOR_ENTRYPOINTS_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl075-doc-accessibility-and-api-contract-review.sh [options]

BL-075 docs/comment accessibility review lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while review TODO rows exist)
  --help, -h         Show usage

Outputs:
  status.tsv
  comment_review_matrix.tsv
  api_doc_coverage_map.md
  stale_comment_remediation.tsv
  contributor_entrypoints.md
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
COMMENT_REVIEW_MATRIX_TSV="${OUT_DIR}/comment_review_matrix.tsv"
API_DOC_COVERAGE_MAP_MD="${OUT_DIR}/api_doc_coverage_map.md"
STALE_COMMENT_REMEDIATION_TSV="${OUT_DIR}/stale_comment_remediation.tsv"
CONTRIBUTOR_ENTRYPOINTS_MD="${OUT_DIR}/contributor_entrypoints.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "surface\tresult\tdetail\n" > "$COMMENT_REVIEW_MATRIX_TSV"
printf "surface\tresult\tdetail\n" > "$STALE_COMMENT_REMEDIATION_TSV"

RUNBOOK="${ROOT_DIR}/Documentation/backlog/bl-075-code-comment-and-api-documentation-accessibility-review.md"
DOC_INDEX="${ROOT_DIR}/Documentation/README.md"
DOC_STANDARDS="${ROOT_DIR}/Documentation/standards.md"
DOXYFILE="${ROOT_DIR}/Documentation/Doxyfile"
ARCHITECTURE_DOC="${ROOT_DIR}/ARCHITECTURE.md"
SCENE_CONTRACT_DOC="${ROOT_DIR}/Documentation/scene-state-contract.md"

CALIBRATION_HDR="${ROOT_DIR}/Source/CalibrationEngine.h"
HEADTRACKING_BRIDGE="${ROOT_DIR}/Source/HeadTrackingBridge.h"
SCENE_BRIDGE_OPS="${ROOT_DIR}/Source/processor_bridge/ProcessorSceneStateBridgeOps.h"
SHARED_CONTRACT_DIR="${ROOT_DIR}/Source/shared_contracts"

if [[ -f "$RUNBOOK" ]]; then
  record "BL075-C1-runbook_exists" "PASS" "runbook present" "$RUNBOOK"
else
  record "BL075-C1-runbook_exists" "FAIL" "runbook missing" "$RUNBOOK"
fi

if [[ -f "$DOXYFILE" ]] && rg -q 'OUTPUT_DIRECTORY.*build/docs_api' "$DOXYFILE" && rg -q 'INPUT.*Source' "$DOXYFILE"; then
  record "BL075-C2-doxygen_config" "PASS" "doxygen config present for Source/API docs" "$DOXYFILE"
else
  record "BL075-C2-doxygen_config" "FAIL" "doxygen config missing or incomplete" "$DOXYFILE"
fi

if rg -q 'Doxygen is the preferred API doc generator' "$DOC_STANDARDS" && rg -q 'Recommended command: `doxygen Documentation/Doxyfile`' "$DOC_STANDARDS"; then
  record "BL075-C3-standards_api_doc_guidance" "PASS" "documentation standards include API-doc guidance" "$DOC_STANDARDS"
else
  record "BL075-C3-standards_api_doc_guidance" "FAIL" "API-doc guidance missing in standards" "$DOC_STANDARDS"
fi

if [[ -f "$ARCHITECTURE_DOC" && -f "$SCENE_CONTRACT_DOC" && -f "$DOC_INDEX" ]]; then
  record "BL075-C4-contributor_docs_present" "PASS" "core contributor doc entrypoints present" "$DOC_INDEX"
else
  record "BL075-C4-contributor_docs_present" "FAIL" "core contributor doc entrypoints missing" "$DOC_INDEX"
fi

if [[ -f "$CALIBRATION_HDR" ]] && rg -q 'Real-time safe|real-time safe|background analysis thread' "$CALIBRATION_HDR"; then
  printf "calibration_engine_threading_comments\tPASS\tcalibration lifecycle/RT-thread comments present\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C5-calibration_comment_surface" "PASS" "calibration comments surface present" "$CALIBRATION_HDR"
else
  printf "calibration_engine_threading_comments\tFAIL\tcalibration lifecycle/RT-thread comments missing\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C5-calibration_comment_surface" "FAIL" "calibration comment surface missing" "$CALIBRATION_HDR"
fi

if [[ -f "$HEADTRACKING_BRIDGE" ]] && rg -q 'staleThresholdMs|packetSizeV1|packetSizeV2' "$HEADTRACKING_BRIDGE"; then
  printf "headtracking_packet_contract_comments\tPASS\theadtracking packet/staleness contract markers present\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C6-headtracking_contract_surface" "PASS" "headtracking contract markers present" "$HEADTRACKING_BRIDGE"
else
  printf "headtracking_packet_contract_comments\tFAIL\theadtracking packet/staleness contract markers missing\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C6-headtracking_contract_surface" "FAIL" "headtracking contract markers missing" "$HEADTRACKING_BRIDGE"
fi

if [[ -f "$SCENE_BRIDGE_OPS" ]] && rg -q 'calibration|telemetry|snapshot|headTracking' "$SCENE_BRIDGE_OPS"; then
  printf "processor_bridge_contract_comments\tPASS\tbridge contract/comment surface present\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C7-bridge_comment_surface" "PASS" "bridge contract/comment surface present" "$SCENE_BRIDGE_OPS"
else
  printf "processor_bridge_contract_comments\tFAIL\tbridge contract/comment surface missing\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C7-bridge_comment_surface" "FAIL" "bridge contract/comment surface missing" "$SCENE_BRIDGE_OPS"
fi

if [[ -d "$SHARED_CONTRACT_DIR" ]] && [[ -n "$(find "$SHARED_CONTRACT_DIR" -maxdepth 1 -name '*.h' -print -quit)" ]]; then
  printf "shared_contract_headers_present\tPASS\tshared contract headers available for contributor/API review\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C8-shared_contract_headers" "PASS" "shared contract headers present" "$SHARED_CONTRACT_DIR"
else
  printf "shared_contract_headers_present\tFAIL\tshared contract headers missing\n" >> "$COMMENT_REVIEW_MATRIX_TSV"
  record "BL075-C8-shared_contract_headers" "FAIL" "shared contract headers missing" "$SHARED_CONTRACT_DIR"
fi

if [[ -f "$CALIBRATION_HDR" ]] && rg -q 'Bump generation at run start so the worker can reject stale analysis' "$CALIBRATION_HDR"; then
  printf "calibration_engine_non_obvious_paths\tPASS\tgeneration guard rationale comment present\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
else
  printf "calibration_engine_non_obvious_paths\tFAIL\tgeneration guard rationale comment missing\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
  record "BL075-R1-calibration_remediation" "FAIL" "generation guard rationale comment missing" "$CALIBRATION_HDR"
fi

if [[ -f "$HEADTRACKING_BRIDGE" ]] && rg -q 'Accept sequence restarts only when the prior' "$HEADTRACKING_BRIDGE"; then
  printf "headtracking_bridge_sequence_and_stale_logic\tPASS\tsequence restart rationale comment present\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
else
  printf "headtracking_bridge_sequence_and_stale_logic\tFAIL\tsequence restart rationale comment missing\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
  record "BL075-R2-headtracking_remediation" "FAIL" "sequence restart rationale comment missing" "$HEADTRACKING_BRIDGE"
fi

if [[ -f "$SCENE_BRIDGE_OPS" ]] && rg -q 'Read coherent audio snapshot once per emitter' "$SCENE_BRIDGE_OPS"; then
  printf "processor_bridge_snapshot_publication\tPASS\tsnapshot publication rationale comment present\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
else
  printf "processor_bridge_snapshot_publication\tFAIL\tsnapshot publication rationale comment missing\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
  record "BL075-R3-scene_bridge_remediation" "FAIL" "snapshot publication rationale comment missing" "$SCENE_BRIDGE_OPS"
fi

shared_contract_comment_count="$(rg -n 'Canonical .*contract|Canonical bridge status keys|Canonical wire contract keys|Canonical confidence/masking contract|Canonical operation/outcome enums' "$SHARED_CONTRACT_DIR"/*.h | wc -l | tr -d '[:space:]')"
if [[ "${shared_contract_comment_count}" -ge 5 ]]; then
  printf "shared_contract_headers_doxygen_surface\tPASS\tshared contract headers include canonical boundary comments\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
else
  printf "shared_contract_headers_doxygen_surface\tFAIL\tshared contract boundary comments incomplete (found=${shared_contract_comment_count})\n" >> "$STALE_COMMENT_REMEDIATION_TSV"
  record "BL075-R4-shared_contract_remediation" "FAIL" "shared contract boundary comments incomplete (found=${shared_contract_comment_count})" "$SHARED_CONTRACT_DIR"
fi

cat > "$API_DOC_COVERAGE_MAP_MD" <<EOF_MAP
Title: BL-075 API Doc Coverage Map (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-075 API Doc Coverage Map (Stub)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}

## Source Surfaces

- Source/CalibrationEngine.h: calibration lifecycle + threading state machine.
- Source/HeadTrackingBridge.h: packet decode contract, sequence/staleness guards, ack semantics.
- Source/processor_bridge/ProcessorSceneStateBridgeOps.h: plugin-to-UI status payload contract publication.
- Source/shared_contracts/*.h: shared plugin/UI/companion contract declarations.

## Doc Tooling

- Doxygen config: Documentation/Doxyfile
- Standards authority: Documentation/standards.md (API Documentation section)
EOF_MAP

cat > "$CONTRIBUTOR_ENTRYPOINTS_MD" <<EOF_ENTRYPOINTS
Title: BL-075 Contributor Entrypoints (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-075 Contributor Entrypoints (Stub)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}

## Canonical Docs

- README.md (project/operator contract)
- ARCHITECTURE.md (architecture source-of-truth)
- Documentation/README.md (documentation tiering and authority)
- Documentation/scene-state-contract.md (runtime status contract)
- Documentation/standards.md (docs/API documentation standards)
- Documentation/backlog/index.md (backlog authority)

## API/Code Contract Surfaces

- Source/shared_contracts/
- Source/processor_bridge/
- Source/HeadTrackingBridge.h
- Source/CalibrationEngine.h
EOF_ENTRYPOINTS

todo_rows=$((
  $(count_todo_rows "$COMMENT_REVIEW_MATRIX_TSV")
  + $(count_todo_rows "$STALE_COMMENT_REMEDIATION_TSV")
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL075-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL075-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL075-C9-contract_mode" "PASS" "contract-only mode completed remediation checks (todo_rows=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl075_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $COMMENT_REVIEW_MATRIX_TSV"
echo "- $API_DOC_COVERAGE_MAP_MD"
echo "- $STALE_COMMENT_REMEDIATION_TSV"
echo "- $CONTRIBUTOR_ENTRYPOINTS_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
