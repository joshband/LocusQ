#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

DRY_RUN=0

if [[ "${1:-}" == "--dry-run" ]]; then
  DRY_RUN=1
elif [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  cat <<'USAGE'
Usage:
  ./scripts/install-git-hygiene-hooks.sh [--dry-run]
USAGE
  exit 0
elif [[ $# -gt 0 ]]; then
  echo "ERROR: Unknown argument: $1" >&2
  exit 2
fi

mkdir -p .githooks
chmod +x .githooks/pre-commit
chmod +x scripts/git-artifact-hygiene-guard.sh

if [[ "${DRY_RUN}" -eq 1 ]]; then
  echo "DRY RUN: would set git config core.hooksPath .githooks"
  echo "DRY RUN: hook ready at .githooks/pre-commit"
  exit 0
fi

if ! git config core.hooksPath .githooks; then
  echo "ERROR: Unable to set core.hooksPath (git config write failed)." >&2
  exit 1
fi

echo "Installed git hygiene pre-commit hook via core.hooksPath=.githooks"
echo "Hook: .githooks/pre-commit"
