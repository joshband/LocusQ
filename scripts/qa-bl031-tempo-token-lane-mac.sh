#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

OUT_DIR="${BL031_OUT_DIR:-$ROOT_DIR/TestEvidence/bl031_slice_d_${TIMESTAMP}}"
SCENARIO_PATH="${BL031_SCENARIO_PATH:-$ROOT_DIR/qa/scenarios/locusq_bl031_tempo_ramp_suite.json}"
QA_BIN="${BL031_QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"
if [[ ! -x "$QA_BIN" ]]; then
  QA_BIN="${BL031_QA_BIN_FALLBACK:-$ROOT_DIR/build_local/locusq_qa_artefacts/locusq_qa}"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
QA_LANE_LOG="$OUT_DIR/qa_lane.log"
SCENARIO_RESULT_LOG="$OUT_DIR/scenario_result.log"
TOKEN_MONO_TSV="$OUT_DIR/token_monotonicity.tsv"
TOKEN_SUMMARY_JSON="$OUT_DIR/token_summary.json"
BUILD_LOG="$OUT_DIR/build.log"
SCENARIO_RUN_LOG="$OUT_DIR/scenario_run.log"
RESULT_COPY_JSON="$OUT_DIR/scenario_result.json"
TOKEN_MODEL_LOG="$OUT_DIR/token_model.log"

printf "check\tresult\tdetail\tartifact\n" >"$STATUS_TSV"
: >"$SCENARIO_RESULT_LOG"

# Keep human-readable progress in qa_lane.log while preserving concise stdout.
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

require_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_status "tool_${cmd}" "PASS" "$(command -v "$cmd")" ""
  else
    log_status "tool_${cmd}" "FAIL" "missing_command" ""
    mark_fail
  fi
}

echo "BL-031 deterministic tempo-ramp lane start: $DOC_TS"
echo "out_dir=$OUT_DIR"
echo "scenario=$SCENARIO_PATH"
echo "qa_bin=$QA_BIN"

require_cmd jq
require_cmd python3
require_cmd rg

if [[ ! -f "$SCENARIO_PATH" ]]; then
  log_status "scenario_file" "FAIL" "missing=$SCENARIO_PATH" "$SCENARIO_PATH"
  mark_fail
fi
if [[ ! -x "$QA_BIN" ]]; then
  log_status "qa_bin" "FAIL" "missing_or_not_executable=$QA_BIN" "$QA_BIN"
  mark_fail
else
  log_status "qa_bin" "PASS" "executable_found" "$QA_BIN"
fi

if [[ "$fail_count" -ne 0 ]]; then
  log_status "lane_prereq" "FAIL" "prerequisite_failure_count=$fail_count" "$STATUS_TSV"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

SCENARIO_ID="$(jq -r '.id // empty' "$SCENARIO_PATH")"
if [[ -z "$SCENARIO_ID" ]]; then
  log_status "scenario_id" "FAIL" "missing_id_in_scenario" "$SCENARIO_PATH"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 1
fi

if [[ "${BL031_SKIP_BUILD:-0}" == "1" ]]; then
  log_status "build_targets" "PASS" "skipped_by_env=BL031_SKIP_BUILD" "$BUILD_LOG"
else
  set +e
  cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8 >"$BUILD_LOG" 2>&1
  build_exit=$?
  set -e
  if [[ "$build_exit" -eq 0 ]]; then
    log_status "build_targets" "PASS" "cmake_build_exit=0" "$BUILD_LOG"
  else
    log_status "build_targets" "FAIL" "cmake_build_exit=${build_exit}" "$BUILD_LOG"
    mark_fail
  fi
fi

RUN_ARTIFACT_DIR="$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID"
if [[ "$fail_count" -eq 0 ]]; then
  rm -rf "$RUN_ARTIFACT_DIR"
  set +e
  "$QA_BIN" --spatial "$SCENARIO_PATH" >"$SCENARIO_RUN_LOG" 2>&1
  scenario_exec_exit=$?
  set -e

  if [[ "$scenario_exec_exit" -eq 0 ]]; then
    log_status "scenario_exec" "PASS" "qa_runner_exit=0" "$SCENARIO_RUN_LOG"
  else
    log_status "scenario_exec" "FAIL" "qa_runner_exit=${scenario_exec_exit}" "$SCENARIO_RUN_LOG"
    mark_fail
  fi
else
  log_status "scenario_exec" "FAIL" "skipped_due_to_previous_failures" "$SCENARIO_RUN_LOG"
  mark_fail
fi

RESULT_JSON=""
RESULT_KIND=""
if [[ "$fail_count" -eq 0 ]]; then
  result_candidates=(
    "$RUN_ARTIFACT_DIR/result.json"
    "$ROOT_DIR/qa_output/locusq_spatial/$SCENARIO_ID/suite_result.json"
    "$ROOT_DIR/qa_output/locusq_spatial/suite_result.json"
    "$ROOT_DIR/qa_output/suite_result.json"
  )
  for candidate in "${result_candidates[@]}"; do
    [[ -f "$candidate" ]] || continue
    if [[ "$candidate" == *"/suite_result.json" ]]; then
      suite_id="$(jq -r '.suite_id // empty' "$candidate" 2>/dev/null || true)"
      if [[ -n "$suite_id" && "$suite_id" != "$SCENARIO_ID" ]]; then
        continue
      fi
      RESULT_KIND="suite"
    else
      RESULT_KIND="scenario"
    fi
    RESULT_JSON="$candidate"
    break
  done
fi

if [[ "$fail_count" -eq 0 && -n "$RESULT_JSON" ]]; then
  cp "$RESULT_JSON" "$RESULT_COPY_JSON"
  result_status="$(jq -r '.status // "UNKNOWN"' "$RESULT_COPY_JSON")"
  if [[ "$RESULT_KIND" == "suite" ]]; then
    warnings_count="$(jq -r '.summary.warned // 0' "$RESULT_COPY_JSON")"
    deadline_status="SUITE_NA"
    allocation_status="SUITE_NA"
    signal_present="SUITE_NA"
    no_clipping="SUITE_NA"
    passed_count="$(jq -r '.summary.passed // 0' "$RESULT_COPY_JSON")"
    failed_count="$(jq -r '.summary.failed // 0' "$RESULT_COPY_JSON")"
    total_count="$(jq -r '.summary.total // 0' "$RESULT_COPY_JSON")"
  else
    warnings_count="$(jq -r '(.warnings // []) | length' "$RESULT_COPY_JSON")"
    deadline_status="$(jq -r '.metrics.deadline.status // "MISSING"' "$RESULT_COPY_JSON")"
    allocation_status="$(jq -r '.metrics.allocation_free.status // "MISSING"' "$RESULT_COPY_JSON")"
    signal_present="$(jq -r '.metrics.signal_present.value // "nan"' "$RESULT_COPY_JSON")"
    no_clipping="$(jq -r '.metrics.no_clipping.value // "nan"' "$RESULT_COPY_JSON")"
    passed_count="NA"
    failed_count="NA"
    total_count="NA"
  fi

  {
    printf "scenario_id=%s\n" "$SCENARIO_ID"
    printf "result_kind=%s\n" "$RESULT_KIND"
    printf "result_status=%s\n" "$result_status"
    printf "warnings_count=%s\n" "$warnings_count"
    printf "deadline_status=%s\n" "$deadline_status"
    printf "allocation_free_status=%s\n" "$allocation_status"
    printf "signal_present_value=%s\n" "$signal_present"
    printf "no_clipping_value=%s\n" "$no_clipping"
    printf "suite_passed=%s\n" "$passed_count"
    printf "suite_failed=%s\n" "$failed_count"
    printf "suite_total=%s\n" "$total_count"
    printf "result_json=%s\n" "$RESULT_COPY_JSON"
  } >>"$SCENARIO_RESULT_LOG"

  if [[ "$result_status" == "PASS" ]]; then
    log_status "scenario_status" "PASS" "result_kind=${RESULT_KIND} result_status=PASS warnings=${warnings_count}" "$RESULT_COPY_JSON"
  else
    log_status "scenario_status" "FAIL" "result_kind=${RESULT_KIND} result_status=${result_status} warnings=${warnings_count}" "$RESULT_COPY_JSON"
    mark_fail
  fi
else
  log_status "scenario_status" "FAIL" "missing_result_json_candidates_checked" "$SCENARIO_RUN_LOG"
  mark_fail
fi

SCHEDULER_SOURCE="$(jq -r '.bl031_contract_checks.source_contract.scheduler_source // empty' "$SCENARIO_PATH")"
if [[ -n "$SCHEDULER_SOURCE" ]]; then
  SCHEDULER_SOURCE_PATH="$ROOT_DIR/$SCHEDULER_SOURCE"
else
  SCHEDULER_SOURCE_PATH="$ROOT_DIR/Source/VisualTokenScheduler.h"
fi

required_tokens_file="$OUT_DIR/source_required_tokens.txt"
jq -r '.bl031_contract_checks.source_contract.required_tokens[]?' "$SCENARIO_PATH" >"$required_tokens_file"
missing_source_tokens=()
if [[ -f "$SCHEDULER_SOURCE_PATH" ]]; then
  while IFS= read -r token; do
    [[ -z "$token" ]] && continue
    if ! rg -Fq -- "$token" "$SCHEDULER_SOURCE_PATH"; then
      missing_source_tokens+=("$token")
    fi
  done <"$required_tokens_file"
else
  missing_source_tokens+=("missing_source_file:$SCHEDULER_SOURCE_PATH")
fi

if [[ "${#missing_source_tokens[@]}" -eq 0 ]]; then
  log_status "source_contract_tokens" "PASS" "required_tokens_present" "$SCHEDULER_SOURCE_PATH"
else
  log_status "source_contract_tokens" "FAIL" "missing_tokens=${missing_source_tokens[*]}" "$SCHEDULER_SOURCE_PATH"
  mark_fail
fi

set +e
python3 - "$SCENARIO_PATH" "$TOKEN_MONO_TSV" "$TOKEN_SUMMARY_JSON" >"$TOKEN_MODEL_LOG" 2>&1 <<'PY'
import json
import math
import statistics
import sys

scenario_path, out_tsv, out_summary = sys.argv[1:4]

with open(scenario_path, "r", encoding="utf-8") as fh:
    scenario = json.load(fh)

contract = scenario.get("bl031_contract_checks") or {}
defaults = contract.get("simulation_defaults") or {}
cases = contract.get("cases") or []

EPS = 1.0e-9


def to_float(value, fallback=0.0):
    try:
        converted = float(value)
    except (TypeError, ValueError):
        return fallback
    return converted if math.isfinite(converted) else fallback


def to_int(value, fallback=0):
    try:
        return int(value)
    except (TypeError, ValueError):
        return fallback


def is_finite_positive(value):
    return math.isfinite(value) and value > 0.0


def first_boundary_at_or_after(start_qn, anchor_qn, step_qn):
    if not (is_finite_positive(step_qn) and math.isfinite(start_qn) and math.isfinite(anchor_qn)):
        return math.inf
    step_count = math.ceil((start_qn - anchor_qn - EPS) / step_qn)
    boundary = anchor_qn + (step_count * step_qn)
    if boundary + EPS < start_qn:
        boundary += step_qn
    return boundary


def simulate_block(ppq_start, block_qn, bpm, playing, host_time_available, beat_len_qn, beats_per_bar_qn, subdivision_step_qn, max_tokens):
    tokens = []
    if not (playing and host_time_available and is_finite_positive(bpm) and block_qn > EPS):
        return tokens

    block_end = ppq_start + block_qn
    bar_anchor = math.floor(ppq_start / beats_per_bar_qn) * beats_per_bar_qn
    next_bar = first_boundary_at_or_after(ppq_start, bar_anchor, beats_per_bar_qn)
    next_beat = first_boundary_at_or_after(ppq_start, 0.0, beat_len_qn)
    next_sub = first_boundary_at_or_after(ppq_start, 0.0, subdivision_step_qn)

    for _ in range(128):
        if len(tokens) >= max_tokens:
            break
        next_boundary = min(next_bar, next_beat, next_sub)
        if not (next_boundary < (block_end - EPS)):
            break

        if abs(next_bar - next_boundary) <= EPS and len(tokens) < max_tokens:
            tokens.append((next_bar, 0))
            next_bar += beats_per_bar_qn
        if abs(next_beat - next_boundary) <= EPS and len(tokens) < max_tokens:
            tokens.append((next_beat, 1))
            next_beat += beat_len_qn
        if abs(next_sub - next_boundary) <= EPS and len(tokens) < max_tokens:
            tokens.append((next_sub, 2))
            next_sub += subdivision_step_qn

    return tokens


def average(values):
    if not values:
        return 0.0
    return float(sum(values) / len(values))


def evaluate_expectations(metrics, expectations):
    ok = True
    reasons = []

    def fail(message):
        nonlocal ok
        ok = False
        reasons.append(message)

    if expectations.get("ppq_non_decreasing") and not metrics["ppq_non_decreasing"]:
        fail("ppq_non_decreasing=false")

    if "require_token_types" in expectations:
        required = {int(x) for x in expectations["require_token_types"]}
        seen = set(metrics["token_types_seen"])
        if not required.issubset(seen):
            fail(f"missing_token_types={sorted(required - seen)}")

    if "min_total_tokens" in expectations:
        required_min = int(expectations["min_total_tokens"])
        if metrics["total_tokens"] < required_min:
            fail(f"total_tokens<{required_min}")

    if "total_tokens_equals" in expectations:
        expected_total = int(expectations["total_tokens_equals"])
        if metrics["total_tokens"] != expected_total:
            fail(f"total_tokens!={expected_total}")

    if "max_tokens_per_block" in expectations:
        max_allowed = int(expectations["max_tokens_per_block"])
        if metrics["max_tokens_observed"] > max_allowed:
            fail(f"max_tokens_per_block>{max_allowed}")

    if "stop_tokens_max" in expectations:
        stop_max = int(expectations["stop_tokens_max"])
        if metrics["stop_tokens"] > stop_max:
            fail(f"stop_tokens>{stop_max}")

    if "resume_tokens_min" in expectations:
        resume_min = int(expectations["resume_tokens_min"])
        if metrics["resume_tokens"] < resume_min:
            fail(f"resume_tokens<{resume_min}")

    if "density_growth_min_ratio" in expectations:
        ratio_min = float(expectations["density_growth_min_ratio"])
        if metrics["density_growth_ratio"] + EPS < ratio_min:
            fail(f"density_growth_ratio<{ratio_min}")

    if "beat_interval_ppq" in expectations:
        target = float(expectations["beat_interval_ppq"])
        tolerance = float(expectations.get("beat_interval_tolerance", 0.001))
        if metrics["beat_interval_count"] <= 0:
            fail("beat_interval_count=0")
        elif abs(metrics["beat_interval_mean_ppq"] - target) > tolerance:
            fail(f"beat_interval_mean={metrics['beat_interval_mean_ppq']:.6f}")

    return ok, reasons


rows = []
overall_pass = True
pass_count = 0
fail_count = 0

for case in cases:
    case_id = str(case.get("id", "unknown_case"))
    acceptance_id = str(case.get("acceptance_id", "n/a"))
    sample_rate = to_float(case.get("sample_rate_hz", defaults.get("sample_rate_hz", 48000.0)), 48000.0)
    block_size = max(1, to_int(case.get("block_size", defaults.get("block_size", 512)), 512))
    numerator = max(1, to_int(case.get("time_signature_numerator", defaults.get("time_signature_numerator", 4)), 4))
    denominator = max(1, to_int(case.get("time_signature_denominator", defaults.get("time_signature_denominator", 4)), 4))
    subdivision = max(1, to_int(case.get("subdivision_per_beat", defaults.get("subdivision_per_beat", 4)), 4))
    max_tokens = max(1, to_int(case.get("max_tokens_per_block", defaults.get("max_tokens_per_block", 32)), 32))

    beat_len_qn = 4.0 / float(denominator)
    beats_per_bar_qn = float(numerator) * beat_len_qn
    subdivision_step_qn = beat_len_qn / float(subdivision)
    block_sec = float(block_size) / float(sample_rate) if sample_rate > 0.0 else 0.0

    ppq = 0.0
    seq = 0
    previous_ppq = -math.inf
    ppq_non_decreasing = True
    token_types_seen = set()
    beat_ppqs = []
    max_tokens_observed = 0
    stop_blocks = 0
    stop_tokens = 0
    resume_tokens = 0
    seen_stop_segment = False
    density_samples = []
    errors = []

    segments = case.get("segments") or []
    for segment in segments:
        playing = bool(segment.get("transport_playing", True))
        host_time_available = bool(segment.get("host_time_available", True))
        tempo_start = to_float(segment.get("tempo_start_bpm", 120.0), 120.0)
        tempo_end = to_float(segment.get("tempo_end_bpm", tempo_start), tempo_start)

        def tempo_at(progress):
            return tempo_start + (tempo_end - tempo_start) * progress

        segment_has_ramp = abs(tempo_end - tempo_start) > EPS and playing and host_time_available

        if "bars" in segment:
            if not (playing and host_time_available):
                errors.append("bars_segment_requires_playing_host_time")
                continue
            bars = max(0.0, to_float(segment.get("bars", 0.0), 0.0))
            target_qn = bars * beats_per_bar_qn
            consumed_qn = 0.0
            guard = 0
            while consumed_qn < target_qn - EPS and guard < 200000:
                guard += 1
                progress = 0.0 if target_qn <= EPS else consumed_qn / target_qn
                bpm = tempo_at(progress)
                block_qn = (bpm * block_sec / 60.0) if (playing and host_time_available and is_finite_positive(bpm) and block_sec > 0.0) else 0.0
                if block_qn <= EPS:
                    errors.append("non_positive_block_qn")
                    break
                if consumed_qn + block_qn > target_qn:
                    block_qn = target_qn - consumed_qn

                block_tokens = simulate_block(
                    ppq_start=ppq,
                    block_qn=block_qn,
                    bpm=bpm,
                    playing=playing,
                    host_time_available=host_time_available,
                    beat_len_qn=beat_len_qn,
                    beats_per_bar_qn=beats_per_bar_qn,
                    subdivision_step_qn=subdivision_step_qn,
                    max_tokens=max_tokens,
                )

                block_count = len(block_tokens)
                max_tokens_observed = max(max_tokens_observed, block_count)
                if segment_has_ramp:
                    density_samples.append((progress, block_count))

                if not playing:
                    stop_blocks += 1
                    stop_tokens += block_count
                    seen_stop_segment = True
                elif seen_stop_segment:
                    resume_tokens += block_count

                for token_ppq, token_type in block_tokens:
                    seq += 1
                    if token_ppq + EPS < previous_ppq:
                        ppq_non_decreasing = False
                    previous_ppq = token_ppq
                    token_types_seen.add(int(token_type))
                    if int(token_type) == 1:
                        beat_ppqs.append(token_ppq)

                ppq += block_qn
                consumed_qn += block_qn

            if guard >= 200000:
                errors.append("segment_guard_exhausted")
        else:
            blocks = max(0, to_int(segment.get("blocks", 0), 0))
            for block_index in range(blocks):
                progress = 0.0 if blocks <= 1 else (float(block_index) / float(blocks - 1))
                bpm = tempo_at(progress)
                block_qn = (bpm * block_sec / 60.0) if (playing and host_time_available and is_finite_positive(bpm) and block_sec > 0.0) else 0.0

                block_tokens = simulate_block(
                    ppq_start=ppq,
                    block_qn=block_qn,
                    bpm=bpm,
                    playing=playing,
                    host_time_available=host_time_available,
                    beat_len_qn=beat_len_qn,
                    beats_per_bar_qn=beats_per_bar_qn,
                    subdivision_step_qn=subdivision_step_qn,
                    max_tokens=max_tokens,
                )

                block_count = len(block_tokens)
                max_tokens_observed = max(max_tokens_observed, block_count)
                if segment_has_ramp:
                    density_samples.append((progress, block_count))

                if not playing:
                    stop_blocks += 1
                    stop_tokens += block_count
                    seen_stop_segment = True
                elif seen_stop_segment:
                    resume_tokens += block_count

                for token_ppq, token_type in block_tokens:
                    seq += 1
                    if token_ppq + EPS < previous_ppq:
                        ppq_non_decreasing = False
                    previous_ppq = token_ppq
                    token_types_seen.add(int(token_type))
                    if int(token_type) == 1:
                        beat_ppqs.append(token_ppq)

                if block_qn > EPS:
                    ppq += block_qn

    beat_intervals = []
    for left, right in zip(beat_ppqs, beat_ppqs[1:]):
        delta = right - left
        if delta > EPS:
            beat_intervals.append(delta)

    head_counts = [count for progress, count in density_samples if progress <= 0.25]
    tail_counts = [count for progress, count in density_samples if progress >= 0.75]
    head_avg = average(head_counts)
    tail_avg = average(tail_counts)
    if head_avg <= EPS:
        density_growth_ratio = 1.0 if tail_avg <= EPS else math.inf
    else:
        density_growth_ratio = tail_avg / head_avg

    metrics = {
        "total_tokens": seq,
        "ppq_non_decreasing": ppq_non_decreasing,
        "sequence_strict": True,
        "token_types_seen": sorted(token_types_seen),
        "max_tokens_observed": max_tokens_observed,
        "stop_blocks": stop_blocks,
        "stop_tokens": stop_tokens,
        "resume_tokens": resume_tokens,
        "density_growth_ratio": density_growth_ratio,
        "beat_interval_mean_ppq": average(beat_intervals),
        "beat_interval_count": len(beat_intervals),
        "errors": errors,
    }

    case_expectations = case.get("expectations") or {}
    case_ok, case_reasons = evaluate_expectations(metrics, case_expectations)
    if errors:
        case_ok = False
        case_reasons.extend(errors)

    row = {
        "acceptance_id": acceptance_id,
        "case_id": case_id,
        "result": "PASS" if case_ok else "FAIL",
        "total_tokens": metrics["total_tokens"],
        "ppq_non_decreasing": str(metrics["ppq_non_decreasing"]).lower(),
        "sequence_strict": str(metrics["sequence_strict"]).lower(),
        "max_tokens_observed": metrics["max_tokens_observed"],
        "max_tokens_allowed": to_int(case_expectations.get("max_tokens_per_block", max_tokens), max_tokens),
        "stop_blocks": metrics["stop_blocks"],
        "stop_tokens": metrics["stop_tokens"],
        "resume_tokens": metrics["resume_tokens"],
        "density_growth_ratio": metrics["density_growth_ratio"],
        "beat_interval_mean_ppq": metrics["beat_interval_mean_ppq"],
        "details": "ok" if case_ok else ";".join(case_reasons),
    }
    rows.append(row)

    if case_ok:
        pass_count += 1
    else:
        overall_pass = False
        fail_count += 1

with open(out_tsv, "w", encoding="utf-8") as fh:
    fh.write("acceptance_id\tcase_id\tresult\ttotal_tokens\tppq_non_decreasing\tsequence_strict\tmax_tokens_observed\tmax_tokens_allowed\tstop_blocks\tstop_tokens\tresume_tokens\tdensity_growth_ratio\tbeat_interval_mean_ppq\tdetails\n")
    for row in rows:
        fh.write(
            f"{row['acceptance_id']}\t{row['case_id']}\t{row['result']}\t{row['total_tokens']}\t"
            f"{row['ppq_non_decreasing']}\t{row['sequence_strict']}\t{row['max_tokens_observed']}\t{row['max_tokens_allowed']}\t"
            f"{row['stop_blocks']}\t{row['stop_tokens']}\t{row['resume_tokens']}\t{row['density_growth_ratio']:.6f}\t"
            f"{row['beat_interval_mean_ppq']:.6f}\t{row['details']}\n"
        )

summary = {
    "scenario_id": scenario.get("id"),
    "overall_result": "PASS" if overall_pass else "FAIL",
    "case_count": len(rows),
    "pass_count": pass_count,
    "fail_count": fail_count,
}
with open(out_summary, "w", encoding="utf-8") as fh:
    json.dump(summary, fh, indent=2)

if overall_pass:
    print("token_model_result=PASS")
    sys.exit(0)

print("token_model_result=FAIL")
for row in rows:
    if row["result"] != "PASS":
        print(f"failed_case={row['case_id']} details={row['details']}")
sys.exit(1)
PY
token_model_exit=$?
set -e

if [[ "$token_model_exit" -eq 0 ]]; then
  log_status "token_monotonicity" "PASS" "deterministic_cases_passed" "$TOKEN_MONO_TSV"
else
  log_status "token_monotonicity" "FAIL" "deterministic_case_failure" "$TOKEN_MONO_TSV"
  mark_fail
fi

if [[ -f "$TOKEN_SUMMARY_JSON" ]]; then
  {
    echo ""
    echo "token_summary_json=$TOKEN_SUMMARY_JSON"
    jq -r '"token_model_overall=" + (.overall_result // "UNKNOWN") + " pass_count=" + ((.pass_count // 0)|tostring) + " fail_count=" + ((.fail_count // 0)|tostring)' "$TOKEN_SUMMARY_JSON" 2>/dev/null || true
  } >>"$SCENARIO_RESULT_LOG"
fi

if [[ "$fail_count" -eq 0 ]]; then
  log_status "lane_result" "PASS" "all_checks_passed" "$STATUS_TSV"
  printf "artifact_dir=%s\n" "$OUT_DIR" >&3
  exit 0
fi

log_status "lane_result" "FAIL" "failure_count=${fail_count}" "$STATUS_TSV"
printf "artifact_dir=%s\n" "$OUT_DIR" >&3
exit 1
