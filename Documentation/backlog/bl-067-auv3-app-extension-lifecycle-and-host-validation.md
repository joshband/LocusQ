Title: BL-067 AUv3 App-Extension Lifecycle and Host Validation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-02

# BL-067 AUv3 App-Extension Lifecycle and Host Validation

## Plain-Language Summary

BL-067 in plain terms: Add production-ready AUv3 format support for LocusQ with deterministic extension lifecycle behavior, sandbox-safe runtime boundaries, and explicit parity validation against existing AU/VST3/CLAP formats. Current state: Open (promotion blocked: no promotion while any execute evidence row is TODO; BL-073 execute-mode gate required). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA owners, release owners, and engineering maintainers who depend on deterministic evidence. |
| What is changing? | Add production-ready AUv3 format support for LocusQ with deterministic extension lifecycle behavior, sandbox-safe runtime boundaries, and explicit parity validation against existing AU/VST3/CLAP formats. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: Open (no promotion while any execute evidence row is TODO; BL-073 gate required). This item is done when required acceptance checks pass and promotion evidence is complete. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Implementation slices | Clarifies execution sequence and ownership. | `## Implementation Slices` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-067 |
| Priority | P1 |
| Status | Open (promotion blocked: no promotion while any execute evidence row is `TODO`; BL-073 execute-mode gate required) |
| Track | A - Runtime Formats |
| Effort | High / L |
| Depends On | BL-048 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | High-cost wrapper (host matrix + extension lifecycle sweeps) |

## Objective

Add production-ready AUv3 format support for LocusQ with deterministic extension lifecycle behavior, sandbox-safe runtime boundaries, and explicit parity validation against existing AU/VST3/CLAP formats.

## Acceptance IDs

- AUv3 build target and packaging pipeline are reproducible and code-signed for host execution.
- Extension lifecycle transitions (cold start, reload, suspend/resume, state restore) complete without crashes or stale state.
- Audio-thread invariants remain intact (no allocation/locks/blocking I/O in realtime callbacks).
- AUv3-specific constraints degrade deterministically without host-name branching behavior.
- AU/VST3/CLAP regression lanes remain green after AUv3 enablement.
- Execute-mode QA evidence contains zero `TODO` rows (BL-073 scaffold-truthfulness gate).

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | AUv3 target wiring and build/packaging contracts | AUv3 target builds and launches in baseline host smoke lane |
| B | Extension-safe runtime boundaries and lifecycle handling | lifecycle transition matrix passes with deterministic state restore |
| C | Cross-format parity and ship evidence packet | AUv3 + AU/VST3/CLAP parity matrix is green and evidence-complete |

## Validation Plan

QA harness script: `scripts/qa-bl067-auv3-lifecycle-mac.sh`.
Evidence schema: `TestEvidence/bl067_*/status.tsv`.

Minimum evidence additions:
- `host_matrix.tsv` (AUv3 host coverage and outcomes)
- `lifecycle_transitions.tsv` (cold/warm/reload/suspend-resume results)
- `parity_regression.tsv` (AUv3 vs AU/VST3/CLAP contract outcomes)
- `packaging_manifest.md` (targets, signing, packaging notes)

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | runbook primary lane command at dev-loop depth | validation matrix + replay summary |
| Candidate intake | T2 | 5 (or heavy-wrapper 2-run cap) | runbook candidate replay command set | contract/execute artifacts + taxonomy |
| Promotion | T3 | 10 (or owner-approved heavy-wrapper 3-run equivalent) | owner-selected promotion replay command set | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run sentinel drill when explicitly requested | parity/sentinel artifacts |

### Cost/Flake Policy

- Diagnose failing run index before repeating full multi-run sweeps.
- Heavy wrappers (`>=20` binary launches per wrapper run) use targeted reruns, candidate at 2 runs, and promotion at 3 runs unless owner requests broader coverage.
- Document cadence overrides with rationale in `lane_notes.md` or `owner_decisions.md`.

## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment (2026-03-01)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

