Title: BL-023 Resize/DPI Hardening
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-03-02

# BL-023: Resize/DPI Hardening

## Plain-Language Summary

This runbook tracks **BL-023** (BL-023: Resize/DPI Hardening). Current status: **In Implementation (C3 mode parity revalidation PASS at TestEvidence/bl023_slice_c3_mode_parity_20260228T174901Z; runtime lane preserved deterministic outputs with one bounded exit=143 recovery event)**. In plain terms: Define and enforce a deterministic resize and DPI behavior contract for LocusQ WebView UI across standalone and plugin-host windows, with explicit pass/fail taxonomy and reproducible host scenarios.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-023: Resize/DPI Hardening |
| Why is this important? | Define and enforce a deterministic resize and DPI behavior contract for LocusQ WebView UI across standalone and plugin-host windows, with explicit pass/fail taxonomy and reproducible host scenarios. |
| How will we deliver it? | Use the validation plan and evidence bundle contract in this runbook to prove behavior and safety before promotion. |
| When is it done? | This item is complete when required replay gates pass and owner promotion packet decisions are recorded without blockers. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-023-resize-dpi-hardening.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Implementation (C3 mode parity revalidation PASS at `TestEvidence/bl023_slice_c3_mode_parity_20260228T174901Z`; runtime lane preserved deterministic outputs with one bounded `exit=143` recovery event) |
| Owner Track | Track C - UX Authoring |
| Depends On | BL-025 (Done) |
| Blocks | BL-030 RL-03 regression visibility when UI resize behavior is unstable |
| Annex Spec | `[Documentation/testing/bl-023-resize-dpi-hardening-qa.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/testing/bl-023-resize-dpi-hardening-qa.md)` |
| Default Replay Tier | T1 (dev-loop deterministic replay) |
| Heavy Lane Budget | High-cost wrapper (follow global heavy-wrapper containment for replay tiers) |

## Objective

Define and enforce a deterministic resize and DPI behavior contract for LocusQ WebView UI across standalone and plugin-host windows, with explicit pass/fail taxonomy and reproducible host scenarios.

## Slice A1 Contract (Docs-Only)

Slice A1 is the contract authority for resize and DPI behavior and does not change source/runtime code.

### 1) Breakpoint and Bounds Contract

| Contract ID | Rule | Pass Criteria |
|---|---|---|
| BL023-BP-001 | Compact breakpoint | Viewport width `< 960px` uses compact layout without clipping or overlap. |
| BL023-BP-002 | Standard breakpoint | Viewport width `960px..1439px` preserves full control rail visibility and stable hit-target mapping. |
| BL023-BP-003 | Wide breakpoint | Viewport width `>= 1440px` preserves spacing hierarchy and no stretched/invalid control geometry. |
| BL023-BD-001 | Minimum usable bounds | UI remains functional at `800x600` (no blocked primary action path). |
| BL023-BD-002 | Maximum reference bounds | UI remains visually stable at `2560x1440` reference window. |

### 2) DPI Scaling Contract

| Contract ID | Rule | Pass Criteria |
|---|---|---|
| BL023-DPI-001 | Supported scale factors | Validate `1.0`, `1.5`, and `2.0` scale contracts where host/display path supports them. |
| BL023-DPI-002 | Pixel-ratio alignment | Effective render pixel ratio delta vs expected scale is `<= 0.05`. |
| BL023-DPI-003 | Hit-target consistency | Pointer hit-target map remains aligned across scale changes and monitor moves. |
| BL023-DPI-004 | Text/control clipping | No clipped labels or controls caused by scale transitions. |

### 3) Resize Cadence Contract

| Contract ID | Rule | Pass Criteria |
|---|---|---|
| BL023-CD-001 | Resize event cadence | Layout update cadence is bounded to display frame cadence (no unbounded thrash). |
| BL023-CD-002 | Settle window | UI settles to stable layout state within `<= 250ms` after final resize event. |
| BL023-CD-003 | No stale mapping | Hit-target map refresh is completed before interaction checks are evaluated. |

### 4) Host Integration Regression Matrix Contract

Authoritative matrix definition is maintained in [bl-023-resize-dpi-hardening-qa.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/testing/bl-023-resize-dpi-hardening-qa.md). Required host lanes:

- Standalone (LocusQ app)
- REAPER (VST3)
- Logic Pro (AU)
- Ableton Live (VST3)

Each lane must record outcome per breakpoint and DPI scenario with deterministic taxonomy IDs.

### 5) Failure Taxonomy Contract

| Taxonomy ID | Failure Class | Definition |
|---|---|---|
| BL023-RZ-001 | layout_overflow | Scroll/overflow or element overlap appears in a contract viewport. |
| BL023-RZ-002 | stale_hit_target_map | Click/touch target map does not match visual control position after resize/DPI change. |
| BL023-RZ-003 | pixel_ratio_mismatch | Effective render ratio diverges from expected scale beyond tolerance. |
| BL023-RZ-004 | clipped_controls | Controls or labels are clipped at contract bounds/scale. |
| BL023-RZ-900 | harness_gate_failure | Deterministic gate/schema/runtime failure outside BL023-RZ-001..004. |
| BL023-RZ-910 | deterministic_replay_signature_divergence | Multi-run signature mismatch versus baseline replay run. |
| BL023-RZ-911 | deterministic_replay_row_count_drift | Multi-run host matrix row-count mismatch versus baseline replay run. |

## Planned Slices

| Slice | Scope | Type | Exit Gate |
|---|---|---|---|
| A1 | Contract definition + QA matrix + taxonomy | Docs only | `validate-docs-freshness` PASS |
| B1 | Runtime resize/DPI diagnostics panel | Code | node/build/selftest regression gates PASS |
| C1 | Host matrix wrapper harness (`qa-bl023-resize-dpi-matrix-mac.sh`) | Script/docs | Contract replay + deterministic summary + freshness gate |
| C2 | Matrix soak hardening (`--runs` replay determinism + fixed taxonomy ordering) | Script/docs | Contract replay + runtime replay + freshness gate |
| A2 | Runtime/UI implementation hardening | Code | Required BL-023 lanes PASS |
| A3 | Soak and promotion packet | Validation/docs | Promotion decision packet complete |

## A2 Execution Steps (Runtime/UI Implementation Hardening)

| Step ID | Workstream | Action | Primary File Targets | Completion Signal |
|---|---|---|---|---|
| A2-01 | Breakpoint/bounds hardening | Enforce deterministic compact/standard/wide breakpoints and minimum/maximum bounds behavior per `BL023-BP-*` and `BL023-BD-*`. | `Source/ui/public/js/index.js`, `Source/ui/public/js/*layout*` | No overflow/clipping regressions in BL023 host matrix lanes. |
| A2-02 | DPI scale conformance | Normalize DPI/scale handling for `1.0/1.5/2.0` with ratio tolerance `<= 0.05` per `BL023-DPI-*`. | `Source/ui/public/js/index.js`, `Source/ui/public/js/*dpi*` | Pixel ratio checks remain within tolerance in matrix evidence. |
| A2-03 | Resize cadence/settle hardening | Bound resize event cadence and settle window (`<= 250ms`) per `BL023-CD-001..003`. | `Source/ui/public/js/index.js`, resize/debounce helpers | Settle-latency check remains PASS in lane outputs. |
| A2-04 | Hit-target synchronization | Guarantee hit-target map refresh completes before interaction checks after resize/DPI transitions. | `Source/ui/public/js/index.js`, hit-test mapping logic | No `BL023-RZ-002` taxonomy hits in runtime matrix replay. |
| A2-05 | Host matrix runtime verification | Validate HM-001..HM-004 behavior in runtime matrix mode with deterministic replay counters. | `scripts/qa-bl023-resize-dpi-matrix-mac.sh`, `Documentation/testing/bl-023-resize-dpi-hardening-qa.md` | Runtime `--runs` lane exits `0` with zero signature/row drift. |
| A2-06 | Gate/exit semantics lock-in | Preserve strict usage exit semantics (`0/1/2`) while runtime hardening lands. | `scripts/qa-bl023-resize-dpi-matrix-mac.sh` | `--runs 0` and unknown-arg probes consistently return exit `2`. |
| A2-07 | Evidence + handoff prep | Capture canonical A2 evidence packet and owner-intake summary with ownership marker. | `TestEvidence/bl023_slice_a2_*`, runbook snapshots | Owner intake packet complete; `SHARED_FILES_TOUCHED` explicitly reported. |

### A2 Completion Gate

A2 is complete when:
- required BL-023 contract + runtime lanes pass at the requested cadence tier;
- no blocking BL023-RZ taxonomy failures remain in the canonical packet;
- docs/evidence are synchronized for owner intake.

## Lifecycle Contract Alignment

- Intake lane follows `Documentation/backlog/_template-intake.md`.
- Promotion decisions are captured using `Documentation/backlog/_template-promotion-decision.md`.
- Done transition must follow `Documentation/backlog/_template-closeout.md` and move the runbook to `Documentation/backlog/done/` in the same change set.
- Canonical promotion evidence stays under `TestEvidence/` (never `/tmp`-only packets).

## Validation (A1)

| Lane | Command | Expected |
|---|---|---|
| BL-023-docs-freshness | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 3 --out-dir TestEvidence/<packet>/contract_runs` | `validation_matrix.tsv`, `determinism_summary.tsv` |
| Candidate intake | T2 | 5 (or heavy-wrapper 2-run cap) | `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs <N> ...` then `--runs <N> ...` | `contract_runs/*`, `exec_runs/*`, taxonomy summary |
| Promotion | T3 | 10 (or owner-approved heavy-wrapper 3-run equivalent) | owner-selected contract+execute replay command set | owner packet + replay evidence |
| Sentinel | T4 | 20 (explicit only) | C3 mode parity drill (`--contract-only --runs 20`, `--runs 20`) | parity and sentinel artifacts |

### Cost/Flake Policy

- For replay failures, diagnose the failing run index before repeating full sweeps.
- Heavy wrappers (`>=20` binary launches per wrapper run) must use targeted debug reruns, with candidate at 2 runs and promotion at 3 runs unless owner requests broader coverage.
- Any cadence override must be documented in `lane_notes.md` or `owner_decisions.md` with rationale.

## Slice C1/C2 Harness Contract

- Script: `./scripts/qa-bl023-resize-dpi-matrix-mac.sh`
- Supported flags: `--runs <N>`, `--out-dir <path>`, `--contract-only`
- Required machine-readable outputs:
  - `status.tsv`
  - `validation_matrix.tsv`
  - `host_matrix_results.tsv`
  - `failure_taxonomy.tsv`
  - `determinism_summary.tsv`
- Exit semantics:
  - `0`: PASS
  - `1`: gate FAIL
  - `2`: usage error

### C2 Determinism Hardening Rules

- Single-run invocation remains backward-compatible (`--runs` omitted behaves as one run).
- For `--runs > 1`, replay output must remain deterministic:
  - `BL023-C1-DET-001`: stable signature rows across runs.
  - `BL023-C2-DET-002`: stable host matrix row-count parity across runs.
  - `BL023-C2-DET-003`: fixed taxonomy schema row-count parity.
- `failure_taxonomy.tsv` must keep fixed taxonomy row ordering to support deterministic parsing.

### C2 Required Evidence Packet

- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `exec_runs/validation_matrix.tsv`
- `exec_runs/host_matrix_results.tsv`
- `exec_runs/failure_taxonomy.tsv`
- `determinism_summary.tsv`
- `harness_notes.md`
- `docs_freshness.log`

## Evidence Contract (A1)

Evidence bundle path: `TestEvidence/bl023_slice_a1_contract_<timestamp>/`

Required artifacts:
- `status.tsv`
- `dpi_resize_contract.md`
- `host_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

## TODOs (A1)

- [x] Define deterministic breakpoint and bounds contract.
- [x] Define DPI scaling and cadence expectations.
- [x] Define host regression matrix for standalone + plugin hosts.
- [x] Define resize/DPI failure taxonomy.
- [x] Record evidence bundle and docs-freshness result.

## Owner Sync N4 Intake (2026-02-26)

- Owner-authoritative intake packet: `TestEvidence/bl023_slice_a1_contract_20260226T165723Z/status.tsv`
- Gate summary:
  - resize/DPI contract: `PASS`
  - host matrix: `PASS`
  - taxonomy table: `PASS`
  - docs freshness: `PASS`
- Owner classification:
  - Slice A1 is accepted and complete.
  - Backlog posture is set to `In Planning` pending implementation slices.

## Slice B1 UI Diagnostics Intake (2026-02-26)

- Worker packet directory: `TestEvidence/bl023_slice_b1_ui_20260226T172047Z`
- Validation summary:
  - `node --check`: `PASS`
  - standalone build: `PASS`
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029` x3: `PASS`
  - docs freshness: `FAIL` (external metadata debt outside B1 ownership)
- Owner interpretation:
  - B1 implementation goals are met (diagnostic card contract and regression checks green).
  - Failure classification is repository-level docs freshness debt, not BL-023 runtime regression.

## Owner Sync N6 Intake (2026-02-26)

- Owner recheck command:
  - `node --check Source/ui/public/js/index.js`: `PASS`
- Owner decision:
  - BL-023 advances to `In Implementation`.
  - External docs-freshness blocker is tracked at owner sync level and is not a BL-023 functional blocker.

## Owner Sync N9 Intake (2026-02-26)

- Owner packet directory: `TestEvidence/owner_sync_bl030_bl020_bl023_n9_20260226T192237Z`
- Intake references:
  - Worker C1 packet: `TestEvidence/bl023_slice_c1_matrix_20260226T173722Z/status.tsv`
  - Owner recheck: `TestEvidence/owner_sync_bl030_bl020_bl023_n9_20260226T192237Z/bl023_recheck/status.tsv`
- Owner replay summary:
  - `--contract-only --runs 3`: `PASS`
  - Determinism signatures: stable across all three runs
  - docs freshness: `PASS`
- Owner decision:
  - BL-023 remains `In Implementation`.
  - C1 matrix harness intake is accepted as deterministic and contract-complete for implementation posture.

## Slice C2 Soak Intake (2026-02-26)

- Worker packet directory: `TestEvidence/bl023_slice_c2_soak_20260226T195042Z`
- Validation summary:
  - contract-only soak (`runs=5`): `PASS`
  - execute-suite replay (`runs=3`): `PASS`
  - docs freshness: `PASS`
- Owner interpretation:
  - C2 soak determinism hardening outputs are coherent and stable.
  - C2 intake is accepted for implementation confidence hardening.

## Owner Sync N13 Intake (2026-02-26)

- Owner packet directory: `TestEvidence/owner_sync_bl030_bl021_bl023_n13_20260226T203010Z`
- Owner recheck command:
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 3 --out-dir .../bl023_recheck`: `PASS`
- Determinism summary:
  - signature divergence count: `0`
  - row count drift: `0`
- Owner decision:
  - BL-023 remains `In Implementation`.
  - Deterministic confidence is reinforced by fresh owner-authoritative replay.
- Note:
  - Requested C3 sentinel packet path was not present; owner used latest available C2 soak packet plus fresh N13 recheck.

## Slice C3 Mode Parity + Exit Semantics Revalidation (2026-02-28)

Objective:
- Revalidate deterministic replay across contract-only and runtime matrix modes at 20-run depth.
- Enforce strict usage-exit semantics for invalid invocations.

Acceptance matrix:

| acceptance_id | gate | threshold |
|---|---|---|
| BL023-C3-001 | Contract-only determinism replay | `--contract-only --runs 20` exits `0` with deterministic summary PASS (`signature_divergence_count=0`, `row_count_drift=0`) |
| BL023-C3-002 | Runtime matrix determinism replay | `--runs 20` exits `0` with deterministic summary PASS (`signature_divergence_count=0`, `row_count_drift=0`) |
| BL023-C3-003 | Cross-mode parity | contract and runtime summaries report equal deterministic counters and zero taxonomy drift rows |
| BL023-C3-004 | Contract run schema completeness | `contract_runs` includes `validation_matrix.tsv`, `host_matrix_results.tsv`, `failure_taxonomy.tsv`, `determinism_summary.tsv` |
| BL023-C3-005 | Runtime run schema completeness | `exec_runs` includes `validation_matrix.tsv`, `host_matrix_results.tsv`, `failure_taxonomy.tsv`, `determinism_summary.tsv` |
| BL023-C3-006 | Usage exit semantics | `--runs 0` and `--unknown` both return exit `2` |
| BL023-C3-007 | Docs freshness gate | `./scripts/validate-docs-freshness.sh` exits `0` |

Failure taxonomy additions:

| failure_id | category | trigger | classification | blocking |
|---|---|---|---|---|
| BL023-RZ-920 | mode_parity_mismatch | cross-mode deterministic counters differ or non-zero drift appears | deterministic_replay_failure | yes |
| BL023-RZ-921 | usage_exit_semantics_failure | invalid usage probes return non-`2` exit code | deterministic_contract_failure | yes |
| BL023-RZ-922 | c3_evidence_schema_incomplete | required C3 packet files missing | deterministic_evidence_failure | yes |
| BL023-RZ-923 | docs_freshness_failure | docs freshness gate exits non-zero | governance_failure | yes |

## Validation Plan (C3)

- `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --help`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl023_slice_c3_mode_parity_<timestamp>/contract_runs`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 20 --out-dir TestEvidence/bl023_slice_c3_mode_parity_<timestamp>/exec_runs`
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0` (expect exit `2`)
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown` (expect exit `2`)
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract (C3)

Required files under `TestEvidence/bl023_slice_c3_mode_parity_<timestamp>/`:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `contract_runs/host_matrix_results.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `contract_runs/determinism_summary.tsv`
- `exec_runs/validation_matrix.tsv`
- `exec_runs/host_matrix_results.tsv`
- `exec_runs/failure_taxonomy.tsv`
- `exec_runs/determinism_summary.tsv`
- `mode_parity.tsv`
- `exit_semantics_probe.tsv`
- `lane_notes.md`
- `docs_freshness.log`

## Slice C3 Execution Snapshot (2026-02-28)

- Canonical packet:
  - `TestEvidence/bl023_slice_c3_mode_parity_20260228T180543Z`
- Validation outcomes:
  - `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh` => `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --help` => `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 20 --out-dir .../contract_runs` => `PASS`
  - `LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143=1 LOCUSQ_BL023_RUNTIME_RETRY_LIMIT=2 LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS=1 LOCUSQ_BL023_SELFTEST_MAX_ATTEMPTS=6 LOCUSQ_BL023_SELFTEST_RETRY_DELAY_SECONDS=4 LOCUSQ_BL023_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS=8 ./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 20 --out-dir .../exec_runs` => `PASS`
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0` => `PASS` (exit `2`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown` => `PASS` (exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Determinism summary:
  - `signature_divergence_count`: contract `0`, runtime `0`
  - `row_count_drift`: contract `0`, runtime `0`
  - `runtime_failure_count`: contract `0`, runtime `0`
  - `taxonomy_row_count`: contract `7`, runtime `7`
- Verdict:
  - C3 mode parity and exit semantics gates are green; packet is owner-intake ready.

## Handoff Return Contract

All worker and owner handoffs for this runbook must include:
- `SHARED_FILES_TOUCHED: no|yes`

Required return block:
```
HANDOFF_READY
TASK: BL-023 Resize/DPI Hardening
RESULT: PASS|FAIL
FILES_TOUCHED: ...
VALIDATION: ...
ARTIFACTS: ...
SHARED_FILES_TOUCHED: no|yes
BLOCKERS: ...
```

## Slice C3 Worker Revalidation Intake (2026-02-28)

- Worker packet directory: `TestEvidence/bl023_slice_c3_mode_parity_20260228T171824Z`
- Validation summary:
  - `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh`: `PASS`
  - `--help`: `PASS` (exit `0`)
  - `--contract-only --runs 20`: `PASS`
  - `--runs 20`: `FAIL` (`run=7`, `runtime_lane_fail:exit=143`)
  - `--runs 0` / `--unknown`: `PASS` (both exit `2`)
  - docs freshness: `PASS`
- Determinism counters:
  - contract lane: `signature_divergence_count=0`, `row_count_drift=0`, `runtime_failure_count=0`
  - runtime lane: `signature_divergence_count=1`, `row_count_drift=0`, `runtime_failure_count=1`
- Failure taxonomy snapshot (`exec_runs/failure_taxonomy.tsv`):
  - `BL023-RZ-900`: count `1`, first_run `7`, lane `BL023-HM-ALL`, detail `runtime_lane_fail`
  - `BL023-RZ-910`: count `1`, first_run `7`, lane `BL023-HM-ALL`, detail `determinism_signature_mismatch`
- Classification:
  - `BL023-C3-001`, `BL023-C3-004`, `BL023-C3-005`, `BL023-C3-006`, `BL023-C3-007`: `PASS`
  - `BL023-C3-002`, `BL023-C3-003`: `FAIL`
  - BL-023 remains `In Implementation` pending runtime replay stabilization.

## Slice C3 Worker Revalidation Intake R2 (2026-02-28)

- Worker packet directory: `TestEvidence/bl023_slice_c3_mode_parity_20260228T174901Z`
- Runtime hardening applied in lane wrapper:
  - bounded retry-on-`143` path in runtime mode only (`LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143`, `LOCUSQ_BL023_RUNTIME_RETRY_LIMIT`, `LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS`)
  - retry events are logged in per-run logs and surfaced in `validation_matrix.tsv` detail.
- Validation summary:
  - `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh`: `PASS`
  - `--help`: `PASS` (exit `0`)
  - `--contract-only --runs 20`: `PASS`
  - `--runs 20`: `PASS` (overall gate pass)
  - `--runs 0` / `--unknown`: `PASS` (both exit `2`)
  - docs freshness: `PASS`
- Determinism counters:
  - contract lane: `signature_divergence_count=0`, `row_count_drift=0`, `runtime_failure_count=0`
  - runtime lane: `signature_divergence_count=0`, `row_count_drift=0`, `runtime_failure_count=0`
  - runtime observability: `runtime_retry143_recovery_count=1` (non-gate metric; recovered at `run=8`)
- Failure taxonomy snapshot (`exec_runs/failure_taxonomy.tsv`):
  - `BL023-RZ-900`: count `0`
  - `BL023-RZ-910`: count `0`
  - `BL023-RZ-911`: count `0`
- Classification:
  - `BL023-C3-001`..`BL023-C3-007`: `PASS`
  - mode parity packet gate: `PASS`

## Slice C3 T4 Sentinel Addendum (2026-02-28)

- Sentinel packet directory: `TestEvidence/bl023_slice_c3_mode_parity_20260228T180258Z`
- Cadence policy posture:
  - executed as explicit `T4` long-run sentinel (user-requested), not as routine gating cadence.
  - canonical promotion/gating packet remains `TestEvidence/bl023_slice_c3_mode_parity_20260228T174901Z`.
- Sentinel lane outcome (`exec_runs_retry_on`):
  - command exit: `0`
  - `runtime_failure_count=0`
  - `runtime_retry143_recovery_count=2`
- Interpretation:
  - bounded retry path remained stable over extended replay depth.
  - no additional blocker taxonomy surfaced in the sentinel run.

## A2-01 Runtime/UI Hardening Snapshot (2026-02-28)

- A2-01 scope:
  - responsive breakpoint threshold alignment (`<960` compact/tight path),
  - dynamic per-resize DPR handling,
  - resize hit-target refresh observability in diagnostics.
- T1 replay packet:
  - `TestEvidence/bl023_slice_a2_t1_replay_20260228T200917Z`
- Validation outcomes:
  - `bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh` => `PASS` (`0`)
  - `./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs 3 --out-dir .../contract_runs` => `PASS` (`0`)
  - `LOCUSQ_BL023_RUNTIME_RETRY_ON_EXIT143=1 LOCUSQ_BL023_RUNTIME_RETRY_LIMIT=2 LOCUSQ_BL023_RUNTIME_RETRY_DELAY_SECONDS=1 LOCUSQ_BL023_SELFTEST_MAX_ATTEMPTS=6 LOCUSQ_BL023_SELFTEST_RETRY_DELAY_SECONDS=4 LOCUSQ_BL023_SELFTEST_RESULT_AFTER_EXIT_GRACE_SECONDS=8 ./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 3 --out-dir .../exec_runs` => `PASS` (`0`)
  - `./scripts/validate-docs-freshness.sh` => `PASS` (`0`)
- Interpretation:
  - A2 runtime/UI hardening entrypoint is green at T1 cadence with no immediate replay regressions.

## Governance Alignment (2026-02-28)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).
