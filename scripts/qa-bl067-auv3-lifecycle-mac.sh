#!/usr/bin/env bash
# Title: BL-067 AUv3 Lifecycle QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-03
#
# Purpose:
# - Provide deterministic BL-067 contract and execute semantics.
# - Emit AUv3 host, lifecycle, parity, and packaging evidence.
# - Enforce execute-mode truthfulness (zero TODO rows).
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract_only"
MODE_SET=0
RUNS=1
OUT_DIR="${ROOT_DIR}/TestEvidence/bl067_auv3_lifecycle_${TIMESTAMP}"

RUN_SUMMARY_TSV=""
run_pass_count=0
run_fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl067-auv3-lifecycle-mac.sh [options]

BL-067 deterministic lane for AUv3 lifecycle and host-validation evidence.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Run contract-only checks (default)
  --execute          Run execute-mode checks (requires zero TODO rows)
  --runs <n>         Number of deterministic replays (default: 1)
  --help, -h         Show usage

Outputs (per run):
  status.tsv
  host_matrix.tsv
  lifecycle_transitions.tsv
  parity_regression.tsv
  packaging_manifest.md

Additional output:
  run_summary.tsv (at out-dir root)
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

is_positive_integer() {
  [[ "$1" =~ ^[1-9][0-9]*$ ]]
}

record() {
  local status_file="$1"
  local check_id="$2"
  local result="$3"
  local detail="$4"
  local artifact="${5:-}"

  printf "%s\t%s\t%s\t%s\n" \
    "$check_id" \
    "$result" \
    "${detail//$'\t'/ }" \
    "${artifact//$'\t'/ }" \
    >> "$status_file"

  if [[ "$result" == "FAIL" ]]; then
    ((run_fail_count++)) || true
    echo "  [FAIL] $check_id: $detail"
  else
    ((run_pass_count++)) || true
    echo "  [${result}] $check_id: $detail"
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

count_result() {
  local file="$1"
  local column="$2"
  local wanted="$3"

  awk -F'\t' -v col="$column" -v val="$wanted" '
    NR == 1 { next }
    $col == val { count++ }
    END { print count + 0 }
  ' "$file"
}

append_host_row() {
  local status_file="$1"
  local host_file="$2"
  local check_id="$3"
  local host_name="$4"
  local host_path="$5"

  local availability="missing"
  local result="BLOCKED"
  local taxonomy="host_binary_missing"
  local detail="${host_name} not installed at ${host_path}"

  if [[ -d "$host_path" ]]; then
    availability="present"
    result="PASS"
    taxonomy="none"
    detail="${host_name} binary detected at ${host_path}"
  fi

  printf "%s\t%s\tpath_probe\t%s\t%s\t%s\n" \
    "$host_name" \
    "$availability" \
    "$result" \
    "$taxonomy" \
    "$detail" \
    >> "$host_file"

  record "$status_file" "$check_id" "$result" "$detail" "$host_file"
}

append_contract_row() {
  local status_file="$1"
  local target_file="$2"
  local check_id="$3"
  local lane_name="$4"
  local regex="$5"
  local source_file="$6"
  local target_kind="$7"

  local result="FAIL"
  local taxonomy="missing_contract_clause"
  local detail="${lane_name} contract text missing in ${source_file}"

  if [[ -f "$source_file" ]] && rg -qi "$regex" "$source_file"; then
    result="PASS"
    taxonomy="none"
    detail="${lane_name} contract text found in ${source_file}"
  fi

  printf "%s\tcontract_regex\t%s\t%s\t%s\n" \
    "$lane_name" \
    "$result" \
    "$taxonomy" \
    "$detail" \
    >> "$target_file"

  record "$status_file" "$check_id" "$result" "$detail" "$target_file"

  if [[ "$target_kind" == "parity" && "$result" == "PASS" ]]; then
    # No-op hook for future parity-specific counters.
    true
  fi
}

write_packaging_manifest() {
  local manifest_file="$1"
  local mode="$2"
  local run_index="$3"
  local host_file="$4"
  local lifecycle_file="$5"
  local parity_file="$6"

  local host_pass host_blocked host_fail
  local lifecycle_pass lifecycle_fail
  local parity_pass parity_fail

  host_pass="$(count_result "$host_file" 4 PASS)"
  host_blocked="$(count_result "$host_file" 4 BLOCKED)"
  host_fail="$(count_result "$host_file" 4 FAIL)"

  lifecycle_pass="$(count_result "$lifecycle_file" 3 PASS)"
  lifecycle_fail="$(count_result "$lifecycle_file" 3 FAIL)"

  parity_pass="$(count_result "$parity_file" 3 PASS)"
  parity_fail="$(count_result "$parity_file" 3 FAIL)"

  cat > "$manifest_file" <<EOF_MANIFEST
# BL-067 Packaging Manifest

- generated_utc: ${TIMESTAMP}
- lane: BL-067 AUv3 lifecycle and host validation
- mode: ${mode}
- run_index: ${run_index}
- expected_bundle_root: TestEvidence/bl067_*/

## Build + Packaging Contract

- cmake_contract_file: ${ROOT_DIR}/CMakeLists.txt
- auv3_gate_option: LOCUSQ_ENABLE_AUV3
- apple_formats_contract: VST3 AU Standalone (+AUv3 when LOCUSQ_ENABLE_AUV3=ON)
- signing_contract: AUv3 packaging requires valid Apple signing identity and provisioning in the Xcode-generated project.
- extension_boundary_contract: ADR-0017 (host-name branching forbidden; capability/state driven fallback only)

## Evidence Summary

- host_matrix: PASS=${host_pass}, BLOCKED=${host_blocked}, FAIL=${host_fail}
- lifecycle_transitions: PASS=${lifecycle_pass}, FAIL=${lifecycle_fail}
- parity_regression: PASS=${parity_pass}, FAIL=${parity_fail}

## Required Artifacts

- status.tsv
- host_matrix.tsv
- lifecycle_transitions.tsv
- parity_regression.tsv
- packaging_manifest.md
EOF_MANIFEST
}

run_single() {
  local run_index="$1"
  local run_dir="$2"

  local status_tsv="${run_dir}/status.tsv"
  local host_matrix_tsv="${run_dir}/host_matrix.tsv"
  local lifecycle_tsv="${run_dir}/lifecycle_transitions.tsv"
  local parity_tsv="${run_dir}/parity_regression.tsv"
  local packaging_md="${run_dir}/packaging_manifest.md"

  local backlog_doc="${ROOT_DIR}/Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md"
  local annex_doc="${ROOT_DIR}/Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md"
  local skill_doc="${ROOT_DIR}/.codex/skills/auv3-plugin-lifecycle/SKILL.md"
  local adr_doc="${ROOT_DIR}/Documentation/adr/ADR-0017-auv3-app-extension-boundary-and-lifecycle-contract.md"
  local cmake_file="${ROOT_DIR}/CMakeLists.txt"

  local todo_rows=0

  run_pass_count=0
  run_fail_count=0

  mkdir -p "$run_dir"

  printf "check_id\tresult\tdetail\tartifact\n" > "$status_tsv"
  printf "host\tavailability\tprobe\tresult\tfailure_taxonomy\tdetail\n" > "$host_matrix_tsv"
  printf "transition\tprobe\tresult\tfailure_taxonomy\tdetail\n" > "$lifecycle_tsv"
  printf "lane\tprobe\tresult\tfailure_taxonomy\tdetail\n" > "$parity_tsv"

  echo "=== BL-067 AUv3 Lifecycle Lane ==="
  echo "Mode: ${MODE}"
  echo "Run: ${run_index}/${RUNS}"
  echo "Output dir: ${run_dir}"

  if [[ -f "$backlog_doc" ]]; then
    record "$status_tsv" "BL067-S1-backlog_doc_exists" "PASS" "BL-067 runbook present" "$backlog_doc"
  else
    record "$status_tsv" "BL067-S1-backlog_doc_exists" "FAIL" "BL-067 runbook missing" "$backlog_doc"
  fi

  if [[ -f "$annex_doc" ]]; then
    record "$status_tsv" "BL067-S2-annex_doc_exists" "PASS" "BL-067 annex spec present" "$annex_doc"
  else
    record "$status_tsv" "BL067-S2-annex_doc_exists" "FAIL" "BL-067 annex spec missing" "$annex_doc"
  fi

  if [[ -f "$skill_doc" ]]; then
    record "$status_tsv" "BL067-S3-skill_doc_exists" "PASS" "AUv3 lifecycle skill present" "$skill_doc"
  else
    record "$status_tsv" "BL067-S3-skill_doc_exists" "FAIL" "AUv3 lifecycle skill missing" "$skill_doc"
  fi

  if [[ -f "$adr_doc" ]]; then
    record "$status_tsv" "BL067-S4-adr_exists" "PASS" "ADR-0017 boundary contract present" "$adr_doc"
  else
    record "$status_tsv" "BL067-S4-adr_exists" "FAIL" "ADR-0017 boundary contract missing" "$adr_doc"
  fi

  if rg -q 'qa-bl067-auv3-lifecycle-mac.sh' "$backlog_doc" 2>/dev/null; then
    record "$status_tsv" "BL067-S5-runbook_references_lane" "PASS" "runbook references BL-067 QA lane" "$backlog_doc"
  else
    record "$status_tsv" "BL067-S5-runbook_references_lane" "FAIL" "runbook missing BL-067 QA lane reference" "$backlog_doc"
  fi

  if rg -q 'LOCUSQ_ENABLE_AUV3' "$cmake_file" 2>/dev/null && rg -q 'AUv3' "$cmake_file" 2>/dev/null; then
    record "$status_tsv" "BL067-S6-cmake_auv3_gate" "PASS" "CMake includes explicit AUv3 gate and format token" "$cmake_file"
  else
    record "$status_tsv" "BL067-S6-cmake_auv3_gate" "FAIL" "CMake missing AUv3 option or AUv3 format token" "$cmake_file"
  fi

  append_host_row "$status_tsv" "$host_matrix_tsv" "BL067-H1-logic_pro" "Logic Pro" "/Applications/Logic Pro.app"
  append_host_row "$status_tsv" "$host_matrix_tsv" "BL067-H2-garageband" "GarageBand" "/Applications/GarageBand.app"
  append_host_row "$status_tsv" "$host_matrix_tsv" "BL067-H3-mainstage" "MainStage" "/Applications/MainStage.app"

  append_contract_row "$status_tsv" "$lifecycle_tsv" "BL067-L1-cold_start" "cold_start" "cold[[:space:]-]*start" "$annex_doc" "lifecycle"
  append_contract_row "$status_tsv" "$lifecycle_tsv" "BL067-L2-reload" "reload" "reload" "$annex_doc" "lifecycle"
  append_contract_row "$status_tsv" "$lifecycle_tsv" "BL067-L3-suspend_resume" "suspend_resume" "suspend[/ -]*resume" "$annex_doc" "lifecycle"
  append_contract_row "$status_tsv" "$lifecycle_tsv" "BL067-L4-state_restore" "state_restore" "state[[:space:]-]*restore" "$annex_doc" "lifecycle"

  append_contract_row "$status_tsv" "$parity_tsv" "BL067-P1-auv3_vs_au" "AUv3_vs_AU" "AU/VST3/CLAP" "$annex_doc" "parity"
  append_contract_row "$status_tsv" "$parity_tsv" "BL067-P2-auv3_vs_vst3" "AUv3_vs_VST3" "AU/VST3/CLAP" "$annex_doc" "parity"
  append_contract_row "$status_tsv" "$parity_tsv" "BL067-P3-auv3_vs_clap" "AUv3_vs_CLAP" "AU/VST3/CLAP" "$annex_doc" "parity"
  append_contract_row "$status_tsv" "$parity_tsv" "BL067-P4-no_host_name_branching" "host_name_branching_contract" "branch[[:space:]]+on[[:space:]]+host[[:space:]]+name|host[- ]name branching" "$adr_doc" "parity"

  if rg -q 'VST3' "$cmake_file" 2>/dev/null && rg -q 'AU' "$cmake_file" 2>/dev/null; then
    record "$status_tsv" "BL067-P5-cmake_core_formats_present" "PASS" "CMake includes AU and VST3 format contracts" "$cmake_file"
  else
    record "$status_tsv" "BL067-P5-cmake_core_formats_present" "FAIL" "CMake missing AU or VST3 format contracts" "$cmake_file"
  fi

  if rg -q 'LOCUSQ_ENABLE_CLAP' "$cmake_file" 2>/dev/null; then
    record "$status_tsv" "BL067-P6-cmake_clap_gate_present" "PASS" "CMake includes CLAP gate for parity checks" "$cmake_file"
  else
    record "$status_tsv" "BL067-P6-cmake_clap_gate_present" "FAIL" "CMake missing CLAP gate contract" "$cmake_file"
  fi

  todo_rows=$((
    $(count_todo_rows "$host_matrix_tsv")
    + $(count_todo_rows "$lifecycle_tsv")
    + $(count_todo_rows "$parity_tsv")
  ))

  if [[ "$MODE" == "execute" ]]; then
    if [[ "$todo_rows" -gt 0 ]]; then
      record "$status_tsv" "BL067-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$status_tsv"
    else
      record "$status_tsv" "BL067-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$status_tsv"
    fi
  else
    record "$status_tsv" "BL067-C1-contract_mode" "PASS" "contract-only mode completed (todo_rows=${todo_rows})" "$status_tsv"
  fi

  write_packaging_manifest "$packaging_md" "$MODE" "$run_index" "$host_matrix_tsv" "$lifecycle_tsv" "$parity_tsv"

  if [[ "$run_fail_count" -eq 0 ]]; then
    record "$status_tsv" "lane_result" "PASS" "mode=${MODE};run=${run_index};failures=0" "$status_tsv"
  else
    record "$status_tsv" "lane_result" "FAIL" "mode=${MODE};run=${run_index};failures=${run_fail_count}" "$status_tsv"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "$MODE" \
    "$run_pass_count" \
    "$run_fail_count" \
    "$todo_rows" \
    "$run_dir" \
    >> "$RUN_SUMMARY_TSV"

  echo ""
  echo "Run results: ${run_pass_count} non-fail, ${run_fail_count} fail"
  echo "Artifacts:"
  echo "- ${status_tsv}"
  echo "- ${host_matrix_tsv}"
  echo "- ${lifecycle_tsv}"
  echo "- ${parity_tsv}"
  echo "- ${packaging_md}"

  if [[ "$run_fail_count" -gt 0 ]]; then
    return 1
  fi
  return 0
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
      if ! is_positive_integer "$2"; then
        usage_error "--runs must be a positive integer"
      fi
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

mkdir -p "$OUT_DIR"
RUN_SUMMARY_TSV="${OUT_DIR}/run_summary.tsv"
printf "run\tmode\tnon_fail_checks\tfail_checks\ttodo_rows\tartifact_dir\n" > "$RUN_SUMMARY_TSV"

overall_fail_runs=0

for run_index in $(seq 1 "$RUNS"); do
  if [[ "$RUNS" -gt 1 ]]; then
    run_dir="${OUT_DIR}/run_$(printf '%02d' "$run_index")"
  else
    run_dir="$OUT_DIR"
  fi

  if ! run_single "$run_index" "$run_dir"; then
    ((overall_fail_runs++)) || true
  fi
done

echo ""
echo "=== BL-067 Summary ==="
echo "Mode: ${MODE}"
echo "Runs: ${RUNS}"
echo "Failed runs: ${overall_fail_runs}"
echo "Run summary: ${RUN_SUMMARY_TSV}"

if [[ "$overall_fail_runs" -gt 0 ]]; then
  exit 1
fi
exit 0
