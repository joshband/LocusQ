#!/usr/bin/env bash
# Title: BL-057 Device Preset Library QA Lane
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-03-03
# Last Modified Date: 2026-03-03
#
# Exit codes:
#   0 all checks passed
#   1 one or more checks failed
#   2 usage/configuration error

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_DIR="${TMPDIR:-/tmp}/locusq_bl057_device_preset_${TIMESTAMP}"
MODE="contract_only"
MODE_SET=0
RUNS=1

STATUS_TSV=""
PRESET_INVENTORY_TSV=""
SWEEP_RESULTS_TSV=""
RUN_SUMMARY_TSV=""
SUMMARY_MD=""

pass_count=0
fail_count=0

usage() {
  cat <<'USAGE'
Usage: qa-bl057-device-preset-library-mac.sh [options]

BL-057 device preset library lane.

Options:
  --out-dir <path>   Artifact output directory
  --contract-only    Contract checks only (default)
  --execute          Execute-mode gate checks
  --runs <N>         Number of deterministic replay runs (default: 1)
  --help, -h         Show usage

Outputs:
  status.tsv
  preset_inventory.tsv
  sweep_results.tsv
  run_summary.tsv
  summary.md
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

ensure_positive_int() {
  local value="$1"
  [[ "$value" =~ ^[0-9]+$ ]] || return 1
  [[ "$value" -ge 1 ]] || return 1
  return 0
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
      ensure_positive_int "$2" || usage_error "--runs must be a positive integer"
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

mkdir -p "$OUT_DIR"

STATUS_TSV="${OUT_DIR}/status.tsv"
PRESET_INVENTORY_TSV="${OUT_DIR}/preset_inventory.tsv"
SWEEP_RESULTS_TSV="${OUT_DIR}/sweep_results.tsv"
RUN_SUMMARY_TSV="${OUT_DIR}/run_summary.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check_id\tresult\tdetail\tartifact\n" > "$STATUS_TSV"
printf "run\tpreset_file\thp_model_id\thp_mode\tpreamp_db\tfilter_count\tresult\tdetail\n" > "$PRESET_INVENTORY_TSV"
printf "run\tpreset_file\tsample_rate_hz\tmax_abs_db_top_10pct_nyquist\tnyquist_db\tthreshold_db\tresult\tdetail\n" > "$SWEEP_RESULTS_TSV"
printf "run\tmode\tresult\tinventory_fail_rows\tsweep_fail_rows\tdetail\n" > "$RUN_SUMMARY_TSV"

EQ_DIR="${ROOT_DIR}/Resources/eq_presets"
EXPECTED_FILES=(
  "airpods_pro_1_anc_on.yaml"
  "airpods_pro_1_anc_off.yaml"
  "airpods_pro_1_transparency.yaml"
  "airpods_pro_2_anc_on.yaml"
  "airpods_pro_2_anc_off.yaml"
  "airpods_pro_2_transparency.yaml"
  "airpods_pro_3_anc_on.yaml"
  "airpods_pro_3_anc_off.yaml"
  "airpods_pro_3_transparency.yaml"
  "sony_wh1000xm5_anc_on.yaml"
  "sony_wh1000xm5_anc_off.yaml"
)

if [[ -d "$EQ_DIR" ]]; then
  record "BL057-C1-eq_dir_exists" "PASS" "eq preset directory exists" "$EQ_DIR"
else
  record "BL057-C1-eq_dir_exists" "FAIL" "eq preset directory missing" "$EQ_DIR"
fi

for preset_file in "${EXPECTED_FILES[@]}"; do
  preset_path="${EQ_DIR}/${preset_file}"
  if [[ -f "$preset_path" ]]; then
    record "BL057-C2-file-${preset_file}" "PASS" "preset file exists" "$preset_path"
  else
    record "BL057-C2-file-${preset_file}" "FAIL" "preset file missing" "$preset_path"
  fi

done

for preset_file in "${EXPECTED_FILES[@]}"; do
  preset_path="${EQ_DIR}/${preset_file}"
  if [[ -f "$preset_path" ]]; then
    if rg -q '^preamp_db:[[:space:]]*[-+]?[0-9]+(\.[0-9]+)?[[:space:]]*$' "$preset_path"; then
      record "BL057-C3-preamp-${preset_file}" "PASS" "preamp_db field present" "$preset_path"
    else
      record "BL057-C3-preamp-${preset_file}" "FAIL" "preamp_db field missing or invalid" "$preset_path"
    fi
  fi

done

SONY_ON="${EQ_DIR}/sony_wh1000xm5_anc_on.yaml"
SONY_OFF="${EQ_DIR}/sony_wh1000xm5_anc_off.yaml"
if [[ -f "$SONY_ON" && -f "$SONY_OFF" ]]; then
  if rg -q '^hp_mode:[[:space:]]*anc_on[[:space:]]*$' "$SONY_ON" && rg -q '^hp_mode:[[:space:]]*anc_off[[:space:]]*$' "$SONY_OFF"; then
    record "BL057-C4-sony_anc_mode_headers" "PASS" "WH-1000XM5 ANC-on/off mode headers present" "$EQ_DIR"
  else
    record "BL057-C4-sony_anc_mode_headers" "FAIL" "WH-1000XM5 ANC mode headers missing/mismatched" "$EQ_DIR"
  fi

  if cmp -s "$SONY_ON" "$SONY_OFF"; then
    record "BL057-C5-sony_anc_split" "FAIL" "WH-1000XM5 ANC-on/off presets are byte-identical" "$EQ_DIR"
  else
    record "BL057-C5-sony_anc_split" "PASS" "WH-1000XM5 ANC-on/off presets are distinct" "$EQ_DIR"
  fi
else
  record "BL057-C4-sony_anc_mode_headers" "FAIL" "cannot validate ANC split because one or both WH-1000XM5 files are missing" "$EQ_DIR"
fi

run_validation_once() {
  local run_index="$1"
  local run_inventory="${OUT_DIR}/run_${run_index}_preset_inventory.tsv"
  local run_sweep="${OUT_DIR}/run_${run_index}_sweep_results.tsv"

  set +e
  python3 - "$EQ_DIR" "$run_index" "$run_inventory" "$run_sweep" <<'PY'
import math
import os
import re
import sys

if len(sys.argv) != 5:
    raise SystemExit(2)

eq_dir = sys.argv[1]
run_index = sys.argv[2]
out_inventory = sys.argv[3]
out_sweep = sys.argv[4]

expected_pairs = [
    ("airpods_pro_1", "anc_on"),
    ("airpods_pro_1", "anc_off"),
    ("airpods_pro_1", "transparency"),
    ("airpods_pro_2", "anc_on"),
    ("airpods_pro_2", "anc_off"),
    ("airpods_pro_2", "transparency"),
    ("airpods_pro_3", "anc_on"),
    ("airpods_pro_3", "anc_off"),
    ("airpods_pro_3", "transparency"),
    ("sony_wh1000xm5", "anc_on"),
    ("sony_wh1000xm5", "anc_off"),
]

sample_rates = (44100.0, 48000.0, 96000.0)
threshold_db = 3.0


def parse_scalar(value):
    value = value.strip().strip('"').strip("'")
    return value


def parse_preset(path):
    data = {"filters": []}
    errors = []
    in_filters = False

    with open(path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            if line.startswith("filters:"):
                in_filters = True
                continue

            if in_filters and line.startswith("-"):
                match = re.match(r"-\s*\{(.*)\}\s*$", line)
                if not match:
                    errors.append("unsupported_filter_syntax")
                    continue

                fields = {}
                for part in match.group(1).split(","):
                    if ":" not in part:
                        continue
                    key, value = part.split(":", 1)
                    fields[key.strip()] = parse_scalar(value)

                for key in ("type", "fc_hz", "gain_db", "q"):
                    if key not in fields:
                        errors.append(f"filter_missing_{key}")

                if any(key not in fields for key in ("type", "fc_hz", "gain_db", "q")):
                    continue

                try:
                    filter_entry = {
                        "type": fields["type"],
                        "fc_hz": float(fields["fc_hz"]),
                        "gain_db": float(fields["gain_db"]),
                        "q": float(fields["q"]),
                    }
                except ValueError:
                    errors.append("filter_numeric_parse_error")
                    continue

                data["filters"].append(filter_entry)
                continue

            if ":" in line and not line.startswith("-"):
                key, value = line.split(":", 1)
                data[key.strip()] = parse_scalar(value)

    required = ("hp_model_id", "hp_mode", "preamp_db")
    for key in required:
        if key not in data:
            errors.append(f"missing_{key}")

    if "preamp_db" in data:
        try:
            data["preamp_db"] = float(data["preamp_db"])
        except ValueError:
            errors.append("invalid_preamp_db")

    return data, errors


def biquad_coefficients(filter_entry, sample_rate):
    filter_type = filter_entry["type"].upper()
    fc_hz = float(filter_entry["fc_hz"])
    gain_db = float(filter_entry["gain_db"])
    q_value = max(float(filter_entry["q"]), 1.0e-4)

    nyquist = sample_rate * 0.5
    fc_hz = min(max(fc_hz, 1.0), nyquist * 0.999999)

    A = 10.0 ** (gain_db / 40.0)
    w0 = 2.0 * math.pi * fc_hz / sample_rate
    cos_w0 = math.cos(w0)
    sin_w0 = math.sin(w0)

    if filter_type == "PK":
        alpha = sin_w0 / (2.0 * q_value)
        b0 = 1.0 + alpha * A
        b1 = -2.0 * cos_w0
        b2 = 1.0 - alpha * A
        a0 = 1.0 + alpha / A
        a1 = -2.0 * cos_w0
        a2 = 1.0 - alpha / A
    elif filter_type in ("LSC", "LS", "LOW_SHELF"):
        slope = max(q_value, 0.1)
        alpha = (sin_w0 / 2.0) * math.sqrt((A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0)
        sqrt_A = math.sqrt(A)
        b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * sqrt_A * alpha)
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0)
        b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * sqrt_A * alpha)
        a0 = (A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * sqrt_A * alpha
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0)
        a2 = (A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * sqrt_A * alpha
    elif filter_type in ("HSC", "HS", "HIGH_SHELF"):
        slope = max(q_value, 0.1)
        alpha = (sin_w0 / 2.0) * math.sqrt((A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0)
        sqrt_A = math.sqrt(A)
        b0 = A * ((A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * sqrt_A * alpha)
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0)
        b2 = A * ((A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * sqrt_A * alpha)
        a0 = (A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * sqrt_A * alpha
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0)
        a2 = (A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * sqrt_A * alpha
    else:
        raise ValueError(f"unsupported_filter_type:{filter_type}")

    return b0, b1, b2, a0, a1, a2


def response_db(preset, sample_rate, freq_hz):
    omega = 2.0 * math.pi * freq_hz / sample_rate
    z1 = complex(math.cos(-omega), math.sin(-omega))
    z2 = z1 * z1

    response = 10.0 ** (float(preset.get("preamp_db", 0.0)) / 20.0)

    for filter_entry in preset["filters"]:
        b0, b1, b2, a0, a1, a2 = biquad_coefficients(filter_entry, sample_rate)
        numerator = b0 + b1 * z1 + b2 * z2
        denominator = a0 + a1 * z1 + a2 * z2
        if abs(denominator) < 1.0e-20:
            return float("inf")
        response *= numerator / denominator

    magnitude = abs(response)
    if magnitude < 1.0e-20:
        return -400.0
    return 20.0 * math.log10(magnitude)


inventory_lines = []
sweep_lines = []
failed = False

for model_id, mode in expected_pairs:
    preset_file = f"{model_id}_{mode}.yaml"
    preset_path = os.path.join(eq_dir, preset_file)

    if not os.path.isfile(preset_path):
        failed = True
        inventory_lines.append(
            "\t".join([
                run_index,
                preset_file,
                "n/a",
                "n/a",
                "n/a",
                "0",
                "FAIL",
                "preset_file_missing",
            ])
        )
        for sample_rate in sample_rates:
            sweep_lines.append(
                "\t".join([
                    run_index,
                    preset_file,
                    str(int(sample_rate)),
                    "nan",
                    "nan",
                    f"{threshold_db:.1f}",
                    "FAIL",
                    "preset_file_missing",
                ])
            )
        continue

    preset, errors = parse_preset(preset_path)

    detail_reasons = []
    if errors:
        detail_reasons.extend(errors)

    if preset.get("hp_model_id") != model_id:
        detail_reasons.append("hp_model_id_mismatch")
    if preset.get("hp_mode") != mode:
        detail_reasons.append("hp_mode_mismatch")
    if not preset.get("filters"):
        detail_reasons.append("filters_empty")

    inventory_result = "PASS" if not detail_reasons else "FAIL"
    if inventory_result == "FAIL":
        failed = True

    preamp_value = preset.get("preamp_db", "n/a")
    if isinstance(preamp_value, float):
        preamp_repr = f"{preamp_value:.3f}"
    else:
        preamp_repr = "n/a"

    inventory_lines.append(
        "\t".join([
            run_index,
            preset_file,
            str(preset.get("hp_model_id", "n/a")),
            str(preset.get("hp_mode", "n/a")),
            preamp_repr,
            str(len(preset.get("filters", []))),
            inventory_result,
            "|".join(detail_reasons) if detail_reasons else "ok",
        ])
    )

    for sample_rate in sample_rates:
        if inventory_result == "FAIL":
            sweep_lines.append(
                "\t".join([
                    run_index,
                    preset_file,
                    str(int(sample_rate)),
                    "nan",
                    "nan",
                    f"{threshold_db:.1f}",
                    "FAIL",
                    "inventory_failed",
                ])
            )
            continue

        nyquist = sample_rate * 0.5
        start_hz = nyquist * 0.9
        points = []
        for index in range(256):
            t = index / 255.0
            freq_hz = start_hz + (nyquist - start_hz) * t
            points.append(response_db(preset, sample_rate, freq_hz))

        max_abs_db = max(abs(value) for value in points)
        nyquist_db = response_db(preset, sample_rate, nyquist)
        sweep_result = "PASS" if max_abs_db <= threshold_db else "FAIL"
        if sweep_result == "FAIL":
            failed = True

        detail = "ok" if sweep_result == "PASS" else f"max_abs_db_gt_{threshold_db:.1f}"
        sweep_lines.append(
            "\t".join([
                run_index,
                preset_file,
                str(int(sample_rate)),
                f"{max_abs_db:.4f}",
                f"{nyquist_db:.4f}",
                f"{threshold_db:.1f}",
                sweep_result,
                detail,
            ])
        )

with open(out_inventory, "w", encoding="utf-8") as inventory_handle:
    inventory_handle.write("run\tpreset_file\thp_model_id\thp_mode\tpreamp_db\tfilter_count\tresult\tdetail\n")
    for line in inventory_lines:
        inventory_handle.write(f"{line}\n")

with open(out_sweep, "w", encoding="utf-8") as sweep_handle:
    sweep_handle.write("run\tpreset_file\tsample_rate_hz\tmax_abs_db_top_10pct_nyquist\tnyquist_db\tthreshold_db\tresult\tdetail\n")
    for line in sweep_lines:
        sweep_handle.write(f"{line}\n")

raise SystemExit(1 if failed else 0)
PY
  local python_exit_code=$?
  set -e

  if [[ ! -f "$run_inventory" || ! -f "$run_sweep" ]]; then
    printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$run_index" \
      "$MODE" \
      "FAIL" \
      "0" \
      "0" \
      "validation artifacts missing" \
      >> "$RUN_SUMMARY_TSV"
    record "BL057-R${run_index}-validation" "FAIL" "run ${run_index} did not produce validation artifacts" "$OUT_DIR"
    return
  fi

  tail -n +2 "$run_inventory" >> "$PRESET_INVENTORY_TSV"
  tail -n +2 "$run_sweep" >> "$SWEEP_RESULTS_TSV"

  local inventory_fail_rows
  local sweep_fail_rows
  inventory_fail_rows="$(awk -F'\t' 'NR > 1 && $7 == "FAIL" { c++ } END { print c + 0 }' "$run_inventory")"
  sweep_fail_rows="$(awk -F'\t' 'NR > 1 && $7 == "FAIL" { c++ } END { print c + 0 }' "$run_sweep")"

  local run_result="PASS"
  local run_detail="all checks passed"
  if [[ "$python_exit_code" -ne 0 ]]; then
    run_result="FAIL"
    run_detail="inventory_fail_rows=${inventory_fail_rows};sweep_fail_rows=${sweep_fail_rows}"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$run_index" \
    "$MODE" \
    "$run_result" \
    "$inventory_fail_rows" \
    "$sweep_fail_rows" \
    "$run_detail" \
    >> "$RUN_SUMMARY_TSV"

  if [[ "$run_result" == "PASS" ]]; then
    record "BL057-R${run_index}-validation" "PASS" "run ${run_index} passed inventory+sweep checks" "$OUT_DIR"
  else
    record "BL057-R${run_index}-validation" "FAIL" "run ${run_index} failed inventory/sweep checks" "$OUT_DIR"
  fi
}

for (( run_index=1; run_index<=RUNS; run_index++ )); do
  run_validation_once "$run_index"
done

run_fail_rows="$(awk -F'\t' 'NR > 1 && $3 == "FAIL" { c++ } END { print c + 0 }' "$RUN_SUMMARY_TSV")"
if [[ "$MODE" == "execute" ]]; then
  if [[ "$run_fail_rows" -eq 0 ]]; then
    record "BL057-E1-execute_gate" "PASS" "execute mode requires zero failing runs and gate is satisfied" "$RUN_SUMMARY_TSV"
  else
    record "BL057-E1-execute_gate" "FAIL" "execute mode requires zero failing runs (found=${run_fail_rows})" "$RUN_SUMMARY_TSV"
  fi
else
  record "BL057-C6-contract_gate" "PASS" "contract mode completed with deterministic run summary" "$RUN_SUMMARY_TSV"
fi

status_fail_rows="$(awk -F'\t' 'NR > 1 && $2 == "FAIL" { c++ } END { print c + 0 }' "$STATUS_TSV")"
status_pass_rows="$(awk -F'\t' 'NR > 1 && $2 == "PASS" { c++ } END { print c + 0 }' "$STATUS_TSV")"
lane_result="PASS"
if [[ "$status_fail_rows" -gt 0 ]]; then
  lane_result="FAIL"
fi

doc_date_utc="$(date -u +%Y-%m-%d)"
cat > "$SUMMARY_MD" <<EOF_SUMMARY
Title: BL-057 Device Preset Library Evidence Summary
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: ${doc_date_utc}
Last Modified Date: ${doc_date_utc}

# BL-057 Device Preset Library Lane Summary

- mode: ${MODE}
- runs: ${RUNS}
- output_directory: ${OUT_DIR}
- lane_result: ${lane_result}
- status_pass_rows: ${status_pass_rows}
- status_fail_rows: ${status_fail_rows}

## Artifacts

- status.tsv
- preset_inventory.tsv
- sweep_results.tsv
- run_summary.tsv
- summary.md (this file)
EOF_SUMMARY

echo "BL-057 preset lane output: ${OUT_DIR}"
echo "BL-057 lane result: ${lane_result} (pass=${status_pass_rows} fail=${status_fail_rows})"

if [[ "$lane_result" == "PASS" ]]; then
  exit 0
fi

exit 1
