#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

FAILURES=0
WARNINGS=0

info() {
  printf 'INFO: %s\n' "$*"
}

warn() {
  printf 'WARN: %s\n' "$*" >&2
  WARNINGS=$((WARNINGS + 1))
}

error() {
  printf 'ERROR: %s\n' "$*" >&2
  FAILURES=$((FAILURES + 1))
}

check_metadata_header() {
  local file="$1"
  local header
  header="$(head -n 40 "${file}")"

  local keys=(
    "Title"
    "Document Type"
    "Author"
    "Created Date"
    "Last Modified Date"
  )

  local missing=()
  local key
  for key in "${keys[@]}"; do
    if ! printf '%s\n' "${header}" | grep -Eq "^${key}: .+"; then
      missing+=("${key}")
    fi
  done

  if [[ "${#missing[@]}" -gt 0 ]]; then
    error "${file} is missing metadata fields: ${missing[*]}"
  fi
}

check_markdown_metadata_scope() {
  local md_files=()

  while IFS= read -r file; do
    md_files+=("${file}")
  done < <(
    find . \
      \( -path "./.git" -o -path "./.venv*" -o -path "./build" -o -path "./build_*" -o -path "./build_local" -o -path "./build-qa-local" -o -path "./build_ship_universal" -o -path "./qa_output" -o -path "./tmp" -o -path "./third_party" \) -prune \
      -o -type f -name "*.md" -print
  )

  if [[ "${#md_files[@]}" -eq 0 ]]; then
    warn "No markdown files discovered for metadata checks"
    return
  fi

  local file
  for file in "${md_files[@]}"; do
    check_metadata_header "${file}"

    if [[ "${file}" == "./Documentation/adr/"* ]]; then
      local base
      base="$(basename "${file}")"
      if [[ ! "${base}" =~ ^ADR-[0-9]{4}-[a-z0-9-]+\.md$ ]]; then
        error "ADR file does not follow naming convention ADR-0000-slug.md: ${file}"
      fi
      if ! head -n 30 "${file}" | grep -Eq '^Document Type: Architecture Decision Record$'; then
        error "ADR file must declare 'Document Type: Architecture Decision Record': ${file}"
      fi
    fi
  done
}

extract_status_date() {
  if [[ ! -f "status.json" ]]; then
    error "Missing status.json"
    return 1
  fi

  if ! command -v jq >/dev/null 2>&1; then
    error "jq is required for docs freshness checks"
    return 1
  fi

  local status_date
  status_date="$(jq -r '.last_modified // empty' status.json | cut -d'T' -f1)"
  if [[ -z "${status_date}" || "${status_date}" == "null" ]]; then
    error "status.json is missing a valid last_modified timestamp"
    return 1
  fi

  printf '%s\n' "${status_date}"
}

check_closeout_bundle_sync() {
  local status_date="$1"
  local files=(
    "README.md"
    "CHANGELOG.md"
    "TestEvidence/build-summary.md"
    "TestEvidence/validation-trend.md"
  )

  local file
  for file in "${files[@]}"; do
    if [[ ! -f "${file}" ]]; then
      error "Missing required closeout bundle file: ${file}"
      continue
    fi

    if ! grep -Eq "^Last Modified Date: ${status_date}$" "${file}"; then
      error "${file} Last Modified Date must match status.json date (${status_date})"
    fi
  done

  if [[ -f "TestEvidence/validation-trend.md" ]]; then
    if ! grep -Eq "^\| ${status_date}T" "TestEvidence/validation-trend.md"; then
      error "TestEvidence/validation-trend.md is missing a trend row for ${status_date}"
    fi
  fi
}

check_generated_doc_output_dirs() {
  local generated_dirs=(
    "Documentation/exports"
  )

  local dir
  for dir in "${generated_dirs[@]}"; do
    if [[ ! -d "${dir}" ]]; then
      continue
    fi

    local file_count
    file_count="$(find "${dir}" -type f | wc -l | tr -d ' ')"
    if [[ "${file_count}" -gt 0 ]]; then
      error "${dir} contains ${file_count} generated file(s). Archive or remove these artifacts before closeout."
    fi
  done
}

main() {
  info "Running LocusQ docs freshness checks from ${ROOT_DIR}"

  check_markdown_metadata_scope
  check_generated_doc_output_dirs

  local status_date
  status_date="$(extract_status_date || true)"
  if [[ -n "${status_date}" ]]; then
    check_closeout_bundle_sync "${status_date}"
  fi

  if [[ "${FAILURES}" -gt 0 ]]; then
    printf 'FAIL: %d issue(s), %d warning(s).\n' "${FAILURES}" "${WARNINGS}" >&2
    exit 1
  fi

  printf 'PASS: docs freshness checks passed with %d warning(s).\n' "${WARNINGS}"
}

main "$@"
