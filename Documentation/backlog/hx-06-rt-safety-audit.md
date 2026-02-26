Title: HX-06 RT Safety Audit Reconciliation Ledger
Document Type: Backlog Support
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# HX-06 RT Safety Audit Reconciliation Ledger

## Purpose

Track recurring RT-audit allowlist reconciliation passes that keep
`scripts/rt-safety-audit.sh` promotion-gate ready as line numbers drift.

Canonical HX-06 implementation runbook remains:
- `Documentation/backlog/done/hx-06-rt-safety-audit.md`

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
