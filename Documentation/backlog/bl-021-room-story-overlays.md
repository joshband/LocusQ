Title: BL-021 Room-Story Overlays
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# BL-021 Room-Story Overlays

## Plain-Language Summary

BL-021 defines the room-story overlay contract for the viewport. It keeps room analysis rendering deterministic, degrades safely on partial payloads, and stays replayable for owner review. Current state: `In Implementation` with C4 parity green and owner intake/promotion still pending.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-021 Room-Story Overlays |
| Why is this important? | Keeps room overlays deterministic, safe on partial data, and easy to validate. |
| How will we deliver it? | Prove the contract through replayable validation lanes and compact evidence packets. |
| When is it done? | When the active acceptance gates pass and owner intake/promotion is approved. |
| Where is the source of truth? | This runbook plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Milestone snapshot table | Compresses the historical replay trail into one scan. | `## Milestone Snapshot` |
| Validation table | Shows the live gate set and evidence contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Implementation (C4 parity green; owner intake/promotion decision pending) |
| Owner Track | Track E — R&D Expansion |
| Depends On | BL-014 (Done), BL-015 (Done), HX-05 |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| Annex Spec | Inline deterministic overlay state contract |

## Objective

Define a deterministic room-story overlay contract for the viewport. Preserve predictability, additive degradation, and replayable owner evidence.

## Core Contract

### Overlay Modes

| Mode ID | Description | Minimum Payload Inputs |
|---|---|---|
| `overlay_off` | No room-story overlays rendered | none |
| `overlay_reflection_paths` | Draw first-order reflection path lines | `room.reflections[]` finite records |
| `overlay_decay_heatmap` | Render decay energy color field | `room.decay_bands[]` finite RT60 bands |
| `overlay_absorption_zones` | Render zone tint by band absorption | `room.absorption_zones[]` finite zone coefficients |
| `overlay_composite_all` | Render all enabled layers together | reflection + decay + absorption payloads |

Mode rules:
- One control source is authoritative at a time.
- `overlay_composite_all` is additive. One bad layer does not hard-fail the others.

### Runtime States

| State ID | Enter Condition | Exit Condition | Deterministic Behavior |
|---|---|---|---|
| `state_idle` | Startup or mode=`overlay_off` | Mode switches to non-off | Emit no overlay geometry updates |
| `state_waiting_payload` | Mode active but no accepted payload yet | First accepted payload | Keep prior frame unchanged; no speculative geometry |
| `state_active_full` | Required payload for selected mode fully present and finite | Payload becomes partial/missing/stale | Render full selected layer set |
| `state_active_degraded` | Selected mode has partial payload availability | Missing fields restored or stale timeout exceeded | Render available layers only; missing layers replaced by fallback visuals |
| `state_stale_hold` | Last accepted payload age exceeds stale threshold and no newer valid payload | Fresh payload accepted or hold timeout exceeded | Freeze last good geometry up to hold window |
| `state_fallback_safe` | Hold timeout exceeded or payload invalid repeatedly | Fresh valid payload accepted | Disable unavailable layers; emit explicit fallback classification |

### Transition Precedence

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

### Additive Fallback

Fallback must be layer-local and deterministic:

| Payload Condition | Layer Impact | Fallback Rule |
|---|---|---|
| Missing `room.reflections[]` | Reflection layer unavailable | Hide reflection lines; keep other active layers |
| Missing `room.decay_bands[]` | Decay heatmap unavailable | Render neutral decay legend state (`no_decay_data`) |
| Missing `room.absorption_zones[]` | Absorption tint unavailable | Render untinted boundaries (`no_absorption_data`) |
| Non-finite numeric field | Affected record invalid | Drop invalid record only; classify as `non_finite_payload_field` |
| Empty arrays for selected mode | Selected layer unavailable | Degrade to `state_active_degraded`; do not force mode reset |
| Repeated invalid payload beyond hold window | Global overlay unsafe | Transition to `state_fallback_safe` |

Fallback thresholds:
- `stale_hold_ms_max = 750`
- `max_invalid_payload_events_before_safe = 3` contiguous events
- `transition_processing_budget_ms_p95 <= 16`

### Active Acceptance Gates

| Acceptance ID | Gate | Pass Rule | Evidence |
|---|---|---|---|
| `BL021-A1-001` | Mode catalog fixed | 5 named modes only | backlog + QA parity |
| `BL021-A1-002` | Runtime state catalog fixed | 6 named states only | backlog + QA parity |
| `BL021-A1-003` | Transition precedence deterministic | Precedence list order matches contract | transition table |
| `BL021-A1-004` | Fallback is layer-local | Partial payload degrades only affected layers | fallback matrix |
| `BL021-A1-005` | Thresholds bounded | `stale_hold_ms_max <= 750`, invalid-events max `<= 3`, p95 `<= 16ms` | threshold table |
| `BL021-A1-006` | Replay is deterministic | Same event log yields same transition hash | replay rules |
| `BL021-C4-001..007` | 20-run parity and exit guards | Contract-only, execute-suite, parity, docs freshness, and schema checks pass | C4 bundle |

### Replay Expectations

Deterministic replay contract for future implementation slices:
1. Given identical ordered control/payload event stream, transition sequence must be identical.
2. Transition trace hash must match across at least 3 replays (`run_01..run_03`).
3. Per-event classification rows must be stable (no row-count drift, no ordering drift).
4. Any mismatch classifies as deterministic contract failure, not transient runtime flake.

Planned replay artifacts (future executable lane):
- `overlay_transition_trace.tsv`
- `replay_hashes.tsv`
- `fallback_classification.tsv`

## Milestone Snapshot

| Milestone | Date | Result | Why it matters | Evidence |
|---|---|---|---|---|
| A1 contract | 2026-02-26 | PASS | Defined modes, states, fallback, and thresholds. | `TestEvidence/bl021_slice_a1_contract_20260226T165747Z/status.tsv` |
| B1 lane | 2026-02-26 | PASS with docs-freshness debt | Replay was stable; docs freshness was external debt then. | `TestEvidence/bl021_slice_b1_lane_20260226T172116Z` |
| N6 owner recheck | 2026-02-26 | PASS | BL-021 advanced to `In Implementation`. | `TestEvidence/owner_sync_bl020_bl021_bl023_bl030_n6_20260226T172348Z/bl021_recheck/status.tsv` |
| C2 soak | 2026-02-26 | PASS | Soak evidence stayed deterministic. | `TestEvidence/bl021_slice_c2_soak_20260226T193200Z` |
| N13 owner recheck | 2026-02-26 | PASS | Fresh owner replay reinforced determinism. | `TestEvidence/owner_sync_bl030_bl021_bl023_n13_20260226T203010Z` |
| C4 parity | 2026-02-28 | PASS | 20-run contract-only and execute-suite parity held. | `TestEvidence/bl021_slice_c4_mode_parity_20260228T170131Z` |
| C4 reconfirm | 2026-02-28 | PASS | Parity and exit semantics stayed stable. | `TestEvidence/bl021_slice_c4_mode_parity_20260228T171133Z` |
| C4b recheck | 2026-02-28 | PASS | Non-interference check stayed green. | `TestEvidence/bl021_slice_c4b_mode_parity_20260228T202813Z` |
| Status refresh | 2026-03-01 | PASS | No new deterministic blocker recorded. | `TestEvidence/bl021_slice_c4_mode_parity_20260228T170131Z/status.tsv`, `TestEvidence/bl021_slice_c4b_mode_parity_20260228T202813Z/status.tsv` |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| `BL021-A1-doc-freshness` | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |
| `BL021-C4-parity` | Automated | `./scripts/qa-bl021-room-story-overlays-lane-mac.sh --contract-only --runs 20 --out-dir .../contract_runs` and `--execute-suite --runs 20 --out-dir .../execute_runs` | Both PASS |
| `BL021-C4-exit-guards` | Automated | `./scripts/qa-bl021-room-story-overlays-lane-mac.sh --runs 0` and `--unknown-flag` | Exit `2` for both |
| `BL021-C4-docs-freshness` | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Ambiguous mode/state mapping during implementation | High | Med | Freeze canonical IDs in A1 and require parity checks |
| Overly strict fallback causes UX dropouts | Med | Med | Additive layer-local degradation rules |
| Nondeterministic replay ordering | High | Med | Contractual precedence + trace hash requirement |

## Current Blocker / Decision Status

- Current status: `In Implementation`.
- Current blocker: no new technical blocker is recorded.
- Open decision: owner intake/promotion review.
- Note: the requested C3 sentinel packet path was not present; owner used the latest available C2 soak packet plus fresh N13 recheck.

## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment

Canonical lifecycle and evidence rules live in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

## Archive Note

The full packet-by-packet replay diary was removed from the active runbook on purpose. If the deep chronology is needed again, use the legacy copy preserved under `Documentation/archive/2026-03-18-doc-surface-consolidation/`.
