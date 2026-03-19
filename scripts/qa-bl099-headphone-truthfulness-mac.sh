#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

MODE="contract"
RUNS=1
APP_PATH="${ROOT_DIR}/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
OUT_DIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --contract-only)
      MODE="contract"
      shift
      ;;
    --execute)
      MODE="execute"
      shift
      ;;
    --runs)
      RUNS="${2:-1}"
      shift 2
      ;;
    --app)
      APP_PATH="${2:?missing app path}"
      shift 2
      ;;
    --out)
      OUT_DIR="${2:?missing output dir}"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

timestamp() {
  date -u +"%Y%m%dT%H%M%SZ"
}

if [[ -z "$OUT_DIR" ]]; then
  OUT_DIR="${ROOT_DIR}/TestEvidence/bl099_headphone_truthfulness_$(timestamp)"
fi
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
SUMMARY_MD="$OUT_DIR/summary.md"
printf "check\tresult\tdetail\n" > "$STATUS_TSV"

record() {
  printf "%s\t%s\t%s\n" "$1" "$2" "$3" | tee -a "$STATUS_TSV"
}

if [[ "$MODE" == "contract" ]]; then
  if rg -n "selftest_scope === \"bl099\"|UI-P1-099A|UI-P1-099B" Source/ui/src/index.ts >/dev/null; then
    record "selftest_scope" "PASS" "bl099 selftest scope wired in Source/ui/src/index.ts"
  else
    record "selftest_scope" "FAIL" "missing bl099 selftest scope wiring in Source/ui/src/index.ts"
    exit 1
  fi

  if rg -n "rendererHeadphoneVerificationScoreProvenance|rendererHeadphoneVerificationCompensationLabel|rendererHeadphoneVerificationCompensationProvenance" \
      Source/ui/src/index.ts Source/processor_bridge/ProcessorUiBridgeOps.h Source/processor_bridge/ProcessorSceneStateBridgeOps.h Source/shared_contracts/HeadphoneVerificationContract.h >/dev/null; then
    record "contract_fields" "PASS" "BL-099 provenance and compensation fields present across contract and bridge layers"
  else
    record "contract_fields" "FAIL" "missing BL-099 provenance or compensation fields"
    exit 1
  fi

  if rg -n "qa-bl099-headphone-truthfulness-mac.sh|bl099" Documentation/backlog/bl-099-headphone-verification-truthfulness-and-compensation-provenance.md status.json >/dev/null; then
    record "traceability" "PASS" "runbook/status trace present for BL-099"
  else
    record "traceability" "FAIL" "missing BL-099 traceability in runbook or status.json"
    exit 1
  fi

  cat > "$SUMMARY_MD" <<EOF
# BL-099 Contract Check

- Result: PASS
- Scope: contract wiring for score provenance and compensation provenance
- Status TSV: $(basename "$STATUS_TSV")
EOF
  exit 0
fi

if [[ ! -d "$APP_PATH" ]]; then
  record "execute_preflight" "FAIL" "standalone app missing at $APP_PATH"
  exit 1
fi
record "execute_preflight" "PASS" "standalone app found at $APP_PATH"

fail_count=0
for ((run=1; run<=RUNS; run++)); do
  LOG_PATH="$OUT_DIR/run_${run}.log"
  if LOCUSQ_UI_SELFTEST_SCOPE=bl099 ./scripts/standalone-ui-selftest-production-p0-mac.sh "$APP_PATH" >"$LOG_PATH" 2>&1; then
    artifact="$(rg -n '^result_json=' "$LOG_PATH" | tail -n 1 | cut -d= -f2-)"
    record "execute_run_${run}" "PASS" "artifact=${artifact:-unknown}"
  else
    record "execute_run_${run}" "FAIL" "log=$LOG_PATH"
    fail_count=$((fail_count + 1))
  fi
done

if [[ "$fail_count" -eq 0 ]]; then
  record "lane_result" "PASS" "bl099 execute lane passed runs=$RUNS"
  cat > "$SUMMARY_MD" <<EOF
# BL-099 Execute Check

- Result: PASS
- Runs: $RUNS
- Status TSV: $(basename "$STATUS_TSV")
EOF
  exit 0
fi

record "lane_result" "FAIL" "bl099 execute lane failed runs=$RUNS fail_count=$fail_count"
cat > "$SUMMARY_MD" <<EOF
# BL-099 Execute Check

- Result: FAIL
- Runs: $RUNS
- Failures: $fail_count
- Status TSV: $(basename "$STATUS_TSV")
EOF
exit 1
