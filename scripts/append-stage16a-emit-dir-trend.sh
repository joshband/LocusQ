#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCENARIO_ID="locusq_emit_dir_spatial_effect"
TREND_FILE_DEFAULT="$ROOT_DIR/TestEvidence/stage16a_emit_dir_discontinuity_trend.tsv"
RUN_DIR_PREFIX="$ROOT_DIR/TestEvidence/stage16a_scenario_validation_"

usage() {
  cat <<'USAGE'
Usage:
  scripts/append-stage16a-emit-dir-trend.sh [--run-dir <path>] [--trend-file <path>] [--force]

Options:
  --run-dir <path>    Stage16A validation directory to read (default: latest by timestamp).
  --trend-file <path> TSV file to append (default: TestEvidence/stage16a_emit_dir_discontinuity_trend.tsv).
  --force             Append even if run_id already exists in trend file.
  -h, --help          Show this help text.
USAGE
}

run_dir=""
trend_file="$TREND_FILE_DEFAULT"
force_append="0"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --run-dir)
      run_dir="${2:-}"
      shift 2
      ;;
    --trend-file)
      trend_file="${2:-}"
      shift 2
      ;;
    --force)
      force_append="1"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ -z "$run_dir" ]]; then
  latest_dir="$(ls -dt "${RUN_DIR_PREFIX}"* 2>/dev/null | head -n 1 || true)"
  if [[ -z "$latest_dir" ]]; then
    echo "ERROR: No Stage 16-A validation directories found under TestEvidence/." >&2
    exit 1
  fi
  run_dir="$latest_dir"
fi

if [[ ! -d "$run_dir" ]]; then
  echo "ERROR: run directory does not exist: $run_dir" >&2
  exit 1
fi

run_dir="$(cd "$run_dir" && pwd -P)"

log_file="$run_dir/${SCENARIO_ID}.log"
if [[ ! -f "$log_file" ]]; then
  echo "ERROR: scenario log missing: $log_file" >&2
  exit 1
fi

run_id="$(basename "$run_dir")"
run_timestamp_utc="${run_id#stage16a_scenario_validation_}"

status_line="$(rg -n '^Status:' "$log_file" | tail -n 1 | sed 's/.*Status: *//' || true)"
if [[ -z "$status_line" ]]; then
  echo "ERROR: Could not parse Status line from $log_file" >&2
  exit 1
fi

discontinuity_value="$(
  rg -n 'discontinuity_count: .*value=' "$log_file" \
    | sed -E 's/.*value=([0-9.\-]+).*/\1/' \
    | tail -n 1 || true
)"
if [[ -z "$discontinuity_value" ]]; then
  discontinuity_value="NA"
fi

mkdir -p "$(dirname "$trend_file")"
if [[ ! -f "$trend_file" ]]; then
  printf 'run_id\trun_timestamp_utc\tscenario_id\tstatus\tdiscontinuity_count\tlog_file\n' > "$trend_file"
fi

if [[ "$force_append" != "1" ]] && rg -q "^${run_id}[[:space:]]" "$trend_file"; then
  echo "SKIP: run_id already exists in trend file: $run_id"
  echo "trend_file: $trend_file"
  exit 0
fi

printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
  "$run_id" \
  "$run_timestamp_utc" \
  "$SCENARIO_ID" \
  "$status_line" \
  "$discontinuity_value" \
  "$log_file" >> "$trend_file"

echo "APPENDED: $run_id -> discontinuity_count=$discontinuity_value status=$status_line"
echo "trend_file: $trend_file"
