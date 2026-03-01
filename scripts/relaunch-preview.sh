#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

LAUNCH_EXIT=0

"$SCRIPT_DIR/launch-preview.sh" --clean || LAUNCH_EXIT=$?

echo "Launch exit code: $LAUNCH_EXIT"
echo "Post-launch status:"
"$SCRIPT_DIR/launch-preview.sh" --status
