Title: BL-020 Confidence Masking Overlay
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-28

# BL-020 Confidence/Masking Overlay

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-020 |
| Priority | P2 |
| Status | In Implementation (Slice B1 lane contract replay-stable; C1 additive native bridge intake integrated; C3 post-C2 reverify is green end-to-end with `non_allowlisted=0`) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-014 (Done), BL-019 (Done) |
| Blocks | none |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| Slice A1 Type | Docs only |

## Objective

Define a deterministic contract for confidence/masking overlays, including field-level input rules, deterministic rendering/degradation behavior, acceptance thresholds, and QA artifact schema.

## Slice A1 Contract Authority

Slice A1 is documentation-only and defines the normative contract for later implementation/validation slices.

### 1) Overlay Input Contract

Per-emitter input object keys and rules:

| Field | Type | Valid Range / Enum | Required | Fallback Behavior |
|---|---|---|---|---|
| `snapshotSeq` | uint64 | monotonic non-decreasing | yes | `0`, mark `BL020-FX-006` |
| `emitterId` | uint | `0..255` | yes | row invalid, mark `BL020-FX-001` |
| `distanceConfidence` | float | finite `[0.0,1.0]` | yes | clamp into range, mark `BL020-FX-002` |
| `occlusionProbability` | float | finite `[0.0,1.0]` | yes | clamp into range, mark `BL020-FX-002` |
| `hrtfMatchQuality` | float | finite `[0.0,1.0]` | yes | clamp into range, mark `BL020-FX-002` |
| `maskingIndex` | float | finite `[0.0,1.0]` | yes | default `1.0`, mark `BL020-FX-005` |
| `combinedConfidence` | float | finite `[0.0,1.0]` | yes | recompute deterministic formula, mark `BL020-FX-003` |
| `overlayAlpha` | float | finite `[0.0,1.0]` | no | default `0.0` |
| `overlayBucket` | enum | `low|mid|high` | no | recompute from thresholds |
| `fallbackReason` | string | deterministic token | no | set explicit token when fallback path used |

Deterministic formula (authoritative):

`combinedConfidence = 0.40*distanceConfidence + 0.30*(1.0-occlusionProbability) + 0.20*hrtfMatchQuality + 0.10*(1.0-maskingIndex)`

Formula tolerance: absolute error `<= 0.01`.

Bucket thresholds:
- `low`: `< 0.40`
- `mid`: `>= 0.40` and `< 0.80`
- `high`: `>= 0.80`

### 2) Deterministic Rendering Expectations

| Contract ID | Expectation | Pass Rule |
|---|---|---|
| BL020-RD-001 | Overlay color/bucket mapping is deterministic for same inputs | 100% same bucket/color class across replay |
| BL020-RD-002 | Overlay alpha is bounded and finite | alpha always in `[0,1]` |
| BL020-RD-003 | Missing optional fields do not break render path | no throw/no hard fail; fallback token present |
| BL020-RD-004 | Required-field violations are surfaced as deterministic contract failures | taxonomy IDs emitted |

### 3) Degradation Policy

When confidence/masking payload is incomplete or invalid:
- Preserve base emitter rendering.
- Disable only the overlay layer for impacted emitter row.
- Emit deterministic fallback reason token.
- Record taxonomy classification for acceptance accounting.

Degradation priority order:
1. `schema_missing_required_field`
2. `value_out_of_range_or_non_finite`
3. `combined_confidence_formula_mismatch`
4. `overlay_bucket_mismatch`

### 4) Acceptance IDs and Thresholds

| Acceptance ID | Gate | Pass Threshold |
|---|---|---|
| BL020-A1-001 | Required field/type validity | 100% active rows valid |
| BL020-A1-002 | Numeric range + finiteness | 0 pre-clamp violations |
| BL020-A1-003 | Combined formula conformance | max abs delta `<= 0.01` |
| BL020-A1-004 | Bucket mapping determinism | 100% row match |
| BL020-A1-005 | Fallback token determinism | 100% fallback rows tokenized |
| BL020-A1-006 | Snapshot sequence monotonicity | 0 regressions |
| BL020-A1-007 | QA artifact schema completeness | all required artifacts + columns present |

### 5) QA Artifact Schema

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

### 6) Failure Taxonomy

| Failure ID | Category | Trigger | Classification | Blocking |
|---|---|---|---|---|
| BL020-FX-001 | schema_missing_required_field | missing required key/type mismatch | deterministic_contract_failure | yes |
| BL020-FX-002 | value_out_of_range_or_non_finite | NaN/Inf or outside `[0,1]` | deterministic_contract_failure | yes |
| BL020-FX-003 | combined_confidence_formula_mismatch | abs delta `> 0.01` | deterministic_contract_failure | yes |
| BL020-FX-004 | overlay_bucket_mismatch | computed bucket differs from threshold rule | deterministic_contract_failure | yes |
| BL020-FX-005 | fallback_reason_missing_or_invalid | fallback path lacks valid reason token | deterministic_contract_failure | yes |
| BL020-FX-006 | snapshot_sequence_non_monotonic | `snapshotSeq` decreases | deterministic_contract_failure | yes |
| BL020-FX-007 | artifact_schema_incomplete | required artifact/columns missing | deterministic_evidence_failure | yes |

## TODOs (Slice A1)

- [x] Define overlay input contract fields/types/ranges/fallback.
- [x] Define deterministic rendering expectations and degradation policy.
- [x] Define acceptance IDs and pass/fail thresholds.
- [x] Define QA artifact schema and failure taxonomy.
- [x] Validate docs freshness and capture evidence.

## Owner Sync N4 Intake (2026-02-26)

- Owner-authoritative intake packet: `TestEvidence/bl020_slice_a1_contract_20260226T170007Z/status.tsv`
- Gate summary:
  - contract spec: `PASS`
  - acceptance matrix: `PASS`
  - failure taxonomy: `PASS`
  - docs freshness: `PASS`
- Owner classification:
  - Slice A1 is accepted and complete.
  - Backlog posture is set to `In Planning` pending implementation slices.

## Slice B1 QA Lane Intake (2026-02-26)

- Worker packet directory: `TestEvidence/bl020_slice_b1_lane_20260226T172017Z`
- Validation summary:
  - lane lint/help: `PASS`
  - contract-only replay (`runs=3`): `PASS`
  - docs freshness: `FAIL` (external metadata debt outside B1 ownership)
- Owner interpretation:
  - B1 lane outputs are coherent and replay-stable.
  - Contract artifacts are complete (`status.tsv`, `validation_matrix.tsv`, `replay_hashes.tsv`, `failure_taxonomy.tsv`).

## Owner Sync N6 Intake (2026-02-26)

- Owner recheck bundle: `TestEvidence/owner_sync_bl020_bl021_bl023_bl030_n6_20260226T172348Z/bl020_recheck/status.tsv`
- Recheck result:
  - `./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 3`: `PASS`
- Owner decision:
  - BL-020 advances to `In Implementation`.
  - External docs-freshness blocker is tracked at owner sync level and is not a BL-020 contract failure.

## Slice C1 Native Contract Bridge (2026-02-26)

Objective: publish an additive native confidence/masking payload block for downstream UI/lane consumers without changing legacy payload contracts.

Native contract schema:
- `schema`: `locusq-confidence-masking-contract-v1`
- `snapshotSeq`: monotonic publish sequence from native process-block publication
- `distanceConfidence`: finite scalar, clamped `[0,1]`
- `occlusionProbability`: finite scalar, clamped `[0,1]`
- `hrtfMatchQuality`: finite scalar, clamped `[0,1]`
- `maskingIndex`: finite scalar, clamped `[0,1]`
- `combinedConfidence`: deterministic formula result, clamped `[0,1]`
- `overlayAlpha`: finite scalar, clamped `[0,1]`
- `overlayBucket`: `low|mid|high` from deterministic thresholds (`<0.40`, `<0.80`, otherwise `high`)
- `fallbackReason`: `none|inactive_mode|profile_mismatch|calibration_chain_fallback|non_finite_input`
- `valid`: bool (`true` while renderer-mode payload is actively published)

Deterministic formula:
- `combinedConfidence = 0.40*distanceConfidence + 0.30*(1.0-occlusionProbability) + 0.20*hrtfMatchQuality + 0.10*(1.0-maskingIndex)`

Publication guarantees:
- Additive-only: no existing BL-009/BL-033/BL-034 fields or semantics are removed/renamed.
- Process-block publication path is lock-free and allocation-free (atomic stores only).
- All published confidence/masking scalars are finite-only and clamped before publication.

Validation and evidence contract:
- Build + smoke + RT safety + docs freshness logs are required under `TestEvidence/bl020_slice_c1_native_<timestamp>/`.
- Required evidence artifacts:
  - `status.tsv`
  - `build.log`
  - `qa_smoke.log`
  - `rt_audit.tsv`
  - `diagnostics_snapshot.json`
  - `contract_delta.md`
  - `docs_freshness.log`

## Owner Sync N9 Intake (2026-02-26)

- Owner packet directory: `TestEvidence/owner_sync_bl030_bl020_bl023_n9_20260226T192237Z`
- Intake references:
  - Worker C1 packet: `TestEvidence/bl020_slice_c1_native_20260226T174052Z/status.tsv`
  - Owner RT replay: `TestEvidence/owner_sync_bl030_bl020_bl023_n9_20260226T192237Z/rt_audit.tsv`
- Owner replay summary:
  - build: `PASS`
  - smoke: `PASS`
  - docs freshness: `PASS`
  - RT audit: `FAIL` (`non_allowlisted=85`)
- Owner decision:
  - BL-020 remains `In Implementation`.
  - C1 native bridge intake is accepted as additive implementation progress.
  - Promotion beyond implementation is blocked by active RT gate findings outside this runbook lane ownership.

## Slice C3 Re-verify After RT Reconcile (2026-02-26)

- Worker packet directory: `TestEvidence/bl020_slice_c3_reverify_20260226T194955Z`
- Input linkage:
  - `TestEvidence/bl020_slice_c1_native_20260226T174052Z/*`
  - `TestEvidence/bl020_rt_gate_c2_20260226T193025Z/*`
- Re-verify command outcomes:
  - build (`cmake --build ...`): `PASS`
  - smoke (`locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`): `PASS`
  - RT audit (`./scripts/rt-safety-audit.sh --print-summary ...`): `PASS` (`non_allowlisted=0`)
  - docs freshness: `PASS`
- C3 conclusion:
  - BL-020 C1 is now green end-to-end at this branch snapshot after C2 RT reconciliation.
  - Packet is owner-consumable for promotion posture review.

## Slice C4 Execute-Mode Parity + Exit Guard (2026-02-28)

Objective:
- Prove deterministic parity between `--contract-only` and `--execute-suite` lane modes at 20-run depth.
- Enforce strict usage-exit semantics for invalid invocations (`exit=2`).
- Keep C4 deterministic by treating `--execute-suite` as execute-mode parity contract checks (runtime suite execution is reserved).

Acceptance matrix:

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


## Validation Plan (C4)

- `bash -n scripts/qa-bl020-confidence-masking-lane-mac.sh`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --help`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/contract_runs`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --execute-suite --runs 20 --out-dir TestEvidence/bl020_slice_c4_mode_parity_<timestamp>/execute_runs`
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl020-confidence-masking-lane-mac.sh --unknown-flag` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (C4)

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

## Slice C4 Done-Candidate Recheck (2026-02-28)

- Worker packet: `TestEvidence/bl020_slice_c4_mode_parity_20260228T170633Z`
- Contract-only replay: `PASS` (20 runs, deterministic replay hash drift count `0`)
- Execute-suite replay: `PASS` (20 runs, deterministic replay hash drift count `0`)
- Cross-mode doc hash mismatch count: `0`
- Cross-mode scenario hash mismatch count: `0`
- Contract failure rows: `0`
- Execute failure rows: `0`
- Exit-probe checks:
  - `--runs 0`: `2` (`PASS`)
  - `--unknown-flag`: `2` (`PASS`)
- Docs freshness: `PASS`
- Packet status: `PASS` (lane result `PASS`; canonical signatures differ by mode by design)

## Latest C4 Done-Candidate Recheck Packet (2026-02-28)

- Worker packet: `TestEvidence/bl020_slice_c4_mode_parity_20260228T175923Z`
- Contract-only replay: `PASS` (20 runs, deterministic replay hash drift count `0`)
- Execute-suite replay: `PASS` (20 runs, deterministic replay hash drift count `0`)
- Cross-mode doc hash mismatch count: `0`
- Cross-mode scenario hash mismatch count: `0`
- Contract failure rows: `0`
- Execute failure rows: `0`
- Exit-probe checks:
  - `--runs 0`: `2` (`PASS`)
  - `--unknown-flag`: `2` (`PASS`)
- Docs freshness: `PASS`
- Packet status: `PASS`

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
