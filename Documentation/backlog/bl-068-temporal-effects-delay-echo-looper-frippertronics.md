Title: BL-068 Temporal Effects Core (Delay/Echo/Looper/Frippertronics)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-17

# BL-068 Temporal Effects Core (Delay/Echo/Looper/Frippertronics)

## Plain-Language Summary

BL-068 in plain terms: Define and integrate a deterministic temporal-effects core spanning delay/echo, controlled feedback behavior, and looper/frippertronics-style layering that remains realtime-safe and host-automation reliable. Current state: Done-candidate (owner promotion replay PASS in this workspace: contract-only 10/10 PASS, execute 3/3 PASS, zero TODO rows, compile-backed execute probe clean). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | DSP/engine maintainers, QA owners, and release owners protecting realtime safety. |
| What is changing? | Define and integrate a deterministic temporal-effects core spanning delay/echo, controlled feedback behavior, and looper/frippertronics-style layering that remains realtime-safe and host-automation reliable. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: Done-candidate (owner promotion replay PASS on 2026-03-17 with zero TODO rows and compile-backed execute evidence). This item is done when the owner promotion packet is accepted and closeout/archive sync is executed. |
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
| Status | Done-candidate (owner sync Z1 2026-03-17: contract-only 10/10 PASS `TestEvidence/bl068_owner_sync_z1_20260317T191642Z/contract_runs/`; execute 3/3 PASS `TestEvidence/bl068_owner_sync_z1_20260317T191642Z/execute_runs/`; zero TODO rows; promotion packet `TestEvidence/bl068_owner_sync_z1_20260317T191642Z/promotion_decision.md`) |
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
Execute-mode note: `--execute` builds a dedicated BL-068 probe under the lane output directory instead of using `build_local`.

Recommended deterministic replay commands:
- `./scripts/qa-bl068-temporal-effects-mac.sh --contract-only --runs 3`
- `./scripts/qa-bl068-temporal-effects-mac.sh --execute --runs 1`

Minimum evidence additions:
- `temporal_modes_matrix.tsv` (delay/echo/looper/frippertronics mode contract results)
- `runaway_guard.tsv` (feedback safety + finite-output checks)
- `transport_recall.tsv` (timeline/recall determinism checks)
- `cpu_latency_budget.tsv` (sample-rate and topology budget snapshots)
- `summary.md` (lane verdict, TODO count, and isolated build-dir reference)
- `lane_notes.md` (command/runs/mode semantics and cadence notes)

Current implemented safe-slice contract surfaces:
- `Source/temporal_effects/TemporalEffectContracts.h`
- `Source/temporal_effects/TemporalModeMatrix.h`
- `Source/dsp/TemporalContractWiring.h`

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

## T2 Candidate Intake Snapshot (2026-03-17)

Harness commands:
- `./scripts/qa-bl068-temporal-effects-mac.sh --contract-only --runs 5`
- `./scripts/qa-bl068-temporal-effects-mac.sh --execute --runs 2`

Evidence dirs:
- `TestEvidence/bl068_temporal_effects_20260317T190255Z/`
- `TestEvidence/bl068_temporal_effects_20260317T190301Z/`

| Artifact | Result | Notes |
|---|---|---|
| `status.tsv` | Contract-only 32 rows PASS; execute 34 rows PASS | 0 failures across pre-flight, constant, matrix, compile-probe, and lane-result checks |
| `temporal_modes_matrix.tsv` | Contract-only 20 rows PASS (4 modes × 5 runs); execute 8 rows PASS (4 modes × 2 runs) | Delay/echo/looper/frippertronics rows remained deterministic across both replay depths |
| `runaway_guard.tsv` | Contract-only 20 rows PASS (4 checks × 5 runs); execute 8 rows PASS (4 checks × 2 runs) | Feedback clamp, wet-sample sanitization, and invalid loop-offset finite-state guard all stayed green |
| `transport_recall.tsv` | Contract-only 10 rows PASS (2 cases × 5 runs); execute 6 rows PASS (3 cases × 2 runs) | Identical snapshots reproduced identical recall tokens; overdub and quantize transitions diverged deterministically |
| `cpu_latency_budget.tsv` | Contract-only 10 rows PASS (2 profiles × 5 runs); execute 4 rows PASS (2 profiles × 2 runs) | 44.1kHz and 192k budget snapshots remained inside the bounded envelope |
| `probe_build/` | Execute compile + run PASS | Dedicated BL-068 probe built inside the evidence folder; no `build_local` reuse |

Zero-TODO row count: **0** (across all four matrix artifacts in both candidate-intake packets).
Lane result: **PASS** (`contract_only;runs=5` and `execute;runs=2`).
Recommendation: **candidate intake PASS; escalated to owner promotion replay**.

## T3 Owner Promotion Snapshot (2026-03-17)

Harness commands:
- `./scripts/qa-bl068-temporal-effects-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl068_owner_sync_z1_20260317T191642Z/contract_runs`
- `./scripts/qa-bl068-temporal-effects-mac.sh --execute --runs 3 --out-dir TestEvidence/bl068_owner_sync_z1_20260317T191642Z/execute_runs`

Owner sync packet:
- `TestEvidence/bl068_owner_sync_z1_20260317T191642Z/promotion_decision.md`

| Artifact | Result | Notes |
|---|---|---|
| `contract_runs/status.tsv` | 32 rows PASS | 0 failures across pre-flight, constants, matrix checks, and lane result |
| `execute_runs/status.tsv` | 34 rows PASS | 0 failures across pre-flight, constants, compile-backed probe, matrix checks, and lane result |
| `contract_runs/temporal_modes_matrix.tsv` | 40 rows PASS (4 modes × 10 runs) | Delay/echo/looper/frippertronics mode metadata stayed deterministic at promotion depth |
| `execute_runs/temporal_modes_matrix.tsv` | 12 rows PASS (4 modes × 3 runs) | Compile-backed probe validated the owned headers across all three execute runs |
| `execute_runs/probe_build/compile.log` | PASS | Dedicated BL-068 probe compiled cleanly without `build_local` reuse |
| `execute_runs/probe_build/probe.log` | PASS | Probe completed with no failing matrix rows |

Zero-TODO row count: **0** (across all four matrix artifacts in both owner replay packets).
Lane result: **PASS** (`contract_only;runs=10` and `execute;runs=3`).
Recommendation: **advance to Done-candidate**.

## Governance Alignment (2026-03-01)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.
