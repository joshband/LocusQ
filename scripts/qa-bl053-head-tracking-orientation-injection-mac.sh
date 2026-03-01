#!/usr/bin/env bash
# qa-bl053-head-tracking-orientation-injection-mac.sh
# BL-053 QA harness: head-tracking orientation injection for virtual_binaural
# calibration monitoring path.
#
# Evidence schema: TestEvidence/bl053_*/status.tsv
# Exit codes:
#   0  All checks passed
#   1  One or more checks failed

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%d_%H%M%S)"
EVIDENCE_DIR="${REPO_ROOT}/TestEvidence/bl053_${TIMESTAMP}"
STATUS_TSV="${EVIDENCE_DIR}/status.tsv"

mkdir -p "${EVIDENCE_DIR}"

pass=0
fail=0

record() {
    local id="$1" result="$2" detail="$3"
    printf '%s\t%s\t%s\n' "${id}" "${result}" "${detail}" >> "${STATUS_TSV}"
    if [[ "${result}" == "PASS" ]]; then
        echo "  [PASS] ${id}: ${detail}"
        (( pass++ )) || true
    else
        echo "  [FAIL] ${id}: ${detail}"
        (( fail++ )) || true
    fi
}

echo "=== BL-053 QA: Head Tracking Orientation Injection ==="
echo "Evidence dir: ${EVIDENCE_DIR}"
printf 'check_id\tresult\tdetail\n' > "${STATUS_TSV}"

SR_FILE="${REPO_ROOT}/Source/SpatialRenderer.h"
SVS_FILE="${REPO_ROOT}/Source/SteamAudioVirtualSurround.h"
PP_H="${REPO_ROOT}/Source/PluginProcessor.h"
PP_CPP="${REPO_ROOT}/Source/PluginProcessor.cpp"

# C1: SteamAudioVirtualSurround applyBlock accepts listenerOrientation pointer.
if grep -q 'const IPLCoordinateSpace3\* listenerOrientation' "${SVS_FILE}" 2>/dev/null; then
    record "C1_apply_block_orientation_param" "PASS" "SteamAudioVirtualSurround::applyBlock orientation param present"
else
    record "C1_apply_block_orientation_param" "FAIL" "Missing orientation parameter in ${SVS_FILE}"
fi

# C2: SpatialRenderer monitoring render accepts optional listenerOrientation.
if grep -q 'const IPLCoordinateSpace3\* listenerOrientation' "${SR_FILE}" 2>/dev/null; then
    record "C2_monitor_render_orientation_param" "PASS" "renderVirtualSurroundForMonitoring orientation param present"
else
    record "C2_monitor_render_orientation_param" "FAIL" "Missing orientation parameter in ${SR_FILE}"
fi

# C3: Orientation rotation scratch/mix buffers for monitoring path exist.
if grep -q 'monitoringHeadPoseRotatedQuadScratch_' "${SR_FILE}" 2>/dev/null \
    && grep -q 'monitoringSpeakerMix_' "${SR_FILE}" 2>/dev/null; then
    record "C3_monitor_orientation_scratch" "PASS" "Monitoring orientation scratch/mix buffers present"
else
    record "C3_monitor_orientation_scratch" "FAIL" "Monitoring orientation scratch/mix buffers missing in ${SR_FILE}"
fi

# C4: Coordinate-space to listener-orientation conversion helper exists.
if grep -q 'tryBuildListenerOrientationFromCoordinateSpace' "${SR_FILE}" 2>/dev/null; then
    record "C4_coordinate_space_helper" "PASS" "Coordinate-space conversion helper present"
else
    record "C4_coordinate_space_helper" "FAIL" "Coordinate-space helper missing in ${SR_FILE}"
fi

# C5: applyCalibrationMonitoringPath builds monitoring orientation pointer.
if grep -q 'monitoringOrientationPtr' "${PP_CPP}" 2>/dev/null \
    && grep -q 'poseSnapshotToCoordinateSpace' "${PP_CPP}" 2>/dev/null; then
    record "C5_monitoring_orientation_wired" "PASS" "Calibration monitoring orientation pointer wiring present"
else
    record "C5_monitoring_orientation_wired" "FAIL" "Calibration monitoring orientation wiring missing in ${PP_CPP}"
fi

# C6: virtual_binaural branch is gated by calibration profile tracking enable.
if grep -q 'monPathId == path::kVirtualBinaural' "${PP_CPP}" 2>/dev/null \
    && grep -q 'calibrationProfileTrackingEnabled' "${PP_CPP}" 2>/dev/null; then
    record "C6_virtual_binaural_tracking_gate" "PASS" "virtual_binaural tracking gate present"
else
    record "C6_virtual_binaural_tracking_gate" "FAIL" "virtual_binaural tracking gate missing in ${PP_CPP}"
fi

# C7: stale/disconnect fallback guard is present.
if grep -q 'kRendererHeadTrackingStaleMs' "${PP_CPP}" 2>/dev/null \
    && grep -q 'currentTimeMillis' "${PP_CPP}" 2>/dev/null; then
    record "C7_stale_pose_guard" "PASS" "Stale/disconnect fallback guard present"
else
    record "C7_stale_pose_guard" "FAIL" "Stale/disconnect fallback guard missing in ${PP_CPP}"
fi

# C8: profile + runtime yaw offsets are composed and applied.
if grep -q 'calibrationProfileYawOffsetDeg' "${PP_CPP}" 2>/dev/null \
    && grep -q 'applyYawOffsetToPose' "${PP_CPP}" 2>/dev/null \
    && grep -q 'runtimeYawOffsetDeg' "${PP_CPP}" 2>/dev/null; then
    record "C8_yaw_offset_composition" "PASS" "Profile and runtime yaw offsets composed/applied"
else
    record "C8_yaw_offset_composition" "FAIL" "Yaw offset composition missing in ${PP_CPP}"
fi

# C9: CalibrationProfile tracking fields are parsed from disk poller.
if grep -q 'hp_tracking_enabled' "${PP_CPP}" 2>/dev/null \
    && grep -q 'hp_yaw_offset_deg' "${PP_CPP}" 2>/dev/null; then
    record "C9_profile_tracking_fields_parsed" "PASS" "CalibrationProfile tracking fields parsed"
else
    record "C9_profile_tracking_fields_parsed" "FAIL" "CalibrationProfile tracking fields missing in ${PP_CPP}"
fi

# C10: Audio-thread-safe tracking/yaw atomics are declared in PluginProcessor.
if grep -q 'calibrationProfileTrackingEnabled' "${PP_H}" 2>/dev/null \
    && grep -q 'calibrationProfileYawOffsetDeg' "${PP_H}" 2>/dev/null; then
    record "C10_tracking_yaw_atomics_declared" "PASS" "Tracking/yaw atomics declared in PluginProcessor.h"
else
    record "C10_tracking_yaw_atomics_declared" "FAIL" "Tracking/yaw atomics missing in ${PP_H}"
fi

# C11: Monitoring call passes orientation pointer into SteamAudioVirtualSurround.
if awk '/applyCalibrationMonitoringPath.*juce::AudioBuffer/,/^}/' "${PP_CPP}" | grep -q 'monitoringOrientationPtr'; then
    record "C11_apply_block_orientation_forwarded" "PASS" "Orientation pointer forwarded to calMonitorVirtualSurround.applyBlock"
else
    record "C11_apply_block_orientation_forwarded" "FAIL" "Orientation pointer not forwarded in applyCalibrationMonitoringPath"
fi

# C12: RT-safety heuristic: no dynamic allocation in applyCalibrationMonitoringPath.
ALLOC_IN_IMPL=$(awk '/applyCalibrationMonitoringPath.*juce::AudioBuffer/,/^}/' "${PP_CPP}" \
    | { grep -E '\bnew\b|\bmalloc\b|\bvector[[:space:]]*\(|\bresize[[:space:]]*\(' || true; } \
    | wc -l | tr -d '[:space:]')
if [[ "${ALLOC_IN_IMPL}" -eq 0 ]]; then
    record "C12_no_rt_allocation" "PASS" "No dynamic allocation detected in applyCalibrationMonitoringPath"
else
    record "C12_no_rt_allocation" "FAIL" "Possible dynamic allocation in applyCalibrationMonitoringPath (${ALLOC_IN_IMPL} hit(s))"
fi

echo ""
echo "Results: ${pass} passed, ${fail} failed"
echo "Evidence: ${STATUS_TSV}"

if [[ "${fail}" -gt 0 ]]; then
    exit 1
fi
exit 0
