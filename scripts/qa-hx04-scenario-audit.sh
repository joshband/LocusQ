#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

MANIFEST_INPUT="${LQ_HX04_MANIFEST:-qa/scenarios/locusq_hx04_required_scenarios.json}"
SUITE_INPUT="${LQ_HX04_SUITE:-qa/scenarios/locusq_hx04_component_parity_suite.json}"
QA_BIN="${LQ_HX04_QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
OUT_DIR_INPUT="${LQ_HX04_OUT_DIR:-TestEvidence/hx04_scenario_audit_${TIMESTAMP}}"
RUN_SUITE="${LQ_HX04_RUN_SUITE:-1}"

usage() {
  cat <<'EOF'
Usage: ./scripts/qa-hx04-scenario-audit.sh [options]

Options:
  --manifest <path>   Required scenario manifest JSON (default: qa/scenarios/locusq_hx04_required_scenarios.json)
  --suite <path>      HX-04 parity suite JSON (default: qa/scenarios/locusq_hx04_component_parity_suite.json)
  --qa-bin <path>     QA runner binary path
  --out-dir <path>    Output artifact directory
  --skip-suite-run    Only audit manifests/suites without executing runner
  -h, --help          Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --manifest)
      MANIFEST_INPUT="$2"
      shift 2
      ;;
    --suite)
      SUITE_INPUT="$2"
      shift 2
      ;;
    --qa-bin)
      QA_BIN="$2"
      shift 2
      ;;
    --out-dir)
      OUT_DIR_INPUT="$2"
      shift 2
      ;;
    --skip-suite-run)
      RUN_SUITE="0"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

resolve_path() {
  local value="$1"
  if [[ "$value" = /* ]]; then
    printf '%s\n' "$value"
  else
    printf '%s/%s\n' "$ROOT_DIR" "$value"
  fi
}

MANIFEST_PATH="$(resolve_path "$MANIFEST_INPUT")"
SUITE_PATH="$(resolve_path "$SUITE_INPUT")"
OUT_DIR="$(resolve_path "$OUT_DIR_INPUT")"

STATUS_TSV="$OUT_DIR/status.tsv"
REPORT_MD="$OUT_DIR/report.md"
MATRIX_TSV="$OUT_DIR/coverage_matrix.tsv"

mkdir -p "$OUT_DIR"
printf "step\tstatus\tdetail\n" >"$STATUS_TSV"

log_status() {
  local step="$1"
  local status="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$step" "$status" "$detail" | tee -a "$STATUS_TSV" >/dev/null
}

fail_and_exit() {
  local msg="$1"
  echo "FAIL: $msg"
  echo "artifact_dir=$OUT_DIR"
  exit 1
}

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "pass" "$(command -v "$cmd")"
  else
    log_status "tool_${cmd}" "fail" "missing_command"
    return 1
  fi
}

require_cmd jq || fail_and_exit "jq not found"

if [[ ! -f "$MANIFEST_PATH" ]]; then
  log_status "manifest" "fail" "missing=${MANIFEST_PATH}"
  fail_and_exit "HX-04 manifest missing"
fi

if [[ ! -f "$SUITE_PATH" ]]; then
  log_status "suite" "fail" "missing=${SUITE_PATH}"
  fail_and_exit "HX-04 suite missing"
fi

if jq -e . "$MANIFEST_PATH" >/dev/null 2>&1; then
  log_status "manifest_json" "pass" "$MANIFEST_PATH"
else
  log_status "manifest_json" "fail" "$MANIFEST_PATH"
  fail_and_exit "HX-04 manifest is invalid JSON"
fi

if jq -e . "$SUITE_PATH" >/dev/null 2>&1; then
  log_status "suite_json" "pass" "$SUITE_PATH"
else
  log_status "suite_json" "fail" "$SUITE_PATH"
  fail_and_exit "HX-04 suite is invalid JSON"
fi

MANIFEST_ID="$(jq -r '.id // ""' "$MANIFEST_PATH")"
SUITE_ID="$(jq -r '.id // ""' "$SUITE_PATH")"
MIN_MEMBERSHIP="$(jq -r '.minimum_suite_membership // 1' "$MANIFEST_PATH")"

if [[ -z "$MANIFEST_ID" || "$MANIFEST_ID" == "null" ]]; then
  log_status "manifest_id" "fail" "id_missing"
  fail_and_exit "HX-04 manifest id is required"
fi

if [[ -z "$SUITE_ID" || "$SUITE_ID" == "null" ]]; then
  log_status "suite_id" "fail" "id_missing"
  fail_and_exit "HX-04 suite id is required"
fi

log_status "manifest_id" "pass" "$MANIFEST_ID"
log_status "suite_id" "pass" "$SUITE_ID"

if ! [[ "$MIN_MEMBERSHIP" =~ ^[0-9]+$ ]] || [[ "$MIN_MEMBERSHIP" -lt 1 ]]; then
  log_status "minimum_suite_membership" "fail" "value=${MIN_MEMBERSHIP}"
  fail_and_exit "minimum_suite_membership must be an integer >= 1"
fi
log_status "minimum_suite_membership" "pass" "$MIN_MEMBERSHIP"

REQUIRED_COMPONENT_SCENARIOS=()
while IFS= read -r line; do
  if [[ -n "$line" ]]; then
    REQUIRED_COMPONENT_SCENARIOS+=("$line")
  fi
done < <(
  jq -r '.required_components[]? | .component as $component | .required_scenario_ids[]? | [$component, .] | @tsv' "$MANIFEST_PATH"
)

if [[ "${#REQUIRED_COMPONENT_SCENARIOS[@]}" -eq 0 ]]; then
  log_status "required_components" "fail" "no_required_scenarios"
  fail_and_exit "manifest has no required component scenarios"
fi

printf "component\tscenario_id\tscenario_file\tsuite_count\tsuite_ids\n" >"$MATRIX_TSV"

while IFS=$'\t' read -r COMPONENT_NAME SCENARIO_ID; do
  SCENARIO_FILE="$(jq -r --arg scenario_id "$SCENARIO_ID" '
    select((.scenario_version // "") != "" and (.id // "") == $scenario_id) | input_filename
  ' "$ROOT_DIR"/qa/scenarios/*.json | head -n 1)"

  if [[ -z "$SCENARIO_FILE" ]]; then
    log_status "required_scenario_${SCENARIO_ID}" "fail" "component=${COMPONENT_NAME}; reason=missing_file"
    fail_and_exit "required scenario missing: ${SCENARIO_ID}"
  fi

  SUITE_MATCHES=()
  while IFS= read -r suite_match; do
    if [[ -n "$suite_match" ]]; then
      SUITE_MATCHES+=("$suite_match")
    fi
  done < <(
    jq -r --arg scenario_id "$SCENARIO_ID" '
      select(.scenario_ids? != null and ([.scenario_ids[]?] | index($scenario_id) != null))
      | .id // empty
    ' "$ROOT_DIR"/qa/scenarios/*_suite.json | sort -u
  )

  SUITE_COUNT="${#SUITE_MATCHES[@]}"
  SUITE_LIST="$(printf '%s,' "${SUITE_MATCHES[@]}")"
  SUITE_LIST="${SUITE_LIST%,}"
  if [[ -z "$SUITE_LIST" ]]; then
    SUITE_LIST="none"
  fi

  printf "%s\t%s\t%s\t%s\t%s\n" "$COMPONENT_NAME" "$SCENARIO_ID" "$SCENARIO_FILE" "$SUITE_COUNT" "$SUITE_LIST" >>"$MATRIX_TSV"

  if [[ "$SUITE_COUNT" -lt "$MIN_MEMBERSHIP" ]]; then
    log_status "scenario_membership_${SCENARIO_ID}" "fail" "component=${COMPONENT_NAME}; suite_count=${SUITE_COUNT}; required=${MIN_MEMBERSHIP}"
    fail_and_exit "required suite membership failed for ${SCENARIO_ID}"
  fi

  if jq -e --arg scenario_id "$SCENARIO_ID" '[.scenario_ids[]?] | index($scenario_id) != null' "$SUITE_PATH" >/dev/null 2>&1; then
    log_status "scenario_membership_${SCENARIO_ID}" "pass" "component=${COMPONENT_NAME}; suite_count=${SUITE_COUNT}; required_suite=${SUITE_ID}"
  else
    log_status "scenario_membership_${SCENARIO_ID}" "fail" "component=${COMPONENT_NAME}; required_suite=${SUITE_ID}"
    fail_and_exit "required suite ${SUITE_ID} does not include ${SCENARIO_ID}"
  fi
done < <(printf '%s\n' "${REQUIRED_COMPONENT_SCENARIOS[@]}")

REQUIRED_SUITE_IDS=()
while IFS= read -r required_suite_id; do
  if [[ -n "$required_suite_id" ]]; then
    REQUIRED_SUITE_IDS+=("$required_suite_id")
  fi
done < <(jq -r '.required_suite_ids[]?' "$MANIFEST_PATH")
for REQUIRED_SUITE_ID in "${REQUIRED_SUITE_IDS[@]}"; do
  SUITE_FILE="$(jq -r --arg suite_id "$REQUIRED_SUITE_ID" '
    select(.scenario_ids? != null and (.id // "") == $suite_id) | input_filename
  ' "$ROOT_DIR"/qa/scenarios/*_suite.json | head -n 1)"

  if [[ -z "$SUITE_FILE" ]]; then
    log_status "required_suite_${REQUIRED_SUITE_ID}" "fail" "missing"
    fail_and_exit "required suite missing: ${REQUIRED_SUITE_ID}"
  fi

  log_status "required_suite_${REQUIRED_SUITE_ID}" "pass" "$SUITE_FILE"
done

if [[ "$RUN_SUITE" == "1" ]]; then
  if [[ ! -x "$QA_BIN" ]]; then
    ALT_QA_BIN="$ROOT_DIR/build/locusq_qa_artefacts/Release/locusq_qa"
    if [[ -x "$ALT_QA_BIN" ]]; then
      QA_BIN="$ALT_QA_BIN"
    else
      log_status "qa_bin" "fail" "missing=${QA_BIN}"
      fail_and_exit "qa runner binary missing"
    fi
  fi
  log_status "qa_bin" "pass" "$QA_BIN"

  SUITE_OUTPUT_DIR="$(jq -r '.runtime_config.output_dir // empty' "$SUITE_PATH")"
  if [[ -z "$SUITE_OUTPUT_DIR" ]]; then
    log_status "suite_output_dir" "fail" "runtime_config.output_dir_missing"
    fail_and_exit "suite runtime_config.output_dir is required"
  fi
  log_status "suite_output_dir" "pass" "$SUITE_OUTPUT_DIR"

  SUITE_LOG="$OUT_DIR/hx04_component_parity_suite.log"
  rm -rf "$ROOT_DIR/$SUITE_OUTPUT_DIR"
  if "$QA_BIN" --spatial "$SUITE_PATH" >"$SUITE_LOG" 2>&1; then
    log_status "suite_run" "pass" "log=${SUITE_LOG}"
  else
    log_status "suite_run" "fail" "log=${SUITE_LOG}"
    fail_and_exit "HX-04 suite execution failed"
  fi

  SUITE_RESULT_JSON="$ROOT_DIR/$SUITE_OUTPUT_DIR/suite_result.json"
  if [[ ! -f "$SUITE_RESULT_JSON" ]]; then
    log_status "suite_result_json" "fail" "missing=${SUITE_RESULT_JSON}"
    fail_and_exit "HX-04 suite_result.json missing"
  fi
  cp "$SUITE_RESULT_JSON" "$OUT_DIR/suite_result.json"
  log_status "suite_result_json" "pass" "$SUITE_RESULT_JSON"

  SUITE_STATUS="$(jq -r '.status // "UNKNOWN"' "$SUITE_RESULT_JSON")"
  PASS_COUNT="$(jq -r '.summary.passed // 0' "$SUITE_RESULT_JSON")"
  WARN_COUNT="$(jq -r '.summary.warned // 0' "$SUITE_RESULT_JSON")"
  FAIL_COUNT="$(jq -r '.summary.failed // 0' "$SUITE_RESULT_JSON")"
  ERROR_COUNT="$(jq -r '.summary.errors // 0' "$SUITE_RESULT_JSON")"

  if [[ "$FAIL_COUNT" -gt 0 || "$ERROR_COUNT" -gt 0 ]]; then
    log_status "suite_assert" "fail" "status=${SUITE_STATUS}; pass=${PASS_COUNT}; warn=${WARN_COUNT}; fail=${FAIL_COUNT}; error=${ERROR_COUNT}"
    fail_and_exit "HX-04 suite reported failures/errors"
  fi

  if [[ "$WARN_COUNT" -gt 0 ]]; then
    log_status "suite_assert" "warn" "status=${SUITE_STATUS}; pass=${PASS_COUNT}; warn=${WARN_COUNT}; fail=${FAIL_COUNT}; error=${ERROR_COUNT}"
  else
    log_status "suite_assert" "pass" "status=${SUITE_STATUS}; pass=${PASS_COUNT}; warn=${WARN_COUNT}; fail=${FAIL_COUNT}; error=${ERROR_COUNT}"
  fi
else
  log_status "suite_run" "warn" "skipped"
fi

cp "$MANIFEST_PATH" "$OUT_DIR/manifest_snapshot.json"
cp "$SUITE_PATH" "$OUT_DIR/suite_snapshot.json"
log_status "artifact_snapshots" "pass" "manifest+suite snapshots copied"

FAIL_STEP_COUNT="$(awk -F'\t' 'NR>1 && $2=="fail" { c++ } END { print c+0 }' "$STATUS_TSV")"
WARN_STEP_COUNT="$(awk -F'\t' 'NR>1 && $2=="warn" { c++ } END { print c+0 }' "$STATUS_TSV")"
PASS_STEP_COUNT="$(awk -F'\t' 'NR>1 && $2=="pass" { c++ } END { print c+0 }' "$STATUS_TSV")"

OVERALL="pass"
if [[ "$FAIL_STEP_COUNT" != "0" ]]; then
  OVERALL="fail"
elif [[ "$WARN_STEP_COUNT" != "0" ]]; then
  OVERALL="pass_with_warnings"
fi

cat >"$REPORT_MD" <<EOF
Title: HX-04 Scenario Coverage Audit Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# HX-04 Scenario Coverage Audit (${TIMESTAMP})

- overall: \`${OVERALL}\`
- manifest: \`${MANIFEST_PATH}\`
- suite: \`${SUITE_PATH}\`
- qa_bin: \`${QA_BIN}\`
- run_suite: \`${RUN_SUITE}\`
- pass_step_count: \`${PASS_STEP_COUNT}\`
- warn_step_count: \`${WARN_STEP_COUNT}\`
- fail_step_count: \`${FAIL_STEP_COUNT}\`

## Outputs

- \`status.tsv\`
- \`coverage_matrix.tsv\`
- \`manifest_snapshot.json\`
- \`suite_snapshot.json\`
- \`suite_result.json\` (when suite run is enabled)
- \`hx04_component_parity_suite.log\` (when suite run is enabled)
EOF

if [[ "$OVERALL" == "fail" ]]; then
  fail_and_exit "HX-04 scenario audit failed"
fi

if [[ "$OVERALL" == "pass_with_warnings" ]]; then
  echo "PASS_WITH_WARNINGS: HX-04 scenario audit completed"
else
  echo "PASS: HX-04 scenario audit completed"
fi
echo "artifact_dir=$OUT_DIR"
