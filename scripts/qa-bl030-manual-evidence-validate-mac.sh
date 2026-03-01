#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE="$(date -u +%Y-%m-%d)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

usage() {
  cat <<'USAGE'
Usage: qa-bl030-manual-evidence-validate-mac.sh --input <manual_evidence_checklist.tsv> [--out-dir <path>]

Deterministic RL-05 manual evidence validator for DEV-01..DEV-06.

Required input columns (tab-separated, exact names):
  device_id
  evidence_status
  artifact_path
  operator
  timestamp_iso8601
  notes

Required rows:
  DEV-01, DEV-02, DEV-03, DEV-04, DEV-05, DEV-06

Outputs:
  status.tsv
  manual_evidence_validation.tsv
  blocker_taxonomy.tsv
  harness_contract.md

Exit semantics:
  0 = RL-05 manual evidence complete
  1 = incomplete/invalid manual evidence
USAGE
}

INPUT_FILE=""
OUT_DIR="${ROOT_DIR}/TestEvidence/bl030_rl05_manual_intake_${TIMESTAMP}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --input requires a value" >&2
        exit 1
      fi
      INPUT_FILE="$2"
      shift 2
      ;;
    --out-dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --out-dir requires a value" >&2
        exit 1
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
      exit 1
      ;;
  esac
done

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
VALIDATION_TSV="${OUT_DIR}/manual_evidence_validation.tsv"
BLOCKER_TSV="${OUT_DIR}/blocker_taxonomy.tsv"
CONTRACT_MD="${OUT_DIR}/harness_contract.md"

printf "step\tresult\texit_code\tdetail\tartifact\n" > "$STATUS_TSV"
printf "device_id\tevidence_status\tartifact_path\toperator\ttimestamp_iso8601\tnotes\tvalidation_result\treason\n" > "$VALIDATION_TSV"
printf "blocker_id\tdevice_id\tcategory\tdetail\tartifact_path\n" > "$BLOCKER_TSV"

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

BLOCKER_SEQ=0
OVERALL_FAIL=0

add_blocker() {
  local device_id="$1"
  local category="$2"
  local detail="$3"
  local artifact_path="$4"
  BLOCKER_SEQ=$((BLOCKER_SEQ + 1))
  OVERALL_FAIL=1
  printf "BL030-G6-%03d\t%s\t%s\t%s\t%s\n" \
    "$BLOCKER_SEQ" \
    "$(sanitize_tsv_field "$device_id")" \
    "$(sanitize_tsv_field "$category")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact_path")" \
    >> "$BLOCKER_TSV"
}

is_iso8601_utc() {
  local value="$1"
  [[ "$value" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$ ]]
}

is_required_device() {
  local device_id="$1"
  case "$device_id" in
    DEV-01|DEV-02|DEV-03|DEV-04|DEV-05|DEV-06) return 0 ;;
    *) return 1 ;;
  esac
}

dev_seen_count() {
  local device_id="$1"
  case "$device_id" in
    DEV-01) printf '%s' "$DEV01_SEEN" ;;
    DEV-02) printf '%s' "$DEV02_SEEN" ;;
    DEV-03) printf '%s' "$DEV03_SEEN" ;;
    DEV-04) printf '%s' "$DEV04_SEEN" ;;
    DEV-05) printf '%s' "$DEV05_SEEN" ;;
    DEV-06) printf '%s' "$DEV06_SEEN" ;;
    *) printf '0' ;;
  esac
}

inc_dev_seen() {
  local device_id="$1"
  case "$device_id" in
    DEV-01) DEV01_SEEN=$((DEV01_SEEN + 1)) ;;
    DEV-02) DEV02_SEEN=$((DEV02_SEEN + 1)) ;;
    DEV-03) DEV03_SEEN=$((DEV03_SEEN + 1)) ;;
    DEV-04) DEV04_SEEN=$((DEV04_SEEN + 1)) ;;
    DEV-05) DEV05_SEEN=$((DEV05_SEEN + 1)) ;;
    DEV-06) DEV06_SEEN=$((DEV06_SEEN + 1)) ;;
    *) ;;
  esac
}

append_validation_row() {
  local device_id="$1"
  local evidence_status="$2"
  local artifact_path="$3"
  local operator="$4"
  local timestamp_iso8601="$5"
  local notes="$6"
  local validation_result="$7"
  local reason="$8"
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$device_id")" \
    "$(sanitize_tsv_field "$evidence_status")" \
    "$(sanitize_tsv_field "$artifact_path")" \
    "$(sanitize_tsv_field "$operator")" \
    "$(sanitize_tsv_field "$timestamp_iso8601")" \
    "$(sanitize_tsv_field "$notes")" \
    "$(sanitize_tsv_field "$validation_result")" \
    "$(sanitize_tsv_field "$reason")" \
    >> "$VALIDATION_TSV"
}

if [[ -z "$INPUT_FILE" ]]; then
  log_status "preflight_input" "FAIL" "1" "missing_required_input_argument" "none"
  add_blocker "DEV-ALL" "deterministic_missing_manual_evidence" "missing_required_input_argument" "none"
  {
    echo "Title: BL-030 RL-05 Manual Evidence Intake Contract"
    echo "Document Type: Test Evidence"
    echo "Author: APC Codex"
    echo "Created Date: ${DOC_DATE}"
    echo "Last Modified Date: ${DOC_DATE}"
    echo
    echo "# BL-030 Manual Evidence Intake Validation"
    echo
    echo "- overall: FAIL"
    echo "- reason: missing --input argument"
  } > "$CONTRACT_MD"
  exit 1
fi

if [[ ! -f "$INPUT_FILE" ]]; then
  log_status "preflight_input" "FAIL" "1" "input_file_not_found" "$INPUT_FILE"
  add_blocker "DEV-ALL" "deterministic_missing_manual_evidence" "input_file_not_found" "$INPUT_FILE"
  {
    echo "Title: BL-030 RL-05 Manual Evidence Intake Contract"
    echo "Document Type: Test Evidence"
    echo "Author: APC Codex"
    echo "Created Date: ${DOC_DATE}"
    echo "Last Modified Date: ${DOC_DATE}"
    echo
    echo "# BL-030 Manual Evidence Intake Validation"
    echo
    echo "- overall: FAIL"
    echo "- input: \`${INPUT_FILE#"$ROOT_DIR/"}\`"
    echo "- reason: input_file_not_found"
  } > "$CONTRACT_MD"
  exit 1
fi

log_status "preflight_input" "PASS" "0" "input_file_found" "$INPUT_FILE"

read_header=0
header_line=""
while IFS= read -r line || [[ -n "$line" ]]; do
  if [[ -n "$line" ]]; then
    header_line="$line"
    read_header=1
    break
  fi
done < "$INPUT_FILE"

if (( read_header == 0 )); then
  log_status "header_schema" "FAIL" "1" "empty_input_file" "$INPUT_FILE"
  add_blocker "DEV-ALL" "deterministic_missing_manual_evidence" "empty_input_file" "$INPUT_FILE"
  OVERALL="FAIL"
  {
    echo "Title: BL-030 RL-05 Manual Evidence Intake Contract"
    echo "Document Type: Test Evidence"
    echo "Author: APC Codex"
    echo "Created Date: ${DOC_DATE}"
    echo "Last Modified Date: ${DOC_DATE}"
    echo
    echo "# BL-030 Manual Evidence Intake Validation"
    echo
    echo "## Result"
    echo "- overall: ${OVERALL}"
    echo "- evaluated_at: ${DOC_TS}"
    echo "- input: \`${INPUT_FILE#"$ROOT_DIR/"}\`"
    echo
    echo "## Exit Semantics"
    echo "- exit 0: RL-05 manual evidence complete"
    echo "- exit 1: incomplete/invalid manual evidence"
  } > "$CONTRACT_MD"
  exit 1
fi

IFS=$'\t' read -r -a header_cols <<< "$header_line"

IDX_DEVICE=-1
IDX_STATUS=-1
IDX_ARTIFACT=-1
IDX_OPERATOR=-1
IDX_TIMESTAMP=-1
IDX_NOTES=-1

for i in "${!header_cols[@]}"; do
  case "${header_cols[$i]}" in
    device_id) IDX_DEVICE="$i" ;;
    evidence_status) IDX_STATUS="$i" ;;
    artifact_path) IDX_ARTIFACT="$i" ;;
    operator) IDX_OPERATOR="$i" ;;
    timestamp_iso8601) IDX_TIMESTAMP="$i" ;;
    notes) IDX_NOTES="$i" ;;
    *) ;;
  esac
done

if (( IDX_DEVICE < 0 || IDX_STATUS < 0 || IDX_ARTIFACT < 0 || IDX_OPERATOR < 0 || IDX_TIMESTAMP < 0 || IDX_NOTES < 0 )); then
  log_status "header_schema" "FAIL" "1" "missing_required_columns" "$INPUT_FILE"
  add_blocker "DEV-ALL" "deterministic_missing_manual_evidence" "missing_required_columns:expected=device_id,evidence_status,artifact_path,operator,timestamp_iso8601,notes" "$INPUT_FILE"
else
  log_status "header_schema" "PASS" "0" "required_columns_present" "$INPUT_FILE"
fi

DEV01_SEEN=0
DEV02_SEEN=0
DEV03_SEEN=0
DEV04_SEEN=0
DEV05_SEEN=0
DEV06_SEEN=0

ROW_INDEX=1
while IFS= read -r line || [[ -n "$line" ]]; do
  ROW_INDEX=$((ROW_INDEX + 1))
  if [[ -z "$line" ]]; then
    continue
  fi
  IFS=$'\t' read -r -a cols <<< "$line"

  device_id=""
  evidence_status=""
  artifact_path=""
  operator=""
  timestamp_iso8601=""
  notes=""

  if (( IDX_DEVICE >= 0 )); then device_id="${cols[$IDX_DEVICE]:-}"; fi
  if (( IDX_STATUS >= 0 )); then evidence_status="${cols[$IDX_STATUS]:-}"; fi
  if (( IDX_ARTIFACT >= 0 )); then artifact_path="${cols[$IDX_ARTIFACT]:-}"; fi
  if (( IDX_OPERATOR >= 0 )); then operator="${cols[$IDX_OPERATOR]:-}"; fi
  if (( IDX_TIMESTAMP >= 0 )); then timestamp_iso8601="${cols[$IDX_TIMESTAMP]:-}"; fi
  if (( IDX_NOTES >= 0 )); then notes="${cols[$IDX_NOTES]:-}"; fi

  row_fail=0
  row_reason="ok"

  if [[ -z "$device_id" || -z "$evidence_status" || -z "$artifact_path" || -z "$operator" || -z "$timestamp_iso8601" || -z "$notes" ]]; then
    row_fail=1
    row_reason="required_field_missing"
    add_blocker "${device_id:-DEV-UNKNOWN}" "deterministic_missing_manual_evidence" "required_field_missing:row=${ROW_INDEX}" "${artifact_path:-$INPUT_FILE}"
  fi

  if [[ "$row_reason" == "ok" ]] && ! is_required_device "$device_id"; then
    row_fail=1
    row_reason="unknown_device_id"
    add_blocker "$device_id" "deterministic_missing_manual_evidence" "unknown_device_id:row=${ROW_INDEX}" "${artifact_path:-$INPUT_FILE}"
  fi

  if [[ "$row_reason" == "ok" ]] && is_required_device "$device_id"; then
    inc_dev_seen "$device_id"
    if (( "$(dev_seen_count "$device_id")" > 1 )); then
      row_fail=1
      row_reason="duplicate_device_row"
      add_blocker "$device_id" "deterministic_missing_manual_evidence" "duplicate_device_row:row=${ROW_INDEX}" "$INPUT_FILE"
    fi
  fi

  if [[ "$row_reason" == "ok" ]] && ! is_iso8601_utc "$timestamp_iso8601"; then
    row_fail=1
    row_reason="invalid_timestamp_iso8601"
    add_blocker "${device_id:-DEV-UNKNOWN}" "deterministic_missing_manual_evidence" "invalid_timestamp_iso8601:row=${ROW_INDEX}" "${artifact_path:-$INPUT_FILE}"
  fi

  if [[ "$row_reason" == "ok" ]] && [[ ! -e "$artifact_path" ]]; then
    row_fail=1
    row_reason="artifact_path_not_found"
    add_blocker "${device_id:-DEV-UNKNOWN}" "deterministic_missing_manual_evidence" "artifact_path_not_found:row=${ROW_INDEX}" "$artifact_path"
  fi

  if [[ "$row_reason" == "ok" ]]; then
    case "$evidence_status" in
      complete|present)
        ;;
      waived|not_applicable_with_waiver)
        if [[ "$device_id" == "DEV-06" ]]; then
          # DEV-06 waiver is an allowed non-failing completion state when artifact exists.
          row_reason="waiver_applied"
        else
          row_fail=1
          row_reason="waiver_not_allowed_for_device"
          add_blocker "$device_id" "deterministic_missing_manual_evidence" "waiver_not_allowed_for_device" "$artifact_path"
        fi
        ;;
      runtime_flake_abrt)
        row_fail=1
        row_reason="runtime_flake_declared"
        add_blocker "$device_id" "runtime_flake_abrt" "runtime_flake_declared_by_evidence_status" "$artifact_path"
        ;;
      missing|incomplete|invalid)
        row_fail=1
        row_reason="evidence_status_not_complete"
        add_blocker "$device_id" "deterministic_missing_manual_evidence" "evidence_status_not_complete" "$artifact_path"
        ;;
      *)
        row_fail=1
        row_reason="invalid_evidence_status"
        add_blocker "$device_id" "deterministic_missing_manual_evidence" "invalid_evidence_status=${evidence_status}" "$artifact_path"
        ;;
    esac
  fi

  if (( row_fail == 1 )); then
    append_validation_row "$device_id" "$evidence_status" "$artifact_path" "$operator" "$timestamp_iso8601" "$notes" "FAIL" "$row_reason"
  else
    if [[ "$row_reason" == "waiver_applied" ]]; then
      append_validation_row "$device_id" "$evidence_status" "$artifact_path" "$operator" "$timestamp_iso8601" "$notes" "PASS" "waiver_applied"
    else
      append_validation_row "$device_id" "$evidence_status" "$artifact_path" "$operator" "$timestamp_iso8601" "$notes" "PASS" "row_valid"
    fi
  fi
done < <(tail -n +2 "$INPUT_FILE")

for device in DEV-01 DEV-02 DEV-03 DEV-04 DEV-05 DEV-06; do
  seen="$(dev_seen_count "$device")"
  if (( seen == 0 )); then
    add_blocker "$device" "deterministic_missing_manual_evidence" "required_row_missing" "$INPUT_FILE"
    append_validation_row "$device" "missing" "$INPUT_FILE" "unknown" "$DOC_TS" "required row absent" "FAIL" "required_row_missing"
  fi
done

if (( OVERALL_FAIL == 0 )); then
  log_status "manual_evidence_gate" "PASS" "0" "rl05_manual_evidence_complete" "$VALIDATION_TSV"
  OVERALL="PASS"
  EXIT_CODE=0
else
  log_status "manual_evidence_gate" "FAIL" "1" "rl05_manual_evidence_incomplete_or_invalid" "$VALIDATION_TSV"
  OVERALL="FAIL"
  EXIT_CODE=1
fi

{
  echo "Title: BL-030 RL-05 Manual Evidence Intake Contract"
  echo "Document Type: Test Evidence"
  echo "Author: APC Codex"
  echo "Created Date: ${DOC_DATE}"
  echo "Last Modified Date: ${DOC_DATE}"
  echo
  echo "# BL-030 Manual Evidence Intake Validation"
  echo
  echo "## Command"
  echo "- \`./scripts/qa-bl030-manual-evidence-validate-mac.sh --input <checklist.tsv> --out-dir <artifact_dir>\`"
  echo
  echo "## Required Input Columns"
  echo "- \`device_id\`"
  echo "- \`evidence_status\`"
  echo "- \`artifact_path\`"
  echo "- \`operator\`"
  echo "- \`timestamp_iso8601\`"
  echo "- \`notes\`"
  echo
  echo "## Required Device Rows"
  echo "- DEV-01"
  echo "- DEV-02"
  echo "- DEV-03"
  echo "- DEV-04"
  echo "- DEV-05"
  echo "- DEV-06"
  echo
  echo "## Blocker Categories"
  echo "- deterministic_missing_manual_evidence"
  echo "- runtime_flake_abrt"
  echo "- not_applicable_with_waiver (invalid waiver usage only)"
  echo
  echo "## Waiver Semantics"
  echo "- DEV-06 may use \`waived\` or \`not_applicable_with_waiver\` when artifact_path exists."
  echo "- Valid DEV-06 waiver rows are PASS with reason \`waiver_applied\`."
  echo "- Waiver status on DEV-01..DEV-05 is invalid and fails intake."
  echo
  echo "## Exit Semantics"
  echo "- exit 0: RL-05 manual evidence complete"
  echo "- exit 1: incomplete/invalid manual evidence"
  echo
  echo "## Artifacts"
  echo "- \`status.tsv\`"
  echo "- \`manual_evidence_validation.tsv\`"
  echo "- \`blocker_taxonomy.tsv\`"
  echo "- \`harness_contract.md\`"
  echo
  echo "## Result"
  echo "- overall: ${OVERALL}"
  echo "- evaluated_at: ${DOC_TS}"
  echo "- input: \`${INPUT_FILE#"$ROOT_DIR/"}\`"
  echo "- artifact_dir: \`${OUT_DIR#"$ROOT_DIR/"}\`"
} > "$CONTRACT_MD"

echo "artifact_dir=$OUT_DIR"
echo "status_tsv=$STATUS_TSV"
echo "manual_evidence_validation_tsv=$VALIDATION_TSV"
echo "blocker_taxonomy_tsv=$BLOCKER_TSV"
echo "harness_contract_md=$CONTRACT_MD"

exit "$EXIT_CODE"
