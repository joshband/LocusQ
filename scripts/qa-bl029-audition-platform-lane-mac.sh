#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

OUT_DIR="${BL029_OUT_DIR:-$ROOT_DIR/TestEvidence/bl029_audition_reactive_qa_slice_g3_${TIMESTAMP}}"
SCENARIO_PATH="${BL029_SCENARIO_PATH:-$ROOT_DIR/qa/scenarios/locusq_audition_platform_showcase.json}"
QA_BIN="${BL029_QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL029_QA_BIN_FALLBACK:-$ROOT_DIR/build_local/locusq_qa_artefacts/locusq_qa}"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
QA_LANE_LOG="$OUT_DIR/qa_lane.log"
SCENARIO_RESULT_LOG="$OUT_DIR/scenario_result.log"
LANE_CONTRACT_MD="$OUT_DIR/lane_contract.md"

printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
: >"$SCENARIO_RESULT_LOG"

# Sandbox-safe logging: avoid process substitution (/dev/fd/*).
exec 3>&1
exec >>"$QA_LANE_LOG" 2>&1

log_status() {
  local check="$1"
  local result="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" "$check" "$result" "$detail" "$artifact" >>"$STATUS_TSV"
  printf "%s: %s - %s\n" "$check" "$result" "$detail"
}

fail_count=0
mark_fail() {
  fail_count=$((fail_count + 1))
}

token_present_in_file() {
  local token="$1"
  local file="$2"
  if rg -Fq -- "$token" "$file"; then
    return 0
  fi
  local escaped_token="${token//\"/\\\"}"
  if [[ "$escaped_token" != "$token" ]] && rg -Fq -- "$escaped_token" "$file"; then
    return 0
  fi
  return 1
}

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "PASS" "$(command -v "$cmd")" ""
  else
    log_status "tool_${cmd}" "FAIL" "missing_command" ""
    mark_fail
  fi
}

echo "BL-029 audition platform lane start: $DOC_TS"
echo "out_dir=$OUT_DIR"
echo "scenario=$SCENARIO_PATH"
echo "qa_bin=$QA_BIN"

require_cmd jq
require_cmd python3
require_cmd shasum
require_cmd rg

if [[ ! -f "$SCENARIO_PATH" ]]; then
  log_status "scenario_file" "FAIL" "missing=$SCENARIO_PATH" "$SCENARIO_PATH"
  mark_fail
fi
if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "FAIL" "missing_or_not_executable=$QA_BIN" "$QA_BIN"
  mark_fail
fi

if [[ "$fail_count" -ne 0 ]]; then
  log_status "lane_prereq" "FAIL" "prerequisite_failure_count=$fail_count" "$STATUS_TSV"
  echo "artifact_dir=$OUT_DIR"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

SCENARIO_ID="$(jq -r '.id' "$SCENARIO_PATH")"
if [[ -z "$SCENARIO_ID" || "$SCENARIO_ID" == "null" ]]; then
  log_status "scenario_id" "FAIL" "missing_id_in_scenario" "$SCENARIO_PATH"
  echo "artifact_dir=$OUT_DIR"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi
REPLAY_RUNS="$(jq -r '(.bl029_contract_checks[]? | select(.id=="deterministic_seed_replay") | .replay_runs) // 2' "$SCENARIO_PATH")"
if ! [[ "$REPLAY_RUNS" =~ ^[0-9]+$ ]]; then
  REPLAY_RUNS=2
fi
REPLAY_RUNS=$(( REPLAY_RUNS < 2 ? 2 : REPLAY_RUNS ))

run_hashes=()
run_result_statuses=()

for run in $(seq 1 "$REPLAY_RUNS"); do
  run_log="$OUT_DIR/scenario_run_${run}.log"
  result_copy="$OUT_DIR/scenario_run_${run}.result.json"
  wet_copy="$OUT_DIR/scenario_run_${run}.wet.wav"
  run_artifact_dir="$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID"

  rm -rf "$run_artifact_dir"

  if "$QA_BIN" --spatial "$SCENARIO_PATH" >"$run_log" 2>&1; then
    log_status "scenario_run_${run}_exec" "PASS" "qa_runner_exit=0" "$run_log"
  else
    log_status "scenario_run_${run}_exec" "FAIL" "qa_runner_nonzero" "$run_log"
    if rg -Fq -- "app_exited_before_result" "$run_log"; then
      log_status "scenario_run_${run}_app_exited_before_result" "FAIL" "signature_detected" "$run_log"
    else
      log_status "scenario_run_${run}_app_exited_before_result" "PASS" "signature_not_detected" "$run_log"
    fi
    mark_fail
    continue
  fi

  if rg -Fq -- "app_exited_before_result" "$run_log"; then
    log_status "scenario_run_${run}_app_exited_before_result" "FAIL" "signature_detected" "$run_log"
    mark_fail
  else
    log_status "scenario_run_${run}_app_exited_before_result" "PASS" "signature_not_detected" "$run_log"
  fi

  run_result_json="$run_artifact_dir/result.json"
  run_wet_wav="$run_artifact_dir/wet.wav"

  if [[ ! -f "$run_result_json" ]]; then
    log_status "scenario_run_${run}_result_json" "FAIL" "missing_result_json" "$run_result_json"
    mark_fail
    continue
  fi

  cp "$run_result_json" "$result_copy"

  run_status="$(jq -r '.status // "UNKNOWN"' "$result_copy")"
  run_rms="$(jq -r '.metrics.signal_present.value // "nan"' "$result_copy")"
  run_peak="$(jq -r '.metrics.no_clipping.value // "nan"' "$result_copy")"
  run_deadline="$(jq -r '.metrics.deadline.status // "MISSING"' "$result_copy")"

  run_result_statuses+=("$run_status")

  printf "run=%s status=%s signal_present_value=%s no_clipping_value=%s deadline_status=%s result_json=%s\n" \
    "$run" "$run_status" "$run_rms" "$run_peak" "$run_deadline" "$result_copy" >>"$SCENARIO_RESULT_LOG"

  if [[ "$run_status" != "PASS" ]]; then
    log_status "scenario_run_${run}_status" "FAIL" "result_status=$run_status" "$result_copy"
    mark_fail
  else
    log_status "scenario_run_${run}_status" "PASS" "result_status=PASS" "$result_copy"
  fi

  if [[ ! -f "$run_wet_wav" ]]; then
    log_status "scenario_run_${run}_wet_wav" "FAIL" "missing_wet_wav" "$run_wet_wav"
    mark_fail
    continue
  fi

  cp "$run_wet_wav" "$wet_copy"
  run_hash="$(shasum -a 256 "$wet_copy" | awk '{print $1}')"
  run_hashes+=("$run_hash")
  printf "run=%s wet_sha256=%s wet_wav=%s\n" "$run" "$run_hash" "$wet_copy" >>"$SCENARIO_RESULT_LOG"
  log_status "scenario_run_${run}_wet_hash" "PASS" "sha256=$run_hash" "$wet_copy"
done

expected_modes=()
expected_modes_file="$OUT_DIR/expected_modes.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="cloud_showcase_mode") | .expected_modes[]?' "$SCENARIO_PATH" >"$expected_modes_file"
while IFS= read -r mode; do
  [[ -z "$mode" ]] && continue
  expected_modes+=("$mode")
done <"$expected_modes_file"

missing_modes=()
for mode in "${expected_modes[@]}"; do
  if ! rg -Fq -- "\"${mode}\"" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    missing_modes+=("$mode")
  fi
done
if [[ "${#missing_modes[@]}" -eq 0 ]]; then
  log_status "cloud_showcase_mode" "PASS" "expected_count=${#expected_modes[@]} all_expected_modes_present" "Source/PluginProcessor.cpp"
else
  log_status "cloud_showcase_mode" "FAIL" "missing_modes=${missing_modes[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

proxy_checks=()
proxy_checks_file="$OUT_DIR/proxy_checks.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="bound_proxy_mode_behavior") | .expected_fields[]?' "$SCENARIO_PATH" >"$proxy_checks_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  proxy_checks+=("$token")
done <"$proxy_checks_file"
proxy_checks+=("mode: typeof cloudState?.mode === \"string\" ? cloudState.mode : \"\"")

proxy_missing=()
for token in "${proxy_checks[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/ui/public/js/index.js"; then
    proxy_missing+=("$token")
  fi
done
if [[ "${#proxy_missing[@]}" -eq 0 ]]; then
  log_status "bound_proxy_mode_behavior" "PASS" "expected_count=${#proxy_checks[@]} scene_state_proxy_contract_present" "Source/ui/public/js/index.js"
else
  log_status "bound_proxy_mode_behavior" "FAIL" "missing_tokens=${proxy_missing[*]}" "Source/ui/public/js/index.js"
  mark_fail
fi

reactive_fields=()
reactive_fields_file="$OUT_DIR/reactive_fields.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="reactive_envelope_contract") | .expected_scene_fields[]?' "$SCENARIO_PATH" >"$reactive_fields_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  reactive_fields+=("$token")
done <"$reactive_fields_file"

missing_reactive_fields=()
for token in "${reactive_fields[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    missing_reactive_fields+=("$token")
  fi
done
if [[ "${#missing_reactive_fields[@]}" -eq 0 ]]; then
  log_status "reactive_envelope_contract_fields" "PASS" "expected_count=${#reactive_fields[@]} present" "Source/PluginProcessor.cpp"
else
  log_status "reactive_envelope_contract_fields" "FAIL" "missing_fields=${missing_reactive_fields[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

reactive_min="$(jq -r '.bl029_contract_checks[]? | select(.id=="reactive_envelope_contract") | .expected_range.min // "nan"' "$SCENARIO_PATH")"
reactive_max="$(jq -r '.bl029_contract_checks[]? | select(.id=="reactive_envelope_contract") | .expected_range.max // "nan"' "$SCENARIO_PATH")"
if python3 - "$reactive_min" "$reactive_max" <<'PY'
import math, sys
mn = float(sys.argv[1])
mx = float(sys.argv[2])
ok = math.isfinite(mn) and math.isfinite(mx) and mn >= 0.0 and mx <= 1.0 and mn < mx
sys.exit(0 if ok else 1)
PY
then
  log_status "reactive_envelope_contract_range" "PASS" "range=[${reactive_min},${reactive_max}]" "$SCENARIO_PATH"
else
  log_status "reactive_envelope_contract_range" "FAIL" "invalid_range=[${reactive_min},${reactive_max}]" "$SCENARIO_PATH"
  mark_fail
fi

reactive_range_tokens=()
reactive_range_tokens_file="$OUT_DIR/reactive_range_tokens.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="reactive_envelope_contract") | .expected_range_tokens[]?' "$SCENARIO_PATH" >"$reactive_range_tokens_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  reactive_range_tokens+=("$token")
done <"$reactive_range_tokens_file"

missing_reactive_range_tokens=()
for token in "${reactive_range_tokens[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    missing_reactive_range_tokens+=("$token")
  fi
done
if [[ "${#missing_reactive_range_tokens[@]}" -eq 0 ]]; then
  log_status "reactive_envelope_contract_range_tokens" "PASS" "expected_count=${#reactive_range_tokens[@]} present" "Source/PluginProcessor.cpp"
else
  log_status "reactive_envelope_contract_range_tokens" "FAIL" "missing_tokens=${missing_reactive_range_tokens[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

reactive_fallback_tokens=()
reactive_fallback_tokens_file="$OUT_DIR/reactive_fallback_tokens.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="reactive_envelope_contract") | .missing_block_fallback_tokens[]?' "$SCENARIO_PATH" >"$reactive_fallback_tokens_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  reactive_fallback_tokens+=("$token")
done <"$reactive_fallback_tokens_file"

missing_reactive_fallback_tokens=()
for token in "${reactive_fallback_tokens[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/ui/public/js/index.js"; then
    missing_reactive_fallback_tokens+=("$token")
  fi
done
if [[ "${#missing_reactive_fallback_tokens[@]}" -eq 0 ]]; then
  log_status "reactive_missing_block_fallback" "PASS" "expected_count=${#reactive_fallback_tokens[@]} tokens_present" "Source/ui/public/js/index.js"
else
  log_status "reactive_missing_block_fallback" "FAIL" "missing_tokens=${missing_reactive_fallback_tokens[*]}" "Source/ui/public/js/index.js"
  mark_fail
fi

if [[ "${#missing_reactive_fields[@]}" -eq 0 && "${#missing_reactive_range_tokens[@]}" -eq 0 && "${#missing_reactive_fallback_tokens[@]}" -eq 0 ]]; then
  log_status "reactive_envelope_contract" "PASS" "all_reactive_contract_checks_passed" "$SCENARIO_PATH"
else
  log_status "reactive_envelope_contract" "FAIL" "field_missing=${#missing_reactive_fields[@]} range_token_missing=${#missing_reactive_range_tokens[@]} fallback_token_missing=${#missing_reactive_fallback_tokens[@]}" "$SCENARIO_PATH"
  mark_fail
fi

rain_snow_plugin_tokens=()
rain_snow_plugin_tokens_file="$OUT_DIR/rain_snow_plugin_tokens.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="rain_snow_fade_semantics") | .expected_plugin_tokens[]?' "$SCENARIO_PATH" >"$rain_snow_plugin_tokens_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  rain_snow_plugin_tokens+=("$token")
done <"$rain_snow_plugin_tokens_file"

missing_rain_snow_plugin_tokens=()
for token in "${rain_snow_plugin_tokens[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    missing_rain_snow_plugin_tokens+=("$token")
  fi
done
if [[ "${#missing_rain_snow_plugin_tokens[@]}" -eq 0 ]]; then
  log_status "rain_snow_fade_semantics_plugin" "PASS" "expected_count=${#rain_snow_plugin_tokens[@]} tokens_present" "Source/PluginProcessor.cpp"
else
  log_status "rain_snow_fade_semantics_plugin" "FAIL" "missing_tokens=${missing_rain_snow_plugin_tokens[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

rain_snow_ui_tokens=()
rain_snow_ui_tokens_file="$OUT_DIR/rain_snow_ui_tokens.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="rain_snow_fade_semantics") | .expected_ui_tokens[]?' "$SCENARIO_PATH" >"$rain_snow_ui_tokens_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  rain_snow_ui_tokens+=("$token")
done <"$rain_snow_ui_tokens_file"

missing_rain_snow_ui_tokens=()
for token in "${rain_snow_ui_tokens[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/ui/public/js/index.js"; then
    missing_rain_snow_ui_tokens+=("$token")
  fi
done
if [[ "${#missing_rain_snow_ui_tokens[@]}" -eq 0 ]]; then
  log_status "rain_snow_fade_semantics_ui" "PASS" "expected_count=${#rain_snow_ui_tokens[@]} tokens_present" "Source/ui/public/js/index.js"
else
  log_status "rain_snow_fade_semantics_ui" "FAIL" "missing_tokens=${missing_rain_snow_ui_tokens[*]}" "Source/ui/public/js/index.js"
  mark_fail
fi

if [[ "${#missing_rain_snow_plugin_tokens[@]}" -eq 0 && "${#missing_rain_snow_ui_tokens[@]}" -eq 0 ]]; then
  log_status "rain_snow_fade_semantics" "PASS" "all_rain_snow_semantics_checks_passed" "$SCENARIO_PATH"
else
  log_status "rain_snow_fade_semantics" "FAIL" "plugin_missing=${#missing_rain_snow_plugin_tokens[@]} ui_missing=${#missing_rain_snow_ui_tokens[@]}" "$SCENARIO_PATH"
  mark_fail
fi

bound_fields=()
bound_fields_file="$OUT_DIR/bound_mode_fields.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="bound_mode_contract") | .expected_scene_fields[]?' "$SCENARIO_PATH" >"$bound_fields_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  bound_fields+=("$token")
done <"$bound_fields_file"

bound_modes=()
bound_modes_file="$OUT_DIR/bound_mode_values.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="bound_mode_contract") | .expected_source_modes[]?' "$SCENARIO_PATH" >"$bound_modes_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  bound_modes+=("$token")
done <"$bound_modes_file"

bound_targets=()
bound_targets_file="$OUT_DIR/bound_targets.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="bound_mode_contract") | .expected_binding_targets[]?' "$SCENARIO_PATH" >"$bound_targets_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  bound_targets+=("$token")
done <"$bound_targets_file"

bound_missing_fields=()
for token in "${bound_fields[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    bound_missing_fields+=("$token")
  fi
done
if [[ "${#bound_missing_fields[@]}" -eq 0 ]]; then
  log_status "bound_mode_contract_fields" "PASS" "expected_count=${#bound_fields[@]} present" "Source/PluginProcessor.cpp"
else
  log_status "bound_mode_contract_fields" "FAIL" "missing_fields=${bound_missing_fields[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

bound_missing_modes=()
for mode in "${bound_modes[@]}"; do
  if ! rg -Fq -- "\"${mode}\"" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    bound_missing_modes+=("$mode")
  fi
done
if [[ "${#bound_missing_modes[@]}" -eq 0 ]]; then
  log_status "bound_mode_contract_source_modes" "PASS" "expected_count=${#bound_modes[@]} values_present" "Source/PluginProcessor.cpp"
else
  log_status "bound_mode_contract_source_modes" "FAIL" "missing_modes=${bound_missing_modes[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

bound_missing_targets=()
for target in "${bound_targets[@]}"; do
  if ! rg -Fq -- "\"${target}\"" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    bound_missing_targets+=("$target")
  fi
done
if [[ "${#bound_missing_targets[@]}" -eq 0 ]]; then
  log_status "bound_mode_contract_binding_targets" "PASS" "expected_count=${#bound_targets[@]} values_present" "Source/PluginProcessor.cpp"
else
  log_status "bound_mode_contract_binding_targets" "FAIL" "missing_targets=${bound_missing_targets[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

if [[ "${#bound_missing_fields[@]}" -eq 0 && "${#bound_missing_modes[@]}" -eq 0 && "${#bound_missing_targets[@]}" -eq 0 ]]; then
  log_status "bound_mode_contract" "PASS" "all_bound_mode_checks_passed" "$SCENARIO_PATH"
else
  log_status "bound_mode_contract" "FAIL" "field_missing=${#bound_missing_fields[@]} mode_missing=${#bound_missing_modes[@]} target_missing=${#bound_missing_targets[@]}" "$SCENARIO_PATH"
  mark_fail
fi

fallback_field="$(jq -r '.bl029_contract_checks[]? | select(.id=="fallback_reason_contract") | .expected_scene_field // ""' "$SCENARIO_PATH")"
if [[ -n "$fallback_field" && "$fallback_field" != "null" ]]; then
  if rg -Fq -- "$fallback_field" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    log_status "fallback_reason_contract_field" "PASS" "scene_field_present=${fallback_field}" "Source/PluginProcessor.cpp"
  else
    log_status "fallback_reason_contract_field" "FAIL" "missing_scene_field=${fallback_field}" "Source/PluginProcessor.cpp"
    mark_fail
  fi
else
  log_status "fallback_reason_contract_field" "FAIL" "scenario_missing_expected_scene_field" "$SCENARIO_PATH"
  mark_fail
fi

fallback_reasons=()
fallback_reasons_file="$OUT_DIR/fallback_reasons.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="fallback_reason_contract") | .expected_reasons[]?' "$SCENARIO_PATH" >"$fallback_reasons_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  fallback_reasons+=("$token")
done <"$fallback_reasons_file"

missing_fallback_reasons=()
for reason in "${fallback_reasons[@]}"; do
  if ! rg -Fq -- "\"${reason}\"" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    missing_fallback_reasons+=("$reason")
  fi
done
if [[ "${#missing_fallback_reasons[@]}" -eq 0 ]]; then
  log_status "fallback_reason_contract_values" "PASS" "expected_count=${#fallback_reasons[@]} values_present" "Source/PluginProcessor.cpp"
else
  log_status "fallback_reason_contract_values" "FAIL" "missing_reasons=${missing_fallback_reasons[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

fallback_ui_tokens=()
fallback_ui_tokens_file="$OUT_DIR/fallback_ui_tokens.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="fallback_reason_contract") | .expected_ui_tokens[]?' "$SCENARIO_PATH" >"$fallback_ui_tokens_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  fallback_ui_tokens+=("$token")
done <"$fallback_ui_tokens_file"

missing_fallback_ui_tokens=()
for token in "${fallback_ui_tokens[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/ui/public/js/index.js"; then
    missing_fallback_ui_tokens+=("$token")
  fi
done
if [[ "${#missing_fallback_ui_tokens[@]}" -eq 0 ]]; then
  log_status "fallback_reason_contract_ui" "PASS" "expected_count=${#fallback_ui_tokens[@]} tokens_present" "Source/ui/public/js/index.js"
else
  log_status "fallback_reason_contract_ui" "FAIL" "missing_tokens=${missing_fallback_ui_tokens[*]}" "Source/ui/public/js/index.js"
  mark_fail
fi

if [[ "${#missing_fallback_reasons[@]}" -eq 0 && "${#missing_fallback_ui_tokens[@]}" -eq 0 ]]; then
  log_status "fallback_reason_contract" "PASS" "all_fallback_reason_checks_passed" "$SCENARIO_PATH"
else
  log_status "fallback_reason_contract" "FAIL" "reason_missing=${#missing_fallback_reasons[@]} ui_missing=${#missing_fallback_ui_tokens[@]}" "$SCENARIO_PATH"
  mark_fail
fi

if [[ "${#run_hashes[@]}" -lt 2 ]]; then
  log_status "deterministic_seed_replay" "FAIL" "insufficient_replay_hashes=${#run_hashes[@]}" "$SCENARIO_RESULT_LOG"
  mark_fail
else
  deterministic_ok=1
  baseline_hash="${run_hashes[0]}"
  for hash in "${run_hashes[@]}"; do
    if [[ "$hash" != "$baseline_hash" ]]; then
      deterministic_ok=0
      break
    fi
  done

  if [[ "$deterministic_ok" -eq 1 ]]; then
    log_status "deterministic_seed_replay" "PASS" "replay_runs=$REPLAY_RUNS sha256=$baseline_hash" "$SCENARIO_RESULT_LOG"
  else
    log_status "deterministic_seed_replay" "FAIL" "hash_mismatch=$(IFS=,; echo "${run_hashes[*]}")" "$SCENARIO_RESULT_LOG"
    mark_fail
  fi
fi

stable_modes=()
stable_modes_file="$OUT_DIR/stable_modes.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="deterministic_seed_replay") | .stable_modes[]?' "$SCENARIO_PATH" >"$stable_modes_file"
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  stable_modes+=("$token")
done <"$stable_modes_file"

if [[ "${#stable_modes[@]}" -gt 0 ]]; then
  missing_stable_modes=()
  for mode in "${stable_modes[@]}"; do
    if ! rg -Fq -- "\"${mode}\"" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
      missing_stable_modes+=("$mode")
    fi
  done
  if [[ "${#missing_stable_modes[@]}" -eq 0 ]]; then
    log_status "deterministic_seed_replay_modes" "PASS" "stable_modes=${stable_modes[*]}" "Source/PluginProcessor.cpp"
    printf "stable_modes=%s\n" "$(IFS=,; echo "${stable_modes[*]}")" >>"$SCENARIO_RESULT_LOG"
  else
    log_status "deterministic_seed_replay_modes" "FAIL" "missing_modes=${missing_stable_modes[*]}" "Source/PluginProcessor.cpp"
    mark_fail
  fi
else
  log_status "deterministic_seed_replay_modes" "PASS" "stable_modes_not_configured" "$SCENARIO_PATH"
fi

reactive_mode_matrix_file="$OUT_DIR/reactive_mode_replay_matrix.tsv"
jq -r '.bl029_contract_checks[]? | select(.id=="reactive_mode_replay_matrix") | .key_modes[]? | [.mode, .plugin_pattern_token, .ui_pattern_token] | @tsv' "$SCENARIO_PATH" >"$reactive_mode_matrix_file"
reactive_mode_matrix_count=0
reactive_mode_matrix_failures=0
while IFS=$'\t' read -r mode plugin_pattern_token ui_pattern_token; do
  [[ -z "$mode" ]] && continue
  reactive_mode_matrix_count=$((reactive_mode_matrix_count + 1))

  mode_ok=1
  detail_tokens=()
  if rg -Fq -- "\"${mode}\"" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    detail_tokens+=("mode_token=ok")
  else
    detail_tokens+=("mode_token=missing")
    mode_ok=0
  fi
  if [[ -n "$plugin_pattern_token" ]] && rg -Fq -- "$plugin_pattern_token" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    detail_tokens+=("plugin_pattern=ok")
  else
    detail_tokens+=("plugin_pattern=missing")
    mode_ok=0
  fi
  if [[ -n "$ui_pattern_token" ]] && rg -Fq -- "$ui_pattern_token" "$ROOT_DIR/Source/ui/public/js/index.js"; then
    detail_tokens+=("ui_pattern=ok")
  else
    detail_tokens+=("ui_pattern=missing")
    mode_ok=0
  fi

  if [[ "$mode_ok" -eq 1 ]]; then
    log_status "reactive_mode_replay_${mode}" "PASS" "$(IFS=,; echo "${detail_tokens[*]}")" "$SCENARIO_PATH"
  else
    log_status "reactive_mode_replay_${mode}" "FAIL" "$(IFS=,; echo "${detail_tokens[*]}")" "$SCENARIO_PATH"
    reactive_mode_matrix_failures=$((reactive_mode_matrix_failures + 1))
  fi
done <"$reactive_mode_matrix_file"

if [[ "$reactive_mode_matrix_count" -eq 0 ]]; then
  log_status "reactive_mode_replay_matrix" "FAIL" "matrix_empty_or_missing_in_scenario" "$SCENARIO_PATH"
  mark_fail
elif [[ "$reactive_mode_matrix_failures" -eq 0 ]]; then
  log_status "reactive_mode_replay_matrix" "PASS" "mode_count=$reactive_mode_matrix_count all_modes_present" "$SCENARIO_PATH"
else
  log_status "reactive_mode_replay_matrix" "FAIL" "mode_count=$reactive_mode_matrix_count failure_count=$reactive_mode_matrix_failures" "$SCENARIO_PATH"
  mark_fail
fi

reactive_telemetry_fields_file="$OUT_DIR/reactive_telemetry_fields.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="reactive_telemetry_bounds_contract") | .expected_scene_fields[]?' "$SCENARIO_PATH" >"$reactive_telemetry_fields_file"
reactive_telemetry_fields=()
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  reactive_telemetry_fields+=("$token")
done <"$reactive_telemetry_fields_file"

missing_reactive_telemetry_fields=()
for token in "${reactive_telemetry_fields[@]}"; do
  if ! token_present_in_file "$token" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    missing_reactive_telemetry_fields+=("$token")
  fi
done
if [[ "${#missing_reactive_telemetry_fields[@]}" -eq 0 ]]; then
  log_status "reactive_telemetry_bounds_contract_fields" "PASS" "expected_count=${#reactive_telemetry_fields[@]} present" "Source/PluginProcessor.cpp"
else
  log_status "reactive_telemetry_bounds_contract_fields" "FAIL" "missing_fields=${missing_reactive_telemetry_fields[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

reactive_telemetry_norm_file="$OUT_DIR/reactive_telemetry_norm_fields.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="reactive_telemetry_bounds_contract") | .expected_norm_fields[]?' "$SCENARIO_PATH" >"$reactive_telemetry_norm_file"
reactive_telemetry_norm_fields=()
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  reactive_telemetry_norm_fields+=("$token")
done <"$reactive_telemetry_norm_file"

missing_reactive_telemetry_norm_fields=()
for token in "${reactive_telemetry_norm_fields[@]}"; do
  if ! token_present_in_file "$token" "$ROOT_DIR/Source/PluginProcessor.cpp"; then
    missing_reactive_telemetry_norm_fields+=("$token")
  fi
done
if [[ "${#missing_reactive_telemetry_norm_fields[@]}" -eq 0 ]]; then
  log_status "reactive_telemetry_bounds_contract_norm_fields" "PASS" "expected_count=${#reactive_telemetry_norm_fields[@]} present" "Source/PluginProcessor.cpp"
else
  log_status "reactive_telemetry_bounds_contract_norm_fields" "FAIL" "missing_fields=${missing_reactive_telemetry_norm_fields[*]}" "Source/PluginProcessor.cpp"
  mark_fail
fi

reactive_telemetry_ui_tokens_file="$OUT_DIR/reactive_telemetry_ui_tokens.txt"
jq -r '.bl029_contract_checks[]? | select(.id=="reactive_telemetry_bounds_contract") | .expected_ui_tokens[]?' "$SCENARIO_PATH" >"$reactive_telemetry_ui_tokens_file"
reactive_telemetry_ui_tokens=()
while IFS= read -r token; do
  [[ -z "$token" ]] && continue
  reactive_telemetry_ui_tokens+=("$token")
done <"$reactive_telemetry_ui_tokens_file"

missing_reactive_telemetry_ui_tokens=()
for token in "${reactive_telemetry_ui_tokens[@]}"; do
  if ! rg -Fq -- "$token" "$ROOT_DIR/Source/ui/public/js/index.js"; then
    missing_reactive_telemetry_ui_tokens+=("$token")
  fi
done
if [[ "${#missing_reactive_telemetry_ui_tokens[@]}" -eq 0 ]]; then
  log_status "reactive_telemetry_bounds_contract_ui_tokens" "PASS" "expected_count=${#reactive_telemetry_ui_tokens[@]} present" "Source/ui/public/js/index.js"
else
  log_status "reactive_telemetry_bounds_contract_ui_tokens" "FAIL" "missing_tokens=${missing_reactive_telemetry_ui_tokens[*]}" "Source/ui/public/js/index.js"
  mark_fail
fi

reactive_telemetry_min="$(jq -r '.bl029_contract_checks[]? | select(.id=="reactive_telemetry_bounds_contract") | .expected_range.min // "nan"' "$SCENARIO_PATH")"
reactive_telemetry_max="$(jq -r '.bl029_contract_checks[]? | select(.id=="reactive_telemetry_bounds_contract") | .expected_range.max // "nan"' "$SCENARIO_PATH")"
if python3 - "$reactive_telemetry_min" "$reactive_telemetry_max" <<'PY'
import math, sys
mn = float(sys.argv[1])
mx = float(sys.argv[2])
ok = math.isfinite(mn) and math.isfinite(mx) and 0.0 <= mn < mx <= 1.0
sys.exit(0 if ok else 1)
PY
then
  log_status "reactive_telemetry_bounds_contract_range" "PASS" "range=[${reactive_telemetry_min},${reactive_telemetry_max}]" "$SCENARIO_PATH"
else
  log_status "reactive_telemetry_bounds_contract_range" "FAIL" "invalid_range=[${reactive_telemetry_min},${reactive_telemetry_max}]" "$SCENARIO_PATH"
  mark_fail
fi

if [[ "${#missing_reactive_telemetry_fields[@]}" -eq 0 \
   && "${#missing_reactive_telemetry_norm_fields[@]}" -eq 0 \
   && "${#missing_reactive_telemetry_ui_tokens[@]}" -eq 0 ]]; then
  log_status "reactive_telemetry_bounds_contract" "PASS" "all_reactive_telemetry_checks_passed" "$SCENARIO_PATH"
else
  log_status "reactive_telemetry_bounds_contract" "FAIL" "field_missing=${#missing_reactive_telemetry_fields[@]} norm_missing=${#missing_reactive_telemetry_norm_fields[@]} ui_missing=${#missing_reactive_telemetry_ui_tokens[@]}" "$SCENARIO_PATH"
  mark_fail
fi

{
  echo "Title: BL-029 Audition Reactive Reliability Lane Contract"
  echo "Document Type: Test Evidence"
  echo "Author: APC Codex"
  echo "Created Date: $(date -u +%Y-%m-%d)"
  echo "Last Modified Date: $(date -u +%Y-%m-%d)"
  echo
  echo "# BL-029 Audition Reactive Reliability Lane Contract"
  echo
  echo "## Coverage"
  printf -- '- Scenario: `%s`\n' "qa/scenarios/locusq_audition_platform_showcase.json"
  printf -- '- QA binary: `%s`\n' "${QA_BIN#$ROOT_DIR/}"
  printf -- '- Replay runs: %s\n' "${REPLAY_RUNS}"
  echo
  echo "## Checks"
  awk -F'\t' 'NR>1 { printf("- %s: %s (%s)\n", $1, $2, $3) }' "$STATUS_TSV"
  echo
  echo "## Artifacts"
  printf -- '- Status TSV: `%s`\n' "${STATUS_TSV#$ROOT_DIR/}"
  printf -- '- Lane log: `%s`\n' "${QA_LANE_LOG#$ROOT_DIR/}"
  printf -- '- Scenario result log: `%s`\n' "${SCENARIO_RESULT_LOG#$ROOT_DIR/}"
} >"$LANE_CONTRACT_MD"

if [[ "$fail_count" -ne 0 ]]; then
  log_status "bl029_audition_platform_lane" "FAIL" "failure_count=$fail_count" "$STATUS_TSV"
  echo "artifact_dir=$OUT_DIR"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

log_status "bl029_audition_platform_lane" "PASS" "failure_count=0" "$STATUS_TSV"
echo "artifact_dir=$OUT_DIR"
printf "artifact_dir=%s\n" "$OUT_DIR" >&3
exit 0
