#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

REPO_STANDALONE_APP="$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
REPO_COMPANION_APP="$ROOT_DIR/companion/LocusQ Headtrack Companion.app"
STANDALONE_APP="/Applications/LocusQ.app"
COMPANION_APP="/Applications/LocusQ Headtrack Companion.app"

if [[ ! -d "$STANDALONE_APP" ]]; then
  STANDALONE_APP="$REPO_STANDALONE_APP"
fi
if [[ ! -d "$COMPANION_APP" ]]; then
  COMPANION_APP="$REPO_COMPANION_APP"
fi

STANDALONE_BIN="$STANDALONE_APP/Contents/MacOS/LocusQ"
COMPANION_MONITOR="$COMPANION_APP/Contents/MacOS/locusq-headtrack-monitor"
COMPANION_BACKEND="$ROOT_DIR/companion/.build/release/locusq-headtrack-companion"
REPO_COMPANION_BACKEND="$ROOT_DIR/companion/.build/arm64-apple-macosx/release/locusq-headtrack-companion"

# Default behavior keeps paired launch clean.
DO_CLEAN=1
USE_BINARY_FALLBACK=0

kill_matching_processes() {
  local pattern="$1"
  local pids
  # `pkill` can fail in this environment due sysmond/process-list restrictions.
  # Use ps+rg+kill as a best-effort, deterministic fallback.
  while IFS= read -r pids; do
    kill -9 "$pids" 2>/dev/null || true
  done < <(ps -axo pid=,command= 2>/dev/null | rg -F "$pattern" | awk '{print $1}' || true)
}

usage() {
  cat <<EOF
Usage: ./scripts/launch-preview.sh [--clean] [--keep] [--fallback] [--status] [--help]

  --clean   Kill existing preview processes before launching (default).
  --keep    Keep existing preview processes (paired launch only).
  --fallback  Allow direct binary launch if open fails.
  --status  Print preview process status and exit.
  --help    Show this help text.
EOF
}

print_status() {
  echo "Preview status:"
  local ps_output
  ps_output="$(ps -ax 2>/dev/null || true)"
  if [[ -z "$ps_output" ]]; then
    echo "Process listing is unavailable in this environment."
    return 0
  fi

  printf '%s\n' "$ps_output" | rg -F "LocusQ.app/Contents/MacOS/LocusQ" || true
  printf '%s\n' "$ps_output" | rg -F "LocusQ Headtrack Companion.app/Contents/MacOS/locusq-headtrack-monitor" || true
  printf '%s\n' "$ps_output" | rg -F "companion/.build/release/locusq-headtrack-companion" || true
}

kill_previews() {
  local pattern
  for pattern in \
    "$STANDALONE_BIN" \
    "LocusQ.app/Contents/MacOS/LocusQ" \
    "$COMPANION_MONITOR" \
    "LocusQ Headtrack Companion.app/Contents/MacOS/locusq-headtrack-companion" \
    "$COMPANION_BACKEND" \
    "$REPO_COMPANION_BACKEND"; do
    kill_matching_processes "$pattern"
  done
}

launch_standalone() {
  if open -na "$STANDALONE_APP"; then
    return 0
  fi

  if (( ! USE_BINARY_FALLBACK )); then
    echo "Unable to launch $STANDALONE_APP via LaunchServices." >&2
    return 1
  fi

  if [[ ! -x "$STANDALONE_BIN" ]]; then
    echo "Standalone executable missing: $STANDALONE_BIN" 1>&2
    return 1
  fi

  "$STANDALONE_BIN" >/tmp/locusq_standalone_stdout.log 2>/tmp/locusq_standalone_stderr.log &
  return 0
}

launch_companion() {
  if open -na "$COMPANION_APP"; then
    return 0
  fi

  if (( ! USE_BINARY_FALLBACK )); then
    echo "Unable to launch $COMPANION_APP via LaunchServices." >&2
    return 1
  fi

  if [[ ! -x "$COMPANION_MONITOR" ]]; then
    echo "Companion monitor executable missing: $COMPANION_MONITOR" >&2
    return 1
  fi

  "$COMPANION_MONITOR" >/tmp/locusq_companion_monitor_stdout.log 2>/tmp/locusq_companion_monitor_stderr.log &
  return 0
}

while (( "$#" )); do
  case "${1}" in
    --clean)
      DO_CLEAN=1
      ;;
    --keep)
      DO_CLEAN=0
      ;;
    --fallback)
      USE_BINARY_FALLBACK=1
      ;;
    --status)
      print_status
      exit 0
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: ${1}"
      usage
      exit 1
      ;;
  esac
  shift
done

if (( DO_CLEAN )); then
  echo "Killing existing preview processes..."
  kill_previews
fi

if [[ ! -d "$STANDALONE_APP" ]]; then
  echo "Missing standalone build: $STANDALONE_APP" >&2
  echo "Run: ./scripts/build-and-install-mac.sh" >&2
  exit 1
fi

if [[ ! -x "$COMPANION_MONITOR" ]]; then
  echo "Missing companion launcher: $COMPANION_MONITOR" >&2
  echo "Run: cd companion && swift build -c release" >&2
  exit 1
fi

LAUNCH_STATUS=0
if ! launch_standalone; then
  echo "Standalone launch failed" >&2
  LAUNCH_STATUS=1
fi

if ! launch_companion; then
  echo "Companion launch failed" >&2
  LAUNCH_STATUS=1
fi

if (( LAUNCH_STATUS == 0 )); then
  echo "Launched:"
  echo " - $STANDALONE_APP"
  echo " - $COMPANION_APP"
else
  echo "Launch failures occurred. Check logs/permissions and try --fallback when needed." >&2
fi

exit "$LAUNCH_STATUS"
