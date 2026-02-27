Title: BL-023 Resize/DPI Hardening
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-26

# BL-023: Resize/DPI Hardening

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Implementation (C2 soak packet PASS; N13 owner recheck `--contract-only --runs 3` PASS with deterministic signatures and row-count parity; deterministic confidence reinforced) |
| Owner Track | Track C - UX Authoring |
| Depends On | BL-025 (Done) |
| Blocks | BL-030 RL-03 regression visibility when UI resize behavior is unstable |

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

## Validation (A1)

| Lane | Command | Expected |
|---|---|---|
| BL-023-docs-freshness | `./scripts/validate-docs-freshness.sh` | Exit 0 |

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
