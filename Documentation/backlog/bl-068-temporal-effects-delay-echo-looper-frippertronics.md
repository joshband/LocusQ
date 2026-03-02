Title: BL-068 Temporal Effects Core (Delay/Echo/Looper/Frippertronics)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-02

# BL-068 Temporal Effects Core (Delay/Echo/Looper/Frippertronics)

## Plain-Language Summary

BL-068 in plain terms: Define and integrate a deterministic temporal-effects core spanning delay/echo, controlled feedback behavior, and looper/frippertronics-style layering that remains realtime-safe and host-automation reliable. Current state: Open (execute-lane scaffold-only; no promotion while any execute evidence row is TODO; BL-073 gate required). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | DSP/engine maintainers, QA owners, and release owners protecting realtime safety. |
| What is changing? | Define and integrate a deterministic temporal-effects core spanning delay/echo, controlled feedback behavior, and looper/frippertronics-style layering that remains realtime-safe and host-automation reliable. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: Open (reprioritized from code-review risk packet; no promotion while any execute evidence row is TODO; BL-073 gate required). This item is done when required acceptance checks pass and promotion evidence is complete. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-068-temporal-effects-delay-echo-looper-frippertronics.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


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
| ID | BL-068 |
| Priority | P1 |
| Status | Open (execute-lane scaffold-only; no promotion while any execute evidence row is `TODO`; BL-073 gate required) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-050, BL-055 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-068-temporal-effects-core-spec-2026-03-01.md` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Define and integrate a deterministic temporal-effects core spanning delay/echo, controlled feedback behavior, and looper/frippertronics-style layering that remains realtime-safe and host-automation reliable.

## Acceptance IDs

- Delay/echo timing and feedback behavior are stable from 44.1kHz through 192kHz.
- Feedback-network safety ceiling prevents runaway/non-finite output in stress lanes.
- Looper overdub/clear/transport interactions are deterministic on session recall.
- Parameter automation and mode transitions are click-safe and zipper-safe.
- Temporal-effect lanes remain compatible with existing spatial and FIR paths.
- Execute-mode QA evidence contains zero `TODO` rows (BL-073 scaffold-truthfulness gate).

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Delay/echo and bounded feedback architecture | finite-output and runaway-guard lanes pass |
| B | Looper + frippertronics-style layering behavior | transport/recall lanes pass without drift or clicks |
| C | Evidence and visualization handshake contracts | deterministic replay + telemetry evidence packet captured |

## Validation Plan

QA harness script: `scripts/qa-bl068-temporal-effects-mac.sh`.
Evidence schema: `TestEvidence/bl068_*/status.tsv`.

Minimum evidence additions:
- `temporal_matrix.tsv` (delay/echo/looper scenario results)
- `runaway_guard.tsv` (feedback safety + finite-output checks)
- `transport_recall.tsv` (timeline/recall determinism checks)
- `cpu_latency_budget.tsv` (sample-rate and topology budget snapshots)

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

