#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

APPLY=0
MANIFEST_PATH=""

usage() {
  cat <<'USAGE'
Usage:
  ./scripts/git-artifact-cleanup-index.sh [options]

Options:
  --apply                   Remove detected paths from index via git rm --cached
  --manifest <path>         Write candidate paths manifest to file
  -h, --help                Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --apply)
      APPLY=1
      shift
      ;;
    --manifest)
      MANIFEST_PATH="$2"
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

TMP_CANDIDATES="$(mktemp)"
TMP_UNIQ="$(mktemp)"
trap 'rm -f "${TMP_CANDIDATES}" "${TMP_UNIQ}"' EXIT

git ls-files -ci --exclude-standard > "${TMP_CANDIDATES}" || true
git ls-files -- 'TestEvidence/archive/*.tar.gz' 'TestEvidence/archive/*.tgz' 'TestEvidence/archive/*.zip' >> "${TMP_CANDIDATES}" || true

awk 'NF && !seen[$0]++' "${TMP_CANDIDATES}" > "${TMP_UNIQ}"
CANDIDATE_COUNT="$(awk 'NF { c++ } END { print c + 0 }' "${TMP_UNIQ}")"

if [[ "${CANDIDATE_COUNT}" -eq 0 ]]; then
  echo "PASS: no tracked ignored/archive cleanup candidates found in index."
  exit 0
fi

echo "Found ${CANDIDATE_COUNT} tracked cleanup candidate(s)."

if [[ -n "${MANIFEST_PATH}" ]]; then
  mkdir -p "$(dirname "${MANIFEST_PATH}")"
  cp "${TMP_UNIQ}" "${MANIFEST_PATH}"
  echo "Wrote cleanup manifest: ${MANIFEST_PATH}"
fi

if [[ "${APPLY}" -eq 1 ]]; then
  while IFS= read -r path; do
    [[ -z "${path}" ]] && continue
    git rm --cached -- "${path}"
  done < "${TMP_UNIQ}"
  echo "Applied index cleanup with git rm --cached."
else
  echo "Dry run candidates:"
  sed -n '1,200p' "${TMP_UNIQ}"
  if [[ "${CANDIDATE_COUNT}" -gt 200 ]]; then
    echo "... (${CANDIDATE_COUNT} total; truncated to first 200 lines)"
  fi
  echo
  echo "Re-run with --apply to remove these paths from index."
fi
