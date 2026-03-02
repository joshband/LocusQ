#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

failures=0

require_literal() {
  local file="$1"
  local needle="$2"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "ERROR: ${file} missing required section: ${needle}" >&2
    failures=$((failures + 1))
  fi
}

visual_block_has_table() {
  local file="$1"
  awk '
    BEGIN { in_block=0; has_table=0 }
    /^## Visual Aid Index/ { in_block=1; next }
    /^## / && in_block { exit }
    in_block && /^\|/ { has_table=1 }
    END { if (has_table) exit 0; exit 1 }
  ' "${file}"
}

visual_block_has_visual_terms() {
  local file="$1"
  awk '
    BEGIN { in_block=0 }
    /^## Visual Aid Index/ { in_block=1; next }
    /^## / && in_block { exit }
    in_block { print }
  ' "${file}" | grep -Eiq 'table|mermaid|diagram|screenshot|image|chart'
}

check_file() {
  local file="$1"
  if grep -Eq '^Document Type: Backlog Support$' "${file}"; then
    return
  fi

  require_literal "${file}" "## Plain-Language Summary"
  require_literal "${file}" "## 6W Snapshot (Who/What/Why/How/When/Where)"
  require_literal "${file}" "## Visual Aid Index"

  if ! visual_block_has_table "${file}"; then
    echo "ERROR: ${file} Visual Aid Index must include a markdown table" >&2
    failures=$((failures + 1))
  fi

  if ! visual_block_has_visual_terms "${file}"; then
    echo "ERROR: ${file} Visual Aid Index must mention at least one visual aid type" >&2
    failures=$((failures + 1))
  fi
}

collect_files() {
  local files=()
  shopt -s nullglob
  files+=(Documentation/backlog/bl-*.md)
  files+=(Documentation/backlog/hx-*.md)
  files+=(Documentation/backlog/done/bl-*.md)
  files+=(Documentation/backlog/done/hx-*.md)
  shopt -u nullglob
  printf '%s\n' "${files[@]}"
}

main() {
  local count=0
  while IFS= read -r file; do
    [[ -z "${file}" ]] && continue
    count=$((count + 1))
    check_file "${file}"
  done < <(collect_files)

  if [[ "${count}" -eq 0 ]]; then
    echo "WARN: no backlog runbooks found for readability validation" >&2
    exit 0
  fi

  if [[ "${failures}" -gt 0 ]]; then
    echo "FAIL: backlog readability checks found ${failures} issue(s)." >&2
    exit 1
  fi

  echo "PASS: backlog readability checks passed for ${count} runbook(s)."
}

main "$@"
