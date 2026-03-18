Title: BL-048 Cross-Platform Shipping Hardening QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-048 Cross-Platform Shipping Hardening QA Contract

## Purpose

Define the deterministic QA contract for BL-048 cross-platform shipping hardening.
This file keeps the release matrix, signing gates, packaging evidence, and replay ladder short and auditable.

## Authority

- primary runbook: `Documentation/backlog/done/bl-048-cross-platform-shipping-hardening.md`
- traceability anchors: `.ideas/architecture.md`, `.ideas/parameter-spec.md`, `Documentation/invariants.md`
- lane harness: `scripts/qa-bl048-shipping-hardening-lane-mac.sh`

## Core Contract

| Area | Required rule |
|---|---|
| platform matrix | required columns must exist, active rows must be unique, and ordering must stay deterministic |
| signing gates | macOS signing, notarization/stapling, and Windows signing must each declare deterministic verification evidence |
| packaging evidence | `packaging_manifest.tsv` and `checksums.tsv` must stay machine-readable |
| parity | backlog, QA, and evidence IDs must stay aligned |

### Required evidence files

Every `TestEvidence/bl048_*_<timestamp>/` packet should include:
- `status.tsv`
- `validation_matrix.tsv`
- `failure_taxonomy.tsv`
- `docs_freshness.log`

Tier-specific adds:
- `platform_matrix.tsv`, `signing_verification.tsv`, `notarization_stapling.tsv`, `packaging_manifest.tsv`, `checksums.tsv` for contract or release-proof packets
- `contract_runs/replay_hashes.tsv` for contract-only replay tiers
- `contract_runs_contract/*`, `contract_runs_execute/*`, `mode_parity.tsv`, `drift_summary.tsv`, and `exit_semantics_probe.tsv` for parity tiers
- `promotion_readiness.md` and `ownership_safety_check.tsv` for final owner-ready packets

## Validation Tiers

| Tier | Runs | Purpose | Evidence shape |
|---|---:|---|---|
| A1 | docs-only | matrix, signing, and packaging contract authority | `acceptance_matrix.tsv`, `failure_taxonomy.tsv`, `docs_freshness.log` |
| B1 | 3 | lane bootstrap | `validation_matrix.tsv`, `contract_runs/*`, `lane_notes.md`, `docs_freshness.log` |
| C2 | 10 | contract soak | same as B1 |
| C3 | 20 | replay sentinel | `validation_matrix.tsv`, `contract_runs/*`, `replay_sentinel_summary.tsv`, `docs_freshness.log` |
| C4 | 50 | soak replay | same as C3 |
| C5 | 20 | exit semantics | C3 shape plus `exit_semantics_probe.tsv` |
| C6r | 20 | execute-mode parity | `contract_runs_contract/*`, `contract_runs_execute/*`, `mode_parity.tsv`, `exit_semantics_probe.tsv` |
| C7 | 50 | long-run parity | C6r shape plus `drift_summary.tsv` |
| D1 | 75 | done-candidate parity | C7 shape |
| D2 | 100 | done-promotion parity | C7 shape plus `promotion_readiness.md` |
| Z16b | 100 | owner-ready reconcile | D2 shape plus `ownership_safety_check.tsv`, `blocker_taxonomy.tsv` |

## Validation Commands

Current lane pattern:

```bash
bash -n scripts/qa-bl048-shipping-hardening-lane-mac.sh
./scripts/qa-bl048-shipping-hardening-lane-mac.sh --help
./scripts/qa-bl048-shipping-hardening-lane-mac.sh --contract-only --runs <N> --out-dir TestEvidence/bl048_<slice>_<timestamp>/contract_runs
./scripts/qa-bl048-shipping-hardening-lane-mac.sh --execute-suite --runs <N> --out-dir TestEvidence/bl048_<slice>_<timestamp>/contract_runs_execute
./scripts/qa-bl048-shipping-hardening-lane-mac.sh --runs 0
./scripts/qa-bl048-shipping-hardening-lane-mac.sh --unknown
./scripts/validate-docs-freshness.sh
```

Use only the commands that apply to the current tier.

## Current Signal

| Item | Status |
|---|---|
| Runbook authority | `Documentation/backlog/done/bl-048-cross-platform-shipping-hardening.md` |
| Current technical state | Done |
| Promotion state | Done |
| Highest-signal owner-ready packet | `TestEvidence/bl048_e2e_promotion_z16b_20260227T225426Z/` |
| Highest-signal done-promotion packet | `TestEvidence/bl048_slice_d2_done_promotion_20260227T222622Z/` |
| Latest docs freshness signal | PASS |

## Milestone Snapshot

| Milestone | Packet | Result | Why it matters |
|---|---|---|---|
| A1 contract | `TestEvidence/bl048_slice_a1_contract_20260227T203929Z/` | PASS | Shipping matrix, signing gates, and evidence schema were defined. |
| B1 lane | `TestEvidence/bl048_slice_b1_lane_20260227T212253Z/` | PASS | Bootstrap replay and schema held. |
| C2 soak | `TestEvidence/bl048_slice_c2_soak_20260227T220243Z/` | PASS | 10-run contract soak stayed deterministic. |
| C3-C5 | `TestEvidence/bl048_slice_c3_replay_sentinel_20260227T222616Z/` etc. | PASS | Sentinel, soak, and exit semantics stayed green. |
| C6r/C7 | `TestEvidence/bl048_slice_c6r_mode_parity_20260227T222619Z/`, `...c7...` | PASS | Execute-mode parity and long-run drift stayed clean. |
| D1/D2 | `TestEvidence/bl048_slice_d1_done_candidate_20260227T222621Z/`, `...d2...` | PASS | Done-candidate and done-promotion parity landed. |
| Z16b owner-ready | `TestEvidence/bl048_e2e_promotion_z16b_20260227T225426Z/` | PASS | Final ownership-safe reconcile produced `promotion_recommendation=Done-candidate` with no blockers. |

## Notes

- Keep this file short.
- Deep replay history belongs in `TestEvidence/`, not here.
