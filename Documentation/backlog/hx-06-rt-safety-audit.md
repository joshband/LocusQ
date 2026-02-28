Title: HX-06 RT Safety Audit Reconciliation Ledger
Document Type: Backlog Support
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-28

# HX-06 RT Safety Audit Reconciliation Ledger

## Purpose

Track recurring RT-audit allowlist reconciliation passes that keep
`scripts/rt-safety-audit.sh` promotion-gate ready as line numbers drift.

Canonical HX-06 implementation runbook remains:
- `Documentation/backlog/done/hx-06-rt-safety-audit.md`

## Governance Alignment

- This file is a `Backlog Support` ledger and is intentionally exempt from active runbook schema fields (for example `Default Replay Tier`, `Heavy Lane Budget`, and `Replay Cadence Plan`).
- Lifecycle/cadence governance for active backlog work is defined in:
  - `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
  - `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)
- Promotion-state authority for HX-06 remains the canonical done runbook path above.

## BL-033 Z2 Reconciliation (2026-02-26)

- Evidence root: `TestEvidence/bl033_rt_gate_z2_20260226T003240Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=94`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 4 | Static lexical false-positives in comments/strings |
| `intentional_current_behavior` | 90 | Non-audio-thread allocations/container setup used for current runtime behavior |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- Docs freshness remained red due root-doc date drift in `README.md` and
  `CHANGELOG.md` relative to `status.json`; those files were outside slice ownership.

## BL-033 Z9 Reconciliation (2026-02-26)

- Evidence root: `TestEvidence/bl033_rt_gate_z9_20260226T010610Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=80`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 3 | Static lexical false-positives in comments/strings |
| `intentional_current_behavior` | 77 | Non-audio-thread allocations/container setup used for current runtime behavior |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- This pass resolved Z9 allowlist drift introduced by BL-033 A2/B2/C2 line movement.

## BL-034 Z2 Reconciliation (2026-02-26)

- Evidence root: `TestEvidence/bl034_rt_gate_z2_20260226T033208Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=119`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 3 | Static lexical false-positives in comments/strings |
| `intentional_current_behavior` | 116 | Non-audio-thread allocations/container setup used for current runtime behavior |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- This pass cleared BL-034 owner blocker by restoring RT gate status to `non_allowlisted=0`.

## BL-034 Z3 Reconciliation (2026-02-26)

- Evidence root: `TestEvidence/bl034_rt_gate_z3_20260226T035619Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=108`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 3 | Static lexical false-positives in comments/strings |
| `intentional_current_behavior` | 105 | Non-audio-thread allocations/container setup used for current runtime behavior |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- This pass re-aligned RT allowlist line drift on the current BL-034 branch map after owner replay.

## BL-032 D2 Reconciliation (2026-02-26)

- Evidence root: `TestEvidence/bl032_rt_gate_d2_20260226T150423Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=92`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 0 | No additional lexical false-positive entries were required in this pass |
| `intentional_current_behavior` | 92 | Non-audio-thread allocations/container setup entries shifted by current branch line-map changes |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- This pass cleared the BL-032 D1 RT blocker by restoring `non_allowlisted=0` on the current branch map.

## BL-020 C2 Reconciliation (2026-02-26)

- Evidence root: `TestEvidence/bl020_rt_gate_c2_20260226T193025Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=85`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 1 | Static lexical false-positive match in comment text |
| `intentional_current_behavior` | 84 | Non-audio-thread allocation/container setup paths currently relied on by runtime construction |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- Entry set matched both inputs exactly before allowlist update:
  - `TestEvidence/bl020_slice_c1_native_20260226T174052Z/rt_audit.tsv`
  - `TestEvidence/owner_sync_bl030_bl020_bl023_n9_20260226T192237Z/rt_audit.tsv`

## BL-035 D2 Reconciliation (2026-02-26)

- Evidence root: `TestEvidence/bl035_rt_gate_d2_20260226T233641Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=87`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 1 | Static lexical false-positive in comment text |
| `intentional_current_behavior` | 86 | Non-audio-thread allocation/container setup entries shifted by current branch line-map changes |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- This pass cleared BL-035 Slice D RT blocker caused by allowlist line-map drift on current branch snapshot.

## BL-035 D5b Reconciliation (2026-02-27)

- Evidence root: `TestEvidence/bl035_rt_gate_d5b_20260227T001744Z/`
- Baseline (`rt_before.tsv`): `non_allowlisted=74`
- Post-reconcile (`rt_after.tsv`): `non_allowlisted=0`
- Allowlist file updated: `scripts/rt-safety-allowlist.txt`
- Delta ledger: `allowlist_delta.md`

### Delta Classification Summary

| Class | Count | Meaning |
|---|---:|---|
| `baseline_debt` | 2 | Static lexical false-positives in lock-free diagnostic/comment text |
| `intentional_current_behavior` | 72 | Non-audio-thread allocation/container setup entries shifted by current branch line-map changes |

### Notes

- Reconciliation used explicit `file:line:rule` entries only (no wildcard rules).
- Requested input pattern `TestEvidence/bl035_slice_d5_integrated_<timestamp>/` was absent in workspace; reconciliation source was the live baseline `rt_before.tsv` generated in this D5b slice.
