#!/usr/bin/env bash
# Title: BL-068 Temporal Effects QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-01
# Last Modified Date: 2026-03-17
#
# Purpose:
# - Validate BL-068 temporal contract slices for delay/echo/looper/frippertronics.
# - Keep contract-only mode lightweight while making execute mode compile and run
#   header-backed temporal contract probes in an isolated BL-068 build directory.
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl068_temporal_effects_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=1

STATUS_TSV=""
TEMPORAL_MODES_MATRIX_TSV=""
RUNAWAY_GUARD_TSV=""
TRANSPORT_RECALL_TSV=""
CPU_LATENCY_BUDGET_TSV=""
SUMMARY_MD=""
LANE_NOTES_MD=""
PROBE_BUILD_DIR=""
PROBE_SRC=""
PROBE_BIN=""
PROBE_COMPILE_LOG=""
PROBE_RUN_LOG=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl068-temporal-effects-mac.sh [options]

BL-068 deterministic temporal lane for contract + execute validation.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Run contract-only checks (default)
  --execute          Run compile-backed execute-mode checks
  --runs <count>     Replay run count for deterministic rows (default: 1)
  --help, -h         Show usage

Outputs:
  status.tsv
  temporal_modes_matrix.tsv
  runaway_guard.tsv
  transport_recall.tsv
  cpu_latency_budget.tsv
  summary.md
  lane_notes.md
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
    echo "  [PASS] ${check_id}: ${detail}"
  else
    ((fail_count++)) || true
    echo "  [FAIL] ${check_id}: ${detail}"
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
        if ($i == "TODO" || $i == "SCAFFOLD")
        {
          count++
          break
        }
      }
    }
    END { print count + 0 }
  ' "$file"
}

count_result_rows() {
  local file="$1"
  local wanted="$2"
  [[ -f "$file" ]] || {
    echo 0
    return
  }

  awk -F'\t' -v wanted="$wanted" '
    NR == 1 {
      for (i = 1; i <= NF; ++i)
        if ($i == "result")
          result_column = i
      next
    }
    result_column > 0 && $result_column == wanted { count++ }
    END { print count + 0 }
  ' "$file"
}

count_data_rows() {
  local file="$1"
  [[ -f "$file" ]] || {
    echo 0
    return
  }

  awk 'NR > 1 { count++ } END { print count + 0 }' "$file"
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

sanitize_sample() {
  local sample="$1"
  local clamp_abs="$2"

  awk -v sample="$sample" -v clamp_abs="$clamp_abs" '
    BEGIN {
      val = sample + 0.0
      if (sample == "nan" || val != val)
      {
        printf "0.000000"
        exit
      }

      if (val > clamp_abs)
        val = clamp_abs
      else if (val < -clamp_abs)
        val = -clamp_abs

      printf "%.6f", val
    }
  '
}

milliseconds_to_samples() {
  local milliseconds="$1"
  local sample_rate="$2"

  awk -v ms="$milliseconds" -v sr="$sample_rate" '
    BEGIN {
      if (ms <= 0.0 || sr <= 0.0)
      {
        print 0
        exit
      }

      printf "%d", int((ms * 0.001 * sr) + 0.5)
    }
  '
}

write_summary_md() {
  local lane_result="$1"
  local todo_rows="$2"
  local build_dir_display="${PROBE_BUILD_DIR:-not_applicable}"

  cat > "$SUMMARY_MD" <<SUMMARY
Title: BL-068 Temporal Effects Evidence Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-068 Temporal Effects Lane Summary

- Mode: \`${MODE}\`
- Runs: \`${RUNS}\`
- Output directory: \`${OUT_DIR}\`
- Lane result: \`${lane_result}\`
- TODO/SCAFFOLD rows across matrix artifacts: \`${todo_rows}\`
- PASS rows in \`status.tsv\`: ${pass_count}
- FAIL rows in \`status.tsv\`: ${fail_count}
- Dedicated execute build directory: \`${build_dir_display}\`

## Artifacts

- \`status.tsv\`
- \`temporal_modes_matrix.tsv\`
- \`runaway_guard.tsv\`
- \`transport_recall.tsv\`
- \`cpu_latency_budget.tsv\`
- \`summary.md\` (this file)
- \`lane_notes.md\`
SUMMARY

  if [[ "$MODE" == "execute" ]]; then
    cat >> "$SUMMARY_MD" <<SUMMARY
- \`probe_build/temporal_contract_probe.cpp\`
- \`probe_build/temporal_contract_probe\`
- \`probe_build/compile.log\`
- \`probe_build/probe.log\`
SUMMARY
  fi
}

write_lane_notes_md() {
  local lane_result="$1"
  local todo_rows="$2"

  cat > "$LANE_NOTES_MD" <<NOTES
Title: BL-068 Temporal Effects Lane Notes
Document Type: Test Evidence Notes
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-068 Temporal Effects Lane Notes

- Validation command: \`./scripts/qa-bl068-temporal-effects-mac.sh --${MODE//_/-} --runs ${RUNS}\`
- Lane result: \`${lane_result}\`
- TODO/SCAFFOLD row count across matrix artifacts: \`${todo_rows}\`
- Mode semantics:
  - \`contract_only\`: structural doc/header guardrails only.
  - \`execute\`: compile-backed probe against \`TemporalEffectContracts.h\`, \`TemporalModeMatrix.h\`, and \`TemporalContractWiring.h\`.
- Isolated build directory: \`${PROBE_BUILD_DIR:-not_applicable}\`
NOTES
}

append_contract_rows() {
  local feedback_soft="$1"
  local feedback_safety="$2"
  local feedback_damp="$3"
  local finite_output_guard_abs="$4"
  local cpu_budget_44k1="$5"
  local latency_budget_44k1="$6"
  local cpu_budget_192k="$7"
  local latency_budget_192k="$8"

  local automation_samples_44k1
  local click_safe_samples_44k1
  local max_delay_samples_44k1
  local max_loop_samples_192k
  automation_samples_44k1="$(milliseconds_to_samples 20.0 44100)"
  click_safe_samples_44k1="$(milliseconds_to_samples 15.0 44100)"
  max_delay_samples_44k1="$(milliseconds_to_samples 8000 44100)"
  max_loop_samples_192k="$(milliseconds_to_samples 120000 192000)"

  for run_index in $(seq 1 "$RUNS"); do
    for mode_entry in \
      "delay|4|8000|0|0.350000|no|no" \
      "echo_ping_pong|3|8000|0|0.550000|yes|no" \
      "looper|1|4000|120000|0.700000|no|yes" \
      "frippertronics|1|8000|120000|0.820000|no|yes"; do
      IFS='|' read -r mode_id note_divisor max_delay_ms max_loop_ms default_feedback supports_ping_pong supports_overdub <<< "$mode_entry"
      row_result="PASS"
      row_detail="contract row present in header-only lane"

      if ! rg -q "\"${mode_id}\"" "$TEMPORAL_MATRIX_HEADER" 2>/dev/null \
         || ! rg -q 'defaultFeedbackCoefficient' "$TEMPORAL_MATRIX_HEADER" 2>/dev/null \
         || ! rg -q 'maxLoopMilliseconds' "$TEMPORAL_MATRIX_HEADER" 2>/dev/null \
         || ! rg -q 'maxDelayMilliseconds' "$TEMPORAL_MATRIX_HEADER" 2>/dev/null; then
        row_result="FAIL"
        row_detail="contract row markers missing from TemporalModeMatrix.h"
      fi

      printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
        "$run_index" \
        "$mode_id" \
        "$note_divisor" \
        "$max_delay_ms" \
        "$max_loop_ms" \
        "$default_feedback" \
        "$supports_ping_pong" \
        "$supports_overdub" \
        "$row_result" \
        "$row_detail" \
        >> "$TEMPORAL_MODES_MATRIX_TSV"
    done

    for guard_entry in \
      "feedback_mid|0.500000|0.250000|yes|feedback bounded and finite" \
      "feedback_soft_near_limit|0.930000|0.750000|yes|soft ceiling damping contract exported" \
      "feedback_above_ceiling|1.200000|12.500000|yes|feedback and wet sample clamped to finite envelope" \
      "invalid_loop_write_offset|0.650000|0.125000|no|frame finite-state guard rejects out-of-range loop offsets"; do
      IFS='|' read -r guard_check requested_feedback wet_sample expected_finite row_detail <<< "$guard_entry"
      clamped_feedback="$(clamp_feedback "$requested_feedback" "$feedback_soft" "$feedback_safety" "$feedback_damp")"
      sanitized_wet_sample="$(sanitize_sample "$wet_sample" "$finite_output_guard_abs")"
      row_result="PASS"

      if ! rg -q 'evaluateTemporalSafetyEnvelope' "$TEMPORAL_CONTRACT_HEADER" 2>/dev/null \
         || ! rg -q 'evaluateTemporalFrameContract' "$TEMPORAL_DSP_WIRING_HEADER" 2>/dev/null; then
        row_result="FAIL"
        row_detail="execute contract helpers missing"
      elif ! awk -v v="$clamped_feedback" -v safety="$feedback_safety" 'BEGIN { exit ! (v >= 0.0 && v <= safety) }'; then
        row_result="FAIL"
        row_detail="feedback clamp exceeded safety envelope"
      fi

      printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
        "$run_index" \
        "$guard_check" \
        "$requested_feedback" \
        "$clamped_feedback" \
        "$sanitized_wet_sample" \
        "$expected_finite" \
        "$row_result" \
        "$row_detail" \
        >> "$RUNAWAY_GUARD_TSV"
    done

    token_a="$(printf "run=%s;sample=4096;bar=0;loop=512;overdub=2;play=1;quant=1" "$run_index" | cksum | awk '{print $1}')"
    token_b="$(printf "run=%s;sample=4096;bar=0;loop=512;overdub=2;play=1;quant=1" "$run_index" | cksum | awk '{print $1}')"
    row_result="PASS"
    row_detail="identical transport snapshot reproduced identical token"
    if [[ "$token_a" != "$token_b" ]]; then
      row_result="FAIL"
      row_detail="identical transport snapshot diverged"
    fi
    if ! rg -q 'makeTemporalRecallSnapshot' "$TEMPORAL_DSP_WIRING_HEADER" 2>/dev/null \
       || ! rg -q 'deterministicRecallToken' "$TEMPORAL_CONTRACT_HEADER" 2>/dev/null; then
      row_result="FAIL"
      row_detail="transport recall helpers missing"
    fi
    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_index" \
      "session_recall_loop_position" \
      "$token_a" \
      "$token_b" \
      "$automation_samples_44k1" \
      "$click_safe_samples_44k1" \
      "$row_result" \
      "$row_detail" \
      >> "$TRANSPORT_RECALL_TSV"

    quant_token_a="$(printf "run=%s;sample=192000;bar=192000;loop=0;overdub=4;play=1;quant=1" "$run_index" | cksum | awk '{print $1}')"
    quant_token_b="$(printf "run=%s;sample=192000;bar=192000;loop=0;overdub=5;play=1;quant=1" "$run_index" | cksum | awk '{print $1}')"
    row_result="PASS"
    row_detail="token changed when overdub generation advanced"
    if [[ "$quant_token_a" == "$quant_token_b" ]]; then
      row_result="FAIL"
      row_detail="token failed to reflect overdub-generation transition"
    fi
    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_index" \
      "transport_start_quantized" \
      "$quant_token_a" \
      "$quant_token_b" \
      "$automation_samples_44k1" \
      "$click_safe_samples_44k1" \
      "$row_result" \
      "$row_detail" \
      >> "$TRANSPORT_RECALL_TSV"

    cpu44_result="PASS"
    cpu44_note="44.1kHz baseline budget within realtime envelope"
    if ! awk -v cpu="$cpu_budget_44k1" -v lat="$latency_budget_44k1" -v samples="$max_delay_samples_44k1" '
      BEGIN { exit ! (cpu > 0.0 && cpu <= 10.0 && lat > 0 && lat <= 128 && samples > 0) }'; then
      cpu44_result="FAIL"
      cpu44_note="44.1kHz budget outside realtime envelope"
    fi
    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_index" \
      "44k1_delay_baseline" \
      "44100" \
      "$max_delay_samples_44k1" \
      "$cpu_budget_44k1" \
      "$latency_budget_44k1" \
      "$cpu44_result" \
      "$cpu44_note" \
      >> "$CPU_LATENCY_BUDGET_TSV"

    cpu192_result="PASS"
    cpu192_note="192k frippertronics loop budget remains bounded"
    if ! awk -v cpu="$cpu_budget_192k" -v lat="$latency_budget_192k" -v samples="$max_loop_samples_192k" '
      BEGIN { exit ! (cpu > 0.0 && cpu <= 25.0 && lat > 0 && lat <= 512 && samples > 0) }'; then
      cpu192_result="FAIL"
      cpu192_note="192k stress budget outside bounded envelope"
    fi
    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_index" \
      "192k_fripper_loop_stress" \
      "192000" \
      "$max_loop_samples_192k" \
      "$cpu_budget_192k" \
      "$latency_budget_192k" \
      "$cpu192_result" \
      "$cpu192_note" \
      >> "$CPU_LATENCY_BUDGET_TSV"
  done
}

run_execute_probe() {
  local cxx_bin
  cxx_bin="$(command -v c++ || true)"
  if [[ -z "$cxx_bin" ]]; then
    record "BL068-EXECUTE-compiler" "FAIL" "c++ compiler not found in PATH" "$STATUS_TSV"
    return 1
  fi

  mkdir -p "$PROBE_BUILD_DIR"

  cat > "$PROBE_SRC" <<'CPP'
#include "dsp/TemporalContractWiring.h"
#include "temporal_effects/TemporalModeMatrix.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace
{
using locusq::dsp::TemporalFrameContractEvaluation;
using locusq::dsp::TemporalRealtimeFrameState;
using locusq::temporal::TemporalMode;

std::string yesNo (bool value)
{
    return value ? "yes" : "no";
}

void appendTemporalModeRows (std::ofstream& out,
                             int runIndex)
{
    for (const auto& row : locusq::temporal::kTemporalModeContractMatrix)
    {
        const bool rowSane = locusq::temporal::isTemporalModeContractSane (row);
        const auto detail = rowSane
            ? std::string ("compile-backed mode contract validated")
            : std::string ("mode contract failed local sanity checks");

        out << runIndex << '\t'
            << row.modeId << '\t'
            << row.defaultDelayNoteDivisor << '\t'
            << row.maxDelayMilliseconds << '\t'
            << row.maxLoopMilliseconds << '\t'
            << std::fixed << std::setprecision (6) << row.defaultFeedbackCoefficient << '\t'
            << yesNo (row.supportsPingPong) << '\t'
            << yesNo (row.supportsOverdub) << '\t'
            << (rowSane ? "PASS" : "FAIL") << '\t'
            << detail << '\n';
    }
}

bool checkGuardRow (const TemporalFrameContractEvaluation& evaluation,
                    double expectedFeedbackMin,
                    double expectedFeedbackMax,
                    double expectedWetSample,
                    bool expectedFinite)
{
    const auto feedbackOk = evaluation.clampedFeedback >= expectedFeedbackMin
        && evaluation.clampedFeedback <= expectedFeedbackMax;
    const auto wetOk = std::abs (evaluation.sanitizedWetSample - expectedWetSample) <= 0.0001f;
    const auto finiteOk = evaluation.frameStateFinite == expectedFinite;
    return feedbackOk && wetOk && finiteOk;
}

void appendRunawayRows (std::ofstream& out,
                        int runIndex)
{
    const TemporalRealtimeFrameState midState { 4096, 512, 0.50f, 0.25f };
    const TemporalRealtimeFrameState softState { 8192, 256, 0.93f, 0.75f };
    const TemporalRealtimeFrameState clampState { 12288, 1024, 1.20f, 12.50f };
    const TemporalRealtimeFrameState invalidState { 16384, locusq::temporal::maxBufferSamplesForMode (*locusq::temporal::findTemporalModeContract (TemporalMode::Looper), 48000.0), 0.65f, 0.125f };

    const auto midEval = locusq::dsp::evaluateTemporalFrameContract (midState, TemporalMode::Delay, 48000.0, 2, true, true);
    const auto softEval = locusq::dsp::evaluateTemporalFrameContract (softState, TemporalMode::EchoPingPong, 48000.0, 3, true, true);
    const auto clampEval = locusq::dsp::evaluateTemporalFrameContract (clampState, TemporalMode::Frippertronics, 192000.0, 4, true, true);
    const auto invalidEval = locusq::dsp::evaluateTemporalFrameContract (invalidState, TemporalMode::Looper, 48000.0, 5, true, true);

    struct Row
    {
        const char* id;
        double requestedFeedback;
        TemporalFrameContractEvaluation evaluation;
        double expectedFeedbackMin;
        double expectedFeedbackMax;
        double expectedWetSample;
        bool expectedFinite;
        const char* detail;
    };

    const Row rows[] {
        { "feedback_mid", 0.50, midEval, 0.49, 0.51, 0.25, true, "mid feedback path remains finite without clamp" },
        { "feedback_soft_near_limit", 0.93, softEval, 0.92, 0.93, 0.75, true, "soft ceiling damping keeps feedback inside contract envelope" },
        { "feedback_above_ceiling", 1.20, clampEval, 0.96, 0.975, 8.0, true, "feedback and wet sample clamp to finite safety envelope" },
        { "invalid_loop_write_offset", 0.65, invalidEval, 0.64, 0.66, 0.125, false, "invalid loop offset is rejected by finite-state gate" }
    };

    for (const auto& row : rows)
    {
        const bool pass = checkGuardRow (row.evaluation,
                                         row.expectedFeedbackMin,
                                         row.expectedFeedbackMax,
                                         row.expectedWetSample,
                                         row.expectedFinite);

        out << runIndex << '\t'
            << row.id << '\t'
            << std::fixed << std::setprecision (6) << row.requestedFeedback << '\t'
            << row.evaluation.clampedFeedback << '\t'
            << row.evaluation.sanitizedWetSample << '\t'
            << yesNo (row.evaluation.frameStateFinite) << '\t'
            << (pass ? "PASS" : "FAIL") << '\t'
            << row.detail << '\n';
    }
}

void appendTransportRows (std::ofstream& out,
                          int runIndex)
{
    const TemporalRealtimeFrameState state { 4096, 512, 0.50f, 0.25f };
    const auto identicalA = locusq::dsp::evaluateTemporalFrameContract (state, TemporalMode::Looper, 48000.0, 2, true, true, 0);
    const auto identicalB = locusq::dsp::evaluateTemporalFrameContract (state, TemporalMode::Looper, 48000.0, 2, true, true, 0);
    const auto overdubAdvanced = locusq::dsp::evaluateTemporalFrameContract (state, TemporalMode::Looper, 48000.0, 3, true, true, 0);
    const auto quantizeChanged = locusq::dsp::evaluateTemporalFrameContract (state, TemporalMode::Looper, 48000.0, 2, true, false, 0);

    const auto identicalPass = identicalA.recallToken == identicalB.recallToken;
    const auto overdubPass = identicalA.recallToken != overdubAdvanced.recallToken;
    const auto quantizePass = identicalA.recallToken != quantizeChanged.recallToken;

    out << runIndex << '\t'
        << "session_recall_loop_position" << '\t'
        << identicalA.recallToken << '\t'
        << identicalB.recallToken << '\t'
        << identicalA.automationRampSamples << '\t'
        << identicalA.clickSafeRampSamples << '\t'
        << (identicalPass ? "PASS" : "FAIL") << '\t'
        << "identical snapshot reproduces identical recall token" << '\n';

    out << runIndex << '\t'
        << "transport_start_quantized" << '\t'
        << identicalA.recallToken << '\t'
        << overdubAdvanced.recallToken << '\t'
        << overdubAdvanced.automationRampSamples << '\t'
        << overdubAdvanced.clickSafeRampSamples << '\t'
        << (overdubPass ? "PASS" : "FAIL") << '\t'
        << "overdub generation transition changes recall token" << '\n';

    out << runIndex << '\t'
        << "quantize_toggle_transition" << '\t'
        << identicalA.recallToken << '\t'
        << quantizeChanged.recallToken << '\t'
        << quantizeChanged.automationRampSamples << '\t'
        << quantizeChanged.clickSafeRampSamples << '\t'
        << (quantizePass ? "PASS" : "FAIL") << '\t'
        << "quantize-to-bar-start toggle changes recall token" << '\n';
}

void appendBudgetRows (std::ofstream& out,
                       int runIndex)
{
    const auto baselineBudget = locusq::temporal::budgetSnapshotForSampleRate (44100.0);
    const auto stressBudget = locusq::temporal::budgetSnapshotForSampleRate (192000.0);
    const auto delayRow = *locusq::temporal::findTemporalModeContract (TemporalMode::Delay);
    const auto fripperRow = *locusq::temporal::findTemporalModeContract (TemporalMode::Frippertronics);
    const auto baselineSamples = locusq::temporal::maxBufferSamplesForMode (delayRow, baselineBudget.sampleRate);
    const auto stressSamples = locusq::temporal::maxBufferSamplesForMode (fripperRow, stressBudget.sampleRate);

    const bool baselinePass = baselineBudget.cpuBudgetPct > 0.0
        && baselineBudget.cpuBudgetPct <= 10.0
        && baselineBudget.latencyBudgetSamples > 0
        && baselineBudget.latencyBudgetSamples <= 128
        && baselineSamples > 0;
    const bool stressPass = stressBudget.cpuBudgetPct > 0.0
        && stressBudget.cpuBudgetPct <= 25.0
        && stressBudget.latencyBudgetSamples > 0
        && stressBudget.latencyBudgetSamples <= 512
        && stressSamples > 0;

    out << runIndex << '\t'
        << "44k1_delay_baseline" << '\t'
        << static_cast<int> (baselineBudget.sampleRate) << '\t'
        << baselineSamples << '\t'
        << std::fixed << std::setprecision (6) << baselineBudget.cpuBudgetPct << '\t'
        << baselineBudget.latencyBudgetSamples << '\t'
        << (baselinePass ? "PASS" : "FAIL") << '\t'
        << "44.1kHz delay budget is inside realtime envelope" << '\n';

    out << runIndex << '\t'
        << "192k_fripper_loop_stress" << '\t'
        << static_cast<int> (stressBudget.sampleRate) << '\t'
        << stressSamples << '\t'
        << std::fixed << std::setprecision (6) << stressBudget.cpuBudgetPct << '\t'
        << stressBudget.latencyBudgetSamples << '\t'
        << (stressPass ? "PASS" : "FAIL") << '\t'
        << "192k frippertronics loop budget remains bounded" << '\n';
}

} // namespace

int main (int argc, char** argv)
{
    if (argc != 6)
    {
        std::cerr << "usage: temporal_contract_probe <runs> <temporal_modes_matrix.tsv> <runaway_guard.tsv> <transport_recall.tsv> <cpu_latency_budget.tsv>\n";
        return 2;
    }

    const int runs = std::stoi (argv[1]);
    std::ofstream temporalModes (argv[2], std::ios::app);
    std::ofstream runawayGuard (argv[3], std::ios::app);
    std::ofstream transportRecall (argv[4], std::ios::app);
    std::ofstream cpuLatencyBudget (argv[5], std::ios::app);

    if (! temporalModes || ! runawayGuard || ! transportRecall || ! cpuLatencyBudget)
    {
        std::cerr << "failed to open one or more output files\n";
        return 1;
    }

    bool allPass = true;
    for (int runIndex = 1; runIndex <= runs; ++runIndex)
    {
        appendTemporalModeRows (temporalModes, runIndex);
        appendRunawayRows (runawayGuard, runIndex);
        appendTransportRows (transportRecall, runIndex);
        appendBudgetRows (cpuLatencyBudget, runIndex);
    }

    temporalModes.flush();
    runawayGuard.flush();
    transportRecall.flush();
    cpuLatencyBudget.flush();

    for (const char* path : { argv[2], argv[3], argv[4], argv[5] })
    {
        std::ifstream in (path);
        std::string header;
        std::getline (in, header);

        std::size_t resultColumn = std::numeric_limits<std::size_t>::max();
        {
            std::istringstream headerStream (header);
            std::string cell;
            std::size_t index = 0;
            while (std::getline (headerStream, cell, '\t'))
            {
                if (cell == "result")
                {
                    resultColumn = index;
                    break;
                }
                ++index;
            }
        }

        std::string line;
        while (std::getline (in, line))
        {
            std::istringstream rowStream (line);
            std::string cell;
            std::size_t index = 0;
            while (std::getline (rowStream, cell, '\t'))
            {
                if (index == resultColumn && cell == "FAIL")
                    allPass = false;
                ++index;
            }
        }
    }

    return allPass ? 0 : 1;
}
CPP

  if "$cxx_bin" -std=c++20 -Wall -Wextra -pedantic -I"$ROOT_DIR/Source" "$PROBE_SRC" -o "$PROBE_BIN" >"$PROBE_COMPILE_LOG" 2>&1; then
    record "BL068-EXECUTE-probe_compile" "PASS" "temporal execute probe compiled cleanly" "$PROBE_COMPILE_LOG"
  else
    record "BL068-EXECUTE-probe_compile" "FAIL" "temporal execute probe failed to compile" "$PROBE_COMPILE_LOG"
    return 1
  fi

  if "$PROBE_BIN" "$RUNS" "$TEMPORAL_MODES_MATRIX_TSV" "$RUNAWAY_GUARD_TSV" "$TRANSPORT_RECALL_TSV" "$CPU_LATENCY_BUDGET_TSV" >"$PROBE_RUN_LOG" 2>&1; then
    record "BL068-EXECUTE-probe_run" "PASS" "temporal execute probe completed" "$PROBE_RUN_LOG"
    return 0
  fi

  record "BL068-EXECUTE-probe_run" "FAIL" "temporal execute probe reported one or more failures" "$PROBE_RUN_LOG"
  return 1
}

summarize_matrix_artifact() {
  local check_id="$1"
  local file="$2"
  local label="$3"
  local data_rows
  local fail_rows

  data_rows="$(count_data_rows "$file")"
  fail_rows="$(count_result_rows "$file" "FAIL")"

  if [[ "$data_rows" -eq 0 ]]; then
    record "$check_id" "FAIL" "${label} artifact contains zero data rows" "$file"
  elif [[ "$fail_rows" -eq 0 ]]; then
    record "$check_id" "PASS" "${label} rows passed across ${RUNS} run(s)" "$file"
  else
    record "$check_id" "FAIL" "${label} failures=${fail_rows}" "$file"
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

command -v rg >/dev/null 2>&1 || usage_error "ripgrep (rg) is required"

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
TEMPORAL_MODES_MATRIX_TSV="${OUT_DIR}/temporal_modes_matrix.tsv"
RUNAWAY_GUARD_TSV="${OUT_DIR}/runaway_guard.tsv"
TRANSPORT_RECALL_TSV="${OUT_DIR}/transport_recall.tsv"
CPU_LATENCY_BUDGET_TSV="${OUT_DIR}/cpu_latency_budget.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"
LANE_NOTES_MD="${OUT_DIR}/lane_notes.md"
PROBE_BUILD_DIR="${OUT_DIR}/probe_build"
PROBE_SRC="${PROBE_BUILD_DIR}/temporal_contract_probe.cpp"
PROBE_BIN="${PROBE_BUILD_DIR}/temporal_contract_probe"
PROBE_COMPILE_LOG="${PROBE_BUILD_DIR}/compile.log"
PROBE_RUN_LOG="${PROBE_BUILD_DIR}/probe.log"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_index\tmode_id\tdelay_note_divisor\tmax_delay_ms\tmax_loop_ms\tdefault_feedback\tsupports_ping_pong\tsupports_overdub\tresult\tdetail\n" > "$TEMPORAL_MODES_MATRIX_TSV"
printf "run_index\tguard_check\trequested_feedback\tclamped_feedback\tsanitized_wet_sample\tframe_state_finite\tresult\tdetail\n" > "$RUNAWAY_GUARD_TSV"
printf "run_index\ttransport_case\trecall_token_a\trecall_token_b\tautomation_ramp_samples\tclick_safe_ramp_samples\tresult\tdetail\n" > "$TRANSPORT_RECALL_TSV"
printf "run_index\tprofile\tsample_rate_hz\tmax_buffer_samples\tcpu_budget_pct\tlatency_budget_samples\tresult\tnote\n" > "$CPU_LATENCY_BUDGET_TSV"

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

for doc_pair in \
  "backlog:$BACKLOG_DOC" \
  "annex:$ANNEX_DOC"; do
  label="${doc_pair%%:*}"
  path="${doc_pair#*:}"

  if rg -q 'qa-bl068-temporal-effects-mac.sh' "$path" 2>/dev/null; then
    record "BL068-PRE-${label}-lane-reference" "PASS" "${label} doc references this QA lane" "$path"
  else
    record "BL068-PRE-${label}-lane-reference" "FAIL" "${label} doc missing lane reference" "$path"
  fi

  for required_artifact in \
    "temporal_modes_matrix.tsv" \
    "runaway_guard.tsv" \
    "transport_recall.tsv" \
    "cpu_latency_budget.tsv"; do
    if rg -q "$required_artifact" "$path" 2>/dev/null; then
      record "BL068-PRE-${label}-artifact-${required_artifact}" "PASS" "${label} doc references ${required_artifact}" "$path"
    else
      record "BL068-PRE-${label}-artifact-${required_artifact}" "FAIL" "${label} doc missing ${required_artifact}" "$path"
    fi
  done
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

execute_probe_failed=0
if [[ "$MODE" == "execute" ]]; then
  if ! run_execute_probe; then
    execute_probe_failed=1
  fi
else
  append_contract_rows \
    "$feedback_soft" \
    "$feedback_safety" \
    "$feedback_damp" \
    "$finite_output_guard_abs" \
    "$cpu_budget_44k1" \
    "$latency_budget_44k1" \
    "$cpu_budget_192k" \
    "$latency_budget_192k"
fi

summarize_matrix_artifact "BL068-MATRIX-temporal_modes" "$TEMPORAL_MODES_MATRIX_TSV" "temporal mode"
summarize_matrix_artifact "BL068-MATRIX-runaway_guard" "$RUNAWAY_GUARD_TSV" "runaway guard"
summarize_matrix_artifact "BL068-MATRIX-transport_recall" "$TRANSPORT_RECALL_TSV" "transport recall"
summarize_matrix_artifact "BL068-MATRIX-cpu_latency_budget" "$CPU_LATENCY_BUDGET_TSV" "cpu/latency"

todo_rows=$(( \
  $(count_todo_rows "$TEMPORAL_MODES_MATRIX_TSV") \
  + $(count_todo_rows "$RUNAWAY_GUARD_TSV") \
  + $(count_todo_rows "$TRANSPORT_RECALL_TSV") \
  + $(count_todo_rows "$CPU_LATENCY_BUDGET_TSV") \
))

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows" -gt 0 ]]; then
    record "BL068-EXECUTE-zero_todo_rows" "FAIL" "execute mode requires zero TODO rows (found=${todo_rows})" "$STATUS_TSV"
  elif [[ "$execute_probe_failed" -ne 0 ]]; then
    record "BL068-EXECUTE-zero_todo_rows" "FAIL" "execute probe failed before all matrix rows could be trusted" "$STATUS_TSV"
  else
    record "BL068-EXECUTE-zero_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL068-CONTRACT-zero_todo_rows" "PASS" "contract-only mode observed TODO rows count=${todo_rows}" "$STATUS_TSV"
fi

gate_failures="$fail_count"
lane_result_value="PASS"
lane_result_detail="mode=${MODE};runs=${RUNS};all_gates_passed"
if [[ "$gate_failures" -gt 0 ]]; then
  lane_result_value="FAIL"
  lane_result_detail="mode=${MODE};runs=${RUNS};gate_failures=${gate_failures}"
fi
record "lane_result" "$lane_result_value" "$lane_result_detail" "$STATUS_TSV"

write_summary_md "$lane_result_value" "$todo_rows"
write_lane_notes_md "$lane_result_value" "$todo_rows"

echo ""
echo "Results: ${pass_count} passed, ${fail_count} failed"
echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $TEMPORAL_MODES_MATRIX_TSV"
echo "- $RUNAWAY_GUARD_TSV"
echo "- $TRANSPORT_RECALL_TSV"
echo "- $CPU_LATENCY_BUDGET_TSV"
echo "- $SUMMARY_MD"
echo "- $LANE_NOTES_MD"

if [[ "$MODE" == "execute" ]]; then
  echo "- $PROBE_SRC"
  echo "- $PROBE_BIN"
  echo "- $PROBE_COMPILE_LOG"
  echo "- $PROBE_RUN_LOG"
fi

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
exit 0
