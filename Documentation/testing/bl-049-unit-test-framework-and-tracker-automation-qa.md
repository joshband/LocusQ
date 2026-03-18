Title: BL-049 Unit Test Framework and Tracker Automation QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-049 Unit Test Framework and Tracker Automation QA Contract

## Status

Active QA contract.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/bl-049-unit-test-framework-and-tracker-automation-qa-legacy.md`

## Goal

Keep BL-049 unit-test framework behavior, tracker schema determinism, flake policy, and CI artifact shape machine-checkable.

## Linked Authority

- `Documentation/backlog/done/bl-049-unit-test-framework-and-tracker-automation.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`

## Core Checks

| Area | Contract |
|---|---|
| A1 framework | `BL049-QA-001..003` cover framework layers, lifecycle order, and aggregation determinism. |
| A1 tracker | `BL049-QA-004..006` cover required fields, enum domain, and `payload_seq` monotonicity. |
| A1 flake/CI | `BL049-QA-007..009` cover flake classes, thresholds, and CI artifact determinism. |
| A1 acceptance | `BL049-A1-001..009` stay parity-locked with the runbook and evidence packet. |
| A1 taxonomy | `BL049-FX-001..010` covers framework, tracker, flake, artifact, evidence, and docs freshness failures. |
| B1 lane | `BL049-B1-001..008` keeps lane bootstrap, replay, and exit semantics explicit. |
| B1 taxonomy | `BL049-FX-101..108` covers lane contract, schema, replay drift, handoff, evidence, and docs freshness failures. |
| C2/C3 | soak and sentinel runs stay deterministic at `10` and `20` runs. |

## Validation Plan

- `./scripts/validate-docs-freshness.sh`
- `bash -n scripts/qa-bl049-unit-test-tracker-lane-mac.sh`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --help`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl049_slice_c2_soak_<timestamp>/contract_runs`
- `./scripts/qa-bl049-unit-test-tracker-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl049_slice_c3_replay_sentinel_<timestamp>/contract_runs`

## Evidence Bundle

Primary bundle roots:
- `TestEvidence/bl049_slice_a1_contract_<timestamp>/`
- `TestEvidence/bl049_slice_b1_lane_<timestamp>/`
- `TestEvidence/bl049_slice_c2_soak_<timestamp>/`
- `TestEvidence/bl049_slice_c3_replay_sentinel_<timestamp>/`

Required files:
- `status.tsv`
- `acceptance_matrix.tsv` or `validation_matrix.tsv`
- `failure_taxonomy.tsv`
- `replay_hashes.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Risks

- Replay drift can hide behind a passing schema.
- Handoff ambiguity can make B1 feel green while A1 evidence is stale.
- Artifact schema churn can break machine consumers before humans notice.

## Archive Note

The verbose original runbook is preserved in the archive copy above.
Use this active file for current QA decisions and the archive file for deep detail.
