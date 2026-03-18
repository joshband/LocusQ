Title: BL-021 Room-Story Overlays QA Contract
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-18

# BL-021 Room-Story Overlays QA Contract

## Status
Active.
This is the compact QA support surface for BL-021 room-story overlays.

## Purpose
Define deterministic acceptance criteria for room-story overlay behavior.
Keep replay outcomes stable.
Keep fallback taxonomy explicit.

## Authority Links
- Runbook: `Documentation/backlog/bl-021-room-story-overlays.md`
- Invariants: `Documentation/invariants.md`
- Related constraints: `Documentation/scene-state-contract.md`, HX-05 payload budget expectations

## Core Overlay Contract
Mode catalog is fixed:
- `overlay_off`
- `overlay_reflection_paths`
- `overlay_decay_heatmap`
- `overlay_absorption_zones`
- `overlay_composite_all`

Runtime state catalog is fixed:
- `state_idle`
- `state_waiting_payload`
- `state_active_full`
- `state_active_degraded`
- `state_stale_hold`
- `state_fallback_safe`

Determinism rules:
- precedence is `event_mode_off` > `event_payload_invalid` > `event_payload_stale_timeout` > `event_payload_partial` > `event_payload_full`
- equal timestamps keep precedence order
- equal timestamp and precedence keep source event order
- additive fallback is layer-local, not global-fail
- `stale_hold_ms_max <= 750`
- `max_invalid_payload_events_before_safe <= 3`
- `non_finite_payload_field` is explicit and deterministic

## Validation Tiers
| Tier | What It Proves | Primary Evidence |
|---|---|---|
| `A1` | contract parity for modes, states, fallback, replay, and cross-surface IDs | backlog + QA parity, hash parity, fallback matrix parity |
| `B1` | executable lane contract and machine-readable artifacts | `scripts/qa-bl021-room-story-overlays-lane-mac.sh`, `status.tsv`, `validation_matrix.tsv`, `replay_hashes.tsv`, `failure_taxonomy.tsv` |
| `C2` | soak hardening across contract-only and execute-suite runs | `soak_summary.tsv`, `contract_runs/*`, `exec_runs/*` |
| `C4` | 20-run parity and strict exit semantics | `mode_parity.tsv`, `replay_sentinel_summary.tsv`, `exit_semantics_probe.tsv` |

## Required Evidence Shape
Per run bundle:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`

Supporting files:
- `qa_lane.log`
- `scenario_contract.log`
- `scenario_result.log`
- `build.log`
- `scenario_run.log`
- `scenario_result.json`
- `lane_notes.md`
- `docs_freshness.log`

Replay outputs must stay stable on ordered input.
`transition_trace_hash`, row count, and row order must match across deterministic reruns.

## Highest-Signal Evidence
- `TestEvidence/bl021_slice_c4_mode_parity_20260228T170131Z/`
- `TestEvidence/bl021_slice_c4_mode_parity_20260228T171133Z/`
- `TestEvidence/bl021_slice_c4b_mode_parity_20260228T202813Z/`

Each snapshot shows:
- syntax + help `PASS`
- contract-only and execute-suite `PASS`
- usage/configuration probes `PASS` with exit `2`
- docs freshness `PASS`
- replay drift `0`
- mode parity `PASS`

## Milestone Snapshot
Current state:
- A1 contract parity locked
- B1 executable lane contract locked
- C2 soak hardening locked
- C4 parity and exit guards locked

Next action:
- keep the current evidence pointers fresh when new overlay slices land
- refresh the lane bundle only when acceptance IDs or fallback taxonomy change

## Validation
Primary gate:
```bash
./scripts/validate-docs-freshness.sh
```

## Archive Note
Long-form tier detail and repeated schema examples live in older evidence bundles and archived snapshots.
Use this file as the active QA contract.
