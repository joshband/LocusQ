#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

usage() {
  cat <<'USAGE'
Usage: qa-hx05-payload-budget-soak-mac.sh --input-dir <path> [--out-dir <path>] [--label <name>]

Deterministic HX-05 payload-budget soak evaluator.

Required:
  --input-dir <path>   Directory containing:
                      payload_metrics.tsv
                      transport_cadence.tsv
                      budget_tier_events.tsv
                      taxonomy_table.tsv

Optional:
  --out-dir <path>     Output artifact directory (default: TestEvidence/hx05_payload_budget_soak_<timestamp>)
  --label <name>       Lane label (default: HX05-LANE-SOAK)
  --help               Print this message

Exit codes:
  0  PASS (all schema checks and thresholds satisfied)
  1  FAIL (schema/threshold/degrade transition violations)
  2  Usage or invocation error
USAGE
}

INPUT_DIR=""
OUT_DIR=""
LANE_LABEL="HX05-LANE-SOAK"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --input-dir requires a value" >&2; usage; exit 2; }
      INPUT_DIR="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --out-dir requires a value" >&2; usage; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    --label)
      [[ $# -ge 2 ]] || { echo "ERROR: --label requires a value" >&2; usage; exit 2; }
      LANE_LABEL="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ -z "$INPUT_DIR" ]]; then
  echo "ERROR: --input-dir is required" >&2
  usage
  exit 2
fi

if [[ -z "$OUT_DIR" ]]; then
  OUT_DIR="$ROOT_DIR/TestEvidence/hx05_payload_budget_soak_${TIMESTAMP}"
fi

mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
TAXONOMY_TSV="$OUT_DIR/taxonomy_table.tsv"
CONTRACT_MD="$OUT_DIR/qa_lane_contract.md"
SUMMARY_KV="$OUT_DIR/eval_summary.kv"
EVAL_LOG="$OUT_DIR/eval.log"

PAYLOAD_TSV="$INPUT_DIR/payload_metrics.tsv"
CADENCE_TSV="$INPUT_DIR/transport_cadence.tsv"
TIER_TSV="$INPUT_DIR/budget_tier_events.tsv"
INPUT_TAXONOMY_TSV="$INPUT_DIR/taxonomy_table.tsv"

printf "check\tresult\tdetail\tartifact\n" > "$STATUS_TSV"

sanitize_tsv_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

log_status() {
  local check="$1"
  local result="$2"
  local detail="$3"
  local artifact="$4"
  printf "%s\t%s\t%s\t%s\n" \
    "$(sanitize_tsv_field "$check")" \
    "$(sanitize_tsv_field "$result")" \
    "$(sanitize_tsv_field "$detail")" \
    "$(sanitize_tsv_field "$artifact")" \
    >> "$STATUS_TSV"
}

log_status "init" "PASS" "lane=${LANE_LABEL}; ts=${DOC_TS}" "$OUT_DIR"

for required in "$PAYLOAD_TSV" "$CADENCE_TSV" "$TIER_TSV" "$INPUT_TAXONOMY_TSV"; do
  if [[ -f "$required" ]]; then
    log_status "artifact_presence" "PASS" "found=$(basename "$required")" "$required"
  else
    log_status "artifact_presence" "FAIL" "missing=$(basename "$required")" "$required"
  fi
done

set +e
python3 - "$PAYLOAD_TSV" "$CADENCE_TSV" "$TIER_TSV" "$INPUT_TAXONOMY_TSV" "$TAXONOMY_TSV" "$SUMMARY_KV" > "$EVAL_LOG" 2>&1 <<'PY'
import csv
import math
import sys
from pathlib import Path

payload_path = Path(sys.argv[1])
cadence_path = Path(sys.argv[2])
tier_path = Path(sys.argv[3])
input_tax_path = Path(sys.argv[4])
out_tax_path = Path(sys.argv[5])
summary_path = Path(sys.argv[6])

required_columns = {
    "payload_metrics": ["window_id", "snapshot_seq", "utc_ms", "bytes", "tier", "burst_count"],
    "transport_cadence": ["window_id", "window_start_ms", "window_end_ms", "cadence_hz", "over_soft_count"],
    "budget_tier_events": ["snapshot_seq", "window_id", "from_tier", "to_tier", "reason", "compliance_streak"],
    "taxonomy_table": ["failure_code", "count", "first_snapshot_seq", "first_window_id"],
}

thresholds = {
    "max_bytes": 65536.0,
    "p95_bytes": 32768.0,
    "max_cadence_hz": 60.0,
    "max_burst_snapshots": 8.0,
    "max_burst_ms": 500.0,
}

scored_windows = {"W1_nominal", "W2_burst", "W3_sustained_stress"}
burst_windows = {"W2_burst", "W3_sustained_stress"}

failure_codes = [
    "oversize_hard_limit",
    "oversize_soft_limit",
    "cadence_violation",
    "burst_overrun",
    "degrade_tier_mismatch",
    "schema_invalid",
    "none",
]

counts = {code: 0 for code in failure_codes}
first = {code: ("", "") for code in failure_codes}
schema_errors = []


def mark_failure(code: str, snapshot_seq: str, window_id: str):
    counts[code] += 1
    if not first[code][0] and not first[code][1]:
        first[code] = (snapshot_seq, window_id)


def parse_tsv(path: Path, schema_name: str):
    if not path.exists():
        schema_errors.append(f"missing_file:{schema_name}:{path}")
        return None

    with path.open("r", encoding="utf-8", newline="") as fh:
        reader = csv.DictReader(fh, delimiter="\t")
        if reader.fieldnames is None:
            schema_errors.append(f"missing_header:{schema_name}")
            return None
        missing = [col for col in required_columns[schema_name] if col not in reader.fieldnames]
        if missing:
            schema_errors.append(f"missing_columns:{schema_name}:{','.join(missing)}")
            return None
        rows = list(reader)
    return rows


def as_float(value, default=None):
    try:
        out = float(value)
        if math.isfinite(out):
            return out
    except (TypeError, ValueError):
        pass
    return default

payload_rows = parse_tsv(payload_path, "payload_metrics")
cadence_rows = parse_tsv(cadence_path, "transport_cadence")
tier_rows = parse_tsv(tier_path, "budget_tier_events")
_ = parse_tsv(input_tax_path, "taxonomy_table")

max_bytes = 0.0
p95_bytes = 0.0
max_cadence = 0.0
max_burst_count = 0.0
max_burst_ms = 0.0

# Payload checks (max + p95 + burst)
if payload_rows is not None:
    scored_payload = []
    burst_points = []
    for row in payload_rows:
        window_id = (row.get("window_id") or "").strip()
        if window_id not in scored_windows:
            continue

        bytes_v = as_float(row.get("bytes"), None)
        burst_v = as_float(row.get("burst_count"), None)
        utc_v = as_float(row.get("utc_ms"), None)
        seq_s = (row.get("snapshot_seq") or "").strip()

        if bytes_v is None or utc_v is None or burst_v is None:
            schema_errors.append("invalid_numeric:payload_metrics")
            continue

        scored_payload.append((bytes_v, seq_s, window_id))
        max_bytes = max(max_bytes, bytes_v)

        if window_id in burst_windows:
            burst_points.append((utc_v, burst_v, seq_s, window_id))

    if not scored_payload:
        schema_errors.append("missing_scored_rows:payload_metrics")
    else:
        values = sorted(v for (v, _, _) in scored_payload)
        rank = max(1, math.ceil(0.95 * len(values)))
        p95_bytes = values[rank - 1]

        if max_bytes > thresholds["max_bytes"]:
            for v, seq_s, window_id in scored_payload:
                if v > thresholds["max_bytes"]:
                    mark_failure("oversize_hard_limit", seq_s, window_id)
                    break

        if p95_bytes > thresholds["p95_bytes"]:
            mark_failure("oversize_soft_limit", scored_payload[0][1], scored_payload[0][2])

    if burst_points:
        # Determine max burst duration using burst_count reset markers.
        active_start = None
        for utc_v, burst_v, seq_s, window_id in burst_points:
            max_burst_count = max(max_burst_count, burst_v)
            if burst_v >= 1.0:
                if burst_v <= 1.0 or active_start is None:
                    active_start = utc_v
                duration = max(0.0, utc_v - active_start)
                max_burst_ms = max(max_burst_ms, duration)
            else:
                active_start = None

        if max_burst_count > thresholds["max_burst_snapshots"] or max_burst_ms > thresholds["max_burst_ms"]:
            first_seq = ""
            first_window = ""
            for utc_v, burst_v, seq_s, window_id in burst_points:
                if burst_v > thresholds["max_burst_snapshots"]:
                    first_seq, first_window = seq_s, window_id
                    break
            if not first_window:
                first_seq, first_window = burst_points[0][2], burst_points[0][3]
            mark_failure("burst_overrun", first_seq, first_window)

# Cadence checks
if cadence_rows is not None:
    seen = False
    for row in cadence_rows:
        window_id = (row.get("window_id") or "").strip()
        if window_id not in scored_windows:
            continue
        cadence_v = as_float(row.get("cadence_hz"), None)
        if cadence_v is None:
            schema_errors.append("invalid_numeric:transport_cadence")
            continue
        seen = True
        if cadence_v > max_cadence:
            max_cadence = cadence_v
        if cadence_v > thresholds["max_cadence_hz"]:
            mark_failure("cadence_violation", "NA", window_id)
            break
    if not seen:
        schema_errors.append("missing_scored_rows:transport_cadence")

# Tier transition checks
if tier_rows is not None:
    allowed_tiers = {"normal", "degrade_t1", "degrade_t2_safe"}
    allowed_transitions = {
        ("normal", "normal"),
        ("normal", "degrade_t1"),
        ("degrade_t1", "degrade_t1"),
        ("degrade_t1", "normal"),
        ("degrade_t1", "degrade_t2_safe"),
        ("degrade_t2_safe", "degrade_t2_safe"),
        ("degrade_t2_safe", "normal"),
    }

    for row in tier_rows:
        seq_s = (row.get("snapshot_seq") or "").strip()
        window_id = (row.get("window_id") or "").strip()
        from_tier = (row.get("from_tier") or "").strip()
        to_tier = (row.get("to_tier") or "").strip()
        reason = (row.get("reason") or "").strip()
        compliance = as_float(row.get("compliance_streak"), None)

        if from_tier not in allowed_tiers or to_tier not in allowed_tiers:
            mark_failure("degrade_tier_mismatch", seq_s, window_id)
            break

        if (from_tier, to_tier) not in allowed_transitions:
            mark_failure("degrade_tier_mismatch", seq_s, window_id)
            break

        if from_tier == "degrade_t1" and to_tier == "normal":
            if compliance is None or compliance < 120.0:
                mark_failure("degrade_tier_mismatch", seq_s, window_id)
                break

        if from_tier == "degrade_t2_safe" and to_tier == "normal":
            if compliance is None or compliance < 240.0:
                mark_failure("degrade_tier_mismatch", seq_s, window_id)
                break

        if not reason:
            mark_failure("degrade_tier_mismatch", seq_s, window_id)
            break

if schema_errors:
    mark_failure("schema_invalid", "NA", "NA")

non_none_total = sum(counts[code] for code in failure_codes if code != "none")
if non_none_total == 0:
    counts["none"] = 1
    first["none"] = ("NA", "NA")

with out_tax_path.open("w", encoding="utf-8", newline="") as fh:
    writer = csv.writer(fh, delimiter="\t")
    writer.writerow(["failure_code", "count", "first_snapshot_seq", "first_window_id"])
    for code in failure_codes:
        seq_s, window_id = first[code]
        writer.writerow([code, counts[code], seq_s or "", window_id or ""])

overall = "PASS" if non_none_total == 0 else "FAIL"
with summary_path.open("w", encoding="utf-8") as fh:
    fh.write(f"overall={overall}\n")
    fh.write(f"max_bytes={max_bytes:.6f}\n")
    fh.write(f"p95_bytes={p95_bytes:.6f}\n")
    fh.write(f"max_cadence_hz={max_cadence:.6f}\n")
    fh.write(f"max_burst_snapshots={max_burst_count:.6f}\n")
    fh.write(f"max_burst_ms={max_burst_ms:.6f}\n")
    fh.write(f"schema_errors_count={len(schema_errors)}\n")
    fh.write(f"schema_errors={'|'.join(schema_errors)}\n")
    for code in failure_codes:
        fh.write(f"count_{code}={counts[code]}\n")
PY
EVAL_EXIT=$?
set -e

if [[ "$EVAL_EXIT" -ne 0 ]]; then
  log_status "evaluator" "FAIL" "python_evaluator_exit=${EVAL_EXIT}" "$EVAL_LOG"
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

log_status "evaluator" "PASS" "python_evaluator_exit=0" "$EVAL_LOG"

kv_get() {
  local key="$1"
  awk -F= -v k="$key" '$1 == k {print substr($0, index($0, "=")+1)}' "$SUMMARY_KV" | tail -n 1
}

OVERALL="$(kv_get overall)"
MAX_BYTES="$(kv_get max_bytes)"
P95_BYTES="$(kv_get p95_bytes)"
MAX_CADENCE="$(kv_get max_cadence_hz)"
MAX_BURST_COUNT="$(kv_get max_burst_snapshots)"
MAX_BURST_MS="$(kv_get max_burst_ms)"
SCHEMA_ERRORS_COUNT="$(kv_get schema_errors_count)"
SCHEMA_ERRORS="$(kv_get schema_errors)"
COUNT_OVERSIZE_HARD="$(kv_get count_oversize_hard_limit)"
COUNT_OVERSIZE_SOFT="$(kv_get count_oversize_soft_limit)"
COUNT_CADENCE="$(kv_get count_cadence_violation)"
COUNT_BURST="$(kv_get count_burst_overrun)"
COUNT_TIER="$(kv_get count_degrade_tier_mismatch)"

if [[ "${COUNT_OVERSIZE_HARD:-0}" != "0" ]]; then
  log_status "threshold_max_bytes" "FAIL" "max_bytes=${MAX_BYTES}; limit=65536; failures=${COUNT_OVERSIZE_HARD}" "$PAYLOAD_TSV"
else
  log_status "threshold_max_bytes" "PASS" "max_bytes=${MAX_BYTES}; limit=65536" "$PAYLOAD_TSV"
fi

if [[ "${COUNT_OVERSIZE_SOFT:-0}" != "0" ]]; then
  log_status "threshold_p95_bytes" "FAIL" "p95_bytes=${P95_BYTES}; limit=32768; failures=${COUNT_OVERSIZE_SOFT}" "$PAYLOAD_TSV"
else
  log_status "threshold_p95_bytes" "PASS" "p95_bytes=${P95_BYTES}; limit=32768" "$PAYLOAD_TSV"
fi

if [[ "${COUNT_CADENCE:-0}" != "0" ]]; then
  log_status "threshold_cadence" "FAIL" "max_cadence_hz=${MAX_CADENCE}; limit=60; failures=${COUNT_CADENCE}" "$CADENCE_TSV"
else
  log_status "threshold_cadence" "PASS" "max_cadence_hz=${MAX_CADENCE}; limit=60" "$CADENCE_TSV"
fi

if [[ "${COUNT_BURST:-0}" != "0" ]]; then
  log_status "threshold_burst" "FAIL" "max_burst_snapshots=${MAX_BURST_COUNT}; max_burst_ms=${MAX_BURST_MS}; limits=8/500; failures=${COUNT_BURST}" "$PAYLOAD_TSV"
else
  log_status "threshold_burst" "PASS" "max_burst_snapshots=${MAX_BURST_COUNT}; max_burst_ms=${MAX_BURST_MS}; limits=8/500" "$PAYLOAD_TSV"
fi

if [[ "${COUNT_TIER:-0}" != "0" ]]; then
  log_status "tier_transition_policy" "FAIL" "tier_transition_failures=${COUNT_TIER}" "$TIER_TSV"
else
  log_status "tier_transition_policy" "PASS" "tier_transition_failures=0" "$TIER_TSV"
fi

if [[ "$SCHEMA_ERRORS_COUNT" != "0" ]]; then
  log_status "schema_validation" "FAIL" "schema_errors=${SCHEMA_ERRORS}" "$INPUT_DIR"
else
  log_status "schema_validation" "PASS" "all_required_artifact_schemas_valid" "$INPUT_DIR"
fi

cat > "$CONTRACT_MD" <<EOF2
Title: HX-05 Payload Budget Soak Harness Contract Snapshot
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_TS}
Last Modified Date: ${DOC_TS}

# HX-05 Payload Budget Soak Harness

- lane: \`${LANE_LABEL}\`
- input_dir: \`${INPUT_DIR}\`
- out_dir: \`${OUT_DIR}\`
- evaluator: deterministic TSV parser with nearest-rank p95.

## Enforced Thresholds

- \`max(bytes) <= 65536\`
- \`p95(bytes) <= 32768\`
- \`max(cadence_hz) <= 60\`
- \`burst <= 8 snapshots and <= 500ms\`

## Computed Metrics

- max_bytes: \`${MAX_BYTES}\`
- p95_bytes: \`${P95_BYTES}\`
- max_cadence_hz: \`${MAX_CADENCE}\`
- max_burst_snapshots: \`${MAX_BURST_COUNT}\`
- max_burst_ms: \`${MAX_BURST_MS}\`
- schema_errors_count: \`${SCHEMA_ERRORS_COUNT}\`

## Output Artifacts

- \`status.tsv\`
- \`taxonomy_table.tsv\`
- \`qa_lane_contract.md\`
EOF2

if [[ "$OVERALL" == "PASS" && "$SCHEMA_ERRORS_COUNT" == "0" ]]; then
  log_status "lane_result" "PASS" "all_hx05_contract_thresholds_satisfied" "$TAXONOMY_TSV"
  echo "artifact_dir=$OUT_DIR"
  exit 0
fi

log_status "lane_result" "FAIL" "contract_violation_detected" "$TAXONOMY_TSV"
echo "artifact_dir=$OUT_DIR"
exit 1
