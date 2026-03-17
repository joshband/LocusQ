#!/usr/bin/env bash
# Title: BL-059 Calibration Integration Smoke Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-02-28
# Last Modified Date: 2026-03-07
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl059_calibration_integration_smoke_${TIMESTAMP}_$$"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
CONTRACT_MATRIX_TSV=""
PROFILE_ROUNDTRIP_TSV=""
ORIENTATION_INVARIANTS_TSV=""
REPLAY_HASHES_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl059-calibration-integration-smoke-mac.sh [options]

BL-059 calibration profile integration smoke lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks with runtime smoke + dependency replays
  --help, -h         Show usage

Outputs:
  status.tsv
  contract_matrix.tsv
  profile_roundtrip.tsv
  orientation_invariants.tsv
  replay_hashes.tsv
USAGE
}

usage_error() {
  local message="$1"
  echo "ERROR: ${message}" >&2
  usage >&2
  exit 2
}

record() {
  local check_id="$1"
  local result="$2"
  local detail="$3"
  local artifact="${4:-}"
  printf "%s\t%s\t%s\t%s\n" \
    "$check_id" \
    "$result" \
    "${detail//$'\t'/ }" \
    "${artifact//$'\t'/ }" \
    >> "$STATUS_TSV"

  if [[ "$result" == "PASS" ]]; then
    ((pass_count++)) || true
  else
    ((fail_count++)) || true
  fi
}

write_hash_row() {
  local artifact_id="$1"
  local result="$2"
  local file_path="$3"
  local sha256="missing"

  if [[ -f "$file_path" ]]; then
    sha256="$(shasum -a 256 "$file_path" | awk '{print $1}')"
  fi

  printf "%s\t%s\t%s\t%s\n" \
    "$artifact_id" \
    "$result" \
    "$sha256" \
    "$file_path" \
    >> "$REPLAY_HASHES_TSV"
}

run_profile_smoke() {
  local scenario_id="$1"
  local hp_model_id="$2"
  local hp_mode="$3"
  local hp_eq_mode="$4"
  local hp_hrtf_mode="$5"
  local tracking_enabled="$6"
  local yaw_offset_deg="$7"
  local sofa_ref="$8"
  local fir_taps_json="$9"

  local fixture_dir="${OUT_DIR}/runtime_profiles"
  local fixture_path="${fixture_dir}/${scenario_id}.json"
  local log_path="${OUT_DIR}/${scenario_id}.log"
  mkdir -p "$fixture_dir"

  cat > "$fixture_path" <<JSON
{
  "schema": "locusq-calibration-profile-v1",
  "user": {
    "subject_id": "H3",
    "sofa_ref": "${sofa_ref}",
    "embedding_hash": "bl059-${scenario_id}"
  },
  "headphone": {
    "hp_model_id": "${hp_model_id}",
    "hp_mode": "${hp_mode}",
    "hp_eq_mode": "${hp_eq_mode}",
    "hp_hrtf_mode": "${hp_hrtf_mode}",
    "hp_peq_bands": [],
    "hp_fir_taps": ${fir_taps_json}
  },
  "tracking": {
    "hp_tracking_enabled": ${tracking_enabled},
    "hp_yaw_offset_deg": ${yaw_offset_deg}
  },
  "verification": {}
}
JSON

  if LOCUSQ_COMPANION_PROFILE_FILE="$fixture_path" \
      "$QA_BIN" --calibrate "$CALIBRATE_SCENARIO" > "$log_path" 2>&1; then
    printf "%s\tPASS\tprofile fixture %s loaded and calibrate smoke exited 0\t%s\n" \
      "$scenario_id" "$fixture_path" "$log_path" >> "$PROFILE_ROUNDTRIP_TSV"
    record "BL059-E2-${scenario_id}" "PASS" \
      "profile fixture ${scenario_id} loaded and calibrate smoke exited 0" "$log_path"
    write_hash_row "$scenario_id" "PASS" "$log_path"
  else
    printf "%s\tFAIL\tprofile fixture %s caused calibrate smoke failure\t%s\n" \
      "$scenario_id" "$fixture_path" "$log_path" >> "$PROFILE_ROUNDTRIP_TSV"
    record "BL059-E2-${scenario_id}" "FAIL" \
      "profile fixture ${scenario_id} caused calibrate smoke failure" "$log_path"
    write_hash_row "$scenario_id" "FAIL" "$log_path"
  fi
}

run_missing_profile_smoke() {
  local scenario_id="$1"
  local missing_path="${OUT_DIR}/runtime_profiles/${scenario_id}.json"
  local log_path="${OUT_DIR}/${scenario_id}.log"

  rm -f "$missing_path"

  if LOCUSQ_COMPANION_PROFILE_FILE="$missing_path" \
      "$QA_BIN" --calibrate "$CALIBRATE_SCENARIO" > "$log_path" 2>&1; then
    printf "%s\tPASS\tmissing profile override path preserved calibrate smoke exit 0\t%s\n" \
      "$scenario_id" "$log_path" >> "$PROFILE_ROUNDTRIP_TSV"
    record "BL059-E2-${scenario_id}" "PASS" \
      "missing profile override path preserved calibrate smoke exit 0" "$log_path"
    write_hash_row "$scenario_id" "PASS" "$log_path"
  else
    printf "%s\tFAIL\tmissing profile override path caused calibrate smoke failure\t%s\n" \
      "$scenario_id" "$log_path" >> "$PROFILE_ROUNDTRIP_TSV"
    record "BL059-E2-${scenario_id}" "FAIL" \
      "missing profile override path caused calibrate smoke failure" "$log_path"
    write_hash_row "$scenario_id" "FAIL" "$log_path"
  fi
}

run_dependency_replay() {
  local scenario_id="$1"
  shift

  local log_path="${OUT_DIR}/${scenario_id}.log"
  if "$@" > "$log_path" 2>&1; then
    printf "%s\tPASS\tdependency replay passed\t%s\n" \
      "$scenario_id" "$log_path" >> "$ORIENTATION_INVARIANTS_TSV"
    record "BL059-E3-${scenario_id}" "PASS" "dependency replay passed" "$log_path"
    write_hash_row "$scenario_id" "PASS" "$log_path"
  else
    printf "%s\tFAIL\tdependency replay failed\t%s\n" \
      "$scenario_id" "$log_path" >> "$ORIENTATION_INVARIANTS_TSV"
    record "BL059-E3-${scenario_id}" "FAIL" "dependency replay failed" "$log_path"
    write_hash_row "$scenario_id" "FAIL" "$log_path"
  fi
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
    --help|-h)
      usage
      exit 0
      ;;
    *)
      usage_error "unknown argument: $1"
      ;;
  esac
done

command -v rg >/dev/null 2>&1 || usage_error "ripgrep (rg) is required"
command -v shasum >/dev/null 2>&1 || usage_error "shasum is required"

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
CONTRACT_MATRIX_TSV="${OUT_DIR}/contract_matrix.tsv"
PROFILE_ROUNDTRIP_TSV="${OUT_DIR}/profile_roundtrip.tsv"
ORIENTATION_INVARIANTS_TSV="${OUT_DIR}/orientation_invariants.tsv"
REPLAY_HASHES_TSV="${OUT_DIR}/replay_hashes.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$CONTRACT_MATRIX_TSV"
printf "scenario\tresult\tdetail\tartifact\n" > "$PROFILE_ROUNDTRIP_TSV"
printf "scenario\tresult\tdetail\tartifact\n" > "$ORIENTATION_INVARIANTS_TSV"
printf "artifact\tresult\tsha256\tpath\n" > "$REPLAY_HASHES_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-059-calibration-profile-integration-handoff.md"
PROCESSOR_CPP="${ROOT_DIR}/Source/PluginProcessor.cpp"
CALIBRATION_BRIDGE_CPP="${ROOT_DIR}/Source/processor_core/ProcessorCalibrationBridge.cpp"
UI_BRIDGE_HDR="${ROOT_DIR}/Source/processor_bridge/ProcessorUiBridgeOps.h"
SCENE_STATE_BRIDGE_HDR="${ROOT_DIR}/Source/processor_bridge/ProcessorSceneStateBridgeOps.h"
CALIBRATION_ENGINE_HDR="${ROOT_DIR}/Source/CalibrationEngine.h"
CALIBRATE_SCENARIO="${ROOT_DIR}/qa/scenarios/locusq_bl052_calibration_monitoring_suite.json"

QA_BIN="${BL059_QA_BIN:-${ROOT_DIR}/build_local/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL059_QA_BIN_FALLBACK:-${ROOT_DIR}/build_local/locusq_qa_artefacts/locusq_qa}"
fi

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL059-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL059-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if [[ -f "$CALIBRATION_ENGINE_HDR" ]]; then
  record "BL059-C2-calibration_engine_exists" "PASS" "calibration engine source present" "$CALIBRATION_ENGINE_HDR"
else
  record "BL059-C2-calibration_engine_exists" "FAIL" "calibration engine source missing" "$CALIBRATION_ENGINE_HDR"
fi

contract_detail=""
contract_failed=0
if rg -q 'LOCUSQ_COMPANION_PROFILE_FILE|LOCUSQ_COMPANION_PROFILE_DIR' "$CALIBRATION_BRIDGE_CPP"; then
  contract_detail+="test_override_profile_path;"
else
  contract_detail+="missing_test_override_profile_path;"
  contract_failed=1
fi
if rg -q 'pollCompanionCalibrationProfileFromDisk|resolveCompanionCalibrationProfileFile|CalibrationProfile\.json' "$CALIBRATION_BRIDGE_CPP"; then
  contract_detail+="companion_profile_poller;"
else
  contract_detail+="missing_companion_profile_poller;"
  contract_failed=1
fi
if rg -q 'clearFirImpulseResponse\(\);' "$CALIBRATION_BRIDGE_CPP" \
   && rg -q 'setHeadphoneCalibrationEnabled \(false\);' "$CALIBRATION_BRIDGE_CPP" \
   && rg -q 'setRequestedSofaHrtf \(\{\}, false\);' "$CALIBRATION_BRIDGE_CPP"; then
  contract_detail+="profile_clear_path;"
else
  contract_detail+="missing_missing_profile_clear_path;"
  contract_failed=1
fi
if rg -q 'setIntegerParameterValueNotifyingHost \("cal_device_profile", profileIndex\);' "$CALIBRATION_BRIDGE_CPP" \
   && rg -q 'setIntegerParameterValueNotifyingHost \("rend_headphone_profile", profileIndex\);' "$CALIBRATION_BRIDGE_CPP"; then
  contract_detail+="apvts_device_sync;"
else
  contract_detail+="missing_apvts_device_sync;"
  contract_failed=1
fi
if rg -q 'applyJsonPeqBands|loadFirTapsFromJson|sofa_ref|reloadSteamAudioRuntime|hp_tracking_enabled|hp_yaw_offset_deg' "$CALIBRATION_BRIDGE_CPP"; then
  contract_detail+="peq_fir_sofa_tracking_ingest;"
else
  contract_detail+="missing_peq_fir_sofa_tracking_ingest;"
  contract_failed=1
fi
if rg -q 'profileSyncSeq|headphoneCalibrationSchema|headphoneVerificationSchema|hpDeviceStatus' "$UI_BRIDGE_HDR"; then
  contract_detail+="ui_status_contract_publication;"
else
  contract_detail+="missing_ui_status_contract_publication;"
  contract_failed=1
fi
if rg -q 'rendererHeadTrackingSeq|rendererHeadTrackingAgeMs|rendererHeadTrackingPoseStale' "$SCENE_STATE_BRIDGE_HDR"; then
  contract_detail+="head_tracking_seq_age_visibility;"
else
  contract_detail+="missing_head_tracking_seq_age_visibility;"
  contract_failed=1
fi

if (( contract_failed == 1 )); then
  printf "companion_profile_handoff_contract\tFAIL\t%s\t%s\n" "$contract_detail" "$CALIBRATION_BRIDGE_CPP" >> "$CONTRACT_MATRIX_TSV"
  record "BL059-C3-companion_profile_handoff_contract" "FAIL" "$contract_detail" "$CONTRACT_MATRIX_TSV"
else
  printf "companion_profile_handoff_contract\tPASS\t%s\t%s\n" "$contract_detail" "$CALIBRATION_BRIDGE_CPP" >> "$CONTRACT_MATRIX_TSV"
  record "BL059-C3-companion_profile_handoff_contract" "PASS" "$contract_detail" "$CONTRACT_MATRIX_TSV"
fi

orientation_detail=""
if rg -q 'tryBuildFreshInterpolatedHeadPose' "$PROCESSOR_CPP" \
   && rg -q 'headPoseInterpolator\.reset\(\);' "$PROCESSOR_CPP" \
   && rg -q 'spatialRenderer\.clearHeadPose\(\);' "$PROCESSOR_CPP"; then
  orientation_detail+="stale_fallback_reset_path;"
else
  orientation_detail+="missing_stale_fallback_reset_path;"
fi
if rg -q 'const float profileYawOffsetDeg = calibrationProfileYawOffsetDeg' "$PROCESSOR_CPP" \
   && rg -q 'applyYawOffsetToPose \(listenerPose, profileYawOffsetDeg \+ runtimeYawOffsetDeg\);' "$PROCESSOR_CPP"; then
  orientation_detail+="yaw_offset_composition;"
else
  orientation_detail+="missing_yaw_offset_composition;"
fi
if rg -q 'buildRendererHeadTrackingSnapshot' "$SCENE_STATE_BRIDGE_HDR" \
   && rg -q 'rendererHeadTrackingInvalidPackets' "$SCENE_STATE_BRIDGE_HDR"; then
  orientation_detail+="renderer_head_tracking_diagnostics;"
else
  orientation_detail+="missing_renderer_head_tracking_diagnostics;"
fi

if [[ "$orientation_detail" == *"missing_"* ]]; then
  printf "orientation_profile_handoff_invariants\tFAIL\t%s\t%s\n" "$orientation_detail" "$PROCESSOR_CPP" >> "$ORIENTATION_INVARIANTS_TSV"
  record "BL059-C4-orientation_profile_handoff_invariants" "FAIL" "$orientation_detail" "$ORIENTATION_INVARIANTS_TSV"
else
  printf "orientation_profile_handoff_invariants\tPASS\t%s\t%s\n" "$orientation_detail" "$PROCESSOR_CPP" >> "$ORIENTATION_INVARIANTS_TSV"
  record "BL059-C4-orientation_profile_handoff_invariants" "PASS" "$orientation_detail" "$ORIENTATION_INVARIANTS_TSV"
fi

if [[ "$MODE" == "execute" ]]; then
  if [[ ! -x "$QA_BIN" ]]; then
    record "BL059-E1-qa_bin_present" "FAIL" "qa binary missing: ${QA_BIN}" "$QA_BIN"
  else
    record "BL059-E1-qa_bin_present" "PASS" "qa binary present: ${QA_BIN}" "$QA_BIN"

    run_profile_smoke "airpods_peq_profile_smoke" "airpods_pro_2" "anc_on" "peq" "default" "true" "12.5" "" "[]"
    run_profile_smoke "sony_peq_profile_smoke" "sony_wh1000xm5" "anc_on" "peq" "default" "false" "0.0" "" "[]"
    run_profile_smoke "custom_sofa_fir_profile_smoke" "custom_sofa" "reference" "fir" "sofa" "true" "-18.0" "sadie2/H3_HRIR.sofa" "[1.0, 0.0, 0.0, 0.0]"
    run_missing_profile_smoke "profile_unload_recovery"

    run_dependency_replay "bl053_orientation_dependency_replay" "${ROOT_DIR}/scripts/qa-bl053-head-tracking-orientation-injection-mac.sh"
    run_dependency_replay "bl055_fir_dependency_replay" "${ROOT_DIR}/scripts/qa-bl055-fir-convolution-engine-mac.sh" --execute
  fi
else
  record "BL059-C5-contract_mode" "PASS" "contract-only mode skips runtime smoke and dependency replays" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl059_lane_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $CONTRACT_MATRIX_TSV"
echo "- $PROFILE_ROUNDTRIP_TSV"
echo "- $ORIENTATION_INVARIANTS_TSV"
echo "- $REPLAY_HASHES_TSV"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
