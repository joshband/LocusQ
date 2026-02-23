#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

QA_BIN_INPUT="${1:-}"
APP_INPUT="${2:-}"

if [[ -z "$QA_BIN_INPUT" ]]; then
  QA_BIN="$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa"
else
  QA_BIN="$QA_BIN_INPUT"
fi

if [[ -z "$APP_INPUT" ]]; then
  APP_EXEC="$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app/Contents/MacOS/LocusQ"
elif [[ -d "$APP_INPUT" ]]; then
  APP_EXEC="$APP_INPUT/Contents/MacOS/LocusQ"
else
  APP_EXEC="$APP_INPUT"
fi

if [[ ! -x "$QA_BIN" ]]; then
  echo "ERROR: QA runner not found: $QA_BIN"
  echo "Build first:"
  echo "  cmake --build build_local --config Release --target locusq_qa -j 8"
  exit 2
fi

OUT_DIR="$ROOT_DIR/TestEvidence/bl009_headphone_contract_${TIMESTAMP}"
mkdir -p "$OUT_DIR"
STATUS_TSV="$OUT_DIR/status.tsv"
REPORT_MD="$OUT_DIR/report.md"

{
  echo -e "step\tstatus\tdetail"
  echo -e "init\tpass\tts=${TIMESTAMP}"
  echo -e "qa_bin\tpass\t${QA_BIN}"
  if [[ -x "$APP_EXEC" ]]; then
    echo -e "standalone_exec\tpass\t${APP_EXEC}"
  else
    echo -e "standalone_exec\twarn\tmissing=${APP_EXEC}"
  fi
} > "$STATUS_TSV"

log_status() {
  local step="$1"
  local status="$2"
  local detail="$3"
  echo -e "${step}\t${status}\t${detail}" | tee -a "$STATUS_TSV" >/dev/null
}

run_spatial_scenario() {
  local label="$1"
  local scenario="$2"
  local log_file="$3"

  if "$QA_BIN" --spatial "$scenario" --sample-rate 48000 --block-size 512 >"$log_file" 2>&1; then
    log_status "${label}" "pass" "${scenario}"
  else
    log_status "${label}" "fail" "${scenario}; log=${log_file}"
    return 1
  fi
}

DOWNMIX_SCENARIO="qa/scenarios/locusq_bl009_headphone_downmix_reference.json"
STEAM_SCENARIO="qa/scenarios/locusq_bl009_headphone_steam_request.json"

run_spatial_scenario "qa_downmix_a" "$DOWNMIX_SCENARIO" "$OUT_DIR/qa_downmix_a.log"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_downmix_reference/wet.wav" "$OUT_DIR/downmix_a.wet.wav"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_downmix_reference/result.json" "$OUT_DIR/downmix_a.result.json"

run_spatial_scenario "qa_downmix_b" "$DOWNMIX_SCENARIO" "$OUT_DIR/qa_downmix_b.log"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_downmix_reference/wet.wav" "$OUT_DIR/downmix_b.wet.wav"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_downmix_reference/result.json" "$OUT_DIR/downmix_b.result.json"

run_spatial_scenario "qa_steam_a" "$STEAM_SCENARIO" "$OUT_DIR/qa_steam_a.log"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_steam_request/wet.wav" "$OUT_DIR/steam_a.wet.wav"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_steam_request/result.json" "$OUT_DIR/steam_a.result.json"

run_spatial_scenario "qa_steam_b" "$STEAM_SCENARIO" "$OUT_DIR/qa_steam_b.log"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_steam_request/wet.wav" "$OUT_DIR/steam_b.wet.wav"
cp "$ROOT_DIR/qa_output/locusq_spatial/locusq_bl009_headphone_steam_request/result.json" "$OUT_DIR/steam_b.result.json"

hash_of() {
  shasum -a 256 "$1" | awk '{print $1}'
}

DOWNMIX_HASH_A="$(hash_of "$OUT_DIR/downmix_a.wet.wav")"
DOWNMIX_HASH_B="$(hash_of "$OUT_DIR/downmix_b.wet.wav")"
STEAM_HASH_A="$(hash_of "$OUT_DIR/steam_a.wet.wav")"
STEAM_HASH_B="$(hash_of "$OUT_DIR/steam_b.wet.wav")"

if [[ "$DOWNMIX_HASH_A" == "$DOWNMIX_HASH_B" ]]; then
  log_status "determinism_downmix" "pass" "$DOWNMIX_HASH_A"
else
  log_status "determinism_downmix" "fail" "${DOWNMIX_HASH_A} vs ${DOWNMIX_HASH_B}"
  exit 1
fi

if [[ "$STEAM_HASH_A" == "$STEAM_HASH_B" ]]; then
  log_status "determinism_steam_request" "pass" "$STEAM_HASH_A"
else
  log_status "determinism_steam_request" "fail" "${STEAM_HASH_A} vs ${STEAM_HASH_B}"
  exit 1
fi

STEAM_AVAILABLE="unknown"
BL009_DETAIL="selftest_not_run"
BL009_PASS="unknown"
SELFTEST_JSON=""

if [[ -x "$APP_EXEC" ]]; then
  if LOCUSQ_UI_SELFTEST_BL009=1 "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" "$APP_EXEC" >"$OUT_DIR/ui_selftest_bl009.log" 2>&1; then
    log_status "ui_selftest_bl009" "pass" "log=$OUT_DIR/ui_selftest_bl009.log"
  else
    log_status "ui_selftest_bl009" "fail" "log=$OUT_DIR/ui_selftest_bl009.log"
    exit 1
  fi

  SELFTEST_JSON="$(awk -F= '/^artifact=/{print $2}' "$OUT_DIR/ui_selftest_bl009.log" | tail -n 1)"
  if [[ -n "$SELFTEST_JSON" && -f "$SELFTEST_JSON" ]]; then
    cp "$SELFTEST_JSON" "$OUT_DIR/ui_selftest_bl009.json"
    BL009_PASS="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P1-009")) | .[0].pass) // false' "$OUT_DIR/ui_selftest_bl009.json")"
    BL009_DETAIL="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P1-009")) | .[0].details) // "missing_details"' "$OUT_DIR/ui_selftest_bl009.json")"
    if [[ "$BL009_PASS" != "true" ]]; then
      log_status "ui_selftest_bl009_check" "fail" "$BL009_DETAIL"
      exit 1
    fi
    log_status "ui_selftest_bl009_check" "pass" "$BL009_DETAIL"
    if [[ "$BL009_DETAIL" == *"steamAvailable=true"* ]]; then
      STEAM_AVAILABLE="true"
    elif [[ "$BL009_DETAIL" == *"steamAvailable=false"* ]]; then
      STEAM_AVAILABLE="false"
    fi
  else
    log_status "ui_selftest_bl009_artifact" "fail" "artifact_missing"
    exit 1
  fi
else
  log_status "ui_selftest_bl009" "warn" "standalone_missing_skip"
fi

if [[ "$STEAM_AVAILABLE" == "true" ]]; then
  if [[ "$DOWNMIX_HASH_A" == "$STEAM_HASH_A" ]]; then
    log_status "cross_mode_divergence" "fail" "steamAvailable=true but hashes match ($DOWNMIX_HASH_A)"
    exit 1
  fi
  log_status "cross_mode_divergence" "pass" "steamAvailable=true hashes_diverge"
elif [[ "$STEAM_AVAILABLE" == "false" ]]; then
  if [[ "$DOWNMIX_HASH_A" != "$STEAM_HASH_A" ]]; then
    log_status "cross_mode_fallback_equivalence" "fail" "steamAvailable=false but hashes differ (${DOWNMIX_HASH_A} vs ${STEAM_HASH_A})"
    exit 1
  fi
  log_status "cross_mode_fallback_equivalence" "pass" "steamAvailable=false hashes_match"
else
  log_status "cross_mode_assertion" "warn" "steamAvailabilityUnknown; no strict cross-mode check applied"
fi

cat >"$REPORT_MD" <<EOF
Title: BL-009 Headphone Contract Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-009 Headphone Contract (${TIMESTAMP})

- qa_bin: \`${QA_BIN}\`
- standalone_exec: \`${APP_EXEC}\`
- steam_available: \`${STEAM_AVAILABLE}\`
- ui_bl009_pass: \`${BL009_PASS}\`

## Hashes

- downmix_a: \`${DOWNMIX_HASH_A}\`
- downmix_b: \`${DOWNMIX_HASH_B}\`
- steam_a: \`${STEAM_HASH_A}\`
- steam_b: \`${STEAM_HASH_B}\`

## UI-P1-009 Detail

\`${BL009_DETAIL}\`

## Artifacts

- \`status.tsv\`
- \`qa_downmix_a.log\`, \`qa_downmix_b.log\`, \`qa_steam_a.log\`, \`qa_steam_b.log\`
- \`downmix_a.wet.wav\`, \`downmix_b.wet.wav\`, \`steam_a.wet.wav\`, \`steam_b.wet.wav\`
- \`ui_selftest_bl009.log\`, \`ui_selftest_bl009.json\` (when standalone available)
EOF

log_status "summary" "pass" "report=$REPORT_MD"
echo "PASS: BL-009 headphone contract complete"
echo "artifact_dir=$OUT_DIR"
