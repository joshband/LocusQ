#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

REF="HEAD"
LARGE_BLOB_MB=20
STRICT=0
CHECK_HISTORY=1
CHECK_LOCAL_RISK=1
SHOW_TOP=10

usage() {
  cat <<'USAGE'
Usage:
  ./scripts/git-artifact-hygiene-audit.sh [options]

Options:
  --ref <git-ref>             Audit a specific commit/tree (default: HEAD)
  --large-blob-mb <megabytes> Threshold for large history blobs (default: 20)
  --show-top <count>          Show top N large blobs (default: 10)
  --no-history                Skip reachable-history large blob scan
  --no-local-risk             Skip local workspace zip-risk checks
  --strict                    Exit non-zero when one or more problems are found
  -h, --help                  Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --ref)
      REF="$2"
      shift 2
      ;;
    --large-blob-mb)
      LARGE_BLOB_MB="$2"
      shift 2
      ;;
    --show-top)
      SHOW_TOP="$2"
      shift 2
      ;;
    --no-history)
      CHECK_HISTORY=0
      shift
      ;;
    --no-local-risk)
      CHECK_LOCAL_RISK=0
      shift
      ;;
    --strict)
      STRICT=1
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

if ! command -v git >/dev/null 2>&1; then
  echo "ERROR: git is required" >&2
  exit 2
fi

if ! git rev-parse --git-dir >/dev/null 2>&1; then
  echo "ERROR: Not inside a git repository" >&2
  exit 2
fi

if ! git rev-parse --verify "${REF}^{commit}" >/dev/null 2>&1; then
  echo "ERROR: Unable to resolve git ref: ${REF}" >&2
  exit 2
fi

if ! [[ "${LARGE_BLOB_MB}" =~ ^[0-9]+$ ]] || ! [[ "${SHOW_TOP}" =~ ^[0-9]+$ ]]; then
  echo "ERROR: --large-blob-mb and --show-top must be integers" >&2
  exit 2
fi

PROBLEMS=0

problem() {
  PROBLEMS=$((PROBLEMS + 1))
}

count_lines() {
  awk 'NF { c++ } END { print c + 0 }'
}

bytes_to_mb() {
  local bytes="$1"
  awk -v b="${bytes}" 'BEGIN { printf "%.2f", b / 1048576 }'
}

COMMIT_SHA="$(git rev-parse "${REF}")"
SHORT_SHA="$(git rev-parse --short "${REF}")"

TREE_PATHS_FILE="$(mktemp)"
OBJECTS_FILE="$(mktemp)"
CATALOG_FILE="$(mktemp)"
LARGE_FILE="$(mktemp)"
trap 'rm -f "${TREE_PATHS_FILE}" "${OBJECTS_FILE}" "${CATALOG_FILE}" "${LARGE_FILE}"' EXIT

git ls-tree -r --name-only "${REF}" > "${TREE_PATHS_FILE}"

echo "Audit result as of ${REF} commit ${COMMIT_SHA}:"
echo

# 1) Tracked paths matching ignore rules.
TRACKED_IGNORED="$(git check-ignore --no-index --stdin < "${TREE_PATHS_FILE}" 2>/dev/null || true)"
TRACKED_IGNORED_COUNT="$(printf '%s\n' "${TRACKED_IGNORED}" | count_lines)"

if [[ "${TRACKED_IGNORED_COUNT}" -gt 0 ]]; then
  problem
  COMPANION_BUILD_COUNT="$(printf '%s\n' "${TRACKED_IGNORED}" | grep -c '^companion/\.build/' || true)"
  TEST_EVIDENCE_COUNT="$(printf '%s\n' "${TRACKED_IGNORED}" | grep -c '^TestEvidence/' || true)"
  STEAM_SDK_COUNT="$(printf '%s\n' "${TRACKED_IGNORED}" | grep -c '^third_party/steam-audio/sdk/' || true)"
  OTHER_IGNORED_COUNT=$((TRACKED_IGNORED_COUNT - COMPANION_BUILD_COUNT - TEST_EVIDENCE_COUNT - STEAM_SDK_COUNT))

  echo "1. ${REF} currently contains committed generated/ignored artifacts (problem)."
  echo
  echo "- ${TRACKED_IGNORED_COUNT} tracked paths match ignore rules."
  echo "- ${COMPANION_BUILD_COUNT} are under companion/.build (Swift build cache/output)."
  echo "- ${TEST_EVIDENCE_COUNT} are under TestEvidence/* ignored patterns."
  echo "- ${STEAM_SDK_COUNT} are under third_party/steam-audio/sdk (local Steam SDK cache)."
  if [[ "${OTHER_IGNORED_COUNT}" -gt 0 ]]; then
    echo "- ${OTHER_IGNORED_COUNT} additional ignored tracked paths exist in other locations."
  fi
else
  echo "1. ${REF} committed generated/ignored artifact check (good)."
  echo
  echo "- 0 tracked paths match ignore rules."
fi

echo

# 2) Non-release archives in current tree.
NON_RELEASE_ARCHIVES="$(grep -E '^TestEvidence/archive/.*\.(tar\.gz|tgz|zip)$' "${TREE_PATHS_FILE}" || true)"
NON_RELEASE_ARCHIVE_COUNT="$(printf '%s\n' "${NON_RELEASE_ARCHIVES}" | count_lines)"

if [[ "${NON_RELEASE_ARCHIVE_COUNT}" -gt 0 ]]; then
  problem
  echo "2. Non-release archive is committed on ${REF} (problem)."
  echo

  while IFS= read -r archive_path; do
    [[ -z "${archive_path}" ]] && continue
    archive_size="$(git cat-file -s "${REF}:${archive_path}" 2>/dev/null || echo 0)"
    archive_size_mb="$(bytes_to_mb "${archive_size}")"
    echo "- ${archive_path} (${archive_size_mb} MB)."
  done <<< "${NON_RELEASE_ARCHIVES}"
else
  echo "2. Non-release archive check on ${REF} (good)."
  echo
  echo "- No tracked TestEvidence archive bundles found."
fi

echo

# 3) Reachable history large accidental artifacts.
if [[ "${CHECK_HISTORY}" -eq 1 ]]; then
  MIN_BYTES=$((LARGE_BLOB_MB * 1024 * 1024))

  git rev-list --objects --all > "${OBJECTS_FILE}"
  git cat-file --batch-check='%(objectname) %(objecttype) %(objectsize) %(rest)' < "${OBJECTS_FILE}" > "${CATALOG_FILE}"

  awk -v min_bytes="${MIN_BYTES}" '$2 == "blob" && $3 >= min_bytes { print }' "${CATALOG_FILE}" \
    | sort -k3,3nr > "${LARGE_FILE}"

  LARGE_COUNT="$(count_lines < "${LARGE_FILE}")"

  if [[ "${LARGE_COUNT}" -gt 0 ]]; then
    problem
    echo "3. History still contains large accidental artifacts (problem, even if not in current tree)."
    echo

    head -n "${SHOW_TOP}" "${LARGE_FILE}" \
      | awk '{ size=$3; $1=$2=$3=""; sub(/^ +/, ""); printf "- %.2f MB %s\n", size/1048576, $0 }'

    STEAM_AUDIO_HITS="$(grep -c 'third_party/steam-audio/steamaudio_.*\.zip' "${LARGE_FILE}" || true)"
    if [[ "${STEAM_AUDIO_HITS}" -gt 0 ]]; then
      echo "- Steam Audio SDK zip blob(s) are still reachable in history."
    fi

    LOCUSQ_QA_HITS="$(grep -Ec 'build(_bl[0-9]+|_[^/]+)?/.*/locusq_qa$|build_bl[0-9]+/.*/locusq_qa$' "${LARGE_FILE}" || true)"
    if [[ "${LOCUSQ_QA_HITS}" -gt 0 ]]; then
      echo "- Build-output binaries (including locusq_qa) remain reachable in history."
    fi
  else
    echo "3. Reachable-history large artifact check (good)."
    echo
    echo "- No reachable blobs >= ${LARGE_BLOB_MB} MB were found."
  fi
else
  echo "3. Reachable-history large artifact check skipped (--no-history)."
fi

echo

# 4) Submodule/subrepo checks.
GITLINKS="$(git ls-tree -r "${REF}" | awk '$1 == "160000" { print $4 }' || true)"
GITLINK_COUNT="$(printf '%s\n' "${GITLINKS}" | count_lines)"

GITMODULES_TRACKED=0
if grep -qx '.gitmodules' "${TREE_PATHS_FILE}"; then
  GITMODULES_TRACKED=1
fi

NESTED_GIT_PATHS="$(grep -E '(^|/)\.git($|/)' "${TREE_PATHS_FILE}" || true)"
NESTED_GIT_COUNT="$(printf '%s\n' "${NESTED_GIT_PATHS}" | count_lines)"

if [[ "${GITLINK_COUNT}" -eq 0 && "${GITMODULES_TRACKED}" -eq 0 && "${NESTED_GIT_COUNT}" -eq 0 ]]; then
  echo "4. Subrepo/submodule check (good)."
  echo
  echo "- No git submodules (gitlink entries)."
  echo "- No tracked .gitmodules."
  echo "- No tracked nested .git directories."
else
  problem
  echo "4. Subrepo/submodule check (problem)."
  echo
  echo "- gitlink entries: ${GITLINK_COUNT}"
  echo "- tracked .gitmodules: ${GITMODULES_TRACKED}"
  echo "- tracked nested .git paths: ${NESTED_GIT_COUNT}"
fi

echo

# 5) Local zip risk not yet ignored.
if [[ "${CHECK_LOCAL_RISK}" -eq 1 && "${REF}" == "HEAD" ]]; then
  CALIBRATION_ZIPS=()
  if [[ -d "Documentation/Calibration POC" ]]; then
    while IFS= read -r zip_path; do
      [[ -n "${zip_path}" ]] && CALIBRATION_ZIPS+=("${zip_path}")
    done < <(find "Documentation/Calibration POC" -maxdepth 1 -type f -name '*.zip' | sort)
  fi

  UNIGNORED_LOCAL_ZIPS=()
  TRACKED_LOCAL_ZIPS=()

  for zip_path in "${CALIBRATION_ZIPS[@]:-}"; do
    [[ -z "${zip_path}" ]] && continue

    if ! git check-ignore -q -- "${zip_path}"; then
      UNIGNORED_LOCAL_ZIPS+=("${zip_path}")
    fi

    if git ls-files --error-unmatch -- "${zip_path}" >/dev/null 2>&1; then
      TRACKED_LOCAL_ZIPS+=("${zip_path}")
    fi
  done

  if [[ "${#UNIGNORED_LOCAL_ZIPS[@]}" -gt 0 ]]; then
    problem
    echo "5. Local risk (not committed yet, but easy to accidentally add)."
    echo
    echo "- Unignored zips in Documentation/Calibration POC:"
    for zip_path in "${UNIGNORED_LOCAL_ZIPS[@]}"; do
      echo "  - ${zip_path}"
    done
  else
    echo "5. Local risk check for Documentation/Calibration POC zips (good)."
    echo
    echo "- No unignored zip files detected in Documentation/Calibration POC."
  fi

  if [[ "${#TRACKED_LOCAL_ZIPS[@]}" -gt 0 ]]; then
    echo "- NOTE: ${#TRACKED_LOCAL_ZIPS[@]} zip file(s) are already tracked in index/history."
  fi
else
  echo "5. Local risk check skipped (ref != HEAD or --no-local-risk)."
fi

echo

if [[ "${PROBLEMS}" -eq 0 ]]; then
  echo "Validation status: tested (full local git audit completed against ${SHORT_SHA}; no blocking issues detected)."
else
  echo "Validation status: partially tested (full local git audit completed against ${SHORT_SHA}; ${PROBLEMS} blocking category/categories detected)."
fi

if [[ "${STRICT}" -eq 1 && "${PROBLEMS}" -gt 0 ]]; then
  exit 1
fi
