Title: BL-036 DSP Finite Output Guardrails QA Contract
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# BL-036 DSP Finite Output Guardrails QA Contract

## Purpose

Define the deterministic QA contract for BL-036 finite-output guardrails.
The contract keeps NaN/Inf/denormal handling fail-safe, replayable, and easy to audit.

## Linked Contracts

- Runbook: `Documentation/backlog/done/bl-036-dsp-finite-output-guardrails.md`
- Invariants: `Documentation/invariants.md`
- Lane harness: `scripts/qa-bl036-finite-output-lane-mac.sh`
- Scenario contract: `qa/scenarios/locusq_bl036_finite_output_suite.json`

## Core Contract

### Guardrail Rules

| Rule | Contract |
|---|---|
| Finite-only inputs | NaN/Inf is replaced with a deterministic finite fallback at each guarded boundary. |
| Denormal handling | `abs(value) < 1.0e-30` is flushed to zero. |
| Output protection | Prefer `abs(sample) <= 1.0`; hard safety clamp guarantees `abs(sample) <= 4.0` before host write. |
| Post-protection check | Non-finite output before host write fails closed to `0.0f`. |

### Failure Taxonomy

| Code | Category | Deterministic Class | Trigger |
|---|---|---|---|
| `BL036-FX-001` | non_finite_input_scalar | deterministic_contract_failure | NaN/Inf scalar at guarded boundary |
| `BL036-FX-002` | non_finite_input_vector_component | deterministic_contract_failure | NaN/Inf vector component at guarded boundary |
| `BL036-FX-003` | denormal_contained | deterministic_contract_failure | denormal-range value flushed to zero |
| `BL036-FX-004` | limiter_state_non_finite | deterministic_contract_failure | limiter state or coefficients become non-finite |
| `BL036-FX-005` | output_sample_non_finite_post_limiter | deterministic_contract_failure | non-finite sample found before host write |
| `BL036-FX-006` | output_hard_clamp_applied | deterministic_contract_failure | output exceeds hard-safety bound |
| `BL036-FX-007` | fallback_reason_missing_or_invalid | deterministic_contract_failure | fallback token missing or unknown |
| `BL036-FX-900` | harness_or_environment_blocker | runtime_execution_failure | validation blocked by external environment |

## Replay / Evidence Contract

Required evidence root:
- `TestEvidence/bl036_<slice>_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `contract_runs/replay_hashes.tsv`
- `contract_runs/failure_taxonomy.tsv`
- `replay_sentinel_summary.tsv` or `soak_summary.tsv`
- `exit_semantics_probe.tsv` when usage probes are part of the lane
- `lane_notes.md`
- `docs_freshness.log`

Required TSV columns:
- `status.tsv`: lane id, result, evidence path, blocker summary
- `validation_matrix.tsv`: run id, command, exit code, result, log path
- `replay_hashes.tsv`: run id, signature hash, row hash, drift flags
- `failure_taxonomy.tsv`: failure id, category, trigger, classification, blocking, severity, expected artifact

## Replay Tiers

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | T1 | 3 | `validation_matrix.tsv` + replay summary |
| Candidate | T2 | 5 | stable replay summary + taxonomy |
| Promotion | T3 | 10 | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20/50/75/100 | explicit soak or release-sentinel evidence |

Cost rule:
- Heavy wrappers should use targeted reruns first.
- Do not repeat full multi-run sweeps unless the failing run index is diagnosed.

## Validation Commands

Current lane contract:

```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs <N> --out-dir TestEvidence/bl036_<slice>_<timestamp>/contract_runs
./scripts/qa-bl036-finite-output-lane-mac.sh --runs 0
./scripts/qa-bl036-finite-output-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

Use `N = 3`, `10`, `20`, `50`, `75`, or `100` based on the replay tier in the runbook.

## Current Signal

| Item | Status |
|---|---|
| Runbook authority | `Documentation/backlog/done/bl-036-dsp-finite-output-guardrails.md` |
| Current technical state | Done |
| Promotion state | Done |
| Latest high-signal done-promotion evidence | `TestEvidence/bl036_slice_d2_done_promotion_20260227T201716Z/` |
| Latest sentinel evidence | `TestEvidence/bl036_slice_c6_release_sentinel_20260227T033705Z/` |
| Latest docs freshness signal | PASS in the latest recorded evidence packets |

## Milestone Snapshot

| Milestone | Packet | Result | Why it matters |
|---|---|---|---|
| A1 contract | `TestEvidence/bl036_slice_a1_contract_20260227T002904Z/` | PASS | Contract surface defined the finite-output rules. |
| B1 lane | `TestEvidence/bl036_slice_b1_lane_20260227T005722Z/` | PASS | Replay contract and evidence schema held. |
| C3 replay sentinel | `TestEvidence/bl036_slice_c3_replay_sentinel_20260227T011846Z/` | PASS | 20-run sentinel stayed deterministic. |
| C4 soak | `TestEvidence/bl036_slice_c4_soak_20260227T013722Z/` | PASS | Long-run soak stayed stable. |
| C5/C5b/C5c semantics | `TestEvidence/bl036_slice_c5*_semantics_20260227T015144Z/` etc. | PASS | Usage probes stayed strict and repeatable. |
| C6 release sentinel | `TestEvidence/bl036_slice_c6_release_sentinel_20260227T033705Z/` | PASS | Release-scale sentinel remained clean. |
| D1 done-candidate | `TestEvidence/bl036_slice_d1_done_candidate_20260227T183420Z/` | PASS | Promotion-ready posture was established. |
| D2 done-promotion | `TestEvidence/bl036_slice_d2_done_promotion_20260227T201716Z/` | PASS | Final promotion evidence landed. |

## Notes

- The detailed packet chronology stays in `TestEvidence/`.
- Keep this file short; add new history only when it changes the active contract.
