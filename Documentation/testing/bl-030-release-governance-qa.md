Title: BL-030 Release Governance QA
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-03-18

# BL-030 Release Governance QA

## Status

Active. This is the short release-governance replay contract for BL-030.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/bl-030-release-governance-qa-legacy.md`

## Goal

Keep release-governance decisions deterministic.
Separate governance blockers from runtime flake.
Keep RL-03 through RL-09 evidence machine-checkable.

## Linked Authority

- Runbook: `Documentation/backlog/done/bl-030-release-governance.md`
- Contracts: `Documentation/scene-state-contract.md`, `Documentation/invariants.md`

## Core Contracts

| Lane | Contract |
|---|---|
| RL-03 | Standalone production self-test must be deterministic. |
| RL-04 | REAPER smoke must publish stage and terminal taxonomy. |
| RL-05 | Device matrix capture and manual-evidence validation must separate deterministic blockers from transient flake. |
| RL-06 | Pluginval replay must be strict, bounded, and taxonomy-rich. |
| RL-08 | `./scripts/validate-docs-freshness.sh` must pass. |
| RL-09 | Release-note closeout traceability must match `CHANGELOG.md`, backlog, and this QA contract. |

## Validation Plan

### RL-04

- Command: `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`
- Required stage fields: `stageBootstrapResult`, `stageInstallResult`, `stageRenderResult`, `stageOutputResult`
- Required terminal fields: `terminalStage`, `terminalReasonCode`, `terminalReasonClass`, `terminalSignalNumber`, `terminalReasonDetail`

### RL-05

- Commands: `qa-bl030-manual-evidence-pack-mac.sh`, `qa-bl030-manual-evidence-validate-mac.sh`, `qa-bl030-device-matrix-capture-mac.sh`
- Fixed replay packet order: pack -> validate -> capture
- Required blocker categories: `deterministic_missing_manual_evidence`, `runtime_flake_abrt`, `not_applicable_with_waiver`
- DEV-01..DEV-06 completion is required, with DEV-06 waiver handling only when explicit

### RL-06

- Command: `./scripts/qa-bl030-pluginval-stability-mac.sh --runs 10 --out-dir <packet>`
- Pass condition: all runs pass, taxonomy is deterministic, and the replay packet is complete

### RL-09

- `CHANGELOG.md` must reference the closeout window and evidence packet path
- RL-05 state must be stated honestly
- Grep traceability must match `CHANGELOG.md`, the backlog done doc, and this QA contract

## Evidence Bundle

Required packet files:
- `status.tsv`
- `rl_gate_matrix.tsv`
- `release_decision.md`
- `blocker_taxonomy.tsv`
- `docs_freshness.log`

When RL-05 is active, include:
- `dev_matrix_results.tsv`
- `manual_evidence_checklist.tsv`
- `replay_transcript.log`
- `command_transcript.log`
- `rl05_reconcile_summary.tsv`

## Risks

- Governance blockers can be mistaken for runtime flake.
- RL-09 can drift if `CHANGELOG.md` is not updated in the same change set.
- Manual-evidence packets can go stale if waiver handling is not explicit.

## Archive Note

The full historical lane narratives and closure snapshots live in the archive copy above.
Use this active file as the short contract and the archive file for legacy detail.
