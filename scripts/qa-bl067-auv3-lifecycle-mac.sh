#!/usr/bin/env bash
# Title: BL-067 AUv3 Lifecycle QA Stub
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-01
#
# Purpose:
# - Provide deterministic BL-067 contract/execute scaffold semantics.
# - Contract mode validates documentation/schema scaffolding.
# - Execute mode enforces truthfulness by failing while execute rows remain TODO.
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl067_auv3_lifecycle_stub_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
HOST_MATRIX_TSV=""
LIFECYCLE_TSV=""
PARITY_TSV=""
PACKAGING_MD=""

usage() {
  cat <<'USAGE'
Usage: qa-bl067-auv3-lifecycle-mac.sh [options]

BL-067 deterministic scaffold lane for AUv3 lifecycle validation.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Run contract-only checks (default)
  --execute          Run execute-mode gate checks (fails while execute rows are TODO)
  --help, -h         Show usage

Outputs:
  status.tsv
  host_matrix.tsv
  lifecycle_transitions.tsv
  parity_regression.tsv
  packaging_manifest.md
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
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
HOST_MATRIX_TSV="${OUT_DIR}/host_matrix.tsv"
LIFECYCLE_TSV="${OUT_DIR}/lifecycle_transitions.tsv"
PARITY_TSV="${OUT_DIR}/parity_regression.tsv"
PACKAGING_MD="${OUT_DIR}/packaging_manifest.md"

pass_count=0
fail_count=0

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
    echo "  [PASS] $check_id: $detail"
  else
    ((fail_count++)) || true
    echo "  [FAIL] $check_id: $detail"
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

echo "=== BL-067 QA Scaffold: AUv3 Lifecycle and Host Validation ==="
echo "Mode: $MODE"
echo "Output dir: $OUT_DIR"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "host\tresult\tdetail\n" > "$HOST_MATRIX_TSV"
printf "transition\tresult\tdetail\n" > "$LIFECYCLE_TSV"
printf "format\tresult\tdetail\n" > "$PARITY_TSV"

cat > "$PACKAGING_MD" <<EOF_MANIFEST
# BL-067 Packaging Manifest (Stub)

- timestamp_utc: ${TIMESTAMP}
- lane: BL-067 scaffold
- mode: ${MODE}
- note: Placeholder manifest until AUv3 signing/packaging automation is wired.
EOF_MANIFEST

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md"
ANNEX_DOC="${ROOT_DIR}/Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md"
SKILL_DOC="${ROOT_DIR}/.codex/skills/auv3-plugin-lifecycle/SKILL.md"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL067-S1-backlog_doc_exists" "PASS" "BL-067 runbook present" "$BACKLOG_DOC"
else
  record "BL067-S1-backlog_doc_exists" "FAIL" "BL-067 runbook missing" "$BACKLOG_DOC"
fi

if [[ -f "$ANNEX_DOC" ]]; then
  record "BL067-S2-annex_doc_exists" "PASS" "BL-067 annex spec present" "$ANNEX_DOC"
else
  record "BL067-S2-annex_doc_exists" "FAIL" "BL-067 annex spec missing" "$ANNEX_DOC"
fi

if [[ -f "$SKILL_DOC" ]]; then
  record "BL067-S3-skill_doc_exists" "PASS" "AUv3 skill present" "$SKILL_DOC"
else
  record "BL067-S3-skill_doc_exists" "FAIL" "AUv3 skill missing" "$SKILL_DOC"
fi

if rg -q 'qa-bl067-auv3-lifecycle-mac.sh' "$BACKLOG_DOC" 2>/dev/null; then
  record "BL067-S4-runbook_references_lane" "PASS" "runbook references this QA lane" "$BACKLOG_DOC"
else
  record "BL067-S4-runbook_references_lane" "FAIL" "runbook missing QA lane reference" "$BACKLOG_DOC"
fi

if rg -q 'BL-067' "$ANNEX_DOC" 2>/dev/null; then
  record "BL067-S5-annex_id_contract" "PASS" "annex includes BL-067 contract identity" "$ANNEX_DOC"
else
  record "BL067-S5-annex_id_contract" "FAIL" "annex missing BL-067 contract identity" "$ANNEX_DOC"
fi

printf "Logic Pro (AUv3)\tTODO\tHost execution lane not implemented in scaffold\n" >> "$HOST_MATRIX_TSV"
printf "GarageBand (AUv3)\tTODO\tHost execution lane not implemented in scaffold\n" >> "$HOST_MATRIX_TSV"
printf "cold_start\tTODO\tRuntime lifecycle probe not implemented in scaffold\n" >> "$LIFECYCLE_TSV"
printf "reload\tTODO\tRuntime lifecycle probe not implemented in scaffold\n" >> "$LIFECYCLE_TSV"
printf "AUv3_vs_AU\tTODO\tCross-format parity lane pending implementation\n" >> "$PARITY_TSV"
printf "AUv3_vs_VST3\tTODO\tCross-format parity lane pending implementation\n" >> "$PARITY_TSV"
printf "AUv3_vs_CLAP\tTODO\tCross-format parity lane pending implementation\n" >> "$PARITY_TSV"

todo_rows=$((
  $(count_todo_rows "$HOST_MATRIX_TSV")
  + $(count_todo_rows "$LIFECYCLE_TSV")
  + $(count_todo_rows "$PARITY_TSV")
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL067-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL067-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL067-C1-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};contract_gates_passed" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};gate_failures=${fail_count}" "$STATUS_TSV"
fi

echo ""
echo "Results: ${pass_count} passed, ${fail_count} failed"
echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $HOST_MATRIX_TSV"
echo "- $LIFECYCLE_TSV"
echo "- $PARITY_TSV"
echo "- $PACKAGING_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
