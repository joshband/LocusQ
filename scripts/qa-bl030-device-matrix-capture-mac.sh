#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
DOC_DATE="$(date -u +%Y-%m-%d)"

usage() {
  cat <<'USAGE'
Usage: qa-bl030-device-matrix-capture-mac.sh [options]

Deterministic RL-05 device matrix capture harness for DEV-01..DEV-06.

Options:
  --out-dir <path>              Output artifact directory.
                                Default: TestEvidence/bl030_rl05_harness_g3_<timestamp>
  --dev01-manual-notes <path>   Manual evidence path for DEV-01.
  --dev02-manual-notes <path>   Manual evidence path for DEV-02.
  --dev03-manual-notes <path>   Manual evidence path for DEV-03.
  --dev04-manual-notes <path>   Manual evidence path for DEV-04.
  --dev05-manual-notes <path>   Manual evidence path for DEV-05.
  --dev06-manual-notes <path>   Manual evidence path for DEV-06 when not waived.
  --dev06-waiver <path>         DEV-06 waiver path. If present, DEV-06 records N/A.
  --skip-build                  Skip preflight build.
  --help, -h                    Print this help.

Artifacts:
  status.tsv
  dev_matrix_results.tsv
  blocker_taxonomy.tsv
  replay_transcript.log
  command_transcript.log (compat)
  harness_contract.md

Blocker categories:
  deterministic_missing_manual_evidence
  runtime_flake_abrt
  not_applicable_with_waiver

Exit semantics:
  0  RL-05 PASS (DEV-01..DEV-05 PASS and DEV-06 PASS or N/A with waiver)
  1  RL-05 FAIL
  2  Usage/invocation error
USAGE
}

OUT_DIR="${BL030_RL05_OUT_DIR:-$ROOT_DIR/TestEvidence/bl030_rl05_device_matrix_capture_${TIMESTAMP}}"
DEV01_MANUAL_NOTES=""
DEV02_MANUAL_NOTES=""
DEV03_MANUAL_NOTES=""
DEV04_MANUAL_NOTES=""
DEV05_MANUAL_NOTES=""
DEV06_MANUAL_NOTES=""
DEV06_WAIVER=""
SKIP_BUILD=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --out-dir requires a value" >&2; usage; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    --dev01-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev01-manual-notes requires a value" >&2; usage; exit 2; }
      DEV01_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev02-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev02-manual-notes requires a value" >&2; usage; exit 2; }
      DEV02_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev03-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev03-manual-notes requires a value" >&2; usage; exit 2; }
      DEV03_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev04-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev04-manual-notes requires a value" >&2; usage; exit 2; }
      DEV04_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev05-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev05-manual-notes requires a value" >&2; usage; exit 2; }
      DEV05_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev06-manual-notes)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev06-manual-notes requires a value" >&2; usage; exit 2; }
      DEV06_MANUAL_NOTES="$2"
      shift 2
      ;;
    --dev06-waiver)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev06-waiver requires a value" >&2; usage; exit 2; }
      DEV06_WAIVER="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

mkdir -p "$OUT_DIR"

if [[ -z "$DEV01_MANUAL_NOTES" ]]; then DEV01_MANUAL_NOTES="$OUT_DIR/dev01_quad_manual_notes.md"; fi
if [[ -z "$DEV02_MANUAL_NOTES" ]]; then DEV02_MANUAL_NOTES="$OUT_DIR/dev02_laptop_manual_notes.md"; fi
if [[ -z "$DEV03_MANUAL_NOTES" ]]; then DEV03_MANUAL_NOTES="$OUT_DIR/dev03_headphone_generic_manual_notes.md"; fi
if [[ -z "$DEV04_MANUAL_NOTES" ]]; then DEV04_MANUAL_NOTES="$OUT_DIR/dev04_steam_manual_notes.md"; fi
if [[ -z "$DEV05_MANUAL_NOTES" ]]; then DEV05_MANUAL_NOTES="$OUT_DIR/dev05_builtin_mic_manual_notes.md"; fi
if [[ -z "$DEV06_MANUAL_NOTES" ]]; then DEV06_MANUAL_NOTES="$OUT_DIR/dev06_external_mic_manual_notes.md"; fi

STATUS_TSV="$OUT_DIR/status.tsv"
DEV_MATRIX_TSV="$OUT_DIR/dev_matrix_results.tsv"
BLOCKER_TSV="$OUT_DIR/blocker_taxonomy.tsv"
HARNESS_CONTRACT_MD="$OUT_DIR/harness_contract.md"
TRANSCRIPT_LOG="$OUT_DIR/replay_transcript.log"
LEGACY_TRANSCRIPT_LOG="$OUT_DIR/command_transcript.log"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "dev_id\tresult\ttimestamp_utc\tclassification\tevidence_path\tnotes\n" > "$DEV_MATRIX_TSV"
printf "blocker_id\tdev_id\tcategory\tdetail\tevidence_path\n" > "$BLOCKER_TSV"
: > "$TRANSCRIPT_LOG"
: > "$LEGACY_TRANSCRIPT_LOG"

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

log_transcript() {
  local message="$1"
  local line
  line="[$(date -u +%Y-%m-%dT%H:%M:%SZ)] $message"
  printf "%s\n" "$line" >> "$TRANSCRIPT_LOG"
  printf "%s\n" "$line" >> "$LEGACY_TRANSCRIPT_LOG"
}

BLOCKER_SEQ=0
add_blocker() {
  local dev_id="$1"
  local category="$2"
  local detail="$3"
  local artifact="$4"
  BLOCKER_SEQ=$((BLOCKER_SEQ + 1))
  printf "BL030-G3-%03d\t%s\t%s\t%s\t%s\n" \
    "$BLOCKER_SEQ" \
    "$(sanitize_tsv_field "$dev_id")" \
    "$(sanitize_tsv_field "$category")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$BLOCKER_TSV"
}

append_dev_result() {
  local dev_id="$1"
  local result="$2"
  local classification="$3"
  local evidence_path="$4"
  local notes="$5"
  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$dev_id" \
    "$result" \
    "$DOC_TS" \
    "$(sanitize_tsv_field "$classification")" \
    "$(sanitize_tsv_field "$evidence_path")" \
    "$(sanitize_tsv_field "$notes")" \
    >> "$DEV_MATRIX_TSV"
}

latest_artifact_dir() {
  local pattern="$1"
  local found=""
  found="$(ls -dt $pattern 2>/dev/null | head -n 1 || true)"
  printf '%s' "$found"
}

file_has_abrt_signature() {
  local file_path="$1"
  if [[ ! -f "$file_path" ]]; then
    return 1
  fi
  rg -n "(Abort trap|app_exited_before_result|signal_name=ABRT|launch_mode_failed_open|renderExitCode=134)" "$file_path" >/dev/null 2>&1
}

manual_evidence_present() {
  local file_path="$1"
  [[ -f "$file_path" && -s "$file_path" ]]
}

run_command_capture() {
  local step="$1"
  local cmd="$2"
  local log_path="$3"
  log_transcript "cmd[$step]=$cmd"
  set +e
  bash -lc "$cmd" > "$log_path" 2>&1
  local ec=$?
  set -e
  if (( ec == 0 )); then
    log_status "$step" "PASS" "$ec" "command_succeeded" "$log_path"
  else
    log_status "$step" "FAIL" "$ec" "command_failed" "$log_path"
  fi
  printf '%s' "$ec"
}

if (( SKIP_BUILD == 0 )); then
  BUILD_LOG="$OUT_DIR/dev00_build.log"
  BUILD_EC="$(run_command_capture "preflight_build" "cd '$ROOT_DIR' && cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8" "$BUILD_LOG")"
else
  BUILD_EC=0
  log_status "preflight_build" "PASS" "0" "skipped_by_flag" "$OUT_DIR"
fi

if (( BUILD_EC != 0 )); then
  add_blocker "DEV-ALL" "runtime_flake_abrt" "preflight_build_failed" "$OUT_DIR/dev00_build.log"
fi

DEV01_RESULT="FAIL"
DEV01_CLASS="deterministic_missing_manual_evidence"
DEV01_NOTES=""
DEV01_EVIDENCE=""
{
  DEV01_BL018_LOG="$OUT_DIR/dev01_bl018_profile_matrix.log"
  DEV01_REAPER_LOG="$OUT_DIR/dev01_reaper_headless.log"
  BEFORE_BL018="$(latest_artifact_dir "$ROOT_DIR/TestEvidence/bl018_profile_matrix_*")"
  DEV01_BL018_EC="$(run_command_capture "dev01_bl018_profile_matrix" "cd '$ROOT_DIR' && ./scripts/qa-bl018-profile-matrix-strict-mac.sh" "$DEV01_BL018_LOG")"
  AFTER_BL018="$(latest_artifact_dir "$ROOT_DIR/TestEvidence/bl018_profile_matrix_*")"
  BL018_ARTIFACT="$AFTER_BL018"
  if [[ -z "$BL018_ARTIFACT" ]]; then BL018_ARTIFACT="$BEFORE_BL018"; fi

  DEV01_REAPER_EC="$(run_command_capture "dev01_reaper_headless" "cd '$ROOT_DIR' && ./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap" "$DEV01_REAPER_LOG")"

  runtime_flake=0
  if (( DEV01_BL018_EC != 0 )) && (file_has_abrt_signature "$DEV01_BL018_LOG" || [[ -n "$BL018_ARTIFACT" && -f "$BL018_ARTIFACT/production_selftest.log" ]] && file_has_abrt_signature "$BL018_ARTIFACT/production_selftest.log"); then
    runtime_flake=1
  fi
  if (( DEV01_REAPER_EC != 0 )) && file_has_abrt_signature "$DEV01_REAPER_LOG"; then
    runtime_flake=1
  fi
  if (( runtime_flake == 1 )); then
    add_blocker "DEV-01" "runtime_flake_abrt" "automation_lane_abrt_signature" "$DEV01_BL018_LOG;$DEV01_REAPER_LOG"
  fi

  manual_missing=0
  if ! manual_evidence_present "$DEV01_MANUAL_NOTES"; then
    manual_missing=1
    add_blocker "DEV-01" "deterministic_missing_manual_evidence" "missing_manual_notes=$DEV01_MANUAL_NOTES" "$DEV01_MANUAL_NOTES"
  fi

  DEV01_EVIDENCE="$DEV01_BL018_LOG;$DEV01_REAPER_LOG;$DEV01_MANUAL_NOTES"
  if (( DEV01_BL018_EC == 0 && DEV01_REAPER_EC == 0 && manual_missing == 0 )); then
    DEV01_RESULT="PASS"
    DEV01_CLASS="none"
    DEV01_NOTES="automation_and_manual_checks_passed"
  else
    if (( manual_missing == 1 )); then
      DEV01_CLASS="deterministic_missing_manual_evidence"
    elif (( runtime_flake == 1 )); then
      DEV01_CLASS="runtime_flake_abrt"
    else
      DEV01_CLASS="deterministic_missing_manual_evidence"
    fi
    DEV01_NOTES="bl018_exit=$DEV01_BL018_EC;reaper_exit=$DEV01_REAPER_EC;manual_missing=$manual_missing"
  fi
  append_dev_result "DEV-01" "$DEV01_RESULT" "$DEV01_CLASS" "$DEV01_EVIDENCE" "$DEV01_NOTES"
}

run_bl009_contract_dev() {
  local dev_id="$1"
  local manual_notes="$2"
  local log_path="$3"
  local step_label="$4"
  local artifact_glob="$5"
  local script_cmd="$6"

  local result="FAIL"
  local class="deterministic_missing_manual_evidence"
  local notes=""
  local evidence=""
  local runtime_flake=0
  local manual_missing=0

  local before_artifact
  before_artifact="$(latest_artifact_dir "$artifact_glob")"
  local ec
  ec="$(run_command_capture "$step_label" "cd '$ROOT_DIR' && $script_cmd" "$log_path")"
  local after_artifact
  after_artifact="$(latest_artifact_dir "$artifact_glob")"
  local artifact_dir="$after_artifact"
  if [[ -z "$artifact_dir" ]]; then artifact_dir="$before_artifact"; fi

  if (( ec != 0 )); then
    if file_has_abrt_signature "$log_path" || [[ -n "$artifact_dir" && -f "$artifact_dir/status.tsv" ]] && file_has_abrt_signature "$artifact_dir/status.tsv"; then
      runtime_flake=1
      add_blocker "$dev_id" "runtime_flake_abrt" "lane_failed_with_abrt_signature" "$log_path;${artifact_dir:-none}"
    fi
  fi

  if ! manual_evidence_present "$manual_notes"; then
    manual_missing=1
    add_blocker "$dev_id" "deterministic_missing_manual_evidence" "missing_manual_notes=$manual_notes" "$manual_notes"
  fi

  evidence="$log_path;${artifact_dir:-none};$manual_notes"
  if (( ec == 0 && manual_missing == 0 )); then
    result="PASS"
    class="none"
    notes="automation_and_manual_checks_passed"
  else
    if (( manual_missing == 1 )); then
      class="deterministic_missing_manual_evidence"
    elif (( runtime_flake == 1 )); then
      class="runtime_flake_abrt"
    else
      class="deterministic_missing_manual_evidence"
    fi
    notes="lane_exit=$ec;manual_missing=$manual_missing"
  fi

  append_dev_result "$dev_id" "$result" "$class" "$evidence" "$notes"
}

run_bl009_contract_dev "DEV-02" "$DEV02_MANUAL_NOTES" "$OUT_DIR/dev02_bl009_headphone_contract.log" "dev02_bl009_contract" "$ROOT_DIR/TestEvidence/bl009_headphone_contract_*" "./scripts/qa-bl009-headphone-contract-mac.sh"
run_bl009_contract_dev "DEV-03" "$DEV03_MANUAL_NOTES" "$OUT_DIR/dev03_bl009_headphone_profile.log" "dev03_bl009_profile_contract" "$ROOT_DIR/TestEvidence/bl009_headphone_profile_contract_*" "./scripts/qa-bl009-headphone-profile-contract-mac.sh"
run_bl009_contract_dev "DEV-04" "$DEV04_MANUAL_NOTES" "$OUT_DIR/dev04_bl009_headphone_contract.log" "dev04_bl009_contract" "$ROOT_DIR/TestEvidence/bl009_headphone_contract_*" "./scripts/qa-bl009-headphone-contract-mac.sh"

run_bl026_selftest_dev() {
  local dev_id="$1"
  local manual_notes="$2"
  local log_path="$3"
  local step_label="$4"

  local ec
  ec="$(run_command_capture "$step_label" "cd '$ROOT_DIR' && LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh" "$log_path")"

  local runtime_flake=0
  local manual_missing=0
  if (( ec != 0 )) && file_has_abrt_signature "$log_path"; then
    runtime_flake=1
    add_blocker "$dev_id" "runtime_flake_abrt" "bl026_selftest_abrt_signature" "$log_path"
  fi
  if ! manual_evidence_present "$manual_notes"; then
    manual_missing=1
    add_blocker "$dev_id" "deterministic_missing_manual_evidence" "missing_manual_notes=$manual_notes" "$manual_notes"
  fi

  local result="FAIL"
  local class="deterministic_missing_manual_evidence"
  local notes="selftest_exit=$ec;manual_missing=$manual_missing"
  if (( ec == 0 && manual_missing == 0 )); then
    result="PASS"
    class="none"
    notes="automation_and_manual_checks_passed"
  else
    if (( manual_missing == 1 )); then
      class="deterministic_missing_manual_evidence"
    elif (( runtime_flake == 1 )); then
      class="runtime_flake_abrt"
    else
      class="deterministic_missing_manual_evidence"
    fi
  fi

  append_dev_result "$dev_id" "$result" "$class" "$log_path;$manual_notes" "$notes"
}

run_bl026_selftest_dev "DEV-05" "$DEV05_MANUAL_NOTES" "$OUT_DIR/dev05_bl026_selftest.log" "dev05_bl026_selftest"

if [[ -n "$DEV06_WAIVER" && -f "$DEV06_WAIVER" && -s "$DEV06_WAIVER" ]]; then
  add_blocker "DEV-06" "not_applicable_with_waiver" "dev06_marked_na_with_waiver" "$DEV06_WAIVER"
  append_dev_result "DEV-06" "N/A" "not_applicable_with_waiver" "$DEV06_WAIVER" "external_mic_hardware_waived"
  log_status "dev06_waiver" "PASS" "0" "waiver_applied" "$DEV06_WAIVER"
else
  run_bl026_selftest_dev "DEV-06" "$DEV06_MANUAL_NOTES" "$OUT_DIR/dev06_bl026_selftest.log" "dev06_bl026_selftest"
fi

RL05_PASS=1
for dev in DEV-01 DEV-02 DEV-03 DEV-04 DEV-05; do
  if ! awk -F'\t' -v d="$dev" 'NR>1 && $1==d && $2=="PASS" { found=1 } END { exit(found ? 0 : 1) }' "$DEV_MATRIX_TSV"; then
    RL05_PASS=0
  fi
done
if ! awk -F'\t' 'NR>1 && $1=="DEV-06" && ($2=="PASS" || $2=="N/A") { found=1 } END { exit(found ? 0 : 1) }' "$DEV_MATRIX_TSV"; then
  RL05_PASS=0
fi

if (( RL05_PASS == 1 )); then
  log_status "rl05_gate_decision" "PASS" "0" "dev01_05_pass_and_dev06_pass_or_na" "$DEV_MATRIX_TSV"
  OVERALL="PASS"
  EXIT_CODE=0
else
  log_status "rl05_gate_decision" "FAIL" "1" "rl05_criteria_not_met" "$DEV_MATRIX_TSV"
  OVERALL="FAIL"
  EXIT_CODE=1
fi

{
  echo "Title: BL-030 RL-05 Device Matrix Capture Harness Contract"
  echo "Document Type: Test Evidence"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE}"
  echo "Last Modified Date: ${DOC_DATE}"
  echo
  echo "# BL-030 RL-05 Device Matrix Capture Harness"
  echo
  echo "## Command"
  echo "- \`./scripts/qa-bl030-device-matrix-capture-mac.sh\`"
  echo
  echo "## Blocker Categories"
  echo "- deterministic_missing_manual_evidence"
  echo "- runtime_flake_abrt"
  echo "- not_applicable_with_waiver"
  echo
  echo "## Exit Semantics"
  echo "- exit 0: DEV-01..DEV-05 are PASS and DEV-06 is PASS or N/A with waiver"
  echo "- exit 1: RL-05 fail criteria"
  echo "- exit 2: usage/invocation error"
  echo
  echo "## Artifacts"
  echo "- \`status.tsv\`"
  echo "- \`dev_matrix_results.tsv\`"
  echo "- \`blocker_taxonomy.tsv\`"
  echo "- \`replay_transcript.log\`"
  echo "- \`command_transcript.log\`"
  echo
  echo "## Result"
  echo "- overall: ${OVERALL}"
  echo "- artifact_dir: \`${OUT_DIR#"$ROOT_DIR/"}\`"
} > "$HARNESS_CONTRACT_MD"

echo "artifact_dir=$OUT_DIR"
echo "status_tsv=$STATUS_TSV"
echo "dev_matrix_tsv=$DEV_MATRIX_TSV"
echo "blocker_taxonomy_tsv=$BLOCKER_TSV"
echo "harness_contract_md=$HARNESS_CONTRACT_MD"

exit "$EXIT_CODE"
