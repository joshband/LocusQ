#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract-only"
RUNS=1
OUT_DIR="${ROOT_DIR}/TestEvidence/bl096_companion_runtime_reunification_${TIMESTAMP}"

usage() {
  cat <<EOF
Usage: qa-bl096-companion-runtime-reunification-mac.sh [options]

Options:
  --contract-only      Run static contract checks only (default)
  --execute            Run companion build/test lane
  --runs <count>       Number of execute runs (default: 1)
  --out <dir>          Output directory (default: ${OUT_DIR})
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only)
      MODE="contract-only"
      shift
      ;;
    --execute)
      MODE="execute"
      shift
      ;;
    --runs)
      RUNS="${2:-}"
      shift 2
      ;;
    --out)
      OUT_DIR="${2:-}"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check\tresult\tdetail\n" > "$STATUS_TSV"

record() {
  local check="$1"
  local result="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$check" "$result" "$detail" >> "$STATUS_TSV"
}

if [[ "$MODE" == "contract-only" ]]; then
  if rg -q 'PosePacketV1' "$ROOT_DIR/companion/Sources/LocusQHeadTrackingCompanion/main.swift"; then
    record "legacy_v1_encoder" "FAIL" "main.swift still carries legacy PosePacketV1"
  else
    record "legacy_v1_encoder" "PASS" "legacy PosePacketV1 removed from shipping executable path"
  fi

  if rg -Fq 'sample.posePacket(sequence:' "$ROOT_DIR/companion/Sources/LocusQHeadTrackingCompanion/main.swift" \
    && rg -Fq 'sample.posePacket(sequence:' "$ROOT_DIR/companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift"; then
    record "canonical_pose_packet_path" "PASS" "live, synthetic, and core runtime all serialize through MotionSample.posePacket"
  else
    record "canonical_pose_packet_path" "FAIL" "executable and core runtime are not fully routed through MotionSample.posePacket"
  fi

  if rg -Fq 'func posePacket(sequence:' "$ROOT_DIR/companion/Sources/LocusQHeadTrackerCore/PosePacket+MotionSample.swift"; then
    record "shared_packet_helper" "PASS" "shared MotionSample posePacket helper present in core package"
  else
    record "shared_packet_helper" "FAIL" "missing shared MotionSample posePacket helper"
  fi

  if rg -q 'testMotionSamplePosePacketUsesCanonicalFlagsAndRotationRateFields' "$ROOT_DIR/companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift" \
    && rg -q 'testMotionSamplePosePacketDefaultsSyntheticFlagsToZero' "$ROOT_DIR/companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift"; then
    record "regression_coverage" "PASS" "tests cover live-style flags and synthetic defaults"
  else
    record "regression_coverage" "FAIL" "missing executable-path regression coverage"
  fi
else
  fail_count=0
  for run in $(seq 1 "$RUNS"); do
    log_path="${OUT_DIR}/run_${run}.log"
    if (cd "$ROOT_DIR/companion" && swift test) >"$log_path" 2>&1; then
      record "swift_test_run_${run}" "PASS" "log=${log_path}"
    else
      record "swift_test_run_${run}" "FAIL" "log=${log_path}"
      fail_count=$((fail_count + 1))
      continue
    fi

    build_log="${OUT_DIR}/build_${run}.log"
    if (cd "$ROOT_DIR/companion" && swift build) >"$build_log" 2>&1; then
      record "swift_build_run_${run}" "PASS" "log=${build_log}"
    else
      record "swift_build_run_${run}" "FAIL" "log=${build_log}"
      fail_count=$((fail_count + 1))
    fi
  done

  if [[ "$fail_count" -eq 0 ]]; then
    record "lane_result" "PASS" "bl096 companion build/test passed runs=${RUNS}"
  else
    record "lane_result" "FAIL" "bl096 companion build/test failed runs=${RUNS} fail_count=${fail_count}"
  fi
fi

{
  echo "# BL-096 Companion Runtime Reunification QA"
  echo
  echo "- mode: \`${MODE}\`"
  echo "- runs: \`${RUNS}\`"
  echo "- status_tsv: \`${STATUS_TSV}\`"
} > "$SUMMARY_MD"

cat "$STATUS_TSV"
