Title: HX-04 Scenario Coverage Audit
Document Type: Test Plan
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# HX-04 Scenario Coverage Audit (2026-02-23)

## Objective
Close HX-04 by enforcing deterministic scenario parity coverage for:
- AirAbsorption
- CalibrationEngine
- directivity path (`emit_dir` + aim)

## Guard Assets
- Required manifest: `qa/scenarios/locusq_hx04_required_scenarios.json`
- Parity suite: `qa/scenarios/locusq_hx04_component_parity_suite.json`
- Audit lane: `scripts/qa-hx04-scenario-audit.sh`
- BL-012 embedded enforcement: `scripts/qa-bl012-harness-backport-tranche1-mac.sh`

## Validation Commands
1. Standalone audit lane:
```sh
./scripts/qa-hx04-scenario-audit.sh --qa-bin build_local/locusq_qa_artefacts/Release/locusq_qa
```
Result: `PASS_WITH_WARNINGS`
- Artifact: `TestEvidence/hx04_scenario_audit_20260223T035254Z/status.tsv`
- Coverage matrix: `TestEvidence/hx04_scenario_audit_20260223T035254Z/coverage_matrix.tsv`

2. Targeted directivity sample scenario:
```sh
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_directivity_aim.json
```
Result: `PASS`
- Artifact: `TestEvidence/hx04_sample_directivity_aim_20260223T035307Z.log`

3. BL-012 tranche-1 lane with HX-04 embedded:
```sh
LQ_BL012_RUN_HARNESS_SANITY=0 LQ_BL012_QA_BIN=build_local/locusq_qa_artefacts/Release/locusq_qa ./scripts/qa-bl012-harness-backport-tranche1-mac.sh
```
Result: `PASS_WITH_WARNINGS`
- Artifact: `TestEvidence/bl012_harness_backport_20260223T035318Z/status.tsv`

## Coverage Matrix Snapshot
| Component | Scenario ID | Guarded In Suite |
|---|---|---|
| AirAbsorption | `locusq_air_absorption_distance` | `locusq_hx04_component_parity_suite` |
| CalibrationEngine | `locusq_calibration_sweep_capture` | `locusq_hx04_component_parity_suite` |
| DirectivityFilter | `locusq_emit_dir_spatial_effect` | `locusq_hx04_component_parity_suite` |
| DirectivityFilter | `locusq_directivity_aim` | `locusq_hx04_component_parity_suite` |

## Closeout
HX-04 closure criteria are met:
- required scenarios are declared and machine-audited,
- dedicated parity suite exists and executes,
- BL-012 lane now fails on parity drift by default.
