#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST_PATH="${ROOT_DIR}/third_party/steam-audio/dependency.env"
STEAM_AUDIO_DIR="${ROOT_DIR}/third_party/steam-audio"
SDK_PARENT_DIR="${STEAM_AUDIO_DIR}/sdk"

FORCE_DOWNLOAD=0
FORCE_EXTRACT=0
VERIFY_ONLY=0

usage() {
  cat <<'USAGE'
Usage: ./scripts/install-steam-audio-sdk.sh [options]

Downloads and installs Steam Audio SDK based on:
  third_party/steam-audio/dependency.env

Options:
  --force-download   Re-download archive even if present.
  --force-extract    Re-extract SDK even if include/phonon.h exists.
  --verify-only      Verify archive checksum only; do not extract.
  --help             Show usage.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --force-download)
      FORCE_DOWNLOAD=1
      shift
      ;;
    --force-extract)
      FORCE_EXTRACT=1
      shift
      ;;
    --verify-only)
      VERIFY_ONLY=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ ! -f "${MANIFEST_PATH}" ]]; then
  echo "ERROR: manifest not found: ${MANIFEST_PATH}" >&2
  exit 1
fi

# shellcheck source=/dev/null
source "${MANIFEST_PATH}"

required_vars=(
  STEAM_AUDIO_VERSION
  STEAM_AUDIO_ARCHIVE
  STEAM_AUDIO_URL
  STEAM_AUDIO_SHA256
  STEAM_AUDIO_SDK_SUBDIR
)
for var_name in "${required_vars[@]}"; do
  if [[ -z "${!var_name:-}" ]]; then
    echo "ERROR: ${var_name} missing in ${MANIFEST_PATH}" >&2
    exit 1
  fi
done

ARCHIVE_PATH="${STEAM_AUDIO_DIR}/${STEAM_AUDIO_ARCHIVE}"
SDK_ROOT="${SDK_PARENT_DIR}/${STEAM_AUDIO_SDK_SUBDIR}"
SDK_HEADER="${SDK_ROOT}/include/phonon.h"

require_cmd() {
  local cmd="$1"
  if ! command -v "${cmd}" >/dev/null 2>&1; then
    echo "ERROR: required command not found: ${cmd}" >&2
    exit 1
  fi
}

require_cmd curl
require_cmd shasum
require_cmd unzip

mkdir -p "${STEAM_AUDIO_DIR}" "${SDK_PARENT_DIR}"

if [[ "${FORCE_DOWNLOAD}" -eq 1 || ! -f "${ARCHIVE_PATH}" ]]; then
  echo "Downloading Steam Audio SDK archive (${STEAM_AUDIO_VERSION})..."
  curl -L --fail --retry 3 --retry-delay 2 -o "${ARCHIVE_PATH}" "${STEAM_AUDIO_URL}"
else
  echo "Archive already present: ${ARCHIVE_PATH}"
fi

actual_sha256="$(shasum -a 256 "${ARCHIVE_PATH}" | awk '{print $1}')"
if [[ "${actual_sha256}" != "${STEAM_AUDIO_SHA256}" ]]; then
  echo "ERROR: checksum mismatch for ${ARCHIVE_PATH}" >&2
  echo "Expected: ${STEAM_AUDIO_SHA256}" >&2
  echo "Actual:   ${actual_sha256}" >&2
  exit 1
fi

echo "Checksum verified: ${STEAM_AUDIO_ARCHIVE}"

if [[ "${VERIFY_ONLY}" -eq 1 ]]; then
  echo "Verify-only mode complete."
  exit 0
fi

if [[ "${FORCE_EXTRACT}" -eq 1 && -d "${SDK_ROOT}" ]]; then
  rm -rf "${SDK_ROOT}"
fi

if [[ -f "${SDK_HEADER}" && "${FORCE_EXTRACT}" -eq 0 ]]; then
  echo "Steam Audio SDK already extracted: ${SDK_ROOT}"
  exit 0
fi

echo "Extracting archive to ${SDK_PARENT_DIR}..."
unzip -q -o "${ARCHIVE_PATH}" -d "${SDK_PARENT_DIR}"

if [[ ! -f "${SDK_HEADER}" ]]; then
  echo "ERROR: extraction completed but header missing: ${SDK_HEADER}" >&2
  exit 1
fi

echo "Steam Audio SDK ready at ${SDK_ROOT}"
