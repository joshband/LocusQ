#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

RUNS=1
CONTRACT_ONLY=0
EXECUTE_SUITE=0
OUT_DIR="${ROOT_DIR}/TestEvidence/bl046_sofa_binaural_lane_${TIMESTAMP}"
SCENARIO_PATH="${ROOT_DIR}/qa/scenarios/locusq_bl046_sofa_binaural_suite.json"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-046-sofa-hrtf-binaural-expansion.md"
QA_DOC="${ROOT_DIR}/Documentation/testing/bl-046-sofa-hrtf-binaural-expansion-qa.md"
SCRIPT_PATH="${ROOT_DIR}/scripts/qa-bl046-sofa-binaural-lane-mac.sh"

usage() {
  cat <<'USAGE'
Usage: qa-bl046-sofa-binaural-lane-mac.sh [options]

BL-046 SOFA/HRTF/binaural deterministic contract lane (B1 bootstrap + C2 soak + C3 replay sentinel + C4/D1/D2 long-run progression).

Options:
  --runs <N>         Replay run count (integer >= 1)
  --out-dir <path>   Artifact output directory
  --scenario <path>  Scenario suite path
  --contract-only    Run deterministic contract checks only (default behavior)
  --execute-suite    Reserved for future runtime execution; currently runs contract checks
  --help, -h         Show usage

Outputs:
  status.tsv
  validation_matrix.tsv
  replay_hashes.tsv
  failure_taxonomy.tsv
  soak_summary.tsv
  replay_sentinel_summary.tsv

Exit semantics:
  0  pass
  1  gate fail
  2  usage/config error
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      [[ $# -ge 2 ]] || { echo "ERROR: --runs requires a value" >&2; usage >&2; exit 2; }
      RUNS="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --out-dir requires a value" >&2; usage >&2; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    --scenario)
      [[ $# -ge 2 ]] || { echo "ERROR: --scenario requires a value" >&2; usage >&2; exit 2; }
      SCENARIO_PATH="$2"
      shift 2
      ;;
    --contract-only)
      CONTRACT_ONLY=1
      EXECUTE_SUITE=0
      shift
      ;;
    --execute-suite)
      EXECUTE_SUITE=1
      CONTRACT_ONLY=0
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  echo "ERROR: --runs must be an integer >= 1 (received: $RUNS)" >&2
  usage >&2
  exit 2
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/validation_matrix.tsv"
REPLAY_TSV="${OUT_DIR}/replay_hashes.tsv"
FAILURE_TSV="${OUT_DIR}/failure_taxonomy.tsv"
SOAK_SUMMARY_TSV="${OUT_DIR}/soak_summary.tsv"
REPLAY_SENTINEL_SUMMARY_TSV="${OUT_DIR}/replay_sentinel_summary.tsv"
FAILURE_EVENTS_TSV="${OUT_DIR}/.failure_events.tsv"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run\tcheck_id\tresult\tdetail\tartifact\n" > "$VALIDATION_TSV"
printf "run\tsignature\tbaseline_signature\tsignature_match\trow_signature\tbaseline_row_signature\trow_match\n" > "$REPLAY_TSV"
printf "failure_id\tcount\tclassification\tdetail\n" > "$FAILURE_TSV"
printf "mode\truns\tsignature_divergence\tmax_signature_divergence\trow_drift\tmax_row_drift\tdeterministic_contract_failure\tfinal_failures\tresult\tbaseline_signature\tbaseline_row_signature\n" > "$SOAK_SUMMARY_TSV"
printf "mode\truns\tsignature_divergence\tmax_signature_divergence\trow_drift\tmax_row_drift\tdeterministic_contract_failure\tfinal_failures\tresult\tbaseline_signature\tbaseline_row_signature\n" > "$REPLAY_SENTINEL_SUMMARY_TSV"
: > "$FAILURE_EVENTS_TSV"

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf "%s" "$value"
}

log_status() {
  local step="$1"
  local result="$2"
  local exit_code="$3"
  local detail="$4"
  local artifact="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$step")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$STATUS_TSV"
}

add_validation() {
  local run="$1"
  local check_id="$2"
  local result="$3"
  local detail="$4"
  local artifact="$5"
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$run")" \
    "$(sanitize_tsv_field "$check_id")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$VALIDATION_TSV"
}

append_failure_event() {
  local failure_id="$1"
  local run="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$failure_id")" \
    "$(sanitize_tsv_field "$run")" \
    "$(sanitize_tsv_field "$detail")" \
    >> "$FAILURE_EVENTS_TSV"
}

hash_text() {
  local text="$1"
  printf "%s" "$text" | shasum -a 256 | awk '{print $1}'
}

OVERALL_FAIL=0

for req in "$SCENARIO_PATH" "$BACKLOG_DOC" "$QA_DOC" "$SCRIPT_PATH"; do
  if [[ -f "$req" ]]; then
    log_status "file_exists" "PASS" "0" "found" "$req"
  else
    log_status "file_exists" "FAIL" "1" "missing" "$req"
    append_failure_event "BL046-B1-901" "0" "missing_required_file:$(basename "$req")"
    OVERALL_FAIL=1
  fi
done

if ! command -v jq >/dev/null 2>&1; then
  log_status "tooling" "FAIL" "1" "missing_jq" "$STATUS_TSV"
  append_failure_event "BL046-B1-901" "0" "missing_required_tool:jq"
  OVERALL_FAIL=1
fi

if [[ "$CONTRACT_ONLY" -eq 1 || "$EXECUTE_SUITE" -eq 0 ]]; then
  SELECTED_MODE="contract_only"
  log_status "mode" "PASS" "0" "contract_only" "$STATUS_TSV"
else
  SELECTED_MODE="execute_suite"
  log_status "mode" "PASS" "0" "execute_suite_requested_runtime_reserved_running_contract_checks_only" "$STATUS_TSV"
fi

run_check_pattern() {
  local run="$1"
  local check_id="$2"
  local pattern="$3"
  local file_path="$4"
  local run_fail_count="$5"
  if rg -q -- "$pattern" "$file_path"; then
    add_validation "$run" "$check_id" "PASS" "pattern_present" "$file_path"
  else
    add_validation "$run" "$check_id" "FAIL" "pattern_missing:${pattern}" "$file_path"
    append_failure_event "BL046-B1-901" "$run" "${check_id}:pattern_missing"
    run_fail_count=$((run_fail_count + 1))
    OVERALL_FAIL=1
  fi
  printf "%s" "$run_fail_count"
}

run_check_jq() {
  local run="$1"
  local check_id="$2"
  local expr="$3"
  local detail="$4"
  local run_fail_count="$5"
  if jq -e "$expr" "$SCENARIO_PATH" >/dev/null 2>&1; then
    add_validation "$run" "$check_id" "PASS" "$detail" "$SCENARIO_PATH"
  else
    add_validation "$run" "$check_id" "FAIL" "$detail" "$SCENARIO_PATH"
    append_failure_event "BL046-B1-901" "$run" "${check_id}:jq_contract_failure"
    run_fail_count=$((run_fail_count + 1))
    OVERALL_FAIL=1
  fi
  printf "%s" "$run_fail_count"
}

baseline_signature=""
baseline_row_signature=""
signature_drift_count=0
row_drift_count=0
max_signature_divergence="$(jq -r '.bl046_contract_checks.c2_soak.max_signature_divergence // .bl046_contract_checks.replay_contract.max_signature_divergence // 0' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
max_row_drift="$(jq -r '.bl046_contract_checks.c2_soak.max_row_drift // .bl046_contract_checks.replay_contract.max_row_drift // 0' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
required_soak_runs="$(jq -r '.bl046_contract_checks.c2_soak.required_runs // 10' "$SCENARIO_PATH" 2>/dev/null || printf "10")"
c3_max_signature_divergence="$(jq -r '.bl046_contract_checks.c3_replay_sentinel.max_signature_divergence // 0' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
c3_max_row_drift="$(jq -r '.bl046_contract_checks.c3_replay_sentinel.max_row_drift // 0' "$SCENARIO_PATH" 2>/dev/null || printf "0")"
c3_required_runs="$(jq -r '.bl046_contract_checks.c3_replay_sentinel.required_runs // 20' "$SCENARIO_PATH" 2>/dev/null || printf "20")"
c4_required_runs="$(jq -r '.bl046_contract_checks.c4_longrun_mode_parity.required_runs // 50' "$SCENARIO_PATH" 2>/dev/null || printf "50")"
d1_required_runs="$(jq -r '.bl046_contract_checks.d1_done_candidate.required_runs // 75' "$SCENARIO_PATH" 2>/dev/null || printf "75")"
d2_required_runs="$(jq -r '.bl046_contract_checks.d2_done_promotion.required_runs // 100' "$SCENARIO_PATH" 2>/dev/null || printf "100")"
if ! [[ "$max_signature_divergence" =~ ^[0-9]+$ ]]; then
  max_signature_divergence=0
fi
if ! [[ "$max_row_drift" =~ ^[0-9]+$ ]]; then
  max_row_drift=0
fi
if ! [[ "$required_soak_runs" =~ ^[0-9]+$ ]] || [[ "$required_soak_runs" -lt 1 ]]; then
  required_soak_runs=10
fi
if ! [[ "$c3_max_signature_divergence" =~ ^[0-9]+$ ]]; then
  c3_max_signature_divergence=0
fi
if ! [[ "$c3_max_row_drift" =~ ^[0-9]+$ ]]; then
  c3_max_row_drift=0
fi
if ! [[ "$c3_required_runs" =~ ^[0-9]+$ ]] || [[ "$c3_required_runs" -lt 1 ]]; then
  c3_required_runs=20
fi
if ! [[ "$c4_required_runs" =~ ^[0-9]+$ ]] || [[ "$c4_required_runs" -lt 1 ]]; then
  c4_required_runs=50
fi
if ! [[ "$d1_required_runs" =~ ^[0-9]+$ ]] || [[ "$d1_required_runs" -lt 1 ]]; then
  d1_required_runs=75
fi
if ! [[ "$d2_required_runs" =~ ^[0-9]+$ ]] || [[ "$d2_required_runs" -lt 1 ]]; then
  d2_required_runs=100
fi

for run in $(seq 1 "$RUNS"); do
  run_log="${OUT_DIR}/run_${run}.log"
  : > "$run_log"

  run_fail_count=0

  run_fail_count="$(run_check_jq "$run" "BL046-B1-001" '.id == "locusq_bl046_sofa_binaural_suite"' "scenario_id_contract" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-B1-002" '
    ([.bl046_contract_checks.acceptance_ids[]?.id] // []) as $ids
    | ["BL046-B1-001","BL046-B1-002","BL046-B1-003","BL046-B1-004","BL046-B1-005","BL046-B1-006","BL046-B1-007","BL046-B1-008"]
    | all(. as $id | ($ids | index($id) != null))
  ' "acceptance_ids_present" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-B1-003" '
    (.bl046_contract_checks.deterministic_hash_inputs.include // []) as $inc
    | ["schema_version","requested_profile_token","sofa_sha256","sample_rate_hz","block_size","output_layout","selection_policy_version","fallback_policy_version"]
    | all(. as $v | ($inc | index($v) != null))
  ' "deterministic_hash_inputs_include_complete" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-B1-004" '
    (.bl046_contract_checks.required_fallback_tokens // []) as $fb
    | ["none","sofa_path_missing","sofa_open_failure","sofa_parse_failure","sofa_convention_unsupported","sofa_dimension_invalid","sofa_non_finite_ir","sofa_digest_mismatch","hrtf_profile_unavailable","binaural_chain_unavailable"]
    | all(. as $v | ($fb | index($v) != null))
  ' "fallback_tokens_complete" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-B1-005" '
    (.bl046_contract_checks.artifact_schema // []) as $art
    | ["status.tsv","validation_matrix.tsv","replay_hashes.tsv","failure_taxonomy.tsv"]
    | all(. as $v | ($art | index($v) != null))
  ' "artifact_schema_complete" "$run_fail_count")"

  run_fail_count="$(run_check_jq "$run" "BL046-C2-001" '
    (.bl046_contract_checks.replay_contract.soak_runs // -1) == 10
    and (.bl046_contract_checks.c2_soak.required_runs // -1) == 10
    and (.bl046_contract_checks.c2_soak.max_signature_divergence // -1) == 0
    and (.bl046_contract_checks.c2_soak.max_row_drift // -1) == 0
  ' "c2_soak_thresholds_declared" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C2-002" '
    ([.bl046_contract_checks.acceptance_ids[]?.id] // []) as $ids
    | ["BL046-C2-001","BL046-C2-002","BL046-C2-003"]
    | all(. as $id | ($ids | index($id) != null))
  ' "c2_acceptance_ids_present" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C2-003" '
    (.bl046_contract_checks.c2_soak.required_evidence // []) as $art
    | ["status.tsv","validation_matrix.tsv","contract_runs/validation_matrix.tsv","contract_runs/replay_hashes.tsv","contract_runs/failure_taxonomy.tsv","soak_summary.tsv","lane_notes.md","docs_freshness.log"]
    | all(. as $v | ($art | index($v) != null))
  ' "c2_required_evidence_complete" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C3-001" '
    (.bl046_contract_checks.c3_replay_sentinel.required_runs // -1) == 20
    and (.bl046_contract_checks.c3_replay_sentinel.max_signature_divergence // -1) == 0
    and (.bl046_contract_checks.c3_replay_sentinel.max_row_drift // -1) == 0
  ' "c3_replay_sentinel_thresholds_declared" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C3-002" '
    ([.bl046_contract_checks.acceptance_ids[]?.id] // []) as $ids
    | ["BL046-C3-001","BL046-C3-002","BL046-C3-003"]
    | all(. as $id | ($ids | index($id) != null))
  ' "c3_acceptance_ids_present" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C3-003" '
    (.bl046_contract_checks.c3_replay_sentinel.required_evidence // []) as $art
    | ["status.tsv","validation_matrix.tsv","contract_runs/validation_matrix.tsv","contract_runs/replay_hashes.tsv","contract_runs/failure_taxonomy.tsv","replay_sentinel_summary.tsv","lane_notes.md","docs_freshness.log"]
    | all(. as $v | ($art | index($v) != null))
  ' "c3_required_evidence_complete" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C4-001" '
    (.bl046_contract_checks.c4_longrun_mode_parity.required_runs // -1) == 50
    and (.bl046_contract_checks.c4_longrun_mode_parity.max_signature_divergence // -1) == 0
    and (.bl046_contract_checks.c4_longrun_mode_parity.max_row_drift // -1) == 0
  ' "c4_longrun_thresholds_declared" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C4-002" '
    ([.bl046_contract_checks.acceptance_ids[]?.id] // []) as $ids
    | ["BL046-C4-001","BL046-C4-002","BL046-C4-003"]
    | all(. as $id | ($ids | index($id) != null))
  ' "c4_acceptance_ids_present" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-C4-003" '
    (.bl046_contract_checks.c4_longrun_mode_parity.required_evidence // []) as $art
    | ["status.tsv","validation_matrix.tsv","contract_runs_contract/validation_matrix.tsv","contract_runs_contract/replay_hashes.tsv","contract_runs_contract/failure_taxonomy.tsv","contract_runs_execute/validation_matrix.tsv","contract_runs_execute/replay_hashes.tsv","mode_parity.tsv","soak_summary.tsv","replay_sentinel_summary.tsv","exit_semantics_probe.tsv","lane_notes.md","docs_freshness.log"]
    | all(. as $v | ($art | index($v) != null))
  ' "c4_required_evidence_complete" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-D1-001" '
    (.bl046_contract_checks.d1_done_candidate.required_runs // -1) == 75
    and (.bl046_contract_checks.d1_done_candidate.max_signature_divergence // -1) == 0
    and (.bl046_contract_checks.d1_done_candidate.max_row_drift // -1) == 0
  ' "d1_done_candidate_thresholds_declared" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-D1-002" '
    ([.bl046_contract_checks.acceptance_ids[]?.id] // []) as $ids
    | ["BL046-D1-001","BL046-D1-002","BL046-D1-003"]
    | all(. as $id | ($ids | index($id) != null))
  ' "d1_acceptance_ids_present" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-D1-003" '
    (.bl046_contract_checks.d1_done_candidate.required_evidence // []) as $art
    | ["status.tsv","validation_matrix.tsv","contract_runs_contract/validation_matrix.tsv","contract_runs_contract/replay_hashes.tsv","contract_runs_contract/failure_taxonomy.tsv","contract_runs_execute/validation_matrix.tsv","contract_runs_execute/replay_hashes.tsv","mode_parity.tsv","soak_summary.tsv","replay_sentinel_summary.tsv","exit_semantics_probe.tsv","lane_notes.md","docs_freshness.log"]
    | all(. as $v | ($art | index($v) != null))
  ' "d1_required_evidence_complete" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-D2-001" '
    (.bl046_contract_checks.d2_done_promotion.required_runs // -1) == 100
    and (.bl046_contract_checks.d2_done_promotion.max_signature_divergence // -1) == 0
    and (.bl046_contract_checks.d2_done_promotion.max_row_drift // -1) == 0
  ' "d2_done_promotion_thresholds_declared" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-D2-002" '
    ([.bl046_contract_checks.acceptance_ids[]?.id] // []) as $ids
    | ["BL046-D2-001","BL046-D2-002","BL046-D2-003"]
    | all(. as $id | ($ids | index($id) != null))
  ' "d2_acceptance_ids_present" "$run_fail_count")"
  run_fail_count="$(run_check_jq "$run" "BL046-D2-003" '
    (.bl046_contract_checks.d2_done_promotion.required_evidence // []) as $art
    | ["status.tsv","validation_matrix.tsv","contract_runs_contract/validation_matrix.tsv","contract_runs_contract/replay_hashes.tsv","contract_runs_contract/failure_taxonomy.tsv","contract_runs_execute/validation_matrix.tsv","contract_runs_execute/replay_hashes.tsv","mode_parity.tsv","soak_summary.tsv","replay_sentinel_summary.tsv","exit_semantics_probe.tsv","promotion_readiness.md","lane_notes.md","docs_freshness.log"]
    | all(. as $v | ($art | index($v) != null))
  ' "d2_required_evidence_complete" "$run_fail_count")"

  run_fail_count="$(run_check_pattern "$run" "BL046-B1-006" 'BL046-B1-001|Validation Plan \(B1\)|qa-bl046-sofa-binaural-lane-mac.sh|locusq_bl046_sofa_binaural_suite.json' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-B1-007" 'BL046-B1-001|B1 Validation|B1 Evidence Contract|qa-bl046-sofa-binaural-lane-mac.sh' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-B1-008" '--contract-only|--execute-suite|Exit semantics|exit 2' "$SCRIPT_PATH" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-C2-004" 'BL046-C2-001|Validation Plan \(C2\)|--runs 10|soak_summary.tsv' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-C2-005" 'BL046-C2-001|C2 Validation|C2 Evidence Contract|--runs 10|soak_summary.tsv' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-C3-004" 'BL046-C3-001|Validation Plan \(C3\)|--runs 20|replay_sentinel_summary.tsv' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-C3-005" 'BL046-C3-001|C3 Validation|C3 Evidence Contract|--runs 20|replay_sentinel_summary.tsv' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-C4-004" 'BL046-C4-001|Validation Plan \(C4\)|--runs 50|mode_parity.tsv|exit_semantics_probe.tsv' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-C4-005" 'BL046-C4-001|C4 Validation|C4 Evidence Contract|--runs 50|mode_parity.tsv|exit_semantics_probe.tsv' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-D1-004" 'BL046-D1-001|Validation Plan \(D1\)|--runs 75|mode_parity.tsv|exit_semantics_probe.tsv' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-D1-005" 'BL046-D1-001|D1 Validation|D1 Evidence Contract|--runs 75|mode_parity.tsv|exit_semantics_probe.tsv' "$QA_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-D2-004" 'BL046-D2-001|Validation Plan \(D2\)|--runs 100|promotion_readiness.md' "$BACKLOG_DOC" "$run_fail_count")"
  run_fail_count="$(run_check_pattern "$run" "BL046-D2-005" 'BL046-D2-001|D2 Validation|D2 Evidence Contract|--runs 100|promotion_readiness.md' "$QA_DOC" "$run_fail_count")"

  scenario_sig="$(jq -c '
    {
      id,
      scenario_ids,
      bl046_contract_checks: {
        acceptance_ids: [.bl046_contract_checks.acceptance_ids[]?.id],
        required_sofa_fields: .bl046_contract_checks.required_sofa_fields,
        required_fallback_tokens: .bl046_contract_checks.required_fallback_tokens,
        replay_contract: .bl046_contract_checks.replay_contract,
        artifact_schema: .bl046_contract_checks.artifact_schema,
        deterministic_hash_inputs: .bl046_contract_checks.deterministic_hash_inputs
      }
    }
  ' "$SCENARIO_PATH" | shasum -a 256 | awk '{print $1}')"
  backlog_sig="$(rg -n 'BL046-B1-|BL046-C2-|BL046-C3-|BL046-C4-|BL046-D1-|BL046-D2-|Validation Plan \(B1\)|Validation Plan \(C2\)|Validation Plan \(C3\)|Validation Plan \(C4\)|Validation Plan \(D1\)|Validation Plan \(D2\)|Evidence Contract \(B1\)|Evidence Contract \(C2\)|Evidence Contract \(C3\)|Evidence Contract \(C4\)|Evidence Contract \(D1\)|Evidence Contract \(D2\)|qa-bl046-sofa-binaural-lane-mac.sh|locusq_bl046_sofa_binaural_suite.json|soak_summary.tsv|replay_sentinel_summary.tsv|mode_parity.tsv|exit_semantics_probe.tsv|promotion_readiness.md' "$BACKLOG_DOC" | shasum -a 256 | awk '{print $1}')"
  qa_sig="$(rg -n 'BL046-B1-|BL046-C2-|BL046-C3-|BL046-C4-|BL046-D1-|BL046-D2-|B1 Validation|C2 Validation|C3 Validation|C4 Validation|D1 Validation|D2 Validation|B1 Evidence Contract|C2 Evidence Contract|C3 Evidence Contract|C4 Evidence Contract|D1 Evidence Contract|D2 Evidence Contract|qa-bl046-sofa-binaural-lane-mac.sh|locusq_bl046_sofa_binaural_suite.json|soak_summary.tsv|replay_sentinel_summary.tsv|mode_parity.tsv|exit_semantics_probe.tsv|promotion_readiness.md' "$QA_DOC" | shasum -a 256 | awk '{print $1}')"
  script_sig="$(rg -n -- '--contract-only|--execute-suite|Exit semantics|--runs must be an integer >= 1|exit 2|replay_hashes.tsv|failure_taxonomy.tsv|soak_summary.tsv|replay_sentinel_summary.tsv|mode_parity.tsv|exit_semantics_probe.tsv' "$SCRIPT_PATH" | shasum -a 256 | awk '{print $1}')"

  signature="$(hash_text "${scenario_sig}|${backlog_sig}|${qa_sig}|${script_sig}")"
  row_signature="$(hash_text "${run_fail_count}")"

  signature_match=1
  row_match=1
  if [[ -z "$baseline_signature" ]]; then
    baseline_signature="$signature"
    baseline_row_signature="$row_signature"
  else
    if [[ "$signature" != "$baseline_signature" ]]; then
      signature_match=0
      signature_drift_count=$((signature_drift_count + 1))
      append_failure_event "BL046-B1-902" "$run" "signature_divergence"
      OVERALL_FAIL=1
    fi
    if [[ "$row_signature" != "$baseline_row_signature" ]]; then
      row_match=0
      row_drift_count=$((row_drift_count + 1))
      append_failure_event "BL046-B1-903" "$run" "row_divergence"
      OVERALL_FAIL=1
    fi
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run" \
    "$signature" \
    "$baseline_signature" \
    "$signature_match" \
    "$row_signature" \
    "$baseline_row_signature" \
    "$row_match" \
    >> "$REPLAY_TSV"

  {
    printf "[run=%s] signature=%s baseline=%s signature_match=%s\n" "$run" "$signature" "$baseline_signature" "$signature_match"
    printf "[run=%s] row_signature=%s baseline_row=%s row_match=%s run_fail_count=%s\n" "$run" "$row_signature" "$baseline_row_signature" "$row_match" "$run_fail_count"
  } >> "$run_log"
done

if [[ -s "$FAILURE_EVENTS_TSV" ]]; then
  awk -F'\t' '
    {
      count[$1]++
    }
    END {
      for (id in count) {
        classification = "deterministic_contract_failure"
        detail = "contract_or_replay_failure"
        if (id == "BL046-B1-901") {
          detail = "missing_required_pattern_or_schema"
        } else if (id == "BL046-B1-902") {
          classification = "deterministic_replay_divergence"
          detail = "replay_signature_divergence"
        } else if (id == "BL046-B1-903") {
          classification = "deterministic_replay_row_drift"
          detail = "replay_row_divergence"
        }
        printf "%s\t%d\t%s\t%s\n", id, count[id], classification, detail
      }
    }
  ' "$FAILURE_EVENTS_TSV" | sort >> "$FAILURE_TSV"
else
  printf "none\t0\tnone\tno_failures\n" >> "$FAILURE_TSV"
fi

if [[ "$signature_drift_count" -eq 0 ]]; then
  log_status "replay_signature" "PASS" "0" "stable" "$REPLAY_TSV"
else
  log_status "replay_signature" "FAIL" "1" "drift_count=${signature_drift_count}" "$REPLAY_TSV"
fi

if [[ "$row_drift_count" -eq 0 ]]; then
  log_status "replay_rows" "PASS" "0" "stable" "$REPLAY_TSV"
else
  log_status "replay_rows" "FAIL" "1" "drift_count=${row_drift_count}" "$REPLAY_TSV"
fi

if [[ "$RUNS" -ge "$required_soak_runs" ]]; then
  log_status "c2_soak_window" "PASS" "0" "target_met:runs=${RUNS}:required=${required_soak_runs}" "$REPLAY_TSV"
else
  log_status "c2_soak_window" "PASS" "0" "target_not_requested:runs=${RUNS}:required=${required_soak_runs}" "$REPLAY_TSV"
fi
if [[ "$RUNS" -ge "$c3_required_runs" ]]; then
  log_status "c3_replay_sentinel_window" "PASS" "0" "target_met:runs=${RUNS}:required=${c3_required_runs}" "$REPLAY_TSV"
else
  log_status "c3_replay_sentinel_window" "PASS" "0" "target_not_requested:runs=${RUNS}:required=${c3_required_runs}" "$REPLAY_TSV"
fi
if [[ "$RUNS" -ge "$c4_required_runs" ]]; then
  log_status "c4_longrun_window" "PASS" "0" "target_met:runs=${RUNS}:required=${c4_required_runs}" "$REPLAY_TSV"
else
  log_status "c4_longrun_window" "PASS" "0" "target_not_requested:runs=${RUNS}:required=${c4_required_runs}" "$REPLAY_TSV"
fi
if [[ "$RUNS" -ge "$d1_required_runs" ]]; then
  log_status "d1_done_candidate_window" "PASS" "0" "target_met:runs=${RUNS}:required=${d1_required_runs}" "$REPLAY_TSV"
else
  log_status "d1_done_candidate_window" "PASS" "0" "target_not_requested:runs=${RUNS}:required=${d1_required_runs}" "$REPLAY_TSV"
fi
if [[ "$RUNS" -ge "$d2_required_runs" ]]; then
  log_status "d2_done_promotion_window" "PASS" "0" "target_met:runs=${RUNS}:required=${d2_required_runs}" "$REPLAY_TSV"
else
  log_status "d2_done_promotion_window" "PASS" "0" "target_not_requested:runs=${RUNS}:required=${d2_required_runs}" "$REPLAY_TSV"
fi

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  soak_result="PASS"
  final_failures=0
else
  soak_result="FAIL"
  final_failures=1
fi
printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
  "$SELECTED_MODE" \
  "$RUNS" \
  "$signature_drift_count" \
  "$max_signature_divergence" \
  "$row_drift_count" \
  "$max_row_drift" \
  "$OVERALL_FAIL" \
  "$final_failures" \
  "$soak_result" \
  "$baseline_signature" \
  "$baseline_row_signature" \
  >> "$SOAK_SUMMARY_TSV"
printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
  "$SELECTED_MODE" \
  "$RUNS" \
  "$signature_drift_count" \
  "$c3_max_signature_divergence" \
  "$row_drift_count" \
  "$c3_max_row_drift" \
  "$OVERALL_FAIL" \
  "$final_failures" \
  "$soak_result" \
  "$baseline_signature" \
  "$baseline_row_signature" \
  >> "$REPLAY_SENTINEL_SUMMARY_TSV"

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  log_status "lane_result" "PASS" "0" "all_contract_checks_passed" "$STATUS_TSV"
  rm -f "$FAILURE_EVENTS_TSV"
  exit 0
fi

log_status "lane_result" "FAIL" "1" "contract_gate_failed" "$STATUS_TSV"
rm -f "$FAILURE_EVENTS_TSV"
exit 1
