Title: BL-023 Resize/DPI Hardening QA
Document Type: QA Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-023 Resize/DPI Hardening QA

## Purpose

This document defines the deterministic regression matrix and pass/fail rules for BL-023 resize and DPI behavior across standalone and plugin hosts.

## Matrix Dimensions

### Viewport Bounds

| ID | Width x Height | Category |
|---|---|---|
| VP-01 | 800x600 | minimum usable |
| VP-02 | 960x640 | compact breakpoint edge |
| VP-03 | 1280x800 | standard baseline |
| VP-04 | 1440x900 | wide breakpoint edge |
| VP-05 | 2560x1440 | max reference |

### DPI/Scale Targets

| ID | Scale | Notes |
|---|---|---|
| DPI-01 | 1.0 | standard display baseline |
| DPI-02 | 1.5 | mixed-scaling path where available |
| DPI-03 | 2.0 | Retina/high-density baseline |

### Host/Runtime Matrix

| Lane ID | Host Mode | Host | Plugin Format | WebView Backend | Required |
|---|---|---|---|---|---|
| BL023-HM-001 | standalone | LocusQ Standalone | n/a | WKWebView | yes |
| BL023-HM-002 | plugin | REAPER | VST3 | WKWebView | yes |
| BL023-HM-003 | plugin | Logic Pro | AU | WKWebView | yes |
| BL023-HM-004 | plugin | Ableton Live | VST3 | WKWebView | yes |

## Deterministic Checks

| Check ID | Check | Pass Criteria |
|---|---|---|
| BL023-CHK-001 | overflow/overlap | No layout overflow or overlapping controls at VP-01..VP-05. |
| BL023-CHK-002 | clipped controls | No clipped controls/labels at required viewport and DPI lanes. |
| BL023-CHK-003 | hit-target map | Interactive hit targets match rendered control positions after resize. |
| BL023-CHK-004 | pixel ratio | Effective ratio delta vs target scale is `<= 0.05`. |
| BL023-CHK-005 | settle latency | Layout stabilization after final resize event is `<= 250ms`. |

## UI Diagnostics Contract (Slice B1)

Renderer diagnostics panel must expose an additive `Resize / DPI` card (collapsed by default) with these fields:

| Field ID | Label | Type | Rule | Unknown Fallback |
|---|---|---|---|---|
| `rend-resize-viewport` | Viewport (W x H) | string | Derived from active viewport resize dimensions | `unknown` |
| `rend-resize-dpr` | Device Pixel Ratio | string | Finite `window.devicePixelRatio` formatted to 2 decimals | `unknown` |
| `rend-resize-bucket` | Layout Bucket | enum string | `compact|standard|wide` from active layout/width contract | `unknown` |
| `rend-resize-settle` | Last Resize Settle | string | Last bounded settle duration in milliseconds | `unknown` |

Supporting contract IDs:

| Contract ID | Rule | Pass Criteria |
|---|---|---|
| BL023-DIAG-001 | Panel default state | Card content hidden on boot (`aria-expanded=false`) |
| BL023-DIAG-002 | Additive behavior | Existing diagnostics cards and selftest IDs unchanged |
| BL023-DIAG-003 | Fallback safety | Missing/unavailable metrics render `unknown` and do not throw |
| BL023-DIAG-004 | Bounded settle | No unbounded polling; settle computed with bounded debounce window |
| BL023-DIAG-005 | Settle threshold | `rend-resize-settle` warning threshold aligns to BL023-CHK-005 (`>250ms` warns) |

## Resize Cadence Expectations

- Resize processing must remain bounded and deterministic; no unbounded layout recalculation loops.
- Final interaction checks are evaluated only after settle condition is reached.
- Cadence anomalies are recorded with a taxonomy ID and reproduction context.

## Failure Taxonomy

| Taxonomy ID | Failure Class | Trigger |
|---|---|---|
| BL023-RZ-001 | layout_overflow | Unexpected scrollbars, overlap, or viewport overflow appears. |
| BL023-RZ-002 | stale_hit_target_map | Click/touch result mismatches rendered position after resize/DPI update. |
| BL023-RZ-003 | pixel_ratio_mismatch | Ratio mismatch exceeds tolerance (`> 0.05`). |
| BL023-RZ-004 | clipped_controls | Controls/labels clipped at contract viewport/DPI lane. |
| BL023-RZ-900 | harness_gate_failure | Deterministic gate/schema/runtime failure outside RZ-001..RZ-004. |
| BL023-RZ-910 | deterministic_replay_signature_divergence | Multi-run signature mismatch vs baseline run. |
| BL023-RZ-911 | deterministic_replay_row_count_drift | Multi-run host matrix row-count mismatch vs baseline run. |

## Evidence Output Contract

For each BL-023 run, record:

- `host_matrix.tsv` rows with: `lane_id,host,mode,format,backend,viewport_id,dpi_id,check_id,result,taxonomy_id,notes`.
- Failure rows must include one taxonomy ID from BL023-RZ-001..004.
- `status.tsv` must summarize overall pass/fail and blocker IDs.

## C1 Host Matrix Harness Contract

Command:
- `./scripts/qa-bl023-resize-dpi-matrix-mac.sh`

Options:
- `--runs <N>`: deterministic replay count, integer `>= 1`
- `--out-dir <path>`: artifact output directory
- `--contract-only`: docs/contract replay without runtime host launch

Required outputs:
- `status.tsv`: lane-level pass/fail summary and exit details
- `validation_matrix.tsv`: run-level checks (`BL023-C1-RUN-001`, `BL023-C1-DET-001`)
- `host_matrix_results.tsv`: machine-readable host lane outcomes (`BL023-HM-001..004`)
- `failure_taxonomy.tsv`: aggregated failure taxonomy counts
- `determinism_summary.tsv`: deterministic replay counters and thresholds

Exit semantics:
- `0`: gate pass
- `1`: gate fail
- `2`: usage error

Deterministic replay rule:
- For `--runs N`, each run must emit a stable matrix signature (`BL023-C1-DET-001`).
- For `--runs N`, each run must preserve host matrix row-count parity (`BL023-C2-DET-002`).
- Signature mismatch is a deterministic failure and must be recorded in `failure_taxonomy.tsv`.
- Row-count mismatch is a deterministic failure and must be recorded in `failure_taxonomy.tsv`.

## C2 Soak Hardening Additions

Slice C2 requires deterministic multi-run hardening with:
1. Stable status/matrix/taxonomy outputs in contract-only and runtime replay modes.
2. Fixed-order taxonomy rows (including zero-count rows) for deterministic parsing.
3. Backward-compatible single-run invocation and unchanged `0/1/2` exit semantics.

### C2 Acceptance Mapping

| C2 ID | Lane Check | Pass Criteria | Artifact |
|---|---|---|---|
| `BL023-C2-001` | `BL023-C1-DET-001` | Replay signatures match baseline across all runs | `validation_matrix.tsv`, `determinism_summary.tsv` |
| `BL023-C2-002` | `BL023-C2-DET-002` | Host matrix row counts match baseline across all runs | `validation_matrix.tsv`, `determinism_summary.tsv` |
| `BL023-C2-003` | `BL023-C2-DET-003` | Taxonomy table row-count matches fixed contract set | `failure_taxonomy.tsv`, `determinism_summary.tsv` |
| `BL023-C2-004` | `overall` | Contract-only and runtime replay lanes preserve strict exit semantics (`0/1/2`) | `status.tsv` |

### C2 Evidence Contract

Required C2 packet artifacts:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/validation_matrix.tsv`
- `exec_runs/validation_matrix.tsv`
- `exec_runs/host_matrix_results.tsv`
- `exec_runs/failure_taxonomy.tsv`
- `determinism_summary.tsv`
- `harness_notes.md`
- `docs_freshness.log`

## Validation

- `./scripts/validate-docs-freshness.sh`

Validation state labels:
- `tested`: Command executed and exited as expected.
- `partially tested`: Some required lanes missing.
- `not tested`: Command not executed.
