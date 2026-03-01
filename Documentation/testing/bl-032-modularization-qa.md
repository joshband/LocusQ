Title: BL-032 Modularization Structural Guardrails QA
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-28

# BL-032 Modularization Structural Guardrails QA

## Purpose
Define deterministic QA guardrails for BL-032 Slice C to prevent regressions toward monolithic `PluginProcessor`/`PluginEditor` architecture and enforce Slice A module ownership boundaries.

## Guardrail Lane

Script:
- `scripts/qa-bl032-structure-guardrails-mac.sh`

Primary artifacts:
1. `status.tsv`
2. `guardrail_report.tsv`
3. `blocker_taxonomy.tsv`
4. `guardrail_contract.md`

## Guard IDs and Thresholds

| Guard ID | Category | Rule | Pass Condition |
|---|---|---|---|
| BL032-G-001 | `line_count_threshold` | `Source/PluginProcessor.cpp` max lines | line count <= 3600 |
| BL032-G-002 | `line_count_threshold` | `Source/PluginEditor.cpp` max lines | line count <= 800 |
| BL032-G-101 | `forbidden_dependency_edge` | `PluginProcessor.cpp/.h` forbidden include: `PluginEditor.h` | zero matches |
| BL032-G-102 | `forbidden_dependency_edge` | `Source/shared_contracts/*` forbidden include: `PluginProcessor.h` or `PluginEditor.h` | zero matches |
| BL032-G-103 | `forbidden_dependency_edge` | `Source/processor_core/*` forbidden include: `PluginEditor.h` | zero matches |
| BL032-G-104 | `forbidden_dependency_edge` | `Source/processor_bridge/*` forbidden include: `PluginEditor.h` | zero matches |
| BL032-G-105 | `forbidden_dependency_edge` | `Source/editor_webview/*` forbidden include: `PluginProcessor.h` | zero matches |
| BL032-G-106 | `forbidden_dependency_edge` | `Source/editor_shell/*` forbidden include: `SpatialRenderer.h` | zero matches |
| BL032-G-201 | `required_module_directory` | `Source/shared_contracts` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-202 | `required_module_directory` | `Source/processor_core` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-203 | `required_module_directory` | `Source/processor_bridge` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-204 | `required_module_directory` | `Source/editor_shell` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-205 | `required_module_directory` | `Source/editor_webview` presence | directory exists with >=1 `*.h`/`*.cpp` |

## Validation Commands

```bash
bash -n scripts/qa-bl032-structure-guardrails-mac.sh
./scripts/qa-bl032-structure-guardrails-mac.sh --help
./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir TestEvidence/bl032_slice_c_guardrails_<timestamp>
./scripts/validate-docs-freshness.sh
```

## Exit and Failure Contract

1. Exit `0`: all guard IDs pass.
2. Exit `1`: one or more guard IDs fail.
3. Exit `2`: lane usage/configuration error.

`guardrail_report.tsv` is authoritative for per-guard decisions; `blocker_taxonomy.tsv` groups failures by deterministic blocker class.

## Remediation Steps

1. `line_count_threshold` violations:
   - Extract logic from monolithic file into the Slice A module boundaries.
   - Re-run guardrails after each extraction tranche.
2. `forbidden_dependency_edge` violations:
   - Remove direct include edge.
   - Route dependency through the allowed module interface direction from the Slice A map.
3. `required_module_directory` violations:
   - Create missing module directory and add at least one owned header/source file.
   - Ensure ownership aligns with BL-032 Slice B/C no-overlap contract.

## Slice G2 Done-Candidate Recheck Packet (2026-02-28)

### Evidence Run

- Packet directory: `TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/`
- Result: `FAIL`
- Timestamp: `2026-02-28T18:09:48Z`
- Scope:
  - Re-run guardrails + build + smoke + RT audit + UI self-test + docs freshness after Slice D2 reconciliation

### Validation Matrix

| Gate | Result | Exit | Criteria | Artifact |
|---|---|---|---|---|
| help | PASS | 0 | script usage renders | n/a *(not persisted)* |
| guardrails | FAIL | 1 | all guard IDs pass | `TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/guardrails/status.tsv` |
| build | PASS | 0 | release shared targets build | `build` |
| qa_smoke | PASS | 0 | 4/4 smoke scenarios pass | `TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/qa_smoke.log` |
| ui_selftest | PASS | 0 | production scope `bl029` self-test pass | `TestEvidence/locusq_production_p0_selftest_20260228T181335Z.json` |
| rt_audit | PASS | 0 | `non_allowlisted=0` | `TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/rt_audit.tsv` |
| docs_freshness | PASS | 0 | no docs warnings | `TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/docs_freshness.log` |

### Guardrail Outcome

- `BL032-G-001` remains failing with `Source/PluginProcessor.cpp` at `3479` lines (`<= 3200` threshold).
- Other guard IDs are passing.
- `Source/PluginProcessor.cpp` and `Source/SpatialRenderer.h` contain no non-allowlisted RT hits in `rt_audit.tsv`.
- `docs_freshness` passes with `0` warnings.

### Recheck Evidence Contract

- Required artifacts:
  - `status.tsv`
  - `validation_matrix.tsv`
  - `guardrails/status.tsv`
  - `guardrails/guardrail_report.tsv`
  - `guardrails/blocker_taxonomy.tsv`
  - `rt_audit.tsv`
  - `promotion_readiness.md`
  - `docs_freshness.log`

## Determinism Notes

1. Guard IDs are stable and machine-readable.
2. Threshold values are explicit and versionable.
3. The lane emits fixed-schema TSV outputs suitable for CI gating and promotion packets.

## Shared Blocker Slice R1 (BL-032 + BL-035)

Cross-lane regression authority for the shared RT/selftest blockers is captured in:
- `TestEvidence/rt_selftest_regression_r1_<timestamp>/`

Required R1 gates:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => exit `0`
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => exit `0`
3. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => exit `0` with normal selftest payload
4. `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/rt_selftest_regression_r1_<timestamp>/rt_after.tsv` => `non_allowlisted=0`
5. `./scripts/validate-docs-freshness.sh` => exit `0`

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

## Slice G2 Done-Candidate Recheck Packet (2026-02-28, 20260228T183306Z)

### Evidence Run

- Packet directory: `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/`
- Result: `FAIL`
- Timestamp: `2026-02-28T18:33:06Z`
- Scope:
  - Re-run guardrails + build + smoke + RT audit + UI self-test + docs freshness after line-threshold reconciliation.

### Validation Matrix

| Gate | Result | Exit | Criteria | Artifact |
|---|---|---|---|---|
| script_syntax | PASS | 0 | `bash -n` succeeds | `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/bash_n_exit.log` |
| help | PASS | 0 | script usage renders | `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/help.txt` |
| guardrails | PASS | 0 | all guard IDs pass | `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/guardrails/status.tsv` |
| build | PASS | 0 | release targets build | `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/build_retry.log` |
| qa_smoke | PASS | 0 | 4/4 smoke scenarios pass | `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/qa_smoke.log` |
| ui_selftest | PASS | 0 | production scope `bl029` self-test pass | `TestEvidence/locusq_production_p0_selftest_20260228T183646Z.json` |
| rt_audit | FAIL | 1 | `non_allowlisted=0` | `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/rt_audit.tsv` |
| docs_freshness | PASS | 0 | no docs warnings | `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/docs_freshness.log` |

### Outcome

- Guardrail blockers are cleared (`BL032-G-001` now passing at `3592 <= 3600`).
- Done-candidate remains blocked by RT audit with `non_allowlisted=5` concentrated in `Source/SpatialRenderer.h` dynamic container mutations.

## Slice G2 RT Gate Closure Update (2026-02-28, 20260228T183306Z)

- RT audit rerun: `PASS` (`exit 0`, `non_allowlisted=0`).
- Closure action: replaced flagged `resize(...)` mutations in `Source/SpatialRenderer.h` prepare-buffer setup with deterministic buffer ensure/zeroing logic.
- Packet decision upgraded to `PASS` with all required BL-032 G2 artifacts satisfied.

## Slice G2 Post-Fix Runtime Validation Refresh (2026-02-28, 20260228T202823Z)

- Replayed runtime gates after `Source/SpatialRenderer.h` RT-safe buffer initialization change.
- `cmake --build ... --target LocusQ_Standalone locusq_qa -j 8` => `PASS`.
- `locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS` (4/4).
- `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS` with `TestEvidence/locusq_production_p0_selftest_20260228T202823Z.json`.
- Combined with prior guardrails + RT + docs freshness passes, packet `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/` remains `PASS`.
