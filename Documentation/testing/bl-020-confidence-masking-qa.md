Title: BL-020 Confidence Masking QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-28

# BL-020 Confidence/Masking QA Contract

## Purpose

Define deterministic acceptance checks and evidence schema for BL-020 confidence/masking overlays.

## Input Contract Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL020-QA-001 | Field presence/type | required keys present with contract types |
| BL020-QA-002 | Range/finiteness | all normalized confidence/masking values finite in `[0,1]` |
| BL020-QA-003 | Combined confidence formula | abs delta vs formula `<= 0.01` |
| BL020-QA-004 | Bucket threshold mapping | bucket matches deterministic thresholds |
| BL020-QA-005 | Fallback token behavior | fallback rows contain deterministic reason token |
| BL020-QA-006 | Sequence monotonicity | `snapshotSeq` non-decreasing |

## Deterministic Rendering + Degradation Contract

- Same input payload must produce the same bucket classification on replay.
- Overlay layer degrades independently: base emitter visual remains available.
- Invalid payload rows are classified and tokenized, not silently ignored.
- Degradation must be deterministic and machine-auditable via taxonomy IDs.

Combined-confidence formula contract:
- `combined_confidence = 0.40*distance_confidence + 0.30*(1.0-occlusion_probability) + 0.20*hrtf_match_quality + 0.10*(1.0-masking_index)`

## Acceptance Matrix Contract

Required `acceptance_matrix.tsv` rows:

| acceptance_id | gate | threshold |
|---|---|---|
| BL020-A1-001 | required field/type validity | 100% valid active rows |
| BL020-A1-002 | numeric range/finiteness | 0 violations |
| BL020-A1-003 | formula conformance | max abs delta `<= 0.01` |
| BL020-A1-004 | bucket determinism | 100% match |
| BL020-A1-005 | fallback determinism | 100% fallback rows tokenized |
| BL020-A1-006 | sequence monotonicity | 0 regressions |
| BL020-A1-007 | artifact schema completeness | all required files + columns present |

## Failure Taxonomy Contract

| failure_id | category | trigger | classification | blocking | severity |
|---|---|---|---|---|---|
| BL020-FX-001 | schema_missing_required_field | missing key/type mismatch | deterministic_contract_failure | yes | major |
| BL020-FX-002 | value_out_of_range_or_non_finite | value out of range or NaN/Inf | deterministic_contract_failure | yes | major |
| BL020-FX-003 | combined_confidence_formula_mismatch | formula delta exceeds tolerance | deterministic_contract_failure | yes | major |
| BL020-FX-004 | overlay_bucket_mismatch | threshold bucket mismatch | deterministic_contract_failure | yes | major |
| BL020-FX-005 | fallback_reason_missing_or_invalid | fallback token absent/invalid | deterministic_contract_failure | yes | major |
| BL020-FX-006 | snapshot_sequence_non_monotonic | sequence regression | deterministic_contract_failure | yes | critical |
| BL020-FX-007 | artifact_schema_incomplete | missing required artifact/columns | deterministic_evidence_failure | yes | major |

## Artifact Requirements

Required evidence bundle:
`TestEvidence/bl020_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `contract_spec.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Slice B1 QA Lane Harness Contract

Canonical command:
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh`

Supported options:
- `--contract-only`
- `--execute-suite`
- `--runs <N>`
- `--out-dir <path>`
- `--help|-h`

Deterministic replay rules:
- Lane computes canonical replay hash input from mode plus contract/suite content hashes.
- Replay hashes must remain identical across repeated runs in a single invocation.
- Any replay hash divergence is a deterministic lane failure.

Exit semantics:
- `0` = lane pass
- `1` = lane/contract failure
- `2` = usage error

B1 required lane outputs:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`

B1 failure taxonomy IDs:

| failure_id | category | trigger | classification | blocking |
|---|---|---|---|---|
| BL020-B1-FX-001 | contract_preflight | contract doc missing | deterministic_contract_failure | yes |
| BL020-B1-FX-002 | suite_preflight | suite file missing | deterministic_contract_failure | yes |
| BL020-B1-FX-003 | suite_preflight | QA binary not executable in execute-suite mode | deterministic_lane_failure | yes |
| BL020-B1-FX-010 | acceptance_contract_incomplete | acceptance IDs missing from contract doc | deterministic_contract_failure | yes |
| BL020-B1-FX-011 | failure_taxonomy_incomplete | failure taxonomy IDs missing from contract doc | deterministic_contract_failure | yes |
| BL020-B1-FX-012 | formula_clause_missing | formula clause missing from contract doc | deterministic_contract_failure | yes |
| BL020-B1-FX-013 | artifact_schema_missing | required artifact schema clause missing | deterministic_contract_failure | yes |
| BL020-B1-FX-014 | replay_hash_mismatch | canonical replay hash mismatch across runs | deterministic_lane_failure | yes |
| BL020-B1-FX-020 | suite_execution_failed | execute-suite run returned non-zero | deterministic_lane_failure | yes |

## Validation

- `./scripts/validate-docs-freshness.sh`

## Slice C1 Native Contract Bridge QA

Scope:
- Validate additive native confidence/masking contract publication from `PluginProcessor` without changing existing UI bridge contracts.

Native payload contract (`locusq-confidence-masking-contract-v1`) required fields:
- `schema`
- `snapshotSeq`
- `distanceConfidence`
- `occlusionProbability`
- `hrtfMatchQuality`
- `maskingIndex`
- `combinedConfidence`
- `overlayAlpha`
- `overlayBucket`
- `fallbackReason`
- `valid`

C1 deterministic checks:
- `BL020-C1-001`: all scalar fields are finite and clamped to `[0,1]`.
- `BL020-C1-002`: `combinedConfidence` matches formula within absolute tolerance `<= 0.01`.
- `BL020-C1-003`: `overlayBucket` threshold mapping is deterministic (`low <0.40`, `mid <0.80`, `high >=0.80`).
- `BL020-C1-004`: publication remains additive and backward-compatible (legacy payload paths unchanged).
- `BL020-C1-005`: publication path is RT-safe (no process-block locks/allocations for payload publication).

C1 validation commands:
- `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
- `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
- `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl020_slice_c1_native_<timestamp>/rt_audit.tsv`
- `./scripts/validate-docs-freshness.sh`

C1 evidence bundle:
- `TestEvidence/bl020_slice_c1_native_<timestamp>/status.tsv`
- `TestEvidence/bl020_slice_c1_native_<timestamp>/build.log`
- `TestEvidence/bl020_slice_c1_native_<timestamp>/qa_smoke.log`
- `TestEvidence/bl020_slice_c1_native_<timestamp>/rt_audit.tsv`
- `TestEvidence/bl020_slice_c1_native_<timestamp>/diagnostics_snapshot.json`
- `TestEvidence/bl020_slice_c1_native_<timestamp>/contract_delta.md`
- `TestEvidence/bl020_slice_c1_native_<timestamp>/docs_freshness.log`

## Slice C3 Post-C2 Re-verify Contract

Purpose:
- Confirm BL-020 C1 native bridge lane is green end-to-end after RT gate reconciliation (C2).

C3 validation commands:
- `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
- `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
- `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl020_slice_c3_reverify_<timestamp>/rt_audit.tsv`
- `./scripts/validate-docs-freshness.sh`

C3 pass criteria:
- build exit code `0`
- smoke exit code `0`
- RT summary reports `non_allowlisted=0`
- docs freshness exit code `0`

C3 required evidence bundle:
- `TestEvidence/bl020_slice_c3_reverify_<timestamp>/status.tsv`
- `TestEvidence/bl020_slice_c3_reverify_<timestamp>/validation_matrix.tsv`
- `TestEvidence/bl020_slice_c3_reverify_<timestamp>/build.log`
- `TestEvidence/bl020_slice_c3_reverify_<timestamp>/qa_smoke.log`
- `TestEvidence/bl020_slice_c3_reverify_<timestamp>/rt_audit.tsv`
- `TestEvidence/bl020_slice_c3_reverify_<timestamp>/reverify_notes.md`
- `TestEvidence/bl020_slice_c3_reverify_<timestamp>/docs_freshness.log`

## C4 Validation (Execute-Mode Parity + Exit Guard)

Acceptance matrix:

| acceptance_id | gate | threshold |
|---|---|---|
| BL020-C4-001 | Contract-only replay sentinel | `--contract-only --runs 20` with deterministic replay (`deterministic_match=yes` for all rows) |
| BL020-C4-002 | Execute-suite replay sentinel | `--execute-suite --runs 20` with deterministic replay (`deterministic_match=yes` for all rows) |
| BL020-C4-003 | Cross-mode parity summary | contract/execute doc and scenario hashes remain identical (`mismatch_count=0`) |
| BL020-C4-004 | Contract taxonomy stability | contract/execute taxonomy files contain zero blocking failure rows |
| BL020-C4-005 | Usage probe `--runs 0` | exit code must be `2` |
| BL020-C4-006 | Usage probe `--unknown-flag` | exit code must be `2` |
| BL020-C4-007 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

Execution-mode note:
- For C4 parity determinism, `--execute-suite` runs execute-mode parity contract checks while runtime suite execution remains reserved.

C4 failure taxonomy:

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| BL020-C4-FX-001 | c4_backlog_contract_missing | backlog C4 validation/evidence references absent | deterministic_contract_failure | yes | major | contract_runs/validation_matrix.tsv |
| BL020-C4-FX-002 | c4_qa_contract_missing | QA C4 validation/evidence references absent | deterministic_contract_failure | yes | major | execute_runs/validation_matrix.tsv |
| BL020-C4-FX-003 | c4_scenario_contract_missing | scenario C4 mode-parity/exit metadata absent | deterministic_contract_failure | yes | major | qa/scenarios/locusq_bl020_confidence_masking_suite.json |
| BL020-C4-FX-004 | c4_script_exit_semantics_missing | script mode/usage exit semantics declaration absent | deterministic_contract_failure | yes | major | scripts/qa-bl020-confidence-masking-lane-mac.sh |
| BL020-FX-401 | lane_c4_mode_parity_failure | mode parity summary contains mismatches | deterministic_replay_failure | yes | critical | mode_parity.tsv |
| BL020-FX-402 | lane_c4_exit_semantics_failure | usage probe exit code differs from `2` | deterministic_contract_failure | yes | major | exit_semantics_probe.tsv |
| BL020-FX-403 | lane_c4_docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes | major | docs_freshness.log |
| BL020-FX-404 | lane_c4_evidence_schema_incomplete | required C4 files missing from bundle | deterministic_evidence_failure | yes | major | status.tsv |

C4 validation commands:
- `bash -n scripts/qa-bl020-confidence-masking-lane-mac.sh`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --help`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/contract_runs`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/execute_runs`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --unknown-flag` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## C4 Evidence Contract

Required files under `TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `execute_runs/validation_matrix.tsv`
- `execute_runs/replay_hashes.tsv`
- `mode_parity.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

Validation status labels:
- `tested` = command run and exit as expected
- `partially tested` = command run but incomplete evidence
- `not tested` = command not run

## C4 Done-Candidate Outcome (2026-02-28)

### Packet
- `TestEvidence/bl020_slice_c4_mode_parity_20260228T170633Z` (prior)

### Validation Result
- `bash -n scripts/qa-bl020-confidence-masking-lane-mac.sh`: PASS
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --help`: PASS
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 20 ...`: PASS
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs 20 ...`: PASS
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --runs 0`: PASS (`2`)
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --unknown-flag`: PASS (`2`)
- `./scripts/validate-docs-freshness.sh`: PASS

### C4 Parity Readout
- Contract runs observed: `20`
- Execute runs observed: `20`
- Contract signature drift count: `0`
- Execute signature drift count: `0`
- Cross-mode doc hash mismatch count: `0`
- Cross-mode scenario hash mismatch count: `0`
- Contract failure rows: `0`
- Execute failure rows: `0`
- `lane_result`: `PASS`

### Latest Packet (2026-02-28)

- Packet: `TestEvidence/bl020_slice_c4_mode_parity_20260228T175923Z`
- Validation command outcomes:
  - `bash -n scripts/qa-bl020-confidence-masking-lane-mac.sh` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --help` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_20260228T175923Z/contract_runs` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_20260228T175923Z/execute_runs` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --runs 0` => `2` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --unknown-flag` => `2` (`PASS`)
  - `./scripts/validate-docs-freshness.sh` => `0` (`PASS`)
- Parity readout:
  - Contract runs observed: `20`
  - Execute runs observed: `20`
  - Contract signature drift count: `0`
  - Execute signature drift count: `0`
  - Cross-mode doc hash mismatch count: `0`
  - Cross-mode scenario hash mismatch count: `0`
  - Contract failure rows: `0`
  - Execute failure rows: `0`
  - Lane result: `PASS`

### C4b Post-R1 Non-Interference Packet (2026-02-28)

- Packet: `TestEvidence/bl020_slice_c4b_mode_parity_20260228T202240Z`
- Validation command outcomes:
  - `bash -n scripts/qa-bl020-confidence-masking-lane-mac.sh` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --help` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 5 --out-dir TestEvidence/bl020_slice_c4b_mode_parity_20260228T202240Z/contract_runs` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs 5 --out-dir TestEvidence/bl020_slice_c4b_mode_parity_20260228T202240Z/execute_runs` => `0` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --runs 0` => `2` (`PASS`)
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --unknown-flag` => `2` (`PASS`)
  - `./scripts/validate-docs-freshness.sh` => `0` (`PASS`)
- Parity readout:
  - Contract runs observed: `5`
  - Execute runs observed: `5`
  - Contract signature drift count: `0`
  - Execute signature drift count: `0`
  - Cross-mode doc hash mismatch count: `0`
  - Cross-mode scenario hash mismatch count: `0`
  - Contract failure rows: `0`
  - Execute failure rows: `0`
  - Lane result: `PASS`

## C4 Owner Intake Snapshot (2026-02-28)

- Readiness: `READY_FOR_OWNER_PROMOTION_REVIEW`
- Canonical packet: `TestEvidence/bl020_slice_c4_mode_parity_20260228T175923Z`
- Supporting non-interference packet: `TestEvidence/bl020_slice_c4b_mode_parity_20260228T202240Z`
- Required gate summary:
  - `--contract-only --runs 20`: `PASS`
  - `--execute-suite --runs 20`: `PASS`
  - parity mismatches: `0`
  - contract/execute failure rows: `0`
  - exit probes: `--runs 0 => 2`, `--unknown-flag => 2`
  - docs freshness: `PASS`
- Ownership safety marker:
  - `SHARED_FILES_TOUCHED: yes`

## C4 Recheck Refresh (2026-02-28, 20260228T203021Z)

Packet:
- `TestEvidence/bl020_slice_c4_mode_parity_20260228T203021Z`

Validation result:
- syntax/help: `PASS`
- contract-only (`runs=20`): `PASS`
- execute-suite (`runs=20`): `PASS`
- usage probes (`--runs 0`, `--unknown-flag`): `PASS` (exit `2`)
- docs freshness: `PASS`

Parity readout:
- contract/execute runs observed: `20/20`
- deterministic replay drift counts: `0/0`
- cross-mode doc/scenario hash mismatch count: `0/0`
- contract/execute failure taxonomy blocking rows: `0/0`
- packet verdict: `PASS`
