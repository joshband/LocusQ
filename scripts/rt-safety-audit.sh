#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
ALLOWLIST_PATH="${REPO_ROOT}/scripts/rt-safety-allowlist.txt"
OUTPUT_PATH=""

usage() {
  cat <<'USAGE'
Usage: scripts/rt-safety-audit.sh [--allowlist <path>] [--output <path>] [--print-summary]

Scans the LocusQ processBlock call graph files for RT-unsafe code patterns and emits TSV:
file    line    pattern severity        allowlisted     snippet

Exit code:
  0 => no violations, or all violations allowlisted
  1 => one or more non-allowlisted violations
USAGE
}

PRINT_SUMMARY=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --allowlist)
      [[ $# -lt 2 ]] && { echo "ERROR: --allowlist requires a path" >&2; exit 2; }
      ALLOWLIST_PATH="$2"
      shift 2
      ;;
    --output)
      [[ $# -lt 2 ]] && { echo "ERROR: --output requires a path" >&2; exit 2; }
      OUTPUT_PATH="$2"
      shift 2
      ;;
    --print-summary)
      PRINT_SUMMARY=1
      shift
      ;;
    -h|--help)
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

if ! command -v rg >/dev/null 2>&1; then
  echo "ERROR: ripgrep (rg) is required for rt-safety-audit.sh" >&2
  exit 2
fi

declare -a SCAN_FILES=(
  "Source/PluginProcessor.cpp"
  "Source/SpatialRenderer.h"
  "Source/PhysicsEngine.h"
  "Source/CalibrationEngine.h"
  "Source/FDNReverb.h"
  "Source/EarlyReflections.h"
  "Source/VBAPPanner.h"
  "Source/SpreadProcessor.h"
  "Source/DirectivityFilter.h"
  "Source/DistanceAttenuator.h"
  "Source/DopplerProcessor.h"
  "Source/AirAbsorption.h"
  "Source/SceneGraph.h"
  "Source/VisualTokenScheduler.h"
)

declare -a ALLOWLIST=()
if [[ -f "${ALLOWLIST_PATH}" ]]; then
  while IFS= read -r raw_line; do
    line="${raw_line%%#*}"
    line="$(printf '%s' "$line" | sed -E 's/^[[:space:]]+|[[:space:]]+$//g')"
    [[ -z "${line}" ]] && continue
    ALLOWLIST+=("${line}")
  done < "${ALLOWLIST_PATH}"
fi

tmp_report="$(mktemp)"
trap 'rm -f "${tmp_report}"' EXIT

printf "file\tline\tpattern\tseverity\tallowlisted\tsnippet\n" > "${tmp_report}"

non_allowlisted=0
total_hits=0

is_allowlisted() {
  local key1="$1:$2"
  local key2="$1:$2:$3"
  local entry
  for entry in "${ALLOWLIST[@]-}"; do
    if [[ "${entry}" == "${key1}" || "${entry}" == "${key2}" ]]; then
      return 0
    fi
  done
  return 1
}

scan_rule() {
  local rule_id="$1"
  local severity="$2"
  local regex="$3"
  local file rel result line_num snippet allowlisted

  for rel in "${SCAN_FILES[@]}"; do
    file="${REPO_ROOT}/${rel}"
    [[ -f "${file}" ]] || continue

    while IFS= read -r result; do
      [[ -z "${result}" ]] && continue
      line_num="${result%%:*}"
      snippet="${result#*:}"
      snippet="$(printf '%s' "${snippet}" | tr '\t' ' ' | sed -E 's/[[:space:]]+/ /g')"

      allowlisted="false"
      if is_allowlisted "${rel}" "${line_num}" "${rule_id}"; then
        allowlisted="true"
      else
        non_allowlisted=$((non_allowlisted + 1))
      fi

      total_hits=$((total_hits + 1))
      printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
        "${rel}" "${line_num}" "${rule_id}" "${severity}" "${allowlisted}" "${snippet}" >> "${tmp_report}"
    done < <(rg -n --no-heading -e "${regex}" "${file}" || true)
  done
}

scan_rule "HEAP_ALLOC" "high" "\\b(new|delete|malloc|calloc|realloc|free)\\b"
scan_rule "LOCKING" "high" "std::(mutex|recursive_mutex|timed_mutex|lock_guard|unique_lock)|\\.lock\\s*\\(|\\.unlock\\s*\\("
scan_rule "DYNAMIC_CONTAINER_MUTATION" "medium" "\\b(push_back|emplace_back|resize|reserve)\\s*\\("
scan_rule "STRING_CONSTRUCTION" "medium" "std::string\\s*\\("
scan_rule "BLOCKING_IO" "high" "std::(cout|cerr)|fprintf\\s*\\(|fopen\\s*\\(|fwrite\\s*\\("
scan_rule "MESSAGE_THREAD_CALL" "high" "MessageManager|triggerAsyncUpdate\\s*\\(|callAsync\\s*\\("
scan_rule "EXCEPTIONS" "high" "\\b(throw|try|catch)\\b"

if [[ -n "${OUTPUT_PATH}" ]]; then
  mkdir -p "$(dirname "${OUTPUT_PATH}")"
  cp "${tmp_report}" "${OUTPUT_PATH}"
fi

cat "${tmp_report}"

if [[ "${PRINT_SUMMARY}" -eq 1 ]]; then
  echo "summary\ttotal_hits=${total_hits}\tnon_allowlisted=${non_allowlisted}\tallowlist=${ALLOWLIST_PATH}" >&2
fi

if [[ "${non_allowlisted}" -gt 0 ]]; then
  exit 1
fi

exit 0
