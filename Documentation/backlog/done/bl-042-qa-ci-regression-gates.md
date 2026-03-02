Title: BL-042 QA CI Regression Gates
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-02

# BL-042 QA CI Regression Gates

## Plain-Language Summary

This runbook tracks **BL-042** (BL-042 QA CI Regression Gates). Current status: **In Planning**. In plain terms: Promote DSP and runtime regression checks into deterministic CI gates so lock-safety, finite-output, spatial math, and lane stability regressions cannot merge silently.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-042 QA CI Regression Gates |
| Why is this important? | Promote DSP and runtime regression checks into deterministic CI gates so lock-safety, finite-output, spatial math, and lane stability regressions cannot merge silently. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when required acceptance criteria, validation lanes, and evidence synchronization are all marked pass. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-042-qa-ci-regression-gates.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Implementation slices table | Explains step-by-step delivery order and boundaries. | `## Implementation Slices` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


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
