#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

usage() {
  cat <<'USAGE'
Usage: qa-bl030-rl05-replay-reconcile-mac.sh --notes-dir <dir> [options]

Deterministic RL-05 reconciliation wrapper that chains:
  1) qa-bl030-manual-evidence-pack-mac.sh
  2) qa-bl030-manual-evidence-validate-mac.sh
  3) qa-bl030-device-matrix-capture-mac.sh

Options:
  --notes-dir <dir>     Manual notes directory for DEV-01..DEV-06 (required)
  --dev06-waiver <path> DEV-06 waiver path (optional, passed through to capture)
  --out-dir <path>      Output directory. Default: TestEvidence/bl030_rl05_reconcile_<timestamp>
  --skip-build          Pass --skip-build to device matrix capture
  --help, -h            Print this help

Wrapper outputs:
  status.tsv
  validation_matrix.tsv
  rl05_reconcile_summary.tsv
  blocker_taxonomy.tsv

Exit semantics:
  0 = RL-05 green
  1 = RL-05 blocker remains
  2 = usage/preflight error
USAGE
}

NOTES_DIR=""
DEV06_WAIVER=""
OUT_DIR="${ROOT_DIR}/TestEvidence/bl030_rl05_reconcile_${TIMESTAMP}"
SKIP_BUILD=0
CURRENT_STEP="initialization"
SIGNAL_TERMINATION_HANDLED=0
RECONCILE_DECISION_EMITTED=0
WRAPPER_BLOCKER_SEQ=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --notes-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --notes-dir requires a value" >&2; usage >&2; exit 2; }
      NOTES_DIR="$2"
      shift 2
      ;;
    --dev06-waiver)
      [[ $# -ge 2 ]] || { echo "ERROR: --dev06-waiver requires a value" >&2; usage >&2; exit 2; }
      DEV06_WAIVER="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --out-dir requires a value" >&2; usage >&2; exit 2; }
      OUT_DIR="$2"
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
      usage >&2
      exit 2
      ;;
  esac
done

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
VALIDATION_TSV="$OUT_DIR/validation_matrix.tsv"
SUMMARY_TSV="$OUT_DIR/rl05_reconcile_summary.tsv"
BLOCKER_TSV="$OUT_DIR/blocker_taxonomy.tsv"
PACK_LOG="$OUT_DIR/pack.run.log"
VALIDATE_LOG="$OUT_DIR/validate.run.log"
CAPTURE_LOG="$OUT_DIR/capture.run.log"

PACK_DIR="$OUT_DIR/pack"
VALIDATE_DIR="$OUT_DIR/validate"
CAPTURE_DIR="$OUT_DIR/capture"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "step\tresult\texit_code\tcommand\tdetail\tartifact\n" > "$VALIDATION_TSV"
printf "metric\tvalue\tdetail\tartifact\n" > "$SUMMARY_TSV"
printf "source\tblocker_id\tdevice_id\tcategory\tdetail\tartifact_path\n" > "$BLOCKER_TSV"

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

append_status() {
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

append_validation() {
  local step="$1"
  local result="$2"
  local exit_code="$3"
  local command="$4"
  local detail="$5"
  local artifact="$6"
  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$step")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$exit_code")" \
    "$(sanitize_tsv_field "$command")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$VALIDATION_TSV"
}

append_summary() {
  local metric="$1"
  local value="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$metric")" \
    "$(sanitize_tsv_field "$value")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$SUMMARY_TSV"
}

append_wrapper_blocker() {
  local source="$1"
  local device_id="$2"
  local category="$3"
  local detail="$4"
  local artifact_path="$5"
  WRAPPER_BLOCKER_SEQ=$((WRAPPER_BLOCKER_SEQ + 1))
  printf "%s\tBL030-RL05W-%03d\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$source")" \
    "$WRAPPER_BLOCKER_SEQ" \
    "$(sanitize_tsv_field "$device_id")" \
    "$(sanitize_tsv_field "$category")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact_path")" \
    >> "$BLOCKER_TSV"
}

record_reconcile_decision() {
  local result="$1"
  local exit_code="$2"
  local detail="$3"
  RECONCILE_DECISION_EMITTED=1
  append_status "rl05_reconcile_decision" "$result" "$exit_code" "$detail" "$SUMMARY_TSV"
  append_validation "rl05_reconcile_decision" "$result" "$exit_code" "wrapper_decision" "$detail" "$SUMMARY_TSV"
  append_summary "rl05_reconcile_result" "$result" "$detail" "$STATUS_TSV"
  append_summary "wrapper_exit_code" "$exit_code" "$detail" "$STATUS_TSV"
}

signal_to_exit_code() {
  local signal="$1"
  case "$signal" in
    HUP) printf '129' ;;
    INT) printf '130' ;;
    QUIT) printf '131' ;;
    TERM) printf '143' ;;
    *) printf '1' ;;
  esac
}

handle_wrapper_signal() {
  local signal="$1"
  local signal_exit_code
  local detail
  signal_exit_code="$(signal_to_exit_code "$signal")"

  if (( SIGNAL_TERMINATION_HANDLED == 1 )); then
    exit "$signal_exit_code"
  fi
  SIGNAL_TERMINATION_HANDLED=1
  set +e

  detail="wrapper_signal_termination;signal=${signal};signal_exit_code=${signal_exit_code};current_step=${CURRENT_STEP}"
  append_wrapper_blocker "wrapper" "DEV-ALL" "runtime_flake_abrt" "$detail" "$STATUS_TSV"
  append_status "signal_termination" "FAIL" "$signal_exit_code" "$detail" "$STATUS_TSV"
  append_validation "signal_termination" "FAIL" "$signal_exit_code" "signal:${signal}" "$detail" "$STATUS_TSV"
  if (( RECONCILE_DECISION_EMITTED == 0 )); then
    record_reconcile_decision "FAIL" "1" "$detail"
  fi

  echo "artifact_dir=$OUT_DIR"
  echo "status_tsv=$STATUS_TSV"
  echo "validation_matrix_tsv=$VALIDATION_TSV"
  echo "rl05_reconcile_summary_tsv=$SUMMARY_TSV"
  echo "blocker_taxonomy_tsv=$BLOCKER_TSV"
  exit "$signal_exit_code"
}

run_cmd() {
  local step="$1"
  local log_path="$2"
  shift 2
  CURRENT_STEP="$step"
  set +e
  "$@" > "$log_path" 2>&1
  local ec=$?
  set -e
  printf '%s' "$ec"
}

append_blockers_from() {
  local source="$1"
  local blocker_file="$2"
  local count=0

  if [[ -f "$blocker_file" ]]; then
    count="$(awk -F '\t' 'NR>1 && NF>=5 { c++ } END { print c+0 }' "$blocker_file")"
    awk -F '\t' -v source="$source" 'NR>1 && NF>=5 {
      printf "%s\t%s\t%s\t%s\t%s\t%s\n", source, $1, $2, $3, $4, $5
    }' "$blocker_file" >> "$BLOCKER_TSV"
  fi

  printf '%s' "$count"
}

resolve_single_note() {
  local pattern="$1"
  shopt -s nullglob
  local matches=("$NOTES_DIR"/$pattern)
  shopt -u nullglob

  if (( ${#matches[@]} == 1 )); then
    printf '%s' "${matches[0]}"
  else
    printf '%s' ""
  fi
}

PACK_SCRIPT="$ROOT_DIR/scripts/qa-bl030-manual-evidence-pack-mac.sh"
VALIDATE_SCRIPT="$ROOT_DIR/scripts/qa-bl030-manual-evidence-validate-mac.sh"
CAPTURE_SCRIPT="$ROOT_DIR/scripts/qa-bl030-device-matrix-capture-mac.sh"

trap 'handle_wrapper_signal HUP' HUP
trap 'handle_wrapper_signal INT' INT
trap 'handle_wrapper_signal QUIT' QUIT
trap 'handle_wrapper_signal TERM' TERM

PRECHECK_FAIL=0

if [[ -z "$NOTES_DIR" ]]; then
  PRECHECK_FAIL=1
  append_status "preflight_notes_dir" "FAIL" "2" "missing_required_argument" "--notes-dir"
  append_validation "preflight_notes_dir" "FAIL" "2" "--notes-dir" "missing_required_argument" "--notes-dir"
fi

if [[ -n "$NOTES_DIR" && ! -d "$NOTES_DIR" ]]; then
  PRECHECK_FAIL=1
  append_status "preflight_notes_dir" "FAIL" "2" "notes_dir_not_found" "$NOTES_DIR"
  append_validation "preflight_notes_dir" "FAIL" "2" "--notes-dir $NOTES_DIR" "notes_dir_not_found" "$NOTES_DIR"
fi

for script_path in "$PACK_SCRIPT" "$VALIDATE_SCRIPT" "$CAPTURE_SCRIPT"; do
  if [[ ! -x "$script_path" ]]; then
    PRECHECK_FAIL=1
    append_status "preflight_script" "FAIL" "2" "script_not_executable" "$script_path"
    append_validation "preflight_script" "FAIL" "2" "$script_path" "script_not_executable" "$script_path"
  fi
done

if (( PRECHECK_FAIL == 1 )); then
  record_reconcile_decision "FAIL" "2" "usage_preflight_error"
  echo "artifact_dir=$OUT_DIR"
  echo "status_tsv=$STATUS_TSV"
  echo "validation_matrix_tsv=$VALIDATION_TSV"
  echo "rl05_reconcile_summary_tsv=$SUMMARY_TSV"
  echo "blocker_taxonomy_tsv=$BLOCKER_TSV"
  exit 2
fi

append_status "preflight_notes_dir" "PASS" "0" "notes_dir_found" "$NOTES_DIR"
append_validation "preflight_notes_dir" "PASS" "0" "--notes-dir $NOTES_DIR" "notes_dir_found" "$NOTES_DIR"

mkdir -p "$PACK_DIR" "$VALIDATE_DIR" "$CAPTURE_DIR"

PACK_CMD=("$PACK_SCRIPT" "--notes-dir" "$NOTES_DIR" "--out-dir" "$PACK_DIR")
PACK_EC="$(run_cmd "manual_pack" "$PACK_LOG" "${PACK_CMD[@]}")"
if [[ "$PACK_EC" == "0" ]]; then
  append_status "manual_pack" "PASS" "$PACK_EC" "command_succeeded" "$PACK_DIR/status.tsv"
  append_validation "manual_pack" "PASS" "$PACK_EC" "${PACK_CMD[*]}" "command_succeeded" "$PACK_LOG"
else
  append_status "manual_pack" "FAIL" "$PACK_EC" "command_failed" "$PACK_DIR/status.tsv"
  append_validation "manual_pack" "FAIL" "$PACK_EC" "${PACK_CMD[*]}" "command_failed" "$PACK_LOG"
fi

CHECKLIST_PATH="$PACK_DIR/manual_evidence_checklist.tsv"
VALIDATE_CMD=("$VALIDATE_SCRIPT" "--input" "$CHECKLIST_PATH" "--out-dir" "$VALIDATE_DIR")
VALIDATE_EC="$(run_cmd "manual_validate" "$VALIDATE_LOG" "${VALIDATE_CMD[@]}")"
if [[ "$VALIDATE_EC" == "0" ]]; then
  append_status "manual_validate" "PASS" "$VALIDATE_EC" "command_succeeded" "$VALIDATE_DIR/status.tsv"
  append_validation "manual_validate" "PASS" "$VALIDATE_EC" "${VALIDATE_CMD[*]}" "command_succeeded" "$VALIDATE_LOG"
else
  append_status "manual_validate" "FAIL" "$VALIDATE_EC" "command_failed" "$VALIDATE_DIR/status.tsv"
  append_validation "manual_validate" "FAIL" "$VALIDATE_EC" "${VALIDATE_CMD[*]}" "command_failed" "$VALIDATE_LOG"
fi

DEV01_NOTE="$(resolve_single_note 'dev01_*manual_notes.md')"
DEV02_NOTE="$(resolve_single_note 'dev02_*manual_notes.md')"
DEV03_NOTE="$(resolve_single_note 'dev03_*manual_notes.md')"
DEV04_NOTE="$(resolve_single_note 'dev04_*manual_notes.md')"
DEV05_NOTE="$(resolve_single_note 'dev05_*manual_notes.md')"
DEV06_NOTE="$(resolve_single_note 'dev06_*manual_notes.md')"

CAPTURE_CMD=("$CAPTURE_SCRIPT" "--out-dir" "$CAPTURE_DIR")
if [[ -n "$DEV01_NOTE" ]]; then CAPTURE_CMD+=("--dev01-manual-notes" "$DEV01_NOTE"); fi
if [[ -n "$DEV02_NOTE" ]]; then CAPTURE_CMD+=("--dev02-manual-notes" "$DEV02_NOTE"); fi
if [[ -n "$DEV03_NOTE" ]]; then CAPTURE_CMD+=("--dev03-manual-notes" "$DEV03_NOTE"); fi
if [[ -n "$DEV04_NOTE" ]]; then CAPTURE_CMD+=("--dev04-manual-notes" "$DEV04_NOTE"); fi
if [[ -n "$DEV05_NOTE" ]]; then CAPTURE_CMD+=("--dev05-manual-notes" "$DEV05_NOTE"); fi
if [[ -n "$DEV06_NOTE" ]]; then CAPTURE_CMD+=("--dev06-manual-notes" "$DEV06_NOTE"); fi
if [[ -n "$DEV06_WAIVER" ]]; then CAPTURE_CMD+=("--dev06-waiver" "$DEV06_WAIVER"); fi
if (( SKIP_BUILD == 1 )); then CAPTURE_CMD+=("--skip-build"); fi

CAPTURE_EC="$(run_cmd "device_matrix_capture" "$CAPTURE_LOG" "${CAPTURE_CMD[@]}")"
if [[ "$CAPTURE_EC" == "0" ]]; then
  append_status "device_matrix_capture" "PASS" "$CAPTURE_EC" "command_succeeded" "$CAPTURE_DIR/status.tsv"
  append_validation "device_matrix_capture" "PASS" "$CAPTURE_EC" "${CAPTURE_CMD[*]}" "command_succeeded" "$CAPTURE_LOG"
else
  append_status "device_matrix_capture" "FAIL" "$CAPTURE_EC" "command_failed" "$CAPTURE_DIR/status.tsv"
  append_validation "device_matrix_capture" "FAIL" "$CAPTURE_EC" "${CAPTURE_CMD[*]}" "command_failed" "$CAPTURE_LOG"
fi

PACK_BLOCKERS="$(append_blockers_from "manual_pack" "$PACK_DIR/blocker_taxonomy.tsv")"
VALIDATE_BLOCKERS="$(append_blockers_from "manual_validate" "$VALIDATE_DIR/blocker_taxonomy.tsv")"
CAPTURE_BLOCKERS="$(append_blockers_from "device_matrix_capture" "$CAPTURE_DIR/blocker_taxonomy.tsv")"
TOTAL_BLOCKERS=$((PACK_BLOCKERS + VALIDATE_BLOCKERS + CAPTURE_BLOCKERS))

append_summary "notes_dir" "$NOTES_DIR" "input_notes_directory" "$NOTES_DIR"
append_summary "dev06_waiver" "${DEV06_WAIVER:-none}" "optional_input" "${DEV06_WAIVER:-none}"
append_summary "pack_exit_code" "$PACK_EC" "manual_pack_exit" "$PACK_LOG"
append_summary "validate_exit_code" "$VALIDATE_EC" "manual_validate_exit" "$VALIDATE_LOG"
append_summary "capture_exit_code" "$CAPTURE_EC" "device_matrix_capture_exit" "$CAPTURE_LOG"
append_summary "pack_blockers" "$PACK_BLOCKERS" "rows_from_pack_blocker_taxonomy" "$PACK_DIR/blocker_taxonomy.tsv"
append_summary "validate_blockers" "$VALIDATE_BLOCKERS" "rows_from_validate_blocker_taxonomy" "$VALIDATE_DIR/blocker_taxonomy.tsv"
append_summary "capture_blockers" "$CAPTURE_BLOCKERS" "rows_from_capture_blocker_taxonomy" "$CAPTURE_DIR/blocker_taxonomy.tsv"
append_summary "total_blockers" "$TOTAL_BLOCKERS" "aggregated_blocker_rows" "$BLOCKER_TSV"
append_summary "checklist_path" "$CHECKLIST_PATH" "pack_output_used_for_validation" "$CHECKLIST_PATH"

WRAPPER_EXIT=1
WRAPPER_RESULT="FAIL"
WRAPPER_DETAIL="rl05_blocker_remains"

if [[ "$PACK_EC" == "2" || "$CAPTURE_EC" == "2" ]]; then
  WRAPPER_EXIT=2
  WRAPPER_RESULT="FAIL"
  WRAPPER_DETAIL="usage_preflight_error_from_child"
elif [[ "$PACK_EC" == "0" && "$VALIDATE_EC" == "0" && "$CAPTURE_EC" == "0" ]]; then
  WRAPPER_EXIT=0
  WRAPPER_RESULT="PASS"
  WRAPPER_DETAIL="rl05_green"
fi

record_reconcile_decision "$WRAPPER_RESULT" "$WRAPPER_EXIT" "$WRAPPER_DETAIL"

echo "artifact_dir=$OUT_DIR"
echo "status_tsv=$STATUS_TSV"
echo "validation_matrix_tsv=$VALIDATION_TSV"
echo "rl05_reconcile_summary_tsv=$SUMMARY_TSV"
echo "blocker_taxonomy_tsv=$BLOCKER_TSV"

exit "$WRAPPER_EXIT"
