Title: LocusQ Test Summary (Non-Manual Acceptance Matrix Post-Fix)
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# LocusQ Test Summary (Non-Manual Acceptance Matrix Post-Fix)

## Run Context

- Command intent: post-fix rerun after Phase 2.5 blocker correction.
- Run ID: `test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z`
- Status ledger: `plugins/LocusQ/TestEvidence/test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z_status.tsv`

## Overall Verdict

- Final `/test` verdict: `PASS_WITH_WARNING`
- Blocking regression resolved: `locusq_25_room_depth_no_coloring` now passes (`rms=-78.2862 dBFS`, min `-80.0 dBFS`).
- Remaining non-blocking warnings:
  - `locusq_phase_2_8_output_layout_stereo_suite` (`2 PASS / 1 WARN / 0 FAIL`)
  - `locusq_phase_2_8_output_layout_quad_suite` (`2 PASS / 1 WARN / 0 FAIL`)

## Matrix Results Snapshot

- Harness sanity: `PASS` (`45/45`)
- `locusq_smoke_suite`: `PASS` (`4 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_5_acceptance_suite`: `PASS` (`9 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_6_acceptance_suite`: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_9_renderer_cpu_suite`: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_11_snapshot_migration_suite`: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)
- `pluginval` strictness 5: `SUCCESS` (exit `0`)
- Standalone open smoke: `PASS` (exit `0`)

## Perf Snapshot

- `qa_full_system_48k512`: `perf_avg_block_time_ms=0.0669724`, `perf_p95_block_time_ms=0.0832101`, `perf_allocation_free=true`
- `qa_guardrail_48k512`: `perf_avg_block_time_ms=0.0850263`, `perf_p95_block_time_ms=0.113294`, `perf_allocation_free=true`

## Primary Evidence

- `plugins/LocusQ/TestEvidence/qa_phase_2_5_suite_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_stereo_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_quad_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/qa_full_system_48k512_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
- `plugins/LocusQ/TestEvidence/pluginval_strict5_test_non_manual_acceptance_matrix_post_25_fix_20260219T204149Z.log`
