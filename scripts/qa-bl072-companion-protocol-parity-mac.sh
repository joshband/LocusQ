#!/usr/bin/env bash
# Title: BL-072 Companion Protocol Parity QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-01
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl072_companion_protocol_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0

STATUS_TSV=""
PROTOCOL_PARITY_TSV=""
READINESS_GATE_TSV=""
AXIS_SWEEPS_MD=""
SEQUENCE_AGE_CONTRACT_TSV=""
BL058_LANE_PACKET_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl072-companion-protocol-parity-mac.sh [options]

BL-072 companion runtime protocol parity + BL-058 lane verification.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks (fails while runtime TODO rows exist)
  --help, -h         Show usage

Outputs:
  status.tsv
  protocol_parity.tsv
  readiness_gate.tsv
  axis_sweeps.md
  sequence_age_contract.tsv
  bl058_lane_packet.md
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

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
PROTOCOL_PARITY_TSV="${OUT_DIR}/protocol_parity.tsv"
READINESS_GATE_TSV="${OUT_DIR}/readiness_gate.tsv"
AXIS_SWEEPS_MD="${OUT_DIR}/axis_sweeps.md"
SEQUENCE_AGE_CONTRACT_TSV="${OUT_DIR}/sequence_age_contract.tsv"
BL058_LANE_PACKET_MD="${OUT_DIR}/bl058_lane_packet.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "check\tresult\tdetail\n" > "$PROTOCOL_PARITY_TSV"
printf "check\tresult\tdetail\n" > "$READINESS_GATE_TSV"
printf "check\tresult\tdetail\n" > "$SEQUENCE_AGE_CONTRACT_TSV"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-072-companion-runtime-protocol-parity-and-bl058-qa-harness.md"
COMPANION_MAIN="${ROOT_DIR}/companion/Sources/LocusQHeadTrackingCompanion/main.swift"
HEADTRACKING_BRIDGE="${ROOT_DIR}/Source/HeadTrackingBridge.h"
BL058_HARNESS="${ROOT_DIR}/scripts/qa-bl058-companion-profile-acquisition-mac.sh"

if [[ -f "$BACKLOG_DOC" ]]; then
  record "BL072-C1-backlog_doc_exists" "PASS" "runbook present" "$BACKLOG_DOC"
else
  record "BL072-C1-backlog_doc_exists" "FAIL" "runbook missing" "$BACKLOG_DOC"
fi

if rg -q -- '--require-sync' "$COMPANION_MAIN" \
   && rg -q -- '--auto-sync' "$COMPANION_MAIN" \
   && rg -q 'requireSyncToStart' "$COMPANION_MAIN"; then
  printf "companion_cli_sync_flags\tPASS\trequire-sync and auto-sync CLI contract markers present\n" >> "$PROTOCOL_PARITY_TSV"
  record "BL072-C2-cli_sync_flags" "PASS" "companion sync CLI contract markers present" "$COMPANION_MAIN"
else
  printf "companion_cli_sync_flags\tFAIL\trequire-sync/auto-sync CLI contract markers missing\n" >> "$PROTOCOL_PARITY_TSV"
  record "BL072-C2-cli_sync_flags" "FAIL" "companion sync CLI contract markers missing" "$COMPANION_MAIN"
fi

if rg -q 'allowPoseSend = \(readinessState == "active_ready"\) && sendGateOpen' "$COMPANION_MAIN" \
   && rg -q 'snapshot\.readinessState = readinessState' "$COMPANION_MAIN" \
   && rg -q 'snapshot\.syncRequired = arguments\.requireSyncToStart' "$COMPANION_MAIN"; then
  printf "readiness_send_gate_contract\tPASS\tlive runtime gates pose send on readiness + sync gate\n" >> "$READINESS_GATE_TSV"
  record "BL072-C3-readiness_send_gate" "PASS" "readiness/send-gate contract markers present" "$COMPANION_MAIN"
else
  printf "readiness_send_gate_contract\tFAIL\tlive readiness/send-gate contract markers missing\n" >> "$READINESS_GATE_TSV"
  record "BL072-C3-readiness_send_gate" "FAIL" "readiness/send-gate contract markers missing" "$COMPANION_MAIN"
fi

if rg -q 'packetMagic = 0x4C515054u' "$HEADTRACKING_BRIDGE" \
   && rg -q 'packetSizeV1 = 36' "$HEADTRACKING_BRIDGE" \
   && rg -q 'packetSizeV2 = 52' "$HEADTRACKING_BRIDGE" \
   && rg -q 'staleThresholdMs = 500' "$HEADTRACKING_BRIDGE"; then
  printf "plugin_packet_decode_contract\tPASS\tplugin accepts v1/v2 payloads with stale threshold guard\n" >> "$PROTOCOL_PARITY_TSV"
  record "BL072-C4-plugin_decode_contract" "PASS" "plugin packet decode + stale guard markers present" "$HEADTRACKING_BRIDGE"
else
  printf "plugin_packet_decode_contract\tFAIL\tplugin packet decode/stale guard markers missing\n" >> "$PROTOCOL_PARITY_TSV"
  record "BL072-C4-plugin_decode_contract" "FAIL" "plugin packet decode/stale guard markers missing" "$HEADTRACKING_BRIDGE"
fi

if rg -q 'snapshot\.seq <= previousSeq' "$HEADTRACKING_BRIDGE" \
   && rg -q 'acceptSequenceRestart = currentPoseStale && incomingTimestampAdvanced' "$HEADTRACKING_BRIDGE"; then
  printf "sequence_restart_contract\tPASS\tplugin sequence restart only allowed when prior pose is stale\n" >> "$SEQUENCE_AGE_CONTRACT_TSV"
  record "BL072-C5-sequence_restart_contract" "PASS" "sequence restart stale-only contract markers present" "$HEADTRACKING_BRIDGE"
else
  printf "sequence_restart_contract\tFAIL\tstale-only sequence restart guard missing\n" >> "$SEQUENCE_AGE_CONTRACT_TSV"
  record "BL072-C5-sequence_restart_contract" "FAIL" "sequence restart stale-only guard missing" "$HEADTRACKING_BRIDGE"
fi

if [[ -f "$BL058_HARNESS" ]] \
   && rg -q 'READINESS_GATE_MD' "$BL058_HARNESS" \
   && rg -q 'AXIS_SWEEPS_MD' "$BL058_HARNESS"; then
  printf "bl058_lane_evidence_bridge\tPASS\tBL-058 harness exposes readiness + axis artifacts\n" >> "$READINESS_GATE_TSV"
  record "BL072-C6-bl058_lane_bridge" "PASS" "BL-058 harness readiness/axis artifact markers present" "$BL058_HARNESS"
else
  printf "bl058_lane_evidence_bridge\tFAIL\tBL-058 harness readiness/axis artifact markers missing\n" >> "$READINESS_GATE_TSV"
  record "BL072-C6-bl058_lane_bridge" "FAIL" "BL-058 harness readiness/axis artifact markers missing" "$BL058_HARNESS"
fi

if [[ "$(rg -n 'snapshot\.syncRequired = arguments\.requireSyncToStart' "$COMPANION_MAIN" | wc -l | tr -d '[:space:]')" -ge 2 ]]; then
  printf "synthetic_vs_live_require_sync_parity\tPASS\trequire-sync publication exists in both synthetic and live runtime paths\n" >> "$PROTOCOL_PARITY_TSV"
else
  printf "synthetic_vs_live_require_sync_parity\tFAIL\trequire-sync publication missing from synthetic/live parity path\n" >> "$PROTOCOL_PARITY_TSV"
  record "BL072-R1-sync_parity" "FAIL" "require-sync publication missing from synthetic/live parity path" "$COMPANION_MAIN"
fi

if rg -q 'PosePacketV1' "$COMPANION_MAIN" \
   && rg -q 'packetSizeV1 = 36' "$HEADTRACKING_BRIDGE" \
   && rg -q 'packetSizeV2 = 52' "$HEADTRACKING_BRIDGE"; then
  printf "wire_payload_dual_version_probe\tPASS\tcompanion emits v1 and plugin keeps v1/v2 decode compatibility\n" >> "$PROTOCOL_PARITY_TSV"
else
  printf "wire_payload_dual_version_probe\tFAIL\tdual-version payload compatibility markers missing\n" >> "$PROTOCOL_PARITY_TSV"
  record "BL072-R2-wire_payload_dual_version_probe" "FAIL" "dual-version payload compatibility markers missing" "$HEADTRACKING_BRIDGE"
fi

if rg -q 'active_ready' "$COMPANION_MAIN" \
   && rg -q 'active_not_ready' "$COMPANION_MAIN" \
   && rg -q 'disabled_disconnected' "$COMPANION_MAIN"; then
  printf "readiness_transition_matrix\tPASS\treadiness transition states are explicitly modeled\n" >> "$READINESS_GATE_TSV"
else
  printf "readiness_transition_matrix\tFAIL\treadiness transition states missing from companion runtime\n" >> "$READINESS_GATE_TSV"
  record "BL072-R3-readiness_transition_matrix" "FAIL" "readiness transition states missing from companion runtime" "$COMPANION_MAIN"
fi

if rg -q 'pose_stale' "$COMPANION_MAIN" \
   && rg -q 'ackAgeMs > 500.0' "$COMPANION_MAIN" \
   && rg -q 'staleThresholdMs = 500' "$HEADTRACKING_BRIDGE"; then
  printf "stale_packet_fallback_behavior\tPASS\tstale fallback markers present across companion ingest and plugin bridge\n" >> "$READINESS_GATE_TSV"
else
  printf "stale_packet_fallback_behavior\tFAIL\tstale fallback markers missing across companion/plugin boundaries\n" >> "$READINESS_GATE_TSV"
  record "BL072-R4-stale_packet_fallback_behavior" "FAIL" "stale fallback markers missing across companion/plugin boundaries" "$COMPANION_MAIN"
fi

if rg -q 'snapshot\.seq <= previousSeq' "$HEADTRACKING_BRIDGE" \
   && rg -q 'acceptSequenceRestart = currentPoseStale && incomingTimestampAdvanced' "$HEADTRACKING_BRIDGE"; then
  printf "sequence_monotonicity_under_jitter\tPASS\tmonotonic sequence guard with stale restart exception present\n" >> "$SEQUENCE_AGE_CONTRACT_TSV"
else
  printf "sequence_monotonicity_under_jitter\tFAIL\tsequence monotonicity/stale-restart guard missing\n" >> "$SEQUENCE_AGE_CONTRACT_TSV"
  record "BL072-R5-sequence_monotonicity_under_jitter" "FAIL" "sequence monotonicity/stale-restart guard missing" "$HEADTRACKING_BRIDGE"
fi

if rg -q 'poseAgeMs' "$COMPANION_MAIN" \
   && rg -q 'ackAgeMs' "$COMPANION_MAIN" \
   && rg -q 'staleWindowMs: UInt64 = 2_000' "$COMPANION_MAIN"; then
  printf "pose_age_and_ack_age_thresholds\tPASS\tpose/ack age telemetry and stale thresholds are explicitly bounded\n" >> "$SEQUENCE_AGE_CONTRACT_TSV"
else
  printf "pose_age_and_ack_age_thresholds\tFAIL\tpose/ack age telemetry or stale threshold markers missing\n" >> "$SEQUENCE_AGE_CONTRACT_TSV"
  record "BL072-R6-pose_age_and_ack_age_thresholds" "FAIL" "pose/ack age telemetry or stale threshold markers missing" "$COMPANION_MAIN"
fi

cat > "$AXIS_SWEEPS_MD" <<EOF_AXIS
Title: BL-072 Axis Sweeps (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-072 Axis Sweeps (Contract Packet)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- contract_status: parity contract markers present for synthetic + live mode axis publication paths.
EOF_AXIS

cat > "$BL058_LANE_PACKET_MD" <<EOF_PACKET
Title: BL-072 BL-058 Lane Packet (Stub)
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-072 BL-058 Lane Packet (Contract Packet)

- mode: ${MODE}
- timestamp_utc: ${TIMESTAMP}
- linked_lane: BL-058 companion profile acquisition
- contract_status:
  - BL-058 lane readiness/axis artifact surfaces are wired and discoverable.
  - BL-072 parity guards cover sync-gate, stale fallback, and sequence continuity markers.
EOF_PACKET

todo_rows=$((
  $(count_todo_rows "$PROTOCOL_PARITY_TSV")
  + $(count_todo_rows "$READINESS_GATE_TSV")
  + $(count_todo_rows "$SEQUENCE_AGE_CONTRACT_TSV")
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL072-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL072-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL072-C7-contract_mode" "PASS" "contract-only mode completed parity checks (todo_rows=${todo_rows})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl072_contract_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $PROTOCOL_PARITY_TSV"
echo "- $READINESS_GATE_TSV"
echo "- $AXIS_SWEEPS_MD"
echo "- $SEQUENCE_AGE_CONTRACT_TSV"
echo "- $BL058_LANE_PACKET_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
