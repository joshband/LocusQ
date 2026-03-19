#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract-only"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl082_runner_app_library_${TIMESTAMP}"
QA_BIN="${ROOT_DIR}/build_local/locusq_qa_artefacts/Release/locusq_qa"

usage() {
  cat <<EOF
Usage: qa-bl082-runner-app-library-mac.sh [options]

Options:
  --contract-only      Run static contract checks only (default)
  --execute            Run local qa runner smoke/suite/discover probes
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
  if [[ "$(wc -l < "$ROOT_DIR/qa/main.cpp" | tr -d ' ')" -le 20 ]] \
    && rg -q 'runLocusQQA' "$ROOT_DIR/qa/main.cpp"; then
    record "thin_entrypoint" "PASS" "qa/main.cpp is a thin process entrypoint"
  else
    record "thin_entrypoint" "FAIL" "qa/main.cpp no longer looks like a thin entrypoint"
  fi

  if rg -q '#include "qa_runner_app/BaseQARunner.h"' "$ROOT_DIR/qa/LocusQQARunner.cpp" \
    && rg -q 'class LocusQQARunner final : public qa::runner_app::BaseQARunner' "$ROOT_DIR/qa/LocusQQARunner.cpp"; then
    record "shared_runner_base" "PASS" "LocusQ runner is BaseQARunner-backed"
  else
    record "shared_runner_base" "FAIL" "LocusQ runner does not appear to use BaseQARunner"
  fi

  if [[ -f "/Users/artbox/Documents/Repos/audio-dsp-qa-harness/lib/qa_runner_app/BaseQARunner.h" ]] \
    && [[ -f "/Users/artbox/Documents/Repos/audio-dsp-qa-harness/lib/qa_runner_app/CliParser.h" ]] \
    && [[ -f "/Users/artbox/Documents/Repos/audio-dsp-qa-harness/lib/qa_runner_app/SuiteRouter.h" ]]; then
    record "upstream_runner_app" "PASS" "audio-dsp-qa-harness exposes qa_runner_app library headers"
  else
    record "upstream_runner_app" "FAIL" "missing upstream qa_runner_app headers"
  fi

  if rg -q 'maybeHandleCustomRun' "$ROOT_DIR/qa/LocusQQARunner.cpp" \
    && rg -q 'handleCustomOption' "$ROOT_DIR/qa/LocusQQARunner.cpp"; then
    record "remaining_local_policy" "PASS" "repo-local runner policy remains isolated in LocusQQARunner.cpp"
  else
    record "remaining_local_policy" "FAIL" "could not find repo-local runner policy hooks"
  fi
else
  if [[ ! -x "$QA_BIN" ]]; then
    record "execute_preflight" "FAIL" "qa binary missing at $QA_BIN"
  else
    record "execute_preflight" "PASS" "qa binary found at $QA_BIN"
    if "$QA_BIN" qa/scenarios/locusq_emitter_passthrough.json >"$OUT_DIR/scenario.log" 2>&1; then
      record "single_scenario" "PASS" "single scenario route passed"
    else
      record "single_scenario" "FAIL" "single scenario route failed log=$OUT_DIR/scenario.log"
    fi

    if "$QA_BIN" qa/scenarios/locusq_smoke_suite.json >"$OUT_DIR/suite.log" 2>&1; then
      record "suite_route" "PASS" "suite route passed"
    else
      record "suite_route" "FAIL" "suite route failed log=$OUT_DIR/suite.log"
    fi

    DISCOVER_DIR="$OUT_DIR/discover_cases"
    mkdir -p "$DISCOVER_DIR"
    cp "$ROOT_DIR/qa/scenarios/locusq_emitter_passthrough.json" "$DISCOVER_DIR/"
    cp "$ROOT_DIR/qa/scenarios/locusq_renderer_no_clipping.json" "$DISCOVER_DIR/"
    if "$QA_BIN" --discover "$DISCOVER_DIR" >"$OUT_DIR/discover.log" 2>&1; then
      record "discover_route" "PASS" "discover route passed"
    else
      record "discover_route" "FAIL" "discover route failed log=$OUT_DIR/discover.log"
    fi
  fi
fi

{
  echo "# BL-082 Runner App Library QA"
  echo
  echo "- mode: \`${MODE}\`"
  echo "- status_tsv: \`${STATUS_TSV}\`"
} > "$SUMMARY_MD"

cat "$STATUS_TSV"
