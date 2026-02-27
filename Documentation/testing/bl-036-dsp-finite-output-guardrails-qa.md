Title: BL-036 DSP Finite Output Guardrails QA Contract
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

# BL-036 DSP Finite Output Guardrails QA Contract

## Purpose

Define deterministic, replay-ready QA requirements for BL-036 finite-output guarantees, including NaN/Inf/denormal containment and limiter/fallback behavior contracts.

## Linked Contracts

- Runbook: `Documentation/backlog/bl-036-dsp-finite-output-guardrails.md`
- Invariants: `Documentation/invariants.md`
- Lane harness: `scripts/qa-bl036-finite-output-lane-mac.sh`
- Scenario contract: `qa/scenarios/locusq_bl036_finite_output_suite.json`
- Slice A1 evidence packet: `TestEvidence/bl036_slice_a1_contract_20260227T002904Z/`
- Slice B1 evidence packet: `TestEvidence/bl036_slice_b1_lane_20260227T005722Z/`
- Slice C2 evidence packet: `TestEvidence/bl036_slice_c2_soak_20260227T010555Z/`
- Slice C3 evidence packet: `TestEvidence/bl036_slice_c3_replay_sentinel_20260227T011846Z/`
- Slice C4 evidence packet: `TestEvidence/bl036_slice_c4_soak_20260227T013722Z/`
- Slice C5 evidence packet: `TestEvidence/bl036_slice_c5_semantics_20260227T015144Z/`
- Slice C5b evidence packet: `TestEvidence/bl036_slice_c5b_semantics_20260227T025149Z/`
- Slice C5c evidence packet: `TestEvidence/bl036_slice_c5c_semantics_20260227T031011Z/`
- Slice C6 evidence packet: `TestEvidence/bl036_slice_c6_release_sentinel_20260227T033705Z/`

## Acceptance IDs (A1)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-A1-001` | Protected finite-output boundaries are fixed and explicit | runbook + QA parity + `acceptance_matrix.tsv` |
| `BL036-A1-002` | NaN/Inf containment rules are deterministic and fail-safe | `finite_output_contract.md` + `failure_taxonomy.tsv` |
| `BL036-A1-003` | Denormal threshold/handling is explicit and bounded | `finite_output_contract.md` |
| `BL036-A1-004` | Limiter and hard-clamp fallback behavior is bounded | runbook + QA parity |
| `BL036-A1-005` | Failure taxonomy schema is machine-readable | `failure_taxonomy.tsv` |
| `BL036-A1-006` | Replay-ready artifact schema is complete | artifact schema section + `status.tsv` |
| `BL036-A1-007` | Docs freshness pass captured | `docs_freshness.log` + `status.tsv` |

## Acceptance IDs (B1)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-B1-001` | Scenario schema + acceptance inventory are parseable and complete | `contract_runs/status.tsv` |
| `BL036-B1-002` | Guarded boundary contract contains required finite boundaries | `contract_runs/status.tsv` |
| `BL036-B1-003` | Containment rules + bounded thresholds are declared | `contract_runs/status.tsv` |
| `BL036-B1-004` | Replay hash stability contract passes deterministic reruns | `contract_runs/replay_hashes.tsv` |
| `BL036-B1-005` | Lane artifact schema is complete | `contract_runs/status.tsv` |
| `BL036-B1-006` | Deterministic hash-input contract excludes nondeterministic fields | `contract_runs/status.tsv` |
| `BL036-B1-007` | Execution-mode contract is explicit (`contract_only`, `execute_suite`) | `contract_runs/status.tsv` |
| `BL036-B1-008` | Failure taxonomy schema includes deterministic/runtime/missing-artifact classes | `contract_runs/failure_taxonomy.tsv` |
| `BL036-B1-009` | Multi-run soak summary contract emitted and bounded | `contract_runs/soak_summary.tsv` |

## Acceptance IDs (C3)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-C3-001` | 20-run replay sentinel executes with zero command failures | `status.tsv`, `validation_matrix.tsv` |
| `BL036-C3-002` | Replay signatures remain stable across all 20 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C3-003` | Semantic row signatures remain stable across all 20 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C3-004` | Failure taxonomy remains zero for deterministic/runtime/missing-artifact classes | `contract_runs/failure_taxonomy.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C3-005` | Required sentinel evidence schema is complete and parseable | `status.tsv`, `lane_notes.md`, `docs_freshness.log` |

## Acceptance IDs (C4)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-C4-001` | 50-run replay sentinel soak executes with zero command failures | `status.tsv`, `validation_matrix.tsv` |
| `BL036-C4-002` | Replay signatures remain stable across all 50 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C4-003` | Semantic row signatures remain stable across all 50 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C4-004` | Failure taxonomy remains zero for deterministic/runtime/missing-artifact classes | `contract_runs/failure_taxonomy.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C4-005` | Required C4 sentinel evidence schema is complete and parseable | `status.tsv`, `lane_notes.md`, `docs_freshness.log` |

## Acceptance IDs (C5)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-C5-001` | 20-run replay executes with zero command failures | `status.tsv`, `validation_matrix.tsv` |
| `BL036-C5-002` | Replay signatures remain stable across 20 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C5-003` | Semantic row signatures remain stable across 20 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C5-004` | Usage error `--runs 0` exits deterministically with code `2` | `exit_semantics_probe.tsv`, `probe_runs0.log` |
| `BL036-C5-005` | Usage error `--unknown-flag` exits deterministically with code `2` | `exit_semantics_probe.tsv`, `probe_unknown_flag.log` |
| `BL036-C5-006` | Required C5 evidence schema is complete and parseable | `status.tsv`, `lane_notes.md`, `docs_freshness.log` |

## Acceptance IDs (C5b)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-C5B-001` | C5 replay/exit probes rerun with deterministic outcomes | `status.tsv`, `validation_matrix.tsv` |
| `BL036-C5B-002` | Replay signatures remain stable across 20 recheck runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C5B-003` | Usage error probes continue to return strict exit `2` | `exit_semantics_probe.tsv` |
| `BL036-C5B-004` | C5b evidence schema is complete and parseable | `status.tsv`, `lane_notes.md` |
| `BL036-C5B-005` | Docs freshness state is captured explicitly for owner intake | `docs_freshness.log`, `status.tsv` |

## Acceptance IDs (C5c)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-C5C-001` | C5 deterministic replay/exit probes rerun with stable outcomes post-H2 | `status.tsv`, `validation_matrix.tsv` |
| `BL036-C5C-002` | Replay signatures remain stable across 20 recheck runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C5C-003` | Usage probes continue to enforce strict exit `2` | `exit_semantics_probe.tsv` |
| `BL036-C5C-004` | C5c evidence schema is complete and parseable | `status.tsv`, `lane_notes.md` |
| `BL036-C5C-005` | Docs freshness state is recaptured for owner intake | `docs_freshness.log`, `status.tsv` |

## Acceptance IDs (C6)

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL036-C6-001` | 50-run release-sentinel replay executes with zero command failures | `status.tsv`, `validation_matrix.tsv` |
| `BL036-C6-002` | Replay signatures remain stable across all 50 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C6-003` | Semantic row signatures remain stable across all 50 runs | `contract_runs/replay_hashes.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C6-004` | Usage probes continue to enforce strict exit `2` for invalid invocation | `exit_semantics_probe.tsv`, `probe_runs0.log`, `probe_unknown_flag.log` |
| `BL036-C6-005` | Failure taxonomy remains zero for deterministic/runtime/missing-artifact classes | `contract_runs/failure_taxonomy.tsv`, `replay_sentinel_summary.tsv` |
| `BL036-C6-006` | Required C6 evidence schema is complete and parseable with freshness captured | `status.tsv`, `lane_notes.md`, `docs_freshness.log` |

## Finite-Output Guardrail Contract Summary

### Boundary Coverage

1. Guarded ingress for values consumed by DSP math.
2. Guarded intermediate accumulation states used in output path.
3. Guarded pre-write output stage for host buffers.

### Containment and Bounds

1. NaN/Inf values are replaced with deterministic finite fallbacks.
2. Denormals are flushed to zero when `abs(value) < 1.0e-30`.
3. Output protection uses:
   - limiter-targeted envelope (`abs(sample) <= 1.0` preferred)
   - hard safety clamp (`abs(sample) <= 4.0` guaranteed before host write)
4. Post-protection non-finite sample detection fails closed to `0.0f`.

## Failure Taxonomy Schema

| Code | Category | Deterministic Class | Trigger |
|---|---|---|---|
| `BL036-FX-001` | non_finite_input_scalar | deterministic_contract_failure | NaN/Inf scalar at guarded boundary |
| `BL036-FX-002` | non_finite_input_vector_component | deterministic_contract_failure | NaN/Inf vector component at guarded boundary |
| `BL036-FX-003` | denormal_contained | deterministic_contract_failure | denormal-range value flushed to zero |
| `BL036-FX-004` | limiter_state_non_finite | deterministic_contract_failure | limiter state/coefficients become non-finite |
| `BL036-FX-005` | output_sample_non_finite_post_limiter | deterministic_contract_failure | non-finite sample found before host write |
| `BL036-FX-006` | output_hard_clamp_applied | deterministic_contract_failure | output exceeds hard-safety bound |
| `BL036-FX-007` | fallback_reason_missing_or_invalid | deterministic_contract_failure | fallback token missing/unknown |
| `BL036-FX-900` | harness_or_environment_blocker | runtime_execution_failure | validation blocked by external environment |

## Replay-Ready Command Contract

A1 docs contract gate:
```bash
./scripts/validate-docs-freshness.sh
```

B1 lane bootstrap contract:
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl036_slice_b1_lane_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

C2 determinism soak contract:
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 10 --out-dir TestEvidence/bl036_slice_c2_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

C3 replay sentinel contract:
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl036_slice_c3_replay_sentinel_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

C4 replay sentinel soak contract:
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl036_slice_c4_soak_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

C5 exit-semantics guard contract:
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl036_slice_c5_semantics_<timestamp>/contract_runs
./scripts/qa-bl036-finite-output-lane-mac.sh --runs 0
./scripts/qa-bl036-finite-output-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

C5b exit-semantics recheck contract:
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl036_slice_c5b_semantics_<timestamp>/contract_runs
./scripts/qa-bl036-finite-output-lane-mac.sh --runs 0
./scripts/qa-bl036-finite-output-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

C5c exit-semantics recheck contract (post-H2):
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl036_slice_c5c_semantics_<timestamp>/contract_runs
./scripts/qa-bl036-finite-output-lane-mac.sh --runs 0
./scripts/qa-bl036-finite-output-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

C6 release sentinel contract:
```bash
bash -n scripts/qa-bl036-finite-output-lane-mac.sh
./scripts/qa-bl036-finite-output-lane-mac.sh --help
./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl036_slice_c6_release_sentinel_<timestamp>/contract_runs
./scripts/qa-bl036-finite-output-lane-mac.sh --runs 0
./scripts/qa-bl036-finite-output-lane-mac.sh --unknown-flag
./scripts/validate-docs-freshness.sh
```

Runtime implementation replay command set (A2/B2/C1 readiness):
```bash
cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh
./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl036_<slice>_<timestamp>/rt_audit.tsv
```

## Artifact Schema

A1 required outputs:
1. `status.tsv`
2. `finite_output_contract.md`
3. `acceptance_matrix.tsv`
4. `failure_taxonomy.tsv`
5. `docs_freshness.log`

B1 required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `lane_notes.md`
7. `docs_freshness.log`

C2 required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `soak_summary.tsv`
7. `lane_notes.md`
8. `docs_freshness.log`

C3 required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `replay_sentinel_summary.tsv`
7. `lane_notes.md`
8. `docs_freshness.log`

C4 required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `replay_sentinel_summary.tsv`
7. `lane_notes.md`
8. `docs_freshness.log`

C5 required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `replay_sentinel_summary.tsv`
7. `exit_semantics_probe.tsv`
8. `lane_notes.md`
9. `docs_freshness.log`

C5b required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `replay_sentinel_summary.tsv`
7. `exit_semantics_probe.tsv`
8. `lane_notes.md`
9. `docs_freshness.log`

C5c required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `replay_sentinel_summary.tsv`
7. `exit_semantics_probe.tsv`
8. `lane_notes.md`
9. `docs_freshness.log`

C6 required outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `contract_runs/validation_matrix.tsv`
4. `contract_runs/replay_hashes.tsv`
5. `contract_runs/failure_taxonomy.tsv`
6. `replay_sentinel_summary.tsv`
7. `exit_semantics_probe.tsv`
8. `lane_notes.md`
9. `docs_freshness.log`

## C5c Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl036_slice_c5c_semantics_20260227T031011Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl036-finite-output-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 20 --out-dir TestEvidence/bl036_slice_c5c_semantics_20260227T031011Z/contract_runs` => `PASS`
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --runs 0` => `PASS` (expected usage exit `2`)
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --unknown-flag` => `PASS` (expected usage exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - C5c deterministic replay and strict exit semantics are stable and freshness gate is green.

## C6 Execution Snapshot (2026-02-27)

- Evidence packet:
  - `TestEvidence/bl036_slice_c6_release_sentinel_20260227T033705Z/status.tsv`
  - `validation_matrix.tsv`
  - `contract_runs/validation_matrix.tsv`
  - `contract_runs/replay_hashes.tsv`
  - `contract_runs/failure_taxonomy.tsv`
  - `replay_sentinel_summary.tsv`
  - `exit_semantics_probe.tsv`
  - `lane_notes.md`
  - `docs_freshness.log`
- Validation:
  - `bash -n scripts/qa-bl036-finite-output-lane-mac.sh` => `PASS`
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --help` => `PASS`
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --contract-only --runs 50 --out-dir TestEvidence/bl036_slice_c6_release_sentinel_20260227T033705Z/contract_runs` => `PASS`
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --runs 0` => `PASS` (expected usage exit `2`)
  - `./scripts/qa-bl036-finite-output-lane-mac.sh --unknown-flag` => `PASS` (expected usage exit `2`)
  - `./scripts/validate-docs-freshness.sh` => `PASS`
- Result:
  - C6 confirms release-sentinel replay depth (50 runs) and strict usage-exit semantics remain deterministic and stable.

Lane internal outputs (additive):
1. `contract_runs/status.tsv`
2. `contract_runs/qa_lane.log`
3. `contract_runs/soak_summary.tsv`
4. `contract_runs/run_01/status.tsv`
5. `contract_runs/run_02/status.tsv`
6. `contract_runs/run_03/status.tsv`

Future runtime implementation outputs (additive):
1. `build.log`
2. `qa_smoke.log`
3. `finite_fuzz.tsv`
4. `rt_audit.tsv`
5. `validation_matrix.tsv`

## Triage Sequence

1. If acceptance IDs drift, restore runbook/QA parity before any runtime-lane authoring.
2. If taxonomy schema is incomplete, update deterministic code mappings before replay lanes.
3. If C2 soak replay diverges (`signature_match`/`row_match` drift), classify as deterministic contract blocker and halt promotion.
4. If C3 replay sentinel diverges (`signature_match`/`row_match` drift), classify as deterministic blocker and require rerun before owner intake.
5. If C4 replay sentinel soak diverges (`signature_match`/`row_match` drift), classify as deterministic blocker and require rerun before owner intake.
6. If C5 usage probes return any exit other than `2`, classify as strict-exit-semantics blocker and require remediation before owner intake.
7. If C5b recheck retains replay/exit pass but docs freshness still fails, classify explicitly as external-docs blocker (not lane-logic regression).
8. If C6 release-sentinel replay diverges or usage probes return any exit other than `2`, classify as a release-sentinel blocker and require rerun before owner intake.
9. If docs freshness fails, resolve metadata violations first.
10. For implementation slices, classify failures as deterministic contract failures versus runtime blockers before owner intake.
