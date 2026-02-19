Title: LocusQ Test Summary (Full Acceptance Rerun Bridge Fix)
Document Type: Test Evidence Summary
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# LocusQ Test Summary (Full Acceptance Rerun Bridge Fix)

## Run Context

- Command intent: rerun full acceptance matrix after host bridge/interactivity triage fix.
- Run ID: `test_full_acceptance_rerun_bridge_fix_20260219T212613Z`
- Status ledger: `plugins/LocusQ/TestEvidence/test_full_acceptance_rerun_bridge_fix_20260219T212613Z_status.tsv`

## Overall Verdict

- Final `/test` verdict: `PASS_WITH_WARNING`
- No blocking QA regressions in this rerun (`phase_2_5`, `phase_2_6`, `phase_2_9`, `phase_2_11` all hard-pass).
- Remaining warn-only items:
  - `locusq_phase_2_8_output_layout_stereo_suite` (`2 PASS / 1 WARN / 0 FAIL`)
  - `locusq_phase_2_8_output_layout_quad_suite` (`2 PASS / 1 WARN / 0 FAIL`)
- Manual DAW UI acceptance remains pending rerun after bridge fix.

## Matrix Results Snapshot

- Harness sanity: `PASS` (`45/45`) on retry (`performance_profiler_test` transient failure on first pass).
- `locusq_smoke_suite`: `PASS` (`4 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_5_acceptance_suite`: `PASS` (`9 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_6_acceptance_suite`: `PASS` (`3 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_8_output_layout_stereo_suite`: `WARN` (`2 PASS / 1 WARN / 0 FAIL`)
- `locusq_phase_2_8_output_layout_quad_suite`: `WARN` (`2 PASS / 1 WARN / 0 FAIL`)
- `locusq_phase_2_9_renderer_cpu_suite`: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)
- `locusq_phase_2_11_snapshot_migration_suite`: `PASS` (`2 PASS / 0 WARN / 0 FAIL`)
- `pluginval` strictness 5 (`skip GUI`): `SUCCESS` (exit `0`)
- `pluginval` strictness 5 (`with GUI` / editor automation enabled): `SUCCESS` (exit `0`)
- Standalone open smoke: `PASS` (exit `0`)

## Perf Snapshot

- `qa_full_system_48k512`: `perf_avg_block_time_ms=0.0687689`, `perf_p95_block_time_ms=0.0904191`, `perf_allocation_free=true`
- `qa_host_edge_roundtrip`: `PASS` (non-finite/discontinuity guards hold)

## Primary Evidence

- `plugins/LocusQ/TestEvidence/qa_phase_2_5_suite_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_stereo_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_phase_2_8_quad_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/qa_full_system_48k512_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
- `plugins/LocusQ/TestEvidence/pluginval_strict5_with_gui_test_full_acceptance_rerun_bridge_fix_20260219T212613Z.log`
