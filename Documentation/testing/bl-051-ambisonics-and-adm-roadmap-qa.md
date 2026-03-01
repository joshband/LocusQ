Title: BL-051 Ambisonics and ADM Roadmap QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-051 Ambisonics and ADM Roadmap QA Contract

## Purpose

Define deterministic contract checks for BL-051 Slice A1b decision-package readiness so roadmap governance can advance without touching active execution lanes.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/bl-051-ambisonics-and-adm-roadmap.md`
- `Documentation/adr/ADR-0014-bl051-ambisonics-adm-roadmap-governance.md`

Traceability anchors:
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`

## A1b Required Artifacts

Required under `TestEvidence/bl051_slice_a1b_decision_package_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `adr_decision.md`
- `migration_plan.tsv`
- `risk_register.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## A1b Pass/Fail Criteria

PASS requires all of:
1. `adr_decision.md` exists and states selected architecture path plus rationale and rollback posture.
2. `migration_plan.tsv` exists with machine-readable phases, gates, dependencies, and rollback criteria.
3. `risk_register.tsv` exists with machine-readable risk IDs, impact, likelihood, mitigations, and owner lanes.
4. `./scripts/validate-docs-freshness.sh` exits `0`.
5. `validation_matrix.tsv` records all required checks as `PASS`.

FAIL if any required artifact is missing, schema rows are incomplete, or docs freshness exits non-zero.

## Validation

- `./scripts/validate-docs-freshness.sh`

## Failure Taxonomy (A1b)

| blocker_id | category | trigger |
|---|---|---|
| BL051-A1B-BLK-001 | adr_decision_missing_or_incomplete | `adr_decision.md` missing required decision fields |
| BL051-A1B-BLK-002 | migration_plan_schema_failure | `migration_plan.tsv` missing required columns/rows |
| BL051-A1B-BLK-003 | risk_register_schema_failure | `risk_register.tsv` missing required columns/rows |
| BL051-A1B-BLK-004 | docs_freshness_failure | docs freshness gate exits non-zero |
| BL051-A1B-BLK-005 | artifact_schema_incomplete | required artifact absent from packet |

## A2 Prototype + Parity Contract

Purpose:
- Define deterministic contract checks for the first BL-051 prototype/parity lane without touching production execution paths.

Required artifacts under `TestEvidence/bl051_slice_a2_prototype_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `prototype_parity_contract.tsv`
- `evidence_schema.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### A2 Pass Criteria

PASS requires all of:
1. `prototype_parity_contract.tsv` declares lane IDs, command contracts, expected deterministic outputs, and strict exit semantics.
2. `evidence_schema.tsv` declares required artifact file names and machine-readable columns for each lane output.
3. `validation_matrix.tsv` contains `PASS` for contract-surface checks and docs freshness gate.
4. `./scripts/validate-docs-freshness.sh` exits `0`.

FAIL if any required artifact is missing, column schema is incomplete, or docs freshness exits non-zero.

### Failure Taxonomy (A2)

| blocker_id | category | trigger |
|---|---|---|
| BL051-A2-BLK-001 | prototype_contract_missing_or_incomplete | `prototype_parity_contract.tsv` missing required lane contract rows |
| BL051-A2-BLK-002 | evidence_schema_missing_or_incomplete | `evidence_schema.tsv` missing required artifact schema rows |
| BL051-A2-BLK-003 | docs_freshness_failure | docs freshness gate exits non-zero |
| BL051-A2-BLK-004 | validation_matrix_incomplete | required A2 checks missing from `validation_matrix.tsv` |

## C1 Backlog Decomposition Contract

Purpose:
- Convert BL-051 roadmap into execution-ready, dependency-ordered work items with deterministic intake gates.

Required artifacts under `TestEvidence/bl051_slice_c1_decomposition_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `decomposition_plan.tsv`
- `dependency_graph.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C1 Pass Criteria

PASS requires all of:
1. `decomposition_plan.tsv` includes work IDs, scope summary, dependency set, and deterministic exit signals.
2. `dependency_graph.tsv` contains a DAG-consistent edge list with no orphaned nodes for declared work IDs.
3. `validation_matrix.tsv` records decomposition schema checks and docs freshness as `PASS`.
4. `./scripts/validate-docs-freshness.sh` exits `0`.

FAIL if decomposition schema is incomplete, dependencies are inconsistent, or docs freshness exits non-zero.

### Failure Taxonomy (C1)

| blocker_id | category | trigger |
|---|---|---|
| BL051-C1-BLK-001 | decomposition_schema_incomplete | missing required columns/rows in `decomposition_plan.tsv` |
| BL051-C1-BLK-002 | dependency_graph_invalid | dependency graph missing required nodes or edges |
| BL051-C1-BLK-003 | docs_freshness_failure | docs freshness gate exits non-zero |
| BL051-C1-BLK-004 | validation_matrix_incomplete | required C1 checks missing from `validation_matrix.tsv` |

## A2 Contract Template Packet (Execution-Lane Ready)

A2 contract artifacts are staged at:
- `TestEvidence/bl051_slice_a2_template_packet_20260228_125502/prototype_parity_contract.tsv`
- `TestEvidence/bl051_slice_a2_template_packet_20260228_125502/evidence_schema.tsv`

Required pass criteria:
- `prototype_parity_contract.tsv` defines owner, baseline lane, acceptance gate, and rollback trigger for each prototype lane.
- `evidence_schema.tsv` defines artifact name, producer lane, freshness window, and validation gate for each required evidence file.
- `validation_matrix.tsv` records `./scripts/validate-docs-freshness.sh` with `exit_code=0`.
- `docs_freshness.log` contains a PASS result for repository docs freshness gate.

