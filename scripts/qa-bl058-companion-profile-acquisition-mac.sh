#!/usr/bin/env bash
# Title: BL-058 Companion Profile Acquisition QA Lane
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
OUT_DIR="${ROOT_DIR}/TestEvidence/bl058_companion_profile_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
RESULTS_TSV=""
AXIS_SWEEPS_MD=""
READINESS_GATE_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl058-companion-profile-acquisition-mac.sh [options]

BL-058 companion profile acquisition lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while runtime TODO rows exist)
  --help, -h         Show usage

Outputs:
  status.tsv
  results.tsv
  axis_sweeps.md
  readiness_gate.md
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
RESULTS_TSV="${OUT_DIR}/results.tsv"
AXIS_SWEEPS_MD="${OUT_DIR}/axis_sweeps.md"
READINESS_GATE_MD="${OUT_DIR}/readiness_gate.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$RESULTS_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-058-companion-profile-acquisition.md"
COMPANION_MAIN="${ROOT_DIR}/companion/Sources/LocusQHeadTrackingCompanion/main.swift"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL058-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL058-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if [[ -f "$COMPANION_MAIN" ]]; then
  record "BL058-C2-companion_runtime_exists" "PASS" "companion runtime source present" "$COMPANION_MAIN"
else
  record "BL058-C2-companion_runtime_exists" "FAIL" "companion runtime source missing" "$COMPANION_MAIN"
fi

if rg -q 'active_not_ready|active_ready|disabled_disconnected' "$BACKLOG_DOC"; then
  printf "readiness_state_contract\tPASS\tstate-machine identifiers present in runbook\n" >> "$RESULTS_TSV"
  record "BL058-C3-readiness_state_contract" "PASS" "runbook readiness contract present" "$BACKLOG_DOC"
else
  printf "readiness_state_contract\tFAIL\tstate-machine identifiers missing from runbook\n" >> "$RESULTS_TSV"
  record "BL058-C3-readiness_state_contract" "FAIL" "runbook readiness contract missing" "$BACKLOG_DOC"
fi

printf "manual_capture_flow\tTODO\tcompanion manual runtime packet pending\n" >> "$RESULTS_TSV"
printf "embedding_latency_lt_50ms\tTODO\tperformance probe pending\n" >> "$RESULTS_TSV"
printf "privacy_no_network\tTODO\truntime privacy audit pending\n" >> "$RESULTS_TSV"

cat > "$AXIS_SWEEPS_MD" <<EOF_AXIS
Title: BL-058 Axis Sweeps (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-058 Axis Sweeps (Stub)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- pending: synthetic yaw/pitch/roll capture evidence.
EOF_AXIS

cat > "$READINESS_GATE_MD" <<EOF_READY
Title: BL-058 Readiness Gate (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-058 Readiness Gate (Stub)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- pending: runtime proof that send gate remains closed until explicit sync.
EOF_READY

todo_rows="$(count_todo_rows "$RESULTS_TSV")"
if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL058-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL058-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL058-C4-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl058_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $RESULTS_TSV"
echo "- $AXIS_SWEEPS_MD"
echo "- $READINESS_GATE_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
