Title: BL-033 Headphone Calibration Core Path
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-26

# BL-033: Headphone Calibration Core Path

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | Done (Owner Z12 final replay PASS; promotion packet finalized) |
| Owner Track | Track A — Runtime Formats |
| Depends On | BL-009, BL-017, BL-026, BL-028 |
| Blocks | BL-034 |
| Annex Spec | `Documentation/plans/bl-033-headphone-calibration-core-spec-2026-02-25.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Headphone profile/state contract and migration wiring (`hp_*` parameters + blob refs) |
| B | High | L | Steam virtual surround monitoring path integration with requested/active diagnostics |
| C | High | L | PEQ/FIR post-chain integration with deterministic engine selection and latency reporting |
| D | Med | M | QA lane scaffold/docs contract (D1) followed by full execution + RT safety closeout (D2) |

## Current Slice Disposition (Owner Sync Z12)

| Slice | Worker Result | Owner Replay Result | Evidence |
|---|---|---|---|
| A1 | FAIL | Recovered in owner replay (`build` + `BL-009` pass) | `TestEvidence/bl033_slice_a1_processor_contract_20260225T232640Z/status.tsv`, `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/status.tsv` |
| B1 | FAIL | Recovered in owner replay (`BL-009` + RT gate pass) | `TestEvidence/bl033_slice_b1_renderer_chain_20260225T232819Z/status.tsv`, `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/status.tsv` |
| D1 | PASS | PASS (`execute-suite` x3 deterministic pass with `--runs`) | `TestEvidence/bl033_slice_d1_qa_contract_20260225T232722Z/status.tsv`, `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/lane_runs/validation_matrix.tsv` |
| A2/B2 | FAIL | Reconciled by Z9+Z11 owner replay (`build/smoke/BL-009` pass; RT drift closed) | `TestEvidence/bl033_slice_a2b2_native_contract_20260226T005521Z/status.tsv`, `TestEvidence/bl033_rt_gate_z9_20260226T010610Z/rt_after.tsv`, `TestEvidence/bl033_owner_sync_z11_20260225_200647/status.tsv` |
| C2 | FAIL | Reconciled by Z9+Z11 owner replay (`build/smoke/BL-009` pass; RT drift closed) | `TestEvidence/bl033_slice_c2_dsp_latency_20260226T005538Z/status.tsv`, `TestEvidence/bl033_rt_gate_z9_20260226T010610Z/rt_after.tsv`, `TestEvidence/bl033_owner_sync_z11_20260225_200647/status.tsv` |
| D2 | FAIL | Reconciled by Z10+Z11 owner replay (`--runs 5` deterministic + docs freshness pass) | `TestEvidence/bl033_slice_d2_qa_closeout_20260226T010105Z/status.tsv`, `TestEvidence/bl033_evidence_hygiene_z10_20260226T010548Z/status.tsv`, `TestEvidence/bl033_owner_sync_z11_20260225_200647/lane_runs/validation_matrix.tsv` |
| Z2 | PASS | PASS (`non_allowlisted=0`) | `TestEvidence/bl033_rt_gate_z2_20260226T003240Z/status.tsv` |
| Z5 | PASS | PASS (docs freshness blocker repaired) | `TestEvidence/bl033_root_docs_z5_20260226T004444Z/status.tsv` |
| Z6 | PASS | PASS (lane hardening + compatibility verified) | `TestEvidence/bl033_lane_hardening_z6_20260226T004506Z/status.tsv` |
| Z7 | PASS | PASS (full replay audit green) | `TestEvidence/bl033_replay_audit_z7_20260226T004641Z/status.tsv` |
| Z9 | PASS | PASS (`rt_before=1` -> `rt_after=0`; non-allowlisted drift closed) | `TestEvidence/bl033_rt_gate_z9_20260226T010610Z/rt_before.tsv`, `TestEvidence/bl033_rt_gate_z9_20260226T010610Z/rt_after.tsv` |
| Z10 | PASS | PASS (evidence metadata hygiene + docs freshness pass) | `TestEvidence/bl033_evidence_hygiene_z10_20260226T010548Z/status.tsv` |
| Z11 | PASS | PASS (owner integration replay green across all required gates) | `TestEvidence/bl033_owner_sync_z11_20260225_200647/status.tsv`, `TestEvidence/bl033_owner_sync_z11_20260225_200647/validation_matrix.tsv` |
| Z12 | PASS | PASS (owner final replay green; Done promotion packet finalized) | `TestEvidence/bl033_done_promotion_z12_20260226T011520Z/status.tsv`, `TestEvidence/bl033_done_promotion_z12_20260226T011520Z/validation_matrix.tsv` |

Owner decision packet:
- `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/owner_decisions.md`
- `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/handoff_resolution.md`
- `TestEvidence/bl033_owner_sync_z11_20260225_200647/owner_decisions.md`
- `TestEvidence/bl033_owner_sync_z11_20260225_200647/handoff_resolution.md`
- `TestEvidence/bl033_done_promotion_z12_20260226T011520Z/promotion_decision.md`
- `TestEvidence/bl033_done_promotion_z12_20260226T011520Z/handoff_resolution.md`

## Objective

Implement an RT-safe, deterministic internal headphone calibration monitoring path for LocusQ using Steam binaural rendering plus optional EQ/FIR compensation, while preserving existing monitoring modes and publishing explicit diagnostics (`requested`, `active`, `stage`, fallback reason) for CALIBRATE and scene-state consumers.

## Scope & Non-Scope

**In scope:**
- Add/solidify headphone calibration contract for APVTS + state payload (`hp_*` family, SOFA/FIR references)
- Integrate `steam_binaural` chain ordering and fallback semantics
- Integrate PEQ/FIR post-binaural compensation stages with explicit latency handling
- Publish diagnostics parity across requested vs active monitoring path/profile
- Add deterministic QA and RT-safety evidence requirements

**Out of scope:**
- Full personalized HRTF generation pipeline
- New UI redesign beyond necessary diagnostics/controls binding
- Replacing internal renderer architecture with full ambisonic-first rewrite
- Device-specific OS binaural stack integration inside plugin process

## Architecture Context

- Research origin:
  - `Documentation/research/LocusQ Headphone Calibration Research Outline.md`
  - `Documentation/research/Headphone Calibration for 3D Audio.pdf`
- Invariants: `Documentation/invariants.md` (RT safety, deterministic scene-state publication)
- Device/monitoring contract: `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- Scene diagnostics contract: `Documentation/scene-state-contract.md`
- Existing headphone baseline: BL-009 closeout in `Documentation/backlog/done/bl-009-steam-headphone-contract.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Headphone profile/state contract and migration | `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, state migration docs/contracts | BL-009/BL-026 complete | deterministic `hp_*` contract landed with migration + idempotence checks |
| B | Steam virtual surround monitoring path integration | `Source/SpatialRenderer.h`, `Source/PluginProcessor.cpp`, diagnostics contracts | Slice A complete | `steam_binaural` path active with explicit requested/active/fallback diagnostics |
| C | PEQ/FIR post-chain + latency contract | DSP modules + `Source/PluginProcessor.cpp` + QA assertions | Slice B complete | PEQ/FIR chain deterministic, no RT safety regressions, latency reporting verified |
| D | QA lane + evidence hardening | `qa/scenarios/*`, `scripts/qa-*.sh`, docs/evidence contracts | Slice C complete | D1 contract lane IDs/artifacts stable; D2 suite/RT lanes pass with machine-readable evidence |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl BL-033 Slice A: Headphone profile/state contract and migration
Load: $skill_impl, $steam-audio-capi, $spatial-audio-engineering, $skill_docs

Objective:
- Add deterministic headphone calibration parameter/state contract (`hp_*`) with migration safety.

Constraints:
- No allocations/locks/blocking in processBlock.
- Preserve backward compatibility for existing state payloads.
- Publish additive diagnostics only.

Validation:
- cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
- ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
- ./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl033_slice_a_<timestamp>/rt_audit.tsv
- ./scripts/validate-docs-freshness.sh

Evidence:
- TestEvidence/bl033_slice_a_<timestamp>/
```

### Slice A — Standalone Fallback Prompt

```
You are implementing BL-033 Slice A in LocusQ.

CONTEXT:
- JUCE plugin with CALIBRATE/EMITTER/RENDERER modes.
- Existing headphone baseline from BL-009.
- New contract must be additive and migration-safe.

TASK:
1) Add headphone calibration parameter/state contract (`hp_*`) for monitoring profile, HRTF mode, EQ mode, FIR metadata.
2) Add migration path for existing state snapshots.
3) Publish requested/active diagnostics fields without breaking existing scene-state consumers.

CONSTRAINTS:
- RT-safe processBlock only (no allocations/locks/I/O).
- Maintain deterministic serialization order.

VALIDATION:
- build + smoke suite + RT safety audit + docs freshness.

EVIDENCE:
- TestEvidence/bl033_slice_a_<timestamp>/status.tsv and diagnostic snapshot.
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-033-D1-contract | Automated | `./scripts/qa-bl033-headphone-core-lane-mac.sh --contract-only --out-dir <out>` | Exit 0; `status.tsv` contains `BL033-D1-001..006` checks |
| BL-033-D1-suite | Automated | `./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --out-dir <out>` | Exit 0; `scenario_status=PASS`; `lane_result=PASS` |
| BL-033-build | Automated | `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` | Exit 0 |
| BL-033-smoke | Automated | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | Suite pass |
| BL-033-hp-contract | Automated | `./scripts/qa-bl009-headphone-contract-mac.sh` | Exit 0; no contract regressions |
| BL-033-rt | Automated | `./scripts/rt-safety-audit.sh --print-summary --output <out>/rt_audit.tsv` | `non_allowlisted=0` |
| BL-033-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Slice D1 Acceptance ID Map

| Acceptance ID | Lane Check | Artifact |
|---|---|---|
| BL033-D1-001 | `BL033-D1-001_contract_schema` | `status.tsv` |
| BL033-D1-002 | `BL033-D1-002_diagnostics_fields` | `status.tsv` |
| BL033-D1-003 | `BL033-D1-003_artifact_schema` | `status.tsv` |
| BL033-D1-004 | `BL033-D1-004_acceptance_parity` | `acceptance_parity.tsv` |
| BL033-D1-005 | `BL033-D1-005_lane_thresholds` | `status.tsv` |
| BL033-D1-006 | `BL033-D1-006_execution_mode` | `status.tsv` |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Monitoring mode routing regressions | High | Med | Preserve explicit requested/active diagnostics and add per-mode assertions |
| RT regressions from FIR/HRTF swaps | High | Med | Enforce non-RT build/swap path with atomic pointer handoff |
| Latency drift across modes | Med | Med | Add explicit latency assertions for bypass/direct/partitioned branches |

## Failure & Rollback Paths

- If headphone contract lane fails: freeze new `hp_*` fields behind fallback defaults and re-run BL-009 lane.
- If RT safety audit fails: isolate offending lines, apply minimal allowlist only for intentional non-RT context.
- If latency mismatch appears: force known-safe bypass profile and keep diagnostics reporting explicit degraded mode.

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Lane status | `TestEvidence/bl033_headphone_core_<timestamp>/status.tsv` | lane, exit, result, notes |
| Lane execution log | `TestEvidence/bl033_headphone_core_<timestamp>/qa_lane.log` | ordered check output, command context |
| Scenario contract log | `TestEvidence/bl033_headphone_core_<timestamp>/scenario_contract.log` | schema checks, thresholds, diagnostics fields |
| Scenario result ledger | `TestEvidence/bl033_headphone_core_<timestamp>/scenario_result.log` | scenario_id, result_status, warnings, result_json |
| Acceptance parity table | `TestEvidence/bl033_headphone_core_<timestamp>/acceptance_parity.tsv` | acceptance_id, cross-doc counts, mapped check, result |
| Taxonomy table | `TestEvidence/bl033_headphone_core_<timestamp>/taxonomy_table.tsv` | failure_class, count, detail |
| Diagnostics snapshot | `TestEvidence/bl033_headphone_core_<timestamp>/diagnostics_snapshot.json` | requested, active, stage, fallbackReason |
| RT audit report | `TestEvidence/bl033_headphone_core_<timestamp>/rt_audit.tsv` | non_allowlisted, rule hits |
| Validation trend row | `TestEvidence/validation-trend.md` | date, lane, result, BL ID |

## Closeout Checklist

- [x] All slices (A-D) implemented and validated
- [x] Monitoring mode contract (`speakers`, `steam_binaural`, `virtual_binaural`) verified
- [x] Latency reporting verified for FIR mode transitions
- [x] Slice D1 acceptance IDs (`BL033-D1-001..006`) pass parity checks
- [x] RT safety lane is green with explicit evidence
- [x] `Documentation/backlog/index.md` and runbook status are synchronized
- [x] `./scripts/validate-docs-freshness.sh` passes
- [x] Owner Z12 final replay and promotion packet complete; BL-033 promoted to `Done`.

## Owner Integration Snapshot (Z1)

Date: `2026-02-26`

Validation replay bundle:
- `TestEvidence/bl033_owner_sync_z1_20260226T000200Z/status.tsv`
- `TestEvidence/bl033_owner_sync_z1_20260226T000200Z/validation_matrix.tsv`
- `TestEvidence/bl033_owner_sync_z1_20260226T000200Z/rt_audit.tsv`

Replay results:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` -> PASS
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> PASS
3. `qa-bl033-headphone-core-lane --execute-suite` (x3) -> PASS/PASS/PASS
4. `./scripts/qa-bl009-headphone-contract-mac.sh` -> PASS
5. `./scripts/rt-safety-audit.sh --print-summary` -> FAIL (`non_allowlisted=94`)
6. `./scripts/validate-docs-freshness.sh` -> FAIL (prior-worker evidence metadata omission)

Disposition:
- Blocked until RT allowlist reconciliation and docs freshness debt are closed on current branch.

## Owner Integration Snapshot (Z8)

Date: `2026-02-26`

Validation replay bundle:
- `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/status.tsv`
- `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/validation_matrix.tsv`
- `TestEvidence/bl033_owner_sync_z8_20260226T004911Z/rt_audit.tsv`

Replay results:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` -> PASS
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> PASS
3. `./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --runs 3` -> PASS (3/3)
4. `./scripts/qa-bl009-headphone-contract-mac.sh` -> PASS
5. `./scripts/rt-safety-audit.sh --print-summary` -> PASS (`non_allowlisted=0`)
6. `jq empty status.json` -> PASS
7. `./scripts/validate-docs-freshness.sh` -> PASS

Disposition:
- Prior Z1 blockers are closed.
- BL-033 state advanced to `In Validation`.

## Owner Integration Snapshot (Z11)

Date: `2026-02-26`

Validation replay bundle:
- `TestEvidence/bl033_owner_sync_z11_20260225_200647/status.tsv`
- `TestEvidence/bl033_owner_sync_z11_20260225_200647/validation_matrix.tsv`
- `TestEvidence/bl033_owner_sync_z11_20260225_200647/rt_audit.tsv`

Replay results:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` -> PASS
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> PASS
3. `./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --runs 5` -> PASS
4. `./scripts/qa-bl009-headphone-contract-mac.sh` -> PASS
5. `./scripts/rt-safety-audit.sh --print-summary` -> PASS (`non_allowlisted=0`)
6. `jq empty status.json` -> PASS
7. `./scripts/validate-docs-freshness.sh` -> PASS

Disposition:
- Z9 RT reconciliation and Z10 evidence hygiene are both confirmed on current branch.
- BL-033 promotion posture is advanced to `Done-candidate`.

## Owner Final Promotion Snapshot (Z12)

Date: `2026-02-26`

Validation replay bundle:
- `TestEvidence/bl033_done_promotion_z12_20260226T011520Z/status.tsv`
- `TestEvidence/bl033_done_promotion_z12_20260226T011520Z/validation_matrix.tsv`
- `TestEvidence/bl033_done_promotion_z12_20260226T011520Z/rt_audit.tsv`

Replay results:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` -> PASS
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` -> PASS
3. `./scripts/qa-bl033-headphone-core-lane-mac.sh --execute-suite --runs 5` -> PASS (deterministic signatures stable across all runs)
4. `./scripts/qa-bl009-headphone-contract-mac.sh` -> PASS
5. `./scripts/rt-safety-audit.sh --print-summary` -> PASS (`non_allowlisted=0`)
6. `jq empty status.json` -> PASS
7. `./scripts/validate-docs-freshness.sh` -> PASS

Disposition:
- BL-033 promotion decision is finalized to `Done`.
- BL-034 dependency gate on BL-033 is now fully satisfied on owner-authoritative evidence.
