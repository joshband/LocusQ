#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

OWNER="${CODEX_OWNER:-Codex}"
COORDINATOR_ID="${CODEX_COORDINATOR_ID:-coordinator}"
WORKER_ID="${CODEX_WORKER_ID:-worker_main}"
RUN_WORKER=1

COORDINATOR_TASK="Coordinate active Codex session work and watchdog gate checks"
COORDINATOR_OUTPUTS="status.json|TestEvidence/build-summary.md|TestEvidence/validation-trend.md"
COORDINATOR_TIMEOUT_MINUTES=240
COORDINATOR_STATUS="WORKING session_start"
COORDINATOR_LAST_ARTIFACT="TestEvidence/thread-heartbeats.tsv"

WORKER_TASK="Execute active LocusQ implementation/testing tasks"
WORKER_OUTPUTS="TestEvidence/test-summary.md|qa_output/suite_result.json|status.json"
WORKER_TIMEOUT_MINUTES=120
WORKER_STATUS="WORKING session_start"
WORKER_LAST_ARTIFACT="TestEvidence/thread-heartbeats.tsv"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/codex-session-bootstrap.sh [options]

Options:
  --owner <name>             Override owner name (default: Codex or $CODEX_OWNER)
  --coordinator-id <id>      Override coordinator thread id (default: coordinator)
  --worker-id <id>           Override worker thread id (default: worker_main)
  --skip-worker              Initialize only coordinator contract/heartbeat
  --coordinator-status <s>   Coordinator heartbeat status
  --worker-status <s>        Worker heartbeat status
  --help                     Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --owner)
      OWNER="$2"
      shift 2
      ;;
    --coordinator-id)
      COORDINATOR_ID="$2"
      shift 2
      ;;
    --worker-id)
      WORKER_ID="$2"
      shift 2
      ;;
    --skip-worker)
      RUN_WORKER=0
      shift
      ;;
    --coordinator-status)
      COORDINATOR_STATUS="$2"
      shift 2
      ;;
    --worker-status)
      WORKER_STATUS="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: Unknown option '$1'"
      usage
      exit 2
      ;;
  esac
done

"$ROOT_DIR/scripts/codex-init" \
  --thread-id "$COORDINATOR_ID" \
  --task "$COORDINATOR_TASK" \
  --expected-outputs "$COORDINATOR_OUTPUTS" \
  --timeout-minutes "$COORDINATOR_TIMEOUT_MINUTES" \
  --owner "$OWNER" \
  --role coordinator \
  --status "$COORDINATOR_STATUS" \
  --last-artifact "$COORDINATOR_LAST_ARTIFACT" \
  --skip-watchdog

if [[ "$RUN_WORKER" -eq 1 ]]; then
  "$ROOT_DIR/scripts/codex-init" \
    --thread-id "$WORKER_ID" \
    --task "$WORKER_TASK" \
    --expected-outputs "$WORKER_OUTPUTS" \
    --timeout-minutes "$WORKER_TIMEOUT_MINUTES" \
    --owner "$OWNER" \
    --role worker \
    --status "$WORKER_STATUS" \
    --last-artifact "$WORKER_LAST_ARTIFACT" \
    --skip-watchdog
fi

"$ROOT_DIR/scripts/thread-watchdog"

echo
echo "Session bootstrap complete."
echo "Coordinator thread: $COORDINATOR_ID"
if [[ "$RUN_WORKER" -eq 1 ]]; then
  echo "Worker thread:      $WORKER_ID"
fi
