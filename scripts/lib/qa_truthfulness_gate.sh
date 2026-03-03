#!/usr/bin/env bash
# Title: QA Truthfulness Gate Helper Library
# Document Type: Script Library
# Author: APC Codex
# Created Date: 2026-03-03
# Last Modified Date: 2026-03-03
#
# Shared helper functions for TODO/scaffold truthfulness gate lanes.

qa_truthfulness_count_scaffold_rows_in_tsv() {
  local file="$1"
  [[ -f "$file" ]] || {
    echo 0
    return
  }

  awk -F'\t' '
    NR == 1 { next }
    {
      for (i = 1; i <= NF; ++i)
      {
        token = toupper($i)
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", token)
        if (token == "TODO" || token == "SCAFFOLD")
        {
          count++
          break
        }
      }
    }
    END { print count + 0 }
  ' "$file"
}

qa_truthfulness_count_scaffold_rows_in_dir() {
  local dir="$1"
  [[ -d "$dir" ]] || {
    echo 0
    return
  }

  local total=0
  local count=0

  while IFS= read -r tsv_file; do
    count="$(qa_truthfulness_count_scaffold_rows_in_tsv "$tsv_file")"
    total=$(( total + count ))
  done < <(find "$dir" -maxdepth 1 -type f -name '*.tsv' | sort)

  echo "$total"
}

qa_truthfulness_mode_policy() {
  local mode="$1"
  case "$mode" in
    contract_only) echo "allow_scaffold_rows" ;;
    execute) echo "enforce_no_scaffold_rows" ;;
    *) echo "unknown_mode" ;;
  esac
}

qa_truthfulness_expected_exit() {
  local mode="$1"
  local scaffold_rows="$2"

  case "$mode" in
    contract_only)
      echo 0
      ;;
    execute)
      if [[ "$scaffold_rows" -gt 0 ]]; then
        echo 1
      else
        echo 0
      fi
      ;;
    *)
      echo 2
      ;;
  esac
}
