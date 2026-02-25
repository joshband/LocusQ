#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

DEFAULT_OUT_DIR="$ROOT_DIR/TestEvidence/bl032_structure_guardrails_${TIMESTAMP}"
OUT_DIR="${BL032_OUT_DIR:-$DEFAULT_OUT_DIR}"

MAX_PLUGINPROCESSOR_LINES="${BL032_MAX_PLUGINPROCESSOR_LINES:-3200}"
MAX_PLUGINEDITOR_LINES="${BL032_MAX_PLUGINEDITOR_LINES:-800}"

usage() {
  cat <<USAGE
Usage: ./scripts/qa-bl032-structure-guardrails-mac.sh [options]

Options:
  --out-dir <path>                   Artifact output directory.
  --max-pluginprocessor-lines <N>    Threshold for Source/PluginProcessor.cpp (default: ${MAX_PLUGINPROCESSOR_LINES}).
  --max-plugineditor-lines <N>       Threshold for Source/PluginEditor.cpp (default: ${MAX_PLUGINEDITOR_LINES}).
  --help                             Show this help.

Exit codes:
  0  All guardrails pass.
  1  One or more guardrails fail.
  2  Usage/configuration error.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --out-dir requires a path" >&2
        usage >&2
        exit 2
      fi
      OUT_DIR="$2"
      shift 2
      ;;
    --max-pluginprocessor-lines)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --max-pluginprocessor-lines requires an integer" >&2
        usage >&2
        exit 2
      fi
      MAX_PLUGINPROCESSOR_LINES="$2"
      shift 2
      ;;
    --max-plugineditor-lines)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --max-plugineditor-lines requires an integer" >&2
        usage >&2
        exit 2
      fi
      MAX_PLUGINEDITOR_LINES="$2"
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

if ! [[ "$MAX_PLUGINPROCESSOR_LINES" =~ ^[0-9]+$ ]]; then
  echo "ERROR: invalid --max-pluginprocessor-lines value: $MAX_PLUGINPROCESSOR_LINES" >&2
  exit 2
fi
if ! [[ "$MAX_PLUGINEDITOR_LINES" =~ ^[0-9]+$ ]]; then
  echo "ERROR: invalid --max-plugineditor-lines value: $MAX_PLUGINEDITOR_LINES" >&2
  exit 2
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
GUARDRAIL_REPORT_TSV="$OUT_DIR/guardrail_report.tsv"
BLOCKER_TAXONOMY_TSV="$OUT_DIR/blocker_taxonomy.tsv"
GUARDRAIL_CONTRACT_MD="$OUT_DIR/guardrail_contract.md"

printf "check\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "guard_id\tcategory\tresult\tobserved\tthreshold\tdetail\tartifact\n" > "$GUARDRAIL_REPORT_TSV"
printf "blocker_class\tcount\tguard_ids\tdetail\n" > "$BLOCKER_TAXONOMY_TSV"

TOTAL_GUARDS=0
FAILED_GUARDS=0
LINE_TOTAL=0
LINE_FAIL=0
LINE_GUARDS=""
LINE_DETAILS=""
EDGE_TOTAL=0
EDGE_FAIL=0
EDGE_GUARDS=""
EDGE_DETAILS=""
DIR_TOTAL=0
DIR_FAIL=0
DIR_GUARDS=""
DIR_DETAILS=""

sanitize_field() {
  local value="${1:-}"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  value="${value//$'\r'/ }"
  printf "%s" "$value"
}

append_csv_token() {
  local existing="$1"
  local token="$2"
  if [[ -z "$existing" ]]; then
    printf "%s" "$token"
  else
    printf "%s,%s" "$existing" "$token"
  fi
}

record_guard() {
  local guard_id="$1"
  local category="$2"
  local result="$3"
  local observed="$4"
  local threshold="$5"
  local detail="$6"
  local artifact="$7"

  TOTAL_GUARDS=$((TOTAL_GUARDS + 1))
  case "$category" in
    line_count_threshold)
      LINE_TOTAL=$((LINE_TOTAL + 1))
      ;;
    forbidden_dependency_edge)
      EDGE_TOTAL=$((EDGE_TOTAL + 1))
      ;;
    required_module_directory)
      DIR_TOTAL=$((DIR_TOTAL + 1))
      ;;
  esac

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$guard_id")" \
    "$(sanitize_field "$category")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$observed")" \
    "$(sanitize_field "$threshold")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$GUARDRAIL_REPORT_TSV"

  if [[ "$result" == "FAIL" ]]; then
    FAILED_GUARDS=$((FAILED_GUARDS + 1))
    case "$category" in
      line_count_threshold)
        LINE_FAIL=$((LINE_FAIL + 1))
        LINE_GUARDS="$(append_csv_token "$LINE_GUARDS" "$guard_id")"
        LINE_DETAILS="$(append_csv_token "$LINE_DETAILS" "$detail")"
        ;;
      forbidden_dependency_edge)
        EDGE_FAIL=$((EDGE_FAIL + 1))
        EDGE_GUARDS="$(append_csv_token "$EDGE_GUARDS" "$guard_id")"
        EDGE_DETAILS="$(append_csv_token "$EDGE_DETAILS" "$detail")"
        ;;
      required_module_directory)
        DIR_FAIL=$((DIR_FAIL + 1))
        DIR_GUARDS="$(append_csv_token "$DIR_GUARDS" "$guard_id")"
        DIR_DETAILS="$(append_csv_token "$DIR_DETAILS" "$detail")"
        ;;
    esac
  fi
}

record_status() {
  local check="$1"
  local result="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$check")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$STATUS_TSV"
}

require_command() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    record_status "tool_${cmd}" "PASS" "$(command -v "$cmd")" ""
  else
    record_status "tool_${cmd}" "FAIL" "missing command: $cmd" ""
    FAILED_GUARDS=$((FAILED_GUARDS + 1))
  fi
}

check_line_threshold() {
  local guard_id="$1"
  local file_path="$2"
  local threshold="$3"

  local abs_file="$ROOT_DIR/$file_path"
  if [[ ! -f "$abs_file" ]]; then
    record_guard "$guard_id" "line_count_threshold" "FAIL" "missing_file" "<= ${threshold}" "required file missing" "$file_path"
    return
  fi

  local line_count
  line_count="$(wc -l < "$abs_file" | tr -d ' ')"
  if [[ "$line_count" -le "$threshold" ]]; then
    record_guard "$guard_id" "line_count_threshold" "PASS" "$line_count" "<= ${threshold}" "line-count threshold satisfied" "$file_path"
  else
    record_guard "$guard_id" "line_count_threshold" "FAIL" "$line_count" "<= ${threshold}" "line-count exceeds threshold" "$file_path"
  fi
}

check_required_module_dir() {
  local guard_id="$1"
  local module_dir="$2"
  local abs_dir="$ROOT_DIR/$module_dir"

  if [[ ! -d "$abs_dir" ]]; then
    record_guard "$guard_id" "required_module_directory" "FAIL" "missing" "directory exists with >=1 source file" "required module directory is absent" "$module_dir"
    return
  fi

  local source_count
  source_count="$(find "$abs_dir" -type f \( -name "*.h" -o -name "*.cpp" \) | wc -l | tr -d ' ')"
  if [[ "$source_count" -ge 1 ]]; then
    record_guard "$guard_id" "required_module_directory" "PASS" "files=${source_count}" "directory exists with >=1 source file" "required module directory present" "$module_dir"
  else
    record_guard "$guard_id" "required_module_directory" "FAIL" "files=0" "directory exists with >=1 source file" "module directory exists but has no source files" "$module_dir"
  fi
}

check_forbidden_include_in_paths() {
  local guard_id="$1"
  local search_scope="$2"
  local pattern="$3"
  local threshold_desc="$4"
  local path_glob="$5"
  local category="forbidden_dependency_edge"

  local matches=""
  if [[ "$search_scope" == "files" ]]; then
    IFS='|' read -r -a files <<< "$path_glob"
    for file in "${files[@]}"; do
      local abs_file="$ROOT_DIR/$file"
      if [[ -f "$abs_file" ]]; then
        local file_hits
        file_hits="$(rg -n --no-heading -e "$pattern" "$abs_file" || true)"
        if [[ -n "$file_hits" ]]; then
          if [[ -n "$matches" ]]; then
            matches+=$'\n'
          fi
          matches+="$file_hits"
        fi
      fi
    done
  else
    local abs_dir="$ROOT_DIR/$path_glob"
    if [[ ! -d "$abs_dir" ]]; then
      record_guard "$guard_id" "$category" "SKIP" "directory_missing" "$threshold_desc" "scope directory not present yet" "$path_glob"
      return
    fi
    matches="$(rg -n --no-heading -g "*.h" -g "*.cpp" -e "$pattern" "$abs_dir" || true)"
  fi

  if [[ -z "$matches" ]]; then
    record_guard "$guard_id" "$category" "PASS" "matches=0" "$threshold_desc" "no forbidden dependency edges detected" "$path_glob"
  else
    local match_count
    local sample
    match_count="$(printf "%s\n" "$matches" | sed '/^$/d' | wc -l | tr -d ' ')"
    sample="$(printf "%s\n" "$matches" | head -n 3 | paste -sd ';' -)"
    record_guard "$guard_id" "$category" "FAIL" "matches=${match_count}" "$threshold_desc" "sample=${sample}" "$path_glob"
  fi
}

write_blocker_taxonomy() {
  local has_blockers=0

  if [[ "$LINE_FAIL" -gt 0 ]]; then
    has_blockers=1
  fi
  if [[ "$EDGE_FAIL" -gt 0 ]]; then
    has_blockers=1
  fi
  if [[ "$DIR_FAIL" -gt 0 ]]; then
    has_blockers=1
  fi

  printf "%s\t%s\t%s\t%s\n" \
    "line_count_threshold" \
    "$LINE_FAIL" \
    "$(sanitize_field "${LINE_GUARDS:-none}")" \
    "$(sanitize_field "${LINE_DETAILS:-none}")" \
    >> "$BLOCKER_TAXONOMY_TSV"

  printf "%s\t%s\t%s\t%s\n" \
    "forbidden_dependency_edge" \
    "$EDGE_FAIL" \
    "$(sanitize_field "${EDGE_GUARDS:-none}")" \
    "$(sanitize_field "${EDGE_DETAILS:-none}")" \
    >> "$BLOCKER_TAXONOMY_TSV"

  printf "%s\t%s\t%s\t%s\n" \
    "required_module_directory" \
    "$DIR_FAIL" \
    "$(sanitize_field "${DIR_GUARDS:-none}")" \
    "$(sanitize_field "${DIR_DETAILS:-none}")" \
    >> "$BLOCKER_TAXONOMY_TSV"

  if [[ "$has_blockers" -eq 0 ]]; then
    printf "%s\t%s\t%s\t%s\n" "none" "0" "none" "no blockers detected" >> "$BLOCKER_TAXONOMY_TSV"
  fi
}

write_guardrail_contract() {
  cat > "$GUARDRAIL_CONTRACT_MD" <<EOF
Title: BL-032 Slice C Structural Guardrail Contract
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-032 Slice C Structural Guardrail Contract

## Purpose
Define deterministic lint guardrails that block regressions toward monolithic PluginProcessor and PluginEditor architecture and enforce Slice A module boundary expectations.

## Threshold Contract

| Guard ID | Contract |
|---|---|
| BL032-G-001 | \`Source/PluginProcessor.cpp\` line count must be <= ${MAX_PLUGINPROCESSOR_LINES}. |
| BL032-G-002 | \`Source/PluginEditor.cpp\` line count must be <= ${MAX_PLUGINEDITOR_LINES}. |
| BL032-G-101 | \`PluginProcessor.cpp/.h\` must not include \`PluginEditor.h\`. |
| BL032-G-102 | \`Source/shared_contracts/*\` must not include \`PluginProcessor.h\` or \`PluginEditor.h\`. |
| BL032-G-103 | \`Source/processor_core/*\` must not include \`PluginEditor.h\`. |
| BL032-G-104 | \`Source/processor_bridge/*\` must not include \`PluginEditor.h\`. |
| BL032-G-105 | \`Source/editor_webview/*\` must not include \`PluginProcessor.h\`. |
| BL032-G-106 | \`Source/editor_shell/*\` must not include \`SpatialRenderer.h\`. |
| BL032-G-201 | \`Source/shared_contracts\` directory exists and contains >=1 source file. |
| BL032-G-202 | \`Source/processor_core\` directory exists and contains >=1 source file. |
| BL032-G-203 | \`Source/processor_bridge\` directory exists and contains >=1 source file. |
| BL032-G-204 | \`Source/editor_shell\` directory exists and contains >=1 source file. |
| BL032-G-205 | \`Source/editor_webview\` directory exists and contains >=1 source file. |

## Exit Semantics
- Exit 0: all guard IDs pass.
- Exit 1: one or more guard IDs fail.
- Exit 2: usage/configuration error.

## Artifact Schema
- \`status.tsv\`: lane summary and pass/fail rollup.
- \`guardrail_report.tsv\`: per-guard machine-readable findings.
- \`blocker_taxonomy.tsv\`: grouped blocker counts by category.
EOF
}

main() {
  require_command rg
  require_command wc
  require_command find
  require_command sed
  require_command paste

  check_line_threshold "BL032-G-001" "Source/PluginProcessor.cpp" "$MAX_PLUGINPROCESSOR_LINES"
  check_line_threshold "BL032-G-002" "Source/PluginEditor.cpp" "$MAX_PLUGINEDITOR_LINES"

  check_forbidden_include_in_paths \
    "BL032-G-101" \
    "files" \
    '#include\\s+"PluginEditor\\.h"' \
    'forbidden include must not appear' \
    'Source/PluginProcessor.cpp|Source/PluginProcessor.h'

  check_forbidden_include_in_paths \
    "BL032-G-102" \
    "dir" \
    '#include\\s+"(PluginProcessor|PluginEditor)\\.h"' \
    'matches=0' \
    'Source/shared_contracts'

  check_forbidden_include_in_paths \
    "BL032-G-103" \
    "dir" \
    '#include\\s+"PluginEditor\\.h"' \
    'matches=0' \
    'Source/processor_core'

  check_forbidden_include_in_paths \
    "BL032-G-104" \
    "dir" \
    '#include\\s+"PluginEditor\\.h"' \
    'matches=0' \
    'Source/processor_bridge'

  check_forbidden_include_in_paths \
    "BL032-G-105" \
    "dir" \
    '#include\\s+"PluginProcessor\\.h"' \
    'matches=0' \
    'Source/editor_webview'

  check_forbidden_include_in_paths \
    "BL032-G-106" \
    "dir" \
    '#include\\s+"SpatialRenderer\\.h"' \
    'matches=0' \
    'Source/editor_shell'

  check_required_module_dir "BL032-G-201" "Source/shared_contracts"
  check_required_module_dir "BL032-G-202" "Source/processor_core"
  check_required_module_dir "BL032-G-203" "Source/processor_bridge"
  check_required_module_dir "BL032-G-204" "Source/editor_shell"
  check_required_module_dir "BL032-G-205" "Source/editor_webview"

  write_blocker_taxonomy
  write_guardrail_contract

  local line_count_failures="$LINE_FAIL"
  local forbidden_edge_failures="$EDGE_FAIL"
  local required_dir_failures="$DIR_FAIL"

  if [[ "$line_count_failures" -eq 0 ]]; then
    record_status "line_count_thresholds" "PASS" "violations=0" "$GUARDRAIL_REPORT_TSV"
  else
    record_status "line_count_thresholds" "FAIL" "violations=${line_count_failures}" "$GUARDRAIL_REPORT_TSV"
  fi

  if [[ "$forbidden_edge_failures" -eq 0 ]]; then
    record_status "forbidden_dependency_edges" "PASS" "violations=0" "$GUARDRAIL_REPORT_TSV"
  else
    record_status "forbidden_dependency_edges" "FAIL" "violations=${forbidden_edge_failures}" "$GUARDRAIL_REPORT_TSV"
  fi

  if [[ "$required_dir_failures" -eq 0 ]]; then
    record_status "required_module_directories" "PASS" "violations=0" "$GUARDRAIL_REPORT_TSV"
  else
    record_status "required_module_directories" "FAIL" "violations=${required_dir_failures}" "$GUARDRAIL_REPORT_TSV"
  fi

  record_status "guardrail_artifacts" "PASS" "report + taxonomy + contract emitted" "$OUT_DIR"

  if [[ "$FAILED_GUARDS" -eq 0 ]]; then
    record_status "guardrail_lane" "PASS" "failed_guards=0 total_guards=${TOTAL_GUARDS}" "$STATUS_TSV"
    echo "BL-032 guardrails PASS (failed_guards=0 total_guards=${TOTAL_GUARDS})"
    echo "artifact_dir=$OUT_DIR"
    exit 0
  fi

  record_status "guardrail_lane" "FAIL" "failed_guards=${FAILED_GUARDS} total_guards=${TOTAL_GUARDS}" "$STATUS_TSV"
  echo "BL-032 guardrails FAIL (failed_guards=${FAILED_GUARDS} total_guards=${TOTAL_GUARDS})"
  echo "artifact_dir=$OUT_DIR"
  exit 1
}

main "$@"
