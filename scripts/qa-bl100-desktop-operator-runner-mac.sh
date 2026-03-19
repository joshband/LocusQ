#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
MODE="contract-only"
RUNS=1
HARNESS_DIR="${ROOT_DIR}/../audio-dsp-qa-harness"
HARNESS_BUILD="${HARNESS_DIR}/build_bl100"
OUT_DIR="${ROOT_DIR}/TestEvidence/bl100_desktop_operator_${TIMESTAMP}"

UPSTREAM_COMMIT="6cea7b95ad34959cd4ce5f7c86eb9bdd88565730"

usage() {
  cat <<EOF
Usage: qa-bl100-desktop-operator-runner-mac.sh [options]

Options:
  --contract-only      Run static contract checks only (default)
  --execute            Run upstream harness tests against build_bl100
  --runs <count>       Number of execute ctest sweeps (default: 1)
  --harness <path>     audio-dsp-qa-harness root (default: ${HARNESS_DIR})
  --build <path>       Harness build dir for BL-100 (default: ${HARNESS_BUILD})
  --out <dir>          Output directory (default: auto-timestamped)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only) MODE="contract-only"; shift ;;
    --execute)       MODE="execute"; shift ;;
    --runs)          RUNS="${2:-}"; shift 2 ;;
    --harness)       HARNESS_DIR="${2:-}"; HARNESS_BUILD="${HARNESS_DIR}/build_bl100"; shift 2 ;;
    --build)         HARNESS_BUILD="${2:-}"; shift 2 ;;
    --out)           OUT_DIR="${2:-}"; shift 2 ;;
    --help|-h)       usage; exit 0 ;;
    *)               echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

mkdir -p "$OUT_DIR"
STATUS_TSV="${OUT_DIR}/status.tsv"
SUMMARY_MD="${OUT_DIR}/summary.md"

printf "check\tresult\tdetail\n" > "$STATUS_TSV"

record() {
  local check="$1" result="$2" detail="$3"
  printf "%s\t%s\t%s\n" "$check" "$result" "$detail" >> "$STATUS_TSV"
}

if [[ "$MODE" == "contract-only" ]]; then
  # S1 — desktop operator core present
  for f in DesktopOperator.h DesktopOperator.cpp MacOSDesktopBackend.h MacOSDesktopBackend.cpp ProcessCommandExecutor.cpp CommandExecutor.h; do
    if [[ -f "${HARNESS_DIR}/lib/desktop_operator/${f}" ]]; then
      record "core_file_${f}" "PASS" "lib/desktop_operator/${f} exists"
    else
      record "core_file_${f}" "FAIL" "lib/desktop_operator/${f} missing"
    fi
  done

  # S2 — artifact contract defined in README
  readme="${HARNESS_DIR}/lib/desktop_operator/README.md"
  for artifact in status.json actions.tsv captures_manifest.json metrics.tsv crash_report_paths.txt; do
    if grep -q "$artifact" "$readme" 2>/dev/null; then
      record "artifact_${artifact//./_}" "PASS" "$artifact documented in README"
    else
      record "artifact_${artifact//./_}" "FAIL" "$artifact missing from README artifact contract"
    fi
  done

  # S2 — failure taxonomy present in C++ source (v1 implemented codes)
  impl_src="${HARNESS_DIR}/lib/desktop_operator/MacOSDesktopBackend.cpp"
  for taxon in launch_failed app_not_ready interaction_failed evidence_missing; do
    if grep -q "\"${taxon}\"" "$impl_src" 2>/dev/null; then
      record "taxonomy_${taxon}" "PASS" "$taxon present in MacOSDesktopBackend.cpp"
    else
      record "taxonomy_${taxon}" "FAIL" "$taxon missing from MacOSDesktopBackend.cpp"
    fi
  done
  # timeout is tracked via timedOut flag in DesktopOperator.cpp
  if grep -q "timedOut" "${HARNESS_DIR}/lib/desktop_operator/DesktopOperator.cpp" 2>/dev/null; then
    record "taxonomy_timeout" "PASS" "timeout tracked via timedOut flag in DesktopOperator.cpp"
  else
    record "taxonomy_timeout" "FAIL" "timeout handling missing from DesktopOperator.cpp"
  fi
  # app_crashed and assertion_failed are v2 scope (deferred per non-goals section)
  record "taxonomy_app_crashed" "INFO" "deferred to v2 — crash report paths collected but app_crashed code not yet emitted"
  record "taxonomy_assertion_failed" "INFO" "deferred to v2 — evidence_missing covers this case in v1"

  # S3 — fixture tests exist
  for t in desktop_operator_test desktop_operator_integration_test; do
    if [[ -f "${HARNESS_DIR}/tests/${t}.cpp" ]]; then
      record "test_source_${t}" "PASS" "tests/${t}.cpp present"
    else
      record "test_source_${t}" "FAIL" "tests/${t}.cpp missing"
    fi
  done

  # S4 — adoption note in README
  if grep -qi "downstream adoption" "$readme" 2>/dev/null; then
    record "adoption_note" "PASS" "downstream adoption section present in README"
  else
    record "adoption_note" "FAIL" "downstream adoption section missing from README"
  fi

  # Upstream commit traceable
  if git -C "${HARNESS_DIR}" cat-file -e "${UPSTREAM_COMMIT}" 2>/dev/null; then
    record "upstream_commit" "PASS" "UPSTREAM_HARNESS_COMMIT=${UPSTREAM_COMMIT} reachable"
  else
    record "upstream_commit" "WARN" "UPSTREAM_HARNESS_COMMIT=${UPSTREAM_COMMIT} not found in harness"
  fi

else
  # Execute mode: run ctest in the existing build_bl100 dir
  if [[ ! -d "$HARNESS_BUILD" ]]; then
    record "execute_preflight" "FAIL" "harness build dir missing: ${HARNESS_BUILD}"
  else
    record "execute_preflight" "PASS" "harness build dir found: ${HARNESS_BUILD}"
  fi

  pass_count=0
  fail_count=0

  for run in $(seq 1 "$RUNS"); do
    log_path="${OUT_DIR}/run_${run}.log"
    if ctest --test-dir "$HARNESS_BUILD" -R 'desktop_operator' --output-on-failure >"$log_path" 2>&1; then
      passed=$(awk '/tests passed/{print $1}' "$log_path" | head -1)
      record "execute_run_${run}" "PASS" "ctest desktop_operator tests=${passed:-?}/? PASS log=${log_path}"
      pass_count=$((pass_count + 1))
    else
      record "execute_run_${run}" "FAIL" "log=${log_path}"
      fail_count=$((fail_count + 1))
    fi
  done

  # Check screenshot artifact from integration test (if QA_DESKTOP_OPERATOR_KEEP_ARTIFACTS=1)
  # In normal mode artifacts are cleaned; record that honesty
  record "screenshot_artifact_note" "INFO" "integration test cleans artifacts unless QA_DESKTOP_OPERATOR_KEEP_ARTIFACTS=1"

  if [[ "$fail_count" -eq 0 ]]; then
    record "lane_result" "PASS" "desktop_operator execute lane passed runs=${RUNS}"
  else
    record "lane_result" "FAIL" "desktop_operator execute lane failed runs=${RUNS} fail_count=${fail_count}"
  fi
fi

{
  echo "# BL-100 Desktop Operator Runner QA"
  echo
  echo "- mode: \`${MODE}\`"
  echo "- runs: \`${RUNS}\`"
  echo "- harness: \`${HARNESS_DIR}\`"
  echo "- upstream_commit: \`${UPSTREAM_COMMIT}\`"
  echo "- status_tsv: \`${STATUS_TSV}\`"
} > "$SUMMARY_MD"

cat "$STATUS_TSV"
