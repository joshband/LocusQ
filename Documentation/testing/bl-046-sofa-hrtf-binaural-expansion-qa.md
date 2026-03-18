Title: BL-046 SOFA HRTF and Binaural Expansion QA Contract
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-046 SOFA HRTF and Binaural Expansion QA Contract

## Purpose

Keep BL-046 QA short, deterministic, and evidence-first.
This runbook covers SOFA ingest, HRTF selection, fallback behavior, replay determinism, and the evidence shape needed to prove them.

## Authority

- Primary backlog authority: [Documentation/backlog/done/bl-046-sofa-hrtf-binaural-expansion.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-046-sofa-hrtf-binaural-expansion.md)
- Traceability anchors: [.ideas/parameter-spec.md](/Users/artbox/Documents/Repos/LocusQ/.ideas/parameter-spec.md), [.ideas/architecture.md](/Users/artbox/Documents/Repos/LocusQ/.ideas/architecture.md), [Documentation/invariants.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/invariants.md)

## Core Contract

| Area | Required rule |
|---|---|
| SOFA ingest | Declare required fields, finite-only bounds, and canonical ingest order. |
| HRTF selection | Declare precedence tiers, lexicographic tie-break, and fail-closed fallback. |
| Replay | Declare the hash input set and the exact trace equality rule. |
| Evidence | Declare required files and TSV columns before closeout. |

Fallback tokens and failure IDs remain mandatory. They are the contract surface for proving deterministic routing to safe output when a path is invalid.

## Validation Tiers

| Tier | Signal | Gate |
|---|---|---|
| `A1` | contract completeness | SOFA, HRTF, fallback, replay, schema, and docs freshness all explicit |
| `B1` | lane bootstrap | script help, contract-only mode, and scenario/acceptance alignment |
| `C2` | soak | `--runs 10`, zero signature divergence, zero row drift |
| `C3` | replay sentinel | `--runs 20`, zero signature divergence, zero row drift |
| `C4` | long-run parity | `--runs 50`, contract-only and execute-suite parity, exit semantics checked |
| `D1` | done candidate | `--runs 75`, parity preserved, evidence schema present |
| `D2` | done promotion | `--runs 100`, parity preserved, promotion readiness present |

## Evidence Shape

Every bundle under `TestEvidence/bl046_*_<timestamp>/` must include:
- `status.tsv`
- `validation_matrix.tsv`
- `replay_hashes.tsv`
- `failure_taxonomy.tsv`
- `lane_notes.md`
- `docs_freshness.log`

Tier-specific adds:
- `acceptance_matrix.tsv` for `A1`
- `sofa_binaural_contract.md` for `A1`
- `contract_runs/` outputs for `B1`, `C2`, and `C3`
- `contract_runs_contract/` and `contract_runs_execute/` plus `mode_parity.tsv`, `soak_summary.tsv`, and `exit_semantics_probe.tsv` for `C4`, `D1`, and `D2`
- `promotion_readiness.md` for `D2`

## Highest-Signal Evidence

- [TestEvidence/bl046_owner_ready_z16_20260227T225448Z/](/Users/artbox/Documents/Repos/LocusQ/TestEvidence/bl046_owner_ready_z16_20260227T225448Z/)
- [TestEvidence/bl046_slice_d2_done_promotion_20260227T222959Z/](/Users/artbox/Documents/Repos/LocusQ/TestEvidence/bl046_slice_d2_done_promotion_20260227T222959Z/)
- [TestEvidence/bl046_slice_c4_longrun_mode_parity_20260227T223236Z/](/Users/artbox/Documents/Repos/LocusQ/TestEvidence/bl046_slice_c4_longrun_mode_parity_20260227T223236Z/)

## Milestone Snapshot

- `A1` complete: contract surface and freshness gate defined.
- `B1` complete: lane bootstrap contract validated.
- `C2` complete: soak contract passed.
- `C3` complete: replay sentinel contract passed.
- `C4` complete: long-run parity contract passed.
- `D1` complete: done-candidate contract passed.
- `D2` complete: owner-ready promotion bundle passed.

## Validation

- `./scripts/validate-docs-freshness.sh`

## Triage Order

1. Fix acceptance or schema drift first.
2. Fix SOFA ingest or HRTF routing gaps second.
3. Fix replay or evidence gaps before closeout.
4. Fix docs freshness last, but never skip it.

## Archive Note

This active contract is the short-form surface.
The long-form lane history and repeated evidence snapshots remain in the done backlog packet and generated evidence bundles.
