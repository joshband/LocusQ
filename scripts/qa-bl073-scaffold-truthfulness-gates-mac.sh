#!/usr/bin/env bash
# Title: BL-073 Scaffold Truthfulness Gates
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-03
#
# Purpose:
# - Validate contract-vs-execute mode semantics for BL-067 and BL-068 lanes.
# - Enforce execute-mode truthfulness by requiring execute failure when required rows are TODO/SCAFFOLD.
# - Provide deterministic replay semantics through --runs.
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl073_truthfulness_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=1

STATUS_TSV=""
MODE_SEMANTICS_TSV=""
TODO_ENFORCEMENT_TSV=""
PROMOTION_POLICY_MD=""
MATRIX_RECONCILE_TSV=""

TRUTH_HELPER="${ROOT_DIR}/scripts/lib/qa_truthfulness_gate.sh"

pass_count=0
fail_count=0
execute_scaffold_rows_total=0

usage() {
  cat <<'USAGE'
Usage: qa-bl073-scaffold-truthfulness-gates-mac.sh [options]

BL-073 gate lane validating execute-mode truthfulness semantics for BL-067/BL-068.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Validate contract semantics only (default mode)
  --execute          Validate execute semantics only (fails while TODO/SCAFFOLD rows remain)
  --runs <N>         Number of deterministic replay runs (integer >= 1, default: 1)
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

count_tsv_files_in_dir() {
  local dir="$1"
  [[ -d "$dir" ]] || {
    echo 0
    return
  }

  find "$dir" -maxdepth 1 -type f -name '*.tsv' | wc -l | tr -d '[:space:]'
}

parse_triplet() {
  local triplet="$1"
  local key="$2"

  local actual_exit scaffold_rows expected_exit tsv_file_count lane_out_dir
  IFS=';' read -r actual_exit scaffold_rows expected_exit tsv_file_count lane_out_dir <<< "$triplet"

  case "$key" in
    exit) echo "$actual_exit" ;;
    scaffold) echo "$scaffold_rows" ;;
    expected) echo "$expected_exit" ;;
    tsv_count) echo "$tsv_file_count" ;;
    dir) echo "$lane_out_dir" ;;
    *) echo "" ;;
  esac
}

run_lane_mode() {
  local lane="$1"
  local script_path="$2"
  local mode="$3"
  local run_index="$4"

  local lane_out_dir="${OUT_DIR}/run_${run_index}/${lane}_${mode}"
  mkdir -p "$lane_out_dir"

  local mode_flag="--contract-only"
  if [[ "$mode" == "execute" ]]; then
    mode_flag="--execute"
  fi

  set +e
  "$script_path" --out-dir "$lane_out_dir" "$mode_flag" > "${lane_out_dir}/stdout.log" 2> "${lane_out_dir}/stderr.log"
  local actual_exit=$?
  set -e

  local scaffold_rows=0
  scaffold_rows="$(qa_truthfulness_count_scaffold_rows_in_dir "$lane_out_dir")"

  local expected_exit=0
  expected_exit="$(qa_truthfulness_expected_exit "$mode" "$scaffold_rows")"

  local tsv_file_count=0
  tsv_file_count="$(count_tsv_files_in_dir "$lane_out_dir")"

  echo "${actual_exit};${scaffold_rows};${expected_exit};${tsv_file_count};${lane_out_dir}"
}

evaluate_lane_run() {
  local lane="$1"
  local mode="$2"
  local run_index="$3"
  local triplet="$4"

  local actual_exit scaffold_rows expected_exit tsv_file_count lane_out_dir
  actual_exit="$(parse_triplet "$triplet" exit)"
  scaffold_rows="$(parse_triplet "$triplet" scaffold)"
  expected_exit="$(parse_triplet "$triplet" expected)"
  tsv_file_count="$(parse_triplet "$triplet" tsv_count)"
  lane_out_dir="$(parse_triplet "$triplet" dir)"

  local policy
  policy="$(qa_truthfulness_mode_policy "$mode")"

  local semantics_result="PASS"
  local semantics_detail="expected_exit=${expected_exit};actual_exit=${actual_exit};scaffold_rows=${scaffold_rows};tsv_file_count=${tsv_file_count}"

  if [[ "$tsv_file_count" -eq 0 ]]; then
    semantics_result="FAIL"
    semantics_detail="lane produced zero TSV artifacts"
  elif [[ "$actual_exit" -ne "$expected_exit" ]]; then
    semantics_result="FAIL"
    semantics_detail="mode-exit mismatch expected=${expected_exit} actual=${actual_exit} scaffold_rows=${scaffold_rows}"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "$lane" \
    "$mode" \
    "$expected_exit" \
    "$actual_exit" \
    "$scaffold_rows" \
    "$policy" \
    "$semantics_result" \
    "$semantics_detail" \
    "$lane_out_dir" \
    >> "$MODE_SEMANTICS_TSV"

  local enforce_result="PASS"
  local enforce_detail=""

  if [[ "$tsv_file_count" -eq 0 ]]; then
    enforce_result="FAIL"
    enforce_detail="no lane TSV artifacts; cannot enforce TODO/SCAFFOLD policy"
  elif [[ "$mode" == "execute" ]]; then
    if [[ "$scaffold_rows" -gt 0 ]]; then
      execute_scaffold_rows_total=$(( execute_scaffold_rows_total + scaffold_rows ))
    fi

    if [[ "$scaffold_rows" -gt 0 && "$actual_exit" -eq 0 ]]; then
      enforce_result="FAIL"
      enforce_detail="false_green: execute passed with TODO/SCAFFOLD rows=${scaffold_rows}"
    elif [[ "$scaffold_rows" -eq 0 && "$actual_exit" -ne 0 ]]; then
      enforce_result="FAIL"
      enforce_detail="false_red: execute failed with zero TODO/SCAFFOLD rows"
    elif [[ "$scaffold_rows" -gt 0 ]]; then
      enforce_detail="execute correctly failed with TODO/SCAFFOLD rows=${scaffold_rows}"
    else
      enforce_detail="execute correctly passed with zero TODO/SCAFFOLD rows"
    fi
  else
    if [[ "$actual_exit" -ne 0 ]]; then
      enforce_result="FAIL"
      enforce_detail="contract-only failed unexpectedly (exit=${actual_exit})"
    else
      enforce_detail="contract-only allows TODO/SCAFFOLD rows=${scaffold_rows}"
    fi
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "$lane" \
    "$mode" \
    "$scaffold_rows" \
    "$expected_exit" \
    "$actual_exit" \
    "$enforce_result" \
    "$enforce_detail" \
    "$lane_out_dir" \
    >> "$TODO_ENFORCEMENT_TSV"

  local reconciled="PASS"
  local reconcile_detail="mode_semantics=${semantics_result};enforcement=${enforce_result}"
  if [[ "$semantics_result" != "PASS" || "$enforce_result" != "PASS" ]]; then
    reconciled="FAIL"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "$lane" \
    "$mode" \
    "$tsv_file_count" \
    "$scaffold_rows" \
    "$expected_exit" \
    "$actual_exit" \
    "$reconciled" \
    "$reconcile_detail" \
    "$lane_out_dir" \
    >> "$MATRIX_RECONCILE_TSV"

  record "BL073-R${run_index}-${lane}-mode_semantics" "$semantics_result" "$semantics_detail" "$MODE_SEMANTICS_TSV"
  record "BL073-R${run_index}-${lane}-todo_enforcement" "$enforce_result" "$enforce_detail" "$TODO_ENFORCEMENT_TSV"
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
    --runs)
      [[ $# -ge 2 ]] || usage_error "--runs requires a value"
      RUNS="$2"
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

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  usage_error "--runs must be an integer >= 1"
fi

if [[ ! -f "$TRUTH_HELPER" ]]; then
  usage_error "required helper missing: $TRUTH_HELPER"
fi

# shellcheck source=/dev/null
source "$TRUTH_HELPER"

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
MODE_SEMANTICS_TSV="${OUT_DIR}/mode_semantics_contract.tsv"
TODO_ENFORCEMENT_TSV="${OUT_DIR}/todo_row_enforcement.tsv"
PROMOTION_POLICY_MD="${OUT_DIR}/promotion_gate_policy.md"
MATRIX_RECONCILE_TSV="${OUT_DIR}/bl067_bl068_matrix_reconcile.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_index\tlane\tmode\texpected_exit\tactual_exit\tscaffold_rows\tpolicy\tsemantics_result\tdetail\tartifact_dir\n" > "$MODE_SEMANTICS_TSV"
printf "run_index\tlane\tmode\tscaffold_rows\texpected_exit\tactual_exit\tenforcement_result\tdetail\tartifact_dir\n" > "$TODO_ENFORCEMENT_TSV"
printf "run_index\tlane\tmode\ttsv_file_count\tscaffold_rows\texpected_exit\tactual_exit\treconciled\tdetail\tartifact_dir\n" > "$MATRIX_RECONCILE_TSV"

BL067_SCRIPT="${ROOT_DIR}/scripts/qa-bl067-auv3-lifecycle-mac.sh"
BL068_SCRIPT="${ROOT_DIR}/scripts/qa-bl068-temporal-effects-mac.sh"

record "BL073-PRE-helper_exists" "PASS" "helper library loaded" "$TRUTH_HELPER"

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

for run_index in $(seq 1 "$RUNS"); do
  bl067_triplet="$(run_lane_mode "BL067" "$BL067_SCRIPT" "$MODE" "$run_index")"
  evaluate_lane_run "BL067" "$MODE" "$run_index" "$bl067_triplet"

  bl068_triplet="$(run_lane_mode "BL068" "$BL068_SCRIPT" "$MODE" "$run_index")"
  evaluate_lane_run "BL068" "$MODE" "$run_index" "$bl068_triplet"
done

if [[ "$MODE" == "execute" ]]; then
  if [[ "$execute_scaffold_rows_total" -gt 0 ]]; then
    record "BL073-GATE-execute_scaffold_blocker" "FAIL" \
      "execute mode blocked: TODO/SCAFFOLD rows detected across runs=${execute_scaffold_rows_total}" \
      "$TODO_ENFORCEMENT_TSV"
  else
    record "BL073-GATE-execute_scaffold_blocker" "PASS" \
      "execute mode clear: zero TODO/SCAFFOLD rows across all runs" \
      "$TODO_ENFORCEMENT_TSV"
  fi
fi

cat > "$PROMOTION_POLICY_MD" <<EOF_POLICY
# BL-073 Promotion Gate Policy

- Generated: ${TIMESTAMP}
- Scope: BL-067 and BL-068 execute-mode truthfulness
- Mode under test: ${MODE}
- Replay runs: ${RUNS}

## Exact Execute Failure Criteria

1. For each lane run, count rows in lane TSV artifacts where any cell is \`TODO\` or \`SCAFFOLD\`.
2. If any execute-mode run contains \`TODO\`/\`SCAFFOLD\` rows, BL-073 returns non-zero and blocks promotion.
3. In execute mode, \`scaffold_rows > 0\` and lane exit code \`0\` is a hard failure (false-green).
4. In execute mode, \`scaffold_rows = 0\` and lane exit code non-zero is a hard failure (false-red).
5. Execute-mode pass criteria requires both: zero scaffold rows across all runs and exit parity for each lane run.

## Contract-Only Semantics

1. Contract-only runs may contain \`TODO\`/\`SCAFFOLD\` rows.
2. Contract-only runs must still exit \`0\`.

## Promotion Packet Requirements

Promotion packets for BL-067/BL-068 must include:

- \`mode_semantics_contract.tsv\`
- \`todo_row_enforcement.tsv\`
- \`bl067_bl068_matrix_reconcile.tsv\`
- \`promotion_gate_policy.md\`
EOF_POLICY

record "BL073-POLICY-promotion_gate_policy" "PASS" "promotion blocker policy emitted" "$PROMOTION_POLICY_MD"

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "bl073_truthfulness_gates_passed mode=${MODE} runs=${RUNS}" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "bl073_truthfulness_gates_failed=${fail_count} mode=${MODE} runs=${RUNS}" "$STATUS_TSV"
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
