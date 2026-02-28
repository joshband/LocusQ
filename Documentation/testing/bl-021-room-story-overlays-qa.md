Title: BL-021 Room-Story Overlays QA Contract
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-28

# BL-021 Room-Story Overlays QA Contract

## Purpose
Define deterministic acceptance criteria for BL-021 room-story overlay behavior so future implementation slices can be validated with stable replay outcomes and explicit fallback taxonomy.

## Linked Contracts
- Runbook: `Documentation/backlog/bl-021-room-story-overlays.md`
- Invariants: `Documentation/invariants.md`
- Related constraints: `Documentation/scene-state-contract.md`, HX-05 payload budget expectations

## Acceptance ID Catalog (Slice A1)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL021-A1-001` | Mode catalog fixed (`overlay_off`, `overlay_reflection_paths`, `overlay_decay_heatmap`, `overlay_absorption_zones`, `overlay_composite_all`) | backlog + QA parity |
| `BL021-A1-002` | Runtime state catalog fixed (`state_idle`, `state_waiting_payload`, `state_active_full`, `state_active_degraded`, `state_stale_hold`, `state_fallback_safe`) | backlog + QA parity |
| `BL021-A1-003` | Transition precedence order fixed and deterministic | precedence table parity |
| `BL021-A1-004` | Additive fallback is layer-local, not global-fail | fallback matrix parity |
| `BL021-A1-005` | Stale hold bounded | `stale_hold_ms_max <= 750` |
| `BL021-A1-006` | Invalid event escalation bounded | `max_invalid_payload_events_before_safe <= 3` |
| `BL021-A1-007` | Non-finite payload handling explicit and deterministic | taxonomy mapping includes `non_finite_payload_field` |
| `BL021-A1-008` | Replay determinism requirement fixed | identical replay hash across repeated runs |
| `BL021-A1-009` | Cross-surface acceptance parity | IDs appear in backlog + QA + evidence matrix |

## Transition Decision Contract

Per-event resolution precedence (highest first):
1. `event_mode_off`
2. `event_payload_invalid`
3. `event_payload_stale_timeout`
4. `event_payload_partial`
5. `event_payload_full`

Determinism rule:
- For equal timestamp events, apply precedence order above.
- If timestamp and precedence are equal, preserve source event order.

## Fallback Taxonomy Contract

| Taxonomy ID | Class | Deterministic Trigger | Expected State Outcome |
|---|---|---|---|
| `missing_reflection_payload` | deterministic_contract_failure | Missing `room.reflections[]` when reflection layer requested | `state_active_degraded` |
| `missing_decay_payload` | deterministic_contract_failure | Missing `room.decay_bands[]` when decay layer requested | `state_active_degraded` |
| `missing_absorption_payload` | deterministic_contract_failure | Missing `room.absorption_zones[]` when absorption layer requested | `state_active_degraded` |
| `non_finite_payload_field` | deterministic_contract_failure | NaN/Inf in required numeric payload fields | record dropped; state unchanged/degraded |
| `payload_stale_timeout` | deterministic_contract_failure | Payload age exceeds hold window (`> 750ms`) | `state_fallback_safe` |
| `mode_off_selected` | control_event | Mode toggle to off | `state_idle` |

## Replay Expectations

Required replay invariants for executable slices:
1. Same ordered input events -> same ordered state transitions.
2. `transition_trace_hash` must be identical across replay runs.
3. `row_count_drift = 0` and `row_order_drift = 0` across replay outputs.
4. Fallback classifications must remain stable per event key.

Expected replay artifact schema:
- `overlay_transition_trace.tsv`: `event_seq`, `event_id`, `from_state`, `to_state`, `decision_reason`, `timestamp_utc`
- `replay_hashes.tsv`: `run_id`, `transition_trace_hash`, `row_count`, `row_order_signature`, `match`
- `fallback_classification.tsv`: `event_id`, `taxonomy_id`, `classification`, `reason`

## A1 Validation Commands

```bash
./scripts/validate-docs-freshness.sh
```

Pass criteria:
- Exit code `0`
- BL-021 backlog + QA acceptance IDs remain in parity

## Triage Sequence

1. If acceptance ID parity fails, fix ID drift before implementation work proceeds.
2. If fallback taxonomy is missing required IDs, update contract before executable lane authoring.
3. If replay expectations are underspecified, block promotion until hash/row stability rules are explicit.
4. If docs freshness fails, resolve metadata/tiering violations before closing slice.

## B1 Executable Lane Contract

Canonical lane script:
- `scripts/qa-bl021-room-story-overlays-lane-mac.sh`

Canonical scenario scaffold:
- `qa/scenarios/locusq_bl021_room_story_suite.json`

Supported lane modes/options:
- `--contract-only`
- `--execute-suite`
- `--runs <N>`
- `--out-dir <path>`

Strict exit semantics:
- `0` all enabled checks pass
- `1` one or more checks fail
- `2` usage/configuration error

### B1 Acceptance IDs

| Acceptance ID | Lane Check | Contract |
|---|---|---|
| `BL021-B1-001` | `BL021-B1-001_contract_schema` | Scenario/lane schema parseable and acceptance IDs declared |
| `BL021-B1-002` | `BL021-B1-002_transition_contract` | Transition state contract declared with required state set |
| `BL021-B1-003` | `BL021-B1-003_fallback_contract` | Fallback taxonomy contract declared with required entries |
| `BL021-B1-004` | `BL021-B1-004_replay_hash_stability` | Replay hash signatures stable across deterministic reruns |
| `BL021-B1-005` | `BL021-B1-005_artifact_schema_complete` | Required artifact schema present for selected mode |
| `BL021-B1-006` | `BL021-B1-006_hash_input_contract` | Hash input includes semantic rows and excludes nondeterministic fields |
| `BL021-B1-007` | `BL021-B1-007_execution_mode_contract` | `contract_only`/`execute_suite` mode contract preserved |
| `BL021-B1-008` | `BL021-B1-008_failure_taxonomy_schema` | Failure taxonomy schema includes deterministic/runtime classes |

### B1 Machine-Readable Artifacts

Required artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`

Supporting artifacts:
- `qa_lane.log`
- `scenario_contract.log`
- `scenario_result.log`
- `build.log` (execute mode)
- `scenario_run.log` (execute mode)
- `scenario_result.json` (execute mode when emitted)

### B1 Determinism Enforcement

For `--runs > 1`:
1. Lane executes deterministic per-run directories (`run_01..run_N`).
2. Replay signatures are computed from semantic status/contract/result/taxonomy content only.
3. Replay fails if:
   - signature divergence exceeds configured threshold
   - row-signature drift exceeds configured threshold
4. Divergence and drift counts are emitted in `validation_matrix.tsv` and `replay_hashes.tsv`.

## C2 Soak Hardening Contract

### C2 Acceptance Mapping

| C2 ID | Requirement | Lane Artifact |
|---|---|---|
| `BL021-C2-001` | Multi-run soak emits deterministic aggregate summary | `soak_summary.tsv` |
| `BL021-C2-002` | Contract-only replay stays hash-stable across `--runs 5` | `contract_runs/replay_hashes.tsv` |
| `BL021-C2-003` | Contract-only taxonomy remains explicit and bounded | `contract_runs/failure_taxonomy.tsv` |
| `BL021-C2-004` | Execute-suite replay emits machine-readable run matrix for `--runs 3` | `exec_runs/validation_matrix.tsv` |
| `BL021-C2-005` | Exit semantics unchanged (`0` pass, `1` gate fail, `2` usage error) | `status.tsv` + command exits |

### C2 Artifact Contract

Required bundle files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `exec_runs/validation_matrix.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

### C2 Validation Matrix

```bash
bash -n scripts/qa-bl021-room-story-overlays-lane-mac.sh
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --help
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only --runs 5 --out-dir TestEvidence/bl021_slice_c2_soak_<timestamp>/contract_runs
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --execute-suite --runs 3 --out-dir TestEvidence/bl021_slice_c2_soak_<timestamp>/exec_runs
./scripts/validate-docs-freshness.sh
```

## C4 Execute-Mode Parity + Exit Guard Contract

### C4 Acceptance Mapping

| C4 ID | Requirement | Lane Artifact |
|---|---|---|
| `BL021-C4-001` | Contract-only replay sentinel remains deterministic for `--runs 20` | `contract_runs/replay_hashes.tsv` |
| `BL021-C4-002` | Execute-suite replay sentinel remains deterministic for `--runs 20` | `execute_runs/replay_hashes.tsv` |
| `BL021-C4-003` | Contract/execute mode parity remains PASS at 20-run depth | `mode_parity.tsv` |
| `BL021-C4-004` | Replay sentinel summary includes both mode rows and PASS verdicts | `replay_sentinel_summary.tsv` |
| `BL021-C4-005` | Usage/configuration guards keep strict exit semantics (`2`) | `exit_semantics_probe.tsv` |
| `BL021-C4-006` | Docs freshness gate remains green | `docs_freshness.log` |
| `BL021-C4-007` | Required C4 evidence schema is complete | `status.tsv`, `validation_matrix.tsv` |

### C4 Validation Matrix

```bash
bash -n scripts/qa-bl021-room-story-overlays-lane-mac.sh
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --help
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl021_slice_c4_mode_parity_<timestamp>/contract_runs
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl021_slice_c4_mode_parity_<timestamp>/execute_runs
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --runs 0
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

### C4 Evidence Contract

Required files under `TestEvidence/bl021_slice_c4_mode_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `execute_runs/validation_matrix.tsv`
- `execute_runs/replay_hashes.tsv`
- `execute_runs/failure_taxonomy.tsv`
- `mode_parity.tsv`
- `replay_sentinel_summary.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## C4 Snapshot (2026-02-28)

- Evidence path: `TestEvidence/bl021_slice_c4_mode_parity_20260228T170131Z/`
- Validation outcome:
  - syntax + help: `PASS`
  - contract-only (`runs=20`): `PASS`
  - execute-suite (`runs=20`): `PASS`
  - usage/configuration probes: `PASS` (`--runs 0` => `2`, `--unknown-flag` => `2`)
  - docs freshness: `PASS`
- Determinism/parity summary:
  - contract-only replay drift: `signature_divergence=0`, `row_drift=0`
  - execute-suite replay drift: `signature_divergence=0`, `row_drift=0`
  - mode parity gate: `PASS`

## C4 Reconfirm Snapshot (2026-02-28)

- Evidence path: `TestEvidence/bl021_slice_c4_mode_parity_20260228T171133Z/`
- Validation outcome:
  - syntax + help: `PASS`
  - contract-only (`runs=20`): `PASS`
  - execute-suite (`runs=20`): `PASS`
  - usage/configuration probes: `PASS` (`--runs 0` => `2`, `--unknown-flag` => `2`)
  - docs freshness: `PASS`
- Determinism/parity summary:
  - contract-only replay drift: `signature_divergence=0`, `row_drift=0`
  - execute-suite replay drift: `signature_divergence=0`, `row_drift=0`
  - mode parity gate: `PASS`
