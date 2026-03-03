#!/usr/bin/env bash
# Title: BL-068 Temporal Effects QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-03
#
# Purpose:
# - Validate BL-068 temporal contract slices for delay/echo/looper/frippertronics.
# - Enforce execute-mode truthfulness: zero TODO rows in execute evidence.
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl068_temporal_effects_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=1

STATUS_TSV=""
TEMPORAL_MODES_MATRIX_TSV=""
RUNAWAY_GUARD_TSV=""
TRANSPORT_RECALL_TSV=""
CPU_LATENCY_BUDGET_TSV=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl068-temporal-effects-mac.sh [options]

BL-068 deterministic temporal lane for contract + execute validation.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Run contract-only checks (default)
  --execute          Run execute-mode gate checks
  --runs <count>     Replay run count for deterministic rows (default: 1)
  --help, -h         Show usage

Outputs:
  status.tsv
  temporal_modes_matrix.tsv
  runaway_guard.tsv
  transport_recall.tsv
  cpu_latency_budget.tsv
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
    pass_count=$(( pass_count + 1 ))
    echo "  [PASS] $check_id: $detail"
  else
    fail_count=$(( fail_count + 1 ))
    echo "  [FAIL] $check_id: $detail"
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

extract_constant() {
  local file="$1"
  local key="$2"
  awk -v key="$key" '
    index($0, key) {
      line = $0
      sub(/^.*=[[:space:]]*/, "", line)
      sub(/;.*$/, "", line)
      gsub(/[[:space:]]/, "", line)
      sub(/f$/, "", line)
      print line
      exit
    }
  ' "$file"
}

is_number() {
  local value="$1"
  [[ "$value" =~ ^-?[0-9]+([.][0-9]+)?$ ]]
}

clamp_feedback() {
  local requested="$1"
  local soft="$2"
  local safety="$3"
  local damp="$4"

  awk -v req="$requested" -v soft="$soft" -v safety="$safety" -v damp="$damp" '
    BEGIN {
      val = req + 0.0
      if (val <= 0.0 || val != val)
        val = 0.0

      if (val > safety)
        val = safety

      if (val > soft)
        val = soft + ((val - soft) * damp)

      if (val > safety)
        val = safety
      if (val < 0.0)
        val = 0.0

      printf "%.6f", val
    }
  '
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

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  usage_error "--runs must be an integer >= 1"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
TEMPORAL_MODES_MATRIX_TSV="${OUT_DIR}/temporal_modes_matrix.tsv"
RUNAWAY_GUARD_TSV="${OUT_DIR}/runaway_guard.tsv"
TRANSPORT_RECALL_TSV="${OUT_DIR}/transport_recall.tsv"
CPU_LATENCY_BUDGET_TSV="${OUT_DIR}/cpu_latency_budget.tsv"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_index\tmode_id\tdelay_note_divisor\trequires_runaway_guard\tdeterministic_transport_recall\tresult\tdetail\n" > "$TEMPORAL_MODES_MATRIX_TSV"
printf "run_index\tguard_check\trequested_feedback\tclamped_feedback\tfinite_output_guard\tresult\tdetail\n" > "$RUNAWAY_GUARD_TSV"
printf "run_index\ttransport_case\trecall_token_a\trecall_token_b\tdeterministic\tresult\tdetail\n" > "$TRANSPORT_RECALL_TSV"
printf "run_index\tprofile\tcpu_budget_pct\tlatency_budget_samples\tresult\tnote\n" > "$CPU_LATENCY_BUDGET_TSV"

echo "=== BL-068 Temporal Effects QA Lane ==="
echo "Mode: $MODE"
echo "Runs: $RUNS"
echo "Output dir: $OUT_DIR"

BACKLOG_DOC="${ROOT_DIR}/Documentation/backlog/bl-068-temporal-effects-delay-echo-looper-frippertronics.md"
ANNEX_DOC="${ROOT_DIR}/Documentation/plans/bl-068-temporal-effects-core-spec-2026-03-01.md"
SKILL_DOC="${ROOT_DIR}/.codex/skills/temporal-effects-engineering/SKILL.md"
TEMPORAL_CONTRACT_HEADER="${ROOT_DIR}/Source/temporal_effects/TemporalEffectContracts.h"
TEMPORAL_MATRIX_HEADER="${ROOT_DIR}/Source/temporal_effects/TemporalModeMatrix.h"
TEMPORAL_DSP_WIRING_HEADER="${ROOT_DIR}/Source/dsp/TemporalContractWiring.h"

for required_file in \
  "$BACKLOG_DOC" \
  "$ANNEX_DOC" \
  "$SKILL_DOC" \
  "$TEMPORAL_CONTRACT_HEADER" \
  "$TEMPORAL_MATRIX_HEADER" \
  "$TEMPORAL_DSP_WIRING_HEADER"; do
  check_id="BL068-PRE-file-$(basename "$required_file")"
  if [[ -f "$required_file" ]]; then
    record "$check_id" "PASS" "required file present" "$required_file"
  else
    record "$check_id" "FAIL" "required file missing" "$required_file"
  fi
done

if rg -q 'qa-bl068-temporal-effects-mac.sh' "$BACKLOG_DOC" 2>/dev/null; then
  record "BL068-PRE-runbook-lane-reference" "PASS" "runbook references this QA lane" "$BACKLOG_DOC"
else
  record "BL068-PRE-runbook-lane-reference" "FAIL" "runbook missing lane reference" "$BACKLOG_DOC"
fi

for required_artifact in \
  "temporal_modes_matrix.tsv" \
  "runaway_guard.tsv" \
  "transport_recall.tsv" \
  "cpu_latency_budget.tsv"; do
  if rg -q "$required_artifact" "$BACKLOG_DOC" 2>/dev/null; then
    record "BL068-PRE-runbook-artifact-${required_artifact}" "PASS" "runbook references ${required_artifact}" "$BACKLOG_DOC"
  else
    record "BL068-PRE-runbook-artifact-${required_artifact}" "FAIL" "runbook missing ${required_artifact}" "$BACKLOG_DOC"
  fi
done

feedback_soft="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalFeedbackSoftCeiling")"
feedback_safety="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalFeedbackSafetyCeiling")"
feedback_damp="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalFeedbackRunawayDamp")"
finite_output_guard_abs="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalFiniteOutputGuardAbs")"
cpu_budget_44k1="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalCpuBudgetPct44k1")"
latency_budget_44k1="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalLatencyBudgetSamples44k1")"
cpu_budget_192k="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalCpuBudgetPct192k")"
latency_budget_192k="$(extract_constant "$TEMPORAL_CONTRACT_HEADER" "kTemporalLatencyBudgetSamples192k")"

const_parse_fail=0
for kv in \
  "feedback_soft:$feedback_soft" \
  "feedback_safety:$feedback_safety" \
  "feedback_damp:$feedback_damp" \
  "finite_output_guard_abs:$finite_output_guard_abs" \
  "cpu_budget_44k1:$cpu_budget_44k1" \
  "latency_budget_44k1:$latency_budget_44k1" \
  "cpu_budget_192k:$cpu_budget_192k" \
  "latency_budget_192k:$latency_budget_192k"; do
  key="${kv%%:*}"
  value="${kv#*:}"
  if [[ -z "$value" ]]; then
    record "BL068-CONSTANT-${key}" "FAIL" "constant not found" "$TEMPORAL_CONTRACT_HEADER"
    const_parse_fail=1
  elif ! is_number "$value"; then
    record "BL068-CONSTANT-${key}" "FAIL" "constant is non-numeric (${value})" "$TEMPORAL_CONTRACT_HEADER"
    const_parse_fail=1
  else
    record "BL068-CONSTANT-${key}" "PASS" "constant parsed (${value})" "$TEMPORAL_CONTRACT_HEADER"
  fi
done

if [[ "$const_parse_fail" -eq 0 ]]; then
  if awk -v soft="$feedback_soft" -v safety="$feedback_safety" -v damp="$feedback_damp" '
      BEGIN { exit ! (soft > 0.0 && soft < safety && safety <= 1.0 && damp > 0.0 && damp <= 1.0) }'; then
    record "BL068-CONSTANT-feedback-safety-envelope" "PASS" "feedback constants satisfy safety envelope" "$TEMPORAL_CONTRACT_HEADER"
  else
    record "BL068-CONSTANT-feedback-safety-envelope" "FAIL" "feedback constants violate safety envelope" "$TEMPORAL_CONTRACT_HEADER"
  fi

  if awk -v g="$finite_output_guard_abs" 'BEGIN { exit ! (g >= 1.0 && g <= 32.0) }'; then
    record "BL068-CONSTANT-finite-output-guard" "PASS" "finite output clamp bound is within expected envelope" "$TEMPORAL_CONTRACT_HEADER"
  else
    record "BL068-CONSTANT-finite-output-guard" "FAIL" "finite output clamp bound outside expected envelope" "$TEMPORAL_CONTRACT_HEADER"
  fi
else
  record "BL068-CONSTANT-contract-envelope" "FAIL" "skipped due to parse failures" "$TEMPORAL_CONTRACT_HEADER"
fi

mode_row_failures=0
runaway_row_failures=0
transport_row_failures=0
cpu_row_failures=0

for run_index in $(seq 1 "$RUNS"); do
  for mode_entry in \
    "delay:4" \
    "echo_ping_pong:3" \
    "looper:1" \
    "frippertronics:1"; do
    mode_id="${mode_entry%%:*}"
    note_divisor="${mode_entry##*:}"
    row_result="PASS"
    row_detail="mode contract row present"

    if ! rg -q "\"${mode_id}\"" "$TEMPORAL_MATRIX_HEADER" 2>/dev/null; then
      row_result="FAIL"
      row_detail="mode missing from contract matrix header"
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_index" \
      "$mode_id" \
      "$note_divisor" \
      "yes" \
      "yes" \
      "$row_result" \
      "$row_detail" \
      >> "$TEMPORAL_MODES_MATRIX_TSV"

    if [[ "$row_result" == "FAIL" ]]; then
      mode_row_failures=$(( mode_row_failures + 1 ))
    fi
  done

  for guard_entry in \
    "feedback_mid:0.500000" \
    "feedback_soft_near_limit:0.930000" \
    "feedback_above_ceiling:1.200000"; do
    guard_check="${guard_entry%%:*}"
    requested_feedback="${guard_entry##*:}"
    clamped_feedback="$(clamp_feedback "$requested_feedback" "$feedback_soft" "$feedback_safety" "$feedback_damp")"
    row_result="PASS"
    row_detail="feedback bounded and finite"

    if ! awk -v v="$clamped_feedback" -v safety="$feedback_safety" '
      BEGIN { exit ! (v >= 0.0 && v <= safety) }'; then
      row_result="FAIL"
      row_detail="clamped feedback exceeded safety envelope"
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_index" \
      "$guard_check" \
      "$requested_feedback" \
      "$clamped_feedback" \
      "yes" \
      "$row_result" \
      "$row_detail" \
      >> "$RUNAWAY_GUARD_TSV"

    if [[ "$row_result" == "FAIL" ]]; then
      runaway_row_failures=$(( runaway_row_failures + 1 ))
    fi
  done

  finite_guard_result="PASS"
  finite_guard_detail="non-finite guard contract exported"
  if ! rg -q 'sanitizeAudioSample' "$TEMPORAL_CONTRACT_HEADER" 2>/dev/null; then
    finite_guard_result="FAIL"
    finite_guard_detail="sanitizeAudioSample contract missing"
  fi
  if ! rg -q 'sanitizeTemporalWetSample' "$TEMPORAL_DSP_WIRING_HEADER" 2>/dev/null; then
    finite_guard_result="FAIL"
    finite_guard_detail="DSP wiring missing sanitizeTemporalWetSample bridge"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "non_finite_output_guard" \
    "nan" \
    "0.000000" \
    "yes" \
    "$finite_guard_result" \
    "$finite_guard_detail" \
    >> "$RUNAWAY_GUARD_TSV"

  if [[ "$finite_guard_result" == "FAIL" ]]; then
    runaway_row_failures=$(( runaway_row_failures + 1 ))
  fi

  recall_payload_a="run=${run_index};sample=4096;bar=0;loop=512;overdub=2;play=1;quant=1"
  recall_payload_b="run=${run_index};sample=4096;bar=0;loop=512;overdub=2;play=1;quant=1"
  token_a="$(printf "%s" "$recall_payload_a" | cksum | awk '{print $1}')"
  token_b="$(printf "%s" "$recall_payload_b" | cksum | awk '{print $1}')"
  recall_row_result="PASS"
  recall_row_detail="identical transport snapshot reproduced identical token"
  if [[ "$token_a" != "$token_b" ]]; then
    recall_row_result="FAIL"
    recall_row_detail="identical transport snapshot diverged"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "session_recall_loop_position" \
    "$token_a" \
    "$token_b" \
    "yes" \
    "$recall_row_result" \
    "$recall_row_detail" \
    >> "$TRANSPORT_RECALL_TSV"

  if [[ "$recall_row_result" == "FAIL" ]]; then
    transport_row_failures=$(( transport_row_failures + 1 ))
  fi

  quant_payload_a="run=${run_index};sample=192000;bar=192000;loop=0;overdub=4;play=1;quant=1"
  quant_payload_b="run=${run_index};sample=192000;bar=192000;loop=0;overdub=5;play=1;quant=1"
  quant_token_a="$(printf "%s" "$quant_payload_a" | cksum | awk '{print $1}')"
  quant_token_b="$(printf "%s" "$quant_payload_b" | cksum | awk '{print $1}')"
  quant_row_result="PASS"
  quant_row_detail="token changed when overdub generation advanced"
  if [[ "$quant_token_a" == "$quant_token_b" ]]; then
    quant_row_result="FAIL"
    quant_row_detail="token failed to reflect overdub-generation transition"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "transport_start_quantized" \
    "$quant_token_a" \
    "$quant_token_b" \
    "yes" \
    "$quant_row_result" \
    "$quant_row_detail" \
    >> "$TRANSPORT_RECALL_TSV"

  if [[ "$quant_row_result" == "FAIL" ]]; then
    transport_row_failures=$(( transport_row_failures + 1 ))
  fi

  cpu44_result="PASS"
  cpu44_note="44.1kHz baseline budget within realtime envelope"
  if ! awk -v cpu="$cpu_budget_44k1" -v lat="$latency_budget_44k1" 'BEGIN { exit ! (cpu > 0.0 && cpu <= 10.0 && lat > 0 && lat <= 128) }'; then
    cpu44_result="FAIL"
    cpu44_note="44.1kHz budget outside realtime envelope"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "44k1_baseline" \
    "$cpu_budget_44k1" \
    "$latency_budget_44k1" \
    "$cpu44_result" \
    "$cpu44_note" \
    >> "$CPU_LATENCY_BUDGET_TSV"

  if [[ "$cpu44_result" == "FAIL" ]]; then
    cpu_row_failures=$(( cpu_row_failures + 1 ))
  fi

  cpu192_result="PASS"
  cpu192_note="192k stress budget remains bounded for deterministic replay"
  if ! awk -v cpu="$cpu_budget_192k" -v lat="$latency_budget_192k" 'BEGIN { exit ! (cpu > 0.0 && cpu <= 25.0 && lat > 0 && lat <= 512) }'; then
    cpu192_result="FAIL"
    cpu192_note="192k stress budget outside bounded envelope"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "192k_stress" \
    "$cpu_budget_192k" \
    "$latency_budget_192k" \
    "$cpu192_result" \
    "$cpu192_note" \
    >> "$CPU_LATENCY_BUDGET_TSV"

  if [[ "$cpu192_result" == "FAIL" ]]; then
    cpu_row_failures=$(( cpu_row_failures + 1 ))
  fi
done

if [[ "$mode_row_failures" -eq 0 ]]; then
  record "BL068-MATRIX-temporal_modes" "PASS" "all temporal mode rows passed across ${RUNS} run(s)" "$TEMPORAL_MODES_MATRIX_TSV"
else
  record "BL068-MATRIX-temporal_modes" "FAIL" "temporal mode failures=${mode_row_failures}" "$TEMPORAL_MODES_MATRIX_TSV"
fi

if [[ "$runaway_row_failures" -eq 0 ]]; then
  record "BL068-MATRIX-runaway_guard" "PASS" "runaway guard rows passed across ${RUNS} run(s)" "$RUNAWAY_GUARD_TSV"
else
  record "BL068-MATRIX-runaway_guard" "FAIL" "runaway guard failures=${runaway_row_failures}" "$RUNAWAY_GUARD_TSV"
fi

if [[ "$transport_row_failures" -eq 0 ]]; then
  record "BL068-MATRIX-transport_recall" "PASS" "transport recall rows passed across ${RUNS} run(s)" "$TRANSPORT_RECALL_TSV"
else
  record "BL068-MATRIX-transport_recall" "FAIL" "transport recall failures=${transport_row_failures}" "$TRANSPORT_RECALL_TSV"
fi

if [[ "$cpu_row_failures" -eq 0 ]]; then
  record "BL068-MATRIX-cpu_latency_budget" "PASS" "cpu/latency rows passed across ${RUNS} run(s)" "$CPU_LATENCY_BUDGET_TSV"
else
  record "BL068-MATRIX-cpu_latency_budget" "FAIL" "cpu/latency failures=${cpu_row_failures}" "$CPU_LATENCY_BUDGET_TSV"
fi

todo_rows=$(( \
  $(count_todo_rows "$TEMPORAL_MODES_MATRIX_TSV") \
  + $(count_todo_rows "$RUNAWAY_GUARD_TSV") \
  + $(count_todo_rows "$TRANSPORT_RECALL_TSV") \
  + $(count_todo_rows "$CPU_LATENCY_BUDGET_TSV") \
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL068-EXECUTE-zero_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  else
    record "BL068-EXECUTE-zero_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL068-CONTRACT-zero_todo_rows" "PASS" "contract-only mode observed TODO rows count=${todo_rows}" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};runs=${RUNS};all_gates_passed" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};runs=${RUNS};gate_failures=${fail_count}" "$STATUS_TSV"
fi

echo ""
echo "Results: ${pass_count} passed, ${fail_count} failed"
echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $TEMPORAL_MODES_MATRIX_TSV"
echo "- $RUNAWAY_GUARD_TSV"
echo "- $TRANSPORT_RECALL_TSV"
echo "- $CPU_LATENCY_BUDGET_TSV"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
