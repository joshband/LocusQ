Title: BL-034 Headphone Verification QA Contract
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-034 Deterministic QA + Release Linkage Contract (Slice C1 + D1 + D2)

## Purpose
Define deterministic QA lane behavior for BL-034 headphone verification/profile governance with strict exit semantics and replay-hash stability checks.

## Linked Contracts
- Runbook: `Documentation/backlog/bl-034-headphone-calibration-verification.md`
- Scenario: `qa/scenarios/locusq_bl034_headphone_verification_suite.json`
- Lane Script: `scripts/qa-bl034-headphone-verification-lane-mac.sh`
- Release Checklist Template: `Documentation/runbooks/release-checklist-template.md`

## Acceptance IDs

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `BL034-C1-001` | Scenario + lane contract schema is parseable and complete | `status.tsv` (`BL034-C1-001_contract_schema`) |
| `BL034-C1-002` | Failure taxonomy schema contains required deterministic/runtime classes | `status.tsv` (`BL034-C1-002_failure_taxonomy_schema`) + `failure_taxonomy.tsv` |
| `BL034-C1-003` | Execution mode semantics are explicit for `contract_only` and `execute_suite` | `status.tsv` (`BL034-C1-003_execution_mode_contract`) |
| `BL034-C1-004` | Replay hash stability is enforced for deterministic reruns | `status.tsv` (`BL034-C1-004_replay_hash_stability`) + `replay_hashes.tsv` |
| `BL034-C1-005` | Required artifact schema is complete | `status.tsv` (`BL034-C1-005_artifact_schema_complete`) |
| `BL034-C1-006` | Hash inputs exclude nondeterministic fields | `status.tsv` (`BL034-C1-006_hash_input_contract`) + `scenario_contract.log` |
| `BL034-D1-001` | BL-034 release-readiness hooks are mapped to BL-030 gate IDs | `acceptance_mapping.tsv` |
| `BL034-D1-002` | Required BL-034 evidence artifacts are mapped to release checklist hook pass criteria | `acceptance_mapping.tsv` |
| `BL034-D1-003` | Deterministic release-readiness pass/fail taxonomy is defined | `failure_taxonomy.tsv` |
| `BL034-D1-004` | D1 acceptance IDs are parity-aligned across runbook + QA + traceability | `release_linkage_contract.md` |
| `BL034-D1-005` | Docs freshness gate pass is captured for D1 linkage packet | `docs_freshness.log` + `status.tsv` |
| `BL034-D2-001` | B2 native hardening evidence is explicitly mapped into release-readiness checks | `acceptance_mapping.tsv` + `TestEvidence/bl034_slice_b2_native_hardening_20260226T033523Z/status.tsv` |
| `BL034-D2-002` | Z2 RT gate drift/reconcile evidence is explicitly mapped to release-governance RT expectations | `acceptance_mapping.tsv` + `TestEvidence/bl034_rt_gate_z2_20260226T033208Z/status.tsv` |
| `BL034-D2-003` | Owner replay expectation after Z2 reconcile is explicit and deterministic | `acceptance_mapping.tsv` + `TestEvidence/bl034_owner_sync_z2_20260226T034919Z/validation_matrix.tsv` |
| `BL034-D2-004` | Owner-side blocker-resolution path is defined when RT drift reappears after worker reconcile | `release_linkage_contract.md` + `linkage_delta.md` |
| `BL034-D2-005` | D2 linkage refresh is additive and preserves existing D1 acceptance IDs | `status.tsv` (`BL034-D2-005_additive_schema_guard`) + `acceptance_mapping.tsv` |

## Lane Command Contract

Contract-only mode (default):
```bash
./scripts/qa-bl034-headphone-verification-lane-mac.sh --contract-only --out-dir TestEvidence/bl034_headphone_verification_<timestamp>
```

Execute-suite mode:
```bash
./scripts/qa-bl034-headphone-verification-lane-mac.sh --execute-suite --out-dir TestEvidence/bl034_headphone_verification_<timestamp>
```

Deterministic replay contract check:
```bash
./scripts/qa-bl034-headphone-verification-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl034_slice_c1_qa_lane_<timestamp>/contract_runs
```

Optional controls:
- `--scenario <path>` overrides scenario suite path.
- `--qa-bin <path>` overrides QA runner path.
- `--skip-build` bypasses build during `--execute-suite`.

## Artifact Schema

Required machine-readable outputs:
1. `status.tsv`
2. `validation_matrix.tsv`
3. `replay_hashes.tsv`
4. `failure_taxonomy.tsv`

Additional required lane outputs:
1. `qa_lane.log`
2. `scenario_contract.log`
3. `scenario_result.log`

Execute-suite additions:
1. `build.log`
2. `scenario_run.log`
3. `scenario_result.json`

## Replay Hash Determinism Contract

Hash inputs include only semantic content:
1. `status.tsv` (`check` + `result` columns)
2. `scenario_contract.log` key/value semantics
3. `scenario_result.log` summary semantics (`result_status`, `warnings`, `passed`, `failed`, `total`)
4. `failure_taxonomy.tsv` class/count rows

Excluded from hash inputs:
1. Timestamps
2. Absolute paths
3. Log line numbers
4. Wall-clock durations

## Failure Taxonomy

| Failure Class | Category | Trigger |
|---|---|---|
| `deterministic_contract_failure` | deterministic | Contract/schema/threshold mismatch |
| `runtime_execution_failure` | runtime | Build/QA runner/tool command failed |
| `missing_result_artifact` | runtime | Required artifact absent |
| `deterministic_replay_divergence` | deterministic | Combined replay hash diverges from baseline |
| `deterministic_replay_row_drift` | deterministic | Semantic replay row signature drifts |

## Release-Governance Linkage Mapping (Slice D1)

| BL-030 Gate Hook | BL-034 D1 Contract Requirement | Required Evidence | Deterministic Pass Rule |
|---|---|---|---|
| `RL-05` | Headphone verification replay/readiness evidence is present for release review | `status.tsv`, `validation_matrix.tsv`, `replay_hashes.tsv`, `failure_taxonomy.tsv` (from BL-034 C1 lane bundle) | Required artifacts present and replay signatures show no divergence for contract runs. |
| `RL-08` | BL-034 linkage docs satisfy freshness contract | `docs_freshness.log` (from D1 bundle) | Freshness command exits `0`. |
| `RL-09` | Release-note readiness pointers are declared for BL-034 | `release_linkage_contract.md` | Contract doc includes gate hooks, acceptance IDs, and evidence pointers. |
| `RL-10` | BL-034 evidence manifest is machine-readable for packaging review | `acceptance_mapping.tsv` | Mapping rows cover all `BL034-D1-001..005` acceptance IDs with explicit gate hooks. |

## Deterministic Release-Readiness Taxonomy (Slice D1)

| Failure Class | Deterministic Category | Trigger |
|---|---|---|
| `missing_release_hook_mapping` | contract_schema | Required BL-030 hook (`RL-05`, `RL-08`, `RL-09`, `RL-10`) missing from D1 mapping table. |
| `missing_required_artifact_mapping` | artifact_schema | Required BL-034 evidence artifact not mapped to a release hook. |
| `release_taxonomy_contract_mismatch` | taxonomy_schema | Runbook/QA/traceability taxonomy definitions diverge or omit required classes. |
| `cross_doc_parity_failure` | cross_reference | D1 acceptance IDs not present across runbook + QA + traceability. |
| `docs_freshness_failure` | freshness_gate | `./scripts/validate-docs-freshness.sh` returns non-zero during D1 review. |

## Release-Governance Linkage Mapping (Slice D2 Refresh)

| BL-030 Gate Hook | BL-034 D2 Contract Requirement | Required Evidence | Deterministic Pass Rule |
|---|---|---|---|
| `RL-05` | B2 native hardening replay prerequisites are locked into BL-034 release-readiness review | `TestEvidence/bl034_slice_b2_native_hardening_20260226T033523Z/status.tsv`, `qa_bl009.log`, `qa_bl034.log` | B2 `status.tsv` rows `build`, `qa_smoke`, `qa_bl009`, `qa_bl034`, and `overall` are all `PASS`. |
| `RL-02` | Z2 RT drift/reconcile evidence is linked as the required RT-gate reconciliation baseline | `TestEvidence/bl034_rt_gate_z2_20260226T033208Z/status.tsv`, `blocker_resolution.md` | Z2 `status.tsv` confirms `rt_before` pass record (`exit=1;non_allowlisted=119`) and `rt_after` pass record (`exit=0;non_allowlisted=0`) with `overall=pass`. |
| `RL-02` | Owner replay expectation after Z2 reconcile is explicit for current-branch RT authority | `TestEvidence/bl034_owner_sync_z2_20260226T034919Z/status.tsv`, `owner_decisions.md`, `rt_audit.tsv` | Owner replay must rerun RT audit on current branch. If `rt_audit=FAIL`, decision remains `Blocked` and next action must require a new RT reconcile slice plus owner replay rerun. |
| `RL-08` | D2 linkage refresh docs satisfy freshness policy | `docs_freshness.log` (from D2 packet) | Freshness command exits `0`. |
| `RL-10` | D2 acceptance mapping stays machine-readable and additive | `acceptance_mapping.tsv` (from D2 packet) | Mapping retains `BL034-D1-001..005` entries unchanged and adds `BL034-D2-001..005` rows without ID reuse/removal. |

## Deterministic Release-Readiness Taxonomy (Slice D2 Additive)

| Failure Class | Deterministic Category | Trigger |
|---|---|---|
| `missing_b2_hardening_mapping` | contract_schema | B2 hardening evidence mapping for `BL034-D2-001` is absent from D2 acceptance map. |
| `missing_z2_rt_reconcile_mapping` | contract_schema | Z2 drift/reconcile evidence mapping for `BL034-D2-002` is absent or does not preserve `119 -> 0` semantics. |
| `owner_replay_expectation_missing` | replay_governance | D2 mapping omits explicit owner replay requirement and branch-authoritative RT audit expectation. |
| `owner_rt_regression_untracked` | blocker_resolution | Owner replay shows `rt_audit=FAIL` but no deterministic next-action/reconcile path is recorded. |
| `additive_acceptance_id_violation` | schema_compatibility | D2 refresh mutates/removes existing `BL034-D1-*` IDs or reuses IDs with divergent meaning. |

## Validation Commands

```bash
bash -n scripts/qa-bl034-headphone-verification-lane-mac.sh
./scripts/qa-bl034-headphone-verification-lane-mac.sh --help
./scripts/qa-bl034-headphone-verification-lane-mac.sh --contract-only --runs 3 --out-dir TestEvidence/bl034_slice_c1_qa_lane_<timestamp>/contract_runs
./scripts/validate-docs-freshness.sh
```

## Triage Sequence
1. Resolve schema/contract failures before execute-suite runs.
2. Resolve artifact-schema failures before replay-hash triage.
3. For replay drift, inspect `validation_matrix.tsv` then `replay_hashes.tsv`.
4. Use `failure_taxonomy.tsv` to classify deterministic vs runtime root cause.

## Slice F UI Diagnostics Contract

Scope: expose BL-034 profile-governance + verification telemetry in the standalone RENDERER diagnostics panel without native contract changes.

### Slice F Acceptance IDs

| Acceptance ID | Requirement | Evidence Signal |
|---|---|---|
| `BL034-F-001` | Headphone Verification diagnostics card renders and consumes additive BL-034 fields without breaking existing cards | `ui_contract.md` + `status.tsv` |
| `BL034-F-002` | `verificationScoreStatus` maps deterministically to chip states `PASS/WARN/FAIL/UNAVAILABLE` | `ui_contract.md` + `selftest_bl009.log` |
| `BL034-F-003` | Score surfaces (`frontBack`, `elevation`, `externalization`, `confidence`) are bounded to `[0..1]` in UI presentation | `ui_contract.md` + `selftest_bl029.log` |
| `BL034-F-004` | Missing/invalid payload fields degrade to neutral `unavailable` text with no throw and no panel break | `ui_contract.md` + `selftest_bl029.log` |
| `BL034-F-005` | BL-029 and BL-009 scoped selftests remain green after UI diagnostics integration | `selftest_bl029.log` + `selftest_bl009.log` |

### Slice F Score-State Mapping

| `rendererHeadphoneVerificationScoreStatus` token | UI chip label | UI chip state |
|---|---|---|
| `pass`, `ok`, `stable`, `verified`, `ready` | `PASS` | `ok` |
| `warn`, `warning`, `degraded`, `fallback`, `review`, `unstable` | `WARN` | `warning` |
| `fail`, `error`, `invalid`, `mismatch`, `blocked` | `FAIL` | `error` |
| missing/empty | `UNAVAILABLE` | `neutral` |
| unknown token | `WARN` | `warning` |
