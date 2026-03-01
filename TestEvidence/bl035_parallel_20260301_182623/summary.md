Title: BL-035 RT Lock-Free Registration Evidence Summary (Parallel Replay)
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-035 Evidence-Only Replay Summary

Evidence directory: `TestEvidence/bl035_parallel_20260301_182623`

## Command Status

1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
   - Exit code: `0`
   - Result: `PASS`

2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
   - Exit code: `0`
   - Result: `PASS`
   - Full output log: `TestEvidence/bl035_parallel_20260301_182623/qa_smoke.log`

3. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh`
   - Exit code: `0`
   - Result: `PASS`
   - Full output log: `TestEvidence/bl035_parallel_20260301_182623/selftest.log`

4. `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl035_parallel_20260301_182623/rt_audit.tsv`
   - Exit code: `1`
   - Result: `FAIL`
   - Output artifact: `TestEvidence/bl035_parallel_20260301_182623/rt_audit.tsv`

5. `./scripts/validate-docs-freshness.sh`
   - Exit code: `0`
   - Result: `PASS`

## Required Artifacts

- `TestEvidence/bl035_parallel_20260301_182623/qa_smoke.log`
- `TestEvidence/bl035_parallel_20260301_182623/selftest.log`
- `TestEvidence/bl035_parallel_20260301_182623/rt_audit.tsv`
- `TestEvidence/bl035_parallel_20260301_182623/status.tsv`
- `TestEvidence/bl035_parallel_20260301_182623/summary.md`
