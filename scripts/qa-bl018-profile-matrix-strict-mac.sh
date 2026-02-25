#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

QA_BIN="${QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
PROBE_BIN="${PROBE_BIN:-$ROOT_DIR/build_local/locusq_bl018_profile_probe_artefacts/Release/locusq_bl018_profile_probe}"
SELFTEST_SCRIPT="${SELFTEST_SCRIPT:-$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh}"
APP_INPUT="${APP_INPUT:-$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app}"

OUT_DIR="$ROOT_DIR/TestEvidence/bl018_profile_matrix_${TIMESTAMP}"
SCENARIO_DIR="$OUT_DIR/scenarios"
mkdir -p "$OUT_DIR" "$SCENARIO_DIR"

PER_PROFILE_TSV="$OUT_DIR/per_profile_results.tsv"
DIAGNOSTICS_SNAPSHOT_JSON="$OUT_DIR/diagnostics_snapshot.json"
PRODUCTION_SELFTEST_LOG="$OUT_DIR/production_selftest.log"
PROFILES_JSONL="$OUT_DIR/.profiles.jsonl"
STATUS_TSV="$OUT_DIR/status.tsv"
REPORT_MD="$OUT_DIR/report.md"

printf "step\tstatus\tdetail\n" >"$STATUS_TSV"
printf "profile\tresult\tqa_status\twarnings\tallocation_free\tdeadline\tdiagnostics_match\tfallback_triggered\trt_safe\trequested\tactive\tstage\thp_requested\thp_active\tnotes\n" >"$PER_PROFILE_TSV"
: >"$PROFILES_JSONL"

log_status() {
  local step="$1"
  local status="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$step" "$status" "$detail" | tee -a "$STATUS_TSV" >/dev/null
}

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool:${cmd}" "pass" "$(command -v "$cmd")"
  else
    log_status "tool:${cmd}" "fail" "not_found"
  fi
}

require_cmd jq
require_cmd python3

if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "fail" "missing=${QA_BIN}"
else
  log_status "qa_bin" "pass" "$QA_BIN"
fi

if [[ ! -x "$PROBE_BIN" ]]; then
  log_status "probe_bin" "fail" "missing=${PROBE_BIN}"
else
  log_status "probe_bin" "pass" "$PROBE_BIN"
fi

if [[ ! -x "$SELFTEST_SCRIPT" ]]; then
  log_status "selftest_script" "fail" "missing=${SELFTEST_SCRIPT}"
else
  log_status "selftest_script" "pass" "$SELFTEST_SCRIPT"
fi

if awk -F'\t' 'NR>1 && $2=="fail" { exit 0 } END { exit 1 }' "$STATUS_TSV"; then
  echo "FAIL: prerequisite missing for BL-018 strict lane" >&2
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

log_status "selftest" "pass" "running_production_selftest"
if "$SELFTEST_SCRIPT" "$APP_INPUT" >"$PRODUCTION_SELFTEST_LOG" 2>&1; then
  SELFTEST_ARTIFACT="$(awk -F= '/^artifact=/{print $2}' "$PRODUCTION_SELFTEST_LOG" | tail -n 1)"
  if [[ -n "$SELFTEST_ARTIFACT" ]]; then
    log_status "selftest" "pass" "artifact=${SELFTEST_ARTIFACT}"
  else
    log_status "selftest" "warn" "completed_without_artifact_line"
  fi
else
  log_status "selftest" "fail" "see=${PRODUCTION_SELFTEST_LOG}"
  echo "FAIL: production self-test failed during BL-018 strict lane" >&2
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

profile_norm() {
  local index="$1"
  python3 - "$index" <<'PY'
import sys
idx = int(sys.argv[1])
print(f"{max(0, min(11, idx))/11.0:.6f}")
PY
}

headphone_norm() {
  local index="$1"
  if [[ "$index" == "1" ]]; then
    echo "1.0"
  else
    echo "0.0"
  fi
}

# profile|alias|spatial_index|channels|headphone_mode|expected_requested|expected_active|expected_stage|expected_hp_requested|expected_hp_active
PROFILES=(
  "mono|mono|1|1|0|stereo_2_0|stereo_2_0|direct|stereo_downmix|stereo_downmix"
  "stereo|stereo|1|2|0|stereo_2_0|stereo_2_0|direct|stereo_downmix|stereo_downmix"
  "quadraphonic|quadraphonic|2|4|0|quad_4_0|quad_4_0|direct|stereo_downmix|stereo_downmix"
  "5.1|5.1|3|8|0|surround_5_2_1|surround_5_2_1|direct|stereo_downmix|stereo_downmix"
  "7.1|7.1|4|10|0|surround_7_2_1|surround_7_2_1|direct|stereo_downmix|stereo_downmix"
  "binaural_generic|binaural_generic|9|2|0|virtual_3d_stereo|virtual_3d_stereo|direct|stereo_downmix|stereo_downmix"
  "binaural_steam|binaural_steam|9|2|1|virtual_3d_stereo|virtual_3d_stereo|direct|steam_binaural|steam_binaural"
  "ambisonic_1st|ambisonic_1st|6|4|0|ambisonic_foa|ambisonic_foa|direct|stereo_downmix|stereo_downmix"
  "ambisonic_3rd|ambisonic_3rd|7|16|0|ambisonic_hoa|ambisonic_hoa|direct|stereo_downmix|stereo_downmix"
)

for entry in "${PROFILES[@]}"; do
  IFS='|' read -r profile alias spatial_index channels headphone_mode expected_requested expected_active expected_stage expected_hp_requested expected_hp_active <<<"$entry"

  scenario_id="locusq_bl018_matrix_${profile//[^a-zA-Z0-9_]/_}"
  scenario_path="$SCENARIO_DIR/${scenario_id}.json"
  profile_value="$(profile_norm "$spatial_index")"
  headphone_value="$(headphone_norm "$headphone_mode")"

  cat >"$scenario_path" <<EOF
{
  "scenario_version": "1.0",
  "id": "${scenario_id}",
  "name": "LocusQ BL-018 Strict Profile ${profile}",
  "category": "performance",
  "description": "Strict BL-018 profile validation lane for ${profile}.",
  "capability_requirements": {
    "required_effect_types": ["SPATIAL"],
    "required_behaviors": ["STATEFUL"],
    "excluded_behaviors": []
  },
  "stimulus": {
    "stimulus_id": "multitone",
    "stimulus_variant": "harmonic",
    "parameters": {
      "fundamental_hz": 110.0,
      "num_harmonics": 8,
      "overall_amplitude": 0.30,
      "duration_seconds": 2.0
    }
  },
  "parameter_variations": {
    "pos_azimuth": 0.78,
    "pos_elevation": 0.44,
    "pos_distance": 0.10,
    "emit_gain": 0.56,
    "emit_mute": 0.0,
    "emit_spread": 0.05,
    "emit_directivity": 0.35,
    "emit_dir_azimuth": 0.75,
    "emit_dir_elevation": 0.42,
    "rend_master_gain": 0.50,
    "rend_quality": 1.0,
    "rend_air_absorb": 0.0,
    "rend_doppler": 0.0,
    "rend_room_enable": 0.0,
    "rend_headphone_mode": ${headphone_value},
    "rend_headphone_profile": 0.0,
    "rend_spatial_profile": ${profile_value}
  },
  "analysis_windows": {
    "full": {
      "type": "time_range",
      "start_seconds": 0.10,
      "end_seconds": 2.0,
      "description": "Strict profile validation window"
    }
  },
  "expected_invariants": {
    "signal_present": {
      "metric": "rms_energy",
      "window": "full",
      "threshold": {
        "min": -90.0
      },
      "severity": "hard_fail"
    },
    "no_nan_inf": {
      "metric": "non_finite",
      "window": "full",
      "threshold": {
        "max_count": 0
      },
      "severity": "hard_fail"
    },
    "no_clipping": {
      "metric": "clipping",
      "window": "full",
      "threshold": {
        "peak_dbfs_max": -0.1
      },
      "severity": "hard_fail"
    },
    "deadline": {
      "metric": "perf_meets_deadline",
      "threshold": {
        "equals": true
      },
      "severity": "hard_fail"
    },
    "allocation_free": {
      "metric": "perf_allocation_free",
      "threshold": {
        "equals": true
      },
      "severity": "hard_fail"
    },
    "avg_block_time": {
      "metric": "perf_avg_block_time_ms",
      "threshold": {
        "max": 3.0
      },
      "severity": "hard_fail"
    }
  },
  "pass_criteria": "All hard_fail invariants must pass."
}
EOF

  qa_log="$OUT_DIR/${profile}.qa.log"
  if "$QA_BIN" --spatial "$scenario_path" --sample-rate 48000 --block-size 512 --channels "$channels" >"$qa_log" 2>&1; then
    qa_status="PASS"
  else
    qa_status="FAIL"
  fi

  result_json="$ROOT_DIR/qa_output/locusq_spatial/${scenario_id}/result.json"
  copied_result_json="$OUT_DIR/${profile}.result.json"
  if [[ -f "$result_json" ]]; then
    cp "$result_json" "$copied_result_json"
  else
    qa_status="FAIL"
  fi

  warnings_count="999"
  allocation_status="MISSING"
  deadline_status="MISSING"
  if [[ -f "$copied_result_json" ]]; then
    warnings_count="$(jq '.warnings | length' "$copied_result_json")"
    allocation_status="$(jq -r '.metrics.allocation_free.status // "MISSING"' "$copied_result_json")"
    deadline_status="$(jq -r '.metrics.deadline.status // "MISSING"' "$copied_result_json")"
    qa_status_from_json="$(jq -r '.status // "UNKNOWN"' "$copied_result_json")"
    if [[ "$qa_status" == "PASS" && "$qa_status_from_json" != "PASS" ]]; then
      qa_status="FAIL"
    fi
  fi

  diag_json="$OUT_DIR/${profile}.diag.json"
  if "$PROBE_BIN" --profile "$alias" --sample-rate 48000 --block-size 512 --channels "$channels" --spatial-profile-index "$spatial_index" --headphone-mode-index "$headphone_mode" >"$diag_json"; then
    :
  else
    qa_status="FAIL"
    printf '{}\n' >"$diag_json"
  fi

  requested="$(jq -r '.rendererSpatialProfileRequested // "unknown"' "$diag_json")"
  active="$(jq -r '.rendererSpatialProfileActive // "unknown"' "$diag_json")"
  stage="$(jq -r '.rendererSpatialProfileStage // "unknown"' "$diag_json")"
  hp_requested="$(jq -r '.rendererHeadphoneModeRequested // "unknown"' "$diag_json")"
  hp_active="$(jq -r '.rendererHeadphoneModeActive // "unknown"' "$diag_json")"
  steam_available="$(jq -r '.rendererSteamAudioAvailable // false' "$diag_json")"
  steam_compiled="$(jq -r '.rendererSteamAudioCompiled // false' "$diag_json")"

  diagnostics_match="true"
  if [[ "$requested" != "$expected_requested" || "$active" != "$expected_active" || "$stage" != "$expected_stage" ]]; then
    diagnostics_match="false"
  fi
  if [[ "$hp_requested" != "$expected_hp_requested" || "$hp_active" != "$expected_hp_active" ]]; then
    diagnostics_match="false"
  fi

  fallback_triggered="false"
  if [[ "$stage" != "direct" ]]; then
    fallback_triggered="true"
  fi
  if [[ "$profile" == "binaural_steam" && "$hp_active" != "steam_binaural" ]]; then
    fallback_triggered="true"
  fi

  rt_safe="true"
  if [[ "$allocation_status" != "PASS" || "$deadline_status" != "PASS" ]]; then
    rt_safe="false"
  fi

  warn_free="true"
  if [[ "$warnings_count" != "0" ]]; then
    warn_free="false"
  fi

  result="pass"
  notes="ok"
  if [[ "$qa_status" != "PASS" ]]; then
    result="fail"
    notes="qa_status_fail"
  elif [[ "$warn_free" != "true" ]]; then
    result="fail"
    notes="warnings_present"
  elif [[ "$rt_safe" != "true" ]]; then
    result="fail"
    notes="rt_violation"
  elif [[ "$diagnostics_match" != "true" ]]; then
    result="fail"
    notes="diagnostics_mismatch"
  elif [[ "$fallback_triggered" != "false" ]]; then
    result="fail"
    notes="fallback_triggered"
  elif [[ "$profile" == "binaural_steam" && "$steam_available" != "true" ]]; then
    result="fail"
    notes="steam_unavailable_compiled_${steam_compiled}"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$profile" "$result" "$qa_status" "$warnings_count" "$allocation_status" "$deadline_status" \
    "$diagnostics_match" "$fallback_triggered" "$rt_safe" "$requested" "$active" "$stage" \
    "$hp_requested" "$hp_active" "$notes" >>"$PER_PROFILE_TSV"

  jq -n \
    --arg profile "$profile" \
    --arg alias "$alias" \
    --arg result "$result" \
    --arg qaStatus "$qa_status" \
    --arg warnings "$warnings_count" \
    --arg allocationStatus "$allocation_status" \
    --arg deadlineStatus "$deadline_status" \
    --arg diagnosticsMatch "$diagnostics_match" \
    --arg fallbackTriggered "$fallback_triggered" \
    --arg rtSafe "$rt_safe" \
    --arg requested "$requested" \
    --arg active "$active" \
    --arg stage "$stage" \
    --arg hpRequested "$hp_requested" \
    --arg hpActive "$hp_active" \
    --arg expectedRequested "$expected_requested" \
    --arg expectedActive "$expected_active" \
    --arg expectedStage "$expected_stage" \
    --arg expectedHpRequested "$expected_hp_requested" \
    --arg expectedHpActive "$expected_hp_active" \
    --arg steamAvailable "$steam_available" \
    --arg steamCompiled "$steam_compiled" \
    --arg notes "$notes" \
    --argjson channels "$channels" \
    --argjson spatialIndex "$spatial_index" \
    --argjson headphoneMode "$headphone_mode" \
    '{
      profile: $profile,
      alias: $alias,
      result: $result,
      qaStatus: $qaStatus,
      warnings: ($warnings | tonumber),
      allocationStatus: $allocationStatus,
      deadlineStatus: $deadlineStatus,
      diagnosticsMatch: ($diagnosticsMatch == "true"),
      fallbackTriggered: ($fallbackTriggered == "true"),
      rtSafe: ($rtSafe == "true"),
      requested: $requested,
      active: $active,
      stage: $stage,
      headphoneRequested: $hpRequested,
      headphoneActive: $hpActive,
      expected: {
        requested: $expectedRequested,
        active: $expectedActive,
        stage: $expectedStage,
        headphoneRequested: $expectedHpRequested,
        headphoneActive: $expectedHpActive
      },
      steamAudio: {
        available: ($steamAvailable == "true"),
        compiled: ($steamCompiled == "true")
      },
      channels: $channels,
      spatialProfileIndex: $spatialIndex,
      headphoneModeIndex: $headphoneMode,
      notes: $notes
    }' >>"$PROFILES_JSONL"
done

jq -s \
  --arg schema "locusq-bl018-profile-matrix-v1" \
  --arg timestamp "$TIMESTAMP" \
  --arg docDate "$DOC_DATE_UTC" \
  --arg selftestLog "$PRODUCTION_SELFTEST_LOG" \
  --arg selftestArtifact "${SELFTEST_ARTIFACT:-}" \
  '{
    schema: $schema,
    timestampUtc: $timestamp,
    generatedDateUtc: $docDate,
    productionSelfTest: {
      log: $selftestLog,
      artifact: $selftestArtifact
    },
    profiles: .
  }' "$PROFILES_JSONL" >"$DIAGNOSTICS_SNAPSHOT_JSON"

fail_count="$(awk -F'\t' 'NR>1 && $2!="pass" { c++ } END { print c+0 }' "$PER_PROFILE_TSV")"
pass_count="$(awk -F'\t' 'NR>1 && $2=="pass" { c++ } END { print c+0 }' "$PER_PROFILE_TSV")"

overall="pass"
if [[ "$fail_count" != "0" ]]; then
  overall="fail"
fi

cat >"$REPORT_MD" <<EOF
Title: BL-018 Profile Matrix Strict Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-018 Profile Matrix Strict (${TIMESTAMP})

- overall: \`${overall}\`
- pass_count: \`${pass_count}\`
- fail_count: \`${fail_count}\`
- production_selftest_log: \`${PRODUCTION_SELFTEST_LOG}\`
- production_selftest_artifact: \`${SELFTEST_ARTIFACT:-n/a}\`

## Required Artifacts

- \`per_profile_results.tsv\`
- \`diagnostics_snapshot.json\`
- \`status.tsv\`
EOF

if [[ "$overall" == "fail" ]]; then
  log_status "overall" "fail" "fail_count=${fail_count}"
  echo "FAIL: BL-018 strict profile matrix detected failures" >&2
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

log_status "overall" "pass" "pass_count=${pass_count}; fail_count=${fail_count}"
echo "PASS: BL-018 strict profile matrix completed"
echo "artifact_dir=$OUT_DIR"
