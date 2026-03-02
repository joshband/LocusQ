#!/usr/bin/env bash
# Title: BL-077 Unified Visual Capture Harness QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-02
# Last Modified Date: 2026-03-02
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl077_capture_harness_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=""
RUNS_SET=0
PROFILE_NAME="dense"
LIVE_CAPTURE=0

STATUS_TSV=""
CAPTURE_CONTRACT_MATRIX_TSV=""
CUE_PROFILE_MATRIX_TSV=""
ARTIFACT_SCHEMA_INVENTORY_TSV=""
REPLAY_HASHES_TSV=""
INTEGRATION_CONSUMERS_TSV=""
EXTENSION_CONTRACT_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl077-capture-harness-mac.sh [options]

BL-077 unified visual capture harness scaffold lane.

Options:
  --out-dir <path>     Artifact output directory
  --contract-only      Contract checks only (default)
  --execute            Execute-mode gate checks (fails while TODO rows remain)
  --runs <N>           Number of replay runs (default: 3 for contract-only, 1 for execute)
  --profile <name>     Capture profile name (default: dense)
  --live-capture       In execute mode, run live screen capture instead of dry-run probes
  --help, -h           Show usage

Outputs:
  status.tsv
  capture_contract_matrix.tsv
  cue_profile_matrix.tsv
  artifact_schema_inventory.tsv
  replay_hashes.tsv
  integration_consumers.tsv
  extension_contract.md
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

count_integration_blockers() {
  local file="$1"
  [[ -f "$file" ]] || {
    echo 0
    return
  }

  awk -F'\t' '
    NR == 1 { next }
    $3 == "yes" && $2 != "PASS" { count++ }
    END { print count + 0 }
  ' "$file"
}

get_hash_value() {
  local tsv_file="$1"
  local key="$2"
  if [[ ! -f "$tsv_file" ]]; then
    echo ""
    return
  fi
  awk -F'\t' -v wanted="$key" 'NR > 1 && $1 == wanted { print $2; exit }' "$tsv_file"
}

count_fail_or_todo_rows() {
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
        if ($i == "FAIL" || $i == "TODO")
        {
          count++
          break
        }
      }
    }
    END { print count + 0 }
  ' "$file"
}

count_missing_required_artifacts() {
  local inventory_file="$1"
  [[ -f "$inventory_file" ]] || {
    echo 0
    return
  }

  awk -F'\t' '
    NR == 1 { next }
    $3 == "yes" && $4 != "yes" { count++ }
    END { print count + 0 }
  ' "$inventory_file"
}

run_capture_probe() {
  local run_id="$1"
  local probe_mode="$2"

  local run_dir="${OUT_DIR}/run_${run_id}"
  mkdir -p "$run_dir"

  local args=(--out-dir "$run_dir" --profile "$PROFILE_NAME")

  if [[ "$probe_mode" == "contract_only" ]]; then
    args+=(--dry-run --no-cues)
  elif [[ "$probe_mode" == "execute" && "$LIVE_CAPTURE" -eq 0 ]]; then
    args+=(--dry-run --no-cues)
  fi

  set +e
  "${ROOT_DIR}/scripts/capture-headtracking-rotation-mac.sh" "${args[@]}" >"${run_dir}/stdout.log" 2>"${run_dir}/stderr.log"
  local exit_code=$?
  set -e

  echo "${exit_code};${run_dir}"
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
      RUNS_SET=1
      shift 2
      ;;
    --profile)
      [[ $# -ge 2 ]] || usage_error "--profile requires a value"
      PROFILE_NAME="$2"
      shift 2
      ;;
    --live-capture)
      LIVE_CAPTURE=1
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

if (( RUNS_SET == 0 )); then
  if [[ "$MODE" == "contract_only" ]]; then
    RUNS=3
  else
    RUNS=1
  fi
fi

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -le 0 ]]; then
  usage_error "--runs must be a positive integer"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
CAPTURE_CONTRACT_MATRIX_TSV="${OUT_DIR}/capture_contract_matrix.tsv"
CUE_PROFILE_MATRIX_TSV="${OUT_DIR}/cue_profile_matrix.tsv"
ARTIFACT_SCHEMA_INVENTORY_TSV="${OUT_DIR}/artifact_schema_inventory.tsv"
REPLAY_HASHES_TSV="${OUT_DIR}/replay_hashes.tsv"
INTEGRATION_CONSUMERS_TSV="${OUT_DIR}/integration_consumers.tsv"
EXTENSION_CONTRACT_MD="${OUT_DIR}/extension_contract.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run_id\tmode\tresult\texit_code\tprofile_contract_hash\tcue_schedule_hash\tartifact_contract_hash\tartifact_presence_hash\tpostprocess_contract_hash\trequired_missing_rows\tpostprocess_nonpass_rows\trun_dir\n" > "$CAPTURE_CONTRACT_MATRIX_TSV"
printf "profile\tresult\texit_code\tcue_count\tcue_schedule_hash\tartifact_dir\n" > "$CUE_PROFILE_MATRIX_TSV"
printf "run_id\tartifact_id\trelative_path\trequired_in_execute\tpresent\tsize_bytes\tsha256\tnotes\n" > "$ARTIFACT_SCHEMA_INVENTORY_TSV"
printf "run_id\tkey\tvalue\tdetail\n" > "$REPLAY_HASHES_TSV"
printf "consumer_id\tstatus\tblocking\tdetail\tartifact\n" > "$INTEGRATION_CONSUMERS_TSV"

CAPTURE_SCRIPT="${ROOT_DIR}/scripts/capture-headtracking-rotation-mac.sh"
PROFILE_DIR="${ROOT_DIR}/scripts/capture_profiles"
SELECTED_PROFILE_FILE="${PROFILE_DIR}/${PROFILE_NAME}.json"
BL058_SCRIPT="${ROOT_DIR}/scripts/qa-bl058-companion-profile-acquisition-mac.sh"

if [[ -x "$CAPTURE_SCRIPT" ]]; then
  record "BL077-PRE-capture_script" "PASS" "capture script is executable" "$CAPTURE_SCRIPT"
else
  record "BL077-PRE-capture_script" "FAIL" "capture script missing or not executable" "$CAPTURE_SCRIPT"
fi

if [[ -f "$SELECTED_PROFILE_FILE" ]]; then
  record "BL077-PRE-profile_exists" "PASS" "selected profile exists" "$SELECTED_PROFILE_FILE"
else
  record "BL077-PRE-profile_exists" "FAIL" "selected profile missing" "$SELECTED_PROFILE_FILE"
fi

for profile in coarse dense; do
  profile_out="${OUT_DIR}/profile_${profile}"
  mkdir -p "$profile_out"

  set +e
  "$CAPTURE_SCRIPT" --out-dir "$profile_out" --profile "$profile" --dry-run --no-cues >"${profile_out}/stdout.log" 2>"${profile_out}/stderr.log"
  profile_ec=$?
  set -e

  cue_count=0
  if [[ -f "${profile_out}/checkpoints.tsv" ]]; then
    cue_count="$(awk 'NR>1{count++} END {print count + 0}' "${profile_out}/checkpoints.tsv")"
  fi
  cue_hash="$(get_hash_value "${profile_out}/replay_hashes.tsv" "cue_schedule_hash")"

  profile_result="PASS"
  if [[ "$profile_ec" -ne 0 || "$cue_count" -le 0 || -z "$cue_hash" ]]; then
    profile_result="FAIL"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$profile" \
    "$profile_result" \
    "$profile_ec" \
    "$cue_count" \
    "$cue_hash" \
    "$profile_out" \
    >> "$CUE_PROFILE_MATRIX_TSV"

  if [[ "$profile_result" == "PASS" ]]; then
    record "BL077-CUE-${profile}" "PASS" "profile validates with non-empty cue schedule" "$profile_out"
  else
    record "BL077-CUE-${profile}" "FAIL" "profile validation failed" "$profile_out"
  fi
done

for ((run_idx=1; run_idx<=RUNS; run_idx++)); do
  run_id="run_$(printf '%02d' "$run_idx")"
  run_triplet="$(run_capture_probe "$run_id" "$MODE")"

  IFS=';' read -r run_exit run_dir <<< "$run_triplet"

  manifest_file="${run_dir}/session_manifest.json"
  inventory_file="${run_dir}/artifact_schema_inventory.tsv"
  hashes_file="${run_dir}/replay_hashes.tsv"
  checkpoint_map_file="${run_dir}/checkpoint_frame_map.tsv"
  contact_sheets_file="${run_dir}/contact_sheets.tsv"
  cue_clips_file="${run_dir}/cue_window_clips.tsv"

  profile_hash="$(get_hash_value "$hashes_file" "profile_contract_hash")"
  cue_hash="$(get_hash_value "$hashes_file" "cue_schedule_hash")"
  artifact_contract_hash="$(get_hash_value "$hashes_file" "artifact_contract_hash")"
  artifact_presence_hash="$(get_hash_value "$hashes_file" "artifact_presence_hash")"
  postprocess_contract_hash="$(get_hash_value "$hashes_file" "postprocess_contract_hash")"
  required_missing_rows="$(count_missing_required_artifacts "$inventory_file")"
  postprocess_nonpass_rows="$(( \
    $(count_fail_or_todo_rows "$checkpoint_map_file") \
    + $(count_fail_or_todo_rows "$contact_sheets_file") \
    + $(count_fail_or_todo_rows "$cue_clips_file") \
  ))"

  run_result="PASS"

  if [[ "$run_exit" -ne 0 ]]; then
    run_result="FAIL"
  elif [[ ! -f "$manifest_file" || ! -f "$inventory_file" || ! -f "$hashes_file" || ! -f "$checkpoint_map_file" || ! -f "$contact_sheets_file" || ! -f "$cue_clips_file" ]]; then
    run_result="FAIL"
  elif [[ "$MODE" == "execute" && "$LIVE_CAPTURE" -eq 0 ]]; then
    run_result="TODO"
  elif [[ "$MODE" == "execute" && "$required_missing_rows" -gt 0 ]]; then
    run_result="FAIL"
  elif [[ "$MODE" == "execute" && "$LIVE_CAPTURE" -eq 1 && "$postprocess_nonpass_rows" -gt 0 ]]; then
    run_result="FAIL"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_id" \
    "$MODE" \
    "$run_result" \
    "$run_exit" \
    "$profile_hash" \
    "$cue_hash" \
    "$artifact_contract_hash" \
    "$artifact_presence_hash" \
    "$postprocess_contract_hash" \
    "$required_missing_rows" \
    "$postprocess_nonpass_rows" \
    "$run_dir" \
    >> "$CAPTURE_CONTRACT_MATRIX_TSV"

  if [[ -f "$inventory_file" ]]; then
    awk -F'\t' -v run_id="$run_id" 'NR > 1 { print run_id"\t"$0 }' "$inventory_file" >> "$ARTIFACT_SCHEMA_INVENTORY_TSV"
  fi
  if [[ -f "$hashes_file" ]]; then
    awk -F'\t' -v run_id="$run_id" 'NR > 1 { print run_id"\t"$0 }' "$hashes_file" >> "$REPLAY_HASHES_TSV"
  fi

  if [[ "$run_result" == "PASS" ]]; then
    record "BL077-RUN-${run_id}" "PASS" "capture probe completed" "$run_dir"
  elif [[ "$run_result" == "TODO" ]]; then
    record "BL077-RUN-${run_id}" "PASS" "execute mode currently running dry-run probe; use --live-capture for real capture" "$run_dir"
  else
    record "BL077-RUN-${run_id}" "FAIL" "capture probe failed" "$run_dir"
  fi
done

hash_unique_profile="$(awk -F'\t' 'NR>1 && $3 != "FAIL" { if ($5 != "") seen[$5]=1 } END { c=0; for (k in seen) c++; print c+0 }' "$CAPTURE_CONTRACT_MATRIX_TSV")"
hash_unique_cue="$(awk -F'\t' 'NR>1 && $3 != "FAIL" { if ($6 != "") seen[$6]=1 } END { c=0; for (k in seen) c++; print c+0 }' "$CAPTURE_CONTRACT_MATRIX_TSV")"
hash_unique_contract="$(awk -F'\t' 'NR>1 && $3 != "FAIL" { if ($7 != "") seen[$7]=1 } END { c=0; for (k in seen) c++; print c+0 }' "$CAPTURE_CONTRACT_MATRIX_TSV")"
hash_unique_postprocess="$(awk -F'\t' 'NR>1 && $3 != "FAIL" { if ($9 != "") seen[$9]=1 } END { c=0; for (k in seen) c++; print c+0 }' "$CAPTURE_CONTRACT_MATRIX_TSV")"

if [[ "$hash_unique_profile" -eq 1 && "$hash_unique_cue" -eq 1 && "$hash_unique_contract" -eq 1 && "$hash_unique_postprocess" -eq 1 ]]; then
  record "BL077-C1-hash_stability" "PASS" "profile/cue/artifact/postprocess contract hashes are stable across replay runs" "$CAPTURE_CONTRACT_MATRIX_TSV"
else
  record "BL077-C1-hash_stability" "FAIL" "hash stability mismatch profile=${hash_unique_profile} cue=${hash_unique_cue} contract=${hash_unique_contract} postprocess=${hash_unique_postprocess}" "$CAPTURE_CONTRACT_MATRIX_TSV"
fi

BL058_CONSUMER_OUT="${OUT_DIR}/integration_bl058"
BL058_CONSUMER_STATUS="${BL058_CONSUMER_OUT}/status.tsv"
bl058_consumer_status="FAIL"
bl058_consumer_detail="BL-058 consumer lane not executed"

if [[ -x "$BL058_SCRIPT" ]]; then
  mkdir -p "$BL058_CONSUMER_OUT"
  set +e
  "$BL058_SCRIPT" --contract-only --out-dir "$BL058_CONSUMER_OUT" >"${BL058_CONSUMER_OUT}/stdout.log" 2>"${BL058_CONSUMER_OUT}/stderr.log"
  bl058_ec=$?
  set -e

  if [[ "$bl058_ec" -eq 0 && -f "$BL058_CONSUMER_STATUS" ]] && awk -F'\t' 'NR > 1 && $1 == "lane_result" && $2 == "PASS" { found=1 } END { exit(found ? 0 : 1) }' "$BL058_CONSUMER_STATUS"; then
    bl058_consumer_status="PASS"
    bl058_consumer_detail="BL-058 contract lane consumes BL-077 capture harness"
  else
    bl058_consumer_status="FAIL"
    bl058_consumer_detail="BL-058 lane did not produce PASS consumer result (exit=${bl058_ec})"
  fi
else
  bl058_consumer_status="FAIL"
  bl058_consumer_detail="BL-058 script missing or not executable"
fi

printf "BL058\t%s\tyes\t%s\t%s\n" \
  "$bl058_consumer_status" \
  "$bl058_consumer_detail" \
  "$BL058_CONSUMER_OUT" \
  >> "$INTEGRATION_CONSUMERS_TSV"

if [[ "$bl058_consumer_status" == "PASS" ]]; then
  record "BL077-I1-bl058_consumer" "PASS" "$bl058_consumer_detail" "$INTEGRATION_CONSUMERS_TSV"
else
  record "BL077-I1-bl058_consumer" "FAIL" "$bl058_consumer_detail" "$INTEGRATION_CONSUMERS_TSV"
fi

printf "BL059\tPLANNED\tno\tconsumer integration planned for Wave C follow-on\t-\n" >> "$INTEGRATION_CONSUMERS_TSV"
printf "BL060\tPLANNED\tno\tconsumer integration planned for Wave C follow-on\t-\n" >> "$INTEGRATION_CONSUMERS_TSV"
printf "BL067\tPLANNED\tno\tconsumer integration planned for Wave C follow-on\t-\n" >> "$INTEGRATION_CONSUMERS_TSV"
printf "BL068\tPLANNED\tno\tconsumer integration planned for Wave C follow-on\t-\n" >> "$INTEGRATION_CONSUMERS_TSV"
printf "BL074\tPLANNED\tno\tconsumer integration planned for Wave C follow-on\t-\n" >> "$INTEGRATION_CONSUMERS_TSV"

cat > "$EXTENSION_CONTRACT_MD" <<EOF_EXTENSION
Title: BL-077 Extension Contract (audio-plugin-coder + audio-dsp-qa-harness)
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DATE_UTC}
Last Modified Date: ${DATE_UTC}

# BL-077 Extension Contract

- timestamp_utc: ${TIMESTAMP}
- mode: ${MODE}
- profile: ${PROFILE_NAME}

## Contract

1. Invocation stays CLI-first (\`capture-headtracking-rotation-mac.sh\`), so downstream harnesses can call without plugin-internal imports.
2. Profile schema is declarative JSON (\`locusq-capture-profile-v1\`) with deterministic cue-point ordering.
3. Session artifacts always include machine-readable manifest, artifact inventory, replay hash tables, checkpoint-frame maps, contact-sheet indices, and cue-window clip indices.
4. Execute-mode promotion remains blocked while required integration consumers (blocking=yes) are unresolved.
EOF_EXTENSION

record "BL077-C2-extension_contract" "PASS" "extension contract artifact written" "$EXTENSION_CONTRACT_MD"

todo_rows_capture="$(count_todo_rows "$CAPTURE_CONTRACT_MATRIX_TSV")"
integration_blockers="$(count_integration_blockers "$INTEGRATION_CONSUMERS_TSV")"

if [[ "$MODE" == "execute" ]]; then
  if [[ "$todo_rows_capture" -gt 0 || "$integration_blockers" -gt 0 ]]; then
    record "BL077-E1-execute_todo_rows" "FAIL" "execute mode requires zero TODO run rows and zero required integration blockers (todo_rows=${todo_rows_capture},integration_blockers=${integration_blockers})" "$STATUS_TSV"
  else
    record "BL077-E1-execute_todo_rows" "PASS" "execute mode has zero TODO rows" "$STATUS_TSV"
  fi
else
  record "BL077-C3-contract_mode" "PASS" "contract-only mode completed scaffold checks (todo_rows=${todo_rows_capture},integration_blockers=${integration_blockers})" "$STATUS_TSV"
fi

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "mode=${MODE};bl077_contract_scaffold_pass" "$STATUS_TSV"
else
  record "lane_result" "FAIL" "mode=${MODE};failures=${fail_count}" "$STATUS_TSV"
fi

echo "Artifacts:"
echo "- $STATUS_TSV"
echo "- $CAPTURE_CONTRACT_MATRIX_TSV"
echo "- $CUE_PROFILE_MATRIX_TSV"
echo "- $ARTIFACT_SCHEMA_INVENTORY_TSV"
echo "- $REPLAY_HASHES_TSV"
echo "- $INTEGRATION_CONSUMERS_TSV"
echo "- $EXTENSION_CONTRACT_MD"

if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi

exit 0
