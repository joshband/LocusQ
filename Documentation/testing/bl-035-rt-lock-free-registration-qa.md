Title: BL-035 RT Lock-Free Registration QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-035 RT Lock-Free Registration QA Contract

## Purpose

Define the deterministic owner-readiness recheck contract for BL-035 to confirm move eligibility toward Done-candidate intake.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/bl-035-rt-lock-free-registration.md`

Traceability anchors:
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`

## Deterministic Hard Gates

Execution must include the same ordered command lanes for D7 owner readiness:

1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
3. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh`
4. `./scripts/rt-safety-audit.sh --print-summary --output <packet>/rt_audit.tsv`
5. `./scripts/validate-docs-freshness.sh`

Gate-level acceptance thresholds:
- Build exit `0`
- QA smoke exit `0`
- Selftest scope BL-029 exit `0`
- RT gate `non_allowlisted=0`
- Docs freshness exit `0`

## Evidence Contract (D7)

Required in `TestEvidence/bl035_slice_d7_owner_ready_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `rt_audit.tsv`
- `blocker_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Acceptance and Blocker Contract

Failure classification for this packet:
- Runtime/launch blocker: `runtime_selftest_app_exit`
- RT contract blocker: `rt_non_allowlist_regression`
- Schema contract blocker: `docs_freshness_failure` (if docs gate fails)

Acceptance rows are recorded in `status.tsv` and `validation_matrix.tsv`.
Packet is `PASS` only when all gates are `PASS`.

## Shared Blocker Slice R1 (BL-032 + BL-035)

R1 targets the shared blockers observed in BL-032 G2 and BL-035 D7:
- selftest `app_exited_before_result` / `exit 134` / `signal ABRT`
- RT audit non-allowlisted hits at:
  - `Source/PluginProcessor.cpp:2758`
  - `Source/PluginProcessor.cpp:2989`
  - `Source/SpatialRenderer.h:1682`

R1 packet path:
- `TestEvidence/rt_selftest_regression_r1_<timestamp>/`

R1 required gates:
1. `selftest` lane exits `0` and emits normal payload (`status=pass`, `ok=true`).
2. `rt_after.tsv` reports `non_allowlisted=0`.
3. docs freshness gate exits `0`.

R1 required artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `build.log`
- `qa_smoke.log`
- `selftest.log`
- `rt_before.tsv`
- `rt_after.tsv`
- `blocker_taxonomy.tsv`
- `fix_summary.md`
- `docs_freshness.log`
