Title: BL-023 Resize/DPI Hardening QA
Document Type: QA Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-03-18

# BL-023 Resize/DPI Hardening QA

## Purpose

Define the deterministic resize and DPI regression contract for BL-023.
This is a compact support surface.
The full execution chronology lives in the done runbook and archive.

## Contract Surface

Primary runbook authority:
- `Documentation/backlog/done/bl-023-resize-dpi-hardening.md`

Traceability anchors:
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`

Current posture:
- `Done`
- Support role: retained QA contract and evidence index
- Highest-signal evidence: `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z/`

## Core Resize/DPI Contract

### Viewport Bounds

| ID | Width x Height | Contract |
|---|---|---|
| VP-01 | 800x600 | minimum usable |
| VP-02 | 960x640 | compact breakpoint edge |
| VP-03 | 1280x800 | standard baseline |
| VP-04 | 1440x900 | wide breakpoint edge |
| VP-05 | 2560x1440 | max reference |

### DPI Targets

| ID | Scale | Contract |
|---|---|---|
| DPI-01 | 1.0 | standard display baseline |
| DPI-02 | 1.5 | mixed-scaling path where available |
| DPI-03 | 2.0 | Retina/high-density baseline |

### Host Matrix

| Lane ID | Host Mode | Host | Format | Backend |
|---|---|---|---|---|
| BL023-HM-001 | standalone | LocusQ Standalone | n/a | WKWebView |
| BL023-HM-002 | plugin | REAPER | VST3 | WKWebView |
| BL023-HM-003 | plugin | Logic Pro | AU | WKWebView |
| BL023-HM-004 | plugin | Ableton Live | VST3 | WKWebView |

### Deterministic Checks

| Check ID | Contract | Pass Condition |
|---|---|---|
| BL023-CHK-001 | overflow/overlap | no layout overflow or overlap at VP-01..VP-05 |
| BL023-CHK-002 | clipped controls | no clipped controls/labels at required viewport and DPI lanes |
| BL023-CHK-003 | hit-target map | interactive hit targets match rendered controls after resize |
| BL023-CHK-004 | pixel ratio | effective ratio delta vs target scale is `<= 0.05` |
| BL023-CHK-005 | settle latency | layout stabilization after final resize is `<= 250ms` |

### Diagnostics Card

Renderer diagnostics must expose a collapsed-by-default `Resize / DPI` card.

| Field ID | Label | Rule | Unknown |
|---|---|---|---|
| `rend-resize-viewport` | Viewport (W x H) | derived from active viewport resize dimensions | `unknown` |
| `rend-resize-dpr` | Device Pixel Ratio | finite `window.devicePixelRatio` to 2 decimals | `unknown` |
| `rend-resize-bucket` | Layout Bucket | `compact|standard|wide` from width contract | `unknown` |
| `rend-resize-settle` | Last Resize Settle | last bounded settle duration in ms | `unknown` |

## Active Validation Tiers

| Tier | Runs | Purpose | Evidence Shape |
|---|---:|---|---|
| A1 | docs-only | contract authority | `status.tsv`, `host_matrix.tsv`, `failure_taxonomy.tsv`, `docs_freshness.log` |
| B1 | 3 | UI diagnostics intake | `validation_matrix.tsv`, `host_matrix.tsv`, `harness_notes.md`, `docs_freshness.log` |
| C1/C2 | 3/5 | matrix harness + soak hardening | `validation_matrix.tsv`, `contract_runs/*`, `exec_runs/*`, `determinism_summary.tsv` |
| C3 | 20 | mode parity + exit semantics | `mode_parity.tsv`, `exit_semantics_probe.tsv`, `docs_freshness.log` |
| A2 | 3 | runtime/UI hardening | `validation_matrix.tsv`, `exec_runs/*`, `determinism_summary.tsv` |
| A3 | owner sync | promotion and done closeout | `promotion_decision.md`, `owner_decisions.md`, `handoff_resolution.md` |

## Required Evidence Shape

Common files across tiers:
- `status.tsv`
- `validation_matrix.tsv`
- `docs_freshness.log`

Tier-specific files:
- `host_matrix.tsv`
- `host_matrix_results.tsv`
- `failure_taxonomy.tsv`
- `determinism_summary.tsv`
- `mode_parity.tsv`
- `exit_semantics_probe.tsv`
- `harness_notes.md`
- `promotion_readiness.md`
- `promotion_decision.md`
- `owner_decisions.md`
- `handoff_resolution.md`

Required TSV columns:
- `host_matrix.tsv`: `lane_id,host,mode,format,backend,viewport_id,dpi_id,check_id,result,taxonomy_id,notes`
- `failure_taxonomy.tsv`: `taxonomy_id,category,trigger,class,blocking,detail`

## Validation Commands

```bash
./scripts/validate-docs-freshness.sh
bash -n scripts/qa-bl023-resize-dpi-matrix-mac.sh
./scripts/qa-bl023-resize-dpi-matrix-mac.sh --help
./scripts/qa-bl023-resize-dpi-matrix-mac.sh --contract-only --runs <N> --out-dir TestEvidence/bl023_slice_<tier>_<timestamp>/contract_runs
./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs <N> --out-dir TestEvidence/bl023_slice_<tier>_<timestamp>/exec_runs
./scripts/qa-bl023-resize-dpi-matrix-mac.sh --runs 0
./scripts/qa-bl023-resize-dpi-matrix-mac.sh --unknown
```

## Milestone Snapshot

| Tier | Date | Result | Why it matters | Evidence |
|---|---|---|---|---|
| A1 | 2026-02-26 | PASS | Contract, host matrix, and taxonomy were defined. | `TestEvidence/bl023_slice_a1_contract_20260226T165723Z/` |
| B1 | 2026-02-26 | PASS | Diagnostics card and baseline replay were stable. | `TestEvidence/bl023_slice_b1_ui_20260226T172047Z/` |
| C1 | 2026-02-26 | PASS | Runtime matrix intake was deterministic. | `TestEvidence/bl023_slice_c1_matrix_20260226T173722Z/` |
| C2 | 2026-02-26 | PASS | Soak hardening stayed deterministic. | `TestEvidence/bl023_slice_c2_soak_20260226T195042Z/` |
| C3 | 2026-02-28 | PASS | 20-run mode parity and exit semantics were green. | `TestEvidence/bl023_slice_c3_mode_parity_20260228T180543Z/` |
| A2 | 2026-02-28 | PASS | Runtime/UI hardening stayed green at T1 depth. | `TestEvidence/bl023_slice_a2_t1_replay_20260228T200917Z/` |
| A3 | 2026-02-28 | PASS | Promotion closeout was accepted. | `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z/` |

## Latest Evidence

| Item | Value |
|---|---|
| Highest-signal packet | `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z/` |
| Latest contract result | `PASS` |
| Latest docs freshness | `PASS` |
| Latest parity summary | contract and runtime counters were stable with zero drift |

## Notes

The detailed replay diary was intentionally removed from the active file.
Use the done runbook and the evidence packets above for deep chronology.
