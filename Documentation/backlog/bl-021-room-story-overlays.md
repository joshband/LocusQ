Title: BL-021 Room-Story Overlays
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-28

# BL-021 Room-Story Overlays

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Implementation (C2 soak packet PASS; C4 execute-mode parity + exit-guard packet PASS on 2026-02-28; N13 owner recheck `--contract-only --runs 3` PASS with stable replay signatures and zero row drift; deterministic confidence reinforced) |
| Owner Track | Track E — R&D Expansion |
| Depends On | BL-014 (Done), BL-015 (Done), HX-05 |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| Annex Spec | Inline deterministic overlay state contract |

## Objective

Define a deterministic room-story overlay contract for the viewport so room analysis data can be rendered predictably, degrade safely on partial payloads, and be validated with replayable acceptance criteria.

## Slice A1 Scope (Docs-Only)

Slice A1 establishes the contract only:
1. Overlay modes, runtime states, and transition precedence.
2. Additive fallback behavior for missing/partial payload content.
3. Acceptance IDs with measurable thresholds and replay expectations.
4. Deterministic failure taxonomy for closeout decisions.

Out of scope for A1:
- Source implementation
- Script changes
- Runtime lane execution beyond docs freshness gate

## Overlay Mode Contract

| Mode ID | Description | Minimum Payload Inputs |
|---|---|---|
| `overlay_off` | No room-story overlays rendered | none |
| `overlay_reflection_paths` | Draw first-order reflection path lines | `room.reflections[]` finite records |
| `overlay_decay_heatmap` | Render decay energy color field | `room.decay_bands[]` finite RT60 bands |
| `overlay_absorption_zones` | Render zone tint by band absorption | `room.absorption_zones[]` finite zone coefficients |
| `overlay_composite_all` | Render all enabled layers together | reflection + decay + absorption payloads |

Mode constraints:
- Only one mode selection source is authoritative at a time (latest valid control event by sequence).
- `overlay_composite_all` is additive: each layer may degrade independently without forcing global hard-fail.

## Runtime State Contract

| State ID | Enter Condition | Exit Condition | Deterministic Behavior |
|---|---|---|---|
| `state_idle` | Startup or mode=`overlay_off` | Mode switches to non-off | Emit no overlay geometry updates |
| `state_waiting_payload` | Mode active but no accepted payload yet | First accepted payload | Keep prior frame unchanged; no speculative geometry |
| `state_active_full` | Required payload for selected mode fully present and finite | Payload becomes partial/missing/stale | Render full selected layer set |
| `state_active_degraded` | Selected mode has partial payload availability | Missing fields restored or stale timeout exceeded | Render available layers only; missing layers replaced by fallback visuals |
| `state_stale_hold` | Last accepted payload age exceeds stale threshold and no newer valid payload | Fresh payload accepted or hold timeout exceeded | Freeze last good geometry up to hold window |
| `state_fallback_safe` | Hold timeout exceeded or payload invalid repeatedly | Fresh valid payload accepted | Disable unavailable layers; emit explicit fallback classification |

## Transition Rules (Deterministic Precedence)

When multiple events co-occur in one cycle, evaluate in this order:
1. `event_mode_off`
2. `event_payload_invalid`
3. `event_payload_stale_timeout`
4. `event_payload_partial`
5. `event_payload_full`

Transition matrix:

| From | Event | To | Contract |
|---|---|---|---|
| `state_idle` | `event_mode_on` | `state_waiting_payload` | Wait for accepted payload; no synthetic defaults |
| `state_waiting_payload` | `event_payload_full` | `state_active_full` | Render full selected mode |
| `state_waiting_payload` | `event_payload_partial` | `state_active_degraded` | Render additive subset + fallbacks |
| `state_active_full` | `event_payload_partial` | `state_active_degraded` | Keep valid layers active, degrade missing layers only |
| `state_active_full` | `event_payload_invalid` | `state_stale_hold` | Hold last-good frame until timeout |
| `state_active_degraded` | `event_payload_full` | `state_active_full` | Promote to full rendering without mode reset |
| `state_active_degraded` | `event_payload_invalid` | `state_stale_hold` | Freeze degraded frame until timeout |
| `state_stale_hold` | `event_payload_full` | `state_active_full` | Resume full rendering |
| `state_stale_hold` | `event_payload_partial` | `state_active_degraded` | Resume degraded rendering |
| `state_stale_hold` | `event_payload_stale_timeout` | `state_fallback_safe` | Enter safe fallback state deterministically |
| any non-idle | `event_mode_off` | `state_idle` | Clear overlay visibility and state counters |

## Additive Fallback Contract

Fallback must be layer-local and deterministic:

| Payload Condition | Layer Impact | Fallback Rule |
|---|---|---|
| Missing `room.reflections[]` | Reflection layer unavailable | Hide reflection lines; keep other active layers |
| Missing `room.decay_bands[]` | Decay heatmap unavailable | Render neutral decay legend state (`no_decay_data`) |
| Missing `room.absorption_zones[]` | Absorption tint unavailable | Render untinted boundaries (`no_absorption_data`) |
| Non-finite numeric field | Affected record invalid | Drop invalid record only; classify as `non_finite_payload_field` |
| Empty arrays for selected mode | Selected layer unavailable | Degrade to `state_active_degraded`; do not force mode reset |
| Repeated invalid payload beyond hold window | Global overlay unsafe | Transition to `state_fallback_safe` |

Fallback window thresholds (A1 contract values):
- `stale_hold_ms_max = 750`
- `max_invalid_payload_events_before_safe = 3` contiguous events
- `transition_processing_budget_ms_p95 <= 16`

## Acceptance IDs (Slice A1)

| Acceptance ID | Requirement | Threshold / Deterministic Rule | Evidence Signal |
|---|---|---|---|
| `BL021-A1-001` | Overlay mode catalog fixed and named | Exactly 5 mode IDs in this contract | backlog + QA doc parity |
| `BL021-A1-002` | Runtime state catalog fixed and named | Exactly 6 state IDs in this contract | backlog + QA doc parity |
| `BL021-A1-003` | Transition precedence is deterministic | Precedence list length=5 and order match required | transition precedence table |
| `BL021-A1-004` | Additive fallback is layer-local | Partial payload degrades only affected layer(s) | fallback matrix |
| `BL021-A1-005` | Stale hold bounded | `stale_hold_ms_max <= 750` | threshold table |
| `BL021-A1-006` | Safe fallback activation bounded | `max_invalid_payload_events_before_safe <= 3` | threshold table |
| `BL021-A1-007` | Non-finite input handling explicit | Invalid record dropped with taxonomy classification | taxonomy table |
| `BL021-A1-008` | Replay determinism contract declared | Same event log yields identical transition trace hash | replay expectation section |
| `BL021-A1-009` | Acceptance mapping is cross-referenced | IDs present in backlog + QA + evidence matrix | A1 evidence packet |

## Replay Expectations (A1 Contract)

Deterministic replay contract for future implementation slices:
1. Given identical ordered control/payload event stream, transition sequence must be identical.
2. Transition trace hash must match across at least 3 replays (`run_01..run_03`).
3. Per-event classification rows must be stable (no row-count drift, no ordering drift).
4. Any mismatch classifies as deterministic contract failure, not transient runtime flake.

Planned replay artifacts (future executable lane):
- `overlay_transition_trace.tsv`
- `replay_hashes.tsv`
- `fallback_classification.tsv`


## Validation Plan (A1 Docs Contract)

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| `BL021-A1-doc-freshness` | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |
| `BL021-A1-contract-parity` | Manual | Compare backlog + QA acceptance IDs | Full ID parity |

## Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Ambiguous mode/state mapping during implementation | High | Med | Freeze canonical IDs in A1 and require parity checks |
| Overly strict fallback causes UX dropouts | Med | Med | Additive layer-local degradation rules |
| Nondeterministic replay ordering | High | Med | Contractual precedence + trace hash requirement |

## Closeout Checklist (Slice A1)

- [x] Overlay modes/states defined with deterministic IDs.
- [x] Transition precedence and matrix documented.
- [x] Additive fallback behavior documented for missing/partial payloads.
- [x] Acceptance IDs and thresholds documented.
- [x] QA contract doc created and cross-referenced.
- [x] Docs freshness validation executed with evidence.

## Owner Sync N4 Intake (2026-02-26)

- Owner-authoritative intake packet: `TestEvidence/bl021_slice_a1_contract_20260226T165747Z/status.tsv`
- Gate summary:
  - room-story contract: `PASS`
  - acceptance matrix: `PASS`
  - taxonomy table: `PASS`
  - docs freshness: `PASS`
- Owner classification:
  - Slice A1 is accepted and complete.
  - Backlog posture remains `In Planning` pending implementation slices.

## Slice B1 QA Lane Intake (2026-02-26)

- Worker packet directory: `TestEvidence/bl021_slice_b1_lane_20260226T172116Z`
- Validation summary:
  - lane lint/help: `PASS`
  - contract-only replay (`runs=3`): `PASS` (stable hashes and row signatures)
  - docs freshness: `FAIL` (external metadata debt outside B1 ownership)
- Owner interpretation:
  - B1 lane outputs are coherent and replay-stable.
  - Contract artifacts are complete (`status.tsv`, `validation_matrix.tsv`, `replay_hashes.tsv`, `failure_taxonomy.tsv`).

## Owner Sync N6 Intake (2026-02-26)

- Owner recheck bundle: `TestEvidence/owner_sync_bl020_bl021_bl023_bl030_n6_20260226T172348Z/bl021_recheck/status.tsv`
- Recheck result:
  - `./scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only --runs 3`: `PASS`
- Owner decision:
  - BL-021 advances to `In Implementation`.
  - External docs-freshness blocker is tracked at owner sync level and is not a BL-021 contract failure.

## Slice C2 Soak Hardening Contract

Slice C2 strengthens replay determinism for BL-021 lane evidence while preserving B1 mode and exit semantics.

### C2 Scope

- Harden multi-run lane outputs for deterministic replay interpretation.
- Preserve `--contract-only` behavior as default contract lane.
- Preserve strict exit semantics:
  - `0` pass
  - `1` gate fail
  - `2` usage/configuration error

### C2 Acceptance IDs

| ID | Requirement | Evidence |
|---|---|---|
| `BL021-C2-001` | Aggregate soak summary is emitted for multi-run lanes | `soak_summary.tsv` |
| `BL021-C2-002` | Contract-only runs (`--runs 5`) remain replay-stable | `contract_runs/replay_hashes.tsv` |
| `BL021-C2-003` | Failure taxonomy remains deterministic and machine-readable | `contract_runs/failure_taxonomy.tsv` |
| `BL021-C2-004` | Execute-suite runs (`--runs 3`) emit deterministic run matrix | `exec_runs/validation_matrix.tsv` |
| `BL021-C2-005` | QA/runbook artifact schema stays explicit and complete | backlog + QA doc parity |

### C2 Required Evidence Bundle

- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `exec_runs/validation_matrix.tsv`
- `soak_summary.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C2 Soak Intake (2026-02-26)

- Worker packet directory: `TestEvidence/bl021_slice_c2_soak_20260226T193200Z`
- Validation summary:
  - contract-only soak (`runs=5`): `PASS`
  - execute-suite replay (`runs=3`): `PASS`
  - docs freshness: `PASS`
- Owner interpretation:
  - C2 soak evidence is coherent and deterministic.
  - C2 intake is accepted for implementation confidence hardening.

## Owner Sync N13 Intake (2026-02-26)

- Owner packet directory: `TestEvidence/owner_sync_bl030_bl021_bl023_n13_20260226T203010Z`
- Owner recheck command:
  - `./scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only --runs 3 --out-dir .../bl021_recheck`: `PASS`
- Determinism summary:
  - replay hash divergence: `0`
  - row drift: `0`
- Owner decision:
  - BL-021 remains `In Implementation`.
  - Deterministic confidence is reinforced by fresh owner-authoritative replay.
- Note:
  - Requested C3 sentinel packet path was not present; owner used latest available C2 soak packet plus fresh N13 recheck.

## Slice C4 Execute-Mode Parity + Exit Guard Contract

Slice C4 extends deterministic confidence by requiring 20-run replay parity across both lane modes and strict usage/configuration exit guards.

### C4 Acceptance IDs

| ID | Requirement | Evidence |
|---|---|---|
| `BL021-C4-001` | Contract-only replay remains deterministic for `--runs 20` | `contract_runs/replay_hashes.tsv` |
| `BL021-C4-002` | Execute-suite replay remains deterministic for `--runs 20` | `execute_runs/replay_hashes.tsv` |
| `BL021-C4-003` | Contract-only vs execute-suite parity remains PASS at 20-run depth | `mode_parity.tsv` |
| `BL021-C4-004` | Replay sentinel aggregate captures both mode summaries | `replay_sentinel_summary.tsv` |
| `BL021-C4-005` | Strict usage/configuration exit guards remain enforced (`--runs 0`, `--unknown-flag` => exit `2`) | `exit_semantics_probe.tsv` |
| `BL021-C4-006` | Docs freshness gate remains green at closeout | `docs_freshness.log` |
| `BL021-C4-007` | C4 artifact schema is complete and machine-readable | `status.tsv`, `validation_matrix.tsv` |

### C4 Validation Plan

```bash
bash -n scripts/qa-bl021-room-story-overlays-lane-mac.sh
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --help
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl021_slice_c4_mode_parity_<timestamp>/contract_runs
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl021_slice_c4_mode_parity_<timestamp>/execute_runs
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --runs 0
./scripts/qa-bl021-room-story-overlays-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

### C4 Required Evidence Bundle

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

## Slice C4 Execute-Mode Parity Intake (2026-02-28)

- Worker packet directory: `TestEvidence/bl021_slice_c4_mode_parity_20260228T170131Z`
- Validation summary:
  - lane lint/help: `PASS`
  - contract-only replay (`runs=20`): `PASS`
  - execute-suite replay (`runs=20`): `PASS`
  - usage/configuration probes: `PASS` (`--runs 0` => `2`, `--unknown-flag` => `2`)
  - docs freshness: `PASS`
- Determinism summary:
  - contract-only signature divergence: `0`, row drift: `0`
  - execute-suite signature divergence: `0`, row drift: `0`
  - mode parity gate: `PASS`
- Owner interpretation:
  - BL-021 remains `In Implementation`.
  - C4 packet confirms execute-mode parity and strict usage-exit guards are deterministic at 20-run depth.

## Slice C4 Execute-Mode Parity Reconfirm (2026-02-28)

- Reconfirm packet directory: `TestEvidence/bl021_slice_c4_mode_parity_20260228T171133Z`
- Reconfirm summary:
  - contract-only replay (`runs=20`): `PASS`
  - execute-suite replay (`runs=20`): `PASS`
  - usage/configuration probes: `PASS` (`--runs 0` => `2`, `--unknown-flag` => `2`)
  - docs freshness: `PASS`
- Notes:
  - Replay parity and strict usage-exit semantics remain deterministic.
  - C4 required evidence set includes `execute_runs/failure_taxonomy.tsv`.

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

All worker and owner handoffs for this runbook must include:
- `SHARED_FILES_TOUCHED: no|yes`

Required return block:
```
HANDOFF_READY
TASK: <BL ID + Title>
RESULT: PASS|FAIL
FILES_TOUCHED: ...
VALIDATION: ...
ARTIFACTS: ...
SHARED_FILES_TOUCHED: no|yes
BLOCKERS: ...
```


## Governance Alignment (2026-02-28)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).
