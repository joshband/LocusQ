#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: run_spatial_lanes.sh [--skip-bl009] [--skip-bl018] [--ui-selftest] [--help]

Runs LocusQ spatial QA lanes from repo root:
- BL-009 headphone/binaural contract lane
- BL-018 ambisonic/layout contract lane
- Optional production UI selftest with BL-009 assertion enabled
USAGE
}

run_bl009=1
run_bl018=1
run_ui=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --skip-bl009)
      run_bl009=0
      ;;
    --skip-bl018)
      run_bl018=0
      ;;
    --ui-selftest)
      run_ui=1
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if [[ $run_bl009 -eq 0 && $run_bl018 -eq 0 && $run_ui -eq 0 ]]; then
  echo "Nothing to run. Enable at least one lane." >&2
  exit 2
fi

repo_root=""
if repo_root_candidate=$(git rev-parse --show-toplevel 2>/dev/null); then
  repo_root="$repo_root_candidate"
else
  repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
fi

cd "$repo_root"

echo "[spatial-lanes] repo_root=$repo_root"

run_lane() {
  local label="$1"
  shift
  echo "[spatial-lanes] START $label"
  "$@"
  echo "[spatial-lanes] PASS  $label"
}

if [[ $run_bl009 -eq 1 ]]; then
  if [[ ! -f scripts/qa-bl009-headphone-contract-mac.sh ]]; then
    echo "Missing scripts/qa-bl009-headphone-contract-mac.sh" >&2
    exit 3
  fi
  run_lane "BL-009" bash scripts/qa-bl009-headphone-contract-mac.sh
fi

if [[ $run_bl018 -eq 1 ]]; then
  if [[ ! -f scripts/qa-bl018-ambisonic-contract-mac.sh ]]; then
    echo "Missing scripts/qa-bl018-ambisonic-contract-mac.sh" >&2
    exit 3
  fi
  run_lane "BL-018" bash scripts/qa-bl018-ambisonic-contract-mac.sh
fi

if [[ $run_ui -eq 1 ]]; then
  if [[ ! -f scripts/standalone-ui-selftest-production-p0-mac.sh ]]; then
    echo "Missing scripts/standalone-ui-selftest-production-p0-mac.sh" >&2
    exit 3
  fi
  run_lane "UI-selftest-BL009" env LOCUSQ_UI_SELFTEST_BL009=1 bash scripts/standalone-ui-selftest-production-p0-mac.sh
fi

echo "[spatial-lanes] complete"
