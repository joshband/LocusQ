#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

OUT_DIR="${BL029_GATE_OUT_DIR:-$ROOT_DIR/TestEvidence/bl029_reliability_gate_p4_${TIMESTAMP}}"
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
HARD_CRITERIA_TSV="$OUT_DIR/hard_criteria.tsv"
FAILURE_TAXONOMY_TSV="$OUT_DIR/failure_taxonomy.tsv"
GATE_CONTRACT_MD="$OUT_DIR/gate_contract.md"
SELFTEST_SUMMARY_TSV="$OUT_DIR/selftest_runs.tsv"

STANDALONE_EXEC="${BL029_GATE_STANDALONE_EXEC:-$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app/Contents/MacOS/LocusQ}"
STANDALONE_APP="${STANDALONE_EXEC%/Contents/MacOS/LocusQ}.app"

BL029_SELFTEST_RUNS_REQUIRED=10
BL009_SELFTEST_RUNS_REQUIRED=5

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "lane\titeration\tharness_exit_code\tterminal_failure_reason\tapp_exit_code\tapp_signal\tapp_signal_name\tapp_exit_status_source\tresult_json\tmetadata_json\tattempt_status_table\tharness_log\n" > "$SELFTEST_SUMMARY_TSV"

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
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

get_meta_field() {
  local meta_path="$1"
  local jq_field="$2"
  local awk_key="$3"
  if [[ -n "$meta_path" && -f "$meta_path" ]]; then
    if command -v jq >/dev/null 2>&1; then
      jq -r "$jq_field // empty" "$meta_path" 2>/dev/null || true
      return 0
    fi
    awk -F= -v key="$awk_key" '$1 == key { print $2 }' "$meta_path" | tail -n 1
    return 0
  fi
  printf ''
}

run_selftest_iteration() {
  local lane="$1"
  local iteration="$2"
  local harness_log="$OUT_DIR/selftest_${lane}_${iteration}.harness.log"
  local result_json="$OUT_DIR/selftest_${lane}_${iteration}.result.json"
  local run_log="$OUT_DIR/selftest_${lane}_${iteration}.run.log"
  local attempt_table="$OUT_DIR/selftest_${lane}_${iteration}.attempts.tsv"
  local meta_json="$OUT_DIR/selftest_${lane}_${iteration}.meta.json"

  local harness_exit_code=0
  local terminal_reason=""
  local app_exit_code=""
  local app_signal=""
  local app_signal_name=""
  local app_exit_status_source=""

  local selftest_cmd=(
    "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh"
    "$STANDALONE_EXEC"
  )

  set +e
  if [[ "$lane" == "bl029" ]]; then
    LOCUSQ_UI_SELFTEST_SCOPE=bl029 \
    LOCUSQ_UI_SELFTEST_RESULT_PATH="$result_json" \
    LOCUSQ_UI_SELFTEST_RUN_LOG_PATH="$run_log" \
    LOCUSQ_UI_SELFTEST_ATTEMPT_TABLE_PATH="$attempt_table" \
    LOCUSQ_UI_SELFTEST_META_PATH="$meta_json" \
    "${selftest_cmd[@]}" > "$harness_log" 2>&1
    harness_exit_code=$?
  else
    LOCUSQ_UI_SELFTEST_BL009=1 \
    LOCUSQ_UI_SELFTEST_RESULT_PATH="$result_json" \
    LOCUSQ_UI_SELFTEST_RUN_LOG_PATH="$run_log" \
    LOCUSQ_UI_SELFTEST_ATTEMPT_TABLE_PATH="$attempt_table" \
    LOCUSQ_UI_SELFTEST_META_PATH="$meta_json" \
    "${selftest_cmd[@]}" > "$harness_log" 2>&1
    harness_exit_code=$?
  fi
  set -e

  terminal_reason="$(get_meta_field "$meta_json" '.terminalFailureReason' 'terminal_failure_reason')"
  app_exit_code="$(get_meta_field "$meta_json" '.appExitCode' 'app_exit_code')"
  app_signal="$(get_meta_field "$meta_json" '.appSignal' 'app_signal')"
  app_signal_name="$(get_meta_field "$meta_json" '.appSignalName' 'app_signal_name')"
  app_exit_status_source="$(get_meta_field "$meta_json" '.appExitStatusSource' 'app_exit_status_source')"

  if [[ -z "$terminal_reason" ]]; then
    terminal_reason="$(awk -F= '/^terminal_failure_reason=/{print $2}' "$harness_log" | tail -n 1)"
  fi
  if [[ -z "$terminal_reason" ]]; then
    terminal_reason="none"
  fi
  if [[ -z "$app_exit_status_source" ]]; then
    app_exit_status_source="not_recorded"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$lane")" \
    "$(sanitize_tsv_field "$iteration")" \
    "$(sanitize_tsv_field "$harness_exit_code")" \
    "$(sanitize_tsv_field "$terminal_reason")" \
    "$(sanitize_tsv_field "$app_exit_code")" \
    "$(sanitize_tsv_field "$app_signal")" \
    "$(sanitize_tsv_field "$app_signal_name")" \
    "$(sanitize_tsv_field "$app_exit_status_source")" \
    "$(sanitize_tsv_field "$result_json")" \
    "$(sanitize_tsv_field "$meta_json")" \
    "$(sanitize_tsv_field "$attempt_table")" \
    "$(sanitize_tsv_field "$harness_log")" \
    >> "$SELFTEST_SUMMARY_TSV"

  return "$harness_exit_code"
}

BUILD_EXIT=0
BL029_LANE_EXIT=0
BL029_SELFTEST_RUNS=0
BL029_SELFTEST_PASS=0
BL009_SELFTEST_RUNS=0
BL009_SELFTEST_PASS=0
DOCS_EXIT=0

BUILD_LOG="$OUT_DIR/build.log"
QA_BL029_LOG="$OUT_DIR/qa_bl029_lane.log"
DOCS_LOG="$OUT_DIR/docs_freshness.log"

set +e
cmake --build build_local --config Release --target LocusQ_Standalone -j 8 > "$BUILD_LOG" 2>&1
BUILD_EXIT=$?
set -e
if (( BUILD_EXIT == 0 )); then
  log_status "build_standalone" "PASS" "$BUILD_EXIT" "target=LocusQ_Standalone" "$BUILD_LOG"
else
  log_status "build_standalone" "FAIL" "$BUILD_EXIT" "build_failed" "$BUILD_LOG"
fi

if (( BUILD_EXIT == 0 )); then
  BL029_LANE_OUT="$OUT_DIR/qa_bl029_lane"
  set +e
  BL029_OUT_DIR="$BL029_LANE_OUT" "$ROOT_DIR/scripts/qa-bl029-audition-platform-lane-mac.sh" > "$QA_BL029_LOG" 2>&1
  BL029_LANE_EXIT=$?
  set -e
  if (( BL029_LANE_EXIT == 0 )); then
    log_status "qa_bl029_lane" "PASS" "$BL029_LANE_EXIT" "lane_passed" "$QA_BL029_LOG"
  else
    log_status "qa_bl029_lane" "FAIL" "$BL029_LANE_EXIT" "lane_failed" "$QA_BL029_LOG"
  fi

  if [[ ! -x "$STANDALONE_EXEC" && -x "$STANDALONE_APP/Contents/MacOS/LocusQ" ]]; then
    STANDALONE_EXEC="$STANDALONE_APP/Contents/MacOS/LocusQ"
  fi

  for i in $(seq 1 "$BL029_SELFTEST_RUNS_REQUIRED"); do
    BL029_SELFTEST_RUNS=$((BL029_SELFTEST_RUNS + 1))
    if run_selftest_iteration "bl029" "$i"; then
      BL029_SELFTEST_PASS=$((BL029_SELFTEST_PASS + 1))
    fi
  done
  if (( BL029_SELFTEST_PASS == BL029_SELFTEST_RUNS_REQUIRED )); then
    log_status "selftest_bl029_x${BL029_SELFTEST_RUNS_REQUIRED}" "PASS" "0" "passes=${BL029_SELFTEST_PASS}/${BL029_SELFTEST_RUNS_REQUIRED}" "$SELFTEST_SUMMARY_TSV"
  else
    log_status "selftest_bl029_x${BL029_SELFTEST_RUNS_REQUIRED}" "FAIL" "1" "passes=${BL029_SELFTEST_PASS}/${BL029_SELFTEST_RUNS_REQUIRED}" "$SELFTEST_SUMMARY_TSV"
  fi

  for i in $(seq 1 "$BL009_SELFTEST_RUNS_REQUIRED"); do
    BL009_SELFTEST_RUNS=$((BL009_SELFTEST_RUNS + 1))
    if run_selftest_iteration "bl009" "$i"; then
      BL009_SELFTEST_PASS=$((BL009_SELFTEST_PASS + 1))
    fi
  done
  if (( BL009_SELFTEST_PASS == BL009_SELFTEST_RUNS_REQUIRED )); then
    log_status "selftest_bl009_x${BL009_SELFTEST_RUNS_REQUIRED}" "PASS" "0" "passes=${BL009_SELFTEST_PASS}/${BL009_SELFTEST_RUNS_REQUIRED}" "$SELFTEST_SUMMARY_TSV"
  else
    log_status "selftest_bl009_x${BL009_SELFTEST_RUNS_REQUIRED}" "FAIL" "1" "passes=${BL009_SELFTEST_PASS}/${BL009_SELFTEST_RUNS_REQUIRED}" "$SELFTEST_SUMMARY_TSV"
  fi
else
  log_status "qa_bl029_lane" "FAIL" "127" "skipped_due_to_build_failure" "$QA_BL029_LOG"
  log_status "selftest_bl029_x${BL029_SELFTEST_RUNS_REQUIRED}" "FAIL" "127" "skipped_due_to_build_failure" "$SELFTEST_SUMMARY_TSV"
  log_status "selftest_bl009_x${BL009_SELFTEST_RUNS_REQUIRED}" "FAIL" "127" "skipped_due_to_build_failure" "$SELFTEST_SUMMARY_TSV"
fi

set +e
"$ROOT_DIR/scripts/validate-docs-freshness.sh" > "$DOCS_LOG" 2>&1
DOCS_EXIT=$?
set -e
if (( DOCS_EXIT == 0 )); then
  log_status "docs_freshness" "PASS" "$DOCS_EXIT" "validate-docs-freshness" "$DOCS_LOG"
else
  log_status "docs_freshness" "FAIL" "$DOCS_EXIT" "docs_freshness_failed" "$DOCS_LOG"
fi

printf "dimension\tvalue\tcount\n" > "$FAILURE_TAXONOMY_TSV"
if [[ -s "$SELFTEST_SUMMARY_TSV" ]]; then
  awk -F '\t' '
    FNR == 1 { next }
    {
      reason = ($4 == "" ? "none" : $4)
      exitCode = ($5 == "" ? "(empty)" : $5)
      signal = ($6 == "" ? "(empty)" : $6)
      signalName = ($7 == "" ? "(empty)" : $7)
      exitStatusSource = ($8 == "" ? "not_recorded" : $8)
      reasonCount[reason]++
      exitCount[exitCode]++
      signalCount[signal]++
      signalNameCount[signalName]++
      exitStatusSourceCount[exitStatusSource]++
    }
    END {
      for (k in reasonCount) printf("terminal_failure_reason\t%s\t%d\n", k, reasonCount[k])
      for (k in exitCount) printf("app_exit_code\t%s\t%d\n", k, exitCount[k])
      for (k in signalCount) printf("app_signal\t%s\t%d\n", k, signalCount[k])
      for (k in signalNameCount) printf("app_signal_name\t%s\t%d\n", k, signalNameCount[k])
      for (k in exitStatusSourceCount) printf("app_exit_status_source\t%s\t%d\n", k, exitStatusSourceCount[k])
    }
  ' "$SELFTEST_SUMMARY_TSV" | sort >> "$FAILURE_TAXONOMY_TSV"
fi

if ! awk -F '\t' '
  $1 == "terminal_failure_reason" && $2 == "app_exited_before_result" { found = 1 }
  END { exit(found ? 0 : 1) }
' "$FAILURE_TAXONOMY_TSV"; then
  printf "terminal_failure_reason\tapp_exited_before_result\t0\n" >> "$FAILURE_TAXONOMY_TSV"
fi

printf "criterion\trequired\tachieved\tresult\tdetail\n" > "$HARD_CRITERIA_TSV"
add_criterion() {
  local criterion="$1"
  local required="$2"
  local achieved="$3"
  local detail="$4"
  local result="PASS"
  if [[ "$required" != "$achieved" ]]; then
    result="FAIL"
  fi
  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$criterion")" \
    "$(sanitize_tsv_field "$required")" \
    "$(sanitize_tsv_field "$achieved")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$detail")" \
    >> "$HARD_CRITERIA_TSV"
}

add_criterion "build_standalone" "1" "$([[ $BUILD_EXIT -eq 0 ]] && echo 1 || echo 0)" "cmake --build build_local --config Release --target LocusQ_Standalone -j 8"
add_criterion "qa_bl029_lane" "1" "$([[ $BL029_LANE_EXIT -eq 0 ]] && echo 1 || echo 0)" "scripts/qa-bl029-audition-platform-lane-mac.sh"
add_criterion "selftest_bl029_runs" "$BL029_SELFTEST_RUNS_REQUIRED" "$BL029_SELFTEST_RUNS" "LOCUSQ_UI_SELFTEST_SCOPE=bl029"
add_criterion "selftest_bl029_passes" "$BL029_SELFTEST_RUNS_REQUIRED" "$BL029_SELFTEST_PASS" "expected_all_pass"
add_criterion "selftest_bl009_runs" "$BL009_SELFTEST_RUNS_REQUIRED" "$BL009_SELFTEST_RUNS" "LOCUSQ_UI_SELFTEST_BL009=1"
add_criterion "selftest_bl009_passes" "$BL009_SELFTEST_RUNS_REQUIRED" "$BL009_SELFTEST_PASS" "expected_all_pass"
add_criterion "docs_freshness" "1" "$([[ $DOCS_EXIT -eq 0 ]] && echo 1 || echo 0)" "scripts/validate-docs-freshness.sh"

OVERALL_RESULT="PASS"
if awk -F '\t' 'FNR > 1 && $4 != "PASS" { exit 1 }' "$HARD_CRITERIA_TSV"; then
  OVERALL_RESULT="PASS"
else
  OVERALL_RESULT="FAIL"
fi

{
  echo "Title: BL-029 Reliability Gate Contract"
  echo "Document Type: Test Evidence"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE_UTC}"
  echo "Last Modified Date: ${DOC_DATE_UTC}"
  echo
  echo "# BL-029 Reliability Gate Runner Slice P4"
  echo
  echo "## Command"
  echo "- \`./scripts/qa-bl029-reliability-gate-mac.sh\`"
  echo
  echo "## Ordered Gate Steps"
  echo "- build standalone"
  echo "- qa BL-029 lane"
  echo "- selftest BL-029 x${BL029_SELFTEST_RUNS_REQUIRED}"
  echo "- selftest BL-009 x${BL009_SELFTEST_RUNS_REQUIRED}"
  echo "- docs freshness"
  echo
  echo "## Result"
  echo "- overall: ${OVERALL_RESULT}"
  echo "- artifacts: \`${OUT_DIR#"$ROOT_DIR/"}\`"
  echo
  echo "## Machine-readable outputs"
  echo "- \`status.tsv\`"
  echo "- \`hard_criteria.tsv\`"
  echo "- \`failure_taxonomy.tsv\`"
  echo "- \`selftest_runs.tsv\`"
} > "$GATE_CONTRACT_MD"

echo "artifact_dir=$OUT_DIR"
echo "status_tsv=$STATUS_TSV"
echo "hard_criteria_tsv=$HARD_CRITERIA_TSV"
echo "failure_taxonomy_tsv=$FAILURE_TAXONOMY_TSV"
echo "gate_contract_md=$GATE_CONTRACT_MD"

if [[ "$OVERALL_RESULT" != "PASS" ]]; then
  exit 1
fi

exit 0
