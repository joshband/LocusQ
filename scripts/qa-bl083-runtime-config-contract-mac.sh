#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract-only"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl083_runtime_config_contract_${TIMESTAMP}"
QA_BIN="${ROOT_DIR}/build_local/locusq_qa_artefacts/Release/locusq_qa"
SUITE_PATH="${ROOT_DIR}/qa/scenarios/locusq_smoke_suite.json"

usage() {
  cat <<EOF
Usage: qa-bl083-runtime-config-contract-mac.sh [options]

Options:
  --contract-only      Run static contract checks only (default)
  --execute            Run local runtime-config proof lane
  --out <dir>          Output directory (default: ${OUT_DIR})
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only) MODE="contract-only"; shift ;;
    --execute) MODE="execute"; shift ;;
    --out) OUT_DIR="${2:-}"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"
printf "check\tresult\tdetail\n" > "$STATUS_TSV"

record() {
  printf "%s\t%s\t%s\n" "$1" "$2" "$3" >> "$STATUS_TSV"
}

if [[ "$MODE" == "contract-only" ]]; then
  if rg -Fq 'ExecutionConfig resolveExecutionConfig' /Users/artbox/Documents/Repos/audio-dsp-qa-harness/scenario_engine/scenario_executor.h \
    && rg -Fq 'return applySuiteRuntimeConfig(config_, suite);' /Users/artbox/Documents/Repos/audio-dsp-qa-harness/scenario_engine/scenario_executor.cpp; then
    record "upstream_resolve_contract" "PASS" "ScenarioExecutor resolves suite runtime config through applySuiteRuntimeConfig"
  else
    record "upstream_resolve_contract" "FAIL" "upstream runtime-config resolution contract missing"
  fi

  if rg -q 'runtimeConfig.hasOverrides' /Users/artbox/Documents/Repos/audio-dsp-qa-harness/scenario_engine/scenario_executor.cpp \
    && rg -q 'did not change the execution config' /Users/artbox/Documents/Repos/audio-dsp-qa-harness/scenario_engine/scenario_executor.cpp; then
    record "warning_contract" "PASS" "ScenarioExecutor emits no-op override warnings"
  else
    record "warning_contract" "FAIL" "no-op override warning contract missing"
  fi

  if [[ "$(wc -l < "$ROOT_DIR/qa/main.cpp" | tr -d ' ')" -le 20 ]]; then
    record "local_manual_workaround_removed" "PASS" "LocusQ no longer carries a bulky local runtime-config workaround in qa/main.cpp"
  else
    record "local_manual_workaround_removed" "FAIL" "qa/main.cpp no longer looks thin"
  fi
else
  if [[ ! -x "$QA_BIN" ]]; then
    record "execute_preflight" "FAIL" "qa binary missing at $QA_BIN"
  else
    TEMP_SUITE="$ROOT_DIR/qa/scenarios/bl083_runtime_override_suite.json"
    cat >"$TEMP_SUITE" <<EOF
{
  "id": "bl083_runtime_override_suite",
  "name": "BL-083 runtime override suite",
  "scenario_ids": [
    "locusq_emitter_passthrough"
  ],
  "runtime_config": {
    "sample_rate": 44100,
    "block_size": 512,
    "num_channels": 1,
    "output_dir": "$OUT_DIR/qa_output",
    "enable_profiling": true,
    "profiling_iterations": 32,
    "profiling_warmup_iterations": 4,
    "profiling_policy": "error"
  }
}
EOF
    trap 'rm -f "$TEMP_SUITE"' EXIT
    if "$QA_BIN" "$TEMP_SUITE" --sample-rate 32000 --block-size 128 --channels 2 >"$OUT_DIR/execute.log" 2>&1; then
      RESULT_JSON="$OUT_DIR/qa_output/locusq_emitter_passthrough/result.json"
      if [[ -f "$RESULT_JSON" ]] \
        && jq -e '.audio_config.sample_rate == 44100' "$RESULT_JSON" >/dev/null \
        && jq -e '.audio_config.block_size == 512' "$RESULT_JSON" >/dev/null \
        && jq -e '.audio_config.num_channels == 1' "$RESULT_JSON" >/dev/null; then
        cp "$RESULT_JSON" "$OUT_DIR/result.json"
        record "suite_runtime_config_execute" "PASS" "suite runtime_config overrode CLI base config and redirected outputs into the suite-owned output_dir"
      else
        record "suite_runtime_config_execute" "FAIL" "result provenance/audio config did not reflect suite runtime_config"
      fi
    else
      record "suite_runtime_config_execute" "FAIL" "qa suite execution failed log=$OUT_DIR/execute.log"
    fi
  fi
fi

{
  echo "# BL-083 Runtime Config Contract QA"
  echo
  echo "- mode: \`${MODE}\`"
  echo "- status_tsv: \`${STATUS_TSV}\`"
} > "$SUMMARY_MD"

cat "$STATUS_TSV"
