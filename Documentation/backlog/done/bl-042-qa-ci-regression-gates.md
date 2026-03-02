Title: BL-042 QA CI Regression Gates
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-02

# BL-042 QA CI Regression Gates

## Plain-Language Summary

BL-042 in plain terms: Promote DSP and runtime regression checks into deterministic CI gates so lock-safety, finite-output, spatial math, and lane stability regressions cannot merge silently. Current state: In Planning. For technical detail, see `## Objective` and `## Validation Plan`.

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

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Implementation slices | Clarifies execution sequence and ownership. | `## Implementation Slices` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-042 |
| Priority | P1 |
| Status | Done (historical planning-state context retained in runbook history) |
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
