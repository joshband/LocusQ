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

OUT_DIR="$ROOT_DIR/TestEvidence/bl009_headphone_profile_contract_${TIMESTAMP}"
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
  local scenario_id="$3"
  local log_file="$OUT_DIR/${label}.log"

  if "$QA_BIN" --spatial "$scenario" --sample-rate 48000 --block-size 512 >"$log_file" 2>&1; then
    log_status "${label}" "pass" "${scenario}"
  else
    log_status "${label}" "fail" "${scenario}; log=${log_file}"
    return 1
  fi

  cp "$ROOT_DIR/qa_output/locusq_spatial/${scenario_id}/wet.wav" "$OUT_DIR/${label}.wet.wav"
  cp "$ROOT_DIR/qa_output/locusq_spatial/${scenario_id}/result.json" "$OUT_DIR/${label}.result.json"
}

SCEN_GENERIC="qa/scenarios/locusq_bl009_headphone_profile_generic.json"
SCEN_AIRPODS="qa/scenarios/locusq_bl009_headphone_profile_airpods.json"
SCEN_SONY="qa/scenarios/locusq_bl009_headphone_profile_sony.json"

run_spatial_scenario "qa_generic_a" "$SCEN_GENERIC" "locusq_bl009_headphone_profile_generic"
run_spatial_scenario "qa_generic_b" "$SCEN_GENERIC" "locusq_bl009_headphone_profile_generic"
run_spatial_scenario "qa_airpods" "$SCEN_AIRPODS" "locusq_bl009_headphone_profile_airpods"
run_spatial_scenario "qa_sony" "$SCEN_SONY" "locusq_bl009_headphone_profile_sony"

hash_of() {
  shasum -a 256 "$1" | awk '{print $1}'
}

HASH_GENERIC_A="$(hash_of "$OUT_DIR/qa_generic_a.wet.wav")"
HASH_GENERIC_B="$(hash_of "$OUT_DIR/qa_generic_b.wet.wav")"
HASH_AIRPODS="$(hash_of "$OUT_DIR/qa_airpods.wet.wav")"
HASH_SONY="$(hash_of "$OUT_DIR/qa_sony.wet.wav")"

if [[ "$HASH_GENERIC_A" == "$HASH_GENERIC_B" ]]; then
  log_status "determinism_generic_profile" "pass" "$HASH_GENERIC_A"
else
  log_status "determinism_generic_profile" "fail" "${HASH_GENERIC_A} vs ${HASH_GENERIC_B}"
  exit 1
fi

if [[ "$HASH_GENERIC_A" == "$HASH_AIRPODS" ]]; then
  log_status "profile_airpods_divergence" "fail" "generic and airpods hashes match (${HASH_GENERIC_A})"
  exit 1
fi
log_status "profile_airpods_divergence" "pass" "${HASH_GENERIC_A} vs ${HASH_AIRPODS}"

if [[ "$HASH_GENERIC_A" == "$HASH_SONY" ]]; then
  log_status "profile_sony_divergence" "fail" "generic and sony hashes match (${HASH_GENERIC_A})"
  exit 1
fi
log_status "profile_sony_divergence" "pass" "${HASH_GENERIC_A} vs ${HASH_SONY}"

if [[ "$HASH_AIRPODS" == "$HASH_SONY" ]]; then
  log_status "profile_airpods_vs_sony" "warn" "hashes match (${HASH_AIRPODS})"
else
  log_status "profile_airpods_vs_sony" "pass" "${HASH_AIRPODS} vs ${HASH_SONY}"
fi

SELFTEST_DETAIL="selftest_not_run"
SELFTEST_PASS="unknown"

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
    SELFTEST_PASS="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P1-009")) | .[0].pass) // false' "$OUT_DIR/ui_selftest_bl009.json")"
    SELFTEST_DETAIL="$(jq -r '((.payload.checks // .result.checks // []) | map(select(.id=="UI-P1-009")) | .[0].details) // "missing_details"' "$OUT_DIR/ui_selftest_bl009.json")"
    if [[ "$SELFTEST_PASS" != "true" ]]; then
      log_status "ui_selftest_bl009_check" "fail" "$SELFTEST_DETAIL"
      exit 1
    fi
    if [[ "$SELFTEST_DETAIL" != *"profileReq="* ]]; then
      log_status "ui_selftest_profile_diag" "fail" "$SELFTEST_DETAIL"
      exit 1
    fi
    log_status "ui_selftest_profile_diag" "pass" "$SELFTEST_DETAIL"
  else
    log_status "ui_selftest_bl009_artifact" "fail" "artifact_missing"
    exit 1
  fi
else
  log_status "ui_selftest_bl009" "warn" "standalone_missing_skip"
fi

cat >"$REPORT_MD" <<EOF
Title: BL-009 Headphone Profile Contract Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-009 Headphone Profile Contract (${TIMESTAMP})

- qa_bin: \`${QA_BIN}\`
- standalone_exec: \`${APP_EXEC}\`
- ui_bl009_pass: \`${SELFTEST_PASS}\`

## Hashes

- generic_a: \`${HASH_GENERIC_A}\`
- generic_b: \`${HASH_GENERIC_B}\`
- airpods: \`${HASH_AIRPODS}\`
- sony: \`${HASH_SONY}\`

## UI-P1-009 Detail

\`${SELFTEST_DETAIL}\`

## Artifacts

- \`status.tsv\`
- \`qa_generic_a.log\`, \`qa_generic_b.log\`, \`qa_airpods.log\`, \`qa_sony.log\`
- \`qa_generic_a.wet.wav\`, \`qa_generic_b.wet.wav\`, \`qa_airpods.wet.wav\`, \`qa_sony.wet.wav\`
- \`ui_selftest_bl009.log\`, \`ui_selftest_bl009.json\` (when standalone available)
EOF

log_status "summary" "pass" "report=$REPORT_MD"
echo "PASS: BL-009 headphone profile contract complete"
echo "artifact_dir=$OUT_DIR"
