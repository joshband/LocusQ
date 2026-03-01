Title: BL-044 Quality-Tier Seamless Switching QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-044 Quality-Tier Seamless Switching QA Contract

## Purpose

Define deterministic QA contract surfaces for seamless quality-tier switching with continuity, latency, fallback, and replay guarantees.

## A1 Acceptance Mapping

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL044-A1-001` | Switching state contract fields are complete | `quality_tier_switch_contract.md`, `acceptance_matrix.tsv` |
| `BL044-A1-002` | Continuity bounds are explicit and machine-readable | `quality_tier_switch_contract.md`, `acceptance_matrix.tsv` |
| `BL044-A1-003` | Latency bound is explicit and machine-readable | `quality_tier_switch_contract.md`, `acceptance_matrix.tsv` |
| `BL044-A1-004` | Fallback policy/tokens are explicit and deterministic | `quality_tier_switch_contract.md`, `failure_taxonomy.tsv` |
| `BL044-A1-005` | Replay hash identity inputs are complete | `quality_tier_switch_contract.md`, `acceptance_matrix.tsv` |
| `BL044-A1-006` | BL044-FX taxonomy coverage is complete | `failure_taxonomy.tsv` |
| `BL044-A1-007` | A1 artifact schema is complete | `status.tsv`, `acceptance_matrix.tsv` |
| `BL044-A1-008` | Docs freshness gate passes | `docs_freshness.log` |

## Failure Taxonomy Contract (A1)

| failure_id | category | trigger | classification | blocking | severity | expected_artifact |
|---|---|---|---|---|---|---|
| `BL044-FX-001` | switch_contract_incomplete | missing required switch-state field | deterministic_contract_failure | yes | major | quality_tier_switch_contract.md |
| `BL044-FX-002` | continuity_bound_missing | discontinuity threshold missing | deterministic_contract_failure | yes | critical | quality_tier_switch_contract.md |
| `BL044-FX-003` | latency_bound_missing | completion latency threshold missing | deterministic_contract_failure | yes | critical | quality_tier_switch_contract.md |
| `BL044-FX-004` | fallback_policy_incomplete | missing fallback reason token or policy step | deterministic_contract_failure | yes | major | quality_tier_switch_contract.md |
| `BL044-FX-005` | replay_hash_inputs_incomplete | replay hash inputs incomplete | deterministic_contract_failure | yes | major | acceptance_matrix.tsv |
| `BL044-FX-006` | non_deterministic_switch_trace | divergent switch traces with identical inputs | deterministic_contract_failure | yes | critical | acceptance_matrix.tsv |
| `BL044-FX-007` | non_finite_transition_value | NaN/Inf during transition path | deterministic_contract_failure | yes | critical | failure_taxonomy.tsv |
| `BL044-FX-008` | artifact_schema_incomplete | required output artifact missing | deterministic_evidence_failure | yes | major | status.tsv |

## Validation Matrix (A1)

```bash
./scripts/validate-docs-freshness.sh
```

## Artifact Schema (A1)

Required output path:
`TestEvidence/bl044_slice_a1_contract_<timestamp>/`

Required files:
- `status.tsv`
- `quality_tier_switch_contract.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

Required `acceptance_matrix.tsv` columns:
- `acceptance_id`
- `gate`
- `threshold`
- `measured_value`
- `result`
- `evidence_path`

Required `failure_taxonomy.tsv` columns:
- `failure_id`
- `category`
- `trigger`
- `classification`
- `blocking`
- `severity`
- `expected_artifact`

## Slice A1 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl044_slice_a1_contract_20260227T204237Z/status.tsv`
  - `quality_tier_switch_contract.md`
  - `acceptance_matrix.tsv`
  - `failure_taxonomy.tsv`
  - `docs_freshness.log`
- Validation outcomes:
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## Slice B1 Lane Bootstrap QA Contract

Scope:
- Bootstrap deterministic BL-044 contract lane replay and machine-readable artifact production.

B1 validation matrix:

```bash
bash -n scripts/qa-bl044-quality-tier-switch-lane-mac.sh
./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --help
./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl044_slice_b1_lane_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

B1 acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL044-B1-001` | A1 acceptance ID declarations are complete | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-002` | BL044-FX taxonomy declarations are complete | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-003` | Switch-state field clause is explicit | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-004` | Continuity clause is explicit | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-005` | Fallback clause is explicit | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-006` | Replay clause is explicit | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-007` | Artifact schema clause is explicit | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-008` | Lane taxonomy declarations are complete | `contract_runs/validation_matrix.tsv` |
| `BL044-B1-009` | Replay hash stability holds across reruns | `contract_runs/replay_hashes.tsv` |

B1 failure taxonomy IDs:
- `BL044-B1-FX-001`
- `BL044-B1-FX-002`
- `BL044-B1-FX-003`
- `BL044-B1-FX-004`
- `BL044-B1-FX-010`
- `BL044-B1-FX-011`
- `BL044-B1-FX-012`
- `BL044-B1-FX-013`
- `BL044-B1-FX-014`
- `BL044-B1-FX-015`
- `BL044-B1-FX-016`
- `BL044-B1-FX-017`
- `BL044-B1-FX-018`
- `BL044-B1-FX-020`

B1 required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice B1 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl044_slice_b1_lane_20260227T210106Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl044-quality-tier-switch-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl044_slice_b1_lane_20260227T210106Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`

## Slice C2 Determinism Soak QA Contract

Scope:
- Re-run BL-044 contract lane in deterministic soak mode (`runs=10`) with strict artifact and replay-hash checks.

C2 validation matrix:

```bash
bash -n scripts/qa-bl044-quality-tier-switch-lane-mac.sh
./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --help
./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl044_slice_c2_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

C2 required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C2 Execution Snapshot (2026-02-27)

- Evidence bundle:
  - `TestEvidence/bl044_slice_c2_soak_20260227T214648Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `soak_summary.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - `bash -n scripts/qa-bl044-quality-tier-switch-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl044-quality-tier-switch-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl044_slice_c2_soak_20260227T214648Z/contract_runs` => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Determinism summary:
  - `deterministic_match_yes=10`
  - `deterministic_match_no=0`
  - `unique_canonical_hashes=1`
- Slice gate summary:
  - Contract replay gate: `PASS`
  - Ownership safety gate: `FAIL` (out-of-ownership git changes detected during parallel execution; see `ownership_safety_check.tsv`)

## Slice Z16 E2E Evidence Localization QA Contract

Scope:
- Localize the externally-produced BL-044 Slice Z end-to-end evidence bundle into repo `TestEvidence` with deterministic provenance parity checks.

Z16 validation matrix:

```bash
./scripts/validate-docs-freshness.sh
repo-path artifact integrity check (hash/count/path parity against source bundle)
```

Z16 acceptance mapping:

| Acceptance ID | Lane Check / Rule | Required Evidence |
|---|---|---|
| `BL044-Z16-001` | File-count parity | `provenance_matrix.tsv` |
| `BL044-Z16-002` | Missing-path parity | `provenance_matrix.tsv` |
| `BL044-Z16-003` | Extra-path parity | `provenance_matrix.tsv` |
| `BL044-Z16-004` | SHA-256 parity | `artifact_hashes.tsv` |
| `BL044-Z16-005` | Size parity | `artifact_hashes.tsv` |
| `BL044-Z16-006` | Docs freshness gate | `docs_freshness.log` |

Z16 required files:
- `status.tsv`
- `provenance_matrix.tsv`
- `artifact_hashes.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice Z16 Execution Snapshot (2026-02-27)

- Input source:
  - `/tmp/locusq_bl044_z_20260227T222337Z/TestEvidence/bl044_end_to_end_z_20260227T222337Z/*`
- Repo destination:
  - `TestEvidence/bl044_end_to_end_z_20260227T222337Z/*`
- Provenance packet:
  - `TestEvidence/bl044_e2e_localization_z16_20260227T225525Z/status.tsv`
  - `provenance_matrix.tsv`
  - `artifact_hashes.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation outcomes:
  - repo-path artifact integrity parity check => `PASS`
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Provenance readout:
  - `source_file_count=28`
  - `destination_file_count=28`
  - `missing_in_destination=0`
  - `extra_in_destination=0`
  - `hash_fail_rows=0`
  - `size_fail_rows=0`
