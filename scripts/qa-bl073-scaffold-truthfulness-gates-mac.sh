#!/usr/bin/env bash
# Title: BL-073 Scaffold Truthfulness Gates
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-01
#
# Purpose:
# - Validate contract-vs-execute mode semantics for BL-067 and BL-068 lanes.
# - Enforce execute-mode truthfulness contract: TODO rows in execute evidence must fail.
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl073_truthfulness_${TIMESTAMP}"

STATUS_TSV=""
MODE_SEMANTICS_TSV=""
TODO_ENFORCEMENT_TSV=""
PROMOTION_POLICY_MD=""
MATRIX_RECONCILE_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl073-scaffold-truthfulness-gates-mac.sh [options]

BL-073 gate lane validating execute-mode truthfulness semantics for BL-067/BL-068.

Options:
  --out-dir <path>   Artifact output directory
  --help, -h         Show usage

Outputs:
  status.tsv
  mode_semantics_contract.tsv
  todo_row_enforcement.tsv
  promotion_gate_policy.md
  bl067_bl068_matrix_reconcile.tsv
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
    echo "  [PASS] $check_id: $detail"
  else
    ((fail_count++)) || true
    echo "  [FAIL] $check_id: $detail"
  fi
}

count_todo_rows_in_tsv() {
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

count_todo_rows_in_dir() {
  local dir="$1"
  [[ -d "$dir" ]] || {
    echo 0
    return
  }

  local total=0
  local file_count=0

  while IFS= read -r tsv_file; do
    local count=0
    count="$(count_todo_rows_in_tsv "$tsv_file")"
    total=$(( total + count ))
    file_count=$(( file_count + 1 ))
  done < <(find "$dir" -maxdepth 1 -type f -name '*.tsv' | sort)

  echo "$total"
}

run_lane_mode() {
  local lane="$1"
  local script_path="$2"
  local mode="$3"

  local lane_out_dir="${OUT_DIR}/${lane}_${mode}"
  mkdir -p "$lane_out_dir"

  local mode_flag="--contract-only"
  if [[ "$mode" == "execute" ]]; then
    mode_flag="--execute"
  fi

  set +e
  "$script_path" --out-dir "$lane_out_dir" "$mode_flag" >"${lane_out_dir}/stdout.log" 2>"${lane_out_dir}/stderr.log"
  local exit_code=$?
  set -e

  local todo_rows=0
  todo_rows="$(count_todo_rows_in_dir "$lane_out_dir")"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$lane" \
    "$mode" \
    "$([[ "$mode" == "contract_only" ]] && echo 0 || echo 1)" \
    "$exit_code" \
    "$todo_rows" \
    "$([[ "$mode" == "contract_only" ]] && echo allow_todo || echo enforce_no_todo)" \
    "$lane_out_dir" \
    >> "$MODE_SEMANTICS_TSV"

  echo "$exit_code;$todo_rows;$lane_out_dir"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || usage_error "--out-dir requires a value"
      OUT_DIR="$2"
      shift 2
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
MODE_SEMANTICS_TSV="${OUT_DIR}/mode_semantics_contract.tsv"
TODO_ENFORCEMENT_TSV="${OUT_DIR}/todo_row_enforcement.tsv"
PROMOTION_POLICY_MD="${OUT_DIR}/promotion_gate_policy.md"
MATRIX_RECONCILE_TSV="${OUT_DIR}/bl067_bl068_matrix_reconcile.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "lane\tmode\texpected_exit\tactual_exit\ttodo_rows\tpolicy\tartifact_dir\n" > "$MODE_SEMANTICS_TSV"
printf "lane\texecute_todo_rows\texecute_exit\tenforcement_result\tdetail\n" > "$TODO_ENFORCEMENT_TSV"
printf "lane\tcontract_todo_rows\tcontract_exit\texecute_todo_rows\texecute_exit\treconciled\n" > "$MATRIX_RECONCILE_TSV"

BL067_SCRIPT="${ROOT_DIR}/scripts/qa-bl067-auv3-lifecycle-mac.sh"
BL068_SCRIPT="${ROOT_DIR}/scripts/qa-bl068-temporal-effects-mac.sh"

for script in "$BL067_SCRIPT" "$BL068_SCRIPT"; do
  if [[ -x "$script" ]]; then
    record "BL073-PRE-script_exists-$(basename "$script")" "PASS" "script is executable" "$script"
  else
    record "BL073-PRE-script_exists-$(basename "$script")" "FAIL" "script missing or not executable" "$script"
  fi
done

if [[ "$fail_count" -gt 0 ]]; then
  record "lane_result" "FAIL" "preflight failures detected" "$STATUS_TSV"
  exit 1
fi

bl067_contract="$(run_lane_mode "BL067" "$BL067_SCRIPT" "contract_only")"
bl067_execute="$(run_lane_mode "BL067" "$BL067_SCRIPT" "execute")"
bl068_contract="$(run_lane_mode "BL068" "$BL068_SCRIPT" "contract_only")"
bl068_execute="$(run_lane_mode "BL068" "$BL068_SCRIPT" "execute")"

parse_triplet() {
  local triplet="$1"
  local key="$2"
  IFS=';' read -r exit_code todo_rows lane_out_dir <<< "$triplet"
  case "$key" in
    exit) echo "$exit_code" ;;
    todo) echo "$todo_rows" ;;
    dir) echo "$lane_out_dir" ;;
    *) echo "" ;;
  esac
}

validate_lane_gate() {
  local lane="$1"
  local contract_triplet="$2"
  local execute_triplet="$3"

  local contract_exit contract_todo execute_exit execute_todo
  contract_exit="$(parse_triplet "$contract_triplet" exit)"
  contract_todo="$(parse_triplet "$contract_triplet" todo)"
  execute_exit="$(parse_triplet "$execute_triplet" exit)"
  execute_todo="$(parse_triplet "$execute_triplet" todo)"

  local enforce_result="PASS"
  local detail="execute mode correctly failed with TODO rows"

  if [[ "$contract_exit" -ne 0 ]]; then
    enforce_result="FAIL"
    detail="contract mode failed unexpectedly (exit=${contract_exit})"
  elif [[ "$execute_todo" -gt 0 && "$execute_exit" -eq 0 ]]; then
    enforce_result="FAIL"
    detail="execute mode passed despite TODO rows (todo=${execute_todo})"
  elif [[ "$execute_todo" -eq 0 && "$execute_exit" -ne 0 ]]; then
    enforce_result="FAIL"
    detail="execute mode failed without TODO rows (todo=0,exit=${execute_exit})"
  fi

  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$lane" \
    "$execute_todo" \
    "$execute_exit" \
    "$enforce_result" \
    "$detail" \
    >> "$TODO_ENFORCEMENT_TSV"

  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$lane" \
    "$contract_todo" \
    "$contract_exit" \
    "$execute_todo" \
    "$execute_exit" \
    "$enforce_result" \
    >> "$MATRIX_RECONCILE_TSV"

  if [[ "$enforce_result" == "PASS" ]]; then
    record "BL073-GATE-${lane}" "PASS" "$detail" "$TODO_ENFORCEMENT_TSV"
  else
    record "BL073-GATE-${lane}" "FAIL" "$detail" "$TODO_ENFORCEMENT_TSV"
  fi
}

validate_lane_gate "BL067" "$bl067_contract" "$bl067_execute"
validate_lane_gate "BL068" "$bl068_contract" "$bl068_execute"

cat > "$PROMOTION_POLICY_MD" <<EOF_POLICY
# BL-073 Promotion Gate Policy

- Generated: ${TIMESTAMP}
- Scope: BL-067 and BL-068 execute-mode truthfulness

## Policy

1. Contract-only outputs may contain scaffold rows marked \`TODO\` and remain eligible for planning.
2. Execute mode is mandatory for promotion review.
3. Execute-mode evidence with one or more \`TODO\` rows is automatic \`NO-GO\`.
4. Promotion packets for BL-067/BL-068 must include this gate's \`todo_row_enforcement.tsv\` and \`bl067_bl068_matrix_reconcile.tsv\` artifacts.
EOF_POLICY

record "BL073-POLICY-promotion_gate_policy" "PASS" "promotion blocker policy emitted" "$PROMOTION_POLICY_MD"

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "bl073_truthfulness_gates_passed" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "bl073_truthfulness_gates_failed=${fail_count}" "$STATUS_TSV"
fi

echo ""
echo "Results: ${pass_count} passed, ${fail_count} failed"
echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $MODE_SEMANTICS_TSV"
echo "- $TODO_ENFORCEMENT_TSV"
echo "- $PROMOTION_POLICY_MD"
echo "- $MATRIX_RECONCILE_TSV"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
