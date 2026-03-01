#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUNS_DEFAULT=1
MODE="contract_only"
MODE_SET=0
RUNS="$RUNS_DEFAULT"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl044_quality_tier_switch_lane_${TIMESTAMP}"

RUNBOOK_DOC="${ROOT_DIR}/Documentation/backlog/bl-044-quality-tier-seamless-switching.md"
QA_DOC="${ROOT_DIR}/Documentation/testing/bl-044-quality-tier-seamless-switching-qa.md"
SCENARIO_FILE="${ROOT_DIR}/qa/scenarios/locusq_bl044_quality_tier_switch_suite.json"
QA_BIN="${LOCUSQ_QA_BIN:-${ROOT_DIR}/build_local/locusq_qa_artefacts/Release/locusq_qa}"

EXIT_PASS=0
EXIT_FAIL=1
EXIT_USAGE=2

usage() {
  cat <<'USAGE'
Usage: qa-bl044-quality-tier-switch-lane-mac.sh [options]

Deterministic BL-044 quality-tier switching contract lane.

Options:
  --contract-only     Run contract checks only (default mode).
  --execute-suite     Run contract checks and execute QA scenario suite.
  --runs <N>          Number of deterministic replay runs (default: 1).
  --out-dir <path>    Output directory for lane artifacts.
  --help, -h          Show this help.

Artifacts:
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv
  replay_sentinel_summary.tsv

Exit semantics:
  0 = pass
  1 = lane/contract fail
  2 = usage error
USAGE
}

die_usage() {
  echo "ERROR: $1" >&2
  exit "$EXIT_USAGE"
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
      exit "$EXIT_PASS"
      ;;
    *)
      die_usage "unknown argument: $1"
      ;;
  esac
done

if ! is_uint "$RUNS" || (( RUNS < 1 )); then
  die_usage "--runs must be a positive integer"
fi

[[ -n "$OUT_DIR" ]] || die_usage "--out-dir must not be empty"
mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/validation_matrix.tsv"
REPLAY_HASHES_TSV="${OUT_DIR}/replay_hashes.tsv"
FAILURE_TAXONOMY_TSV="${OUT_DIR}/failure_taxonomy.tsv"
REPLAY_SENTINEL_SUMMARY_TSV="${OUT_DIR}/replay_sentinel_summary.tsv"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_index\tgate_id\tgate\tthreshold\tmeasured_value\tresult\tartifact\n" > "$VALIDATION_TSV"
printf "run_index\tmode\tcanonical_sha256\trunbook_sha256\tqa_doc_sha256\tscenario_sha256\tdeterministic_match\tartifact\n" > "$REPLAY_HASHES_TSV"
printf "run_index\tfailure_id\tcategory\ttrigger\tclassification\tblocking\tdetail\tartifact\n" > "$FAILURE_TAXONOMY_TSV"
printf "mode\truns\tdeterministic_matches\tdeterministic_mismatches\tunique_canonical_hashes\tvalidation_pass_rows\tvalidation_fail_rows\tcontract_lane_result\treplay_determinism_result\tsuite_execution_result\tresult\tbaseline_canonical_sha256\tartifact\n" > "$REPLAY_SENTINEL_SUMMARY_TSV"

FAILURE_COUNT=0
FIRST_HASH=""

append_status() {
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$1")" \
    "$(sanitize_tsv_field "$2")" \
    "$(sanitize_tsv_field "$3")" \
    "$(sanitize_tsv_field "$4")" \
    "$(sanitize_tsv_field "$5")" \
    >> "$STATUS_TSV"
}

append_validation() {
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$1")" \
    "$(sanitize_tsv_field "$2")" \
    "$(sanitize_tsv_field "$3")" \
    "$(sanitize_tsv_field "$4")" \
    "$(sanitize_tsv_field "$5")" \
    "$(sanitize_tsv_field "$6")" \
    "$(sanitize_tsv_field "$7")" \
    >> "$VALIDATION_TSV"
}

append_replay_hash() {
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$1")" \
    "$(sanitize_tsv_field "$2")" \
    "$(sanitize_tsv_field "$3")" \
    "$(sanitize_tsv_field "$4")" \
    "$(sanitize_tsv_field "$5")" \
    "$(sanitize_tsv_field "$6")" \
    "$(sanitize_tsv_field "$7")" \
    "$(sanitize_tsv_field "$8")" \
    >> "$REPLAY_HASHES_TSV"
}

append_failure() {
  FAILURE_COUNT=$((FAILURE_COUNT + 1))
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$1")" \
    "$(sanitize_tsv_field "$2")" \
    "$(sanitize_tsv_field "$3")" \
    "$(sanitize_tsv_field "$4")" \
    "$(sanitize_tsv_field "$5")" \
    "$(sanitize_tsv_field "$6")" \
    "$(sanitize_tsv_field "$7")" \
    "$(sanitize_tsv_field "$8")" \
    >> "$FAILURE_TAXONOMY_TSV"
}

sha256_file() {
  shasum -a 256 "$1" | awk '{print $1}'
}

runbook_exists=0
qa_doc_exists=0
scenario_exists=0
qa_bin_ready=1

[[ -f "$RUNBOOK_DOC" ]] && runbook_exists=1
[[ -f "$QA_DOC" ]] && qa_doc_exists=1
[[ -f "$SCENARIO_FILE" ]] && scenario_exists=1
if [[ "$MODE" == "execute_suite" && ! -x "$QA_BIN" ]]; then
  qa_bin_ready=0
fi

if (( runbook_exists == 1 )); then
  append_status "preflight_runbook_doc" "PASS" "0" "runbook doc found" "$RUNBOOK_DOC"
else
  append_status "preflight_runbook_doc" "FAIL" "1" "runbook doc missing" "$RUNBOOK_DOC"
  append_failure "0" "BL044-B1-FX-001" "contract_preflight" "runbook doc missing" "deterministic_contract_failure" "yes" "$RUNBOOK_DOC" "$RUNBOOK_DOC"
fi

if (( qa_doc_exists == 1 )); then
  append_status "preflight_qa_doc" "PASS" "0" "qa doc found" "$QA_DOC"
else
  append_status "preflight_qa_doc" "FAIL" "1" "qa doc missing" "$QA_DOC"
  append_failure "0" "BL044-B1-FX-002" "contract_preflight" "qa doc missing" "deterministic_contract_failure" "yes" "$QA_DOC" "$QA_DOC"
fi

if (( scenario_exists == 1 )); then
  append_status "preflight_scenario" "PASS" "0" "scenario file found" "$SCENARIO_FILE"
else
  append_status "preflight_scenario" "FAIL" "1" "scenario file missing" "$SCENARIO_FILE"
  append_failure "0" "BL044-B1-FX-003" "scenario_preflight" "scenario file missing" "deterministic_contract_failure" "yes" "$SCENARIO_FILE" "$SCENARIO_FILE"
fi

if [[ "$MODE" == "execute_suite" ]]; then
  if (( qa_bin_ready == 1 )); then
    append_status "preflight_qa_bin" "PASS" "0" "qa binary executable" "$QA_BIN"
  else
    append_status "preflight_qa_bin" "FAIL" "1" "qa binary not executable" "$QA_BIN"
    append_failure "0" "BL044-B1-FX-004" "suite_preflight" "qa binary not executable" "deterministic_lane_failure" "yes" "$QA_BIN" "$QA_BIN"
  fi
else
  append_status "preflight_qa_bin" "PASS" "0" "not required in contract-only mode" "$QA_BIN"
fi

if (( runbook_exists == 0 || qa_doc_exists == 0 || scenario_exists == 0 || qa_bin_ready == 0 )); then
  append_status "overall" "FAIL" "1" "preflight failure" "$STATUS_TSV"
  exit "$EXIT_FAIL"
fi

runbook_sha="$(sha256_file "$RUNBOOK_DOC")"
qa_doc_sha="$(sha256_file "$QA_DOC")"
scenario_sha="$(sha256_file "$SCENARIO_FILE")"

run_suite_if_needed() {
  local run_index="$1"
  local log_path="$2"
  local exit_code="0"

  if [[ "$MODE" != "execute_suite" ]]; then
    append_validation "$run_index" "BL044-B1-020" "suite_execution" "not_run_in_contract_mode" "not_run" "PASS" "$SCENARIO_FILE"
    return 0
  fi

  set +e
  "$QA_BIN" "$SCENARIO_FILE" > "$log_path" 2>&1
  exit_code=$?
  set -e

  if [[ "$exit_code" == "0" ]]; then
    append_validation "$run_index" "BL044-B1-020" "suite_execution" "exit_code=0" "exit_code=0" "PASS" "$log_path"
  else
    append_validation "$run_index" "BL044-B1-020" "suite_execution" "exit_code=0" "exit_code=${exit_code}" "FAIL" "$log_path"
    append_failure "$run_index" "BL044-B1-FX-020" "suite_execution_failed" "qa suite non-zero exit" "deterministic_lane_failure" "yes" "exit_code=${exit_code}" "$log_path"
  fi
}

for run_index in $(seq 1 "$RUNS"); do
  a1_acceptance_count="$(rg -o 'BL044-A1-00[1-9]' "$RUNBOOK_DOC" "$QA_DOC" | sort -u | wc -l | tr -d ' ')"
  fx_count="$(rg -o 'BL044-FX-00[1-9]' "$RUNBOOK_DOC" "$QA_DOC" | sort -u | wc -l | tr -d ' ')"
  b1_fx_count="$(rg -o 'BL044-B1-FX-0[0-9]{2}' "$RUNBOOK_DOC" "$QA_DOC" | sort -u | wc -l | tr -d ' ')"

  switch_fields_present="yes"
  for tok in quality_tier_requested quality_tier_active quality_tier_switch_state quality_tier_switch_crossfade_samples quality_tier_switch_latency_samples quality_tier_switch_fallback_reason; do
    if ! rg -n "$tok" "$RUNBOOK_DOC" >/dev/null 2>&1; then
      switch_fields_present="no"
      break
    fi
  done

  continuity_clause_present="yes"
  for tok in switch_peak_discontinuity_abs switch_rms_step_delta switch_completion_latency_samples switch_non_finite_count; do
    if ! rg -n "$tok" "$RUNBOOK_DOC" >/dev/null 2>&1; then
      continuity_clause_present="no"
      break
    fi
  done

  fallback_clause_present="yes"
  for tok in invalid_requested_tier crossfade_window_invalid latency_budget_exceeded non_finite_detected contract_violation; do
    if ! rg -n "$tok" "$RUNBOOK_DOC" >/dev/null 2>&1; then
      fallback_clause_present="no"
      break
    fi
  done

  replay_clause_present="no"
  if rg -n 'Required replay hash inputs|Determinism rule' "$RUNBOOK_DOC" "$QA_DOC" >/dev/null 2>&1; then
    replay_clause_present="yes"
  fi

  artifact_schema_present="no"
  if rg -n 'status\.tsv|validation_matrix\.tsv|replay_hashes\.tsv|failure_taxonomy\.tsv' "$QA_DOC" >/dev/null 2>&1; then
    artifact_schema_present="yes"
  fi

  if (( a1_acceptance_count >= 8 )); then
    append_validation "$run_index" "BL044-B1-001" "acceptance_ids_declared" "count>=8" "count=${a1_acceptance_count}" "PASS" "$RUNBOOK_DOC"
  else
    append_validation "$run_index" "BL044-B1-001" "acceptance_ids_declared" "count>=8" "count=${a1_acceptance_count}" "FAIL" "$RUNBOOK_DOC"
    append_failure "$run_index" "BL044-B1-FX-010" "acceptance_contract_incomplete" "missing A1 acceptance IDs" "deterministic_contract_failure" "yes" "count=${a1_acceptance_count}" "$RUNBOOK_DOC"
  fi

  if (( fx_count >= 8 )); then
    append_validation "$run_index" "BL044-B1-002" "failure_taxonomy_declared" "count>=8" "count=${fx_count}" "PASS" "$RUNBOOK_DOC"
  else
    append_validation "$run_index" "BL044-B1-002" "failure_taxonomy_declared" "count>=8" "count=${fx_count}" "FAIL" "$RUNBOOK_DOC"
    append_failure "$run_index" "BL044-B1-FX-011" "failure_taxonomy_incomplete" "missing BL044-FX IDs" "deterministic_contract_failure" "yes" "count=${fx_count}" "$RUNBOOK_DOC"
  fi

  if [[ "$switch_fields_present" == "yes" ]]; then
    append_validation "$run_index" "BL044-B1-003" "switch_field_clause_present" "present" "present" "PASS" "$RUNBOOK_DOC"
  else
    append_validation "$run_index" "BL044-B1-003" "switch_field_clause_present" "present" "missing" "FAIL" "$RUNBOOK_DOC"
    append_failure "$run_index" "BL044-B1-FX-012" "switch_field_clause_missing" "switch state field clause missing" "deterministic_contract_failure" "yes" "required switch fields absent" "$RUNBOOK_DOC"
  fi

  if [[ "$continuity_clause_present" == "yes" ]]; then
    append_validation "$run_index" "BL044-B1-004" "continuity_clause_present" "present" "present" "PASS" "$RUNBOOK_DOC"
  else
    append_validation "$run_index" "BL044-B1-004" "continuity_clause_present" "present" "missing" "FAIL" "$RUNBOOK_DOC"
    append_failure "$run_index" "BL044-B1-FX-013" "continuity_clause_missing" "continuity threshold clause missing" "deterministic_contract_failure" "yes" "required continuity fields absent" "$RUNBOOK_DOC"
  fi

  if [[ "$fallback_clause_present" == "yes" ]]; then
    append_validation "$run_index" "BL044-B1-005" "fallback_clause_present" "present" "present" "PASS" "$RUNBOOK_DOC"
  else
    append_validation "$run_index" "BL044-B1-005" "fallback_clause_present" "present" "missing" "FAIL" "$RUNBOOK_DOC"
    append_failure "$run_index" "BL044-B1-FX-014" "fallback_clause_missing" "fallback token clause missing" "deterministic_contract_failure" "yes" "required fallback tokens absent" "$RUNBOOK_DOC"
  fi

  if [[ "$replay_clause_present" == "yes" ]]; then
    append_validation "$run_index" "BL044-B1-006" "replay_clause_present" "present" "present" "PASS" "$QA_DOC"
  else
    append_validation "$run_index" "BL044-B1-006" "replay_clause_present" "present" "missing" "FAIL" "$QA_DOC"
    append_failure "$run_index" "BL044-B1-FX-015" "replay_clause_missing" "replay determinism clause missing" "deterministic_contract_failure" "yes" "replay clause absent" "$QA_DOC"
  fi

  if [[ "$artifact_schema_present" == "yes" ]]; then
    append_validation "$run_index" "BL044-B1-007" "artifact_schema_declared" "present" "present" "PASS" "$QA_DOC"
  else
    append_validation "$run_index" "BL044-B1-007" "artifact_schema_declared" "present" "missing" "FAIL" "$QA_DOC"
    append_failure "$run_index" "BL044-B1-FX-016" "artifact_schema_missing" "artifact schema clause missing" "deterministic_contract_failure" "yes" "required artifact names absent" "$QA_DOC"
  fi

  if (( b1_fx_count >= 8 )); then
    append_validation "$run_index" "BL044-B1-008" "lane_failure_taxonomy_declared" "count>=8" "count=${b1_fx_count}" "PASS" "$QA_DOC"
  else
    append_validation "$run_index" "BL044-B1-008" "lane_failure_taxonomy_declared" "count>=8" "count=${b1_fx_count}" "FAIL" "$QA_DOC"
    append_failure "$run_index" "BL044-B1-FX-017" "lane_taxonomy_incomplete" "missing BL044-B1-FX IDs" "deterministic_contract_failure" "yes" "count=${b1_fx_count}" "$QA_DOC"
  fi

  run_log="${OUT_DIR}/run_$(printf '%02d' "$run_index").suite.log"
  run_suite_if_needed "$run_index" "$run_log"

  canonical_input="mode=${MODE}|runbook_sha=${runbook_sha}|qa_doc_sha=${qa_doc_sha}|scenario_sha=${scenario_sha}|a1_acceptance_count=${a1_acceptance_count}|fx_count=${fx_count}|b1_fx_count=${b1_fx_count}|switch_fields=${switch_fields_present}|continuity=${continuity_clause_present}|fallback=${fallback_clause_present}|replay=${replay_clause_present}|artifact_schema=${artifact_schema_present}"
  canonical_hash="$(printf '%s' "$canonical_input" | shasum -a 256 | awk '{print $1}')"

  deterministic_match="yes"
  if [[ -z "$FIRST_HASH" ]]; then
    FIRST_HASH="$canonical_hash"
  elif [[ "$canonical_hash" != "$FIRST_HASH" ]]; then
    deterministic_match="no"
    append_failure "$run_index" "BL044-B1-FX-018" "replay_hash_mismatch" "canonical hash mismatch across runs" "deterministic_lane_failure" "yes" "expected=${FIRST_HASH};actual=${canonical_hash}" "$REPLAY_HASHES_TSV"
  fi

  if [[ "$deterministic_match" == "yes" ]]; then
    append_validation "$run_index" "BL044-B1-009" "replay_hash_stability" "stable" "stable" "PASS" "$REPLAY_HASHES_TSV"
  else
    append_validation "$run_index" "BL044-B1-009" "replay_hash_stability" "stable" "diverged" "FAIL" "$REPLAY_HASHES_TSV"
  fi

  append_replay_hash "$run_index" "$MODE" "$canonical_hash" "$runbook_sha" "$qa_doc_sha" "$scenario_sha" "$deterministic_match" "$SCENARIO_FILE"
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
  if rg -n $'BL044-B1-020\t.*\tFAIL\t' "$VALIDATION_TSV" >/dev/null 2>&1; then
    append_status "suite_execution" "FAIL" "1" "one or more suite runs failed" "$VALIDATION_TSV"
  else
    append_status "suite_execution" "PASS" "0" "all suite runs passed" "$VALIDATION_TSV"
  fi
else
  append_status "suite_execution" "PASS" "0" "not run in contract-only mode" "$VALIDATION_TSV"
fi

deterministic_matches="$(awk -F'\t' 'NR>1 && $7=="yes" {n++} END {print n+0}' "$REPLAY_HASHES_TSV")"
deterministic_mismatches="$(awk -F'\t' 'NR>1 && $7=="no" {n++} END {print n+0}' "$REPLAY_HASHES_TSV")"
unique_canonical_hashes="$(awk -F'\t' 'NR>1 {print $3}' "$REPLAY_HASHES_TSV" | sort -u | awk 'END {print NR+0}')"
validation_pass_rows="$(awk -F'\t' 'NR>1 && $6=="PASS" {n++} END {print n+0}' "$VALIDATION_TSV")"
validation_fail_rows="$(awk -F'\t' 'NR>1 && $6=="FAIL" {n++} END {print n+0}' "$VALIDATION_TSV")"
contract_lane_result="$(awk -F'\t' '$1=="contract_validation" {print $2}' "$STATUS_TSV" | tail -n1)"
replay_determinism_result="$(awk -F'\t' '$1=="replay_determinism" {print $2}' "$STATUS_TSV" | tail -n1)"
suite_execution_result="$(awk -F'\t' '$1=="suite_execution" {print $2}' "$STATUS_TSV" | tail -n1)"
baseline_hash="$(awk -F'\t' 'NR==2 {print $3}' "$REPLAY_HASHES_TSV")"

if (( FAILURE_COUNT == 0 )); then
  printf "0\tnone\tnone\tnone\tnone\tno\tno_failures_observed\t%s\n" "$FAILURE_TAXONOMY_TSV" >> "$FAILURE_TAXONOMY_TSV"
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$MODE" "$RUNS" "$deterministic_matches" "$deterministic_mismatches" "$unique_canonical_hashes" "$validation_pass_rows" "$validation_fail_rows" \
    "${contract_lane_result:-PASS}" "${replay_determinism_result:-PASS}" "${suite_execution_result:-PASS}" "PASS" "${baseline_hash}" "$REPLAY_HASHES_TSV" >> "$REPLAY_SENTINEL_SUMMARY_TSV"
  append_status "overall" "PASS" "0" "mode=${MODE};runs=${RUNS}" "$STATUS_TSV"
  exit "$EXIT_PASS"
fi

printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
  "$MODE" "$RUNS" "$deterministic_matches" "$deterministic_mismatches" "$unique_canonical_hashes" "$validation_pass_rows" "$validation_fail_rows" \
  "${contract_lane_result:-FAIL}" "${replay_determinism_result:-FAIL}" "${suite_execution_result:-FAIL}" "FAIL" "${baseline_hash}" "$REPLAY_HASHES_TSV" >> "$REPLAY_SENTINEL_SUMMARY_TSV"
append_status "overall" "FAIL" "1" "mode=${MODE};runs=${RUNS};failures=${FAILURE_COUNT}" "$STATUS_TSV"
exit "$EXIT_FAIL"
