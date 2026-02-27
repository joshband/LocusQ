Title: BL-037 Emitter Snapshot CPU Budget QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-037 Emitter Snapshot CPU Budget QA Contract

## Purpose

Define deterministic acceptance checks and evidence schema for BL-037 A1 snapshot CPU-budget contract.

## Deterministic QA Checks

| Check ID | Rule | Pass Condition |
|---|---|---|
| BL037-QA-001 | Snapshot cadence rule coverage | `snapshot_period_blocks`, demand gate, decision order are defined |
| BL037-QA-002 | Late-join window | publish occurs within `<=1` block after demand rise |
| BL037-QA-003 | CPU budget thresholds | p95/max/overrun thresholds present with explicit windows |
| BL037-QA-004 | Budget guard determinism | guard entry (`8` consecutive) and exit (`64` blocks) fixed |
| BL037-QA-005 | Replay determinism identity | hash input set and equality rule explicit |
| BL037-QA-006 | Fallback taxonomy | all required BL037-FX IDs and triggers defined |
| BL037-QA-007 | Artifact schema | required artifacts and TSV columns defined |

## Acceptance Matrix Contract (A1)

Canonical acceptance rows:

| acceptance_id | gate | threshold |
|---|---|---|
| BL037-A1-001 | Frequency contract completeness | all required cadence fields/rules defined |
| BL037-A1-002 | Late-join determinism | first publish after demand rise within `<=1` block |
| BL037-A1-003 | CPU budget envelope defined | p95/max/overrun thresholds all explicit |
| BL037-A1-004 | Degradation policy determinism | guard entry/exit windows fixed and explicit |
| BL037-A1-005 | Replay determinism contract | replay hash inputs + equality rule explicit |
| BL037-A1-006 | Failure taxonomy completeness | all required BL037-FX IDs defined |
| BL037-A1-007 | Artifact schema completeness | required artifact list + columns explicit |
| BL037-A1-008 | Docs freshness | `./scripts/validate-docs-freshness.sh` exit `0` |

Required `acceptance_matrix.tsv` columns:
- `acceptance_id`
- `gate`
- `threshold`
- `measured_value`
- `result`
- `evidence_path`

## Failure Taxonomy Contract (A1)

| failure_id | category | trigger | classification | blocking | severity |
|---|---|---|---|---|---|
| BL037-FX-001 | frequency_contract_incomplete | cadence contract missing field/rule | deterministic_contract_failure | yes | major |
| BL037-FX-002 | late_join_window_violation | first publish exceeds one block | deterministic_contract_failure | yes | critical |
| BL037-FX-003 | cpu_budget_threshold_missing | p95/max/overrun thresholds absent | deterministic_contract_failure | yes | major |
| BL037-FX-004 | degradation_policy_nondeterministic | guard window/exit not fixed | deterministic_contract_failure | yes | major |
| BL037-FX-005 | replay_identity_incomplete | replay hash input set incomplete | deterministic_contract_failure | yes | major |
| BL037-FX-006 | publish_trace_nondeterministic | replay output sequence diverges | deterministic_contract_failure | yes | critical |
| BL037-FX-007 | non_finite_cpu_sample | NaN/Inf budget metric | deterministic_contract_failure | yes | major |
| BL037-FX-008 | artifact_schema_incomplete | required evidence artifact/columns missing | deterministic_evidence_failure | yes | major |

Required `failure_taxonomy.tsv` columns:
- `failure_id`
- `category`
- `trigger`
- `classification`
- `blocking`
- `severity`
- `expected_artifact`

## Evidence Bundle

Required output path:
`TestEvidence/bl037_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `cpu_budget_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`


## Slice B1 Lane QA Harness Contract

Canonical command:
- `./scripts/qa-bl037-snapshot-budget-lane-mac.sh`

Supported options:
- `--contract-only`
- `--execute-suite`
- `--runs <N>`
- `--out-dir <path>`
- `--help|-h`

Exit semantics:
- `0` = pass
- `1` = lane/contract failure
- `2` = usage error

B1 deterministic checks:
- `BL037-B1-001` acceptance ID declarations complete.
- `BL037-B1-002` BL037-FX taxonomy declarations complete.
- `BL037-B1-003` publish decision token clause complete.
- `BL037-B1-004` CPU budget threshold clause complete.
- `BL037-B1-005` replay clause declared.
- `BL037-B1-006` artifact schema declared.
- `BL037-B1-007` lane-specific BL037-B1-FX taxonomy declarations complete.
- `BL037-B1-008` replay hash stable across reruns.

B1 lane failure IDs (required):

| failure_id | category | trigger | classification | blocking |
|---|---|---|---|---|
| BL037-B1-FX-001 | contract_preflight | runbook doc missing | deterministic_contract_failure | yes |
| BL037-B1-FX-002 | contract_preflight | qa doc missing | deterministic_contract_failure | yes |
| BL037-B1-FX-003 | scenario_preflight | scenario file missing | deterministic_contract_failure | yes |
| BL037-B1-FX-004 | suite_preflight | qa binary missing in execute-suite mode | deterministic_lane_failure | yes |
| BL037-B1-FX-010 | acceptance_contract_incomplete | missing BL037-A1 acceptance IDs | deterministic_contract_failure | yes |
| BL037-B1-FX-011 | failure_taxonomy_incomplete | missing BL037-FX base taxonomy IDs | deterministic_contract_failure | yes |
| BL037-B1-FX-012 | decision_clause_missing | publish decision token clause missing | deterministic_contract_failure | yes |
| BL037-B1-FX-013 | budget_clause_missing | CPU budget threshold clause missing | deterministic_contract_failure | yes |
| BL037-B1-FX-014 | replay_hash_mismatch | canonical replay hash mismatch across reruns | deterministic_lane_failure | yes |
| BL037-B1-FX-015 | artifact_schema_missing | lane artifact schema clause missing | deterministic_contract_failure | yes |
| BL037-B1-FX-016 | replay_clause_missing | replay determinism clause missing | deterministic_contract_failure | yes |
| BL037-B1-FX-017 | lane_taxonomy_incomplete | missing BL037-B1-FX lane taxonomy IDs | deterministic_contract_failure | yes |
| BL037-B1-FX-020 | suite_execution_failed | execute-suite run returned non-zero | deterministic_lane_failure | yes |

B1 required lane outputs:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`

B1 required owner-facing evidence:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C2 Soak QA Contract

Scope:
- Validate deterministic replay stability of BL-037 budget/degradation decision outputs over 10 contract-only reruns.

C2 acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL037-C2-001` | `--contract-only --runs 10` keeps all `deterministic_match=yes` rows | `contract_runs/replay_hashes.tsv` |
| `BL037-C2-002` | No mismatch/failure rows in replay matrix | `contract_runs/validation_matrix.tsv` |
| `BL037-C2-003` | Failure taxonomy class rows remain explicit and bounded | `contract_runs/failure_taxonomy.tsv` |
| `BL037-C2-004` | Soak rollup summarizes replay and failure counts deterministically | `soak_summary.tsv` |
| `BL037-C2-005` | Docs freshness remains pass | `docs_freshness.log` |

C2 validation matrix:

```bash
bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl037_slice_c2_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

## Slice C3 Replay Sentinel QA Contract

Scope:
- Extend deterministic replay sentinel to 20 contract-only runs while preserving strict lane exit semantics.

C3 validation matrix:

```bash
bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c3_replay_sentinel_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

C3 acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL037-C3-001` | Syntax + help checks both pass | `status.tsv`, `validation_matrix.tsv` |
| `BL037-C3-002` | `deterministic_match=yes` for all 20 replay rows | `contract_runs/replay_hashes.tsv` |
| `BL037-C3-003` | `FAIL` rows = `0` in contract run matrix | `contract_runs/validation_matrix.tsv` |
| `BL037-C3-004` | deterministic/artifact/runtime failure classes remain `0` | `contract_runs/failure_taxonomy.tsv` |
| `BL037-C3-005` | sentinel rollup table exists and is machine-readable | `replay_sentinel_summary.tsv` |
| `BL037-C3-006` | docs freshness still pass | `docs_freshness.log` |

C3 failure taxonomy additions:

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| `BL037-C3-FX-001` | replay_sentinel_hash_divergence | canonical hash mismatch across 20 runs | deterministic_lane_failure | yes | critical | contract_runs/replay_hashes.tsv |
| `BL037-C3-FX-002` | replay_sentinel_validation_failure | one or more validation matrix rows fail | deterministic_contract_failure | yes | critical | contract_runs/validation_matrix.tsv |
| `BL037-C3-FX-003` | replay_sentinel_artifact_missing | required C3 artifact absent | deterministic_evidence_failure | yes | major | status.tsv |
| `BL037-C3-FX-004` | replay_sentinel_docs_freshness_failed | docs freshness gate non-zero | governance_failure | yes | major | docs_freshness.log |

C3 required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C3 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl037_slice_c3_replay_sentinel_20260227T012033Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c3_replay_sentinel_20260227T012033Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## Slice C4 Replay Sentinel Soak QA Contract

Scope:
- Execute and validate a 50-run contract-only replay sentinel soak for deterministic confidence.

C4 validation matrix:

```bash
bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl037_slice_c4_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

C4 acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL037-C4-001` | Syntax and help checks pass | `status.tsv`, `validation_matrix.tsv` |
| `BL037-C4-002` | `deterministic_match=yes` for all 50 replay rows | `contract_runs/replay_hashes.tsv` |
| `BL037-C4-003` | Validation matrix has zero FAIL rows across 50 runs | `contract_runs/validation_matrix.tsv` |
| `BL037-C4-004` | Failure taxonomy remains zero for lane failure classes | `contract_runs/failure_taxonomy.tsv` |
| `BL037-C4-005` | Replay sentinel rollup is machine-readable and deterministic | `replay_sentinel_summary.tsv` |
| `BL037-C4-006` | Docs freshness remains pass | `docs_freshness.log` |

C4 failure taxonomy additions:

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| `BL037-C4-FX-001` | replay_sentinel_hash_divergence | canonical hash mismatch across 50 runs | deterministic_lane_failure | yes | critical | contract_runs/replay_hashes.tsv |
| `BL037-C4-FX-002` | replay_sentinel_validation_failure | one or more validation rows fail | deterministic_contract_failure | yes | critical | contract_runs/validation_matrix.tsv |
| `BL037-C4-FX-003` | replay_sentinel_artifact_missing | required C4 artifact absent | deterministic_evidence_failure | yes | major | status.tsv |
| `BL037-C4-FX-004` | replay_sentinel_docs_freshness_failed | docs freshness gate non-zero | governance_failure | yes | major | docs_freshness.log |

C4 required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C4 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl037_slice_c4_soak_20260227T014043Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl037_slice_c4_soak_20260227T014043Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## Slice C5 Exit-Semantics Guard QA Contract

Scope:
- Enforce deterministic 20-run replay and strict CLI exit semantics (`0` pass, `1` gate fail, `2` usage error).

C5 validation matrix:

```bash
bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c5_semantics_<timestamp>/contract_runs
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --runs 0
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --bad-arg
./scripts/validate-docs-freshness.sh
```

C5 acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL037-C5-001` | Syntax + help checks pass | `status.tsv`, `validation_matrix.tsv` |
| `BL037-C5-002` | `deterministic_match=yes` on all 20 replay rows | `contract_runs/replay_hashes.tsv` |
| `BL037-C5-003` | `FAIL` rows = `0` in contract run matrix | `contract_runs/validation_matrix.tsv` |
| `BL037-C5-004` | Failure taxonomy remains explicit and machine-readable | `contract_runs/failure_taxonomy.tsv` |
| `BL037-C5-005` | Replay summary exists with deterministic rollup fields | `replay_sentinel_summary.tsv` |
| `BL037-C5-006` | Usage-negative probes return `exit=2` for both probes | `exit_semantics_probe.tsv` |
| `BL037-C5-007` | Docs freshness gate remains pass | `docs_freshness.log` |

C5 failure taxonomy additions:

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| `BL037-C5-FX-001` | replay_sentinel_hash_divergence | canonical hash mismatch across 20 runs | deterministic_lane_failure | yes | critical | contract_runs/replay_hashes.tsv |
| `BL037-C5-FX-002` | replay_sentinel_validation_failure | one or more validation matrix rows fail | deterministic_contract_failure | yes | critical | contract_runs/validation_matrix.tsv |
| `BL037-C5-FX-003` | usage_exit_semantics_violation | `--runs 0` or `--bad-arg` exits with non-`2` | deterministic_contract_failure | yes | critical | exit_semantics_probe.tsv |
| `BL037-C5-FX-004` | replay_sentinel_artifact_missing | required C5 artifact absent | deterministic_evidence_failure | yes | major | status.tsv |
| `BL037-C5-FX-005` | replay_sentinel_docs_freshness_failed | docs freshness gate non-zero | governance_failure | yes | major | docs_freshness.log |

C5 required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C5 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl037_slice_c5_semantics_20260227T015358Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c5_semantics_20260227T015358Z/contract_runs` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --runs 0` => `PASS` (expected usage exit `2`)
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --bad-arg` => `PASS` (expected usage exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## Slice C5b Exit-Semantics Recheck Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl037_slice_c5b_semantics_20260227T025209Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c5b_semantics_20260227T025209Z/contract_runs` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --runs 0` => `PASS` (expected usage exit `2`)
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --bad-arg` => `PASS` (expected usage exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `FAIL` (external metadata debt under `Documentation/Calibration POC/`)

## Slice C5c Exit-Semantics Guard Recheck QA Contract (Post-H2)

Scope:
- Recheck deterministic 20-run contract replay and strict usage-exit semantics after H2 state updates.

C5c validation matrix:

```bash
bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c5c_semantics_<timestamp>/contract_runs
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --runs 0
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --bad-arg
./scripts/validate-docs-freshness.sh
```

C5c acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL037-C5c-001` | 20-run replay stays deterministic (`deterministic_match=yes` for all rows) | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL037-C5c-002` | Usage-negative probes return strict usage `exit=2` for both paths | `exit_semantics_probe.tsv` |
| `BL037-C5c-003` | Docs freshness gate returns pass in post-H2 state | `docs_freshness.log` |

C5c required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C5c Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl037_slice_c5c_semantics_20260227T031111Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl037_slice_c5c_semantics_20260227T031111Z/contract_runs` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --runs 0` => `PASS` (expected usage exit `2`)
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --bad-arg` => `PASS` (expected usage exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## Slice C6 Release Sentinel QA Contract

Scope:
- Release sentinel validation at 50 deterministic contract replays with strict usage-exit probes.

C6 validation matrix:

```bash
bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl037_slice_c6_release_sentinel_<timestamp>/contract_runs
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --runs 0
./scripts/qa-bl037-snapshot-budget-lane-mac.sh --bad-arg
./scripts/validate-docs-freshness.sh
```

C6 acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL037-C6-001` | `deterministic_match=yes` on all 50 replay rows | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL037-C6-002` | Contract validation matrix has `FAIL` rows = `0` | `contract_runs/validation_matrix.tsv` |
| `BL037-C6-003` | Usage-negative probes return strict usage `exit=2` for both paths | `exit_semantics_probe.tsv` |
| `BL037-C6-004` | Docs freshness gate remains pass | `docs_freshness.log` |
| `BL037-C6-005` | Required C6 evidence schema is complete and parseable | `status.tsv`, `validation_matrix.tsv`, `lane_notes.md` |

C6 required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C6 Release Sentinel Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl037_slice_c6_release_sentinel_20260227T033724Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl037-snapshot-budget-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl037_slice_c6_release_sentinel_20260227T033724Z/contract_runs` => `PASS`
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --runs 0` => `PASS` (expected usage exit `2`)
  - `./scripts/qa-bl037-snapshot-budget-lane-mac.sh --bad-arg` => `PASS` (expected usage exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`


## Validation

- `./scripts/validate-docs-freshness.sh`

Validation status labels:
- `tested` = command run and expected exit observed
- `partially tested` = command run but evidence incomplete
- `not tested` = command not run
