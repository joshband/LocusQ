#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE="$(date -u +%Y-%m-%d)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

NOTES_DIR=""
OUT_DIR="${ROOT_DIR}/TestEvidence/bl030_rl05_manual_pack_${TIMESTAMP}"

usage() {
  cat <<'USAGE'
Usage: qa-bl030-manual-evidence-pack-mac.sh --notes-dir <dir> [--out-dir <path>]

Deterministic RL-05 manual evidence packet compiler.
Parses six manual note files and compiles a G6-compatible checklist.

Required input note patterns in --notes-dir:
  dev01_*manual_notes.md
  dev02_*manual_notes.md
  dev03_*manual_notes.md
  dev04_*manual_notes.md
  dev05_*manual_notes.md
  dev06_*manual_notes.md

Required fields in each note file:
  device_id
  evidence_status
  artifact_path
  operator
  timestamp_iso8601
  notes

Outputs:
  status.tsv
  manual_evidence_checklist.tsv
  pack_validation.tsv
  blocker_taxonomy.tsv
  harness_contract.md

Exit semantics:
  0 = checklist complete/valid
  1 = missing/invalid inputs
  2 = usage error
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --notes-dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --notes-dir requires a value" >&2
        exit 2
      fi
      NOTES_DIR="$2"
      shift 2
      ;;
    --out-dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --out-dir requires a value" >&2
        exit 2
      fi
      OUT_DIR="$2"
      shift 2
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

if [[ -z "$NOTES_DIR" ]]; then
  echo "ERROR: --notes-dir is required" >&2
  usage >&2
  exit 2
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
CHECKLIST_TSV="$OUT_DIR/manual_evidence_checklist.tsv"
PACK_VALIDATION_TSV="$OUT_DIR/pack_validation.tsv"
BLOCKER_TSV="$OUT_DIR/blocker_taxonomy.tsv"
CONTRACT_MD="$OUT_DIR/harness_contract.md"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "device_id\tnote_file\tdevice_id_field\tevidence_status\tartifact_path\toperator\ttimestamp_iso8601\tnotes\tvalidation_result\treason\n" > "$PACK_VALIDATION_TSV"
printf "blocker_id\tdevice_id\tcategory\tdetail\tartifact_path\n" > "$BLOCKER_TSV"
printf "device_id\tevidence_status\tartifact_path\toperator\ttimestamp_iso8601\tnotes\n" > "$CHECKLIST_TSV"

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

append_pack_validation() {
  local device_id="$1"
  local note_file="$2"
  local device_id_field="$3"
  local evidence_status="$4"
  local artifact_path="$5"
  local operator="$6"
  local timestamp_iso8601="$7"
  local notes="$8"
  local validation_result="$9"
  local reason="${10}"
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$device_id")" \
    "$(sanitize_tsv_field "$note_file")" \
    "$(sanitize_tsv_field "$device_id_field")" \
    "$(sanitize_tsv_field "$evidence_status")" \
    "$(sanitize_tsv_field "$artifact_path")" \
    "$(sanitize_tsv_field "$operator")" \
    "$(sanitize_tsv_field "$timestamp_iso8601")" \
    "$(sanitize_tsv_field "$notes")" \
    "$(sanitize_tsv_field "$validation_result")" \
    "$(sanitize_tsv_field "$reason")" \
    >> "$PACK_VALIDATION_TSV"
}

BLOCKER_SEQ=0
OVERALL_FAIL=0

add_blocker() {
  local device_id="$1"
  local category="$2"
  local detail="$3"
  local artifact_path="$4"
  BLOCKER_SEQ=$((BLOCKER_SEQ + 1))
  OVERALL_FAIL=1
  printf "BL030-I1-%03d\t%s\t%s\t%s\t%s\n" \
    "$BLOCKER_SEQ" \
    "$(sanitize_tsv_field "$device_id")" \
    "$(sanitize_tsv_field "$category")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact_path")" \
    >> "$BLOCKER_TSV"
}

extract_field() {
  local file="$1"
  local key="$2"
  awk -v key="$key" '
    {
      line=$0
      sub(/\r$/, "", line)
      prefix="^[[:space:]-]*" key ":[[:space:]]*"
      if (line ~ prefix) {
        sub(prefix, "", line)
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", line)
        print line
        exit
      }
    }
  ' "$file"
}

is_iso8601_utc() {
  local value="$1"
  [[ "$value" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$ ]]
}

is_allowed_status() {
  local value="$1"
  case "$value" in
    complete|present|waived|not_applicable_with_waiver|runtime_flake_abrt|missing|incomplete|invalid) return 0 ;;
    *) return 1 ;;
  esac
}

normalize_evidence_status() {
  local value="$1"
  case "$value" in
    pass) printf '%s' "complete" ;;
    fail) printf '%s' "incomplete" ;;
    *) printf '%s' "$value" ;;
  esac
}

resolve_artifact_path() {
  local value="$1"
  if [[ -z "$value" ]]; then
    printf '%s' ""
    return
  fi

  if [[ "$value" == /* ]]; then
    printf '%s' "$value"
  else
    printf '%s' "$ROOT_DIR/$value"
  fi
}

if [[ ! -d "$NOTES_DIR" ]]; then
  log_status "preflight_notes_dir" "FAIL" "1" "notes_dir_not_found" "$NOTES_DIR"
  add_blocker "DEV-ALL" "deterministic_missing_manual_evidence" "notes_dir_not_found" "$NOTES_DIR"
else
  log_status "preflight_notes_dir" "PASS" "0" "notes_dir_found" "$NOTES_DIR"
fi

DEV_IDS=("DEV-01" "DEV-02" "DEV-03" "DEV-04" "DEV-05" "DEV-06")
DEV_GLOBS=("dev01_*manual_notes.md" "dev02_*manual_notes.md" "dev03_*manual_notes.md" "dev04_*manual_notes.md" "dev05_*manual_notes.md" "dev06_*manual_notes.md")

for idx in "${!DEV_IDS[@]}"; do
  dev_id="${DEV_IDS[$idx]}"
  dev_glob="${DEV_GLOBS[$idx]}"
  note_file=""

  if [[ -d "$NOTES_DIR" ]]; then
    shopt -s nullglob
    matches=("$NOTES_DIR"/$dev_glob)
    shopt -u nullglob

    if [[ "${#matches[@]}" -eq 1 ]]; then
      note_file="${matches[0]}"
    elif [[ "${#matches[@]}" -eq 0 ]]; then
      add_blocker "$dev_id" "deterministic_missing_manual_evidence" "note_file_missing_for_pattern:${dev_glob}" "$NOTES_DIR"
      append_pack_validation "$dev_id" "" "" "" "" "" "" "" "FAIL" "note_file_missing"
      printf "%s\t\t\t\t\t\n" "$dev_id" >> "$CHECKLIST_TSV"
      continue
    else
      add_blocker "$dev_id" "deterministic_schema_mismatch" "multiple_note_files_for_pattern:${dev_glob}" "$NOTES_DIR"
      append_pack_validation "$dev_id" "${matches[*]}" "" "" "" "" "" "" "FAIL" "multiple_note_files"
      printf "%s\t\t\t\t\t\n" "$dev_id" >> "$CHECKLIST_TSV"
      continue
    fi
  else
    append_pack_validation "$dev_id" "" "" "" "" "" "" "" "FAIL" "notes_dir_missing"
    printf "%s\t\t\t\t\t\n" "$dev_id" >> "$CHECKLIST_TSV"
    continue
  fi

  device_id_field="$(extract_field "$note_file" "device_id")"
  evidence_status_raw="$(extract_field "$note_file" "evidence_status")"
  evidence_status="$(normalize_evidence_status "$evidence_status_raw")"
  artifact_path="$(extract_field "$note_file" "artifact_path")"
  operator="$(extract_field "$note_file" "operator")"
  timestamp_iso8601="$(extract_field "$note_file" "timestamp_iso8601")"
  notes="$(extract_field "$note_file" "notes")"

  reason_list=()

  if [[ -z "$device_id_field" || -z "$evidence_status_raw" || -z "$artifact_path" || -z "$operator" || -z "$timestamp_iso8601" || -z "$notes" ]]; then
    reason_list+=("missing_required_field")
    add_blocker "$dev_id" "deterministic_missing_manual_evidence" "missing_required_field_in_note" "$note_file"
  fi

  if [[ -n "$device_id_field" && "$device_id_field" != "$dev_id" ]]; then
    reason_list+=("device_id_mismatch")
    add_blocker "$dev_id" "deterministic_schema_mismatch" "device_id_field_mismatch:${device_id_field}" "$note_file"
  fi

  if [[ -n "$evidence_status_raw" ]] && ! is_allowed_status "$evidence_status"; then
    reason_list+=("invalid_evidence_status")
    add_blocker "$dev_id" "deterministic_schema_mismatch" "invalid_evidence_status:${evidence_status_raw}" "$note_file"
  fi

  if [[ -n "$timestamp_iso8601" ]] && ! is_iso8601_utc "$timestamp_iso8601"; then
    reason_list+=("invalid_timestamp_iso8601")
    add_blocker "$dev_id" "deterministic_schema_mismatch" "invalid_timestamp_iso8601:${timestamp_iso8601}" "$note_file"
  fi

  if [[ -n "$artifact_path" ]]; then
    resolved_artifact="$(resolve_artifact_path "$artifact_path")"
    if [[ ! -f "$resolved_artifact" ]]; then
      reason_list+=("artifact_path_missing")
      add_blocker "$dev_id" "deterministic_missing_manual_evidence" "artifact_path_not_found:${artifact_path}" "$note_file"
    fi
  fi

  validation_result="PASS"
  validation_reason="ok"
  if [[ "${#reason_list[@]}" -gt 0 ]]; then
    validation_result="FAIL"
    validation_reason="$(IFS=,; echo "${reason_list[*]}")"
    OVERALL_FAIL=1
  fi

  append_pack_validation "$dev_id" "$note_file" "$device_id_field" "$evidence_status" "$artifact_path" "$operator" "$timestamp_iso8601" "$notes" "$validation_result" "$validation_reason"
  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$device_id_field")" \
    "$(sanitize_tsv_field "$evidence_status")" \
    "$(sanitize_tsv_field "$artifact_path")" \
    "$(sanitize_tsv_field "$operator")" \
    "$(sanitize_tsv_field "$timestamp_iso8601")" \
    "$(sanitize_tsv_field "$notes")" \
    >> "$CHECKLIST_TSV"
done

if [[ "$OVERALL_FAIL" -eq 0 ]]; then
  log_status "pack_compile" "PASS" "0" "manual_evidence_checklist_valid" "$CHECKLIST_TSV"
  OVERALL="PASS"
  EXIT_CODE=0
else
  log_status "pack_compile" "FAIL" "1" "manual_evidence_checklist_invalid" "$CHECKLIST_TSV"
  OVERALL="FAIL"
  EXIT_CODE=1
fi

{
  echo "Title: BL-030 RL-05 Manual Evidence Packet Compiler Contract"
  echo "Document Type: Test Evidence"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE}"
  echo "Last Modified Date: ${DOC_DATE}"
  echo
  echo "# BL-030 RL-05 Manual Evidence Packet Compiler"
  echo
  echo "## Result"
  echo "- overall: ${OVERALL}"
  echo "- evaluated_at: ${DOC_TS}"
  echo "- notes_dir: ${NOTES_DIR}"
  echo "- checklist: ${CHECKLIST_TSV}"
  echo
  echo "## Required Output Schema"
  echo "- Columns: device_id, evidence_status, artifact_path, operator, timestamp_iso8601, notes"
  echo "- Required rows: DEV-01..DEV-06"
  echo
  echo "## Exit Semantics"
  echo "- exit 0: checklist complete/valid"
  echo "- exit 1: missing/invalid inputs"
  echo "- exit 2: usage error"
} > "$CONTRACT_MD"

echo "artifact_dir=${OUT_DIR}"
echo "status_tsv=${STATUS_TSV}"
echo "manual_evidence_checklist_tsv=${CHECKLIST_TSV}"
echo "pack_validation_tsv=${PACK_VALIDATION_TSV}"
echo "blocker_taxonomy_tsv=${BLOCKER_TSV}"
echo "harness_contract_md=${CONTRACT_MD}"

exit "$EXIT_CODE"
