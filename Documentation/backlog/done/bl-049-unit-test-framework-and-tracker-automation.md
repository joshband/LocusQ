Title: BL-049 Unit Test Framework and Tracker Automation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-02

# BL-049 Unit Test Framework and Tracker Automation

## Plain-Language Summary

This runbook tracks **BL-049** (BL-049 Unit Test Framework and Tracker Automation). Current status: **In Planning**. In plain terms: Add deterministic component-level unit tests (DSP + scene + tracking bridge) and automate head-tracking bridge validation to close current coverage gaps.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-049 Unit Test Framework and Tracker Automation |
| Why is this important? | Add deterministic component-level unit tests (DSP + scene + tracking bridge) and automate head-tracking bridge validation to close current coverage gaps. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when required acceptance criteria, validation lanes, and evidence synchronization are all marked pass. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-049-unit-test-framework-and-tracker-automation.md` plus repo-local evidence under `TestEvidence/...`. |

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
| ID | BL-049 |
| Priority | P1 |
| Status | In Planning |
| Track | D - QA Platform |
| Effort | High / L |
| Depends On | BL-042 |
| Blocks | — |

## Objective

Add deterministic component-level unit tests (DSP + scene + tracking bridge) and automate head-tracking bridge validation to close current coverage gaps.

## Scope

In scope:
- Unit-test harness target and deterministic component tests.
- HeadTrackingBridge packet/connect/reconnect automation.
- CI-ready machine-readable test reporting.

Out of scope:
- End-user feature changes.
- Manual-only listening QA replacements.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Unit-test harness target and core DSP tests | Component tests pass under deterministic thresholds |
| B | SceneGraph + HeadTrackingBridge automation tests | Tracker bridge failure modes covered |
| C | CI integration and evidence contract | Unit tests run in BL-042 gate matrix |

## TODOs

- [ ] Add unit-test target and deterministic test runner contract.
- [ ] Implement core DSP component tests (VBAP/FDN/Doppler/etc.).
- [ ] Add SceneGraph concurrency lifecycle tests.
- [ ] Add HeadTrackingBridge packet lifecycle automation tests.
- [ ] Wire unit outputs into CI gate and evidence packet.

## Validation Plan

- `cmake --build build_local --config Release --target LocusQ_UnitTests -j 8`
- `./build_local/LocusQ_UnitTests`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `unit_results.tsv`
- `tracker_bridge_results.tsv`
- `coverage_matrix.tsv`
- `docs_freshness.log`
