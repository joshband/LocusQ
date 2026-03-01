#!/usr/bin/env bash
# qa-bl052-steam-audio-virtual-surround-mac.sh
# BL-052 QA harness: SteamAudioVirtualSurround quad→binaural + monitoring mode switch.
# Platform: macOS (arm64 / x86_64)
#
# Evidence schema: TestEvidence/bl052_*/status.tsv
# Required artefacts: Source/QuadSpeakerLayout.h, Source/SteamAudioVirtualSurround.h
#
# Exit codes:
#   0  All checks pass
#   1  One or more checks failed

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%d_%H%M%S)"
EVIDENCE_DIR="${REPO_ROOT}/TestEvidence/bl052_${TIMESTAMP}"
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

echo "=== BL-052 QA: SteamAudioVirtualSurround ==="
echo "Evidence dir: ${EVIDENCE_DIR}"
printf 'check_id\tresult\tdetail\n' >> "${STATUS_TSV}"

# ── C1: QuadSpeakerLayout.h exists ──────────────────────────────────────────
C1_FILE="${REPO_ROOT}/Source/QuadSpeakerLayout.h"
if [[ -f "${C1_FILE}" ]]; then
    record "C1_quad_layout_header_exists" "PASS" "${C1_FILE}"
else
    record "C1_quad_layout_header_exists" "FAIL" "Missing: ${C1_FILE}"
fi

# ── C2: QuadSpeakerLayout::Quadraphonic = 0 defined ─────────────────────────
if grep -q 'Quadraphonic *= *0' "${C1_FILE}" 2>/dev/null; then
    record "C2_quadraphonic_enum_value" "PASS" "QuadSpeakerLayout::Quadraphonic = 0 found"
else
    record "C2_quadraphonic_enum_value" "FAIL" "QuadSpeakerLayout::Quadraphonic = 0 not found in ${C1_FILE}"
fi

# ── C3: SteamAudioVirtualSurround.h exists ──────────────────────────────────
C3_FILE="${REPO_ROOT}/Source/SteamAudioVirtualSurround.h"
if [[ -f "${C3_FILE}" ]]; then
    record "C3_virtual_surround_header_exists" "PASS" "${C3_FILE}"
else
    record "C3_virtual_surround_header_exists" "FAIL" "Missing: ${C3_FILE}"
fi

# ── C4: SteamAudioVirtualSurround::prepare() declared ───────────────────────
if grep -q 'void prepare' "${C3_FILE}" 2>/dev/null; then
    record "C4_prepare_declared" "PASS" "prepare() found in SteamAudioVirtualSurround.h"
else
    record "C4_prepare_declared" "FAIL" "prepare() not found in ${C3_FILE}"
fi

# ── C5: SteamAudioVirtualSurround::applyBlock() declared ────────────────────
if grep -q 'bool applyBlock' "${C3_FILE}" 2>/dev/null; then
    record "C5_apply_block_declared" "PASS" "applyBlock() found in SteamAudioVirtualSurround.h"
else
    record "C5_apply_block_declared" "FAIL" "applyBlock() not found in ${C3_FILE}"
fi

# ── C6: renderVirtualSurroundForMonitoring in SpatialRenderer.h ─────────────
C6_FILE="${REPO_ROOT}/Source/SpatialRenderer.h"
if grep -q 'renderVirtualSurroundForMonitoring' "${C6_FILE}" 2>/dev/null; then
    record "C6_render_monitoring_method" "PASS" "renderVirtualSurroundForMonitoring found in SpatialRenderer.h"
else
    record "C6_render_monitoring_method" "FAIL" "renderVirtualSurroundForMonitoring not found in ${C6_FILE}"
fi

# ── C7: monitoringInputPtrs_ / monitoringOutputPtrs_ private arrays ─────────
if grep -q 'monitoringInputPtrs_' "${C6_FILE}" 2>/dev/null \
    && grep -q 'monitoringOutputPtrs_' "${C6_FILE}" 2>/dev/null; then
    record "C7_monitoring_ptr_arrays" "PASS" "monitoringInputPtrs_ and monitoringOutputPtrs_ found in SpatialRenderer.h"
else
    record "C7_monitoring_ptr_arrays" "FAIL" "Monitoring pointer arrays missing from ${C6_FILE}"
fi

# ── C8: PluginProcessor includes SteamAudioVirtualSurround ──────────────────
PP_H="${REPO_ROOT}/Source/PluginProcessor.h"
if grep -q 'SteamAudioVirtualSurround.h' "${PP_H}" 2>/dev/null; then
    record "C8_pp_includes_vss" "PASS" "SteamAudioVirtualSurround.h included in PluginProcessor.h"
else
    record "C8_pp_includes_vss" "FAIL" "SteamAudioVirtualSurround.h not included in ${PP_H}"
fi

# ── C9: calMonitorVirtualSurround member declared ───────────────────────────
if grep -q 'calMonitorVirtualSurround' "${PP_H}" 2>/dev/null; then
    record "C9_cal_monitor_member" "PASS" "calMonitorVirtualSurround member found in PluginProcessor.h"
else
    record "C9_cal_monitor_member" "FAIL" "calMonitorVirtualSurround not found in ${PP_H}"
fi

# ── C10: applyCalibrationMonitoringPath declared ────────────────────────────
if grep -q 'applyCalibrationMonitoringPath' "${PP_H}" 2>/dev/null; then
    record "C10_monitoring_path_decl" "PASS" "applyCalibrationMonitoringPath declared in PluginProcessor.h"
else
    record "C10_monitoring_path_decl" "FAIL" "applyCalibrationMonitoringPath not found in ${PP_H}"
fi

# ── C11: PluginProcessor.cpp wires monitoring path in Calibrate mode ─────────
PP_CPP="${REPO_ROOT}/Source/PluginProcessor.cpp"
if grep -q 'applyCalibrationMonitoringPath' "${PP_CPP}" 2>/dev/null; then
    record "C11_pp_monitoring_wired" "PASS" "applyCalibrationMonitoringPath wired in PluginProcessor.cpp"
else
    record "C11_pp_monitoring_wired" "FAIL" "applyCalibrationMonitoringPath not wired in ${PP_CPP}"
fi

# ── C12: speakers path is no-op (kSpeakers returns early) ───────────────────
if grep -A 5 'kSpeakers' "${PP_CPP}" 2>/dev/null | grep -q 'return'; then
    record "C12_speakers_path_noop" "PASS" "kSpeakers path returns early (unchanged)"
else
    record "C12_speakers_path_noop" "FAIL" "speakers path noop not confirmed in ${PP_CPP}"
fi

# ── C13: RT-safety — no heap alloc in applyCalibrationMonitoringPath ─────────
# Heuristic: check that no 'new', 'malloc', 'vector(', 'resize(' appear in
# the applyCalibrationMonitoringPath function body in PluginProcessor.cpp.
ALLOC_IN_IMPL=$(awk '/applyCalibrationMonitoringPath.*juce::AudioBuffer/,/^}/' "${PP_CPP}" \
    | { grep -E '\bnew\b|\bmalloc\b|\bvector[[:space:]]*\(|\bresize[[:space:]]*\(' || true; } \
    | wc -l | tr -d '[:space:]')
if [[ "${ALLOC_IN_IMPL}" -eq 0 ]]; then
    record "C13_no_rt_allocation" "PASS" "No dynamic allocation detected in applyCalibrationMonitoringPath"
else
    record "C13_no_rt_allocation" "FAIL" "Possible dynamic allocation in applyCalibrationMonitoringPath (${ALLOC_IN_IMPL} hit(s))"
fi

# ── C14: calMonitorVirtualSurround.prepare() called in prepareToPlay ─────────
if grep -q 'calMonitorVirtualSurround.prepare' "${PP_CPP}" 2>/dev/null; then
    record "C14_prepare_called" "PASS" "calMonitorVirtualSurround.prepare() called in PluginProcessor.cpp"
else
    record "C14_prepare_called" "FAIL" "calMonitorVirtualSurround.prepare() not found in ${PP_CPP}"
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "Results: ${pass} passed, ${fail} failed"
echo "Evidence: ${STATUS_TSV}"

if [[ "${fail}" -gt 0 ]]; then
    exit 1
fi
exit 0
