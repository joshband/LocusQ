Title: BL-039 Parameter Relay Spec Generation QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-039 Parameter Relay Spec Generation QA Contract

## Status

Active. This is the short QA contract for BL-039 relay-spec generation and replay checks.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/bl-039-parameter-relay-spec-generation-qa-legacy.md`

## Goal

Keep BL-039 schema generation deterministic.
Keep ordering, hashing, replay, and acceptance parity machine-checkable.

## Linked Authority

- Runbook: `Documentation/backlog/bl-039-parameter-relay-spec-generation.md`
- Inputs: `.ideas/parameter-spec.md`, `.ideas/architecture.md`, `Documentation/invariants.md`

## Core Contracts

| Area | Contract |
|---|---|
| Schema | Required fields, types, and key uniqueness are explicit. |
| Ordering | Mode scope order is `global, calibrate, emitter, renderer`; sorting is ASCII deterministic. |
| Drift | `spec_content_sha256`, `schema_definition_sha256`, and `ordering_fingerprint_sha256` remain stable for unchanged inputs. |
| Replay | Required files and columns exist for `relay_hashes.tsv` and `relay_drift_report.tsv`. |
| Parity | Acceptance IDs match across runbook, QA, and evidence. |

## Validation Plan

### A1 Contract

- `BL039-QA-001..012`
- `BL039-A1-001..008`
- `./scripts/validate-docs-freshness.sh`

### B1 Drift Lane

- Script: `./scripts/qa-bl039-parameter-relay-drift-mac.sh`
- Modes: `--contract-only` and `--execute-suite`
- Required runs: `3`
- Exit semantics: `0` pass, `1` contract/lane fail, `2` usage/config error
- Required outputs: `status.tsv`, `validation_matrix.tsv`, `replay_hashes.tsv`, `failure_taxonomy.tsv`

### C2/C3/C4 Replay Lanes

- `C2`: `runs=10`, drift summary stable, replay rows stable
- `C3`: `runs=20`, sentinel replay stable
- `C4`: `runs=50`, soak replay stable

### C5 Mode Parity

- `--contract-only` and `--execute-suite` must stay alias-parity clean
- Negative usage probe `--runs 0` must exit `2`
- Required outputs include `mode_parity.tsv`, `drift_summary.tsv`, and `exit_semantics_probe.tsv`

## Evidence Bundle

- `TestEvidence/bl039_slice_a1_contract_<timestamp>/`
- `TestEvidence/bl039_slice_b1_lane_<timestamp>/`
- `TestEvidence/bl039_slice_c2_soak_<timestamp>/`
- `TestEvidence/bl039_slice_c3_replay_sentinel_<timestamp>/`
- `TestEvidence/bl039_slice_c4_soak_<timestamp>/`
- `TestEvidence/bl039_slice_c5_semantics_<timestamp>/`

Required common artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## Risks

- Ordering drift can invalidate generated specs.
- Replay hash drift can hide schema regressions.
- Mode parity failures can break contract-only versus execute-suite assumptions.

## Archive Note

The full historical slices, snapshots, and execution logs live in the archive copy above.
Use this active file as the execution contract and the archive file for legacy detail.
