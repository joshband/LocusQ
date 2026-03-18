#!/usr/bin/env bash
# Title: BL-067 AUv3 Lifecycle QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-17
#
# Purpose:
# - Provide deterministic BL-067 contract and execute semantics.
# - Emit AUv3 host, lifecycle, parity, and packaging evidence.
# - Enforce execute-mode truthfulness (zero TODO rows).

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"
MODE="contract_only"
MODE_SET=0
RUNS=1
OUT_DIR="${ROOT_DIR}/TestEvidence/bl067_auv3_lifecycle_${TIMESTAMP}"
BUILD_ROOT="${ROOT_DIR}/build_bl067_auv3_lane_${TIMESTAMP}"
JUCE_DIR_DEFAULT="${ROOT_DIR}/../audio-plugin-coder/_tools/JUCE"

RUN_SUMMARY_TSV=""
run_pass_count=0
run_fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl067-auv3-lifecycle-mac.sh [options]

BL-067 deterministic lane for AUv3 lifecycle and host-validation evidence.

Options:
  --out-dir <path>      Artifact output directory
  --build-root <path>   Scratch build root (default: build_bl067_auv3_lane_<timestamp>)
  --contract-only       Run contract/build-graph checks only (default)
  --execute             Run execute-mode checks (requires zero TODO rows)
  --runs <n>            Number of deterministic replays (default: 1)
  --help, -h            Show usage

Outputs (per run):
  status.tsv
  host_matrix.tsv
  lifecycle_transitions.tsv
  parity_regression.tsv
  packaging_manifest.md
  configure.log
  xcodebuild-list.log

Additional execute-mode output:
  xcodebuild-build.log
  appex_codesign.txt
  appex_info.plist.txt
  shared_code_symbols.txt

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

  case "$result" in
    FAIL)
      ((run_fail_count++)) || true
      ;;
    *)
      ((run_pass_count++)) || true
      ;;
  esac

  echo "  [${result}] $check_id: $detail"
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

append_row() {
  local file="$1"
  shift
  printf "%b\n" "$1" >> "$file"
}

run_and_capture() {
  local log_file="$1"
  shift

  if "$@" >"$log_file" 2>&1; then
    return 0
  fi
  return 1
}

project_has_scheme() {
  local scheme_list="$1"
  local scheme="$2"
  rg -q "^[[:space:]]+${scheme}$" "$scheme_list"
}

file_contains_regex() {
  local file="$1"
  local regex="$2"
  [[ -f "$file" ]] && rg -q "$regex" "$file"
}

append_host_inventory_row() {
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
    taxonomy="host_execution_pending"
    detail="${host_name} detected at ${host_path}; inventory only, host launch/execution is still pending"
  fi

  printf "%s\t%s\tinventory_only\t%s\t%s\t%s\n" \
    "$host_name" \
    "$availability" \
    "$result" \
    "$taxonomy" \
    "$detail" \
    >> "$host_file"

  record "$status_file" "$check_id" "$result" "$detail" "$host_file"
}

resolve_juce_dir() {
  if [[ -n "${JUCE_DIR:-}" ]]; then
    echo "${JUCE_DIR}"
    return
  fi

  if [[ -d "$JUCE_DIR_DEFAULT" ]]; then
    echo "$JUCE_DIR_DEFAULT"
    return
  fi

  echo ""
}

write_packaging_manifest() {
  local manifest_file="$1"
  local mode="$2"
  local run_index="$3"
  local build_dir="$4"
  local host_file="$5"
  local lifecycle_file="$6"
  local parity_file="$7"
  local appex_path="$8"
  local embedded_appex_path="$9"
  local codesign_file="${10}"
  local plist_file="${11}"
  local scheme_list_file="${12}"

  local host_pass host_blocked host_fail
  local lifecycle_pass lifecycle_blocked lifecycle_fail
  local parity_pass parity_blocked parity_fail
  local codesign_signature team_identifier bundle_identifier

  host_pass="$(count_result "$host_file" 4 PASS)"
  host_blocked="$(count_result "$host_file" 4 BLOCKED)"
  host_fail="$(count_result "$host_file" 4 FAIL)"

  lifecycle_pass="$(count_result "$lifecycle_file" 3 PASS)"
  lifecycle_blocked="$(count_result "$lifecycle_file" 3 BLOCKED)"
  lifecycle_fail="$(count_result "$lifecycle_file" 3 FAIL)"

  parity_pass="$(count_result "$parity_file" 3 PASS)"
  parity_blocked="$(count_result "$parity_file" 3 BLOCKED)"
  parity_fail="$(count_result "$parity_file" 3 FAIL)"

  codesign_signature="not_probed"
  team_identifier="not_probed"
  bundle_identifier="not_probed"

  if [[ -f "$codesign_file" ]]; then
    codesign_signature="$(sed -n 's/^Signature=//p' "$codesign_file" | head -n1)"
    team_identifier="$(sed -n 's/^TeamIdentifier=//p' "$codesign_file" | head -n1)"
    [[ -n "$codesign_signature" ]] || codesign_signature="unknown"
    [[ -n "$team_identifier" ]] || team_identifier="unknown"
  fi

  if [[ -f "$plist_file" ]]; then
    bundle_identifier="$(sed -n 's/.*"CFBundleIdentifier" => "\(.*\)"/\1/p' "$plist_file" | head -n1)"
    [[ -n "$bundle_identifier" ]] || bundle_identifier="unknown"
  fi

  cat > "$manifest_file" <<EOF_MANIFEST
Title: BL-067 Packaging Manifest
Document Type: Test Evidence Manifest
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-067 Packaging Manifest

- generated_utc: ${TIMESTAMP}
- lane: BL-067 AUv3 lifecycle and host validation
- mode: ${mode}
- run_index: ${run_index}
- build_dir: ${build_dir}
- xcode_project: ${build_dir}/LocusQ.xcodeproj
- scheme_inventory_log: ${scheme_list_file}

## Build + Packaging Contract

- cmake_contract_file: ${ROOT_DIR}/CMakeLists.txt
- auv3_gate_option: LOCUSQ_ENABLE_AUV3
- apple_formats_contract: VST3 AU Standalone (+AUv3 when LOCUSQ_ENABLE_AUV3=ON)
- signed_build_contract: host-ready AUv3 packaging requires non-adhoc Apple signing for both the app extension and embedding host app
- extension_boundary_contract: ADR-0017 (host-name branching forbidden; capability/state driven fallback only)

## Concrete Artifact Paths

- auv3_bundle: ${appex_path}
- embedded_auv3_bundle: ${embedded_appex_path}
- bundle_identifier: ${bundle_identifier}
- codesign_signature: ${codesign_signature}
- team_identifier: ${team_identifier}

## Evidence Summary

- host_matrix: PASS=${host_pass}, BLOCKED=${host_blocked}, FAIL=${host_fail}
- lifecycle_transitions: PASS=${lifecycle_pass}, BLOCKED=${lifecycle_blocked}, FAIL=${lifecycle_fail}
- parity_regression: PASS=${parity_pass}, BLOCKED=${parity_blocked}, FAIL=${parity_fail}

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
  local build_dir="$3"

  local status_tsv="${run_dir}/status.tsv"
  local host_matrix_tsv="${run_dir}/host_matrix.tsv"
  local lifecycle_tsv="${run_dir}/lifecycle_transitions.tsv"
  local parity_tsv="${run_dir}/parity_regression.tsv"
  local packaging_md="${run_dir}/packaging_manifest.md"
  local configure_log="${run_dir}/configure.log"
  local scheme_log="${run_dir}/xcodebuild-list.log"
  local build_log="${run_dir}/xcodebuild-build.log"
  local appex_codesign_txt="${run_dir}/appex_codesign.txt"
  local appex_plist_txt="${run_dir}/appex_info.plist.txt"
  local shared_symbols_txt="${run_dir}/shared_code_symbols.txt"

  local backlog_doc="${ROOT_DIR}/Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md"
  local annex_doc="${ROOT_DIR}/Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md"
  local skill_doc="${ROOT_DIR}/.codex/skills/auv3-plugin-lifecycle/SKILL.md"
  local adr_doc="${ROOT_DIR}/Documentation/adr/ADR-0017-auv3-app-extension-boundary-and-lifecycle-contract.md"
  local cmake_file="${ROOT_DIR}/CMakeLists.txt"
  local serializer_file="${ROOT_DIR}/Source/processor_core/ProcessorStateSerializer.cpp"
  local processor_header="${ROOT_DIR}/Source/PluginProcessor.h"
  local juce_dir
  local xcode_project
  local generated_auv3_info_plist
  local appex_path
  local embedded_appex_path
  local shared_code_lib
  local todo_rows=0

  run_pass_count=0
  run_fail_count=0

  mkdir -p "$run_dir"
  mkdir -p "$build_dir"

  printf "check_id\tresult\tdetail\tartifact\n" > "$status_tsv"
  printf "host\tavailability\tprobe\tresult\tfailure_taxonomy\tdetail\n" > "$host_matrix_tsv"
  printf "transition\tprobe\tresult\tfailure_taxonomy\tdetail\n" > "$lifecycle_tsv"
  printf "lane\tprobe\tresult\tfailure_taxonomy\tdetail\n" > "$parity_tsv"

  echo "=== BL-067 AUv3 Lifecycle Lane ==="
  echo "Mode: ${MODE}"
  echo "Run: ${run_index}/${RUNS}"
  echo "Output dir: ${run_dir}"
  echo "Build dir: ${build_dir}"

  if [[ "$(uname -s)" == "Darwin" ]]; then
    record "$status_tsv" "BL067-S0-darwin_host" "PASS" "macOS host detected" "$status_tsv"
  else
    record "$status_tsv" "BL067-S0-darwin_host" "FAIL" "BL-067 lane requires macOS" "$status_tsv"
  fi

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

  juce_dir="$(resolve_juce_dir)"
  if [[ -n "$juce_dir" ]] && [[ -d "$juce_dir" ]]; then
    if run_and_capture "$configure_log" \
      cmake -S "$ROOT_DIR" -B "$build_dir" -G Xcode \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DLOCUSQ_ENABLE_AUV3=ON \
        -DLOCUSQ_ENABLE_CLAP=OFF \
        -DJUCE_DIR="$juce_dir"; then
      record "$status_tsv" "BL067-S7-configure_xcode" "PASS" "clean Xcode configure succeeded with LOCUSQ_ENABLE_AUV3=ON" "$configure_log"
    else
      record "$status_tsv" "BL067-S7-configure_xcode" "FAIL" "clean Xcode configure failed with LOCUSQ_ENABLE_AUV3=ON" "$configure_log"
    fi
  else
    printf "JUCE_DIR resolution failed. Checked: %s\n" "$JUCE_DIR_DEFAULT" > "$configure_log"
    record "$status_tsv" "BL067-S7-configure_xcode" "FAIL" "JUCE checkout not found for BL-067 scratch configure" "$configure_log"
  fi

  xcode_project="${build_dir}/LocusQ.xcodeproj"
  if [[ -d "$xcode_project" ]] && run_and_capture "$scheme_log" xcodebuild -list -project "$xcode_project"; then
    record "$status_tsv" "BL067-S8-xcode_scheme_list" "PASS" "xcodebuild -list succeeded for AUv3 scratch project" "$scheme_log"
  else
    record "$status_tsv" "BL067-S8-xcode_scheme_list" "FAIL" "xcodebuild -list failed for AUv3 scratch project" "$scheme_log"
  fi

  if project_has_scheme "$scheme_log" "LocusQ_AUv3"; then
    record "$status_tsv" "BL067-S9-auv3_scheme_present" "PASS" "generated Xcode project exposes LocusQ_AUv3 scheme" "$scheme_log"
  else
    record "$status_tsv" "BL067-S9-auv3_scheme_present" "FAIL" "generated Xcode project missing LocusQ_AUv3 scheme" "$scheme_log"
  fi

  generated_auv3_info_plist="${build_dir}/CMakeFiles/LocusQ_AUv3.dir/Info.plist"
  if file_contains_regex "$generated_auv3_info_plist" 'NSExtension'; then
    append_row "$lifecycle_tsv" "cold_start\tgenerated_plist\tPASS\tnone\tGenerated AUv3 Info.plist contains NSExtension contract"
    record "$status_tsv" "BL067-L1-cold_start" "PASS" "generated AUv3 Info.plist contains NSExtension contract" "$generated_auv3_info_plist"
  else
    append_row "$lifecycle_tsv" "cold_start\tgenerated_plist\tFAIL\tmissing_generated_plist_contract\tGenerated AUv3 Info.plist missing NSExtension contract"
    record "$status_tsv" "BL067-L1-cold_start" "FAIL" "generated AUv3 Info.plist missing NSExtension contract" "$generated_auv3_info_plist"
  fi

  if file_contains_regex "$xcode_project/project.pbxproj" 'PBXCopyFilesBuildPhase' && \
     file_contains_regex "$xcode_project/project.pbxproj" 'RemoveHeadersOnCopy' && \
     file_contains_regex "$xcode_project/project.pbxproj" 'dstSubfolderSpec = 13;'; then
    append_row "$lifecycle_tsv" "reload\tproject_embed_contract\tPASS\tnone\tGenerated project defines copy-files embedding phase for the AUv3 bundle"
    record "$status_tsv" "BL067-L2-reload" "PASS" "Generated project defines copy-files embedding phase for the AUv3 bundle" "$xcode_project/project.pbxproj"
  else
    append_row "$lifecycle_tsv" "reload\tproject_embed_contract\tFAIL\tmissing_embed_contract\tGenerated project is missing the AUv3 copy-files embedding phase"
    record "$status_tsv" "BL067-L2-reload" "FAIL" "Generated project is missing the AUv3 copy-files embedding phase" "$xcode_project/project.pbxproj"
  fi

  if file_contains_regex "$processor_header" 'releaseResources' && file_contains_regex "$processor_header" 'prepareToPlay'; then
    append_row "$lifecycle_tsv" "suspend_resume\tsource_override_probe\tPASS\tnone\tProcessor declares prepareToPlay/releaseResources lifecycle hooks"
    record "$status_tsv" "BL067-L3-suspend_resume" "PASS" "Processor declares prepareToPlay/releaseResources lifecycle hooks" "$processor_header"
  else
    append_row "$lifecycle_tsv" "suspend_resume\tsource_override_probe\tFAIL\tmissing_lifecycle_overrides\tProcessor missing prepareToPlay/releaseResources lifecycle hooks"
    record "$status_tsv" "BL067-L3-suspend_resume" "FAIL" "Processor missing prepareToPlay/releaseResources lifecycle hooks" "$processor_header"
  fi

  if file_contains_regex "$serializer_file" 'getStateInformation' && file_contains_regex "$serializer_file" 'setStateInformation'; then
    append_row "$lifecycle_tsv" "state_restore\tsource_state_probe\tPASS\tnone\tProcessor state serializer defines getStateInformation/setStateInformation"
    record "$status_tsv" "BL067-L4-state_restore" "PASS" "Processor state serializer defines getStateInformation/setStateInformation" "$serializer_file"
  else
    append_row "$lifecycle_tsv" "state_restore\tsource_state_probe\tFAIL\tmissing_state_restore_hooks\tProcessor state serializer missing getStateInformation/setStateInformation"
    record "$status_tsv" "BL067-L4-state_restore" "FAIL" "Processor state serializer missing getStateInformation/setStateInformation" "$serializer_file"
  fi

  if project_has_scheme "$scheme_log" "LocusQ_AU"; then
    append_row "$parity_tsv" "AUv3_vs_AU\txcode_scheme_probe\tPASS\tnone\tGenerated Xcode project retains AU scheme alongside AUv3"
    record "$status_tsv" "BL067-P1-auv3_vs_au" "PASS" "generated Xcode project retains AU scheme alongside AUv3" "$scheme_log"
  else
    append_row "$parity_tsv" "AUv3_vs_AU\txcode_scheme_probe\tFAIL\tmissing_au_scheme\tGenerated Xcode project missing AU scheme"
    record "$status_tsv" "BL067-P1-auv3_vs_au" "FAIL" "generated Xcode project missing AU scheme" "$scheme_log"
  fi

  if project_has_scheme "$scheme_log" "LocusQ_VST3"; then
    append_row "$parity_tsv" "AUv3_vs_VST3\txcode_scheme_probe\tPASS\tnone\tGenerated Xcode project retains VST3 scheme alongside AUv3"
    record "$status_tsv" "BL067-P2-auv3_vs_vst3" "PASS" "generated Xcode project retains VST3 scheme alongside AUv3" "$scheme_log"
  else
    append_row "$parity_tsv" "AUv3_vs_VST3\txcode_scheme_probe\tFAIL\tmissing_vst3_scheme\tGenerated Xcode project missing VST3 scheme"
    record "$status_tsv" "BL067-P2-auv3_vs_vst3" "FAIL" "generated Xcode project missing VST3 scheme" "$scheme_log"
  fi

  if file_contains_regex "$cmake_file" 'LOCUSQ_ENABLE_CLAP'; then
    append_row "$parity_tsv" "AUv3_vs_CLAP\tcmake_gate_probe\tBLOCKED\tslice_c_parity_deferred\tCLAP parity build remains deferred in BL-067 Slice A/B; gate still present in CMake"
    record "$status_tsv" "BL067-P3-auv3_vs_clap" "BLOCKED" "CLAP parity build remains deferred in BL-067 Slice A/B; CMake gate is present" "$cmake_file"
  else
    append_row "$parity_tsv" "AUv3_vs_CLAP\tcmake_gate_probe\tFAIL\tmissing_clap_gate\tCMake missing CLAP gate needed for later parity checks"
    record "$status_tsv" "BL067-P3-auv3_vs_clap" "FAIL" "CMake missing CLAP gate needed for later parity checks" "$cmake_file"
  fi

  if rg -n 'Logic Pro|GarageBand|MainStage' "$ROOT_DIR/Source" > /dev/null 2>&1; then
    append_row "$parity_tsv" "host_name_branching_contract\tsource_scan\tFAIL\thost_name_branching_detected\tSource tree contains host-name-specific branching markers"
    record "$status_tsv" "BL067-P4-no_host_name_branching" "FAIL" "Source tree contains host-name-specific branching markers" "$ROOT_DIR/Source"
  else
    append_row "$parity_tsv" "host_name_branching_contract\tsource_scan\tPASS\tnone\tNo host-name-specific branching markers found in Source"
    record "$status_tsv" "BL067-P4-no_host_name_branching" "PASS" "No host-name-specific branching markers found in Source" "$ROOT_DIR/Source"
  fi

  if rg -q 'VST3' "$cmake_file" 2>/dev/null && rg -q 'AU' "$cmake_file" 2>/dev/null; then
    record "$status_tsv" "BL067-P5-cmake_core_formats_present" "PASS" "CMake includes AU and VST3 format contracts" "$cmake_file"
  else
    record "$status_tsv" "BL067-P5-cmake_core_formats_present" "FAIL" "CMake missing AU or VST3 format contracts" "$cmake_file"
  fi

  if rg -q 'LOCUSQ_ENABLE_CLAP' "$cmake_file" 2>/dev/null; then
    record "$status_tsv" "BL067-P6-cmake_clap_gate_present" "PASS" "CMake includes CLAP gate for later parity checks" "$cmake_file"
  else
    record "$status_tsv" "BL067-P6-cmake_clap_gate_present" "FAIL" "CMake missing CLAP gate contract" "$cmake_file"
  fi

  append_host_inventory_row "$status_tsv" "$host_matrix_tsv" "BL067-H1-logic_pro" "Logic Pro" "/Applications/Logic Pro.app"
  append_host_inventory_row "$status_tsv" "$host_matrix_tsv" "BL067-H2-garageband" "GarageBand" "/Applications/GarageBand.app"
  append_host_inventory_row "$status_tsv" "$host_matrix_tsv" "BL067-H3-mainstage" "MainStage" "/Applications/MainStage.app"

  appex_path="${build_dir}/LocusQ_artefacts/Release/AUv3/LocusQ.appex"
  embedded_appex_path="${build_dir}/LocusQ_artefacts/Release/Standalone/LocusQ.app/Contents/PlugIns/LocusQ.appex"
  shared_code_lib="${build_dir}/LocusQ_artefacts/Release/libLocusQ_SharedCode.a"

  if [[ "$MODE" == "execute" ]]; then
    if run_and_capture "$build_log" \
      xcodebuild -project "$xcode_project" \
        -scheme LocusQ_AUv3 \
        -configuration Release \
        -destination "generic/platform=macOS" \
        CODE_SIGNING_ALLOWED=NO \
        build; then
      record "$status_tsv" "BL067-E2-build_release_unsigned" "PASS" "unsigned AUv3 release build succeeded" "$build_log"
    else
      record "$status_tsv" "BL067-E2-build_release_unsigned" "FAIL" "unsigned AUv3 release build failed" "$build_log"
    fi

    if [[ -d "$appex_path" ]]; then
      record "$status_tsv" "BL067-E3-appex_exists" "PASS" "AUv3 appex bundle produced at expected path" "$appex_path"
    else
      record "$status_tsv" "BL067-E3-appex_exists" "FAIL" "AUv3 appex bundle missing after build" "$appex_path"
    fi

    if [[ -d "$embedded_appex_path" ]]; then
      append_row "$lifecycle_tsv" "reload\tembedded_bundle_probe\tPASS\tnone\tStandalone host app embeds LocusQ.appex after build"
      record "$status_tsv" "BL067-E4-standalone_embeds_appex" "PASS" "Standalone host app embeds LocusQ.appex after build" "$embedded_appex_path"
    else
      append_row "$lifecycle_tsv" "reload\tembedded_bundle_probe\tFAIL\tmissing_embedded_appex\tStandalone host app missing embedded LocusQ.appex after build"
      record "$status_tsv" "BL067-E4-standalone_embeds_appex" "FAIL" "Standalone host app missing embedded LocusQ.appex after build" "$embedded_appex_path"
    fi

    if [[ -d "$appex_path" ]]; then
      if codesign -dv --verbose=4 "$appex_path" >"$appex_codesign_txt" 2>&1; then
        local signature team_id
        signature="$(sed -n 's/^Signature=//p' "$appex_codesign_txt" | head -n1)"
        team_id="$(sed -n 's/^TeamIdentifier=//p' "$appex_codesign_txt" | head -n1)"

        if [[ "$signature" == "adhoc" || -z "$team_id" || "$team_id" == "not set" ]]; then
          record "$status_tsv" "BL067-E5-signing_ready" "BLOCKED" "AUv3 bundle is only adhoc-signed; host-ready Apple signing is still pending" "$appex_codesign_txt"
        else
          record "$status_tsv" "BL067-E5-signing_ready" "PASS" "AUv3 bundle carries non-adhoc Apple signing metadata" "$appex_codesign_txt"
        fi
      else
        record "$status_tsv" "BL067-E5-signing_ready" "FAIL" "codesign inspection failed for AUv3 bundle" "$appex_codesign_txt"
      fi

      if plutil -p "${appex_path}/Contents/Info.plist" >"$appex_plist_txt"; then
        if rg -q '"sandboxSafe" => true' "$appex_plist_txt" && rg -q '"NSExtensionPointIdentifier" => "com.apple.AudioUnit-UI"' "$appex_plist_txt"; then
          append_row "$lifecycle_tsv" "cold_start\tbundle_metadata_probe\tPASS\tnone\tBuilt AUv3 bundle advertises AudioUnit extension metadata and sandboxSafe=true"
          record "$status_tsv" "BL067-E6-appex_metadata" "PASS" "Built AUv3 bundle advertises AudioUnit extension metadata and sandboxSafe=true" "$appex_plist_txt"
        else
          append_row "$lifecycle_tsv" "cold_start\tbundle_metadata_probe\tFAIL\tmissing_bundle_metadata\tBuilt AUv3 bundle missing expected AudioUnit extension metadata"
          record "$status_tsv" "BL067-E6-appex_metadata" "FAIL" "Built AUv3 bundle missing expected AudioUnit extension metadata" "$appex_plist_txt"
        fi
      else
        record "$status_tsv" "BL067-E6-appex_metadata" "FAIL" "failed to inspect AUv3 Info.plist" "$appex_plist_txt"
      fi
    fi

    if [[ -f "$shared_code_lib" ]] && nm -gj "$shared_code_lib" >"$shared_symbols_txt" 2>/dev/null; then
      if rg -q '__ZN20LocusQAudioProcessor13prepareToPlayEdi' "$shared_symbols_txt" && \
         rg -q '__ZN20LocusQAudioProcessor16releaseResourcesEv' "$shared_symbols_txt"; then
        append_row "$lifecycle_tsv" "suspend_resume\tcompiled_symbol_probe\tPASS\tnone\tShared code exports prepareToPlay/releaseResources lifecycle hooks"
        record "$status_tsv" "BL067-E7-suspend_resume_symbols" "PASS" "Shared code exports prepareToPlay/releaseResources lifecycle hooks" "$shared_symbols_txt"
      else
        append_row "$lifecycle_tsv" "suspend_resume\tcompiled_symbol_probe\tFAIL\tmissing_compiled_lifecycle_symbols\tShared code missing compiled prepareToPlay/releaseResources lifecycle hooks"
        record "$status_tsv" "BL067-E7-suspend_resume_symbols" "FAIL" "Shared code missing compiled prepareToPlay/releaseResources lifecycle hooks" "$shared_symbols_txt"
      fi

      if rg -q '__ZN20LocusQAudioProcessor19getStateInformationERN4juce11MemoryBlockE' "$shared_symbols_txt" && \
         rg -q '__ZN20LocusQAudioProcessor19setStateInformationEPKvi' "$shared_symbols_txt"; then
        append_row "$lifecycle_tsv" "state_restore\tcompiled_symbol_probe\tPASS\tnone\tShared code exports getStateInformation/setStateInformation state hooks"
        record "$status_tsv" "BL067-E8-state_restore_symbols" "PASS" "Shared code exports getStateInformation/setStateInformation state hooks" "$shared_symbols_txt"
      else
        append_row "$lifecycle_tsv" "state_restore\tcompiled_symbol_probe\tFAIL\tmissing_compiled_state_symbols\tShared code missing compiled getStateInformation/setStateInformation hooks"
        record "$status_tsv" "BL067-E8-state_restore_symbols" "FAIL" "Shared code missing compiled getStateInformation/setStateInformation hooks" "$shared_symbols_txt"
      fi
    else
      record "$status_tsv" "BL067-E7-suspend_resume_symbols" "FAIL" "failed to inspect compiled shared-code symbols" "$shared_symbols_txt"
      record "$status_tsv" "BL067-E8-state_restore_symbols" "FAIL" "failed to inspect compiled shared-code symbols" "$shared_symbols_txt"
    fi
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

  write_packaging_manifest \
    "$packaging_md" \
    "$MODE" \
    "$run_index" \
    "$build_dir" \
    "$host_matrix_tsv" \
    "$lifecycle_tsv" \
    "$parity_tsv" \
    "$appex_path" \
    "$embedded_appex_path" \
    "$appex_codesign_txt" \
    "$appex_plist_txt" \
    "$scheme_log"

  if [[ "$run_fail_count" -eq 0 ]]; then
    record "$status_tsv" "lane_result" "PASS" "mode=${MODE};run=${run_index};failures=0" "$status_tsv"
  else
    record "$status_tsv" "lane_result" "FAIL" "mode=${MODE};run=${run_index};failures=${run_fail_count}" "$status_tsv"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "$MODE" \
    "$run_pass_count" \
    "$run_fail_count" \
    "$todo_rows" \
    "$build_dir" \
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
    --build-root)
      [[ $# -ge 2 ]] || usage_error "--build-root requires a value"
      BUILD_ROOT="$2"
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
mkdir -p "$BUILD_ROOT"
RUN_SUMMARY_TSV="${OUT_DIR}/run_summary.tsv"
printf "run\tmode\tnon_fail_checks\tfail_checks\ttodo_rows\tbuild_dir\tartifact_dir\n" > "$RUN_SUMMARY_TSV"

overall_fail_runs=0

for run_index in $(seq 1 "$RUNS"); do
  if [[ "$RUNS" -gt 1 ]]; then
    run_dir="${OUT_DIR}/run_$(printf '%02d' "$run_index")"
    build_dir="${BUILD_ROOT}/run_$(printf '%02d' "$run_index")"
  else
    run_dir="$OUT_DIR"
    build_dir="$BUILD_ROOT"
  fi

  if ! run_single "$run_index" "$run_dir" "$build_dir"; then
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
