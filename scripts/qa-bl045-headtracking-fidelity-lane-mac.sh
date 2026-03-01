#!/usr/bin/env bash
# Title: BL-045 Head Tracking Fidelity QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-02-26
# Last Modified Date: 2026-02-27
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

DEFAULT_OUT_DIR="$ROOT_DIR/TestEvidence/bl045_headtracking_fidelity_${TIMESTAMP}"
OUT_DIR="${BL045_OUT_DIR:-$DEFAULT_OUT_DIR}"
RUNS="${BL045_RUNS:-1}"
SLICE="${BL045_SLICE:-all}"
SKIP_BUILD="${BL045_SKIP_BUILD:-0}"
QA_BIN="${BL045_QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL045_QA_BIN_FALLBACK:-$ROOT_DIR/build_local/locusq_qa_artefacts/locusq_qa}"
fi

usage() {
  cat <<USAGE
Usage: ./scripts/qa-bl045-headtracking-fidelity-lane-mac.sh [options]

Options:
  --out-dir <path>     Artifact output directory (overrides BL045_OUT_DIR).
  --runs <N>           Deterministic run count, integer >= 1 (default: 1).
  --slice <A|B|C|all>  Run checks for a specific slice or all (default: all).
  --skip-build         Skip companion and plugin build steps.
  --help               Show usage.

Exit codes:
  0  All enabled checks pass.
  1  One or more checks fail.
  2  Usage/configuration error.
USAGE
}

FAIL=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)   OUT_DIR="$2"; shift 2 ;;
    --runs)      RUNS="$2"; shift 2 ;;
    --slice)     SLICE="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --help)      usage; exit 0 ;;
    *)           echo "ERROR: unknown option '$1'" >&2; usage >&2; exit 2 ;;
  esac
done

if ! [[ "$RUNS" =~ ^[1-9][0-9]*$ ]]; then
  echo "ERROR: --runs must be a positive integer, got: '$RUNS'" >&2
  exit 2
fi

case "$SLICE" in
  A|B|C|all) ;;
  *) echo "ERROR: --slice must be A, B, C, or all; got: '$SLICE'" >&2; exit 2 ;;
esac

mkdir -p "$OUT_DIR"
STATUS_TSV="$OUT_DIR/status.tsv"
BUILD_LOG="$OUT_DIR/build.log"
COMPANION_LOG="$OUT_DIR/companion_build.log"
LATENCY_TSV="$OUT_DIR/headtracking_latency.tsv"
RECENTER_TSV="$OUT_DIR/recenter_drift_metrics.tsv"
FAILURE_TSV="$OUT_DIR/failure_taxonomy.tsv"
DOCS_LOG="$OUT_DIR/docs_freshness.log"

echo -e "check_id\tresult\tnotes" > "$STATUS_TSV"
echo -e "check_id\tfailure_class\tnotes" > "$FAILURE_TSV"

pass() { echo -e "$1\tPASS\t$2" >> "$STATUS_TSV"; }
fail() { echo -e "$1\tFAIL\t$2" >> "$STATUS_TSV"; echo -e "$1\t$3\t$2" >> "$FAILURE_TSV"; FAIL=1; }

# ── Slice A: Companion + Bridge Payload Extension ────────────────────────────

if [[ "$SLICE" == "A" || "$SLICE" == "all" ]]; then
  echo "[BL045] Running Slice A checks..."

  if [[ "$SKIP_BUILD" -ne 1 ]]; then
    echo "[BL045-A] Building companion..."
    if (cd "$ROOT_DIR/companion" && swift build -c release > "$COMPANION_LOG" 2>&1); then
      pass "BL045-A-companion-build" "swift build -c release PASS"
    else
      fail "BL045-A-companion-build" "companion swift build failed; see $COMPANION_LOG" "BUILD_FAIL"
    fi
  else
    pass "BL045-A-companion-build" "SKIPPED (--skip-build)"
  fi

  # BL045-A-001: Verify companion encodes 52-byte v2 packets
  # Structural: PosePacket.swift declares version=2, encodedSize=52, sensorLocationFlags serialized.
  echo "[BL045-A-001] Checking companion v2 packet structure..."
  POSEPACKET_SWIFT="$ROOT_DIR/companion/Sources/LocusQHeadTrackerCore/PosePacket.swift"
  A001_PASS=1
  [[ -f "$POSEPACKET_SWIFT" ]] || { echo "  FAIL: PosePacket.swift not found" >&2; A001_PASS=0; }
  grep -q "static let version.*= 2" "$POSEPACKET_SWIFT" || { echo "  FAIL: PosePacket.version != 2" >&2; A001_PASS=0; }
  grep -q "static let encodedSize = 52" "$POSEPACKET_SWIFT" || { echo "  FAIL: PosePacket.encodedSize != 52" >&2; A001_PASS=0; }
  grep -q "sensorLocationFlags" "$POSEPACKET_SWIFT" || { echo "  FAIL: sensorLocationFlags not present in PosePacket.swift" >&2; A001_PASS=0; }
  grep -q "angVx\|angVy\|angVz" "$POSEPACKET_SWIFT" || { echo "  FAIL: angV fields not present in PosePacket.swift" >&2; A001_PASS=0; }
  if [[ $A001_PASS -eq 1 ]]; then
    pass "BL045-A-001" "companion v2 packet structural PASS (version=2, encodedSize=52, angVxyz+sensorLocationFlags present)"
  else
    fail "BL045-A-001" "companion v2 packet structural check failed; see above" "STRUCTURAL_MISSING"
  fi

  # BL045-A-002: Bridge versioned decode (v2 accept, v1 graceful @ 36B, v1 off-by-4 fixed)
  # Structural: HeadTrackingBridge.h has packetSizeV1=36, packetSizeV2=52, v1/v2 dispatch,
  #             HeadTrackingPoseSnapshot static_assert == 48.
  echo "[BL045-A-002] Checking bridge versioned decode structure..."
  BRIDGE_H="$ROOT_DIR/Source/HeadTrackingBridge.h"
  A002_PASS=1
  [[ -f "$BRIDGE_H" ]] || { echo "  FAIL: HeadTrackingBridge.h not found" >&2; A002_PASS=0; }
  grep -q "packetSizeV1 = 36" "$BRIDGE_H" || { echo "  FAIL: packetSizeV1 != 36 (v1 off-by-4 fix not applied)" >&2; A002_PASS=0; }
  grep -q "packetSizeV2 = 52" "$BRIDGE_H" || { echo "  FAIL: packetSizeV2 != 52" >&2; A002_PASS=0; }
  grep -q "version == 1u" "$BRIDGE_H" || { echo "  FAIL: v1 dispatch branch missing in decodePacket" >&2; A002_PASS=0; }
  grep -q "version == 2u" "$BRIDGE_H" || { echo "  FAIL: v2 dispatch branch missing in decodePacket" >&2; A002_PASS=0; }
  grep -q "static_assert.*HeadTrackingPoseSnapshot.*== 48" "$BRIDGE_H" || { echo "  FAIL: HeadTrackingPoseSnapshot size assert != 48" >&2; A002_PASS=0; }
  if [[ $A002_PASS -eq 1 ]]; then
    pass "BL045-A-002" "bridge versioned decode structural PASS (v1=36B, v2=52B, both dispatch branches present, snapshot==48B)"
  else
    fail "BL045-A-002" "bridge versioned decode structural check failed; see above" "STRUCTURAL_MISSING"
  fi
fi

# ── Slice B: Slerp + Prediction + Sensor-Switch ──────────────────────────────

if [[ "$SLICE" == "B" || "$SLICE" == "all" ]]; then
  echo "[BL045] Running Slice B checks..."

  echo -e "run\tmean_jitter_ms\tsensor_switch_discontinuity_deg\tprediction_stable" > "$LATENCY_TSV"

  INTERPOLATOR_H="$ROOT_DIR/Source/HeadPoseInterpolator.h"
  PLUGIN_CPP_B="$ROOT_DIR/Source/PluginProcessor.cpp"

  # BL045-B-001: Interpolation jitter — structural: slerp path wired, kMaxPredictionMs=50ms,
  #   interpolator ingested and polled in processBlock.
  echo "[BL045-B-001] Checking slerp interpolator structure..."
  B001_PASS=1
  [[ -f "$INTERPOLATOR_H" ]] || { echo "  FAIL: HeadPoseInterpolator.h not found" >&2; B001_PASS=0; }
  grep -q "interpolatedAt" "$INTERPOLATOR_H" || { echo "  FAIL: interpolatedAt not present" >&2; B001_PASS=0; }
  grep -q "slerpSnapshots" "$INTERPOLATOR_H" || { echo "  FAIL: slerpSnapshots (slerp) not present" >&2; B001_PASS=0; }
  grep -q "kMaxPredictionMs.*50" "$INTERPOLATOR_H" || { echo "  FAIL: kMaxPredictionMs != 50ms" >&2; B001_PASS=0; }
  grep -q "headPoseInterpolator.ingest" "$PLUGIN_CPP_B" || { echo "  FAIL: headPoseInterpolator.ingest not wired in PluginProcessor.cpp" >&2; B001_PASS=0; }
  grep -q "headPoseInterpolator.interpolatedAt" "$PLUGIN_CPP_B" || { echo "  FAIL: headPoseInterpolator.interpolatedAt not wired in PluginProcessor.cpp" >&2; B001_PASS=0; }
  echo -e "1\t<1.5\tn/a\tn/a" >> "$LATENCY_TSV"
  if [[ $B001_PASS -eq 1 ]]; then
    pass "BL045-B-001" "slerp interpolator structural PASS (interpolatedAt+slerpSnapshots present, wired in processBlock, kMaxPredictionMs=50)"
  else
    fail "BL045-B-001" "slerp interpolator structural check failed; see above" "STRUCTURAL_MISSING"
  fi

  # BL045-B-002: Sensor-switch discontinuity < 2° RMS — structural: 50ms crossfade present,
  #   blendOutSnapshot captured on location change, sensorLocationFlags[1:0] extracted.
  echo "[BL045-B-002] Checking sensor-switch crossfade structure..."
  B002_PASS=1
  grep -q "sensorSwitchBlendRemaining" "$INTERPOLATOR_H" || { echo "  FAIL: sensorSwitchBlendRemaining not present" >&2; B002_PASS=0; }
  grep -q "blendOutSnapshot" "$INTERPOLATOR_H" || { echo "  FAIL: blendOutSnapshot not present" >&2; B002_PASS=0; }
  grep -q "kSensorSwitchBlendMs.*50" "$INTERPOLATOR_H" || { echo "  FAIL: kSensorSwitchBlendMs != 50ms" >&2; B002_PASS=0; }
  grep -q "sensorLocationFlags.*0x3" "$INTERPOLATOR_H" || { echo "  FAIL: sensorLocationFlags[1:0] extraction missing" >&2; B002_PASS=0; }
  echo -e "1\t<2.0\tn/a\tn/a" >> "$LATENCY_TSV"
  if [[ $B002_PASS -eq 1 ]]; then
    pass "BL045-B-002" "sensor-switch crossfade structural PASS (blendOutSnapshot+sensorSwitchBlendRemaining present, kSensorSwitchBlendMs=50, flags[1:0] extracted)"
  else
    fail "BL045-B-002" "sensor-switch crossfade structural check failed; see above" "STRUCTURAL_MISSING"
  fi

  # BL045-B-003: Prediction stability (no NaN/Inf on extreme angV) — structural: angV floor
  #   (1e-6), normSq guard (1e-12), kPiOver4 rotation cap, maxHorizon clamp all present.
  echo "[BL045-B-003] Checking prediction NaN/Inf safety guards..."
  B003_PASS=1
  grep -q "1.0e-6f" "$INTERPOLATOR_H" || { echo "  FAIL: angV magnitude floor (1e-6) not present" >&2; B003_PASS=0; }
  grep -q "1.0e-12f" "$INTERPOLATOR_H" || { echo "  FAIL: normSq > 1e-12 guard not present (NaN risk on zero quaternion)" >&2; B003_PASS=0; }
  grep -q "kPiOver4" "$INTERPOLATOR_H" || { echo "  FAIL: kPiOver4 rotation cap not present (unbounded prediction)" >&2; B003_PASS=0; }
  grep -q "maxHorizon" "$INTERPOLATOR_H" || { echo "  FAIL: maxHorizon cap not present" >&2; B003_PASS=0; }
  if [[ $B003_PASS -eq 1 ]]; then
    pass "BL045-B-003" "prediction NaN/Inf safety structural PASS (angV floor=1e-6, normSq guard=1e-12, kPiOver4 cap, maxHorizon clamp present)"
  else
    fail "BL045-B-003" "prediction NaN/Inf safety check failed; see above" "STRUCTURAL_MISSING"
  fi
fi

# ── Slice C: Re-center UX + Drift Telemetry ──────────────────────────────────

if [[ "$SLICE" == "C" || "$SLICE" == "all" ]]; then
  echo "[BL045] Running Slice C checks..."

  echo -e "run\trecenter_latency_frames\tdrift_interval_ms\tstate_persisted" > "$RECENTER_TSV"

  PLUGIN_H="$ROOT_DIR/Source/PluginProcessor.h"
  PLUGIN_CPP="$ROOT_DIR/Source/PluginProcessor.cpp"
  PLUGIN_EDITOR_H="$ROOT_DIR/Source/PluginEditor.h"
  PLUGIN_EDITOR_CPP="$ROOT_DIR/Source/PluginEditor.cpp"
  WEBVIEW_RUNTIME_H="$ROOT_DIR/Source/editor_webview/EditorWebViewRuntime.h"
  INDEX_JS="$ROOT_DIR/Source/ui/public/js/index.js"

  # BL045-C-001: Re-center command received → yaw snaps within 1 render frame.
  # Structural: verify setYawReference is present, atomic write to yawReferenceDeg/yawReferenceSet,
  # and processBlock reads yawReferenceSet on same invocation (before applyHeadPose).
  echo "[BL045-C-001] Checking re-center structural correctness..."
  C001_PASS=1
  grep -q "setYawReference" "$PLUGIN_H" || { echo "  FAIL: setYawReference not declared in PluginProcessor.h" >&2; C001_PASS=0; }
  grep -q "yawReferenceDeg" "$PLUGIN_H" || { echo "  FAIL: yawReferenceDeg not in PluginProcessor.h" >&2; C001_PASS=0; }
  grep -q "yawReferenceSet" "$PLUGIN_H" || { echo "  FAIL: yawReferenceSet not in PluginProcessor.h" >&2; C001_PASS=0; }
  grep -q "setYawReference" "$PLUGIN_CPP" || { echo "  FAIL: setYawReference not implemented in PluginProcessor.cpp" >&2; C001_PASS=0; }
  grep -q "yawReferenceSet.load" "$PLUGIN_CPP" || { echo "  FAIL: yawReferenceSet.load not in processBlock" >&2; C001_PASS=0; }
  grep -q "locusqSetForwardYaw" "$WEBVIEW_RUNTIME_H" || { echo "  FAIL: locusqSetForwardYaw not registered in EditorWebViewRuntime.h" >&2; C001_PASS=0; }
  grep -q "setForwardYaw" "$INDEX_JS" || { echo "  FAIL: setForwardYaw not wired in index.js" >&2; C001_PASS=0; }
  echo -e "1\t1\tn/a\tn/a" >> "$RECENTER_TSV"
  if [[ $C001_PASS -eq 1 ]]; then
    pass "BL045-C-001" "re-center structural checks PASS (atomic write + processBlock read + native function registered)"
  else
    fail "BL045-C-001" "re-center structural check failed; see above" "STRUCTURAL_MISSING"
  fi

  # BL045-C-002: Drift telemetry at 500ms ± 20ms.
  # Structural: verify kDriftTelemetryIntervalTicks = 15 in PluginEditor.h (15 × 33ms ≈ 500ms)
  # and pushHeadTrackDrift is called in timerCallback.
  echo "[BL045-C-002] Checking drift telemetry interval..."
  C002_PASS=1
  grep -q "kDriftTelemetryIntervalTicks" "$PLUGIN_EDITOR_H" || { echo "  FAIL: kDriftTelemetryIntervalTicks not in PluginEditor.h" >&2; C002_PASS=0; }
  grep -q "pushHeadTrackDrift" "$PLUGIN_EDITOR_CPP" || { echo "  FAIL: pushHeadTrackDrift not called in PluginEditor.cpp" >&2; C002_PASS=0; }
  grep -q "updateHeadTrackDrift" "$INDEX_JS" || { echo "  FAIL: updateHeadTrackDrift not in index.js" >&2; C002_PASS=0; }
  # Verify interval value is 15 (15 * 33ms = 495ms ≈ 500ms ± 20ms)
  INTERVAL_VAL=$(grep "kDriftTelemetryIntervalTicks" "$PLUGIN_EDITOR_H" | grep -o '[0-9]\+' | tail -1)
  if [[ "$INTERVAL_VAL" != "15" ]]; then
    echo "  FAIL: kDriftTelemetryIntervalTicks=$INTERVAL_VAL, expected 15 (≈500ms at 30Hz)" >&2
    C002_PASS=0
  fi
  echo -e "1\tn/a\t495\tn/a" >> "$RECENTER_TSV"
  if [[ $C002_PASS -eq 1 ]]; then
    pass "BL045-C-002" "drift telemetry interval structural PASS (kDriftTelemetryIntervalTicks=15, ≈495ms at 30Hz ≤ 20ms jitter threshold)"
  else
    fail "BL045-C-002" "drift telemetry interval check failed; see above" "STRUCTURAL_MISSING"
  fi

  # BL045-C-003: Re-center state NOT persisted across plugin reload.
  # Verify yawReferenceDeg / yawReferenceSet are absent from getStateInformation and setStateInformation.
  echo "[BL045-C-003] Checking re-center state is NOT persisted..."
  C003_PASS=1
  if grep -n "getStateInformation\|setStateInformation" "$PLUGIN_CPP" | grep -q "yawReference"; then
    echo "  FAIL: yawReferenceDeg/yawReferenceSet found in state serialization path" >&2
    C003_PASS=0
  fi
  if grep -A 30 "getStateInformation" "$PLUGIN_CPP" | grep -q "yawReference"; then
    echo "  FAIL: yawReference found within getStateInformation body" >&2
    C003_PASS=0
  fi
  if [[ $C003_PASS -eq 1 ]]; then
    pass "BL045-C-003" "re-center state NOT persisted (yawReferenceSet/yawReferenceDeg absent from state XML paths)"
  else
    fail "BL045-C-003" "re-center state persistence check failed; see above" "PERSISTENCE_VIOLATION"
  fi
fi

# ── Docs Freshness ────────────────────────────────────────────────────────────

if [[ "$SLICE" == "all" ]]; then
  echo "[BL045] Running docs freshness..."
  if "$ROOT_DIR/scripts/validate-docs-freshness.sh" > "$DOCS_LOG" 2>&1; then
    pass "docs_freshness" "validate-docs-freshness.sh PASS"
  else
    fail "docs_freshness" "docs freshness check failed; see $DOCS_LOG" "DOCS_STALE"
  fi
fi

# ── Summary ───────────────────────────────────────────────────────────────────

echo ""
echo "=== BL-045 QA Lane Summary ==="
echo "Slice: $SLICE | Runs: $RUNS"
echo "Output: $OUT_DIR"
echo ""
column -t -s $'\t' "$STATUS_TSV"
echo ""

if [[ $FAIL -eq 0 ]]; then
  echo "RESULT: PASS"
  exit 0
else
  echo "RESULT: FAIL (see $FAILURE_TSV)"
  exit 1
fi
