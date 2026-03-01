Title: BL-049 Unit Test Framework and Tracker Automation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-049 Unit Test Framework and Tracker Automation

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
