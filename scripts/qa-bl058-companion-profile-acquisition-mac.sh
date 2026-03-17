#!/usr/bin/env bash
# Title: BL-058 Companion Profile Acquisition QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-07
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl058_companion_profile_${TIMESTAMP}_$$"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
RESULTS_TSV=""
AXIS_SWEEPS_MD=""
READINESS_GATE_MD=""
CAPTURE_CONSUMER_BRIDGE_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl058-companion-profile-acquisition-mac.sh [options]

BL-058 companion profile acquisition lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks with companion selftest + synthetic sweep probes
  --help, -h         Show usage

Outputs:
  status.tsv
  results.tsv
  axis_sweeps.md
  readiness_gate.md
  capture_consumer_bridge.tsv
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

append_result_row() {
  local check_id="$1"
  local result="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" \
    "$check_id" \
    "$result" \
    "${detail//$'\t'/ }" \
    >> "$RESULTS_TSV"
}

axis_summary_line() {
  local label="$1"
  local artifact="$2"
  awk -F'\t' -v label="$label" -v artifact="$artifact" '
    function abs(v) { return v < 0 ? -v : v }
    NR == 1 { next }
    {
      yaw = abs($10) + 0
      pitch = abs($11) + 0
      roll = abs($12) + 0
      if (yaw > maxYaw) maxYaw = yaw
      if (pitch > maxPitch) maxPitch = pitch
      if (roll > maxRoll) maxRoll = roll
      rows++
    }
    END {
      printf "- %s: rows=%d max|yaw|=%.3f max|pitch|=%.3f max|roll|=%.3f (%s)\n",
        label, rows + 0, maxYaw + 0, maxPitch + 0, maxRoll + 0, artifact
    }
  ' "$artifact"
}

axis_check() {
  local artifact="$1"
  local primary_field="$2"
  local secondary_a_field="$3"
  local secondary_b_field="$4"
  local min_primary="$5"
  local max_secondary="$6"

  awk -F'\t' \
    -v primary_field="$primary_field" \
    -v secondary_a_field="$secondary_a_field" \
    -v secondary_b_field="$secondary_b_field" \
    -v min_primary="$min_primary" \
    -v max_secondary="$max_secondary" '
    function abs(v) { return v < 0 ? -v : v }
    NR == 1 { next }
    {
      primary = abs($(primary_field)) + 0
      secondaryA = abs($(secondary_a_field)) + 0
      secondaryB = abs($(secondary_b_field)) + 0
      if (primary > maxPrimary) maxPrimary = primary
      if (secondaryA > maxSecondaryA) maxSecondaryA = secondaryA
      if (secondaryB > maxSecondaryB) maxSecondaryB = secondaryB
      rows++
    }
    END {
      pass = rows > 0 && maxPrimary >= min_primary && maxSecondaryA <= max_secondary && maxSecondaryB <= max_secondary
      printf "rows=%d max_primary=%.3f max_secondary_a=%.3f max_secondary_b=%.3f",
        rows + 0, maxPrimary + 0, maxSecondaryA + 0, maxSecondaryB + 0
      exit(pass ? 0 : 1)
    }
  ' "$artifact"
}

readiness_check() {
  local artifact="$1"
  local expected_gate="$2"
  local expected_packets_relation="$3"

  awk -F'\t' \
    -v expected_gate="$expected_gate" \
    -v expected_packets_relation="$expected_packets_relation" '
    NR == 1 { next }
    {
      readiness = $4
      gate = $5
      packets = $3 + 0
    }
    END {
      pass = readiness == "active_ready" && gate == expected_gate
      if (expected_packets_relation == "zero")
        pass = pass && packets == 0
      else if (expected_packets_relation == "positive")
        pass = pass && packets > 0
      printf "readiness=%s gate=%s packets=%d", readiness, gate, packets
      exit(pass ? 0 : 1)
    }
  ' "$artifact"
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
command -v swift >/dev/null 2>&1 || usage_error "swift is required"
command -v awk >/dev/null 2>&1 || usage_error "awk is required"

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
RESULTS_TSV="${OUT_DIR}/results.tsv"
AXIS_SWEEPS_MD="${OUT_DIR}/axis_sweeps.md"
READINESS_GATE_MD="${OUT_DIR}/readiness_gate.md"
CAPTURE_CONSUMER_BRIDGE_TSV="${OUT_DIR}/capture_consumer_bridge.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$RESULTS_TSV"
printf "check\tresult\tdetail\tartifact\n" > "$CAPTURE_CONSUMER_BRIDGE_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-058-companion-profile-acquisition.md"
COMPANION_MAIN="${ROOT_DIR}/companion/Sources/LocusQHeadTrackingCompanion/main.swift"
MATCHER_CORE="${ROOT_DIR}/companion/Sources/LocusQHeadTrackerCore/EarPhotoMatcher.swift"
PROFILE_CORE="${ROOT_DIR}/companion/Sources/LocusQHeadTrackerCore/CalibrationProfile.swift"
BL077_CAPTURE_SCRIPT="${ROOT_DIR}/scripts/capture-headtracking-rotation-mac.sh"
COMPANION_DIR="${ROOT_DIR}/companion"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL058-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL058-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if [[ -f "$COMPANION_MAIN" ]]; then
  record "BL058-C2-companion_runtime_exists" "PASS" "companion runtime source present" "$COMPANION_MAIN"
else
  record "BL058-C2-companion_runtime_exists" "FAIL" "companion runtime source missing" "$COMPANION_MAIN"
fi

if [[ -f "$MATCHER_CORE" && -f "$PROFILE_CORE" ]]; then
  record "BL058-C2b-profile_core_exists" "PASS" "matcher and calibration profile core sources present" "$MATCHER_CORE"
else
  record "BL058-C2b-profile_core_exists" "FAIL" "matcher/profile core sources missing" "$MATCHER_CORE"
fi

if rg -q 'active_not_ready|active_ready|disabled_disconnected' "$BACKLOG_DOC"; then
  printf "readiness_state_contract\tPASS\tstate-machine identifiers present in runbook\n" >> "$RESULTS_TSV"
  record "BL058-C3-readiness_state_contract" "PASS" "runbook readiness contract present" "$BACKLOG_DOC"
else
  printf "readiness_state_contract\tFAIL\tstate-machine identifiers missing from runbook\n" >> "$RESULTS_TSV"
  record "BL058-C3-readiness_state_contract" "FAIL" "runbook readiness contract missing" "$BACKLOG_DOC"
fi

if [[ -x "$BL077_CAPTURE_SCRIPT" ]]; then
  record "BL058-C4-bl077_capture_script_exists" "PASS" "BL-077 capture harness script is executable" "$BL077_CAPTURE_SCRIPT"
else
  record "BL058-C4-bl077_capture_script_exists" "FAIL" "BL-077 capture harness script missing or not executable" "$BL077_CAPTURE_SCRIPT"
fi

if rg -q 'captureSelect|captureClear|applyProfile|profileAcquisition' "$COMPANION_MAIN"; then
  printf "profile_acquisition_ui_surface\tPASS\tcompanion runtime exposes capture/apply bridge controls\n" >> "$RESULTS_TSV"
  record "BL058-C4b-profile_acquisition_ui_surface" "PASS" "capture/apply controls present in companion monitor" "$COMPANION_MAIN"
else
  printf "profile_acquisition_ui_surface\tFAIL\tcapture/apply bridge controls missing from companion runtime\n" >> "$RESULTS_TSV"
  record "BL058-C4b-profile_acquisition_ui_surface" "FAIL" "capture/apply controls missing from companion monitor" "$COMPANION_MAIN"
fi

if rg -q 'matchEmbeddings|makeEmbedding|writeToDisk' "$MATCHER_CORE" "$PROFILE_CORE" "$COMPANION_MAIN"; then
  printf "local_match_and_profile_write\tPASS\tlocal matcher + profile write path markers present\n" >> "$RESULTS_TSV"
  record "BL058-C4c-local_match_and_profile_write" "PASS" "local matching/profile write path markers present" "$MATCHER_CORE"
else
  printf "local_match_and_profile_write\tFAIL\tlocal matcher/profile write path markers missing\n" >> "$RESULTS_TSV"
  record "BL058-C4c-local_match_and_profile_write" "FAIL" "local matching/profile write path markers missing" "$MATCHER_CORE"
fi

if rg -q 'https?://' "$ROOT_DIR/companion/Sources"; then
  printf "privacy_no_network_static\tFAIL\tunexpected network URL literal found in companion sources\n" >> "$RESULTS_TSV"
  record "BL058-C4d-privacy_no_network_static" "FAIL" "unexpected network URL literal found in companion sources" "$ROOT_DIR/companion/Sources"
else
  printf "privacy_no_network_static\tPASS\tno network URL literals found in companion sources\n" >> "$RESULTS_TSV"
  record "BL058-C4d-privacy_no_network_static" "PASS" "no network URL literals found in companion sources" "$ROOT_DIR/companion/Sources"
fi

BL077_CONSUMER_DIR="${OUT_DIR}/bl077_capture_contract"
BL077_CONSUMER_STDOUT="${BL077_CONSUMER_DIR}/stdout.log"
BL077_CONSUMER_STDERR="${BL077_CONSUMER_DIR}/stderr.log"
mkdir -p "$BL077_CONSUMER_DIR"

bl077_consumer_result="FAIL"
bl077_consumer_detail="capture harness contract probe failed"

if [[ -x "$BL077_CAPTURE_SCRIPT" ]]; then
  set +e
  "$BL077_CAPTURE_SCRIPT" \
    --out-dir "$BL077_CONSUMER_DIR" \
    --profile dense \
    --dry-run \
    --no-cues >"$BL077_CONSUMER_STDOUT" 2>"$BL077_CONSUMER_STDERR"
  bl077_capture_ec=$?
  set -e

  bl077_manifest="${BL077_CONSUMER_DIR}/session_manifest.json"
  bl077_inventory="${BL077_CONSUMER_DIR}/artifact_schema_inventory.tsv"
  bl077_hashes="${BL077_CONSUMER_DIR}/replay_hashes.tsv"
  bl077_checkpoint_map="${BL077_CONSUMER_DIR}/checkpoint_frame_map.tsv"

  if [[ "$bl077_capture_ec" -eq 0 && -f "$bl077_manifest" && -f "$bl077_inventory" && -f "$bl077_hashes" && -f "$bl077_checkpoint_map" ]]; then
    bl077_consumer_result="PASS"
    bl077_consumer_detail="BL-058 consumed BL-077 capture harness contract artifacts"
  else
    bl077_consumer_result="FAIL"
    bl077_consumer_detail="BL-077 probe exit=${bl077_capture_ec}; required artifacts missing"
  fi
else
  bl077_consumer_result="FAIL"
  bl077_consumer_detail="BL-077 capture harness script unavailable"
fi

printf "bl077_capture_contract_probe\t%s\t%s\t%s\n" \
  "$bl077_consumer_result" \
  "$bl077_consumer_detail" \
  "$BL077_CONSUMER_DIR" \
  >> "$CAPTURE_CONSUMER_BRIDGE_TSV"

if [[ "$bl077_consumer_result" == "PASS" ]]; then
  printf "bl077_capture_contract_probe\tPASS\tBL-058 invokes BL-077 harness in contract mode\n" >> "$RESULTS_TSV"
  record "BL058-C5-bl077_capture_consumer_contract" "PASS" "$bl077_consumer_detail" "$CAPTURE_CONSUMER_BRIDGE_TSV"
else
  printf "bl077_capture_contract_probe\tFAIL\tBL-058 failed to invoke BL-077 harness contract path\n" >> "$RESULTS_TSV"
  record "BL058-C5-bl077_capture_consumer_contract" "FAIL" "$bl077_consumer_detail" "$CAPTURE_CONSUMER_BRIDGE_TSV"
fi

printf "manual_capture_flow\tTODO\tcompanion manual runtime packet pending\n" >> "$RESULTS_TSV"
printf "embedding_latency_lt_50ms\tTODO\tperformance probe pending\n" >> "$RESULTS_TSV"
printf "privacy_no_network\tTODO\truntime privacy audit pending\n" >> "$RESULTS_TSV"

cat > "$AXIS_SWEEPS_MD" <<EOF_AXIS
Title: BL-058 Axis Sweeps (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-058 Axis Sweeps (Stub)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- pending: synthetic yaw/pitch/roll capture evidence.
EOF_AXIS

cat > "$READINESS_GATE_MD" <<EOF_READY
Title: BL-058 Readiness Gate (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-058 Readiness Gate (Stub)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- pending: runtime proof that send gate remains closed until explicit sync.
EOF_READY

todo_rows="$(count_todo_rows "$RESULTS_TSV")"
if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL058-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL058-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL058-C4-contract_mode" "PASS" "contract-only mode allows TODO execute rows (count=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl058_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $RESULTS_TSV"
echo "- $AXIS_SWEEPS_MD"
echo "- $READINESS_GATE_MD"
echo "- $CAPTURE_CONSUMER_BRIDGE_TSV"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
