#!/usr/bin/env bash
# Title: BL-076 SpatialRenderer Structure Guardrails
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-03
# Last Modified Date: 2026-03-03
#
# Purpose:
# - Enforce deterministic structure and dependency boundaries for SpatialRenderer decomposition.
# - Keep RT audit and smoke-lane contract availability visible while extraction waves continue.
# - Emit bridge payload parity evidence for scene-state contract stability.
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl076_spatial_renderer_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=""
RUNS_SET=0

MAX_SPATIAL_RENDERER_H_LINES="${BL076_MAX_SPATIAL_RENDERER_H_LINES:-5000}"
MAX_MODULE_HEADER_LINES="${BL076_MAX_MODULE_HEADER_LINES:-250}"
MAX_MODULE_CPP_LINES="${BL076_MAX_MODULE_CPP_LINES:-700}"
MIN_EXTRACTED_MODULE_FILES="${BL076_MIN_EXTRACTED_MODULE_FILES:-2}"

STATUS_TSV=""
STRUCTURE_GUARDRAILS_TSV=""
DEPENDENCY_MATRIX_TSV=""
RT_AUDIT_TSV=""
SMOKE_PARITY_TSV=""
BRIDGE_PARITY_TSV=""

pass_count=0
fail_count=0
base_smoke_signature_hash=""
base_module_inventory_hash=""
base_bridge_signature_hash=""

SPATIAL_RENDERER_H="${ROOT_DIR}/Source/SpatialRenderer.h"
SPATIAL_MODULE_DIR="${ROOT_DIR}/Source/spatial_renderer"
SPATIAL_TYPES_H="${SPATIAL_MODULE_DIR}/SpatialRendererTypes.h"
SPATIAL_ROUTER_H="${SPATIAL_MODULE_DIR}/SpatialProfileRouter.h"
BRIDGE_OPS_H="${ROOT_DIR}/Source/processor_bridge/ProcessorSceneStateBridgeOps.h"
UI_INDEX_JS="${ROOT_DIR}/Source/ui/public/js/index.js"
RT_AUDIT_SCRIPT="${ROOT_DIR}/scripts/rt-safety-audit.sh"

REQUIRED_SMOKE_LANES=(
  "scripts/qa-bl009-headphone-contract-mac.sh"
  "scripts/qa-bl009-headphone-profile-contract-mac.sh"
  "scripts/qa-bl018-ambisonic-contract-mac.sh"
  "scripts/qa-bl018-profile-matrix-strict-mac.sh"
  "scripts/qa-bl052-steam-audio-virtual-surround-mac.sh"
  "scripts/qa-bl053-head-tracking-orientation-injection-mac.sh"
  "scripts/qa-bl069-rt-safe-preset-pipeline-mac.sh"
)

usage() {
  cat <<'USAGE'
Usage: qa-bl076-spatial-renderer-structure-guardrails-mac.sh [options]

BL-076 structure/dependency guardrail lane for SpatialRenderer decomposition.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute checks (same checks with strict zero-failure gate)
  --runs <N>         Number of deterministic replay runs
  --help, -h         Show usage

Outputs:
  status.tsv
  spatial_renderer_structure_guardrails.tsv
  spatial_renderer_module_dependency_matrix.tsv
  rt_audit.tsv
  smoke_parity_matrix.tsv
  bridge_payload_parity.tsv
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

sanitize_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  value="${value//$'\r'/ }"
  printf "%s" "$value"
}

hash_text() {
  local text="$1"
  if command -v shasum >/dev/null 2>&1; then
    printf "%s" "$text" | shasum -a 256 | awk '{ print $1 }'
    return
  fi
  if command -v openssl >/dev/null 2>&1; then
    printf "%s" "$text" | openssl dgst -sha256 | awk '{ print $2 }'
    return
  fi
  usage_error "sha256 tool unavailable (need shasum or openssl)"
}

record_status() {
  local check_id="$1"
  local result="$2"
  local detail="$3"
  local artifact="${4:-}"

  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$check_id")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$STATUS_TSV"

  if [[ "$result" == "PASS" ]]; then
    ((pass_count++)) || true
  else
    ((fail_count++)) || true
  fi
}

append_structure_row() {
  local run_index="$1"
  local guard_id="$2"
  local category="$3"
  local result="$4"
  local observed="$5"
  local threshold="$6"
  local detail="$7"
  local artifact="$8"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$run_index")" \
    "$(sanitize_field "$guard_id")" \
    "$(sanitize_field "$category")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$observed")" \
    "$(sanitize_field "$threshold")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$STRUCTURE_GUARDRAILS_TSV"
}

append_dependency_row() {
  local run_index="$1"
  local rule_id="$2"
  local scope="$3"
  local result="$4"
  local matches="$5"
  local detail="$6"
  local artifact="$7"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$run_index")" \
    "$(sanitize_field "$rule_id")" \
    "$(sanitize_field "$scope")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$matches")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$DEPENDENCY_MATRIX_TSV"
}

append_rt_row() {
  local run_index="$1"
  local rt_exit="$2"
  local total_hits="$3"
  local allowlisted_hits="$4"
  local non_allowlisted="$5"
  local result="$6"
  local detail="$7"
  local artifact="$8"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$run_index")" \
    "$(sanitize_field "$rt_exit")" \
    "$(sanitize_field "$total_hits")" \
    "$(sanitize_field "$allowlisted_hits")" \
    "$(sanitize_field "$non_allowlisted")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$RT_AUDIT_TSV"
}

append_smoke_row() {
  local run_index="$1"
  local check_id="$2"
  local result="$3"
  local detail="$4"
  local artifact="$5"

  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$run_index")" \
    "$(sanitize_field "$check_id")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$SMOKE_PARITY_TSV"
}

append_bridge_row() {
  local run_index="$1"
  local check_id="$2"
  local result="$3"
  local detail="$4"
  local artifact="$5"

  printf "%s\t%s\t%s\t%s\t%s\n" \
    "$(sanitize_field "$run_index")" \
    "$(sanitize_field "$check_id")" \
    "$(sanitize_field "$result")" \
    "$(sanitize_field "$detail")" \
    "$(sanitize_field "$artifact")" \
    >> "$BRIDGE_PARITY_TSV"
}

line_count() {
  local file_path="$1"
  if [[ ! -f "$file_path" ]]; then
    echo ""
    return
  fi
  wc -l < "$file_path" | tr -d '[:space:]'
}

count_token_rows_in_tsv() {
  local file="$1"
  local token_a="$2"
  local token_b="$3"
  [[ -f "$file" ]] || {
    echo 0
    return
  }

  awk -F'\t' -v a="$token_a" -v b="$token_b" '
    NR == 1 { next }
    {
      for (i = 1; i <= NF; ++i)
      {
        value = toupper($i)
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
        if (value == a || value == b)
        {
          count++
          break
        }
      }
    }
    END { print count + 0 }
  ' "$file"
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
      RUNS="$2"
      RUNS_SET=1
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

if (( RUNS_SET == 0 )); then
  if [[ "$MODE" == "contract_only" ]]; then
    RUNS=3
  else
    RUNS=1
  fi
fi

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  usage_error "--runs must be an integer >= 1"
fi

for numeric_var in \
  "$MAX_SPATIAL_RENDERER_H_LINES" \
  "$MAX_MODULE_HEADER_LINES" \
  "$MAX_MODULE_CPP_LINES" \
  "$MIN_EXTRACTED_MODULE_FILES"; do
  if ! [[ "$numeric_var" =~ ^[0-9]+$ ]]; then
    usage_error "line/file thresholds must be integer values"
  fi
done

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
STRUCTURE_GUARDRAILS_TSV="${OUT_DIR}/spatial_renderer_structure_guardrails.tsv"
DEPENDENCY_MATRIX_TSV="${OUT_DIR}/spatial_renderer_module_dependency_matrix.tsv"
RT_AUDIT_TSV="${OUT_DIR}/rt_audit.tsv"
SMOKE_PARITY_TSV="${OUT_DIR}/smoke_parity_matrix.tsv"
BRIDGE_PARITY_TSV="${OUT_DIR}/bridge_payload_parity.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_index\tguard_id\tcategory\tresult\tobserved\tthreshold\tdetail\tartifact\n" > "$STRUCTURE_GUARDRAILS_TSV"
printf "run_index\trule_id\tscope\tresult\tmatches\tdetail\tartifact\n" > "$DEPENDENCY_MATRIX_TSV"
printf "run_index\trt_exit\ttotal_hits\tallowlisted_hits\tnon_allowlisted\tresult\tdetail\tartifact\n" > "$RT_AUDIT_TSV"
printf "run_index\tcheck_id\tresult\tdetail\tartifact\n" > "$SMOKE_PARITY_TSV"
printf "run_index\tcheck_id\tresult\tdetail\tartifact\n" > "$BRIDGE_PARITY_TSV"

for required_file in \
  "$SPATIAL_RENDERER_H" \
  "$SPATIAL_TYPES_H" \
  "$SPATIAL_ROUTER_H" \
  "$BRIDGE_OPS_H" \
  "$UI_INDEX_JS" \
  "$RT_AUDIT_SCRIPT"; do
  if [[ -f "$required_file" ]]; then
    record_status "BL076-PRE-$(basename "$required_file")" "PASS" "required file present" "$required_file"
  else
    record_status "BL076-PRE-$(basename "$required_file")" "FAIL" "required file missing" "$required_file"
  fi
done

for run_index in $(seq 1 "$RUNS"); do
  run_dir="${OUT_DIR}/run_${run_index}"
  mkdir -p "$run_dir"

  # Structure guardrails.
  spatial_lines="$(line_count "$SPATIAL_RENDERER_H")"
  if [[ -n "$spatial_lines" && "$spatial_lines" -le "$MAX_SPATIAL_RENDERER_H_LINES" ]]; then
    append_structure_row "$run_index" "BL076-SG-001" "line_count_threshold" "PASS" "$spatial_lines" "<=${MAX_SPATIAL_RENDERER_H_LINES}" "SpatialRenderer.h is within current decomposition cap" "$SPATIAL_RENDERER_H"
    record_status "BL076-R${run_index}-SG-001" "PASS" "SpatialRenderer.h lines=${spatial_lines} <= ${MAX_SPATIAL_RENDERER_H_LINES}" "$STRUCTURE_GUARDRAILS_TSV"
  else
    append_structure_row "$run_index" "BL076-SG-001" "line_count_threshold" "FAIL" "${spatial_lines:-missing}" "<=${MAX_SPATIAL_RENDERER_H_LINES}" "SpatialRenderer.h exceeds cap or is missing" "$SPATIAL_RENDERER_H"
    record_status "BL076-R${run_index}-SG-001" "FAIL" "SpatialRenderer.h lines=${spatial_lines:-missing} > ${MAX_SPATIAL_RENDERER_H_LINES}" "$STRUCTURE_GUARDRAILS_TSV"
  fi

  module_file_count="$(find "$SPATIAL_MODULE_DIR" -maxdepth 1 -type f \( -name '*.h' -o -name '*.cpp' \) | wc -l | tr -d '[:space:]')"
  if [[ "$module_file_count" -ge "$MIN_EXTRACTED_MODULE_FILES" ]]; then
    append_structure_row "$run_index" "BL076-SG-002" "module_file_count" "PASS" "$module_file_count" ">=$MIN_EXTRACTED_MODULE_FILES" "spatial_renderer module directory has extracted units" "$SPATIAL_MODULE_DIR"
    record_status "BL076-R${run_index}-SG-002" "PASS" "module files=${module_file_count}" "$STRUCTURE_GUARDRAILS_TSV"
  else
    append_structure_row "$run_index" "BL076-SG-002" "module_file_count" "FAIL" "$module_file_count" ">=$MIN_EXTRACTED_MODULE_FILES" "module extraction floor not met" "$SPATIAL_MODULE_DIR"
    record_status "BL076-R${run_index}-SG-002" "FAIL" "module files=${module_file_count}" "$STRUCTURE_GUARDRAILS_TSV"
  fi

  if rg -q --fixed-strings '#include "spatial_renderer/SpatialRendererTypes.h"' "$SPATIAL_RENDERER_H"; then
    append_structure_row "$run_index" "BL076-SG-003" "required_include" "PASS" "present" "SpatialRendererTypes include required" "SpatialRenderer.h includes extracted types module" "$SPATIAL_RENDERER_H"
    record_status "BL076-R${run_index}-SG-003" "PASS" "SpatialRenderer.h includes SpatialRendererTypes.h" "$STRUCTURE_GUARDRAILS_TSV"
  else
    append_structure_row "$run_index" "BL076-SG-003" "required_include" "FAIL" "missing" "SpatialRendererTypes include required" "SpatialRenderer.h missing extracted types include" "$SPATIAL_RENDERER_H"
    record_status "BL076-R${run_index}-SG-003" "FAIL" "SpatialRenderer.h missing SpatialRendererTypes.h include" "$STRUCTURE_GUARDRAILS_TSV"
  fi

  if rg -q --fixed-strings '#include "spatial_renderer/SpatialProfileRouter.h"' "$SPATIAL_RENDERER_H"; then
    append_structure_row "$run_index" "BL076-SG-004" "required_include" "PASS" "present" "SpatialProfileRouter include required" "SpatialRenderer.h includes extracted routing module" "$SPATIAL_RENDERER_H"
    record_status "BL076-R${run_index}-SG-004" "PASS" "SpatialRenderer.h includes SpatialProfileRouter.h" "$STRUCTURE_GUARDRAILS_TSV"
  else
    append_structure_row "$run_index" "BL076-SG-004" "required_include" "FAIL" "missing" "SpatialProfileRouter include required" "SpatialRenderer.h missing extracted routing include" "$SPATIAL_RENDERER_H"
    record_status "BL076-R${run_index}-SG-004" "FAIL" "SpatialRenderer.h missing SpatialProfileRouter.h include" "$STRUCTURE_GUARDRAILS_TSV"
  fi

  for header in "$SPATIAL_MODULE_DIR"/*.h; do
    [[ -f "$header" ]] || continue
    header_name="$(basename "$header")"
    header_lines="$(line_count "$header")"
    guard_id="BL076-SG-H-${header_name}"
    if [[ -n "$header_lines" && "$header_lines" -le "$MAX_MODULE_HEADER_LINES" ]]; then
      append_structure_row "$run_index" "$guard_id" "module_header_line_cap" "PASS" "$header_lines" "<=${MAX_MODULE_HEADER_LINES}" "module header line cap satisfied" "$header"
      record_status "BL076-R${run_index}-${guard_id}" "PASS" "${header_name} lines=${header_lines} <= ${MAX_MODULE_HEADER_LINES}" "$STRUCTURE_GUARDRAILS_TSV"
    else
      append_structure_row "$run_index" "$guard_id" "module_header_line_cap" "FAIL" "${header_lines:-missing}" "<=${MAX_MODULE_HEADER_LINES}" "module header line cap exceeded or file missing" "$header"
      record_status "BL076-R${run_index}-${guard_id}" "FAIL" "${header_name} lines=${header_lines:-missing} > ${MAX_MODULE_HEADER_LINES}" "$STRUCTURE_GUARDRAILS_TSV"
    fi
  done

  for cpp_file in "$SPATIAL_MODULE_DIR"/*.cpp; do
    [[ -f "$cpp_file" ]] || continue
    cpp_name="$(basename "$cpp_file")"
    cpp_lines="$(line_count "$cpp_file")"
    guard_id="BL076-SG-C-${cpp_name}"
    if [[ -n "$cpp_lines" && "$cpp_lines" -le "$MAX_MODULE_CPP_LINES" ]]; then
      append_structure_row "$run_index" "$guard_id" "module_cpp_line_cap" "PASS" "$cpp_lines" "<=${MAX_MODULE_CPP_LINES}" "module implementation line cap satisfied" "$cpp_file"
      record_status "BL076-R${run_index}-${guard_id}" "PASS" "${cpp_name} lines=${cpp_lines} <= ${MAX_MODULE_CPP_LINES}" "$STRUCTURE_GUARDRAILS_TSV"
    else
      append_structure_row "$run_index" "$guard_id" "module_cpp_line_cap" "FAIL" "${cpp_lines:-missing}" "<=${MAX_MODULE_CPP_LINES}" "module implementation line cap exceeded or file missing" "$cpp_file"
      record_status "BL076-R${run_index}-${guard_id}" "FAIL" "${cpp_name} lines=${cpp_lines:-missing} > ${MAX_MODULE_CPP_LINES}" "$STRUCTURE_GUARDRAILS_TSV"
    fi
  done

  # Dependency matrix.
  types_scenegraph_hits="$(rg -n --no-heading --pcre2 '#include\\s+\"SceneGraph\\.h\"' "$SPATIAL_TYPES_H" || true)"
  if [[ -z "$types_scenegraph_hits" ]]; then
    append_dependency_row "$run_index" "BL076-DM-001" "SpatialRendererTypes.h" "PASS" "0" "types module does not depend on SceneGraph" "$SPATIAL_TYPES_H"
    record_status "BL076-R${run_index}-DM-001" "PASS" "SpatialRendererTypes.h does not include SceneGraph.h" "$DEPENDENCY_MATRIX_TSV"
  else
    append_dependency_row "$run_index" "BL076-DM-001" "SpatialRendererTypes.h" "FAIL" ">=1" "types module unexpectedly depends on SceneGraph" "$SPATIAL_TYPES_H"
    record_status "BL076-R${run_index}-DM-001" "FAIL" "SpatialRendererTypes.h includes SceneGraph.h" "$DEPENDENCY_MATRIX_TSV"
  fi

  types_steam_hits="$(rg -n --no-heading --pcre2 'phonon\\.h' "$SPATIAL_TYPES_H" || true)"
  if [[ -z "$types_steam_hits" ]]; then
    append_dependency_row "$run_index" "BL076-DM-002" "SpatialRendererTypes.h" "PASS" "0" "types module does not include Steam runtime headers" "$SPATIAL_TYPES_H"
    record_status "BL076-R${run_index}-DM-002" "PASS" "SpatialRendererTypes.h does not include phonon.h" "$DEPENDENCY_MATRIX_TSV"
  else
    append_dependency_row "$run_index" "BL076-DM-002" "SpatialRendererTypes.h" "FAIL" ">=1" "types module includes Steam runtime headers" "$SPATIAL_TYPES_H"
    record_status "BL076-R${run_index}-DM-002" "FAIL" "SpatialRendererTypes.h includes phonon.h" "$DEPENDENCY_MATRIX_TSV"
  fi

  types_headphone_hits="$(rg -n --no-heading --pcre2 'headphone_dsp/' "$SPATIAL_TYPES_H" || true)"
  if [[ -z "$types_headphone_hits" ]]; then
    append_dependency_row "$run_index" "BL076-DM-003" "SpatialRendererTypes.h" "PASS" "0" "types module avoids headphone DSP implementation dependencies" "$SPATIAL_TYPES_H"
    record_status "BL076-R${run_index}-DM-003" "PASS" "SpatialRendererTypes.h avoids headphone_dsp includes" "$DEPENDENCY_MATRIX_TSV"
  else
    append_dependency_row "$run_index" "BL076-DM-003" "SpatialRendererTypes.h" "FAIL" ">=1" "types module depends on headphone DSP implementation" "$SPATIAL_TYPES_H"
    record_status "BL076-R${run_index}-DM-003" "FAIL" "SpatialRendererTypes.h includes headphone_dsp paths" "$DEPENDENCY_MATRIX_TSV"
  fi

  router_steam_hits="$(rg -n --no-heading --pcre2 'phonon\\.h' "$SPATIAL_ROUTER_H" || true)"
  if [[ -z "$router_steam_hits" ]]; then
    append_dependency_row "$run_index" "BL076-DM-004" "SpatialProfileRouter.h" "PASS" "0" "router module avoids Steam runtime headers" "$SPATIAL_ROUTER_H"
    record_status "BL076-R${run_index}-DM-004" "PASS" "SpatialProfileRouter.h avoids phonon.h" "$DEPENDENCY_MATRIX_TSV"
  else
    append_dependency_row "$run_index" "BL076-DM-004" "SpatialProfileRouter.h" "FAIL" ">=1" "router module includes Steam runtime headers" "$SPATIAL_ROUTER_H"
    record_status "BL076-R${run_index}-DM-004" "FAIL" "SpatialProfileRouter.h includes phonon.h" "$DEPENDENCY_MATRIX_TSV"
  fi

  router_headphone_hits="$(rg -n --no-heading --pcre2 'headphone_dsp/' "$SPATIAL_ROUTER_H" || true)"
  if [[ -z "$router_headphone_hits" ]]; then
    append_dependency_row "$run_index" "BL076-DM-005" "SpatialProfileRouter.h" "PASS" "0" "router module avoids headphone DSP implementation dependencies" "$SPATIAL_ROUTER_H"
    record_status "BL076-R${run_index}-DM-005" "PASS" "SpatialProfileRouter.h avoids headphone_dsp includes" "$DEPENDENCY_MATRIX_TSV"
  else
    append_dependency_row "$run_index" "BL076-DM-005" "SpatialProfileRouter.h" "FAIL" ">=1" "router module includes headphone DSP implementation dependencies" "$SPATIAL_ROUTER_H"
    record_status "BL076-R${run_index}-DM-005" "FAIL" "SpatialProfileRouter.h includes headphone_dsp paths" "$DEPENDENCY_MATRIX_TSV"
  fi

  router_types_include_hits="$(rg -n --no-heading --fixed-strings '#include "SpatialRendererTypes.h"' "$SPATIAL_ROUTER_H" || true)"
  if [[ -n "$router_types_include_hits" ]]; then
    append_dependency_row "$run_index" "BL076-DM-006" "SpatialProfileRouter.h" "PASS" ">=1" "router module includes extracted type contracts" "$SPATIAL_ROUTER_H"
    record_status "BL076-R${run_index}-DM-006" "PASS" "SpatialProfileRouter.h includes SpatialRendererTypes.h" "$DEPENDENCY_MATRIX_TSV"
  else
    append_dependency_row "$run_index" "BL076-DM-006" "SpatialProfileRouter.h" "FAIL" "0" "router module missing extracted type contract include" "$SPATIAL_ROUTER_H"
    record_status "BL076-R${run_index}-DM-006" "FAIL" "SpatialProfileRouter.h missing SpatialRendererTypes.h include" "$DEPENDENCY_MATRIX_TSV"
  fi

  module_scenegraph_hits="$(rg -n --no-heading --pcre2 '#include\\s+\"SceneGraph\\.h\"' "$SPATIAL_MODULE_DIR" -g '*.h' -g '*.cpp' || true)"
  if [[ -z "$module_scenegraph_hits" ]]; then
    append_dependency_row "$run_index" "BL076-DM-007" "Source/spatial_renderer/*" "PASS" "0" "extracted module files avoid SceneGraph include edge" "$SPATIAL_MODULE_DIR"
    record_status "BL076-R${run_index}-DM-007" "PASS" "Source/spatial_renderer/* avoids SceneGraph include edge" "$DEPENDENCY_MATRIX_TSV"
  else
    append_dependency_row "$run_index" "BL076-DM-007" "Source/spatial_renderer/*" "FAIL" ">=1" "extracted module files include SceneGraph.h" "$SPATIAL_MODULE_DIR"
    record_status "BL076-R${run_index}-DM-007" "FAIL" "Source/spatial_renderer/* contains SceneGraph include edge" "$DEPENDENCY_MATRIX_TSV"
  fi

  # RT audit snapshot.
  rt_raw="${run_dir}/rt_audit_raw.tsv"
  rt_stdout="${run_dir}/rt_audit_stdout.log"
  rt_stderr="${run_dir}/rt_audit_stderr.log"
  set +e
  "$RT_AUDIT_SCRIPT" --print-summary --output "$rt_raw" > "$rt_stdout" 2> "$rt_stderr"
  rt_exit=$?
  set -e

  total_hits=0
  allowlisted_hits=0
  non_allowlisted=0
  if [[ -f "$rt_raw" ]]; then
    total_hits="$(awk -F'\t' 'NR > 1 { count++ } END { print count + 0 }' "$rt_raw")"
    allowlisted_hits="$(awk -F'\t' 'NR > 1 && tolower($5) == "true" { count++ } END { print count + 0 }' "$rt_raw")"
    non_allowlisted="$(awk -F'\t' 'NR > 1 && tolower($5) != "true" { count++ } END { print count + 0 }' "$rt_raw")"
  fi

  rt_result="PASS"
  rt_detail="rt_exit=${rt_exit};non_allowlisted=${non_allowlisted};total_hits=${total_hits}"
  if [[ "$rt_exit" -ne 0 || "$non_allowlisted" -ne 0 ]]; then
    rt_result="FAIL"
    rt_detail="rt audit failed: rt_exit=${rt_exit};non_allowlisted=${non_allowlisted};total_hits=${total_hits}"
  fi
  append_rt_row "$run_index" "$rt_exit" "$total_hits" "$allowlisted_hits" "$non_allowlisted" "$rt_result" "$rt_detail" "$rt_raw"
  record_status "BL076-R${run_index}-RT-001" "$rt_result" "$rt_detail" "$RT_AUDIT_TSV"

  # Smoke parity matrix.
  for lane_rel in "${REQUIRED_SMOKE_LANES[@]}"; do
    lane_abs="${ROOT_DIR}/${lane_rel}"
    lane_id="$(basename "$lane_rel")"
    if [[ -x "$lane_abs" ]]; then
      append_smoke_row "$run_index" "BL076-SM-${lane_id}" "PASS" "lane script exists and is executable" "$lane_abs"
      record_status "BL076-R${run_index}-SM-${lane_id}" "PASS" "lane script exists and is executable" "$SMOKE_PARITY_TSV"
    else
      append_smoke_row "$run_index" "BL076-SM-${lane_id}" "FAIL" "lane script missing or not executable" "$lane_abs"
      record_status "BL076-R${run_index}-SM-${lane_id}" "FAIL" "lane script missing or not executable" "$SMOKE_PARITY_TSV"
    fi
  done

  smoke_signature_text="$(rg -n --no-heading --pcre2 \
    'renderVirtualSurroundForMonitoring|void\s+process\s*\(|resolveSpatialProfileForHost\s*\(|writeSurround521Sample|writeSurround721Sample|writeSurround742Sample|initialiseSteamAudioRuntimeIfEnabled|teardownSteamAudioRuntime' \
    "$SPATIAL_RENDERER_H" || true)"
  if [[ -z "$smoke_signature_text" ]]; then
    append_smoke_row "$run_index" "BL076-SM-signature-hash" "FAIL" "signature probe produced empty result" "$SPATIAL_RENDERER_H"
    record_status "BL076-R${run_index}-SM-signature-hash" "FAIL" "smoke signature probe produced empty result" "$SMOKE_PARITY_TSV"
  else
    smoke_signature_hash="$(hash_text "$smoke_signature_text")"
    if [[ "$run_index" -eq 1 ]]; then
      base_smoke_signature_hash="$smoke_signature_hash"
      append_smoke_row "$run_index" "BL076-SM-signature-hash" "PASS" "baseline signature hash=${smoke_signature_hash}" "$SPATIAL_RENDERER_H"
      record_status "BL076-R${run_index}-SM-signature-hash" "PASS" "baseline smoke signature hash=${smoke_signature_hash}" "$SMOKE_PARITY_TSV"
    elif [[ "$smoke_signature_hash" == "$base_smoke_signature_hash" ]]; then
      append_smoke_row "$run_index" "BL076-SM-signature-hash" "PASS" "signature stable vs baseline ${base_smoke_signature_hash}" "$SPATIAL_RENDERER_H"
      record_status "BL076-R${run_index}-SM-signature-hash" "PASS" "smoke signature stable vs baseline" "$SMOKE_PARITY_TSV"
    else
      append_smoke_row "$run_index" "BL076-SM-signature-hash" "FAIL" "signature drift baseline=${base_smoke_signature_hash} current=${smoke_signature_hash}" "$SPATIAL_RENDERER_H"
      record_status "BL076-R${run_index}-SM-signature-hash" "FAIL" "smoke signature drift detected" "$SMOKE_PARITY_TSV"
    fi
  fi

  module_inventory_text="$(
    find "$SPATIAL_MODULE_DIR" -maxdepth 1 -type f \( -name '*.h' -o -name '*.cpp' \) \
      | LC_ALL=C sort \
      | while IFS= read -r module_file; do
          module_lines="$(line_count "$module_file")"
          printf "%s:%s\n" "${module_file#${ROOT_DIR}/}" "${module_lines}"
        done
  )"
  module_inventory_hash="$(hash_text "$module_inventory_text")"
  if [[ "$run_index" -eq 1 ]]; then
    base_module_inventory_hash="$module_inventory_hash"
    append_smoke_row "$run_index" "BL076-SM-module-inventory" "PASS" "baseline module inventory hash=${module_inventory_hash}" "$SPATIAL_MODULE_DIR"
    record_status "BL076-R${run_index}-SM-module-inventory" "PASS" "baseline module inventory hash=${module_inventory_hash}" "$SMOKE_PARITY_TSV"
  elif [[ "$module_inventory_hash" == "$base_module_inventory_hash" ]]; then
    append_smoke_row "$run_index" "BL076-SM-module-inventory" "PASS" "module inventory stable vs baseline ${base_module_inventory_hash}" "$SPATIAL_MODULE_DIR"
    record_status "BL076-R${run_index}-SM-module-inventory" "PASS" "module inventory stable vs baseline" "$SMOKE_PARITY_TSV"
  else
    append_smoke_row "$run_index" "BL076-SM-module-inventory" "FAIL" "module inventory drift baseline=${base_module_inventory_hash} current=${module_inventory_hash}" "$SPATIAL_MODULE_DIR"
    record_status "BL076-R${run_index}-SM-module-inventory" "FAIL" "module inventory drift detected" "$SMOKE_PARITY_TSV"
  fi

  # Bridge payload parity.
  if rg -q --fixed-strings 'snapshotSchema' "$BRIDGE_OPS_H"; then
    append_bridge_row "$run_index" "BL076-BP-001" "PASS" "bridge payload emits snapshotSchema" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-001" "PASS" "bridge payload emits snapshotSchema" "$BRIDGE_PARITY_TSV"
  else
    append_bridge_row "$run_index" "BL076-BP-001" "FAIL" "bridge payload missing snapshotSchema" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-001" "FAIL" "bridge payload missing snapshotSchema" "$BRIDGE_PARITY_TSV"
  fi

  if rg -q --fixed-strings 'nativeBridgeDiagnosticsSchema' "$BRIDGE_OPS_H"; then
    append_bridge_row "$run_index" "BL076-BP-002" "PASS" "bridge payload emits nativeBridgeDiagnosticsSchema" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-002" "PASS" "bridge payload emits nativeBridgeDiagnosticsSchema" "$BRIDGE_PARITY_TSV"
  else
    append_bridge_row "$run_index" "BL076-BP-002" "FAIL" "bridge payload missing nativeBridgeDiagnosticsSchema" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-002" "FAIL" "bridge payload missing nativeBridgeDiagnosticsSchema" "$BRIDGE_PARITY_TSV"
  fi

  if rg -q --fixed-strings 'nativeBridgeDiagnostics' "$BRIDGE_OPS_H"; then
    append_bridge_row "$run_index" "BL076-BP-003" "PASS" "bridge payload includes nativeBridgeDiagnostics object" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-003" "PASS" "bridge payload includes nativeBridgeDiagnostics object" "$BRIDGE_PARITY_TSV"
  else
    append_bridge_row "$run_index" "BL076-BP-003" "FAIL" "bridge payload missing nativeBridgeDiagnostics object" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-003" "FAIL" "bridge payload missing nativeBridgeDiagnostics object" "$BRIDGE_PARITY_TSV"
  fi

  if rg -q --fixed-strings 'Object.prototype.hasOwnProperty.call(data, "nativeBridgeDiagnosticsSchema")' "$UI_INDEX_JS"; then
    append_bridge_row "$run_index" "BL076-BP-004" "PASS" "UI payload parser checks native bridge diagnostics schema key" "$UI_INDEX_JS"
    record_status "BL076-R${run_index}-BP-004" "PASS" "UI parser checks native bridge diagnostics schema key" "$BRIDGE_PARITY_TSV"
  else
    append_bridge_row "$run_index" "BL076-BP-004" "FAIL" "UI payload parser missing native bridge schema key check" "$UI_INDEX_JS"
    record_status "BL076-R${run_index}-BP-004" "FAIL" "UI parser missing native bridge schema key check" "$BRIDGE_PARITY_TSV"
  fi

  if rg -q --fixed-strings 'if (typeof data?.snapshotSchema === "string" && data.snapshotSchema.trim()) {' "$UI_INDEX_JS"; then
    append_bridge_row "$run_index" "BL076-BP-005" "PASS" "UI parser ingests snapshotSchema contract key" "$UI_INDEX_JS"
    record_status "BL076-R${run_index}-BP-005" "PASS" "UI parser ingests snapshotSchema contract key" "$BRIDGE_PARITY_TSV"
  else
    append_bridge_row "$run_index" "BL076-BP-005" "FAIL" "UI parser missing snapshotSchema ingestion" "$UI_INDEX_JS"
    record_status "BL076-R${run_index}-BP-005" "FAIL" "UI parser missing snapshotSchema ingestion" "$BRIDGE_PARITY_TSV"
  fi

  bridge_signature_text="$(
    {
      rg -n --no-heading --pcre2 'snapshotSchema|snapshotSeq|profileSyncSeq|nativeBridgeDiagnosticsSchema|nativeBridgeAvailable|nativeBridgeBackend|nativeBridgeDiagnostics' "$BRIDGE_OPS_H" || true
      rg -n --no-heading --pcre2 'snapshotSchema|snapshotSeq|profileSyncSeq|nativeBridgeDiagnosticsSchema|nativeBridgeAvailable|nativeBridgeBackend|nativeBridgeDiagnostics' "$UI_INDEX_JS" || true
    } | LC_ALL=C sort
  )"
  bridge_signature_hash="$(hash_text "$bridge_signature_text")"
  if [[ "$run_index" -eq 1 ]]; then
    base_bridge_signature_hash="$bridge_signature_hash"
    append_bridge_row "$run_index" "BL076-BP-signature-hash" "PASS" "baseline bridge signature hash=${bridge_signature_hash}" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-signature-hash" "PASS" "baseline bridge signature hash=${bridge_signature_hash}" "$BRIDGE_PARITY_TSV"
  elif [[ "$bridge_signature_hash" == "$base_bridge_signature_hash" ]]; then
    append_bridge_row "$run_index" "BL076-BP-signature-hash" "PASS" "bridge signature stable vs baseline ${base_bridge_signature_hash}" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-signature-hash" "PASS" "bridge signature stable vs baseline" "$BRIDGE_PARITY_TSV"
  else
    append_bridge_row "$run_index" "BL076-BP-signature-hash" "FAIL" "bridge signature drift baseline=${base_bridge_signature_hash} current=${bridge_signature_hash}" "$BRIDGE_OPS_H"
    record_status "BL076-R${run_index}-BP-signature-hash" "FAIL" "bridge signature drift detected" "$BRIDGE_PARITY_TSV"
  fi
done

execute_scaffold_rows=0
if [[ "$MODE" == "execute" ]]; then
  for table in \
    "$STRUCTURE_GUARDRAILS_TSV" \
    "$DEPENDENCY_MATRIX_TSV" \
    "$RT_AUDIT_TSV" \
    "$SMOKE_PARITY_TSV" \
    "$BRIDGE_PARITY_TSV"; do
    rows="$(count_token_rows_in_tsv "$table" "TODO" "SCAFFOLD")"
    execute_scaffold_rows=$(( execute_scaffold_rows + rows ))
  done

  if [[ "$execute_scaffold_rows" -eq 0 ]]; then
    record_status "BL076-EXEC-scaffold_rows" "PASS" "execute artifact tables contain zero TODO/SCAFFOLD rows" "$OUT_DIR"
  else
    record_status "BL076-EXEC-scaffold_rows" "FAIL" "execute artifact tables contain TODO/SCAFFOLD rows=${execute_scaffold_rows}" "$OUT_DIR"
  fi
fi

if [[ "$fail_count" -eq 0 ]]; then
  record_status "BL076-lane_result" "PASS" "mode=${MODE};runs=${RUNS};pass=${pass_count};fail=${fail_count}" "$OUT_DIR"
  echo "BL-076 guardrails PASS (mode=${MODE}, runs=${RUNS}, pass=${pass_count}, fail=${fail_count})"
  exit 0
fi

record_status "BL076-lane_result" "FAIL" "mode=${MODE};runs=${RUNS};pass=${pass_count};fail=${fail_count}" "$OUT_DIR"
echo "BL-076 guardrails FAIL (mode=${MODE}, runs=${RUNS}, pass=${pass_count}, fail=${fail_count})"
exit 1
