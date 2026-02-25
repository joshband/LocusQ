---
Title: HX-05 Payload Budget and Throttle Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-25
---

# HX-05: Payload Budget and Throughput Contract

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | Done (owner sync packet finalized from Slice D promotion evidence) |
| Owner Track | Track F - Hardening |
| Depends On | BL-016 (Done), BL-025 (Done) |
| Blocks | BL-027 throughput hardening slices |
| Slice Scope | Slice A+B+C+D (Slice D promotion packet replay + governance decision) |

## Objective

Define authoritative scene-state payload and bridge cadence limits so UI transport remains deterministic and bounded under load.

## Slice A Scope (Completed)

In scope:
- Hard budget thresholds (bytes/update, cadence, burst behavior, degradation behavior)
- Explicit pass/fail acceptance criteria tied to evidence artifacts
- Additive schema guidance only (no breaking contract changes)
- Traceability IDs mapped into implementation traceability

Out of scope:
- Runtime publisher/throttle code changes
- WebView rendering code changes
- Backlog/index/status promotion updates

## Authoritative Budget Contract (Normative)

| Budget Dimension | Normal Target | Soft Limit | Hard Limit | Enforcement Contract |
|---|---:|---:|---:|---|
| Serialized snapshot payload bytes/update | <= 24,576 B | <= 32,768 B | <= 65,536 B | Publisher must remain <= hard limit; soft-limit overage is temporary and burst-governed. |
| Scene-state publication cadence | 30 Hz nominal | <= 45 Hz burst cap | 60 Hz absolute cap | Publisher must never exceed 60 Hz. Above 30 Hz is allowed only during bounded burst windows. |
| Soft-overage burst window | n/a | max 8 consecutive snapshots over soft limit | n/a | Must recover to <= soft limit within 500 ms from burst start. |
| Hard-overage policy | n/a | n/a | 1 hard-overage snapshot triggers degrade tier | Immediate degrade action required; repeated hard overage escalates to safe mode. |

### Degradation Policy (Normative)

| Tier | Entry Condition | Required Behavior | Exit Condition |
|---|---|---|---|
| `normal` | Within soft limit and cadence target | Full payload at nominal cadence | n/a |
| `degrade_t1` | Hard overage once OR soft burst window exceeded | Clamp publication cadence to <= 20 Hz and prioritize core fields (`emitters`, `listener`, `speakers`, diagnostics) over optional overlays | 120 consecutive compliant snapshots |
| `degrade_t2_safe` | Hard overage in 3 of any 10 consecutive snapshots | Clamp cadence to <= 10 Hz and publish minimal deterministic transport subset until pressure clears | 240 consecutive compliant snapshots |

### Additive Schema Guidance (Slice A, non-breaking)

If transport budget telemetry is added in future slices, fields must be additive and optional:

- `snapshotPayloadBytes` (integer): serialized bytes for accepted snapshot
- `snapshotBudgetTier` (string enum): `normal`, `degrade_t1`, `degrade_t2_safe`
- `snapshotBurstCount` (integer): current consecutive over-soft count
- `snapshotBudgetPolicyVersion` (string): contract policy marker, initial value `hx05-v1`

Backward-compatibility rule: UI consumers must ignore unknown fields and preserve existing rendering behavior when these fields are absent.

## Acceptance Criteria (Slice A)

| Acceptance ID | Requirement | Pass Evidence |
|---|---|---|
| `HX05-AC-001` | Budget table defines explicit limits for bytes/update, cadence, burst, degradation | `Documentation/scene-state-contract.md` includes normative HX-05 budget section |
| `HX05-AC-002` | Additive schema guidance is documented with backward-compatibility behavior | `Documentation/scene-state-contract.md` includes optional additive field guidance |
| `HX05-AC-003` | Implementation traceability includes HX-05 contract rows | `Documentation/implementation-traceability.md` includes HX-05 rows |
| `HX05-AC-004` | Docs freshness gate passes after updates | `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/docs_freshness.log` exit 0 |

## Enforcement Checks (Measurable Artifacts)

| Check ID | Measurement | Artifact Contract | Pass/Fail Rule |
|---|---|---|---|
| `HX05-CHECK-01` | Serialized bytes/update distribution | `payload_metrics.tsv` (`snapshot_seq`, `bytes`, `cadence_hz`) | `p95(bytes) <= 32768` and `max(bytes) <= 65536` |
| `HX05-CHECK-02` | Cadence cap and burst duration | `transport_cadence.tsv` (`window_start_ms`, `hz`, `burst_count`) | `max(hz) <= 60` and no burst window > 500 ms |
| `HX05-CHECK-03` | Degrade tier transitions | `budget_tier_events.tsv` (`seq`, `tier`, `reason`) | Entry/exit transitions match policy and are deterministic for replayed input |
| `HX05-CHECK-04` | UI stale/fallback safety while degraded | `selftest_budget_guard.tsv` | No stale lockup; controls remain interactive |

## Slice A Evidence Bundle Contract

Required artifacts for this slice:
- `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/status.tsv`
- `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/budget_contract.md`
- `TestEvidence/hx05_payload_budget_slice_a_<timestamp>/docs_freshness.log`

## Slice B QA Lane Spec (This Slice)

### Lane Purpose

Define deterministic QA/stress validation contract for payload-budget enforcement slices so runtime implementation can be checked against fixed windows and artifact schemas.

### Canonical Lane Definition

| Lane ID | Intent | Execution Mode | Determinism Requirement |
|---|---|---|---|
| `HX05-LANE-SOAK` | Payload/cadence soak under bounded stress | deterministic replay inputs + fixed sample windows | identical input trace must produce identical pass/fail taxonomy counts and tier-transition sequence |

### Sample Window Contract

| Window ID | Duration | Purpose | Required Signals |
|---|---:|---|---|
| `W0_warmup` | 10 s | prime caches, discard startup transients | collect but exclude from pass/fail scoring |
| `W1_nominal` | 60 s | baseline cadence and payload in steady state | `bytes`, `cadence_hz`, `tier`, `burst_count` |
| `W2_burst` | 30 s | controlled emitter churn / burst pressure | `bytes`, `cadence_hz`, `tier`, `burst_count`, `reason` |
| `W3_sustained_stress` | 120 s | long-horizon degradation/recovery behavior | `bytes`, `cadence_hz`, `tier`, `burst_count`, `reason` |

Scoring rule: only `W1..W3` contribute to lane verdict.

### Pass/Fail Thresholds (Normative)

| Metric | Threshold | Scope | Fail Taxonomy Mapping |
|---|---|---|---|
| `max(bytes)` | `<= 65536` | `W1..W3` | `oversize_hard_limit` |
| `p95(bytes)` | `<= 32768` | `W1..W3` | `oversize_soft_limit` |
| `max(cadence_hz)` | `<= 60` | `W1..W3` | `cadence_violation` |
| burst over-soft run length | `<= 8` snapshots and `<= 500 ms` | `W2..W3` | `burst_overrun` |
| tier transition correctness | transitions must satisfy policy (`normal`, `degrade_t1`, `degrade_t2_safe`) | `W2..W3` | `degrade_tier_mismatch` |

### Artifact Schema Contract (Slice B)

| Artifact | Required Columns / Fields | Notes |
|---|---|---|
| `payload_metrics.tsv` | `window_id`, `snapshot_seq`, `utc_ms`, `bytes`, `tier`, `burst_count` | one row per accepted snapshot |
| `transport_cadence.tsv` | `window_id`, `window_start_ms`, `window_end_ms`, `cadence_hz`, `over_soft_count` | fixed analysis windows (1 s bins) |
| `budget_tier_events.tsv` | `snapshot_seq`, `window_id`, `from_tier`, `to_tier`, `reason`, `compliance_streak` | transition log only |
| `taxonomy_table.tsv` | `failure_code`, `count`, `first_snapshot_seq`, `first_window_id` | aggregate failure taxonomy |
| `status.tsv` | `lane`, `result`, `exit_code`, `timestamp`, `artifact` | machine-readable lane verdict |
| `qa_lane_contract.md` | contract version + thresholds + deterministic replay notes | human-readable summary |

### Failure Taxonomy (Slice B)

| Failure Code | Trigger Condition | Severity | Required Evidence |
|---|---|---|---|
| `oversize_hard_limit` | any snapshot `bytes > 65536` | hard fail | offending rows in `payload_metrics.tsv` |
| `oversize_soft_limit` | `p95(bytes) > 32768` across scored windows | fail | percentile report + raw rows |
| `burst_overrun` | over-soft burst length exceeds `8` snapshots or `500 ms` | fail | `transport_cadence.tsv` + `budget_tier_events.tsv` |
| `cadence_violation` | any scored window cadence exceeds `60 Hz` cap | hard fail | `transport_cadence.tsv` |
| `degrade_tier_mismatch` | observed transitions differ from policy entry/exit contract | fail | `budget_tier_events.tsv` replay diff |

### Acceptance Criteria (Slice B)

| Acceptance ID | Requirement | Pass Evidence |
|---|---|---|
| `HX05-B-AC-001` | Soak lane sample windows and scoring rules are specified | this document (`Slice B QA Lane Spec`) |
| `HX05-B-AC-002` | Artifact schemas are fully specified for deterministic validation | this document (`Artifact Schema Contract`) |
| `HX05-B-AC-003` | Failure taxonomy covers oversize, burst overrun, cadence violation, and tier mismatch | this document (`Failure Taxonomy`) + evidence `taxonomy_table.tsv` |
| `HX05-B-AC-004` | Slice B acceptance IDs are mapped in implementation traceability | `Documentation/implementation-traceability.md` HX-05 Slice B rows |
| `HX05-B-AC-005` | Docs freshness gate passes for Slice B docs-only update | `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/docs_freshness.log` exit 0 |

## Slice B Evidence Bundle Contract

Required artifacts for this slice:
- `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/status.tsv`
- `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/qa_lane_contract.md`
- `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/taxonomy_table.tsv`
- `TestEvidence/hx05_payload_budget_slice_b_<timestamp>/docs_freshness.log`

## Slice C Soak Harness (This Slice)

### Harness Implementation

Script:
- `scripts/qa-hx05-payload-budget-soak-mac.sh`

Interface:
- `--input-dir <path>` (required)
- `--out-dir <path>` (optional)
- `--label <name>` (optional)
- `--help`

Strict exits:
- `0`: pass (schema + thresholds + transition checks all valid)
- `1`: fail (schema invalid or contract violation)
- `2`: invocation/usage error

### Enforced Inputs and Schemas

Required input artifacts:
- `payload_metrics.tsv`
- `transport_cadence.tsv`
- `budget_tier_events.tsv`
- `taxonomy_table.tsv`

Required schema columns:
- payload metrics: `window_id`, `snapshot_seq`, `utc_ms`, `bytes`, `tier`, `burst_count`
- transport cadence: `window_id`, `window_start_ms`, `window_end_ms`, `cadence_hz`, `over_soft_count`
- tier events: `snapshot_seq`, `window_id`, `from_tier`, `to_tier`, `reason`, `compliance_streak`
- taxonomy: `failure_code`, `count`, `first_snapshot_seq`, `first_window_id`

### Deterministic Threshold Evaluation

Scored windows:
- `W1_nominal`, `W2_burst`, `W3_sustained_stress`

Burst windows:
- `W2_burst`, `W3_sustained_stress`

Deterministic checks:
- `max(bytes) <= 65536`
- nearest-rank `p95(bytes) <= 32768`
- `max(cadence_hz) <= 60`
- `max burst_count <= 8` and `max burst duration <= 500 ms`
- tier transitions and recovery streaks match policy

### Slice C Failure Taxonomy

Output taxonomy rows are deterministic and ordered:
- `oversize_hard_limit`
- `oversize_soft_limit`
- `cadence_violation`
- `burst_overrun`
- `degrade_tier_mismatch`
- `schema_invalid`
- `none`

### Acceptance Criteria (Slice C)

| Acceptance ID | Requirement | Pass Evidence |
|---|---|---|
| `HX05-C-AC-001` | Harness script exists with strict exit semantics and help contract | `scripts/qa-hx05-payload-budget-soak-mac.sh` + `--help` output |
| `HX05-C-AC-002` | Harness validates all required artifact schemas before scoring | harness `status.tsv` (`schema_validation`) + `eval.log` |
| `HX05-C-AC-003` | Harness enforces A+B thresholds deterministically on scored windows | harness `status.tsv` + `taxonomy_table.tsv` |
| `HX05-C-AC-004` | PASS fixture returns exit `0` and FAIL fixture returns exit `1` | `pass_fixture_result.tsv`, `fail_fixture_result.tsv` |
| `HX05-C-AC-005` | Docs freshness gate passes for Slice C change set | `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/docs_freshness.log` |

## Slice C Evidence Bundle Contract

Required artifacts for this slice:
- `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/status.tsv`
- `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/qa_lane_contract.md`
- `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/taxonomy_table.tsv`
- `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/pass_fixture_result.tsv`
- `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/fail_fixture_result.tsv`
- `TestEvidence/hx05_payload_budget_slice_c_<timestamp>/docs_freshness.log`

## Slice D Done Promotion Packet (This Slice)

### Input Evidence Consolidation

Slice D uses the following prior evidence bundles as promotion inputs:
- `TestEvidence/hx05_payload_budget_slice_a_20260225T171134Z/`
- `TestEvidence/hx05_payload_budget_slice_b_20260225T174310Z/`
- `TestEvidence/hx05_payload_budget_slice_c_20260225T175148Z/`

### Fresh Validation Replay (Slice D)

| Check | Command | Expected | Observed | Result |
|---|---|---|---|---|
| PASS fixture replay | `./scripts/qa-hx05-payload-budget-soak-mac.sh --input-dir TestEvidence/hx05_payload_budget_slice_c_20260225T175148Z/fixtures/pass --out-dir TestEvidence/hx05_done_promotion_slice_d_20260225T211307Z/pass_run --label HX05-DONE` | exit `0` | exit `0` | PASS |
| FAIL fixture negative control | `./scripts/qa-hx05-payload-budget-soak-mac.sh --input-dir TestEvidence/hx05_payload_budget_slice_c_20260225T175148Z/fixtures/fail --out-dir TestEvidence/hx05_done_promotion_slice_d_20260225T211307Z/fail_run --label HX05-DONE` | exit `1` | exit `1` | PASS |
| Docs freshness | `./scripts/validate-docs-freshness.sh` | exit `0` | exit `0` | PASS |

### Promotion Decision Rule and Verdict

Promotion rule:
1. PASS fixture exits `0`
2. FAIL fixture exits `1` (negative control confirms contract failure detection)
3. docs freshness exits `0`

Verdict for Slice D replay: **PASS**.  
HX-05 is **Done** on current branch state (owner promotion completed).

### Slice D Evidence Bundle Contract

Required artifacts for this slice:
- `TestEvidence/hx05_done_promotion_slice_d_<timestamp>/status.tsv`
- `TestEvidence/hx05_done_promotion_slice_d_<timestamp>/validation_matrix.tsv`
- `TestEvidence/hx05_done_promotion_slice_d_<timestamp>/pass_fixture_result.tsv`
- `TestEvidence/hx05_done_promotion_slice_d_<timestamp>/fail_fixture_result.tsv`
- `TestEvidence/hx05_done_promotion_slice_d_<timestamp>/promotion_decision.md`
- `TestEvidence/hx05_done_promotion_slice_d_<timestamp>/docs_freshness.log`

## Closeout Checklist (Slice A+B+C+D)

- [x] Hard payload/throughput limits documented
- [x] Burst and degradation policy documented
- [x] Additive schema guidance documented
- [x] Acceptance IDs and measurable checks documented
- [x] Traceability rows added in implementation traceability
- [x] Slice B lane windows/thresholds/artifact schemas documented
- [x] Slice B failure taxonomy documented
- [x] Slice B acceptance IDs defined
- [x] Slice C soak harness script implemented with strict exit semantics
- [x] Slice C fixture-based PASS/FAIL validation defined
- [x] Slice D done-promotion replay packet captured with pass/fail negative control and docs freshness gate
- [x] Runtime budget enforcement implementation intentionally deferred to future follow-on item (non-blocking for HX-05 contract closeout)
- [x] Runtime stress/perf validation lane intentionally deferred to future follow-on item (non-blocking for HX-05 contract closeout)
