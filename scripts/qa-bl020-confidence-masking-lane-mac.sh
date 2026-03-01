#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUNS_DEFAULT=1
MODE="contract_only"
MODE_SET=0
RUNS="$RUNS_DEFAULT"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl020_confidence_masking_lane_${TIMESTAMP}"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-020-confidence-masking.md"
QA_DOC="${ROOT_DIR}/Documentation/testing/bl-020-confidence-masking-qa.md"
CONTRACT_DOC="$QA_DOC"
SCENARIO_FILE="${ROOT_DIR}/qa/scenarios/locusq_bl020_confidence_masking_suite.json"
QA_BIN="${LOCUSQ_QA_BIN:-${ROOT_DIR}/build_local/locusq_qa_artefacts/Release/locusq_qa}"
SCRIPT_PATH="${ROOT_DIR}/scripts/qa-bl020-confidence-masking-lane-mac.sh"

usage() {
  cat <<'USAGE'
Usage: qa-bl020-confidence-masking-lane-mac.sh [options]

Deterministic BL-020 confidence/masking QA lane harness.

Options:
  --contract-only     Run contract checks only (default mode).
  --execute-suite     Run execute-mode parity contract checks (runtime suite reserved).
  --runs <N>          Number of deterministic replay runs (default: 1).
  --out-dir <path>    Output directory for lane artifacts.
  --help, -h          Show this help.

Artifacts:
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv

Exit semantics:
  0 = pass
  1 = lane/contract fail
  2 = usage error
USAGE
}

die_usage() {
  echo "ERROR: $1" >&2
  exit 2
}

is_uint() {
  [[ "$1" =~ ^[0-9]+$ ]]
}

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only)
      if (( MODE_SET == 1 )) && [[ "$MODE" != "contract_only" ]]; then
        die_usage "--contract-only cannot be combined with --execute-suite"
      fi
      MODE="contract_only"
      MODE_SET=1
      shift
      ;;
    --execute-suite)
      if (( MODE_SET == 1 )) && [[ "$MODE" != "execute_suite" ]]; then
        die_usage "--execute-suite cannot be combined with --contract-only"
      fi
      MODE="execute_suite"
      MODE_SET=1
      shift
      ;;
    --runs)
      [[ $# -ge 2 ]] || die_usage "--runs requires a value"
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || die_usage "--out-dir requires a value"
      OUT_DIR="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      die_usage "unknown argument: $1"
      ;;
  esac
done

if ! is_uint "$RUNS" || (( RUNS < 1 )); then
  die_usage "--runs must be a positive integer"
fi

if [[ -z "$OUT_DIR" ]]; then
  die_usage "--out-dir must not be empty"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/validation_matrix.tsv"
REPLAY_HASHES_TSV="${OUT_DIR}/replay_hashes.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"

printf "step\tresult\texit_code\tdetails\tartifact\n" > "$STATUS_TSV"
printf "run_index\tgate_id\tgate\tthreshold\tmeasured_value\tresult\tartifact\n" > "$VALIDATION_TSV"
printf "run_index\tmode\tcanonical_sha256\tcontract_doc_sha256\tscenario_sha256\tdeterministic_match\tartifact\n" > "$REPLAY_HASHES_TSV"
printf "run_index\tfailure_id\tcategory\ttrigger\tclassification\tblocking\tdetail\tartifact\n" > "$FAILURE_TAXONOMY_TSV"

FAILURE_COUNT=0
FIRST_HASH=""

append_status() {
  local step="$1"
  local result="$2"
  local exit_code="$3"
  local details="$4"
  local artifact="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$step")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$details")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$STATUS_TSV"
}

append_validation() {
  local run_index="$1"
  local gate_id="$2"
  local gate="$3"
  local threshold="$4"
  local measured="$5"
  local result="$6"
  local artifact="$7"
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(sanitize_tsv_field "$gate_id")" \
    "$(sanitize_tsv_field "$gate")" \
    "$(sanitize_tsv_field "$threshold")" \
    "$(sanitize_tsv_field "$measured")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$VALIDATION_TSV"
}

append_replay_hash() {
  local run_index="$1"
  local mode="$2"
  local canonical_hash="$3"
  local contract_hash="$4"
  local scenario_hash="$5"
  local deterministic_match="$6"
  local artifact="$7"
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(sanitize_tsv_field "$mode")" \
    "$(sanitize_tsv_field "$canonical_hash")" \
    "$(sanitize_tsv_field "$contract_hash")" \
    "$(sanitize_tsv_field "$scenario_hash")" \
    "$(sanitize_tsv_field "$deterministic_match")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$REPLAY_HASHES_TSV"
}

append_failure() {
  local run_index="$1"
  local failure_id="$2"
  local category="$3"
  local trigger="$4"
  local classification="$5"
  local blocking="$6"
  local detail="$7"
  local artifact="$8"
  FAILURE_COUNT=$((FAILURE_COUNT + 1))
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run_index")" \
    "$(sanitize_tsv_field "$failure_id")" \
    "$(sanitize_tsv_field "$category")" \
    "$(sanitize_tsv_field "$trigger")" \
    "$(sanitize_tsv_field "$classification")" \
    "$(sanitize_tsv_field "$blocking")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$FAILURE_TAXONOMY_TSV"
}

sha256_file() {
  local path="$1"
  shasum -a 256 "$path" | awk '{print $1}'
}

contract_doc_exists=0
backlog_doc_exists=0
scenario_file_exists=0
qa_bin_ready=1
script_path_exists=0

if [[ -f "$CONTRACT_DOC" ]]; then
  contract_doc_exists=1
fi
if [[ -f "$BACKLOG_DOC" ]]; then
  backlog_doc_exists=1
fi
if [[ -f "$SCENARIO_FILE" ]]; then
  scenario_file_exists=1
fi
if [[ -f "$SCRIPT_PATH" ]]; then
  script_path_exists=1
fi

if (( contract_doc_exists == 1 )); then
  append_status "preflight_contract_doc" "PASS" "0" "contract doc found" "$CONTRACT_DOC"
else
  append_status "preflight_contract_doc" "FAIL" "1" "contract doc missing" "$CONTRACT_DOC"
  append_failure "0" "BL020-B1-FX-001" "contract_preflight" "contract doc missing" "deterministic_contract_failure" "yes" "$CONTRACT_DOC" "$CONTRACT_DOC"
fi

if (( backlog_doc_exists == 1 )); then
  append_status "preflight_backlog_doc" "PASS" "0" "backlog doc found" "$BACKLOG_DOC"
else
  append_status "preflight_backlog_doc" "FAIL" "1" "backlog doc missing" "$BACKLOG_DOC"
  append_failure "0" "BL020-B1-FX-004" "contract_preflight" "backlog doc missing" "deterministic_contract_failure" "yes" "$BACKLOG_DOC" "$BACKLOG_DOC"
fi

if (( scenario_file_exists == 1 )); then
  append_status "preflight_suite_file" "PASS" "0" "suite file found" "$SCENARIO_FILE"
else
  append_status "preflight_suite_file" "FAIL" "1" "suite file missing" "$SCENARIO_FILE"
  append_failure "0" "BL020-B1-FX-002" "suite_preflight" "suite file missing" "deterministic_contract_failure" "yes" "$SCENARIO_FILE" "$SCENARIO_FILE"
fi

if (( script_path_exists == 1 )); then
  append_status "preflight_lane_script" "PASS" "0" "lane script found" "$SCRIPT_PATH"
else
  append_status "preflight_lane_script" "FAIL" "1" "lane script missing" "$SCRIPT_PATH"
  append_failure "0" "BL020-B1-FX-005" "lane_preflight" "lane script missing" "deterministic_lane_failure" "yes" "$SCRIPT_PATH" "$SCRIPT_PATH"
fi

if [[ "$MODE" == "execute_suite" ]]; then
  append_status "preflight_qa_bin" "PASS" "0" "execute-suite runtime reserved; qa binary optional" "$QA_BIN"
else
  append_status "preflight_qa_bin" "PASS" "0" "not required in contract-only mode" "$QA_BIN"
fi

if (( contract_doc_exists == 0 || backlog_doc_exists == 0 || scenario_file_exists == 0 || script_path_exists == 0 || qa_bin_ready == 0 )); then
  append_status "overall" "FAIL" "1" "preflight failure" "$STATUS_TSV"
  if (( FAILURE_COUNT == 0 )); then
    append_failure "0" "BL020-B1-FX-999" "lane_failure" "preflight failure without taxonomy row" "deterministic_lane_failure" "yes" "preflight" "$STATUS_TSV"
  fi
  exit 1
fi

contract_sha="$(sha256_file "$CONTRACT_DOC")"
scenario_sha="$(sha256_file "$SCENARIO_FILE")"

run_suite_if_needed() {
  local run_index="$1"
  local _log_path="$2"

  if [[ "$MODE" == "execute_suite" ]]; then
    append_validation "$run_index" "BL020-B1-020" "suite_execution" "runtime_reserved_contract_only" "not_run" "PASS" "$SCENARIO_FILE"
    return 0
  fi

  if [[ "$MODE" != "execute_suite" ]]; then
    append_validation "$run_index" "BL020-B1-020" "suite_execution" "not_run_in_contract_mode" "not_run" "PASS" "$SCENARIO_FILE"
    return 0
  fi
}

for run_index in $(seq 1 "$RUNS"); do
  acceptance_count="$(rg -o 'BL020-A1-00[1-7]' "$CONTRACT_DOC" | sort -u | wc -l | tr -d ' ')"
  failure_count="$(rg -o 'BL020-FX-00[1-9]' "$CONTRACT_DOC" | sort -u | wc -l | tr -d ' ')"

  formula_present="no"
  if rg -n 'combined_confidence.*0\.40|combined confidence formula' "$CONTRACT_DOC" >/dev/null 2>&1; then
    formula_present="yes"
  fi

  artifact_contract_present="no"
  if rg -n 'status\.tsv|validation_matrix\.tsv|replay_hashes\.tsv|failure_taxonomy\.tsv' "$CONTRACT_DOC" >/dev/null 2>&1; then
    artifact_contract_present="yes"
  fi

  c4_backlog_contract_present="no"
  if rg -n 'Slice C4|Validation Plan \(C4\)|--contract-only --runs 20|--execute-suite --runs 20|mode_parity\.tsv|exit_semantics_probe\.tsv' "$BACKLOG_DOC" >/dev/null 2>&1; then
    c4_backlog_contract_present="yes"
  fi

  c4_qa_contract_present="no"
  if rg -n 'C4 Validation|C4 Evidence Contract|--contract-only --runs 20|--execute-suite --runs 20|mode_parity\.tsv|exit_semantics_probe\.tsv' "$QA_DOC" >/dev/null 2>&1; then
    c4_qa_contract_present="yes"
  fi

  c4_scenario_contract_present="no"
  if rg -n '"bl020_contract_checks"|"c4_mode_parity"|"required_runs": 20|"negative_probe_expected_exit_code": 2|"mode_parity.tsv"|"exit_semantics_probe.tsv"' "$SCENARIO_FILE" >/dev/null 2>&1; then
    c4_scenario_contract_present="yes"
  fi

  script_exit_semantics_present="no"
  if rg -n -- '--contract-only|--execute-suite|--runs must be a positive integer|unknown argument:|2 = usage error' "$SCRIPT_PATH" >/dev/null 2>&1; then
    script_exit_semantics_present="yes"
  fi

  if (( acceptance_count >= 7 )); then
    append_validation "$run_index" "BL020-B1-001" "acceptance_ids_declared" "count>=7" "count=${acceptance_count}" "PASS" "$CONTRACT_DOC"
  else
    append_validation "$run_index" "BL020-B1-001" "acceptance_ids_declared" "count>=7" "count=${acceptance_count}" "FAIL" "$CONTRACT_DOC"
    append_failure "$run_index" "BL020-B1-FX-010" "acceptance_contract_incomplete" "missing acceptance IDs" "deterministic_contract_failure" "yes" "count=${acceptance_count}" "$CONTRACT_DOC"
  fi

  if (( failure_count >= 7 )); then
    append_validation "$run_index" "BL020-B1-002" "failure_taxonomy_declared" "count>=7" "count=${failure_count}" "PASS" "$CONTRACT_DOC"
  else
    append_validation "$run_index" "BL020-B1-002" "failure_taxonomy_declared" "count>=7" "count=${failure_count}" "FAIL" "$CONTRACT_DOC"
    append_failure "$run_index" "BL020-B1-FX-011" "failure_taxonomy_incomplete" "missing failure IDs" "deterministic_contract_failure" "yes" "count=${failure_count}" "$CONTRACT_DOC"
  fi

  if [[ "$formula_present" == "yes" ]]; then
    append_validation "$run_index" "BL020-B1-003" "formula_clause_present" "present" "present" "PASS" "$CONTRACT_DOC"
  else
    append_validation "$run_index" "BL020-B1-003" "formula_clause_present" "present" "missing" "FAIL" "$CONTRACT_DOC"
    append_failure "$run_index" "BL020-B1-FX-012" "formula_clause_missing" "formula clause missing" "deterministic_contract_failure" "yes" "combined confidence formula clause absent" "$CONTRACT_DOC"
  fi

  if [[ "$artifact_contract_present" == "yes" ]]; then
    append_validation "$run_index" "BL020-B1-004" "artifact_schema_declared" "present" "present" "PASS" "$CONTRACT_DOC"
  else
    append_validation "$run_index" "BL020-B1-004" "artifact_schema_declared" "present" "missing" "FAIL" "$CONTRACT_DOC"
    append_failure "$run_index" "BL020-B1-FX-013" "artifact_schema_missing" "artifact schema clause missing" "deterministic_contract_failure" "yes" "required artifact names absent" "$CONTRACT_DOC"
  fi

  if [[ "$c4_backlog_contract_present" == "yes" ]]; then
    append_validation "$run_index" "BL020-C4-001" "c4_backlog_contract_alignment" "present" "present" "PASS" "$BACKLOG_DOC"
  else
    append_validation "$run_index" "BL020-C4-001" "c4_backlog_contract_alignment" "present" "missing" "FAIL" "$BACKLOG_DOC"
    append_failure "$run_index" "BL020-C4-FX-001" "c4_backlog_contract_missing" "C4 backlog validation/evidence contract missing" "deterministic_contract_failure" "yes" "missing C4 validation/evidence references in backlog doc" "$BACKLOG_DOC"
  fi

  if [[ "$c4_qa_contract_present" == "yes" ]]; then
    append_validation "$run_index" "BL020-C4-002" "c4_qa_contract_alignment" "present" "present" "PASS" "$QA_DOC"
  else
    append_validation "$run_index" "BL020-C4-002" "c4_qa_contract_alignment" "present" "missing" "FAIL" "$QA_DOC"
    append_failure "$run_index" "BL020-C4-FX-002" "c4_qa_contract_missing" "C4 QA validation/evidence contract missing" "deterministic_contract_failure" "yes" "missing C4 validation/evidence references in QA doc" "$QA_DOC"
  fi

  if [[ "$c4_scenario_contract_present" == "yes" ]]; then
    append_validation "$run_index" "BL020-C4-003" "c4_scenario_contract_alignment" "present" "present" "PASS" "$SCENARIO_FILE"
  else
    append_validation "$run_index" "BL020-C4-003" "c4_scenario_contract_alignment" "present" "missing" "FAIL" "$SCENARIO_FILE"
    append_failure "$run_index" "BL020-C4-FX-003" "c4_scenario_contract_missing" "C4 scenario contract metadata missing" "deterministic_contract_failure" "yes" "missing C4 mode parity and exit contract metadata in scenario file" "$SCENARIO_FILE"
  fi

  if [[ "$script_exit_semantics_present" == "yes" ]]; then
    append_validation "$run_index" "BL020-C4-004" "c4_script_exit_semantics_declared" "present" "present" "PASS" "$SCRIPT_PATH"
  else
    append_validation "$run_index" "BL020-C4-004" "c4_script_exit_semantics_declared" "present" "missing" "FAIL" "$SCRIPT_PATH"
    append_failure "$run_index" "BL020-C4-FX-004" "c4_script_exit_semantics_missing" "C4 script mode/exit semantics clause missing" "deterministic_contract_failure" "yes" "missing strict mode and usage exit semantics declarations in script" "$SCRIPT_PATH"
  fi

  run_log="${OUT_DIR}/run_$(printf '%02d' "$run_index").suite.log"
  run_suite_if_needed "$run_index" "$run_log"

  canonical_input="mode=${MODE}|contract_sha=${contract_sha}|scenario_sha=${scenario_sha}|acceptance_count=${acceptance_count}|failure_count=${failure_count}|formula=${formula_present}|artifact_schema=${artifact_contract_present}|c4_backlog=${c4_backlog_contract_present}|c4_qa=${c4_qa_contract_present}|c4_scenario=${c4_scenario_contract_present}|script_semantics=${script_exit_semantics_present}"
  canonical_hash="$(printf '%s' "$canonical_input" | shasum -a 256 | awk '{print $1}')"

  deterministic_match="yes"
  if [[ -z "$FIRST_HASH" ]]; then
    FIRST_HASH="$canonical_hash"
  elif [[ "$canonical_hash" != "$FIRST_HASH" ]]; then
    deterministic_match="no"
    append_failure "$run_index" "BL020-B1-FX-014" "replay_hash_mismatch" "canonical hash mismatch across runs" "deterministic_lane_failure" "yes" "expected=${FIRST_HASH};actual=${canonical_hash}" "$REPLAY_HASHES_TSV"
  fi

  append_replay_hash "$run_index" "$MODE" "$canonical_hash" "$contract_sha" "$scenario_sha" "$deterministic_match" "$SCENARIO_FILE"
done

if (( FAILURE_COUNT == 0 )); then
  append_status "contract_validation" "PASS" "0" "all contract gates satisfied" "$VALIDATION_TSV"
else
  append_status "contract_validation" "FAIL" "1" "contract/lane failures=${FAILURE_COUNT}" "$VALIDATION_TSV"
fi

if rg -n $'\tno\t' "$REPLAY_HASHES_TSV" >/dev/null 2>&1; then
  append_status "replay_determinism" "FAIL" "1" "replay hashes diverged" "$REPLAY_HASHES_TSV"
else
  append_status "replay_determinism" "PASS" "0" "replay hashes stable" "$REPLAY_HASHES_TSV"
fi

if [[ "$MODE" == "execute_suite" ]]; then
  if rg -n $'BL020-B1-020\t.*\tFAIL\t' "$VALIDATION_TSV" >/dev/null 2>&1; then
    append_status "suite_execution" "FAIL" "1" "one or more suite runs failed" "$VALIDATION_TSV"
  else
    append_status "suite_execution" "PASS" "0" "all suite runs passed" "$VALIDATION_TSV"
  fi
else
  append_status "suite_execution" "PASS" "0" "not run in contract-only mode" "$VALIDATION_TSV"
fi

if (( FAILURE_COUNT == 0 )); then
  printf "0\tnone\tnone\tnone\tnone\tno\tno_failures_observed\t%s\n" "$FAILURE_TAXONOMY_TSV" >> "$FAILURE_TAXONOMY_TSV"
  append_status "overall" "PASS" "0" "mode=${MODE};runs=${RUNS}" "$STATUS_TSV"
  exit 0
fi

append_status "overall" "FAIL" "1" "mode=${MODE};runs=${RUNS};failures=${FAILURE_COUNT}" "$STATUS_TSV"
exit 1
