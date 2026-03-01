Title: BL-042 QA CI Regression Gates
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-042 QA CI Regression Gates

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-042 |
| Priority | P1 |
| Status | In Planning |
| Track | G - Release/Governance |
| Effort | Med / M |
| Depends On | BL-035, BL-036, BL-041, HX-06 (Done) |
| Blocks | BL-030 |

## Objective

Promote DSP and runtime regression checks into deterministic CI gates so lock-safety, finite-output, spatial math, and lane stability regressions cannot merge silently.

## Scope

In scope:
- CI wiring for key local QA harness lanes.
- Deterministic replay/hash output checks in CI context.
- Release governance integration for gate outcomes.

Out of scope:
- Feature implementation unrelated to validation automation.
- Non-deterministic manual-only acceptance substitutions.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Define CI gate matrix and required artifacts | Matrix approved and documented |
| B | Wire harness lanes into workflow gates | CI gates run and classify deterministic failures correctly |
| C | Release governance alignment | BL-030 references updated with enforced gate set |

## TODOs

- [ ] Define mandatory CI lanes for RT safety, smoke, and determinism.
- [ ] Add machine-readable artifact checks as merge criteria.
- [ ] Ensure environment-blocked cases are classified, not silently ignored.
- [ ] Update BL-030 governance docs with enforced gate matrix.
- [ ] Capture CI validation evidence and promotion decision packet.

## Validation Plan

- `ruby -e 'require "yaml"; YAML.load_file(".github/workflows/qa_harness.yml")'`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `ci_gate_matrix.tsv`
- `workflow_lint.log`
- `gate_contract.md`
- `docs_freshness.log`
