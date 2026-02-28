Title: BL-034 Headphone Calibration Verification and Profile Governance
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-28

# BL-034: Headphone Calibration Verification and Profile Governance

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | Done (Owner Z6 promotion finalized; Owner Z7 post-done confidence replay remains green) |
| Owner Track | Track D — QA Platform |
| Depends On | BL-033 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-034-headphone-calibration-verification-spec-2026-02-25.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Device profile catalog and fallback taxonomy contract |
| B | Med | M | Perceptual verification workflow contract and score persistence |
| C | High | L | Deterministic QA lane set + replay hash contract |
| D | Med | M | Release-governance evidence linkage for headphone readiness |

## Objective

Define and enforce a deterministic verification + profile-governance layer for headphone calibration so profile selection, fallback behavior, and perceptual verification outcomes are reproducible, machine-readable, and release-governance ready.

## Scope & Non-Scope

**In scope:**
- Device/profile catalog contract (generic, known profiles, custom SOFA refs)
- Fallback taxonomy and deterministic reason publication when assets are missing/invalid
- Verification metric storage contract (front/back, elevation, externalization confidence)
- Deterministic QA lane definitions and replay evidence schema
- Release checklist integration points for headphone readiness evidence

**Out of scope:**
- New DSP algorithms beyond BL-033 core
- Personalized HRTF generation tooling
- Broad UI redesign beyond verification-state exposure
- Host-specific proprietary head-tracking integrations

## Architecture Context

- Upstream dependency: BL-033 core monitoring path and state contract
- Research origin:
  - `Documentation/research/LocusQ Headphone Calibration Research Outline.md`
  - `Documentation/research/Headphone Calibration for 3D Audio.pdf`
- Device compatibility contract: `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- Release governance reference: `Documentation/runbooks/release-checklist-template.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Profile library + fallback reason taxonomy | profile/state contracts, docs, QA schema | BL-033 at least Slice B complete | profile/fallback matrix deterministic and documented |
| B | Verification metric contract + persistence | processor diagnostics paths + docs | Slice A complete | verification metrics serialize/load without drift |
| C | QA lanes + replay hashes | `qa/scenarios/*`, `scripts/qa-*.sh`, evidence schemas | Slice B complete | deterministic replay and failure taxonomy green |
| D | Release-governance linkage | release docs and evidence pointers | Slice C complete | BL-030-compatible evidence contract published |

## Current Slice Disposition (Owner Sync Z7)

| Slice | Worker Result | Owner Replay Result | Evidence |
|---|---|---|---|
| A1 | PASS | PASS (docs contract + freshness green) | `TestEvidence/bl034_slice_a1_profile_contract_20260226T012928Z/status.tsv` |
| B1 | PASS | PASS (build/smoke/BL-009 functional gates green) | `TestEvidence/bl034_slice_b1_native_metrics_20260226T013354Z/status.tsv` |
| B2 | PASS | PASS (native determinism hardening gates green) | `TestEvidence/bl034_slice_b2_native_hardening_20260226T033523Z/status.tsv` |
| C1 | PASS | PASS (lane deterministic replay hashes stable) | `TestEvidence/bl034_slice_c1_qa_lane_20260226T013221Z/status.tsv`, `TestEvidence/bl034_owner_sync_z2_20260226T034919Z/lane_runs/replay_hashes.tsv` |
| D1 | PASS | PASS (docs linkage contract + freshness green) | `TestEvidence/bl034_slice_d1_release_linkage_20260226T033432Z/status.tsv` |
| D2 | PASS | PASS (additive release-linkage refresh + freshness green) | `TestEvidence/bl034_slice_d2_release_linkage_20260226T035925Z/status.tsv` |
| Z1 | — | FAIL (RT audit blocker: `non_allowlisted=119`) | `TestEvidence/bl034_owner_sync_z1_20260226T031026Z/status.tsv`, `TestEvidence/bl034_owner_sync_z1_20260226T031026Z/rt_audit.tsv` |
| Z2 (RT reconcile worker) | PASS | PASS (`rt_before=119`, `rt_after=0`) | `TestEvidence/bl034_rt_gate_z2_20260226T033208Z/status.tsv`, `TestEvidence/bl034_rt_gate_z2_20260226T033208Z/rt_after.tsv` |
| Z2 (owner replay) | — | FAIL (fresh RT audit drift: `non_allowlisted=108`) | `TestEvidence/bl034_owner_sync_z2_20260226T034919Z/status.tsv`, `TestEvidence/bl034_owner_sync_z2_20260226T034919Z/rt_audit.tsv` |
| Z3 (RT reconcile worker) | PASS | PASS (`rt_before=108`, `rt_after=0`) | `TestEvidence/bl034_rt_gate_z3_20260226T035619Z/status.tsv`, `TestEvidence/bl034_rt_gate_z3_20260226T035619Z/rt_after.tsv` |
| Z4 (replay audit worker) | PASS | PASS (deterministic replay runs=5, signature/row mismatch=0) | `TestEvidence/bl034_replay_audit_z4_20260226T035632Z/status.tsv`, `TestEvidence/bl034_replay_audit_z4_20260226T035632Z/lane_runs/replay_hashes.tsv` |
| Z3 (owner replay) | — | PASS (all owner gates green; RT non_allowlisted=0) | `TestEvidence/bl034_owner_sync_z3_20260226T040304Z/status.tsv`, `TestEvidence/bl034_owner_sync_z3_20260226T040304Z/rt_audit.tsv` |
| E1 (determinism soak worker) | PASS | PASS (`runs=10`, signature drift=0, row drift=0) | `TestEvidence/bl034_determinism_soak_e1_20260226T040823Z/status.tsv`, `TestEvidence/bl034_determinism_soak_e1_20260226T040823Z/soak_summary.tsv` |
| E2 (RT sentinel worker) | PASS | PASS (`run_01..03 non_allowlisted=0`) | `TestEvidence/bl034_rt_sentinel_e2_20260226T040835Z/status.tsv`, `TestEvidence/bl034_rt_sentinel_e2_20260226T040835Z/rt_stability_summary.tsv` |
| E3 (contract parity worker) | PASS | PASS (`missing=0`, `schema_drift=0`) | `TestEvidence/bl034_contract_parity_e3_20260226T040851Z/status.tsv`, `TestEvidence/bl034_contract_parity_e3_20260226T040851Z/missing_or_drift.tsv` |
| Z5 (owner replay) | — | PASS (all owner gates green; RT non_allowlisted=0; BL-009 lane pass) | `TestEvidence/bl034_owner_sync_z5_20260226T041435Z/status.tsv`, `TestEvidence/bl034_owner_sync_z5_20260226T041435Z/rt_audit.tsv` |
| Z6 (final done promotion owner replay) | — | PASS (all promotion gates green; deterministic lane stable; RT non_allowlisted=0) | `TestEvidence/bl034_done_promotion_z6_20260226T041946Z/status.tsv`, `TestEvidence/bl034_done_promotion_z6_20260226T041946Z/validation_matrix.tsv` |
| F (standalone diagnostics UI worker) | PASS | PASS (BL-029/BL-009 scoped selftests green; docs freshness green) | `TestEvidence/bl034_slice_f_ui_diag_20260226T042553Z/status.tsv`, `TestEvidence/bl034_slice_f_ui_diag_20260226T042553Z/ui_contract.md` |
| F1 (cross-lane stress worker) | PASS | PASS (BL-009 x5 and BL-034 lane x2 deterministic replay) | `TestEvidence/bl034_cross_lane_stress_f1_20260226T041919Z/status.tsv`, `TestEvidence/bl034_cross_lane_stress_f1_20260226T041919Z/stress_summary.tsv` |
| F2 (RT drift watch worker) | PASS | PASS (RT audits `run_01..05` all `non_allowlisted=0`) | `TestEvidence/bl034_rt_drift_watch_f2_20260226T041934Z/status.tsv`, `TestEvidence/bl034_rt_drift_watch_f2_20260226T041934Z/drift_summary.tsv` |
| Z7 (owner replay post-done confidence) | — | PASS (all owner gates green; done posture retained) | `TestEvidence/bl034_owner_sync_z7_20260226T042832Z/status.tsv`, `TestEvidence/bl034_owner_sync_z7_20260226T042832Z/validation_matrix.tsv` |

## Slice A1 Contract (Profile Catalog + Fallback Taxonomy)

### Canonical Profile Catalog Identities (Normative)

| Profile ID | Class | Source | Notes |
|---|---|---|---|
| `generic` | built_in_reference | bundled fallback profile | Mandatory baseline profile; must always be resolvable. |
| `airpods_pro_2` | built_in_reference | bundled tuned profile | Deterministic vendor-tuned profile identity. |
| `sony_wh1000xm5` | built_in_reference | bundled tuned profile | Deterministic vendor-tuned profile identity. |
| `custom_sofa` | external_reference | user-specified SOFA reference | Requires explicit `customSofaRef` token when requested/active. |

### Deterministic Fallback Taxonomy (Normative)

| Reason Code | Class | Required Fallback Target | Deterministic Trigger |
|---|---|---|---|
| `none` | no_fallback | `none` | Requested profile resolved without downgrade. |
| `requested_profile_unavailable` | profile_resolution | `generic` | Requested profile ID not present in active catalog domain. |
| `requested_profile_invalid` | profile_validation | `generic` | Requested profile token malformed or outside enum domain. |
| `custom_sofa_ref_missing` | external_reference | `generic` | `custom_sofa` requested/active but reference token empty. |
| `custom_sofa_ref_invalid` | external_reference | `generic` | `custom_sofa` reference fails bounded token validation. |
| `steam_unavailable` | runtime_capability | `generic` | Binaural profile path requested while Steam runtime unavailable. |
| `output_incompatible` | routing_capability | `generic` | Active output path cannot host requested headphone profile route. |
| `monitoring_path_bypassed` | runtime_resolution | `generic` | Requested monitoring/profile path downgraded by runtime resolver. |
| `catalog_version_mismatch` | contract_version | `generic` | Published profile diagnostics incompatible with expected catalog version. |

### Slice A1 Acceptance IDs

| Acceptance ID | Requirement | Primary Contract Surface | Required Artifact |
|---|---|---|---|
| `BL034-A1-AC-001` | Canonical profile catalog identity set is explicitly defined (`generic`, `airpods_pro_2`, `sony_wh1000xm5`, `custom_sofa`) | this runbook + `Documentation/scene-state-contract.md` | `profile_catalog_contract.md` |
| `BL034-A1-AC-002` | Deterministic fallback taxonomy codes and mandatory fallback targets are explicitly defined | this runbook + `Documentation/scene-state-contract.md` | `fallback_taxonomy.tsv` |
| `BL034-A1-AC-003` | Additive diagnostics publication requirements and bounded value domains/ranges are documented | `Documentation/scene-state-contract.md` | `acceptance_id_map.tsv` |
| `BL034-A1-AC-004` | Machine-checkable downstream artifact contract (file names + column schema) is published | this runbook (`Slice A1 Machine-Checkable Artifact Contract`) | `acceptance_id_map.tsv` |
| `BL034-A1-AC-005` | Documentation freshness gate passes for A1 change set | `./scripts/validate-docs-freshness.sh` | `docs_freshness.log` |

### Slice A1 Machine-Checkable Artifact Contract

All A1/downstream-lane profile-governance artifacts must be TSV or Markdown with deterministic row/field names.

| Artifact | Path Pattern | Required Schema |
|---|---|---|
| Profile catalog contract | `TestEvidence/bl034_slice_a1_profile_contract_<timestamp>/profile_catalog_contract.md` | Contains exactly one identity table with columns `profileId`, `class`, `source`, `requiresCustomSofaRef`, `boundedDomain`. |
| Fallback taxonomy matrix | `TestEvidence/bl034_slice_a1_profile_contract_<timestamp>/fallback_taxonomy.tsv` | Header: `reason_code`, `class`, `fallback_target`, `deterministic_trigger`, `compatibility`; one row per normative reason code. |
| Acceptance map | `TestEvidence/bl034_slice_a1_profile_contract_<timestamp>/acceptance_id_map.tsv` | Header: `acceptance_id`, `description`, `contract_source`, `artifact`, `status`. |
| Lane status | `TestEvidence/bl034_slice_a1_profile_contract_<timestamp>/status.tsv` | Header: `lane`, `status`, `detail`; must include `docs_freshness` and `overall`. |

## Slice D1 Release-Governance Linkage Contract

### Slice D1 Acceptance IDs

| Acceptance ID | Requirement | Primary Contract Surface | Required Artifact |
|---|---|---|---|
| `BL034-D1-001` | BL-034 release-readiness hooks are explicitly mapped to BL-030 checklist gates | this runbook (`Slice D1 Release Checklist Hook Mapping`) + `Documentation/testing/bl-034-headphone-verification-qa.md` | `acceptance_mapping.tsv` |
| `BL034-D1-002` | Required BL-034 evidence artifacts are mapped to gate-specific pass criteria | this runbook + QA doc + `Documentation/implementation-traceability.md` | `acceptance_mapping.tsv` |
| `BL034-D1-003` | Deterministic pass/fail taxonomy for release-readiness review is explicitly defined | this runbook (`Slice D1 Deterministic Release-Readiness Taxonomy`) + QA doc | `failure_taxonomy.tsv` |
| `BL034-D1-004` | Cross-document parity across runbook + QA + traceability is preserved for D1 IDs | this runbook + QA doc + traceability | `release_linkage_contract.md` |
| `BL034-D1-005` | Documentation freshness gate passes for D1 linkage packet | `./scripts/validate-docs-freshness.sh` | `docs_freshness.log` |

### Slice D1 Release Checklist Hook Mapping

| BL-030 Gate Hook | BL-034 Linkage Purpose | Required BL-034 Evidence Artifact | D1 Pass Criteria |
|---|---|---|---|
| `RL-05` Device rerun matrix | Verify headphone profile/fallback/verification artifacts are present for release-readiness review | `TestEvidence/bl034_slice_c1_qa_lane_<timestamp>/status.tsv`, `validation_matrix.tsv`, `replay_hashes.tsv`, `failure_taxonomy.tsv` | All required BL-034 C1 artifacts exist and report no deterministic divergence. |
| `RL-08` Docs freshness | Ensure BL-034 linkage docs and evidence metadata satisfy freshness policy | `TestEvidence/bl034_slice_d1_release_linkage_<timestamp>/docs_freshness.log` | `./scripts/validate-docs-freshness.sh` exits `0`. |
| `RL-09` Release-note evidence pointer check | Require canonical BL-034 release-readiness evidence pointers for downstream release notes | `TestEvidence/bl034_slice_d1_release_linkage_<timestamp>/release_linkage_contract.md` | Contract doc includes gate hooks, required artifacts, and acceptance IDs. |
| `RL-10` Packaging evidence manifest alignment | Ensure release packet has deterministic artifact list for BL-034 headphone readiness claims | `TestEvidence/bl034_slice_d1_release_linkage_<timestamp>/acceptance_mapping.tsv` | Mapping table includes gate hook, artifact path pattern, and review policy for each D1 acceptance. |

### Slice D1 Deterministic Release-Readiness Taxonomy

| Failure Class | Deterministic Category | Gate Impact | Trigger |
|---|---|---|---|
| `missing_release_hook_mapping` | contract_schema | `RL-05/RL-09/RL-10` | Required BL-030 gate hook row absent from D1 linkage mapping. |
| `missing_required_artifact_mapping` | artifact_schema | `RL-05/RL-10` | Required BL-034 evidence artifact is not mapped to a release hook. |
| `release_taxonomy_contract_mismatch` | taxonomy_schema | all linked gates | D1 taxonomy in runbook/QA/traceability is inconsistent or incomplete. |
| `cross_doc_parity_failure` | cross_reference | all linked gates | D1 acceptance IDs not present across runbook + QA + traceability. |
| `docs_freshness_failure` | freshness_gate | `RL-08` | Docs freshness gate returns non-zero during D1 linkage review. |

## Agent Mega-Prompt

### Slice C — Skill-Aware Prompt

```
/test BL-034 Slice C: deterministic headphone verification lane set
Load: $skill_testing, $skill_test, $skill_docs, $skill_troubleshooting

Objective:
- Implement and validate deterministic QA lane coverage for headphone profile + verification contracts.

Constraints:
- No source-side DSP redesign in this slice.
- Lane outputs must be machine-readable and replayable.
- Failure taxonomy must classify oversights (profile missing, fallback mismatch, score drift).

Validation:
- cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8
- ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
- ./scripts/qa-bl009-headphone-contract-mac.sh
- ./scripts/validate-docs-freshness.sh

Evidence:
- TestEvidence/bl034_headphone_verification_<timestamp>/
```

### Slice C — Standalone Fallback Prompt

```
You are implementing BL-034 Slice C for LocusQ.

TASK:
1) Define deterministic QA scenarios and scripts for headphone profile selection and verification score persistence.
2) Emit machine-readable results with failure taxonomy.
3) Ensure outputs are replayable and hash-stable for the same input set.

CONSTRAINTS:
- Keep schema additive and backward compatible.
- Do not introduce non-deterministic timestamps in hash-critical payloads.

VALIDATION:
- build, smoke suite, headphone contract lane, docs freshness.

EVIDENCE:
- status.tsv, replay_hashes.tsv, failure_taxonomy.tsv, diagnostics_snapshot.json.
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-034-build | Automated | `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8` | Exit 0 |
| BL-034-smoke | Automated | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | Suite pass |
| BL-034-hp-contract | Automated | `./scripts/qa-bl009-headphone-contract-mac.sh` | Exit 0 |
| BL-034-selftest | Automated | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` | Exit 0 |
| BL-034-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Verification metrics become subjective/non-repeatable | High | Med | Define bounded score semantics and deterministic playback seeds |
| Profile fallback reasons are ambiguous | Med | Med | Publish explicit reason codes + expected fallback target |
| QA lanes become flaky across runs | High | Med | Add replay hash and per-lane strict exit semantics |

## Failure & Rollback Paths

- If verification replay hashes drift: freeze lane inputs and rebaseline only with explicit owner sign-off.
- If profile fallback mismatches occur: force conservative fallback profile and mark lane fail with reason code.
- If selftest instability appears: classify by taxonomy and isolate script/runtime serialization before code changes.

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Lane status | `TestEvidence/bl034_headphone_verification_<timestamp>/status.tsv` | lane, exit, result |
| Profile matrix | `TestEvidence/bl034_headphone_verification_<timestamp>/per_profile_results.tsv` | profileId, requested, active, fallbackReason, result |
| Replay hashes | `TestEvidence/bl034_headphone_verification_<timestamp>/replay_hashes.tsv` | scenario, run, hash |
| Failure taxonomy | `TestEvidence/bl034_headphone_verification_<timestamp>/failure_taxonomy.tsv` | class, count, note |

## Closeout Checklist

- [x] BL-033 dependency conditions satisfied
- [x] Profile catalog + fallback taxonomy contract landed (Slice A1)
- [x] Verification score persistence contract validated (Slice B1 telemetry + diagnostics snapshot)
- [x] QA lanes deterministic with replay evidence (Slice C1 + Owner Z1 lane replay)
- [x] Release-governance linkage evidence documented (Slice D1 linkage contract + deterministic taxonomy)
- [x] `Documentation/backlog/index.md` synchronized
- [x] `./scripts/validate-docs-freshness.sh` passes
- [x] Owner replay RT gate green on current branch (`non_allowlisted=0`; Owner Z3)
- [x] Extended determinism/RT/parity confidence packets integrated (E1/E2/E3)
- [x] Promotion posture advanced to `Done-candidate` (Owner Z5)
- [x] Final BL-034 done-promotion packet recorded (Owner Z6 -> `Done`)
- [x] Post-done confidence packets integrated (F/F1/F2) and owner replay reconfirmed (Z7)

## Owner Integration Snapshot (Z5)

Date: `2026-02-26`

Validation replay bundle:
- `TestEvidence/bl034_owner_sync_z5_20260226T041435Z/status.tsv`
- `TestEvidence/bl034_owner_sync_z5_20260226T041435Z/validation_matrix.tsv`
- `TestEvidence/bl034_owner_sync_z5_20260226T041435Z/rt_audit.tsv`

Replay results:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` -> PASS
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> PASS
3. `./scripts/qa-bl009-headphone-contract-mac.sh` -> PASS
4. `./scripts/qa-bl034-headphone-verification-lane-mac.sh --execute-suite --runs 5` -> PASS
5. `./scripts/rt-safety-audit.sh --print-summary` -> PASS (`non_allowlisted=0`)
6. `jq empty status.json` -> PASS
7. `./scripts/validate-docs-freshness.sh` -> PASS

Disposition:
- BL-034 advances from `In Validation` to `Done-candidate`.
- E1/E2/E3 packets provide additional deterministic soak, RT sentinel, and contract parity confidence on top of Z3/Z4/D2 closure.

## Owner Integration Snapshot (Z6 Final Done Promotion)

Date: `2026-02-26`

Validation replay bundle:
- `TestEvidence/bl034_done_promotion_z6_20260226T041946Z/status.tsv`
- `TestEvidence/bl034_done_promotion_z6_20260226T041946Z/validation_matrix.tsv`
- `TestEvidence/bl034_done_promotion_z6_20260226T041946Z/rt_audit.tsv`
- `TestEvidence/bl034_done_promotion_z6_20260226T041946Z/lane_runs/validation_matrix.tsv`

Replay results:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` -> PASS
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> PASS
3. `./scripts/qa-bl009-headphone-contract-mac.sh` -> PASS
4. `./scripts/qa-bl034-headphone-verification-lane-mac.sh --execute-suite --runs 5` -> PASS
5. `./scripts/rt-safety-audit.sh --print-summary` -> PASS (`non_allowlisted=0`)
6. `jq empty status.json` -> PASS
7. `./scripts/validate-docs-freshness.sh` -> PASS

Disposition:
- BL-034 advances from `Done-candidate` to `Done`.
- Promotion decision and handoff resolution are recorded in `TestEvidence/bl034_done_promotion_z6_20260226T041946Z/promotion_decision.md` and `.../handoff_resolution.md`.

## Owner Integration Snapshot (Z7 Post-Done Confidence Sync)

Date: `2026-02-26`

Validation replay bundle:
- `TestEvidence/bl034_owner_sync_z7_20260226T042832Z/status.tsv`
- `TestEvidence/bl034_owner_sync_z7_20260226T042832Z/validation_matrix.tsv`
- `TestEvidence/bl034_owner_sync_z7_20260226T042832Z/rt_audit.tsv`
- `TestEvidence/bl034_owner_sync_z7_20260226T042832Z/lane_runs/validation_matrix.tsv`

Resolved input handoffs:
- `TestEvidence/bl034_slice_f_ui_diag_20260226T042553Z/status.tsv`
- `TestEvidence/bl034_cross_lane_stress_f1_20260226T041919Z/status.tsv`
- `TestEvidence/bl034_rt_drift_watch_f2_20260226T041934Z/status.tsv`

Replay results:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` -> PASS
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> PASS
3. `./scripts/qa-bl009-headphone-contract-mac.sh` -> PASS
4. `./scripts/qa-bl034-headphone-verification-lane-mac.sh --execute-suite --runs 5` -> PASS
5. `./scripts/rt-safety-audit.sh --print-summary` -> PASS (`non_allowlisted=0`)
6. `jq empty status.json` -> PASS
7. `./scripts/validate-docs-freshness.sh` -> PASS

Disposition:
- BL-034 remains `Done`; no regression detected.
- Post-done confidence evidence is recorded in `TestEvidence/bl034_owner_sync_z7_20260226T042832Z/owner_decisions.md` and `.../handoff_resolution.md`.


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-034-headphone-calibration-verification.md` |

### Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | Legacy closeout documented | `Evidence References` |
| Lane replay/parity | Legacy closeout documented | `Evidence References` |
| RT safety | Legacy closeout documented | `Evidence References` |
| Docs freshness | Legacy closeout documented | `Evidence References` |
| Status schema | Legacy closeout documented | `Evidence References` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | Required for modern promotions; legacy packets may predate marker | `Evidence References` |

### Backlog/Status Sync Checklist

- [x] Runbook archived under `Documentation/backlog/done/`
- [x] Backlog index links the done runbook
- [x] Historical evidence references retained
- [ ] Legacy packet retrofitted to modern owner packet template (`_template-promotion-decision.md`) where needed
- [ ] Legacy closeout fully normalized to modern checklist fields where needed
