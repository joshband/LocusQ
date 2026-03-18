Title: BL-020 Confidence Masking Overlay
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# BL-020 Confidence/Masking Overlay

## Plain-Language Summary

BL-020 defines a deterministic confidence/masking overlay contract. The goal is simple: keep overlay behavior replayable, degrade safely, and ship evidence that humans and agents can read fast. Current state: `In Validation`; the latest C4 refresh packet passed at `20260228T203021Z`, the C4b post-R1 packet passed at `20260228T202240Z`, and owner promotion review is still pending.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Operators, QA owners, release owners, and agents that need one source of truth. |
| What is changing? | The confidence/masking overlay contract. |
| Why is this important? | It keeps overlay behavior deterministic and safe to validate. |
| How will we deliver it? | Replay the contract, capture evidence, then seek owner promotion review. |
| When is it done? | When the active replay gates pass and promotion is owner-approved. |
| Where is the source of truth? | This runbook and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast scan of state, dependencies, and decision posture. | `## Status Ledger` |
| Core contract tables | Input, rendering, failure, and acceptance rules. | `## Core Contract` |
| Validation tables | Current gates and evidence shape. | `## Validation Plan` |
| Milestone snapshot | Compact history without replay archaeology. | `## Milestone Snapshot` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-020 |
| Priority | P2 |
| Status | In Validation (latest C4 refresh `PASS` at `20260228T203021Z`; C4b post-R1 `PASS` at `20260228T202240Z`; owner promotion review pending) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-014 (Done), BL-019 (Done) |
| Blocks | none |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| Current Decision | Wait for owner promotion review after green C4 evidence |

## Objective

Define a deterministic contract for confidence/masking overlays, including input rules, replay behavior, degradation behavior, acceptance thresholds, and QA evidence shape.

## Core Contract

Slice A1 is documentation-only and defines the normative contract for later implementation/validation slices.

### Input Contract

Per-emitter input object keys and rules:

| Field | Type | Valid Range / Enum | Required | Fallback Behavior |
|---|---|---|---|---|
| `snapshotSeq` | uint64 | monotonic non-decreasing | yes | `0`, mark `BL020-FX-006` |
| `emitterId` | uint | `0..255` | yes | row invalid, mark `BL020-FX-001` |
| `distanceConfidence` | float | finite `[0.0,1.0]` | yes | clamp, mark `BL020-FX-002` |
| `occlusionProbability` | float | finite `[0.0,1.0]` | yes | clamp, mark `BL020-FX-002` |
| `hrtfMatchQuality` | float | finite `[0.0,1.0]` | yes | clamp, mark `BL020-FX-002` |
| `maskingIndex` | float | finite `[0.0,1.0]` | yes | default `1.0`, mark `BL020-FX-005` |
| `combinedConfidence` | float | finite `[0.0,1.0]` | yes | recompute formula, mark `BL020-FX-003` |
| `overlayAlpha` | float | finite `[0.0,1.0]` | no | default `0.0` |
| `overlayBucket` | enum | `low|mid|high` | no | recompute from thresholds |
| `fallbackReason` | string | deterministic token | no | set a token when fallback is used |

Deterministic formula:

`combinedConfidence = 0.40*distanceConfidence + 0.30*(1.0-occlusionProbability) + 0.20*hrtfMatchQuality + 0.10*(1.0-maskingIndex)`

Formula tolerance: absolute error `<= 0.01`.

Bucket thresholds:
- `low`: `< 0.40`
- `mid`: `>= 0.40` and `< 0.80`
- `high`: `>= 0.80`

### Rendering Contract

| Contract ID | Expectation | Pass Rule |
|---|---|---|
| BL020-RD-001 | Overlay color/bucket mapping is deterministic for same inputs | 100% same bucket/color class across replay |
| BL020-RD-002 | Overlay alpha is bounded and finite | alpha always in `[0,1]` |
| BL020-RD-003 | Missing optional fields do not break render path | no throw/no hard fail; fallback token present |
| BL020-RD-004 | Required-field violations are surfaced as deterministic contract failures | taxonomy IDs emitted |

### Degradation Policy

When confidence/masking payload is incomplete or invalid:
- Preserve base emitter rendering.
- Disable only the overlay layer for the impacted row.
- Emit a deterministic fallback reason token.
- Record taxonomy classification for acceptance accounting.

Fallback priority order:
1. `schema_missing_required_field`
2. `value_out_of_range_or_non_finite`
3. `combined_confidence_formula_mismatch`
4. `overlay_bucket_mismatch`

### Acceptance Gates

| Acceptance ID | Gate | Pass Threshold |
|---|---|---|
| BL020-A1-001 | Required field/type validity | 100% active rows valid |
| BL020-A1-002 | Numeric range + finiteness | 0 pre-clamp violations |
| BL020-A1-003 | Combined formula conformance | max abs delta `<= 0.01` |
| BL020-A1-004 | Bucket mapping determinism | 100% row match |
| BL020-A1-005 | Fallback token determinism | 100% fallback rows tokenized |
| BL020-A1-006 | Snapshot sequence monotonicity | 0 regressions |
| BL020-A1-007 | QA artifact schema completeness | all required artifacts + columns present |

### QA Artifact Schema

Required bundle path:
`TestEvidence/bl020_slice_a1_contract_<timestamp>/`

Required artifacts:
- `status.tsv`
- `contract_spec.md`
- `acceptance_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

Required `acceptance_matrix.tsv` columns:
- `acceptance_id`, `gate`, `threshold`, `measured_value`, `result`, `evidence_path`

Required `failure_taxonomy.tsv` columns:
- `failure_id`, `category`, `trigger`, `classification`, `blocking`, `severity`, `expected_artifact`

### Failure Taxonomy

| Failure ID | Category | Trigger | Classification | Blocking |
|---|---|---|---|---|
| BL020-FX-001 | schema_missing_required_field | missing required key/type mismatch | deterministic_contract_failure | yes |
| BL020-FX-002 | value_out_of_range_or_non_finite | NaN/Inf or outside `[0,1]` | deterministic_contract_failure | yes |
| BL020-FX-003 | combined_confidence_formula_mismatch | abs delta `> 0.01` | deterministic_contract_failure | yes |
| BL020-FX-004 | overlay_bucket_mismatch | computed bucket differs from threshold rule | deterministic_contract_failure | yes |
| BL020-FX-005 | fallback_reason_missing_or_invalid | fallback path lacks valid reason token | deterministic_contract_failure | yes |
| BL020-FX-006 | snapshot_sequence_non_monotonic | `snapshotSeq` decreases | deterministic_contract_failure | yes |
| BL020-FX-007 | artifact_schema_incomplete | required artifact/columns missing | deterministic_evidence_failure | yes |

## Validation Plan

Current C4 gate:

| acceptance_id | gate | threshold |
|---|---|---|
| BL020-C4-001 | Contract-only replay sentinel | `--contract-only --runs 20` with `replay_hash_drift_count=0` and zero failing validation rows |
| BL020-C4-002 | Execute-suite replay sentinel | `--execute-suite --runs 20` with `replay_hash_drift_count=0` and zero failing validation rows |
| BL020-C4-003 | Cross-mode parity | `cross_mode_doc_hash_mismatch_count=0` and `cross_mode_scenario_hash_mismatch_count=0` |
| BL020-C4-004 | Contract taxonomy stability | `contract_failure_rows_nonzero=0` and `execute_failure_rows_nonzero=0` |
| BL020-C4-005 | Usage negative probe (`--runs 0`) | exit code must be `2` |
| BL020-C4-006 | Usage negative probe (`--unknown-flag`) | exit code must be `2` |
| BL020-C4-007 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

Failure taxonomy additions:

| failure_id | category | trigger | classification | blocking |
|---|---|---|---|---|
| BL020-C4-FX-001 | c4_backlog_contract_missing | backlog doc missing C4 validation/evidence clauses | deterministic_contract_failure | yes |
| BL020-C4-FX-002 | c4_qa_contract_missing | QA doc missing C4 validation/evidence clauses | deterministic_contract_failure | yes |
| BL020-C4-FX-003 | c4_scenario_contract_missing | scenario missing C4 mode-parity/exit contract metadata | deterministic_contract_failure | yes |
| BL020-C4-FX-004 | c4_script_exit_semantics_missing | lane script missing strict mode/usage-exit declarations | deterministic_contract_failure | yes |
| BL020-FX-401 | lane_c4_mode_parity_failure | contract and execute mode evidence mismatch | deterministic_replay_failure | yes |
| BL020-FX-402 | lane_c4_exit_semantics_failure | usage probes do not return strict exit `2` | deterministic_contract_failure | yes |
| BL020-FX-403 | lane_c4_docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes |
| BL020-FX-404 | lane_c4_evidence_schema_incomplete | required C4 evidence files missing | deterministic_evidence_failure | yes |

Required files under `TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `execute_runs/validation_matrix.tsv`
- `execute_runs/replay_hashes.tsv`
- `mode_parity.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

Current validation commands:
- `bash -n scripts/qa-bl020-confidence-masking-lane-mac.sh`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --help`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/contract_runs`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/execute_runs`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --unknown-flag` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## Current Decision / Blockers

| Item | Status |
|---|---|
| Backlog status | `In Validation` |
| Technical blocker | None currently open in this runbook |
| Promotion blocker | Owner promotion review pending |
| Safety note | Current evidence is green; archive or promotion should remain owner-confirmed |

## Milestone Snapshot

| Milestone | Packet | Result | Why it matters |
|---|---|---|---|
| A1 intake | `TestEvidence/bl020_slice_a1_contract_20260226T170007Z/status.tsv` | `PASS` | Contract approved and complete. |
| B1 lane intake | `TestEvidence/bl020_slice_b1_lane_20260226T172017Z` | `PASS` | Replay output was coherent; docs freshness debt was external. |
| N6 owner recheck | `TestEvidence/owner_sync_bl020_bl021_bl023_n6_20260226T172348Z/bl020_recheck/status.tsv` | `PASS` | BL-020 moved to `In Implementation`. |
| C1 native bridge | `TestEvidence/bl020_slice_c1_native_20260226T174052Z/status.tsv` | `PASS` then RT-blocked in owner review | Additive native bridge accepted, but promotion was blocked by RT findings at the time. |
| C3 re-verify | `TestEvidence/bl020_slice_c3_reverify_20260226T194955Z/` | `PASS` | C1 became green after RT reconciliation. |
| C4 parity | `TestEvidence/bl020_slice_c4_mode_parity_20260228T203021Z/status.tsv` | `PASS` | 20-run parity and exit semantics are green. |
| C4b non-interference | `TestEvidence/bl020_slice_c4b_mode_parity_20260228T202240Z/status.tsv` | `PASS` | Confirms C4 did not introduce interference. |

## Replay Cadence

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | T1 | 3 | validation matrix + replay summary |
| Candidate intake | T2 | 5 | contract/execute artifacts + taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ | explicit parity/sentinel artifacts |

Cost and flake policy:
- Diagnose the failing run index before repeating a full sweep.
- Heavy wrappers use targeted reruns, candidate at 2 runs, and promotion at 3 runs unless the owner asks for more.
- Record cadence overrides in `lane_notes.md` or `owner_decisions.md`.

## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment

Canonical lifecycle and evidence rules live in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

## TODOs

- [x] Define overlay input contract, degradation policy, and failure taxonomy.
- [x] Capture the current active replay gates and evidence pointers.
- [x] Replace the long replay history with a compact milestone snapshot.
- [ ] Record the owner promotion decision when it is made.

## Archive Note

The full packet-by-packet replay diary was removed from the active runbook on purpose. If the deep chronology is needed again, use the legacy copy preserved under `Documentation/archive/2026-03-18-doc-surface-consolidation/`.
