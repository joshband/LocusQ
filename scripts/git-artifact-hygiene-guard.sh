#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

MODE="staged"
DIFF_BASE=""
MAX_FILE_SIZE_MB=20

usage() {
  cat <<'USAGE'
Usage:
  ./scripts/git-artifact-hygiene-guard.sh [options]

Modes:
  (default)                Guard currently staged files for commit.
  --diff-base <git-ref>    Guard files changed from <git-ref>...HEAD (CI mode).

Options:
  --max-file-size-mb <N>   Maximum allowed blob size in MB (default: 20)
  -h, --help               Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --diff-base)
      MODE="diff"
      DIFF_BASE="$2"
      shift 2
      ;;
    --max-file-size-mb)
      MAX_FILE_SIZE_MB="$2"
      shift 2
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

if ! [[ "${MAX_FILE_SIZE_MB}" =~ ^[0-9]+$ ]]; then
  echo "ERROR: --max-file-size-mb must be an integer" >&2
  exit 2
fi

if ! git rev-parse --git-dir >/dev/null 2>&1; then
  echo "ERROR: Not inside a git repository" >&2
  exit 2
fi

if [[ "${MODE}" == "diff" ]]; then
  if ! git rev-parse --verify "${DIFF_BASE}^{commit}" >/dev/null 2>&1; then
    echo "ERROR: Unable to resolve --diff-base ref: ${DIFF_BASE}" >&2
    exit 2
  fi
fi

CANDIDATES=()
while IFS= read -r path; do
  [[ -n "${path}" ]] && CANDIDATES+=("${path}")
done < <(
  if [[ "${MODE}" == "staged" ]]; then
    git diff --cached --name-only --diff-filter=ACMR
  else
    git diff --name-only --diff-filter=ACMR "${DIFF_BASE}...HEAD"
  fi
)

if [[ "${#CANDIDATES[@]}" -eq 0 ]]; then
  if [[ "${MODE}" == "staged" ]]; then
    echo "PASS: no staged files to guard."
  else
    echo "PASS: no changed files to guard from ${DIFF_BASE}...HEAD."
  fi
  exit 0
fi

MAX_BYTES=$((MAX_FILE_SIZE_MB * 1024 * 1024))

RULES=(
  '^companion/\.build/::companion build cache/output'
  '^build(/|_|-|$)::generated build output directory'
  '^build_bl[^/]*/::generated branch build output directory'
  '^qa_output/::generated QA output directory'
  '^tmp/::temporary output directory'
  '^TestEvidence/archive/.*\.(tar|tar\.gz|tgz|zip)$::archived evidence bundle'
  '^TestEvidence/.*\.(tar|tar\.gz|tgz|zip)$::compressed test evidence bundle'
  '^Documentation/Calibration POC/.*\.zip$::Calibration POC zip archive'
  '^third_party/steam-audio/steamaudio_.*\.zip$::Steam Audio SDK zip archive'
)

ISSUE_COUNT=0

report_issue() {
  ISSUE_COUNT=$((ISSUE_COUNT + 1))
  printf 'BLOCK: %s\n' "$1"
}

for path in "${CANDIDATES[@]}"; do
  [[ -z "${path}" ]] && continue

  for rule in "${RULES[@]}"; do
    pattern="${rule%%::*}"
    reason="${rule#*::}"
    if [[ "${path}" =~ ${pattern} ]]; then
      report_issue "${path} (${reason})"
    fi
  done

  if git check-ignore -q --no-index -- "${path}"; then
    report_issue "${path} (matches .gitignore rules)"
  fi

  blob_spec=":${path}"
  if [[ "${MODE}" == "diff" ]]; then
    blob_spec="HEAD:${path}"
  fi

  if git cat-file -e "${blob_spec}" 2>/dev/null; then
    blob_size="$(git cat-file -s "${blob_spec}")"
    if [[ "${blob_size}" -gt "${MAX_BYTES}" ]]; then
      blob_size_mb="$(awk -v b="${blob_size}" 'BEGIN { printf "%.2f", b/1048576 }')"
      report_issue "${path} (${blob_size_mb} MB exceeds ${MAX_FILE_SIZE_MB} MB limit)"
    fi
  fi
done

if [[ "${ISSUE_COUNT}" -gt 0 ]]; then
  echo
  echo "FAIL: git artifact hygiene guard found ${ISSUE_COUNT} issue(s)."
  if [[ "${MODE}" == "staged" ]]; then
    echo "Hint: unstage blocked files with: git restore --staged <path>"
  else
    echo "Hint: remove blocked files from this change before merge."
  fi
  exit 1
fi

if [[ "${MODE}" == "staged" ]]; then
  echo "PASS: git artifact hygiene guard passed for staged files."
else
  echo "PASS: git artifact hygiene guard passed for ${DIFF_BASE}...HEAD."
fi
